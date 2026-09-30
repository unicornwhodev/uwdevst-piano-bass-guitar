#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Engine/FxProcessors.h"
#include "../../Shared/PresetManifest.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>

namespace
{
constexpr const char* kOutputGain    = "output_gain";
constexpr const char* kSelectedPiano = "selected_piano";
constexpr const char* kLfoRate       = "lfo_rate";
constexpr const char* kLfoDepth      = "lfo_depth";
constexpr const char* kLfoWave       = "lfo_wave";
constexpr const char* kLfoDestination = "lfo_destination";
constexpr const char* kPitchBendRange = "pitch_bend_range";
constexpr const char* kVelocityCurve  = "velocity_curve";
constexpr const char* kMonoMode       = "mono_mode";
constexpr const char* kTremoloSync    = "tremolo_sync";

constexpr const char* kMacroWarmth      = "macro_warmth";
constexpr const char* kMacroBrillance   = "macro_brillance";
constexpr const char* kMacroExpression   = "macro_expression";
constexpr const char* kMacroResonance   = "macro_resonance";
constexpr const char* kModWheelTarget   = "mod_wheel_target";
constexpr const char* kLegacyMacroWarmth = "warmth";
constexpr const char* kLegacyMacroBrillance = "brillance";
constexpr const char* kLegacyMacroExpression = "expression";
constexpr const char* kLegacyMacroResonance = "resonance_macro";

constexpr const char* kCompThreshold = "comp_threshold";
constexpr const char* kCompRatio     = "comp_ratio";
constexpr const char* kCompAttack    = "comp_attack";
constexpr const char* kCompRelease   = "comp_release";
constexpr const char* kCompMakeup    = "comp_makeup";
constexpr const char* kCompMix       = "comp_mix";

constexpr const char* kSatDrive = "sat_drive";
constexpr const char* kSatMix   = "sat_mix";

constexpr const char* kTransientAttack  = "transient_attack";
constexpr const char* kTransientSustain = "transient_sustain";
constexpr const char* kTransientMix     = "transient_mix";

constexpr const char* kReverbSize    = "reverb_size";
constexpr const char* kReverbDamping = "reverb_damping";
constexpr const char* kReverbWidth   = "reverb_width";
constexpr const char* kReverbMix     = "reverb_mix";
constexpr const char* kReverbPreDelay = "reverb_predelay";
constexpr const char* kReverbEnabled = "fx_tab7_en";
constexpr const char* kReverbType    = "reverb_type";

constexpr const char* kEqLowFreq    = "eq_low_freq";
constexpr const char* kEqLowGain    = "eq_low_gain";
constexpr const char* kEqMidFreq    = "eq_mid_freq";
constexpr const char* kEqMidGain    = "eq_mid_gain";
constexpr const char* kEqMidQ       = "eq_mid_q";
constexpr const char* kEqHighFreq   = "eq_high_freq";
constexpr const char* kEqHighGain   = "eq_high_gain";

constexpr const char* kChorusRate   = "chorus_rate";
constexpr const char* kChorusDepth  = "chorus_depth";
constexpr const char* kChorusMix    = "chorus_mix";

constexpr const char* kDelayTime    = "delay_time";
constexpr const char* kDelayFeedback = "delay_feedback";
constexpr const char* kDelayMix     = "delay_mix";
constexpr const char* kDelaySync    = "delay_sync";
constexpr const char* kDelayNoteDiv = "delay_division";

constexpr const char* kSaturationEnabled = "fx_tab0_en";
constexpr const char* kTransientEnabled  = "fx_tab1_en";
constexpr const char* kCompressorEnabled = "fx_tab2_en";
constexpr const char* kEqEnabled         = "fx_tab3_en";
constexpr const char* kChorusEnabled     = "fx_tab4_en";
constexpr const char* kDelayEnabled      = "fx_tab5_en";
constexpr const char* kLimiterEnabled    = "fx_tab6_en";
constexpr const char* kFxLock            = "fx_lock";
constexpr const char* kLegacySaturationEnabled = "saturation_enabled";
constexpr const char* kLegacyTransientEnabled = "transient_enabled";
constexpr const char* kLegacyCompressorEnabled = "compressor_enabled";
constexpr const char* kLegacyEqEnabled = "eq_enabled";
constexpr const char* kLegacyChorusEnabled = "chorus_enabled";
constexpr const char* kLegacyDelayEnabled = "delay_enabled";
constexpr const char* kLegacyLimiterEnabled = "limiter_enabled";
constexpr const char* kLegacyCollectionReverbEnabled = "reverb_enabled";

// Legacy parameter IDs — used for backward-compatible preset and state loading
constexpr const char* kLegacyDelayNoteDiv  = "delay_note_div"; // renamed to "delay_division"
constexpr const char* kLegacyReverbEnabled = "fx_reverb_en";   // renamed to "fx_tab7_en"

constexpr const char* kLimiterThreshold = "limiter_threshold";
constexpr const char* kLimiterRelease   = "limiter_release";

constexpr const char* kPianoHammerHardnessSuffix = "hammer_hardness";
constexpr const char* kPianoToneBrightnessSuffix = "tone_brightness";
constexpr const char* kPianoStringResonanceSuffix = "string_resonance";
constexpr const char* kPianoSoundboardAmountSuffix = "soundboard_amount";
constexpr const char* kPianoModelCharacterSuffix = "model_character";
constexpr const char* kPianoLowPassHzSuffix = "low_pass_hz";
constexpr const char* kPianoOutputSuffix = "output";

constexpr const char* kLegacyPianoHammerSuffix = "hammer";
constexpr const char* kLegacyPianoBrightnessSuffix = "brightness";
constexpr const char* kLegacyPianoStringResSuffix = "string_res";
constexpr const char* kLegacyPianoSoundboardSuffix = "soundboard";
constexpr const char* kLegacyPianoCharacterSuffix = "character";
constexpr const char* kLegacyPianoCutoffSuffix = "cutoff";

constexpr const char* kPresetPianoIndexAttr = "piano_index";
constexpr const char* kPresetHammerHardnessAttr = "hammer_hardness";
constexpr const char* kPresetToneBrightnessAttr = "tone_brightness";
constexpr const char* kPresetStringResonanceAttr = "string_resonance";
constexpr const char* kPresetSoundboardAmountAttr = "soundboard_amount";
constexpr const char* kPresetModelCharacterAttr = "model_character";
constexpr const char* kPresetLowPassHzAttr = "low_pass_hz";

constexpr const char* kLegacyPresetPianoIndexAttr = "piano";
constexpr const char* kLegacyPresetHammerAttr = "hammer";
constexpr const char* kLegacyPresetBrightnessAttr = "brightness";
constexpr const char* kLegacyPresetStringResAttr = "string_res";
constexpr const char* kLegacyPresetSoundboardAttr = "soundboard";
constexpr const char* kLegacyPresetCharacterAttr = "character";
constexpr const char* kLegacyPresetCutoffAttr = "cutoff";

constexpr const char* kFxCacheNodeType = "FX_CACHE";
constexpr const char* kFxCachePianoIndexAttr = "piano";
constexpr int kCurrentPresetFormatVersion = 5;
constexpr float kSustainHoldEnter = 0.70f;
constexpr float kSustainHoldExit  = 0.58f;

std::uint64_t splitMix64(std::uint64_t value)
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

std::int64_t makeSystemRandomSeed()
{
    auto& systemRandom = juce::Random::getSystemRandom();
    const auto hi = static_cast<std::uint64_t>(static_cast<std::uint32_t>(systemRandom.nextInt()));
    const auto lo = static_cast<std::uint64_t>(static_cast<std::uint32_t>(systemRandom.nextInt()));
    return static_cast<std::int64_t>((hi << 32) ^ lo ^ static_cast<std::uint64_t>(juce::Time::getHighResolutionTicks()));
}

juce::StringArray makeOutputChoices()
{
    juce::StringArray outputs;
    outputs.add("Master");
    for (int i = 0; i < PianoSynthAudioProcessor::kNumAuxOutputs; ++i)
        outputs.add("Out " + juce::String(i + 1));
    return outputs;
}

float clamp01(float v) { return juce::jlimit(0.0f, 1.0f, v); }

juce::File findWritableDirectory(const juce::File& preferred, const juce::String& fallbackRelative)
{
    auto tryDirectory = [](const juce::File& base) -> juce::File
    {
        auto dir = base;
        dir.createDirectory();
        if (!dir.isDirectory())
            return {};

        auto probe = dir.getNonexistentChildFile(".write_probe", ".tmp", false);
        if (probe.replaceWithText("ok"))
        {
            probe.deleteFile();
            return dir;
        }

        probe.deleteFile();
        return {};
    };

    if (auto dir = tryDirectory(preferred); dir != juce::File{})
        return dir;

    const auto tempFallback = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getChildFile(fallbackRelative);
    if (auto dir = tryDirectory(tempFallback); dir != juce::File{})
        return dir;

    auto cwdFallback = juce::File::getCurrentWorkingDirectory()
                           .getChildFile(".musique_user_data")
                           .getChildFile(fallbackRelative);
    cwdFallback.createDirectory();
    return cwdFallback;
}

bool readXmlBoolAttribute(const juce::XmlElement& xml,
                          const char* attrName,
                          bool fallback,
                          int& warningCount);
bool readXmlBoolAttributeWithLegacy(const juce::XmlElement& xml,
                                    const char* attrName,
                                    const char* legacyAttrName,
                                    bool fallback,
                                    int& warningCount);
void sanitizeFxSettings(mps::GlobalFxSettings& fx);

void logSanitizationWarnings(const juce::String& scope,
                             const juce::String& source,
                             const int warningCount)
{
    if (warningCount <= 0)
        return;

    juce::Logger::writeToLog("[" + scope + "] Sanitized " + juce::String(warningCount)
        + " fields in " + source);
}

std::optional<double> tryParseScalar(const juce::String& value)
{
    const auto raw = value.trim();
    if (raw.isEmpty())
        return std::nullopt;

    char* end = nullptr;
    const auto parsed = std::strtod(raw.toRawUTF8(), &end);
    if (end == raw.toRawUTF8() || end == nullptr || *end != '\0' || !std::isfinite(parsed))
        return std::nullopt;
    return parsed;
}

std::optional<double> tryParseScalar(const juce::var& value)
{
    return tryParseScalar(value.toString());
}

bool tryParseXmlDoubleAttribute(const juce::XmlElement& xml, const char* attrName, double& value)
{
    if (!xml.hasAttribute(attrName))
        return false;

    const auto raw = xml.getStringAttribute(attrName).trim();
    if (raw.isEmpty())
        return false;

    char* end = nullptr;
    const auto parsed = std::strtod(raw.toRawUTF8(), &end);
    if (end == raw.toRawUTF8() || end == nullptr || *end != '\0' || !std::isfinite(parsed))
        return false;

    value = parsed;
    return true;
}

float readValidatedXmlFloat(const juce::XmlElement& xml,
                            const char* attrName,
                            float fallback,
                            float minValue,
                            float maxValue,
                            int& warningCount)
{
    double parsed = 0.0;
    if (!tryParseXmlDoubleAttribute(xml, attrName, parsed))
    {
        if (xml.hasAttribute(attrName))
            ++warningCount;
        return fallback;
    }

    const auto clamped = juce::jlimit(minValue, maxValue, static_cast<float>(parsed));
    if (clamped != static_cast<float>(parsed))
        ++warningCount;
    return clamped;
}

float readValidatedXmlFloatWithLegacy(const juce::XmlElement& xml,
                                      const char* attrName,
                                      const char* legacyAttrName,
                                      float fallback,
                                      float minValue,
                                      float maxValue,
                                      int& warningCount)
{
    if (xml.hasAttribute(attrName))
        return readValidatedXmlFloat(xml, attrName, fallback, minValue, maxValue, warningCount);
    if (legacyAttrName != nullptr && xml.hasAttribute(legacyAttrName))
        return readValidatedXmlFloat(xml, legacyAttrName, fallback, minValue, maxValue, warningCount);
    return fallback;
}

int readValidatedXmlInt(const juce::XmlElement& xml,
                        const char* attrName,
                        int fallback,
                        int minValue,
                        int maxValue,
                        int& warningCount)
{
    double parsed = 0.0;
    if (!tryParseXmlDoubleAttribute(xml, attrName, parsed))
    {
        if (xml.hasAttribute(attrName))
            ++warningCount;
        return fallback;
    }

    const auto rounded = static_cast<int>(std::lround(parsed));
    const auto clamped = juce::jlimit(minValue, maxValue, rounded);
    if (clamped != rounded || std::abs(parsed - std::round(parsed)) > 1.0e-6)
        ++warningCount;
    return clamped;
}

int readValidatedXmlIntWithLegacy(const juce::XmlElement& xml,
                                  const char* attrName,
                                  const char* legacyAttrName,
                                  int fallback,
                                  int minValue,
                                  int maxValue,
                                  int& warningCount)
{
    if (xml.hasAttribute(attrName))
        return readValidatedXmlInt(xml, attrName, fallback, minValue, maxValue, warningCount);
    if (legacyAttrName != nullptr && xml.hasAttribute(legacyAttrName))
        return readValidatedXmlInt(xml, legacyAttrName, fallback, minValue, maxValue, warningCount);
    return fallback;
}

std::string readXmlStringAttribute(const juce::XmlElement& xml,
                                   const char* attrName,
                                   const std::string& fallback)
{
    return xml.getStringAttribute(attrName, juce::String(fallback)).toStdString();
}

float readValidatedTreeFloat(const juce::ValueTree& tree,
                             const char* propertyName,
                             float fallback,
                             float minValue,
                             float maxValue,
                             int& warningCount)
{
    if (auto parsed = tryParseScalar(tree.getProperty(propertyName, fallback)))
    {
        const auto clamped = juce::jlimit(minValue, maxValue, static_cast<float>(*parsed));
        if (std::abs(clamped - static_cast<float>(*parsed)) > 1.0e-6f)
            ++warningCount;
        return clamped;
    }

    if (tree.hasProperty(propertyName))
        ++warningCount;
    return fallback;
}

int readValidatedTreeInt(const juce::ValueTree& tree,
                         const char* propertyName,
                         int fallback,
                         int minValue,
                         int maxValue,
                         int& warningCount)
{
    if (auto parsed = tryParseScalar(tree.getProperty(propertyName, fallback)))
    {
        const auto rounded = static_cast<int>(std::lround(*parsed));
        const auto clamped = juce::jlimit(minValue, maxValue, rounded);
        if (clamped != rounded || std::abs(*parsed - std::round(*parsed)) > 1.0e-6)
            ++warningCount;
        return clamped;
    }

    if (tree.hasProperty(propertyName))
        ++warningCount;
    return fallback;
}

bool readValidatedTreeBool(const juce::ValueTree& tree,
                           const char* propertyName,
                           bool fallback,
                           int& warningCount)
{
    if (auto parsed = tryParseScalar(tree.getProperty(propertyName, fallback ? 1 : 0)))
        return std::lround(*parsed) != 0;

    if (tree.hasProperty(propertyName))
        ++warningCount;
    return fallback;
}

float getDoubleAttributeWithLegacy(const juce::XmlElement& xml,
                                   const char* currentName,
                                   const char* legacyName,
                                   const float fallback)
{
    if (xml.hasAttribute(currentName))
        return static_cast<float>(xml.getDoubleAttribute(currentName, fallback));
    if (legacyName != nullptr && xml.hasAttribute(legacyName))
        return static_cast<float>(xml.getDoubleAttribute(legacyName, fallback));
    return fallback;
}

juce::String migrateLegacyPianoParamId(const juce::String& paramId)
{
    if (!paramId.startsWith("piano_"))
        return paramId;

    // Match the complete suffix after piano_<index>_. Current tone_brightness
    // and model_character must not be migrated again as legacy suffixes.
    const int separator = paramId.indexOfChar(6, '_');
    if (separator < 0)
        return paramId;
    const auto prefix = paramId.substring(0, separator + 1);
    const auto suffix = paramId.substring(separator + 1);
    if (suffix == kLegacyPianoHammerSuffix)
        return prefix + kPianoHammerHardnessSuffix;
    if (suffix == kLegacyPianoBrightnessSuffix)
        return prefix + kPianoToneBrightnessSuffix;
    if (suffix == kLegacyPianoStringResSuffix)
        return prefix + kPianoStringResonanceSuffix;
    if (suffix == kLegacyPianoSoundboardSuffix)
        return prefix + kPianoSoundboardAmountSuffix;
    if (suffix == kLegacyPianoCharacterSuffix)
        return prefix + kPianoModelCharacterSuffix;
    if (suffix == kLegacyPianoCutoffSuffix)
        return prefix + kPianoLowPassHzSuffix;
    return paramId;
}

void migrateLegacyStateXml(juce::XmlElement& element)
{
    if (element.hasAttribute("id"))
    {
        const juce::String originalId = element.getStringAttribute("id");
        element.setAttribute("id", migrateLegacyPianoParamId(originalId));

        // Delay note division: rename and remap index from old 5-division order to new 6-division order
        // Old: 0=1/4, 1=1/8, 2=1/16, 3=Dotted1/8, 4=Triplet1/8
        // New: 0=1/4, 1=1/8, 2=Dotted1/8, 3=Triplet1/8, 4=1/16, 5=Dotted1/16
        if (originalId == kLegacyDelayNoteDiv)
        {
            element.setAttribute("id", kDelayNoteDiv);
            if (element.hasAttribute("value"))
            {
                static constexpr int kDivRemap[5] = { 0, 1, 4, 2, 3 };
                const int oldVal = juce::jlimit(0, 4, element.getIntAttribute("value", 0));
                element.setAttribute("value", kDivRemap[oldVal]);
            }
        }
        // Reverb enable: rename "fx_reverb_en" → "fx_tab7_en"
        else if (originalId == kLegacyReverbEnabled)
        {
            element.setAttribute("id", kReverbEnabled);
        }
    }

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
        migrateLegacyStateXml(*child);
}

bool isLegacyPresetStorage(const juce::XmlElement& xml)
{
    const auto formatVersion = xml.getIntAttribute("format_version", 0);
    return formatVersion < kCurrentPresetFormatVersion
        || xml.hasAttribute(kLegacyPresetPianoIndexAttr)
        || xml.hasAttribute(kLegacyPresetHammerAttr)
        || xml.hasAttribute(kLegacyPresetBrightnessAttr)
        || xml.hasAttribute(kLegacyPresetStringResAttr)
        || xml.hasAttribute(kLegacyPresetSoundboardAttr)
        || xml.hasAttribute(kLegacyPresetCharacterAttr)
        || xml.hasAttribute(kLegacyPresetCutoffAttr);
}

bool hasAttributes(const juce::XmlElement& xml, std::initializer_list<const char*> names)
{
    for (const auto* name : names)
    {
        if (!xml.hasAttribute(name))
            return false;
    }
    return true;
}

modmatrix::MatrixState materializeModMatrixState(const mps::PresetPerformanceState& performance)
{
    auto state = performance.modMatrixState;
    state.pitchBendRange = juce::jlimit(1, 24, juce::roundToInt(performance.pitchBendRange));
    state.lfo2Rate = std::isfinite(state.lfo2Rate) ? juce::jmax(0.01f, state.lfo2Rate) : 2.0f;
    state.lfo2Wave = juce::jlimit(0, 3, state.lfo2Wave);
    for (auto& slot : state.slots)
    {
        slot.amount = juce::jlimit(-1.0f, 1.0f, slot.amount);
        if (slot.source == modmatrix::Source::None || slot.destination == modmatrix::Destination::None)
        {
            slot.source = modmatrix::Source::None;
            slot.destination = modmatrix::Destination::None;
            slot.amount = 0.0f;
        }
    }
    return state;
}

void writeCompleteModMatrixXml(juce::XmlElement& parent, const mps::PresetPerformanceState& performance)
{
    const auto state = materializeModMatrixState(performance);
    auto* matEl = parent.createNewChildElement("ModMatrix");
    matEl->setAttribute("pbRange", state.pitchBendRange);
    matEl->setAttribute("lfo2Rate", static_cast<double>(state.lfo2Rate));
    matEl->setAttribute("lfo2Wave", state.lfo2Wave);

    for (int slotIndex = 0; slotIndex < modmatrix::kMaxSlots; ++slotIndex)
    {
        const auto& slot = state.slots[static_cast<std::size_t>(slotIndex)];
        auto* slotEl = matEl->createNewChildElement("Slot");
        slotEl->setAttribute("idx", slotIndex);
        slotEl->setAttribute("src", static_cast<int>(slot.source));
        slotEl->setAttribute("dst", static_cast<int>(slot.destination));
        slotEl->setAttribute("amt", static_cast<double>(slot.amount));
    }
}

bool hasCanonicalModMatrixXml(const juce::XmlElement& parent)
{
    auto* matEl = parent.getChildByName("ModMatrix");
    if (matEl == nullptr || !hasAttributes(*matEl, { "pbRange", "lfo2Rate", "lfo2Wave" }))
        return false;

    std::array<bool, modmatrix::kMaxSlots> seen {};
    int slotCount = 0;
    for (auto* slotEl : matEl->getChildWithTagNameIterator("Slot"))
    {
        if (!hasAttributes(*slotEl, { "idx", "src", "dst", "amt" }))
            return false;

        const auto slotIndex = slotEl->getIntAttribute("idx", -1);
        if (slotIndex < 0 || slotIndex >= modmatrix::kMaxSlots || seen[static_cast<std::size_t>(slotIndex)])
            return false;

        seen[static_cast<std::size_t>(slotIndex)] = true;
        ++slotCount;
    }

    return slotCount == modmatrix::kMaxSlots
        && std::all_of(seen.begin(), seen.end(), [] (bool present) { return present; });
}

bool hasCanonicalPresetSchema(const juce::XmlElement& xml)
{
    if (!hasAttributes(xml, {
            "name",
            "format_version",
            "synth_index",
            "instrument_index",
            kPresetPianoIndexAttr,
            "level",
            "tune",
            kPresetHammerHardnessAttr,
            "attack",
            "decay",
            "sustain",
            "release",
            kPresetToneBrightnessAttr,
            kPresetStringResonanceAttr,
            kPresetSoundboardAmountAttr,
            "damping",
            kPresetModelCharacterAttr,
            kPresetLowPassHzAttr,
            "pan",
            "output",
            kMacroWarmth,
            kMacroBrillance,
            kMacroExpression,
            kMacroResonance,
            kLfoRate,
            kLfoDepth,
            kLfoWave,
            kLfoDestination,
            kPitchBendRange,
            kVelocityCurve,
            kMonoMode,
            kTremoloSync,
            kModWheelTarget,
            "intent",
            "tags",
            "family",
            "mix_role",
            kReverbEnabled,
            kSaturationEnabled,
            kTransientEnabled,
            kCompressorEnabled,
            kEqEnabled,
            kChorusEnabled,
            kDelayEnabled,
            kLimiterEnabled,
            kSatDrive,
            kSatMix,
            kTransientAttack,
            kTransientSustain,
            kTransientMix,
            kCompThreshold,
            kCompRatio,
            kCompAttack,
            kCompRelease,
            kCompMakeup,
            kCompMix,
            kEqLowFreq,
            kEqLowGain,
            kEqMidFreq,
            kEqMidGain,
            kEqMidQ,
            kEqHighFreq,
            kEqHighGain,
            kChorusRate,
            kChorusDepth,
            kChorusMix,
            kDelayTime,
            kDelayFeedback,
            kDelayMix,
            kDelaySync,
            kDelayNoteDiv,
            kReverbSize,
            kReverbDamping,
            kReverbWidth,
            kReverbMix,
            kReverbPreDelay,
            kLimiterThreshold,
            kLimiterRelease }))
    {
        return false;
    }

    if (xml.hasTagName("PianoFactoryPreset") && !xml.hasAttribute("preset_index"))
        return false;

    return hasCanonicalModMatrixXml(xml);
}

bool shouldRewritePresetXml(const juce::XmlElement& xml)
{
    return isLegacyPresetStorage(xml) || !hasCanonicalPresetSchema(xml);
}

void writeFxXmlAttributes(juce::XmlElement& xml, const mps::GlobalFxSettings& fx)
{
    xml.setAttribute(kReverbEnabled, fx.reverbEnabled ? 1 : 0);
    xml.setAttribute(kLegacyReverbEnabled, fx.reverbEnabled ? 1 : 0);
    xml.setAttribute(kReverbType, fx.reverbType);
    xml.setAttribute(kSaturationEnabled, fx.saturationEnabled ? 1 : 0);
    xml.setAttribute(kTransientEnabled, fx.transientEnabled ? 1 : 0);
    xml.setAttribute(kCompressorEnabled, fx.compressorEnabled ? 1 : 0);
    xml.setAttribute(kEqEnabled, fx.eqEnabled ? 1 : 0);
    xml.setAttribute(kChorusEnabled, fx.chorusEnabled ? 1 : 0);
    xml.setAttribute(kDelayEnabled, fx.delayEnabled ? 1 : 0);
    xml.setAttribute(kLimiterEnabled, fx.limiterEnabled ? 1 : 0);
    xml.setAttribute(kSatDrive, static_cast<double>(fx.satDrive));
    xml.setAttribute(kSatMix, static_cast<double>(fx.satMix));
    xml.setAttribute(kTransientAttack, static_cast<double>(fx.transientAttack));
    xml.setAttribute(kTransientSustain, static_cast<double>(fx.transientSustain));
    xml.setAttribute(kTransientMix, static_cast<double>(fx.transientMix));
    xml.setAttribute(kCompThreshold, static_cast<double>(fx.compThreshold));
    xml.setAttribute(kCompRatio, static_cast<double>(fx.compRatio));
    xml.setAttribute(kCompAttack, static_cast<double>(fx.compAttack));
    xml.setAttribute(kCompRelease, static_cast<double>(fx.compRelease));
    xml.setAttribute(kCompMakeup, static_cast<double>(fx.compMakeup));
    xml.setAttribute(kCompMix, static_cast<double>(fx.compMix));
    xml.setAttribute(kEqLowFreq, static_cast<double>(fx.eqLowFreq));
    xml.setAttribute(kEqLowGain, static_cast<double>(fx.eqLowGain));
    xml.setAttribute(kEqMidFreq, static_cast<double>(fx.eqMidFreq));
    xml.setAttribute(kEqMidGain, static_cast<double>(fx.eqMidGain));
    xml.setAttribute(kEqMidQ, static_cast<double>(fx.eqMidQ));
    xml.setAttribute(kEqHighFreq, static_cast<double>(fx.eqHighFreq));
    xml.setAttribute(kEqHighGain, static_cast<double>(fx.eqHighGain));
    xml.setAttribute(kChorusRate, static_cast<double>(fx.chorusRate));
    xml.setAttribute(kChorusDepth, static_cast<double>(fx.chorusDepth));
    xml.setAttribute(kChorusMix, static_cast<double>(fx.chorusMix));
    xml.setAttribute(kDelayTime, static_cast<double>(fx.delayTime));
    xml.setAttribute(kDelayFeedback, static_cast<double>(fx.delayFeedback));
    xml.setAttribute(kDelayMix, static_cast<double>(fx.delayMix));
    xml.setAttribute(kDelaySync, fx.delaySync ? 1 : 0);
    xml.setAttribute(kDelayNoteDiv, fx.delayNoteDivision);
    xml.setAttribute(kLegacyDelayNoteDiv, fx.delayNoteDivision);
    xml.setAttribute(kReverbSize, static_cast<double>(fx.reverbSize));
    xml.setAttribute(kReverbDamping, static_cast<double>(fx.reverbDamping));
    xml.setAttribute(kReverbWidth, static_cast<double>(fx.reverbWidth));
    xml.setAttribute(kReverbMix, static_cast<double>(fx.reverbMix));
    xml.setAttribute(kReverbPreDelay, static_cast<double>(fx.reverbPredelay));
    xml.setAttribute(kLimiterThreshold, static_cast<double>(fx.limiterThreshold));
    xml.setAttribute(kLimiterRelease, static_cast<double>(fx.limiterRelease));
}

void readFxXmlAttributes(const juce::XmlElement& xml, mps::GlobalFxSettings& fx)
{
    int warningCount = 0;
    fx.reverbEnabled = readXmlBoolAttributeWithLegacy(xml, kReverbEnabled, kLegacyCollectionReverbEnabled,
                                                      fx.reverbEnabled, warningCount);
    if (xml.hasAttribute(kReverbType))
        fx.reverbType = readValidatedXmlInt(xml, kReverbType, fx.reverbType, 0, 3, warningCount);
    fx.saturationEnabled = readXmlBoolAttributeWithLegacy(xml, kSaturationEnabled, kLegacySaturationEnabled,
                                                          fx.saturationEnabled, warningCount);
    fx.transientEnabled = readXmlBoolAttributeWithLegacy(xml, kTransientEnabled, kLegacyTransientEnabled,
                                                         fx.transientEnabled, warningCount);
    fx.compressorEnabled = readXmlBoolAttributeWithLegacy(xml, kCompressorEnabled, kLegacyCompressorEnabled,
                                                          fx.compressorEnabled, warningCount);
    fx.eqEnabled = readXmlBoolAttributeWithLegacy(xml, kEqEnabled, kLegacyEqEnabled,
                                                  fx.eqEnabled, warningCount);
    fx.chorusEnabled = readXmlBoolAttributeWithLegacy(xml, kChorusEnabled, kLegacyChorusEnabled,
                                                      fx.chorusEnabled, warningCount);
    fx.delayEnabled = readXmlBoolAttributeWithLegacy(xml, kDelayEnabled, kLegacyDelayEnabled,
                                                     fx.delayEnabled, warningCount);
    fx.limiterEnabled = readXmlBoolAttributeWithLegacy(xml, kLimiterEnabled, kLegacyLimiterEnabled,
                                                       fx.limiterEnabled, warningCount);
    fx.satDrive = readValidatedXmlFloat(xml, kSatDrive, fx.satDrive, 1.0f, 16.0f, warningCount);
    fx.satMix = readValidatedXmlFloat(xml, kSatMix, fx.satMix, 0.0f, 1.0f, warningCount);
    fx.transientAttack = readValidatedXmlFloat(xml, kTransientAttack, fx.transientAttack, -1.0f, 1.0f, warningCount);
    fx.transientSustain = readValidatedXmlFloat(xml, kTransientSustain, fx.transientSustain, -1.0f, 1.0f, warningCount);
    fx.transientMix = readValidatedXmlFloat(xml, kTransientMix, fx.transientMix, 0.0f, 1.0f, warningCount);
    fx.compThreshold = readValidatedXmlFloat(xml, kCompThreshold, fx.compThreshold, -60.0f, 0.0f, warningCount);
    fx.compRatio = readValidatedXmlFloat(xml, kCompRatio, fx.compRatio, 1.0f, 20.0f, warningCount);
    fx.compAttack = readValidatedXmlFloat(xml, kCompAttack, fx.compAttack, 0.1f, 100.0f, warningCount);
    fx.compRelease = readValidatedXmlFloat(xml, kCompRelease, fx.compRelease, 5.0f, 500.0f, warningCount);
    fx.compMakeup = readValidatedXmlFloat(xml, kCompMakeup, fx.compMakeup, 0.0f, 24.0f, warningCount);
    fx.compMix = readValidatedXmlFloat(xml, kCompMix, fx.compMix, 0.0f, 1.0f, warningCount);
    fx.eqLowFreq = readValidatedXmlFloat(xml, kEqLowFreq, fx.eqLowFreq, 20.0f, 500.0f, warningCount);
    fx.eqLowGain = readValidatedXmlFloat(xml, kEqLowGain, fx.eqLowGain, -12.0f, 12.0f, warningCount);
    fx.eqMidFreq = readValidatedXmlFloat(xml, kEqMidFreq, fx.eqMidFreq, 200.0f, 8000.0f, warningCount);
    fx.eqMidGain = readValidatedXmlFloat(xml, kEqMidGain, fx.eqMidGain, -12.0f, 12.0f, warningCount);
    fx.eqMidQ = readValidatedXmlFloat(xml, kEqMidQ, fx.eqMidQ, 0.1f, 10.0f, warningCount);
    fx.eqHighFreq = readValidatedXmlFloat(xml, kEqHighFreq, fx.eqHighFreq, 2000.0f, 20000.0f, warningCount);
    fx.eqHighGain = readValidatedXmlFloat(xml, kEqHighGain, fx.eqHighGain, -12.0f, 12.0f, warningCount);
    fx.chorusRate = readValidatedXmlFloat(xml, kChorusRate, fx.chorusRate, 0.1f, 5.0f, warningCount);
    fx.chorusDepth = readValidatedXmlFloat(xml, kChorusDepth, fx.chorusDepth, 0.0f, 1.0f, warningCount);
    fx.chorusMix = readValidatedXmlFloat(xml, kChorusMix, fx.chorusMix, 0.0f, 1.0f, warningCount);
    fx.delayTime = readValidatedXmlFloat(xml, kDelayTime, fx.delayTime, 1.0f, 2000.0f, warningCount);
    fx.delayFeedback = readValidatedXmlFloat(xml, kDelayFeedback, fx.delayFeedback, 0.0f, 0.95f, warningCount);
    fx.delayMix = readValidatedXmlFloat(xml, kDelayMix, fx.delayMix, 0.0f, 1.0f, warningCount);
    fx.delaySync = readXmlBoolAttribute(xml, kDelaySync, fx.delaySync, warningCount);
    if (xml.hasAttribute(kDelayNoteDiv))
    {
        fx.delayNoteDivision = readValidatedXmlInt(xml, kDelayNoteDiv, fx.delayNoteDivision, 0, 5, warningCount);
    }
    else
    {
        // Legacy: "delay_note_div" with old 5-division order → remap to new 6-division order
        const int legacyDiv = readValidatedXmlInt(xml, kLegacyDelayNoteDiv, fx.delayNoteDivision, 0, 4, warningCount);
        static constexpr int kDivRemap[5] = { 0, 1, 4, 2, 3 };
        fx.delayNoteDivision = kDivRemap[juce::jlimit(0, 4, legacyDiv)];
    }
    fx.reverbSize = readValidatedXmlFloat(xml, kReverbSize, fx.reverbSize, 0.0f, 1.0f, warningCount);
    fx.reverbDamping = readValidatedXmlFloat(xml, kReverbDamping, fx.reverbDamping, 0.0f, 1.0f, warningCount);
    fx.reverbWidth = readValidatedXmlFloat(xml, kReverbWidth, fx.reverbWidth, 0.0f, 1.0f, warningCount);
    fx.reverbMix = readValidatedXmlFloat(xml, kReverbMix, fx.reverbMix, 0.0f, 1.0f, warningCount);
    fx.reverbPredelay = readValidatedXmlFloat(xml, kReverbPreDelay, fx.reverbPredelay, 0.0f, 100.0f, warningCount);
    fx.limiterThreshold = readValidatedXmlFloat(xml, kLimiterThreshold, fx.limiterThreshold, -12.0f, 0.0f, warningCount);
    fx.limiterRelease = readValidatedXmlFloat(xml, kLimiterRelease, fx.limiterRelease, 1.0f, 200.0f, warningCount);
    sanitizeFxSettings(fx);

    if (warningCount > 0)
        juce::Logger::writeToLog("[PianoPreset] FX sanitization warnings=" + juce::String(warningCount));
}

bool readXmlBoolAttribute(const juce::XmlElement& xml,
                          const char* attrName,
                          const bool fallback,
                          int& warningCount)
{
    auto parsed = tryParseScalar(xml.getStringAttribute(attrName));
    if (!parsed.has_value())
    {
        if (xml.hasAttribute(attrName))
            ++warningCount;
        return fallback;
    }

    const auto rounded = static_cast<int>(std::lround(*parsed));
    if (std::abs(*parsed - std::round(*parsed)) > 1.0e-6)
        ++warningCount;
    return rounded != 0;
}

bool readXmlBoolAttributeWithLegacy(const juce::XmlElement& xml,
                                    const char* attrName,
                                    const char* legacyAttrName,
                                    const bool fallback,
                                    int& warningCount)
{
    if (xml.hasAttribute(attrName))
        return readXmlBoolAttribute(xml, attrName, fallback, warningCount);
    if (legacyAttrName != nullptr && xml.hasAttribute(legacyAttrName))
        return readXmlBoolAttribute(xml, legacyAttrName, fallback, warningCount);
    return fallback;
}

int readPresetOutputBusAttribute(const juce::XmlElement& xml, const int fallback, int& warningCount)
{
    double parsed = 0.0;
    if (!tryParseXmlDoubleAttribute(xml, "output", parsed))
    {
        if (xml.hasAttribute("output"))
            ++warningCount;
        return fallback;
    }

    const auto rounded = std::round(parsed);
    if (std::abs(parsed - rounded) > 1.0e-4)
    {
        ++warningCount;
        return fallback;
    }

    return juce::jlimit(0, PianoSynthAudioProcessor::kNumAuxOutputs, static_cast<int>(rounded));
}

void sanitizeFxSettings(mps::GlobalFxSettings& fx)
{
    fx.satDrive = juce::jlimit(1.0f, 16.0f, fx.satDrive);
    fx.satMix = clamp01(fx.satMix);
    fx.transientAttack = juce::jlimit(-1.0f, 1.0f, fx.transientAttack);
    fx.transientSustain = juce::jlimit(-1.0f, 1.0f, fx.transientSustain);
    fx.transientMix = clamp01(fx.transientMix);
    fx.eqLowFreq = juce::jlimit(20.0f, 500.0f, fx.eqLowFreq);
    fx.eqLowGain = juce::jlimit(-12.0f, 12.0f, fx.eqLowGain);
    fx.eqMidFreq = juce::jlimit(200.0f, 8000.0f, fx.eqMidFreq);
    fx.eqMidGain = juce::jlimit(-12.0f, 12.0f, fx.eqMidGain);
    fx.eqMidQ = juce::jlimit(0.1f, 10.0f, fx.eqMidQ);
    fx.eqHighFreq = juce::jlimit(2000.0f, 20000.0f, fx.eqHighFreq);
    fx.eqHighGain = juce::jlimit(-12.0f, 12.0f, fx.eqHighGain);
    fx.compThreshold = juce::jlimit(-60.0f, 0.0f, fx.compThreshold);
    fx.compRatio = juce::jlimit(1.0f, 20.0f, fx.compRatio);
    fx.compAttack = juce::jlimit(0.1f, 100.0f, fx.compAttack);
    fx.compRelease = juce::jlimit(5.0f, 500.0f, fx.compRelease);
    fx.compMakeup = juce::jlimit(0.0f, 24.0f, fx.compMakeup);
    fx.compMix = clamp01(fx.compMix);
    fx.chorusRate = juce::jlimit(0.1f, 5.0f, fx.chorusRate);
    fx.chorusDepth = clamp01(fx.chorusDepth);
    fx.chorusMix = clamp01(fx.chorusMix);
    fx.delayTime = juce::jlimit(1.0f, 2000.0f, fx.delayTime);
    fx.delayFeedback = juce::jlimit(0.0f, 0.95f, fx.delayFeedback);
    fx.delayMix = clamp01(fx.delayMix);
    fx.delayNoteDivision = juce::jlimit(0, 5, fx.delayNoteDivision);
    fx.reverbType = juce::jlimit(0, 3, fx.reverbType);
    fx.reverbSize = clamp01(fx.reverbSize);
    fx.reverbDamping = clamp01(fx.reverbDamping);
    fx.reverbWidth = clamp01(fx.reverbWidth);
    fx.reverbMix = clamp01(fx.reverbMix);
    fx.reverbPredelay = juce::jlimit(0.0f, 100.0f, fx.reverbPredelay);
    fx.limiterThreshold = juce::jlimit(-12.0f, 0.0f, fx.limiterThreshold);
    fx.limiterRelease = juce::jlimit(1.0f, 200.0f, fx.limiterRelease);
}

void writeFxValueTree(juce::ValueTree& tree, const mps::GlobalFxSettings& fx)
{
    tree.setProperty(kReverbEnabled, fx.reverbEnabled, nullptr);
    tree.setProperty(kSaturationEnabled, fx.saturationEnabled, nullptr);
    tree.setProperty(kTransientEnabled, fx.transientEnabled, nullptr);
    tree.setProperty(kCompressorEnabled, fx.compressorEnabled, nullptr);
    tree.setProperty(kEqEnabled, fx.eqEnabled, nullptr);
    tree.setProperty(kChorusEnabled, fx.chorusEnabled, nullptr);
    tree.setProperty(kDelayEnabled, fx.delayEnabled, nullptr);
    tree.setProperty(kLimiterEnabled, fx.limiterEnabled, nullptr);
    tree.setProperty(kSatDrive, fx.satDrive, nullptr);
    tree.setProperty(kSatMix, fx.satMix, nullptr);
    tree.setProperty(kTransientAttack, fx.transientAttack, nullptr);
    tree.setProperty(kTransientSustain, fx.transientSustain, nullptr);
    tree.setProperty(kTransientMix, fx.transientMix, nullptr);
    tree.setProperty(kCompThreshold, fx.compThreshold, nullptr);
    tree.setProperty(kCompRatio, fx.compRatio, nullptr);
    tree.setProperty(kCompAttack, fx.compAttack, nullptr);
    tree.setProperty(kCompRelease, fx.compRelease, nullptr);
    tree.setProperty(kCompMakeup, fx.compMakeup, nullptr);
    tree.setProperty(kCompMix, fx.compMix, nullptr);
    tree.setProperty(kEqLowFreq, fx.eqLowFreq, nullptr);
    tree.setProperty(kEqLowGain, fx.eqLowGain, nullptr);
    tree.setProperty(kEqMidFreq, fx.eqMidFreq, nullptr);
    tree.setProperty(kEqMidGain, fx.eqMidGain, nullptr);
    tree.setProperty(kEqMidQ, fx.eqMidQ, nullptr);
    tree.setProperty(kEqHighFreq, fx.eqHighFreq, nullptr);
    tree.setProperty(kEqHighGain, fx.eqHighGain, nullptr);
    tree.setProperty(kChorusRate, fx.chorusRate, nullptr);
    tree.setProperty(kChorusDepth, fx.chorusDepth, nullptr);
    tree.setProperty(kChorusMix, fx.chorusMix, nullptr);
    tree.setProperty(kDelayTime, fx.delayTime, nullptr);
    tree.setProperty(kDelayFeedback, fx.delayFeedback, nullptr);
    tree.setProperty(kDelayMix, fx.delayMix, nullptr);
    tree.setProperty(kDelaySync, fx.delaySync, nullptr);
    tree.setProperty(kDelayNoteDiv, fx.delayNoteDivision, nullptr);
    tree.setProperty(kReverbType, fx.reverbType, nullptr);
    tree.setProperty(kReverbSize, fx.reverbSize, nullptr);
    tree.setProperty(kReverbDamping, fx.reverbDamping, nullptr);
    tree.setProperty(kReverbWidth, fx.reverbWidth, nullptr);
    tree.setProperty(kReverbMix, fx.reverbMix, nullptr);
    tree.setProperty(kReverbPreDelay, fx.reverbPredelay, nullptr);
    tree.setProperty(kLimiterThreshold, fx.limiterThreshold, nullptr);
    tree.setProperty(kLimiterRelease, fx.limiterRelease, nullptr);
}

void readFxValueTree(const juce::ValueTree& tree, mps::GlobalFxSettings& fx, int& warningCount)
{
    fx.reverbEnabled = readValidatedTreeBool(tree, kReverbEnabled, fx.reverbEnabled, warningCount);
    fx.saturationEnabled = readValidatedTreeBool(tree, kSaturationEnabled, fx.saturationEnabled, warningCount);
    fx.transientEnabled = readValidatedTreeBool(tree, kTransientEnabled, fx.transientEnabled, warningCount);
    fx.compressorEnabled = readValidatedTreeBool(tree, kCompressorEnabled, fx.compressorEnabled, warningCount);
    fx.eqEnabled = readValidatedTreeBool(tree, kEqEnabled, fx.eqEnabled, warningCount);
    fx.chorusEnabled = readValidatedTreeBool(tree, kChorusEnabled, fx.chorusEnabled, warningCount);
    fx.delayEnabled = readValidatedTreeBool(tree, kDelayEnabled, fx.delayEnabled, warningCount);
    fx.limiterEnabled = readValidatedTreeBool(tree, kLimiterEnabled, fx.limiterEnabled, warningCount);
    fx.satDrive = readValidatedTreeFloat(tree, kSatDrive, fx.satDrive, 1.0f, 16.0f, warningCount);
    fx.satMix = readValidatedTreeFloat(tree, kSatMix, fx.satMix, 0.0f, 1.0f, warningCount);
    fx.transientAttack = readValidatedTreeFloat(tree, kTransientAttack, fx.transientAttack, -1.0f, 1.0f, warningCount);
    fx.transientSustain = readValidatedTreeFloat(tree, kTransientSustain, fx.transientSustain, -1.0f, 1.0f, warningCount);
    fx.transientMix = readValidatedTreeFloat(tree, kTransientMix, fx.transientMix, 0.0f, 1.0f, warningCount);
    fx.compThreshold = readValidatedTreeFloat(tree, kCompThreshold, fx.compThreshold, -60.0f, 0.0f, warningCount);
    fx.compRatio = readValidatedTreeFloat(tree, kCompRatio, fx.compRatio, 1.0f, 20.0f, warningCount);
    fx.compAttack = readValidatedTreeFloat(tree, kCompAttack, fx.compAttack, 0.1f, 100.0f, warningCount);
    fx.compRelease = readValidatedTreeFloat(tree, kCompRelease, fx.compRelease, 5.0f, 500.0f, warningCount);
    fx.compMakeup = readValidatedTreeFloat(tree, kCompMakeup, fx.compMakeup, 0.0f, 24.0f, warningCount);
    fx.compMix = readValidatedTreeFloat(tree, kCompMix, fx.compMix, 0.0f, 1.0f, warningCount);
    fx.eqLowFreq = readValidatedTreeFloat(tree, kEqLowFreq, fx.eqLowFreq, 20.0f, 500.0f, warningCount);
    fx.eqLowGain = readValidatedTreeFloat(tree, kEqLowGain, fx.eqLowGain, -12.0f, 12.0f, warningCount);
    fx.eqMidFreq = readValidatedTreeFloat(tree, kEqMidFreq, fx.eqMidFreq, 200.0f, 8000.0f, warningCount);
    fx.eqMidGain = readValidatedTreeFloat(tree, kEqMidGain, fx.eqMidGain, -12.0f, 12.0f, warningCount);
    fx.eqMidQ = readValidatedTreeFloat(tree, kEqMidQ, fx.eqMidQ, 0.1f, 10.0f, warningCount);
    fx.eqHighFreq = readValidatedTreeFloat(tree, kEqHighFreq, fx.eqHighFreq, 2000.0f, 20000.0f, warningCount);
    fx.eqHighGain = readValidatedTreeFloat(tree, kEqHighGain, fx.eqHighGain, -12.0f, 12.0f, warningCount);
    fx.chorusRate = readValidatedTreeFloat(tree, kChorusRate, fx.chorusRate, 0.1f, 5.0f, warningCount);
    fx.chorusDepth = readValidatedTreeFloat(tree, kChorusDepth, fx.chorusDepth, 0.0f, 1.0f, warningCount);
    fx.chorusMix = readValidatedTreeFloat(tree, kChorusMix, fx.chorusMix, 0.0f, 1.0f, warningCount);
    fx.delayTime = readValidatedTreeFloat(tree, kDelayTime, fx.delayTime, 1.0f, 2000.0f, warningCount);
    fx.delayFeedback = readValidatedTreeFloat(tree, kDelayFeedback, fx.delayFeedback, 0.0f, 0.95f, warningCount);
    fx.delayMix = readValidatedTreeFloat(tree, kDelayMix, fx.delayMix, 0.0f, 1.0f, warningCount);
    fx.delaySync = readValidatedTreeBool(tree, kDelaySync, fx.delaySync, warningCount);
    if (tree.hasProperty(kDelayNoteDiv))
    {
        fx.delayNoteDivision = readValidatedTreeInt(tree, kDelayNoteDiv, fx.delayNoteDivision, 0, 5, warningCount);
    }
    else
    {
        // Legacy: "delay_note_div" with old 5-division order → remap to new 6-division order
        const int legacyDiv = readValidatedTreeInt(tree, kLegacyDelayNoteDiv, fx.delayNoteDivision, 0, 4, warningCount);
        static constexpr int kDivRemap[5] = { 0, 1, 4, 2, 3 };
        fx.delayNoteDivision = kDivRemap[juce::jlimit(0, 4, legacyDiv)];
    }
    if (tree.hasProperty(kReverbType))
        fx.reverbType = readValidatedTreeInt(tree, kReverbType, fx.reverbType, 0, 3, warningCount);
    fx.reverbSize = readValidatedTreeFloat(tree, kReverbSize, fx.reverbSize, 0.0f, 1.0f, warningCount);
    fx.reverbDamping = readValidatedTreeFloat(tree, kReverbDamping, fx.reverbDamping, 0.0f, 1.0f, warningCount);
    fx.reverbWidth = readValidatedTreeFloat(tree, kReverbWidth, fx.reverbWidth, 0.0f, 1.0f, warningCount);
    fx.reverbMix = readValidatedTreeFloat(tree, kReverbMix, fx.reverbMix, 0.0f, 1.0f, warningCount);
    fx.reverbPredelay = readValidatedTreeFloat(tree, kReverbPreDelay, fx.reverbPredelay, 0.0f, 100.0f, warningCount);
    fx.limiterThreshold = readValidatedTreeFloat(tree, kLimiterThreshold, fx.limiterThreshold, -12.0f, 0.0f, warningCount);
    fx.limiterRelease = readValidatedTreeFloat(tree, kLimiterRelease, fx.limiterRelease, 1.0f, 200.0f, warningCount);
    sanitizeFxSettings(fx);
}

std::unique_ptr<juce::XmlElement> createPresetXml(const juce::String& tagName,
                                                  const juce::String& name,
                                                  const int pianoIndex,
                                                  const mps::PianoSettings& settings,
                                                  const mps::PresetPerformanceState& performance,
                                                  const mps::GlobalFxSettings& fx,
                                                  const int outputBus,
                                                  const mps::PresetMetadata* metadata = nullptr,
                                                  std::optional<int> presetIndex = std::nullopt)
{
    auto root = std::make_unique<juce::XmlElement>(tagName);
    root->setAttribute("name", name);
    root->setAttribute("format_version", kCurrentPresetFormatVersion);
    root->setAttribute("synth_index", 2);
    root->setAttribute("instrument_index", pianoIndex);
    root->setAttribute(kPresetPianoIndexAttr, pianoIndex);
    if (presetIndex.has_value())
        root->setAttribute("preset_index", *presetIndex);
    root->setAttribute("level", static_cast<double>(settings.level));
    root->setAttribute("tune", static_cast<double>(settings.tuneSemitones));
    root->setAttribute(kPresetHammerHardnessAttr, static_cast<double>(settings.tone.hammerHardness));
    root->setAttribute("attack", static_cast<double>(settings.envelope.attackSeconds));
    root->setAttribute("decay", static_cast<double>(settings.envelope.decaySeconds));
    root->setAttribute("sustain", static_cast<double>(settings.envelope.sustainLevel));
    root->setAttribute("release", static_cast<double>(settings.envelope.releaseSeconds));
    root->setAttribute(kPresetToneBrightnessAttr, static_cast<double>(settings.tone.brightness));
    root->setAttribute(kPresetStringResonanceAttr, static_cast<double>(settings.resonance.stringResonance));
    root->setAttribute(kPresetSoundboardAmountAttr, static_cast<double>(settings.resonance.soundboardAmount));
    root->setAttribute("damping", static_cast<double>(settings.resonance.damping));
    root->setAttribute(kPresetModelCharacterAttr, static_cast<double>(settings.performance.modelCharacter));
    root->setAttribute(kPresetLowPassHzAttr, static_cast<double>(settings.tone.lowPassHz));
    root->setAttribute("pan", static_cast<double>(settings.spatial.pan));
    root->setAttribute("output", outputBus);
    root->setAttribute(kMacroWarmth, static_cast<double>(performance.macroWarmth));
    root->setAttribute(kMacroBrillance, static_cast<double>(performance.macroBrillance));
    root->setAttribute(kMacroExpression, static_cast<double>(performance.macroExpression));
    root->setAttribute(kMacroResonance, static_cast<double>(performance.macroResonance));
    root->setAttribute(kLfoRate, static_cast<double>(performance.lfoRate));
    root->setAttribute(kLfoDepth, static_cast<double>(performance.lfoDepth));
    root->setAttribute(kLfoWave, performance.lfoWave);
    root->setAttribute(kLfoDestination, static_cast<int>(performance.lfoDestination));
    root->setAttribute(kPitchBendRange, static_cast<double>(performance.pitchBendRange));
    root->setAttribute(kVelocityCurve, performance.velocityCurve);
    root->setAttribute(kMonoMode, performance.monoMode ? 1 : 0);
    root->setAttribute(kTremoloSync, performance.tremoloSync ? 1 : 0);
    root->setAttribute(kModWheelTarget, performance.modWheelTarget);
    root->setAttribute("intent", juce::String(metadata != nullptr ? metadata->intent : std::string {}));
    root->setAttribute("tags", juce::String(metadata != nullptr ? metadata->tags : std::string {}));
    root->setAttribute("family", juce::String(metadata != nullptr ? metadata->family : std::string {}));
    root->setAttribute("mix_role", juce::String(metadata != nullptr ? metadata->mixRole : std::string {}));
    writeFxXmlAttributes(*root, fx);
    writeCompleteModMatrixXml(*root, performance);
    return root;
}
} // namespace

// =============================================================================
// Bus layout
// =============================================================================
auto PianoSynthAudioProcessor::createBusLayout() -> BusesProperties
{
    BusesProperties buses;
    buses = buses.withOutput("Master", juce::AudioChannelSet::stereo(), true);
    for (int i = 0; i < kNumAuxOutputs; ++i)
        buses = buses.withOutput("Piano " + juce::String(i + 1) + " Out",
                                 juce::AudioChannelSet::stereo(), false);
    return buses;
}

bool PianoSynthAudioProcessor::isLfoDestinationAvailableForPiano(const int pianoIndex,
                                                                 const mps::LfoDestination destination) noexcept
{
    if (destination == mps::LfoDestination::Off
        || destination == mps::LfoDestination::Tremolo
        || destination == mps::LfoDestination::AutoPan)
        return true;

    if (destination == mps::LfoDestination::ChorusMotion)
        return mps::isFxAvailable(pianoIndex, mps::GlobalFxSlot::Chorus);

    return false;
}

mps::LfoDestination PianoSynthAudioProcessor::sanitizeLfoDestinationForPiano(const int pianoIndex,
                                                                             const mps::LfoDestination destination) noexcept
{
    return isLfoDestinationAvailableForPiano(pianoIndex, destination)
        ? destination
        : mps::LfoDestination::Off;
}

bool PianoSynthAudioProcessor::isInstrumentControlAvailableForPiano(const int pianoIndex,
                                                                    const juce::String& suffix) noexcept
{
    const auto safePianoIndex = juce::jlimit(0, mps::kNumPianos - 1, pianoIndex);
    if (mps::getFamily(safePianoIndex) != mps::Family::Electric)
        return true;

    if (suffix == kPianoStringResonanceSuffix || suffix == kPianoSoundboardAmountSuffix)
        return false;

    if (suffix == "damping")
        return safePianoIndex == 7;

    return true;
}

bool PianoSynthAudioProcessor::usesTempoSyncedTremolo(const int pianoIndex) noexcept
{
    return mps::getFamily(juce::jlimit(0, mps::kNumPianos - 1, pianoIndex)) == mps::Family::Electric;
}

// =============================================================================
// Constructor
// =============================================================================
PianoSynthAudioProcessor::PianoSynthAudioProcessor()
    : AudioProcessor(createBusLayout()),
      parameters(*this, &undoManager, juce::Identifier("MPS_PARAMS"), createParameterLayout()),
      factoryPresetBanks(mps::getFactoryPresetBanks())
{
    undoManager.setMaxNumberOfStoredUnits(100, 30);
    modWheelTargetRaw = parameters.getRawParameterValue(kModWheelTarget);
    currentPresetIndices.fill(0);
    currentUserPresetFiles.fill(juce::File{});

    // Initialize per-piano FX cache from first factory preset of each piano
    for (int p = 0; p < mps::kNumPianos; ++p)
        cachedFxPerPiano[static_cast<std::size_t>(p)] =
            mps::maskUnavailableFx(p, factoryPresetBanks[static_cast<std::size_t>(p)][0].fx);

    // Pre-allocate voice pool (avoids RT allocation in the audio thread)
    for (auto& slot : voices)
        for (int p = 0; p < mps::kNumPianos; ++p)
            slot.voiceBank[static_cast<std::size_t>(p)] = mps::createVoiceForPiano(p);

    parameters.addParameterListener(kSelectedPiano, this);
    parameters.addParameterListener(kLfoDestination, this);

    archiveLegacyPresetLibraryIfNeeded();
    loadFactoryOverrides();
    for (int p = 0; p < mps::kNumPianos; ++p)
    {
        const auto& bank = factoryPresetBanks[static_cast<std::size_t>(p)];
        if (!bank.empty())
        {
            applyPianoPresetSettings(p, bank[0].settings);
            setParamValue(makePianoParamId(p, kPianoOutputSuffix), static_cast<float>(bank[0].outputBus));
            cachedFxPerPiano[static_cast<std::size_t>(p)] = mps::maskUnavailableFx(p, bank[0].fx);
        }
    }

    const int bootPiano = getSelectedPianoIndex();
    const auto& bootBank = factoryPresetBanks[static_cast<std::size_t>(bootPiano)];
    if (!bootBank.empty())
        applyInstrumentPreset(bootPiano, bootBank[0], false);
    lastSelectedPiano = bootPiano;
}

PianoSynthAudioProcessor::~PianoSynthAudioProcessor()
{
    parameters.removeParameterListener(kLfoDestination, this);
    parameters.removeParameterListener(kSelectedPiano, this);
}

void PianoSynthAudioProcessor::parameterChanged(const juce::String& parameterID, const float newValue)
{
    if (suppressConditionalSanitization)
        return;

    if (parameterID == kSelectedPiano)
    {
        sanitizePianoDependentState(true, juce::roundToInt(newValue));
        return;
    }

    if (parameterID == kLfoDestination)
        sanitizePianoDependentState(true);
}

std::int64_t PianoSynthAudioProcessor::makeRandomSeed() const noexcept
{
    const auto counter = realtimeRandomCounter.fetch_add(0x9e3779b97f4a7c15ULL, std::memory_order_acq_rel);
    const auto mixed = splitMix64(counter ^ static_cast<std::uint64_t>(renderSeed));
    return static_cast<std::int64_t>(mixed & 0x7fffffffffffffffULL);
}

std::int64_t PianoSynthAudioProcessor::makeDeterministicVoiceSeed(const int slotIndex,
                                                                  const int pianoIndex,
                                                                  const int midiChannel,
                                                                  const int midiNote,
                                                                  const std::uint64_t noteOnOrder) const noexcept
{
    std::uint64_t seed = splitMix64(static_cast<std::uint64_t>(renderSeed));
    seed = splitMix64(seed ^ (static_cast<std::uint64_t>(slotIndex + 1) << 1));
    seed = splitMix64(seed ^ (static_cast<std::uint64_t>(pianoIndex + 1) << 9));
    seed = splitMix64(seed ^ (static_cast<std::uint64_t>(midiChannel) << 17));
    seed = splitMix64(seed ^ (static_cast<std::uint64_t>(midiNote) << 25));
    seed = splitMix64(seed ^ (noteOnOrder << 33));
    return static_cast<std::int64_t>(seed & 0x7fffffffffffffffULL);
}

void PianoSynthAudioProcessor::resetPedalNoiseRandomStream()
{
    if (deterministicRenderMode)
    {
        const auto pedalSeed = static_cast<std::int64_t>(
            splitMix64(static_cast<std::uint64_t>(renderSeed) ^ 0xd1b54a32d192ed03ULL) & 0x7fffffffffffffffULL);
        publishPedalNoiseRandomSeed(static_cast<std::uint64_t>(pedalSeed));
        return;
    }

    publishPedalNoiseRandomSeed(static_cast<std::uint64_t>(makeRandomSeed()));
}

void PianoSynthAudioProcessor::publishPedalNoiseRandomSeed(const std::uint64_t seed) noexcept
{
    pendingPedalNoiseSeed.store(seed != 0u ? seed : 0x6a09e667f3bcc909ULL, std::memory_order_release);
    pendingPedalNoiseSeedValid.store(true, std::memory_order_release);
}

void PianoSynthAudioProcessor::consumePendingPedalNoiseRandomSeed() noexcept
{
    if (!pendingPedalNoiseSeedValid.exchange(false, std::memory_order_acq_rel))
        return;

    const auto seed = pendingPedalNoiseSeed.load(std::memory_order_acquire);
    pedalNoiseRandomState = seed != 0u ? seed : 0x6a09e667f3bcc909ULL;
}

void PianoSynthAudioProcessor::refreshDeterministicRenderMode()
{
    const bool shouldBeDeterministic = offlinePresetMode || isNonRealtime();
    if (shouldBeDeterministic == deterministicRenderMode)
        return;

    deterministicRenderMode = shouldBeDeterministic;
    resetPedalNoiseRandomStream();
}

// =============================================================================
void PianoSynthAudioProcessor::applyPianoPresetSettings(int pianoIndex, const mps::PianoSettings& s)
{
    setParamValue(makePianoParamId(pianoIndex, "level"),       s.level);
    setParamValue(makePianoParamId(pianoIndex, "tune"),        s.tuneSemitones);
    setParamValue(makePianoParamId(pianoIndex, kPianoHammerHardnessSuffix),      s.tone.hammerHardness);
    setParamValue(makePianoParamId(pianoIndex, "attack"),      s.envelope.attackSeconds);
    setParamValue(makePianoParamId(pianoIndex, "decay"),       s.envelope.decaySeconds);
    setParamValue(makePianoParamId(pianoIndex, "sustain"),     s.envelope.sustainLevel);
    setParamValue(makePianoParamId(pianoIndex, "release"),     s.envelope.releaseSeconds);
    setParamValue(makePianoParamId(pianoIndex, kPianoToneBrightnessSuffix),  s.tone.brightness);
    setParamValue(makePianoParamId(pianoIndex, kPianoStringResonanceSuffix),  s.resonance.stringResonance);
    setParamValue(makePianoParamId(pianoIndex, kPianoSoundboardAmountSuffix),  s.resonance.soundboardAmount);
    setParamValue(makePianoParamId(pianoIndex, "damping"),     s.resonance.damping);
    setParamValue(makePianoParamId(pianoIndex, kPianoModelCharacterSuffix),   s.performance.modelCharacter);
    setParamValue(makePianoParamId(pianoIndex, kPianoLowPassHzSuffix),      s.tone.lowPassHz);
    setParamValue(makePianoParamId(pianoIndex, "pan"),         s.spatial.pan);
}

void PianoSynthAudioProcessor::applyInstrumentPreset(int pianoIndex,
                                                     const mps::InstrumentPreset& preset,
                                                     bool preserveFxLock)
{
    applyPianoPresetSettings(pianoIndex, preset.settings);
    setParamValue(makePianoParamId(pianoIndex, kPianoOutputSuffix), static_cast<float>(preset.outputBus));
    applyPerformanceState(preset.performance);

    const auto maskedFx = mps::maskUnavailableFx(pianoIndex, preset.fx);
    const bool fxLocked = preserveFxLock && getParamValue(kFxLock) >= 0.5f;
    if (!fxLocked)
        restoreFx(maskedFx);

    cachedFxPerPiano[static_cast<std::size_t>(pianoIndex)] = fxLocked
        ? snapshotFx(pianoIndex)
        : maskedFx;
}

mps::InstrumentPreset PianoSynthAudioProcessor::captureCurrentPresetState(int pianoIndex,
                                                                          const juce::String& name) const
{
    mps::InstrumentPreset preset;
    preset.name = name.toStdString();
    preset.settings.level = getParamValue(makePianoParamId(pianoIndex, "level"));
    preset.settings.tuneSemitones = getParamValue(makePianoParamId(pianoIndex, "tune"));
    preset.settings.tone.hammerHardness = getParamValue(makePianoParamId(pianoIndex, kPianoHammerHardnessSuffix));
    preset.settings.envelope.attackSeconds = getParamValue(makePianoParamId(pianoIndex, "attack"));
    preset.settings.envelope.decaySeconds = getParamValue(makePianoParamId(pianoIndex, "decay"));
    preset.settings.envelope.sustainLevel = getParamValue(makePianoParamId(pianoIndex, "sustain"));
    preset.settings.envelope.releaseSeconds = getParamValue(makePianoParamId(pianoIndex, "release"));
    preset.settings.tone.brightness = getParamValue(makePianoParamId(pianoIndex, kPianoToneBrightnessSuffix));
    preset.settings.resonance.stringResonance = getParamValue(makePianoParamId(pianoIndex, kPianoStringResonanceSuffix));
    preset.settings.resonance.soundboardAmount = getParamValue(makePianoParamId(pianoIndex, kPianoSoundboardAmountSuffix));
    preset.settings.resonance.damping = getParamValue(makePianoParamId(pianoIndex, "damping"));
    preset.settings.performance.modelCharacter = getParamValue(makePianoParamId(pianoIndex, kPianoModelCharacterSuffix));
    preset.settings.tone.lowPassHz = getParamValue(makePianoParamId(pianoIndex, kPianoLowPassHzSuffix));
    preset.settings.spatial.pan = getParamValue(makePianoParamId(pianoIndex, "pan"));
    preset.fx = snapshotFx(pianoIndex);
    preset.outputBus = juce::jlimit(0, kNumAuxOutputs,
        static_cast<int>(std::round(getParamValue(makePianoParamId(pianoIndex, kPianoOutputSuffix)))));
    preset.performance = snapshotPerformanceState();
    switch (mps::getFamily(pianoIndex))
    {
        case mps::Family::Concert: preset.metadata.family = "concert"; break;
        case mps::Family::Vintage: preset.metadata.family = "vintage"; break;
        case mps::Family::Electric: preset.metadata.family = "electric"; break;
    }
    preset.metadata.intent = "user";
    preset.metadata.tags = "user,custom";
    preset.metadata.mixRole = "custom";
    return preset;
}

// =============================================================================
// Parameter layout
// =============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
PianoSynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto& banks = mps::getFactoryPresetBanks();
    const auto outputChoices = makeOutputChoices();

    // --- Global params ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kOutputGain, "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f), -3.0f));

    juce::StringArray pianoChoices;
    for (int i = 0; i < mps::kNumPianos; ++i)
        pianoChoices.add(mps::getPianoName(i));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kSelectedPiano, "Selected Piano", pianoChoices, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLfoRate, "LFO Rate",
        juce::NormalisableRange<float>(0.05f, 12.0f, 0.0001f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLfoDepth, "LFO Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kLfoWave, "LFO Wave",
        juce::StringArray{ "Sine", "Triangle", "Saw", "Square" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kLfoDestination, "LFO Destination",
        juce::StringArray{ "Off", "Tremolo", "Auto Pan", "Chorus Motion" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kPitchBendRange, "Pitch Bend Range",
        juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kVelocityCurve, "Velocity Curve",
        juce::StringArray{ "Linear", "Soft", "Softer", "Hard", "Harder", "Fixed", "Touch" }, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>("mono_mode", "Mono Mode", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("tremolo_sync", "Tremolo BPM Sync", false));

    // --- Arpeggiator ---
    layout.add(std::make_unique<juce::AudioParameterBool>("arp_enable", "Arp Enable", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "arp_mode", "Arp Mode",
        juce::StringArray{ "Up", "Down", "Up-Down", "Random" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "arp_rate", "Arp Rate",
        juce::StringArray{ "1/4", "1/8", "1/8T", "1/16", "1/16T" }, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>("arp_octaves", "Arp Octaves", 1, 4, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "arp_gate", "Arp Gate",
        juce::NormalisableRange<float>(0.05f, 1.0f, 0.01f), 0.75f));
    layout.add(std::make_unique<juce::AudioParameterBool>("arp_hold", "Arp Hold", false));

    // --- Macros ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroWarmth, "Macro Chaleur",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroBrillance, "Macro Brillance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroExpression, "Macro Expression",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroResonance, "Macro R\xC3\xA9sonance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.3f));

    // --- Mod Wheel Target ---
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kModWheelTarget, "Mod Wheel Target",
        juce::StringArray{ "Off", "Expression" }, 1));

    // --- FX: Compressor ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompThreshold, "Comp Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.01f), -19.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompRatio, "Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.01f), 3.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompAttack, "Comp Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.01f), 10.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompRelease, "Comp Release",
        juce::NormalisableRange<float>(5.0f, 500.0f, 0.01f), 120.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompMakeup, "Comp Makeup",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompMix, "Comp Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 1.0f));

    // --- FX: Saturator ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kSatDrive, "Sat Drive",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.01f), 1.8f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kSatMix, "Sat Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.15f));

    // --- FX: Transient ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kTransientAttack, "Transient Attack",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0001f), 0.10f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kTransientSustain, "Transient Sustain",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kTransientMix, "Transient Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.4f));

    // FX enable toggles (true = active)
    layout.add(std::make_unique<juce::AudioParameterBool>(kSaturationEnabled, "FX Tab 0 Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kTransientEnabled, "FX Tab 1 Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kCompressorEnabled, "FX Tab 2 Enable", true));

    // --- FX: Reverb (Dattorro Plate) ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbSize, "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.55f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbDamping, "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.50f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbWidth, "Reverb Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.80f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbMix, "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbPreDelay, "Reverb Pre-Delay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 10.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(kReverbEnabled, "FX Reverb Enable", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kReverbType, "Reverb Type",
        juce::StringArray{ "Plate", "Hall", "Room", "Chamber" }, 0));

    // --- FX: 3-Band Parametric EQ ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqLowFreq, "EQ Low Freq",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.1f, 0.4f), 200.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqLowGain, "EQ Low Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqMidFreq, "EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 8000.0f, 0.1f, 0.35f), 1000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqMidGain, "EQ Mid Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqMidQ, "EQ Mid Q",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqHighFreq, "EQ High Freq",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.3f), 5000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqHighGain, "EQ High Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(kEqEnabled, "FX Tab 3 Enable", true));

    // --- FX: Stereo Chorus ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusRate, "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 5.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusDepth, "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusMix, "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(kChorusEnabled, "FX Tab 4 Enable", true));

    // --- FX: Stereo Delay ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kDelayTime, "Delay Time",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.35f), 300.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kDelayFeedback, "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.30f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kDelayMix, "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        kDelaySync, "Delay Sync", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kDelayNoteDiv, "Delay Note Division",
        juce::StringArray{ "1/4", "1/8", "Dotted 1/8", "Triplet 1/8", "1/16", "Dotted 1/16" }, 1));
    layout.add(std::make_unique<juce::AudioParameterBool>(kDelayEnabled, "FX Tab 5 Enable", true));

    // --- FX: Output Limiter ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLimiterThreshold, "Limiter Threshold",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.01f), -0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLimiterRelease, "Limiter Release",
        juce::NormalisableRange<float>(1.0f, 200.0f, 0.1f), 50.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(kLimiterEnabled, "FX Tab 6 Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxLock, "FX Lock", false));

    // --- Per-piano parameters (8 pianos x 14 + output) ---
    for (int p = 0; p < mps::kNumPianos; ++p)
    {
        const auto& def = banks[static_cast<std::size_t>(p)][0].settings;
        const auto prefix = juce::String(mps::getPianoName(p)) + " ";

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, "level"), prefix + "Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.level));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, "tune"), prefix + "Tune",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), def.tuneSemitones));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, kPianoHammerHardnessSuffix), prefix + "Hammer Hardness",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.tone.hammerHardness));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, "attack"), prefix + "Attack",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.0001f), def.envelope.attackSeconds));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, "decay"), prefix + "Decay",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.001f), def.envelope.decaySeconds));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, "sustain"), prefix + "Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.envelope.sustainLevel));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, "release"), prefix + "Release",
            juce::NormalisableRange<float>(0.01f, 5.0f, 0.0001f), def.envelope.releaseSeconds));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, kPianoToneBrightnessSuffix), prefix + "Tone Brightness",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.tone.brightness));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, kPianoStringResonanceSuffix), prefix + "String Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.resonance.stringResonance));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, kPianoSoundboardAmountSuffix), prefix + "Soundboard Amount",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.resonance.soundboardAmount));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, "damping"), prefix + "Damping",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.resonance.damping));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, kPianoModelCharacterSuffix), prefix + "Model Character",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.performance.modelCharacter));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, kPianoLowPassHzSuffix), prefix + "Low Pass Hz",
            juce::NormalisableRange<float>(120.0f, 18000.0f, 0.0f, 0.28f), def.tone.lowPassHz));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePianoParamId(p, "pan"), prefix + "Pan",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), def.spatial.pan));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            makePianoParamId(p, kPianoOutputSuffix), prefix + "Output",
            outputChoices, 0));
    }

    return layout;
}

juce::String PianoSynthAudioProcessor::makePianoParamId(int pianoIndex, const juce::String& suffix)
{
    return "piano_" + juce::String(pianoIndex) + "_" + suffix;
}

// =============================================================================
// Prepare / Release
// =============================================================================
void PianoSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    preparedSampleRate = std::max(1.0, sampleRate);
    const int scratchSamples = juce::jmax(32768, samplesPerBlock);
    voiceCountSmoothCoeff = 1.0f - std::exp(-1.0f / (0.050f * static_cast<float>(preparedSampleRate)));
    voiceCountScale = 1.0f;
    sustainPedalPosition.fill(0.0f);
    sustainHoldLatched.fill(false);
    sostenutoPedalDown.fill(false);
    unaCordaPedalDown.fill(false);
    nextNoteOnOrder = 1;
    pitchBend.reset();
    realtimeRandomCounter.store(
        splitMix64(static_cast<std::uint64_t>(renderSeed)
                   ^ static_cast<std::uint64_t>(juce::Time::getHighResolutionTicks())),
        std::memory_order_release);
    arpState.heldNoteCount = 0;
    arpRandomState = static_cast<std::uint32_t>(
        realtimeRandomCounter.load(std::memory_order_acquire) ^ 0x243f6a88u);

    for (auto& slot : voices)
    {
        slot.active = nullptr;
        slot.dying  = nullptr;
        slot.dyingBus = 0;
        slot.activeVelocity = 0.5f;
        slot.dyingVelocity = 0.5f;
        slot.midiNote = -1;
        slot.keyDown = false;
        slot.deferredNoteOff = false;
        slot.sostenutoCaptured = false;
        slot.noteOnOrder = 0;
    }

    const juce::dsp::ProcessSpec spec {
        preparedSampleRate,
        static_cast<juce::uint32>(juce::jmax(1, scratchSamples)),
        static_cast<juce::uint32>(juce::jmax(1, getMainBusNumOutputChannels()))
    };

    compressor.reset();
    compressor.prepare(spec);
    compressor.setThreshold(-19.0f);
    compressor.setRatio(3.0f);
    compressor.setAttack(10.0f);
    compressor.setRelease(120.0f);

    plateReverb.prepare(preparedSampleRate, scratchSamples);
    convReverb.prepare(preparedSampleRate, scratchSamples);
    parametricEQ.prepare(preparedSampleRate);
    stereoChorus.prepare(preparedSampleRate, scratchSamples);
    stereoDelay.prepare(preparedSampleRate, scratchSamples);
    outputLimiter.prepare(preparedSampleRate);
    satOversampler.initProcessing(static_cast<size_t>(juce::jmax(1, scratchSamples)));

    compCache = CompressorCache{};
    fxDryBuffer.setSize(static_cast<int>(spec.numChannels),
                        static_cast<int>(spec.maximumBlockSize), false, true, true);
    const int reverbBufferChannels = juce::jmax(2, static_cast<int>(spec.numChannels));
    reverbDryBuffer.setSize(reverbBufferChannels,
                            static_cast<int>(spec.maximumBlockSize), false, true, true);
    reverbConvBuffer.setSize(reverbBufferChannels,
                             static_cast<int>(spec.maximumBlockSize), false, true, true);
    reverbPredelayCapacity = juce::jmax(2, static_cast<int>(std::ceil(preparedSampleRate * 0.100)) + 2);
    reverbPredelayBuffer.setSize(2, reverbPredelayCapacity, false, true, true);
    stealScratchBuffer.setSize(2, juce::jmax(kStealFadeSamples, kRouteFadeSamples), false, true, true);
    transientFastEnv = { 0.0f, 0.0f };
    transientSlowEnv = { 0.0f, 0.0f };
    lfoPhase = 0.0f;
    outputGainCurrent = juce::Decibels::decibelsToGain(getParamValue(kOutputGain));
    pedalNoiseLevel = 0.0f;
    pedalNoiseDecay = 1.0f;
    arpFilteredMidi.ensureSize(4096);
    reverbPredelayBuffer.clear();
    reverbPredelayWritePos = 0;
    lastConvReverbType = -1;
    convReverbSwitchFadeRemaining = 0;
    refreshDeterministicRenderMode();
    resetPedalNoiseRandomStream();
}

void PianoSynthAudioProcessor::releaseResources()
{
    sustainPedalPosition.fill(0.0f);
    sustainHoldLatched.fill(false);
    sostenutoPedalDown.fill(false);
    unaCordaPedalDown.fill(false);
    nextNoteOnOrder = 1;
    pitchBend.reset();

    for (auto& slot : voices)
    {
        slot.active = nullptr;
        slot.dying  = nullptr;
        slot.dyingBus = 0;
        slot.activeVelocity = 0.5f;
        slot.dyingVelocity = 0.5f;
        slot.midiNote = -1;
        slot.keyDown = false;
        slot.deferredNoteOff = false;
        slot.sostenutoCaptured = false;
        slot.noteOnOrder = 0;
    }
    fxDryBuffer.setSize(0, 0);
    reverbDryBuffer.setSize(0, 0);
    reverbConvBuffer.setSize(0, 0);
    reverbPredelayBuffer.setSize(0, 0);
    reverbPredelayCapacity = 0;
    stealScratchBuffer.setSize(0, 0);
    pedalNoiseLevel = 0.0f;
    pedalNoiseDecay = 1.0f;
    arpFilteredMidi.clear();
}

bool PianoSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.outputBuses.isEmpty())
        return false;

    const auto mainOutput = layouts.getMainOutputChannelSet();
    if (mainOutput != juce::AudioChannelSet::mono() &&
        mainOutput != juce::AudioChannelSet::stereo())
        return false;

    for (int busIndex = 1; busIndex < layouts.outputBuses.size(); ++busIndex)
    {
        const auto auxSet = layouts.getChannelSet(false, busIndex);
        if (auxSet.isDisabled()) continue;
        if (auxSet != juce::AudioChannelSet::mono() &&
            auxSet != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

// =============================================================================
// Arpeggiator
// =============================================================================
void PianoSynthAudioProcessor::processArpeggiator(juce::MidiBuffer& midi, int numSamples, int pianoIdx)
{
    // --- Read params ---
    const bool arpEnabled = getParamValue("arp_enable") >= 0.5f;
    if (!arpEnabled)
        return;

    const int  arpMode    = static_cast<int>(std::round(getParamValue("arp_mode")));    // 0=Up,1=Down,2=UpDown,3=Random
    const int  arpRateIdx = static_cast<int>(std::round(getParamValue("arp_rate")));    // 0..4
    const int  arpOctaves = static_cast<int>(std::round(getParamValue("arp_octaves"))); // 1..4
    const float arpGate   = juce::jlimit(0.05f, 1.0f, getParamValue("arp_gate"));
    const bool arpHold    = getParamValue("arp_hold") >= 0.5f;

    auto heldBegin = arpState.heldNotes.begin();
    auto heldEnd = [&]() { return arpState.heldNotes.begin() + arpState.heldNoteCount; };
    auto containsHeldNote = [&](const int note)
    {
        return std::binary_search(heldBegin, heldEnd(), note);
    };
    auto addHeldNote = [&](const int note)
    {
        if (containsHeldNote(note) || arpState.heldNoteCount >= ArpState::kMaxHeldNotes)
            return;

        auto end = heldEnd();
        auto insertIt = std::lower_bound(heldBegin, end, note);
        std::move_backward(insertIt, end, end + 1);
        *insertIt = note;
        ++arpState.heldNoteCount;
    };
    auto removeHeldNote = [&](const int note)
    {
        auto end = heldEnd();
        auto it = std::lower_bound(heldBegin, end, note);
        if (it == end || *it != note)
            return;

        std::move(it + 1, end, it);
        --arpState.heldNoteCount;
    };

    // --- Intercept incoming MIDI: collect held notes, swallow note-on/off ---
    arpFilteredMidi.clear();
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            addHeldNote(msg.getNoteNumber());
        }
        else if (msg.isNoteOff())
        {
            const int note = msg.getNoteNumber();
            removeHeldNote(note);
            // Hold mode: save last note until new notes pressed
            if (arpHold && arpState.heldNoteCount == 0)
            {
                arpState.heldNoteForHold = note;
                addHeldNote(note);
            }
        }
        else
        {
            arpFilteredMidi.addEvent(msg, meta.samplePosition);
        }
    }
    midi = arpFilteredMidi;

    if (arpState.heldNoteCount == 0)
    {
        // Restore held note for hold mode
        if (arpHold && arpState.heldNoteForHold >= 0)
        {
            addHeldNote(arpState.heldNoteForHold);
        }
        else if (arpState.heldNoteForHold >= 0)
        {
            arpState.heldNoteForHold = -1; // Clear when not in hold mode
        }
        if (arpState.heldNoteCount == 0)
        {
            // Release any hanging note
            if (arpState.noteIsOn && arpState.lastTriggeredNote >= 0)
            {
                midi.addEvent(juce::MidiMessage::noteOff(arpState.lastTriggeredChannel, arpState.lastTriggeredNote), 0);
                triggerNoteOff(arpState.lastTriggeredChannel, arpState.lastTriggeredNote);
                arpState.noteIsOn = false;
            }
            arpState.currentStep = 0;
            arpState.currentOctave = 0;
            arpState.phaseAccum = 0.0;
            arpState.gateCountdown = 0;
            return;
        }
    }

    // --- Step duration in samples ---
    const double bpm = static_cast<double>(currentBpm) > 0.0 ? static_cast<double>(currentBpm) : 120.0;
    const double beatSamples = preparedSampleRate * 60.0 / bpm;

    // Rate factors (relative to 1/4 note)
    static constexpr double kRateFactors[] = { 1.0, 0.5, 1.0/3.0, 0.25, 1.0/6.0 };
    const double rateFactor = kRateFactors[juce::jlimit(0, 4, arpRateIdx)];
    const double stepDurationSamples = beatSamples * rateFactor;

    // --- Gate note-off handling ---
    for (int s = 0; s < numSamples; ++s)
    {
        if (arpState.gateCountdown > 0)
        {
            --arpState.gateCountdown;
            if (arpState.gateCountdown == 0 && arpState.noteIsOn && arpState.lastTriggeredNote >= 0)
            {
                triggerNoteOff(arpState.lastTriggeredChannel, arpState.lastTriggeredNote);
                arpState.noteIsOn = false;
            }
        }
    }

    // --- Advance phase and trigger steps ---
    arpState.phaseAccum += static_cast<double>(numSamples);

    while (arpState.phaseAccum >= stepDurationSamples)
    {
        arpState.phaseAccum -= stepDurationSamples;

        // Release current note if gate < 1
        if (arpState.noteIsOn && arpState.lastTriggeredNote >= 0)
        {
            triggerNoteOff(arpState.lastTriggeredChannel, arpState.lastTriggeredNote);
            arpState.noteIsOn = false;
        }

        if (arpState.heldNoteCount == 0)
            break;

        const int numNotes = arpState.heldNoteCount;

        // Compute next step index
        int stepIdx = arpState.currentStep;
        if (arpMode == 0) // Up
        {
            stepIdx = arpState.currentStep % numNotes;
            if (++arpState.currentStep >= numNotes * arpOctaves)
                arpState.currentStep = 0;
        }
        else if (arpMode == 1) // Down
        {
            const int total = numNotes * arpOctaves;
            stepIdx = (total - 1 - arpState.currentStep) % numNotes;
            arpState.currentOctave = (total - 1 - arpState.currentStep) / numNotes;
            if (++arpState.currentStep >= total)
                arpState.currentStep = 0;
        }
        else if (arpMode == 2) // Up-Down
        {
            const int total = numNotes * arpOctaves;
            int pos = arpState.currentStep % (total > 1 ? (2 * total - 2) : 1);
            if (pos >= total) pos = 2 * total - 2 - pos;
            stepIdx = pos % numNotes;
            arpState.currentOctave = pos / numNotes;
            if (++arpState.currentStep >= (total > 1 ? (2 * total - 2) : 1))
                arpState.currentStep = 0;
        }
        else // Random
        {
            stepIdx = nextArpRandomInt(numNotes);
            arpState.currentOctave = juce::jlimit(1, 4, nextArpRandomInt(juce::jmax(1, arpOctaves)) + 1);
        }

        if (arpMode != 1 && arpMode != 2)
            arpState.currentOctave = arpState.currentStep / numNotes;

        const int baseNote  = arpState.heldNotes[static_cast<std::size_t>(
                                  juce::jlimit(0, numNotes - 1, stepIdx))];
        const int noteToPlay = juce::jlimit(0, 127, baseNote + arpState.currentOctave * 12);

        arpState.lastTriggeredNote = noteToPlay;
        arpState.lastTriggeredChannel = 1;
        triggerNoteOn(pianoIdx, 1, noteToPlay, applyVelocityCurve(0.8f, velocityCurve));
        arpState.noteIsOn = true;
        arpState.gateCountdown = juce::jmax(1, static_cast<int>(stepDurationSamples * static_cast<double>(arpGate)));
    }
}

int PianoSynthAudioProcessor::nextArpRandomInt(const int upperExclusive) noexcept
{
    if (upperExclusive <= 1)
        return 0;
    arpRandomState = arpRandomState * 1664525u + 1013904223u;
    return static_cast<int>((arpRandomState >> 1) % static_cast<std::uint32_t>(upperExclusive));
}

// =============================================================================
// Process block
// =============================================================================
void PianoSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    refreshDeterministicRenderMode();
    consumePendingPedalNoiseRandomSeed();

    const auto outputBusCount = getBusCount(false);
    for (int busIndex = 0; busIndex < outputBusCount; ++busIndex)
        getBusBuffer(buffer, false, busIndex).clear();

    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    const int pianoIdx = getSelectedPianoIndex();

    // Sync pitch bend range and velocity curve from parameters
    pitchBend.bendSemitones = getParamValue(kPitchBendRange);
    modulationMatrix.pitchBendRange.store(static_cast<int>(std::round(pitchBend.bendSemitones)),
                                          std::memory_order_relaxed);
    pitchBend.updateFactor();
    velocityCurve = intToVelocityCurve(static_cast<int>(std::round(getParamValue(kVelocityCurve))));

    // Update current BPM (used for tremolo sync and delay)
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 0.0) currentBpm = static_cast<float>(*bpm);

    // FX cache: on piano switch, snapshot old FX (read-only, safe on audio thread)
    // and defer the restoreFx (which calls setValueNotifyingHost) to the message thread
    if (pianoIdx != lastSelectedPiano)
    {
        if (lastSelectedPiano >= 0 && lastSelectedPiano < mps::kNumPianos)
            cachedFxPerPiano[static_cast<std::size_t>(lastSelectedPiano)] = snapshotFx(lastSelectedPiano);

        if (pianoIdx >= 0 && pianoIdx < mps::kNumPianos)
        {
            pendingFxRestorePiano.store(pianoIdx, std::memory_order_relaxed);
            triggerAsyncUpdate();   // lock-free; handleAsyncUpdate() runs on message thread
        }

        lastSelectedPiano = pianoIdx;
    }

    // --- Arpeggiator: pre-process MIDI, swallow note-on/off if enabled ---
    processArpeggiator(midiMessages, buffer.getNumSamples(), pianoIdx);

    auto mainBuffer = getBusBuffer(buffer, false, 0);

    auto computeRealtimeVoiceMod = [&](const int numSamples)
    {
        const int wave = juce::jlimit(0, 3, static_cast<int>(std::round(getParamValue(kLfoWave))));
        float lfo1Val = 0.0f;
        switch (wave)
        {
            case 1: lfo1Val = 1.0f - 4.0f * std::abs(lfoPhase - 0.5f); break;
            case 2: lfo1Val = lfoPhase * 2.0f - 1.0f; break;
            case 3: lfo1Val = lfoPhase < 0.5f ? 1.0f : -1.0f; break;
            default: lfo1Val = mps::fastSin(lfoPhase); break;
        }

        const float lfo2Val = modulationMatrix.lfo2.tickBlock(static_cast<float>(preparedSampleRate), numSamples);
        const float modWheel = modulationMatrix.modWheelValue.load(std::memory_order_relaxed);
        const float aftertouch = modulationMatrix.aftertouchValue.load(std::memory_order_relaxed);
        const float pitchBendValue = modulationMatrix.pitchBendValue.load(std::memory_order_relaxed);
        float envelope = 0.0f;
        float velocitySum = 0.0f;
        int envCount = 0;
        {
            for (const auto& vs : voices)
            {
                if (vs.active && vs.active->isActive())
                {
                    envelope += vs.active->getLevelEstimate();
                    velocitySum += vs.activeVelocity;
                    ++envCount;
                }
            }
            envelope = envCount > 0 ? envelope / static_cast<float>(envCount) : 0.0f;
            activeVoiceCountAtomic.store(envCount, std::memory_order_relaxed);
        }

        int activeCount = 0;
        int releasingCount = 0;
        for (const auto& vs : voices)
        {
            if (vs.active && vs.active->isActive())
            {
                ++activeCount;
                if (vs.active->isReleasing())
                    ++releasingCount;
            }
            if (vs.dying && vs.dying->isActive())
            {
                ++activeCount;
                if (vs.dying->isReleasing())
                    ++releasingCount;
            }
        }

        const float density = juce::jlimit(0.0f, 1.0f,
            (static_cast<float>(juce::jmax(0, activeCount - 1)) * 0.10f)
            + (static_cast<float>(releasingCount) * 0.05f));
        const float densityResonanceScale = 1.0f - density * 0.32f;

        auto makeContext = [&](const float velocity)
        {
            modmatrix::ModContext ctx;
            ctx.lfo1       = lfo1Val;
            ctx.lfo2       = lfo2Val;
            ctx.modWheel   = modWheel;
            ctx.aftertouch = aftertouch;
            ctx.pitchBend  = pitchBendValue;
            ctx.velocity   = juce::jlimit(0.0f, 1.0f, velocity);
            ctx.envelope   = envelope;
            return ctx;
        };

        const float averageVelocity = envCount > 0
            ? juce::jlimit(0.0f, 1.0f, velocitySum / static_cast<float>(envCount))
            : lastNoteVelocity;
        cachedModResult = modulationMatrix.process(makeContext(averageVelocity));

        return [this,
                lfo1Val,
                lfo2Val,
                modWheel,
                aftertouch,
                pitchBendValue,
                envelope,
                densityResonanceScale](const float voiceVelocity)
        {
            modmatrix::ModContext ctx;
            ctx.lfo1       = lfo1Val;
            ctx.lfo2       = lfo2Val;
            ctx.modWheel   = modWheel;
            ctx.aftertouch = aftertouch;
            ctx.pitchBend  = pitchBendValue;
            ctx.velocity   = juce::jlimit(0.0f, 1.0f, voiceVelocity);
            ctx.envelope   = envelope;
            const auto voiceResult = modulationMatrix.process(ctx);
            return mps::VoiceRealtimeModulation {
                voiceResult.cutoffMul,
                voiceResult.resonance,
                voiceResult.attackScale,
                voiceResult.decayScale,
                densityResonanceScale
            };
        };
    };

    auto renderVoiceRange = [&](const int startSample, const int numSamples)
    {
        if (numSamples <= 0)
            return;

        const auto makeRealtimeVoiceMod = computeRealtimeVoiceMod(numSamples);

        const auto resolveOutputBus = [&](const int requestedBus)
        {
            const int safeBus = juce::jlimit(0, outputBusCount - 1, requestedBus);
            if (safeBus > 0)
            {
                auto auxBuffer = getBusBuffer(buffer, false, safeBus);
                if (auxBuffer.getNumChannels() > 0 && auxBuffer.getNumSamples() > 0)
                    return safeBus;
            }

            return 0;
        };

        const auto mixScratchToBus = [&](const int requestedBus,
                                         const int destStartSample,
                                         const int samplesToMix,
                                         const float gainStart,
                                         const float gainEnd)
        {
            if (samplesToMix <= 0)
                return;

            const auto* scratchLeft = stealScratchBuffer.getReadPointer(0);
            const auto* scratchRight = stealScratchBuffer.getNumChannels() > 1
                ? stealScratchBuffer.getReadPointer(1) : nullptr;

            const auto mixIntoBuffer = [&](juce::AudioBuffer<float>& targetBuffer)
            {
                auto* targetLeft = targetBuffer.getWritePointer(0) + destStartSample;
                auto* targetRight = targetBuffer.getNumChannels() > 1
                    ? targetBuffer.getWritePointer(1) + destStartSample : nullptr;

                const float gainStep = samplesToMix > 1
                    ? (gainEnd - gainStart) / static_cast<float>(samplesToMix - 1)
                    : 0.0f;
                float gain = gainStart;
                for (int i = 0; i < samplesToMix; ++i)
                {
                    targetLeft[i] += scratchLeft[i] * gain;
                    if (targetRight != nullptr)
                        targetRight[i] += (scratchRight != nullptr ? scratchRight[i] : 0.0f) * gain;
                    gain += gainStep;
                }
            };

            const int resolvedBus = resolveOutputBus(requestedBus);
            if (resolvedBus > 0)
            {
                auto auxBuffer = getBusBuffer(buffer, false, resolvedBus);
                mixIntoBuffer(auxBuffer);
            }
            else
            {
                mixIntoBuffer(mainBuffer);
            }
        };

        int activeVoicesThisBlock = 0;
        int releasingVoicesThisBlock = 0;
        for (const auto& slot : voices)
        {
            if (slot.active && slot.active->isActive())
            {
                ++activeVoicesThisBlock;
                if (slot.active->isReleasing())
                    ++releasingVoicesThisBlock;
            }
            if (slot.dying && slot.dying->isActive())
            {
                ++activeVoicesThisBlock;
                if (slot.dying->isReleasing())
                    ++releasingVoicesThisBlock;
            }
        }

        const auto makeVoiceContext = [&](const mps::PianoVoice& target,
                                          const int ownerPiano,
                                          const int ownerChannel,
                                          const std::uint64_t ownerOrder)
        {
            mps::VoiceRenderContext context;
            context.activeVoiceCount = activeVoicesThisBlock;
            context.releasingVoiceCount = releasingVoicesThisBlock;
            context.density = juce::jlimit(0.0f, 1.0f,
                static_cast<float>(juce::jmax(0, activeVoicesThisBlock - 1)) * 0.075f
                + static_cast<float>(releasingVoicesThisBlock) * 0.055f);
            context.releasePressure = activeVoicesThisBlock > 0
                ? juce::jlimit(0.0f, 1.0f,
                    static_cast<float>(releasingVoicesThisBlock) / static_cast<float>(activeVoicesThisBlock))
                : 0.0f;

            if (ownerChannel >= 1 && ownerChannel <= static_cast<int>(sustainPedalPosition.size()))
            {
                const auto index = static_cast<std::size_t>(ownerChannel - 1);
                context.sustainPressure = sustainHoldLatched[index]
                    ? juce::jmax(0.65f, sustainPedalPosition[index])
                    : sustainPedalPosition[index] * 0.55f;
            }

            const int targetNote = target.getMidiNote();
            float repeated = 0.0f;
            float collision = 0.0f;

            const auto scanVoice = [&](const mps::PianoVoice* other,
                                       const int piano,
                                       const int channel,
                                       const std::uint64_t order)
            {
                if (other == nullptr || other == &target || !other->isActive())
                    return;
                if (piano != ownerPiano || channel != ownerChannel)
                    return;

                const int note = other->getMidiNote();
                const int semitoneDistance = std::abs(note - targetNote);
                const int intervalClass = semitoneDistance % 12;
                const bool sameNote = semitoneDistance == 0;
                const bool closeOrSharedPartial = semitoneDistance <= 2
                    || intervalClass == 0 || intervalClass == 5 || intervalClass == 7;
                const float ageWeight = order > ownerOrder ? 1.15f : 0.85f;
                const float releaseWeight = other->isReleasing() ? 1.25f : 0.75f;

                if (sameNote)
                    repeated += 0.48f * releaseWeight * ageWeight;
                if (closeOrSharedPartial)
                    collision += 0.14f * releaseWeight * ageWeight;
            };

            for (const auto& slot : voices)
            {
                scanVoice(slot.active, slot.pianoIndex, slot.midiChannel, slot.noteOnOrder);
                scanVoice(slot.dying, slot.pianoIndex, slot.midiChannel, slot.noteOnOrder);
            }

            context.repeatedNotePressure = juce::jlimit(0.0f, 1.0f, repeated);
            context.harmonicCollision = juce::jlimit(0.0f, 1.0f, collision);
            context.registerPressure = targetNote < 48 ? -1.0f : (targetNote > 72 ? 1.0f : 0.0f);
            context.tailOwnership = target.isReleasing()
                ? juce::jlimit(0.15f, 1.0f,
                    1.0f - context.repeatedNotePressure * 0.34f
                         - context.harmonicCollision * 0.26f
                         - context.density * 0.14f
                         - context.sustainPressure * 0.08f)
                : 1.0f;

            return context;
        };

        for (auto& slot : voices)
        {
            if (slot.dying && slot.dying->isActive())
            {
                slot.dying->applyRealtimeModulation(makeRealtimeVoiceMod(slot.dyingVelocity));
                slot.dying->setRenderContext(makeVoiceContext(*slot.dying, slot.pianoIndex,
                                                              slot.midiChannel, slot.noteOnOrder));
                const int dBus = juce::jlimit(0, outputBusCount - 1, slot.dyingBus);
                if (dBus > 0)
                {
                    auto auxBuffer = getBusBuffer(buffer, false, dBus);
                    if (auxBuffer.getNumChannels() > 0 && auxBuffer.getNumSamples() > 0)
                        slot.dying->render(auxBuffer, startSample, numSamples);
                    else
                        slot.dying->render(mainBuffer, startSample, numSamples);
                }
                else
                {
                    slot.dying->render(mainBuffer, startSample, numSamples);
                }
                if (!slot.dying->isActive())
                {
                    slot.dying = nullptr;
                    slot.dyingBus = 0;
                    slot.dyingVelocity = 0.5f;
                }
            }
            else if (slot.dying)
            {
                slot.dying = nullptr;
                slot.dyingBus = 0;
                slot.dyingVelocity = 0.5f;
            }

            if (slot.active && slot.active->isActive())
            {
                const int requestedBus = juce::jlimit(0, outputBusCount - 1,
                    static_cast<int>(std::round(getParamValue(
                        makePianoParamId(slot.pianoIndex, kPianoOutputSuffix)))));
                if (requestedBus != slot.renderBus)
                {
                    slot.previousRenderBus = slot.renderBus;
                    slot.renderBus = requestedBus;
                    slot.routeFadeRemaining = kRouteFadeSamples;
                }

                float pitchFactor = pitchBend.pitchBendFactor;
                if (std::abs(cachedModResult.pitchSemi) > 0.001f)
                    pitchFactor *= std::exp2(cachedModResult.pitchSemi / 12.0f);
                slot.active->setPitchBendFactor(pitchFactor);
                slot.active->applyRealtimeModulation(makeRealtimeVoiceMod(slot.activeVelocity));
                slot.active->setRenderContext(makeVoiceContext(*slot.active, slot.pianoIndex,
                                                               slot.midiChannel, slot.noteOnOrder));

                auto renderDirectToBus = [&](const int targetBus,
                                             const int destStartSample,
                                             const int samplesToRender)
                {
                    if (samplesToRender <= 0)
                        return;

                    const int resolvedBus = resolveOutputBus(targetBus);
                    if (resolvedBus > 0)
                    {
                        auto auxBuffer = getBusBuffer(buffer, false, resolvedBus);
                        slot.active->render(auxBuffer, destStartSample, samplesToRender);
                    }
                    else
                    {
                        slot.active->render(mainBuffer, destStartSample, samplesToRender);
                    }
                };

                int renderedRange = 0;
                if (slot.attackBlendRemaining > 0)
                {
                    const int blendSamples = std::min(numSamples, slot.attackBlendRemaining);
                    const int blendOffset = kStealFadeSamples - slot.attackBlendRemaining;

                    stealScratchBuffer.clear();
                    slot.active->render(stealScratchBuffer, 0, blendSamples);

                    const float startGain = static_cast<float>(blendOffset + 1)
                        / static_cast<float>(kStealFadeSamples);
                    const float endGain = static_cast<float>(blendOffset + blendSamples)
                        / static_cast<float>(kStealFadeSamples);
                    mixScratchToBus(slot.renderBus, startSample, blendSamples, startGain, endGain);

                    slot.attackBlendRemaining -= blendSamples;
                    renderedRange = blendSamples;
                }

                if (renderedRange < numSamples && slot.routeFadeRemaining > 0)
                {
                    const int fadeSamples = std::min(numSamples - renderedRange, slot.routeFadeRemaining);
                    const int fadeOffset = kRouteFadeSamples - slot.routeFadeRemaining;

                    stealScratchBuffer.clear();
                    slot.active->render(stealScratchBuffer, 0, fadeSamples);

                    const float startToGain = static_cast<float>(fadeOffset + 1)
                        / static_cast<float>(kRouteFadeSamples);
                    const float endToGain = static_cast<float>(fadeOffset + fadeSamples)
                        / static_cast<float>(kRouteFadeSamples);

                    const int fromBus = resolveOutputBus(slot.previousRenderBus);
                    const int toBus = resolveOutputBus(slot.renderBus);
                    if (fromBus == toBus)
                    {
                        mixScratchToBus(slot.renderBus, startSample + renderedRange, fadeSamples, 1.0f, 1.0f);
                    }
                    else
                    {
                        mixScratchToBus(slot.previousRenderBus, startSample + renderedRange, fadeSamples,
                                        1.0f - startToGain, 1.0f - endToGain);
                        mixScratchToBus(slot.renderBus, startSample + renderedRange, fadeSamples,
                                        startToGain, endToGain);
                    }

                    slot.routeFadeRemaining -= fadeSamples;
                    if (slot.routeFadeRemaining <= 0)
                        slot.previousRenderBus = slot.renderBus;
                    renderedRange += fadeSamples;
                }

                if (renderedRange < numSamples)
                    renderDirectToBus(slot.renderBus, startSample + renderedRange, numSamples - renderedRange);

                if (!slot.active->isActive())
                    clearVoice(slot);
            }
        }

        for (auto& sf : stealFades)
        {
            if (sf.remaining <= 0)
                continue;

            int targetBus = juce::jlimit(0, outputBusCount - 1, sf.busIndex);
            if (targetBus > 0 && getChannelCountOfBus(false, targetBus) <= 0)
                targetBus = 0;

            auto targetBuffer = getBusBuffer(buffer, false, targetBus);
            const bool useMainBuffer = targetBuffer.getNumChannels() <= 0 || targetBuffer.getNumSamples() <= 0;
            auto& mixBuffer = useMainBuffer ? mainBuffer : targetBuffer;

            const int offset = kStealFadeSamples - sf.remaining;
            const int toMix = std::min(sf.remaining, numSamples);
            auto* left  = mixBuffer.getWritePointer(0) + startSample;
            auto* right = mixBuffer.getNumChannels() > 1 ? mixBuffer.getWritePointer(1) + startSample : nullptr;
            for (int i = 0; i < toMix; ++i)
            {
                left[i] += sf.left[static_cast<std::size_t>(offset + i)];
                if (right)
                    right[i] += sf.right[static_cast<std::size_t>(offset + i)];
            }
            sf.remaining -= toMix;
        }

        int activeCount = 0;
        for (const auto& slot : voices)
            if (slot.active && slot.active->isActive())
                ++activeCount;

        const float targetScale = (activeCount > 1)
            ? 1.0f / std::pow(static_cast<float>(activeCount), 0.15f)
            : 1.0f;
        const float coeff = (targetScale < voiceCountScale) ? voiceCountSmoothCoeff : voiceCountSmoothCoeff * 0.2f;
        voiceCountScale += coeff * (targetScale - voiceCountScale);

        for (int busIndex = 0; busIndex < outputBusCount; ++busIndex)
        {
            auto busBuffer = getBusBuffer(buffer, false, busIndex);
            if (busBuffer.getNumChannels() <= 0 || busBuffer.getNumSamples() <= 0)
                continue;

            busBuffer.applyGain(startSample, numSamples, voiceCountScale);

            if (std::abs(cachedModResult.levelMul - 1.0f) > 0.001f)
                busBuffer.applyGain(startSample, numSamples, cachedModResult.levelMul);

            if (std::abs(cachedModResult.pan) > 0.001f && busBuffer.getNumChannels() >= 2)
            {
                const float panMod = juce::jlimit(-1.0f, 1.0f, cachedModResult.pan);
                const float gainL = std::sqrt(0.5f * (1.0f - panMod));
                const float gainR = std::sqrt(0.5f * (1.0f + panMod));
                busBuffer.applyGain(0, startSample, numSamples, gainL);
                busBuffer.applyGain(1, startSample, numSamples, gainR);
            }
        }
    };

    int renderedSamples = 0;
    for (const auto metadata : midiMessages)
    {
        const int eventSample = juce::jlimit(0, buffer.getNumSamples(), metadata.samplePosition);
        renderVoiceRange(renderedSamples, eventSample - renderedSamples);
        renderedSamples = eventSample;

        const auto msg = metadata.getMessage();
        modulationMatrix.handleMidiMessage(msg);
        const auto midiChannel = msg.getChannel();
        if (msg.isNoteOn())
            triggerNoteOn(pianoIdx, midiChannel, msg.getNoteNumber(),
                          applyVelocityCurve(msg.getFloatVelocity(), velocityCurve));
        else if (msg.isNoteOff())
            triggerNoteOff(midiChannel, msg.getNoteNumber());
        else if (msg.isController() && msg.getControllerNumber() == 64)
            handleSustainPedal(midiChannel, static_cast<float>(msg.getControllerValue()) / 127.0f);
        else if (msg.isAllNotesOff())
            releaseVoices(midiChannel, false);
        else if (msg.isAllSoundOff())
            releaseVoices(midiChannel, true);
        else if (msg.isController() && msg.getControllerNumber() == 121)
        {
            handleSustainPedal(midiChannel, 0.0f);
            if (midiChannel >= 1 && midiChannel <= static_cast<int>(sustainPedalPosition.size()))
            {
                sustainPedalPosition[static_cast<std::size_t>(midiChannel - 1)] = 0.0f;
                sustainHoldLatched[static_cast<std::size_t>(midiChannel - 1)] = false;
                sostenutoPedalDown[static_cast<std::size_t>(midiChannel - 1)] = false;
                unaCordaPedalDown[static_cast<std::size_t>(midiChannel - 1)] = false;
            }
            pitchBend.reset();
        }
        else if (msg.isPitchWheel())
            pitchBend.setPitchWheel(msg.getPitchWheelValue());
        else if (msg.isController() && msg.getControllerNumber() == 120)
            panicAllVoices();
        else if (msg.isController() && msg.getControllerNumber() == 66)
            handleSostenutoPedal(midiChannel, msg.getControllerValue() >= 64);
        else if (msg.isController() && msg.getControllerNumber() == 67)
            handleUnaCordaPedal(midiChannel, msg.getControllerValue() >= 64);
        else if (msg.isController())
            handleMidiCC(msg.getControllerNumber(), msg.getControllerValue(), pianoIdx);
    }

    renderVoiceRange(renderedSamples, buffer.getNumSamples() - renderedSamples);
    midiMessages.clear();

    // Auxiliary outs are always dry/direct stems. Sum them back into the main dry mix
    // before the master FX chain so the main bus remains the full post-FX production output.
    for (int busIndex = 1; busIndex < outputBusCount; ++busIndex)
    {
        auto auxBuffer = getBusBuffer(buffer, false, busIndex);
        if (auxBuffer.getNumChannels() <= 0 || auxBuffer.getNumSamples() <= 0)
            continue;

        const int mainChannels = mainBuffer.getNumChannels();
        const int auxChannels = auxBuffer.getNumChannels();
        const int samples = juce::jmin(mainBuffer.getNumSamples(), auxBuffer.getNumSamples());

        if (mainChannels == 1 && auxChannels > 1)
        {
            auto* mainData = mainBuffer.getWritePointer(0);
            const auto* left = auxBuffer.getReadPointer(0);
            const auto* right = auxBuffer.getReadPointer(1);
            for (int sample = 0; sample < samples; ++sample)
                mainData[sample] += 0.5f * (left[sample] + right[sample]);
            continue;
        }

        const int channelsToCopy = juce::jmin(mainChannels, auxChannels);
        for (int channel = 0; channel < channelsToCopy; ++channel)
            mainBuffer.addFrom(channel, 0, auxBuffer, channel, 0, samples);
    }

    if (mainBuffer.getNumChannels() > 0 && mainBuffer.getNumSamples() > 0)
    {
        renderPedalNoise(mainBuffer);
        processGlobalTransient(mainBuffer);
        processGlobalSaturator(mainBuffer);
        processGlobalEQ(mainBuffer);
        processGlobalCompressor(mainBuffer);
        processGlobalChorus(mainBuffer);
        processGlobalDelay(mainBuffer);
        applyGlobalLfo(mainBuffer);
        processGlobalReverb(mainBuffer);

        // Apply output gain BEFORE the limiter so the limiter can protect against clipping.
        const float targetGain = juce::Decibels::decibelsToGain(getParamValue(kOutputGain));
        const int numSamples = mainBuffer.getNumSamples();
        const float gainStep = numSamples > 0
            ? (targetGain - outputGainCurrent) / static_cast<float>(numSamples)
            : 0.0f;
        for (int channel = 0; channel < mainBuffer.getNumChannels(); ++channel)
        {
            auto* data = mainBuffer.getWritePointer(channel);
            float gain = outputGainCurrent;
            for (int sample = 0; sample < numSamples; ++sample)
            {
                data[sample] *= gain;
                gain += gainStep;
            }
        }
        outputGainCurrent = targetGain;

        processOutputLimiter(mainBuffer);
    }
}

// =============================================================================
// Editor
// =============================================================================
juce::AudioProcessorEditor* PianoSynthAudioProcessor::createEditor()
{
    return new PianoSynthAudioProcessorEditor(*this);
}

double PianoSynthAudioProcessor::getTailLengthSeconds() const { return mps::kMaxVoiceAgeSec + 5.0; }

// =============================================================================
// Programs / presets
// =============================================================================
int PianoSynthAudioProcessor::getNumPrograms()
{
    const int p = getSelectedPianoIndex();
    return static_cast<int>(factoryPresetBanks[static_cast<std::size_t>(p)].size());
}

int PianoSynthAudioProcessor::getCurrentProgram()
{
    const int p = getSelectedPianoIndex();
    return juce::jmax(0, currentPresetIndices[static_cast<std::size_t>(p)]);
}

void PianoSynthAudioProcessor::setCurrentProgram(int index) { applyFactoryPreset(index); }

const juce::String PianoSynthAudioProcessor::getProgramName(int index)
{
    const int p = getSelectedPianoIndex();
    const auto& bank = factoryPresetBanks[static_cast<std::size_t>(p)];
    if (index < 0 || index >= static_cast<int>(bank.size())) return {};
    return juce::String(juce::CharPointer_UTF8(bank[static_cast<std::size_t>(index)].name.c_str()));
}

void PianoSynthAudioProcessor::changeProgramName(int, const juce::String&) {}

// =============================================================================
// RT-safe deferred parameter update — called from audio thread (handleMidiCC)
// =============================================================================
void PianoSynthAudioProcessor::rebuildMidiLearnSnapshot()
{
    for (auto& slot : midiLearnParamSnapshot)
        slot.store(nullptr, std::memory_order_relaxed);

    for (const auto& [cc, paramId] : midiLearnMap)
    {
        if (cc >= 0 && cc < static_cast<int>(midiLearnParamSnapshot.size()))
            midiLearnParamSnapshot[static_cast<std::size_t>(cc)].store(parameters.getParameter(paramId), std::memory_order_release);
    }
}
void PianoSynthAudioProcessor::queueParamUpdate(juce::RangedAudioParameter* param, float normalisedValue)
{
    int start1, size1, start2, size2;
    pendingParamFifo.prepareToWrite(1, start1, size1, start2, size2);
    if (size1 > 0)
    {
        pendingParamQueue[static_cast<std::size_t>(start1)] = { param, normalisedValue };
        pendingParamFifo.finishedWrite(1);
    }
    triggerAsyncUpdate();
}

// =============================================================================
// Deferred updates (message thread) — MIDI CC params + FX restore on piano switch
// =============================================================================
void PianoSynthAudioProcessor::handleAsyncUpdate()
{
    const int learnedCc = pendingMidiLearnCc.exchange(-1, std::memory_order_acq_rel);
    auto* learnedParam = pendingMidiLearnParam.exchange(nullptr, std::memory_order_acq_rel);
    const float learnedValue = pendingMidiLearnValue.load(std::memory_order_acquire);

    if (learnedCc >= 0 && learnedCc < static_cast<int>(midiLearnParamSnapshot.size()) && learnedParam != nullptr)
    {
        const auto paramId = learnedParam->getParameterID();
        {
            juce::ScopedLock sl(midiLearnLock);
            for (auto it = midiLearnMap.begin(); it != midiLearnMap.end();)
            {
                if (it->second == paramId)
                    it = midiLearnMap.erase(it);
                else
                    ++it;
            }
            midiLearnMap[learnedCc] = paramId;
            rebuildMidiLearnSnapshot();
        }
        midiLearnArmedParamId = {};
        learnedParam->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, learnedValue));
    }

    // --- Drain deferred MIDI CC parameter updates ---
    {
        const int numReady = pendingParamFifo.getNumReady();
        int start1, size1, start2, size2;
        pendingParamFifo.prepareToRead(numReady, start1, size1, start2, size2);
        for (int i = 0; i < size1; ++i)
        {
            auto& entry = pendingParamQueue[static_cast<std::size_t>(start1 + i)];
            if (entry.param != nullptr)
                entry.param->setValueNotifyingHost(entry.normalisedValue);
        }
        for (int i = 0; i < size2; ++i)
        {
            auto& entry = pendingParamQueue[static_cast<std::size_t>(start2 + i)];
            if (entry.param != nullptr)
                entry.param->setValueNotifyingHost(entry.normalisedValue);
        }
        pendingParamFifo.finishedRead(size1 + size2);
    }

    // --- Deferred FX restore on piano switch ---
    const int pianoIdx = pendingFxRestorePiano.exchange(-1, std::memory_order_relaxed);
    if (pianoIdx >= 0 && pianoIdx < mps::kNumPianos)
    {
        if (getParamValue(kFxLock) < 0.5f)
            restoreFx(cachedFxPerPiano[static_cast<std::size_t>(pianoIdx)]);
    }
}

#if UWDEVST_PIANO_TEST_BUILD
void PianoSynthAudioProcessor::flushPendingAsyncUpdatesForTests()
{
    handleAsyncUpdate();
}
#endif

// =============================================================================
// Preset randomization (M4)
// =============================================================================
void PianoSynthAudioProcessor::randomizePreset(float amount)
{
    auto& rng = juce::Random::getSystemRandom();
    const int pianoIdx = getSelectedPianoIndex();
    static constexpr const char* kRandSuffixes[] = {
        "level", "attack", "decay", "sustain", "release",
        kPianoToneBrightnessSuffix, kPianoHammerHardnessSuffix,
        kPianoStringResonanceSuffix, kPianoSoundboardAmountSuffix,
        "damping", kPianoModelCharacterSuffix
    };
    for (auto* suffix : kRandSuffixes)
    {
        const auto paramId = makePianoParamId(pianoIdx, suffix);
        if (auto* param = parameters.getParameter(paramId))
        {
            const float cur = param->getValue();  // normalised 0..1
            const float delta = (rng.nextFloat() * 2.0f - 1.0f) * amount;
            param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, cur + delta));
        }
    }
}

// =============================================================================
// State
// =============================================================================
void PianoSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    const int selectedPiano = getSelectedPianoIndex();
    cachedFxPerPiano[static_cast<std::size_t>(selectedPiano)] = snapshotFx(selectedPiano);

    for (int childIndex = state.getNumChildren(); --childIndex >= 0;)
    {
        auto child = state.getChild(childIndex);
        if (child.hasType(kFxCacheNodeType))
            state.removeChild(childIndex, nullptr);
    }

    for (int p = 0; p < mps::kNumPianos; ++p)
        state.setProperty("pi_" + juce::String(p),
                          currentPresetIndices[static_cast<std::size_t>(p)], nullptr);

    for (int p = 0; p < mps::kNumPianos; ++p)
    {
        juce::ValueTree fxState(kFxCacheNodeType);
        fxState.setProperty(kFxCachePianoIndexAttr, p, nullptr);
        writeFxValueTree(fxState, cachedFxPerPiano[static_cast<std::size_t>(p)]);
        state.appendChild(fxState, nullptr);
    }

    for (int p = 0; p < mps::kNumPianos; ++p)
    {
        auto& f = currentUserPresetFiles[static_cast<std::size_t>(p)];
        if (f.existsAsFile())
            state.setProperty("upf_" + juce::String(p),
                              f.getFullPathName(), nullptr);
    }

    modulationMatrix.pitchBendRange.store(static_cast<int>(std::round(getParamValue(kPitchBendRange))),
                                          std::memory_order_relaxed);
    if (auto xml = state.createXml())
    {
        modulationMatrix.saveToXml(*xml);
        saveMidiLearnToXml(*xml);
        copyXmlToBinary(*xml, destData);
    }
}

void PianoSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto xmlState = getXmlFromBinary(data, sizeInBytes);
    if (xmlState == nullptr || !xmlState->hasTagName(parameters.state.getType()))
        return;

    offlinePresetMode = false;
    refreshDeterministicRenderMode();

    auto migratedXml = std::make_unique<juce::XmlElement>(*xmlState);
    migrateLegacyStateXml(*migratedXml);
    modulationMatrix.loadFromXml(*migratedXml);
    loadMidiLearnFromXml(*migratedXml);
    auto restoredState = juce::ValueTree::fromXml(*migratedXml);
    parameters.replaceState(restoredState);
    sanitizeAllParameterValues();

    for (int p = 0; p < mps::kNumPianos; ++p)
    {
        int pi = static_cast<int>(restoredState.getProperty("pi_" + juce::String(p), 0));
        const auto& bank = factoryPresetBanks[static_cast<std::size_t>(p)];
        currentPresetIndices[static_cast<std::size_t>(p)] =
            juce::jlimit(0, juce::jmax(0, static_cast<int>(bank.size()) - 1), pi);

        auto upfKey = "upf_" + juce::String(p);
        auto path   = restoredState.getProperty(upfKey, "").toString();
        if (path.isNotEmpty())
        {
            juce::File f(path);
            if (f.existsAsFile())
                currentUserPresetFiles[static_cast<std::size_t>(p)] = f;
        }
    }

    for (int childIndex = 0; childIndex < restoredState.getNumChildren(); ++childIndex)
    {
        const auto child = restoredState.getChild(childIndex);
        if (!child.hasType(kFxCacheNodeType))
            continue;

        const int pianoIndex = static_cast<int>(child.getProperty(kFxCachePianoIndexAttr, -1));
        if (pianoIndex < 0 || pianoIndex >= mps::kNumPianos)
            continue;

        int warningCount = 0;
        readFxValueTree(child, cachedFxPerPiano[static_cast<std::size_t>(pianoIndex)], warningCount);
        cachedFxPerPiano[static_cast<std::size_t>(pianoIndex)] =
            mps::maskUnavailableFx(pianoIndex, cachedFxPerPiano[static_cast<std::size_t>(pianoIndex)]);
        logSanitizationWarnings("PianoState", "ValueTree::FX_CACHE piano_" + juce::String(pianoIndex), warningCount);
    }

    lastSelectedPiano = getSelectedPianoIndex();
    cachedFxPerPiano[static_cast<std::size_t>(lastSelectedPiano)] = snapshotFx(lastSelectedPiano);
    if (getParamValue(kFxLock) < 0.5f)
        restoreFx(cachedFxPerPiano[static_cast<std::size_t>(lastSelectedPiano)]);
}

// =============================================================================
// Factory preset management
// =============================================================================
juce::StringArray PianoSynthAudioProcessor::getFactoryPresetNames() const
{
    const int p = getSelectedPianoIndex();
    juce::StringArray names;
    for (const auto& pr : factoryPresetBanks[static_cast<std::size_t>(p)])
        names.add(juce::String(juce::CharPointer_UTF8(pr.name.c_str())));
    return names;
}

int PianoSynthAudioProcessor::getCurrentFactoryPresetIndex() const noexcept
{
    return currentPresetIndices[static_cast<std::size_t>(getSelectedPianoIndex())];
}

void PianoSynthAudioProcessor::applyFactoryPreset(int presetIndex)
{
    const int p = getSelectedPianoIndex();
    const auto& bank = factoryPresetBanks[static_cast<std::size_t>(p)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size()))
        return;

    undoManager.beginNewTransaction();
    applyInstrumentPreset(p, bank[static_cast<std::size_t>(presetIndex)], true);
    currentPresetIndices[static_cast<std::size_t>(p)] = presetIndex;
    currentUserPresetFiles[static_cast<std::size_t>(p)] = juce::File{};
    offlinePresetMode = false;
    refreshDeterministicRenderMode();
    updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
}

void PianoSynthAudioProcessor::applyOfflinePreset(int pianoIndex, const mps::InstrumentPreset& preset)
{
    const int resolvedPiano = juce::jlimit(0, mps::kNumPianos - 1, pianoIndex);
    setParamValue(kSelectedPiano, static_cast<float>(resolvedPiano));
    applyInstrumentPreset(resolvedPiano, preset, false);
    currentPresetIndices[static_cast<std::size_t>(resolvedPiano)] = -1;
    currentUserPresetFiles[static_cast<std::size_t>(resolvedPiano)] = juce::File{};
    lastSelectedPiano = resolvedPiano;
    offlinePresetMode = true;
    refreshDeterministicRenderMode();

    // Offline preset renders must start from a clean engine state so repeated
    // bank scans do not inherit tails, gain smoothing, or held pedal state.
    panicAllVoices();
    sostenutoPedalDown.fill(false);
    unaCordaPedalDown.fill(false);
    stealFades = {};
    transientFastEnv = { 0.0f, 0.0f };
    transientSlowEnv = { 0.0f, 0.0f };
    voiceCountScale = 1.0f;
    activeVoiceCountAtomic.store(0, std::memory_order_relaxed);
    lfoPhase = 0.0f;
    nextNoteOnOrder = 1;
    plateReverb.reset();
    convReverb.reset();
    lastConvReverbType = -1;
    convReverbSwitchFadeRemaining = 0;
    parametricEQ.reset();
    stereoChorus.reset();
    stereoDelay.reset();
    outputLimiter.reset();
    compressor.reset();
    compCache = CompressorCache{};
    outputGainCurrent = juce::Decibels::decibelsToGain(getParamValue(kOutputGain));
    pedalNoiseLevel = 0.0f;
    pedalNoiseDecay = 1.0f;
    resetPedalNoiseRandomStream();
}

bool PianoSynthAudioProcessor::importPresetXmlFile(const juce::File& file,
                                                   const int fallbackPianoIndex,
                                                   mps::InstrumentPreset& outPreset,
                                                   int& outPianoIndex,
                                                   juce::String* message)
{
    if (!file.existsAsFile())
    {
        if (message != nullptr)
            *message = "Preset file does not exist";
        return false;
    }

    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
    {
        if (message != nullptr)
            *message = "Preset XML is not parseable";
        return false;
    }

    if (!xml->hasTagName("PianoPreset") && !xml->hasTagName("PianoFactoryPreset"))
    {
        if (message != nullptr)
            *message = "Unsupported preset root tag: " + xml->getTagName();
        return false;
    }

    const auto safeFallbackPiano = juce::jlimit(0, mps::kNumPianos - 1, fallbackPianoIndex);
    const auto pianoIdentity = musique::preset::getSynthIdentity(2);
    const bool hasExplicitPianoIndex = xml->hasAttribute(pianoIdentity.instrumentAttrName)
        || xml->hasAttribute("instrument_index")
        || xml->hasAttribute("instrumentIndex")
        || xml->hasAttribute("index")
        || xml->hasAttribute("piano")
        || xml->hasAttribute("inst")
        || xml->hasAttribute("instr")
        || xml->hasAttribute("instrIndex");

    const int requestedPianoIndex = hasExplicitPianoIndex
        ? musique::preset::readInstrumentIndexFromXml(*xml, pianoIdentity)
        : safeFallbackPiano;
    outPianoIndex = juce::jlimit(0, mps::kNumPianos - 1, requestedPianoIndex);

    const auto& bank = mps::getFactoryPresetBanks()[static_cast<std::size_t>(outPianoIndex)];
    if (!bank.empty())
        outPreset = bank.front();
    else
        outPreset.settings = mps::getDefaultSettings(outPianoIndex);

    outPreset.name = xml->getStringAttribute("name", file.getFileNameWithoutExtension()).toStdString();

    int warningCount = 0;
    auto& s = outPreset.settings;
    auto& perf = outPreset.performance;
    s.level = readValidatedXmlFloat(*xml, "level", s.level, 0.0f, 1.0f, warningCount);
    s.tuneSemitones = readValidatedXmlFloat(*xml, "tune", s.tuneSemitones, -24.0f, 24.0f, warningCount);
    s.tone.hammerHardness = readValidatedXmlFloatWithLegacy(*xml, kPresetHammerHardnessAttr, kLegacyPresetHammerAttr,
                                                            s.tone.hammerHardness, 0.0f, 1.0f, warningCount);
    s.envelope.attackSeconds = readValidatedXmlFloat(*xml, "attack", s.envelope.attackSeconds, 0.0f, 2.0f, warningCount);
    s.envelope.decaySeconds = readValidatedXmlFloat(*xml, "decay", s.envelope.decaySeconds, 0.1f, 10.0f, warningCount);
    s.envelope.sustainLevel = readValidatedXmlFloat(*xml, "sustain", s.envelope.sustainLevel, 0.0f, 1.0f, warningCount);
    s.envelope.releaseSeconds = readValidatedXmlFloat(*xml, "release", s.envelope.releaseSeconds, 0.01f, 5.0f, warningCount);
    s.tone.brightness = readValidatedXmlFloatWithLegacy(*xml, kPresetToneBrightnessAttr, kLegacyPresetBrightnessAttr,
                                                        s.tone.brightness, 0.0f, 1.0f, warningCount);
    s.resonance.stringResonance = readValidatedXmlFloatWithLegacy(*xml, kPresetStringResonanceAttr, kLegacyPresetStringResAttr,
                                                                  s.resonance.stringResonance, 0.0f, 1.0f, warningCount);
    s.resonance.soundboardAmount = readValidatedXmlFloatWithLegacy(*xml, kPresetSoundboardAmountAttr, kLegacyPresetSoundboardAttr,
                                                                   s.resonance.soundboardAmount, 0.0f, 1.0f, warningCount);
    s.resonance.damping = readValidatedXmlFloat(*xml, "damping", s.resonance.damping, 0.0f, 1.0f, warningCount);
    s.performance.modelCharacter = readValidatedXmlFloatWithLegacy(*xml, kPresetModelCharacterAttr, kLegacyPresetCharacterAttr,
                                                                   s.performance.modelCharacter, 0.0f, 1.0f, warningCount);
    s.tone.lowPassHz = readValidatedXmlFloatWithLegacy(*xml, kPresetLowPassHzAttr, kLegacyPresetCutoffAttr,
                                                       s.tone.lowPassHz, 20.0f, 20000.0f, warningCount);
    s.spatial.pan = readValidatedXmlFloat(*xml, "pan", s.spatial.pan, -1.0f, 1.0f, warningCount);

    perf.macroWarmth = readValidatedXmlFloatWithLegacy(*xml, kMacroWarmth, kLegacyMacroWarmth,
                                                       perf.macroWarmth, 0.0f, 1.0f, warningCount);
    perf.macroBrillance = readValidatedXmlFloatWithLegacy(*xml, kMacroBrillance, kLegacyMacroBrillance,
                                                          perf.macroBrillance, 0.0f, 1.0f, warningCount);
    perf.macroExpression = readValidatedXmlFloatWithLegacy(*xml, kMacroExpression, kLegacyMacroExpression,
                                                           perf.macroExpression, 0.0f, 1.0f, warningCount);
    perf.macroResonance = readValidatedXmlFloatWithLegacy(*xml, kMacroResonance, kLegacyMacroResonance,
                                                          perf.macroResonance, 0.0f, 1.0f, warningCount);
    perf.lfoRate = readValidatedXmlFloat(*xml, kLfoRate, perf.lfoRate, 0.05f, 12.0f, warningCount);
    perf.lfoDepth = readValidatedXmlFloat(*xml, kLfoDepth, perf.lfoDepth, 0.0f, 1.0f, warningCount);
    perf.lfoWave = readValidatedXmlInt(*xml, kLfoWave, perf.lfoWave, 0, 3, warningCount);
    perf.lfoDestination = sanitizeLfoDestinationForPiano(
        outPianoIndex,
        static_cast<mps::LfoDestination>(
            readValidatedXmlInt(*xml, kLfoDestination, static_cast<int>(perf.lfoDestination), 0, 3, warningCount)));
    perf.pitchBendRange = readValidatedXmlFloat(*xml, kPitchBendRange, perf.pitchBendRange, 1.0f, 24.0f, warningCount);
    perf.velocityCurve = readValidatedXmlInt(*xml, kVelocityCurve, perf.velocityCurve, 0, 6, warningCount);
    perf.monoMode = readValidatedXmlInt(*xml, kMonoMode, perf.monoMode ? 1 : 0, 0, 1, warningCount) != 0;
    perf.tremoloSync = readValidatedXmlInt(*xml, kTremoloSync, perf.tremoloSync ? 1 : 0, 0, 1, warningCount) != 0;
    perf.modWheelTarget = readValidatedXmlInt(*xml, kModWheelTarget, perf.modWheelTarget, 0, 1, warningCount);
    modmatrix::MatrixState modMatrixState = perf.modMatrixState;
    if (modmatrix::ModulationMatrix::loadStateFromXml(*xml, modMatrixState))
        perf.modMatrixState = modMatrixState;
    perf.modMatrixState = materializeModMatrixState(perf);

    readFxXmlAttributes(*xml, outPreset.fx);
    outPreset.fx = mps::maskUnavailableFx(outPianoIndex, outPreset.fx);
    outPreset.outputBus = readPresetOutputBusAttribute(*xml, outPreset.outputBus, warningCount);
    outPreset.metadata.intent = readXmlStringAttribute(*xml, "intent", outPreset.metadata.intent);
    outPreset.metadata.tags = readXmlStringAttribute(*xml, "tags", outPreset.metadata.tags);
    outPreset.metadata.family = readXmlStringAttribute(*xml, "family", outPreset.metadata.family);
    outPreset.metadata.mixRole = readXmlStringAttribute(*xml, "mix_role", outPreset.metadata.mixRole);

    if (auto* metadataNode = xml->getChildByName("Metadata"))
    {
        if (outPreset.metadata.mixRole.empty())
            outPreset.metadata.mixRole = readXmlStringAttribute(*metadataNode, "role", outPreset.metadata.mixRole);
        if (outPreset.metadata.tags.empty())
            outPreset.metadata.tags = readXmlStringAttribute(*metadataNode, "usage", outPreset.metadata.tags);
    }

    if (message != nullptr)
    {
        *message = {};
        if (requestedPianoIndex != outPianoIndex)
            *message << "Invalid piano_index sanitized to " << juce::String(outPianoIndex) << ". ";
        if (warningCount > 0)
            *message << "Sanitized " << juce::String(warningCount) << " preset fields.";
    }

    return true;
}

bool PianoSynthAudioProcessor::saveFactoryPreset(int presetIndex)
{
    const int p = getSelectedPianoIndex();
    auto& bank = factoryPresetBanks[static_cast<std::size_t>(p)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size()))
        return false;

    auto captured = captureCurrentPresetState(p, bank[static_cast<std::size_t>(presetIndex)].name);
    captured.metadata = bank[static_cast<std::size_t>(presetIndex)].metadata;
    bank[static_cast<std::size_t>(presetIndex)] = captured;

    auto dir = getFactoryOverridesDirectory().getChildFile("piano_" + juce::String(p));
    dir.createDirectory();
    auto file = dir.getChildFile(juce::String(presetIndex) + ".xml");

    auto root = createPresetXml("PianoFactoryPreset",
                                juce::String(captured.name),
                                p,
                                captured.settings,
                                captured.performance,
                                captured.fx,
                                captured.outputBus,
                                &captured.metadata,
                                presetIndex);
    return root->writeTo(file);
}

void PianoSynthAudioProcessor::loadFactoryOverrides()
{
    auto baseDir = getFactoryOverridesDirectory();
    for (int p = 0; p < mps::kNumPianos; ++p)
    {
        auto dir = baseDir.getChildFile("piano_" + juce::String(p));
        if (!dir.isDirectory()) continue;
        auto& bank = factoryPresetBanks[static_cast<std::size_t>(p)];
        for (int i = 0; i < static_cast<int>(bank.size()); ++i)
        {
            auto file = dir.getChildFile(juce::String(i) + ".xml");
            if (!file.existsAsFile()) continue;
            auto xml = juce::XmlDocument::parse(file);
            if (xml == nullptr || !xml->hasTagName("PianoFactoryPreset")) continue;
            const bool needsRewrite = shouldRewritePresetXml(*xml);
            int warningCount = 0;
            auto& preset = bank[static_cast<std::size_t>(i)];
            auto& s = preset.settings;
            s.level = sanitizeParameterValue(makePianoParamId(p, "level"),
                readValidatedXmlFloat(*xml, "level", s.level, 0.0f, 1.0f, warningCount), s.level, &warningCount);
            s.tuneSemitones = sanitizeParameterValue(makePianoParamId(p, "tune"),
                readValidatedXmlFloat(*xml, "tune", s.tuneSemitones, -24.0f, 24.0f, warningCount), s.tuneSemitones, &warningCount);
            s.tone.hammerHardness = sanitizeParameterValue(makePianoParamId(p, kPianoHammerHardnessSuffix),
                getDoubleAttributeWithLegacy(*xml, kPresetHammerHardnessAttr, kLegacyPresetHammerAttr, s.tone.hammerHardness), s.tone.hammerHardness, &warningCount);
            s.envelope.attackSeconds = sanitizeParameterValue(makePianoParamId(p, "attack"),
                readValidatedXmlFloat(*xml, "attack", s.envelope.attackSeconds, 0.0f, 2.0f, warningCount), s.envelope.attackSeconds, &warningCount);
            s.envelope.decaySeconds = sanitizeParameterValue(makePianoParamId(p, "decay"),
                readValidatedXmlFloat(*xml, "decay", s.envelope.decaySeconds, 0.1f, 10.0f, warningCount), s.envelope.decaySeconds, &warningCount);
            s.envelope.sustainLevel = sanitizeParameterValue(makePianoParamId(p, "sustain"),
                readValidatedXmlFloat(*xml, "sustain", s.envelope.sustainLevel, 0.0f, 1.0f, warningCount), s.envelope.sustainLevel, &warningCount);
            s.envelope.releaseSeconds = sanitizeParameterValue(makePianoParamId(p, "release"),
                readValidatedXmlFloat(*xml, "release", s.envelope.releaseSeconds, 0.01f, 5.0f, warningCount), s.envelope.releaseSeconds, &warningCount);
            s.tone.brightness = sanitizeParameterValue(makePianoParamId(p, kPianoToneBrightnessSuffix),
                getDoubleAttributeWithLegacy(*xml, kPresetToneBrightnessAttr, kLegacyPresetBrightnessAttr, s.tone.brightness), s.tone.brightness, &warningCount);
            s.resonance.stringResonance = sanitizeParameterValue(makePianoParamId(p, kPianoStringResonanceSuffix),
                getDoubleAttributeWithLegacy(*xml, kPresetStringResonanceAttr, kLegacyPresetStringResAttr, s.resonance.stringResonance), s.resonance.stringResonance, &warningCount);
            s.resonance.soundboardAmount = sanitizeParameterValue(makePianoParamId(p, kPianoSoundboardAmountSuffix),
                getDoubleAttributeWithLegacy(*xml, kPresetSoundboardAmountAttr, kLegacyPresetSoundboardAttr, s.resonance.soundboardAmount), s.resonance.soundboardAmount, &warningCount);
            s.resonance.damping = sanitizeParameterValue(makePianoParamId(p, "damping"),
                readValidatedXmlFloat(*xml, "damping", s.resonance.damping, 0.0f, 1.0f, warningCount), s.resonance.damping, &warningCount);
            s.performance.modelCharacter = sanitizeParameterValue(makePianoParamId(p, kPianoModelCharacterSuffix),
                getDoubleAttributeWithLegacy(*xml, kPresetModelCharacterAttr, kLegacyPresetCharacterAttr, s.performance.modelCharacter), s.performance.modelCharacter, &warningCount);
            s.tone.lowPassHz = sanitizeParameterValue(makePianoParamId(p, kPianoLowPassHzSuffix),
                getDoubleAttributeWithLegacy(*xml, kPresetLowPassHzAttr, kLegacyPresetCutoffAttr, s.tone.lowPassHz), s.tone.lowPassHz, &warningCount);
            s.spatial.pan = sanitizeParameterValue(makePianoParamId(p, "pan"),
                readValidatedXmlFloat(*xml, "pan", s.spatial.pan, -1.0f, 1.0f, warningCount), s.spatial.pan, &warningCount);
            preset.performance.macroWarmth = sanitizeParameterValue(kMacroWarmth,
                readValidatedXmlFloat(*xml, kMacroWarmth, preset.performance.macroWarmth, 0.0f, 1.0f, warningCount), preset.performance.macroWarmth, &warningCount);
            preset.performance.macroBrillance = sanitizeParameterValue(kMacroBrillance,
                readValidatedXmlFloat(*xml, kMacroBrillance, preset.performance.macroBrillance, 0.0f, 1.0f, warningCount), preset.performance.macroBrillance, &warningCount);
            preset.performance.macroExpression = sanitizeParameterValue(kMacroExpression,
                readValidatedXmlFloat(*xml, kMacroExpression, preset.performance.macroExpression, 0.0f, 1.0f, warningCount), preset.performance.macroExpression, &warningCount);
            preset.performance.macroResonance = sanitizeParameterValue(kMacroResonance,
                readValidatedXmlFloat(*xml, kMacroResonance, preset.performance.macroResonance, 0.0f, 1.0f, warningCount), preset.performance.macroResonance, &warningCount);
            preset.performance.lfoRate = sanitizeParameterValue(kLfoRate,
                readValidatedXmlFloat(*xml, kLfoRate, preset.performance.lfoRate, 0.05f, 12.0f, warningCount), preset.performance.lfoRate, &warningCount);
            preset.performance.lfoDepth = sanitizeParameterValue(kLfoDepth,
                readValidatedXmlFloat(*xml, kLfoDepth, preset.performance.lfoDepth, 0.0f, 1.0f, warningCount), preset.performance.lfoDepth, &warningCount);
            preset.performance.lfoWave = readValidatedXmlInt(*xml, kLfoWave, preset.performance.lfoWave, 0, 3, warningCount);
            preset.performance.lfoDestination = sanitizeLfoDestinationForPiano(
                p,
                static_cast<mps::LfoDestination>(
                    readValidatedXmlInt(*xml, kLfoDestination, static_cast<int>(preset.performance.lfoDestination), 0, 3, warningCount)));
            preset.performance.pitchBendRange = sanitizeParameterValue(kPitchBendRange,
                readValidatedXmlFloat(*xml, kPitchBendRange, preset.performance.pitchBendRange, 1.0f, 24.0f, warningCount),
                preset.performance.pitchBendRange,
                &warningCount);
            preset.performance.velocityCurve = juce::jlimit(0, 6,
                readValidatedXmlInt(*xml, kVelocityCurve, preset.performance.velocityCurve, 0, 6, warningCount));
            preset.performance.monoMode = readValidatedXmlInt(*xml, kMonoMode,
                preset.performance.monoMode ? 1 : 0, 0, 1, warningCount) != 0;
            preset.performance.tremoloSync = readValidatedXmlInt(*xml, kTremoloSync,
                preset.performance.tremoloSync ? 1 : 0, 0, 1, warningCount) != 0;
            preset.performance.modWheelTarget = juce::jlimit(0, 1,
                readValidatedXmlInt(*xml, kModWheelTarget, preset.performance.modWheelTarget, 0, 1, warningCount));
            modmatrix::MatrixState modMatrixState;
            if (modmatrix::ModulationMatrix::loadStateFromXml(*xml, modMatrixState))
                preset.performance.modMatrixState = modMatrixState;
            preset.performance.modMatrixState = materializeModMatrixState(preset.performance);
            readFxXmlAttributes(*xml, preset.fx);
            preset.fx = mps::maskUnavailableFx(p, preset.fx);
            preset.outputBus = juce::jlimit(0, kNumAuxOutputs,
                readValidatedXmlInt(*xml, "output", preset.outputBus, 0, kNumAuxOutputs, warningCount));
            preset.metadata.intent = readXmlStringAttribute(*xml, "intent", preset.metadata.intent);
            preset.metadata.tags = readXmlStringAttribute(*xml, "tags", preset.metadata.tags);
            preset.metadata.family = readXmlStringAttribute(*xml, "family", preset.metadata.family);
            preset.metadata.mixRole = readXmlStringAttribute(*xml, "mix_role", preset.metadata.mixRole);

    logSanitizationWarnings("PianoFactoryPreset", file.getFullPathName(), warningCount);

            if (needsRewrite)
            {
                auto normalizedXml = createPresetXml("PianoFactoryPreset",
                                                    xml->getStringAttribute("name", juce::String(preset.name)),
                                                    p,
                                                    s,
                                                    preset.performance,
                                                    preset.fx,
                                                    preset.outputBus,
                                                    &preset.metadata,
                                                    i);
                normalizedXml->writeTo(file);
            }
        }
    }
}

// =============================================================================
// User presets
// =============================================================================
juce::File PianoSynthAudioProcessor::getFactoryOverridesDirectory()
{
    const auto preferred = getLibraryRootDirectory().getChildFile("FactoryOverrides_v4");
    return findWritableDirectory(preferred, "MusiquePianoSynth/FactoryOverrides_v4");
}

juce::File PianoSynthAudioProcessor::getLibraryRootDirectory()
{
#if defined(UWDEVST_QA_PRESET_HOME)
    return juce::File(UWDEVST_QA_PRESET_HOME);
#endif
    const auto preferred = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("MusiquePianoSynth");
    return findWritableDirectory(preferred, "MusiquePianoSynth");
}

juce::File PianoSynthAudioProcessor::getUserPresetsDirectory(int pianoIndex)
{
    const auto preferred = getLibraryRootDirectory()
                               .getChildFile("Presets_v4")
                               .getChildFile("piano_" + juce::String(pianoIndex));
    return findWritableDirectory(preferred,
                                 "MusiquePianoSynth/Presets_v4/piano_" + juce::String(juce::jmax(0, pianoIndex)));
}

void PianoSynthAudioProcessor::archiveLegacyPresetLibraryIfNeeded()
{
    auto root = getLibraryRootDirectory();
    auto sentinel = root.getChildFile("preset_library_v4_migrated.flag");
    if (sentinel.existsAsFile())
        return;

    auto legacyPresets = root.getChildFile("Presets");
    auto legacyOverrides = root.getChildFile("FactoryOverrides");
    const bool hasLegacy = legacyPresets.exists() || legacyOverrides.exists();

    if (hasLegacy)
    {
        auto archiveRoot = root.getChildFile("Archive");
        archiveRoot.createDirectory();
        const auto stamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        auto archiveDir = archiveRoot.getChildFile("legacy_piano_library_" + stamp);
        archiveDir.createDirectory();

        auto moveIfPresent = [&archiveDir](const juce::File& source, const juce::String& targetName)
        {
            if (!source.exists())
                return;

            auto target = archiveDir.getChildFile(targetName);
            if (!source.moveFileTo(target))
            {
                if (source.isDirectory())
                {
                    source.copyDirectoryTo(target);
                    source.deleteRecursively();
                }
                else
                {
                    source.copyFileTo(target);
                    source.deleteFile();
                }
            }
        };

        moveIfPresent(legacyPresets, "Presets");
        moveIfPresent(legacyOverrides, "FactoryOverrides");
    }

    getFactoryOverridesDirectory();
    for (int pianoIndex = 0; pianoIndex < mps::kNumPianos; ++pianoIndex)
        getUserPresetsDirectory(pianoIndex);

    sentinel.replaceWithText("migrated_to_v4\n", false, false, "\n");
}

juce::Array<juce::File> PianoSynthAudioProcessor::scanUserPresets() const
{
    juce::Array<juce::File> results;
    auto dir = getUserPresetsDirectory(getSelectedPianoIndex());
    if (dir.isDirectory())
        dir.findChildFiles(results, juce::File::findFiles, false, "*.xml");
    results.sort();
    return results;
}

bool PianoSynthAudioProcessor::writePresetManifest(const juce::File& presetFile,
                                                   const juce::String& presetName,
                                                   int pianoIndex,
                                                   const juce::String& sourceModel) const
{
    const auto identity = musique::preset::getSynthIdentity(2);
    if (!identity.isValid())
        return false;

    musique::preset::PresetManifest manifest;
    manifest.synthId = identity.synthId;
    manifest.synthType = identity.synthType;
    manifest.instrumentIndex = juce::jlimit(0, mps::kNumPianos - 1, pianoIndex);
    manifest.instrumentName = mps::getPianoName(manifest.instrumentIndex);
    manifest.presetName = presetName;
    manifest.xmlRootTag = identity.xmlRootTag;
    manifest.sourceModel = sourceModel;
    manifest.createdAt = juce::Time::getCurrentTime().toISO8601(true);
    manifest.sourcePath = presetFile.getFullPathName();
    manifest.validationVersion = 1;

    return musique::preset::saveManifestToFile(
        musique::preset::manifestFileForPresetFile(presetFile), manifest);
}

bool PianoSynthAudioProcessor::saveUserPreset(const juce::String& name)
{
    if (name.isEmpty()) return false;
    const int p = getSelectedPianoIndex();
    auto file = getUserPresetsDirectory(p).getChildFile(
        juce::File::createLegalFileName(name) + ".xml");
    auto captured = captureCurrentPresetState(p, name);
    auto root = createPresetXml("PianoPreset", name, p, captured.settings,
                                captured.performance,
                                captured.fx,
                                captured.outputBus,
                                &captured.metadata);

    if (root->writeTo(file))
    {
        writePresetManifest(file, name, p);
        currentUserPresetFiles[static_cast<std::size_t>(p)] = file;
        currentPresetIndices[static_cast<std::size_t>(p)] = -1;
        return true;
    }
    return false;
}

bool PianoSynthAudioProcessor::updateUserPreset(const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    const int p = getSelectedPianoIndex();
    auto captured = captureCurrentPresetState(p, file.getFileNameWithoutExtension());
    auto root = createPresetXml("PianoPreset", file.getFileNameWithoutExtension(), p, captured.settings,
                                captured.performance,
                                captured.fx,
                                captured.outputBus,
                                &captured.metadata);

    if (root->writeTo(file))
    {
        writePresetManifest(file, file.getFileNameWithoutExtension(), p);
        currentUserPresetFiles[static_cast<std::size_t>(p)] = file;
        currentPresetIndices[static_cast<std::size_t>(p)] = -1;
        return true;
    }
    return false;
}

bool PianoSynthAudioProcessor::deleteUserPreset(const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    const int p = getSelectedPianoIndex();
    if (currentUserPresetFiles[static_cast<std::size_t>(p)] == file)
        currentUserPresetFiles[static_cast<std::size_t>(p)] = juce::File{};
    musique::preset::manifestFileForPresetFile(file).deleteFile();
    return file.deleteFile();
}

bool PianoSynthAudioProcessor::loadUserPreset(const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    undoManager.beginNewTransaction();
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || !xml->hasTagName("PianoPreset")) return false;
    const bool needsRewrite = shouldRewritePresetXml(*xml);

    const int currentPianoIndex = getSelectedPianoIndex();
    offlinePresetMode = false;
    refreshDeterministicRenderMode();
    const auto pianoIdentity = musique::preset::getSynthIdentity(2);
    const bool hasExplicitPianoIndex = xml->hasAttribute(pianoIdentity.instrumentAttrName)
        || xml->hasAttribute("instrument_index")
        || xml->hasAttribute("instrumentIndex")
        || xml->hasAttribute("index")
        || xml->hasAttribute("piano")
        || xml->hasAttribute("inst")
        || xml->hasAttribute("instr")
        || xml->hasAttribute("instrIndex");
    const int requestedPianoIndex = hasExplicitPianoIndex
        ? musique::preset::readInstrumentIndexFromXml(*xml, pianoIdentity)
        : currentPianoIndex;
    const int targetPianoIndex = (requestedPianoIndex >= 0 && requestedPianoIndex < mps::kNumPianos)
        ? requestedPianoIndex
        : currentPianoIndex;

    if (requestedPianoIndex != targetPianoIndex)
    {
        juce::Logger::writeToLog("[PianoPreset] Invalid piano_index=" + juce::String(requestedPianoIndex)
            + ", keeping current piano " + juce::String(currentPianoIndex));
    }

    if (targetPianoIndex != currentPianoIndex)
        setParamValue(kSelectedPiano, static_cast<float>(targetPianoIndex));

    const int p = targetPianoIndex;
    int warningCount = 0;
    const auto defaultParamValue = [this](const juce::String& paramId) -> float
    {
        if (auto* parameter = parameters.getParameter(paramId))
        {
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
                return ranged->convertFrom0to1(ranged->getDefaultValue());
        }
        return 0.0f;
    };
    setParamValue(makePianoParamId(p, "level"),
        sanitizeParameterValue(makePianoParamId(p, "level"),
            readValidatedXmlFloat(*xml, "level", getParamValue(makePianoParamId(p, "level")), 0.0f, 1.0f, warningCount),
            getParamValue(makePianoParamId(p, "level")), &warningCount));
    setParamValue(makePianoParamId(p, "tune"),
        sanitizeParameterValue(makePianoParamId(p, "tune"),
            readValidatedXmlFloat(*xml, "tune", getParamValue(makePianoParamId(p, "tune")), -24.0f, 24.0f, warningCount),
            getParamValue(makePianoParamId(p, "tune")), &warningCount));
    setParamValue(makePianoParamId(p, kPianoHammerHardnessSuffix),
        sanitizeParameterValue(makePianoParamId(p, kPianoHammerHardnessSuffix),
            getDoubleAttributeWithLegacy(*xml, kPresetHammerHardnessAttr, kLegacyPresetHammerAttr, getParamValue(makePianoParamId(p, kPianoHammerHardnessSuffix))),
            getParamValue(makePianoParamId(p, kPianoHammerHardnessSuffix)), &warningCount));
    setParamValue(makePianoParamId(p, "attack"),
        sanitizeParameterValue(makePianoParamId(p, "attack"),
            readValidatedXmlFloat(*xml, "attack", getParamValue(makePianoParamId(p, "attack")), 0.0f, 2.0f, warningCount),
            getParamValue(makePianoParamId(p, "attack")), &warningCount));
    setParamValue(makePianoParamId(p, "decay"),
        sanitizeParameterValue(makePianoParamId(p, "decay"),
            readValidatedXmlFloat(*xml, "decay", getParamValue(makePianoParamId(p, "decay")), 0.1f, 10.0f, warningCount),
            getParamValue(makePianoParamId(p, "decay")), &warningCount));
    setParamValue(makePianoParamId(p, "sustain"),
        sanitizeParameterValue(makePianoParamId(p, "sustain"),
            readValidatedXmlFloat(*xml, "sustain", getParamValue(makePianoParamId(p, "sustain")), 0.0f, 1.0f, warningCount),
            getParamValue(makePianoParamId(p, "sustain")), &warningCount));
    setParamValue(makePianoParamId(p, "release"),
        sanitizeParameterValue(makePianoParamId(p, "release"),
            readValidatedXmlFloat(*xml, "release", getParamValue(makePianoParamId(p, "release")), 0.01f, 5.0f, warningCount),
            getParamValue(makePianoParamId(p, "release")), &warningCount));
    setParamValue(makePianoParamId(p, kPianoToneBrightnessSuffix),
        sanitizeParameterValue(makePianoParamId(p, kPianoToneBrightnessSuffix),
            getDoubleAttributeWithLegacy(*xml, kPresetToneBrightnessAttr, kLegacyPresetBrightnessAttr, getParamValue(makePianoParamId(p, kPianoToneBrightnessSuffix))),
            getParamValue(makePianoParamId(p, kPianoToneBrightnessSuffix)), &warningCount));
    setParamValue(makePianoParamId(p, kPianoStringResonanceSuffix),
        sanitizeParameterValue(makePianoParamId(p, kPianoStringResonanceSuffix),
            getDoubleAttributeWithLegacy(*xml, kPresetStringResonanceAttr, kLegacyPresetStringResAttr, getParamValue(makePianoParamId(p, kPianoStringResonanceSuffix))),
            getParamValue(makePianoParamId(p, kPianoStringResonanceSuffix)), &warningCount));
    setParamValue(makePianoParamId(p, kPianoSoundboardAmountSuffix),
        sanitizeParameterValue(makePianoParamId(p, kPianoSoundboardAmountSuffix),
            getDoubleAttributeWithLegacy(*xml, kPresetSoundboardAmountAttr, kLegacyPresetSoundboardAttr, getParamValue(makePianoParamId(p, kPianoSoundboardAmountSuffix))),
            getParamValue(makePianoParamId(p, kPianoSoundboardAmountSuffix)), &warningCount));
    setParamValue(makePianoParamId(p, "damping"),
        sanitizeParameterValue(makePianoParamId(p, "damping"),
            readValidatedXmlFloat(*xml, "damping", getParamValue(makePianoParamId(p, "damping")), 0.0f, 1.0f, warningCount),
            getParamValue(makePianoParamId(p, "damping")), &warningCount));
    setParamValue(makePianoParamId(p, kPianoModelCharacterSuffix),
        sanitizeParameterValue(makePianoParamId(p, kPianoModelCharacterSuffix),
            getDoubleAttributeWithLegacy(*xml, kPresetModelCharacterAttr, kLegacyPresetCharacterAttr, getParamValue(makePianoParamId(p, kPianoModelCharacterSuffix))),
            getParamValue(makePianoParamId(p, kPianoModelCharacterSuffix)), &warningCount));
    setParamValue(makePianoParamId(p, kPianoLowPassHzSuffix),
        sanitizeParameterValue(makePianoParamId(p, kPianoLowPassHzSuffix),
            getDoubleAttributeWithLegacy(*xml, kPresetLowPassHzAttr, kLegacyPresetCutoffAttr, getParamValue(makePianoParamId(p, kPianoLowPassHzSuffix))),
            getParamValue(makePianoParamId(p, kPianoLowPassHzSuffix)), &warningCount));
    setParamValue(makePianoParamId(p, "pan"),
        sanitizeParameterValue(makePianoParamId(p, "pan"),
            readValidatedXmlFloat(*xml, "pan", getParamValue(makePianoParamId(p, "pan")), -1.0f, 1.0f, warningCount),
            getParamValue(makePianoParamId(p, "pan")), &warningCount));
    setParamValue(makePianoParamId(p, kPianoOutputSuffix), static_cast<float>(juce::jlimit(0, kNumAuxOutputs,
        readValidatedXmlInt(*xml, "output", static_cast<int>(std::round(getParamValue(makePianoParamId(p, kPianoOutputSuffix)))), 0, kNumAuxOutputs, warningCount))));
    setParamValue(kMacroWarmth, sanitizeParameterValue(kMacroWarmth,
        readValidatedXmlFloat(*xml, kMacroWarmth, getParamValue(kMacroWarmth), 0.0f, 1.0f, warningCount), getParamValue(kMacroWarmth), &warningCount));
    setParamValue(kMacroBrillance, sanitizeParameterValue(kMacroBrillance,
        readValidatedXmlFloat(*xml, kMacroBrillance, getParamValue(kMacroBrillance), 0.0f, 1.0f, warningCount), getParamValue(kMacroBrillance), &warningCount));
    setParamValue(kMacroExpression, sanitizeParameterValue(kMacroExpression,
        readValidatedXmlFloat(*xml, kMacroExpression, getParamValue(kMacroExpression), 0.0f, 1.0f, warningCount), getParamValue(kMacroExpression), &warningCount));
    setParamValue(kMacroResonance, sanitizeParameterValue(kMacroResonance,
        readValidatedXmlFloat(*xml, kMacroResonance, getParamValue(kMacroResonance), 0.0f, 1.0f, warningCount), getParamValue(kMacroResonance), &warningCount));
    setParamValue(kLfoRate, sanitizeParameterValue(kLfoRate,
        readValidatedXmlFloat(*xml, kLfoRate, getParamValue(kLfoRate), 0.05f, 12.0f, warningCount), getParamValue(kLfoRate), &warningCount));
    setParamValue(kLfoDepth, sanitizeParameterValue(kLfoDepth,
        readValidatedXmlFloat(*xml, kLfoDepth, getParamValue(kLfoDepth), 0.0f, 1.0f, warningCount), getParamValue(kLfoDepth), &warningCount));
    setParamValue(kLfoWave, static_cast<float>(readValidatedXmlInt(*xml, kLfoWave, static_cast<int>(std::round(getParamValue(kLfoWave))), 0, 3, warningCount)));
    setParamValue(kLfoDestination, static_cast<float>(readValidatedXmlInt(*xml, kLfoDestination, static_cast<int>(std::round(getParamValue(kLfoDestination))), 0, 3, warningCount)));
    setParamValue(kPitchBendRange, sanitizeParameterValue(kPitchBendRange,
        readValidatedXmlFloat(*xml, kPitchBendRange, defaultParamValue(kPitchBendRange), 1.0f, 24.0f, warningCount),
        defaultParamValue(kPitchBendRange), &warningCount));
    setParamValue(kVelocityCurve, static_cast<float>(juce::jlimit(0, 6,
        readValidatedXmlInt(*xml, kVelocityCurve, static_cast<int>(std::round(defaultParamValue(kVelocityCurve))), 0, 6, warningCount))));
    setParamValue(kMonoMode, static_cast<float>(readValidatedXmlInt(*xml, kMonoMode,
        static_cast<int>(std::round(defaultParamValue(kMonoMode))), 0, 1, warningCount)));
    setParamValue(kTremoloSync, static_cast<float>(readValidatedXmlInt(*xml, kTremoloSync,
        static_cast<int>(std::round(defaultParamValue(kTremoloSync))), 0, 1, warningCount)));
    setParamValue(kModWheelTarget, static_cast<float>(juce::jlimit(0, 1,
        readValidatedXmlInt(*xml, kModWheelTarget, static_cast<int>(std::round(defaultParamValue(kModWheelTarget))), 0, 1, warningCount))));
    modulationMatrix.loadFromXml(*xml);

    auto presetFx = snapshotFx(p);
    readFxXmlAttributes(*xml, presetFx);
    presetFx = mps::maskUnavailableFx(p, presetFx);
    const bool fxLocked = getParamValue(kFxLock) >= 0.5f;
    if (!fxLocked)
        restoreFx(presetFx);
    cachedFxPerPiano[static_cast<std::size_t>(p)] = fxLocked ? snapshotFx(p) : presetFx;

    logSanitizationWarnings("PianoPreset", file.getFullPathName(), warningCount);

    if (needsRewrite)
    {
        auto captured = captureCurrentPresetState(p, xml->getStringAttribute("name", file.getFileNameWithoutExtension()));
        auto normalizedXml = createPresetXml("PianoPreset",
                                             xml->getStringAttribute("name", file.getFileNameWithoutExtension()),
                                             p,
                                             captured.settings,
                                             captured.performance,
                                             presetFx,
                                             captured.outputBus,
                                             &captured.metadata);
        normalizedXml->writeTo(file);
    }

    // Back-fill manifest if missing (legacy presets or first load since manifest support)
    if (!musique::preset::manifestFileForPresetFile(file).existsAsFile())
        writePresetManifest(file, xml->getStringAttribute("name", file.getFileNameWithoutExtension()), p);

    currentUserPresetFiles[static_cast<std::size_t>(p)] = file;
    currentPresetIndices[static_cast<std::size_t>(p)] = -1;
    updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
    return true;
}

bool PianoSynthAudioProcessor::isCurrentPresetUser() const noexcept
{
    return currentUserPresetFiles[static_cast<std::size_t>(getSelectedPianoIndex())].existsAsFile();
}

juce::File PianoSynthAudioProcessor::getCurrentUserPresetFile() const noexcept
{
    return currentUserPresetFiles[static_cast<std::size_t>(getSelectedPianoIndex())];
}

// =============================================================================
// Voice management
// =============================================================================
int PianoSynthAudioProcessor::getSelectedPianoIndex() const
{
    return juce::jlimit(0, mps::kNumPianos - 1,
                        static_cast<int>(std::round(getParamValue(kSelectedPiano))));
}

bool PianoSynthAudioProcessor::isFxAvailableForCurrentPiano(mps::GlobalFxSlot slot) const
{
    return mps::isFxAvailable(getSelectedPianoIndex(), slot);
}

float PianoSynthAudioProcessor::getParamValue(const juce::String& paramId) const
{
    if (const auto* raw = parameters.getRawParameterValue(paramId))
        return raw->load();
    return 0.0f;
}

float PianoSynthAudioProcessor::sanitizeParameterValue(const juce::String& paramId,
                                                       float value,
                                                       float fallback,
                                                       int* warningCount) const
{
    if (!std::isfinite(value))
    {
        if (warningCount != nullptr)
            ++(*warningCount);
        return fallback;
    }

    auto* parameter = parameters.getParameter(paramId);
    auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
    if (ranged == nullptr)
        return value;

    const auto normalised = juce::jlimit(0.0f, 1.0f, ranged->convertTo0to1(value));
    const auto sanitized = ranged->convertFrom0to1(normalised);
    if (warningCount != nullptr && std::abs(sanitized - value) > 1.0e-4f)
        ++(*warningCount);
    return sanitized;
}

void PianoSynthAudioProcessor::setParamValueInternal(const juce::String& paramId, float value, bool notifyHost)
{
    if (auto* parameter = parameters.getParameter(paramId))
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
        const auto fallback = ranged != nullptr
            ? ranged->convertFrom0to1(ranged->getDefaultValue())
            : 0.0f;
        const auto sanitized = sanitizeParameterValue(paramId, value, fallback);
        const auto normalised = parameter->convertTo0to1(sanitized);
        if (notifyHost)
        {
            parameter->setValueNotifyingHost(normalised);
        }
        else
        {
            parameter->setValue(normalised);
            parameter->sendValueChangedMessageToListeners(normalised);
        }

        if (!suppressConditionalSanitization)
        {
            if (paramId == kSelectedPiano)
            {
                sanitizePianoDependentState(notifyHost, juce::roundToInt(sanitized));
            }
            else if (paramId == kLfoDestination)
            {
                sanitizePianoDependentState(notifyHost);
            }
        }
    }
}

void PianoSynthAudioProcessor::setParamValue(const juce::String& paramId, float value)
{
    setParamValueInternal(paramId, value, true);
}

void PianoSynthAudioProcessor::sanitizeAllParameterValues()
{
    static constexpr const char* kGlobalParamIds[] = {
        kOutputGain, kSelectedPiano, kLfoRate, kLfoDepth, kLfoWave, kLfoDestination,
        kMacroWarmth, kMacroBrillance, kMacroExpression, kMacroResonance,
        kCompThreshold, kCompRatio, kCompAttack, kCompRelease, kCompMakeup, kCompMix,
        kSatDrive, kSatMix,
        kTransientAttack, kTransientSustain, kTransientMix,
        kReverbSize, kReverbDamping, kReverbWidth, kReverbMix, kReverbPreDelay, kReverbEnabled,
        kEqLowFreq, kEqLowGain, kEqMidFreq, kEqMidGain, kEqMidQ, kEqHighFreq, kEqHighGain, kEqEnabled,
        kChorusRate, kChorusDepth, kChorusMix, kChorusEnabled,
        kDelayTime, kDelayFeedback, kDelayMix, kDelaySync, kDelayNoteDiv, kDelayEnabled,
        kLimiterThreshold, kLimiterRelease, kLimiterEnabled,
        kSaturationEnabled, kTransientEnabled, kCompressorEnabled, kFxLock
    };

    for (const auto* paramId : kGlobalParamIds)
        setParamValueInternal(paramId, getParamValue(paramId), false);

    static constexpr const char* kPianoParamSuffixes[] = {
        "level", "tune", kPianoHammerHardnessSuffix, "attack", "decay", "sustain", "release",
        kPianoToneBrightnessSuffix, kPianoStringResonanceSuffix, kPianoSoundboardAmountSuffix,
        "damping", kPianoModelCharacterSuffix, kPianoLowPassHzSuffix, "pan", kPianoOutputSuffix
    };

    for (int pianoIndex = 0; pianoIndex < mps::kNumPianos; ++pianoIndex)
        for (const auto* suffix : kPianoParamSuffixes)
            setParamValueInternal(makePianoParamId(pianoIndex, suffix),
                                  getParamValue(makePianoParamId(pianoIndex, suffix)),
                                  false);

    sanitizePianoDependentState(false);
}

void PianoSynthAudioProcessor::sanitizePianoDependentState(const bool notifyHost, const int pianoIndexOverride)
{
    if (suppressConditionalSanitization)
        return;

    juce::ScopedValueSetter<bool> scopedSanitization(suppressConditionalSanitization, true);
    const int pianoIndex = pianoIndexOverride >= 0
        ? juce::jlimit(0, mps::kNumPianos - 1, pianoIndexOverride)
        : getSelectedPianoIndex();

    const auto currentDestination = static_cast<mps::LfoDestination>(
        juce::jlimit(0, 3, static_cast<int>(std::round(getParamValue(kLfoDestination)))));
    const auto sanitizedDestination = sanitizeLfoDestinationForPiano(pianoIndex, currentDestination);

    if (sanitizedDestination != currentDestination)
    {
        if (auto* parameter = parameters.getParameter(kLfoDestination))
        {
            const auto normalised = parameter->convertTo0to1(static_cast<float>(sanitizedDestination));
            juce::ignoreUnused(notifyHost);
            parameter->setValue(normalised);
            parameter->sendValueChangedMessageToListeners(normalised);
        }
    }
}

mps::PresetPerformanceState PianoSynthAudioProcessor::snapshotPerformanceState() const
{
    mps::PresetPerformanceState state;
    state.macroWarmth = getParamValue(kMacroWarmth);
    state.macroBrillance = getParamValue(kMacroBrillance);
    state.macroExpression = getParamValue(kMacroExpression);
    state.macroResonance = getParamValue(kMacroResonance);
    state.lfoRate = getParamValue(kLfoRate);
    state.lfoDepth = getParamValue(kLfoDepth);
    state.lfoWave = juce::jlimit(0, 3, static_cast<int>(std::round(getParamValue(kLfoWave))));
    state.lfoDestination = sanitizeLfoDestinationForPiano(
        getSelectedPianoIndex(),
        static_cast<mps::LfoDestination>(
            juce::jlimit(0, 3, static_cast<int>(std::round(getParamValue(kLfoDestination))))));
    state.pitchBendRange = getParamValue(kPitchBendRange);
    state.velocityCurve = juce::jlimit(0, 6, static_cast<int>(std::round(getParamValue(kVelocityCurve))));
    state.monoMode = getParamValue(kMonoMode) >= 0.5f;
    state.tremoloSync = getParamValue(kTremoloSync) >= 0.5f;
    state.modWheelTarget = juce::jlimit(0, 1, static_cast<int>(std::round(getParamValue(kModWheelTarget))));
    state.modMatrixState = modulationMatrix.captureState();
    return state;
}

void PianoSynthAudioProcessor::applyPerformanceState(const mps::PresetPerformanceState& state)
{
    setParamValue(kMacroWarmth, state.macroWarmth);
    setParamValue(kMacroBrillance, state.macroBrillance);
    setParamValue(kMacroExpression, state.macroExpression);
    setParamValue(kMacroResonance, state.macroResonance);
    setParamValue(kLfoRate, state.lfoRate);
    setParamValue(kLfoDepth, state.lfoDepth);
    setParamValue(kLfoWave, static_cast<float>(state.lfoWave));
    setParamValue(kLfoDestination, static_cast<float>(
        sanitizeLfoDestinationForPiano(getSelectedPianoIndex(), state.lfoDestination)));
    setParamValue(kPitchBendRange, state.pitchBendRange);
    setParamValue(kVelocityCurve, static_cast<float>(state.velocityCurve));
    setParamValue(kMonoMode, state.monoMode ? 1.0f : 0.0f);
    setParamValue(kTremoloSync, state.tremoloSync ? 1.0f : 0.0f);
    setParamValue(kModWheelTarget, static_cast<float>(state.modWheelTarget));
    modulationMatrix.applyState(materializeModMatrixState(state));
}

mps::PianoSettings PianoSynthAudioProcessor::snapshotPianoSettings(int pianoIndex) const
{
    mps::PianoSettings s;
    s.level          = getParamValue(makePianoParamId(pianoIndex, "level"));
    s.tuneSemitones  = getParamValue(makePianoParamId(pianoIndex, "tune"));
    s.tone.hammerHardness = getParamValue(makePianoParamId(pianoIndex, kPianoHammerHardnessSuffix));
    s.envelope.attackSeconds  = getParamValue(makePianoParamId(pianoIndex, "attack"));
    s.envelope.decaySeconds   = getParamValue(makePianoParamId(pianoIndex, "decay"));
    s.envelope.sustainLevel   = getParamValue(makePianoParamId(pianoIndex, "sustain"));
    s.envelope.releaseSeconds = getParamValue(makePianoParamId(pianoIndex, "release"));
    s.tone.brightness     = getParamValue(makePianoParamId(pianoIndex, kPianoToneBrightnessSuffix));
    s.resonance.stringResonance = getParamValue(makePianoParamId(pianoIndex, kPianoStringResonanceSuffix));
    s.resonance.soundboardAmount     = getParamValue(makePianoParamId(pianoIndex, kPianoSoundboardAmountSuffix));
    s.resonance.damping        = getParamValue(makePianoParamId(pianoIndex, "damping"));
    s.performance.modelCharacter      = getParamValue(makePianoParamId(pianoIndex, kPianoModelCharacterSuffix));
    s.tone.lowPassHz       = getParamValue(makePianoParamId(pianoIndex, kPianoLowPassHzSuffix));
    s.spatial.pan            = getParamValue(makePianoParamId(pianoIndex, "pan"));

    applyPerformanceMacros(pianoIndex, s);

    // L6: BPM-sync tremolo rate (8th note = BPM/60/2)
    if (usesTempoSyncedTremolo(pianoIndex)
        && getParamValue("tremolo_sync") >= 0.5f
        && currentBpm > 0.0f)
        s.performance.tremoloRateHz = currentBpm / 60.0f * 0.5f;

    return s;
}

void PianoSynthAudioProcessor::applyPerformanceMacros(int pianoIndex, mps::PianoSettings& s) const
{
    const auto warmth = (getParamValue(kMacroWarmth) - 0.5f) * 2.0f;
    const auto brillance = (getParamValue(kMacroBrillance) - 0.5f) * 2.0f;
    const auto express = (getParamValue(kMacroExpression) - 0.5f) * 2.0f;
    const auto resonance = (getParamValue(kMacroResonance) - 0.5f) * 2.0f;

    const auto family = mps::getFamily(pianoIndex);
    switch (family)
    {
        case mps::Family::Concert:
            s.tone.brightness = clamp01(s.tone.brightness + warmth * 0.22f);
            s.tone.hammerHardness = clamp01(s.tone.hammerHardness + warmth * 0.10f);
            s.tone.lowPassHz = juce::jlimit(120.0f, 18000.0f, s.tone.lowPassHz * std::pow(2.0f, warmth * 0.55f));

            s.tone.brightness = clamp01(s.tone.brightness + brillance * 0.10f);
            s.envelope.attackSeconds = juce::jlimit(0.0f, 2.0f, s.envelope.attackSeconds * (1.0f - brillance * 0.22f));
            s.level = juce::jlimit(0.0f, 1.0f, s.level + brillance * 0.06f);

            s.resonance.soundboardAmount = clamp01(s.resonance.soundboardAmount + express * 0.20f);
            s.envelope.decaySeconds = juce::jlimit(0.1f, 10.0f, s.envelope.decaySeconds * (1.0f + express * 0.18f));
            s.envelope.releaseSeconds = juce::jlimit(0.01f, 5.0f, s.envelope.releaseSeconds * (1.0f + express * 0.20f));

            s.resonance.stringResonance = clamp01(s.resonance.stringResonance + resonance * 0.22f);
            s.resonance.soundboardAmount = clamp01(s.resonance.soundboardAmount + resonance * 0.14f);
            s.resonance.damping = clamp01(s.resonance.damping - resonance * 0.12f);
            break;

        case mps::Family::Vintage:
            s.tone.brightness = clamp01(s.tone.brightness - warmth * 0.18f);
            s.tone.hammerHardness = clamp01(s.tone.hammerHardness - warmth * 0.08f);
            s.performance.modelCharacter = clamp01(s.performance.modelCharacter + warmth * 0.24f);
            s.resonance.damping = clamp01(s.resonance.damping + warmth * 0.10f);

            s.performance.modelCharacter = clamp01(s.performance.modelCharacter + brillance * 0.18f);
            s.tuneSemitones = juce::jlimit(-24.0f, 24.0f, s.tuneSemitones + brillance * 0.18f);
            s.tone.brightness = clamp01(s.tone.brightness + brillance * 0.08f);

            s.envelope.attackSeconds = juce::jlimit(0.0f, 2.0f, s.envelope.attackSeconds * (1.0f - express * 0.30f));
            s.tone.hammerHardness = clamp01(s.tone.hammerHardness + express * 0.18f);
            s.envelope.decaySeconds = juce::jlimit(0.1f, 10.0f, s.envelope.decaySeconds * (1.0f - express * 0.18f));

            s.resonance.soundboardAmount = clamp01(s.resonance.soundboardAmount + resonance * 0.24f);
            s.envelope.releaseSeconds = juce::jlimit(0.01f, 5.0f, s.envelope.releaseSeconds * (1.0f + resonance * 0.25f));
            s.resonance.stringResonance = clamp01(s.resonance.stringResonance + resonance * 0.10f);
            break;

        case mps::Family::Electric:
            s.tone.hammerHardness = clamp01(s.tone.hammerHardness + warmth * 0.22f);
            s.performance.modelCharacter = clamp01(s.performance.modelCharacter + warmth * 0.18f);
            s.level = juce::jlimit(0.0f, 1.0f, s.level + warmth * 0.04f);

            s.tone.brightness = clamp01(s.tone.brightness + brillance * 0.20f);
            s.tone.lowPassHz = juce::jlimit(120.0f, 18000.0f, s.tone.lowPassHz * std::pow(2.0f, brillance * 0.42f));
            s.resonance.stringResonance = clamp01(s.resonance.stringResonance + brillance * 0.08f);

            s.performance.modelCharacter = clamp01(s.performance.modelCharacter + express * 0.20f);
            s.tone.lowPassHz = juce::jlimit(120.0f, 18000.0f, s.tone.lowPassHz * std::pow(2.0f, express * 0.28f));
            s.tone.brightness = clamp01(s.tone.brightness + express * 0.08f);

            s.spatial.pan = juce::jlimit(-1.0f, 1.0f, s.spatial.pan + resonance * 0.08f);
            s.envelope.releaseSeconds = juce::jlimit(0.01f, 5.0f, s.envelope.releaseSeconds * (1.0f + resonance * 0.25f));
            s.resonance.soundboardAmount = clamp01(s.resonance.soundboardAmount + resonance * 0.08f);
            break;
    }
}

int PianoSynthAudioProcessor::findFreeVoice() const
{
    // 1. Prefer free/inactive slots
    for (int i = 0; i < kMaxVoices; ++i)
        if (!voices[static_cast<std::size_t>(i)].active || !voices[static_cast<std::size_t>(i)].active->isActive())
            return i;

    // 2. Steal oldest releasing voice
    int bestReleasing = -1;
    std::uint64_t oldestReleasingOrder = std::numeric_limits<std::uint64_t>::max();
    for (int i = 0; i < kMaxVoices; ++i)
    {
        const auto& v = voices[static_cast<std::size_t>(i)];
        if (v.active && v.active->isReleasing() && v.noteOnOrder < oldestReleasingOrder)
        {
            oldestReleasingOrder = v.noteOnOrder;
            bestReleasing = i;
        }
    }
    if (bestReleasing >= 0)
        return bestReleasing;

    // 3. Steal oldest active voice
    int bestActive = 0;
    std::uint64_t oldestActiveOrder = std::numeric_limits<std::uint64_t>::max();
    for (int i = 0; i < kMaxVoices; ++i)
    {
        const auto& v = voices[static_cast<std::size_t>(i)];
        if (v.noteOnOrder < oldestActiveOrder)
        {
            oldestActiveOrder = v.noteOnOrder;
            bestActive = i;
        }
    }
    return bestActive;
}

int PianoSynthAudioProcessor::findFreeVoiceForNote(int midiNote, int pianoIndex) const
{
    int newestActiveMatch = -1;
    std::uint64_t newestActiveOrder = 0;

    for (int i = 0; i < kMaxVoices; ++i)
    {
        const auto& v = voices[static_cast<std::size_t>(i)];
        if (v.active && v.midiNote == midiNote && v.pianoIndex == pianoIndex)
        {
            if (!v.active->isActive())
                return i;
            if (v.active->isReleasing())
                return i;

            if (newestActiveMatch < 0 || v.noteOnOrder >= newestActiveOrder)
            {
                newestActiveMatch = i;
                newestActiveOrder = v.noteOnOrder;
            }
        }
    }

    if (newestActiveMatch >= 0)
        return newestActiveMatch;

    return findFreeVoice();
}

void PianoSynthAudioProcessor::clearVoice(VoiceSlot& slot)
{
    slot.active = nullptr;
    slot.dying  = nullptr;
    slot.dyingBus = 0;
    slot.activeVelocity = 0.5f;
    slot.dyingVelocity = 0.5f;
    slot.renderBus = 0;
    slot.previousRenderBus = 0;
    slot.midiNote = -1;
    slot.pianoIndex = 0;
    slot.midiChannel = 1;
    slot.keyDown = false;
    slot.deferredNoteOff = false;
    slot.sostenutoCaptured = false;
    slot.attackBlendRemaining = 0;
    slot.routeFadeRemaining = 0;
    slot.noteOnOrder = 0;
}

// =========================================================================
// MIDI CC page-based mapping  (Novation FLkey Mini – full control)
//
// 8 knobs (CC 21-28) are paged across 7 pages to control ALL parameters.
// CC 1 (mod wheel / touch-strip) controls macro_expression when mod_wheel_target != Off.
// CC 102 / 103  = previous / next page  (assignable to < > buttons).
// CC 44-50      = direct page select    (assignable to pads in CC mode).
// Each CC value (0-127) is normalised to the parameter's full range.
// Per-piano parameters follow the currently selected piano index.
// =========================================================================

namespace
{
    struct CCSlot {
        const char* paramId;        // global param, or nullptr for per-piano
        const char* pianoSuffix;    // per-piano suffix (when paramId == nullptr)
    };

    static constexpr int kKnobsPerPage = 8;

    // Page names shown in UI
    static const char* kCCPageNames[] = {
        "MACROS",       // 0
        "ENVELOPE",     // 1
        "TONE",         // 2
        "REVERB/DELAY", // 3
        "DYNAMICS",     // 4
        "EQ",           // 5
        "MOD/LIMITER"   // 6
    };

    // 7 pages x 8 knobs – covers every tweakable parameter
    static const CCSlot kCCPages[][kKnobsPerPage] = {
        // Page 0 ─ Macros & Master
        { { "macro_warmth",    nullptr }, { "macro_brillance", nullptr },
          { "macro_resonance", nullptr }, { "lfo_rate",        nullptr },
          { "lfo_depth",       nullptr }, { "reverb_mix",      nullptr },
          { "chorus_mix",      nullptr }, { "output_gain",     nullptr } },

        // Page 1 ─ Envelope (per-piano)
        { { nullptr, "attack" },           { nullptr, "decay"  },
          { nullptr, "sustain" },          { nullptr, "release" },
          { nullptr, "hammer_hardness" },  { nullptr, "tone_brightness" },
          { nullptr, "level" },            { nullptr, "tune" } },

        // Page 2 ─ Tone (per-piano)
        { { nullptr, "string_resonance" },  { nullptr, "soundboard_amount" },
          { nullptr, "model_character" },   { nullptr, "damping" },
          { nullptr, "low_pass_hz" },       { nullptr, "pan" },
          { "delay_mix", nullptr },         { "sat_mix",   nullptr } },

        // Page 3 ─ Reverb & Delay
        { { "reverb_size",     nullptr }, { "reverb_damping",  nullptr },
          { "reverb_width",    nullptr }, { "reverb_mix",      nullptr },
          { "reverb_predelay", nullptr }, { "delay_time",      nullptr },
          { "delay_feedback",  nullptr }, { "delay_mix",       nullptr } },

        // Page 4 ─ Dynamics (Compressor + Saturation)
        { { "comp_threshold", nullptr }, { "comp_ratio",  nullptr },
          { "comp_attack",    nullptr }, { "comp_release", nullptr },
          { "comp_makeup",    nullptr }, { "comp_mix",     nullptr },
          { "sat_drive",      nullptr }, { "sat_mix",      nullptr } },

        // Page 5 ─ EQ
        { { "eq_low_freq",  nullptr }, { "eq_low_gain",  nullptr },
          { "eq_mid_freq",  nullptr }, { "eq_mid_gain",  nullptr },
          { "eq_mid_q",     nullptr }, { "eq_high_freq", nullptr },
          { "eq_high_gain", nullptr }, { "transient_mix", nullptr } },

        // Page 6 ─ Modulation (Chorus + Transient + Limiter)
        { { "chorus_rate",       nullptr }, { "chorus_depth",      nullptr },
          { "chorus_mix",        nullptr }, { "transient_attack",  nullptr },
          { "transient_sustain", nullptr }, { "transient_mix",     nullptr },
          { "limiter_threshold", nullptr }, { "limiter_release",   nullptr } }
    };
}

const char* PianoSynthAudioProcessor::getCCPageName(int page) noexcept
{
    if (page >= 0 && page < kNumCCPages)
        return kCCPageNames[page];
    return "???";
}

void PianoSynthAudioProcessor::handleMidiCC(int ccNumber, int ccValue, int pianoIndex)
{
    const float normalisedCc = static_cast<float>(ccValue) / 127.0f;

    // --- MIDI Learn: capture incoming CC when a param is armed ---
    if (midiLearnArmed.load(std::memory_order_acquire))
    {
        if (auto* param = midiLearnArmedParam.load(std::memory_order_acquire))
        {
            pendingMidiLearnValue.store(normalisedCc, std::memory_order_release);
            pendingMidiLearnParam.store(param, std::memory_order_release);
            pendingMidiLearnCc.store(ccNumber, std::memory_order_release);
            midiLearnArmedParam.store(nullptr, std::memory_order_release);
            midiLearnArmed.store(false, std::memory_order_release);
            triggerAsyncUpdate();
        }
        return;
    }

    // --- MIDI Learn: apply learned mappings before FLkey pages ---
    if (ccNumber >= 0 && ccNumber < static_cast<int>(midiLearnParamSnapshot.size()))
    {
        if (auto* param = midiLearnParamSnapshot[static_cast<std::size_t>(ccNumber)].load(std::memory_order_acquire))
        {
            queueParamUpdate(param, normalisedCc);
            return;
        }
    }

    // --- Mod wheel: controls expression macro AND feeds modulation matrix ---
    if (ccNumber == 1)
    {
        modulationMatrix.modWheelValue.store(normalisedCc, std::memory_order_relaxed);

        // Only map to macro if mod_wheel_target != Off (index 0)
        if (modWheelTargetRaw == nullptr || modWheelTargetRaw->load(std::memory_order_relaxed) >= 0.5f)
        {
            if (auto* param = parameters.getParameter("macro_expression"))
                queueParamUpdate(param, normalisedCc);
        }
        return;
    }

    // --- Page navigation: CC 102 = prev, CC 103 = next ---
    if (ccNumber == 102 && ccValue > 0)
    {
        int p = midiCCPage.load(std::memory_order_relaxed);
        midiCCPage.store((p + kNumCCPages - 1) % kNumCCPages, std::memory_order_relaxed);
        return;
    }
    if (ccNumber == 103 && ccValue > 0)
    {
        int p = midiCCPage.load(std::memory_order_relaxed);
        midiCCPage.store((p + 1) % kNumCCPages, std::memory_order_relaxed);
        return;
    }

    // --- Direct page select via pads: CC 44-50 ---
    if (ccNumber >= 44 && ccNumber <= 50 && ccValue > 0)
    {
        midiCCPage.store(ccNumber - 44, std::memory_order_relaxed);
        return;
    }

    // --- Knob mapping: CC 21-28 -> paged parameters ---
    if (ccNumber < 21 || ccNumber > 28)
        return;

    const int knobIndex = ccNumber - 21;
    const int page = midiCCPage.load(std::memory_order_relaxed);
    const auto& slot = kCCPages[page][knobIndex];

    juce::String paramId;
    if (slot.paramId != nullptr)
        paramId = slot.paramId;
    else if (slot.pianoSuffix != nullptr)
        paramId = makePianoParamId(pianoIndex, slot.pianoSuffix);
    else
        return;

    if (auto* param = parameters.getParameter(paramId))
        queueParamUpdate(param, normalisedCc);
}

void PianoSynthAudioProcessor::handleSustainPedal(int midiChannel, float damperValue)
{
    if (midiChannel < 1 || midiChannel > static_cast<int>(sustainPedalPosition.size()))
        return;

    const float newPos = juce::jlimit(0.0f, 1.0f, damperValue);
    const auto channelIndex = static_cast<std::size_t>(midiChannel - 1);
    auto& channelSustain = sustainPedalPosition[channelIndex];
    auto& sustainLatched = sustainHoldLatched[channelIndex];
    const float oldPos = channelSustain;
    channelSustain = newPos;
    const bool wasHolding = sustainLatched;
    if (sustainLatched)
        sustainLatched = newPos >= kSustainHoldExit;
    else
        sustainLatched = newPos >= kSustainHoldEnter;
    const bool isHolding = sustainLatched;

    if (std::abs(newPos - oldPos) >= 0.08f)
    {
        pedalNoiseLevel = juce::jmax(pedalNoiseLevel,
            mps::kPedalNoiseLevel * (0.35f + 0.65f * std::abs(newPos - oldPos)));
        pedalNoiseDecay = std::exp(-1.0f / (mps::kPedalNoiseDecaySec
            * static_cast<float>(juce::jmax(1.0, preparedSampleRate))));
    }

    // Update damper position on all active voices for this channel
    for (auto& slot : voices)
    {
        if (slot.active && slot.active->isActive() && slot.midiChannel == midiChannel)
        {
            if (slot.active->isAcoustic())
                static_cast<mps::AcousticPianoVoiceBase*>(slot.active)->setDamperPosition(newPos);
        }
    }

    // Release fully-held notes once the pedal drops below the hold threshold.
    if (wasHolding && !isHolding)
    {
        for (auto& slot : voices)
        {
            if (slot.active && slot.active->isActive() && !slot.active->isReleasing()
                && slot.midiChannel == midiChannel
                && !slot.keyDown && slot.deferredNoteOff)
            {
                slot.active->noteOff();
                slot.deferredNoteOff = false;
            }
        }
    }

    // Re-pedaling: only recapture voices that are still audibly alive.
    if (!wasHolding && isHolding)
    {
        int channelActiveCount = 0;
        for (const auto& slot : voices)
            if (slot.active && slot.active->isActive() && slot.midiChannel == midiChannel)
                ++channelActiveCount;

        for (auto& slot : voices)
        {
            bool hasNewerHeldSameNote = false;
            for (const auto& other : voices)
            {
                if (&other == &slot)
                    continue;
                if (other.active && other.active->isActive()
                    && other.midiChannel == midiChannel
                    && other.pianoIndex == slot.pianoIndex
                    && other.midiNote == slot.midiNote
                    && other.keyDown
                    && other.noteOnOrder > slot.noteOnOrder)
                {
                    hasNewerHeldSameNote = true;
                    break;
                }
            }

            if (slot.active && slot.active->isActive()
                && slot.active->isReleasing()
                && slot.midiChannel == midiChannel
                && slot.active->getLevelEstimate() > 0.006f
                && channelActiveCount < 16
                && !hasNewerHeldSameNote)
            {
                slot.active->reSustain();
                slot.deferredNoteOff = true;
            }
        }
    }
}

void PianoSynthAudioProcessor::handleSostenutoPedal(int midiChannel, bool on)
{
    if (midiChannel < 1 || midiChannel > 16) return;

    auto& state = sostenutoPedalDown[static_cast<std::size_t>(midiChannel - 1)];
    state = on;

    if (on)
    {
        // Capture all currently-held notes (not releasing, key still down or sustained)
        for (auto& slot : voices)
        {
            if (slot.active && slot.active->isActive()
                && !slot.active->isReleasing()
                && slot.midiChannel == midiChannel)
            {
                slot.sostenutoCaptured = true;
            }
        }
    }
    else
    {
        // Release all sostenuto-captured voices whose keys are not held
        for (auto& slot : voices)
        {
            if (slot.sostenutoCaptured && slot.midiChannel == midiChannel)
            {
                slot.sostenutoCaptured = false;
                if (slot.active && slot.active->isActive()
                    && !slot.active->isReleasing()
                    && !slot.keyDown && !slot.deferredNoteOff)
                {
                    slot.active->noteOff();
                }
            }
        }
    }
}

void PianoSynthAudioProcessor::handleUnaCordaPedal(int midiChannel, bool on)
{
    if (midiChannel < 1 || midiChannel > 16) return;

    unaCordaPedalDown[static_cast<std::size_t>(midiChannel - 1)] = on;

    for (auto& slot : voices)
    {
        if (slot.active && slot.active->isActive() && slot.midiChannel == midiChannel)
            slot.active->setUnaCorda(on);
    }
}

void PianoSynthAudioProcessor::releaseVoices(int midiChannel, bool immediate)
{
    for (auto& slot : voices)
    {
        if (!slot.active || !slot.active->isActive())
            continue;
        if (midiChannel != 0 && slot.midiChannel != midiChannel)
            continue;

        if (immediate)
        {
            clearVoice(slot);
            continue;
        }

        slot.keyDown = false;
        slot.deferredNoteOff = false;
        if (!slot.active->isReleasing())
            slot.active->noteOff();
    }
}

void PianoSynthAudioProcessor::panicAllVoices()
{
    for (auto& pedalState : sustainPedalPosition)
        pedalState = 0.0f;
    for (auto& sustainState : sustainHoldLatched)
        sustainState = false;

    pitchBend.reset();
    for (auto& slot : voices)
        clearVoice(slot);
}

void PianoSynthAudioProcessor::triggerNoteOn(int pianoIndex, int midiChannel, int midiNote, float velocity)
{
    if (pianoIndex < 0 || pianoIndex >= mps::kNumPianos) return;
    if (preparedSampleRate <= 0.0) return;
    if (midiChannel < 1 || midiChannel > 16) return;

    const bool monoMode = getParamValue("mono_mode") >= 0.5f;
    int monoReuseSlot = -1;
    std::uint64_t newestSamePianoOrder = 0;

    if (monoMode)
    {
        for (int i = 0; i < kMaxVoices; ++i)
        {
            const auto& vs = voices[static_cast<std::size_t>(i)];
            if (vs.active && vs.active->isActive() && vs.pianoIndex == pianoIndex
                && (monoReuseSlot < 0 || vs.noteOnOrder >= newestSamePianoOrder))
            {
                monoReuseSlot = i;
                newestSamePianoOrder = vs.noteOnOrder;
            }
        }

        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (i == monoReuseSlot)
                continue;

            auto& vs = voices[static_cast<std::size_t>(i)];
            if (vs.active && vs.active->isActive() && vs.pianoIndex == pianoIndex)
            {
                vs.active->forceQuickRelease();
                vs.keyDown = false;
                vs.deferredNoteOff = false;
            }
        }
    }

    // Piano retrigger policy: a new attack owns the pitch. Older released tails on
    // the same key are damped quickly to keep repeated notes readable.
    for (auto& vs : voices)
    {
        if (vs.active && vs.active->isActive()
            && vs.pianoIndex == pianoIndex
            && vs.midiChannel == midiChannel
            && vs.midiNote == midiNote
            && (!vs.keyDown || vs.active->isReleasing() || vs.deferredNoteOff))
        {
            vs.active->forceQuickRelease();
            vs.keyDown = false;
            vs.deferredNoteOff = false;
            vs.sostenutoCaptured = false;
        }
    }

    const int slot = monoReuseSlot >= 0 ? monoReuseSlot : findFreeVoiceForNote(midiNote, pianoIndex);
    const bool monoSteal = monoMode && slot == monoReuseSlot;
    auto& v = voices[static_cast<std::size_t>(slot)];

    // Handle voice stealing
    if (v.active && v.active->isActive())
    {
        if (v.pianoIndex != pianoIndex)
        {
            // Cross-piano steal: old voiceBank entry survives as dying voice
            v.dying = v.active;
            v.dyingVelocity = v.activeVelocity;
            v.dying->forceQuickRelease();
            v.dyingBus = juce::jlimit(0, kNumAuxOutputs, v.renderBus);
        }
        else
        {
            // Same-piano steal: use 64-sample StealFade buffer capture
            for (auto& sf : stealFades)
            {
                if (sf.remaining == 0)
                {
                    if (monoSteal)
                        v.active->forceQuickRelease();

                    stealScratchBuffer.clear();
                    v.active->render(stealScratchBuffer, 0, kStealFadeSamples);
                    for (int i = 0; i < kStealFadeSamples; ++i)
                    {
                        const float g = 1.0f - static_cast<float>(i) / static_cast<float>(kStealFadeSamples);
                        sf.left[static_cast<std::size_t>(i)]  = stealScratchBuffer.getSample(0, i) * g;
                        sf.right[static_cast<std::size_t>(i)] = stealScratchBuffer.getNumChannels() > 1
                                                                ? stealScratchBuffer.getSample(1, i) * g : 0.0f;
                    }
                    sf.remaining = kStealFadeSamples;
                    sf.busIndex = juce::jlimit(0, kNumAuxOutputs, v.renderBus);
                    break;
                }
            }
        }
    }

    v.active = v.voiceBank[static_cast<std::size_t>(pianoIndex)].get();
    v.midiNote = midiNote;
    v.pianoIndex = pianoIndex;
    v.midiChannel = midiChannel;
    v.activeVelocity = juce::jlimit(0.0f, 1.0f, velocity);
    v.keyDown = true;
    v.deferredNoteOff = false;
    v.sostenutoCaptured = false;
    v.attackBlendRemaining = monoSteal ? kStealFadeSamples : 0;
    v.renderBus = juce::jlimit(
        0, kNumAuxOutputs,
        static_cast<int>(std::round(getParamValue(makePianoParamId(pianoIndex, kPianoOutputSuffix)))));
    v.previousRenderBus = v.renderBus;
    v.routeFadeRemaining = 0;
    v.noteOnOrder = nextNoteOnOrder++;
    v.active->setRandomSeed(deterministicRenderMode
        ? makeDeterministicVoiceSeed(slot, pianoIndex, midiChannel, midiNote, v.noteOnOrder)
        : makeRandomSeed());

    auto settings = snapshotPianoSettings(pianoIndex);
    v.active->noteOn(settings, midiNote, velocity, preparedSampleRate);
    lastNoteVelocity = velocity;

    // Set current damper position for half-pedaling
    if (midiChannel >= 1 && midiChannel <= static_cast<int>(sustainPedalPosition.size()))
    {
        if (v.active->isAcoustic())
            static_cast<mps::AcousticPianoVoiceBase*>(v.active)->setDamperPosition(sustainPedalPosition[static_cast<std::size_t>(midiChannel - 1)]);
    }

    // Apply current una corda state
    if (midiChannel >= 1 && midiChannel <= 16
        && unaCordaPedalDown[static_cast<std::size_t>(midiChannel - 1)])
    {
        v.active->setUnaCorda(true);
    }
}

void PianoSynthAudioProcessor::triggerNoteOff(int midiChannel, int midiNote)
{
    VoiceSlot* targetVoice = nullptr;
    std::uint64_t newestOrder = 0;

    for (auto& slot : voices)
    {
        if (slot.active && slot.active->isActive() && !slot.active->isReleasing()
            && slot.keyDown && slot.midiNote == midiNote
            && slot.midiChannel == midiChannel
            && slot.noteOnOrder >= newestOrder)
        {
            newestOrder = slot.noteOnOrder;
            targetVoice = &slot;
        }
    }

    if (targetVoice == nullptr)
        return;

    targetVoice->keyDown = false;

    // Sostenuto holds captured voices even after key release
    if (targetVoice->sostenutoCaptured)
    {
        targetVoice->deferredNoteOff = true;
        return;
    }

    if (midiChannel >= 1 && midiChannel <= static_cast<int>(sustainPedalPosition.size())
        && sustainHoldLatched[static_cast<std::size_t>(midiChannel - 1)])
    {
        targetVoice->deferredNoteOff = true;
        return;
    }

    targetVoice->deferredNoteOff = false;
    targetVoice->active->noteOff();
}

// =============================================================================
// Global FX
// =============================================================================
void PianoSynthAudioProcessor::updateGlobalEffectParameters()
{
    const auto threshold = getParamValue(kCompThreshold);
    const auto ratio     = getParamValue(kCompRatio);
    const auto attack    = getParamValue(kCompAttack);
    const auto release   = getParamValue(kCompRelease);

    if (threshold != compCache.threshold) { compressor.setThreshold(threshold); compCache.threshold = threshold; }
    if (ratio     != compCache.ratio)     { compressor.setRatio(ratio);          compCache.ratio     = ratio; }
    if (attack    != compCache.attack)    { compressor.setAttack(attack);         compCache.attack    = attack; }
    if (release   != compCache.release)   { compressor.setRelease(release);       compCache.release   = release; }
}

void PianoSynthAudioProcessor::processGlobalTransient(juce::AudioBuffer<float>& mainBuffer)
{
    if (!isFxAvailableForCurrentPiano(mps::GlobalFxSlot::Transient)
        || getParamValue("fx_tab1_en") < 0.5f) return;
    const auto mix     = clamp01(getParamValue(kTransientMix));
    const auto attack  = juce::jlimit(-1.0f, 1.0f, getParamValue(kTransientAttack));
    const auto sustain = juce::jlimit(-1.0f, 1.0f, getParamValue(kTransientSustain));

    if (mix <= 0.0001f || (std::abs(attack) <= 0.0001f && std::abs(sustain) <= 0.0001f))
        return;

    const auto sampleRate = static_cast<float>(std::max(1.0, preparedSampleRate));
    const auto fastCoeff = std::exp(-1.0f / (0.0018f * sampleRate));
    const auto slowCoeff = std::exp(-1.0f / (0.055f  * sampleRate));

    for (int ch = 0; ch < mainBuffer.getNumChannels(); ++ch)
    {
        auto* data = mainBuffer.getWritePointer(ch);
        auto& fast = transientFastEnv[static_cast<std::size_t>(juce::jlimit(0, 1, ch))];
        auto& slow = transientSlowEnv[static_cast<std::size_t>(juce::jlimit(0, 1, ch))];

        for (int i = 0; i < mainBuffer.getNumSamples(); ++i)
        {
            const auto dry = data[i];
            const auto absSample = std::abs(dry);
            fast = fastCoeff * fast + (1.0f - fastCoeff) * absSample;
            slow = slowCoeff * slow + (1.0f - slowCoeff) * absSample;

            const auto transient = fast - slow;
            const auto gain = juce::jlimit(0.2f, 4.0f,
                1.0f + attack * std::max(0.0f, transient) * 7.0f
                     + sustain * std::max(0.0f, -transient) * 5.0f);
            data[i] = dry + (dry * gain - dry) * mix;
        }
    }
}

void PianoSynthAudioProcessor::processGlobalSaturator(juce::AudioBuffer<float>& mainBuffer)
{
    if (!isFxAvailableForCurrentPiano(mps::GlobalFxSlot::Saturator)
        || getParamValue("fx_tab0_en") < 0.5f) return;
    const auto mix = clamp01(getParamValue(kSatMix));
    if (mix <= 0.0001f) return;

    const auto drive = juce::jlimit(1.0f, 16.0f, getParamValue(kSatDrive));
    const auto norm  = 1.0f / std::max(0.0001f, std::tanh(drive));

    // 2x oversampling to reduce aliasing from tanh waveshaping
    juce::dsp::AudioBlock<float> block(mainBuffer);
    auto osBlock = satOversampler.processSamplesUp(block);

    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        auto* data = osBlock.getChannelPointer(ch);
        for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
        {
            const auto dry = data[i];
            const auto wet = std::tanh(dry * drive) * norm;
            data[i] = dry + (wet - dry) * mix;
        }
    }

    satOversampler.processSamplesDown(block);
}

void PianoSynthAudioProcessor::processGlobalCompressor(juce::AudioBuffer<float>& mainBuffer)
{
    if (!isFxAvailableForCurrentPiano(mps::GlobalFxSlot::Compressor)
        || getParamValue("fx_tab2_en") < 0.5f) return;
    const auto mix = clamp01(getParamValue(kCompMix));
    const auto makeupGain = juce::Decibels::decibelsToGain(getParamValue(kCompMakeup));

    if (mix <= 0.0001f && std::abs(makeupGain - 1.0f) <= 0.0001f)
        return;

    updateGlobalEffectParameters();
    const int numCh = mainBuffer.getNumChannels();
    const int numSamples = mainBuffer.getNumSamples();
    if (fxDryBuffer.getNumChannels() < numCh || fxDryBuffer.getNumSamples() < numSamples)
        return;

    for (int ch = 0; ch < numCh; ++ch)
        fxDryBuffer.copyFrom(ch, 0, mainBuffer, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> block(mainBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);
    mainBuffer.applyGain(makeupGain);

    if (mix < 0.9999f)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* wet = mainBuffer.getWritePointer(ch);
            const auto* dry = fxDryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] + (wet[i] - dry[i]) * mix;
        }
    }
}

void PianoSynthAudioProcessor::applyGlobalLfo(juce::AudioBuffer<float>& mainBuffer)
{
    const auto numCh = mainBuffer.getNumChannels();
    const auto numSamples = mainBuffer.getNumSamples();
    if (numCh <= 0 || numSamples <= 0) return;

    const float rateHz = juce::jlimit(0.05f, 12.0f, getParamValue(kLfoRate))
                       * cachedModResult.lfo1RateMul;
    const float depth  = clamp01(getParamValue(kLfoDepth));
    const auto destination = static_cast<mps::LfoDestination>(
        juce::jlimit(0, 3, static_cast<int>(std::round(getParamValue(kLfoDestination)))));

    const int wave = juce::jlimit(0, 3, static_cast<int>(std::round(getParamValue(kLfoWave))));
    const float phaseInc = rateHz / static_cast<float>(juce::jmax(1.0, preparedSampleRate));
    auto advancePhaseOnly = [&]()
    {
        lfoPhase += phaseInc * static_cast<float>(numSamples);
        lfoPhase -= std::floor(lfoPhase);
    };

    if (depth <= 0.0001f
        || destination == mps::LfoDestination::Off
        || destination == mps::LfoDestination::ChorusMotion)
    {
        advancePhaseOnly();
        return;
    }

    constexpr float kTremDepth = 0.45f;
    constexpr float kPanDepth  = 0.65f;

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = numCh > 1 ? mainBuffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        float lfo = 0.0f;
        switch (wave)
        {
            case 1: lfo = 1.0f - 4.0f * std::abs(lfoPhase - 0.5f); break;
            case 2: lfo = lfoPhase * 2.0f - 1.0f; break;
            case 3: lfo = lfoPhase < 0.5f ? 1.0f : -1.0f; break;
            default: lfo = mps::fastSin(lfoPhase); break;
        }

        if (destination == mps::LfoDestination::Tremolo)
        {
            const float tremAmt = depth * kTremDepth;
            const float trem = 1.0f - tremAmt * 0.5f + lfo * tremAmt * 0.5f;
            left[i] *= trem;
            if (right != nullptr)
                right[i] *= trem;
        }
        else
        {
            if (right != nullptr)
            {
                const float pan = juce::jlimit(-1.0f, 1.0f, lfo * depth * kPanDepth);
                const float gL = std::sqrt(0.5f * (1.0f - pan));
                const float gR = std::sqrt(0.5f * (1.0f + pan));
                left[i]  *= gL;
                right[i] *= gR;
            }
        }

        lfoPhase += phaseInc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    }
}

// =============================================================================
// Global EQ (3-Band Parametric)
// =============================================================================
void PianoSynthAudioProcessor::processGlobalEQ(juce::AudioBuffer<float>& mainBuffer)
{
    if (!isFxAvailableForCurrentPiano(mps::GlobalFxSlot::Eq)
        || getParamValue("fx_tab3_en") < 0.5f) return;

    const int numCh      = mainBuffer.getNumChannels();
    const int numSamples = mainBuffer.getNumSamples();
    if (numCh <= 0 || numSamples <= 0) return;

    mps::fx::ParametricEQ3Band::Params ep;
    ep.lowFreq   = getParamValue(kEqLowFreq);
    ep.lowGainDb = getParamValue(kEqLowGain);
    ep.midFreq   = getParamValue(kEqMidFreq) * std::exp2(cachedModResult.eqMidFreqAdd);
    ep.midGainDb = getParamValue(kEqMidGain) + cachedModResult.eqMidGainAdd;
    ep.midQ      = getParamValue(kEqMidQ);
    ep.highFreq  = getParamValue(kEqHighFreq);
    ep.highGainDb = getParamValue(kEqHighGain);

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = numCh > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    parametricEQ.process(left, right, numSamples, ep);
}

// =============================================================================
// Global Chorus
// =============================================================================
void PianoSynthAudioProcessor::processGlobalChorus(juce::AudioBuffer<float>& mainBuffer)
{
    if (!isFxAvailableForCurrentPiano(mps::GlobalFxSlot::Chorus)
        || getParamValue("fx_tab4_en") < 0.5f) return;

    const int numCh      = mainBuffer.getNumChannels();
    const int numSamples = mainBuffer.getNumSamples();
    if (numCh <= 0 || numSamples <= 0) return;

    mps::fx::StereoChorus::Params cp;
    cp.rateHz = getParamValue(kChorusRate);
    cp.depth  = getParamValue(kChorusDepth);
    cp.mix    = getParamValue(kChorusMix);
    const auto destination = static_cast<mps::LfoDestination>(
        juce::jlimit(0, 3, static_cast<int>(std::round(getParamValue(kLfoDestination)))));
    const auto lfoDepth = clamp01(getParamValue(kLfoDepth));
    if (destination == mps::LfoDestination::ChorusMotion && lfoDepth > 0.0001f)
    {
        const float lfo = mps::fastSin(lfoPhase);
        cp.rateHz = juce::jlimit(0.1f, 5.0f, cp.rateHz * (1.0f + lfoDepth * 0.45f * lfo));
        cp.depth = clamp01(cp.depth + lfoDepth * (0.18f + 0.18f * lfo));
        cp.mix = clamp01(cp.mix + lfoDepth * 0.10f);
    }

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = numCh > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    stereoChorus.process(left, right, numSamples, cp);
}

// =============================================================================
// Global Delay (optional BPM sync)
// =============================================================================
void PianoSynthAudioProcessor::processGlobalDelay(juce::AudioBuffer<float>& mainBuffer)
{
    if (!isFxAvailableForCurrentPiano(mps::GlobalFxSlot::Delay)
        || getParamValue("fx_tab5_en") < 0.5f) return;

    const int numCh      = mainBuffer.getNumChannels();
    const int numSamples = mainBuffer.getNumSamples();
    if (numCh <= 0 || numSamples <= 0) return;

    mps::fx::StereoDelay::Params dp;
    dp.timeMs   = getParamValue(kDelayTime);
    dp.feedback = getParamValue(kDelayFeedback);
    dp.mix      = getParamValue(kDelayMix);
    dp.syncToBpm = getParamValue(kDelaySync) >= 0.5f;
    dp.noteDiv   = static_cast<int>(std::round(getParamValue(kDelayNoteDiv)));

    // Get host BPM if available; disable sync if no valid BPM (standalone, offline bounce)
    dp.bpm = 120.0f;
    bool bpmValid = false;
    if (auto* ph = getPlayHead())
    {
        if (auto posInfo = ph->getPosition())
        {
            if (auto bpm = posInfo->getBpm())
            {
                if (*bpm > 0.0)
                {
                    dp.bpm = static_cast<float>(*bpm);
                    bpmValid = true;
                }
            }
        }
    }
    if (dp.syncToBpm && !bpmValid)
        dp.syncToBpm = false;

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = numCh > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    stereoDelay.process(left, right, numSamples, dp);
}

// =============================================================================
// Global Reverb (Dattorro Plate or Synthetic Convolution)
// =============================================================================
void PianoSynthAudioProcessor::processGlobalReverb(juce::AudioBuffer<float>& mainBuffer)
{
    if (!isFxAvailableForCurrentPiano(mps::GlobalFxSlot::Reverb)
        || getParamValue(kReverbEnabled) < 0.5f) return;

    const int numCh      = mainBuffer.getNumChannels();
    const int numSamples = mainBuffer.getNumSamples();
    if (numCh <= 0 || numSamples <= 0) return;

    const int reverbType = static_cast<int>(std::round(getParamValue(kReverbType))); // 0=Plate, 1=Hall, 2=Room, 3=Chamber

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = numCh > 1 ? mainBuffer.getWritePointer(1) : nullptr;

    if (reverbType == 0)
    {
        // Original Dattorro plate reverb
        mps::fx::DattorroPlateReverb::Params rp;
        rp.decay      = clamp01(getParamValue(kReverbSize));
        rp.damping    = clamp01(getParamValue(kReverbDamping));
        rp.width      = clamp01(getParamValue(kReverbWidth));
        rp.mix        = clamp01(getParamValue(kReverbMix));
        rp.preDelayMs = getParamValue(kReverbPreDelay);
        plateReverb.process(left, right, numSamples, rp);
    }
    else
    {
        // Synthetic convolution reverb (Hall / Room / Chamber)
        if (reverbType != lastConvReverbType)
        {
            convReverb.setType(reverbType);
            lastConvReverbType = reverbType;
            convReverbSwitchFadeRemaining = 64;
            reverbPredelayBuffer.clear();
            reverbPredelayWritePos = 0;
        }

        const float mix   = clamp01(getParamValue(kReverbMix));
        const float preDel = getParamValue(kReverbPreDelay); // used as-is for pre-delay

        if (reverbDryBuffer.getNumChannels() < numCh || reverbDryBuffer.getNumSamples() < numSamples)
            return;
        if (reverbConvBuffer.getNumChannels() < numCh || reverbConvBuffer.getNumSamples() < numSamples)
            return;

        // Capture dry before convolution
        reverbDryBuffer.copyFrom(0, 0, left, numSamples);
        if (right != nullptr)
            reverbDryBuffer.copyFrom(1, 0, right, numSamples);

        // Circular pre-delay: bounded, preallocated, and continuous across blocks.
        const int preDelSamples = juce::jlimit(0, juce::jmax(0, reverbPredelayCapacity - 1),
            static_cast<int>(preDel * 0.001f * static_cast<float>(preparedSampleRate)));
        auto* convL = reverbConvBuffer.getWritePointer(0);
        auto* convR = reverbConvBuffer.getWritePointer(1);
        auto* predelayL = reverbPredelayBuffer.getWritePointer(0);
        auto* predelayR = reverbPredelayBuffer.getWritePointer(1);
        for (int n = 0; n < numSamples; ++n)
        {
            const int readPos = (reverbPredelayWritePos - preDelSamples + reverbPredelayCapacity)
                % reverbPredelayCapacity;
            convL[n] = predelayL[readPos];
            predelayL[reverbPredelayWritePos] = left[n];
            if (right != nullptr)
            {
                convR[n] = predelayR[readPos];
                predelayR[reverbPredelayWritePos] = right[n];
            }
            else
            {
                convR[n] = 0.0f;
                predelayR[reverbPredelayWritePos] = 0.0f;
            }
            reverbPredelayWritePos = (reverbPredelayWritePos + 1) % reverbPredelayCapacity;
        }

        convReverb.processBlock(reverbConvBuffer, numSamples);

        // Wet/dry mix
        constexpr int kConvReverbSwitchFadeSamples = 64;
        for (int n = 0; n < numSamples; ++n)
        {
            float effectiveMix = mix;
            if (convReverbSwitchFadeRemaining > 0)
            {
                const int fadePos = kConvReverbSwitchFadeSamples - convReverbSwitchFadeRemaining;
                effectiveMix *= juce::jlimit(0.0f, 1.0f,
                    static_cast<float>(fadePos + 1) / static_cast<float>(kConvReverbSwitchFadeSamples));
                --convReverbSwitchFadeRemaining;
            }
            left[n]  = reverbDryBuffer.getSample(0, n) * (1.0f - effectiveMix)
                     + reverbConvBuffer.getSample(0, n) * effectiveMix;
            if (right != nullptr)
                right[n] = reverbDryBuffer.getSample(1, n) * (1.0f - effectiveMix)
                         + reverbConvBuffer.getSample(1, n) * effectiveMix;
        }
    }
}

// =============================================================================
// Output Limiter
// =============================================================================
void PianoSynthAudioProcessor::processOutputLimiter(juce::AudioBuffer<float>& mainBuffer)
{
    if (!isFxAvailableForCurrentPiano(mps::GlobalFxSlot::Limiter)
        || getParamValue("fx_tab6_en") < 0.5f) return;

    const int numCh      = mainBuffer.getNumChannels();
    const int numSamples = mainBuffer.getNumSamples();
    if (numCh <= 0 || numSamples <= 0) return;

    mps::fx::OutputLimiter::Params lp;
    lp.thresholdDb = getParamValue(kLimiterThreshold);
    lp.releaseMs   = getParamValue(kLimiterRelease);

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = numCh > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    outputLimiter.process(left, right, numSamples, lp);
}

void PianoSynthAudioProcessor::renderPedalNoise(juce::AudioBuffer<float>& mainBuffer)
{
    if (pedalNoiseLevel <= 1.0e-5f)
        return;

    auto* left = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    for (int sample = 0; sample < mainBuffer.getNumSamples(); ++sample)
    {
        pedalNoiseRandomState = splitMix64(pedalNoiseRandomState);
        const float white = (static_cast<float>((pedalNoiseRandomState >> 40) & 0x00ffffffu)
                             * (1.0f / 16777216.0f)) * 2.0f - 1.0f;
        const float shaped = std::tanh(white * 1.75f) * pedalNoiseLevel;
        left[sample] += shaped;
        if (right != nullptr)
            right[sample] += shaped * 0.92f;
        pedalNoiseLevel *= pedalNoiseDecay;
        if (pedalNoiseLevel <= 1.0e-5f)
            break;
    }
}

// =============================================================================
mps::GlobalFxSettings PianoSynthAudioProcessor::snapshotFx(int pianoIndex) const
{
    mps::GlobalFxSettings fx;
    fx.reverbEnabled    = getParamValue(kReverbEnabled) >= 0.5f;
    fx.saturationEnabled = getParamValue(kSaturationEnabled) >= 0.5f;
    fx.transientEnabled = getParamValue(kTransientEnabled) >= 0.5f;
    fx.compressorEnabled = getParamValue(kCompressorEnabled) >= 0.5f;
    fx.eqEnabled = getParamValue(kEqEnabled) >= 0.5f;
    fx.chorusEnabled = getParamValue(kChorusEnabled) >= 0.5f;
    fx.delayEnabled = getParamValue(kDelayEnabled) >= 0.5f;
    fx.limiterEnabled = getParamValue(kLimiterEnabled) >= 0.5f;
    fx.satDrive         = getParamValue(kSatDrive);
    fx.satMix           = getParamValue(kSatMix);
    fx.transientAttack  = getParamValue(kTransientAttack);
    fx.transientSustain = getParamValue(kTransientSustain);
    fx.transientMix     = getParamValue(kTransientMix);
    fx.eqLowFreq        = getParamValue(kEqLowFreq);
    fx.eqLowGain        = getParamValue(kEqLowGain);
    fx.eqMidFreq         = getParamValue(kEqMidFreq);
    fx.eqMidGain         = getParamValue(kEqMidGain);
    fx.eqMidQ            = getParamValue(kEqMidQ);
    fx.eqHighFreq        = getParamValue(kEqHighFreq);
    fx.eqHighGain        = getParamValue(kEqHighGain);
    fx.compThreshold     = getParamValue(kCompThreshold);
    fx.compRatio         = getParamValue(kCompRatio);
    fx.compAttack        = getParamValue(kCompAttack);
    fx.compRelease       = getParamValue(kCompRelease);
    fx.compMakeup        = getParamValue(kCompMakeup);
    fx.compMix           = getParamValue(kCompMix);
    fx.chorusRate        = getParamValue(kChorusRate);
    fx.chorusDepth       = getParamValue(kChorusDepth);
    fx.chorusMix         = getParamValue(kChorusMix);
    fx.delayTime         = getParamValue(kDelayTime);
    fx.delayFeedback     = getParamValue(kDelayFeedback);
    fx.delayMix          = getParamValue(kDelayMix);
    fx.delaySync         = getParamValue(kDelaySync) >= 0.5f;
    fx.delayNoteDivision = static_cast<int>(std::round(getParamValue(kDelayNoteDiv)));
    fx.reverbType        = static_cast<int>(std::round(getParamValue(kReverbType)));
    fx.reverbSize        = getParamValue(kReverbSize);
    fx.reverbDamping     = getParamValue(kReverbDamping);
    fx.reverbWidth       = getParamValue(kReverbWidth);
    fx.reverbMix         = getParamValue(kReverbMix);
    fx.reverbPredelay    = getParamValue(kReverbPreDelay);
    fx.limiterThreshold  = getParamValue(kLimiterThreshold);
    fx.limiterRelease    = getParamValue(kLimiterRelease);
    const int resolvedPianoIndex = (pianoIndex >= 0 && pianoIndex < mps::kNumPianos)
        ? pianoIndex
        : getSelectedPianoIndex();
    return mps::maskUnavailableFx(resolvedPianoIndex, fx);
}

void PianoSynthAudioProcessor::restoreFx(const mps::GlobalFxSettings& fx)
{
    const auto masked = mps::maskUnavailableFx(getSelectedPianoIndex(), fx);
    setParamValue(kReverbEnabled,     masked.reverbEnabled ? 1.0f : 0.0f);
    setParamValue(kSaturationEnabled, masked.saturationEnabled ? 1.0f : 0.0f);
    setParamValue(kTransientEnabled,  masked.transientEnabled ? 1.0f : 0.0f);
    setParamValue(kCompressorEnabled, masked.compressorEnabled ? 1.0f : 0.0f);
    setParamValue(kEqEnabled,         masked.eqEnabled ? 1.0f : 0.0f);
    setParamValue(kChorusEnabled,     masked.chorusEnabled ? 1.0f : 0.0f);
    setParamValue(kDelayEnabled,      masked.delayEnabled ? 1.0f : 0.0f);
    setParamValue(kLimiterEnabled,    masked.limiterEnabled ? 1.0f : 0.0f);
    setParamValue(kSatDrive,          masked.satDrive);
    setParamValue(kSatMix,            masked.satMix);
    setParamValue(kTransientAttack,   masked.transientAttack);
    setParamValue(kTransientSustain,  masked.transientSustain);
    setParamValue(kTransientMix,      masked.transientMix);
    setParamValue(kEqLowFreq,         masked.eqLowFreq);
    setParamValue(kEqLowGain,         masked.eqLowGain);
    setParamValue(kEqMidFreq,         masked.eqMidFreq);
    setParamValue(kEqMidGain,         masked.eqMidGain);
    setParamValue(kEqMidQ,            masked.eqMidQ);
    setParamValue(kEqHighFreq,        masked.eqHighFreq);
    setParamValue(kEqHighGain,        masked.eqHighGain);
    setParamValue(kCompThreshold,     masked.compThreshold);
    setParamValue(kCompRatio,         masked.compRatio);
    setParamValue(kCompAttack,        masked.compAttack);
    setParamValue(kCompRelease,       masked.compRelease);
    setParamValue(kCompMakeup,        masked.compMakeup);
    setParamValue(kCompMix,           masked.compMix);
    setParamValue(kChorusRate,        masked.chorusRate);
    setParamValue(kChorusDepth,       masked.chorusDepth);
    setParamValue(kChorusMix,         masked.chorusMix);
    setParamValue(kDelayTime,         masked.delayTime);
    setParamValue(kDelayFeedback,     masked.delayFeedback);
    setParamValue(kDelayMix,          masked.delayMix);
    setParamValue(kDelaySync,         masked.delaySync ? 1.0f : 0.0f);
    setParamValue(kDelayNoteDiv,      static_cast<float>(masked.delayNoteDivision));
    setParamValue(kReverbType,        static_cast<float>(masked.reverbType));
    setParamValue(kReverbSize,        masked.reverbSize);
    setParamValue(kReverbDamping,     masked.reverbDamping);
    setParamValue(kReverbWidth,       masked.reverbWidth);
    setParamValue(kReverbMix,         masked.reverbMix);
    setParamValue(kReverbPreDelay,    masked.reverbPredelay);
    setParamValue(kLimiterThreshold,  masked.limiterThreshold);
    setParamValue(kLimiterRelease,    masked.limiterRelease);
}

// =============================================================================
// MIDI Learn
// =============================================================================
void PianoSynthAudioProcessor::midiLearnArm(const juce::String& paramId)
{
    auto* param = parameters.getParameter(paramId);
    midiLearnArmedParamId = param != nullptr ? paramId : juce::String{};
    midiLearnArmedParam.store(param, std::memory_order_release);
    midiLearnArmed.store(param != nullptr, std::memory_order_release);
}

void PianoSynthAudioProcessor::midiLearnClear(int ccNumber)
{
    juce::ScopedLock sl(midiLearnLock);
    midiLearnMap.erase(ccNumber);
    rebuildMidiLearnSnapshot();
}

void PianoSynthAudioProcessor::midiLearnClearAll()
{
    juce::ScopedLock sl(midiLearnLock);
    midiLearnMap.clear();
    rebuildMidiLearnSnapshot();
    midiLearnArmed.store(false, std::memory_order_release);
    midiLearnArmedParam.store(nullptr, std::memory_order_release);
    midiLearnArmedParamId = {};
}

std::vector<std::pair<int, juce::String>> PianoSynthAudioProcessor::getMidiLearnMappings() const
{
    juce::ScopedLock sl(midiLearnLock);
    std::vector<std::pair<int, juce::String>> result;
    result.reserve(midiLearnMap.size());
    for (const auto& kv : midiLearnMap)
        result.emplace_back(kv.first, kv.second);
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b){ return a.first < b.first; });
    return result;
}

void PianoSynthAudioProcessor::saveMidiLearnToXml(juce::XmlElement& xml) const
{
    juce::ScopedLock sl(midiLearnLock);
    auto* node = xml.createNewChildElement("MidiLearn");
    for (const auto& kv : midiLearnMap)
    {
        auto* map = node->createNewChildElement("Map");
        map->setAttribute("cc", kv.first);
        map->setAttribute("param", kv.second);
    }
}

void PianoSynthAudioProcessor::loadMidiLearnFromXml(const juce::XmlElement& xml)
{
    const auto* node = xml.getChildByName("MidiLearn");
    if (node == nullptr)
        return;

    juce::ScopedLock sl(midiLearnLock);
    midiLearnMap.clear();
    for (auto* child = node->getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->hasTagName("Map"))
        {
            const int cc = child->getIntAttribute("cc", -1);
            const auto param = child->getStringAttribute("param");
            if (cc >= 0 && cc <= 127 && param.isNotEmpty())
                midiLearnMap[cc] = param;
        }
    }
    rebuildMidiLearnSnapshot();
}

// =============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PianoSynthAudioProcessor();
}
