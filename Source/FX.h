// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_dsp/juce_dsp.h>

namespace FX
{
    class Distortion
    {
    public:
        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            sampleRate = spec.sampleRate;
        }

        void setDrive(float newDrive) { drive = newDrive; }
        void setMix(float newMix)     { mix = newMix; }

        void process(juce::AudioBuffer<float>& buffer)
        {
            if (mix <= 0.001f && drive <= 1.01f) return;

            const int numChannels = buffer.getNumChannels();
            const int numSamples = buffer.getNumSamples();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* channelData = buffer.getWritePointer(ch);
                for (int s = 0; s < numSamples; ++s)
                {
                    float dry = channelData[s];
                    // Asymmetric soft saturator curve
                    float x = dry * drive;
                    float wet = std::tanh(x + 0.1f * x * x) * 0.95f;
                    channelData[s] = dry * (1.0f - mix) + wet * mix;
                }
            }
        }

    private:
        double sampleRate = 44100.0;
        float drive = 1.0f;
        float mix = 0.0f;
    };

    class StereoDelay
    {
    public:
        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            sampleRate = static_cast<float>(spec.sampleRate);
            maxDelaySamples = static_cast<int>(sampleRate * 2.0f);
            delayBuffer.setSize(2, maxDelaySamples);
            delayBuffer.clear();
            writePos = 0;
        }

        void setParameters(float timeSec, float fb, float m)
        {
            delayTimeSamples = juce::jlimit(10.0f, static_cast<float>(maxDelaySamples - 10), timeSec * sampleRate);
            feedback = juce::jlimit(0.0f, 0.95f, fb);
            mix = juce::jlimit(0.0f, 1.0f, m);
        }

        void process(juce::AudioBuffer<float>& buffer)
        {
            if (mix <= 0.001f) return;

            const int numChannels = std::min(buffer.getNumChannels(), delayBuffer.getNumChannels());
            const int numSamples = buffer.getNumSamples();

            for (int s = 0; s < numSamples; ++s)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    float dry = buffer.getSample(ch, s);
                    
                    // Ping-pong delay time offset between left and right
                    float offsetSamples = (ch == 1) ? delayTimeSamples * 1.25f : delayTimeSamples;
                    if (offsetSamples >= maxDelaySamples) offsetSamples = delayTimeSamples;

                    float readPos = static_cast<float>(writePos) - offsetSamples;
                    if (readPos < 0.0f) readPos += static_cast<float>(maxDelaySamples);

                    int iReadPos = static_cast<int>(readPos);
                    float frac = readPos - static_cast<float>(iReadPos);
                    int nextReadPos = (iReadPos + 1) % maxDelaySamples;

                    float delayed = delayBuffer.getSample(ch, iReadPos) * (1.0f - frac)
                                  + delayBuffer.getSample(ch, nextReadPos) * frac;

                    // Lowpass damped feedback
                    dampFilter[ch] = dampFilter[ch] * 0.4f + delayed * 0.6f;
                    delayBuffer.setSample(ch, writePos, dry + dampFilter[ch] * feedback);

                    buffer.setSample(ch, s, dry * (1.0f - mix) + delayed * mix);
                }

                writePos = (writePos + 1) % maxDelaySamples;
            }
        }

    private:
        float sampleRate = 44100.0f;
        int maxDelaySamples = 88200;
        float delayTimeSamples = 22050.0f;
        float feedback = 0.4f;
        float mix = 0.0f;
        int writePos = 0;
        float dampFilter[2] = { 0.0f, 0.0f };
        juce::AudioBuffer<float> delayBuffer;
    };

    class StereoChorus
    {
    public:
        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            chorus.prepare(spec);
        }

        void setParameters(float rateHz, float depth, float mix)
        {
            chorus.setRate(rateHz);
            chorus.setDepth(depth);
            chorus.setMix(mix);
            chorus.setFeedback(0.2f);
        }

        void process(juce::AudioBuffer<float>& buffer)
        {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            chorus.process(context);
        }

    private:
        juce::dsp::Chorus<float> chorus;
    };

    class ReverbProcessor
    {
    public:
        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            reverb.prepare(spec);
        }

        void setParameters(float roomSize, float damping, float wetLevel)
        {
            juce::dsp::Reverb::Parameters params;
            params.roomSize = roomSize;
            params.damping = damping;
            params.wetLevel = wetLevel;
            params.dryLevel = 1.0f - wetLevel * 0.5f;
            params.width = 1.0f;
            reverb.setParameters(params);
        }

        void process(juce::AudioBuffer<float>& buffer)
        {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            reverb.process(context);
        }

    private:
        juce::dsp::Reverb reverb;
    };
}
