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
            dcCoef = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 8.0f / static_cast<float>(sampleRate));
            reset();
        }

        void reset()
        {
            dc[0] = dc[1] = 0.0f;
            prevMix = mix;
            prevDrive = drive;
        }

        void setDrive(float newDrive) { drive = newDrive; }
        void setMix(float newMix)     { mix = newMix; }

        void process(juce::AudioBuffer<float>& buffer)
        {
            if (mix <= 0.001f && prevMix <= 0.001f)
            {
                prevMix = mix;
                prevDrive = drive;
                return;                       // fully dry, nothing to do (drive alone changes nothing)
            }

            const int numChannels = juce::jmin(2, buffer.getNumChannels());
            const int numSamples = buffer.getNumSamples();
            const float inv = 1.0f / static_cast<float>(numSamples);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* channelData = buffer.getWritePointer(ch);
                float dcState = dc[ch];
                for (int s = 0; s < numSamples; ++s)
                {
                    // mix and drive ramp over the block so moving the knobs does not click
                    const float t = static_cast<float>(s) * inv;
                    const float m = prevMix + (mix - prevMix) * t;
                    const float d = prevDrive + (drive - prevDrive) * t;

                    const float dry = channelData[s];
                    // Asymmetric soft saturator curve. The quadratic term flips sign below x = -10,
                    // which would turn big negative peaks positive, so x is limited to a safe range.
                    const float x = juce::jmax(-8.0f, dry * d);
                    float wet = std::tanh(x + 0.1f * x * x) * 0.95f;

                    // The asymmetry adds a DC offset; remove it so it can not eat headroom or click
                    dcState += (wet - dcState) * dcCoef;
                    wet -= dcState;

                    channelData[s] = dry * (1.0f - m) + wet * m;
                }
                dc[ch] = dcState;
            }
            prevMix = mix;
            prevDrive = drive;
        }

    private:
        double sampleRate = 44100.0;
        float drive = 1.0f, prevDrive = 1.0f;
        float mix = 0.0f, prevMix = 0.0f;
        float dcCoef = 0.001f;
        float dc[2] = { 0.0f, 0.0f };
    };

    class StereoDelay
    {
    public:
        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            sampleRate = static_cast<float>(spec.sampleRate);
            maxDelaySamples = static_cast<int>(sampleRate * 2.0f);
            delayBuffer.setSize(2, maxDelaySamples);
            mixCoef = 1.0f - std::exp(-1.0f / (0.005f * sampleRate));
            timeCoef = 1.0f - std::exp(-1.0f / (0.030f * sampleRate));
            reset();
        }

        void reset()
        {
            delayBuffer.clear();
            writePos = 0;
            dampFilter[0] = dampFilter[1] = 0.0f;
            active = false;
        }

        void setParameters(float timeSec, float fb, float m)
        {
            timeTarget = juce::jlimit(10.0f, static_cast<float>(maxDelaySamples - 10), timeSec * sampleRate);
            feedback = juce::jlimit(0.0f, 0.95f, fb);
            mixTarget = juce::jlimit(0.0f, 1.0f, m);
            if (! active)                      // while idle there is nothing to glide from
            {
                timeSm = timeTarget;
                mixSm = mixTarget;
            }
        }

        void process(juce::AudioBuffer<float>& buffer)
        {
            if (mixTarget <= 0.001f && mixSm <= 0.001f)
            {
                if (active)                    // switched off: forget the old echoes so they do not come back
                    reset();
                return;
            }
            active = true;

            const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
            const int numSamples = buffer.getNumSamples();
            float* io[2] = { buffer.getWritePointer(0), numChannels > 1 ? buffer.getWritePointer(1) : nullptr };
            float* line[2] = { delayBuffer.getWritePointer(0), delayBuffer.getWritePointer(1) };
            const float maxD = static_cast<float>(maxDelaySamples);

            for (int s = 0; s < numSamples; ++s)
            {
                mixSm  += (mixTarget  - mixSm)  * mixCoef;
                timeSm += (timeTarget - timeSm) * timeCoef;

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const float dry = io[ch][s];

                    // Right channel is delayed a little longer than the left (stereo spread)
                    float offsetSamples = (ch == 1) ? timeSm * 1.25f : timeSm;
                    if (offsetSamples >= maxD) offsetSamples = timeSm;

                    float readPos = static_cast<float>(writePos) - offsetSamples;
                    if (readPos < 0.0f) readPos += maxD;

                    const int iReadPos = static_cast<int>(readPos);
                    const float frac = readPos - static_cast<float>(iReadPos);
                    const int nextReadPos = (iReadPos + 1 >= maxDelaySamples) ? 0 : iReadPos + 1;

                    const float delayed = line[ch][iReadPos] * (1.0f - frac) + line[ch][nextReadPos] * frac;

                    // Lowpass damped feedback
                    dampFilter[ch] = dampFilter[ch] * 0.4f + delayed * 0.6f;
                    line[ch][writePos] = dry + dampFilter[ch] * feedback;

                    io[ch][s] = dry * (1.0f - mixSm) + delayed * mixSm;
                }

                if (++writePos >= maxDelaySamples)
                    writePos = 0;
            }
        }

    private:
        float sampleRate = 44100.0f;
        int maxDelaySamples = 88200;
        float timeTarget = 22050.0f, timeSm = 22050.0f;
        float feedback = 0.4f;
        float mixTarget = 0.0f, mixSm = 0.0f;
        float mixCoef = 0.01f, timeCoef = 0.001f;
        int writePos = 0;
        bool active = false;
        float dampFilter[2] = { 0.0f, 0.0f };
        juce::AudioBuffer<float> delayBuffer;
    };

    // Chorus and reverb: the JUCE processors smooth their own mix. They are only run while the
    // mix is audible (plus a short hold so the smoothing can finish) and are cleared when they
    // are switched off, so nothing runs - and nothing old comes back - while they are off.
    class StereoChorus
    {
    public:
        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            chorus.prepare(spec);
            holdSamples = static_cast<int>(0.25 * spec.sampleRate);
            idleSamples = holdSamples;
            active = false;
            lastRate = lastDepth = lastMix = -1.0f;
        }

        void setParameters(float rateHz, float depth, float mix)
        {
            if (rateHz != lastRate || depth != lastDepth || mix != lastMix)
            {
                chorus.setRate(rateHz);
                chorus.setDepth(depth);
                chorus.setMix(mix);
                chorus.setFeedback(0.2f);
                lastRate = rateHz; lastDepth = depth; lastMix = mix;
            }
            targetMix = mix;
        }

        void process(juce::AudioBuffer<float>& buffer)
        {
            if (targetMix > 0.0005f) { idleSamples = 0; active = true; }
            else idleSamples += buffer.getNumSamples();

            if (idleSamples >= holdSamples)
            {
                if (active) { chorus.reset(); active = false; }
                return;
            }

            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            chorus.process(context);
        }

    private:
        juce::dsp::Chorus<float> chorus;
        float lastRate = -1.0f, lastDepth = -1.0f, lastMix = -1.0f, targetMix = 0.0f;
        int holdSamples = 12000, idleSamples = 12000;
        bool active = false;
    };

    class ReverbProcessor
    {
    public:
        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            reverb.prepare(spec);
            holdSamples = static_cast<int>(0.25 * spec.sampleRate);
            idleSamples = holdSamples;
            active = false;
            lastSize = lastDamp = lastWet = -1.0f;
        }

        void setParameters(float roomSize, float damping, float wetLevel)
        {
            if (roomSize != lastSize || damping != lastDamp || wetLevel != lastWet)
            {
                juce::dsp::Reverb::Parameters params;
                params.roomSize = roomSize;
                params.damping = damping;
                params.wetLevel = wetLevel;
                params.dryLevel = 1.0f - wetLevel * 0.5f;
                params.width = 1.0f;
                reverb.setParameters(params);
                lastSize = roomSize; lastDamp = damping; lastWet = wetLevel;
            }
            targetWet = wetLevel;
        }

        void process(juce::AudioBuffer<float>& buffer)
        {
            if (targetWet > 0.0005f) { idleSamples = 0; active = true; }
            else idleSamples += buffer.getNumSamples();

            if (idleSamples >= holdSamples)
            {
                if (active) { reverb.reset(); active = false; }
                buffer.applyGain(kDryScale);      // what the reverb itself does to the dry signal (see below)
                return;
            }

            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            reverb.process(context);
        }

    private:
        // juce::Reverb multiplies the dry level by 2, so with dryLevel = 1 the dry signal is
        // +6 dB louder, also at wet = 0. The factory presets were balanced with that gain, so it
        // is kept; the bypassed (idle) reverb applies the same factor to stay level-identical.
        static constexpr float kDryScale = 2.0f;

        juce::dsp::Reverb reverb;
        float lastSize = -1.0f, lastDamp = -1.0f, lastWet = -1.0f, targetWet = 0.0f;
        int holdSamples = 12000, idleSamples = 12000;
        bool active = false;
    };
}
