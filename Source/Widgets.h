// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "MidiLearn.h"

// ==============================================================================
// 1. Oscilloscope / Real-Time Waveform Display
// ==============================================================================
class OscilloscopeComponent : public juce::Component, private juce::Timer
{
public:
    OscilloscopeComponent();
    ~OscilloscopeComponent() override;

    void setSnapshotData(const std::vector<float>& data);
    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    std::vector<float> displayBuffer;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeComponent)
};

// ==============================================================================
// 2. Interactive / Visual ADSR Envelope Graph
// ==============================================================================
class EnvelopeVisualizerComponent : public juce::Component
{
public:
    explicit EnvelopeVisualizerComponent(const juce::Colour& lineColour = juce::Colour(0xff00e5ff));
    void setParameters(float attack, float decay, float sustain, float release);
    void paint(juce::Graphics& g) override;

private:
    float a = 0.05f, d = 0.3f, s = 0.7f, r = 0.5f;
    juce::Colour traceColour;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeVisualizerComponent)
};

// ==============================================================================
// 3. 16 Polyphonic Voice Activity LEDs
// ==============================================================================
class VoiceLedsComponent : public juce::Component
{
public:
    VoiceLedsComponent();
    void setNumActive(int count);
    void paint(juce::Graphics& g) override;

private:
    int activeVoices = 1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceLedsComponent)
};

// ==============================================================================
// 4. Section Card / Panel with Cyber Framing
// ==============================================================================
class SectionPanel : public juce::Component
{
public:
    explicit SectionPanel(const juce::String& title, const juce::String& badge = "");
    void paint(juce::Graphics& g) override;

private:
    juce::String sectionTitle;
    juce::String sectionBadge;
};

// ==============================================================================
// 5. Labeled Cyber Rotary Knob (Supports Standard & Hero size, with MIDI Learn)
// ==============================================================================
class LabeledRotary : public juce::Component
{
public:
    LabeledRotary(const juce::String& labelText,
                  juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramId,
                  bool isHero = false,
                  const juce::Colour& accent = juce::Colour(0xff00e5ff),
                  MidiLearnManager* midiLearn = nullptr);

    void setMidiLearnManager(MidiLearnManager* ml) { midiLearnManager = ml; }
    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void showMidiLearnMenu();

    juce::Slider& getSlider() { return slider; }
    const juce::String& getParamId() const { return parameterId; }

private:
    juce::Slider slider;
    juce::Label label;
    bool isHeroKnob = false;
    juce::String labelTextStr;
    juce::String parameterId;
    juce::Colour accentColour;
    MidiLearnManager* midiLearnManager = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabeledRotary)
};

// ==============================================================================
// 6. Cyber Mode / Waveform Button Selector Bar (with Stepped MIDI Controller Learn)
// ==============================================================================
class ModeSelectorBar : public juce::Component
{
public:
    ModeSelectorBar(juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& paramId,
                    const juce::StringArray& optionLabels,
                    MidiLearnManager* midiLearn = nullptr);

    void setMidiLearnManager(MidiLearnManager* ml) { midiLearnManager = ml; }
    void resized() override;
    void updateSelection();
    void mouseDown(const juce::MouseEvent& e) override;
    void showMidiLearnMenu();

private:
    juce::RangedAudioParameter* param = nullptr;
    juce::String parameterId;
    juce::OwnedArray<juce::TextButton> buttons;
    MidiLearnManager* midiLearnManager = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModeSelectorBar)
};

// ==============================================================================
// 7. Labeled Cyber Linear Fader (for ADSR Envelopes, with MIDI Learn)
// ==============================================================================
class LabeledFader : public juce::Component
{
public:
    LabeledFader(const juce::String& labelText,
                 juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& paramId,
                 const juce::Colour& accent = juce::Colour(0xff00e5ff),
                 MidiLearnManager* midiLearn = nullptr);

    void setMidiLearnManager(MidiLearnManager* ml) { midiLearnManager = ml; }
    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void showMidiLearnMenu();

    juce::Slider& getSlider() { return slider; }
    const juce::String& getParamId() const { return parameterId; }

private:
    juce::Slider slider;
    juce::Label label;
    juce::String labelTextStr;
    juce::String parameterId;
    juce::Colour accentColour;
    MidiLearnManager* midiLearnManager = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabeledFader)
};

