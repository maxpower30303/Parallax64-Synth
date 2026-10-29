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
            triState = 0.0f;
        }

        void setFrequency(float freq)
        {
            frequency = juce::jlimit(1.0f, currentSampleRate * 0.49f, freq);
            phaseIncrement = frequency / currentSampleRate;
        }

        void setWaveform(Waveform newWave) { wave = newWave; }
        void setPulseWidth(float pw) { pulseWidth = juce::jlimit(0.05f, 0.95f, pw); }

        void resetPhase() { phase = 0.0f; triState = 0.0f; }

        // Hard sync: true if the last processSample() call wrapped the phase, i.e. a new
        // period of this oscillator started somewhere inside that sample.
        bool didWrap() const { return wrapped; }

        // How far into the new period this oscillator already is, in samples (0..1).
        // Valid right after processSample() when didWrap() is true.
        float samplesSinceWrap() const { return phase / phaseIncrement; }

        // Restarts the period `samplesAgo` samples in the past (sub-sample accurate sync).
        void syncRestart(float samplesAgo) { phase = samplesAgo * phaseIncrement; }

        float processSample()
        {
            float out = 0.0f;
            const float dt = phaseIncrement;

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
                    float p2 = phase + (1.0f - pulseWidth);
                    if (p2 >= 1.0f) p2 -= 1.0f;
                    out -= polyBLEP(p2, dt);
                    break;
                }
                case Waveform::Triangle:
                {
                    // Integrated square wave approximation with Blep
                    float sq = (phase < 0.5f) ? 1.0f : -1.0f;
                    sq += polyBLEP(phase, dt);
                    float p2 = phase + 0.5f;
                    if (p2 >= 1.0f) p2 -= 1.0f;
                    sq -= polyBLEP(p2, dt);
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
            wrapped = phase >= 1.0f;
            if (wrapped)
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
        bool wrapped = false;
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
            t1 = t2 = t3 = t4 = 0.0f;
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

        void setAggressive(bool shouldBeAggressive)
        {
            aggressive = shouldBeAggressive;
            driveAmt = aggressive ? 1.8f : 1.0f;
        }

        // tanh of each stage state is kept between samples (t1..t4) instead of being
        // computed twice, which saves 3 of 8 tanh calls per sample and gives the
        // same result bit for bit.
        float processSample(float input)
        {
            const float u = std::tanh((input * driveAmt) - feedback * s4);

            s1 += g * (u - t1);   t1 = std::tanh(s1);
            s2 += g * (t1 - t2);  t2 = std::tanh(s2);
            s3 += g * (t2 - t3);  t3 = std::tanh(s3);
            s4 += g * (t3 - t4);  t4 = std::tanh(s4);

            if (aggressive)
                return std::tanh(s4 * 1.3f) * 0.85f;

            return s4;
        }

    private:
        float fs = 44100.0f;
        float cutoffHz = 1000.0f;
        float g = 0.1f;
        float feedback = 0.0f;
        float driveAmt = 1.0f;
        bool aggressive = false;
        float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;
        float t1 = 0.0f, t2 = 0.0f, t3 = 0.0f, t4 = 0.0f;
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

    // Low-frequency oscillator that is read once per control block and then advanced
    // by that many samples, so the (expensive) waveform is not evaluated for every sample.
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

        float value() const
        {
            switch (wave)
            {
                case Waveform::Saw:      return 1.0f - (2.0f * phase);
                case Waveform::Square:   return (phase < 0.5f) ? 1.0f : -1.0f;
                case Waveform::Triangle: return 2.0f * std::fabs(2.0f * (phase - std::floor(phase + 0.5f))) - 1.0f;
                case Waveform::Sine:
                default:                 return std::sin(phase * TWO_PI);
            }
        }

        void advance(int numSamples)
        {
            phase += phaseInc * static_cast<float>(numSamples);
            phase -= std::floor(phase);
        }

    private:
        float currentSampleRate = 44100.0f;
        float rate = 2.0f;
        float phase = 0.0f;
        float phaseInc = 0.001f;
        Waveform wave = Waveform::Sine;
    };
}
