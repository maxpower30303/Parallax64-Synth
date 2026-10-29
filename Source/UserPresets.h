// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// User presets are stored as one XML file per preset in the user's application
// data folder (Windows: %APPDATA%\Parallax64\P64 Synthesizer\UserPresets). The
// file name is the preset name; the file holds a full copy of the parameter
// state, so a user preset is independent of the factory preset list.
namespace UserPresets
{
    inline const juce::String fileExtension = ".p64preset";
    inline const juce::String defaultPrefix = "UserPreset";

    inline juce::File getFolder()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Parallax64")
                   .getChildFile("P64 Synthesizer")
                   .getChildFile("UserPresets");
    }

    // Makes a string usable as a file name on every platform. Returns an empty
    // string if nothing usable is left.
    inline juce::String sanitizeName(const juce::String& raw)
    {
        juce::String s;
        for (auto c : raw)
            if (c >= 32 && juce::String("\\/:*?\"<>|").indexOfChar(c) < 0)
                s += juce::String::charToString(c);

        s = s.trim().substring(0, 48).trim();
        while (s.endsWithChar('.'))
            s = s.dropLastCharacters(1).trim();

        static const char* reserved[] = { "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5",
                                          "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4",
                                          "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
        for (auto* r : reserved)
            if (s.equalsIgnoreCase(r))
                return s + "_";

        return s;
    }

    inline juce::File getFileFor(const juce::String& name)
    {
        return getFolder().getChildFile(name + fileExtension);
    }

    inline bool exists(const juce::String& name)
    {
        return getFileFor(name).existsAsFile();
    }

    inline juce::StringArray listNames()
    {
        juce::StringArray names;
        for (auto& f : getFolder().findChildFiles(juce::File::findFiles, false, "*" + fileExtension))
            names.add(f.getFileNameWithoutExtension());

        names.sortNatural();
        return names;
    }

    // "UserPreset1", "UserPreset2", ... continuing after the highest number in use.
    inline juce::String suggestName()
    {
        int highest = 0;
        for (auto& name : listNames())
        {
            if (! name.startsWithIgnoreCase(defaultPrefix))
                continue;

            auto digits = name.substring(defaultPrefix.length());
            if (digits.isNotEmpty() && digits.length() <= 6 && digits.containsOnly("0123456789"))
                highest = juce::jmax(highest, digits.getIntValue());
        }
        return defaultPrefix + juce::String(highest + 1);
    }

    inline bool save(const juce::String& name, juce::AudioProcessorValueTreeState& apvts)
    {
        if (! getFolder().createDirectory().wasOk())
            return false;

        juce::XmlElement root("P64_USER_PRESET");
        root.setAttribute("version", 1);
        root.setAttribute("name", name);

        if (auto state = apvts.copyState().createXml())
            root.addChildElement(state.release());

        // Write to a temporary file first so a failed write can't destroy an existing preset.
        auto target = getFileFor(name);
        juce::TemporaryFile temp(target);
        if (! root.writeTo(temp.getFile()))
            return false;

        return temp.overwriteTargetFileWithTemporary();
    }

    inline bool load(const juce::String& name, juce::AudioProcessorValueTreeState& apvts)
    {
        auto xml = juce::XmlDocument::parse(getFileFor(name));
        if (xml == nullptr || ! xml->hasTagName("P64_USER_PRESET"))
            return false;

        auto* params = xml->getChildByName(apvts.state.getType());
        if (params == nullptr)
            return false;

        apvts.replaceState(juce::ValueTree::fromXml(*params));
        return true;
    }
}
