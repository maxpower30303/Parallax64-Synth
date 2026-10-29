// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#include "PluginEditor.h"

P64AudioProcessorEditor::P64AudioProcessorEditor(P64AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      keyboard(audioProcessor.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lookAndFeel);

    // =========================================================================
    // 1. Header Bar Components
    // =========================================================================
    titleLabel.setText(juce::String(juce::CharPointer_UTF8("\xe2\xac\xa1 PARALLAX64 SYNTH")), juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(13.5f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    addAndMakeVisible(titleLabel);

    // Banks A, B, C, D, E, F
    // A: Bass, B: Lead, C: Pad, D: Mallet, E: Arp, F: FX
    const juce::StringArray bankNames = { "A", "B", "C", "D", "E", "F" };
    const juce::StringArray bankTooltips = {
        "Bank A: Bass", "Bank B: Lead", "Bank C: Pad",
        "Bank D: Mallet", "Bank E: Arp", "Bank F: FX"
    };

    for (int b = 0; b < bankNames.size(); ++b)
    {
        auto* btn = bankButtons.add(new juce::TextButton(bankNames[b]));
        btn->setTooltip(bankTooltips[b]);
        btn->setClickingTogglesState(true);
        btn->setRadioGroupId(901);
        btn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00e5ff));
        btn->setColour(juce::TextButton::textColourOnId, juce::Colour(0xff090c10));
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121824));
        if (b == 0) btn->setToggleState(true, juce::dontSendNotification);

        btn->onClick = [this, b]() {
            selectPreset(b, currentSlot);
        };
        btn->addMouseListener(this, false);
        addAndMakeVisible(btn);
    }

    // Preset Slots 1..8
    for (int i = 0; i < 8; ++i)
    {
        auto* btn = slotButtons.add(new juce::TextButton(juce::String(i + 1)));
        btn->setClickingTogglesState(true);
        btn->setRadioGroupId(902);
        btn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00e5ff));
        btn->setColour(juce::TextButton::textColourOnId, juce::Colour(0xff090c10));
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff121824));
        if (i == 0) btn->setToggleState(true, juce::dontSendNotification);

        btn->onClick = [this, i]() {
            selectPreset(currentBank, i);
        };
        btn->addMouseListener(this, false);
        addAndMakeVisible(btn);
    }

    // Save Button (Warm Amber)
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xfff59e0b));
    saveButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff090c10));
    addAndMakeVisible(saveButton);

    // Preset Name Display
    presetNameLabel.setText(audioProcessor.getProgramName(0), juce::dontSendNotification);
    presetNameLabel.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
    presetNameLabel.setJustificationType(juce::Justification::centred);
    presetNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xff38bdf8));
    presetNameLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff080f1a));
    presetNameLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff16253c));
    presetNameLabel.addMouseListener(this, false);
    addAndMakeVisible(presetNameLabel);

    // Presets Combo (Behind the scenes / accessible)
    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
    {
        presetBox.addItem(audioProcessor.getProgramName(i), i + 1);
    }
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this]() {
        int sel = presetBox.getSelectedId() - 1;
        if (sel >= 0 && sel < audioProcessor.getNumPrograms())
        {
            selectPreset(sel / 8, sel % 8);
        }
    };
    presetBox.setVisible(false);
    addChildComponent(presetBox);

    // Wire MidiLearn callbacks for external button controllers stepping through presets & banks
    auto& ml = audioProcessor.getMidiLearn();
    ml.onPresetNext = [this]()
    {
        juce::MessageManager::callAsync([this]()
        {
            int nextSlot = (currentSlot + 1) % 8;
            int nextBank = currentBank;
            if (nextSlot == 0)
                nextBank = (currentBank + 1) % 6;
            selectPreset(nextBank, nextSlot);
        });
    };

    ml.onBankNext = [this]()
    {
        juce::MessageManager::callAsync([this]()
        {
            int nextBank = (currentBank + 1) % 6;
            selectPreset(nextBank, currentSlot);
        });
    };

    // Voice LEDs removed from top-right per UI redesign
    voiceLeds.setVisible(false);

    // =========================================================================
    // 2. Section Panels
    // =========================================================================
    addAndMakeVisible(lfoSection);
    addAndMakeVisible(perfSection);
    addAndMakeVisible(arpSection);
    addAndMakeVisible(osc1Section);
    addAndMakeVisible(osc2Section);
    addAndMakeVisible(subSection);
    addAndMakeVisible(mixerSection);
    addAndMakeVisible(filterSection);
    addAndMakeVisible(filterEnvSection);
    addAndMakeVisible(ampEnvSection);
    addAndMakeVisible(fxSection);

    // =========================================================================
    // 3. Mode Selectors
    // =========================================================================
    auto& apvts = audioProcessor.getAPVTS();
    juce::StringArray waves { "SAW", "SQU", "TRI", "SIN" };
    juce::StringArray subWaves { "SQU", "SIN", "TRI" };
    juce::StringArray lfoDests { "CUT", "PIT", "AMP", "PWM" };
    juce::StringArray fTypes { "LP 12", "LP 24", "HP", "LAD", "BP" };
    juce::StringArray voiceModes { "POLY", "MONO" };
    juce::StringArray arpModes { "UP", "DOWN", "U/D", "RND" };
    juce::StringArray onOff { "OFF", "ON" };

    lfoWaveSelector = std::make_unique<ModeSelectorBar>(apvts, IDs::lfo1Wave, waves, &audioProcessor.getMidiLearn());
    addAndMakeVisible(*lfoWaveSelector);

    lfoDestSelector = std::make_unique<ModeSelectorBar>(apvts, IDs::lfo1Dest, lfoDests, &audioProcessor.getMidiLearn());
    addAndMakeVisible(*lfoDestSelector);

    arpModeSelector = std::make_unique<ModeSelectorBar>(apvts, IDs::arpMode, arpModes, &audioProcessor.getMidiLearn());
    addAndMakeVisible(*arpModeSelector);

    arpEnableSelector = std::make_unique<ModeSelectorBar>(apvts, IDs::arpEnabled, onOff, &audioProcessor.getMidiLearn());
    addAndMakeVisible(*arpEnableSelector);

    osc1WaveSelector = std::make_unique<ModeSelectorBar>(apvts, IDs::osc1Wave, waves, &audioProcessor.getMidiLearn());
    addAndMakeVisible(*osc1WaveSelector);

    osc2WaveSelector = std::make_unique<ModeSelectorBar>(apvts, IDs::osc2Wave, waves, &audioProcessor.getMidiLearn());
    addAndMakeVisible(*osc2WaveSelector);

    subWaveSelector = std::make_unique<ModeSelectorBar>(apvts, IDs::subWave, subWaves, &audioProcessor.getMidiLearn());
    addAndMakeVisible(*subWaveSelector);

    filterTypeSelector = std::make_unique<ModeSelectorBar>(apvts, IDs::filterType, fTypes, &audioProcessor.getMidiLearn());
    addAndMakeVisible(*filterTypeSelector);

    voiceModeSelector = std::make_unique<ModeSelectorBar>(apvts, IDs::voiceMode, voiceModes, &audioProcessor.getMidiLearn());
    addAndMakeVisible(*voiceModeSelector);

    // =========================================================================
    // 4. Graphs & FX Toggles
    // =========================================================================
    addAndMakeVisible(filterEnvGraph);
    addAndMakeVisible(ampEnvGraph);

    auto setupFxButton = [this, &apvts](juce::TextButton& btn, const juce::String& text, const juce::String& paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment) {
        btn.setButtonText(text);
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00e5ff));
        btn.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff050b14));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff64748b));
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, btn);
        addAndMakeVisible(btn);
    };

    setupFxButton(chorusToggle, "CHO ON", IDs::chorusEnabled, chorusAttachment);
    setupFxButton(delayToggle,  "DLY ON", IDs::delayEnabled,  delayAttachment);
    setupFxButton(reverbToggle, "REV ON", IDs::reverbEnabled, reverbAttachment);
    setupFxButton(osc2SyncToggle, "OSC2 SYNC", IDs::osc2Sync, osc2SyncAttachment);
    setupFxButton(unisonToggle, "UNISON", IDs::unisonEnabled, unisonAttachment);

    // =========================================================================
    // 5. Rotary Knobs & ADSR Faders
    // =========================================================================
    initRotaries();

    // =========================================================================
    // 6. Virtual Piano Keyboard (Spans Full Width edge-to-edge)
    // =========================================================================
    keyboard.setAvailableRange(24, 84); // C1 to C6 (61 Keys)
    keyboard.setScrollButtonsVisible(false);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffe2e8f0));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff12161f));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0x9900e5ff));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0x3300e5ff));
    addAndMakeVisible(keyboard);

    // Window Dimensions
    setSize(1180, 710);
    startTimerHz(30);
}

P64AudioProcessorEditor::~P64AudioProcessorEditor()
{
    stopTimer();
    chorusAttachment.reset();
    delayAttachment.reset();
    reverbAttachment.reset();
    rotaries.clear();
    faders.clear();
    setLookAndFeel(nullptr);
}

void P64AudioProcessorEditor::initRotaries()
{
    auto& apvts = audioProcessor.getAPVTS();

    auto addKnob = [this, &apvts](const juce::String& key, const juce::String& label, const juce::String& paramId, bool isHero = false, juce::Colour col = juce::Colour(0xff00e5ff)) {
        rotaries[key] = std::make_unique<LabeledRotary>(label, apvts, paramId, isHero, col, &audioProcessor.getMidiLearn());
        addAndMakeVisible(*rotaries[key]);
    };

    auto addFader = [this, &apvts](const juce::String& key, const juce::String& label, const juce::String& paramId, juce::Colour col = juce::Colour(0xff00e5ff)) {
        faders[key] = std::make_unique<LabeledFader>(label, apvts, paramId, col, &audioProcessor.getMidiLearn());
        addAndMakeVisible(*faders[key]);
    };

    // Col 1: LFO & ARP / PERF
    addKnob("lfoRate", "RATE", IDs::lfo1Rate);
    addKnob("lfoDepth", "DEPTH", IDs::lfo1Depth);

    addKnob("glide", "GLIDE", IDs::glideTime, false, juce::Colour(0xff94a3b8));
    addKnob("pitchBend", "BEND", IDs::pitchBend, false, juce::Colour(0xff94a3b8));
    addKnob("arpRate", "RATE", IDs::arpRate, false, juce::Colour(0xff00e5ff));
    addKnob("arpGate", "GATE", IDs::arpGate, false, juce::Colour(0xff00e5ff));
    addKnob("arpOct", "OCT", IDs::arpOctave, false, juce::Colour(0xff00e5ff));

    // Col 2: OSC 1, OSC 2, SUB
    addKnob("osc1Oct", "OCT", IDs::osc1Octave);
    addKnob("osc1Semi", "SEMI", IDs::osc1Semi);
    addKnob("osc1Fine", "FINE", IDs::osc1Fine);
    addKnob("osc1PW", "PW", IDs::osc1PulseWidth);

    addKnob("osc2Oct", "OCT", IDs::osc2Octave);
    addKnob("osc2Semi", "SEMI", IDs::osc2Semi);
    addKnob("osc2Fine", "FINE", IDs::osc2Fine);

    addKnob("subOct", "OCT", IDs::subOctave);

    // Col 3: MIX (Compact knobs, strictly balanced)
    addKnob("mixOsc1", "OSC 1", IDs::osc1Level, false, juce::Colour(0xff00e5ff));
    addKnob("mixOsc2", "OSC 2", IDs::osc2Level, false, juce::Colour(0xff00e5ff));
    addKnob("mixSub",  "SUB",   IDs::subLevel,  false, juce::Colour(0xff38bdf8));
    addKnob("mixNoise","NOISE", IDs::noiseLevel,false, juce::Colour(0xfff59e0b));

    // Col 4: FILTER (Hero Cutoff & satellites) + FILTER ENV (ADSR Linear Faders)
    addKnob("filterCutoff", "CUTOFF", IDs::filterCutoff, true, juce::Colour(0xff00e5ff));
    addKnob("filterRes", "RESO", IDs::filterRes, false, juce::Colour(0xff00e5ff));
    addKnob("filterDrive", "DRIVE", IDs::distDrive, false, juce::Colour(0xffff9900));
    addKnob("distMix", "DIST", IDs::distMix, false, juce::Colour(0xffff9900));
    addKnob("filterEnvAmt", "ENV", IDs::filterEnvAmt, false, juce::Colour(0xff00e5ff));
    addKnob("filterKeyTrack", "KEY", IDs::filterKeyTrack, false, juce::Colour(0xff00e5ff));

    addFader("fAtk", "A", IDs::filterAttack, juce::Colour(0xff00e5ff));
    addFader("fDec", "D", IDs::filterDecay,  juce::Colour(0xff00e5ff));
    addFader("fSus", "S", IDs::filterSustain,juce::Colour(0xff00e5ff));
    addFader("fRel", "R", IDs::filterRelease,juce::Colour(0xff00e5ff));

    // Col 5: AMP ENV (ADSR Linear Faders) + OUTPUT
    addFader("aAtk", "A", IDs::ampAttack, juce::Colour(0xffff9900));
    addFader("aDec", "D", IDs::ampDecay,  juce::Colour(0xffff9900));
    addFader("aSus", "S", IDs::ampSustain,juce::Colour(0xffff9900));
    addFader("aRel", "R", IDs::ampRelease,juce::Colour(0xffff9900));

    addKnob("velSens", "VEL", IDs::velocitySens, false, juce::Colour(0xff38bdf8));
    addKnob("chorusMix", "CHORUS", IDs::chorusMix, false, juce::Colour(0xff38bdf8));
    addKnob("masterVol", "VOLUME", IDs::masterVolume, false, juce::Colour(0xff38bdf8));
    addKnob("delayMix",  "DELAY",  IDs::delayMix, false, juce::Colour(0xff64748b));
    addKnob("reverbMix", "REVERB", IDs::reverbMix, false, juce::Colour(0xff64748b));
}

void P64AudioProcessorEditor::timerCallback()
{
    // Update Envelope visualizers from parameters
    auto& apvts = audioProcessor.getAPVTS();
    filterEnvGraph.setParameters(
        *apvts.getRawParameterValue(IDs::filterAttack),
        *apvts.getRawParameterValue(IDs::filterDecay),
        *apvts.getRawParameterValue(IDs::filterSustain),
        *apvts.getRawParameterValue(IDs::filterRelease)
    );

    ampEnvGraph.setParameters(
        *apvts.getRawParameterValue(IDs::ampAttack),
        *apvts.getRawParameterValue(IDs::ampDecay),
        *apvts.getRawParameterValue(IDs::ampSustain),
        *apvts.getRawParameterValue(IDs::ampRelease)
    );

    // Update voice activity indicators
    float rms = (audioProcessor.getRMSLevel(0) + audioProcessor.getRMSLevel(1)) * 0.5f;
    int numActive = juce::jlimit(1, 16, static_cast<int>(rms * 28.0f) + 1);
    voiceLeds.setNumActive(numActive);

    // ModeSelectorBar has no attachment mechanism of its own (unlike the
    // Slider/ButtonAttachments the knobs and FX toggles use), so it never
    // finds out about parameter changes that didn't come from clicking it
    // directly - preset loads, host automation, undo/redo, MIDI Learn
    // stepping. Without this it visually freezes on the last value the user
    // clicked while the actual parameter (and the sound) has already moved
    // on, until the user happens to click that same selector again.
    lfoWaveSelector->updateSelection();
    lfoDestSelector->updateSelection();
    arpModeSelector->updateSelection();
    arpEnableSelector->updateSelection();
    osc1WaveSelector->updateSelection();
    osc2WaveSelector->updateSelection();
    subWaveSelector->updateSelection();
    filterTypeSelector->updateSelection();
    voiceModeSelector->updateSelection();
}

void P64AudioProcessorEditor::paint(juce::Graphics& g)
{
    // Dark cyber aesthetic background
    g.fillAll(juce::Colour(0xff070b12));

    // Outer subtle border
    g.setColour(juce::Colour(0xff182234));
    g.drawRect(getLocalBounds(), 1);

    // Signal Flow Routing Arrows
    g.setColour(juce::Colour(0xff475569));
    g.setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));

    if (!arrow1Area.isEmpty())
        g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x86\x92")), arrow1Area, juce::Justification::centred, false);
    if (!arrow2Area.isEmpty())
        g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x86\x92")), arrow2Area, juce::Justification::centred, false);
    if (!arrow3Area.isEmpty())
        g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x86\x92")), arrow3Area, juce::Justification::centred, false);
}

void P64AudioProcessorEditor::resized()
{
    // 1. Full-Width Bottom Piano Keyboard
    auto fullBounds = getLocalBounds();
    auto kbHeight = 84;
    auto kbArea = fullBounds.removeFromBottom(kbHeight);
    keyboard.setBounds(0, kbArea.getY(), getWidth(), kbHeight);

    // Dynamically calculate exact white key width across 36 white keys (C1 to C6)
    float kw = static_cast<float>(getWidth()) / 36.0f;
    keyboard.setKeyWidth(kw);

    // 2. Usable Work Area
    auto area = fullBounds.reduced(8);

    // =========================================================================
    // Top Bar (Header)
    // =========================================================================
    auto topBar = area.removeFromTop(36);
    titleLabel.setBounds(topBar.removeFromLeft(185));

    // Preset Menu moved completely to the right
    // Rightmost: Preset Name Readout
    presetNameLabel.setBounds(topBar.removeFromRight(155).reduced(0, 5));
    topBar.removeFromRight(6);

    // Save Button
    saveButton.setBounds(topBar.removeFromRight(48).reduced(1, 5));
    topBar.removeFromRight(8);

    // Numbered Slots 8 down to 1 (so they appear 1..8 from left to right)
    for (int i = slotButtons.size() - 1; i >= 0; --i)
    {
        slotButtons[i]->setBounds(topBar.removeFromRight(22).reduced(1, 5));
    }
    topBar.removeFromRight(8);

    // Banks F down to A (so they appear A, B, C, D, E, F from left to right)
    for (int i = bankButtons.size() - 1; i >= 0; --i)
    {
        bankButtons[i]->setBounds(topBar.removeFromRight(24).reduced(1, 5));
    }

    area.removeFromTop(6);
    area.removeFromBottom(6);

    // =========================================================================
    // 5 Columns Synth Matrix with Authentic Proportions & Routing Spacers
    // =========================================================================
    // Proportions: Col1=195px, Col2=195px, Arrow1=14px, Col3(Mix)=100px, Arrow2=14px, Col4(ADSRs)=308px, Arrow3=14px, Col5(Filter & Output)=remaining (~300px)
    int col1W = 195;
    int col2W = 195;
    int arrowW = 14;
    int col3W = 100;
    int col4W = 308;

    auto col1 = area.removeFromLeft(col1W);
    area.removeFromLeft(4);

    auto col2 = area.removeFromLeft(col2W);
    area.removeFromLeft(4);

    arrow1Area = area.removeFromLeft(arrowW);
    area.removeFromLeft(4);

    auto col3 = area.removeFromLeft(col3W);
    area.removeFromLeft(4);

    arrow2Area = area.removeFromLeft(arrowW);
    area.removeFromLeft(4);

    auto col4 = area.removeFromLeft(col4W);
    area.removeFromLeft(4);

    arrow3Area = area.removeFromLeft(arrowW);
    area.removeFromLeft(4);

    auto col5 = area; // remaining width for FILTER & OUTPUT

    int colSpacing = 6;

    // -------------------------------------------------------------------------
    // Column 1: LFO & ARP / PERF
    // -------------------------------------------------------------------------
    auto lfoArea = col1.removeFromTop(180);
    lfoSection.setBounds(lfoArea);
    auto lfoContent = lfoArea.reduced(6, 24);
    lfoWaveSelector->setBounds(lfoContent.removeFromTop(20));
    lfoContent.removeFromTop(4);

    auto lfoKnobRow = lfoContent.removeFromTop(88);
    int lfoKw = lfoKnobRow.getWidth() / 2;
    rotaries["lfoRate"]->setBounds(lfoKnobRow.removeFromLeft(lfoKw));
    rotaries["lfoDepth"]->setBounds(lfoKnobRow);

    lfoContent.removeFromTop(4);
    lfoDestSelector->setBounds(lfoContent.removeFromTop(20));

    col1.removeFromTop(colSpacing);

    // PERF (Voice-mode buttons: Poly/Mono, Osc2 Sync, Unison)
    auto perfArea = col1.removeFromTop(114);
    perfSection.setBounds(perfArea);
    auto perfContent = perfArea.reduced(6, 24); // 24px matches every other section's header clearance
    voiceModeSelector->setBounds(perfContent.removeFromTop(20));
    perfContent.removeFromTop(4);
    osc2SyncToggle.setBounds(perfContent.removeFromTop(20));
    perfContent.removeFromTop(4);
    unisonToggle.setBounds(perfContent);

    col1.removeFromTop(colSpacing);

    // ARP
    arpSection.setBounds(col1);
    auto arpContent = col1.reduced(6, 24);

    arpModeSelector->setBounds(arpContent.removeFromTop(20));
    arpContent.removeFromTop(4);
    arpEnableSelector->setBounds(arpContent.removeFromTop(20));
    arpContent.removeFromTop(6);

    auto arpRow1 = arpContent.removeFromTop(88);
    int arpR1w = arpRow1.getWidth() / 2;
    rotaries["glide"]->setBounds(arpRow1.removeFromLeft(arpR1w));
    rotaries["pitchBend"]->setBounds(arpRow1);

    arpContent.removeFromTop(6);
    auto arpRow2 = arpContent.removeFromTop(88);
    int arpR2w = arpRow2.getWidth() / 3;
    rotaries["arpRate"]->setBounds(arpRow2.removeFromLeft(arpR2w));
    rotaries["arpGate"]->setBounds(arpRow2.removeFromLeft(arpR2w));
    rotaries["arpOct"]->setBounds(arpRow2);

    // -------------------------------------------------------------------------
    // Column 2: OSC 1, OSC 2 & SUB
    // -------------------------------------------------------------------------
    int oscSectionH = (col2.getHeight() - colSpacing * 2) / 3;

    // OSC 1
    auto osc1Area = col2.removeFromTop(oscSectionH);
    osc1Section.setBounds(osc1Area);
    auto osc1Content = osc1Area.reduced(6, 24);
    osc1WaveSelector->setBounds(osc1Content.removeFromTop(20));
    osc1Content.removeFromTop(4);
    int k3w = osc1Content.getWidth() / 3;
    rotaries["osc1Oct"]->setBounds(osc1Content.removeFromLeft(k3w));
    rotaries["osc1Semi"]->setBounds(osc1Content.removeFromLeft(k3w));
    rotaries["osc1PW"]->setBounds(osc1Content);

    col2.removeFromTop(colSpacing);

    // OSC 2
    auto osc2Area = col2.removeFromTop(oscSectionH);
    osc2Section.setBounds(osc2Area);
    auto osc2Content = osc2Area.reduced(6, 24);
    osc2WaveSelector->setBounds(osc2Content.removeFromTop(20));
    osc2Content.removeFromTop(4);
    rotaries["osc2Oct"]->setBounds(osc2Content.removeFromLeft(k3w));
    rotaries["osc2Semi"]->setBounds(osc2Content.removeFromLeft(k3w));
    rotaries["osc2Fine"]->setBounds(osc2Content);

    col2.removeFromTop(colSpacing);

    // SUB OSC
    subSection.setBounds(col2);
    auto subContent = col2.reduced(6, 24);
    subWaveSelector->setBounds(subContent.removeFromTop(20));
    subContent.removeFromTop(4);
    int subKw = subContent.getWidth() / 2;
    rotaries["subOct"]->setBounds(subContent.removeFromLeft(subKw));
    rotaries["osc1Fine"]->setBounds(subContent);

    // -------------------------------------------------------------------------
    // Column 3: MIX (Knobs sized identically to AMP ENV knobs)
    // -------------------------------------------------------------------------
    mixerSection.setBounds(col3);
    auto mixContent = col3.reduced(6, 26);
    int mixKnobH = mixContent.getHeight() / 4;

    int envKnobW = (col4W - 12) / 4;
    auto placeMixerKnob = [envKnobW](std::unique_ptr<LabeledRotary>& knob, juce::Rectangle<int> slotArea) {
        int targetW = juce::jmin(slotArea.getWidth() - 2, envKnobW + 2);
        int targetH = juce::jmin(slotArea.getHeight() - 4, 102);
        knob->setBounds(slotArea.getCentreX() - targetW / 2, slotArea.getCentreY() - targetH / 2, targetW, targetH);
    };

    placeMixerKnob(rotaries["mixOsc1"], mixContent.removeFromTop(mixKnobH));
    placeMixerKnob(rotaries["mixOsc2"], mixContent.removeFromTop(mixKnobH));
    placeMixerKnob(rotaries["mixSub"],  mixContent.removeFromTop(mixKnobH));
    placeMixerKnob(rotaries["mixNoise"],mixContent);

    // -------------------------------------------------------------------------
    // Column 4: FILTER ENV (Top) & AMP ENV (Bottom) - Beide ADSR Sektionen übereinander
    // -------------------------------------------------------------------------
    int envSectionH = (col4.getHeight() - colSpacing) / 2;

    // FILTER ENV (Oben)
    auto fEnvArea = col4.removeFromTop(envSectionH);
    filterEnvSection.setBounds(fEnvArea);
    auto fEnvContent = fEnvArea.reduced(6, 24);
    filterEnvGraph.setBounds(fEnvContent.removeFromTop(62));
    fEnvContent.removeFromTop(4);
    int fEnvFaderW = fEnvContent.getWidth() / 4;
    faders["fAtk"]->setBounds(fEnvContent.removeFromLeft(fEnvFaderW));
    faders["fDec"]->setBounds(fEnvContent.removeFromLeft(fEnvFaderW));
    faders["fSus"]->setBounds(fEnvContent.removeFromLeft(fEnvFaderW));
    faders["fRel"]->setBounds(fEnvContent);

    col4.removeFromTop(colSpacing);

    // AMP ENV (Unten)
    ampEnvSection.setBounds(col4);
    auto ampContent = col4.reduced(6, 24);
    ampEnvGraph.setBounds(ampContent.removeFromTop(62));
    ampContent.removeFromTop(4);
    int ampFaderW = ampContent.getWidth() / 4;
    faders["aAtk"]->setBounds(ampContent.removeFromLeft(ampFaderW));
    faders["aDec"]->setBounds(ampContent.removeFromLeft(ampFaderW));
    faders["aSus"]->setBounds(ampContent.removeFromLeft(ampFaderW));
    faders["aRel"]->setBounds(ampContent);

    // -------------------------------------------------------------------------
    // Column 5: FILTER (Top) & OUTPUT / MASTER (Bottom) - Beide Sektionen gleich groß
    // -------------------------------------------------------------------------
    int col5SectionH = (col5.getHeight() - colSpacing) / 2;

    // FILTER (Gleich groß wie Output Sektion)
    auto fArea = col5.removeFromTop(col5SectionH);
    filterSection.setBounds(fArea);
    auto fContent = fArea.reduced(6, 24);
    filterTypeSelector->setBounds(fContent.removeFromTop(20));
    fContent.removeFromTop(6);

    // Main Filter Area: Hero Cutoff on Left (50%), 2x2 satellite knobs on Right (50%)
    int leftHeroW = static_cast<int>(fContent.getWidth() * 0.50f);
    auto heroCutoffArea = fContent.removeFromLeft(leftHeroW);
    auto satArea = fContent;

    // Center the Hero Cutoff knob
    int heroSize = juce::jmin(heroCutoffArea.getWidth() - 6, heroCutoffArea.getHeight());
    rotaries["filterCutoff"]->setBounds(heroCutoffArea.getCentreX() - heroSize / 2,
                                        heroCutoffArea.getCentreY() - heroSize / 2,
                                        heroSize, heroSize);

    // Satellites: Row 1 = Reso, Drive, Dist Mix (3 across) | Row 2 = Env, Key (2 across)
    auto satRow1 = satArea.removeFromTop(satArea.getHeight() / 2);
    auto satRow2 = satArea;
    int satThirdW = satRow1.getWidth() / 3;
    int satHalfW = satRow2.getWidth() / 2;

    rotaries["filterRes"]->setBounds(satRow1.removeFromLeft(satThirdW).reduced(2, 2));
    rotaries["filterDrive"]->setBounds(satRow1.removeFromLeft(satThirdW).reduced(2, 2));
    rotaries["distMix"]->setBounds(satRow1.reduced(2, 2));

    rotaries["filterEnvAmt"]->setBounds(satRow2.removeFromLeft(satHalfW).reduced(2, 2));
    rotaries["filterKeyTrack"]->setBounds(satRow2.reduced(2, 2));

    col5.removeFromTop(colSpacing);

    // OUTPUT & MASTER (Gleich groß wie Filter Sektion, Chorus/Delay/Reverb in Reihe mit On/Off, darunter VEL und Volume)
    fxSection.setBounds(col5);
    auto fxContent = col5.reduced(6, 24);

    // Row 1: FX Row (Chorus, Delay, Reverb mit On/Off Schaltern)
    int fxRowH = static_cast<int>((fxContent.getHeight() - 8) * 0.56f);
    auto fxRow = fxContent.removeFromTop(fxRowH);
    int fxColW = fxRow.getWidth() / 3;

    auto chCol = fxRow.removeFromLeft(fxColW).reduced(2, 0);
    chorusToggle.setBounds(chCol.removeFromTop(20).reduced(2, 0));
    chCol.removeFromTop(2);
    rotaries["chorusMix"]->setBounds(chCol);

    auto dlyCol = fxRow.removeFromLeft(fxColW).reduced(2, 0);
    delayToggle.setBounds(dlyCol.removeFromTop(20).reduced(2, 0));
    dlyCol.removeFromTop(2);
    rotaries["delayMix"]->setBounds(dlyCol);

    auto revCol = fxRow.reduced(2, 0);
    reverbToggle.setBounds(revCol.removeFromTop(20).reduced(2, 0));
    revCol.removeFromTop(2);
    rotaries["reverbMix"]->setBounds(revCol);

    fxContent.removeFromTop(8);

    // Row 2: Performance & Master Row (VEL und Volume darunter)
    auto masterRow = fxContent;
    int masterColW = masterRow.getWidth() / 2;
    rotaries["velSens"]->setBounds(masterRow.removeFromLeft(masterColW).reduced(8, 2));
    rotaries["masterVol"]->setBounds(masterRow.reduced(8, 2));
}

void P64AudioProcessorEditor::selectPreset(int bank, int slot)
{
    currentBank = juce::jlimit(0, 5, bank);
    currentSlot = juce::jlimit(0, 7, slot);

    for (int b = 0; b < bankButtons.size(); ++b)
        bankButtons[b]->setToggleState(b == currentBank, juce::dontSendNotification);

    for (int s = 0; s < slotButtons.size(); ++s)
        slotButtons[s]->setToggleState(s == currentSlot, juce::dontSendNotification);

    int presetIdx = currentBank * 8 + currentSlot;
    if (presetIdx >= 0 && presetIdx < audioProcessor.getNumPrograms())
    {
        audioProcessor.setCurrentProgram(presetIdx);
        presetNameLabel.setText(audioProcessor.getProgramName(presetIdx), juce::dontSendNotification);
        presetBox.setSelectedId(presetIdx + 1, juce::dontSendNotification);
    }
}

void P64AudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showPresetMidiLearnMenu();
    }
}

void P64AudioProcessorEditor::showPresetMidiLearnMenu()
{
    auto& ml = audioProcessor.getMidiLearn();
    juce::PopupMenu menu;

    int currentPresetCC = ml.getCCForParam("PRESET_NEXT");
    int currentBankCC = ml.getCCForParam("BANK_NEXT");

    menu.addSectionHeader("PRESETS [Button Controller MIDI Learn]");

    // Preset Stepper
    if (ml.isLearningParam("PRESET_NEXT"))
    {
        menu.addItem(1, "MIDI Learn: Aktiv für Nächstes Preset (Taste/Pad auf Controller drücken...)");
    }
    else
    {
        juce::String title = "MIDI Learn: Nächstes Preset (Button Controller schaltet 1..8 durch)";
        if (currentPresetCC >= 0)
        {
            if (currentPresetCC >= 200)
                title += " [Aktuell: Note #" + juce::String(currentPresetCC - 200) + "]";
            else
                title += " [Aktuell: CC #" + juce::String(currentPresetCC) + "]";
        }
        menu.addItem(1, title);
    }

    // Bank Stepper
    if (ml.isLearningParam("BANK_NEXT"))
    {
        menu.addItem(2, "MIDI Learn: Aktiv für Nächste Bank (Taste/Pad auf Controller drücken...)");
    }
    else
    {
        juce::String title = "MIDI Learn: Nächste Bank (Button Controller schaltet A..F durch)";
        if (currentBankCC >= 0)
        {
            if (currentBankCC >= 200)
                title += " [Aktuell: Note #" + juce::String(currentBankCC - 200) + "]";
            else
                title += " [Aktuell: CC #" + juce::String(currentBankCC) + "]";
        }
        menu.addItem(2, title);
    }

    if (currentPresetCC >= 0 || currentBankCC >= 0)
    {
        menu.addSeparator();
        menu.addItem(3, "Preset MIDI Zuweisung löschen");
    }

    menu.addSeparator();
    menu.addItem(4, "Nächstes Preset von links nach rechts laden");
    menu.addItem(5, "Nächste Bank A-F laden");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, &ml](int result)
    {
        if (result == 1)
        {
            if (ml.isLearningParam("PRESET_NEXT"))
                ml.stopLearning();
            else
                ml.startLearning("PRESET_NEXT", true);
        }
        else if (result == 2)
        {
            if (ml.isLearningParam("BANK_NEXT"))
                ml.stopLearning();
            else
                ml.startLearning("BANK_NEXT", true);
        }
        else if (result == 3)
        {
            ml.clearMappingForParam("PRESET_NEXT");
            ml.clearMappingForParam("BANK_NEXT");
        }
        else if (result == 4)
        {
            int nextSlot = (currentSlot + 1) % 8;
            int nextBank = currentBank;
            if (nextSlot == 0)
                nextBank = (currentBank + 1) % 6;
            selectPreset(nextBank, nextSlot);
        }
        else if (result == 5)
        {
            int nextBank = (currentBank + 1) % 6;
            selectPreset(nextBank, currentSlot);
        }
    });
}

