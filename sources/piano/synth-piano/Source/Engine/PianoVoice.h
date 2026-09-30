#pragma once

#include <JuceHeader.h>
#include "PianoDefs.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

namespace mps
{

struct VoiceRealtimeModulation
{
    float cutoffMul = 1.0f;
    float resonanceOffset = 0.0f;
    float attackScale = 1.0f;
    float decayScale = 1.0f;
    float densityResonanceScale = 1.0f;
};

struct VoiceRenderContext
{
    int activeVoiceCount = 0;
    int releasingVoiceCount = 0;
    float density = 0.0f;
    float releasePressure = 0.0f;
    float sustainPressure = 0.0f;
    float repeatedNotePressure = 0.0f;
    float harmonicCollision = 0.0f;
    float registerPressure = 0.0f;
    float tailOwnership = 1.0f;
};

class PianoVoice
{
public:
    virtual ~PianoVoice() = default;

    virtual void noteOn(const PianoSettings& settings,
                        int midiNote, float velocity, double sampleRate) = 0;
    virtual void noteOff() = 0;
    virtual void render(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) = 0;

    virtual bool isActive() const noexcept = 0;
    virtual bool isReleasing() const noexcept = 0;
    virtual int getMidiNote() const noexcept = 0;
    virtual float getLevelEstimate() const noexcept = 0;

    virtual void setPitchBendFactor(float f) noexcept = 0;
    virtual void setRandomSeed(std::int64_t seed) noexcept = 0;
    virtual void applyRealtimeModulation(const VoiceRealtimeModulation& modulation) noexcept = 0;
    virtual void setRenderContext(const VoiceRenderContext& /*context*/) noexcept {}

    /// Force a fast 64-sample exponential release (used by dying-voice fade-out)
    virtual void forceQuickRelease() noexcept = 0;

    /// Type flag for RT-safe downcasting (avoids dynamic_cast on audio thread)
    virtual bool isAcoustic() const noexcept { return false; }

    /// Re-sustain: recapture a releasing voice when pedal is re-pressed
    virtual void reSustain() {}

    /// Una corda (soft pedal) state
    virtual void setUnaCorda(bool /*on*/) noexcept {}
};

class AcousticPianoVoiceBase : public PianoVoice
{
public:
    void noteOn(const PianoSettings& settings,
                int midiNote, float velocity, double sampleRate) override;
    void noteOff() override;
    void render(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override;

    bool isActive() const noexcept override { return envState != EnvState::Off; }
    bool isReleasing() const noexcept override { return envState == EnvState::Release; }
    int  getMidiNote() const noexcept override { return midiNote; }
    float getLevelEstimate() const noexcept override { return envLevel; }

    void setPitchBendFactor(float f) noexcept override { pitchBendFactor = f; }
    void setRandomSeed(std::int64_t seed) noexcept override { pendingRandomSeed = seed; hasPendingRandomSeed = true; }
    void applyRealtimeModulation(const VoiceRealtimeModulation& modulation) noexcept override;
    void setRenderContext(const VoiceRenderContext& context) noexcept override { renderContext = context; }
    void forceQuickRelease() noexcept override;

    bool isAcoustic() const noexcept override { return true; }
    void reSustain() override;
    void setUnaCorda(bool on) noexcept override { unaCordaActive = on; }

    // Half-pedaling: continuous damper position (0 = up, 1 = fully down)
    void setDamperPosition(float pos) noexcept { damperPosition = juce::jlimit(0.0f, 1.0f, pos); }

private:
    // 5-phase envelope: Attack → PromptSound → AfterSound → Sustain → Release
    enum class EnvState { Off, Attack, PromptSound, AfterSound, Sustain, Release };

    float readComb(const float* buf, int bufSize, int writePos, float delaySamples) const;
    virtual int getPianoIndex() const noexcept = 0;
    virtual PianoCharacteristics getCharacteristics() const = 0;
    virtual PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const;
    virtual bool shouldUseThirdString(const PianoCharacteristics& characteristics, int midiNote) const;
    virtual float blendDetunedSignal(float mainSignal, float detunedSignal) const;
    virtual float applyCharacterProcessing(float signal, const PianoSettings& settings) const;
    PianoSettings applyDedicatedSettingsProfile(PianoSettings settings, int midiNote, float velocity) const;
    void initialiseDedicatedModel(float f0, float fsr) noexcept;
    float applyDedicatedAcousticModel(float signal) noexcept;
    void updateRealtimeEnvelope() noexcept;
    void updateRealtimeFilter() noexcept;
    void updateMaxAgeSamples() noexcept;

    int pianoIndex = 0;
    PianoInstrumentAlgorithm pianoAlgorithm = PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard;
    bool pianoDedicatedActive = false;
    bool pianoModelOnly = false;
    float dedicatedPhaseA = 0.0f;
    float dedicatedPhaseB = 0.0f;
    float dedicatedPhaseIncA = 0.0f;
    float dedicatedPhaseIncB = 0.0f;
    float dedicatedEnvA = 0.0f;
    float dedicatedEnvB = 0.0f;
    float dedicatedDecayA = 1.0f;
    float dedicatedDecayB = 1.0f;
    float dedicatedNoiseEnv = 0.0f;
    float dedicatedNoiseDecay = 1.0f;
    float dedicatedFilterState = 0.0f;
    float dedicatedBodyState = 0.0f;
    float dedicatedBodyCoeff = 0.0f;

    PianoSettings         settings{};
    PianoCharacteristics  chars{};
    double sr       = 44100.0;
    float  vel      = 0.0f;
    int    midiNote = -1;

    EnvState envState = EnvState::Off;

    // Multi-partial oscillator bank (up to kMaxPartials = 32)
    struct PartialState
    {
        float phase     = 0.0f;
        float phaseInc  = 0.0f;
        float amplitude = 0.0f;
        float decayCoeff = 1.0f;
    };
    static_assert(std::is_standard_layout_v<PartialState>);
    static_assert(sizeof(PartialState) == sizeof(float) * 4);
    static_assert(offsetof(PartialState, phase) == 0);
    static_assert(offsetof(PartialState, phaseInc) == sizeof(float));
    static_assert(offsetof(PartialState, amplitude) == sizeof(float) * 2);
    static_assert(offsetof(PartialState, decayCoeff) == sizeof(float) * 3);
    std::array<PartialState, kMaxPartials> partials{};
    int numActivePartials = 0;

    // Detuned second partial bank (for string detuning / honky-tonk)
    std::array<PartialState, kMaxPartials> detunedPartials{};
    bool hasDetuning = false;

    // Third string bank (acoustic unison: 3 strings per note above ~F3)
    std::array<PartialState, kMaxPartials> string3Partials{};
    bool hasThirdString = false;

    // Duplex resonance (upper partials above treble bridge, note > 60)
    std::array<PartialState, kMaxDuplexPartials> duplexPartials{};
    int numDuplexPartials = 0;

    // 5-phase envelope
    float envLevel            = 0.0f;
    float attackRate          = 0.0f;
    float promptSoundCoeff    = 1.0f;   // prompt-sound decay
    float promptSoundTarget   = 0.0f;   // level where prompt→after transition
    float afterSoundCoeff     = 1.0f;   // after-sound decay
    float afterSoundTarget    = 0.0f;   // level where after→sustain transition
    float sustainLevel        = 0.0f;   // sustain platform level
    float releaseCoeff        = 1.0f;

    // Stulov hammer model (non-linear felt excitation)
    float stulovForce        = 0.0f;    // current hammer force
    float stulovCompression  = 0.0f;    // felt compression state
    float stulovDecayCoeff   = 1.0f;    // contact duration decay
    float stulovStiffness    = 0.0f;    // register-scaled stiffness
    float stulovGainScale    = kStulovOutputScale; // SR-adapted excitation scaling

    // Hammer noise (spectral coloring of Stulov excitation)
    float hammerEnvLevel    = 0.0f;
    float hammerDecayCoeff  = 1.0f;
    float hammerFiltState   = 0.0f;
    float hammerFiltCoeff   = 0.0f;

    // Key click (mechanical noise on key-down)
    float keyClickLevel     = 0.0f;
    float keyClickDecay     = 1.0f;
    int   keyClickRampLen   = 0;      // 0.5ms onset ramp
    int   keyClickRampPos   = 0;
    // Key release click
    float keyRelClickLevel  = 0.0f;
    float keyRelClickDecay  = 1.0f;
    int   keyRelClickRampLen = 0;
    int   keyRelClickRampPos = 0;

    // Damper thud (mechanical release articulation)
    float damperThudLevel     = 0.0f;
    float damperThudDecay     = 1.0f;
    float damperThudFiltState = 0.0f;
    int   damperThudRampLen   = 0;     // onset ramp length in samples
    int   damperThudRampPos   = 0;     // current position in ramp
    int   releaseDelaySamples = 0;     // half-pedal damper settling delay

    // Half-pedaling
    float damperPosition = 0.0f;    // 0 = up, 1 = fully down

    // Una corda (soft pedal)
    bool unaCordaActive = false;

    // SVF filter state
    float svfLow     = 0.0f;
    float svfBand    = 0.0f;
    float filterF    = 0.0f;
    float filterQinv = 0.0f;
    float filterFCurrent       = 0.0f;
    float filterFTarget        = 0.0f;
    // FIX 1.3: Highpass filter state
    float hpState              = 0.0f;
    float hpPrevInput          = 0.0f;
    float hpCoeff              = 0.0f;
    float brightnessDecayCoeff = 1.0f;
    VoiceRealtimeModulation realtimeModulation {};
    VoiceRenderContext renderContext {};

    // Velocity-derived envelope modulation scales (P7/P13 fix)
    // Isolated from settings to prevent side-effects on shared state
    struct VelocityMod
    {
        float sustainScale = 1.0f;  // multiplicative sustain boost from velocity
        float decayScale   = 1.0f;  // multiplicative decay speedup from velocity
    };
    VelocityMod velocityMod {};

    // Body resonator (comb filter) — heap-allocated buffer pool
    static constexpr int kBodyBufSize = 8192;
    std::vector<float> bodyBuf = std::vector<float>(kBodyBufSize, 0.0f);
    int   bodyWritePos     = 0;
    float bodyDelaySamples = 100.0f;
    float bodyFeedback     = 0.0f;
    float bodyDampState    = 0.0f;

    // Sympathetic resonance matrix — 12 coupled comb filters (heap-allocated)
    static constexpr int kSympCombBufSize = 4096;
    struct SympResonator
    {
        std::vector<float> buf = std::vector<float>(kSympCombBufSize, 0.0f);
        int   writePos     = 0;
        float delaySamples = 50.0f;
        float feedback     = 0.0f;
        float dampState    = 0.0f;
    };
    std::array<SympResonator, kSympMatrixSize> sympMatrix{};
    float sympMatrixGain = 0.0f;

    // Soundboard resonance — 8 calibrated biquad modes
    struct SoundboardMode
    {
        float state1  = 0.0f;
        float state2  = 0.0f;
        float coeffA  = 0.0f;    // 2*cos(w0)*r
        float coeffB  = 0.0f;    // -(r^2)
        float gain    = 0.0f;
    };
    std::array<SoundboardMode, kSoundboardModes> sbModes{};

    // Pitch bend
    float pitchBendFactor = 1.0f;

    // Pan
    float panL = 0.7071f;
    float panR = 0.7071f;

    // Noise
    juce::Random rng;
    std::int64_t pendingRandomSeed = 0;
    bool hasPendingRandomSeed = false;

    int ageSamples    = 0;
    int maxAgeSamples = 0;
};

// =========================================================================
// Electric piano engine — instrument-specific synthesis via virtual hooks.
// Base provides: envelope, SVF filter, pan, output chain.
// Subclasses override: onNoteOn, onNoteOff, generateSample.
// =========================================================================
class ElectricPianoVoiceBase : public PianoVoice
{
public:
    void noteOn(const PianoSettings& settings,
                int midiNote, float velocity, double sampleRate) override;
    void noteOff() override;
    void render(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override;

    bool isActive() const noexcept override { return envState != EnvState::Off; }
    bool isReleasing() const noexcept override { return envState == EnvState::Release; }
    int  getMidiNote() const noexcept override { return midiNote; }
    float getLevelEstimate() const noexcept override { return envLevel; }

    void setPitchBendFactor(float f) noexcept override { pitchBendFactor = f; }
    void setRandomSeed(std::int64_t seed) noexcept override { pendingRandomSeed = seed; hasPendingRandomSeed = true; }
    void applyRealtimeModulation(const VoiceRealtimeModulation& modulation) noexcept override;
    void setRenderContext(const VoiceRenderContext& context) noexcept override { renderContext = context; }
    void forceQuickRelease() noexcept override;

protected:
    enum class EnvState { Off, Attack, Decay1, Decay2, Release };

    // --- Virtual hooks for instrument-specific synthesis ---
    virtual int getPianoIndex() const noexcept = 0;
    virtual PianoCharacteristics getCharacteristics() const = 0;
    virtual PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const;
    virtual float applyCharacterProcessing(float signal, const PianoSettings& settings) const;
    PianoSettings applyDedicatedSettingsProfile(PianoSettings settings, int midiNote, float velocity) const;
    void initialiseDedicatedModel(float f0, float fsr) noexcept;
    float applyDedicatedElectricModel(float signal) noexcept;
    void updateRealtimeEnvelope() noexcept;
    void updateRealtimeFilter() noexcept;
    void updateMaxAgeSamples() noexcept;

    // Called at end of noteOn — set up instrument-specific oscillators/state
    virtual void onNoteOn(float f0, float fsr);
    // Called at start of noteOff — instrument-specific release behavior
    virtual void onNoteOff();
    // Per-sample signal generation — returns raw instrument signal
    virtual float generateSample(float pitchFactor);

    // --- Shared state accessible to subclasses ---
    int pianoIndex = 0;
    PianoInstrumentAlgorithm pianoAlgorithm = PianoInstrumentAlgorithm::RhodesTinePickupBark;
    bool pianoDedicatedActive = false;
    bool pianoModelOnly = false;
    float dedicatedPhaseA = 0.0f;
    float dedicatedPhaseB = 0.0f;
    float dedicatedPhaseIncA = 0.0f;
    float dedicatedPhaseIncB = 0.0f;
    float dedicatedEnvA = 0.0f;
    float dedicatedEnvB = 0.0f;
    float dedicatedDecayA = 1.0f;
    float dedicatedDecayB = 1.0f;
    float dedicatedNoiseEnv = 0.0f;
    float dedicatedNoiseDecay = 1.0f;
    float dedicatedFilterState = 0.0f;

    PianoSettings         settings{};
    PianoCharacteristics  chars{};
    double sr       = 44100.0;
    float  vel      = 0.0f;
    int    midiNote = -1;

    EnvState envState = EnvState::Off;

    // Partial oscillator bank (default synthesis, used by onNoteOn/generateSample defaults)
    static constexpr int kElecMaxPartials = 12;
    struct PartialState
    {
        float phase     = 0.0f;
        float phaseInc  = 0.0f;
        float amplitude = 0.0f;
        float decayCoeff = 1.0f;
    };
    static_assert(std::is_standard_layout_v<PartialState>);
    static_assert(sizeof(PartialState) == sizeof(float) * 4);
    static_assert(offsetof(PartialState, phase) == 0);
    static_assert(offsetof(PartialState, phaseInc) == sizeof(float));
    static_assert(offsetof(PartialState, amplitude) == sizeof(float) * 2);
    static_assert(offsetof(PartialState, decayCoeff) == sizeof(float) * 3);
    std::array<PartialState, kElecMaxPartials> partials{};
    int numActivePartials = 0;

    // Envelope
    float envLevel     = 0.0f;
    float attackRate   = 0.0f;
    float decay1Coeff  = 1.0f;
    float decay1Target = 0.0f;
    float decay2Coeff  = 1.0f;
    float releaseCoeff = 1.0f;

    // Excitation (tine / hammer)
    float hammerEnvLevel    = 0.0f;
    float hammerDecayCoeff  = 1.0f;
    float hammerFiltState   = 0.0f;
    float hammerFiltCoeff   = 0.0f;

    // SVF filter state
    float svfLow     = 0.0f;
    float svfBand    = 0.0f;
    float filterF    = 0.0f;
    float filterQinv = 0.0f;
    float filterFCurrent      = 0.0f;
    float filterFTarget       = 0.0f;
    float brightnessDecayCoeff = 1.0f;
    VoiceRealtimeModulation realtimeModulation {};
    VoiceRenderContext renderContext {};

    // Pitch bend
    float pitchBendFactor = 1.0f;

    // Pan
    float panL = 0.7071f;
    float panR = 0.7071f;

    // Noise
    juce::Random rng;
    std::int64_t pendingRandomSeed = 0;
    bool hasPendingRandomSeed = false;

    int ageSamples    = 0;
    int maxAgeSamples = 0;
};

class SteinwayVoice final : public AcousticPianoVoiceBase
{
private:
    int getPianoIndex() const noexcept override { return 0; }
    PianoCharacteristics getCharacteristics() const override;
    PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const override;
};

class BosendorferVoice final : public AcousticPianoVoiceBase
{
private:
    int getPianoIndex() const noexcept override { return 1; }
    PianoCharacteristics getCharacteristics() const override;
    PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const override;
    bool shouldUseThirdString(const PianoCharacteristics& characteristics, int midiNote) const override;
};

class YamahaVoice final : public AcousticPianoVoiceBase
{
private:
    int getPianoIndex() const noexcept override { return 2; }
    PianoCharacteristics getCharacteristics() const override;
    PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const override;
};

class BastringueVoice final : public AcousticPianoVoiceBase
{
private:
    int getPianoIndex() const noexcept override { return 3; }
    PianoCharacteristics getCharacteristics() const override;
    PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const override;
    bool shouldUseThirdString(const PianoCharacteristics& characteristics, int midiNote) const override;
    float blendDetunedSignal(float mainSignal, float detunedSignal) const override;
};

class PreparedPianoVoice final : public AcousticPianoVoiceBase
{
private:
    int getPianoIndex() const noexcept override { return 4; }
    PianoCharacteristics getCharacteristics() const override;
    PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const override;
    bool shouldUseThirdString(const PianoCharacteristics& characteristics, int midiNote) const override;
};

class RhodesVoice final : public ElectricPianoVoiceBase
{
private:
    int getPianoIndex() const noexcept override { return 5; }
    PianoCharacteristics getCharacteristics() const override;
    PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const override;
    float applyCharacterProcessing(float signal, const PianoSettings& settings) const override;

    void  onNoteOn(float f0, float fsr) override;
    float generateSample(float pitchFactor) override;

    // Tine oscillator (fast attack, bell-like decay)
    float tinePhase     = 0.0f;
    float tinePhaseInc  = 0.0f;
    float tineAmp       = 0.0f;
    float tineDecayCoeff = 1.0f;

    // Tonebar oscillator (slow sustain, warmth)
    float tonebarPhase     = 0.0f;
    float tonebarPhaseInc  = 0.0f;
    float tonebarAmp       = 0.0f;
    float tonebarDecayCoeff = 1.0f;

    // Non-linear tine→tonebar coupling
    float couplingState = 0.0f;

    // Bell partials (upper metallic shimmer on attack)
    struct BellPartial { float phase = 0.0f, phaseInc = 0.0f, amp = 0.0f, decayCoeff = 1.0f; };
    static_assert(std::is_standard_layout_v<BellPartial>);
    static_assert(sizeof(BellPartial) == sizeof(float) * 4);
    static_assert(offsetof(BellPartial, phase) == 0);
    static_assert(offsetof(BellPartial, phaseInc) == sizeof(float));
    static_assert(offsetof(BellPartial, amp) == sizeof(float) * 2);
    static_assert(offsetof(BellPartial, decayCoeff) == sizeof(float) * 3);
    std::array<BellPartial, kRhodesBellPartials> bellPartials{};

    // Suitcase tremolo LFO
    float tremoloPhase    = 0.0f;
    float tremoloPhaseInc = 0.0f;
    float tremoloDepth    = 0.0f;
};

class WurlitzerVoice final : public ElectricPianoVoiceBase
{
private:
    int getPianoIndex() const noexcept override { return 6; }
    PianoCharacteristics getCharacteristics() const override;
    PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const override;
    float applyCharacterProcessing(float signal, const PianoSettings& settings) const override;

    void  onNoteOn(float f0, float fsr) override;
    float generateSample(float pitchFactor) override;

    // FM synthesis: carrier + modulator
    float carrierPhase     = 0.0f;
    float carrierPhaseInc  = 0.0f;
    float modulatorPhase   = 0.0f;
    float modulatorPhaseInc = 0.0f;
    float fmIndex          = 0.0f;
    float fmIndexDecayCoeff = 1.0f;
    float fmBaseIndex      = 0.0f;

    // Reed bandpass resonance (biquad)
    float reedState1 = 0.0f;
    float reedState2 = 0.0f;
    float reedA1     = 0.0f;
    float reedA2     = 0.0f;
    float reedB0     = 0.0f;
    float reedB2     = 0.0f;

    // Internal amp saturation drive
    float satDrive = 1.0f;

    // Per-voice tremolo LFO
    float tremoloPhase    = 0.0f;
    float tremoloPhaseInc = 0.0f;
    float tremoloDepth    = 0.0f;
};

class ClavinetVoice final : public ElectricPianoVoiceBase
{
private:
    int getPianoIndex() const noexcept override { return 7; }
    PianoCharacteristics getCharacteristics() const override;
    PianoSettings adaptSettings(const PianoSettings& settings, int midiNote, float velocity) const override;
    float applyCharacterProcessing(float signal, const PianoSettings& settings) const override;

    void  onNoteOn(float f0, float fsr) override;
    void  onNoteOff() override;
    float generateSample(float pitchFactor) override;

    // String partials (sharp odd harmonics)
    struct ClavPartial { float phase = 0.0f, phaseInc = 0.0f, amp = 0.0f, decayCoeff = 1.0f; };
    static_assert(std::is_standard_layout_v<ClavPartial>);
    static_assert(sizeof(ClavPartial) == sizeof(float) * 4);
    static_assert(offsetof(ClavPartial, phase) == 0);
    static_assert(offsetof(ClavPartial, phaseInc) == sizeof(float));
    static_assert(offsetof(ClavPartial, amp) == sizeof(float) * 2);
    static_assert(offsetof(ClavPartial, decayCoeff) == sizeof(float) * 3);
    std::array<ClavPartial, kClavMaxPartials> clavPartials{};
    int numClavPartials = 0;

    // Pickup highpass filter (simulates pickup position A/B blend)
    float pickupHPState = 0.0f;   // previous input x[n-1]
    float pickupHPOut   = 0.0f;   // previous output y[n-1]
    float pickupHPCoeff = 0.0f;

    // Tone filters (brilliant / treble emulation)
    float brillHPState  = 0.0f;
    float brillHPOut    = 0.0f;
    float brillHPCoeff  = 0.0f;
    float trebleLPState = 0.0f;
    float trebleLPCoeff = 0.0f;

    // String release on noteOff (key lifts, string rings briefly)
    float releaseRingLevel = 0.0f;
    float releaseRingDecay = 1.0f;
    float releaseNoiseLevel = 0.0f;
    float releaseNoiseDecay = 1.0f;

    // Per-voice tremolo LFO
    float tremoloPhase    = 0.0f;
    float tremoloPhaseInc = 0.0f;
    float tremoloDepth    = 0.0f;
};

std::unique_ptr<PianoVoice> createVoiceForPiano(int pianoIndex);

} // namespace mps
