// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#include "SynthVoice.h"

namespace
{
    // Unison side voices: +-11 cents, half level under the centre pair.
    const float kUnisonDown = static_cast<float>(std::pow(2.0, -11.0 / 1200.0));
    const float kUnisonUp   = static_cast<float>(std::pow(2.0,  11.0 / 1200.0));
    constexpr float kUnisonSideLevel = 0.5f;
    constexpr float kEnvOctaveRange = 6.0f;   // +/- 6 octaves at full env & full amount
}

SynthVoice::SynthVoice()
{
}

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::prepare(double sampleRate, int samplesPerBlock, int outputChannels)
{
    currentSampleRate = sampleRate;
    levelCoef = 1.0f - std::exp(-static_cast<float>(kControlRate) / (0.004f * static_cast<float>(sampleRate)));

    osc1.prepare(sampleRate);
    osc2.prepare(sampleRate);
    subOsc.prepare(sampleRate);
    lfo1.prepare(sampleRate);

    osc1UniLo.prepare(sampleRate);
    osc1UniHi.prepare(sampleRate);
    osc2UniLo.prepare(sampleRate);
    osc2UniHi.prepare(sampleRate);

    ampAdsr.setSampleRate(sampleRate);
    filterAdsr.setSampleRate(sampleRate);
    envParamsInitialised = false;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(outputChannels);

    filter.prepare(spec);
    filter.reset();

    ladderFilter.prepare(sampleRate);
    ladderFilterR.prepare(sampleRate);
    ladderFilterAgg.prepare(sampleRate);
    ladderFilterAgg.setAggressive(true);
    ladderFilterAggR.prepare(sampleRate);
    ladderFilterAggR.setAggressive(true);

    filterRes = -1.0f;   // forces the resonance to be pushed to all filters on the next applyParams
    voiceBuffer.setSize(outputChannels, samplesPerBlock);
}

void SynthVoice::applyParams(const VoiceParams& p)
{
    // Oscillators
    const auto w1 = static_cast<DSP::Waveform>(p.osc1Wave);
    const auto w2 = static_cast<DSP::Waveform>(p.osc2Wave);
    osc1.setWaveform(w1);
    osc2.setWaveform(w2);
    osc1PW = p.osc1PW;
    osc1.setPulseWidth(osc1PW);
    osc1Ratio = std::exp2((p.osc1Oct * 12.0f + p.osc1Semi + p.osc1Fine * 0.01f) / 12.0f);
    osc2Ratio = std::exp2((p.osc2Oct * 12.0f + p.osc2Semi + p.osc2Fine * 0.01f) / 12.0f);
    osc1Level = p.osc1Level;
    osc2Level = p.osc2Level;
    osc2Sync = p.osc2Sync;

    // Unison side oscillators mirror the centre oscillators' waveform/PW
    unisonOn = p.unison;
    osc1UniLo.setWaveform(w1);  osc1UniHi.setWaveform(w1);
    osc2UniLo.setWaveform(w2);  osc2UniHi.setWaveform(w2);
    osc1UniLo.setPulseWidth(osc1PW);
    osc1UniHi.setPulseWidth(osc1PW);

    // Sub oscillator waveform choice (Square, Sine, Triangle)
    subOsc.setWaveform(p.subWave == 0 ? DSP::Waveform::Square
                                      : (p.subWave == 1 ? DSP::Waveform::Sine : DSP::Waveform::Triangle));
    subRatio = std::exp2(static_cast<float>(p.subOct));
    subLevel = p.subLevel;
    noiseLevel = p.noiseLevel;

    // Filter. Type indices must match the "Filter Type" choice list in Parameters.h and the
    // fTypes labels in PluginEditor.cpp: 0 = 12dB LP, 1 = 24dB LP (ladder), 2 = Highpass,
    // 3 = Ladder Aggressive, 4 = Bandpass
    if (p.filterType != filterType)
    {
        filterType = p.filterType;
        ladderFilter.reset();  ladderFilterR.reset();
        ladderFilterAgg.reset();  ladderFilterAggR.reset();
        filter.reset();
    }
    switch (filterType)
    {
        case 2:  filter.setType(juce::dsp::StateVariableTPTFilterType::highpass); break;
        case 4:  filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass); break;
        default: filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass); break;
    }
    if (p.resonance != filterRes)
    {
        filterRes = p.resonance;
        filter.setResonance(filterRes);
        ladderFilter.setResonance(filterRes);
        ladderFilterR.setResonance(filterRes);
        ladderFilterAgg.setResonance(filterRes);
        ladderFilterAggR.setResonance(filterRes);
    }
    baseCutoff = p.cutoff;
    filterEnvAmt = p.envAmount;
    filterKeyTrack = p.keyTrack;

    // Envelopes (only touched when a value actually changed)
    const bool ampChanged = ampParams.attack != p.ampA || ampParams.decay != p.ampD
                         || ampParams.sustain != p.ampS || ampParams.release != p.ampR;
    const bool filtChanged = filterParams.attack != p.filtA || filterParams.decay != p.filtD
                          || filterParams.sustain != p.filtS || filterParams.release != p.filtR;
    if (ampChanged || ! envParamsInitialised)
    {
        ampParams.attack = p.ampA; ampParams.decay = p.ampD; ampParams.sustain = p.ampS; ampParams.release = p.ampR;
        ampAdsr.setParameters(ampParams);
    }
    if (filtChanged || ! envParamsInitialised)
    {
        filterParams.attack = p.filtA; filterParams.decay = p.filtD; filterParams.sustain = p.filtS; filterParams.release = p.filtR;
        filterAdsr.setParameters(filterParams);
    }
    envParamsInitialised = true;

    // LFO
    lfo1.setWaveform(static_cast<DSP::Waveform>(p.lfoWave));
    lfo1.setRate(p.lfoRate);
    lfoDepth = p.lfoDepth;
    lfoDest = p.lfoDest;

    glideTime = p.glideTime;
    velocitySens = p.velocitySens;
    pitchBendRangeSemitones = p.pitchBendRange;
    updatePitchBend();
}

void SynthVoice::updatePitchBend()
{
    pitchBendSemitones = ((pitchWheelPos - 8192) / 8192.0f) * pitchBendRangeSemitones;
}

float SynthVoice::keytrackFactor()
{
    if (currentFreq != cachedKeyFreq || filterKeyTrack != cachedKeyTrack)
    {
        cachedKeyFactor = std::pow(currentFreq / 261.63f, filterKeyTrack);   // C4 reference
        cachedKeyFreq = currentFreq;
        cachedKeyTrack = filterKeyTrack;
    }
    return cachedKeyFactor;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    currentVelocity = velocity;
    velocityGain = 1.0f - velocitySens * (1.0f - velocity);   // sens 0 = ignore velocity, sens 1 = linear

    const float targetNoteFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
    const float from = (glideState != nullptr) ? glideState->lastFreq : 0.0f;

    if (glideTime > 0.005f && from > 20.0f && from != targetNoteFreq)
    {
        // Portamento from the note that was played before, linear in pitch (constant ratio per step)
        currentFreq = from;
        targetFreq = targetNoteFreq;
        const float steps = glideTime * static_cast<float>(currentSampleRate) / static_cast<float>(kControlRate);
        glideMulPerGroup = std::pow(targetNoteFreq / from, 1.0f / juce::jmax(1.0f, steps));
        glideActive = true;
    }
    else
    {
        currentFreq = targetNoteFreq;
        targetFreq = targetNoteFreq;
        glideActive = false;
    }
    if (glideState != nullptr)
        glideState->lastFreq = targetNoteFreq;

    pitchWheelPos = currentPitchWheelPosition;
    updatePitchBend();

    // Start from the current knob positions instead of gliding in from stale smoothed values
    osc1Lv = osc1Level;  osc2Lv = osc2Level;  subLv = subLevel;  noiseLv = noiseLevel;
    baseSmoothed = baseCutoff;

    if (osc2Sync)
    {
        osc1.resetPhase();
        osc2.resetPhase();
        osc1UniLo.resetPhase();
        osc1UniHi.resetPhase();
        osc2UniLo.resetPhase();
        osc2UniHi.resetPhase();
    }

    ampAdsr.noteOn();
    filterAdsr.noteOn();
}

void SynthVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampAdsr.noteOff();
        filterAdsr.noteOff();
    }
    else
    {
        // Voice stolen: also clear the filters, a resonant filter would otherwise keep
        // ringing into the next note that uses this voice.
        clearCurrentNote();
        ampAdsr.reset();
        filterAdsr.reset();
        filter.reset();
        ladderFilter.reset();  ladderFilterR.reset();
        ladderFilterAgg.reset();  ladderFilterAggR.reset();
    }
}

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchWheelPos = newPitchWheelValue;
    updatePitchBend();
}

void SynthVoice::controllerMoved(int, int)
{
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    numSamples = juce::jmin(numSamples, voiceBuffer.getNumSamples());   // never write past the scratch buffer
    voiceBuffer.clear(0, numSamples);

    float* leftOut = voiceBuffer.getWritePointer(0);
    const bool stereoOut = voiceBuffer.getNumChannels() > 1;
    float* rightOut = stereoOut ? voiceBuffer.getWritePointer(1) : nullptr;

    // Right-channel filtering only exists for unison on a real stereo bus. Channel 1 of the
    // SVF is sized from the bus layout in prepare(), so it must never be used on a mono bus.
    const bool unisonStereo = unisonOn && stereoOut;
    const bool useLadder = (filterType == 1 || filterType == 3);
    DSP::LadderFilter& ladderL = (filterType == 3) ? ladderFilterAgg : ladderFilter;
    DSP::LadderFilter& ladderR = (filterType == 3) ? ladderFilterAggR : ladderFilterR;

    const float bendFactor = std::exp2(pitchBendSemitones * (1.0f / 12.0f));
    const bool syncOn = osc2Sync;

    int s = 0;
    bool finished = false;

    while (s < numSamples && ! finished)
    {
        const int n = juce::jmin(kControlRate, numSamples - s);

        // ---- control-rate section: everything that moves slowly -----------------------
        if (glideActive)
        {
            currentFreq *= glideMulPerGroup;
            if ((glideMulPerGroup >= 1.0f && currentFreq >= targetFreq) || (glideMulPerGroup < 1.0f && currentFreq <= targetFreq))
            {
                currentFreq = targetFreq;
                glideActive = false;
            }
        }

        osc1Lv    += (osc1Level  - osc1Lv)    * levelCoef;
        osc2Lv    += (osc2Level  - osc2Lv)    * levelCoef;
        subLv     += (subLevel   - subLv)     * levelCoef;
        noiseLv   += (noiseLevel - noiseLv)   * levelCoef;
        baseSmoothed += (baseCutoff - baseSmoothed) * levelCoef;

        const float lfoVal = lfo1.value() * lfoDepth;
        lfo1.advance(n);

        float filterEnvVal = filterAdsr.getNextSample();
        for (int i = 1; i < n; ++i)
            filterAdsr.getNextSample();

        // Pitch: osc ratio (oct/semi/fine) * pitch bend * LFO vibrato (+-12 semitones at full depth)
        const float pitchMul = (lfoDest == 1) ? std::exp2(lfoVal) : 1.0f;
        const float f1 = currentFreq * osc1Ratio * bendFactor * pitchMul;
        const float f2 = currentFreq * osc2Ratio * bendFactor * pitchMul;
        osc1.setFrequency(f1);
        osc2.setFrequency(f2);
        subOsc.setFrequency(currentFreq * subRatio * bendFactor);
        if (unisonOn)
        {
            osc1UniLo.setFrequency(f1 * kUnisonDown);  osc1UniHi.setFrequency(f1 * kUnisonUp);
            osc2UniLo.setFrequency(f2 * kUnisonDown);  osc2UniHi.setFrequency(f2 * kUnisonUp);
        }
        if (lfoDest == 3) // PWM
        {
            const float pw = juce::jlimit(0.05f, 0.95f, osc1PW + lfoVal * 0.4f);
            osc1.setPulseWidth(pw);
            if (unisonOn) { osc1UniLo.setPulseWidth(pw); osc1UniHi.setPulseWidth(pw); }
        }

        // Filter cutoff. All modulation is applied as octave (exponential) offsets, so the
        // sweep sounds the same wherever the base cutoff sits and a small negative env amount
        // can never slam the cutoff to the 20 Hz floor.
        float cutoff = baseSmoothed;
        if (filterKeyTrack > 0.0f)
            cutoff *= keytrackFactor();
        if (filterEnvAmt != 0.0f)
            cutoff *= std::exp2(filterEnvVal * filterEnvAmt * kEnvOctaveRange);
        if (lfoDest == 0)
            cutoff *= std::exp2(lfoVal * 3.0f);
        cutoff = juce::jlimit(20.0f, 20000.0f, cutoff);

        if (useLadder)
        {
            ladderL.setCutoffFrequency(cutoff);
            if (unisonStereo)
                ladderR.setCutoffFrequency(cutoff);
        }
        else
        {
            filter.setCutoffFrequency(cutoff);
        }

        // ---- audio-rate section ---------------------------------------------------------
        for (int i = 0; i < n; ++i, ++s)
        {
            const float o1 = osc1.processSample() * osc1Lv;
            const float o2 = osc2.processSample() * osc2Lv;
            if (syncOn && osc1.didWrap())
                osc2.syncRestart(osc1.samplesSinceWrap());   // hard sync: slave restarts with the master

            float uniLo = 0.0f, uniHi = 0.0f;
            if (unisonOn)
            {
                const float aLo = osc1UniLo.processSample() * osc1Lv;
                const float bLo = osc2UniLo.processSample() * osc2Lv;
                if (syncOn && osc1UniLo.didWrap())
                    osc2UniLo.syncRestart(osc1UniLo.samplesSinceWrap());

                const float aHi = osc1UniHi.processSample() * osc1Lv;
                const float bHi = osc2UniHi.processSample() * osc2Lv;
                if (syncOn && osc1UniHi.didWrap())
                    osc2UniHi.syncRestart(osc1UniHi.samplesSinceWrap());

                // A detuned copy panned to each side widens the stereo image instead of only
                // adding volume; the side voices sit at half level under the centre pair.
                uniLo = (aLo + bLo) * kUnisonSideLevel;
                uniHi = (aHi + bHi) * kUnisonSideLevel;
            }

            const float sub = subOsc.processSample() * subLv;
            const float noise = noiseGen.processSample() * noiseLv;
            const float common = o1 + o2 + sub + noise;
            const float mixSumLeft  = (common + uniLo) * 0.40f;
            const float mixSumRight = (common + uniHi) * 0.40f;

            float filteredLeft, filteredRight;
            if (useLadder)
            {
                filteredLeft = ladderL.processSample(mixSumLeft);
                filteredRight = unisonStereo ? ladderR.processSample(mixSumRight) : filteredLeft;
            }
            else
            {
                filteredLeft = filter.processSample(0, mixSumLeft);
                filteredRight = unisonStereo ? filter.processSample(1, mixSumRight) : filteredLeft;
            }

            // Amp: envelope * velocity (shaped by the VEL knob) * optional LFO tremolo, with voice headroom
            float ampFinal = ampAdsr.getNextSample() * velocityGain;
            if (lfoDest == 2)
                ampFinal *= juce::jlimit(0.0f, 1.0f, 1.0f + lfoVal);
            ampFinal *= 0.30f;

            leftOut[s] = filteredLeft * ampFinal;
            if (rightOut != nullptr)
                rightOut[s] = filteredRight * ampFinal;

            if (! ampAdsr.isActive())
            {
                clearCurrentNote();
                finished = true;
                break;
            }
        }
    }

    // Accumulate into master buffer
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
        outputBuffer.addFrom(ch, startSample, voiceBuffer, std::min(ch, voiceBuffer.getNumChannels() - 1), 0, numSamples);
}
