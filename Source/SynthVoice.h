// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP.h"
#include "Parameters.h"

// Everything a voice needs from the parameter tree. The processor reads the tree once
// per block and hands the same snapshot to all voices.
struct VoiceParams
{
    int osc1Wave = 0, osc1Oct = 0, osc1Semi = 0;
    float osc1Fine = 0.0f, osc1Level = 0.7f, osc1PW = 0.5f;

    int osc2Wave = 0, osc2Oct = 0, osc2Semi = 7;
    float osc2Fine = 5.0f, osc2Level = 0.5f;
    bool osc2Sync = false;

    bool unison = false;

    int subWave = 0, subOct = -1;
    float subLevel = 0.4f, noiseLevel = 0.0f;

    int filterType = 1;
    float cutoff = 2500.0f, resonance = 2.0f, envAmount = 0.5f, keyTrack = 0.5f;

    float ampA = 0.01f, ampD = 0.4f, ampS = 0.7f, ampR = 0.4f;
    float filtA = 0.02f, filtD = 0.45f, filtS = 0.3f, filtR = 0.4f;

    int lfoWave = 3;
    float lfoRate = 2.0f, lfoDepth = 0.2f;
    int lfoDest = 0;

    float glideTime = 0.0f;
    float pitchBendRange = 2.0f;
    float velocitySens = 0.8f;
};

// Pitch the previous note ended on, shared by all voices, so glide always starts from the
// note that was played before (whichever voice that happened to be).
struct GlideState
{
    float lastFreq = 0.0f;
};

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    // Filter cutoff, LFO, pitch factors and levels are updated once per this many samples.
    static constexpr int kControlRate = 8;

    SynthVoice();

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void prepare(double sampleRate, int samplesPerBlock, int outputChannels);
    void applyParams(const VoiceParams& p);
    void setGlideState(GlideState* state) { glideState = state; }

private:
    float keytrackFactor();
    void updatePitchBend();

    double currentSampleRate = 44100.0;
    float levelCoef = 0.1f;

    float currentVelocity = 0.8f;
    float velocityGain = 0.8f;
    float velocitySens = 0.8f;

    float targetFreq = 440.0f;
    float currentFreq = 440.0f;
    float glideMulPerGroup = 1.0f;
    bool glideActive = false;
    float glideTime = 0.0f;
    GlideState* glideState = nullptr;

    int pitchWheelPos = 8192;
    float pitchBendSemitones = 0.0f;
    float pitchBendRangeSemitones = 2.0f;

    // Oscillators
    DSP::BandlimitedOscillator osc1;
    DSP::BandlimitedOscillator osc2;
    DSP::BandlimitedOscillator subOsc;
    DSP::NoiseGenerator noiseGen;
    DSP::LowFrequencyOscillator lfo1;

    // Unison: detuned side copies of Osc 1 / Osc 2, panned L/R for width.
    DSP::BandlimitedOscillator osc1UniLo, osc1UniHi;
    DSP::BandlimitedOscillator osc2UniLo, osc2UniHi;
    bool unisonOn = false;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> filter;   // 12dB LP / HP / BP (stereo via channel 0/1)
    DSP::LadderFilter ladderFilter;                     // 24dB LP (ladder), left/center
    DSP::LadderFilter ladderFilterR;                    // 24dB LP, right channel (for unison width)
    DSP::LadderFilter ladderFilterAgg;                  // Ladder Aggressive, left/center
    DSP::LadderFilter ladderFilterAggR;                 // Ladder Aggressive, right channel

    // Envelopes
    juce::ADSR ampAdsr;
    juce::ADSR filterAdsr;
    juce::ADSR::Parameters ampParams;
    juce::ADSR::Parameters filterParams;
    bool envParamsInitialised = false;

    // Cached parameters
    float osc1PW = 0.5f;
    float osc1Ratio = 1.0f, osc2Ratio = 1.0f, subRatio = 0.5f;   // pitch factors from octave/semi/fine
    float osc1Level = 0.7f, osc2Level = 0.5f, subLevel = 0.4f, noiseLevel = 0.0f;
    float osc1Lv = 0.7f, osc2Lv = 0.5f, subLv = 0.4f, noiseLv = 0.0f;   // smoothed levels
    bool osc2Sync = false;

    int filterType = 1;
    float baseCutoff = 2500.0f, baseSmoothed = 2500.0f;
    float filterRes = -1.0f;
    float filterEnvAmt = 0.5f;
    float filterKeyTrack = 0.5f;
    float cachedKeyFreq = -1.0f, cachedKeyTrack = -1.0f, cachedKeyFactor = 1.0f;

    float lfoDepth = 0.2f;
    int lfoDest = 0;

    juce::AudioBuffer<float> voiceBuffer;
};
