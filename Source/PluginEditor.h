// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "P64LookAndFeel.h"
#include "Widgets.h"

class P64AudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit P64AudioProcessorEditor(P64AudioProcessor&);
    ~P64AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void selectPreset(int bank, int slot);
    void showPresetMidiLearnMenu();
    void showSaveDialog();
    void showUserPresetMenu();

private:
    P64AudioProcessor& audioProcessor;
    P64LookAndFeel lookAndFeel;

    // Top Header UI
    juce::Label titleLabel;
    juce::ImageComponent logoImage;
    juce::OwnedArray<juce::TextButton> bankButtons;
    juce::OwnedArray<juce::TextButton> slotButtons;
    juce::ComboBox presetBox;
    juce::Label presetNameLabel;
    juce::TextButton saveButton { "SAVE" };
    VoiceLedsComponent voiceLeds;
    int currentBank = 0; // 0=A (Bass), 1=B (Lead), 2=C (Pad), 3=D (Mallet), 4=E (Arp), 5=F (FX)
    int currentSlot = 0; // 0..7 (1..8)
    juce::String currentUserPreset; // name of the loaded user preset, empty while a factory preset is active

    // 5 Columns Section Panels
    SectionPanel lfoSection { "LFO", "MOD" };
    SectionPanel perfSection { "PERF", "VOICE" };
    SectionPanel arpSection { "ARP", "ARPEGGIATOR" };

    SectionPanel osc1Section { "OSC 1", "PRIMARY" };
    SectionPanel osc2Section { "OSC 2", "SECONDARY" };
    SectionPanel subSection { "SUB OSC", "OCTAVE" };

    SectionPanel mixerSection { "MIX", "ROUTING" };

    SectionPanel filterSection { "FILTER", "ANALOG TPT" };
    SectionPanel filterEnvSection { "FILTER ENV", "ADSR" };

    SectionPanel ampEnvSection { "AMP ENV", "AMPLITUDE" };
    SectionPanel fxSection { "OUTPUT", "MASTER & FX" };

    // Selectors
    std::unique_ptr<ModeSelectorBar> lfoWaveSelector;
    std::unique_ptr<ModeSelectorBar> lfoDestSelector;
    std::unique_ptr<ModeSelectorBar> arpModeSelector;
    std::unique_ptr<ModeSelectorBar> arpEnableSelector;
    std::unique_ptr<ModeSelectorBar> osc1WaveSelector;
    std::unique_ptr<ModeSelectorBar> osc2WaveSelector;
    std::unique_ptr<ModeSelectorBar> subWaveSelector;
    std::unique_ptr<ModeSelectorBar> filterTypeSelector;
    std::unique_ptr<ModeSelectorBar> voiceModeSelector;

    // Envelopes Visualizers & Oscilloscope
    EnvelopeVisualizerComponent filterEnvGraph { juce::Colour(0xff00e5ff) };
    EnvelopeVisualizerComponent ampEnvGraph { juce::Colour(0xffff9900) };
    OscilloscopeComponent oscilloscope;

    // Rotary Knobs & ADSR Linear Faders (Managed by map / unique_ptr)
    std::map<juce::String, std::unique_ptr<LabeledRotary>> rotaries;
    std::map<juce::String, std::unique_ptr<LabeledFader>> faders;

    // FX On/Off Toggle Buttons & Attachments
    juce::TextButton chorusToggle { "CHORUS" };
    juce::TextButton delayToggle  { "DELAY" };
    juce::TextButton reverbToggle { "REVERB" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chorusAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbAttachment;

    // Perf Section
    juce::TextButton osc2SyncToggle { "OSC2 SYNC" };
    juce::TextButton unisonToggle { "UNISON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> osc2SyncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> unisonAttachment;

    // Bottom Piano Keyboard
    juce::MidiKeyboardComponent keyboard;

    // Signal Flow Routing Arrow Rectangles
    juce::Rectangle<int> arrow1Area;
    juce::Rectangle<int> arrow2Area;
    juce::Rectangle<int> arrow3Area;

    void initRotaries();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(P64AudioProcessorEditor)
};
