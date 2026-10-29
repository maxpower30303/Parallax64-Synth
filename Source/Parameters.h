// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace IDs
{
    // Oscillator 1
    inline const juce::String osc1Wave      = "osc1_wave";
    inline const juce::String osc1Octave    = "osc1_octave";
    inline const juce::String osc1Semi      = "osc1_semi";
    inline const juce::String osc1Fine      = "osc1_fine";
    inline const juce::String osc1Level     = "osc1_level";
    inline const juce::String osc1PulseWidth= "osc1_pw";

    // Oscillator 2
    inline const juce::String osc2Wave      = "osc2_wave";
    inline const juce::String osc2Octave    = "osc2_octave";
    inline const juce::String osc2Semi      = "osc2_semi";
    inline const juce::String osc2Fine      = "osc2_fine";
    inline const juce::String osc2Level     = "osc2_level";
    inline const juce::String osc2Sync      = "osc2_sync";

    // Sub Oscillator
    inline const juce::String subWave       = "sub_wave";
    inline const juce::String subOctave     = "sub_octave";
    inline const juce::String subLevel      = "sub_level";

    // Noise Generator
    inline const juce::String noiseLevel    = "noise_level";

    // Filter
    inline const juce::String filterType    = "filter_type";
    inline const juce::String filterCutoff  = "filter_cutoff";
    inline const juce::String filterRes     = "filter_res";
    inline const juce::String filterEnvAmt  = "filter_env_amt";
    inline const juce::String filterKeyTrack= "filter_key_track";

    // Amp Envelope
    inline const juce::String ampAttack     = "amp_attack";
    inline const juce::String ampDecay      = "amp_decay";
    inline const juce::String ampSustain    = "amp_sustain";
    inline const juce::String ampRelease    = "amp_release";

    // Filter Envelope
    inline const juce::String filterAttack  = "filter_attack";
    inline const juce::String filterDecay   = "filter_decay";
    inline const juce::String filterSustain = "filter_sustain";
    inline const juce::String filterRelease = "filter_release";

    // LFO 1
    inline const juce::String lfo1Wave      = "lfo1_wave";
    inline const juce::String lfo1Rate      = "lfo1_rate";
    inline const juce::String lfo1Depth     = "lfo1_depth";
    inline const juce::String lfo1Dest      = "lfo1_dest";

    // Effects
    inline const juce::String distDrive     = "dist_drive";
    inline const juce::String distMix       = "dist_mix";
    inline const juce::String chorusRate    = "chorus_rate";
    inline const juce::String chorusDepth   = "chorus_depth";
    inline const juce::String chorusMix     = "chorus_mix";
    inline const juce::String chorusEnabled = "chorus_enabled";
    inline const juce::String delayTime     = "delay_time";
    inline const juce::String delayFeedback = "delay_feedback";
    inline const juce::String delayMix      = "delay_mix";
    inline const juce::String delayEnabled  = "delay_enabled";
    inline const juce::String reverbSize    = "reverb_size";
    inline const juce::String reverbDamp    = "reverb_damp";
    inline const juce::String reverbMix     = "reverb_mix";
    inline const juce::String reverbEnabled = "reverb_enabled";

    // Master
    inline const juce::String masterVolume  = "master_volume";
    inline const juce::String glideTime     = "glide_time";

    // Arpeggiator & Performance
    inline const juce::String arpEnabled    = "arp_enabled";
    inline const juce::String arpMode       = "arp_mode";
    inline const juce::String arpRate       = "arp_rate";
    inline const juce::String arpGate       = "arp_gate";
    inline const juce::String arpOctave     = "arp_octave";
    inline const juce::String pitchBend     = "pitch_bend";
    inline const juce::String velocitySens  = "velocity_sens";

    // Voice Mode
    inline const juce::String voiceMode     = "voice_mode"; // 0 = Poly, 1 = Mono
    inline const juce::String unisonEnabled = "unison_enabled";
    inline const juce::String presetLevel   = "preset_level"; // dB, set by each preset to even out loudness; not in the UI
}

class Parameters
{
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // Waveform Choices
        juce::StringArray waves { "Saw", "Square", "Triangle", "Sine" };
        juce::StringArray subWaves { "Square", "Sine", "Triangle" };
        juce::StringArray filterTypes { "12dB LP", "24dB LP", "Highpass", "Ladder Aggressive", "Bandpass" };
        juce::StringArray lfoDestinations { "Cutoff", "Pitch", "Amp", "PWM" };

        // Osc 1
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{IDs::osc1Wave, 1}, "Osc 1 Wave", waves, 0));
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{IDs::osc1Octave, 1}, "Osc 1 Octave", -2, 2, 0));
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{IDs::osc1Semi, 1}, "Osc 1 Semi", -12, 12, 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::osc1Fine, 1}, "Osc 1 Fine", -50.0f, 50.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::osc1Level, 1}, "Osc 1 Level", 0.0f, 1.0f, 0.75f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::osc1PulseWidth, 1}, "Osc 1 Pulse Width", 0.1f, 0.9f, 0.5f));

        // Osc 2
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{IDs::osc2Wave, 1}, "Osc 2 Wave", waves, 1));
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{IDs::osc2Octave, 1}, "Osc 2 Octave", -2, 2, 0));
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{IDs::osc2Semi, 1}, "Osc 2 Semi", -12, 12, 7));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::osc2Fine, 1}, "Osc 2 Fine", -50.0f, 50.0f, 5.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::osc2Level, 1}, "Osc 2 Level", 0.0f, 1.0f, 0.6f));
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{IDs::osc2Sync, 1}, "Osc 2 Hard Sync", false));

        // Sub Osc
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{IDs::subWave, 1}, "Sub Wave", subWaves, 0));
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{IDs::subOctave, 1}, "Sub Octave", -2, -1, -1));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::subLevel, 1}, "Sub Level", 0.0f, 1.0f, 0.4f));

        // Noise
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::noiseLevel, 1}, "Noise Level", 0.0f, 1.0f, 0.0f));

        // Filter
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{IDs::filterType, 1}, "Filter Type", filterTypes, 1));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{IDs::filterCutoff, 1}, "Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 2400.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::filterRes, 1}, "Filter Resonance", 0.1f, 10.0f, 2.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::filterEnvAmt, 1}, "Filter Env Amount", -1.0f, 1.0f, 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::filterKeyTrack, 1}, "Filter Key Track", 0.0f, 1.0f, 0.5f));

        // Amp ADSR
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::ampAttack, 1}, "Amp Attack", 0.001f, 4.0f, 0.01f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::ampDecay, 1}, "Amp Decay", 0.001f, 4.0f, 0.4f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::ampSustain, 1}, "Amp Sustain", 0.0f, 1.0f, 0.7f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::ampRelease, 1}, "Amp Release", 0.001f, 6.0f, 0.5f));

        // Filter ADSR
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::filterAttack, 1}, "Filter Attack", 0.001f, 4.0f, 0.02f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::filterDecay, 1}, "Filter Decay", 0.001f, 4.0f, 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::filterSustain, 1}, "Filter Sustain", 0.0f, 1.0f, 0.3f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::filterRelease, 1}, "Filter Release", 0.001f, 6.0f, 0.4f));

        // LFO 1
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{IDs::lfo1Wave, 1}, "LFO 1 Wave", waves, 3));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{IDs::lfo1Rate, 1}, "LFO 1 Rate",
            juce::NormalisableRange<float>(0.05f, 25.0f, 0.01f, 0.4f), 3.5f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::lfo1Depth, 1}, "LFO 1 Depth", 0.0f, 1.0f, 0.2f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{IDs::lfo1Dest, 1}, "LFO 1 Dest", lfoDestinations, 0));

        // Distortion
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::distDrive, 1}, "Distortion Drive", 1.0f, 12.0f, 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::distMix, 1}, "Distortion Mix", 0.0f, 1.0f, 0.0f));

        // Chorus
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{IDs::chorusEnabled, 1}, "Chorus On/Off", true));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::chorusRate, 1}, "Chorus Rate", 0.1f, 6.0f, 1.2f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::chorusDepth, 1}, "Chorus Depth", 0.0f, 1.0f, 0.35f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::chorusMix, 1}, "Chorus Mix", 0.0f, 1.0f, 0.0f));

        // Delay
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{IDs::delayEnabled, 1}, "Delay On/Off", true));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::delayTime, 1}, "Delay Time", 0.02f, 1.5f, 0.35f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::delayFeedback, 1}, "Delay Feedback", 0.0f, 0.92f, 0.45f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::delayMix, 1}, "Delay Mix", 0.0f, 1.0f, 0.25f));

        // Reverb
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{IDs::reverbEnabled, 1}, "Reverb On/Off", true));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::reverbSize, 1}, "Reverb Room Size", 0.0f, 1.0f, 0.6f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::reverbDamp, 1}, "Reverb Damping", 0.0f, 1.0f, 0.4f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::reverbMix, 1}, "Reverb Mix", 0.0f, 1.0f, 0.2f));

        // Master
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::masterVolume, 1}, "Master Volume", 0.0f, 1.2f, 0.8f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::glideTime, 1}, "Portamento Glide", 0.0f, 0.5f, 0.0f));

        // Arpeggiator & Performance
        juce::StringArray arpModes { "Up", "Down", "Up/Down", "Random" };
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{IDs::arpEnabled, 1}, "Arp On/Off", false));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{IDs::arpMode, 1}, "Arp Mode", arpModes, 2));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::arpRate, 1}, "Arp Rate", 1.0f, 16.0f, 4.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::arpGate, 1}, "Arp Gate", 0.1f, 0.95f, 0.5f));
        params.push_back(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{IDs::arpOctave, 1}, "Arp Octave", 1, 3, 1));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::pitchBend, 1}, "Pitch Bend Range", 0.0f, 12.0f, 2.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::velocitySens, 1}, "Velocity Sens", 0.0f, 1.0f, 0.8f));

        // Voice Mode
        juce::StringArray voiceModes { "Poly", "Mono" };
        params.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{IDs::voiceMode, 1}, "Voice Mode", voiceModes, 0));
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{IDs::unisonEnabled, 1}, "Unison", false));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{IDs::presetLevel, 1}, "Preset Level",
                                                                     juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
                                                                     juce::AudioParameterFloatAttributes().withAutomatable(false)));

        return { params.begin(), params.end() };
    }
};
