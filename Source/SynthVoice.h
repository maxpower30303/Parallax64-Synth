// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP.h"
#include "Parameters.h"

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice();

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void prepare(double sampleRate, int samplesPerBlock, int outputChannels);
    void updateParameters(juce::AudioProcessorValueTreeState& apvts);

private:
    double currentSampleRate = 44100.0;
    float currentVelocity = 0.8f;
    float targetFreq = 440.0f;
    float currentFreq = 440.0f;
    float glideRate = 0.0f;
    float pitchBendSemitones = 0.0f;
    float pitchBendRangeSemitones = 2.0f;

    // Oscillators
    DSP::BandlimitedOscillator osc1;
    DSP::BandlimitedOscillator osc2;
    DSP::BandlimitedOscillator subOsc;
    DSP::NoiseGenerator noiseGen;
    DSP::LowFrequencyOscillator lfo1;

    // Unison: detuned side copies of Osc 1 / Osc 2, panned L/R for width.
    // The center oscillators above stay as-is; these are only used when
    // unisonOn is set.
    DSP::BandlimitedOscillator osc1UniLo, osc1UniHi;
    DSP::BandlimitedOscillator osc2UniLo, osc2UniHi;
    bool unisonOn = false;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> filter;   // 12dB LP / HP / BP (stereo via channel 0/1)
    DSP::LadderFilter ladderFilter;                 // 24dB LP (ladder), left/center
    DSP::LadderFilter ladderFilterR;                // 24dB LP, right channel (for unison width)
    DSP::LadderFilter ladderFilterAgg;              // Ladder Aggressive, left/center
    DSP::LadderFilter ladderFilterAggR;             // Ladder Aggressive, right channel

    // Envelopes
    juce::ADSR ampAdsr;
    juce::ADSR filterAdsr;
    juce::ADSR::Parameters ampParams;
    juce::ADSR::Parameters filterParams;

    // Cached Synth Parameters
    int osc1Oct = 0, osc1Semi = 0;
    float osc1Fine = 0.0f, osc1Level = 0.7f, osc1PW = 0.5f;

    int osc2Oct = 0, osc2Semi = 7;
    float osc2Fine = 5.0f, osc2Level = 0.5f;
    bool osc2Sync = false;

    int subOct = -1;
    float subLevel = 0.4f;
    float noiseLevel = 0.0f;

    int filterType = 1;
    float baseCutoff = 2500.0f;
    float filterRes = 2.0f;
    float filterEnvAmt = 0.5f;
    float filterKeyTrack = 0.5f;

    float lfoDepth = 0.2f;
    int lfoDest = 0;
    float glideTime = 0.0f;

    juce::AudioBuffer<float> voiceBuffer;
};
