// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

P64AudioProcessor::P64AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", Parameters::createParameterLayout()),
      midiLearn(apvts)
{
    // Add polyphonic synthesizer voices
    for (int i = 0; i < NUM_VOICES; ++i)
        synth.addVoice(new SynthVoice());

    synth.addSound(new SynthSound());

    waveformSnapshot.resize(512, 0.0f);
}

P64AudioProcessor::~P64AudioProcessor()
{
}

void P64AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    distortion.prepare(spec);
    chorus.prepare(spec);
    delay.prepare(spec);
    reverb.prepare(spec);

    arpeggiator.prepare(sampleRate);
    monoStack.reset();
}

void P64AudioProcessor::releaseResources()
{
}

bool P64AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void P64AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Process MIDI Learn
    midiLearn.handleIncomingMidi(midiMessages);

    // Update voice parameters from APVTS
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->updateParameters(apvts);
        }
    }

    // Process notes from the UI virtual keyboard
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // Arpeggiator Processing
    bool arpOn = *apvts.getRawParameterValue(IDs::arpEnabled) > 0.5f;
    int arpM = static_cast<int>(*apvts.getRawParameterValue(IDs::arpMode));
    float arpR = *apvts.getRawParameterValue(IDs::arpRate);
    float arpG = *apvts.getRawParameterValue(IDs::arpGate);
    int arpO = static_cast<int>(*apvts.getRawParameterValue(IDs::arpOctave));

    // Mono/Poly voice mode: collapse held notes to a single last-note-priority
    // line before the arp/synth see them. Skipped while the arp is running,
    // since it already produces its own monophonic note stream.
    bool monoOn = static_cast<int>(*apvts.getRawParameterValue(IDs::voiceMode)) == 1;
    if (monoOn && !arpOn)
        monoStack.process(midiMessages);
    else
        monoStack.reset();

    arpeggiator.process(midiMessages, buffer.getNumSamples(), arpOn, arpM, arpR, arpG, arpO);

    // Render polyphonic synth voices
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // FX: Distortion
    float distDr = *apvts.getRawParameterValue(IDs::distDrive);
    float distMx = *apvts.getRawParameterValue(IDs::distMix);
    distortion.setDrive(distDr);
    distortion.setMix(distMx);
    distortion.process(buffer);

    // FX: Chorus
    bool chOn = apvts.getRawParameterValue(IDs::chorusEnabled)->load() > 0.5f;
    float chRate = apvts.getRawParameterValue(IDs::chorusRate)->load();
    float chDepth = apvts.getRawParameterValue(IDs::chorusDepth)->load();
    float chMix = chOn ? apvts.getRawParameterValue(IDs::chorusMix)->load() : 0.0f;
    chorus.setParameters(chRate, chDepth, chMix);
    chorus.process(buffer);

    // FX: Delay
    bool dlyOn = apvts.getRawParameterValue(IDs::delayEnabled)->load() > 0.5f;
    float delTime = apvts.getRawParameterValue(IDs::delayTime)->load();
    float delFb = apvts.getRawParameterValue(IDs::delayFeedback)->load();
    float delMix = dlyOn ? apvts.getRawParameterValue(IDs::delayMix)->load() : 0.0f;
    delay.setParameters(delTime, delFb, delMix);
    delay.process(buffer);

    // FX: Reverb
    bool revOn = apvts.getRawParameterValue(IDs::reverbEnabled)->load() > 0.5f;
    float revSize = apvts.getRawParameterValue(IDs::reverbSize)->load();
    float revDamp = apvts.getRawParameterValue(IDs::reverbDamp)->load();
    float revMix = revOn ? apvts.getRawParameterValue(IDs::reverbMix)->load() : 0.0f;
    reverb.setParameters(revSize, revDamp, revMix);
    reverb.process(buffer);

    // Master Volume
    float masterVol = apvts.getRawParameterValue(IDs::masterVolume)->load();
    buffer.applyGain(masterVol);

    // Master Output Soft-Limiter (transparent safety ceiling preventing digital inter-sample clipping)
    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        auto* chData = buffer.getWritePointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            float x = chData[s];
            if (x > 1.0f)
                chData[s] = 0.95f + 0.05f * std::tanh((x - 0.95f) / 0.05f);
            else if (x < -1.0f)
                chData[s] = -0.95f + 0.05f * std::tanh((x + 0.95f) / 0.05f);
        }
    }

    // RMS Metering & Waveform capture
    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        rmsLevels[ch] = buffer.getRMSLevel(ch, 0, buffer.getNumSamples());
    }

    if (totalNumOutputChannels > 0 && buffer.getNumSamples() > 0)
    {
        const float* readPtr = buffer.getReadPointer(0);
        int stride = std::max(1, buffer.getNumSamples() / 32);
        for (int i = 0; i < buffer.getNumSamples(); i += stride)
        {
            waveformSnapshot[waveformWriteIndex] = readPtr[i];
            waveformWriteIndex = (waveformWriteIndex + 1) % waveformSnapshot.size();
        }
    }
}

juce::AudioProcessorEditor* P64AudioProcessor::createEditor()
{
    return new P64AudioProcessorEditor(*this);
}

int P64AudioProcessor::getNumPrograms()
{
    return Presets::getNumPresets();
}

int P64AudioProcessor::getCurrentProgram()
{
    return currentPresetIndex;
}

void P64AudioProcessor::setCurrentProgram(int index)
{
    currentPresetIndex = index;
    Presets::applyPreset(index, apvts);
}

const juce::String P64AudioProcessor::getProgramName(int index)
{
    return Presets::getPresetName(index);
}

void P64AudioProcessor::changeProgramName(int, const juce::String&)
{
}

void P64AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement root("P64_STATE");

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> paramsXml(state.createXml());
    root.addChildElement(paramsXml.release());

    root.addChildElement(midiLearn.createXml().release());

    copyXmlToBinary(root, destData);
}

void P64AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() == nullptr)
        return;

    if (xmlState->hasTagName("P64_STATE"))
    {
        if (auto* paramsXml = xmlState->getChildByName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*paramsXml));

        midiLearn.restoreFromXml(xmlState->getChildByName("MIDI_LEARN"));
    }
    else if (xmlState->hasTagName(apvts.state.getType()))
    {
        // Backwards-compat: state saved by an older version without MIDI-Learn persistence.
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new P64AudioProcessor();
}
