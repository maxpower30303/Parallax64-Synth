// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Flix

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace DSP
{
    constexpr float TWO_PI = 6.28318530717958647692f;

    // PolyBLEP residual function for antialiased hard transitions
    inline float polyBLEP(float t, float dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        else if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    enum class Waveform
    {
        Saw = 0,
        Square,
        Triangle,
        Sine
    };

    class BandlimitedOscillator
    {
    public:
        void prepare(double sampleRate)
        {
            currentSampleRate = static_cast<float>(sampleRate);
            phase = 0.0f;
        }

        void setFrequency(float freq)
        {
            frequency = juce::jlimit(1.0f, currentSampleRate * 0.49f, freq);
            phaseIncrement = frequency / currentSampleRate;
        }

        void setWaveform(Waveform newWave) { wave = newWave; }
        void setPulseWidth(float pw) { pulseWidth = juce::jlimit(0.05f, 0.95f, pw); }

        void resetPhase() { phase = 0.0f; }

        float processSample()
        {
            float out = 0.0f;
            float dt = phaseIncrement;

            switch (wave)
            {
                case Waveform::Saw:
                {
                    out = (2.0f * phase) - 1.0f;
                    out -= polyBLEP(phase, dt);
                    break;
                }
                case Waveform::Square:
                {
                    out = (phase < pulseWidth) ? 1.0f : -1.0f;
                    out += polyBLEP(phase, dt);
                    out -= polyBLEP(std::fmod(phase + (1.0f - pulseWidth), 1.0f), dt);
                    break;
                }
                case Waveform::Triangle:
                {
                    // Integrated square wave approximation with Blep
                    float sq = (phase < 0.5f) ? 1.0f : -1.0f;
                    sq += polyBLEP(phase, dt);
                    sq -= polyBLEP(std::fmod(phase + 0.5f, 1.0f), dt);
                    // Leaky integrator for triangle
                    triState = triState * 0.999f + sq * (4.0f * dt);
                    out = juce::jlimit(-1.0f, 1.0f, triState);
                    break;
                }
                case Waveform::Sine:
                default:
                {
                    out = std::sin(phase * TWO_PI);
                    break;
                }
            }

            phase += phaseIncrement;
            if (phase >= 1.0f)
                phase -= 1.0f;

            return out;
        }

    private:
        float currentSampleRate = 44100.0f;
        float frequency = 440.0f;
        float phase = 0.0f;
        float phaseIncrement = 0.01f;
        float pulseWidth = 0.5f;
        float triState = 0.0f;
        Waveform wave = Waveform::Saw;
    };

    // 4-pole transistor-ladder-style filter (24 dB/oct).
    // Each stage is a one-pole lowpass with tanh saturation, modelling the
    // soft-clipping behaviour of the original transistor ladder; resonance
    // is fed back around all four stages, matching the classic topology.
    class LadderFilter
    {
    public:
        void prepare(double sampleRate)
        {
            fs = static_cast<float>(sampleRate);
            reset();
        }

        void reset()
        {
            s1 = s2 = s3 = s4 = 0.0f;
        }

        void setCutoffFrequency(float hz)
        {
            cutoffHz = juce::jlimit(20.0f, 20000.0f, hz);
            float wcNorm = juce::jlimit(0.0001f, 0.42f, cutoffHz / fs);
            g = 1.0f - std::exp(-TWO_PI * wcNorm);
        }

        // Accepts the same 0.1 - 10.0 Q range the SVF filter uses, mapped
        // onto a 0 - ~4.2 feedback amount (4.0 is where the ladder starts
        // to self-oscillate). The aggressive mode raises that ceiling further
        // and drives the input harder, so cranking resonance pushes the
        // ladder into genuine, screaming self-oscillation instead of the
        // tanh feedback just gently compressing it.
        void setResonance(float q)
        {
            float maxFeedback = aggressive ? 4.7f : 4.2f;
            feedback = juce::jmap(juce::jlimit(0.1f, 10.0f, q), 0.1f, 10.0f, 0.0f, maxFeedback);
        }

        void setAggressive(bool shouldBeAggressive) { aggressive = shouldBeAggressive; }

        float processSample(float input)
        {
            float driveAmt = aggressive ? 1.8f : 1.0f;
            float fbSample = (input * driveAmt) - feedback * s4;

            s1 += g * (std::tanh(fbSample) - std::tanh(s1));
            s2 += g * (std::tanh(s1) - std::tanh(s2));
            s3 += g * (std::tanh(s2) - std::tanh(s3));
            s4 += g * (std::tanh(s3) - std::tanh(s4));

            if (aggressive)
                return std::tanh(s4 * 1.3f) * 0.85f;

            return s4;
        }

    private:
        float fs = 44100.0f;
        float cutoffHz = 1000.0f;
        float g = 0.1f;
        float feedback = 0.0f;
        bool aggressive = false;
        float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;
    };

    class NoiseGenerator
    {
    public:
        float processSample()
        {
            return random.nextFloat() * 2.0f - 1.0f;
        }
    private:
        juce::Random random;
    };

    class LowFrequencyOscillator
    {
    public:
        void prepare(double sampleRate)
        {
            currentSampleRate = static_cast<float>(sampleRate);
            phase = 0.0f;
        }

        void setRate(float rateHz)
        {
            rate = rateHz;
            phaseInc = rate / currentSampleRate;
        }

        void setWaveform(Waveform w) { wave = w; }

        float processSample()
        {
            float out = 0.0f;
            switch (wave)
            {
                case Waveform::Saw:      out = 1.0f - (2.0f * phase); break;
                case Waveform::Square:   out = (phase < 0.5f) ? 1.0f : -1.0f; break;
                case Waveform::Triangle: out = 2.0f * std::fabs(2.0f * (phase - std::floor(phase + 0.5f))) - 1.0f; break;
                case Waveform::Sine:
                default:                 out = std::sin(phase * TWO_PI); break;
            }

            phase += phaseInc;
            if (phase >= 1.0f) phase -= 1.0f;
            return out;
        }

    private:
        float currentSampleRate = 44100.0f;
        float rate = 2.0f;
        float phase = 0.0f;
        float phaseInc = 0.001f;
        Waveform wave = Waveform::Sine;
    };
}
