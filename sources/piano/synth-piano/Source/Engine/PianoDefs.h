#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace mps
{
constexpr int kNumPianos   = 8;
constexpr int kNumFamilies = 3;
constexpr int kMaxPianosPerFamily = 3;

constexpr int kFamilySize[]  = { 3, 2, 3 };
constexpr int kFamilyStart[] = { 0, 3, 5 };

// =========================================================================
// Per-note inharmonicity B lookup table (Lamb-to-Michigan physics model).
// Derived from: fn = f0 * sqrt(1 + B * n²).
// Values are reference B × 10^6 for MIDI notes 21-108.
// Calibrated around a Steinway D reference; scale per piano with
// StringCharacteristics::inharmonicityScale.
// =========================================================================
constexpr std::array<std::uint32_t, 88> kInharmonicityBLut = {{
    // A0–B0 (notes 21–23) — very long strings, highest B
    560u, 520u, 480u,
    // C1–B1 (notes 24–35)
    450u, 420u, 395u, 370u, 345u, 325u, 305u, 285u, 268u, 252u, 238u, 225u,
    // C2–B2 (notes 36–47)
    212u, 200u, 189u, 178u, 168u, 158u, 149u, 141u, 133u, 126u, 119u, 112u,
    // C3–B3 (notes 48–59)
    106u, 100u,  94u,  89u,  84u,  79u,  75u,  71u,  67u,  63u,  60u,  57u,
    // C4–B4 (notes 60–71) — mid register
     54u,  51u,  48u,  46u,  43u,  41u,  39u,  37u,  35u,  33u,  32u,  30u,
    // C5–B5 (notes 72–83) — trebly, shorter strings
     29u,  27u,  26u,  25u,  23u,  22u,  21u,  20u,  19u,  18u,  17u,  17u,
    // C6–B6 (notes 84–95) — short strings, lower B
     16u,  15u,  14u,  14u,  13u,  12u,  12u,  11u,  11u,  10u,  10u,   9u,
    // C7–C8 (notes 96–108) — shortest strings, lowest B
      9u,   8u,   8u,   7u,   7u,   6u,   6u,   6u,   5u,   5u,   5u,   4u,   4u
}};

// =========================================================================
// Synthesis constants (replaces magic numbers in PianoVoice.cpp)
// =========================================================================
constexpr int   kMaxPartials             = 32;          // increased from 12 for bass richness
constexpr float kInharmonicityRefHz      = 261.625f;    // middle C reference for legacy helpers
constexpr float kMaxInharmonicityB       = 0.0025f;     // hard guard for fn = f0 * sqrt(1 + B*n²)
constexpr float kHammerHardnessBaseBlend = 0.4f;        // constant portion of hammer hardness
constexpr float kHammerHardnessVelBlend  = 0.6f;        // velocity-scaled portion
constexpr float kHammerCutoffBase        = 2.0f;        // base multiplier on f0
constexpr float kHammerCutoffRange       = 12.0f;       // hardness → cutoff multiplier
constexpr float kHammerCutoffVelBase     = 0.3f;        // min velocity contribution
constexpr float kHammerCutoffVelRange    = 0.7f;        // velocity-scaled portion
constexpr float kPartialBaseAmpFloor     = 0.15f;       // floor ratio for partial amplitude
constexpr float kPartialHammerFilterMix  = 0.85f;       // mix of hammer filter vs floor
constexpr float kBrightnessMultMin       = 0.5f;        // brightness knob → min multiplier
constexpr float kBrightnessMultRange     = 1.5f;        // brightness knob → range
constexpr float kDetuneDepthFactor       = 0.02f;       // detuning depth relative to f0
constexpr float kHammerNoiseDecaySec     = 0.0032f;     // hammer noise envelope decay time
constexpr float kHammerNoiseMix          = 0.11f;       // hammer noise mix level
constexpr float kDamperThudMix           = 0.008f;      // damper thud level scaling
constexpr float kDamperThudDecaySec      = 0.015f;      // damper thud decay time
constexpr float kDamperThudFilterCoeff   = 0.85f;       // damper thud LP filter
constexpr float kBodyInputGain           = 0.3f;        // body comb input scaling
constexpr float kBodyOutputGain          = 0.4f;        // body comb output mix
constexpr float kBodyMaxFeedback         = 0.80f;       // body comb max feedback
constexpr float kBodyDampFilterRatio     = 0.4f;        // body damping filter strength
constexpr float kSympInputGain           = 0.06f;       // sympathetic comb input scaling
constexpr float kSympOutputGain          = 0.4f;        // sympathetic comb output mix
constexpr float kSympMaxFeedback         = 0.70f;       // sympathetic comb max feedback
constexpr float kSoundboardPrimaryGain   = 0.3f;        // soundboard 1st mode gain scaling
constexpr float kSoundboardSecondaryGain = 0.12f;       // soundboard 2nd mode gain scaling
constexpr float kSoundboardTertiaryGain  = 0.06f;       // soundboard 3rd mode gain scaling
constexpr float kSoundboardSecondaryQ    = 0.6f;        // Q ratio for 2nd soundboard mode
constexpr float kSoundboardTertiaryQ     = 0.4f;        // Q ratio for 3rd soundboard mode
constexpr float kSoundboardTertiaryFreqRatio = 3.7f;    // 3rd mode freq ratio to f0
constexpr float kSustainDecayCoeff       = 0.99999f;    // sustain platform decay (~10.5s to -60 dB at 44.1k)
constexpr float kSustainKillThreshold    = 0.001f;      // Phase 4 fix: early voice kill in sustain phase (-66 dB, was -46 dB)
constexpr float kRegisterPanSpread       = 0.35f;       // stereo width from register position
constexpr float kMaxVoiceAgeSec          = 45.0f;       // absolute maximum voice age
constexpr float kCharacterAcousticAmount = 0.15f;       // character processing amount (acoustic)
constexpr float kCharacterElectricGain   = 3.0f;        // character processing drive (electric)
constexpr float kMinPartialAmplitude     = 0.000005f;   // soundboard gain threshold (-106 dB)
constexpr float kPartialNormReference    = 2.0f;        // target sqrt(totalAmp) for normalization

// Stulov hammer model constants (non-linear felt excitation)
constexpr float kStulovAlpha             = 2.5f;        // felt compression exponent (Stulov)
constexpr float kStulovContactTimeSec    = 0.002f;      // nominal contact time at pp
constexpr float kStulovContactTimeVelScale = 0.0015f;   // contact shortening per velocity
constexpr float kStulovFeltStiffnessBase = 1.0e9f;      // reference stiffness at mid-register
constexpr float kStulovHysteresisRatio   = 0.12f;       // energy lost to felt hysteresis
constexpr float kStulovOutputScale       = 1.5e-10f;    // hammer excitation → audio scaling
constexpr float kStulovRefSampleRate     = 44100.0f;    // reference SR for scaling

// Sympathetic resonance matrix constants
constexpr int   kSympMatrixSize          = 12;          // number of coupled resonators
constexpr float kSympMatrixCoupling      = 0.018f;      // inter-resonator coupling
constexpr float kSympMatrixDecayBase     = 0.9997f;     // base per-sample decay

// Extended soundboard mode count
constexpr int   kSoundboardModes         = 8;           // calibrated modes (was 3)

// Soundboard mode frequency ratios (measured from real soundboards)
// Mode 0 = fundamental (f0); subsequent modes are based on structural resonances
constexpr float kSoundboardModeFreqRatio[kSoundboardModes] = {
    1.0f, 2.0f, 3.7f, 5.4f, 7.3f, 9.2f, 11.7f, 14.1f
};
constexpr float kSoundboardModeGain[kSoundboardModes] = {
    0.30f, 0.12f, 0.06f, 0.035f, 0.02f, 0.012f, 0.008f, 0.005f
};
constexpr float kSoundboardModeQ[kSoundboardModes] = {
    1.0f, 0.6f, 0.4f, 0.35f, 0.3f, 0.25f, 0.22f, 0.18f
};

// Duplex resonance constants (partials above the treble bridge)
constexpr int   kMaxDuplexPartials       = 6;           // number of duplex partials
constexpr float kDuplexGainBase          = 0.015f;      // base level of duplex resonance
constexpr float kDuplexFreqRatioStart    = 8.0f;        // lowest duplex partial ratio to f0
constexpr int   kDuplexNoteThreshold     = 60;          // duplex only above middle C

// Key mechanical noise constants
constexpr float kKeyClickLevel           = 0.024f;      // key-down click amplitude
constexpr float kKeyClickDecaySec        = 0.0012f;     // key click envelope decay
constexpr float kKeyRelClickLevel        = 0.016f;      // key-up click amplitude
constexpr float kKeyRelClickDecaySec     = 0.001f;      // key release click decay
constexpr float kPedalNoiseLevel         = 0.02f;       // sustain pedal mechanical noise
constexpr float kPedalNoiseDecaySec      = 0.008f;      // pedal noise decay

// =========================================================================
// Electric piano synthesis constants
// =========================================================================

// --- Rhodes tine + tonebar model ---
constexpr float kRhodesTonebarDetuneHz   = 0.07f;       // stable body; avoid slow beating that blurs chords
constexpr float kRhodesCouplingCoeff     = 0.22f;       // non-linear tine→tonebar coupling
constexpr float kRhodesTineDecayBase     = 2.5f;        // tine decay seconds (short, bell-like)
constexpr float kRhodesTonebarDecayBase  = 4.0f;        // warmth with a controlled tail between chord changes
constexpr int   kRhodesBellPartials      = 4;           // upper metallic bell partials
constexpr float kRhodesBellAmpBase       = 0.08f;       // transient tine modes, subordinate to the warm body
constexpr float kRhodesBellDecayFactor   = 0.24f;       // upper tine modes decay before the tonebar
// --- Rhodes asymmetric pickup ---
constexpr float kRhodesPickupAsymmetry   = 0.14f;       // pickup asymmetry without a dominant bright overtone
// --- Rhodes suitcase tremolo ---
constexpr float kRhodesTremoloRateHz     = 5.8f;        // default LFO rate
constexpr float kRhodesTremoloDepth      = 0.45f;       // amplitude modulation depth
constexpr float kRhodesTremoloPanDepth   = 0.35f;       // stereo pan modulation depth

// --- Wurlitzer reed model (FM / waveshaping) ---
constexpr float kWurliFMIndexBase        = 1.10f;       // initial phase-modulation depth, in radians
constexpr float kWurliFMIndexVelScale    = 0.90f;       // forte adds reed harmonics while retaining the fundamental
constexpr float kWurliModRatio           = 1.0f;        // modulator:carrier frequency ratio
constexpr float kWurliModDecayFactor     = 0.18f;       // reed edge settles into the body after the strike
constexpr float kWurliReedQ              = 2.6f;        // broad reed resonance rather than a ringing narrow mode
// --- Wurlitzer 200A internal amp saturation ---
constexpr float kWurliSatDriveBase       = 1.15f;       // clean soft playing; velocity supplies the bark
constexpr float kWurliSatVelScale        = 2.5f;        // velocity-dependent drive increase
constexpr float kWurliSatMix             = 0.70f;       // wet/dry saturation blend
// --- Wurlitzer per-voice tremolo ---
constexpr float kWurliTremoloRateHz      = 5.2f;        // default LFO rate
constexpr float kWurliTremoloDepth       = 0.40f;       // amplitude modulation depth
constexpr float kWurliTremoloPanDepth    = 0.30f;       // stereo pan modulation depth

// --- Clavinet string model ---
constexpr int   kClavMaxPartials         = 10;          // string partials
constexpr float kClavStringDecayBase     = 1.5f;        // string decay seconds
constexpr float kClavHammerWidthSec      = 0.003f;      // hammer contact duration
// --- Clavinet pickup selector ---
constexpr float kClavPickupAFreq         = 180.0f;      // retain the string body at the neck pickup
constexpr float kClavPickupBFreq         = 900.0f;      // bridge pickup remains more incisive
constexpr float kClavBrilliantFreq       = 3500.0f;     // brilliant filter cutoff
constexpr float kClavTrebleFreq          = 2000.0f;     // treble filter cutoff
// --- Clavinet string release ---
constexpr float kClavReleaseDecaySec     = 0.065f;      // string release ring time
constexpr float kClavReleaseNoiseLevel   = 0.055f;      // mechanical release noise
constexpr float kClavReleaseRingMix      = 0.30f;       // string ring on release
// --- Clavinet per-voice tremolo (subtle, auto-wah style) ---
constexpr float kClavTremoloRateHz       = 4.5f;        // default LFO rate
constexpr float kClavTremoloDepth        = 0.25f;       // amplitude modulation depth
constexpr float kClavTremoloPanDepth     = 0.20f;       // stereo pan modulation depth

// =========================================================================
// Piano families
// =========================================================================
enum class Family { Concert = 0, Vintage, Electric };
enum class LfoDestination { Off = 0, Tremolo, AutoPan, ChorusMotion };

enum class PianoRenderEngineMode
{
    LegacyFamily = 0,
    V2,
    V2ModelOnly
};

enum class PianoEngineReadiness
{
    TargetOnly = 0,
    DedicatedVoice
};

enum class PianoInstrumentAlgorithm
{
    SteinwayConcertDuplexSoundboard = 0,
    BosendorferImperialBassResonator,
    YamahaCfxBrightScaleAction,
    BastringueHonkyTonkTackRail,
    PreparedPianoObjectBuzz,
    RhodesTinePickupBark,
    WurlitzerReedAmpBite,
    ClavinetPickupStringSnap
};

struct PianoInstrumentModel
{
    const char* modelId = "";
    const char* targetEngineId = "";
    Family family = Family::Concert;
    PianoInstrumentAlgorithm algorithm = PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard;
    PianoEngineReadiness readiness = PianoEngineReadiness::TargetOnly;
    const char* voiceClass = "";
    const char* exciterModel = "";
    const char* resonatorModel = "";
    const char* auditionFocus = "";
};

struct StringCharacteristics
{
    float inharmonicityScale = 1.0f; // multiplier applied to the reference per-note B table
    float unisonDetuning = 0.0f;
    int partialCount = 0;
    float sympatheticIntervalSemis = 12.0f;
    bool hasDampers = true;
    float duplexScale = 0.0f;     // 0 = no duplex, 1 = full duplex resonance
};

struct ExcitationCharacteristics
{
    float hammerHardnessBase = 0.5f;
};

struct ResonanceCharacteristics
{
    float soundboardQ = 0.0f;
    float bodyDelayRatio = 1.0f;
    float bodyDamping = 0.0f;
};

struct EnvelopeCharacteristics
{
    float decay1Ratio = 0.35f;        // "prompt sound" ratio
    float decay2Ratio = 2.5f;         // "after sound" ratio
    float sustainPlatform = 0.30f;
    float promptSoundRatio = 0.12f;   // prompt-sound level (initial burst)
    float afterSoundRatio  = 0.50f;   // after-sound blend target
    float sustainDecayCoeff = kSustainDecayCoeff; // per-piano natural sustain decay
};

struct OutputCharacteristics
{
    bool isElectric = false;
};

// =========================================================================
// Per-piano synthesis character (not user-editable)
// =========================================================================
// =========================================================================
// Railsback stretched tuning correction (acoustic pianos only)
// Values in cents for MIDI notes 21-108 (88 keys), approximating the
// measured inharmonicity-driven stretch of concert grand pianos.
// =========================================================================
// Returns actual reference inharmonicity coefficient B for a given MIDI note.
inline float getReferenceInharmonicityB(int midiNote) noexcept
{
    const int raw = midiNote - 21;
    const int idx = raw < 0 ? 0 : (raw > 87 ? 87 : raw);
    return static_cast<float>(kInharmonicityBLut[idx]) * 1e-6f;
}

inline float getScaledInharmonicityB(int midiNote, const StringCharacteristics& string) noexcept
{
    const float scale = string.inharmonicityScale > 0.0f ? string.inharmonicityScale : 0.0f;
    const float value = getReferenceInharmonicityB(midiNote) * scale;
    return value > kMaxInharmonicityB ? kMaxInharmonicityB : value;
}

constexpr float kRailsbackCents[88] = {
    // A0–B0 (notes 21–23)
    -28.0f, -26.0f, -24.5f,
    // C1–B1 (notes 24–35)
    -23.0f, -21.5f, -20.0f, -18.5f, -17.0f, -15.5f, -14.0f, -13.0f, -12.0f, -11.0f, -10.0f, -9.0f,
    // C2–B2 (notes 36–47)
    -8.0f, -7.2f, -6.4f, -5.6f, -5.0f, -4.4f, -3.8f, -3.3f, -2.8f, -2.4f, -2.0f, -1.6f,
    // C3–B3 (notes 48–59)
    -1.2f, -0.8f, -0.5f, -0.2f, 0.0f, 0.2f, 0.5f, 0.8f, 1.0f, 1.2f, 1.4f, 1.7f,
    // C4–B4 (notes 60–71)
    2.0f, 2.3f, 2.7f, 3.1f, 3.5f, 3.9f, 4.4f, 4.9f, 5.4f, 5.9f, 6.5f, 7.1f,
    // C5–B5 (notes 72–83)
    7.8f, 8.5f, 9.3f, 10.1f, 11.0f, 11.9f, 12.9f, 13.9f, 15.0f, 16.1f, 17.3f, 18.5f,
    // C6–B6 (notes 84–95)
    19.8f, 21.2f, 22.6f, 24.0f, 25.4f, 26.8f, 28.2f, 29.5f, 30.7f, 31.9f, 33.0f, 34.0f,
    // C7–C8 (notes 96–108 = 13 notes)
    35.0f, 36.0f, 37.0f, 38.0f, 39.0f, 40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f
};

// Returns Railsback pitch correction in cents for a given MIDI note, scaled by railsbackAmount.
// Only valid for midiNote in [21, 108].
inline float getRailsbackCents(int midiNote, float railsbackAmount) noexcept
{
    const int raw = midiNote - 21;
    const int idx = raw < 0 ? 0 : (raw > 87 ? 87 : raw);
    return kRailsbackCents[idx] * railsbackAmount;
}

struct PianoCharacteristics
{
    StringCharacteristics string;
    ExcitationCharacteristics excitation;
    ResonanceCharacteristics resonance;
    EnvelopeCharacteristics envelope;
    OutputCharacteristics output;
    float railsbackScale = 0.0f;   // 0 = ET, 1 = full concert grand stretch
};

// =========================================================================
// Per-piano user parameters
// =========================================================================
struct ToneSettings
{
    float hammerHardness = 0.5f;
    float brightness = 0.5f;
    float lowPassHz = 12000.0f;
    float highPassHz = 0.0f;  // FIX 1.3: 0 = off, 20-200Hz = HP enabled
};

struct EnvelopeSettings
{
    float attackSeconds = 0.005f;
    float decaySeconds = 4.0f;
    float sustainLevel = 0.25f;
    float releaseSeconds = 0.3f;
};

struct ResonanceSettings
{
    float stringResonance = 0.3f;
    float soundboardAmount = 0.5f;
    float damping = 0.4f;
};

struct PerformanceSettings
{
    float modelCharacter = 0.5f;
    float tremoloRateHz  = 0.0f;   // 0 = use model default constant
};

struct SpatialSettings
{
    float pan = 0.0f;
};

// =========================================================================
// Global FX settings stored per-preset
// =========================================================================
struct GlobalFxSettings
{
    // Saturator
    float satDrive         = 1.8f;
    float satMix           = 0.15f;
    // Transient
    float transientAttack  = 0.10f;
    float transientSustain = 0.0f;
    float transientMix     = 0.4f;
    // EQ
    float eqLowFreq   = 200.0f;
    float eqLowGain   = 0.0f;
    float eqMidFreq   = 1000.0f;
    float eqMidGain   = 0.0f;
    float eqMidQ       = 1.0f;
    float eqHighFreq  = 5000.0f;
    float eqHighGain  = 0.0f;
    // Compressor
    float compThreshold = -19.0f;
    float compRatio     = 3.0f;
    float compAttack    = 10.0f;
    float compRelease   = 120.0f;
    float compMakeup    = 0.0f;
    float compMix       = 1.0f;
    // Chorus
    float chorusRate    = 1.0f;
    float chorusDepth   = 0.5f;
    float chorusMix     = 0.0f;
    // Delay
    float delayTime     = 300.0f;
    float delayFeedback = 0.30f;
    float delayMix      = 0.0f;
    bool delaySync      = false;
    int delayNoteDivision = 1;
    // Reverb (Dattorro)
    int reverbType       = 0;
    float reverbSize     = 0.55f;
    float reverbDamping  = 0.50f;
    float reverbWidth    = 0.80f;
    float reverbMix      = 0.25f;
    float reverbPredelay = 10.0f;
    // Limiter
    float limiterThreshold = -0.3f;
    float limiterRelease   = 50.0f;
    // FX enables
    bool reverbEnabled = true;
    bool saturationEnabled = true;
    bool transientEnabled = true;
    bool compressorEnabled = true;
    bool eqEnabled = true;
    bool chorusEnabled = true;
    bool delayEnabled = true;
    bool limiterEnabled = true;
};

struct PianoSettings
{
    float level = 0.8f;
    float tuneSemitones = 0.0f;
    ToneSettings tone;
    EnvelopeSettings envelope;
    ResonanceSettings resonance;
    PerformanceSettings performance;
    SpatialSettings spatial;

    constexpr PianoSettings() = default;

    constexpr PianoSettings(float levelIn,
                            float tuneSemitonesIn,
                            float hammerHardnessIn,
                            float attackSecondsIn,
                            float decaySecondsIn,
                            float sustainLevelIn,
                            float releaseSecondsIn,
                            float brightnessIn,
                            float stringResonanceIn,
                            float soundboardAmountIn,
                            float dampingIn,
                            float modelCharacterIn,
                            float lowPassHzIn,
                            float panIn) noexcept
        : level(levelIn),
          tuneSemitones(tuneSemitonesIn),
          tone { hammerHardnessIn, brightnessIn, lowPassHzIn },
          envelope { attackSecondsIn, decaySecondsIn, sustainLevelIn, releaseSecondsIn },
          resonance { stringResonanceIn, soundboardAmountIn, dampingIn },
          performance { modelCharacterIn },
          spatial { panIn }
    {
    }
};

// =========================================================================
// FX availability per instrument
// =========================================================================
enum class GlobalFxSlot
{
    Saturator = 0,
    Transient,
    Compressor,
    Eq,
    Chorus,
    Delay,
    Reverb,
    Limiter
};

struct FxAvailability
{
    bool saturator  = true;
    bool transient  = true;
    bool compressor = true;
    bool eq         = true;
    bool chorus     = false;
    bool delay      = true;
    bool reverb     = true;
    bool limiter    = true;
};

// =========================================================================
// Accessors (definitions in PianoDefs.cpp)
// =========================================================================
Family      getFamily               (int pianoIndex);
int         getFamilyStartIndex     (Family family);
const char* getFamilyName           (int familyIndex);
const char* getPianoName            (int pianoIndex);
const char* getPianoShortName       (int pianoIndex);
const PianoCharacteristics& getCharacteristics (int pianoIndex);
PianoSettings               getDefaultSettings (int pianoIndex);
const char*                 getPianoDescription(int pianoIndex);
const FxAvailability&       getFxAvailability  (int pianoIndex);
bool                        isFxAvailable      (int pianoIndex, GlobalFxSlot slot);
GlobalFxSettings            maskUnavailableFx  (int pianoIndex, const GlobalFxSettings& fx);

const std::array<PianoInstrumentModel, kNumPianos>& getPianoInstrumentModels() noexcept;
const PianoInstrumentModel& getPianoInstrumentModel(int pianoIndex) noexcept;
PianoInstrumentAlgorithm    getPianoInstrumentAlgorithm(int pianoIndex) noexcept;
const char*                 getPianoInstrumentAlgorithmName(PianoInstrumentAlgorithm algorithm) noexcept;
const char*                 getPianoEngineReadinessName(PianoEngineReadiness readiness) noexcept;
const char*                 getPianoRenderEngineModeName(PianoRenderEngineMode mode) noexcept;
PianoRenderEngineMode       getPianoRenderEngineMode() noexcept;
void                        setPianoRenderEngineMode(PianoRenderEngineMode mode) noexcept;
bool                        isPianoDedicatedVoiceActive() noexcept;

} // namespace mps
