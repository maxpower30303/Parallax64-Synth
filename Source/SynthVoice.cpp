// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#include "SynthVoice.h"

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

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(outputChannels);

    filter.prepare(spec);
    filter.reset();

    ladderFilter.prepare(sampleRate);
    ladderFilter.reset();
    ladderFilterR.prepare(sampleRate);
    ladderFilterR.reset();

    ladderFilterAgg.prepare(sampleRate);
    ladderFilterAgg.reset();
    ladderFilterAgg.setAggressive(true);
    ladderFilterAggR.prepare(sampleRate);
    ladderFilterAggR.reset();
    ladderFilterAggR.setAggressive(true);

    voiceBuffer.setSize(outputChannels, samplesPerBlock);
}

void SynthVoice::updateParameters(juce::AudioProcessorValueTreeState& apvts)
{
    // Waveforms
    auto w1 = static_cast<int>(*apvts.getRawParameterValue(IDs::osc1Wave));
    osc1.setWaveform(static_cast<DSP::Waveform>(w1));
    osc1PW = *apvts.getRawParameterValue(IDs::osc1PulseWidth);
    osc1.setPulseWidth(osc1PW);
    osc1Oct = static_cast<int>(*apvts.getRawParameterValue(IDs::osc1Octave));
    osc1Semi = static_cast<int>(*apvts.getRawParameterValue(IDs::osc1Semi));
    osc1Fine = *apvts.getRawParameterValue(IDs::osc1Fine);
    osc1Level = *apvts.getRawParameterValue(IDs::osc1Level);

    auto w2 = static_cast<int>(*apvts.getRawParameterValue(IDs::osc2Wave));
    osc2.setWaveform(static_cast<DSP::Waveform>(w2));
    osc2Oct = static_cast<int>(*apvts.getRawParameterValue(IDs::osc2Octave));
    osc2Semi = static_cast<int>(*apvts.getRawParameterValue(IDs::osc2Semi));
    osc2Fine = *apvts.getRawParameterValue(IDs::osc2Fine);
    osc2Level = *apvts.getRawParameterValue(IDs::osc2Level);
    osc2Sync = *apvts.getRawParameterValue(IDs::osc2Sync) > 0.5f;

    // Unison side oscillators mirror the center oscillators' waveform/PW
    unisonOn = *apvts.getRawParameterValue(IDs::unisonEnabled) > 0.5f;
    osc1UniLo.setWaveform(static_cast<DSP::Waveform>(w1));
    osc1UniHi.setWaveform(static_cast<DSP::Waveform>(w1));
    osc1UniLo.setPulseWidth(osc1PW);
    osc1UniHi.setPulseWidth(osc1PW);
    osc2UniLo.setWaveform(static_cast<DSP::Waveform>(w2));
    osc2UniHi.setWaveform(static_cast<DSP::Waveform>(w2));

    auto sw = static_cast<int>(*apvts.getRawParameterValue(IDs::subWave));
    // Sub oscillator waveform choice (Square, Sine, Triangle)
    DSP::Waveform mappedSub = (sw == 0) ? DSP::Waveform::Square : ((sw == 1) ? DSP::Waveform::Sine : DSP::Waveform::Triangle);
    subOsc.setWaveform(mappedSub);
    subOct = static_cast<int>(*apvts.getRawParameterValue(IDs::subOctave));
    subLevel = *apvts.getRawParameterValue(IDs::subLevel);

    noiseLevel = *apvts.getRawParameterValue(IDs::noiseLevel);

    // Filter
    filterType = static_cast<int>(*apvts.getRawParameterValue(IDs::filterType));
    baseCutoff = *apvts.getRawParameterValue(IDs::filterCutoff);
    filterRes = *apvts.getRawParameterValue(IDs::filterRes);
    filterEnvAmt = *apvts.getRawParameterValue(IDs::filterEnvAmt);
    filterKeyTrack = *apvts.getRawParameterValue(IDs::filterKeyTrack);

    // Filter type indices (must match the "Filter Type" choice list in Parameters.h
    // and the fTypes button labels in PluginEditor.cpp):
    // 0 = 12dB LP, 1 = 24dB LP (ladder), 2 = Highpass, 3 = Ladder Aggressive
    // (hotter feedback ceiling + extra drive), 4 = Bandpass
    switch (filterType)
    {
        case 0: filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass); break;
        case 2: filter.setType(juce::dsp::StateVariableTPTFilterType::highpass); break;
        case 4: filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass); break;
        default: filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass); break;
    }
    filter.setResonance(filterRes);
    ladderFilter.setResonance(filterRes);
    ladderFilterR.setResonance(filterRes);
    ladderFilterAgg.setResonance(filterRes);
    ladderFilterAggR.setResonance(filterRes);

    // ADSRs
    ampParams.attack = *apvts.getRawParameterValue(IDs::ampAttack);
    ampParams.decay = *apvts.getRawParameterValue(IDs::ampDecay);
    ampParams.sustain = *apvts.getRawParameterValue(IDs::ampSustain);
    ampParams.release = *apvts.getRawParameterValue(IDs::ampRelease);
    ampAdsr.setParameters(ampParams);

    filterParams.attack = *apvts.getRawParameterValue(IDs::filterAttack);
    filterParams.decay = *apvts.getRawParameterValue(IDs::filterDecay);
    filterParams.sustain = *apvts.getRawParameterValue(IDs::filterSustain);
    filterParams.release = *apvts.getRawParameterValue(IDs::filterRelease);
    filterAdsr.setParameters(filterParams);

    // LFO 1
    auto lfoW = static_cast<int>(*apvts.getRawParameterValue(IDs::lfo1Wave));
    lfo1.setWaveform(static_cast<DSP::Waveform>(lfoW));
    lfo1.setRate(*apvts.getRawParameterValue(IDs::lfo1Rate));
    lfoDepth = *apvts.getRawParameterValue(IDs::lfo1Depth);
    lfoDest = static_cast<int>(*apvts.getRawParameterValue(IDs::lfo1Dest));

    glideTime = *apvts.getRawParameterValue(IDs::glideTime);
    pitchBendRangeSemitones = *apvts.getRawParameterValue(IDs::pitchBend);
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition)
{
    currentVelocity = velocity;
    float targetNoteFreq = static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));

    if (glideTime > 0.005f && currentFreq > 20.0f)
    {
        targetFreq = targetNoteFreq;
        glideRate = (targetFreq - currentFreq) / (glideTime * static_cast<float>(currentSampleRate));
    }
    else
    {
        currentFreq = targetNoteFreq;
        targetFreq = targetNoteFreq;
        glideRate = 0.0f;
    }

    pitchWheelMoved(currentPitchWheelPosition);

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
        clearCurrentNote();
        ampAdsr.reset();
        filterAdsr.reset();
    }
}

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
    pitchBendSemitones = ((newPitchWheelValue - 8192) / 8192.0f) * pitchBendRangeSemitones;
}

void SynthVoice::controllerMoved(int, int)
{
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    voiceBuffer.clear();

    auto* leftOut = voiceBuffer.getWritePointer(0);
    bool stereoOut = voiceBuffer.getNumChannels() > 1;
    auto* rightOut = stereoOut ? voiceBuffer.getWritePointer(1) : nullptr;

    for (int s = 0; s < numSamples; ++s)
    {
        if (glideRate != 0.0f)
        {
            currentFreq += glideRate;
            if ((glideRate > 0.0f && currentFreq >= targetFreq) || (glideRate < 0.0f && currentFreq <= targetFreq))
            {
                currentFreq = targetFreq;
                glideRate = 0.0f;
            }
        }

        float lfoVal = lfo1.processSample() * lfoDepth;

        // Pitch calculation
        float pitchMod = (lfoDest == 1) ? lfoVal * 12.0f : 0.0f;

        // Osc 1 Frequency
        float osc1Freq = currentFreq * std::pow(2.0f, (osc1Oct * 12.0f + osc1Semi + pitchBendSemitones + pitchMod + osc1Fine * 0.01f) / 12.0f);
        osc1.setFrequency(osc1Freq);
        if (lfoDest == 3) // PWM
            osc1.setPulseWidth(juce::jlimit(0.05f, 0.95f, osc1PW + lfoVal * 0.4f));

        // Osc 2 Frequency
        float osc2Freq = currentFreq * std::pow(2.0f, (osc2Oct * 12.0f + osc2Semi + pitchBendSemitones + pitchMod + osc2Fine * 0.01f) / 12.0f);
        osc2.setFrequency(osc2Freq);

        // Sub Osc Frequency
        float subFreq = currentFreq * std::pow(2.0f, (subOct * 12.0f + pitchBendSemitones) / 12.0f);
        subOsc.setFrequency(subFreq);

        // Render Oscillators with mixer bus headroom (-8 dB)
        float o1 = osc1.processSample() * osc1Level;
        float o2 = osc2.processSample() * osc2Level;
        float sub = subOsc.processSample() * subLevel;
        float noise = noiseGen.processSample() * noiseLevel;

        float centerOsc = o1 + o2;
        float subAndNoise = sub + noise;

        // Unison: a detuned copy of Osc1+Osc2 panned to each side, so the
        // stereo image widens instead of just adding volume. Side voices sit
        // at half level under the center pair to keep the loudness jump modest.
        float uniLo = 0.0f, uniHi = 0.0f;
        if (unisonOn)
        {
            constexpr float unisonDetuneCents = 11.0f;
            constexpr float unisonSideLevel = 0.5f;
            float detuneDown = std::pow(2.0f, -unisonDetuneCents / 1200.0f);
            float detuneUp   = std::pow(2.0f,  unisonDetuneCents / 1200.0f);

            osc1UniLo.setFrequency(osc1Freq * detuneDown);
            osc1UniHi.setFrequency(osc1Freq * detuneUp);
            osc2UniLo.setFrequency(osc2Freq * detuneDown);
            osc2UniHi.setFrequency(osc2Freq * detuneUp);

            uniLo = (osc1UniLo.processSample() * osc1Level + osc2UniLo.processSample() * osc2Level) * unisonSideLevel;
            uniHi = (osc1UniHi.processSample() * osc1Level + osc2UniHi.processSample() * osc2Level) * unisonSideLevel;
        }

        float mixSumLeft  = (centerOsc + uniLo + subAndNoise) * 0.40f;
        float mixSumRight = (centerOsc + uniHi + subAndNoise) * 0.40f;

        // Envelope values
        float ampEnvVal = ampAdsr.getNextSample();
        float filterEnvVal = filterAdsr.getNextSample();

        // Filter Cutoff Modulation
        // All modulation sources are applied as octave (exponential) offsets rather
        // than raw Hz, so the perceived amount of sweep is consistent regardless of
        // where the base cutoff sits, and a modest env/LFO amount can never slam the
        // cutoff down to the 20 Hz floor for the whole attack like a flat +/-Hz term does.
        float cutoff = baseCutoff;
        if (filterKeyTrack > 0.0f)
        {
            float noteOffset = (currentFreq / 261.63f); // C4 reference
            cutoff *= std::pow(noteOffset, filterKeyTrack);
        }

        constexpr float envOctaveRange = 6.0f; // +/- 6 octaves at full env & full amount
        cutoff *= std::pow(2.0f, filterEnvVal * filterEnvAmt * envOctaveRange);

        if (lfoDest == 0) // LFO to Cutoff
            cutoff *= std::pow(2.0f, lfoVal * 3.0f);

        cutoff = juce::jlimit(20.0f, 20000.0f, cutoff);

        // Filter sample(s). Mono output, or unison off: mono signal through
        // channel 0 only, duplicated to both outputs (cheap, same as before).
        // Unison on with a stereo bus: the L/R detuned mixes need independent
        // filter state per side, so the right channel gets its own filter
        // pass. Channel 1 of `filter` only exists when the bus is actually
        // stereo (it's sized from outputChannels in prepare()), so this must
        // never be reached with a mono bus - doing so would read/write past
        // the filter's per-channel state array.
        bool unisonStereo = unisonOn && stereoOut;
        float filteredLeft, filteredRight;
        if (filterType == 1 || filterType == 3)
        {
            DSP::LadderFilter& ladderL = (filterType == 3) ? ladderFilterAgg : ladderFilter;
            DSP::LadderFilter& ladderR = (filterType == 3) ? ladderFilterAggR : ladderFilterR;

            ladderL.setCutoffFrequency(cutoff);
            filteredLeft = ladderL.processSample(mixSumLeft);

            if (unisonStereo)
            {
                ladderR.setCutoffFrequency(cutoff);
                filteredRight = ladderR.processSample(mixSumRight);
            }
            else
            {
                filteredRight = filteredLeft;
            }
        }
        else
        {
            filter.setCutoffFrequency(cutoff);
            filteredLeft = filter.processSample(0, mixSumLeft);
            filteredRight = unisonStereo ? filter.processSample(1, mixSumRight) : filteredLeft;
        }

        // Amp modulation with polyphonic voice headroom (-10.5 dB)
        float ampFinal = ampEnvVal * currentVelocity;
        if (lfoDest == 2) // LFO to Amp
            ampFinal *= juce::jlimit(0.0f, 1.0f, 1.0f + lfoVal);

        constexpr float voiceHeadroom = 0.30f;
        leftOut[s] = filteredLeft * ampFinal * voiceHeadroom;
        if (rightOut != nullptr)
            rightOut[s] = filteredRight * ampFinal * voiceHeadroom;

        if (!ampAdsr.isActive())
        {
            clearCurrentNote();
            break;
        }
    }

    // Accumulate into master buffer
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
    {
        outputBuffer.addFrom(ch, startSample, voiceBuffer, std::min(ch, voiceBuffer.getNumChannels() - 1), 0, numSamples);
    }
}
