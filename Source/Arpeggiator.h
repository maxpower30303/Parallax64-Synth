// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <algorithm>

class Arpeggiator
{
public:
    Arpeggiator() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        reset();
    }

    void reset()
    {
        heldNotes.clear();
        heldNotes.reserve(128);
        notePool.reserve(128 * 3);
        currentStep = 0;
        sampleCounter = 0;
        lastArpNote = -1;
    }

    void process(juce::MidiBuffer& midiMessages,
                 int numSamples,
                 bool enabled,
                 int mode,        // 0: Up, 1: Down, 2: Up/Down, 3: Random
                 float rateParam, // 1 to 16
                 float gateParam, // 0.1 to 0.95
                 int octaves)     // 1 to 3
    {
        if (!enabled)
        {
            if (lastArpNote >= 0)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastArpNote, 0.0f), 0);
                lastArpNote = -1;
            }
            heldNotes.clear();
            return;
        }

        // Collect new NoteOns and NoteOffs
        incomingNotes.clear();
        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                int note = msg.getNoteNumber();
                if (std::find(heldNotes.begin(), heldNotes.end(), note) == heldNotes.end())
                {
                    heldNotes.push_back(note);
                    std::sort(heldNotes.begin(), heldNotes.end());
                }
            }
            else if (msg.isNoteOff())
            {
                int note = msg.getNoteNumber();
                auto it = std::find(heldNotes.begin(), heldNotes.end(), note);
                if (it != heldNotes.end())
                    heldNotes.erase(it);
            }
            else
            {
                incomingNotes.addEvent(msg, metadata.samplePosition);
            }
        }

        midiMessages.clear();
        // Pass through non-note messages
        midiMessages.addEvents(incomingNotes, 0, numSamples, 0);

        if (heldNotes.empty())
        {
            if (lastArpNote >= 0)
            {
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastArpNote, 0.0f), 0);
                lastArpNote = -1;
            }
            currentStep = 0;
            sampleCounter = 0;
            return;
        }

        // Build note pool based on octaves (1, 2, or 3)
        notePool.clear();
        int clampedOctaves = juce::jlimit(1, 3, octaves);
        for (int oct = 0; oct < clampedOctaves; ++oct)
        {
            for (int n : heldNotes)
            {
                int noteWithOct = n + oct * 12;
                if (noteWithOct <= 127)
                    notePool.push_back(noteWithOct);
            }
        }

        if (notePool.empty())
            return;

        // Calculate step length in samples (approx 120 bpm, rate from 1 to 16)
        float speedHz = juce::jlimit(1.0f, 20.0f, rateParam * 1.5f);
        int stepSamples = static_cast<int>(currentSampleRate / speedHz);
        if (stepSamples < 128) stepSamples = 128;
        int gateSamples = static_cast<int>(stepSamples * juce::jlimit(0.1f, 0.95f, gateParam));

        for (int sample = 0; sample < numSamples; ++sample)
        {
            if (sampleCounter == 0)
            {
                // Send Note Off for previous note
                if (lastArpNote >= 0)
                {
                    midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastArpNote, 0.0f), sample);
                    lastArpNote = -1;
                }

                // Choose target note based on mode
                int targetNote = notePool[0];
                int poolSize = static_cast<int>(notePool.size());

                switch (mode)
                {
                    case 0: // Up
                        targetNote = notePool[currentStep % poolSize];
                        break;
                    case 1: // Down
                        targetNote = notePool[poolSize - 1 - (currentStep % poolSize)];
                        break;
                    case 2: // Up/Down
                    {
                        if (poolSize <= 1)
                        {
                            targetNote = notePool[0];
                        }
                        else
                        {
                            int cycleLen = 2 * (poolSize - 1);
                            int modIdx = currentStep % cycleLen;
                            int mappedIdx = (modIdx < poolSize) ? modIdx : (cycleLen - modIdx);
                            targetNote = notePool[mappedIdx];
                        }
                        break;
                    }
                    case 3: // Random
                    default:
                        targetNote = notePool[static_cast<size_t>(rng.nextInt(poolSize))];
                        break;
                }

                // Send Note On
                midiMessages.addEvent(juce::MidiMessage::noteOn(1, targetNote, 0.85f), sample);
                lastArpNote = targetNote;
                currentStep++;
            }
            else if (sampleCounter == gateSamples && lastArpNote >= 0)
            {
                // Send Note Off at gate threshold
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, lastArpNote, 0.0f), sample);
                lastArpNote = -1;
            }

            sampleCounter++;
            if (sampleCounter >= stepSamples)
                sampleCounter = 0;
        }
    }

private:
    double currentSampleRate = 44100.0;
    std::vector<int> heldNotes;
    // scratch containers kept as members so processing never allocates in steady state
    juce::MidiBuffer incomingNotes;
    std::vector<int> notePool;
    int currentStep = 0;
    int sampleCounter = 0;
    int lastArpNote = -1;
    juce::Random rng;
};
