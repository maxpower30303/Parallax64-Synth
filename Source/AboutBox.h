// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>

// "About" window: logo, name, version, copyright, licence and third-party notices.
class AboutContent : public juce::Component
{
public:
    AboutContent()
    {
        logo = juce::ImageCache::getFromMemory(BinaryData::P64_synth_jpg, BinaryData::P64_synth_jpgSize);

        text.setMultiLine(true, true);
        text.setReadOnly(true);
        text.setCaretVisible(false);
        text.setScrollbarsShown(true);
        text.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff080f1a));
        text.setColour(juce::TextEditor::textColourId, juce::Colour(0xffcbd5e1));
        text.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff16253c));
        text.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff16253c));
        text.setFont(juce::Font(juce::FontOptions(12.5f)));
        text.setText(getNotices(), false);
        addAndMakeVisible(text);

        link.setButtonText("Source code on GitHub");
        link.setURL(juce::URL("https://github.com/maxpower30303/Parallax64-Synth"));
        link.setColour(juce::HyperlinkButton::textColourId, juce::Colour(0xff00e5ff));
        link.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(link);

        close.setButtonText("Close");
        close.onClick = [this]()
        {
            if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
                dialog->exitModalState(0);
        };
        addAndMakeVisible(close);

        setSize(580, 540);
    }

    static void show(juce::Component* centreAround)
    {
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(new AboutContent());
        options.dialogTitle = "About Parallax64 Synth";
        options.dialogBackgroundColour = juce::Colour(0xff10141b);
        options.componentToCentreAround = centreAround;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff10141b));
        g.drawImageWithin(logo, 16, 12, 96, 82, juce::RectanglePlacement::centred);

        const int textX = 124;
        g.setColour(juce::Colour(0xff00e5ff));
        g.setFont(juce::Font(juce::FontOptions(28.0f).withStyle("Bold")));
        g.drawText("Parallax64 Synth", textX, 14, getWidth() - textX - 16, 34, juce::Justification::centredLeft);

        g.setColour(juce::Colour(0xffe2e8f0));
        g.setFont(juce::Font(juce::FontOptions(16.0f)));
        g.drawText("by Flix", textX, 48, getWidth() - textX - 16, 22, juce::Justification::centredLeft);

        g.setColour(juce::Colour(0xff94a3b8));
        g.setFont(juce::Font(juce::FontOptions(13.0f)));
        g.drawText(juce::String("Version ") + getVersion() + "     "
                       + juce::String(juce::CharPointer_UTF8("\xc2\xa9")) + " 2026 Flix",
                   textX, 72, getWidth() - textX - 16, 20, juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(16);
        area.removeFromTop(86);

        auto bottom = area.removeFromBottom(30);
        close.setBounds(bottom.removeFromRight(90));
        link.setBounds(bottom.removeFromLeft(280));

        area.removeFromBottom(10);
        text.setBounds(area);
    }

private:
    static juce::String getVersion()
    {
       #ifdef JucePlugin_VersionString
        return JucePlugin_VersionString;
       #else
        return "development build";
       #endif
    }

    static juce::String getNotices()
    {
        return juce::String(
            "LICENSE\n"
            "Parallax64 Synth is free software: you can redistribute it and/or modify it under the terms of the "
            "GNU Affero General Public License, version 3 (AGPL-3.0-only).\n\n"
            "This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even "
            "the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"
            "Anyone who distributes this software or a modified version has to pass on the source code under the same "
            "licence. The source code of this version is available at "
            "https://github.com/maxpower30303/Parallax64-Synth\n"
            "The full licence text is in the file LICENSE and at https://www.gnu.org/licenses/agpl-3.0.html\n\n"
            "THIRD-PARTY SOFTWARE\n"
            "This program is built with the following components. The full notices and licence texts are in the "
            "file THIRD_PARTY_NOTICES.txt that comes with every release.\n\n"
            "- JUCE 9, Raw Material Software Limited - used under the AGPL-3.0 option\n"
            "- Steinberg VST 3 SDK - MIT licence\n"
            "- Steinberg ASIO SDK - used under the GPL-3.0 option\n"
            "- HarfBuzz - MIT licence\n"
            "- SheenBidi - Apache-2.0 licence\n"
            "- LunaSVG and PlutoVG - MIT licence\n"
            "- libpng - PNG Reference Library licence\n"
            "- Independent JPEG Group's JPEG library\n"
            "- zlib - zlib licence\n"
            "- FLAC, libogg and libvorbis - BSD-3-Clause licence\n\n"
            "TRADEMARKS\n"
            "VST is a trademark of Steinberg Media Technologies GmbH, registered in Europe and other countries. "
            "ASIO is a trademark and software of Steinberg Media Technologies GmbH. All other names are the property "
            "of their respective owners; no affiliation with or endorsement by them is implied.\n");
    }

    juce::Image logo;
    juce::TextEditor text;
    juce::HyperlinkButton link;
    juce::TextButton close;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutContent)
};
