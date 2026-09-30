#include "GuitarInstrumentModel.h"

#include <algorithm>
#include <atomic>

namespace mgs
{
namespace
{
constexpr std::array<GuitarInstrumentModelProfile, kNumInstruments> kProfiles {{
    {
        "folk_steel_body_string_noise",
        "steel_string_karplus_body_sympathetic",
        "hard pick and fingertip scrape",
        "dreadnought body with sympathetic string haze",
        "pick edge, steel brightness, wooden body bloom",
        SynthMode::KarplusStrong,
        1.18f, 0.92f, 1.18f, 1.08f, 0.10f, 1.00f, 0.00f, 1.04f, 0.82f, 1.05f
    },
    {
        "twelve_string_octave_course",
        "paired_course_karplus_octave_detune",
        "paired pick impulse with octave-course smear",
        "wide steel body and coupled course beating",
        "course shimmer, octave split, sympathetic wash",
        SynthMode::KarplusStrong,
        1.22f, 0.88f, 1.12f, 1.10f, 0.16f, 1.00f, 0.00f, 1.16f, 0.80f, 1.12f
    },
    {
        "flamenca_nylon_tap_body",
        "nylon_string_fast_attack_percussive_body",
        "nail attack with rasgueado-like rake",
        "dry nylon body with compact percussive low end",
        "fast transient, nylon snap, short wooden bloom",
        SynthMode::KarplusStrong,
        1.34f, 0.74f, 0.92f, 0.94f, 0.07f, 0.96f, 0.00f, 0.96f, 0.86f, 0.75f
    },
    {
        "clean_electric_pickup_body",
        "single_coil_string_pickup_karplus",
        "pick impulse into clean pickup transient",
        "light body resonance and pickup mid focus",
        "clean attack, pickup edge, restrained body",
        SynthMode::KarplusStrong,
        1.14f, 0.96f, 0.48f, 0.84f, 0.03f, 1.02f, 0.00f, 0.98f, 0.42f, 0.82f
    },
    {
        "crunch_electric_amp_string",
        "driven_string_pickup_amp_saturation",
        "pick impulse into asymmetric pickup drive",
        "small body plus amp-stage compression",
        "crunch bite, pick definition, compact sustain",
        SynthMode::KarplusStrong,
        1.10f, 0.98f, 0.30f, 0.78f, 0.02f, 1.22f, 0.00f, 0.94f, 0.35f, 0.92f
    },
    {
        "lead_electric_sustain_amp",
        "sustained_pickup_string_drive",
        "compressed pick impulse and drive sustain",
        "focused pickup path with long controlled tail",
        "lead sustain, drive focus, stable high notes",
        SynthMode::KarplusStrong,
        1.08f, 1.08f, 0.22f, 0.74f, 0.01f, 1.34f, 0.02f, 1.02f, 0.22f, 1.00f
    },
    {
        "hybrid_synth_guitar_string_pad",
        "picked_string_continuous_synth_body",
        "pick plus controlled continuous excitation",
        "hybrid body/filter path with synth sustain",
        "string attack into electronic sustain",
        SynthMode::Hybrid,
        0.94f, 1.08f, 0.66f, 0.74f, 0.02f, 1.18f, 0.34f, 1.18f, 0.24f, 0.68f
    },
    {
        "electric_pad_detuned_course",
        "soft_pick_detuned_pad_body",
        "soft pick feeding continuous pad excitation",
        "detuned paired strings through wide pad body",
        "slow attack, detuned string pad, wide release",
        SynthMode::Pad,
        0.76f, 1.24f, 0.48f, 0.64f, 0.02f, 0.98f, 0.36f, 1.34f, 0.14f, 0.50f
    },
    {
        "ambient_guitar_cloud",
        "pitched_string_cloud_continuous_resonator",
        "blurred pick onset with continuous sustain",
        "wide diffuse resonator and detuned string cloud",
        "ambient bloom, pitch tracking, long stereo tail",
        SynthMode::Pad,
        0.66f, 1.32f, 0.50f, 0.62f, 0.03f, 0.94f, 0.34f, 1.48f, 0.12f, 0.42f
    },
}};

std::atomic<int> gRenderMode { static_cast<int>(GuitarRenderEngineMode::V2) };
} // namespace

const std::array<GuitarInstrumentModelProfile, kNumInstruments>& getGuitarInstrumentModelProfiles()
{
    return kProfiles;
}

const GuitarInstrumentModelProfile& getGuitarInstrumentModelProfile(const int instrumentIndex)
{
    return kProfiles[static_cast<std::size_t>(std::clamp(instrumentIndex, 0, kNumInstruments - 1))];
}

GuitarRenderEngineMode getGuitarRenderEngineMode() noexcept
{
    const auto value = gRenderMode.load(std::memory_order_relaxed);
    switch (static_cast<GuitarRenderEngineMode>(value))
    {
        case GuitarRenderEngineMode::LegacyFamily:
        case GuitarRenderEngineMode::V2:
        case GuitarRenderEngineMode::V2ModelOnly:
            return static_cast<GuitarRenderEngineMode>(value);
    }

    return GuitarRenderEngineMode::V2;
}

void setGuitarRenderEngineMode(const GuitarRenderEngineMode mode) noexcept
{
    gRenderMode.store(static_cast<int>(mode), std::memory_order_relaxed);
}

const char* getGuitarRenderEngineModeName(const GuitarRenderEngineMode mode) noexcept
{
    switch (mode)
    {
        case GuitarRenderEngineMode::LegacyFamily: return "legacy_family";
        case GuitarRenderEngineMode::V2: return "v2";
        case GuitarRenderEngineMode::V2ModelOnly: return "v2_model_only";
    }

    return "unknown";
}

} // namespace mgs
