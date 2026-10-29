// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#include "Widgets.h"

// ==============================================================================
// 1. OscilloscopeComponent
// ==============================================================================
OscilloscopeComponent::OscilloscopeComponent()
{
    displayBuffer.resize(256, 0.0f);
    startTimerHz(30);
}

OscilloscopeComponent::~OscilloscopeComponent()
{
    stopTimer();
}

void OscilloscopeComponent::setSnapshotData(const std::vector<float>& data)
{
    if (data.empty()) return;
    size_t copySize = std::min(displayBuffer.size(), data.size());
    std::copy_n(data.begin(), copySize, displayBuffer.begin());
}

void OscilloscopeComponent::timerCallback()
{
    repaint();
}

void OscilloscopeComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark inset screen
    g.setColour(juce::Colour(0xff090c10));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(0xff1f2937));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Grid lines
    g.setColour(juce::Colour(0xff121820));
    float midY = bounds.getCentreY();
    g.drawHorizontalLine(static_cast<int>(midY), bounds.getX(), bounds.getRight());
    g.drawVerticalLine(static_cast<int>(bounds.getCentreX()), bounds.getY(), bounds.getBottom());

    // Waveform curve
    juce::Path p;
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float step = w / static_cast<float>(displayBuffer.size());

    for (size_t i = 0; i < displayBuffer.size(); ++i)
    {
        float x = bounds.getX() + static_cast<float>(i) * step;
        float y = midY - displayBuffer[i] * (h * 0.44f);
        if (i == 0) p.startNewSubPath(x, y);
        else p.lineTo(x, y);
    }

    // Glow trace
    g.setColour(juce::Colour(0x3300e5ff));
    g.strokePath(p, juce::PathStrokeType(4.0f));

    // Core trace
    g.setColour(juce::Colour(0xff00e5ff));
    g.strokePath(p, juce::PathStrokeType(1.5f));
}

// ==============================================================================
// 2. EnvelopeVisualizerComponent
// ==============================================================================
EnvelopeVisualizerComponent::EnvelopeVisualizerComponent(const juce::Colour& lineColour)
    : traceColour(lineColour)
{
}

void EnvelopeVisualizerComponent::setParameters(float attack, float decay, float sustain, float release)
{
    a = juce::jlimit(0.001f, 4.0f, attack);
    d = juce::jlimit(0.001f, 4.0f, decay);
    s = juce::jlimit(0.0f, 1.0f, sustain);
    r = juce::jlimit(0.001f, 6.0f, release);
    repaint();
}

void EnvelopeVisualizerComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(2.0f);

    // Screen background
    g.setColour(juce::Colour(0xff090c10));
    g.fillRoundedRectangle(b, 4.0f);
    g.setColour(juce::Colour(0xff1a2333));
    g.drawRoundedRectangle(b, 4.0f, 1.0f);

    // Grid markings
    g.setColour(juce::Colour(0xff121820));
    g.drawHorizontalLine(static_cast<int>(b.getBottom() - b.getHeight() * s), b.getX(), b.getRight());

    // Calculate ADSR key points proportionally
    float totalTime = a + d + 1.0f + r; // 1.0s dummy sustain hold
    float startX = b.getX() + 6.0f;
    float usableW = b.getWidth() - 12.0f;
    float baselineY = b.getBottom() - 4.0f;
    float peakY = b.getY() + 4.0f;
    float sustY = baselineY - (baselineY - peakY) * s;

    float p1X = startX + (a / totalTime) * usableW;
    float p2X = p1X + (d / totalTime) * usableW;
    float p3X = p2X + (1.0f / totalTime) * usableW;
    float p4X = b.getRight() - 6.0f;

    juce::Path curve;
    curve.startNewSubPath(startX, baselineY);
    curve.lineTo(p1X, peakY);
    curve.lineTo(p2X, sustY);
    curve.lineTo(p3X, sustY);
    curve.lineTo(p4X, baselineY);

    // Filled translucent gradient under curve
    juce::Path fillP = curve;
    fillP.lineTo(p4X, baselineY);
    fillP.lineTo(startX, baselineY);
    fillP.closeSubPath();

    juce::ColourGradient grad(traceColour.withAlpha(0.25f), 0, peakY,
                              traceColour.withAlpha(0.02f), 0, baselineY, false);
    g.setGradientFill(grad);
    g.fillPath(fillP);

    // Glowing curve line
    g.setColour(traceColour.withAlpha(0.35f));
    g.strokePath(curve, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(traceColour);
    g.strokePath(curve, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// ==============================================================================
// 3. VoiceLedsComponent
// ==============================================================================
VoiceLedsComponent::VoiceLedsComponent()
{
}

void VoiceLedsComponent::setNumActive(int count)
{
    activeVoices = juce::jlimit(0, 16, count);
    repaint();
}

void VoiceLedsComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    float ledRadius = 3.0f;
    float step = b.getWidth() / 16.0f;

    for (int i = 0; i < 16; ++i)
    {
        float cx = b.getX() + (i + 0.5f) * step;
        float cy = b.getCentreY();
        bool active = (i < activeVoices);

        if (active)
        {
            g.setColour(juce::Colour(0x6600e5ff));
            g.fillEllipse(cx - ledRadius - 1.5f, cy - ledRadius - 1.5f, (ledRadius + 1.5f) * 2.0f, (ledRadius + 1.5f) * 2.0f);
            g.setColour(juce::Colour(0xff00e5ff));
            g.fillEllipse(cx - ledRadius, cy - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f);
        }
        else
        {
            g.setColour(juce::Colour(0xff1b222d));
            g.fillEllipse(cx - ledRadius, cy - ledRadius, ledRadius * 2.0f, ledRadius * 2.0f);
        }
    }
}

// ==============================================================================
// 4. SectionPanel
// ==============================================================================
SectionPanel::SectionPanel(const juce::String& title, const juce::String& badge)
    : sectionTitle(title), sectionBadge(badge)
{
}

void SectionPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    // Dark sleek chassis
    g.setColour(juce::Colour(0xff10141b));
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(juce::Colour(0xff222a38));
    g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

    // Header bar
    g.setColour(juce::Colour(0xff00e5ff));
    g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    g.drawText(sectionTitle.toUpperCase(), 10, 5, getWidth() - 20, 14, juce::Justification::left, true);

    if (sectionBadge.isNotEmpty())
    {
        g.setColour(juce::Colour(0xff64748b));
        g.setFont(juce::Font(juce::FontOptions(8.5f).withStyle("Regular")));
        g.drawText(sectionBadge.toUpperCase(), 0, 5, getWidth() - 10, 14, juce::Justification::right, true);
    }

    // Subtle divider line
    g.setColour(juce::Colour(0xff19202c));
    g.drawHorizontalLine(22, bounds.getX() + 6.0f, bounds.getRight() - 6.0f);
}

// ==============================================================================
// 5. LabeledRotary
// ==============================================================================
LabeledRotary::LabeledRotary(const juce::String& labelText,
                             juce::AudioProcessorValueTreeState& apvts,
                             const juce::String& paramId,
                             bool isHero,
                             const juce::Colour& accent,
                             MidiLearnManager* midiLearn)
    : isHeroKnob(isHero),
      labelTextStr(labelText),
      parameterId(paramId),
      accentColour(accent),
      midiLearnManager(midiLearn)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, isHero ? 60 : 46, isHero ? 16 : 13);
    slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe2e8f0));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b0e14));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff1e293b));
    slider.setComponentID(isHero ? "HeroKnob" : "StandardKnob");
    slider.setPopupMenuEnabled(false);
    slider.addMouseListener(this, false);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(isHero ? 11.0f : 9.5f).withStyle("Bold")));
    label.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    label.addMouseListener(this, false);
    addAndMakeVisible(label);

    if (apvts.getParameter(paramId) != nullptr)
    {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
    }
}

void LabeledRotary::resized()
{
    auto b = getLocalBounds();
    label.setBounds(b.removeFromBottom(isHeroKnob ? 18 : 15));
    slider.setBounds(b);
}

void LabeledRotary::paintOverChildren(juce::Graphics& g)
{
    if (midiLearnManager == nullptr) return;

    if (midiLearnManager->isLearningParam(parameterId))
    {
        // Glowing cyan learn border badge
        auto b = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(juce::Colour(0xff00e5ff).withAlpha(0.8f));
        g.drawRoundedRectangle(b, 4.0f, 1.5f);

        g.setColour(juce::Colour(0xff00e5ff));
        g.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
        g.drawText("LEARN", 2, 2, getWidth() - 4, 10, juce::Justification::topRight, false);
    }
    else
    {
        int cc = midiLearnManager->getCCForParam(parameterId);
        if (cc >= 0)
        {
            // Tiny cyan indicator dot showing mapped CC
            g.setColour(juce::Colour(0xff38bdf8));
            g.fillEllipse(getWidth() - 9.0f, 3.0f, 5.0f, 5.0f);
        }
    }
}

void LabeledRotary::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showMidiLearnMenu();
    }
}

void LabeledRotary::showMidiLearnMenu()
{
    if (midiLearnManager == nullptr) return;

    juce::PopupMenu menu;
    int currentCC = midiLearnManager->getCCForParam(parameterId);
    bool isLearningThis = midiLearnManager->isLearningParam(parameterId);

    menu.addSectionHeader(labelTextStr + " (" + parameterId + ")");

    if (isLearningThis)
    {
        menu.addItem(1, "MIDI Learn: Aktiv (Klicken zum Abbrechen)");
    }
    else
    {
        juce::String learnTitle = "MIDI Learn (Regler auf MIDI Controller bewegen)";
        if (currentCC >= 0)
            learnTitle += " [Aktuell: CC #" + juce::String(currentCC) + "]";
        menu.addItem(1, learnTitle);
    }

    if (currentCC >= 0)
    {
        menu.addItem(2, "Zuweisung löschen (CC #" + juce::String(currentCC) + ")");
    }

    menu.addSeparator();
    menu.addItem(3, "Alle MIDI Zuweisungen löschen");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this](int result)
    {
        if (midiLearnManager == nullptr) return;
        if (result == 1)
        {
            if (midiLearnManager->isLearningParam(parameterId))
                midiLearnManager->stopLearning();
            else
                midiLearnManager->startLearning(parameterId, false);
            repaint();
        }
        else if (result == 2)
        {
            midiLearnManager->clearMappingForParam(parameterId);
            repaint();
        }
        else if (result == 3)
        {
            midiLearnManager->clearAllMappings();
            repaint();
        }
    });
}

// ==============================================================================
// 6. ModeSelectorBar
// ==============================================================================
ModeSelectorBar::ModeSelectorBar(juce::AudioProcessorValueTreeState& apvts,
                                 const juce::String& paramId,
                                 const juce::StringArray& optionLabels,
                                 MidiLearnManager* midiLearn)
    : parameterId(paramId), midiLearnManager(midiLearn)
{
    param = apvts.getParameter(paramId);

    for (int i = 0; i < optionLabels.size(); ++i)
    {
        auto* btn = buttons.add(new juce::TextButton(optionLabels[i]));
        btn->setClickingTogglesState(true);
        btn->setRadioGroupId(1001 + (paramId.hashCode() & 0xffff));
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff131822));
        btn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00e5ff));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colour(0xff94a3b8));
        btn->setColour(juce::TextButton::textColourOnId, juce::Colour(0xff090c10));

        btn->onClick = [this, i]()
        {
            if (param != nullptr)
            {
                float normVal = param->getNormalisableRange().convertTo0to1(static_cast<float>(i));
                param->setValueNotifyingHost(normVal);
            }
        };

        btn->addMouseListener(this, false);
        addAndMakeVisible(btn);
    }

    updateSelection();
}

void ModeSelectorBar::updateSelection()
{
    if (param != nullptr)
    {
        int currentIndex = static_cast<int>(std::round(param->convertFrom0to1(param->getValue())));
        for (int i = 0; i < buttons.size(); ++i)
        {
            buttons[i]->setToggleState(i == currentIndex, juce::dontSendNotification);
        }
    }
}

void ModeSelectorBar::resized()
{
    auto b = getLocalBounds();
    int num = buttons.size();
    if (num == 0) return;
    int btnWidth = b.getWidth() / num;

    for (int i = 0; i < num; ++i)
    {
        buttons[i]->setBounds(i * btnWidth, 0, (i == num - 1) ? (b.getWidth() - i * btnWidth) : btnWidth - 1, b.getHeight());
    }
}

void ModeSelectorBar::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showMidiLearnMenu();
    }
}

void ModeSelectorBar::showMidiLearnMenu()
{
    if (midiLearnManager == nullptr) return;

    juce::PopupMenu menu;
    int currentCC = midiLearnManager->getCCForParam(parameterId);
    bool isLearningThis = midiLearnManager->isLearningParam(parameterId);

    menu.addSectionHeader(parameterId + " [Button Stepper]");

    if (isLearningThis)
    {
        menu.addItem(1, "MIDI Learn: Aktiv (Button/Pad auf Controller drücken...)");
    }
    else
    {
        juce::String learnTitle = "MIDI Learn: Button Controller (Weiterschalten von links nach rechts)";
        if (currentCC >= 0)
        {
            if (currentCC >= 200)
                learnTitle += " [Aktuell: Note #" + juce::String(currentCC - 200) + "]";
            else
                learnTitle += " [Aktuell: CC #" + juce::String(currentCC) + "]";
        }
        menu.addItem(1, learnTitle);
    }

    if (currentCC >= 0)
    {
        menu.addItem(2, "Zuweisung löschen");
    }

    menu.addSeparator();
    menu.addItem(3, "Jetzt weiter schalten (Nächste Option von links nach rechts)");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this](int result)
    {
        if (midiLearnManager == nullptr) return;
        if (result == 1)
        {
            if (midiLearnManager->isLearningParam(parameterId))
                midiLearnManager->stopLearning();
            else
                midiLearnManager->startLearning(parameterId, true);
        }
        else if (result == 2)
        {
            midiLearnManager->clearMappingForParam(parameterId);
        }
        else if (result == 3)
        {
            midiLearnManager->stepParameterForward(parameterId);
            updateSelection();
        }
    });
}

// ==============================================================================
// 7. LabeledFader
// ==============================================================================
LabeledFader::LabeledFader(const juce::String& labelText,
                           juce::AudioProcessorValueTreeState& apvts,
                           const juce::String& paramId,
                           const juce::Colour& accent,
                           MidiLearnManager* midiLearn)
    : labelTextStr(labelText),
      parameterId(paramId),
      accentColour(accent),
      midiLearnManager(midiLearn)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 13);
    slider.setColour(juce::Slider::thumbColourId, accent);
    slider.setColour(juce::Slider::trackColourId, accent);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe2e8f0));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff0b0e14));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff1e293b));
    slider.setComponentID("AdsrFader");
    slider.setPopupMenuEnabled(false);
    slider.addMouseListener(this, false);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
    label.setColour(juce::Label::textColourId, accent.brighter(0.2f));
    label.addMouseListener(this, false);
    addAndMakeVisible(label);

    if (apvts.getParameter(paramId) != nullptr)
    {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
    }
}

void LabeledFader::resized()
{
    auto b = getLocalBounds();
    label.setBounds(b.removeFromBottom(15));
    slider.setBounds(b);
}

void LabeledFader::paintOverChildren(juce::Graphics& g)
{
    if (midiLearnManager == nullptr) return;

    if (midiLearnManager->isLearningParam(parameterId))
    {
        auto b = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(accentColour.withAlpha(0.85f));
        g.drawRoundedRectangle(b, 4.0f, 1.5f);

        g.setColour(accentColour);
        g.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
        g.drawText("LEARN", 2, 2, getWidth() - 4, 10, juce::Justification::topRight, false);
    }
    else
    {
        int cc = midiLearnManager->getCCForParam(parameterId);
        if (cc >= 0)
        {
            g.setColour(accentColour);
            g.fillEllipse(getWidth() - 8.0f, 3.0f, 5.0f, 5.0f);
        }
    }
}

void LabeledFader::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showMidiLearnMenu();
    }
}

void LabeledFader::showMidiLearnMenu()
{
    if (midiLearnManager == nullptr) return;

    juce::PopupMenu menu;
    int currentCC = midiLearnManager->getCCForParam(parameterId);
    bool isLearningThis = midiLearnManager->isLearningParam(parameterId);

    menu.addSectionHeader(labelTextStr + " (" + parameterId + ")");

    if (isLearningThis)
    {
        menu.addItem(1, "MIDI Learn: Aktiv (Klicken zum Abbrechen)");
    }
    else
    {
        juce::String learnTitle = "MIDI Learn (Fader auf MIDI Controller bewegen)";
        if (currentCC >= 0)
            learnTitle += " [Aktuell: CC #" + juce::String(currentCC) + "]";
        menu.addItem(1, learnTitle);
    }

    if (currentCC >= 0)
    {
        menu.addItem(2, "Zuweisung löschen (CC #" + juce::String(currentCC) + ")");
    }

    menu.addSeparator();
    menu.addItem(3, "Alle MIDI Zuweisungen löschen");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this](int result)
    {
        if (midiLearnManager == nullptr) return;
        if (result == 1)
        {
            if (midiLearnManager->isLearningParam(parameterId))
                midiLearnManager->stopLearning();
            else
                midiLearnManager->startLearning(parameterId, false);
            repaint();
        }
        else if (result == 2)
        {
            midiLearnManager->clearMappingForParam(parameterId);
            repaint();
        }
        else if (result == 3)
        {
            midiLearnManager->clearAllMappings();
            repaint();
        }
    });
}

