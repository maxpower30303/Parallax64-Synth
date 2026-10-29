// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "SynthVoice.h"
#include "FX.h"
#include "MidiLearn.h"
#include "Arpeggiator.h"
#include "MonoStack.h"

class P64AudioProcessor : public juce::AudioProcessor
{
public:
    P64AudioProcessor();
    ~P64AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "P64 Synthesizer"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    MidiLearnManager& getMidiLearn() { return midiLearn; }
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    // Visualizer data access
    float getRMSLevel(int channel) const { return rmsLevels[channel]; }
    const std::vector<float>& getWaveformSnapshot() const { return waveformSnapshot; }

private:
    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState apvts;
    juce::Synthesiser synth;
    Arpeggiator arpeggiator;
    MonoStack monoStack;
    bool lastArpOn = false;
    bool lastMonoOn = false;
    static constexpr int NUM_VOICES = 8;

    // Effects
    FX::Distortion distortion;
    FX::StereoChorus chorus;
    FX::StereoDelay delay;
    FX::ReverbProcessor reverb;

    // Midi Learn
    MidiLearnManager midiLearn;

    // Metering & Visualizer
    float rmsLevels[2] = { 0.0f, 0.0f };
    std::vector<float> waveformSnapshot;
    size_t waveformWriteIndex = 0;

    int currentPresetIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(P64AudioProcessor)
};
