// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#include "P64LookAndFeel.h"

P64LookAndFeel::P64LookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, backgroundDark);
    setColour(juce::Label::textColourId, textLight);
}

void P64LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPosProportional, float rotaryStartAngle,
                                      float rotaryEndAngle, juce::Slider& slider)
{
    bool isLargeHero = (slider.getComponentID() == "HeroKnob");
    auto margin = isLargeHero ? 8.0f : 5.0f;

    float radius = 0.0f;
    if (isLargeHero)
    {
        radius = (float) juce::jmin(width / 2, height / 2) - margin;
    }
    else
    {
        // Standard knob size - precisely normalized to match OSC knobs
        // OSC knobs with width ~64px have radius = 26.5f.
        // Capping standard knob radius at 26.5f prevents 2-knob rows (LFO, ARP row 1, OUTPUT row 2)
        // from ballooning larger than the 3-knob rows.
        const float standardMaxRadius = 26.5f;
        float availableRadius = (float) juce::jmin(width / 2, height / 2) - margin;
        radius = juce::jmin(standardMaxRadius, availableRadius);
    }

    auto centreX = (float) x + (float) width * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    if (isLargeHero)
    {
        // Outer cyber glow for hero cutoff knob
        g.setColour(accentCyan.withAlpha(0.12f));
        g.fillEllipse(rx - 6.0f, ry - 6.0f, rw + 12.0f, rw + 12.0f);
        g.setColour(accentCyan.withAlpha(0.25f));
        g.drawEllipse(rx - 6.0f, ry - 6.0f, rw + 12.0f, rw + 12.0f, 1.2f);
    }

    // Knob outer bevel / body
    g.setColour(knobBorder);
    g.fillEllipse(rx - 2.0f, ry - 2.0f, rw + 4.0f, rw + 4.0f);

    juce::ColourGradient grad(knobBody.brighter(0.15f), centreX, ry, knobBody.darker(0.35f), centreX, ry + rw, false);
    g.setGradientFill(grad);
    g.fillEllipse(rx, ry, rw, rw);

    // Inner rim
    g.setColour(juce::Colour(0xff1b2636));
    g.drawEllipse(rx + 1.0f, ry + 1.0f, rw - 2.0f, rw - 2.0f, 1.0f);

    // Track background
    float trackRadius = radius + (isLargeHero ? 4.0f : 3.0f);
    juce::Path trackBg;
    trackBg.addCentredArc(centreX, centreY, trackRadius, trackRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff0d131d));
    g.strokePath(trackBg, juce::PathStrokeType(isLargeHero ? 4.5f : 3.0f));

    // Active value arc
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, trackRadius, trackRadius, 0.0f, rotaryStartAngle, angle, true);

    if (isLargeHero)
    {
        g.setColour(accentCyan.withAlpha(0.35f));
        g.strokePath(valueArc, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour(accentCyan);
    g.strokePath(valueArc, juce::PathStrokeType(isLargeHero ? 4.5f : 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer indicator needle
    juce::Path p;
    auto pointerLength = radius * (isLargeHero ? 0.70f : 0.62f);
    auto pointerThickness = isLargeHero ? 3.0f : 2.0f;
    p.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 3.0f, pointerThickness, pointerLength, 1.0f);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(accentCyan.brighter(0.5f));
    g.fillPath(p);
}

void P64LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float minSliderPos, float maxSliderPos,
                                      const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical && style != juce::Slider::LinearBarVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    auto accent = slider.findColour(juce::Slider::thumbColourId, true);
    if (accent.isTransparent())
        accent = accentCyan;

    // Track positioning
    const float trackW = 5.0f;
    const float trackX = (float) x + (float) width * 0.5f - trackW * 0.5f;
    const float trackY = (float) y + 6.0f;
    const float trackH = (float) height - 12.0f;

    // Outer track groove / bezel
    g.setColour(juce::Colour(0xff060a12));
    g.fillRoundedRectangle(trackX - 1.0f, trackY - 1.0f, trackW + 2.0f, trackH + 2.0f, 2.5f);
    g.setColour(juce::Colour(0xff152238));
    g.drawRoundedRectangle(trackX - 1.0f, trackY - 1.0f, trackW + 2.0f, trackH + 2.0f, 2.5f, 1.0f);

    // Active track fill from bottom (maxSliderPos or track bottom) up to sliderPos
    float bottomY = trackY + trackH;
    float fillTopY = juce::jlimit(trackY, bottomY, sliderPos);
    float fillH = bottomY - fillTopY;
    if (fillH > 1.0f)
    {
        g.setColour(accent.withAlpha(0.6f));
        g.fillRoundedRectangle(trackX, fillTopY, trackW, fillH, 2.0f);
    }

    // Thumb / Fader Handle Dimensions
    float thumbW = juce::jmin(28.0f, (float) width - 6.0f);
    float thumbH = 14.0f;
    float thumbX = (float) x + (float) width * 0.5f - thumbW * 0.5f;
    float thumbY = juce::jlimit(trackY, bottomY - thumbH, sliderPos - thumbH * 0.5f);

    // Fader Cap Outer Cyber Glow
    g.setColour(accent.withAlpha(0.20f));
    g.fillRoundedRectangle(thumbX - 2.0f, thumbY - 2.0f, thumbW + 4.0f, thumbH + 4.0f, 3.5f);

    // Fader Cap Border
    g.setColour(juce::Colour(0xff223147));
    g.fillRoundedRectangle(thumbX, thumbY, thumbW, thumbH, 2.5f);

    // Fader Cap Body Gradient (Brushed Dark Metallic)
    juce::ColourGradient capGrad(juce::Colour(0xff2d3b50), thumbX, thumbY, juce::Colour(0xff121824), thumbX, thumbY + thumbH, false);
    g.setGradientFill(capGrad);
    g.fillRoundedRectangle(thumbX + 1.0f, thumbY + 1.0f, thumbW - 2.0f, thumbH - 2.0f, 2.0f);

    // Top & Bottom Grip Grooves
    g.setColour(juce::Colour(0xff090e17));
    g.fillRect(thumbX + 4.0f, thumbY + 3.0f, thumbW - 8.0f, 1.0f);
    g.fillRect(thumbX + 4.0f, thumbY + thumbH - 4.0f, thumbW - 8.0f, 1.0f);

    // Center Illuminated Accent Line
    g.setColour(accent.brighter(0.4f));
    g.fillRect(thumbX + 3.0f, thumbY + thumbH * 0.5f - 1.0f, thumbW - 6.0f, 2.0f);
}

void P64LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                          const juce::Colour& backgroundColour,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour);
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto baseCol = button.getToggleState() ? accentCyan.darker(0.4f) : backgroundDark;

    if (shouldDrawButtonAsHighlighted) baseCol = baseCol.brighter(0.15f);
    if (shouldDrawButtonAsDown) baseCol = baseCol.darker(0.1f);

    g.setColour(baseCol);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(button.getToggleState() ? accentCyan : knobBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.5f);
}

void P64LookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                  int, int, int, int, juce::ComboBox& box)
{
    auto bounds = box.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(backgroundDark);
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(knobBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Arrow indicator
    juce::Path arrow;
    float arrowX = (float) width - 16.0f;
    float arrowY = (float) height * 0.5f - 2.0f;
    arrow.addTriangle(arrowX, arrowY, arrowX + 8.0f, arrowY, arrowX + 4.0f, arrowY + 5.0f);
    g.setColour(accentCyan);
    g.fillPath(arrow);
}

juce::Font P64LookAndFeel::getLabelFont(juce::Label& label)
{
    // The header title sets its own (larger) font.
    if (label.getName() == "title")
        return label.getFont();

    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
}
