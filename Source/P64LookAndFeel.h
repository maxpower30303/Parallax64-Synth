// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class P64LookAndFeel : public juce::LookAndFeel_V4
{
public:
    P64LookAndFeel();
    ~P64LookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    juce::Font getLabelFont(juce::Label&) override;

private:
    juce::Colour backgroundDark { 0xff16191f };
    juce::Colour panelBackground { 0xff1f242d };
    juce::Colour accentCyan { 0xff00e5ff };
    juce::Colour accentOrange { 0xffff7b00 };
    juce::Colour textLight { 0xffe2e8f0 };
    juce::Colour knobBody { 0xff282e3a };
    juce::Colour knobBorder { 0xff3b4455 };
};
