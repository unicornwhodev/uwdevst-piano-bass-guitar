#pragma once

#include "GuitarDefs.h"

#include <array>
#include <cstdint>
#include <memory>

namespace mgs
{

struct VoiceModulation
{
    float cutoffMul = 1.0f;
    float resonanceAdd = 0.0f;
    float panAdd = 0.0f;
    float levelMul = 1.0f;
    float pitchSemi = 0.0f;
    float attackScale = 1.0f;
    float decayScale = 1.0f;
};

class GuitarVoice
{
public:
    virtual ~GuitarVoice() = default;

    virtual void noteOn(int midiNote, float velocity, double sampleRate,
                        const InstrSettings& settings,
                        uint32_t chordHash = 0) = 0;
    virtual void noteOff() = 0;
    virtual void renderBlock(float* outL, float* outR, int numSamples, double sampleRate) = 0;
    virtual bool isActive() const noexcept = 0;
    virtual bool isReleasing() const noexcept = 0;
    virtual void setPitchBendFactor(float factor) noexcept = 0;
    virtual void setTargetNote(int midiNote, double sampleRate) = 0;
    virtual void setPalmMute(float amount) noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual void updateFilter(float lowPassHz, float stringBrightness, double sr) noexcept = 0;
    virtual void forceQuickRelease() noexcept = 0;
    virtual void setVoiceModulation(const VoiceModulation& modulation, double sampleRate) noexcept = 0;
    virtual float getEnvelopeLevel() const noexcept = 0;
};

class GuitarVoiceBase : public GuitarVoice
{
public:
    explicit GuitarVoiceBase(int instrumentIndexIn);

    void noteOn(int midiNote, float velocity, double sampleRate,
                const InstrSettings& settings,
                uint32_t chordHash = 0) override;
    void noteOff() override;
    void renderBlock(float* outL, float* outR, int numSamples, double sampleRate) override;
    bool isActive() const noexcept override { return active; }
    bool isReleasing() const noexcept override { return envStage == Release; }
    void setPitchBendFactor(float factor) noexcept override { pitchBendFactor = factor; }
    void setTargetNote(int midiNote, double sampleRate) override;
    void setPalmMute(float amount) noexcept override { palmMuteTarget = amount; }
    void reset() noexcept override;
    void updateFilter(float lowPassHz, float stringBrightness, double sr) noexcept override;
    void forceQuickRelease() noexcept override;
    void setVoiceModulation(const VoiceModulation& modulation, double sampleRate) noexcept override;
    float getEnvelopeLevel() const noexcept override { return envLevel; }

protected:
    struct VoiceModel
    {
        float pitchFollow = -1.0f;
        float pitchReferenceHz = 220.0f;
        float stringBrightnessScale = 1.0f;
        float brightnessLift = 0.0f;
        float pluckBrightnessScale = 1.0f;
        float pluckAttackLift = 0.0f;
        float pluckPositionOffset = 0.0f;
        float outputTrim = 1.0f;
        float decayScale = 1.0f;
        float feedbackFloor = 0.90f;
        float feedbackCeil = 0.9999f;
        bool enableSecondaryString = false;
        float secondaryMix = 0.0f;
        float secondaryPitchRatio = 1.0f;
        float secondaryDetuneScale = 12.0f;
        float secondaryExciteScale = 0.7f;
        bool useContinuousExcitation = false;
        float continuousExciteLevel = 0.0f;
        float continuousExciteSustain = 0.0f;
        float exciteDecayScale = 1.0f;
        float exciteBrightnessScale = 1.0f;
        float bodyScale = 1.0f;
        float bodyFeedbackScale = 1.0f;
        float bodyBrightnessScale = 1.0f;
        bool enableSecondaryBodyModes = true;
        float driveBias = 0.0f;
        float driveScale = 1.0f;
        float cutoffScale = 1.0f;
        float attackScale = 1.0f;
        float releaseScale = 1.0f;
        float stereoWidthScale = 1.0f;
        float registerPanScale = 1.0f;
        bool enableFingerNoise = false;
        float fingerNoiseAmount = 0.0f;
        float fingerNoiseDecaySeconds = 0.008f;
        float ageLimitSeconds = 30.0f;

        // Phase 1.1 – enhanced Karplus-Strong
        float inharmonicityScale = 1.0f;
        float loopFilter2Bright = 0.0f;   // 0 = disabled; >0 adds second LP stage

        // Phase 1.2 – transient envelope stage
        float transientTimeScale = 1.0f;
        float transientLevel = 0.70f;     // target after transient (before decay)

        // Phase 1.3 – sympathetic string resonance
        float sympatheticGain = 0.0f;     // 0 = off; >0.01 enables

        // Phase 1.4 – extra body modes
        float body4FreqRatio = 0.0f;      // 0 = disabled
        float body5FreqRatio = 0.0f;
    };

    virtual VoiceModel buildVoiceModel(int midiNote, float velocity,
                                       double sampleRate,
                                       const InstrSettings& settings,
                                       const InstrCharacteristics& chars) const = 0;
    void applyRuntimeModelProfile(VoiceModel& voiceModel, const InstrCharacteristics& chars) const noexcept;

    const InstrCharacteristics& characteristics() const noexcept;
    float nextRandom();
    float readString(const std::array<float, 4096>& buf,
                     int writePos, float delaySamples) const;
    float readBody(float delaySamples) const;

    static constexpr int kMaxDelay = 4096;
    static constexpr int kBodyBufSize = 4096;

    const int instrumentIndex;
    VoiceModel model;

    std::array<float, kMaxDelay> stringBuf = {};
    int stringWritePos = 0;
    float stringLength = 100.0f;
    float stringFeedback = 0.99f;
    float stringFilterState = 0.0f;
    float stringBrightCoeff = 0.5f;
    float stringPickPos = 0.15f;
    float brightCoeffTarget = 0.5f;
    float brightCoeffCurrent = 0.5f;
    float brightDecayCoeff = 1.0f;

    // Phase 1.1 – allpass inharmonicity
    float apCoeff = 0.0f;
    float apState = 0.0f;
    float ap2State = 0.0f;            // secondary string

    // Phase 1.1 – second loop filter for frequency-dependent decay
    float stringFilterState2 = 0.0f;
    float stringBrightCoeff2 = 1.0f;
    float string2FilterState2 = 0.0f;

    std::array<float, kMaxDelay> string2Buf = {};
    int string2WritePos = 0;
    float string2Length = 100.0f;
    float string2FilterState = 0.0f;
    float string2Mix = 0.0f;

    float exciteLevel = 0.0f;
    float exciteDecayCoef = 0.999f;
    float exciteBright = 0.5f;
    float excitePrev = 0.0f;

    float fingerNoiseLevel = 0.0f;
    float fingerNoiseDecay = 1.0f;
    float fingerNoiseFiltState = 0.0f;
    float storedSR = 44100.0f;

    std::array<float, kBodyBufSize> bodyBuf = {};
    int bodyWritePos = 0;
    float bodyDelay = 0.0f;
    float bodyFeedback = 0.0f;
    float bodyDampState = 0.0f;
    float bodyDamping = 0.0f;

    float body2State1 = 0.0f, body2State2 = 0.0f;
    float body2CoeffA = 0.0f, body2CoeffB = 0.0f, body2Gain = 0.0f;
    float body3State1 = 0.0f, body3State2 = 0.0f;
    float body3CoeffA = 0.0f, body3CoeffB = 0.0f, body3Gain = 0.0f;

    // Phase 1.4 – additional body modes
    float body4State1 = 0.0f, body4State2 = 0.0f;
    float body4CoeffA = 0.0f, body4CoeffB = 0.0f, body4Gain = 0.0f;
    float body5State1 = 0.0f, body5State2 = 0.0f;
    float body5CoeffA = 0.0f, body5CoeffB = 0.0f, body5Gain = 0.0f;

    // Phase 1.3 – sympathetic string resonance (biquad resonators)
    static constexpr int kSympStrings = 6;
    struct SympResonator
    {
        float s1 = 0.0f, s2 = 0.0f;
        float a = 0.0f, b = 0.0f, gain = 0.0f;
    };
    std::array<SympResonator, kSympStrings> sympRes = {};
    float sympGain = 0.0f;
    float sympDecayCoeff = 0.9998f;

    float svfBand = 0.0f;
    float svfLow = 0.0f;
    float filterF = 0.0f;
    float filterQinv = 0.707f;
    float filterFTarget = 0.0f;
    float filterQinvTarget = 0.707f;

    float driveAmount = 1.0f;
    float driveNorm = 1.0f;
    float driveAdaaPreviousInput = 0.0f;
    float driveOsPreviousDry = 0.0f;

    static constexpr int kFadeOutSamples = 64;
    // Denormal threshold for filter state protection (IEEE 754 single-precision)
    static constexpr float kDenormalThreshold = 1e-15f;
    // Minimum signal threshold for "very small" comparisons
    static constexpr float kVerySmallSignalThreshold = 0.001f;
    // Round-robin pluck positions: bridge (0), middle (1), neck (2)
    static constexpr float kPluckPositions[3] = { 0.08f, 0.25f, 0.42f };
    // Round-robin brightness offsets per RR variant (additive to base brightness)
    static constexpr float kRrBrightnessOffsets[3] = { 0.03f, -0.01f, 0.06f };
    // Round-robin velocity offsets per RR variant (multiplicative)
    static constexpr float kRrVelocityScales[3] = { 1.02f, 0.98f, 1.01f };

    enum EnvStage { Off, Attack, Transient, Decay, Sustain, Release };
    EnvStage envStage = Off;
    int fadeOutRemaining = 0;
    float fadeOutCoeff = 0.0f;  // computed per-release in renderBlock
    float envLevel = 0.0f;
    float envAttackInc = 0.0f;
    float envDecayMul = 1.0f;
    float envSustain = 0.0f;
    float envRelMul = 1.0f;

    // Phase 1.2 – transient stage
    float envTransientMul = 1.0f;
    float envTransientTarget = 0.7f;

    float panL = 0.707f;
    float panR = 0.707f;
    float stereoWidth = 0.0f;

    float velocity = 0.0f;
    float levelGain = 1.0f;
    float baseLevelGain = 1.0f;
    bool active = false;
    float age = 0.0f;
    float pitchBendFactor = 1.0f;
    float modPitchFactor = 1.0f;
    float smoothedPitchFactor = 1.0f;
    float baseStringLength = 100.0f;
    float baseString2Length = 100.0f;
    float baseAttackSeconds = 0.003f;
    float baseDecaySeconds = 1.0f;
    float baseReleaseSeconds = 0.25f;
    float basePan = 0.0f;
    float baseBodyFeedback = 0.0f;
    float baseBody2Gain = 0.0f;
    float baseBody3Gain = 0.0f;
    float baseBody4Gain = 0.0f;
    float baseBody5Gain = 0.0f;
    bool quickReleaseForced = false;
    VoiceModulation currentModulation {};

    // Portamento (legato)
    float targetStringLength = 0.0f;
    float targetString2Length = 0.0f;
    float portamentoPitchFactor = 1.0f;
    float targetPortamentoPitchFactor = 1.0f;
    float portamentoPitchFactor2 = 1.0f;
    float targetPortamentoPitchFactor2 = 1.0f;
    float portamentoCoeff = 1.0f;
    bool portamentoActive = false;
    float storedTuneSemitones = 0.0f;

    // Palm mute
    float palmMuteTarget = 0.0f;
    float palmMuteCurrent = 0.0f;

    // Round-robin pluck variation (3 positions: bridge/middle/neck)
    uint8_t rrIndex = 0;   // current RR position (0-2), wraps on each noteOn
    uint8_t rrCount = 3;   // total variations

    // Strum direction (+1 = up-stroke, -1 = down-stroke) — set from chordHash
    int strumDirection = 1;

    // DC blocker (production-grade: prevents low-frequency drift from body resonance & drive)
    float dcX1 = 0.0f;
    float dcY1 = 0.0f;

    uint32_t rngState = 12345u;

private:
    void updateEnvelopeRatesFromModulation() noexcept;
    void updateStereoPanFromModulation() noexcept;
};

class FolkSteelVoice final : public GuitarVoiceBase
{
public:
    FolkSteelVoice();

private:
    VoiceModel buildVoiceModel(int midiNote, float velocity, double sampleRate,
                               const InstrSettings& settings,
                               const InstrCharacteristics& chars) const override;
};

class TwelveStringVoice final : public GuitarVoiceBase
{
public:
    TwelveStringVoice();

private:
    VoiceModel buildVoiceModel(int midiNote, float velocity, double sampleRate,
                               const InstrSettings& settings,
                               const InstrCharacteristics& chars) const override;
};

class FlamencaVoice final : public GuitarVoiceBase
{
public:
    FlamencaVoice();

private:
    VoiceModel buildVoiceModel(int midiNote, float velocity, double sampleRate,
                               const InstrSettings& settings,
                               const InstrCharacteristics& chars) const override;
};

class CleanVoice final : public GuitarVoiceBase
{
public:
    CleanVoice();

private:
    VoiceModel buildVoiceModel(int midiNote, float velocity, double sampleRate,
                               const InstrSettings& settings,
                               const InstrCharacteristics& chars) const override;
};

class CrunchVoice final : public GuitarVoiceBase
{
public:
    CrunchVoice();

private:
    VoiceModel buildVoiceModel(int midiNote, float velocity, double sampleRate,
                               const InstrSettings& settings,
                               const InstrCharacteristics& chars) const override;
};

class LeadVoice final : public GuitarVoiceBase
{
public:
    LeadVoice();

private:
    VoiceModel buildVoiceModel(int midiNote, float velocity, double sampleRate,
                               const InstrSettings& settings,
                               const InstrCharacteristics& chars) const override;
};

class SynthGuitarVoice final : public GuitarVoiceBase
{
public:
    SynthGuitarVoice();

private:
    VoiceModel buildVoiceModel(int midiNote, float velocity, double sampleRate,
                               const InstrSettings& settings,
                               const InstrCharacteristics& chars) const override;
};

class EPadVoice final : public GuitarVoiceBase
{
public:
    EPadVoice();

private:
    VoiceModel buildVoiceModel(int midiNote, float velocity, double sampleRate,
                               const InstrSettings& settings,
                               const InstrCharacteristics& chars) const override;
};

class AmbientVoice final : public GuitarVoiceBase
{
public:
    AmbientVoice();

private:
    VoiceModel buildVoiceModel(int midiNote, float velocity, double sampleRate,
                               const InstrSettings& settings,
                               const InstrCharacteristics& chars) const override;
};

std::unique_ptr<GuitarVoice> createVoiceForInstrument(int instrumentIndex);

} // namespace mgs
