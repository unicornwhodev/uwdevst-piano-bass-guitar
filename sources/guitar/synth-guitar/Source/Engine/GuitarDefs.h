#pragma once

#include <array>
#include <cstdint>

namespace mgs   // Musique Guitar Synth
{

// =========================================================================
// Instrument count & families
// =========================================================================
constexpr int kNumInstruments = 9;
constexpr int kNumFamilies    = 3;

//  Family 0 – ACOUSTIQUE    (0-2)  Folk Steel, 12 Cordes, Flamenca
//  Family 1 – \xC3\x89LECTRIQUE   (3-5)  Clean, Crunch, Lead
//  Family 2 – \xC3\x89LECTRONIQUE (6-8)  Synth Guitar, E-Guitar Pad, Guitar Ambient

constexpr int kFamilySize[]  = { 3, 3, 3 };
constexpr int kFamilyStart[] = { 0, 3, 6 };

enum class Family { Acoustique = 0, Electrique, Electronique };

enum class GlobalFxSlot
{
    Reverb = 0,
    Saturator,
    Transient,
    Compressor,
    Eq,
    Chorus,
    Delay,
    Limiter,
    Cabinet
};

// =========================================================================
// Synthesis mode
// =========================================================================
enum class SynthMode { KarplusStrong = 0, Pad, Hybrid };

// =========================================================================
// Per-instrument characteristics (compile-time-friendly)
// =========================================================================
struct StringCharacteristics
{
    float decaySeconds;   // base string decay time (sec)
    float brightness;     // string loop LP brightness
    float inharmonicity;  // paired-string detune factor
};

struct ExcitationCharacteristics
{
    float brightness;     // excitation noise brightness
    float decaySeconds;   // excitation noise decay (sec)
    float position;       // pick position along string (0-1)
};

struct BodyCharacteristics
{
    float sizeRatio;      // body resonator frequency ratio vs played note
    float resonance;      // body comb feedback
    float brightness;     // body HF content
};

struct OutputCharacteristics
{
    float drive;          // inherent drive/distortion
    float pitchFollow;    // 0 = pad-like, 1 = fully pitched
    float stereoWidth;    // natural stereo spread
};

struct InstrCharacteristics
{
    SynthMode                 synthMode;
    StringCharacteristics     string;
    ExcitationCharacteristics excitation;
    BodyCharacteristics       body;
    OutputCharacteristics     output;
    bool                      legatoEnabled = false;
};

// =========================================================================
// Per-instrument adjustable settings (14 knobs)
// =========================================================================
struct ToneSettings
{
    float stringBrightness = 0.50f;
    float lowPassHz = 8000.0f;
};

struct EnvelopeSettings
{
    float attackSeconds = 0.005f;
    float decaySeconds = 2.0f;
    float sustainLevel = 0.30f;
    float releaseSeconds = 0.30f;
};

struct ResonanceSettings
{
    float bodyAmount = 0.50f;
    float driveAmount = 0.0f;
};

struct PerformanceSettings
{
    float attackBrightness = 0.50f;
    float pickPosition = 0.50f;
};

struct SpatialSettings
{
    float stereoWidth = 0.40f;
    float pan = 0.0f;
};

// =========================================================================
// Global FX settings snapshot (saved per factory preset)
// =========================================================================
struct GlobalFxSettings
{
    // Saturator
    float satDrive         = 1.5f;
    float satMix           = 0.10f;
    bool  saturatorOn      = true;
    // Transient
    float transientAttack  = 0.05f;
    float transientSustain = 0.0f;
    float transientMix     = 0.3f;
    bool  transientOn      = true;
    // EQ
    float eqLowFreq   = 200.0f;
    float eqLowGain   = 0.0f;
    float eqMidFreq   = 1000.0f;
    float eqMidGain   = 0.0f;
    float eqMidQ      = 1.0f;
    float eqHighFreq  = 5000.0f;
    float eqHighGain  = 0.0f;
    bool  eqOn        = true;
    // Compressor
    float compThreshold = -19.0f;
    float compRatio     = 3.0f;
    float compAttack    = 10.0f;
    float compRelease   = 120.0f;
    float compMakeup    = 0.0f;
    float compMix       = 1.0f;
    bool  compressorOn  = true;
    // Chorus
    float chorusRate    = 1.0f;
    float chorusDepth   = 0.5f;
    float chorusDelay   = 7.0f;
    float chorusMix     = 0.0f;
    bool  chorusOn      = true;
    // Delay
    float delayTime     = 300.0f;
    float delayFeedback = 0.30f;
    float delayMix      = 0.0f;
    bool  delayOn       = true;
    // Reverb (Dattorro)
    float reverbSize     = 0.55f;
    float reverbDamping  = 0.45f;
    float reverbWidth    = 0.80f;
    float reverbMix      = 0.22f;
    bool  reverbOn       = true;
    // Limiter
    float limiterThreshold = -0.3f;
    float limiterRelease   = 50.0f;
    bool  limiterOn        = true;
    // Cabinet
    float cabMix         = 0.0f;
    bool  cabinetOn      = true;
};

struct FxAvailability
{
    bool reverb     = true;
    bool saturator  = true;
    bool transient  = true;
    bool compressor = true;
    bool eq         = true;
    bool chorus     = true;
    bool delay      = true;
    bool limiter    = true;
    bool cabinet    = false;
};

// =========================================================================
// Per-instrument adjustable settings (14 knobs)
// =========================================================================
struct InstrSettings
{
    float level = 0.80f;
    float tuneSemitones = 0.0f;
    ToneSettings tone {};
    EnvelopeSettings envelope {};
    ResonanceSettings resonance {};
    PerformanceSettings performance {};
    SpatialSettings spatial {};

    constexpr InstrSettings() = default;

    // Keep legacy preset literal ordering stable while exposing more precise names.
    constexpr InstrSettings(float levelIn,
                            float tuneSemitonesIn,
                            float stringBrightnessIn,
                            float attackSecondsIn,
                            float decaySecondsIn,
                            float sustainLevelIn,
                            float releaseSecondsIn,
                            float bodyAmountIn,
                            float driveAmountIn,
                            float attackBrightnessIn,
                            float stereoWidthIn,
                            float pickPositionIn,
                            float lowPassHzIn,
                            float panIn) noexcept
        : level(levelIn),
          tuneSemitones(tuneSemitonesIn),
          tone { stringBrightnessIn, lowPassHzIn },
          envelope { attackSecondsIn, decaySecondsIn, sustainLevelIn, releaseSecondsIn },
          resonance { bodyAmountIn, driveAmountIn },
          performance { attackBrightnessIn, pickPositionIn },
          spatial { stereoWidthIn, panIn }
    {
    }
};

// =========================================================================
// Accessors (implemented in GuitarDefs.cpp)
// =========================================================================
Family                      getFamily(int instrIndex);
int                         getFamilyStartIndex(Family family);
const char*                 getFamilyName(int familyIndex);
const char*                 getInstrName(int instrIndex);
const char*                 getInstrShortName(int instrIndex);
const InstrCharacteristics& getCharacteristics(int instrIndex);
InstrSettings               getDefaultSettings(int instrIndex);
const char*                 getInstrDescription(int instrIndex);
const FxAvailability&       getFxAvailability(int instrIndex);
bool                        isFxAvailable(int instrIndex, GlobalFxSlot slot);
GlobalFxSettings            maskUnavailableFx(int instrIndex, const GlobalFxSettings& fx);

} // namespace mgs
