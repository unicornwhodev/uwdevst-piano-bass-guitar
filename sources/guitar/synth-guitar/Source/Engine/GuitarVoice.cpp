#include "GuitarVoice.h"
#include "GuitarInstrumentModel.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
float clamp01(const float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float tanhAntiderivative(const float x) noexcept
{
    constexpr float kLogTwo = 0.6931471805599453f;
    const auto ax = std::abs(x);
    if (ax < 12.0f)
        return std::log(std::cosh(x));
    return ax - kLogTwo;
}

float adaaTanh(const float x, const float previousX) noexcept
{
    const auto delta = x - previousX;
    if (std::abs(delta) < 1.0e-5f)
        return std::tanh(0.5f * (x + previousX));

    return (tanhAntiderivative(x) - tanhAntiderivative(previousX)) / delta;
}
}

namespace mgs
{

GuitarVoiceBase::GuitarVoiceBase(const int instrumentIndexIn)
    : instrumentIndex(instrumentIndexIn)
{
    reset();
}

const InstrCharacteristics& GuitarVoiceBase::characteristics() const noexcept
{
    return getCharacteristics(instrumentIndex);
}

void GuitarVoiceBase::applyRuntimeModelProfile(VoiceModel& voiceModel,
                                               const InstrCharacteristics& chars) const noexcept
{
    const auto mode = getGuitarRenderEngineMode();
    if (mode == GuitarRenderEngineMode::LegacyFamily)
    {
        voiceModel.enableFingerNoise = false;
        voiceModel.fingerNoiseAmount = 0.0f;
        voiceModel.inharmonicityScale = std::min(voiceModel.inharmonicityScale, 0.30f);
        voiceModel.loopFilter2Bright = 0.0f;
        voiceModel.transientTimeScale = 1.0f;
        voiceModel.transientLevel = 0.82f;
        voiceModel.sympatheticGain = 0.0f;
        voiceModel.enableSecondaryBodyModes = false;
        voiceModel.body4FreqRatio = 0.0f;
        voiceModel.body5FreqRatio = 0.0f;
        voiceModel.bodyFeedbackScale = 1.0f;
        voiceModel.bodyBrightnessScale = 1.0f;
        voiceModel.driveBias = 0.0f;
        voiceModel.driveScale = std::min(voiceModel.driveScale, 1.0f);
        voiceModel.stereoWidthScale = 1.0f;
        voiceModel.registerPanScale = 1.0f;
        if (chars.synthMode == SynthMode::KarplusStrong)
        {
            voiceModel.useContinuousExcitation = false;
            voiceModel.continuousExciteLevel = 0.0f;
        }
        return;
    }

    const auto& profile = getGuitarInstrumentModelProfile(instrumentIndex);
    voiceModel.pluckBrightnessScale = std::clamp(voiceModel.pluckBrightnessScale * profile.pluckFocus, 0.15f, 2.20f);
    voiceModel.stringBrightnessScale = std::clamp(voiceModel.stringBrightnessScale * (0.94f + profile.pluckFocus * 0.06f), 0.15f, 2.20f);
    voiceModel.pluckAttackLift = std::clamp(voiceModel.pluckAttackLift + (profile.pluckFocus - 1.0f) * 0.08f, -0.20f, 0.30f);
    voiceModel.attackScale = std::clamp(voiceModel.attackScale * profile.transientFocus, 0.35f, 4.80f);
    voiceModel.transientTimeScale = std::clamp(voiceModel.transientTimeScale * profile.transientFocus, 0.35f, 3.80f);
    voiceModel.transientLevel = std::clamp(voiceModel.transientLevel - (1.0f - profile.transientFocus) * 0.10f, 0.50f, 0.96f);
    voiceModel.bodyScale = std::clamp(voiceModel.bodyScale * profile.bodyFocus, 0.05f, 1.80f);
    voiceModel.bodyFeedbackScale = std::clamp(voiceModel.bodyFeedbackScale * profile.bodyFeedbackFocus, 0.20f, 1.40f);
    voiceModel.bodyBrightnessScale = std::clamp(voiceModel.bodyBrightnessScale * (0.90f + profile.bodyFocus * 0.10f), 0.30f, 1.30f);
    voiceModel.sympatheticGain = std::clamp(std::max(voiceModel.sympatheticGain, profile.sympatheticFocus), 0.0f, 0.24f);
    voiceModel.driveScale = std::clamp(voiceModel.driveScale * profile.driveFocus, 0.55f, 1.80f);
    voiceModel.driveBias = std::clamp(voiceModel.driveBias + (profile.driveFocus - 1.0f) * 0.18f, -0.10f, 0.45f);
    voiceModel.stereoWidthScale = std::clamp(voiceModel.stereoWidthScale * profile.stereoFocus, 0.35f, 1.80f);
    voiceModel.loopFilter2Bright = std::max(voiceModel.loopFilter2Bright, profile.loopFilterFocus);
    voiceModel.inharmonicityScale = std::clamp(voiceModel.inharmonicityScale * profile.inharmonicityFocus, 0.05f, 1.60f);

    if (profile.continuousFocus > 0.001f)
    {
        voiceModel.useContinuousExcitation = true;
        voiceModel.continuousExciteLevel = std::max(voiceModel.continuousExciteLevel, profile.continuousFocus);
    }
}

void GuitarVoiceBase::reset() noexcept
{
    stringBuf.fill(0.0f);
    string2Buf.fill(0.0f);
    bodyBuf.fill(0.0f);

    stringWritePos = 0;
    stringLength = 100.0f;
    stringFeedback = 0.99f;
    stringFilterState = 0.0f;
    stringBrightCoeff = 0.5f;
    stringPickPos = 0.15f;
    brightCoeffTarget = 0.5f;
    brightCoeffCurrent = 0.5f;
    brightDecayCoeff = 1.0f;

    apCoeff = 0.0f;
    apState = 0.0f;
    ap2State = 0.0f;
    stringFilterState2 = 0.0f;
    stringBrightCoeff2 = 1.0f;
    string2FilterState2 = 0.0f;

    string2WritePos = 0;
    string2Length = 100.0f;
    string2FilterState = 0.0f;
    string2Mix = 0.0f;

    exciteLevel = 0.0f;
    exciteDecayCoef = 0.999f;
    exciteBright = 0.5f;
    excitePrev = 0.0f;

    fingerNoiseLevel = 0.0f;
    fingerNoiseDecay = 1.0f;
    fingerNoiseFiltState = 0.0f;
    storedSR = 44100.0f;

    bodyWritePos = 0;
    bodyDelay = 0.0f;
    bodyFeedback = 0.0f;
    bodyDampState = 0.0f;
    bodyDamping = 0.0f;

    body2State1 = 0.0f; body2State2 = 0.0f;
    body2CoeffA = 0.0f; body2CoeffB = 0.0f; body2Gain = 0.0f;
    body3State1 = 0.0f; body3State2 = 0.0f;
    body3CoeffA = 0.0f; body3CoeffB = 0.0f; body3Gain = 0.0f;

    body4State1 = 0.0f; body4State2 = 0.0f;
    body4CoeffA = 0.0f; body4CoeffB = 0.0f; body4Gain = 0.0f;
    body5State1 = 0.0f; body5State2 = 0.0f;
    body5CoeffA = 0.0f; body5CoeffB = 0.0f; body5Gain = 0.0f;

    for (auto& sr : sympRes) { sr = {}; }
    sympGain = 0.0f;
    sympDecayCoeff = 0.9998f;

    svfBand = 0.0f;
    svfLow = 0.0f;
    filterF = 0.0f;
    filterQinv = 0.707f;
    filterFTarget = 0.0f;
    filterQinvTarget = 0.707f;

    driveAmount = 1.0f;
    driveNorm = 1.0f;
    driveAdaaPreviousInput = 0.0f;
    driveOsPreviousDry = 0.0f;

    envStage = Off;
    fadeOutRemaining = 0;
    envLevel = 0.0f;
    envAttackInc = 0.0f;
    envDecayMul = 1.0f;
    envSustain = 0.0f;
    envRelMul = 1.0f;
    envTransientMul = 1.0f;
    envTransientTarget = 0.7f;

    panL = 0.707f;
    panR = 0.707f;
    stereoWidth = 0.0f;

    velocity = 0.0f;
    levelGain = 1.0f;
    baseLevelGain = 1.0f;
    active = false;
    age = 0.0f;
    pitchBendFactor = 1.0f;
    modPitchFactor = 1.0f;
    smoothedPitchFactor = 1.0f;
    baseStringLength = 100.0f;
    baseString2Length = 100.0f;
    baseAttackSeconds = 0.003f;
    baseDecaySeconds = 1.0f;
    baseReleaseSeconds = 0.25f;
    basePan = 0.0f;
    baseBodyFeedback = 0.0f;
    baseBody2Gain = 0.0f;
    baseBody3Gain = 0.0f;
    baseBody4Gain = 0.0f;
    baseBody5Gain = 0.0f;
    quickReleaseForced = false;
    currentModulation = {};
    rngState = 12345u;

    portamentoActive = false;
    targetStringLength = 0.0f;
    targetString2Length = 0.0f;
    portamentoPitchFactor = 1.0f;
    targetPortamentoPitchFactor = 1.0f;
    portamentoPitchFactor2 = 1.0f;
    targetPortamentoPitchFactor2 = 1.0f;
    portamentoCoeff = 1.0f;
    storedTuneSemitones = 0.0f;
    palmMuteTarget = 0.0f;
    palmMuteCurrent = 0.0f;

    dcX1 = 0.0f;
    dcY1 = 0.0f;

    model = {};
}

float GuitarVoiceBase::nextRandom()
{
    rngState = rngState * 1664525u + 1013904223u;
    return static_cast<float>(rngState >> 8) / 16777216.0f;
}

float GuitarVoiceBase::readString(const std::array<float, 4096>& buf,
                                  int writePos, float delaySamples) const
{
    float readPos = static_cast<float>(writePos) - delaySamples;
    if (readPos < 0.0f)
        readPos += static_cast<float>(kMaxDelay);

    const int i1 = static_cast<int>(readPos) % kMaxDelay;
    const float frac = readPos - std::floor(readPos);
    const int i0 = (i1 - 1 + kMaxDelay) % kMaxDelay;
    const int i2 = (i1 + 1) % kMaxDelay;
    const int i3 = (i1 + 2) % kMaxDelay;

    const float y0 = buf[static_cast<std::size_t>(i0)];
    const float y1 = buf[static_cast<std::size_t>(i1)];
    const float y2 = buf[static_cast<std::size_t>(i2)];
    const float y3 = buf[static_cast<std::size_t>(i3)];

    // Hermite cubic interpolation
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

float GuitarVoiceBase::readBody(const float delaySamples) const
{
    float readPos = static_cast<float>(bodyWritePos) - delaySamples;
    if (readPos < 0.0f)
        readPos += static_cast<float>(kBodyBufSize);

    const int i1 = static_cast<int>(readPos) % kBodyBufSize;
    const float frac = readPos - std::floor(readPos);
    const int i0 = (i1 - 1 + kBodyBufSize) % kBodyBufSize;
    const int i2 = (i1 + 1) % kBodyBufSize;
    const int i3 = (i1 + 2) % kBodyBufSize;

    const float y0 = bodyBuf[static_cast<std::size_t>(i0)];
    const float y1 = bodyBuf[static_cast<std::size_t>(i1)];
    const float y2 = bodyBuf[static_cast<std::size_t>(i2)];
    const float y3 = bodyBuf[static_cast<std::size_t>(i3)];

    // Hermite cubic interpolation (matches string delay quality)
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

void GuitarVoiceBase::noteOn(const int midiNote, const float vel, const double sampleRate,
                             const InstrSettings& settings, const uint32_t chordHash)
{
    reset();
    active = true;
    velocity = vel;
    storedSR = static_cast<float>(sampleRate);
    storedTuneSemitones = settings.tuneSemitones;

    if (chordHash != 0)
        rngState = chordHash;
    else
        rngState = static_cast<uint32_t>(midiNote * 73 + static_cast<int>(vel * 1000.0f));

    const auto& chars = characteristics();
    model = buildVoiceModel(midiNote, vel, sampleRate, settings, chars);
    applyRuntimeModelProfile(model, chars);

    const float sr = static_cast<float>(sampleRate);
    const float tuned = static_cast<float>(midiNote) + settings.tuneSemitones;
    float fundHz = 440.0f * std::pow(2.0f, (tuned - 69.0f) / 12.0f);

    const float pitchFollow = model.pitchFollow >= 0.0f ? model.pitchFollow : chars.output.pitchFollow;
    fundHz *= pitchFollow;
    fundHz += (1.0f - pitchFollow) * model.pitchReferenceHz;
    fundHz = std::clamp(fundHz, 20.0f, sr * 0.45f);

    levelGain = settings.level * vel * model.outputTrim;
    baseLevelGain = levelGain;
    baseStringLength = std::clamp(sr / fundHz, 2.0f, static_cast<float>(kMaxDelay - 2));
    stringLength = baseStringLength;
    smoothedPitchFactor = std::max(0.25f, pitchBendFactor * modPitchFactor);
    targetStringLength = baseStringLength;
    portamentoPitchFactor = 1.0f;
    targetPortamentoPitchFactor = 1.0f;
    stringWritePos = static_cast<int>(stringLength);

    float bright = chars.string.brightness * settings.tone.stringBrightness * model.stringBrightnessScale + model.brightnessLift;
    bright = std::clamp(bright, 0.05f, 0.98f);
    stringBrightCoeff = bright;
    brightCoeffTarget = stringBrightCoeff;
    brightCoeffCurrent = std::min(0.99f, stringBrightCoeff + vel * vel * 0.12f);
    {
        const float brightDecayTime = 0.5f + vel * 0.8f;
        brightDecayCoeff = std::exp(-1.0f / (brightDecayTime * sr));
    }

    const float regDecayScale = 1.0f - static_cast<float>(midiNote - 60) / 130.0f;
    const float decaySamples = chars.string.decaySeconds * settings.envelope.decaySeconds * sr
                             * std::max(0.35f, regDecayScale) * model.decayScale;
    stringFeedback = std::pow(0.001f, stringLength / std::max(decaySamples, 1.0f));
    stringFeedback = std::clamp(stringFeedback, model.feedbackFloor, model.feedbackCeil);

    // Phase 1.1 — allpass inharmonicity coefficient
    apCoeff = std::clamp(chars.string.inharmonicity * model.inharmonicityScale * 40.0f, 0.0f, 0.15f);
    // Phase 1.1 — second loop filter (frequency-dependent HF decay)
    stringBrightCoeff2 = (model.loopFilter2Bright > 0.001f)
        ? std::clamp(model.loopFilter2Bright, 0.05f, 0.99f)
        : 1.0f;

    // Round-robin: select pluck position based on RR index, then apply user pickPosition offset
    const uint8_t posIdx = rrIndex % 3;
    const float basePos = kPluckPositions[posIdx];
    stringPickPos = std::clamp(
        basePos * (1.0f + (settings.performance.pickPosition - 0.5f) * 0.4f)
        + model.pluckPositionOffset,
        0.01f, 0.49f);
    // Apply RR-based brightness and velocity variations (visible to both branches below)
    const float rrBrightOffset = kRrBrightnessOffsets[posIdx];
    const float rrVelScale = kRrVelocityScales[posIdx];
    const float rrBrightMod = 1.0f + rrBrightOffset;
    const float velWithRr = vel * rrVelScale;
    // Advance RR counter for next noteOn
    if (++rrIndex >= rrCount) rrIndex = 0;

    // Strum direction from chordHash: bit 31 encodes direction (1=up, -1=down)
    strumDirection = ((chordHash >> 31) & 1) == 0 ? 1 : -1;

    {
        const float pluckRegNorm = std::clamp((static_cast<float>(midiNote) - 21.0f) / 67.0f, 0.0f, 1.0f);
        const float pluckBright = std::clamp(chars.excitation.brightness * settings.performance.attackBrightness * model.pluckBrightnessScale
                                             * (0.45f + pluckRegNorm * 0.55f) + model.pluckAttackLift,
                                             0.05f, 0.98f) * rrBrightMod;
        
        // Phase 2 — Rake pattern: simulates the temporal spread of a real guitar pluck
        // Rake direction inverts with strumDirection (up/down stroke) for natural variation
        const float rakeDirection = strumDirection * ((midiNote % 2 == 0) ? 1.0f : -1.0f);
        const float rakeAmountBase = 0.12f * std::clamp(settings.performance.attackBrightness, 0.3f, 1.0f);
        // Phase 2.1 — Stochastic variation: rakeAmount varies each note for natural feel
        const float rakeAmount = rakeAmountBase * (0.7f + 0.3f * nextRandom());
        
        float previous = 0.0f;
        const int exciteLength = std::max(2, static_cast<int>(stringLength));
        for (int i = 0; i < exciteLength; ++i)
        {
            const float raw = nextRandom() * 2.0f - 1.0f;
            // Phase 2.1 — Stochastic noise: small amplitude variation for organic feel
            const float noiseMod = 1.0f + 0.08f * (nextRandom() - 0.5f);
            
            // Phase 2 — Apply rake envelope: early samples have more amplitude
            const float timeNorm = static_cast<float>(i) / static_cast<float>(exciteLength);
            const float rakeEnvelope = std::exp(-timeNorm * (3.0f + rakeDirection * 1.5f));
            const float rawWithRake = raw * noiseMod * (1.0f + rakeAmount * rakeEnvelope);
            
            float filtered = previous + pluckBright * (rawWithRake - previous);
            previous = filtered;

            const float combDelay = stringPickPos * stringLength;
            if (combDelay > 1.0f && i >= static_cast<int>(combDelay))
            {
                const int past = i - static_cast<int>(combDelay);
                filtered -= stringBuf[static_cast<std::size_t>(past)] * 0.5f;
            }

            stringBuf[static_cast<std::size_t>(i)] = filtered * velWithRr;
        }
    }

    if (model.enableSecondaryString)
    {
        const float secondaryRatio = std::max(0.25f, model.secondaryPitchRatio);
        const float detune = 1.0f + chars.string.inharmonicity * model.secondaryDetuneScale;
        baseString2Length = std::clamp(sr / (fundHz * secondaryRatio * detune), 2.0f, static_cast<float>(kMaxDelay - 2));
        string2Length = baseString2Length;
        targetString2Length = baseString2Length;
        portamentoPitchFactor2 = 1.0f;
        targetPortamentoPitchFactor2 = 1.0f;
        string2WritePos = static_cast<int>(string2Length);
        string2Mix = model.secondaryMix;

        float previous = 0.0f;
        const int exciteLength = std::max(2, static_cast<int>(string2Length));
        const float string2Bright = std::clamp(chars.excitation.brightness * settings.performance.attackBrightness * model.pluckBrightnessScale,
                                               0.05f, 0.98f) * rrBrightMod;
        for (int i = 0; i < exciteLength; ++i)
        {
            const float raw = nextRandom() * 2.0f - 1.0f;
            const float filtered = previous + string2Bright * (raw - previous);
            previous = filtered;
            string2Buf[static_cast<std::size_t>(i)] = filtered * velWithRr;
        }
    }

    if (model.useContinuousExcitation || chars.synthMode == SynthMode::Pad || chars.synthMode == SynthMode::Hybrid)
    {
        exciteLevel = model.continuousExciteLevel * vel;
    }

    const float exciteDecaySeconds = std::max(chars.excitation.decaySeconds * model.exciteDecayScale, 0.001f);
    exciteDecayCoef = std::exp(-1.0f / (exciteDecaySeconds * sr));
    exciteBright = std::clamp(chars.excitation.brightness * settings.performance.attackBrightness * model.exciteBrightnessScale, 0.05f, 0.99f);

    const float bodyAmount = settings.resonance.bodyAmount * model.bodyScale;
    if (chars.body.resonance > 0.001f && bodyAmount > 0.01f)
    {
        float bodyHz = fundHz * chars.body.sizeRatio;
        bodyHz = std::clamp(bodyHz, 30.0f, sr * 0.4f);
        bodyDelay = std::min(sr / bodyHz, static_cast<float>(kBodyBufSize - 2));
        bodyFeedback = chars.body.resonance * bodyAmount * 0.92f * model.bodyFeedbackScale;
        // Frequency-dependent damping: higher body resonance damps HF more (wood absorption)
        const float freqDampScale = std::clamp(bodyHz / 400.0f, 0.5f, 1.5f);
        bodyDamping = std::clamp((1.0f - chars.body.brightness * 0.5f * model.bodyBrightnessScale) * freqDampScale,
                                 0.05f, 0.95f);
    }

    updateFilter(settings.tone.lowPassHz, settings.tone.stringBrightness, sr);

    const float drive = std::clamp(chars.output.drive + settings.resonance.driveAmount * model.driveScale + model.driveBias, 0.0f, 1.0f);
    driveAmount = 1.0f + drive * 8.0f;
    driveNorm = 1.0f / std::max(0.001f, std::tanh(driveAmount));

    baseAttackSeconds = settings.envelope.attackSeconds;
    baseDecaySeconds = settings.envelope.decaySeconds;
    baseReleaseSeconds = settings.envelope.releaseSeconds;
    quickReleaseForced = false;
    currentModulation = {};
    envStage = Attack;
    envLevel = 0.0f;
    envSustain = settings.envelope.sustainLevel;
    updateEnvelopeRatesFromModulation();

    // Phase 1.2 — transient stage (rapid decay from 1.0 to transientTarget)
    {
        const float transientTime = std::max(0.01f, 0.035f * model.transientTimeScale);
        envTransientMul = std::exp(-1.0f / (transientTime * sr));
        envTransientTarget = std::clamp(model.transientLevel, 0.1f, 0.99f);
    }

    if (model.enableSecondaryBodyModes && chars.body.resonance > 0.01f && bodyAmount > 0.01f)
    {
        constexpr float twoPi = 6.283185307f;
        const float bodyHz = fundHz * chars.body.sizeRatio;

        const float f2 = bodyHz * 0.6f;
        if (f2 > 20.0f && f2 < sr * 0.45f)
        {
            const float w2 = twoPi * f2 / sr;
            const float r2 = std::exp(-w2 / std::max(0.5f, chars.body.resonance * bodyAmount * 4.0f));
            body2CoeffA = 2.0f * r2 * std::cos(w2);
            body2CoeffB = -(r2 * r2);
            body2Gain = bodyAmount * 0.10f * (1.0f - r2) * 2.0f * std::sin(w2);
        }

        const float f3 = bodyHz * 2.2f;
        if (f3 > 20.0f && f3 < sr * 0.45f)
        {
            const float w3 = twoPi * f3 / sr;
            const float r3 = std::exp(-w3 / std::max(0.5f, chars.body.resonance * bodyAmount * 3.0f));
            body3CoeffA = 2.0f * r3 * std::cos(w3);
            body3CoeffB = -(r3 * r3);
            body3Gain = bodyAmount * 0.05f * (1.0f - r3) * 2.0f * std::sin(w3);
        }

        // Phase 1.4 — additional body modes
        if (model.body4FreqRatio > 0.01f)
        {
            const float f4 = bodyHz * model.body4FreqRatio;
            if (f4 > 20.0f && f4 < sr * 0.45f)
            {
                const float w4 = twoPi * f4 / sr;
                const float r4 = std::exp(-w4 / std::max(0.5f, chars.body.resonance * bodyAmount * 3.5f));
                body4CoeffA = 2.0f * r4 * std::cos(w4);
                body4CoeffB = -(r4 * r4);
                body4Gain = bodyAmount * 0.04f * (1.0f - r4) * 2.0f * std::sin(w4);
            }
        }
        if (model.body5FreqRatio > 0.01f)
        {
            const float f5 = bodyHz * model.body5FreqRatio;
            if (f5 > 20.0f && f5 < sr * 0.45f)
            {
                const float w5 = twoPi * f5 / sr;
                const float r5 = std::exp(-w5 / std::max(0.5f, chars.body.resonance * bodyAmount * 3.0f));
                body5CoeffA = 2.0f * r5 * std::cos(w5);
                body5CoeffB = -(r5 * r5);
                body5Gain = bodyAmount * 0.03f * (1.0f - r5) * 2.0f * std::sin(w5);
            }
        }
    }

    // Phase 1.3 — sympathetic string resonance (acoustic family)
    if (model.sympatheticGain > 0.001f)
    {
        sympGain = model.sympatheticGain;
        const float sympDecaySeconds = std::clamp(baseDecaySeconds * 1.5f, 0.25f, 12.0f);
        sympDecayCoeff = std::exp(std::log(0.001f) / std::max(1.0f, sympDecaySeconds * sr));
        constexpr float sympFreqs[kSympStrings] = { 82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f };
        constexpr float twoPiS = 6.283185307f;

        // Compute played note frequency for harmonic proximity weighting
        const float playedHz = 440.0f * std::pow(2.0f, (static_cast<float>(midiNote) - 69.0f) / 12.0f);
        for (int i = 0; i < kSympStrings; ++i)
        {
            const float w = twoPiS * sympFreqs[i] / sr;
            if (w > 0.0f && sympFreqs[i] < sr * 0.45f)
            {
                const float r = std::exp(-w * 0.15f);
                sympRes[static_cast<std::size_t>(i)].a = 2.0f * r * std::cos(w);
                sympRes[static_cast<std::size_t>(i)].b = -(r * r);
                // Harmonic proximity: boost resonators whose freq is a near-integer ratio of played note
                const float ratio = playedHz / sympFreqs[i];
                const float nearestHarmonic = std::round(ratio);
                const float harmonicDist = std::abs(ratio - nearestHarmonic);
                const float proximityBoost = 1.0f + 2.0f * std::exp(-harmonicDist * 20.0f);
                sympRes[static_cast<std::size_t>(i)].gain = sympGain * proximityBoost * (1.0f - r) * 2.0f * std::sin(w);
            }
        }
    }

    stereoWidth = clamp01(settings.spatial.stereoWidth * model.stereoWidthScale + chars.output.stereoWidth * 0.20f);
    const float registerPan = (static_cast<float>(midiNote - 64) / 87.0f) * 0.20f * model.registerPanScale;
    basePan = std::clamp(settings.spatial.pan + registerPan, -1.0f, 1.0f);
    baseBodyFeedback = bodyFeedback;
    baseBody2Gain = body2Gain;
    baseBody3Gain = body3Gain;
    baseBody4Gain = body4Gain;
    baseBody5Gain = body5Gain;
    updateStereoPanFromModulation();
}

void GuitarVoiceBase::setTargetNote(int midiNote, double sampleRate)
{
    const float sr = static_cast<float>(sampleRate);
    const auto& chars = characteristics();

    const float tuned = static_cast<float>(midiNote) + storedTuneSemitones;
    float fundHz = 440.0f * std::pow(2.0f, (tuned - 69.0f) / 12.0f);

    const float pitchFollow = model.pitchFollow >= 0.0f ? model.pitchFollow : chars.output.pitchFollow;
    fundHz *= pitchFollow;
    fundHz += (1.0f - pitchFollow) * model.pitchReferenceHz;
    fundHz = std::clamp(fundHz, 20.0f, sr * 0.45f);

    targetStringLength = std::clamp(sr / fundHz, 2.0f, static_cast<float>(kMaxDelay - 2));
    targetPortamentoPitchFactor = std::clamp(baseStringLength / targetStringLength, 0.25f, 4.0f);

    if (model.enableSecondaryString && string2Mix > 0.001f)
    {
        const float secondaryRatio = std::max(0.25f, model.secondaryPitchRatio);
        const float detune = 1.0f + chars.string.inharmonicity * model.secondaryDetuneScale;
        targetString2Length = std::clamp(sr / (fundHz * secondaryRatio * detune), 2.0f, static_cast<float>(kMaxDelay - 2));
        targetPortamentoPitchFactor2 = std::clamp(baseString2Length / targetString2Length, 0.25f, 4.0f);
    }
    else
    {
        targetPortamentoPitchFactor2 = targetPortamentoPitchFactor;
    }

    portamentoCoeff = std::exp(-1.0f / (0.05f * sr));
    portamentoActive = true;
}

void GuitarVoiceBase::noteOff()
{
    if (envStage == Off)
        return;

    quickReleaseForced = false;
    envStage = Release;
    if (model.enableFingerNoise)
    {
        fingerNoiseLevel = envLevel * velocity * model.fingerNoiseAmount;
        fingerNoiseDecay = std::exp(-1.0f / (model.fingerNoiseDecaySeconds * std::max(1.0f, storedSR)));
        fingerNoiseFiltState = 0.0f;
    }
}

void GuitarVoiceBase::updateFilter(float lowPassHz, float stringBrightness, double sr) noexcept
{
    const float srF = static_cast<float>(sr);
    float cutNorm = std::clamp((lowPassHz * model.cutoffScale) / srF, 20.0f / srF, 0.45f);
    float nextFilterF = 2.0f * std::sin(3.14159265f * cutNorm);
    const float baseQinv = 1.0f / std::max(0.5f, 0.5f + (1.0f - stringBrightness) * 1.0f);
    const float resonanceScale = std::exp2(-currentModulation.resonanceAdd * 0.75f);
    float nextFilterQinv = std::clamp(baseQinv * resonanceScale, 0.15f, 2.0f);
    const float maxF = -nextFilterQinv + std::sqrt(nextFilterQinv * nextFilterQinv + 4.0f);
    nextFilterF = std::min(nextFilterF, maxF * 0.95f);

    filterFTarget = nextFilterF;
    filterQinvTarget = nextFilterQinv;
    if (!active || envStage == Off)
    {
        filterF = filterFTarget;
        filterQinv = filterQinvTarget;
    }
}

void GuitarVoiceBase::forceQuickRelease() noexcept
{
    if (envStage == Off)
        return;
    envStage = Release;
    quickReleaseForced = true;
    const int releaseSamples = std::max(1, static_cast<int>(std::round(std::max(1.0f, storedSR) * 0.005f)));
    envRelMul = std::exp(std::log(0.001f) / static_cast<float>(releaseSamples));
}

void GuitarVoiceBase::setVoiceModulation(const VoiceModulation& modulation, double sampleRate) noexcept
{
    (void) sampleRate;
    currentModulation = modulation;
    modPitchFactor = std::exp2(modulation.pitchSemi / 12.0f);
    levelGain = baseLevelGain * std::clamp(modulation.levelMul, 0.0f, 4.0f);
    const float resonanceScale = std::clamp(1.0f + modulation.resonanceAdd * 0.4f, 0.2f, 1.8f);
    bodyFeedback = baseBodyFeedback * resonanceScale;
    body2Gain = baseBody2Gain * resonanceScale;
    body3Gain = baseBody3Gain * resonanceScale;
    body4Gain = baseBody4Gain * resonanceScale;
    body5Gain = baseBody5Gain * resonanceScale;
    updateStereoPanFromModulation();
    updateEnvelopeRatesFromModulation();
}

void GuitarVoiceBase::updateEnvelopeRatesFromModulation() noexcept
{
    const float sr = std::max(1.0f, storedSR);
    const float attackSeconds = std::max(baseAttackSeconds * model.attackScale * std::clamp(currentModulation.attackScale, 0.0625f, 16.0f), 0.0005f);
    const float decaySeconds = std::max(baseDecaySeconds * model.decayScale * std::clamp(currentModulation.decayScale, 0.0625f, 16.0f), 0.01f);
    envAttackInc = 1.0f / (attackSeconds * sr);
    envDecayMul = std::exp(-1.0f / (decaySeconds * sr));
    if (!quickReleaseForced)
    {
        const float releaseSeconds = std::max(baseReleaseSeconds * model.releaseScale, 0.005f);
        envRelMul = std::exp(-1.0f / (releaseSeconds * sr));
    }
}

void GuitarVoiceBase::updateStereoPanFromModulation() noexcept
{
    const float totalPan = std::clamp(basePan + currentModulation.panAdd, -1.0f, 1.0f);
    const float panAngle = (totalPan * 0.5f + 0.5f) * 1.5707963f;
    panL = std::cos(panAngle);
    panR = std::sin(panAngle);
}

void GuitarVoiceBase::renderBlock(float* outL, float* outR, const int numSamples, const double sampleRate)
{
    if (!active)
    {
        std::memset(outL, 0, static_cast<std::size_t>(numSamples) * sizeof(float));
        std::memset(outR, 0, static_cast<std::size_t>(numSamples) * sizeof(float));
        return;
    }

    const float sr = static_cast<float>(sampleRate);

    for (int s = 0; s < numSamples; ++s)
    {
    // Portamento (legato glide)
    if (portamentoActive)
    {
        portamentoPitchFactor = targetPortamentoPitchFactor
            + (portamentoPitchFactor - targetPortamentoPitchFactor) * portamentoCoeff;
        if (string2Mix > 0.001f)
        {
            portamentoPitchFactor2 = targetPortamentoPitchFactor2
                + (portamentoPitchFactor2 - targetPortamentoPitchFactor2) * portamentoCoeff;
        }

        if (std::abs(portamentoPitchFactor - targetPortamentoPitchFactor) < 0.0005f)
        {
            baseStringLength = targetStringLength;
            if (string2Mix > 0.001f)
                baseString2Length = targetString2Length;
            portamentoPitchFactor = 1.0f;
            targetPortamentoPitchFactor = 1.0f;
            portamentoPitchFactor2 = 1.0f;
            targetPortamentoPitchFactor2 = 1.0f;
            portamentoActive = false;
        }
    }

    const float targetPitchFactor = std::max(0.25f, pitchBendFactor * modPitchFactor);
    const float pitchSmoothCoeff = std::exp(-1.0f / std::max(1.0f, sr * 0.008f));
    smoothedPitchFactor = targetPitchFactor + (smoothedPitchFactor - targetPitchFactor) * pitchSmoothCoeff;
    const float combinedPitchFactor = smoothedPitchFactor;
    if (portamentoActive || std::abs(combinedPitchFactor - 1.0f) > 0.0001f)
    {
        const float glidePitchFactor = portamentoActive ? portamentoPitchFactor : 1.0f;
        stringLength = std::clamp(baseStringLength / (combinedPitchFactor * glidePitchFactor),
                                  2.0f, static_cast<float>(kMaxDelay - 2));
        if (string2Mix > 0.001f)
        {
            const float glidePitchFactor2 = portamentoActive ? portamentoPitchFactor2 : 1.0f;
            string2Length = std::clamp(baseString2Length / (combinedPitchFactor * glidePitchFactor2),
                                       2.0f, static_cast<float>(kMaxDelay - 2));
        }
    }

    switch (envStage)
    {
        case Attack:
            envLevel += envAttackInc;
            if (envLevel >= 1.0f) { envLevel = 1.0f; envStage = Transient; }
            break;
        case Transient:
            envLevel = envTransientTarget + (envLevel - envTransientTarget) * envTransientMul;
            if (envLevel <= envTransientTarget + 0.001f) { envLevel = envTransientTarget; envStage = Decay; }
            break;
        case Decay:
            envLevel = envSustain + (envLevel - envSustain) * envDecayMul;
            if (envLevel <= envSustain + 0.0001f) envStage = Sustain;
            break;
        case Sustain:
            break;
        case Release:
            envLevel *= envRelMul;
            if (envLevel < 0.0001f)
            {
                envStage = Off;
                fadeOutRemaining = std::max(1, static_cast<int>(
                    std::ceil(std::max(0.005f, baseReleaseSeconds * 0.5f) * std::max(1.0f, storedSR))));
                fadeOutCoeff = std::exp(std::log(0.001f) / static_cast<float>(fadeOutRemaining));
            }
            break;
        case Off:
            if (fadeOutRemaining <= 0)
            {
                active = false;
                outL[s] = 0.0f;
                outR[s] = 0.0f;
                if (s + 1 < numSamples)
                {
                    std::memset(outL + s + 1, 0, static_cast<std::size_t>(numSamples - s - 1) * sizeof(float));
                    std::memset(outR + s + 1, 0, static_cast<std::size_t>(numSamples - s - 1) * sizeof(float));
                }
                return;
            }
            // Apply smooth fade-out instead of hard silence
            envLevel *= fadeOutCoeff;
            --fadeOutRemaining;
            break;
    }

    float exciteSig = 0.0f;
    if (model.continuousExciteSustain > 0.0001f && envStage != Release && envStage != Off)
    {
        const float sustainExcite = model.continuousExciteSustain * velocity * (0.35f + envLevel * 0.65f);
        exciteLevel = std::max(exciteLevel, sustainExcite);
    }

    if (exciteLevel > 0.0001f)
    {
        const float raw = nextRandom() * 2.0f - 1.0f;
        const float filtered = excitePrev + exciteBright * (raw - excitePrev);
        excitePrev = filtered;
        exciteSig = filtered * exciteLevel;
        exciteLevel *= exciteDecayCoef;
    }

    float fingerNoiseSig = 0.0f;
    if (fingerNoiseLevel > 0.0001f)
    {
        const float rawNoise = nextRandom() * 2.0f - 1.0f;
        fingerNoiseFiltState += 0.4f * (rawNoise - fingerNoiseFiltState);
        fingerNoiseSig = (rawNoise - fingerNoiseFiltState) * fingerNoiseLevel;
        fingerNoiseLevel *= fingerNoiseDecay;
    }

    // Palm mute smoothing & derived scale factors
    palmMuteCurrent += (90.0f / sr) * (palmMuteTarget - palmMuteCurrent);
    const float pmFeedbackScale = 1.0f - palmMuteCurrent * 0.12f;
    const float pmBrightScale = 1.0f - palmMuteCurrent * 0.45f;

    const float strOut = readString(stringBuf, stringWritePos, stringLength);
    brightCoeffCurrent = brightCoeffTarget + (brightCoeffCurrent - brightCoeffTarget) * brightDecayCoeff;
    stringFilterState += (brightCoeffCurrent * pmBrightScale) * (strOut - stringFilterState);

    // Phase 1.1 — second loop filter stage (frequency-dependent HF decay)
    float loopSig = stringFilterState;
    if (stringBrightCoeff2 < 0.999f)
    {
        stringFilterState2 += stringBrightCoeff2 * (loopSig - stringFilterState2);
        loopSig = stringFilterState2;
    }

    // Phase 1.1 — allpass inharmonicity
    if (apCoeff > 0.0001f)
    {
        const float d = loopSig - apCoeff * apState;
        loopSig = apCoeff * d + apState;
        apState = d;
    }

    stringBuf[static_cast<std::size_t>(stringWritePos)] = loopSig * (stringFeedback * pmFeedbackScale) + exciteSig;
    stringWritePos = (stringWritePos + 1) % kMaxDelay;

    float signal = strOut + fingerNoiseSig;
    if (string2Mix > 0.001f)
    {
        const float str2Out = readString(string2Buf, string2WritePos, string2Length);
        string2FilterState += (stringBrightCoeff * pmBrightScale) * (str2Out - string2FilterState);

        float loop2 = string2FilterState;
        if (stringBrightCoeff2 < 0.999f)
        {
            string2FilterState2 += stringBrightCoeff2 * (loop2 - string2FilterState2);
            loop2 = string2FilterState2;
        }
        if (apCoeff > 0.0001f)
        {
            const float d2 = loop2 - apCoeff * ap2State;
            loop2 = apCoeff * d2 + ap2State;
            ap2State = d2;
        }

        string2Buf[static_cast<std::size_t>(string2WritePos)] = loop2 * (stringFeedback * pmFeedbackScale) + exciteSig * model.secondaryExciteScale;
        string2WritePos = (string2WritePos + 1) % kMaxDelay;

        signal = signal * (1.0f - string2Mix * 0.5f) + str2Out * string2Mix * 0.5f;
    }

    if (driveAmount > 1.05f)
    {
        const float dryDriveInput = signal;
        const float x = dryDriveInput * driveAmount;
        if (driveAmount > 4.0f)
        {
            const float midDry = 0.5f * (driveOsPreviousDry + dryDriveInput);
            const float midX = midDry * driveAmount;
            const float osA = adaaTanh(midX, driveAdaaPreviousInput);
            const float osB = adaaTanh(x, midX);
            signal = 0.5f * (osA + osB) * driveNorm;
        }
        else
        {
            signal = adaaTanh(x, driveAdaaPreviousInput) * driveNorm;
        }
        driveAdaaPreviousInput = x;
        driveOsPreviousDry = dryDriveInput;
    }
    else
    {
        driveAdaaPreviousInput = signal * driveAmount;
        driveOsPreviousDry = signal;
    }

    if (bodyFeedback > 0.001f && bodyDelay > 1.0f)
    {
        const float delayed = readBody(bodyDelay);
        bodyDampState += bodyDamping * (delayed - bodyDampState);
        const float bodyOut = bodyDampState * bodyFeedback;
        bodyBuf[static_cast<std::size_t>(bodyWritePos)] = signal + bodyOut;
        bodyWritePos = (bodyWritePos + 1) % kBodyBufSize;
        signal += bodyOut * 0.5f;
    }

    if (body2Gain > 0.00005f)
    {
        const float body2Out = signal + body2CoeffA * body2State1 + body2CoeffB * body2State2;
        body2State2 = body2State1;
        body2State1 = body2Out;
        signal += body2Out * body2Gain;
    }
    if (body3Gain > 0.00005f)
    {
        const float body3Out = signal + body3CoeffA * body3State1 + body3CoeffB * body3State2;
        body3State2 = body3State1;
        body3State1 = body3Out;
        signal += body3Out * body3Gain;
    }

    // Phase 1.4 — additional body modes
    if (body4Gain > 0.00005f)
    {
        const float body4Out = signal + body4CoeffA * body4State1 + body4CoeffB * body4State2;
        body4State2 = body4State1;
        body4State1 = body4Out;
        signal += body4Out * body4Gain;
    }
    if (body5Gain > 0.00005f)
    {
        const float body5Out = signal + body5CoeffA * body5State1 + body5CoeffB * body5State2;
        body5State2 = body5State1;
        body5State1 = body5Out;
        signal += body5Out * body5Gain;
    }

    // Phase 1.3 — sympathetic string resonance
    if (sympGain > 0.0001f)
    {
        float sympOut = 0.0f;
        for (auto& res : sympRes)
        {
            if (res.gain > 0.00001f)
            {
                const float out = signal + res.a * res.s1 + res.b * res.s2;
                res.s2 = res.s1 * sympDecayCoeff;
                res.s1 = out * sympDecayCoeff;
                sympOut += out * res.gain;
            }
        }
        signal += sympOut;
    }

    // DC blocker (R ~= 0.995 — removes sub-20 Hz drift from body resonance & drive)
    {
        constexpr float dcR = 0.995f;
        const float dcY = signal - dcX1 + dcR * dcY1;
        dcX1 = signal;
        dcY1 = dcY;
        signal = dcY;
    }

    {
        const float filterSmoothCoeff = std::exp(-1.0f / std::max(1.0f, sr * 0.004f));
        filterF = filterFTarget + (filterF - filterFTarget) * filterSmoothCoeff;
        filterQinv = filterQinvTarget + (filterQinv - filterQinvTarget) * filterSmoothCoeff;
    }

    {
        const float hp = signal - svfLow - filterQinv * svfBand;
        svfBand += filterF * hp;
        svfLow += filterF * svfBand;
        // Denormal protection for filter states
        if (std::abs(svfBand) < kDenormalThreshold) svfBand = 0.0f;
        if (std::abs(svfLow ) < kDenormalThreshold) svfLow  = 0.0f;
        signal = svfLow;
    }

    const float stereoOffset = stereoWidth * 0.3f;
    float sigL = signal * (1.0f + stereoOffset);
    float sigR = signal * (1.0f - stereoOffset);
    if (string2Mix > kVerySmallSignalThreshold)
    {
        const float str2 = readString(string2Buf, string2WritePos, string2Length);
        sigL += str2 * string2Mix * stereoWidth * 0.15f;
        sigR -= str2 * string2Mix * stereoWidth * 0.15f;
    }

    const float gain = envLevel * levelGain;
    outL[s] = sigL * gain * panL;
    outR[s] = sigR * gain * panR;

    age += 1.0f / sr;
    constexpr float kMaxMusicalVoiceAgeSeconds = 600.0f;
    if (age > kMaxMusicalVoiceAgeSeconds && envStage != Off)
    {
        envStage = Off;
        fadeOutRemaining = std::max(1, static_cast<int>(std::round(sr * 0.005f)));
        fadeOutCoeff = std::exp(std::log(0.001f) / static_cast<float>(fadeOutRemaining));
    }

    } // end of per-sample loop
}

FolkSteelVoice::FolkSteelVoice() : GuitarVoiceBase(0) {}
TwelveStringVoice::TwelveStringVoice() : GuitarVoiceBase(1) {}
FlamencaVoice::FlamencaVoice() : GuitarVoiceBase(2) {}
CleanVoice::CleanVoice() : GuitarVoiceBase(3) {}
CrunchVoice::CrunchVoice() : GuitarVoiceBase(4) {}
LeadVoice::LeadVoice() : GuitarVoiceBase(5) {}
SynthGuitarVoice::SynthGuitarVoice() : GuitarVoiceBase(6) {}
EPadVoice::EPadVoice() : GuitarVoiceBase(7) {}
AmbientVoice::AmbientVoice() : GuitarVoiceBase(8) {}

GuitarVoiceBase::VoiceModel FolkSteelVoice::buildVoiceModel(int, float, double,
                                                            const InstrSettings&, const InstrCharacteristics&) const
{
    VoiceModel vm;
    vm.enableFingerNoise = true;
    vm.fingerNoiseAmount = 0.0065f;
    vm.stringBrightnessScale = 1.28f;
    vm.pluckBrightnessScale = 1.25f;
    vm.outputTrim = 1.70f;
    vm.attackScale = 2.0f;         // ~10ms — realistic steel string pluck
    vm.bodyScale = 1.15f;
    vm.bodyFeedbackScale = 1.05f;
    vm.stereoWidthScale = 1.0f;
    // Phase 1.1
    vm.inharmonicityScale = 1.0f;
    vm.loopFilter2Bright = 0.85f;
    // Phase 1.2
    vm.transientTimeScale = 1.0f;
    vm.transientLevel = 0.72f;
    // Phase 1.3
    vm.sympatheticGain = 0.08f;
    // Phase 1.4
    vm.body4FreqRatio = 1.6f;
    vm.body5FreqRatio = 3.4f;
    return vm;
}

GuitarVoiceBase::VoiceModel TwelveStringVoice::buildVoiceModel(int midiNote, float, double,
                                                               const InstrSettings&, const InstrCharacteristics&) const
{
    VoiceModel vm;
    const bool octaveCourse = midiNote <= 59;
    vm.enableFingerNoise = true;
    vm.fingerNoiseAmount = 0.0050f;
    vm.stringBrightnessScale = 1.34f;
    vm.pluckBrightnessScale = 1.30f;
    vm.outputTrim = 1.82f;
    vm.enableSecondaryString = true;
    vm.secondaryMix = octaveCourse ? 0.74f : 0.82f;
    vm.secondaryPitchRatio = octaveCourse ? 2.0f : 1.0f;
    vm.secondaryDetuneScale = octaveCourse ? 1.1f : 3.0f;
    vm.secondaryExciteScale = octaveCourse ? 0.52f : 0.68f;
    vm.attackScale = 2.0f;         // ~10ms — realistic steel string pluck
    vm.stereoWidthScale = 1.20f;
    vm.decayScale = 0.92f;
    // Phase 1.1
    vm.inharmonicityScale = 1.1f;
    vm.loopFilter2Bright = 0.82f;
    // Phase 1.2
    vm.transientTimeScale = 0.9f;
    vm.transientLevel = 0.68f;
    // Phase 1.3
    vm.sympatheticGain = 0.12f;
    // Phase 1.4
    vm.body4FreqRatio = 1.5f;
    vm.body5FreqRatio = 3.2f;
    return vm;
}

GuitarVoiceBase::VoiceModel FlamencaVoice::buildVoiceModel(int, float, double,
                                                           const InstrSettings&, const InstrCharacteristics&) const
{
    VoiceModel vm;
    vm.enableFingerNoise = true;
    vm.fingerNoiseAmount = 0.0035f;
    vm.stringBrightnessScale = 1.35f;
    vm.pluckBrightnessScale = 1.55f;
    vm.pluckAttackLift = 0.10f;
    vm.outputTrim = 1.78f;
    vm.attackScale = 1.0f;             // ~5ms — percussive but realistic nylon
    vm.decayScale = 0.70f;
    vm.releaseScale = 0.70f;
    vm.bodyScale = 0.90f;
    // Phase 1.1
    vm.inharmonicityScale = 0.7f;
    vm.loopFilter2Bright = 0.88f;
    // Phase 1.2
    vm.transientTimeScale = 0.7f;
    vm.transientLevel = 0.60f;
    // Phase 1.3
    vm.sympatheticGain = 0.06f;
    // Phase 1.4
    vm.body4FreqRatio = 1.8f;
    vm.body5FreqRatio = 3.6f;
    return vm;
}

GuitarVoiceBase::VoiceModel CleanVoice::buildVoiceModel(int, float, double,
                                                        const InstrSettings&, const InstrCharacteristics&) const
{
    VoiceModel vm;
    vm.bodyScale = 0.35f;
    vm.stringBrightnessScale = 1.28f;
    vm.pluckBrightnessScale = 1.25f;
    vm.outputTrim = 1.72f;
    vm.bodyFeedbackScale = 0.80f;
    vm.releaseScale = 0.90f;
    vm.stereoWidthScale = 0.95f;
    vm.secondaryPitchRatio = 1.0f;   // explicit: no harmonic secondary string
    // Phase 1.1
    vm.inharmonicityScale = 0.8f;
    // Phase 1.2
    vm.transientTimeScale = 1.0f;
    vm.transientLevel = 0.78f;
    // Phase 1.3 — minimal sympathetic for electric
    vm.sympatheticGain = 0.02f;
    return vm;
}

GuitarVoiceBase::VoiceModel CrunchVoice::buildVoiceModel(int, float, double,
                                                         const InstrSettings&, const InstrCharacteristics&) const
{
    VoiceModel vm;
    vm.bodyScale = 0.22f;
    vm.driveBias = 0.14f;
    vm.driveScale = 1.20f;
    vm.cutoffScale = 0.92f;
    vm.decayScale = 0.95f;
    vm.secondaryPitchRatio = 1.0f;   // explicit: no harmonic secondary string
    // Phase 1.1
    vm.inharmonicityScale = 0.9f;
    // Phase 1.2
    vm.transientTimeScale = 1.0f;
    vm.transientLevel = 0.75f;
    // Phase 1.3
    vm.sympatheticGain = 0.01f;
    return vm;
}

GuitarVoiceBase::VoiceModel LeadVoice::buildVoiceModel(int, float, double,
                                                       const InstrSettings&, const InstrCharacteristics&) const
{
    VoiceModel vm;
    vm.bodyScale = 0.10f;
    vm.driveBias = 0.24f;
    vm.driveScale = 1.35f;
    vm.decayScale = 1.18f;
    vm.releaseScale = 1.45f;
    vm.feedbackFloor = 0.93f;
    vm.cutoffScale = 1.05f;
    vm.secondaryPitchRatio = 1.0f;   // explicit: no harmonic secondary string
    // Phase 1.1
    vm.inharmonicityScale = 1.0f;
    // Phase 1.2
    vm.transientTimeScale = 1.2f;
    vm.transientLevel = 0.80f;
    return vm;
}

GuitarVoiceBase::VoiceModel SynthGuitarVoice::buildVoiceModel(int, float, double,
                                                              const InstrSettings&, const InstrCharacteristics&) const
{
    VoiceModel vm;
    vm.outputTrim = 1.15f;
    vm.useContinuousExcitation = true;
    vm.continuousExciteLevel = 0.30f;
    vm.continuousExciteSustain = 0.018f;
    vm.exciteDecayScale = 1.80f;
    vm.exciteBrightnessScale = 0.85f;
    vm.bodyScale = 0.50f;
    vm.bodyFeedbackScale = 0.68f;
    vm.bodyBrightnessScale = 0.78f;
    vm.enableSecondaryBodyModes = false;
    vm.driveBias = 0.10f;
    vm.driveScale = 1.10f;
    vm.attackScale = 1.20f;
    vm.releaseScale = 1.25f;
    vm.cutoffScale = 0.92f;
    vm.stereoWidthScale = 1.15f;
    vm.secondaryPitchRatio = 1.0f;   // explicit: unison second string
    vm.secondaryDetuneScale = 3.0f;
    // Phase 1.1
    vm.inharmonicityScale = 0.5f;
    // Phase 1.2
    vm.transientTimeScale = 1.15f;
    vm.transientLevel = 0.85f;
    return vm;
}

GuitarVoiceBase::VoiceModel EPadVoice::buildVoiceModel(int, float, double,
                                                       const InstrSettings&, const InstrCharacteristics&) const
{
    VoiceModel vm;
    vm.outputTrim = 2.20f;
    vm.enableSecondaryString = true;
    vm.secondaryMix = 0.55f;
    vm.secondaryPitchRatio = 1.0f;
    vm.secondaryDetuneScale = 2.5f;
    vm.useContinuousExcitation = true;
    vm.continuousExciteLevel = 0.28f;
    vm.continuousExciteSustain = 0.075f;
    vm.exciteDecayScale = 2.25f;
    vm.pluckBrightnessScale = 0.75f;
    vm.attackScale = 2.05f;
    vm.decayScale = 1.35f;
    vm.releaseScale = 1.70f;
    vm.cutoffScale = 0.84f;
    vm.stereoWidthScale = 1.30f;
    vm.bodyScale = 0.40f;
    // Phase 1.1
    vm.inharmonicityScale = 0.3f;
    // Phase 1.2
    vm.transientTimeScale = 1.45f;
    vm.transientLevel = 0.90f;
    return vm;
}

GuitarVoiceBase::VoiceModel AmbientVoice::buildVoiceModel(int, float, double,
                                                          const InstrSettings&, const InstrCharacteristics&) const
{
    VoiceModel vm;
    vm.outputTrim = 3.00f;
    vm.pitchFollow = 1.0f;
    vm.pitchReferenceHz = 220.0f;
    vm.enableSecondaryString = true;
    vm.secondaryMix = 0.78f;
    vm.secondaryPitchRatio = 1.0f;
    vm.secondaryDetuneScale = 2.5f;
    vm.useContinuousExcitation = true;
    vm.continuousExciteLevel = 0.26f;
    vm.continuousExciteSustain = 0.058f;
    vm.exciteDecayScale = 2.55f;
    vm.attackScale = 2.55f;
    vm.decayScale = 1.55f;
    vm.releaseScale = 2.10f;
    vm.cutoffScale = 0.72f;
    vm.stereoWidthScale = 1.45f;
    vm.registerPanScale = 0.65f;
    vm.bodyScale = 0.42f;
    // Phase 1.1
    vm.inharmonicityScale = 0.2f;
    // Phase 1.2
    vm.transientTimeScale = 1.65f;
    vm.transientLevel = 0.92f;
    return vm;
}

std::unique_ptr<GuitarVoice> createVoiceForInstrument(const int instrumentIndex)
{
    switch (std::clamp(instrumentIndex, 0, kNumInstruments - 1))
    {
        case 0: return std::make_unique<FolkSteelVoice>();
        case 1: return std::make_unique<TwelveStringVoice>();
        case 2: return std::make_unique<FlamencaVoice>();
        case 3: return std::make_unique<CleanVoice>();
        case 4: return std::make_unique<CrunchVoice>();
        case 5: return std::make_unique<LeadVoice>();
        case 6: return std::make_unique<SynthGuitarVoice>();
        case 7: return std::make_unique<EPadVoice>();
        case 8: return std::make_unique<AmbientVoice>();
        default: break;
    }
    return std::make_unique<FolkSteelVoice>();
}

} // namespace mgs
