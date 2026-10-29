// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <set>
#include <functional>
#include <memory>

class MidiLearnManager
{
public:
    explicit MidiLearnManager(juce::AudioProcessorValueTreeState& state)
        : apvts(state)
    {
        // Default standard CC mappings
        ccMap[1]  = "filter_cutoff";   // Mod Wheel -> Filter Cutoff
        ccMap[71] = "filter_res";      // Resonance
        ccMap[74] = "filter_cutoff";   // Brightness
        ccMap[73] = "amp_attack";      // Attack
        ccMap[72] = "amp_release";     // Release
        ccMap[91] = "reverb_mix";      // Reverb Send
        ccMap[92] = "delay_mix";       // Delay Send
    }

    void startLearning(const juce::String& parameterId, bool isStepped = false)
    {
        const juce::ScopedLock sl(lock);
        isLearning = true;
        learningParamId = parameterId;
        learningIsStepped = isStepped;
    }

    void stopLearning()
    {
        const juce::ScopedLock sl(lock);
        isLearning = false;
        learningParamId = "";
        learningIsStepped = false;
    }

    void toggleArm(const juce::String& parameterId = "filter_cutoff", bool isStepped = false)
    {
        const juce::ScopedLock sl(lock);
        if (isLearning)
            stopLearning();
        else
            startLearning(parameterId, isStepped);
    }

    bool getIsLearning() const { return isLearning; }
    juce::String getLearningParamId() const { const juce::ScopedLock sl(lock); return learningParamId; }
    bool isLearningParam(const juce::String& paramId) const { const juce::ScopedLock sl(lock); return isLearning && (learningParamId == paramId); }

    int getCCForParam(const juce::String& parameterId) const
    {
        const juce::ScopedLock sl(lock);
        for (const auto& pair : ccMap)
        {
            if (pair.second == parameterId)
                return pair.first;
        }
        return -1;
    }

    void mapCC(int cc, const juce::String& parameterId, bool isStepped = false)
    {
        const juce::ScopedLock sl(lock);
        ccMap[cc] = parameterId;
        if (isStepped)
            steppedParamIds.insert(parameterId);
        else
            steppedParamIds.erase(parameterId);
    }

    void clearMappingForParam(const juce::String& parameterId)
    {
        const juce::ScopedLock sl(lock);
        for (auto it = ccMap.begin(); it != ccMap.end(); )
        {
            if (it->second == parameterId)
                it = ccMap.erase(it);
            else
                ++it;
        }
        steppedParamIds.erase(parameterId);
    }

    void clearAllMappings()
    {
        const juce::ScopedLock sl(lock);
        ccMap.clear();
        steppedParamIds.clear();
    }

    // Persist custom CC mappings alongside the plugin state, so learned
    // controller assignments survive a save/reload instead of silently
    // reverting to the hardcoded defaults from the constructor.
    std::unique_ptr<juce::XmlElement> createXml() const
    {
        const juce::ScopedLock sl(lock);
        auto xml = std::make_unique<juce::XmlElement>("MIDI_LEARN");
        for (const auto& pair : ccMap)
        {
            auto* entry = xml->createNewChildElement("MAP");
            entry->setAttribute("cc", pair.first);
            entry->setAttribute("param", pair.second);
            entry->setAttribute("stepped", steppedParamIds.count(pair.second) > 0);
        }
        return xml;
    }

    void restoreFromXml(const juce::XmlElement* xml)
    {
        if (xml == nullptr || !xml->hasTagName("MIDI_LEARN"))
            return;

        const juce::ScopedLock sl(lock);
        ccMap.clear();
        steppedParamIds.clear();

        for (auto* entry : xml->getChildIterator())
        {
            if (!entry->hasTagName("MAP"))
                continue;

            int cc = entry->getIntAttribute("cc");
            juce::String paramId = entry->getStringAttribute("param");
            ccMap[cc] = paramId;
            if (entry->getBoolAttribute("stepped"))
                steppedParamIds.insert(paramId);
        }
    }

    // Callbacks for preset and bank switching triggered by external button controllers
    std::function<void()> onPresetNext;
    std::function<void()> onBankNext;
    std::function<void()> onPresetPrev;

    void stepParameterForward(const juce::String& paramId)
    {
        if (paramId == "PRESET_NEXT")
        {
            if (onPresetNext) onPresetNext();
            return;
        }
        if (paramId == "BANK_NEXT")
        {
            if (onBankNext) onBankNext();
            return;
        }
        if (paramId == "PRESET_PREV")
        {
            if (onPresetPrev) onPresetPrev();
            return;
        }

        if (auto* param = apvts.getParameter(paramId))
        {
            if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
            {
                int numChoices = choiceParam->choices.size();
                if (numChoices > 0)
                {
                    int curr = choiceParam->getIndex();
                    int next = (curr + 1) % numChoices;
                    choiceParam->setValueNotifyingHost(choiceParam->getNormalisableRange().convertTo0to1(static_cast<float>(next)));
                }
            }
            else if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param))
            {
                bool curr = boolParam->get();
                boolParam->setValueNotifyingHost(curr ? 0.0f : 1.0f);
            }
            else if (auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param))
            {
                auto range = intParam->getRange();
                int curr = intParam->get();
                int next = curr + 1;
                if (next > range.getEnd())
                    next = range.getStart();
                intParam->setValueNotifyingHost(intParam->getNormalisableRange().convertTo0to1(static_cast<float>(next)));
            }
            else
            {
                float norm = param->getValue();
                float val = param->convertFrom0to1(norm);
                auto r = param->getNormalisableRange();
                float step = r.interval > 0.0f ? r.interval : 1.0f;
                float next = val + step;
                if (next > r.end)
                    next = r.start;
                param->setValueNotifyingHost(r.convertTo0to1(next));
            }
        }
    }

    // Audio thread. The maps are shared with the UI thread, so the lock is only tried: if the
    // UI holds it right now (a mapping is being edited) this block's controller messages are
    // skipped instead of waiting.
    void handleIncomingMidi(const juce::MidiBuffer& midiMessages)
    {
        if (midiMessages.isEmpty())
            return;

        const juce::ScopedTryLock sl(lock);
        if (! sl.isLocked())
            return;

        for (const auto metadata : midiMessages)
        {
            auto msg = metadata.getMessage();
            if (msg.isController())
            {
                int cc = msg.getControllerNumber();
                int ccValInt = msg.getControllerValue();
                float val = static_cast<float>(ccValInt) / 127.0f;

                if (isLearning && learningParamId.isNotEmpty())
                {
                    mapCC(cc, learningParamId, learningIsStepped);
                    stopLearning();
                    continue;
                }

                auto it = ccMap.find(cc);
                if (it != ccMap.end())
                {
                    const juce::String& paramId = it->second;
                    bool isStepped = steppedParamIds.count(paramId) > 0
                                     || paramId == "PRESET_NEXT"
                                     || paramId == "BANK_NEXT"
                                     || paramId == "PRESET_PREV";

                    if (isStepped)
                    {
                        // External button controller: trigger on press (value >= 64)
                        if (ccValInt >= 64)
                        {
                            stepParameterForward(paramId);
                        }
                    }
                    else
                    {
                        if (auto* param = apvts.getParameter(paramId))
                        {
                            param->setValueNotifyingHost(val);
                        }
                    }
                }
            }
            else if (msg.isNoteOn() && msg.getVelocity() > 0)
            {
                // Support external button controllers sending Note-On messages
                int noteCC = 200 + msg.getNoteNumber();

                if (isLearning && learningParamId.isNotEmpty() && learningIsStepped)
                {
                    mapCC(noteCC, learningParamId, true);
                    stopLearning();
                    continue;
                }

                auto it = ccMap.find(noteCC);
                if (it != ccMap.end())
                {
                    stepParameterForward(it->second);
                }
            }
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    mutable juce::CriticalSection lock;
    std::map<int, juce::String> ccMap;
    std::set<juce::String> steppedParamIds;
    bool isLearning = false;
    juce::String learningParamId;
    bool learningIsStepped = false;
};
