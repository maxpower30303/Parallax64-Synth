// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <algorithm>

// Rewrites incoming note on/off events into monophonic last-note-priority
// behaviour: only one note is ever sounding. Releasing the currently
// sounding note falls back to the most recently held note instead of
// going silent, matching classic mono synth behaviour.
class MonoStack
{
public:
    void reset()
    {
        heldNotes.clear();
        currentNote = -1;
    }

    void process(juce::MidiBuffer& midiMessages)
    {
        juce::MidiBuffer output;

        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            int samplePos = metadata.samplePosition;

            if (msg.isNoteOn())
            {
                int note = msg.getNoteNumber();
                int channel = msg.getChannel();

                heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
                                                [note](const HeldNote& h) { return h.note == note; }),
                                 heldNotes.end());
                heldNotes.push_back({ note, msg.getVelocity() });

                if (currentNote >= 0 && currentNote != note)
                    output.addEvent(juce::MidiMessage::noteOff(channel, currentNote, 0.0f), samplePos);

                output.addEvent(msg, samplePos);
                currentNote = note;
            }
            else if (msg.isNoteOff())
            {
                int note = msg.getNoteNumber();
                int channel = msg.getChannel();

                heldNotes.erase(std::remove_if(heldNotes.begin(), heldNotes.end(),
                                                [note](const HeldNote& h) { return h.note == note; }),
                                 heldNotes.end());

                if (note == currentNote)
                {
                    if (!heldNotes.empty())
                    {
                        const HeldNote fallback = heldNotes.back();
                        output.addEvent(juce::MidiMessage::noteOff(channel, note, 0.0f), samplePos);
                        output.addEvent(juce::MidiMessage::noteOn(channel, fallback.note,
                                                                   static_cast<juce::uint8>(fallback.velocity)),
                                         samplePos);
                        currentNote = fallback.note;
                    }
                    else
                    {
                        output.addEvent(msg, samplePos);
                        currentNote = -1;
                    }
                }
                // Releasing a note that isn't currently sounding: it was already
                // superseded by a later note-on, so just drop the stray note-off.
            }
            else
            {
                output.addEvent(msg, samplePos);
            }
        }

        midiMessages.swapWith(output);
    }

private:
    struct HeldNote { int note; juce::uint8 velocity; };
    std::vector<HeldNote> heldNotes;
    int currentNote = -1;
};
