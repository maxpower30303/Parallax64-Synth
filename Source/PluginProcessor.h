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
    double getTailLengthSeconds() const override;

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

    // Name shown in the preset display: the factory preset, or the user preset that was loaded.
    // Kept in the processor (and saved with the session) so a reopened editor / reloaded
    // session shows the sound that is actually loaded.
    juce::String getPresetLabel() const;
    void setUserPresetLabel(const juce::String& name);
    bool isUserPreset() const { return currentPresetIndex < 0; }

private:
    struct ParamCache
    {
        std::atomic<float> *osc1Wave, *osc1Oct, *osc1Semi, *osc1Fine, *osc1Level, *osc1PW;
        std::atomic<float> *osc2Wave, *osc2Oct, *osc2Semi, *osc2Fine, *osc2Level, *osc2Sync;
        std::atomic<float> *subWave, *subOct, *subLevel, *noiseLevel;
        std::atomic<float> *filterType, *cutoff, *res, *envAmt, *keyTrack;
        std::atomic<float> *ampA, *ampD, *ampS, *ampR, *filtA, *filtD, *filtS, *filtR;
        std::atomic<float> *lfoWave, *lfoRate, *lfoDepth, *lfoDest;
        std::atomic<float> *glide, *bendRange, *velSens, *unison;
        std::atomic<float> *distDrive, *distMix;
        std::atomic<float> *chOn, *chRate, *chDepth, *chMix;
        std::atomic<float> *dlOn, *dlTime, *dlFb, *dlMix;
        std::atomic<float> *rvOn, *rvSize, *rvDamp, *rvMix;
        std::atomic<float> *master, *presetLevel;
        std::atomic<float> *arpOn, *arpMode, *arpRate, *arpGate, *arpOct, *voiceMode;
    };

    void fillVoiceParams(VoiceParams& vp) const;


    juce::MidiKeyboardState keyboardState;
    juce::AudioProcessorValueTreeState apvts;
    juce::Synthesiser synth;
    Arpeggiator arpeggiator;
    MonoStack monoStack;
    bool lastArpOn = false;
    bool lastMonoOn = false;
    float lastOutputGain = 1.0f;
    static constexpr int NUM_VOICES = 8;
    SynthVoice* voices[NUM_VOICES] = {};
    GlideState glideState;
    ParamCache pc {};

    // Effects
    FX::Distortion distortion;
    FX::StereoChorus chorus;
    FX::StereoDelay delay;
    FX::ReverbProcessor reverb;

    // Midi Learn
    MidiLearnManager midiLearn;

    int currentPresetIndex = 0;      // -1 while a user preset is loaded
    juce::String userPresetLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(P64AudioProcessor)
};
