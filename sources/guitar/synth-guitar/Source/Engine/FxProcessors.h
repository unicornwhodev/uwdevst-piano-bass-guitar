#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

#include "SinTable.h"

namespace mgs {
namespace fx {

inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

inline float tanhAntiderivative(float x) noexcept
{
    constexpr float kLogTwo = 0.6931471805599453f;
    const float ax = std::abs(x);
    if (ax < 12.0f)
        return std::log(std::cosh(x));
    return ax - kLogTwo;
}

inline float adaaTanh(float x, float previousX) noexcept
{
    const float delta = x - previousX;
    if (std::abs(delta) < 1.0e-5f)
        return std::tanh(0.5f * (x + previousX));
    return (tanhAntiderivative(x) - tanhAntiderivative(previousX)) / delta;
}

inline float hermite(float frac, float y0, float y1, float y2, float y3)
{
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

// =============================================================================
// Fractional delay line (Hermite interpolation)
// =============================================================================
class DelayLine
{
public:
    void allocate(int maxSamples)
    {
        buf.assign(static_cast<std::size_t>(maxSamples + 4), 0.0f);
        mask = static_cast<int>(buf.size());
        wp   = 0;
    }

    void clear()
    {
        std::fill(buf.begin(), buf.end(), 0.0f);
        wp = 0;
    }

    void push(float x)
    {
        buf[static_cast<std::size_t>(wp)] = x;
        if (++wp >= mask) wp = 0;
    }

    float read(float delaySamples) const
    {
        const float d = std::max(0.0f, delaySamples);
        const int   di  = static_cast<int>(d);
        const float frac = d - static_cast<float>(di);
        auto idx = [&](int offset) -> std::size_t {
            int i = wp - 1 - offset;
            while (i < 0) i += mask;
            return static_cast<std::size_t>(i % mask);
        };
        const float y0 = buf[idx(di + 1)];
        const float y1 = buf[idx(di)];
        const float y2 = buf[idx(std::max(0, di - 1))];
        const float y3 = buf[idx(std::max(0, di - 2))];
        return hermite(frac, y0, y1, y2, y3);
    }

    float readLinear(float delaySamples) const
    {
        const float d = std::max(0.0f, delaySamples);
        const int   di  = static_cast<int>(d);
        const float frac = d - static_cast<float>(di);
        auto idx = [&](int offset) -> std::size_t {
            int i = wp - 1 - offset;
            while (i < 0) i += mask;
            return static_cast<std::size_t>(i % mask);
        };
        return buf[idx(di)] + frac * (buf[idx(di + 1)] - buf[idx(di)]);
    }

private:
    std::vector<float> buf;
    int mask = 0;
    int wp   = 0;
};

// =============================================================================
// One-pole lowpass
// =============================================================================
struct OnePole
{
    float state = 0.0f;
    float process(float x, float coeff)
    {
        state += coeff * (x - state);
        state += 1e-25f;       // flush denormals
        state -= 1e-25f;
        return state;
    }
    void  clear() { state = 0.0f; }
};

// =============================================================================
// All-pass filter
// =============================================================================
class AllPass
{
public:
    void allocate(int maxLen) { delay.allocate(maxLen + 4); }
    void clear() { delay.clear(); }

    float process(float x, float delaySamples, float coeff)
    {
        const float delayed = delay.readLinear(delaySamples);
        const float y = -coeff * x + delayed;
        delay.push(x + coeff * y);
        return y;
    }

private:
    DelayLine delay;
};

// =============================================================================
// Dattorro Plate Reverb
// =============================================================================
class DattorroPlateReverb
{
public:
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = std::max(1.0, sampleRate);
        const float scale = static_cast<float>(sr / 29761.0);

        preDelay.allocate(static_cast<int>(sr * 0.1) + 16);

        for (int i = 0; i < 4; ++i)
            inDiff[i].allocate(static_cast<int>(kInDiffLen[i] * scale) + 16);

        for (int i = 0; i < 2; ++i)
        {
            tankModApf[i].allocate(static_cast<int>(kTankApfLen[i] * scale * 1.15f) + 16);
            tankDelay[i].allocate(static_cast<int>(kTankDelayLen[i] * scale) + 16);
        }

        scaleFactor = scale;
        reset();
    }

    void reset()
    {
        preDelay.clear();
        for (auto& d : inDiff) d.clear();
        for (auto& a : tankModApf) a.clear();
        for (auto& d : tankDelay) d.clear();
        for (auto& f : tankDamp) f.clear();
        for (auto& f : inBandwidth) f.clear();
        for (auto& s : tankState) s = 0.0f;
        modPhase = 0.0f;
        currentDecay = 0.55f;
        currentDamping = 0.50f;
        currentWidth = 0.80f;
        currentMix = 0.0f;
    }

    struct Params
    {
        float decay      = 0.55f;
        float damping    = 0.50f;
        float width      = 0.80f;
        float mix        = 0.25f;
        float preDelayMs = 0.0f;
    };

    void process(float* left, float* right, int numSamples, const Params& p)
    {
        const float targetDecay = clamp01(p.decay);
        const float targetDamping = clamp01(p.damping);
        const float targetWidth = clamp01(p.width);
        const float targetMix = clamp01(p.mix);
        if (targetMix <= 0.0001f && currentMix <= 0.0001f) return;

        const float decayStep = (targetDecay - currentDecay) / static_cast<float>(juce::jmax(1, numSamples));
        const float dampingStep = (targetDamping - currentDamping) / static_cast<float>(juce::jmax(1, numSamples));
        const float widthStep = (targetWidth - currentWidth) / static_cast<float>(juce::jmax(1, numSamples));
        const float mixStep = (targetMix - currentMix) / static_cast<float>(juce::jmax(1, numSamples));
        const float preDelaySamples = clamp01(p.preDelayMs / 100.0f)
                                       * static_cast<float>(sr) * 0.1f;
        const float modRate = 0.8f / static_cast<float>(sr);
        const float modDepth = 8.0f * scaleFactor;

        const float id0 = kInDiffLen[0] * scaleFactor;
        const float id1 = kInDiffLen[1] * scaleFactor;
        const float id2 = kInDiffLen[2] * scaleFactor;
        const float id3 = kInDiffLen[3] * scaleFactor;

        const float ta0 = kTankApfLen[0] * scaleFactor;
        const float ta1 = kTankApfLen[1] * scaleFactor;

        const float td0 = kTankDelayLen[0] * scaleFactor;
        const float td1 = kTankDelayLen[1] * scaleFactor;

        for (int i = 0; i < numSamples; ++i)
        {
            const float decay = 0.25f + currentDecay * 0.73f;
            const float decayDiff = decay * 0.6f + 0.1f;
            const float dampCoeff = 1.0f - currentDamping * 0.7f;
            const float bw = 0.9995f - currentDamping * 0.3f;
            const float width = currentWidth;
            const float mix = currentMix;
            const float dryL = left[i];
            const float dryR = right != nullptr ? right[i] : dryL;

            float input = (dryL + dryR) * 0.5f;
            preDelay.push(input);
            input = preDelay.readLinear(preDelaySamples);

            input = inBandwidth[0].process(input, bw);

            input = inDiff[0].process(input, id0, 0.75f);
            input = inDiff[1].process(input, id1, 0.75f);
            input = inDiff[2].process(input, id2, 0.625f);
            input = inDiff[3].process(input, id3, 0.625f);

            const float lfo = mgs::fastSin(modPhase);
            modPhase += modRate;
            if (modPhase >= 1.0f) modPhase -= 1.0f;

            float t0 = input + tankState[1] * decay;
            t0 = tankModApf[0].process(t0, ta0 + lfo * modDepth, decayDiff);
            tankDelay[0].push(t0);
            t0 = tankDelay[0].readLinear(td0);
            t0 = tankDamp[0].process(t0, dampCoeff) * decay;
            tankState[0] = t0;

            float t1 = input + tankState[0] * decay;
            t1 = tankModApf[1].process(t1, ta1 - lfo * modDepth, decayDiff);
            tankDelay[1].push(t1);
            t1 = tankDelay[1].readLinear(td1);
            t1 = tankDamp[1].process(t1, dampCoeff) * decay;
            tankState[1] = t1;

            // flush tank denormals
            for (auto& s : tankState) { s += 1e-25f; s -= 1e-25f; }

            const float wetL = tankState[0];
            const float wetR = tankState[1];

            const float wetMono = (wetL + wetR) * 0.5f;
            const float outWetL = wetMono + (wetL - wetMono) * width;
            const float outWetR = wetMono + (wetR - wetMono) * width;

            const float dry = 1.0f - mix * 0.5f;
            left[i]  = dryL * dry + outWetL * mix;
            if (right != nullptr)
                right[i] = dryR * dry + outWetR * mix;

            currentDecay += decayStep;
            currentDamping += dampingStep;
            currentWidth += widthStep;
            currentMix += mixStep;
        }

        currentDecay = targetDecay;
        currentDamping = targetDamping;
        currentWidth = targetWidth;
        currentMix = targetMix;
    }

private:
    double sr = 44100.0;
    float scaleFactor = 1.0f;

    static constexpr float kInDiffLen[4]    = { 142.0f, 107.0f, 379.0f, 277.0f };
    static constexpr float kTankApfLen[2]   = { 672.0f, 908.0f };
    static constexpr float kTankDelayLen[2] = { 4453.0f, 3720.0f };

    DelayLine preDelay;
    AllPass   inDiff[4];
    AllPass   tankModApf[2];
    DelayLine tankDelay[2];
    OnePole   tankDamp[2];
    OnePole   inBandwidth[1];
    float     tankState[2] = { 0.0f, 0.0f };
    float     modPhase = 0.0f;
    float     currentDecay = 0.55f;
    float     currentDamping = 0.50f;
    float     currentWidth = 0.80f;
    float     currentMix = 0.25f;
};

// =============================================================================
// 3-Band Parametric EQ (Low Shelf / Mid Peak / High Shelf)
// =============================================================================
class ParametricEQ3Band
{
public:
    void prepare(double sampleRate)
    {
        sr = std::max(1.0, sampleRate);
        coeffsValid = false;
        smoothedParams = Params {};
        for (auto& st : state) st = {};
    }

    void reset()
    {
        coeffsValid = false;
        smoothedParams = Params {};
        for (auto& st : state) st = {};
    }

    struct Params
    {
        float lowFreq   = 200.0f;
        float lowGainDb = 0.0f;
        float midFreq   = 1000.0f;
        float midGainDb = 0.0f;
        float midQ      = 1.0f;
        float highFreq  = 5000.0f;
        float highGainDb = 0.0f;
    };

    void process(float* left, float* right, int numSamples, const Params& p)
    {
        const Params target = sanitizeParams(p);
        if (std::abs(target.lowGainDb)  < 0.05f &&
            std::abs(target.midGainDb)  < 0.05f &&
            std::abs(target.highGainDb) < 0.05f)
            return;

        if (!coeffsValid)
            smoothedParams = target;
        else
            smoothParamsToward(smoothedParams, target, 0.35f);

        if (!coeffsValid || paramsChanged(lastCoeffParams, smoothedParams))
        {
            coeffs[0] = sanitizeCoeffs(calcLowShelf(smoothedParams.lowFreq, smoothedParams.lowGainDb));
            coeffs[1] = sanitizeCoeffs(calcPeaking(smoothedParams.midFreq, smoothedParams.midGainDb, smoothedParams.midQ));
            coeffs[2] = sanitizeCoeffs(calcHighShelf(smoothedParams.highFreq, smoothedParams.highGainDb));
            lastCoeffParams = smoothedParams;
            coeffsValid = true;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            float L = left[i];
            float R = right != nullptr ? right[i] : 0.0f;
            for (int b = 0; b < 3; ++b)
            {
                L = biquadDF2T(state[b * 2],     coeffs[b], L);
                R = biquadDF2T(state[b * 2 + 1], coeffs[b], R);
            }
            if (!std::isfinite(L))
            {
                L = 0.0f;
                for (int b = 0; b < 3; ++b)
                    state[b * 2] = {};
            }
            if (right != nullptr && !std::isfinite(R))
            {
                R = 0.0f;
                for (int b = 0; b < 3; ++b)
                    state[b * 2 + 1] = {};
            }
            left[i] = L;
            if (right != nullptr) right[i] = R;
        }
    }

private:
    struct BiquadCoeffs { float b0=1, b1=0, b2=0, a1=0, a2=0; };
    struct BiquadState  { float z1=0, z2=0; };

    static float biquadDF2T(BiquadState& st, const BiquadCoeffs& c, float x)
    {
        const float y = c.b0 * x + st.z1;
        st.z1 = c.b1 * x - c.a1 * y + st.z2;
        st.z2 = c.b2 * x - c.a2 * y;
        if (!std::isfinite(y) || !std::isfinite(st.z1) || !std::isfinite(st.z2))
        {
            st = {};
            return 0.0f;
        }
        return y;
    }

    Params sanitizeParams(Params p) const noexcept
    {
        const float maxFreq = std::max(20.0f, static_cast<float>(sr) * 0.45f);
        p.lowFreq = std::clamp(p.lowFreq, 20.0f, maxFreq);
        p.midFreq = std::clamp(p.midFreq, 20.0f, maxFreq);
        p.highFreq = std::clamp(p.highFreq, 20.0f, maxFreq);
        p.lowGainDb = std::clamp(p.lowGainDb, -18.0f, 18.0f);
        p.midGainDb = std::clamp(p.midGainDb, -18.0f, 18.0f);
        p.highGainDb = std::clamp(p.highGainDb, -18.0f, 18.0f);
        p.midQ = std::clamp(p.midQ, 0.10f, 8.0f);
        return p;
    }

    static bool paramsChanged(const Params& a, const Params& b) noexcept
    {
        return std::abs(a.lowFreq - b.lowFreq) > 0.5f
            || std::abs(a.lowGainDb - b.lowGainDb) > 0.01f
            || std::abs(a.midFreq - b.midFreq) > 0.5f
            || std::abs(a.midGainDb - b.midGainDb) > 0.01f
            || std::abs(a.midQ - b.midQ) > 0.001f
            || std::abs(a.highFreq - b.highFreq) > 0.5f
            || std::abs(a.highGainDb - b.highGainDb) > 0.01f;
    }

    static void smoothParamsToward(Params& current, const Params& target, float amount) noexcept
    {
        current.lowFreq += (target.lowFreq - current.lowFreq) * amount;
        current.lowGainDb += (target.lowGainDb - current.lowGainDb) * amount;
        current.midFreq += (target.midFreq - current.midFreq) * amount;
        current.midGainDb += (target.midGainDb - current.midGainDb) * amount;
        current.midQ += (target.midQ - current.midQ) * amount;
        current.highFreq += (target.highFreq - current.highFreq) * amount;
        current.highGainDb += (target.highGainDb - current.highGainDb) * amount;
    }

    static BiquadCoeffs sanitizeCoeffs(BiquadCoeffs c) noexcept
    {
        if (!std::isfinite(c.b0) || !std::isfinite(c.b1) || !std::isfinite(c.b2)
            || !std::isfinite(c.a1) || !std::isfinite(c.a2))
            return {};
        return c;
    }

    BiquadCoeffs calcLowShelf(float freq, float gainDb) const
    {
        freq = std::clamp(freq, 20.0f, static_cast<float>(sr) * 0.45f);
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sr);
        const float cosw = std::cos(w0), sinw = std::sin(w0);
        const float alpha = sinw / (2.0f * 0.707f);
        const float sqA = std::sqrt(A);
        const float a0 = (A + 1.0f) + (A - 1.0f) * cosw + 2.0f * sqA * alpha;
        BiquadCoeffs c;
        c.b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw + 2.0f * sqA * alpha) / a0;
        c.b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw) / a0;
        c.b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw - 2.0f * sqA * alpha) / a0;
        c.a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw) / a0;
        c.a2 = ((A + 1.0f) + (A - 1.0f) * cosw - 2.0f * sqA * alpha) / a0;
        return c;
    }

    BiquadCoeffs calcPeaking(float freq, float gainDb, float Q) const
    {
        freq = std::clamp(freq, 20.0f, static_cast<float>(sr) * 0.45f);
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sr);
        const float cosw = std::cos(w0), sinw = std::sin(w0);
        const float alpha = sinw / (2.0f * std::max(0.01f, Q));
        const float a0 = 1.0f + alpha / A;
        BiquadCoeffs c;
        c.b0 = (1.0f + alpha * A) / a0;
        c.b1 = (-2.0f * cosw) / a0;
        c.b2 = (1.0f - alpha * A) / a0;
        c.a1 = c.b1;
        c.a2 = (1.0f - alpha / A) / a0;
        return c;
    }

    BiquadCoeffs calcHighShelf(float freq, float gainDb) const
    {
        freq = std::clamp(freq, 20.0f, static_cast<float>(sr) * 0.45f);
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sr);
        const float cosw = std::cos(w0), sinw = std::sin(w0);
        const float alpha = sinw / (2.0f * 0.707f);
        const float sqA = std::sqrt(A);
        const float a0 = (A + 1.0f) - (A - 1.0f) * cosw + 2.0f * sqA * alpha;
        BiquadCoeffs c;
        c.b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw + 2.0f * sqA * alpha) / a0;
        c.b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw) / a0;
        c.b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw - 2.0f * sqA * alpha) / a0;
        c.a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw) / a0;
        c.a2 = ((A + 1.0f) - (A - 1.0f) * cosw - 2.0f * sqA * alpha) / a0;
        return c;
    }

    double sr = 44100.0;
    BiquadCoeffs coeffs[3];
    BiquadState  state[6];
    Params lastCoeffParams {};
    Params smoothedParams {};
    bool coeffsValid = false;
};

// =============================================================================
// Stereo Chorus (modulated delay with quadrature LFO)
// =============================================================================
class StereoChorus
{
public:
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = std::max(1.0, sampleRate);
        const int maxDelaySamples = static_cast<int>(sr * 0.05) + 16;
        for (auto& d : delay) d.allocate(maxDelaySamples);
        reset();
    }

    void reset()
    {
        for (auto& d : delay) d.clear();
        lfoPhase[0] = 0.0f;
        lfoPhase[1] = 0.25f;
        currentRateHz = 1.0f;
        currentDepth = 0.5f;
        currentMix = 0.0f;
    }

    struct Params
    {
        float rateHz = 1.0f;
        float depth  = 0.5f;
        float mix    = 0.0f;
    };

    void process(float* left, float* right, int numSamples, const Params& p)
    {
        const float targetMix = clamp01(p.mix);
        if (targetMix <= 0.0001f && currentMix <= 0.0001f) return;

        const float targetRate = std::max(0.01f, p.rateHz);
        const float targetDepth = clamp01(p.depth);
        const float mixStep = (targetMix - currentMix) / static_cast<float>(juce::jmax(1, numSamples));
        const float rateStep = (targetRate - currentRateHz) / static_cast<float>(juce::jmax(1, numSamples));
        const float depthStep = (targetDepth - currentDepth) / static_cast<float>(juce::jmax(1, numSamples));

        const int numCh = (right != nullptr) ? 2 : 1;
        float* ch[2] = { left, right };

        for (int i = 0; i < numSamples; ++i)
        {
            const float phaseInc = currentRateHz / static_cast<float>(sr);
            const float baseDelay = 0.007f * static_cast<float>(sr);
            const float modAmt = currentDepth * 0.003f * static_cast<float>(sr);
            for (int c = 0; c < numCh; ++c)
            {
                const float lfo = mgs::fastSin(lfoPhase[c]);
                const float delaySamples = baseDelay + lfo * modAmt;

                delay[c].push(ch[c][i]);
                const float wet = delay[c].read(delaySamples);
                ch[c][i] = ch[c][i] * (1.0f - currentMix) + wet * currentMix;
            }

            lfoPhase[0] += phaseInc;
            if (lfoPhase[0] >= 1.0f) lfoPhase[0] -= 1.0f;
            lfoPhase[1] += phaseInc;
            if (lfoPhase[1] >= 1.0f) lfoPhase[1] -= 1.0f;
            currentRateHz += rateStep;
            currentDepth += depthStep;
            currentMix += mixStep;
        }

        currentRateHz = targetRate;
        currentDepth = targetDepth;
        currentMix = targetMix;
    }

private:
    double sr = 44100.0;
    DelayLine delay[2];
    float lfoPhase[2] = { 0.0f, 0.25f };
    float currentRateHz = 1.0f;
    float currentDepth = 0.5f;
    float currentMix = 0.0f;
};

// =============================================================================
// Stereo Delay with optional BPM sync
// =============================================================================
class StereoDelay
{
public:
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = std::max(1.0, sampleRate);
        const int maxDelaySamples = static_cast<int>(sr * 2.0) + 16;
        for (auto& d : delay) d.allocate(maxDelaySamples);
        reset();
    }

    void reset()
    {
        for (auto& d : delay) d.clear();
        currentDelaySamples = static_cast<float>(sr) * 0.3f;
        currentFeedback = 0.30f;
        currentMix = 0.0f;
    }

    struct Params
    {
        float timeMs     = 300.0f;
        float feedback   = 0.30f;
        float mix        = 0.0f;
        bool  syncToBpm  = false;
        float bpm        = 120.0f;
        int   noteDiv    = 0;
    };

    void process(float* left, float* right, int numSamples, const Params& p)
    {
        const float targetMix = clamp01(p.mix);
        if (targetMix <= 0.0001f && currentMix <= 0.0001f) return;

        float targetDelaySamples;
        if (p.syncToBpm && p.bpm > 20.0f)
        {
            const float beatSec = 60.0f / std::max(20.0f, p.bpm);
            float mult = 1.0f;
            switch (p.noteDiv)
            {
                case 1: mult = 0.5f;    break;
                case 2: mult = 0.75f;   break;
                case 3: mult = 1.0f/3.0f; break;
                case 4: mult = 0.25f;   break;
                case 5: mult = 0.375f;  break;
                default: mult = 1.0f;   break;
            }
            targetDelaySamples = beatSec * mult * static_cast<float>(sr);
        }
        else
        {
            targetDelaySamples = std::max(1.0f, p.timeMs) * 0.001f * static_cast<float>(sr);
        }

        const float maxDelay = static_cast<float>(sr) * 2.0f - 2.0f;
        targetDelaySamples = std::min(targetDelaySamples, maxDelay);
        const float targetFeedback = std::min(0.95f, std::max(0.0f, p.feedback));
        const float delayStep = (targetDelaySamples - currentDelaySamples) / static_cast<float>(juce::jmax(1, numSamples));
        const float feedbackStep = (targetFeedback - currentFeedback) / static_cast<float>(juce::jmax(1, numSamples));
        const float mixStep = (targetMix - currentMix) / static_cast<float>(juce::jmax(1, numSamples));
        const int numCh = (right != nullptr) ? 2 : 1;
        float* ch[2] = { left, right };

        for (int i = 0; i < numSamples; ++i)
        {
            for (int c = 0; c < numCh; ++c)
            {
                const float delayed = delay[c].readLinear(currentDelaySamples);
                delay[c].push(ch[c][i] + delayed * currentFeedback);
                ch[c][i] = ch[c][i] * (1.0f - currentMix) + delayed * currentMix;
            }

            currentDelaySamples += delayStep;
            currentFeedback += feedbackStep;
            currentMix += mixStep;
        }

        currentDelaySamples = targetDelaySamples;
        currentFeedback = targetFeedback;
        currentMix = targetMix;
    }

private:
    double sr = 44100.0;
    DelayLine delay[2];
    float currentDelaySamples = 0.0f;
    float currentFeedback = 0.30f;
    float currentMix = 0.0f;
};

// =============================================================================
// Output Limiter (feed-forward brick-wall)
// =============================================================================
class OutputLimiter
{
public:
    void prepare(double sampleRate)
    {
        sr = std::max(1.0, sampleRate);
        reset();
    }

    void reset()
    {
        envL = 0.0f;
        envR = 0.0f;
    }

    struct Params
    {
        float thresholdDb = -0.3f;
        float releaseMs   = 50.0f;
    };

    void process(float* left, float* right, int numSamples, const Params& p)
    {
        const float thresh = std::pow(10.0f, std::min(0.0f, p.thresholdDb) / 20.0f);
        if (thresh >= 0.9999f) return;

        const float relCoeff = std::exp(-1.0f / (std::max(1.0f, p.releaseMs) * 0.001f
                                                  * static_cast<float>(sr)));

        for (int i = 0; i < numSamples; ++i)
        {
            {
                const float absL = std::abs(left[i]);
                if (absL > envL)
                    envL = absL;
                else
                    envL = relCoeff * envL + (1.0f - relCoeff) * absL;

                if (envL > thresh)
                    left[i] *= thresh / envL;
            }

            if (right != nullptr)
            {
                const float absR = std::abs(right[i]);
                if (absR > envR)
                    envR = absR;
                else
                    envR = relCoeff * envR + (1.0f - relCoeff) * absR;

                if (envR > thresh)
                    right[i] *= thresh / envR;
            }
        }
    }

private:
    double sr = 44100.0;
    float envL = 0.0f;
    float envR = 0.0f;
};

// =============================================================================
// Cabinet Simulation (fixed-topology biquad chain modeling speaker cabinet)
// =============================================================================
class CabinetSim
{
public:
    struct Params
    {
        float mix = 0.0f;
        float highPassHz = 80.0f;
        float bodyHz = 400.0f;
        float bodyGainDb = 4.0f;
        float bodyQ = 0.8f;
        float presenceHz = 2500.0f;
        float presenceGainDb = 3.0f;
        float presenceQ = 1.2f;
        float lowPassHz = 5000.0f;
        float lowPassQ = 0.707f;
    };

    void prepare(double sampleRate)
    {
        sr = std::max(1.0, sampleRate);
        coeffsValid = false;
        smoothedParams = Params {};
        recomputeCoeffs(Params{});
        reset();
    }

    void reset()
    {
        coeffsValid = false;
        smoothedParams = Params {};
        for (auto& st : bqState) st = {};
    }

    void process(float* left, float* right, int numSamples, const Params& p)
    {
        const float mix = clamp01(p.mix);
        if (mix <= 0.0001f) return;

        const Params target = sanitizeParams(p);
        if (!coeffsValid)
            smoothedParams = target;
        else
            smoothParamsToward(smoothedParams, target, 0.35f);

        if (!coeffsValid || paramsChanged(lastParams, smoothedParams))
            recomputeCoeffs(smoothedParams);

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = left[i];
            const float dryR = right != nullptr ? right[i] : 0.0f;
            float L = dryL;
            float R = dryR;

            for (int b = 0; b < kNumStages; ++b)
            {
                L = biquadDF2T(bqState[b * 2],     bqCoeffs[b], L);
                R = biquadDF2T(bqState[b * 2 + 1], bqCoeffs[b], R);
            }

            if (!std::isfinite(L))
            {
                L = dryL;
                for (int b = 0; b < kNumStages; ++b)
                    bqState[b * 2] = {};
            }
            if (right != nullptr && !std::isfinite(R))
            {
                R = dryR;
                for (int b = 0; b < kNumStages; ++b)
                    bqState[b * 2 + 1] = {};
            }

            left[i] = dryL + (L - dryL) * mix;
            if (right != nullptr)
                right[i] = dryR + (R - dryR) * mix;
        }
    }

private:
    static constexpr int kNumStages = 4;

    struct BQCoeffs { float b0=1, b1=0, b2=0, a1=0, a2=0; };
    struct BQState  { float z1=0, z2=0; };

    static float biquadDF2T(BQState& st, const BQCoeffs& c, float x)
    {
        const float y = c.b0 * x + st.z1;
        st.z1 = c.b1 * x - c.a1 * y + st.z2;
        st.z2 = c.b2 * x - c.a2 * y;
        if (!std::isfinite(y) || !std::isfinite(st.z1) || !std::isfinite(st.z2))
        {
            st = {};
            return 0.0f;
        }
        return y;
    }

    BQCoeffs calcHighPass(float freq, float Q) const
    {
        freq = std::clamp(freq, 20.0f, static_cast<float>(sr) * 0.45f);
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sr);
        const float cosw = std::cos(w0), sinw = std::sin(w0);
        const float alpha = sinw / (2.0f * std::max(0.01f, Q));
        const float a0 = 1.0f + alpha;
        BQCoeffs c;
        c.b0 =  (1.0f + cosw) * 0.5f / a0;
        c.b1 = -(1.0f + cosw)        / a0;
        c.b2 =  (1.0f + cosw) * 0.5f / a0;
        c.a1 = -2.0f * cosw / a0;
        c.a2 = (1.0f - alpha) / a0;
        return sanitizeCoeffs(c);
    }

    BQCoeffs calcLowPass(float freq, float Q) const
    {
        freq = std::clamp(freq, 20.0f, static_cast<float>(sr) * 0.45f);
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sr);
        const float cosw = std::cos(w0), sinw = std::sin(w0);
        const float alpha = sinw / (2.0f * std::max(0.01f, Q));
        const float a0 = 1.0f + alpha;
        BQCoeffs c;
        c.b0 = (1.0f - cosw) * 0.5f / a0;
        c.b1 = (1.0f - cosw)        / a0;
        c.b2 = (1.0f - cosw) * 0.5f / a0;
        c.a1 = -2.0f * cosw / a0;
        c.a2 = (1.0f - alpha) / a0;
        return sanitizeCoeffs(c);
    }

    BQCoeffs calcPeaking(float freq, float gainDb, float Q) const
    {
        freq = std::clamp(freq, 20.0f, static_cast<float>(sr) * 0.45f);
        const float A = std::pow(10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * 3.14159265f * freq / static_cast<float>(sr);
        const float cosw = std::cos(w0), sinw = std::sin(w0);
        const float alpha = sinw / (2.0f * std::max(0.01f, Q));
        const float a0 = 1.0f + alpha / A;
        BQCoeffs c;
        c.b0 = (1.0f + alpha * A) / a0;
        c.b1 = (-2.0f * cosw) / a0;
        c.b2 = (1.0f - alpha * A) / a0;
        c.a1 = c.b1;
        c.a2 = (1.0f - alpha / A) / a0;
        return sanitizeCoeffs(c);
    }

    void recomputeCoeffs(const Params& p)
    {
        bqCoeffs[0] = calcHighPass(std::clamp(p.highPassHz, 40.0f, 180.0f),
                                   std::clamp(p.bodyQ * 0.9f, 0.45f, 1.20f));
        bqCoeffs[1] = calcPeaking(std::clamp(p.bodyHz, 180.0f, 650.0f),
                                  std::clamp(p.bodyGainDb, -6.0f, 8.0f),
                                  std::clamp(p.bodyQ, 0.35f, 2.0f));
        bqCoeffs[2] = calcPeaking(std::clamp(p.presenceHz, 1200.0f, 4200.0f),
                                  std::clamp(p.presenceGainDb, -6.0f, 7.0f),
                                  std::clamp(p.presenceQ, 0.45f, 2.5f));
        bqCoeffs[3] = calcLowPass(std::clamp(p.lowPassHz, 2800.0f, 9000.0f),
                                  std::clamp(p.lowPassQ, 0.45f, 1.20f));
        lastParams = p;
        coeffsValid = true;
    }

    Params sanitizeParams(Params p) const noexcept
    {
        const float maxFreq = std::max(20.0f, static_cast<float>(sr) * 0.45f);
        p.highPassHz = std::clamp(p.highPassHz, 20.0f, maxFreq);
        p.bodyHz = std::clamp(p.bodyHz, 20.0f, maxFreq);
        p.presenceHz = std::clamp(p.presenceHz, 20.0f, maxFreq);
        p.lowPassHz = std::clamp(p.lowPassHz, 20.0f, maxFreq);
        p.bodyGainDb = std::clamp(p.bodyGainDb, -12.0f, 12.0f);
        p.presenceGainDb = std::clamp(p.presenceGainDb, -12.0f, 12.0f);
        p.bodyQ = std::clamp(p.bodyQ, 0.10f, 8.0f);
        p.presenceQ = std::clamp(p.presenceQ, 0.10f, 8.0f);
        p.lowPassQ = std::clamp(p.lowPassQ, 0.10f, 8.0f);
        return p;
    }

    static bool paramsChanged(const Params& a, const Params& b) noexcept
    {
        return std::abs(a.highPassHz - b.highPassHz) > 0.5f
            || std::abs(a.bodyHz - b.bodyHz) > 0.5f
            || std::abs(a.bodyGainDb - b.bodyGainDb) > 0.01f
            || std::abs(a.bodyQ - b.bodyQ) > 0.001f
            || std::abs(a.presenceHz - b.presenceHz) > 0.5f
            || std::abs(a.presenceGainDb - b.presenceGainDb) > 0.01f
            || std::abs(a.presenceQ - b.presenceQ) > 0.001f
            || std::abs(a.lowPassHz - b.lowPassHz) > 0.5f
            || std::abs(a.lowPassQ - b.lowPassQ) > 0.001f;
    }

    static void smoothParamsToward(Params& current, const Params& target, float amount) noexcept
    {
        current.mix = target.mix;
        current.highPassHz += (target.highPassHz - current.highPassHz) * amount;
        current.bodyHz += (target.bodyHz - current.bodyHz) * amount;
        current.bodyGainDb += (target.bodyGainDb - current.bodyGainDb) * amount;
        current.bodyQ += (target.bodyQ - current.bodyQ) * amount;
        current.presenceHz += (target.presenceHz - current.presenceHz) * amount;
        current.presenceGainDb += (target.presenceGainDb - current.presenceGainDb) * amount;
        current.presenceQ += (target.presenceQ - current.presenceQ) * amount;
        current.lowPassHz += (target.lowPassHz - current.lowPassHz) * amount;
        current.lowPassQ += (target.lowPassQ - current.lowPassQ) * amount;
    }

    static BQCoeffs sanitizeCoeffs(BQCoeffs c) noexcept
    {
        if (!std::isfinite(c.b0) || !std::isfinite(c.b1) || !std::isfinite(c.b2)
            || !std::isfinite(c.a1) || !std::isfinite(c.a2))
            return {};
        return c;
    }

    double sr = 44100.0;
    BQCoeffs bqCoeffs[kNumStages];
    BQState  bqState[kNumStages * 2];
    Params lastParams {};
    Params smoothedParams {};
    bool coeffsValid = false;
};

// =============================================================================
// SyntheticConvReverb - algorithmic convolution reverb (no external IR files).
// Generates impulse responses algorithmically - no external IR files required.
// Types: 0=Plate (passthrough to DattorroPlateReverb), 1=Hall, 2=Room, 3=Chamber
// =============================================================================
class SyntheticConvReverb
{
public:
    enum class Type { Hall = 1, Room = 2, Chamber = 3 };

    void prepare(double sampleRate, int maxBlockSize)
    {
        sr = std::max(1.0, sampleRate);
        spec.sampleRate       = sr;
        spec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, maxBlockSize));
        spec.numChannels      = 2;
        for (auto& c : convs)
        {
            c.prepare(spec);
            c.reset();
        }
        workBuffer.setSize(2, static_cast<int>(spec.maximumBlockSize), false, false, true);
        buildIR(convs[0], static_cast<int>(Type::Hall), 0.5f, 0.5f, 0.8f);
        buildIR(convs[1], static_cast<int>(Type::Room), 0.5f, 0.5f, 0.8f);
        buildIR(convs[2], static_cast<int>(Type::Chamber), 0.5f, 0.5f, 0.8f);
        reset();
    }

    void setType(int type, float size = 0.5f, float damping = 0.5f, float width = 0.8f) noexcept
    {
        if (type < 1 || type > 3)
            return;

        currentType = type;
        targetSize = clamp01(size);
        targetDamping = clamp01(damping);
        targetWidth = clamp01(width);
    }

    // Processes stereo samples (dry in -> wet out, fully wet). Caller handles dry/wet mixing externally.
    void processBlock(float* left, float* right, int numSamples)
    {
        if (currentType < 1 || numSamples <= 0)
            return;
        if (workBuffer.getNumChannels() < 2 || workBuffer.getNumSamples() < numSamples)
            return;

        const bool mono = right == nullptr || right == left;
        workBuffer.copyFrom(0, 0, left, numSamples);
        if (mono)
            workBuffer.copyFrom(1, 0, left, numSamples);
        else
            workBuffer.copyFrom(1, 0, right, numSamples);

        juce::dsp::AudioBlock<float> block(workBuffer);
        auto subBlock = block.getSubBlock(0, static_cast<std::size_t>(numSamples));
        juce::dsp::ProcessContextReplacing<float> ctx(subBlock);
        convs[static_cast<std::size_t>(currentType - 1)].process(ctx);

        auto* wetL = workBuffer.getWritePointer(0);
        auto* wetR = workBuffer.getWritePointer(1);
        const float sizeStep = (targetSize - currentSize) / static_cast<float>(juce::jmax(1, numSamples));
        const float dampingStep = (targetDamping - currentDamping) / static_cast<float>(juce::jmax(1, numSamples));
        const float widthStep = (targetWidth - currentWidth) / static_cast<float>(juce::jmax(1, numSamples));
        float size = currentSize;
        float damping = currentDamping;
        float width = currentWidth;

        for (int i = 0; i < numSamples; ++i)
        {
            const float lpCoeff = lerp(0.58f, 0.045f, damping);
            reverbLpL += lpCoeff * (wetL[i] - reverbLpL);
            reverbLpR += lpCoeff * (wetR[i] - reverbLpR);

            const float roomGain = lerp(0.74f, 1.22f, size);
            const float hiBlend = lerp(0.28f, 0.86f, 1.0f - damping);
            float l = (reverbLpL + (wetL[i] - reverbLpL) * hiBlend) * roomGain;
            float r = (reverbLpR + (wetR[i] - reverbLpR) * hiBlend) * roomGain;

            const float mid = 0.5f * (l + r);
            const float side = 0.5f * (l - r) * lerp(0.18f, 1.65f, width);
            l = mid + side;
            r = mid - side;

            wetL[i] = std::isfinite(l) ? l : 0.0f;
            wetR[i] = std::isfinite(r) ? r : 0.0f;
            size += sizeStep;
            damping += dampingStep;
            width += widthStep;
        }

        currentSize = targetSize;
        currentDamping = targetDamping;
        currentWidth = targetWidth;

        std::memcpy(left, workBuffer.getReadPointer(0), static_cast<std::size_t>(numSamples) * sizeof(float));
        if (!mono)
            std::memcpy(right, workBuffer.getReadPointer(1), static_cast<std::size_t>(numSamples) * sizeof(float));
    }

    void reset()
    {
        for (auto& c : convs)
            c.reset();
        currentType = static_cast<int>(Type::Hall);
        targetSize = currentSize = 0.5f;
        targetDamping = currentDamping = 0.5f;
        targetWidth = currentWidth = 0.8f;
        reverbLpL = 0.0f;
        reverbLpR = 0.0f;
    }

private:
    static float lerp(float a, float b, float t) noexcept
    {
        return a + (b - a) * t;
    }

    void buildIR(juce::dsp::Convolution& target, int type, float size, float damping, float width)
    {
        float rt60 = 1.8f;
        float density = 0.85f;
        float brightness = 0.6f;
        float preDelayMs = 24.0f;

        if (type == static_cast<int>(Type::Room))
        {
            rt60 = 0.7f;
            density = 0.65f;
            brightness = 0.75f;
            preDelayMs = 11.0f;
        }
        else if (type == static_cast<int>(Type::Chamber))
        {
            rt60 = 1.3f;
            density = 0.78f;
            brightness = 0.55f;
            preDelayMs = 18.0f;
        }

        rt60 *= lerp(0.72f, 1.35f, size);
        density = std::clamp(density * lerp(0.90f, 1.08f, size), 0.25f, 0.98f);
        brightness = lerp(0.92f, 0.28f, damping);
        preDelayMs *= lerp(0.65f, 1.35f, size);

        const int irLen = static_cast<int>(sr * static_cast<double>(rt60)) + 1;
        juce::AudioBuffer<float> irBuf(2, irLen);
        const int basePreDelayLen = static_cast<int>(sr * (preDelayMs * 0.001f));
        const int stereoSkew = static_cast<int>(sr * width * 0.0035f);

        for (int ch = 0; ch < 2; ++ch)
        {
            juce::Random rng(0x4B1D + type * 37 + ch * 101);
            auto* data = irBuf.getWritePointer(ch);
            const float decayCoeff = std::exp(-6.908f / (rt60 * static_cast<float>(sr)));
            float env = 1.0f;
            float lpState = 0.0f;
            const float lpCoeff = brightness * 0.30f + 0.04f;
            const int preDelayLen = std::max(0, basePreDelayLen + (ch == 0 ? -stereoSkew / 2 : stereoSkew / 2));

            for (int n = 0; n < irLen; ++n)
            {
                if (n < preDelayLen) { data[n] = 0.0f; continue; }
                const float white  = rng.nextFloat() * 2.0f - 1.0f;
                const float corr   = lpState + lpCoeff * (white - lpState);
                lpState = corr;
                const float decorrelated = ch == 0 ? corr : (corr * (0.78f + width * 0.18f) - white * width * 0.12f);
                data[n] = (white * (1.0f - density) + decorrelated * density) * env;
                env *= decayCoeff;
            }

            float peak = 0.0f;
            for (int n = 0; n < irLen; ++n)
                peak = std::max(peak, std::abs(data[n]));
            if (peak > 1e-6f)
            {
                const float normGain = 0.5f / peak;
                for (int n = 0; n < irLen; ++n)
                    data[n] *= normGain;
            }
        }

        target.loadImpulseResponse(std::move(irBuf), sr,
                                   juce::dsp::Convolution::Stereo::yes,
                                   juce::dsp::Convolution::Trim::no,
                                   juce::dsp::Convolution::Normalise::no);
    }

    std::array<juce::dsp::Convolution, 3> convs;
    juce::AudioBuffer<float>        workBuffer;
    juce::dsp::ProcessSpec          spec {};
    double                          sr = 44100.0;
    int                             currentType = static_cast<int>(Type::Hall);
    float                           targetSize = 0.5f;
    float                           targetDamping = 0.5f;
    float                           targetWidth = 0.8f;
    float                           currentSize = 0.5f;
    float                           currentDamping = 0.5f;
    float                           currentWidth = 0.8f;
    float                           reverbLpL = 0.0f;
    float                           reverbLpR = 0.0f;
};

} // namespace fx
} // namespace mgs
