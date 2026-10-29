// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include "UserPresets.h"

P64AudioProcessor::P64AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", Parameters::createParameterLayout()),
      midiLearn(apvts)
{
    // Add polyphonic synthesizer voices
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        voices[i] = new SynthVoice();
        voices[i]->setGlideState(&glideState);
        synth.addVoice(voices[i]);
    }

    synth.addSound(new SynthSound());

    auto P = [this](const juce::String& id) { return apvts.getRawParameterValue(id); };
    pc.osc1Wave = P(IDs::osc1Wave);   pc.osc1Oct = P(IDs::osc1Octave);  pc.osc1Semi = P(IDs::osc1Semi);
    pc.osc1Fine = P(IDs::osc1Fine);   pc.osc1Level = P(IDs::osc1Level); pc.osc1PW = P(IDs::osc1PulseWidth);
    pc.osc2Wave = P(IDs::osc2Wave);   pc.osc2Oct = P(IDs::osc2Octave);  pc.osc2Semi = P(IDs::osc2Semi);
    pc.osc2Fine = P(IDs::osc2Fine);   pc.osc2Level = P(IDs::osc2Level); pc.osc2Sync = P(IDs::osc2Sync);
    pc.subWave = P(IDs::subWave);     pc.subOct = P(IDs::subOctave);    pc.subLevel = P(IDs::subLevel);
    pc.noiseLevel = P(IDs::noiseLevel);
    pc.filterType = P(IDs::filterType); pc.cutoff = P(IDs::filterCutoff); pc.res = P(IDs::filterRes);
    pc.envAmt = P(IDs::filterEnvAmt); pc.keyTrack = P(IDs::filterKeyTrack);
    pc.ampA = P(IDs::ampAttack);      pc.ampD = P(IDs::ampDecay);       pc.ampS = P(IDs::ampSustain);   pc.ampR = P(IDs::ampRelease);
    pc.filtA = P(IDs::filterAttack);  pc.filtD = P(IDs::filterDecay);   pc.filtS = P(IDs::filterSustain); pc.filtR = P(IDs::filterRelease);
    pc.lfoWave = P(IDs::lfo1Wave);    pc.lfoRate = P(IDs::lfo1Rate);    pc.lfoDepth = P(IDs::lfo1Depth); pc.lfoDest = P(IDs::lfo1Dest);
    pc.glide = P(IDs::glideTime);     pc.bendRange = P(IDs::pitchBend); pc.velSens = P(IDs::velocitySens);
    pc.unison = P(IDs::unisonEnabled);
    pc.distDrive = P(IDs::distDrive); pc.distMix = P(IDs::distMix);
    pc.chOn = P(IDs::chorusEnabled);  pc.chRate = P(IDs::chorusRate);   pc.chDepth = P(IDs::chorusDepth); pc.chMix = P(IDs::chorusMix);
    pc.dlOn = P(IDs::delayEnabled);   pc.dlTime = P(IDs::delayTime);    pc.dlFb = P(IDs::delayFeedback);  pc.dlMix = P(IDs::delayMix);
    pc.rvOn = P(IDs::reverbEnabled);  pc.rvSize = P(IDs::reverbSize);   pc.rvDamp = P(IDs::reverbDamp);   pc.rvMix = P(IDs::reverbMix);
    pc.master = P(IDs::masterVolume); pc.presetLevel = P(IDs::presetLevel);
    pc.arpOn = P(IDs::arpEnabled);    pc.arpMode = P(IDs::arpMode);     pc.arpRate = P(IDs::arpRate);
    pc.arpGate = P(IDs::arpGate);     pc.arpOct = P(IDs::arpOctave);    pc.voiceMode = P(IDs::voiceMode);
}

void P64AudioProcessor::fillVoiceParams(VoiceParams& vp) const
{
    auto I = [](const std::atomic<float>* a) { return juce::roundToInt(a->load()); };

    vp.osc1Wave = I(pc.osc1Wave);  vp.osc1Oct = I(pc.osc1Oct);  vp.osc1Semi = I(pc.osc1Semi);
    vp.osc1Fine = pc.osc1Fine->load();  vp.osc1Level = pc.osc1Level->load();  vp.osc1PW = pc.osc1PW->load();

    vp.osc2Wave = I(pc.osc2Wave);  vp.osc2Oct = I(pc.osc2Oct);  vp.osc2Semi = I(pc.osc2Semi);
    vp.osc2Fine = pc.osc2Fine->load();  vp.osc2Level = pc.osc2Level->load();
    vp.osc2Sync = pc.osc2Sync->load() > 0.5f;
    vp.unison = pc.unison->load() > 0.5f;

    vp.subWave = I(pc.subWave);  vp.subOct = I(pc.subOct);
    vp.subLevel = pc.subLevel->load();  vp.noiseLevel = pc.noiseLevel->load();

    vp.filterType = I(pc.filterType);
    vp.cutoff = pc.cutoff->load();  vp.resonance = pc.res->load();
    vp.envAmount = pc.envAmt->load();  vp.keyTrack = pc.keyTrack->load();

    vp.ampA = pc.ampA->load();  vp.ampD = pc.ampD->load();  vp.ampS = pc.ampS->load();  vp.ampR = pc.ampR->load();
    vp.filtA = pc.filtA->load();  vp.filtD = pc.filtD->load();  vp.filtS = pc.filtS->load();  vp.filtR = pc.filtR->load();

    vp.lfoWave = I(pc.lfoWave);  vp.lfoRate = pc.lfoRate->load();
    vp.lfoDepth = pc.lfoDepth->load();  vp.lfoDest = I(pc.lfoDest);

    vp.glideTime = pc.glide->load();
    vp.pitchBendRange = pc.bendRange->load();
    vp.velocitySens = pc.velSens->load();
}

double P64AudioProcessor::getTailLengthSeconds() const
{
    // Longest audible tail of whatever is switched on: voice release, delay repeats, reverb.
    double tail = pc.ampR->load();

    if (pc.dlOn->load() > 0.5f && pc.dlMix->load() > 0.001f)
    {
        const double fb = juce::jlimit(0.05, 0.95, static_cast<double>(pc.dlFb->load()));
        const double repeats = 1.0 + std::log(0.001) / std::log(fb);      // until -60 dB
        tail += juce::jmin(15.0, pc.dlTime->load() * 1.25 * repeats);
    }

    if (pc.rvOn->load() > 0.5f && pc.rvMix->load() > 0.0005f)
        tail += 1.5 + pc.rvSize->load() * 8.0;

    return tail;
}

P64AudioProcessor::~P64AudioProcessor()
{
}

void P64AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (auto* voice : voices)
        voice->prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    glideState.lastFreq = 0.0f;

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
    lastOutputGain = juce::Decibels::decibelsToGain(pc.presetLevel->load()) * pc.master->load();
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

    // Read the parameter tree once and give every voice the same snapshot
    VoiceParams vp;
    fillVoiceParams(vp);
    for (auto* voice : voices)
        voice->applyParams(vp);

    // Process notes from the UI virtual keyboard
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // Arpeggiator Processing
    const bool arpOn = pc.arpOn->load() > 0.5f;
    const int arpM = static_cast<int>(pc.arpMode->load());
    const float arpR = pc.arpRate->load();
    const float arpG = pc.arpGate->load();
    const int arpO = static_cast<int>(pc.arpOct->load());

    // Mono/Poly voice mode: collapse held notes to a single last-note-priority
    // line before the arp/synth see them. Skipped while the arp is running,
    // since it already produces its own monophonic note stream.
    const bool monoOn = static_cast<int>(pc.voiceMode->load()) == 1;

    // Both stages rewrite the note stream. A note that started under the old
    // routing would never receive its note-off once the routing has changed (for
    // example when a preset switches the arp on while a key is held), so release
    // everything whenever the routing changes.
    if (arpOn != lastArpOn || monoOn != lastMonoOn)
    {
        synth.allNotesOff(0, true);
        arpeggiator.reset();
        monoStack.reset();
        lastArpOn = arpOn;
        lastMonoOn = monoOn;
    }

    if (monoOn && !arpOn)
        monoStack.process(midiMessages);
    else
        monoStack.reset();

    arpeggiator.process(midiMessages, buffer.getNumSamples(), arpOn, arpM, arpR, arpG, arpO);

    // Render polyphonic synth voices
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // FX chain: Distortion -> Chorus -> Delay -> Reverb (each one idles when it is off)
    distortion.setDrive(pc.distDrive->load());
    distortion.setMix(pc.distMix->load());
    distortion.process(buffer);

    chorus.setParameters(pc.chRate->load(), pc.chDepth->load(),
                         pc.chOn->load() > 0.5f ? pc.chMix->load() : 0.0f);
    chorus.process(buffer);

    delay.setParameters(pc.dlTime->load(), pc.dlFb->load(),
                        pc.dlOn->load() > 0.5f ? pc.dlMix->load() : 0.0f);
    delay.process(buffer);

    reverb.setParameters(pc.rvSize->load(), pc.rvDamp->load(),
                         pc.rvOn->load() > 0.5f ? pc.rvMix->load() : 0.0f);
    reverb.process(buffer);

    // Per-preset loudness trim (after the effects, so it does not change how hard they are driven)
    // and master volume in one gain, ramped over the block so neither clicks when changed
    const float outputGain = juce::Decibels::decibelsToGain(pc.presetLevel->load()) * pc.master->load();
    buffer.applyGainRamp(0, buffer.getNumSamples(), lastOutputGain, outputGain);
    lastOutputGain = outputGain;

    // Master Output Soft-Limiter (transparent safety ceiling preventing digital inter-sample clipping);
    // only touches the buffer when something actually exceeds full scale
    bool needsLimit = false;
    for (int ch = 0; ch < totalNumOutputChannels && ! needsLimit; ++ch)
        needsLimit = buffer.getMagnitude(ch, 0, buffer.getNumSamples()) > 1.0f;

    if (needsLimit)
    {
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
    return juce::jmax(0, currentPresetIndex);
}

void P64AudioProcessor::setCurrentProgram(int index)
{
    currentPresetIndex = index;
    userPresetLabel = juce::String();
    Presets::applyPreset(index, apvts);
}

juce::String P64AudioProcessor::getPresetLabel() const
{
    if (currentPresetIndex < 0 && userPresetLabel.isNotEmpty())
        return userPresetLabel;

    return Presets::getPresetName(juce::jmax(0, currentPresetIndex));
}

void P64AudioProcessor::setUserPresetLabel(const juce::String& name)
{
    currentPresetIndex = -1;
    userPresetLabel = name;
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
    root.setAttribute("preset", currentPresetIndex);
    if (currentPresetIndex < 0)
        root.setAttribute("presetName", userPresetLabel);

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
            UserPresets::replaceStateWithDefaults(apvts, juce::ValueTree::fromXml(*paramsXml));

        const int savedPreset = xmlState->getIntAttribute("preset", 0);
        if (savedPreset < 0 && xmlState->getStringAttribute("presetName").isNotEmpty())
            setUserPresetLabel(xmlState->getStringAttribute("presetName"));
        else
        {
            currentPresetIndex = juce::jlimit(0, Presets::getNumPresets() - 1, savedPreset);
            userPresetLabel = juce::String();
        }

        midiLearn.restoreFromXml(xmlState->getChildByName("MIDI_LEARN"));
    }
    else if (xmlState->hasTagName(apvts.state.getType()))
    {
        // Backwards-compat: state saved by an older version without MIDI-Learn persistence.
        UserPresets::replaceStateWithDefaults(apvts, juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new P64AudioProcessor();
}
