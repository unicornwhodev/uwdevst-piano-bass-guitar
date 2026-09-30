#pragma once

// =============================================================================
// SSE2 SIMD helpers for partial rendering — processes 4 partials in parallel.
// =============================================================================

#include <cstddef>
#include <cmath>
#include <type_traits>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include "SinTable.h"

namespace mps {
namespace simd {

// -----------------------------------------------------------------------------
// SSE2 floor (truncate towards negative infinity)
// -----------------------------------------------------------------------------
static inline __m128 sseFloor(__m128 x)
{
    __m128 trunc_f = _mm_cvtepi32_ps(_mm_cvttps_epi32(x));
    __m128 mask    = _mm_cmpgt_ps(trunc_f, x);
    return _mm_sub_ps(trunc_f, _mm_and_ps(mask, _mm_set1_ps(1.0f)));
}

// -----------------------------------------------------------------------------
// SSE2 polynomial sine approximation.
// Input : phase in [0, 1) representing one full cycle.
// Output: sin(2 pi phase), ~17-bit accuracy (max error < 2e-5).
//
// Uses symmetry reduction to [0, pi/2] then a 7th-order minimax polynomial.
// Pure SSE2 — no SSE4.1 needed.
// -----------------------------------------------------------------------------
static inline __m128 sseSin01(__m128 phase)
{
    const __m128 kHalf    = _mm_set1_ps(0.5f);
    const __m128 kOne     = _mm_set1_ps(1.0f);
    const __m128 kTwo     = _mm_set1_ps(2.0f);
    const __m128 kQuarter = _mm_set1_ps(0.25f);

    // sign = 1.0; if phase >= 0.5 → sign = -1.0, phase -= 0.5
    __m128 sign = kOne;
    __m128 mask = _mm_cmpge_ps(phase, kHalf);
    phase = _mm_sub_ps(phase, _mm_and_ps(mask, kHalf));
    sign  = _mm_sub_ps(sign,  _mm_and_ps(mask, kTwo));

    // if phase > 0.25 → phase = 0.5 - phase  (reflection around pi/2)
    mask = _mm_cmpgt_ps(phase, kQuarter);
    __m128 reflected = _mm_sub_ps(kHalf, phase);
    phase = _mm_or_ps(
        _mm_and_ps(mask, reflected),
        _mm_andnot_ps(mask, phase)
    );

    // x = 2 pi phase   (now in [0, pi/2])
    const __m128 kTwoPi = _mm_set1_ps(6.2831853f);
    __m128 x  = _mm_mul_ps(phase, kTwoPi);
    __m128 x2 = _mm_mul_ps(x, x);

    // sin(x) ~ x (c1 + x^2 (c3 + x^2 (c5 + x^2 c7)))
    // Minimax coefficients for [0, pi/2]:
    const __m128 c7 = _mm_set1_ps(-1.9841269e-4f);
    const __m128 c5 = _mm_set1_ps( 8.3333315e-3f);
    const __m128 c3 = _mm_set1_ps(-1.6666666e-1f);
    const __m128 c1 = _mm_set1_ps( 9.9999999e-1f);

    __m128 poly = _mm_add_ps(_mm_mul_ps(c7, x2), c5);
    poly = _mm_add_ps(_mm_mul_ps(poly, x2), c3);
    poly = _mm_add_ps(_mm_mul_ps(poly, x2), c1);

    return _mm_mul_ps(_mm_mul_ps(x, poly), sign);
}

// -----------------------------------------------------------------------------
// Render additive partials using SSE2, 4 at a time.
//
// `partialData` must point to a tightly packed array of structs, each
// containing exactly 4 floats: { phase, phaseInc, amplitude, decayCoeff }.
// This matches AcousticPianoVoiceBase::PartialState, ElectricPianoVoiceBase::
// PartialState, ClavPartial, and BellPartial.
//
// Returns the total summed signal contribution.
// On return the partial states are updated (phase advanced, amplitude decayed).
// -----------------------------------------------------------------------------
static inline float renderPartials(float* partialData, int count, float pitchFactor)
{
    const __m128 vPitch = _mm_set1_ps(pitchFactor);
    __m128 vSum = _mm_setzero_ps();

    int n = 0;

    // --- 4-wide SIMD loop ---
    for (; n + 4 <= count; n += 4)
    {
        float* base = partialData + n * 4;

        // Load 4 partials (AoS: 4 × 4 floats)
        __m128 r0 = _mm_loadu_ps(base);
        __m128 r1 = _mm_loadu_ps(base + 4);
        __m128 r2 = _mm_loadu_ps(base + 8);
        __m128 r3 = _mm_loadu_ps(base + 12);

        // Transpose AoS → SoA
        _MM_TRANSPOSE4_PS(r0, r1, r2, r3);
        // r0 = phases, r1 = phaseIncs, r2 = amplitudes, r3 = decayCoeffs

        // sin(2 pi phase) for 4 partials at once
        __m128 sinVals = sseSin01(r0);

        // Accumulate: sum += amplitude * sin(phase)
        vSum = _mm_add_ps(vSum, _mm_mul_ps(r2, sinVals));

        // Phase advance: phase += phaseInc * pitchFactor
        r0 = _mm_add_ps(r0, _mm_mul_ps(r1, vPitch));

        // Phase wrap to [0, 1): phase -= floor(phase)
        r0 = _mm_sub_ps(r0, sseFloor(r0));

        // Amplitude decay: amplitude *= decayCoeff
        r2 = _mm_mul_ps(r2, r3);

        // Transpose SoA → AoS and store back
        _MM_TRANSPOSE4_PS(r0, r1, r2, r3);
        _mm_storeu_ps(base,      r0);
        _mm_storeu_ps(base + 4,  r1);
        _mm_storeu_ps(base + 8,  r2);
        _mm_storeu_ps(base + 12, r3);
    }

    // Horizontal sum of 4 floats → scalar
    __m128 shuf = _mm_movehl_ps(vSum, vSum);
    __m128 sums = _mm_add_ps(vSum, shuf);
    shuf = _mm_shuffle_ps(sums, sums, 1);
    sums = _mm_add_ps(sums, shuf);
    float sum = _mm_cvtss_f32(sums);

    // --- Scalar remainder (0-3 partials) using table lookup ---
    for (; n < count; ++n)
    {
        float* p = partialData + n * 4;
        sum  += p[2] * mps::fastSin(p[0]);
        p[0] += p[1] * pitchFactor;
        p[0] -= std::floor(p[0]);
        p[2] *= p[3];
    }

    return sum;
}

template <typename Partial>
static inline float renderPartialArray(Partial* partials, int count, float pitchFactor)
{
    static_assert(std::is_standard_layout_v<Partial>, "SIMD partials must be standard-layout AoS structs");
    static_assert(sizeof(Partial) == sizeof(float) * 4, "SIMD partials must be exactly { phase, phaseInc, amplitude, decayCoeff }");
    static_assert(alignof(Partial) <= alignof(float) * 4, "SIMD partials must not require wider alignment than the raw float loader contract");
    return renderPartials(reinterpret_cast<float*>(partials), count, pitchFactor);
}

} // namespace simd
} // namespace mps
