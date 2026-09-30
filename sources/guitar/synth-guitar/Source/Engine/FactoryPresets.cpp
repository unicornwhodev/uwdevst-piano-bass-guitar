#include "FactoryPresets.h"

#include <algorithm>
#include <array>
#include <cstddef>

namespace mgs
{
namespace
{

float clamp01(const float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

std::string familyLabelForInstrument(const int instrIndex)
{
    switch (getFamily(instrIndex))
    {
        case Family::Acoustique:   return "acoustic";
        case Family::Electrique:   return "electric";
        case Family::Electronique: return "hybrid";
    }

    return "guitar";
}

std::string mixRoleForInstrument(const int instrIndex)
{
    if (instrIndex >= 6)
        return "texture";
    if (instrIndex == 5)
        return "lead";
    if (instrIndex <= 2)
        return "pluck";
    return "rhythm";
}

int outputBusForInstrument(const int instrIndex)
{
    if (instrIndex < 3)
        return 0;
    if (instrIndex < 6)
        return 1;
    if (instrIndex < 8)
        return 2;
    return 3;
}

PresetMetadata makeMetadata(const int instrIndex, const bool signature)
{
    PresetMetadata metadata;
    metadata.familyLabel = familyLabelForInstrument(instrIndex);
    metadata.mixRole = mixRoleForInstrument(instrIndex);
    metadata.nominalPeakDb = metadata.mixRole == "texture" ? (signature ? -13.0f : -15.0f)
                           : metadata.mixRole == "lead"    ? (signature ? -10.5f : -12.0f)
                           : metadata.mixRole == "pluck"   ? (signature ? -12.0f : -14.0f)
                                                           : (signature ? -11.5f : -13.5f);

    metadata.tags = {
        "guitar",
        "factory",
        signature ? "signature" : "reference",
        metadata.mixRole == "texture" ? "texture" : "core",
        signature ? "showcase" : "dry"
    };

    if (!signature)
        metadata.tags.push_back("writer");
    if (signature)
        metadata.tags.push_back("audition");
    return metadata;
}

GlobalFxSettings makeFx(const int instrIndex, const bool signature)
{
    GlobalFxSettings fx {};
    const auto availability = getFxAvailability(instrIndex);

    fx.saturatorOn = false;
    fx.transientOn = availability.transient;
    fx.eqOn = availability.eq;
    fx.compressorOn = availability.compressor;
    fx.chorusOn = false;
    fx.delayOn = false;
    fx.reverbOn = availability.reverb;
    fx.limiterOn = availability.limiter;
    fx.cabinetOn = false;

    fx.satDrive = 1.0f;
    fx.satMix = 0.0f;
    fx.transientAttack = signature ? 0.08f : 0.04f;
    fx.transientSustain = 0.0f;
    fx.transientMix = signature ? 0.16f : 0.08f;
    fx.eqLowFreq = 180.0f;
    fx.eqLowGain = 0.0f;
    fx.eqMidFreq = 1000.0f;
    fx.eqMidGain = 0.0f;
    fx.eqMidQ = 1.0f;
    fx.eqHighFreq = 5500.0f;
    fx.eqHighGain = 0.0f;
    fx.compThreshold = signature ? -18.0f : -22.0f;
    fx.compRatio = signature ? 2.6f : 1.8f;
    fx.compAttack = 12.0f;
    fx.compRelease = signature ? 130.0f : 160.0f;
    fx.compMakeup = 0.0f;
    fx.compMix = signature ? 0.55f : 0.35f;
    fx.chorusRate = 0.8f;
    fx.chorusDepth = 0.18f;
    fx.chorusDelay = 7.0f;
    fx.chorusMix = 0.0f;
    fx.delayTime = 300.0f;
    fx.delayFeedback = 0.0f;
    fx.delayMix = 0.0f;
    fx.reverbSize = signature ? 0.38f : 0.28f;
    fx.reverbDamping = 0.50f;
    fx.reverbWidth = signature ? 0.78f : 0.62f;
    fx.reverbMix = signature ? 0.08f : 0.035f;
    fx.limiterThreshold = -0.8f;
    fx.limiterRelease = 55.0f;
    fx.cabMix = 0.0f;

    switch (instrIndex)
    {
        case 0: // Folk Steel
            fx.eqLowFreq = 210.0f;
            fx.eqLowGain = signature ? 0.8f : 0.3f;
            fx.eqHighFreq = 6200.0f;
            fx.eqHighGain = signature ? 0.4f : -0.4f;
            fx.reverbMix = signature ? 0.075f : 0.030f;
            break;

        case 1: // 12 Cordes
            fx.eqHighFreq = 7200.0f;
            fx.eqHighGain = signature ? 0.8f : 0.2f;
            fx.chorusOn = availability.chorus && signature;
            fx.chorusDepth = 0.16f;
            fx.chorusMix = signature ? 0.055f : 0.0f;
            fx.reverbMix = signature ? 0.085f : 0.035f;
            break;

        case 2: // Flamenca
            fx.eqLowFreq = 170.0f;
            fx.eqMidFreq = 1900.0f;
            fx.eqMidGain = signature ? 0.9f : 0.2f;
            fx.eqHighFreq = 7600.0f;
            fx.transientAttack = signature ? 0.14f : 0.08f;
            fx.transientMix = signature ? 0.22f : 0.12f;
            fx.reverbMix = signature ? 0.055f : 0.020f;
            break;

        case 3: // Clean
            fx.eqLowFreq = 190.0f;
            fx.eqMidFreq = 1250.0f;
            fx.eqHighFreq = 6500.0f;
            fx.chorusOn = availability.chorus && signature;
            fx.chorusDepth = 0.20f;
            fx.chorusMix = signature ? 0.060f : 0.0f;
            fx.cabinetOn = availability.cabinet;
            fx.cabMix = signature ? 0.24f : 0.16f;
            fx.reverbMix = signature ? 0.070f : 0.030f;
            break;

        case 4: // Crunch
            fx.saturatorOn = availability.saturator && signature;
            fx.satDrive = signature ? 1.8f : 1.0f;
            fx.satMix = signature ? 0.13f : 0.0f;
            fx.eqMidFreq = 1300.0f;
            fx.eqMidGain = signature ? 1.2f : 0.3f;
            fx.eqHighGain = -0.7f;
            fx.compMix = signature ? 0.60f : 0.42f;
            fx.cabinetOn = availability.cabinet;
            fx.cabMix = signature ? 0.42f : 0.28f;
            fx.reverbMix = signature ? 0.050f : 0.020f;
            break;

        case 5: // Lead
            fx.saturatorOn = availability.saturator && signature;
            fx.satDrive = signature ? 2.1f : 1.0f;
            fx.satMix = signature ? 0.16f : 0.0f;
            fx.eqLowFreq = 160.0f;
            fx.eqMidFreq = 1500.0f;
            fx.eqMidGain = signature ? 1.4f : 0.4f;
            fx.eqHighGain = -0.9f;
            fx.compThreshold = signature ? -16.0f : -20.0f;
            fx.compRatio = signature ? 3.4f : 2.2f;
            fx.compMix = signature ? 0.70f : 0.48f;
            fx.cabinetOn = availability.cabinet;
            fx.cabMix = signature ? 0.50f : 0.34f;
            fx.reverbMix = signature ? 0.060f : 0.025f;
            break;

        case 6: // Synth Guitar
            fx.eqLowFreq = 220.0f;
            fx.eqMidFreq = 950.0f;
            fx.eqHighFreq = 5800.0f;
            fx.chorusOn = availability.chorus && signature;
            fx.chorusDepth = 0.22f;
            fx.chorusMix = signature ? 0.090f : 0.0f;
            fx.reverbMix = signature ? 0.100f : 0.040f;
            break;

        case 7: // E-Guitar Pad
            fx.eqLowFreq = 250.0f;
            fx.eqMidFreq = 760.0f;
            fx.eqMidGain = signature ? -0.6f : -0.3f;
            fx.eqHighFreq = 4600.0f;
            fx.eqHighGain = signature ? -0.7f : -0.4f;
            fx.chorusOn = availability.chorus && signature;
            fx.chorusDepth = 0.28f;
            fx.chorusMix = signature ? 0.120f : 0.0f;
            fx.compThreshold = -24.0f;
            fx.compRatio = 1.8f;
            fx.reverbSize = signature ? 0.52f : 0.36f;
            fx.reverbMix = signature ? 0.135f : 0.055f;
            break;

        case 8: // Guitar Ambient
            fx.eqLowFreq = 300.0f;
            fx.eqLowGain = 0.5f;
            fx.eqMidFreq = 650.0f;
            fx.eqMidGain = -0.8f;
            fx.eqHighFreq = 4200.0f;
            fx.eqHighGain = signature ? -0.3f : -0.5f;
            fx.chorusOn = availability.chorus && signature;
            fx.chorusDepth = 0.24f;
            fx.chorusMix = signature ? 0.110f : 0.0f;
            fx.reverbSize = signature ? 0.60f : 0.42f;
            fx.reverbDamping = 0.58f;
            fx.reverbMix = signature ? 0.150f : 0.060f;
            break;

        default:
            break;
    }

    fx.delayOn = false;
    fx.delayMix = 0.0f;
    fx.delayFeedback = 0.0f;

    auto masked = maskUnavailableFx(instrIndex, fx);
    if (!availability.saturator)
    {
        masked.satDrive = 1.0f;
        masked.satMix = 0.0f;
    }
    if (!availability.transient)
    {
        masked.transientAttack = 0.0f;
        masked.transientSustain = 0.0f;
        masked.transientMix = 0.0f;
    }
    if (!availability.compressor)
    {
        masked.compMakeup = 0.0f;
        masked.compMix = 0.0f;
    }
    if (!availability.chorus)
        masked.chorusMix = 0.0f;
    if (!availability.delay)
    {
        masked.delayTime = 300.0f;
        masked.delayFeedback = 0.0f;
        masked.delayMix = 0.0f;
    }
    if (!availability.cabinet)
        masked.cabMix = 0.0f;
    return masked;
}

InstrSettings makeSettings(const int instrIndex, const bool signature)
{
    auto settings = getDefaultSettings(instrIndex);
    settings.tuneSemitones = 0.0f;
    settings.spatial.pan = 0.0f;

    switch (instrIndex)
    {
        case 0: // Folk Steel
            settings.level = signature ? 0.92f : 0.88f;
            settings.tone.stringBrightness = signature ? 0.82f : 0.76f;
            settings.envelope.attackSeconds = 0.003f;
            settings.envelope.decaySeconds = signature ? 3.1f : 2.8f;
            settings.envelope.sustainLevel = signature ? 0.12f : 0.08f;
            settings.envelope.releaseSeconds = signature ? 0.36f : 0.28f;
            settings.resonance.bodyAmount = signature ? 0.58f : 0.52f;
            settings.resonance.driveAmount = 0.0f;
            settings.performance.attackBrightness = signature ? 0.84f : 0.78f;
            settings.performance.pickPosition = signature ? 0.47f : 0.50f;
            settings.spatial.stereoWidth = signature ? 0.36f : 0.28f;
            settings.tone.lowPassHz = signature ? 13200.0f : 12200.0f;
            break;

        case 1: // 12 Cordes
            settings.level = signature ? 0.91f : 0.87f;
            settings.tone.stringBrightness = signature ? 0.88f : 0.82f;
            settings.envelope.attackSeconds = 0.004f;
            settings.envelope.decaySeconds = signature ? 2.9f : 2.6f;
            settings.envelope.sustainLevel = 0.10f;
            settings.envelope.releaseSeconds = signature ? 0.42f : 0.34f;
            settings.resonance.bodyAmount = signature ? 0.52f : 0.48f;
            settings.resonance.driveAmount = 0.0f;
            settings.performance.attackBrightness = signature ? 0.86f : 0.80f;
            settings.performance.pickPosition = signature ? 0.52f : 0.50f;
            settings.spatial.stereoWidth = signature ? 0.62f : 0.48f;
            settings.tone.lowPassHz = signature ? 14000.0f : 12800.0f;
            break;

        case 2: // Flamenca
            settings.level = signature ? 0.93f : 0.89f;
            settings.tone.stringBrightness = signature ? 0.92f : 0.86f;
            settings.envelope.attackSeconds = 0.001f;
            settings.envelope.decaySeconds = signature ? 1.45f : 1.25f;
            settings.envelope.sustainLevel = 0.04f;
            settings.envelope.releaseSeconds = signature ? 0.22f : 0.18f;
            settings.resonance.bodyAmount = signature ? 0.48f : 0.42f;
            settings.resonance.driveAmount = 0.0f;
            settings.performance.attackBrightness = signature ? 0.98f : 0.92f;
            settings.performance.pickPosition = signature ? 0.58f : 0.55f;
            settings.spatial.stereoWidth = signature ? 0.30f : 0.22f;
            settings.tone.lowPassHz = signature ? 14500.0f : 13400.0f;
            break;

        case 3: // Clean
            settings.level = signature ? 0.90f : 0.86f;
            settings.tone.stringBrightness = signature ? 0.80f : 0.74f;
            settings.envelope.attackSeconds = 0.002f;
            settings.envelope.decaySeconds = signature ? 3.3f : 3.0f;
            settings.envelope.sustainLevel = signature ? 0.24f : 0.20f;
            settings.envelope.releaseSeconds = signature ? 0.32f : 0.26f;
            settings.resonance.bodyAmount = signature ? 0.18f : 0.14f;
            settings.resonance.driveAmount = signature ? 0.04f : 0.0f;
            settings.performance.attackBrightness = signature ? 0.82f : 0.76f;
            settings.performance.pickPosition = signature ? 0.48f : 0.52f;
            settings.spatial.stereoWidth = signature ? 0.36f : 0.26f;
            settings.tone.lowPassHz = signature ? 13200.0f : 12200.0f;
            break;

        case 4: // Crunch
            settings.level = signature ? 0.46f : 0.42f;
            settings.tone.stringBrightness = signature ? 0.56f : 0.50f;
            settings.envelope.attackSeconds = 0.002f;
            settings.envelope.decaySeconds = signature ? 2.8f : 2.5f;
            settings.envelope.sustainLevel = signature ? 0.26f : 0.22f;
            settings.envelope.releaseSeconds = signature ? 0.26f : 0.22f;
            settings.resonance.bodyAmount = 0.10f;
            settings.resonance.driveAmount = signature ? 0.34f : 0.24f;
            settings.performance.attackBrightness = signature ? 0.60f : 0.52f;
            settings.performance.pickPosition = signature ? 0.58f : 0.52f;
            settings.spatial.stereoWidth = signature ? 0.30f : 0.22f;
            settings.tone.lowPassHz = signature ? 7600.0f : 6800.0f;
            break;

        case 5: // Lead
            settings.level = signature ? 0.42f : 0.38f;
            settings.tone.stringBrightness = signature ? 0.60f : 0.54f;
            settings.envelope.attackSeconds = 0.003f;
            settings.envelope.decaySeconds = signature ? 2.9f : 2.5f;
            settings.envelope.sustainLevel = signature ? 0.36f : 0.30f;
            settings.envelope.releaseSeconds = signature ? 0.30f : 0.24f;
            settings.resonance.bodyAmount = 0.05f;
            settings.resonance.driveAmount = signature ? 0.42f : 0.30f;
            settings.performance.attackBrightness = signature ? 0.48f : 0.42f;
            settings.performance.pickPosition = signature ? 0.62f : 0.58f;
            settings.spatial.stereoWidth = signature ? 0.25f : 0.18f;
            settings.tone.lowPassHz = signature ? 8800.0f : 7800.0f;
            break;

        case 6: // Synth Guitar
            settings.level = signature ? 0.70f : 0.64f;
            settings.tone.stringBrightness = signature ? 0.54f : 0.46f;
            settings.envelope.attackSeconds = signature ? 0.010f : 0.006f;
            settings.envelope.decaySeconds = signature ? 3.8f : 3.4f;
            settings.envelope.sustainLevel = signature ? 0.50f : 0.46f;
            settings.envelope.releaseSeconds = signature ? 0.48f : 0.38f;
            settings.resonance.bodyAmount = 0.0f;
            settings.resonance.driveAmount = signature ? 0.18f : 0.10f;
            settings.performance.attackBrightness = signature ? 0.46f : 0.38f;
            settings.performance.pickPosition = signature ? 0.58f : 0.52f;
            settings.spatial.stereoWidth = signature ? 0.58f : 0.44f;
            settings.tone.lowPassHz = signature ? 7000.0f : 6200.0f;
            break;

        case 7: // E-Guitar Pad
            settings.level = signature ? 0.66f : 0.60f;
            settings.tone.stringBrightness = signature ? 0.42f : 0.36f;
            settings.envelope.attackSeconds = signature ? 0.050f : 0.025f;
            settings.envelope.decaySeconds = signature ? 5.6f : 4.8f;
            settings.envelope.sustainLevel = signature ? 0.58f : 0.52f;
            settings.envelope.releaseSeconds = signature ? 1.35f : 1.05f;
            settings.resonance.bodyAmount = signature ? 0.20f : 0.16f;
            settings.resonance.driveAmount = signature ? 0.07f : 0.03f;
            settings.performance.attackBrightness = signature ? 0.28f : 0.22f;
            settings.performance.pickPosition = signature ? 0.44f : 0.40f;
            settings.spatial.stereoWidth = signature ? 0.72f : 0.58f;
            settings.tone.lowPassHz = signature ? 5600.0f : 5000.0f;
            break;

        case 8: // Guitar Ambient
            settings.level = signature ? 0.96f : 0.90f;
            settings.tone.stringBrightness = signature ? 0.40f : 0.34f;
            settings.envelope.attackSeconds = signature ? 0.150f : 0.090f;
            settings.envelope.decaySeconds = signature ? 9.0f : 7.8f;
            settings.envelope.sustainLevel = signature ? 0.64f : 0.56f;
            settings.envelope.releaseSeconds = signature ? 2.60f : 2.00f;
            settings.resonance.bodyAmount = signature ? 0.36f : 0.30f;
            settings.resonance.driveAmount = 0.0f;
            settings.performance.attackBrightness = signature ? 0.36f : 0.28f;
            settings.performance.pickPosition = signature ? 0.38f : 0.34f;
            settings.spatial.stereoWidth = signature ? 0.86f : 0.74f;
            settings.tone.lowPassHz = signature ? 5000.0f : 4300.0f;
            break;

        default:
            break;
    }

    settings.level = clamp01(settings.level);
    settings.tone.stringBrightness = clamp01(settings.tone.stringBrightness);
    settings.envelope.sustainLevel = clamp01(settings.envelope.sustainLevel);
    settings.resonance.bodyAmount = clamp01(settings.resonance.bodyAmount);
    settings.resonance.driveAmount = clamp01(settings.resonance.driveAmount);
    settings.performance.attackBrightness = clamp01(settings.performance.attackBrightness);
    settings.performance.pickPosition = clamp01(settings.performance.pickPosition);
    settings.spatial.stereoWidth = clamp01(settings.spatial.stereoWidth);
    settings.spatial.pan = std::clamp(settings.spatial.pan, -1.0f, 1.0f);
    settings.tone.lowPassHz = std::clamp(settings.tone.lowPassHz, 120.0f, 16000.0f);
    return settings;
}

InstrumentPreset makePreset(const int instrIndex, const bool signature)
{
    InstrumentPreset preset;
    preset.name = std::string(getInstrName(instrIndex)) + (signature ? " Signature" : " Reference");
    preset.settings = makeSettings(instrIndex, signature);
    preset.fx = makeFx(instrIndex, signature);
    preset.outputBus = outputBusForInstrument(instrIndex);
    preset.playMode = 0.0f;
    preset.palmMute = 0.0f;
    preset.metadata = makeMetadata(instrIndex, signature);
    return preset;
}

#include "Curated8FactoryPresets.inc"

} // namespace

const std::array<std::vector<InstrumentPreset>, kNumInstruments>& getFactoryPresetBanks()
{
    static const std::array<std::vector<InstrumentPreset>, kNumInstruments> banks = []()
    {
        std::array<std::vector<InstrumentPreset>, kNumInstruments> builtBanks {};
        for (int instrIndex = 0; instrIndex < kNumInstruments; ++instrIndex)
        {
            auto& bank = builtBanks[static_cast<std::size_t>(instrIndex)];
            bank.reserve(10);
            bank.push_back(makePreset(instrIndex, false));
            bank.push_back(makePreset(instrIndex, true));
        }
        appendCurated8FactoryPresets(builtBanks);
        return builtBanks;
    }();

    return banks;
}

const std::vector<CollectionPreset>& getFactoryPresets()
{
    static const std::vector<CollectionPreset> presets;
    return presets;
}

} // namespace mgs
