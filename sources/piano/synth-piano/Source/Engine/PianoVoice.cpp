#include "PianoVoice.h"
#include "SinTable.h"
#include "SimdHelpers.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace mps
{

namespace
{
float clamp01(const float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float normaliseSoftClip(const float signal, const float drive)
{
    return std::tanh(signal * drive) / std::max(0.01f, std::tanh(drive));
}

std::int64_t makeSystemRandomSeed()
{
    auto& systemRandom = juce::Random::getSystemRandom();
    const auto hi = static_cast<std::uint64_t>(static_cast<std::uint32_t>(systemRandom.nextInt()));
    const auto lo = static_cast<std::uint64_t>(static_cast<std::uint32_t>(systemRandom.nextInt()));
    return static_cast<std::int64_t>((hi << 32) ^ lo ^ static_cast<std::uint64_t>(juce::Time::getHighResolutionTicks()));
}

float computeSafeSvfCoefficient(const float frequencyHz, const float sampleRate, const float filterQinv) noexcept
{
    const float fsr = std::max(1.0f, sampleRate);
    const float qInv = std::isfinite(filterQinv) ? std::max(0.001f, filterQinv) : 1.0f;
    const float maxF = -qInv + std::sqrt(qInv * qInv + 4.0f);
    if (!std::isfinite(maxF) || maxF <= 0.0f)
        return 0.0f;

    const float safeFreq = juce::jlimit(20.0f, fsr * 0.45f, std::isfinite(frequencyHz) ? frequencyHz : 20.0f);
    const float rawF = 2.0f * std::sin(juce::MathConstants<float>::pi * safeFreq / fsr);
    const float coeff = juce::jlimit(0.0f, maxF * 0.95f, rawF);
    return std::isfinite(coeff) ? coeff : 0.0f;
}

float contextReleaseDampingCoefficient(const float dampingOver50Ms, const double sampleRate) noexcept
{
    const float boundedDamping = juce::jlimit(0.01f, 1.0f,
        std::isfinite(dampingOver50Ms) ? dampingOver50Ms : 1.0f);
    const float samplesPer50Ms = std::max(1.0f, static_cast<float>(sampleRate) * 0.050f);
    return std::pow(boundedDamping, 1.0f / samplesPer50Ms);
}
} // namespace

float AcousticPianoVoiceBase::readComb(const float* buf, const int bufSize,
                               const int writePos, const float delaySamples) const
{
    const float readPos = static_cast<float>(writePos) - delaySamples;
    const int idx1 = static_cast<int>(std::floor(readPos));
    const float frac = readPos - static_cast<float>(idx1);

    auto wrap = [bufSize](int i) -> int {
        return ((i % bufSize) + bufSize) % bufSize;
    };

    // Hermite cubic interpolation (4-point, third-order) for cleaner resonance
    const float y0 = buf[wrap(idx1 - 1)];
    const float y1 = buf[wrap(idx1)];
    const float y2 = buf[wrap(idx1 + 1)];
    const float y3 = buf[wrap(idx1 + 2)];

    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

PianoSettings AcousticPianoVoiceBase::adaptSettings(const PianoSettings& inputSettings, int, float) const
{
    return inputSettings;
}

bool AcousticPianoVoiceBase::shouldUseThirdString(const PianoCharacteristics& characteristics, const int note) const
{
    // Note 53 = F3: cutoff where the piano's treble bridge begins (3-string unison section above this point)
    return !characteristics.output.isElectric
        && characteristics.string.unisonDetuning < 0.05f
        && note > 53;
}

float AcousticPianoVoiceBase::blendDetunedSignal(const float mainSignal, const float detunedSignal) const
{
    return mainSignal * 0.55f + detunedSignal * 0.45f;
}

float AcousticPianoVoiceBase::applyCharacterProcessing(float signal, const PianoSettings& activeSettings) const
{
    if (activeSettings.performance.modelCharacter <= 0.01f)
        return signal;

    if (chars.output.isElectric)
    {
        const float drive = 1.0f + activeSettings.performance.modelCharacter * kCharacterElectricGain;
        return normaliseSoftClip(signal, drive);
    }

    const float charAmount = activeSettings.performance.modelCharacter * kCharacterAcousticAmount;
    return signal + std::tanh(signal * 2.0f) * charAmount;
}

PianoSettings AcousticPianoVoiceBase::applyDedicatedSettingsProfile(PianoSettings s,
                                                                    const int note,
                                                                    const float velocity) const
{
    if (!pianoDedicatedActive)
        return s;

    const float depth = pianoModelOnly ? 1.25f : 1.0f;
    switch (pianoAlgorithm)
    {
        case PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard:
            s.resonance.soundboardAmount = clamp01(s.resonance.soundboardAmount + 0.055f * depth);
            s.resonance.stringResonance = clamp01(s.resonance.stringResonance + 0.040f * depth);
            s.resonance.damping = clamp01(s.resonance.damping - 0.018f * depth);
            s.tone.brightness = clamp01(s.tone.brightness + 0.026f * depth + velocity * 0.012f);
            s.tone.lowPassHz = juce::jlimit(120.0f, 18000.0f,
                s.tone.lowPassHz * (0.990f + velocity * 0.030f));
            break;

        case PianoInstrumentAlgorithm::BosendorferImperialBassResonator:
        {
            const float bass = note < 64
                ? juce::jlimit(0.0f, 1.0f, (64.0f - static_cast<float>(note)) / 43.0f)
                : 0.0f;
            const float lowMid = note < 72
                ? juce::jlimit(0.0f, 1.0f, (72.0f - static_cast<float>(note)) / 51.0f)
                : 0.0f;
            s.resonance.soundboardAmount = clamp01(s.resonance.soundboardAmount
                + (0.050f + lowMid * 0.055f + bass * 0.075f) * depth);
            s.resonance.stringResonance = clamp01(s.resonance.stringResonance
                + (0.018f + bass * 0.090f) * depth);
            s.resonance.damping = clamp01(s.resonance.damping - (0.025f + bass * 0.055f) * depth);
            s.tone.hammerHardness = clamp01(s.tone.hammerHardness - 0.034f * depth);
            s.tone.brightness = clamp01(s.tone.brightness - (0.035f + velocity * 0.010f) * depth);
            s.envelope.decaySeconds = juce::jmin(13.5f,
                s.envelope.decaySeconds * (1.0f + (0.08f + lowMid * 0.10f + bass * 0.32f) * depth));
            s.envelope.releaseSeconds = juce::jmin(2.0f, s.envelope.releaseSeconds * (1.04f + bass * 0.12f));
            s.tone.lowPassHz = juce::jlimit(120.0f, 18000.0f,
                s.tone.lowPassHz * (0.925f - bass * 0.050f));
            break;
        }

        case PianoInstrumentAlgorithm::YamahaCfxBrightScaleAction:
            s.tone.hammerHardness = clamp01(s.tone.hammerHardness + 0.055f * depth + velocity * 0.040f);
            s.tone.brightness = clamp01(s.tone.brightness + 0.070f * depth + velocity * 0.025f);
            s.resonance.soundboardAmount = clamp01(s.resonance.soundboardAmount - 0.018f * depth);
            s.resonance.stringResonance = clamp01(s.resonance.stringResonance - 0.012f * depth);
            s.resonance.damping = clamp01(s.resonance.damping + 0.062f * depth);
            s.envelope.decaySeconds = juce::jmax(0.10f, s.envelope.decaySeconds * (0.840f - velocity * 0.040f));
            s.envelope.releaseSeconds = juce::jmax(0.06f, s.envelope.releaseSeconds * 0.82f);
            s.tone.lowPassHz = juce::jlimit(120.0f, 18000.0f, s.tone.lowPassHz * 1.070f);
            s.tone.highPassHz = juce::jmax(s.tone.highPassHz, 36.0f);
            break;

        case PianoInstrumentAlgorithm::BastringueHonkyTonkTackRail:
            s.performance.modelCharacter = clamp01(s.performance.modelCharacter + 0.150f * depth + velocity * 0.045f);
            s.resonance.damping = clamp01(s.resonance.damping + 0.060f * depth);
            s.envelope.decaySeconds = juce::jmax(0.18f, s.envelope.decaySeconds * 0.92f);
            s.tone.lowPassHz = juce::jlimit(120.0f, 18000.0f, s.tone.lowPassHz * 0.94f);
            break;

        case PianoInstrumentAlgorithm::PreparedPianoObjectBuzz:
            s.performance.modelCharacter = clamp01(s.performance.modelCharacter + 0.035f * depth);
            s.resonance.damping = clamp01(s.resonance.damping + 0.055f * depth);
            s.envelope.decaySeconds = juce::jmax(0.18f, s.envelope.decaySeconds * 0.94f);
            break;

        default:
            break;
    }

    return s;
}

void AcousticPianoVoiceBase::initialiseDedicatedModel(const float f0, const float fsr) noexcept
{
    dedicatedPhaseA = rng.nextFloat();
    dedicatedPhaseB = rng.nextFloat();
    dedicatedPhaseIncA = 0.0f;
    dedicatedPhaseIncB = 0.0f;
    dedicatedEnvA = 0.0f;
    dedicatedEnvB = 0.0f;
    dedicatedDecayA = 1.0f;
    dedicatedDecayB = 1.0f;
    dedicatedNoiseEnv = 0.0f;
    dedicatedNoiseDecay = 1.0f;
    dedicatedFilterState = 0.0f;
    dedicatedBodyState = 0.0f;
    dedicatedBodyCoeff = 0.0f;

    if (!pianoDedicatedActive || fsr <= 1.0f)
        return;

    const float modelScale = pianoModelOnly ? 1.30f : 1.0f;
    auto setOsc = [f0, fsr](float& phaseInc, float ratio)
    {
        phaseInc = juce::jlimit(0.0f, 0.475f, f0 * ratio / fsr);
    };
    auto setDecay = [fsr](const float seconds)
    {
        return std::exp(-1.0f / (juce::jmax(0.004f, seconds) * fsr));
    };

    switch (pianoAlgorithm)
    {
        case PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard:
            setOsc(dedicatedPhaseIncA, 2.0f);
            setOsc(dedicatedPhaseIncB, 10.6f);
            dedicatedEnvA = 0.024f * modelScale * settings.resonance.stringResonance;
            dedicatedEnvB = 0.018f * modelScale * settings.resonance.soundboardAmount;
            dedicatedDecayA = setDecay(0.95f + settings.envelope.decaySeconds * 0.22f);
            dedicatedDecayB = setDecay(0.62f);
            break;

        case PianoInstrumentAlgorithm::BosendorferImperialBassResonator:
        {
            const float bass = midiNote < 64
                ? juce::jlimit(0.0f, 1.0f, (64.0f - static_cast<float>(midiNote)) / 43.0f)
                : 0.0f;
            const float lowMid = midiNote < 72
                ? juce::jlimit(0.0f, 1.0f, (72.0f - static_cast<float>(midiNote)) / 51.0f)
                : 0.0f;
            setOsc(dedicatedPhaseIncA, 0.5f);
            setOsc(dedicatedPhaseIncB, 1.25f);
            dedicatedEnvA = (0.020f + lowMid * 0.020f + bass * 0.070f) * modelScale;
            dedicatedEnvB = (0.018f + lowMid * 0.030f) * modelScale;
            dedicatedDecayA = setDecay(2.20f + settings.envelope.decaySeconds * 0.35f);
            dedicatedDecayB = setDecay(1.45f + lowMid * 0.50f);
            dedicatedBodyCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 58.0f / fsr);
            break;
        }

        case PianoInstrumentAlgorithm::YamahaCfxBrightScaleAction:
            setOsc(dedicatedPhaseIncA, 3.0f);
            setOsc(dedicatedPhaseIncB, 12.5f);
            dedicatedEnvA = 0.024f * modelScale * (0.50f + vel);
            dedicatedEnvB = 0.011f * modelScale * (0.60f + settings.tone.brightness);
            dedicatedDecayA = setDecay(0.14f);
            dedicatedDecayB = setDecay(0.07f);
            dedicatedNoiseEnv = 0.035f * modelScale * vel;
            dedicatedNoiseDecay = setDecay(0.012f);
            dedicatedBodyCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 3200.0f / fsr);
            break;

        case PianoInstrumentAlgorithm::BastringueHonkyTonkTackRail:
            setOsc(dedicatedPhaseIncA, 1.006f);
            setOsc(dedicatedPhaseIncB, 0.80f);
            dedicatedEnvA = 0.030f * modelScale * settings.performance.modelCharacter;
            dedicatedEnvB = 0.012f * modelScale;
            dedicatedDecayA = setDecay(0.80f);
            dedicatedDecayB = setDecay(1.40f);
            dedicatedNoiseEnv = 0.026f * modelScale * vel;
            dedicatedNoiseDecay = setDecay(0.012f);
            break;

        case PianoInstrumentAlgorithm::PreparedPianoObjectBuzz:
            setOsc(dedicatedPhaseIncA, 2.31f);
            setOsc(dedicatedPhaseIncB, 5.43f);
            dedicatedEnvA = 0.014f * modelScale * (0.25f + settings.performance.modelCharacter) * vel;
            dedicatedEnvB = 0.004f * modelScale * vel * vel;
            dedicatedDecayA = setDecay(0.095f);
            dedicatedDecayB = setDecay(0.035f);
            dedicatedNoiseEnv = 0.008f * modelScale * vel;
            dedicatedNoiseDecay = setDecay(0.012f);
            break;

        default:
            break;
    }
}

float AcousticPianoVoiceBase::applyDedicatedAcousticModel(float signal) noexcept
{
    if (!pianoDedicatedActive)
        return signal;

    auto osc = [this](float& phase, const float phaseInc, float& env, const float decay) noexcept
    {
        if (env <= 1.0e-6f || phaseInc <= 0.0f)
            return 0.0f;

        const float value = fastSin(phase) * env;
        phase += phaseInc * pitchBendFactor;
        phase -= std::floor(phase);
        env *= decay;
        return value;
    };

    const float a = osc(dedicatedPhaseA, dedicatedPhaseIncA, dedicatedEnvA, dedicatedDecayA);
    const float b = osc(dedicatedPhaseB, dedicatedPhaseIncB, dedicatedEnvB, dedicatedDecayB);
    float noise = 0.0f;
    if (dedicatedNoiseEnv > 1.0e-6f)
    {
        const float raw = rng.nextFloat() * 2.0f - 1.0f;
        dedicatedFilterState += 0.28f * (raw - dedicatedFilterState);
        noise = (raw - dedicatedFilterState) * dedicatedNoiseEnv;
        dedicatedNoiseEnv *= dedicatedNoiseDecay;
    }

    switch (pianoAlgorithm)
    {
        case PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard:
            signal += a * 0.92f + b * 0.95f + std::tanh(signal * 1.6f) * 0.010f;
            break;

        case PianoInstrumentAlgorithm::BosendorferImperialBassResonator:
            dedicatedBodyState += dedicatedBodyCoeff * (signal - dedicatedBodyState);
            signal = signal * 0.975f + dedicatedBodyState * 0.130f + a * 1.10f + b * 0.62f;
            break;

        case PianoInstrumentAlgorithm::YamahaCfxBrightScaleAction:
        {
            dedicatedBodyState += dedicatedBodyCoeff * (signal - dedicatedBodyState);
            const float actionEdge = signal - dedicatedBodyState;
            signal = signal * 0.965f + actionEdge * 0.095f + a * 0.90f + b * 0.68f + noise * 0.95f;
            signal = signal * 0.982f + std::tanh(signal * 2.4f) * 0.020f;
            break;
        }

        case PianoInstrumentAlgorithm::BastringueHonkyTonkTackRail:
            signal = signal * (0.985f + b * 0.08f) + a * 0.80f + noise * 0.95f;
            break;

        case PianoInstrumentAlgorithm::PreparedPianoObjectBuzz:
            signal = signal * 0.99f + a * 0.80f + b * 0.45f + noise * 0.50f;
            break;

        default:
            break;
    }

    return std::isfinite(signal) ? signal : 0.0f;
}

void AcousticPianoVoiceBase::updateRealtimeEnvelope() noexcept
{
    const auto fsr = static_cast<float>(sr);
    if (fsr <= 0.0f)
        return;

    const float attackSeconds = settings.envelope.attackSeconds * realtimeModulation.attackScale;
    attackRate = (attackSeconds > 0.0001f)
        ? 1.0f / (attackSeconds * fsr)
        : 1.0f;

    // Apply velocity-derived decay speedup (P7/P13 fix — from voice state, not settings)
    const float velDecayScale = velocityMod.decayScale * realtimeModulation.decayScale;
    const float decaySeconds = settings.envelope.decaySeconds * velDecayScale;

    const float promptTime = std::max(0.01f, decaySeconds * chars.envelope.promptSoundRatio);
    promptSoundCoeff = std::exp(-1.0f / (promptTime * fsr));
    promptSoundTarget = chars.envelope.afterSoundRatio;

    const float afterTime = std::max(0.05f, decaySeconds * chars.envelope.decay1Ratio);
    afterSoundCoeff = std::exp(-1.0f / (afterTime * fsr));

    // Apply velocity-derived sustain boost (P7/P13 fix — from voice state, not settings)
    const float velSustainScale = velocityMod.sustainScale * (chars.string.hasDampers ? 1.0f : 0.5f);
    afterSoundTarget = chars.envelope.sustainPlatform * settings.envelope.sustainLevel * velSustainScale;
    sustainLevel = afterSoundTarget;
}

void AcousticPianoVoiceBase::updateRealtimeFilter() noexcept
{
    const auto fsr = static_cast<float>(sr);
    if (fsr <= 0.0f)
        return;

    const float baseQ = juce::jmax(0.5f, 0.7f + settings.tone.brightness * 0.5f);
    const float modQ = juce::jlimit(0.5f, 2.5f, baseQ + realtimeModulation.resonanceOffset);
    filterQinv = 1.0f / modQ;

    filterF = computeSafeSvfCoefficient(settings.tone.lowPassHz * realtimeModulation.cutoffMul, fsr, filterQinv);
    filterFTarget = filterF;
    const float maxAllowedF = computeSafeSvfCoefficient(fsr * 0.45f, fsr, filterQinv);
    filterFCurrent = std::isfinite(filterFCurrent) && filterFCurrent > 0.0f ? filterFCurrent : filterFTarget;
    filterFCurrent = juce::jlimit(0.0f, maxAllowedF, filterFCurrent);
}

void AcousticPianoVoiceBase::updateMaxAgeSamples() noexcept
{
    const float decaySeconds = settings.envelope.decaySeconds * realtimeModulation.decayScale;
    maxAgeSamples = static_cast<int>(sr * std::max(1.0f,
        decaySeconds * 6.0f + settings.envelope.releaseSeconds * 3.0f));
    if (maxAgeSamples > static_cast<int>(sr * static_cast<double>(kMaxVoiceAgeSec)))
        maxAgeSamples = static_cast<int>(sr * static_cast<double>(kMaxVoiceAgeSec));
}

void AcousticPianoVoiceBase::applyRealtimeModulation(const VoiceRealtimeModulation& modulation) noexcept
{
    realtimeModulation.cutoffMul = juce::jlimit(0.0625f, 16.0f, modulation.cutoffMul);
    realtimeModulation.resonanceOffset = juce::jlimit(-1.0f, 1.0f, modulation.resonanceOffset);
    realtimeModulation.attackScale = juce::jlimit(0.0625f, 16.0f, modulation.attackScale);
    realtimeModulation.decayScale = juce::jlimit(0.0625f, 16.0f, modulation.decayScale);
    realtimeModulation.densityResonanceScale = juce::jlimit(0.35f, 1.0f, modulation.densityResonanceScale);
    updateRealtimeEnvelope();
    updateRealtimeFilter();
    updateMaxAgeSamples();
}

void AcousticPianoVoiceBase::noteOn(const PianoSettings& inputSettings,
                            const int note, const float velocity,
                            const double sampleRate)
{
    rng.setSeed(hasPendingRandomSeed ? pendingRandomSeed : makeSystemRandomSeed());
    hasPendingRandomSeed = false;

    pianoIndex = juce::jlimit(0, kNumPianos - 1, getPianoIndex());
    const auto& model = getPianoInstrumentModel(pianoIndex);
    pianoAlgorithm = model.algorithm;
    const auto renderMode = getPianoRenderEngineMode();
    pianoDedicatedActive = model.readiness == PianoEngineReadiness::DedicatedVoice
        && renderMode != PianoRenderEngineMode::LegacyFamily;
    pianoModelOnly = renderMode == PianoRenderEngineMode::V2ModelOnly;

    settings = adaptSettings(inputSettings, note, velocity);
    settings = applyDedicatedSettingsProfile(settings, note, velocity);
    chars = getCharacteristics();
    sr = std::max(1.0, sampleRate);
    vel = juce::jlimit(0.0f, 1.0f, velocity);
    midiNote = note;
    ageSamples = 0;
    realtimeModulation = {};

    const auto fsr = static_cast<float>(sr);

    // Apply Railsback stretched tuning if this piano model specifies it
    const float railsbackOffsetSemis = (chars.railsbackScale > 0.0f)
        ? getRailsbackCents(note, chars.railsbackScale) / 100.0f
        : 0.0f;
    const float f0 = 440.0f * std::pow(2.0f,
        (static_cast<float>(note) - 69.0f + settings.tuneSemitones + railsbackOffsetSemis) / 12.0f);
    initialiseDedicatedModel(f0, fsr);

    const float effectiveB = getScaledInharmonicityB(note, chars.string);

    // Register-adaptive partial count (up to kMaxPartials = 32)
    const int registerLimited = (midiNote <= 72)
        ? chars.string.partialCount
        : std::max(4, chars.string.partialCount - (midiNote - 72) / 3);
    const int np = std::min(registerLimited, kMaxPartials);
    numActivePartials = np;

    const float effectiveHardness = chars.excitation.hammerHardnessBase *
        (kHammerHardnessBaseBlend + kHammerHardnessVelBlend * settings.tone.hammerHardness);
    const float hammerCutoff = f0 * (kHammerCutoffBase + effectiveHardness * kHammerCutoffRange * (kHammerCutoffVelBase + kHammerCutoffVelRange * vel));
    const float brightMult = kBrightnessMultMin + settings.tone.brightness * kBrightnessMultRange;

    for (int n = 0; n < np; ++n)
    {
        const int harmonic = n + 1;
        const float fn = static_cast<float>(harmonic) * f0 *
            std::sqrt(1.0f + effectiveB * static_cast<float>(harmonic * harmonic));

        if (fn >= fsr * 0.48f)
        {
            numActivePartials = std::max(1, n);
            break;
        }

        const float baseAmp = 1.0f / static_cast<float>(harmonic);
        const float freqRatio = fn / (hammerCutoff * brightMult);
        const float hammerFilter = 1.0f / (1.0f + freqRatio * freqRatio);
        // FIX 3.2: Velocity spread — subtle per-partial amplitude variation
        // Each partial gets a random ±5% variation for natural imperfection
        const float velocitySpread = 0.95f + 0.1f * rng.nextFloat();  // 0.95 to 1.05
        // Felt/object preparation attenuates upper string modes; a fixed floor
        // must not keep the rejected highs ringing after the contact has ended.
        const bool mutedPreparation = pianoIndex == 4;
        const float ampFloor = mutedPreparation ? 0.04f : kPartialBaseAmpFloor;
        const float filterMix = mutedPreparation ? 0.96f : kPartialHammerFilterMix;
        const float amp = baseAmp * (ampFloor + filterMix * hammerFilter) * velocitySpread;

        const float partialDecayScale = 1.0f /
            (1.0f + settings.resonance.damping * static_cast<float>(harmonic - 1) * 0.25f);
        const float partialDecayTime = std::max(0.02f,
            settings.envelope.decaySeconds * chars.envelope.decay2Ratio * partialDecayScale);
        const float decayCoeff = std::exp(-1.0f / (partialDecayTime * fsr));

        auto& partial = partials[static_cast<std::size_t>(n)];
        partial.phase = 0.0f;
        partial.phaseInc = fn / fsr;
        partial.amplitude = amp;
        partial.decayCoeff = decayCoeff;
    }

    // -- Normalize partial amplitudes for consistent volume across registers --
    // FIX 2.1: Register-adaptive normalization — expected partials vs actual
    {
        float totalAmp = 0.0f;
        for (int n = 0; n < numActivePartials; ++n)
            totalAmp += partials[static_cast<std::size_t>(n)].amplitude;

        if (totalAmp > 0.001f)
        {
            // FIX 2.1: Normalize based on expected partials for this register
            // The grave has more partials but shouldn't sound disproportionately louder
            const float expectedPartials = (midiNote <= 72)
                ? static_cast<float>(chars.string.partialCount)
                : std::max(4.0f, static_cast<float>(chars.string.partialCount) - static_cast<float>(midiNote - 72) / 3.0f);
            const float partialNormFactor = std::sqrt(expectedPartials / std::max(1.0f, static_cast<float>(numActivePartials)));
            
            // FIX 2.2: Register-adaptive volume boost for bass
            // Bass notes have longer wavelengths that tend to sound quieter in room acoustics
            // Boost low register to compensate for psychoacoustic loudness curve
            const float bassBoostFactor = (midiNote < 48) 
                ? 1.0f + (48.0f - static_cast<float>(midiNote)) / 96.0f * 0.35f  // +35% max for A0
                : 1.0f;
            const float normFactor = kPartialNormReference * bassBoostFactor / std::sqrt(totalAmp) * partialNormFactor;
            for (int n = 0; n < numActivePartials; ++n)
                partials[static_cast<std::size_t>(n)].amplitude *= normFactor;
        }
    }

    // -- Detuned string bank --
    hasDetuning = (chars.string.unisonDetuning > 0.001f);
    if (hasDetuning)
    {
        const float detuneHz = chars.string.unisonDetuning * settings.performance.modelCharacter * f0 * kDetuneDepthFactor;
        for (int n = 0; n < numActivePartials; ++n)
        {
            auto& detuned = detunedPartials[static_cast<std::size_t>(n)];
            const auto& original = partials[static_cast<std::size_t>(n)];
            detuned = original;
            detuned.phase = rng.nextFloat();
            detuned.phaseInc += detuneHz * static_cast<float>(n + 1) / fsr;
        }
    }

    // -- Third string (unison) --
    hasThirdString = shouldUseThirdString(chars, midiNote);
    if (hasThirdString)
    {
        const float mistuneRatio3 = std::pow(2.0f, (0.8f + vel * 1.2f) / 1200.0f) - 1.0f;
        const float mistuneRatio1 = std::pow(2.0f, (-0.6f - vel * 0.8f) / 1200.0f) - 1.0f;

        for (int n = 0; n < numActivePartials; ++n)
        {
            const int harmonic = n + 1;
            const auto& original = partials[static_cast<std::size_t>(n)];

            auto& string3 = string3Partials[static_cast<std::size_t>(n)];
            string3 = original;
            string3.phase = rng.nextFloat();
            string3.phaseInc += mistuneRatio3 * f0 * static_cast<float>(harmonic) / fsr;

            partials[static_cast<std::size_t>(n)].phaseInc +=
                mistuneRatio1 * f0 * static_cast<float>(harmonic) / fsr;
        }
    }

    // -- Duplex resonance (upper partials above treble bridge) --
    numDuplexPartials = 0;
    if (chars.string.duplexScale > 0.001f && midiNote > kDuplexNoteThreshold)
    {
        float duplexRatioStart = kDuplexFreqRatioStart;
        float duplexRatioStep = 2.0f;
        float duplexGainScale = 1.0f;
        float duplexDecayScale = 1.5f;
        switch (pianoAlgorithm)
        {
            case PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard:
                duplexRatioStart = 8.3f;
                duplexRatioStep = 2.25f;
                duplexGainScale = 1.25f;
                duplexDecayScale = 1.65f;
                break;

            case PianoInstrumentAlgorithm::BosendorferImperialBassResonator:
                duplexRatioStart = 6.6f;
                duplexRatioStep = 1.55f;
                duplexGainScale = 0.52f;
                duplexDecayScale = 2.20f;
                break;

            case PianoInstrumentAlgorithm::YamahaCfxBrightScaleAction:
                duplexRatioStart = 9.4f;
                duplexRatioStep = 2.55f;
                duplexGainScale = 1.55f;
                duplexDecayScale = 0.95f;
                break;

            default:
                break;
        }

        const float duplexAmt = chars.string.duplexScale * settings.resonance.stringResonance;
        for (int d = 0; d < kMaxDuplexPartials; ++d)
        {
            const float ratio = duplexRatioStart + static_cast<float>(d) * duplexRatioStep;
            const float dFreq = f0 * ratio;
            if (dFreq >= fsr * 0.45f) break;

            auto& dp = duplexPartials[static_cast<std::size_t>(d)];
            dp.phase = rng.nextFloat();
            dp.phaseInc = dFreq / fsr;
            dp.amplitude = kDuplexGainBase * duplexAmt * duplexGainScale
                / (1.0f + static_cast<float>(d) * 0.5f);
            dp.decayCoeff = std::exp(-1.0f / (settings.envelope.decaySeconds * duplexDecayScale * fsr));
            numDuplexPartials = d + 1;
        }
    }

    // -- 5-phase envelope --

    // FIX 1.1: Attack continuity — remove discontinuity when attack is very short
    // Previously: if attackSeconds <= 0.0001, jumped directly to PromptSound with envLevel = 1.0f
    // Now: always use Attack state with ultra-fast rate for smooth transition
    if (settings.envelope.attackSeconds > 0.0001f)
    {
        attackRate = 1.0f / (settings.envelope.attackSeconds * fsr);
        envState = EnvState::Attack;
    }
    else
    {
        // Ultra-fast attack: ~0.05ms instead of jumping to PromptSound
        // This maintains envelope continuity for rhythm/percussive presets
        attackRate = 1.0f / (0.00005f * fsr);  // ~2-3 samples at 44.1k
        envState = EnvState::Attack;
    }

    // FIX 1.2: Velocity → Sustain/Decay coupling (P7/P13 fix)
    // Store velocity-derived scales in voice state (not in shared settings)
    // This prevents side-effects when settings are reused across notes
    velocityMod.sustainScale = 1.0f + vel * vel * 0.12f;   // subtle quadratic boost
    velocityMod.decayScale   = 1.0f - vel * 0.04f;           // slight decay shortening

    // Hard hits decay slightly faster initially (string settles)
    // (velDecayBoost applied via velocityMod.decayScale in updateRealtimeEnvelope)

    // Prompt-sound phase: fast initial decay from 1.0 to promptSoundTarget
    const float promptTime = std::max(0.01f, settings.envelope.decaySeconds * chars.envelope.promptSoundRatio);
    promptSoundCoeff = std::exp(-1.0f / (promptTime * fsr));
    promptSoundTarget = chars.envelope.afterSoundRatio;

    // After-sound phase: slower decay from promptSoundTarget to sustainLevel
    const float afterTime = std::max(0.05f, settings.envelope.decaySeconds * chars.envelope.decay1Ratio);
    afterSoundCoeff = std::exp(-1.0f / (afterTime * fsr));
    afterSoundTarget = chars.envelope.sustainPlatform * settings.envelope.sustainLevel *
        (chars.string.hasDampers ? 1.0f : 0.5f);
    sustainLevel = afterSoundTarget;

    // Release
    releaseCoeff = std::exp(-1.0f / (std::max(0.005f, settings.envelope.releaseSeconds) * fsr));
    releaseDelaySamples = 0;

    envLevel = 0.0f;

    // -- Stulov hammer model --
    {
        const float contactTime = std::max(0.0005f,
            kStulovContactTimeSec - kStulovContactTimeVelScale * vel);
        stulovDecayCoeff = std::exp(-1.0f / (contactTime * fsr));
        
        // FIX 2.3: Register-adaptive felt stiffness
        // Bass strings are heavier → more inertia → hammer feels "softer" (lower effective stiffness)
        // Treble strings are lighter → less inertia → hammer feels "brighter" (higher effective stiffness)
        // Formula: stulovStiffness = base * registerScale * stringLengthFactor
        //   registerScale = f0/refHz   (bass=0.19, treble=3.8) — frequency proxy
        //   stringLengthFactor = sqrt(refHz/f0)  (bass=2.29, treble=0.51) — string-length proxy
        // Combined effect on bass: 0.19 * 2.29 = 0.44 → lower stiffness → softer attack ✓
        // Combined effect on treble: 3.8 * 0.51 = 1.94 → higher stiffness → brighter attack ✓
        // The comment previously said "bass → higher stiffness" which was inverted.
        const float registerScale = std::max(0.1f, f0 / kInharmonicityRefHz);
        const float stringLengthFactor = std::sqrt(kInharmonicityRefHz / std::max(20.0f, f0)); // inverse
        stulovStiffness = kStulovFeltStiffnessBase * registerScale * stringLengthFactor;
        
        stulovCompression = vel;    // initial compression proportional to velocity
        stulovForce = stulovStiffness * std::pow(stulovCompression, kStulovAlpha);
        stulovGainScale = kStulovOutputScale * (kStulovRefSampleRate / fsr);
    }

    // Hammer noise (spectral coloring)
    hammerEnvLevel = vel * (1.0f + effectiveHardness * 2.0f);
    hammerDecayCoeff = std::exp(-1.0f / (kHammerNoiseDecaySec * fsr));
    {
        const float hammerNorm = juce::jlimit(0.0f, 1.0f,
            (static_cast<float>(note) - 21.0f) / 67.0f);
        hammerFiltCoeff = 0.97f - hammerNorm * 0.55f;
        hammerFiltState = 0.0f;
    }

    // -- Key click (louder in treble where the mechanism is more audible) --
    {
        const float regNorm = juce::jlimit(0.0f, 1.0f, (static_cast<float>(midiNote) - 21.0f) / 87.0f);
        keyClickLevel = kKeyClickLevel * vel * (0.75f + 0.35f * regNorm);
    }
    keyClickDecay = std::exp(-1.0f / (kKeyClickDecaySec * fsr));
    keyClickRampLen = std::max(1, static_cast<int>(0.0005f * fsr)); // 0.5ms onset
    keyClickRampPos = 0;
    keyRelClickLevel = 0.0f;
    keyRelClickDecay = std::exp(-1.0f / (kKeyRelClickDecaySec * fsr));

    // -- Damper thud (reset, activated on noteOff) --
    damperThudLevel = 0.0f;
    damperThudFiltState = 0.0f;

    // -- SVF filter --
    filterQinv = 1.0f / juce::jmax(0.5f, 0.7f + settings.tone.brightness * 0.5f);
    filterF = computeSafeSvfCoefficient(settings.tone.lowPassHz, fsr, filterQinv);
    svfLow = 0.0f;
    svfBand = 0.0f;

    // FIX 1.3: Initialize highpass filter
    if (settings.tone.highPassHz > 1.0f) {
        hpCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * settings.tone.highPassHz / fsr);
    } else {
        hpCoeff = 0.0f;  // off
    }
    hpState = 0.0f;
    hpPrevInput = 0.0f;

    filterFTarget = filterF;
    {
        // FIX 3.1: Enhanced velocity → brightness curve
        // - Cubic curve for more musical response (soft = warm, hard = bright)
        // - Register-adaptive: bass notes respond MORE to velocity (realistic piano behavior)
        const float registerVelSensitivity = 0.5f + 0.5f * (1.0f - juce::jlimit(0.0f, 1.0f, 
            (static_cast<float>(midiNote) - 21.0f) / 67.0f)); // bass = 1.0, treble = 0.5
        
        // Cubic velocity curve: vel^3 emphasizes forte playing, vel^0.5 softens pp
        const float cubicVel = vel * vel * vel;  // emphasizes forte
        const float softVelCurve = std::sqrt(vel);  // softens piano
        
        // Blend curves based on hammerHardness (user control)
        const float velCurve = softVelCurve + (cubicVel - softVelCurve) * settings.tone.hammerHardness;
        
        // Final boost with register sensitivity
        const float boost = velCurve * settings.tone.hammerHardness * 3.0f * registerVelSensitivity;
        filterFCurrent = computeSafeSvfCoefficient(settings.tone.lowPassHz * (1.0f + boost), fsr, filterQinv);
        
        // Register-dependent brightness decay: bass strings ring bright longer
        const float registerNorm = juce::jlimit(0.0f, 1.0f, (static_cast<float>(midiNote) - 21.0f) / 87.0f);
        const float brightDecayTime = (0.30f + vel * 0.30f) * (1.0f + (1.0f - registerNorm) * 1.5f);
        brightnessDecayCoeff = std::exp(-1.0f / (brightDecayTime * fsr));
    }

    // -- Body resonator comb --
    const float bodyFreq = f0 * chars.resonance.bodyDelayRatio;
    bodyDelaySamples = juce::jlimit(2.0f, static_cast<float>(kBodyBufSize - 2),
                                    fsr / std::max(20.0f, bodyFreq));
    bodyFeedback = settings.resonance.soundboardAmount * kBodyMaxFeedback;
    bodyDampState = 0.0f;
    bodyWritePos = 0;
    std::fill(bodyBuf.begin(), bodyBuf.end(), 0.0f);

    // -- Sympathetic resonance matrix (12 coupled comb filters) --
    sympMatrixGain = settings.resonance.stringResonance;
    {
        // Tune 12 resonators to musically-related intervals around f0
        static constexpr float kSympIntervals[kSympMatrixSize] = {
            12.0f, 7.0f, 5.0f, 24.0f, 19.0f, -12.0f,
            4.0f, 3.0f, 16.0f, 36.0f, -5.0f, -7.0f
        };
        static constexpr float kBosendorferSympIntervals[kSympMatrixSize] = {
            -12.0f, -19.0f, 7.0f, 0.0f, 5.0f, -24.0f,
            -5.0f, 12.0f, 3.0f, 16.0f, -7.0f, 19.0f
        };
        static constexpr float kYamahaSympIntervals[kSympMatrixSize] = {
            12.0f, 19.0f, 24.0f, 7.0f, 28.0f, 31.0f,
            5.0f, 16.0f, 36.0f, 40.0f, 9.0f, 3.0f
        };
        const float* sympIntervals = kSympIntervals;
        float feedbackModelScale = 1.0f;
        switch (pianoAlgorithm)
        {
            case PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard:
                feedbackModelScale = 1.06f;
                break;

            case PianoInstrumentAlgorithm::BosendorferImperialBassResonator:
                sympIntervals = kBosendorferSympIntervals;
                feedbackModelScale = 1.20f;
                break;

            case PianoInstrumentAlgorithm::YamahaCfxBrightScaleAction:
                sympIntervals = kYamahaSympIntervals;
                feedbackModelScale = 0.84f;
                break;

            default:
                break;
        }

        for (int r = 0; r < kSympMatrixSize; ++r)
        {
            auto& res = sympMatrix[static_cast<std::size_t>(r)];
            const float sympFreq = f0 * std::pow(2.0f, sympIntervals[r] / 12.0f);
            const float clampedFreq = juce::jlimit(20.0f, fsr * 0.45f, sympFreq);
            res.delaySamples = juce::jlimit(2.0f, static_cast<float>(kSympCombBufSize - 2),
                                            fsr / clampedFreq);
            // Register-dependent decay: bass resonators ring longer
            const float regNorm = juce::jlimit(0.0f, 1.0f, (static_cast<float>(midiNote) - 21.0f) / 87.0f);
            const float regDecay = kSympMatrixDecayBase + (1.0f - regNorm) * 0.0002f;
            res.feedback = regDecay *
                settings.resonance.stringResonance * feedbackModelScale * (1.0f - static_cast<float>(r) * 0.03f);
            res.dampState = 0.0f;
            res.writePos = 0;
            std::fill(res.buf.begin(), res.buf.end(), 0.0f);
        }
    }

    // -- 8-mode soundboard --
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        const float sbAmount = settings.resonance.soundboardAmount;
        for (int m = 0; m < kSoundboardModes; ++m)
        {
            auto& mode = sbModes[static_cast<std::size_t>(m)];
            float modeFreqRatio = kSoundboardModeFreqRatio[m];
            float modeGain = kSoundboardModeGain[m];
            float modeQ = kSoundboardModeQ[m];
            switch (pianoAlgorithm)
            {
                case PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard:
                    modeFreqRatio *= (m >= 4 ? 1.025f : 1.0f);
                    modeGain *= (m >= 3 ? 1.18f : 1.04f);
                    modeQ *= 1.05f;
                    break;

                case PianoInstrumentAlgorithm::BosendorferImperialBassResonator:
                    modeFreqRatio *= (m == 0 ? 0.72f : 0.82f + static_cast<float>(m) * 0.015f);
                    modeGain *= (m < 3 ? 1.34f : 0.58f);
                    modeQ *= 1.22f;
                    break;

                case PianoInstrumentAlgorithm::YamahaCfxBrightScaleAction:
                    modeFreqRatio *= (m >= 2 ? 1.10f : 1.04f);
                    modeGain *= (m < 2 ? 0.82f : 1.42f);
                    modeQ *= 0.78f;
                    break;

                default:
                    break;
            }

            const float modeFreq = f0 * modeFreqRatio;
            if (modeFreq >= fsr * 0.45f || chars.resonance.soundboardQ < 0.01f)
            {
                mode = {};
                continue;
            }
            const float w0 = twoPi * modeFreq / fsr;
            const float bwInv = chars.resonance.soundboardQ * sbAmount * modeQ;
            const float r = std::exp(-w0 / std::max(0.5f, bwInv));
            mode.coeffA = 2.0f * r * std::cos(w0);
            mode.coeffB = -(r * r);
            const float normFactor = (1.0f - r) * 2.0f * std::sin(w0);
            mode.gain = sbAmount * modeGain * normFactor;
            mode.state1 = 0.0f;
            mode.state2 = 0.0f;
        }
    }

    // -- Pan --
    const float registerPan = (static_cast<float>(midiNote - 64) / 87.0f) * kRegisterPanSpread;
    const float totalPan = juce::jlimit(-1.0f, 1.0f, settings.spatial.pan + registerPan);
    panL = std::sqrt(0.5f * (1.0f - totalPan));
    panR = std::sqrt(0.5f * (1.0f + totalPan));

    updateMaxAgeSamples();
}

void AcousticPianoVoiceBase::forceQuickRelease() noexcept
{
    if (envState == EnvState::Off)
        return;
    envState = EnvState::Release;
    releaseDelaySamples = 0;
    const auto releaseSamples = std::max(1.0f, static_cast<float>(std::max(1.0, sr)) * 0.005f);
    releaseCoeff = std::exp(std::log(0.001f) / releaseSamples);
}

void AcousticPianoVoiceBase::noteOff()
{
    if (envState == EnvState::Off || envState == EnvState::Release)
        return;

    const auto fsr = static_cast<float>(sr);

    // Half-pedaling: damperPosition expresses pedal-down amount.
    // Low values behave like normal key release, higher values progressively preserve ring.
    const float effectiveDamping = 1.0f - damperPosition;  // 0=pedal fully down, 1=pedal fully up

    if (effectiveDamping < 0.02f)
    {
        // Pedal fully down — sustain the string until pedal release.
        return;
    }

    envState = EnvState::Release;

    const float releaseScale = 0.20f + 0.80f * effectiveDamping * effectiveDamping;
    const float scaledReleaseTime = std::max(0.005f, settings.envelope.releaseSeconds) / releaseScale;
    releaseCoeff = std::exp(-1.0f / (scaledReleaseTime * fsr));
    releaseDelaySamples = static_cast<int>((0.040f * damperPosition * damperPosition) * fsr);

    // Key release click
    keyRelClickLevel = kKeyRelClickLevel * vel * std::sqrt(effectiveDamping);
    keyRelClickDecay = std::exp(-1.0f / (kKeyRelClickDecaySec * fsr));
    keyRelClickRampLen = std::max(1, static_cast<int>(0.0005f * static_cast<float>(sr))); // 0.5ms onset
    keyRelClickRampPos = 0;

    // Damper thud — proportional to current level and damping effectiveness
    if (chars.string.hasDampers)
    {
        damperThudLevel = envLevel * vel * kDamperThudMix * (0.35f + 0.65f * effectiveDamping);
        damperThudDecay = std::exp(-1.0f / (kDamperThudDecaySec * fsr));
        damperThudFiltState = 0.0f;
        damperThudRampLen = std::max(1, static_cast<int>(0.001f * fsr)); // 1ms onset
        damperThudRampPos = 0;
    }
}

void AcousticPianoVoiceBase::reSustain()
{
    // Recapture a releasing voice when the sustain pedal is re-pressed
    if (envState != EnvState::Release)
        return;

    // Transition back to Sustain phase at current level
    releaseDelaySamples = 0;
    envState = EnvState::Sustain;
}

void AcousticPianoVoiceBase::render(juce::AudioBuffer<float>& buffer,
                            const int startSample, const int numSamples)
{
    if (envState == EnvState::Off)
        return;

    const int numChannels = buffer.getNumChannels();
    if (numChannels <= 0)
        return;

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    // Hoist loop-invariant multiplies
    // Una corda shifts the hammer to hit fewer strings → softer, mellower tone
    const float unaCordaGain = unaCordaActive ? 0.75f : 1.0f;
    const float outputGain = vel * settings.level * unaCordaGain;
    const float pedalLift = juce::jlimit(0.0f, 1.0f, damperPosition);
    const float pedalResonanceLift = std::sqrt(pedalLift);
    const float densityPressure = juce::jlimit(0.0f, 1.0f,
        juce::jmax(renderContext.density, 1.0f - realtimeModulation.densityResonanceScale));
    const float releasePressure = juce::jlimit(0.0f, 1.0f, renderContext.releasePressure);
    const float sustainPressure = juce::jlimit(0.0f, 1.0f, renderContext.sustainPressure);
    const float repeatedPressure = juce::jlimit(0.0f, 1.0f, renderContext.repeatedNotePressure);
    const float collisionPressure = juce::jlimit(0.0f, 1.0f, renderContext.harmonicCollision);
    const float tailOwnership = juce::jlimit(0.18f, 1.0f, renderContext.tailOwnership);
    const float registerPressure = juce::jlimit(-1.0f, 1.0f, renderContext.registerPressure);
    const float resonanceBudget = juce::jlimit(0.32f, 1.0f,
        1.0f - densityPressure * 0.22f
             - releasePressure * 0.12f
             - repeatedPressure * 0.24f
             - collisionPressure * 0.28f
             - (1.0f - tailOwnership) * 0.20f);
    const float soundboardBudget = juce::jlimit(0.36f, 1.0f,
        resonanceBudget * (1.0f - juce::jmax(0.0f, -registerPressure) * 0.10f));
    const float bodyBudget = juce::jlimit(0.28f, 1.0f,
        resonanceBudget * (1.0f - juce::jmax(0.0f, -registerPressure) * 0.18f));
    const float sympBudget = juce::jlimit(0.24f, 1.0f,
        resonanceBudget * (1.0f - sustainPressure * 0.08f - collisionPressure * 0.12f));
    const float duplexBudget = juce::jlimit(0.35f, 1.0f,
        resonanceBudget * (1.0f - repeatedPressure * 0.16f - collisionPressure * 0.18f));
    const float densityResonanceScale = juce::jlimit(0.35f, 1.0f,
        realtimeModulation.densityResonanceScale * resonanceBudget);
    const float releaseTailScale = envState == EnvState::Release
        ? juce::jlimit(0.48f, 1.0f, tailOwnership * (0.88f - releasePressure * 0.14f))
        : 1.0f;
    const float bodyMixScale = (0.35f + pedalResonanceLift * 0.65f) * bodyBudget;
    const float sympMixScale = (0.20f + pedalResonanceLift * 0.80f) * sympBudget;
    const float bodyFeedbackScale = (0.60f + pedalResonanceLift * 0.40f) * releaseTailScale;
    const float duplexDensityScale = (0.82f + densityResonanceScale * 0.18f) * duplexBudget;
    const float releaseExtraDamping = juce::jlimit(0.88f, 1.0f,
        1.0f - releasePressure * 0.035f
             - repeatedPressure * 0.040f
             - collisionPressure * 0.050f
             - (1.0f - tailOwnership) * 0.060f);
    const float releaseContextCoeff = contextReleaseDampingCoefficient(releaseExtraDamping, sr);

    for (int i = 0; i < numSamples; ++i)
    {
        if (envState == EnvState::Off)
            break;

        // ---- 5-phase envelope ----
        switch (envState)
        {
        case EnvState::Attack:
            envLevel += attackRate;
            if (envLevel >= 1.0f)
            {
                envLevel = 1.0f;
                envState = EnvState::PromptSound;
            }
            break;

        case EnvState::PromptSound:
            envLevel = promptSoundTarget + (envLevel - promptSoundTarget) * promptSoundCoeff;
            if (envLevel <= promptSoundTarget + 0.003f)
            {
                envLevel = promptSoundTarget;
                envState = EnvState::AfterSound;
            }
            break;

        case EnvState::AfterSound:
            envLevel = afterSoundTarget + (envLevel - afterSoundTarget) * afterSoundCoeff;
            if (envLevel <= afterSoundTarget + 0.002f)
            {
                envLevel = afterSoundTarget;
                envState = EnvState::Sustain;
            }
            break;

        case EnvState::Sustain:
            // Sustain decay — string energy dissipates naturally via partials (per-piano rate)
            envLevel *= chars.envelope.sustainDecayCoeff;
            if (envLevel < kSustainKillThreshold)
            {
                envLevel = 0.0f;
                envState = EnvState::Off;
                break;
            }
            break;

        case EnvState::Release:
            if (releaseDelaySamples > 0)
            {
                --releaseDelaySamples;
            }
            else
            {
                envLevel *= releaseCoeff * releaseContextCoeff;
            }
            if (envLevel < 0.0001f)
            {
                envLevel = 0.0f;
                envState = EnvState::Off;
                break;
            }
            break;

        case EnvState::Off:
            break;
        }

        if (envState == EnvState::Off)
            break;

        // ---- Sum partials (SIMD: 4 at a time via SSE2) ----
        const float pitchFactor = pitchBendFactor;
        float signal = simd::renderPartialArray(partials.data(), numActivePartials, pitchFactor);

        // ---- Detuned string bank ----
        if (hasDetuning)
        {
            float detuned = simd::renderPartialArray(detunedPartials.data(), numActivePartials, pitchFactor);
            signal = blendDetunedSignal(signal, detuned);
        }

        // ---- Third string ----
        if (hasThirdString)
        {
            float string3Out = simd::renderPartialArray(string3Partials.data(), numActivePartials, pitchFactor);
            signal = signal * (2.0f / 3.0f) + string3Out * (1.0f / 3.0f);
        }

        // ---- Duplex resonance ----
        // FIX 2.3: Register-adaptive duplex resonance — upper register has disproportionately bright duplex
        if (numDuplexPartials > 0)
        {
            // Duplex resonance is most audible in mid-to-high register where string length is short
            const float duplexRegisterBoost = (midiNote > 60)
                ? juce::jlimit(0.5f, 2.0f, 1.0f + (static_cast<float>(midiNote) - 60.0f) / 54.0f)
                : 1.0f;
            // Duplex is more prominent when pedal is down (sympathetic excitation)
            const float duplexPedalBoost = 0.7f + pedalResonanceLift * 0.3f;
            const float duplexGain = duplexRegisterBoost * duplexPedalBoost * duplexDensityScale;
            float duplexOut = simd::renderPartialArray(duplexPartials.data(), numDuplexPartials, pitchFactor);
            signal += duplexOut * duplexGain;
        }

        // ---- Dynamic partial pruning: trim decayed trailing partials ----
        while (numActivePartials > 0 &&
               partials[static_cast<std::size_t>(numActivePartials - 1)].amplitude < kMinPartialAmplitude)
            --numActivePartials;

        if (numDuplexPartials > 0)
        {
            while (numDuplexPartials > 0 &&
                   duplexPartials[static_cast<std::size_t>(numDuplexPartials - 1)].amplitude < kMinPartialAmplitude)
                --numDuplexPartials;
        }

        // ---- Stulov hammer excitation ----
        if (stulovCompression > 0.0001f)
        {
            stulovForce = stulovStiffness * std::pow(stulovCompression, kStulovAlpha);
            // Non-linear hysteresis: some energy is lost to felt compression
            const float hysteresisLoss = 1.0f - kStulovHysteresisRatio * stulovCompression;
            signal += stulovForce * stulovGainScale * hysteresisLoss;
            stulovCompression *= stulovDecayCoeff;
        }

        // ---- Hammer noise (spectral coloring) ----
        if (hammerEnvLevel > 0.001f)
        {
            const float rawNoise = rng.nextFloat() * 2.0f - 1.0f;
            hammerFiltState = hammerFiltCoeff * hammerFiltState
                            + (1.0f - hammerFiltCoeff) * rawNoise;
            signal += hammerFiltState * hammerEnvLevel * kHammerNoiseMix;
            hammerEnvLevel *= hammerDecayCoeff;
        }

        // ---- Key click (with 0.5ms onset ramp) ----
        if (keyClickLevel > 0.0001f)
        {
            float clickGain = 1.0f;
            if (keyClickRampPos < keyClickRampLen)
            {
                clickGain = static_cast<float>(keyClickRampPos) / static_cast<float>(keyClickRampLen);
                ++keyClickRampPos;
            }
            const float clickNoise = rng.nextFloat() * 2.0f - 1.0f;
            signal += clickNoise * keyClickLevel * clickGain;
            keyClickLevel *= keyClickDecay;
        }

        // ---- Key release click (with 0.5ms onset ramp) ----
        if (keyRelClickLevel > 0.0001f)
        {
            float relClickGain = 1.0f;
            if (keyRelClickRampPos < keyRelClickRampLen)
            {
                relClickGain = static_cast<float>(keyRelClickRampPos) / static_cast<float>(keyRelClickRampLen);
                ++keyRelClickRampPos;
            }
            const float relClickNoise = rng.nextFloat() * 2.0f - 1.0f;
            signal += relClickNoise * keyRelClickLevel * relClickGain;
            keyRelClickLevel *= keyRelClickDecay;
        }

        // ---- Damper thud (with 1ms onset ramp) ----
        if (damperThudLevel > 0.0001f)
        {
            float thudGain = 1.0f;
            if (damperThudRampPos < damperThudRampLen)
            {
                thudGain = static_cast<float>(damperThudRampPos) / static_cast<float>(damperThudRampLen);
                ++damperThudRampPos;
            }
            const float rawNoise = rng.nextFloat() * 2.0f - 1.0f;
            damperThudFiltState = kDamperThudFilterCoeff * damperThudFiltState
                                + (1.0f - kDamperThudFilterCoeff) * rawNoise;
            signal += damperThudFiltState * damperThudLevel * thudGain;
            damperThudLevel *= damperThudDecay;
        }

        signal = applyDedicatedAcousticModel(signal);

        // Split direct excitation from secondary resonators so dense chords do not
        // feed already-resonant energy back into the body/sympathetic networks.
        const float resonatorInput = signal;

        // ---- 8-mode soundboard resonance ----
        // FIX 2.2: Register-adaptive resonance — bass notes accumulate too much resonance
        const float registerDamp = (midiNote < 48)
            ? 0.5f + 0.5f * (juce::jlimit(21.0f, 48.0f, static_cast<float>(midiNote)) - 21.0f) / 27.0f
            : 1.0f;
        for (int m = 0; m < kSoundboardModes; ++m)
        {
            auto& mode = sbModes[static_cast<std::size_t>(m)];
            if (mode.gain > kMinPartialAmplitude)
            {
                const float sbOut = resonatorInput + mode.coeffA * mode.state1 + mode.coeffB * mode.state2;
                mode.state2 = mode.state1;
                mode.state1 = sbOut;
                signal += sbOut * mode.gain * registerDamp * soundboardBudget;
            }
        }

        // ---- Body resonator comb (with blow-up protection) ----
        if (bodyFeedback > 0.001f)
        {
            const float delayed = readComb(bodyBuf.data(), kBodyBufSize, bodyWritePos, bodyDelaySamples);
            bodyDampState += chars.resonance.bodyDamping * kBodyDampFilterRatio * (delayed - bodyDampState);
            // Soft-clip before feedback to prevent numerical blow-up on extreme resonance
            constexpr float kBodyClip = 8.0f;
            const float bodyInput = juce::jlimit(-kBodyClip, kBodyClip,
                resonatorInput * kBodyInputGain + bodyDampState * bodyFeedback * bodyFeedbackScale);
            bodyBuf[static_cast<std::size_t>(bodyWritePos)] = bodyInput;
            bodyWritePos = (bodyWritePos + 1) % kBodyBufSize;
            signal += delayed * settings.resonance.soundboardAmount * kBodyOutputGain * bodyMixScale;
        }

        // ---- Sympathetic resonance matrix (12 coupled combs) ----
        if (sympMatrixGain > 0.001f)
        {
            // Forward pass: accumulate coupling left-to-right
            float sympFwd = 0.0f;
            float sympDelayed[kSympMatrixSize];
            for (int r = 0; r < kSympMatrixSize; ++r)
            {
                auto& res = sympMatrix[static_cast<std::size_t>(r)];
                sympDelayed[r] = readComb(res.buf.data(), kSympCombBufSize, res.writePos, res.delaySamples);
                sympFwd += sympDelayed[r];
            }
            // Reverse pass: accumulate coupling right-to-left and write back
            float sympRev = 0.0f;
            float sympSum = 0.0f;
            for (int r = kSympMatrixSize - 1; r >= 0; --r)
            {
                auto& res = sympMatrix[static_cast<std::size_t>(r)];
                res.dampState += 0.3f * (sympDelayed[r] - res.dampState);
                res.dampState *= 0.9998f;  // energy leak prevents pitch drift in chords
                // Bidirectional coupling: energy flows both directions
                // Soft-clip before feedback to prevent numerical blow-up
                constexpr float kSympClip = 8.0f;
                const float couplingInput = sympFwd + sympRev - sympDelayed[r];
                const float couplingClamped = juce::jlimit(-kSympClip, kSympClip, couplingInput);
                const float coupling = couplingClamped * kSympMatrixCoupling;
                const float sympFeedback = juce::jlimit(-kSympClip, kSympClip,
                    res.dampState * res.feedback * sympMixScale + coupling);
                res.buf[static_cast<std::size_t>(res.writePos)] = juce::jlimit(-kSympClip, kSympClip,
                    resonatorInput * kSympInputGain + sympFeedback);
                res.writePos = (res.writePos + 1) % kSympCombBufSize;
                sympRev += sympDelayed[r];
                sympSum += sympDelayed[r];
            }
            signal += sympSum * sympMatrixGain * kSympOutputGain * sympMixScale;
        }

        // ---- Character processing ----
        signal = applyCharacterProcessing(signal, settings);

        // ---- SVF lowpass with brightness envelope ----
        filterFCurrent = filterFTarget + (filterFCurrent - filterFTarget) * brightnessDecayCoeff;
        if (!std::isfinite(filterFCurrent))
            filterFCurrent = filterFTarget;
        {
            const float hp = signal - svfLow - filterQinv * svfBand;
            svfBand += filterFCurrent * hp;
            svfLow  += filterFCurrent * svfBand;
            if (!std::isfinite(svfBand) || !std::isfinite(svfLow))
            {
                svfBand = 0.0f;
                svfLow = 0.0f;
            }
            signal = svfLow;
        }

        // ---- FIX 1.3: Highpass filter (removes DC offset and excess bass) ----
        if (hpCoeff > 0.0f)
        {
            const float hp = hpCoeff * (hpState + signal - hpPrevInput);
            hpPrevInput = signal;
            hpState = std::isfinite(hp) ? hp : 0.0f;
            if (!std::isfinite(hp))
                hpPrevInput = 0.0f;
            signal = hpState;
        }

        signal *= envLevel * outputGain;

        const int idx = startSample + i;
        left[idx] += signal * panL;
        if (right != nullptr)
            right[idx] += signal * panR;

        ++ageSamples;
        if (ageSamples >= maxAgeSamples)
        {
            envState = EnvState::Off;
            break;
        }
    }
}

// =========================================================================
// ElectricPianoVoiceBase — stripped engine for electric pianos
// =========================================================================

PianoSettings ElectricPianoVoiceBase::adaptSettings(const PianoSettings& inputSettings, int, float) const
{
    return inputSettings;
}

float ElectricPianoVoiceBase::applyCharacterProcessing(float signal, const PianoSettings& activeSettings) const
{
    if (activeSettings.performance.modelCharacter <= 0.01f)
        return signal;

    const float drive = 1.0f + activeSettings.performance.modelCharacter * kCharacterElectricGain;
    return normaliseSoftClip(signal, drive);
}

PianoSettings ElectricPianoVoiceBase::applyDedicatedSettingsProfile(PianoSettings s,
                                                                    const int note,
                                                                    const float velocity) const
{
    if (!pianoDedicatedActive)
        return s;

    const float depth = pianoModelOnly ? 1.22f : 1.0f;
    switch (pianoAlgorithm)
    {
        case PianoInstrumentAlgorithm::RhodesTinePickupBark:
            s.performance.modelCharacter = clamp01(s.performance.modelCharacter + velocity * velocity * 0.045f * depth);
            s.envelope.decaySeconds = juce::jmin(8.5f, s.envelope.decaySeconds * 1.04f);
            break;

        case PianoInstrumentAlgorithm::WurlitzerReedAmpBite:
            s.performance.modelCharacter = clamp01(s.performance.modelCharacter + velocity * velocity * 0.040f * depth);
            s.tone.lowPassHz = juce::jlimit(120.0f, 14000.0f, s.tone.lowPassHz * 0.96f);
            if (note < 64)
                s.resonance.damping = clamp01(s.resonance.damping + 0.040f * depth);
            break;

        case PianoInstrumentAlgorithm::ClavinetPickupStringSnap:
            s.performance.modelCharacter = clamp01(s.performance.modelCharacter + velocity * velocity * 0.035f * depth);
            s.envelope.releaseSeconds = juce::jmin(s.envelope.releaseSeconds, 0.115f);
            s.envelope.decaySeconds = juce::jmax(0.22f, s.envelope.decaySeconds * 0.94f);
            break;

        default:
            break;
    }

    return s;
}

void ElectricPianoVoiceBase::initialiseDedicatedModel(const float f0, const float fsr) noexcept
{
    dedicatedPhaseA = rng.nextFloat();
    dedicatedPhaseB = rng.nextFloat();
    dedicatedPhaseIncA = 0.0f;
    dedicatedPhaseIncB = 0.0f;
    dedicatedEnvA = 0.0f;
    dedicatedEnvB = 0.0f;
    dedicatedDecayA = 1.0f;
    dedicatedDecayB = 1.0f;
    dedicatedNoiseEnv = 0.0f;
    dedicatedNoiseDecay = 1.0f;
    dedicatedFilterState = 0.0f;

    if (!pianoDedicatedActive || fsr <= 1.0f)
        return;

    const float modelScale = pianoModelOnly ? 1.25f : 1.0f;
    auto setOsc = [f0, fsr](float& phaseInc, float ratio)
    {
        phaseInc = juce::jlimit(0.0f, 0.475f, f0 * ratio / fsr);
    };
    auto setDecay = [fsr](const float seconds)
    {
        return std::exp(-1.0f / (juce::jmax(0.004f, seconds) * fsr));
    };

    switch (pianoAlgorithm)
    {
        case PianoInstrumentAlgorithm::RhodesTinePickupBark:
            setOsc(dedicatedPhaseIncA, 2.01f);
            setOsc(dedicatedPhaseIncB, 6.17f);
            dedicatedEnvA = 0.014f * modelScale * vel;
            dedicatedEnvB = 0.002f * modelScale * settings.performance.modelCharacter * vel;
            dedicatedDecayA = setDecay(0.36f);
            dedicatedDecayB = setDecay(0.16f);
            dedicatedNoiseEnv = 0.014f * modelScale * vel;
            dedicatedNoiseDecay = setDecay(0.012f);
            break;

        case PianoInstrumentAlgorithm::WurlitzerReedAmpBite:
            setOsc(dedicatedPhaseIncA, 1.0f);
            setOsc(dedicatedPhaseIncB, 3.02f);
            dedicatedEnvA = 0.030f * modelScale * (0.4f + vel);
            dedicatedEnvB = 0.016f * modelScale * settings.performance.modelCharacter;
            dedicatedDecayA = setDecay(0.22f);
            dedicatedDecayB = setDecay(0.12f);
            dedicatedNoiseEnv = 0.020f * modelScale * vel;
            dedicatedNoiseDecay = setDecay(0.018f);
            break;

        case PianoInstrumentAlgorithm::ClavinetPickupStringSnap:
            setOsc(dedicatedPhaseIncA, 3.0f);
            setOsc(dedicatedPhaseIncB, 5.0f);
            dedicatedEnvA = 0.034f * modelScale * vel;
            dedicatedEnvB = 0.018f * modelScale * settings.performance.modelCharacter;
            dedicatedDecayA = setDecay(0.11f);
            dedicatedDecayB = setDecay(0.070f);
            dedicatedNoiseEnv = 0.030f * modelScale * vel;
            dedicatedNoiseDecay = setDecay(0.010f);
            break;

        default:
            break;
    }
}

float ElectricPianoVoiceBase::applyDedicatedElectricModel(float signal) noexcept
{
    if (!pianoDedicatedActive)
        return signal;

    auto osc = [this](float& phase, const float phaseInc, float& env, const float decay) noexcept
    {
        if (env <= 1.0e-6f || phaseInc <= 0.0f)
            return 0.0f;

        const float value = fastSin(phase) * env;
        phase += phaseInc * pitchBendFactor;
        phase -= std::floor(phase);
        env *= decay;
        return value;
    };

    const float a = osc(dedicatedPhaseA, dedicatedPhaseIncA, dedicatedEnvA, dedicatedDecayA);
    const float b = osc(dedicatedPhaseB, dedicatedPhaseIncB, dedicatedEnvB, dedicatedDecayB);
    float noise = 0.0f;
    if (dedicatedNoiseEnv > 1.0e-6f)
    {
        const float raw = rng.nextFloat() * 2.0f - 1.0f;
        dedicatedFilterState += 0.35f * (raw - dedicatedFilterState);
        noise = (raw - dedicatedFilterState) * dedicatedNoiseEnv;
        dedicatedNoiseEnv *= dedicatedNoiseDecay;
    }

    switch (pianoAlgorithm)
    {
        case PianoInstrumentAlgorithm::RhodesTinePickupBark:
            signal += a * 0.80f + b * 0.55f + noise * 0.55f;
            signal += signal * std::abs(signal) * 0.045f * (0.4f + settings.performance.modelCharacter);
            break;

        case PianoInstrumentAlgorithm::WurlitzerReedAmpBite:
        {
            const float bite = signal - dedicatedFilterState;
            signal += a * 0.72f + b * 0.60f + noise * 0.70f + bite * 0.045f;
            signal = signal * 0.80f + std::tanh(signal * 1.35f) * 0.20f;
            break;
        }

        case PianoInstrumentAlgorithm::ClavinetPickupStringSnap:
        {
            const float quack = signal - dedicatedFilterState;
            signal += a * 0.86f + b * 0.72f + noise * 0.95f + quack * 0.065f;
            signal = signal * 0.92f + std::tanh(signal * 2.4f) * 0.08f;
            break;
        }

        default:
            break;
    }

    return std::isfinite(signal) ? signal : 0.0f;
}

void ElectricPianoVoiceBase::updateRealtimeEnvelope() noexcept
{
    const auto fsr = static_cast<float>(sr);
    if (fsr <= 0.0f)
        return;

    const float attackSeconds = settings.envelope.attackSeconds * realtimeModulation.attackScale;
    attackRate = (attackSeconds > 0.0001f)
        ? 1.0f / (attackSeconds * fsr)
        : 1.0f;

    const float decaySeconds = settings.envelope.decaySeconds * realtimeModulation.decayScale;
    const float decay1Time = std::max(0.01f, decaySeconds * chars.envelope.decay1Ratio);
    decay1Coeff = std::exp(-1.0f / (decay1Time * fsr));
    decay1Target = chars.envelope.sustainPlatform * settings.envelope.sustainLevel *
        (chars.string.hasDampers ? 1.0f : 0.5f);

    const float decay2Time = std::max(0.05f, decaySeconds * chars.envelope.decay2Ratio);
    decay2Coeff = std::exp(-1.0f / (decay2Time * fsr));
}

void ElectricPianoVoiceBase::updateRealtimeFilter() noexcept
{
    const auto fsr = static_cast<float>(sr);
    if (fsr <= 0.0f)
        return;

    const float baseQ = juce::jmax(0.5f, 0.7f + settings.tone.brightness * 0.5f);
    const float modQ = juce::jlimit(0.5f, 2.5f, baseQ + realtimeModulation.resonanceOffset);
    filterQinv = 1.0f / modQ;

    filterF = computeSafeSvfCoefficient(settings.tone.lowPassHz * realtimeModulation.cutoffMul, fsr, filterQinv);
    filterFTarget = filterF;
    const float maxAllowedF = computeSafeSvfCoefficient(fsr * 0.45f, fsr, filterQinv);
    filterFCurrent = std::isfinite(filterFCurrent) && filterFCurrent > 0.0f ? filterFCurrent : filterFTarget;
    filterFCurrent = juce::jlimit(0.0f, maxAllowedF, filterFCurrent);
}

void ElectricPianoVoiceBase::updateMaxAgeSamples() noexcept
{
    const float decaySeconds = settings.envelope.decaySeconds * realtimeModulation.decayScale;
    maxAgeSamples = static_cast<int>(sr * std::max(1.0f,
        decaySeconds * 6.0f + settings.envelope.releaseSeconds * 3.0f));
    if (maxAgeSamples > static_cast<int>(sr * static_cast<double>(kMaxVoiceAgeSec)))
        maxAgeSamples = static_cast<int>(sr * static_cast<double>(kMaxVoiceAgeSec));
}

void ElectricPianoVoiceBase::applyRealtimeModulation(const VoiceRealtimeModulation& modulation) noexcept
{
    realtimeModulation.cutoffMul = juce::jlimit(0.0625f, 16.0f, modulation.cutoffMul);
    realtimeModulation.resonanceOffset = juce::jlimit(-1.0f, 1.0f, modulation.resonanceOffset);
    realtimeModulation.attackScale = juce::jlimit(0.0625f, 16.0f, modulation.attackScale);
    realtimeModulation.decayScale = juce::jlimit(0.0625f, 16.0f, modulation.decayScale);
    realtimeModulation.densityResonanceScale = juce::jlimit(0.35f, 1.0f, modulation.densityResonanceScale);
    updateRealtimeEnvelope();
    updateRealtimeFilter();
    updateMaxAgeSamples();
}

void ElectricPianoVoiceBase::noteOn(const PianoSettings& inputSettings,
                                     const int note, const float velocity,
                                     const double sampleRate)
{
    rng.setSeed(hasPendingRandomSeed ? pendingRandomSeed : makeSystemRandomSeed());
    hasPendingRandomSeed = false;

    pianoIndex = juce::jlimit(0, kNumPianos - 1, getPianoIndex());
    const auto& model = getPianoInstrumentModel(pianoIndex);
    pianoAlgorithm = model.algorithm;
    const auto renderMode = getPianoRenderEngineMode();
    pianoDedicatedActive = model.readiness == PianoEngineReadiness::DedicatedVoice
        && renderMode != PianoRenderEngineMode::LegacyFamily;
    pianoModelOnly = renderMode == PianoRenderEngineMode::V2ModelOnly;

    settings = adaptSettings(inputSettings, note, velocity);
    settings = applyDedicatedSettingsProfile(settings, note, velocity);
    chars = getCharacteristics();
    sr = std::max(1.0, sampleRate);
    vel = juce::jlimit(0.0f, 1.0f, velocity);
    midiNote = note;
    ageSamples = 0;
    realtimeModulation = {};

    const auto fsr = static_cast<float>(sr);

    const float f0 = 440.0f * std::pow(2.0f,
        (static_cast<float>(note) - 69.0f + settings.tuneSemitones) / 12.0f);

    // Envelope (shared by all electric pianos)
    attackRate = (settings.envelope.attackSeconds > 0.0001f)
        ? 1.0f / (settings.envelope.attackSeconds * fsr)
        : 1.0f;

    const float decay1Time = std::max(0.01f, settings.envelope.decaySeconds * chars.envelope.decay1Ratio);
    decay1Coeff = std::exp(-1.0f / (decay1Time * fsr));
    decay1Target = chars.envelope.sustainPlatform * settings.envelope.sustainLevel *
        (chars.string.hasDampers ? 1.0f : 0.5f);

    const float decay2Time = std::max(0.05f, settings.envelope.decaySeconds * chars.envelope.decay2Ratio);
    decay2Coeff = std::exp(-1.0f / (decay2Time * fsr));

    releaseCoeff = std::exp(-1.0f / (std::max(0.005f, settings.envelope.releaseSeconds) * fsr));

    envLevel = 0.0f;
    envState = (settings.envelope.attackSeconds > 0.0001f) ? EnvState::Attack : EnvState::Decay1;
    if (envState == EnvState::Decay1)
        envLevel = 1.0f;

    // SVF filter (shared)
    filterQinv = 1.0f / juce::jmax(0.5f, 0.7f + settings.tone.brightness * 0.5f);
    filterF = computeSafeSvfCoefficient(settings.tone.lowPassHz, fsr, filterQinv);
    svfLow = 0.0f;
    svfBand = 0.0f;

    filterFTarget = filterF;
    {
        const float boost = vel * vel * settings.tone.hammerHardness * 2.5f;
        filterFCurrent = computeSafeSvfCoefficient(settings.tone.lowPassHz * (1.0f + boost), fsr, filterQinv);
        // Register-dependent brightness decay: bass strings ring bright longer
        const float registerNorm = juce::jlimit(0.0f, 1.0f, (static_cast<float>(midiNote) - 21.0f) / 87.0f);
        const float brightDecayTime = (0.30f + vel * 0.30f) * (1.0f + (1.0f - registerNorm) * 1.5f);
        brightnessDecayCoeff = std::exp(-1.0f / (brightDecayTime * fsr));
    }

    // Pan (shared)
    const float registerPan = (static_cast<float>(midiNote - 64) / 87.0f) * kRegisterPanSpread;
    const float totalPan = juce::jlimit(-1.0f, 1.0f, settings.spatial.pan + registerPan);
    panL = std::sqrt(0.5f * (1.0f - totalPan));
    panR = std::sqrt(0.5f * (1.0f + totalPan));

    updateMaxAgeSamples();

    // Instrument-specific initialisation
    onNoteOn(f0, fsr);
    initialiseDedicatedModel(f0, fsr);
}

void ElectricPianoVoiceBase::forceQuickRelease() noexcept
{
    if (envState == EnvState::Off)
        return;
    envState = EnvState::Release;
    const auto releaseSamples = std::max(1.0f, static_cast<float>(std::max(1.0, sr)) * 0.005f);
    releaseCoeff = std::exp(std::log(0.001f) / releaseSamples);
}

void ElectricPianoVoiceBase::noteOff()
{
    if (envState != EnvState::Off && envState != EnvState::Release)
    {
        onNoteOff();
        envState = EnvState::Release;
    }
}

// Default onNoteOn: generic additive partial setup + hammer excitation
void ElectricPianoVoiceBase::onNoteOn(const float f0, const float fsr)
{
    const float effectiveB = getScaledInharmonicityB(midiNote, chars.string);

    const int registerLimited = (midiNote <= 72)
        ? chars.string.partialCount
        : std::max(4, chars.string.partialCount - (midiNote - 72) / 3);
    const int np = std::min(registerLimited, kElecMaxPartials);
    numActivePartials = np;

    const float effectiveHardness = chars.excitation.hammerHardnessBase *
        (kHammerHardnessBaseBlend + kHammerHardnessVelBlend * settings.tone.hammerHardness);
    const float hammerCutoff = f0 * (kHammerCutoffBase + effectiveHardness * kHammerCutoffRange *
        (kHammerCutoffVelBase + kHammerCutoffVelRange * vel));
    const float brightMult = kBrightnessMultMin + settings.tone.brightness * kBrightnessMultRange;

    for (int n = 0; n < np; ++n)
    {
        const int harmonic = n + 1;
        const float fn = static_cast<float>(harmonic) * f0 *
            std::sqrt(1.0f + effectiveB * static_cast<float>(harmonic * harmonic));

        if (fn >= fsr * 0.48f)
        {
            numActivePartials = std::max(1, n);
            break;
        }

        const float baseAmp = 1.0f / static_cast<float>(harmonic);
        const float freqRatio = fn / (hammerCutoff * brightMult);
        const float hammerFilter = 1.0f / (1.0f + freqRatio * freqRatio);
        const float amp = baseAmp * (kPartialBaseAmpFloor + kPartialHammerFilterMix * hammerFilter);

        const float partialDecayScale = 1.0f /
            (1.0f + settings.resonance.damping * static_cast<float>(harmonic - 1) * 0.25f);
        const float partialDecayTime = std::max(0.02f,
            settings.envelope.decaySeconds * chars.envelope.decay2Ratio * partialDecayScale);
        const float decayCoeff = std::exp(-1.0f / (partialDecayTime * fsr));

        auto& partial = partials[static_cast<std::size_t>(n)];
        partial.phase = 0.0f;
        partial.phaseInc = fn / fsr;
        partial.amplitude = amp;
        partial.decayCoeff = decayCoeff;
    }

    // Hammer excitation
    const float effectiveHardness2 = chars.excitation.hammerHardnessBase *
        (kHammerHardnessBaseBlend + kHammerHardnessVelBlend * settings.tone.hammerHardness);
    hammerEnvLevel = vel * (1.0f + effectiveHardness2 * 2.0f);
    hammerDecayCoeff = std::exp(-1.0f / (kHammerNoiseDecaySec * fsr));
    {
        const float hammerNorm = juce::jlimit(0.0f, 1.0f,
            (static_cast<float>(midiNote) - 21.0f) / 67.0f);
        hammerFiltCoeff = 0.97f - hammerNorm * 0.55f;
        hammerFiltState = 0.0f;
    }
}

// Default onNoteOff: no instrument-specific behavior
void ElectricPianoVoiceBase::onNoteOff()
{
}

// Default generateSample: additive partial sum + hammer noise
float ElectricPianoVoiceBase::generateSample(const float pitchFactor)
{
    float signal = simd::renderPartialArray(partials.data(), numActivePartials, pitchFactor);

    // Hammer excitation noise
    if (hammerEnvLevel > 0.001f)
    {
        const float rawNoise = rng.nextFloat() * 2.0f - 1.0f;
        hammerFiltState = hammerFiltCoeff * hammerFiltState
                        + (1.0f - hammerFiltCoeff) * rawNoise;
        signal += hammerFiltState * hammerEnvLevel * kHammerNoiseMix;
        hammerEnvLevel *= hammerDecayCoeff;
    }

    return signal;
}

void ElectricPianoVoiceBase::render(juce::AudioBuffer<float>& buffer,
                                     const int startSample, const int numSamples)
{
    if (envState == EnvState::Off)
        return;

    const int numChannels = buffer.getNumChannels();
    if (numChannels <= 0)
        return;

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    // Hoist loop-invariant multiplies
    const float outputGain = vel * settings.level;
    const float releaseExtraDamping = juce::jlimit(0.90f, 1.0f,
        1.0f - renderContext.repeatedNotePressure * 0.045f
             - renderContext.harmonicCollision * 0.035f
             - (1.0f - juce::jlimit(0.15f, 1.0f, renderContext.tailOwnership)) * 0.055f);
    const float releaseContextCoeff = contextReleaseDampingCoefficient(releaseExtraDamping, sr);

    for (int i = 0; i < numSamples; ++i)
    {
        if (envState == EnvState::Off)
            break;

        switch (envState)
        {
        case EnvState::Attack:
            envLevel += attackRate;
            if (envLevel >= 1.0f)
            {
                envLevel = 1.0f;
                envState = EnvState::Decay1;
            }
            break;

        case EnvState::Decay1:
            envLevel = decay1Target + (envLevel - decay1Target) * decay1Coeff;
            if (envLevel <= decay1Target + 0.002f)
            {
                envLevel = decay1Target;
                envState = EnvState::Decay2;
            }
            break;

        case EnvState::Decay2:
            envLevel *= decay2Coeff;
            if (envLevel < 0.0001f)
            {
                envLevel = 0.0f;
                envState = EnvState::Off;
                break;
            }
            break;

        case EnvState::Release:
            envLevel *= releaseCoeff * releaseContextCoeff;
            if (envLevel < 0.0001f)
            {
                envLevel = 0.0f;
                envState = EnvState::Off;
                break;
            }
            break;

        case EnvState::Off:
            break;
        }

        if (envState == EnvState::Off)
            break;

        // Instrument-specific signal generation
        const float pitchFactor = pitchBendFactor;
        float signal = generateSample(pitchFactor);

        signal = applyDedicatedElectricModel(signal);

        // Pickup / character processing
        signal = applyCharacterProcessing(signal, settings);

        // SVF lowpass with brightness envelope
        filterFCurrent = filterFTarget + (filterFCurrent - filterFTarget) * brightnessDecayCoeff;
        if (!std::isfinite(filterFCurrent))
            filterFCurrent = filterFTarget;
        {
            const float hp = signal - svfLow - filterQinv * svfBand;
            svfBand += filterFCurrent * hp;
            svfLow  += filterFCurrent * svfBand;
            if (!std::isfinite(svfBand) || !std::isfinite(svfLow))
            {
                svfBand = 0.0f;
                svfLow = 0.0f;
            }
            signal = svfLow;
        }

        const float densityTrim = juce::jlimit(0.82f, 1.0f,
            1.0f - renderContext.density * 0.055f - renderContext.repeatedNotePressure * 0.045f);
        signal *= envLevel * outputGain * densityTrim;

        const int idx = startSample + i;
        left[idx] += signal * panL;
        if (right != nullptr)
            right[idx] += signal * panR;

        ++ageSamples;
        if (ageSamples >= maxAgeSamples)
        {
            envState = EnvState::Off;
            break;
        }
    }
}

// =========================================================================
// Acoustic piano voice subclasses
// =========================================================================

PianoCharacteristics SteinwayVoice::getCharacteristics() const
{
    return mps::getCharacteristics(0);
}

PianoSettings SteinwayVoice::adaptSettings(const PianoSettings& inputSettings, const int note, const float velocity) const
{
    auto s = inputSettings;
    // Progressive brightness curve across registers (not just a step at 72)
    const float registerNorm = juce::jlimit(0.0f, 1.0f, (static_cast<float>(note) - 40.0f) / 48.0f);
    s.tone.brightness = juce::jlimit(0.0f, 1.0f, s.tone.brightness + registerNorm * 0.055f + velocity * 0.028f);
    // Rich soundboard for the concert grand
    s.resonance.soundboardAmount = juce::jlimit(0.0f, 1.0f, s.resonance.soundboardAmount + 0.035f);
    s.resonance.stringResonance = juce::jlimit(0.0f, 1.0f, s.resonance.stringResonance + 0.020f);
    // Velocity-dependent cutoff: bright hammer strike on forte, warm on piano
    s.tone.lowPassHz = juce::jlimit(120.0f, 18000.0f, s.tone.lowPassHz * (0.92f + velocity * 0.18f));
    return s;
}

PianoCharacteristics BosendorferVoice::getCharacteristics() const
{
    return mps::getCharacteristics(1);
}

PianoSettings BosendorferVoice::adaptSettings(const PianoSettings& inputSettings, const int note, const float velocity) const
{
    auto s = inputSettings;
    // Deep bass enhancement — characteristic Bösendorfer Imperial warmth
    if (note < 60)
    {
        const float bassDepth = juce::jlimit(0.0f, 1.0f, (60.0f - static_cast<float>(note)) / 39.0f);
        s.resonance.soundboardAmount = juce::jlimit(0.0f, 1.0f, s.resonance.soundboardAmount + 0.060f + bassDepth * 0.105f);
        s.resonance.stringResonance = juce::jlimit(0.0f, 1.0f, s.resonance.stringResonance + 0.025f + bassDepth * 0.080f);
        // Extended sustain for lower bass — long singing quality
        s.envelope.decaySeconds = std::min(12.0f, s.envelope.decaySeconds * (1.0f + bassDepth * 0.38f));
    }
    // Slightly warmer overall — lower cutoff for the characteristically dark Bösendorfer tone
    s.tone.hammerHardness = juce::jlimit(0.0f, 1.0f, s.tone.hammerHardness - 0.025f);
    s.tone.brightness = juce::jlimit(0.0f, 1.0f, s.tone.brightness - 0.030f - velocity * 0.010f);
    s.tone.lowPassHz = std::max(120.0f, s.tone.lowPassHz * 0.90f);
    // More damping in high register to avoid brittleness
    if (note > 72)
        s.resonance.damping = juce::jlimit(0.0f, 1.0f, s.resonance.damping + 0.04f);
    return s;
}

bool BosendorferVoice::shouldUseThirdString(const PianoCharacteristics& characteristics, const int note) const
{
    return !characteristics.output.isElectric
        && characteristics.string.unisonDetuning < 0.05f
        && note > 50;
}

PianoCharacteristics YamahaVoice::getCharacteristics() const
{
    return mps::getCharacteristics(2);
}

PianoSettings YamahaVoice::adaptSettings(const PianoSettings& inputSettings, const int note, const float velocity) const
{
    auto s = inputSettings;
    // Non-linear hammer hardness: more pronounced in treble
    const float registerFactor = juce::jlimit(0.0f, 1.0f, (static_cast<float>(note) - 36.0f) / 52.0f);
    s.tone.hammerHardness = juce::jlimit(0.0f, 1.0f, s.tone.hammerHardness + 0.050f + registerFactor * 0.060f);
    // Velocity-dependent brightness for articulated attack 
    s.tone.brightness = juce::jlimit(0.0f, 1.0f, s.tone.brightness + 0.025f + velocity * 0.070f);
    s.resonance.soundboardAmount = juce::jlimit(0.0f, 1.0f, s.resonance.soundboardAmount - 0.030f);
    s.resonance.damping = juce::jlimit(0.0f, 1.0f, s.resonance.damping + 0.052f);
    // Tighter decay in medium register for "clean" CFX character
    if (note >= 48 && note <= 84)
        s.envelope.decaySeconds = std::max(0.1f, s.envelope.decaySeconds * 0.80f);
    // Slightly open cutoff — modern bright sound
    s.tone.lowPassHz = juce::jlimit(120.0f, 18000.0f, s.tone.lowPassHz * 1.08f);
    return s;
}

PianoCharacteristics BastringueVoice::getCharacteristics() const
{
    return mps::getCharacteristics(3);
}

PianoSettings BastringueVoice::adaptSettings(const PianoSettings& inputSettings, const int note, const float velocity) const
{
    auto s = inputSettings;
    // Exaggerated character for honky-tonk flavor
    s.performance.modelCharacter = juce::jlimit(0.0f, 1.0f, s.performance.modelCharacter + 0.10f);
    // Less soundboard — old worn piano body
    s.resonance.soundboardAmount = juce::jlimit(0.0f, 1.0f, s.resonance.soundboardAmount - 0.04f);
    // Increased damping in treble — worn strings lose brilliance
    if (note > 60)
    {
        const float wearFactor = juce::jlimit(0.0f, 1.0f, (static_cast<float>(note) - 60.0f) / 28.0f);
        s.resonance.damping = juce::jlimit(0.0f, 1.0f, s.resonance.damping + wearFactor * 0.12f);
    }
    // Velocity makes the detuning more prominent — harder playing = more honky-tonk
    s.performance.modelCharacter = juce::jlimit(0.0f, 1.0f, s.performance.modelCharacter + velocity * 0.06f);
    return s;
}

bool BastringueVoice::shouldUseThirdString(const PianoCharacteristics&, const int) const
{
    return false;
}

float BastringueVoice::blendDetunedSignal(const float mainSignal, const float detunedSignal) const
{
    return mainSignal * 0.48f + detunedSignal * 0.52f;
}

PianoCharacteristics PreparedPianoVoice::getCharacteristics() const
{
    return mps::getCharacteristics(4);
}

PianoSettings PreparedPianoVoice::adaptSettings(const PianoSettings& inputSettings, const int note, const float velocity) const
{
    auto s = inputSettings;
    // Damped string body with short object contacts, still velocity sensitive.
    s.resonance.damping = clamp01(s.resonance.damping + 0.06f);
    s.tone.hammerHardness = clamp01(s.tone.hammerHardness + velocity * 0.025f);
    const float treble = clamp01((static_cast<float>(note) - 60.0f) / 28.0f);
    s.tone.brightness = clamp01(s.tone.brightness * (0.88f - treble * 0.10f));
    s.resonance.stringResonance = clamp01(s.resonance.stringResonance * 0.80f);
    return s;
}

bool PreparedPianoVoice::shouldUseThirdString(const PianoCharacteristics&, const int) const
{
    return false;
}

PianoCharacteristics RhodesVoice::getCharacteristics() const
{
    return mps::getCharacteristics(5);
}

PianoSettings RhodesVoice::adaptSettings(const PianoSettings& inputSettings, const int note, const float velocity) const
{
    auto s = inputSettings;
    // Velocity-dependent bark: forte playing drives the pickup harder
    s.performance.modelCharacter = juce::jlimit(0.0f, 1.0f, s.performance.modelCharacter + velocity * velocity * 0.10f);
    // Bell-like brightness in treble — characteristic Rhodes tine sound
    if (note > 64)
    {
        const float bellFactor = juce::jlimit(0.0f, 1.0f, (static_cast<float>(note) - 64.0f) / 24.0f);
        s.tone.brightness = juce::jlimit(0.0f, 1.0f, s.tone.brightness + bellFactor * 0.045f);
        s.tone.lowPassHz = juce::jmin(s.tone.lowPassHz, 12800.0f - bellFactor * 1200.0f);
    }
    // Warm bass — reduce hammer hardness for rounder low end
    if (note < 52)
        s.tone.hammerHardness = juce::jlimit(0.0f, 1.0f, s.tone.hammerHardness - 0.06f);
    return s;
}

void RhodesVoice::onNoteOn(const float f0, const float fsr)
{
    // --- Tine oscillator: fast-decaying, bell-like attack ---
    tinePhase = 0.0f;
    tinePhaseInc = f0 / fsr;
    const float bassWeight = juce::jlimit(0.0f, 1.0f, (52.0f - static_cast<float>(midiNote)) / 24.0f);
    tineAmp = (0.7f + vel * 0.3f) * (1.0f - bassWeight * 0.20f);
    const float registerNorm = juce::jlimit(0.0f, 1.0f,
        (static_cast<float>(midiNote) - 40.0f) / 48.0f);
    const float tineDecayTime = kRhodesTineDecayBase * (1.0f - registerNorm * 0.5f)
        * settings.envelope.decaySeconds / 4.0f;
    tineDecayCoeff = std::exp(-1.0f / (std::max(0.05f, tineDecayTime) * fsr));

    // --- Tonebar oscillator: slow-decaying warmth, slightly detuned ---
    tonebarPhase = 0.0f;
    tonebarPhaseInc = (f0 + kRhodesTonebarDetuneHz) / fsr;
    tonebarAmp = (0.42f + vel * 0.12f) * (1.0f - bassWeight * 0.35f);
    const float tonebarDecayTime = kRhodesTonebarDecayBase
        * settings.envelope.decaySeconds / 4.0f;
    tonebarDecayCoeff = std::exp(-1.0f / (std::max(0.1f, tonebarDecayTime) * fsr));

    // Coupling state
    couplingState = 0.0f;

    // --- Bell partials: metallic shimmer on attack ---
    // Measured Rhodes Mark I bell mode ratios
    constexpr float kBellRatios[4] = { 2.0f, 2.76f, 4.07f, 5.38f };
    for (int b = 0; b < kRhodesBellPartials; ++b)
    {
        const float ratio = kBellRatios[b];
        const float bellFreq = f0 * ratio;
        if (bellFreq >= fsr * 0.45f)
        {
            bellPartials[static_cast<std::size_t>(b)] = { 0.0f, 0.0f, 0.0f, 1.0f };
            continue;
        }
        const float bellAmp = kRhodesBellAmpBase * vel
            / static_cast<float>(b + 1);
        // Bell partials have tighter high-register roll-off
        const float bellRolloff = (bellFreq > 4000.0f)
            ? 4000.0f / bellFreq : 1.0f;
        const float registerBellCap = juce::jlimit(0.58f, 1.0f,
            1.0f - juce::jmax(0.0f, static_cast<float>(midiNote) - 64.0f) / 72.0f);
        const float bellDecayTime = tineDecayTime * kRhodesBellDecayFactor
            / static_cast<float>(b + 1);
        bellPartials[static_cast<std::size_t>(b)] = {
            0.0f,
            bellFreq / fsr,
            bellAmp * bellRolloff * registerBellCap,
            std::exp(-1.0f / (std::max(0.01f, bellDecayTime) * fsr))
        };
    }

    // --- Suitcase tremolo LFO ---
    tremoloPhase = 0.0f;
    tremoloPhaseInc = (settings.performance.tremoloRateHz > 0.0f
                       ? settings.performance.tremoloRateHz : kRhodesTremoloRateHz) / fsr;
    tremoloDepth = kRhodesTremoloDepth * settings.performance.modelCharacter;

    // Hammer excitation (brief tine strike noise)
    const float effectiveHardness = chars.excitation.hammerHardnessBase *
        (kHammerHardnessBaseBlend + kHammerHardnessVelBlend * settings.tone.hammerHardness);
    hammerEnvLevel = vel * (1.0f + effectiveHardness * 1.5f);
    hammerDecayCoeff = std::exp(-1.0f / (kHammerNoiseDecaySec * 0.8f * fsr));
    {
        const float hammerNorm = juce::jlimit(0.0f, 1.0f,
            (static_cast<float>(midiNote) - 21.0f) / 67.0f);
        hammerFiltCoeff = 0.92f - hammerNorm * 0.70f;
        hammerFiltState = 0.0f;
    }
}

float RhodesVoice::generateSample(const float pitchFactor)
{
    // --- Tine oscillator ---
    float tineSig = tineAmp * fastSin(tinePhase);
    tinePhase += tinePhaseInc * pitchFactor;
    tinePhase -= std::floor(tinePhase);
    tineAmp *= tineDecayCoeff;

    // --- Tonebar oscillator ---
    float tonebarSig = tonebarAmp * fastSin(tonebarPhase);
    tonebarPhase += tonebarPhaseInc * pitchFactor;
    tonebarPhase -= std::floor(tonebarPhase);
    tonebarAmp *= tonebarDecayCoeff;

    // --- Non-linear coupling: tine drives tonebar ---
    // The tine's vibration energy transfers to the tonebar with a non-linear
    // (cubic) coupling term, creating subtle intermodulation
    const float couplingInput = tineSig * tineSig * tineSig * kRhodesCouplingCoeff;
    couplingState = couplingState * 0.99f + couplingInput * 0.01f;
    tonebarSig += couplingState;

    // --- Bell partials (metallic shimmer, SIMD 4-wide) ---
    float bellSig = simd::renderPartialArray(bellPartials.data(), kRhodesBellPartials, pitchFactor);

    // Mix tine (bright attack) + tonebar (warm sustain) + bell shimmer
    float signal = tineSig * 0.66f + tonebarSig * 0.32f + bellSig;

    // --- Hammer strike noise ---
    if (hammerEnvLevel > 0.001f)
    {
        const float rawNoise = rng.nextFloat() * 2.0f - 1.0f;
        hammerFiltState = hammerFiltCoeff * hammerFiltState
                        + (1.0f - hammerFiltCoeff) * rawNoise;
        signal += hammerFiltState * hammerEnvLevel * kHammerNoiseMix * 0.7f;
        hammerEnvLevel *= hammerDecayCoeff;
    }

    // --- Suitcase tremolo (amplitude + stereo modulation) ---
    if (tremoloDepth > 0.001f)
    {
        const float lfo = fastSin(tremoloPhase);
        tremoloPhase += tremoloPhaseInc;
        tremoloPhase -= std::floor(tremoloPhase);
        // Amplitude modulation
        signal *= 1.0f - tremoloDepth * 0.5f * (1.0f - lfo);
        // Stereo pan modulation (applied to base panL/panR)
        const float panMod = lfo * kRhodesTremoloPanDepth * tremoloDepth;
        panL = std::sqrt(0.5f * (1.0f - panMod));
        panR = std::sqrt(0.5f * (1.0f + panMod));
    }

    return signal;
}

float RhodesVoice::applyCharacterProcessing(float signal, const PianoSettings& activeSettings) const
{
    if (activeSettings.performance.modelCharacter <= 0.01f)
        return signal;

    // Asymmetric pickup: signal + k*signal² creates even harmonics
    const float k = kRhodesPickupAsymmetry * activeSettings.performance.modelCharacter;
    signal = signal + k * signal * signal;

    // Soft overdrive from pickup saturation on forte playing
    const float drive = 1.0f + activeSettings.performance.modelCharacter * (0.45f + vel * vel * 0.75f);
    const float shaped = std::tanh(signal * drive);
    const float clipped = shaped / std::max(0.01f, std::tanh(drive));
    return signal * 0.60f + clipped * 0.40f;
}

PianoCharacteristics WurlitzerVoice::getCharacteristics() const
{
    return mps::getCharacteristics(6);
}

PianoSettings WurlitzerVoice::adaptSettings(const PianoSettings& inputSettings, const int note, const float velocity) const
{
    auto s = inputSettings;
    // Velocity-dependent hammer hardness for percussive attack
    s.tone.hammerHardness = juce::jlimit(0.0f, 1.0f, s.tone.hammerHardness + 0.03f + velocity * 0.04f);
    s.tone.brightness = juce::jmin(s.tone.brightness, 0.54f);
    s.tone.lowPassHz = juce::jmin(s.tone.lowPassHz, 9500.0f);
    // Growl in mid-bass register — characteristic Wurlitzer reed bark
    if (note < 60)
    {
        const float growlFactor = juce::jlimit(0.0f, 1.0f, (60.0f - static_cast<float>(note)) / 24.0f);
        s.performance.modelCharacter = juce::jlimit(0.0f, 1.0f, s.performance.modelCharacter + growlFactor * 0.08f + velocity * 0.05f);
    }
    // Treble Wurlitzer can become fatiguing; cap brightness/lowpass as velocity rises.
    if (note > 72)
    {
        const float highFactor = juce::jlimit(0.0f, 1.0f, (static_cast<float>(note) - 72.0f) / 24.0f);
        s.tone.brightness = juce::jmin(0.58f, s.tone.brightness + highFactor * 0.01f);
        s.tone.lowPassHz = juce::jmin(s.tone.lowPassHz, 11200.0f - highFactor * 1800.0f - velocity * 900.0f);
        s.tone.hammerHardness = juce::jlimit(0.0f, 1.0f, s.tone.hammerHardness - highFactor * 0.03f);
    }
    return s;
}

void WurlitzerVoice::onNoteOn(const float f0, const float fsr)
{
    // --- FM carrier at f0 ---
    carrierPhase = 0.0f;
    carrierPhaseInc = f0 / fsr;

    // --- FM modulator (ratio:1 = same frequency → rich harmonics) ---
    modulatorPhase = 0.0f;
    modulatorPhaseInc = f0 * kWurliModRatio / fsr;

    // --- FM index: high on attack → decays to cleaner sustain ---
    const float highRegisterTrim = midiNote > 72
        ? juce::jlimit(0.72f, 1.0f, 1.0f - (static_cast<float>(midiNote) - 72.0f) / 96.0f)
        : 1.0f;
    fmBaseIndex = (kWurliFMIndexBase + vel * kWurliFMIndexVelScale) * highRegisterTrim;
    fmIndex = fmBaseIndex;
    const float fmDecayTime = settings.envelope.decaySeconds * kWurliModDecayFactor;
    fmIndexDecayCoeff = std::exp(-1.0f / (std::max(0.05f, fmDecayTime) * fsr));

    // --- Reed bandpass resonance (biquad BPF at f0) ---
    {
        const float w0 = 2.0f * juce::MathConstants<float>::pi * f0 / fsr;
        const float alpha = std::sin(w0) / (2.0f * kWurliReedQ);
        const float a0inv = 1.0f / (1.0f + alpha);
        reedB0 = alpha * a0inv;          // bandpass gain
        reedB2 = -reedB0;                // opposite sign
        reedA1 = -2.0f * std::cos(w0) * a0inv;
        reedA2 = (1.0f - alpha) * a0inv;
        reedState1 = 0.0f;
        reedState2 = 0.0f;
    }

    // --- Internal amp saturation drive ---
    satDrive = (kWurliSatDriveBase + vel * kWurliSatVelScale)
        * (midiNote > 72 ? (0.88f + 0.12f * highRegisterTrim) : 1.0f);

    // --- Per-voice tremolo LFO ---
    tremoloPhase = 0.0f;
    tremoloPhaseInc = (settings.performance.tremoloRateHz > 0.0f
                       ? settings.performance.tremoloRateHz : kWurliTremoloRateHz) / fsr;
    tremoloDepth = kWurliTremoloDepth * settings.performance.modelCharacter;

    // Hammer excitation (reed strike noise)
    const float effectiveHardness = chars.excitation.hammerHardnessBase *
        (kHammerHardnessBaseBlend + kHammerHardnessVelBlend * settings.tone.hammerHardness);
    hammerEnvLevel = vel * (1.0f + effectiveHardness * 2.2f);
    hammerDecayCoeff = std::exp(-1.0f / (kHammerNoiseDecaySec * 1.2f * fsr));
    {
        const float hammerNorm = juce::jlimit(0.0f, 1.0f,
            (static_cast<float>(midiNote) - 21.0f) / 67.0f);
        hammerFiltCoeff = 0.90f - hammerNorm * 0.65f;
        hammerFiltState = 0.0f;
    }
}

float WurlitzerVoice::generateSample(const float pitchFactor)
{
    // --- FM synthesis: carrier modulated by modulator ---
    // Modulator signal
    const float modSig = fastSin(modulatorPhase);
    modulatorPhase += modulatorPhaseInc * pitchFactor;
    modulatorPhase -= std::floor(modulatorPhase);

    // Carrier with frequency modulation
    // fastSin consumes cycles; the modulation index is a phase depth in radians.
    const float fmPhase = carrierPhase + fmIndex * modSig / juce::MathConstants<float>::twoPi;
    float signal = fastSin(fmPhase - std::floor(fmPhase));
    carrierPhase += carrierPhaseInc * pitchFactor;
    carrierPhase -= std::floor(carrierPhase);

    // Decay FM index → cleaner sustain over time
    fmIndex = fmIndex * fmIndexDecayCoeff;
    // Floor: some residual FM even in sustain
    if (fmIndex < fmBaseIndex * 0.06f)
        fmIndex = fmBaseIndex * 0.06f;

    // --- Reed bandpass resonance (biquad Direct Form II) ---
    {
        const float w = signal - reedA1 * reedState1 - reedA2 * reedState2;
        const float reedOut = reedB0 * w + reedB2 * reedState2;
        reedState2 = reedState1;
        reedState1 = w;
        // Mix FM + reed resonance for characteristic nasal tone
        signal = signal * 0.55f + reedOut * 0.45f;
    }

    // --- Reed strike noise ---
    if (hammerEnvLevel > 0.001f)
    {
        const float rawNoise = rng.nextFloat() * 2.0f - 1.0f;
        hammerFiltState = hammerFiltCoeff * hammerFiltState
                        + (1.0f - hammerFiltCoeff) * rawNoise;
        signal += hammerFiltState * hammerEnvLevel * kHammerNoiseMix * 0.45f;
        hammerEnvLevel *= hammerDecayCoeff;
    }

    // --- Per-voice tremolo (200A amp tremolo) ---
    if (tremoloDepth > 0.001f)
    {
        const float lfo = fastSin(tremoloPhase);
        tremoloPhase += tremoloPhaseInc;
        tremoloPhase -= std::floor(tremoloPhase);
        signal *= 1.0f - tremoloDepth * 0.5f * (1.0f - lfo);
        const float panMod = lfo * kWurliTremoloPanDepth * tremoloDepth;
        panL = std::sqrt(0.5f * (1.0f - panMod));
        panR = std::sqrt(0.5f * (1.0f + panMod));
    }

    return signal * 0.935f;
}

float WurlitzerVoice::applyCharacterProcessing(float signal, const PianoSettings& activeSettings) const
{
    if (activeSettings.performance.modelCharacter <= 0.01f)
        return signal;

    // 200A internal amp saturation: asymmetric waveshaping
    const float drive = satDrive * (1.0f + activeSettings.performance.modelCharacter * 0.65f);
    // Asymmetric clipping: positive peaks clip harder (tube-like)
    const float pos = std::tanh(signal * drive * 1.1f);
    const float neg = std::tanh(signal * drive * 0.9f);
    const float saturated = (signal >= 0.0f) ? pos : neg;
    const float normalised = saturated / std::max(0.01f, std::tanh(drive));

    return signal * (1.0f - kWurliSatMix) + normalised * kWurliSatMix;
}

PianoCharacteristics ClavinetVoice::getCharacteristics() const
{
    return mps::getCharacteristics(7);
}

PianoSettings ClavinetVoice::adaptSettings(const PianoSettings& inputSettings, const int note, const float velocity) const
{
    auto s = inputSettings;
    // Reduced resonance — Clavinet is a hammered string with pickups, no acoustic body
    s.resonance.stringResonance = juce::jlimit(0.0f, 1.0f, s.resonance.stringResonance * 0.6f);
    s.resonance.soundboardAmount = juce::jlimit(0.0f, 1.0f, s.resonance.soundboardAmount * 0.7f);
    // Ultra-short release — characteristic snappy Clavinet decay
    s.envelope.releaseSeconds = std::min(s.envelope.releaseSeconds, 0.16f);
    s.envelope.decaySeconds = juce::jmax(0.28f, s.envelope.decaySeconds * 1.08f);
    // Velocity-dependent attack sharpness — percussive key strike
    s.tone.hammerHardness = juce::jlimit(0.0f, 1.0f, s.tone.hammerHardness + velocity * 0.08f);
    // Brighter in upper register — string becomes thinner and brighter
    if (note > 60)
    {
        const float brightFactor = juce::jlimit(0.0f, 1.0f, (static_cast<float>(note) - 60.0f) / 28.0f);
        s.tone.brightness = juce::jlimit(0.0f, 1.0f, s.tone.brightness + brightFactor * 0.03f);
        s.tone.lowPassHz = juce::jlimit(120.0f, 16000.0f, s.tone.lowPassHz * (1.0f + brightFactor * 0.03f));
    }
    return s;
}

void ClavinetVoice::onNoteOn(const float f0, const float fsr)
{
    // --- String partials: predominantly odd harmonics (string clamped at fret) ---
    const int np = std::min(chars.string.partialCount, kClavMaxPartials);
    numClavPartials = np;

    const float brightMult = kBrightnessMultMin + settings.tone.brightness * kBrightnessMultRange;
    const float decayBase = kClavStringDecayBase * settings.envelope.decaySeconds / 4.0f;

    for (int n = 0; n < np; ++n)
    {
        const int harmonic = n + 1;
        const float fn = f0 * static_cast<float>(harmonic);
        if (fn >= fsr * 0.45f)
        {
            numClavPartials = n;
            break;
        }

        // Odd harmonics are stronger (string clamped at one end)
        const bool isOdd = (harmonic % 2 == 1);
        const float oddBoost = isOdd ? 1.0f : 0.4f;
        const float baseAmp = oddBoost / static_cast<float>(harmonic);

        // High-frequency roll-off via brightness
        const float rolloff = 1.0f / (1.0f + (fn / (f0 * 4.0f * brightMult)) * (fn / (f0 * 4.0f * brightMult)));
        const float amp = baseAmp * rolloff;

        // Per-partial decay: higher partials decay faster
        const float partialDecay = decayBase / (1.0f + static_cast<float>(harmonic - 1) * 0.3f
            * settings.resonance.damping);
        const float decayCoeff = std::exp(-1.0f / (std::max(0.02f, partialDecay) * fsr));

        clavPartials[static_cast<std::size_t>(n)] = { 0.0f, fn / fsr, amp, decayCoeff };
    }

    // --- Pickup highpass filter ---
    // modelCharacter controls pickup blend: 0 = neck (warm), 1 = bridge (bright)
    const float pickupFreq = kClavPickupAFreq
        + settings.performance.modelCharacter * (kClavPickupBFreq - kClavPickupAFreq);
    pickupHPCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * pickupFreq / fsr);
    pickupHPState = 0.0f;
    pickupHPOut   = 0.0f;

    // --- Brilliant filter (treble boost highpass) ---
    brillHPCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * kClavBrilliantFreq / fsr);
    brillHPState = 0.0f;
    brillHPOut = 0.0f;

    // --- Treble filter (lowpass) ---
    trebleLPCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * kClavTrebleFreq / fsr);
    trebleLPState = 0.0f;

    // Reset release state
    releaseRingLevel = 0.0f;
    releaseRingDecay = 1.0f;
    releaseNoiseLevel = 0.0f;
    releaseNoiseDecay = 1.0f;

    // Per-voice tremolo LFO
    tremoloPhase = 0.0f;
    tremoloPhaseInc = (settings.performance.tremoloRateHz > 0.0f
                       ? settings.performance.tremoloRateHz : kClavTremoloRateHz) / fsr;
    tremoloDepth = kClavTremoloDepth * settings.performance.modelCharacter;

    // Hammer excitation (sharp percussive click)
    hammerEnvLevel = vel * 2.0f;
    hammerDecayCoeff = std::exp(-1.0f / (kClavHammerWidthSec * fsr));
    hammerFiltCoeff = 0.7f;
    hammerFiltState = 0.0f;
}

void ClavinetVoice::onNoteOff()
{
    const auto fsr = static_cast<float>(sr);

    // --- String release: when key lifts, string rings briefly at different pitch ---
    // The key lifting off the fret allows the full string to vibrate momentarily
    releaseRingLevel = kClavReleaseRingMix * vel * 0.85f;
    releaseRingDecay = std::exp(-1.0f / (kClavReleaseDecaySec * fsr));

    // Mechanical noise from key release
    releaseNoiseLevel = kClavReleaseNoiseLevel * vel * 0.80f;
    releaseNoiseDecay = std::exp(-1.0f / (kClavReleaseDecaySec * 0.5f * fsr));
}

float ClavinetVoice::generateSample(const float pitchFactor)
{
    // --- Sum string partials (SIMD: 4 at a time) ---
    float signal = simd::renderPartialArray(clavPartials.data(), numClavPartials, pitchFactor);

    // --- Hammer click ---
    if (hammerEnvLevel > 0.001f)
    {
        const float rawNoise = rng.nextFloat() * 2.0f - 1.0f;
        hammerFiltState = hammerFiltCoeff * hammerFiltState
                        + (1.0f - hammerFiltCoeff) * rawNoise;
        signal += hammerFiltState * hammerEnvLevel * kHammerNoiseMix * 0.60f;
        hammerEnvLevel *= hammerDecayCoeff;
    }

    // --- String release ring (after noteOff) ---
    if (releaseRingLevel > 0.0001f)
    {
        // Brief ring of the full string at slightly shifted pitch
        float ringOut = 0.0f;
        for (int n = 0; n < numClavPartials; ++n)
        {
            auto& p = clavPartials[static_cast<std::size_t>(n)];
            ringOut += p.amp * 0.3f * fastSin(p.phase * 0.998f); // slightly different pitch
        }
        signal += ringOut * releaseRingLevel;
        releaseRingLevel *= releaseRingDecay;
    }

    // --- Release mechanical noise ---
    if (releaseNoiseLevel > 0.0001f)
    {
        const float releaseNoise = rng.nextFloat() * 2.0f - 1.0f;
        signal += releaseNoise * releaseNoiseLevel;
        releaseNoiseLevel *= releaseNoiseDecay;
    }

    // --- Pickup highpass filter (simulates pickup position) ---
    {
        const float input = signal;
        // Standard one-pole HP: y[n] = alpha * (y[n-1] + x[n] - x[n-1])
        const float hp = pickupHPCoeff * (pickupHPOut + input - pickupHPState);
        pickupHPState = input;
        pickupHPOut   = hp;
        // Blend original + highpassed for pickup position character
        signal = input * 0.4f + hp * 0.6f;
    }

    // --- Brilliant filter: add high-frequency emphasis ---
    if (settings.tone.brightness > 0.5f)
    {
        const float brillAmount = (settings.tone.brightness - 0.5f) * 2.0f;
        const float brillHP = brillHPCoeff * (brillHPOut + signal - brillHPState);
        brillHPState = signal;
        brillHPOut = brillHP;
        signal += brillHP * brillAmount * 0.15f;
    }

    // --- Treble lowpass: tame highs when brightness is low ---
    if (settings.tone.brightness < 0.5f)
    {
        const float trebleAmount = (0.5f - settings.tone.brightness) * 2.0f;
        trebleLPState += trebleLPCoeff * (signal - trebleLPState);
        signal = signal * (1.0f - trebleAmount * 0.4f) + trebleLPState * trebleAmount * 0.4f;
    }

    // --- Per-voice tremolo ---
    if (tremoloDepth > 0.001f)
    {
        const float lfo = fastSin(tremoloPhase);
        tremoloPhase += tremoloPhaseInc;
        tremoloPhase -= std::floor(tremoloPhase);
        signal *= 1.0f - tremoloDepth * 0.5f * (1.0f - lfo);
        const float panMod = lfo * kClavTremoloPanDepth * tremoloDepth;
        panL = std::sqrt(0.5f * (1.0f - panMod));
        panR = std::sqrt(0.5f * (1.0f + panMod));
    }

    const float lowRegisterGain = midiNote < 64
        ? juce::jlimit(1.0f, 1.35f, 1.0f + (64.0f - static_cast<float>(midiNote)) * 0.018f)
        : 1.0f;
    const float boosted = signal * 1.75f * lowRegisterGain;
    const float limited = std::tanh(boosted * 0.72f) / std::tanh(0.72f);
    return boosted * 0.70f + limited * 0.30f;
}

float ClavinetVoice::applyCharacterProcessing(float signal, const PianoSettings& activeSettings) const
{
    if (activeSettings.performance.modelCharacter <= 0.01f)
        return signal;

    // Clavinet has a snappy, funky character — hard clipping with presence boost
    const float drive = 1.0f + activeSettings.performance.modelCharacter * (0.80f + vel * vel * 0.90f);
    const float clipped = std::tanh(signal * drive) / std::max(0.01f, std::tanh(drive));
    return signal * 0.65f + clipped * 0.35f;
}

std::unique_ptr<PianoVoice> createVoiceForPiano(const int pianoIndex)
{
    switch (juce::jlimit(0, kNumPianos - 1, pianoIndex))
    {
        case 0: return std::make_unique<SteinwayVoice>();
        case 1: return std::make_unique<BosendorferVoice>();
        case 2: return std::make_unique<YamahaVoice>();
        case 3: return std::make_unique<BastringueVoice>();
        case 4: return std::make_unique<PreparedPianoVoice>();
        case 5: return std::make_unique<RhodesVoice>();
        case 6: return std::make_unique<WurlitzerVoice>();
        case 7: return std::make_unique<ClavinetVoice>();
        default: break;
    }

    return std::make_unique<SteinwayVoice>();
}

} // namespace mps
