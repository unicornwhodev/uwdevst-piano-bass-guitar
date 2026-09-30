#include "FactoryPresets.h"

#include <JuceHeader.h>

#include <array>
#include <initializer_list>
#include <string_view>

namespace mps
{
namespace
{
using namespace std::literals;

constexpr int kFactoryPresetsPerPiano = 8;

GlobalFxSettings makeFx(
    float satDrive, float satMix,
    float transientAttack, float transientSustain, float transientMix,
    float eqLowFreq, float eqLowGain, float eqMidFreq, float eqMidGain, float eqMidQ, float eqHighFreq, float eqHighGain,
    float compThreshold, float compRatio, float compAttack, float compRelease, float compMakeup, float compMix,
    float chorusRate, float chorusDepth, float chorusMix,
    float delayTime, float delayFeedback, float delayMix, bool delaySync, int delayNoteDivision,
    int reverbType, float reverbSize, float reverbDamping, float reverbWidth, float reverbMix, float reverbPredelay,
    float limiterThreshold, float limiterRelease,
    bool reverbEnabled, bool saturationEnabled, bool transientEnabled, bool compressorEnabled, bool eqEnabled, bool chorusEnabled, bool delayEnabled, bool limiterEnabled) noexcept
{
    GlobalFxSettings fx {};
    fx.satDrive = satDrive;
    fx.satMix = satMix;
    fx.transientAttack = transientAttack;
    fx.transientSustain = transientSustain;
    fx.transientMix = transientMix;
    fx.eqLowFreq = eqLowFreq;
    fx.eqLowGain = eqLowGain;
    fx.eqMidFreq = eqMidFreq;
    fx.eqMidGain = eqMidGain;
    fx.eqMidQ = eqMidQ;
    fx.eqHighFreq = eqHighFreq;
    fx.eqHighGain = eqHighGain;
    fx.compThreshold = compThreshold;
    fx.compRatio = compRatio;
    fx.compAttack = compAttack;
    fx.compRelease = compRelease;
    fx.compMakeup = compMakeup;
    fx.compMix = compMix;
    fx.chorusRate = chorusRate;
    fx.chorusDepth = chorusDepth;
    fx.chorusMix = chorusMix;
    fx.delayTime = delayTime;
    fx.delayFeedback = delayFeedback;
    fx.delayMix = delayMix;
    fx.delaySync = delaySync;
    fx.delayNoteDivision = delayNoteDivision;
    fx.reverbType = reverbType;
    fx.reverbSize = reverbSize;
    fx.reverbDamping = reverbDamping;
    fx.reverbWidth = reverbWidth;
    fx.reverbMix = reverbMix;
    fx.reverbPredelay = reverbPredelay;
    fx.limiterThreshold = limiterThreshold;
    fx.limiterRelease = limiterRelease;
    fx.reverbEnabled = reverbEnabled;
    fx.saturationEnabled = saturationEnabled;
    fx.transientEnabled = transientEnabled;
    fx.compressorEnabled = compressorEnabled;
    fx.eqEnabled = eqEnabled;
    fx.chorusEnabled = chorusEnabled;
    fx.delayEnabled = delayEnabled;
    fx.limiterEnabled = limiterEnabled;
    return fx;
}

modmatrix::MatrixState makeModMatrix(
    int pitchBendRange,
    float lfo2Rate,
    int lfo2Wave,
    std::initializer_list<modmatrix::ModSlot> slots) noexcept
{
    modmatrix::MatrixState state {};
    state.pitchBendRange = pitchBendRange;
    state.lfo2Rate = lfo2Rate;
    state.lfo2Wave = lfo2Wave;
    int slotIndex = 0;
    for (const auto& slot : slots)
    {
        if (slotIndex >= modmatrix::kMaxSlots)
            break;

        state.slots[static_cast<std::size_t>(slotIndex)] = slot;
        ++slotIndex;
    }
    return state;
}

PresetPerformanceState makePerformance(
    float macroWarmth,
    float macroBrillance,
    float macroExpression,
    float macroResonance,
    float lfoRate,
    float lfoDepth,
    int lfoWave,
    LfoDestination lfoDestination,
    float pitchBendRange,
    int velocityCurve,
    bool monoMode,
    bool tremoloSync,
    int modWheelTarget,
    modmatrix::MatrixState modMatrixState) noexcept
{
    PresetPerformanceState state {};
    state.macroWarmth = macroWarmth;
    state.macroBrillance = macroBrillance;
    state.macroExpression = macroExpression;
    state.macroResonance = macroResonance;
    state.lfoRate = lfoRate;
    state.lfoDepth = lfoDepth;
    state.lfoWave = lfoWave;
    state.lfoDestination = lfoDestination;
    state.pitchBendRange = pitchBendRange;
    state.velocityCurve = velocityCurve;
    state.monoMode = monoMode;
    state.tremoloSync = tremoloSync;
    state.modWheelTarget = modWheelTarget;
    state.modMatrixState = modMatrixState;
    return state;
}

PresetMetadata makeMetadata(const Family family,
                            const char* intent,
                            const char* tags,
                            const char* mixRole) noexcept
{
    const char* familyKey = "concert";
    switch (family)
    {
        case Family::Concert: familyKey = "concert"; break;
        case Family::Vintage: familyKey = "vintage"; break;
        case Family::Electric: familyKey = "electric"; break;
    }

    return {
        intent != nullptr ? intent : "",
        tags != nullptr ? tags : "",
        familyKey,
        mixRole != nullptr ? mixRole : ""
    };
}

bool isLfoDestinationAvailableForPiano(const int pianoIndex, const LfoDestination destination) noexcept
{
    if (destination == LfoDestination::Off
        || destination == LfoDestination::Tremolo
        || destination == LfoDestination::AutoPan)
        return true;

    if (destination == LfoDestination::ChorusMotion)
        return isFxAvailable(pianoIndex, GlobalFxSlot::Chorus);

    return false;
}

void normalizePresetForProductTruth(const int pianoIndex, InstrumentPreset& preset)
{
    const auto family = getFamily(pianoIndex);
    const auto defaults = getDefaultSettings(pianoIndex);

    if (!isLfoDestinationAvailableForPiano(pianoIndex, preset.performance.lfoDestination))
    {
        preset.performance.lfoDestination = LfoDestination::Off;
        preset.performance.lfoDepth = 0.0f;
        preset.performance.lfoRate = 1.0f;
        preset.performance.lfoWave = 0;
    }

    if (family != Family::Electric)
        preset.performance.tremoloSync = false;

    if (family == Family::Electric)
    {
        preset.settings.resonance.stringResonance = defaults.resonance.stringResonance;
        preset.settings.resonance.soundboardAmount = defaults.resonance.soundboardAmount;

        if (pianoIndex != 7)
            preset.settings.resonance.damping = defaults.resonance.damping;
    }
}

InstrumentPreset makePreset(const char* name,
                            PianoSettings settings,
                            GlobalFxSettings fx,
                            int outputBus,
                            PresetPerformanceState performance,
                            PresetMetadata metadata)
{
    InstrumentPreset preset {};
    preset.name = name != nullptr ? name : "";
    preset.settings = settings;
    preset.fx = fx;
    preset.outputBus = outputBus;
    preset.performance = performance;
    preset.metadata = metadata;
    return preset;
}

void finalizePreset(const int pianoIndex, InstrumentPreset& preset)
{
    // Retain preset identities and their distinct roles while correcting the
    // four keyboard profiles reported by human listening.
    if (pianoIndex == 4)
    {
        preset.settings.tone.hammerHardness *= 0.78f;
        preset.settings.tone.brightness *= 0.70f;
        preset.settings.performance.modelCharacter *= 0.58f;
        preset.settings.resonance.stringResonance *= 0.65f;
        preset.settings.resonance.soundboardAmount = juce::jlimit(0.0f, 1.0f, preset.settings.resonance.soundboardAmount + 0.08f);
        preset.settings.envelope.decaySeconds = juce::jlimit(0.35f, 2.0f, preset.settings.envelope.decaySeconds * 1.35f);
    }
    else if (pianoIndex == 5)
    {
        preset.settings.tone.brightness *= 0.90f;
        preset.fx.satMix *= 0.65f;
        preset.settings.envelope.releaseSeconds = juce::jmin(preset.settings.envelope.releaseSeconds, 0.20f);
        preset.settings.envelope.decaySeconds *= 0.85f;
    }
    else if (pianoIndex == 6)
    {
        preset.settings.tone.hammerHardness *= 0.88f;
        preset.fx.satMix *= 0.65f;
        preset.fx.transientAttack *= 0.65f;
    }
    else if (pianoIndex == 7)
    {
        preset.settings.tone.brightness *= 0.92f;
        preset.fx.satMix *= 0.60f;
        preset.fx.transientAttack *= 0.75f;
    }
    if (pianoIndex >= 4)
        preset.fx.eqHighGain = juce::jmin(preset.fx.eqHighGain, 1.5f);
    preset.settings.level = juce::jlimit(0.0f, 1.0f,
                                         preset.settings.level * juce::Decibels::decibelsToGain(-6.0f));
    preset.settings.tuneSemitones = juce::jlimit(-0.10f, 0.10f, preset.settings.tuneSemitones);
    preset.fx = maskUnavailableFx(pianoIndex, preset.fx);
    preset.fx.limiterEnabled = isFxAvailable(pianoIndex, GlobalFxSlot::Limiter);
    preset.outputBus = juce::jlimit(0, 4, preset.outputBus);
    preset.performance.pitchBendRange = juce::jlimit(1.0f, 24.0f, preset.performance.pitchBendRange);
    preset.performance.velocityCurve = juce::jlimit(0, 6, preset.performance.velocityCurve);
    preset.performance.modWheelTarget = juce::jlimit(0, 1, preset.performance.modWheelTarget);
    preset.performance.modMatrixState.lfo2Rate = juce::jmax(0.05f, preset.performance.modMatrixState.lfo2Rate);
    preset.performance.modMatrixState.lfo2Wave = juce::jlimit(0, 3, preset.performance.modMatrixState.lfo2Wave);
    normalizePresetForProductTruth(pianoIndex, preset);
    if (pianoIndex == 5)
    {
        preset.fx.delayEnabled = false;
        preset.fx.delayMix = 0.0f;
        preset.fx.delayFeedback = 0.0f;
        preset.fx.chorusMix = juce::jmin(preset.fx.chorusMix, 0.06f);
        preset.fx.chorusDepth = juce::jmin(preset.fx.chorusDepth, 0.12f);
        preset.fx.reverbMix = juce::jmin(preset.fx.reverbMix, 0.035f);
        preset.fx.reverbSize = juce::jmin(preset.fx.reverbSize, 0.30f);
        preset.fx.reverbPredelay = 0.0f;
        preset.fx.satMix = juce::jmin(preset.fx.satMix, 0.05f);
        preset.fx.satDrive = juce::jmin(preset.fx.satDrive, 1.35f);
        preset.fx.compMix = juce::jmin(preset.fx.compMix, 0.18f);
    }
}

std::array<std::vector<InstrumentPreset>, kNumPianos> createFactoryBanks()
{
    std::array<std::vector<InstrumentPreset>, kNumPianos> banks {};

    // steinway_d
    {
        auto& bank = banks[static_cast<std::size_t>(0)];
        bank.reserve(kFactoryPresetsPerPiano);
        bank.push_back(
        makePreset(
            "Velvet Stern",
            PianoSettings{ 0.78f, 0.0f, 0.52f, 0.006f, 3.8f, 0.34f, 0.45f, 0.62f, 0.42f, 0.6f, 0.4f, 0.5f, 10000.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.52f, 0.5f, 0.72f, 0.04f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "faire chanter le Steinway dans un contexte solo ou neo-classique sans perdre sa tenue; faire tenir le motif principal sans ecraser le reste du mix.",
                         "steinway,grand,concert,grand-lyrique,medium,stable,reference,dry",
                         "reference")));
        bank.push_back(
        makePreset(
            "Tight Cadence",
            PianoSettings{ 0.78f, 0.0f, 0.57f, 0.006f, 3.8f, 0.1875f, 0.1975f, 0.64f, 0.3f, 0.42f, 0.45f, 0.5f, 10000.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.18f, 0.0f, 0.22f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.1f, 20.0f, 120.0f, 0.0f, 0.22f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.52f, 0.5f, 0.72f, 0.05f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "utiliser le Steinway comme moteur harmonique plutot que comme simple tapis; mettre en avant des syncopes, octaves ou ponctuations melodiques.",
                         "steinway,grand,concert,moteur-harmonique,haut,vif,stable,close,dry",
                         "close")));
        bank.push_back(
        makePreset(
            "Ivory Bloom",
            PianoSettings{ 0.78f, 0.0f, 0.52f, 0.006f, 3.8f, 0.34f, 0.45f, 0.64f, 0.48f, 0.68f, 0.39f, 0.5f, 10000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.18f, 0.0f, 0.22f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.24f, 0.0f, false, 1,
                   0, 0.58f, 0.36f, 0.74f, 0.18f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, true, false, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "faire chanter le Steinway dans un contexte solo ou neo-classique sans perdre sa tenue; mettre en avant des syncopes, octaves ou ponctuations melodiques.",
                         "steinway,grand,concert,grand-lyrique,haut,vif,stable,featured,mix-ready",
                         "featured")));
        bank.push_back(
        makePreset(
            "Amber Sonnet",
            PianoSettings{ 0.78f, 0.0f, 0.52f, 0.006f, 3.8f, 0.34f, 0.45f, 0.55f, 0.48f, 0.68f, 0.37f, 0.5f, 10000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 6200.0f, -1.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.58f, 0.36f, 0.74f, 0.18f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "faire chanter le Steinway dans un contexte solo ou neo-classique sans perdre sa tenue; poser un lit harmonique credibile sous voix, cordes ou guitares.",
                         "steinway,grand,concert,grand-lyrique,bas-medium,moyen,stable,warm,mix-ready",
                         "warm")));
        bank.push_back(
        makePreset(
            "Chisel Waltz",
            PianoSettings{ 0.78f, 0.0f, 0.57f, 0.006f, 3.8f, 0.16f, 0.104f, 0.64f, 0.2544f, 0.4f, 0.42f, 0.5f, 10000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.15f, -0.06f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.1f, 20.0f, 120.0f, 0.0f, 0.22f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.48f, 0.36f, 0.74f, 0.045f, 40.0f,
                   -0.3f, 50.0f,
                   true, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "utiliser le Steinway comme moteur harmonique plutot que comme simple tapis; soutenir des melodies longues, rubato ou accompagnements respirants.",
                         "steinway,grand,concert,moteur-harmonique,medium-haut,doux,souple,cut,mix-ready",
                         "cut")));
        bank.push_back(
        makePreset(
            "Brisk Ledger",
            PianoSettings{ 0.78f, 0.0f, 0.57f, 0.006f, 3.8f, 0.1875f, 0.2195f, 0.64f, 0.36f, 0.5f, 0.4f, 0.5f, 10000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.15f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.1f, 20.0f, 120.0f, 0.0f, 0.22f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.6f, 0.36f, 0.74f, 0.06f, 20.0f,
                   -0.3f, 50.0f,
                   true, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "utiliser le Steinway comme moteur harmonique plutot que comme simple tapis; laisser vivre les partiels et les pedales sans devenir une simple nappe floue.",
                         "steinway,grand,concert,moteur-harmonique,grave-large,ample,ouvert,groove,mix-ready",
                         "groove")));
        bank.push_back(
        makePreset(
            "Anchor Bloom",
            PianoSettings{ 0.78f, 0.0f, 0.55f, 0.006f, 3.8f, 0.34f, 0.5f, 0.59f, 0.48f, 0.68f, 0.33f, 0.5f, 10000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.24f, 0.0f, false, 1,
                   0, 0.6f, 0.36f, 0.86f, 0.18f, 20.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "tirer la profondeur orchestrale du Steinway sans basculer dans la boue; laisser vivre les partiels et les pedales sans devenir une simple nappe floue.",
                         "steinway,grand,concert,pedale-cinematique,grave-large,ample,ouvert,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Cutting Study",
            PianoSettings{ 0.78f, 0.0f, 0.57f, 0.006f, 3.8f, 0.1875f, 0.2107f, 0.64f, 0.36f, 0.5f, 0.44f, 0.5f, 10000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.15f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.1f, 20.0f, 120.0f, 0.0f, 0.22f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.78f, 0.06f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "utiliser le Steinway comme moteur harmonique plutot que comme simple tapis; ouvrir le spectre pour un arrangement plus cinematique ou moderne.",
                         "steinway,grand,concert,moteur-harmonique,large,moyen,ample,groove,mix-ready",
                         "groove")));
        for (auto& preset : bank)
            finalizePreset(0, preset);
    }

    // bosendorfer_imperial
    {
        auto& bank = banks[static_cast<std::size_t>(1)];
        bank.reserve(kFactoryPresetsPerPiano);
        bank.push_back(
        makePreset(
            "Bass Lantern",
            PianoSettings{ 0.76f, 0.0f, 0.48f, 0.007f, 3.0f, 0.4f, 0.49f, 0.51f, 0.42f, 0.6f, 0.36f, 0.55f, 8000.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   110.0f, 0.8f, 1700.0f, 0.7f, 0.8f, 7000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 0.85f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.52f, 0.5f, 0.72f, 0.04f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, false, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "exploiter le registre grave unique sans manger tout l'arrangement; installer un piano intime ou un clavier serre en avant du mix.",
                         "bosendorfer,imperial,concert,fondation-grave,medium,retenu,stable,reference,dry",
                         "reference")));
        bank.push_back(
        makePreset(
            "Focused Marble",
            PianoSettings{ 0.76f, 0.0f, 0.51f, 0.007f, 3.0f, 0.4f, 0.58f, 0.56f, 0.42f, 0.6f, 0.39f, 0.55f, 8000.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.0f, 20.0f, 120.0f, 0.0f, 0.2f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.52f, 0.36f, 0.72f, 0.05f, 10.0f,
                   -0.6f, 80.0f,
                   true, false, false, true, false, false, false, true),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "garder la signature imperiale mais avec un contour plus metrone; porter une progression harmonique cinematique, generique ou underscore.",
                         "bosendorfer,imperial,concert,comping-noble,large,ample,evolutif,close,dry",
                         "close")));
        bank.push_back(
        makePreset(
            "Silken Hall",
            PianoSettings{ 0.76f, 0.0f, 0.45f, 0.007f, 3.0f, 0.4f, 0.57f, 0.52f, 0.55f, 0.8f, 0.29f, 0.55f, 8000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.24f, 0.0f, false, 1,
                   0, 0.6f, 0.36f, 0.74f, 0.18f, 20.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "mettre en avant le chant typiquement viennois du Bosendorfer; laisser vivre les partiels et les pedales sans devenir une simple nappe floue.",
                         "bosendorfer,imperial,concert,chant-viennois,grave-large,ample,ouvert,featured,mix-ready",
                         "featured")));
        bank.push_back(
        makePreset(
            "Amber Breath",
            PianoSettings{ 0.76f, 0.0f, 0.45f, 0.007f, 3.0f, 0.4f, 0.48f, 0.52f, 0.55f, 0.8f, 0.38f, 0.55f, 8000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.24f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.3f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.6f, 0.36f, 0.74f, 0.19f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "mettre en avant le chant typiquement viennois du Bosendorfer; garder du rebond dans les cellules rythmiques et les compings serres.",
                         "bosendorfer,imperial,concert,chant-viennois,medium,nerveux,sec,warm,mix-ready",
                         "warm")));
        bank.push_back(
        makePreset(
            "Defined Waltz",
            PianoSettings{ 0.76f, 0.0f, 0.51f, 0.007f, 3.0f, 0.2f, 0.104f, 0.56f, 0.3076f, 0.46f, 0.39f, 0.55f, 8000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.0f, 20.0f, 120.0f, 0.0f, 0.2f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.78f, 0.045f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "garder la signature imperiale mais avec un contour plus metrone; ouvrir le spectre pour un arrangement plus cinematique ou moderne.",
                         "bosendorfer,imperial,concert,comping-noble,large,moyen,ample,cut,mix-ready",
                         "cut")));
        bank.push_back(
        makePreset(
            "Bronze Outline",
            PianoSettings{ 0.76f, 0.0f, 0.51f, 0.007f, 3.0f, 0.2f, 0.104f, 0.52f, 0.3076f, 0.46f, 0.38f, 0.55f, 8000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 6200.0f, -1.0f,
                   -18.0f, 2.0f, 20.0f, 120.0f, 0.0f, 0.2f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.78f, 0.06f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, true, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "garder la signature imperiale mais avec un contour plus metrone; poser un lit harmonique credibile sous voix, cordes ou guitares.",
                         "bosendorfer,imperial,concert,comping-noble,bas-medium,moyen,stable,groove,mix-ready",
                         "groove")));
        bank.push_back(
        makePreset(
            "Sacred Floor",
            PianoSettings{ 0.76f, 0.0f, 0.48f, 0.007f, 3.0f, 0.4f, 0.58f, 0.56f, 0.55f, 0.8f, 0.31f, 0.55f, 8000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.24f, 0.0f, false, 1,
                   0, 0.72f, 0.36f, 0.88f, 0.18f, 10.0f,
                   -0.6f, 80.0f,
                   true, false, false, false, false, false, false, true),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "faire respirer la resonance suspendue de l'Imperial dans des espaces larges; porter une progression harmonique cinematique, generique ou underscore.",
                         "bosendorfer,imperial,concert,texture-large,large,ample,evolutif,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Clear Vienna",
            PianoSettings{ 0.76f, 0.0f, 0.51f, 0.007f, 3.0f, 0.2f, 0.104f, 0.56f, 0.3076f, 0.46f, 0.41f, 0.55f, 8000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.78f, 0.06f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 1, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "garder la signature imperiale mais avec un contour plus metrone; faire tenir le motif principal sans ecraser le reste du mix.",
                         "bosendorfer,imperial,concert,comping-noble,medium,stable,groove,mix-ready",
                         "groove")));
        for (auto& preset : bank)
            finalizePreset(1, preset);
    }

    // yamaha_cfx
    {
        auto& bank = banks[static_cast<std::size_t>(2)];
        bank.reserve(kFactoryPresetsPerPiano);
        bank.push_back(
        makePreset(
            "Accurate Loft",
            PianoSettings{ 0.79f, 0.0f, 0.59f, 0.005f, 3.2f, 0.32f, 0.36f, 0.7f, 0.42f, 0.6f, 0.505f, 0.48f, 12000.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.24f, 0.0f, 0.0f,
                   120.0f, 0.0f, 1900.0f, 0.5f, 1.0f, 7600.0f, 0.3f,
                   -18.0f, 2.3f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.52f, 0.5f, 0.72f, 0.04f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, true, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "obtenir un piano de reference tres controllable en enregistrement; garder du rebond dans les cellules rythmiques et les compings serres.",
                         "yamaha,cfx,concert,reference-studio,medium,nerveux,sec,reference,dry",
                         "reference")));
        bank.push_back(
        makePreset(
            "Direct Arc",
            PianoSettings{ 0.79f, 0.0f, 0.62f, 0.005f, 3.2f, 0.32f, 0.46f, 0.75f, 0.42f, 0.6f, 0.435f, 0.48f, 12000.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.9f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.52f, 0.36f, 0.72f, 0.05f, 10.0f,
                   -0.6f, 80.0f,
                   true, false, false, true, false, false, false, true),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "faire avancer le CFX au premier plan avec sa signature transparente; porter une progression harmonique cinematique, generique ou underscore.",
                         "yamaha,cfx,concert,lead-moderne,large,ample,evolutif,close,dry",
                         "close")));
        bank.push_back(
        makePreset(
            "Carbon Hall",
            PianoSettings{ 0.79f, 0.0f, 0.62f, 0.005f, 3.2f, 0.32f, 0.36f, 0.74f, 0.42f, 0.6f, 0.485f, 0.48f, 12000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.24f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.3f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.24f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.18f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "faire avancer le CFX au premier plan avec sa signature transparente; garder du rebond dans les cellules rythmiques et les compings serres.",
                         "yamaha,cfx,concert,lead-moderne,medium,nerveux,sec,featured,mix-ready",
                         "featured")));
        bank.push_back(
        makePreset(
            "Blue Reflection",
            PianoSettings{ 0.79f, 0.0f, 0.56f, 0.005f, 3.2f, 0.32f, 0.36f, 0.66f, 0.42f, 0.6f, 0.485f, 0.48f, 12000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.24f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.3f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.45f, 0.36f, 0.74f, 0.14f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "adoucir la machine moderne pour les contextes plus intimes; garder du rebond dans les cellules rythmiques et les compings serres.",
                         "yamaha,cfx,concert,douceur-moderne,medium,nerveux,sec,warm,mix-ready",
                         "warm")));
        bank.push_back(
        makePreset(
            "Bright Atlas",
            PianoSettings{ 0.79f, 0.0f, 0.62f, 0.005f, 3.2f, 0.32f, 0.37f, 0.74f, 0.42f, 0.6f, 0.475f, 0.48f, 12000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1700.0f, 0.7f, 0.8f, 7000.0f, 0.0f,
                   -18.0f, 1.9f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.25f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, true, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "faire avancer le CFX au premier plan avec sa signature transparente; installer un piano intime ou un clavier serre en avant du mix.",
                         "yamaha,cfx,concert,lead-moderne,medium,retenu,stable,cut,mix-ready",
                         "cut")));
        bank.push_back(
        makePreset(
            "Fast Prism",
            PianoSettings{ 0.79f, 0.0f, 0.61f, 0.005f, 3.2f, 0.1765f, 0.1975f, 0.73f, 0.3f, 0.42f, 0.445f, 0.48f, 12000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.18f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.2f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.6f, 0.36f, 0.74f, 0.06f, 20.0f,
                   -0.3f, 50.0f,
                   true, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "transformer le CFX en moteur rythmique tres propre; laisser vivre les partiels et les pedales sans devenir une simple nappe floue.",
                         "yamaha,cfx,concert,comping-net,grave-large,ample,ouvert,groove,mix-ready",
                         "groove")));
        bank.push_back(
        makePreset(
            "Arc Window",
            PianoSettings{ 0.79f, 0.0f, 0.55f, 0.005f, 3.2f, 0.32f, 0.47f, 0.66f, 0.42f, 0.6f, 0.435f, 0.48f, 12000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5200.0f, -1.2f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.24f, 0.0f, false, 1,
                   0, 0.68f, 0.36f, 0.86f, 0.14f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "donner au CFX une ampleur cinematique tout en gardant sa precision; servir les morceaux fragiles, minimalistes ou contemplatifs.",
                         "yamaha,cfx,concert,texture-claire,medium-bas,calme,feutre,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Grid Pulse",
            PianoSettings{ 0.79f, 0.0f, 0.61f, 0.005f, 3.2f, 0.14f, 0.0914f, 0.75f, 0.2088f, 0.32f, 0.505f, 0.48f, 12000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.18f, -0.06f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.78f, 0.06f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Concert,
                         "transformer le CFX en moteur rythmique tres propre; faire tenir le motif principal sans ecraser le reste du mix.",
                         "yamaha,cfx,concert,comping-net,medium,stable,groove,mix-ready",
                         "groove")));
        for (auto& preset : bank)
            finalizePreset(2, preset);
    }

    // bastringue
    {
        auto& bank = banks[static_cast<std::size_t>(3)];
        bank.reserve(kFactoryPresetsPerPiano);
        bank.push_back(
        makePreset(
            "Reel Lantern",
            PianoSettings{ 0.72f, 0.0f, 0.68f, 0.006f, 1.6f, 0.24f, 0.32f, 0.76f, 0.3f, 0.55f, 0.585f, 0.7f, 9800.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.22f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.72f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "faire avancer le rythme et soutenir violon ou accordeon dans un contexte folk; faire tenir le motif principal sans ecraser le reste du mix.",
                         "upright,bastringue,honky,vintage,comping-de-danse,medium,stable,reference,dry",
                         "reference")));
        bank.push_back(
        makePreset(
            "Fiddle Floor",
            PianoSettings{ 0.72f, 0.0f, 0.68f, 0.006f, 1.6f, 0.24f, 0.36f, 0.76f, 0.3f, 0.55f, 0.545f, 0.7f, 9800.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.22f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.4f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.48f, 0.36f, 0.72f, 0.08f, 40.0f,
                   -0.3f, 50.0f,
                   true, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "faire avancer le rythme et soutenir violon ou accordeon dans un contexte folk; soutenir des melodies longues, rubato ou accompagnements respirants.",
                         "upright,bastringue,honky,vintage,comping-de-danse,medium-haut,doux,souple,close,dry",
                         "close")));
        bank.push_back(
        makePreset(
            "Barn Accent",
            PianoSettings{ 0.72f, 0.0f, 0.68f, 0.006f, 1.6f, 0.24f, 0.32f, 0.76f, 0.3f, 0.55f, 0.565f, 0.7f, 9800.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, -0.6f, 2100.0f, 1.2f, 1.0f, 6200.0f, -1.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.25f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, false, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "donner au Bastringue un vrai role d'appui harmonique tres visible; poser un lit harmonique credibile sous voix, cordes ou guitares.",
                         "upright,bastringue,honky,vintage,coupe-d-ensemble,bas-medium,moyen,stable,warm,mix-ready",
                         "warm")));
        bank.push_back(
        makePreset(
            "Crisp Contra",
            PianoSettings{ 0.72f, 0.0f, 0.68f, 0.006f, 1.6f, 0.24f, 0.35f, 0.76f, 0.3f, 0.55f, 0.575f, 0.7f, 9800.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.1f, 20.0f, 120.0f, 0.0f, 0.22f,
                   0.1f, 0.0f, 0.08f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.78f, 0.2f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, true, false, true, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "employer le Bastringue comme pulse harmonique moderne mais credible; ouvrir le spectre pour un arrangement plus cinematique ou moderne.",
                         "upright,bastringue,honky,vintage,moteur-folk-pop,large,moyen,ample,cut,mix-ready",
                         "cut")));
        bank.push_back(
        makePreset(
            "Boot Counter",
            PianoSettings{ 0.72f, 0.0f, 0.68f, 0.006f, 1.6f, 0.1323f, 0.1493f, 0.76f, 0.18f, 0.37f, 0.565f, 0.7f, 9800.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.22f, 0.0f, 0.0f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5200.0f, -1.2f,
                   -18.0f, 2.4f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.74f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, true, true, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "faire avancer le rythme et soutenir violon ou accordeon dans un contexte folk; servir les morceaux fragiles, minimalistes ou contemplatifs.",
                         "upright,bastringue,honky,vintage,comping-de-danse,medium-bas,calme,feutre,groove,mix-ready",
                         "groove")));
        bank.push_back(
        makePreset(
            "Copper Button",
            PianoSettings{ 0.72f, 0.0f, 0.68f, 0.006f, 1.6f, 0.24f, 0.32f, 0.76f, 0.3f, 0.55f, 0.535f, 0.7f, 9800.0f, 0.0f },
            makeFx(1.0f, 0.16f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 6200.0f, -1.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.25f, 10.0f,
                   -0.3f, 50.0f,
                   false, true, false, false, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "offrir une variante typee qui garde un vrai usage d'arrangement; poser un lit harmonique credibile sous voix, cordes ou guitares.",
                         "upright,bastringue,honky,vintage,couleur-de-caractere,bas-medium,moyen,stable,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Brisk Porch",
            PianoSettings{ 0.72f, 0.0f, 0.68f, 0.006f, 1.6f, 0.1323f, 0.1229f, 0.76f, 0.18f, 0.37f, 0.6f, 0.7f, 9800.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.24f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.3f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.78f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "faire avancer le rythme et soutenir violon ou accordeon dans un contexte folk; garder du rebond dans les cellules rythmiques et les compings serres.",
                         "upright,bastringue,honky,vintage,comping-de-danse,medium,nerveux,sec,groove,mix-ready",
                         "groove")));
        bank.push_back(
        makePreset(
            "Dust Measure",
            PianoSettings{ 0.72f, 0.0f, 0.68f, 0.006f, 1.6f, 0.24f, 0.32f, 0.76f, 0.3f, 0.55f, 0.555f, 0.7f, 9800.0f, 0.0f },
            makeFx(1.0f, 0.16f,
                   0.18f, 0.0f, 0.22f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.25f, 10.0f,
                   -0.3f, 50.0f,
                   false, true, true, false, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "offrir une variante typee qui garde un vrai usage d'arrangement; mettre en avant des syncopes, octaves ou ponctuations melodiques.",
                         "upright,bastringue,honky,vintage,couleur-de-caractere,haut,vif,stable,character,mix-ready",
                         "character")));
        for (auto& preset : bank)
            finalizePreset(3, preset);
    }

    // piano_prepare
    {
        auto& bank = banks[static_cast<std::size_t>(4)];
        bank.reserve(kFactoryPresetsPerPiano);
        bank.push_back(
        makePreset(
            "Bolt Lantern",
            PianoSettings{ 0.7f, 0.0f, 0.68f, 0.004f, 0.4f, 0.18f, 0.28f, 0.72f, 0.24f, 0.45f, 0.6f, 0.75f, 8200.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.2f, 0.0f, 0.24f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.72f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "creer des frappes metalliques jouables, entre clavier et petite percussion accordee; faire tenir le motif principal sans ecraser le reste du mix.",
                         "prepared,extended,objects,vintage,percussion-metallique,medium,stable,reference,dry",
                         "reference")));
        bank.push_back(
        makePreset(
            "Metal Thread",
            PianoSettings{ 0.7f, 0.0f, 0.68f, 0.004f, 0.45f, 0.18f, 0.32f, 0.72f, 0.24f, 0.45f, 0.6f, 0.75f, 8200.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.2f, 0.0f, 0.24f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 0.85f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.48f, 0.36f, 0.72f, 0.08f, 40.0f,
                   -0.3f, 50.0f,
                   true, false, true, false, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "creer des frappes metalliques jouables, entre clavier et petite percussion accordee; soutenir des melodies longues, rubato ou accompagnements respirants.",
                         "prepared,extended,objects,vintage,percussion-metallique,medium-haut,doux,souple,close,dry",
                         "close")));
        bank.push_back(
        makePreset(
            "Cardboard Veil",
            PianoSettings{ 0.7f, 0.0f, 0.62f, 0.004f, 0.42f, 0.18f, 0.24f, 0.63f, 0.24f, 0.45f, 0.6f, 0.75f, 6600.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.24f, 0.0f, 0.0f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.3f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.74f, 0.14f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "obtenir un prepare poetique et tonique plutot qu'un simple effet special; garder du rebond dans les cellules rythmiques et les compings serres.",
                         "prepared,extended,objects,vintage,prepare-feutre,medium,nerveux,sec,featured,mix-ready",
                         "featured")));
        bank.push_back(
        makePreset(
            "Moss Register",
            PianoSettings{ 0.7f, 0.0f, 0.62f, 0.004f, 0.51f, 0.18f, 0.33f, 0.63f, 0.24f, 0.45f, 0.6f, 0.75f, 6600.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.6f, 0.36f, 0.74f, 0.18f, 20.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "obtenir un prepare poetique et tonique plutot qu'un simple effet special; laisser vivre les partiels et les pedales sans devenir une simple nappe floue.",
                         "prepared,extended,objects,vintage,prepare-feutre,grave-large,ample,ouvert,warm,mix-ready",
                         "warm")));
        bank.push_back(
        makePreset(
            "Muted Mallet",
            PianoSettings{ 0.7f, 0.0f, 0.67f, 0.004f, 0.41f, 0.0992f, 0.1229f, 0.72f, 0.12f, 0.27f, 0.6f, 0.75f, 7500.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.18f, 0.0f, 0.22f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.6f, 20.0f, 120.0f, 0.0f, 0.22f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.78f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "sortir un prepared piano quasi percussif mais encore organisable musicalement; mettre en avant des syncopes, octaves ou ponctuations melodiques.",
                         "prepared,extended,objects,vintage,prepare-percussif,haut,vif,stable,groove,mix-ready",
                         "groove")));
        bank.push_back(
        makePreset(
            "Bali Spark",
            PianoSettings{ 0.7f, 0.0f, 0.65f, 0.004f, 0.45f, 0.18f, 0.31f, 0.72f, 0.24f, 0.45f, 0.6f, 0.75f, 7900.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   0.1f, 0.0f, 0.08f,
                   220.0f, 0.18f, 0.1f, false, 1,
                   0, 0.42f, 0.36f, 0.78f, 0.2f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, false, true, true, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "orienter le prepare vers une percussion harmonique de type gamelan; ouvrir le spectre pour un arrangement plus cinematique ou moderne.",
                         "prepared,extended,objects,vintage,couleur-gamelan,large,moyen,ample,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Bell Weave",
            PianoSettings{ 0.7f, 0.0f, 0.65f, 0.004f, 0.43f, 0.18f, 0.28f, 0.72f, 0.24f, 0.45f, 0.6f, 0.75f, 8400.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.18f, 0.0f, 0.22f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   220.0f, 0.18f, 0.1f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.25f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, false, false, false, true, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "orienter le prepare vers une percussion harmonique de type gamelan; mettre en avant des syncopes, octaves ou ponctuations melodiques.",
                         "prepared,extended,objects,vintage,couleur-gamelan,haut,vif,stable,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Floor Bolt",
            PianoSettings{ 0.7f, 0.0f, 0.67f, 0.004f, 0.42f, 0.0992f, 0.1229f, 0.62f, 0.12f, 0.27f, 0.6f, 0.75f, 6600.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.26f, 0.0f, 0.0f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 6200.0f, -1.0f,
                   -18.0f, 2.6f, 20.0f, 120.0f, 0.0f, 0.22f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.78f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, true, true, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Vintage,
                         "sortir un prepared piano quasi percussif mais encore organisable musicalement; poser un lit harmonique credibile sous voix, cordes ou guitares.",
                         "prepared,extended,objects,vintage,prepare-percussif,bas-medium,moyen,stable,groove,mix-ready",
                         "groove")));
        for (auto& preset : bank)
            finalizePreset(4, preset);
    }

    // rhodes_mark_i
    {
        auto& bank = banks[static_cast<std::size_t>(5)];
        bank.reserve(kFactoryPresetsPerPiano);
        bank.push_back(
        makePreset(
            "Honey Tine",
            PianoSettings{ 0.74f, 0.0f, 0.41f, 0.01f, 2.5f, 0.42f, 0.38f, 0.58f, 0.1f, 0.0f, 0.3f, 0.5f, 8150.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.72f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "obtenir le Rhodes chaleureux qui porte la grille sans piquer; faire tenir le motif principal sans ecraser le reste du mix.",
                         "rhodes,tine,electric,soul-chaud,medium,stable,reference,dry",
                         "reference")));
        bank.push_back(
        makePreset(
            "Dry Pocket",
            PianoSettings{ 0.74f, 0.0f, 0.48f, 0.01f, 2.5f, 0.42f, 0.35f, 0.58f, 0.1f, 0.0f, 0.3f, 0.5f, 9500.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.3f, 20.0f, 120.0f, 0.0f, 0.23f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.72f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "faire sortir un Rhodes qui accompagne comme une guitare rythmique; mettre en avant des syncopes, octaves ou ponctuations melodiques.",
                         "rhodes,tine,electric,comping-funk,haut,vif,stable,close,dry",
                         "close")));
        bank.push_back(
        makePreset(
            "Clear Orbit",
            PianoSettings{ 0.74f, 0.0f, 0.47f, 0.01f, 2.5f, 0.42f, 0.43f, 0.58f, 0.1f, 0.0f, 0.3f, 0.5f, 9150.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   0.18f, 0.16f, 0.12f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.6f, 0.36f, 0.74f, 0.18f, 20.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, false, true, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "mettre en avant le brillant chantant du tine sans le rendre sec; laisser vivre les partiels et les pedales sans devenir une simple nappe floue.",
                         "rhodes,tine,electric,bell-rhodes,grave-large,ample,ouvert,featured,mix-ready",
                         "featured")));
        bank.push_back(
        makePreset(
            "Amber Lounge",
            PianoSettings{ 0.74f, 0.0f, 0.37f, 0.01f, 2.5f, 0.42f, 0.4f, 0.58f, 0.1f, 0.0f, 0.3f, 0.5f, 8150.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5200.0f, -1.2f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   0.22f, 0.24f, 0.2f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.74f, 0.14f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, true, true, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "obtenir le Rhodes chaleureux qui porte la grille sans piquer; servir les morceaux fragiles, minimalistes ou contemplatifs.",
                         "rhodes,tine,electric,soul-chaud,medium-bas,calme,feutre,warm,mix-ready",
                         "warm")));
        bank.push_back(
        makePreset(
            "Bark Pocket",
            PianoSettings{ 0.74f, 0.0f, 0.48f, 0.01f, 2.5f, 0.42f, 0.35f, 0.58f, 0.1f, 0.0f, 0.3f, 0.5f, 9050.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.25f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "faire sortir un Rhodes qui accompagne comme une guitare rythmique; faire tenir le motif principal sans ecraser le reste du mix.",
                         "rhodes,tine,electric,comping-funk,medium,stable,cut,mix-ready",
                         "cut")));
        bank.push_back(
        makePreset(
            "Needle Jam",
            PianoSettings{ 0.74f, 0.0f, 0.48f, 0.01f, 2.5f, 0.2316f, 0.28f, 0.58f, 0.1f, 0.0f, 0.3f, 0.5f, 9050.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.3f, 20.0f, 120.0f, 0.0f, 0.23f,
                   0.1f, 0.0f, 0.04f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.78f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, true, false, true, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "faire sortir un Rhodes qui accompagne comme une guitare rythmique; ouvrir le spectre pour un arrangement plus cinematique ou moderne.",
                         "rhodes,tine,electric,comping-funk,large,moyen,ample,groove,mix-ready",
                         "groove")));
        bank.push_back(
        makePreset(
            "Dream Rotor",
            PianoSettings{ 0.74f, 0.0f, 0.45f, 0.01f, 2.5f, 0.42f, 0.43f, 0.58f, 0.1f, 0.0f, 0.3f, 0.5f, 8750.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 0.0f, 0.18f,
                   0.28f, 0.3f, 0.26f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.25f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, true, false, true, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "transformer le Rhodes en couche enveloppante tres musicalement stable; faire tenir le motif principal sans ecraser le reste du mix.",
                         "rhodes,tine,electric,rhodes-large,medium,stable,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Bright Suitcase",
            PianoSettings{ 0.74f, 0.0f, 0.47f, 0.01f, 2.5f, 0.42f, 0.41f, 0.58f, 0.1f, 0.0f, 0.3f, 0.5f, 9150.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   0.18f, 0.16f, 0.08f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.78f, 0.2f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, false, true, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 2, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "mettre en avant le brillant chantant du tine sans le rendre sec; ouvrir le spectre pour un arrangement plus cinematique ou moderne.",
                         "rhodes,tine,electric,bell-rhodes,large,moyen,ample,cut,mix-ready",
                         "cut")));
        for (auto& preset : bank)
            finalizePreset(5, preset);
    }

    // wurlitzer_200a
    {
        auto& bank = banks[static_cast<std::size_t>(6)];
        bank.reserve(kFactoryPresetsPerPiano);
        bank.push_back(
        makePreset(
            "Soul Comb",
            PianoSettings{ 0.7f, 0.0f, 0.56f, 0.008f, 1.8f, 0.36f, 0.35f, 0.62f, 0.08f, 0.0f, 0.4f, 0.55f, 7750.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 4200.0f, -0.75f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 0.0f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.72f, 0.08f, 10.0f,
                   -0.3f, 50.0f,
                   false, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "tenir la chaleur emotive du Wurli sans trop d'agressivite; faire tenir le motif principal sans ecraser le reste du mix.",
                         "wurlitzer,reed,electric,soul-vivant,medium,stable,reference,dry",
                         "reference")));
        bank.push_back(
        makePreset(
            "Dry Arcade",
            PianoSettings{ 0.7f, 0.0f, 0.64f, 0.008f, 1.8f, 0.36f, 0.41f, 0.62f, 0.08f, 0.0f, 0.4f, 0.55f, 8550.0f, 0.0f },
            makeFx(1.0f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 4200.0f, -0.75f,
                   -18.0f, 2.4f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.72f, 0.36f, 0.72f, 0.08f, 10.0f,
                   -0.6f, 80.0f,
                   true, true, false, true, false, false, false, true),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "sortir le cote nerveux et mordant du 200A dans un vrai contexte de groupe; porter une progression harmonique cinematique, generique ou underscore.",
                         "wurlitzer,reed,electric,bark-aggressif,large,ample,evolutif,close,dry",
                         "close")));
        bank.push_back(
        makePreset(
            "Amber Click",
            PianoSettings{ 0.7f, 0.0f, 0.52f, 0.008f, 1.8f, 0.36f, 0.37f, 0.62f, 0.08f, 0.0f, 0.4f, 0.55f, 7750.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 4200.0f, -1.2f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   0.16f, 0.1f, 0.08f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.74f, 0.14f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, true, true, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "tenir la chaleur emotive du Wurli sans trop d'agressivite; servir les morceaux fragiles, minimalistes ou contemplatifs.",
                         "wurlitzer,reed,electric,soul-vivant,medium-bas,calme,feutre,warm,mix-ready",
                         "warm")));
        bank.push_back(
        makePreset(
            "Crank Cabinet",
            PianoSettings{ 0.7f, 0.0f, 0.64f, 0.008f, 1.8f, 0.36f, 0.35f, 0.62f, 0.08f, 0.0f, 0.4f, 0.55f, 8150.0f, 0.0f },
            makeFx(1.0f, 0.18f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 4200.0f, -1.0f,
                   -18.0f, 2.4f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.25f, 10.0f,
                   -0.3f, 50.0f,
                   false, true, false, true, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "sortir le cote nerveux et mordant du 200A dans un vrai contexte de groupe; poser un lit harmonique credibile sous voix, cordes ou guitares.",
                         "wurlitzer,reed,electric,bark-aggressif,bas-medium,moyen,stable,cut,mix-ready",
                         "cut")));
        bank.push_back(
        makePreset(
            "Amber Jukebox",
            PianoSettings{ 0.7f, 0.0f, 0.56f, 0.008f, 1.8f, 0.36f, 0.37f, 0.62f, 0.08f, 0.0f, 0.4f, 0.55f, 7650.0f, 0.0f },
            makeFx(1.0f, 0.14f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 4200.0f, -1.2f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.74f, 0.14f, 10.0f,
                   -0.3f, 50.0f,
                   true, true, false, false, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "conserver l'identite Wurli en l'inscrivant dans une couleur retro believable; servir les morceaux fragiles, minimalistes ou contemplatifs.",
                         "wurlitzer,reed,electric,vintage-pop,medium-bas,calme,feutre,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Blue Parlor",
            PianoSettings{ 0.7f, 0.0f, 0.57f, 0.008f, 1.8f, 0.36f, 0.35f, 0.62f, 0.08f, 0.0f, 0.4f, 0.55f, 7500.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 4200.0f, -1.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.52f, 0.36f, 0.74f, 0.16f, 10.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "rendre le Wurlitzer credible dans un cadre plus sensible et chantant; poser un lit harmonique credibile sous voix, cordes ou guitares.",
                         "wurlitzer,reed,electric,ballade-wurli,bas-medium,moyen,stable,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Brown Reed",
            PianoSettings{ 0.7f, 0.0f, 0.56f, 0.008f, 1.8f, 0.36f, 0.39f, 0.62f, 0.08f, 0.0f, 0.4f, 0.55f, 7750.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 4200.0f, -0.75f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 0.0f, 1.0f,
                   0.16f, 0.1f, 0.08f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.48f, 0.36f, 0.74f, 0.16f, 40.0f,
                   -0.3f, 50.0f,
                   true, false, false, false, false, true, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "tenir la chaleur emotive du Wurli sans trop d'agressivite; soutenir des melodies longues, rubato ou accompagnements respirants.",
                         "wurlitzer,reed,electric,soul-vivant,medium-haut,doux,souple,warm,mix-ready",
                         "warm")));
        bank.push_back(
        makePreset(
            "Drive Reed",
            PianoSettings{ 0.7f, 0.0f, 0.64f, 0.008f, 1.8f, 0.36f, 0.39f, 0.62f, 0.08f, 0.0f, 0.4f, 0.55f, 8550.0f, 0.0f },
            makeFx(1.0f, 0.18f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 4200.0f, -0.75f,
                   -18.0f, 2.4f, 20.0f, 120.0f, 0.0f, 0.24f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.48f, 0.36f, 0.74f, 0.16f, 40.0f,
                   -0.3f, 50.0f,
                   true, true, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 3, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "sortir le cote nerveux et mordant du 200A dans un vrai contexte de groupe; soutenir des melodies longues, rubato ou accompagnements respirants.",
                         "wurlitzer,reed,electric,bark-aggressif,medium-haut,doux,souple,cut,mix-ready",
                         "cut")));
        for (auto& preset : bank)
            finalizePreset(6, preset);
    }

    // clavinet_d6
    {
        auto& bank = banks[static_cast<std::size_t>(7)];
        bank.reserve(kFactoryPresetsPerPiano);
        bank.push_back(
        makePreset(
            "Funk Razor",
            PianoSettings{ 0.98f, 0.0f, 0.82f, 0.003f, 0.5f, 0.16f, 0.16f, 0.82f, 0.05f, 0.0f, 0.47f, 0.65f, 11000.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 4.5f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.72f, 0.08f, 10.0f,
                   -0.4f, 50.0f,
                   false, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 4, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "tenir un vrai role de guitare rythmique clavier dans le groove; faire tenir le motif principal sans ecraser le reste du mix.",
                         "clavinet,clav,pluck,electric,clav-funk,medium,stable,reference,dry",
                         "reference")));
        bank.push_back(
        makePreset(
            "Dry Engine",
            PianoSettings{ 0.98f, 0.0f, 0.78f, 0.003f, 0.5f, 0.16f, 0.16f, 0.82f, 0.05f, 0.0f, 0.47f, 0.65f, 10500.0f, 0.0f },
            makeFx(1.8f, 0.12f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 4.5f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.72f, 0.08f, 10.0f,
                   -0.4f, 50.0f,
                   false, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 4, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "utiliser le D6 comme outil de pulsation nette et efficace; faire tenir le motif principal sans ecraser le reste du mix.",
                         "clavinet,clav,pluck,electric,moteur-sec,medium,stable,close,dry",
                         "close")));
        bank.push_back(
        makePreset(
            "Lead Stringer",
            PianoSettings{ 0.98f, 0.0f, 0.8f, 0.003f, 0.535f, 0.16f, 0.16f, 0.82f, 0.05f, 0.0f, 0.47f, 0.65f, 10500.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 1.8f, 20.0f, 120.0f, 4.5f, 0.18f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.8f, 0.08f, 10.0f,
                   -0.4f, 50.0f,
                   false, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 4, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "permettre des petites phrases lead sans sortir de la grammaire Clavinet; faire tenir le motif principal sans ecraser le reste du mix.",
                         "clavinet,clav,pluck,electric,lead-clav,medium,stable,featured,mix-ready",
                         "featured")));
        bank.push_back(
        makePreset(
            "Low Wah",
            PianoSettings{ 0.98f, 0.0f, 0.74f, 0.003f, 0.565f, 0.16f, 0.16f, 0.82f, 0.05f, 0.0f, 0.45f, 0.65f, 9600.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 7000.0f, -1.1f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 1.5f, 1.0f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.72f, 0.36f, 0.88f, 0.08f, 10.0f,
                   -0.6f, 80.0f,
                   true, false, false, false, true, false, false, true),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 4, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "garder l'articulation du clavinet pour les contextes plus lisses; porter une progression harmonique cinematique, generique ou underscore.",
                         "clavinet,clav,pluck,electric,clav-doux,large,ample,evolutif,warm,mix-ready",
                         "warm")));
        bank.push_back(
        makePreset(
            "Glass Stinger",
            PianoSettings{ 0.98f, 0.0f, 0.8f, 0.003f, 0.6f, 0.16f, 0.16f, 0.82f, 0.05f, 0.0f, 0.45f, 0.65f, 10500.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.0f, 20.0f, 120.0f, 4.5f, 0.2f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.72f, 0.36f, 0.88f, 0.08f, 10.0f,
                   -0.6f, 80.0f,
                   true, false, false, true, false, false, false, true),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 4, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "permettre des petites phrases lead sans sortir de la grammaire Clavinet; porter une progression harmonique cinematique, generique ou underscore.",
                         "clavinet,clav,pluck,electric,lead-clav,large,ample,evolutif,cut,mix-ready",
                         "cut")));
        bank.push_back(
        makePreset(
            "Split Rail",
            PianoSettings{ 0.98f, 0.0f, 0.82f, 0.003f, 0.5f, 0.0882f, 0.0966f, 0.82f, 0.05f, 0.0f, 0.49f, 0.65f, 11000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   120.0f, 0.0f, 1700.0f, 0.7f, 0.8f, 7000.0f, 0.0f,
                   -18.0f, 2.5f, 20.0f, 120.0f, 4.5f, 0.26f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.18f, 0.0f, false, 1,
                   0, 0.55f, 0.5f, 0.78f, 0.08f, 10.0f,
                   -0.4f, 50.0f,
                   false, false, false, true, true, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 4, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "tenir un vrai role de guitare rythmique clavier dans le groove; installer un piano intime ou un clavier serre en avant du mix.",
                         "clavinet,clav,pluck,electric,clav-funk,medium,retenu,stable,groove,mix-ready",
                         "groove")));
        bank.push_back(
        makePreset(
            "Arc Clack",
            PianoSettings{ 0.98f, 0.0f, 0.8f, 0.003f, 0.555f, 0.16f, 0.16f, 0.82f, 0.05f, 0.0f, 0.46f, 0.65f, 11000.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -18.0f, 2.0f, 20.0f, 120.0f, 4.5f, 0.2f,
                   1.0f, 0.5f, 0.0f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.74f, 0.08f, 10.0f,
                   -0.4f, 50.0f,
                   true, false, false, true, false, false, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 4, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "permettre des petites phrases lead sans sortir de la grammaire Clavinet; mettre en avant des syncopes, octaves ou ponctuations melodiques.",
                         "clavinet,clav,pluck,electric,lead-clav,haut,vif,stable,character,mix-ready",
                         "character")));
        bank.push_back(
        makePreset(
            "Chrome Strut",
            PianoSettings{ 0.98f, 0.0f, 0.78f, 0.003f, 0.565f, 0.16f, 0.16f, 0.82f, 0.05f, 0.0f, 0.45f, 0.65f, 11200.0f, 0.0f },
            makeFx(1.8f, 0.15f,
                   0.1f, 0.0f, 0.4f,
                   200.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 5000.0f, 0.0f,
                   -19.0f, 3.0f, 10.0f, 120.0f, 1.5f, 1.0f,
                   0.22f, 0.1f, 0.08f,
                   300.0f, 0.3f, 0.0f, false, 1,
                   0, 0.42f, 0.36f, 0.78f, 0.08f, 10.0f,
                   -0.4f, 50.0f,
                   true, false, false, false, false, true, false, false),
            0,
            makePerformance(0.5f, 0.5f, 0.5f, 0.5f,
                            1.0f, 0.0f, 0, LfoDestination(0),
                            2.0f, 4, false, false, 1,
                            makeModMatrix(2, 2.0f, 0,
{
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f },
            { modmatrix::Source(0), modmatrix::Destination(0), 0.0f }
                            })),
            makeMetadata(Family::Electric,
                         "faire rayonner le D6 dans un arrangement plus pop ou disco; ouvrir le spectre pour un arrangement plus cinematique ou moderne.",
                         "clavinet,clav,pluck,electric,clav-brillant,large,moyen,ample,character,mix-ready",
                         "character")));
        for (auto& preset : bank)
            finalizePreset(7, preset);
    }

    return banks;
}
} // namespace

const std::array<std::vector<InstrumentPreset>, kNumPianos>& getFactoryPresetBanks()
{
    static const auto banks = createFactoryBanks();
    return banks;
}

bool isSupportedMixRole(std::string_view mixRole) noexcept
{
    return mixRole == "reference"sv
        || mixRole == "close"sv
        || mixRole == "featured"sv
        || mixRole == "warm"sv
        || mixRole == "cut"sv
        || mixRole == "groove"sv
        || mixRole == "character"sv
        || mixRole == "signature"sv;
}

std::string_view getPresetBrowserTier(std::string_view mixRole) noexcept
{
    if (mixRole == "reference"sv || mixRole == "close"sv)
        return "dry"sv;
    if (mixRole == "featured"sv || mixRole == "character"sv)
        return "featured"sv;
    if (mixRole == "warm"sv || mixRole == "cut"sv || mixRole == "groove"sv)
        return "mix"sv;
    if (mixRole == "signature"sv)
        return "signature"sv;

    return {};
}

} // namespace mps
