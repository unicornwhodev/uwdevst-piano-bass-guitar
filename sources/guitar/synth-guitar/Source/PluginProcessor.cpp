#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../Shared/PresetManifest.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>

namespace
{
constexpr const char* kOutputGain          = "output_gain";
constexpr const char* kSelectedInstr       = "selected_instr";
constexpr const char* kQualityMode         = "quality_mode";
constexpr const char* kDelaySync           = "delay_sync";
constexpr const char* kDelayDivision       = "delay_division";
constexpr const char* kLfoRate             = "lfo_rate";
constexpr const char* kLfoDepth            = "lfo_depth";
constexpr const char* kLfoWave             = "lfo_wave";
constexpr const char* kPlayMode            = "play_mode";
constexpr const char* kPalmMute            = "palm_mute";
constexpr const char* kStrumSpread         = "strum_spread";

constexpr const char* kMacroCorps     = "macro_corps";
constexpr const char* kMacroBrillance = "macro_brillance";
constexpr const char* kMacroGain      = "macro_gain";
constexpr const char* kMacroEspace    = "macro_espace";

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

constexpr const char* kChorusRate  = "chorus_rate";
constexpr const char* kChorusDepth = "chorus_depth";
constexpr const char* kChorusDelay = "chorus_delay";
constexpr const char* kChorusMix   = "chorus_mix";

constexpr const char* kReverbSize    = "reverb_size";
constexpr const char* kReverbDamping = "reverb_damping";
constexpr const char* kReverbWidth   = "reverb_width";
constexpr const char* kReverbMix     = "reverb_mix";

constexpr const char* kEqLowFreq   = "eq_low_freq";
constexpr const char* kEqLowGain   = "eq_low_gain";
constexpr const char* kEqMidFreq   = "eq_mid_freq";
constexpr const char* kEqMidGain   = "eq_mid_gain";
constexpr const char* kEqMidQ      = "eq_mid_q";
constexpr const char* kEqHighFreq  = "eq_high_freq";
constexpr const char* kEqHighGain  = "eq_high_gain";

constexpr const char* kDelayTime     = "delay_time";
constexpr const char* kDelayFeedback = "delay_feedback";
constexpr const char* kDelayMix      = "delay_mix";

constexpr const char* kLimiterThreshold = "limiter_threshold";
constexpr const char* kLimiterRelease   = "limiter_release";

constexpr const char* kCabMix = "cab_mix";

constexpr const char* kFxSatEnable      = "fx_tab0_en";
constexpr const char* kFxTransientEnable= "fx_tab1_en";
constexpr const char* kFxCompEnable     = "fx_tab2_en";
constexpr const char* kFxReverbEnable   = "fx_reverb_en";
constexpr const char* kFxEqEnable       = "fx_eq_en";
constexpr const char* kFxChorusEnable   = "fx_chorus_en";
constexpr const char* kFxDelayEnable    = "fx_delay_en";
constexpr const char* kFxLimiterEnable  = "fx_limiter_en";
constexpr const char* kFxCabinetEnable  = "fx_cabinet_en";
constexpr const char* kInstrumentFxState = "inst_fx_state";
constexpr const char* kInstrumentFxEntry = "fx_instrument";

constexpr const char* kInstrStringBrightnessSuffix = "string_brightness";
constexpr const char* kInstrLowPassHzSuffix = "low_pass_hz";
constexpr const char* kInstrBodyAmountSuffix = "body_amount";
constexpr const char* kInstrAttackBrightnessSuffix = "attack_brightness";
constexpr const char* kInstrPickPositionSuffix = "pick_position";
constexpr const char* kInstrOutputSuffix = "output";

constexpr const char* kLegacyInstrStringBrightnessSuffix = "brightness";
constexpr const char* kLegacyInstrLowPassHzSuffix = "cutoff";
constexpr const char* kLegacyInstrBodySuffix = "body";
constexpr const char* kLegacyInstrAttackBrightnessSuffix = "pluck";
constexpr const char* kLegacyInstrPickPositionSuffix = "character";

constexpr const char* kPresetStringBrightnessAttr = "string_brightness";
constexpr const char* kPresetLowPassHzAttr = "low_pass_hz";
constexpr const char* kPresetBodyAmountAttr = "body_amount";
constexpr const char* kPresetAttackBrightnessAttr = "attack_brightness";
constexpr const char* kPresetPickPositionAttr = "pick_position";
constexpr const char* kPresetFormatVersionAttr = "format_version";
constexpr const char* kPresetInstrumentIndexAttr = "instrument_index";
constexpr const char* kPresetSynthIndexAttr = "synth_index";
constexpr const char* kPresetLegacyInstrumentIndexAttr = "instrIndex";
constexpr const char* kPresetLegacyFactoryIndexAttr = "presetIndex";
constexpr const char* kPresetFactoryIndexAttr = "preset_index";
constexpr const char* kPresetMixRoleAttr = "mix_role";
constexpr const char* kPresetFamilyAttr = "family";
constexpr const char* kPresetTagsAttr = "tags";
constexpr const char* kPresetNominalPeakDbAttr = "nominal_peak_db";

constexpr int kCurrentPresetFormatVersion = 3;
constexpr int kSynthIndex = 6;

constexpr const char* kLegacyPresetStringBrightnessAttr = "brightness";
constexpr const char* kLegacyPresetLowPassHzAttr = "cutoff";
constexpr const char* kLegacyPresetBodyAttr = "body";
constexpr const char* kLegacyPresetAttackBrightnessAttr = "pluck";
constexpr const char* kLegacyPresetPickPositionAttr = "character";

juce::StringArray makeOutputChoices()
{
    juce::StringArray outputs;
    outputs.add("Master");
    for (int i = 0; i < GuitarSynthAudioProcessor::kNumAuxOutputs; ++i)
        outputs.add("Out " + juce::String(i + 1));
    return outputs;
}

juce::StringArray makeQualityChoices()
{
    return { "Live", "Studio" };
}

juce::StringArray makeDelaySyncChoices()
{
    return { "Off", "Host" };
}

juce::StringArray makeDelayDivisionChoices()
{
    return { "1/4", "1/8", "1/8D", "1/8T", "1/16", "1/16D" };
}

float clamp01(float v) { return juce::jlimit(0.0f, 1.0f, v); }

juce::ValueTree findStateParameterNode(juce::ValueTree state, const juce::String& paramId)
{
    for (int childIndex = 0; childIndex < state.getNumChildren(); ++childIndex)
    {
        auto child = state.getChild(childIndex);
        if (child.getProperty("id").toString() == paramId)
            return child;
    }

    return {};
}

void setStateParameterValue(juce::ValueTree& state, const juce::String& paramId, const juce::var& value)
{
    if (auto child = findStateParameterNode(state, paramId); child.isValid())
    {
        child.setProperty("value", value, nullptr);
        return;
    }

    const auto childType = state.getNumChildren() > 0
        ? state.getChild(0).getType()
        : juce::Identifier("PARAM");
    juce::ValueTree child(childType);
    child.setProperty("id", paramId, nullptr);
    child.setProperty("value", value, nullptr);
    state.appendChild(child, nullptr);
}

bool readStateParameterValue(const juce::ValueTree& state, const juce::String& paramId, float& valueOut)
{
    if (auto child = findStateParameterNode(state, paramId); child.isValid())
    {
        const auto raw = static_cast<double>(child.getProperty("value", 0.0f));
        if (std::isfinite(raw))
        {
            valueOut = static_cast<float>(raw);
            return true;
        }
    }

    return false;
}

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

float readFiniteXmlFloat(const juce::XmlElement& xml,
                         const char* currentName,
                         const char* legacyName,
                         float fallback)
{
    const auto readAttr = [&xml](const char* name, float defaultValue)
    {
        if (name == nullptr || !xml.hasAttribute(name))
            return defaultValue;

        const auto raw = xml.getDoubleAttribute(name, defaultValue);
        return std::isfinite(raw) ? static_cast<float>(raw) : defaultValue;
    };

    if (currentName != nullptr && xml.hasAttribute(currentName))
        return readAttr(currentName, fallback);
    if (legacyName != nullptr && xml.hasAttribute(legacyName))
        return readAttr(legacyName, fallback);
    return fallback;
}

float readFiniteStateFloat(const juce::ValueTree& state, const juce::Identifier& name, float fallback)
{
    const auto value = state.getProperty(name, fallback);
    const auto raw = static_cast<double>(value);
    return std::isfinite(raw) ? static_cast<float>(raw) : fallback;
}

float getDoubleAttributeWithLegacy(const juce::XmlElement& xml,
                                   const char* currentName,
                                   const char* legacyName,
                                   const float fallback)
{
    return readFiniteXmlFloat(xml, currentName, legacyName, fallback);
}

juce::String migrateLegacyInstrParamId(const juce::String& paramId)
{
    if (!paramId.startsWith("instr_"))
        return paramId;

    if (paramId.endsWith("_" + juce::String(kLegacyInstrStringBrightnessSuffix)))
        return paramId.dropLastCharacters(static_cast<int>(std::strlen(kLegacyInstrStringBrightnessSuffix))) + kInstrStringBrightnessSuffix;
    if (paramId.endsWith("_" + juce::String(kLegacyInstrLowPassHzSuffix)))
        return paramId.dropLastCharacters(static_cast<int>(std::strlen(kLegacyInstrLowPassHzSuffix))) + kInstrLowPassHzSuffix;
    if (paramId.endsWith("_" + juce::String(kLegacyInstrBodySuffix)))
        return paramId.dropLastCharacters(static_cast<int>(std::strlen(kLegacyInstrBodySuffix))) + kInstrBodyAmountSuffix;
    if (paramId.endsWith("_" + juce::String(kLegacyInstrAttackBrightnessSuffix)))
        return paramId.dropLastCharacters(static_cast<int>(std::strlen(kLegacyInstrAttackBrightnessSuffix))) + kInstrAttackBrightnessSuffix;
    if (paramId.endsWith("_" + juce::String(kLegacyInstrPickPositionSuffix)))
        return paramId.dropLastCharacters(static_cast<int>(std::strlen(kLegacyInstrPickPositionSuffix))) + kInstrPickPositionSuffix;
    return paramId;
}

bool isLegacyPresetStorage(const juce::XmlElement& xml)
{
    const auto formatVersion = xml.getIntAttribute(kPresetFormatVersionAttr, 0);
    return formatVersion < 2
        || xml.hasAttribute(kLegacyPresetStringBrightnessAttr)
        || xml.hasAttribute(kLegacyPresetLowPassHzAttr)
        || xml.hasAttribute(kLegacyPresetBodyAttr)
        || xml.hasAttribute(kLegacyPresetAttackBrightnessAttr)
        || xml.hasAttribute(kLegacyPresetPickPositionAttr);
}

struct PresetMetadataSnapshot
{
    juce::String mixRole;
    juce::String family;
    juce::String tags;
    float nominalPeakDb = -12.0f;
};

struct PresetPersistenceState
{
    juce::String name;
    int instrIndex = 0;
    std::optional<int> presetIndex;
    mgs::InstrSettings settings;
    mgs::GlobalFxSettings fxSettings;
    int outputBus = 0;
    modmatrix::MatrixState modMatrixState;
    float qualityMode = 0.0f;
    float delaySync = 0.0f;
    float delayDivision = 1.0f;
    float playMode = 0.0f;
    float palmMute = 0.0f;
    PresetMetadataSnapshot metadata;
};

juce::String slugifyInstrumentName(juce::String value)
{
    value = value.toLowerCase().replaceCharacter(' ', '-');
    value = value.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789-_");
    while (value.contains("--"))
        value = value.replace("--", "-");
    return value.trimCharactersAtStart("-").trimCharactersAtEnd("-");
}

juce::String familyLabelForInstrument(const int instrIndex)
{
    switch (mgs::getFamily(instrIndex))
    {
        case mgs::Family::Acoustique: return "acoustic";
        case mgs::Family::Electrique: return "electric";
        case mgs::Family::Electronique: return "hybrid";
    }

    return "guitar";
}

juce::String joinFactoryTags(const std::vector<std::string>& tags)
{
    juce::StringArray items;
    for (const auto& tag : tags)
        items.add(juce::String(juce::CharPointer_UTF8(tag.c_str())));
    return items.joinIntoString(",");
}

std::vector<std::string> splitTags(const juce::String& tags)
{
    juce::StringArray items;
    items.addTokens(tags, ",", "\"");
    items.trim();
    items.removeEmptyStrings();

    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(items.size()));
    for (const auto& item : items)
        result.emplace_back(item.toStdString());
    return result;
}

PresetMetadataSnapshot makeUserPresetMetadata(const int instrIndex)
{
    PresetMetadataSnapshot metadata;
    metadata.mixRole = "custom";
    metadata.family = familyLabelForInstrument(instrIndex);
    const auto instrumentSlug = slugifyInstrumentName(mgs::getInstrName(instrIndex));
    metadata.tags = juce::StringArray { "guitar", "user", "custom", metadata.family, instrumentSlug }.joinIntoString(",");
    metadata.nominalPeakDb = -12.0f;
    return metadata;
}

PresetMetadataSnapshot makeFactoryPresetMetadata(const mgs::PresetMetadata& source)
{
    PresetMetadataSnapshot metadata;
    metadata.mixRole = juce::String(juce::CharPointer_UTF8(source.mixRole.c_str()));
    metadata.family = juce::String(juce::CharPointer_UTF8(source.familyLabel.c_str()));
    metadata.tags = joinFactoryTags(source.tags);
    metadata.nominalPeakDb = source.nominalPeakDb;
    return metadata;
}

PresetMetadataSnapshot readPresetMetadataFromXml(const juce::XmlElement& xml,
                                                 PresetMetadataSnapshot fallback)
{
    if (xml.hasAttribute(kPresetMixRoleAttr))
        fallback.mixRole = xml.getStringAttribute(kPresetMixRoleAttr);
    if (xml.hasAttribute(kPresetFamilyAttr))
        fallback.family = xml.getStringAttribute(kPresetFamilyAttr);
    if (xml.hasAttribute(kPresetTagsAttr))
        fallback.tags = xml.getStringAttribute(kPresetTagsAttr);
    if (xml.hasAttribute(kPresetNominalPeakDbAttr))
        fallback.nominalPeakDb = readFiniteXmlFloat(xml, kPresetNominalPeakDbAttr, nullptr, fallback.nominalPeakDb);
    return fallback;
}

bool hasCompleteMetadata(const juce::XmlElement& xml)
{
    return xml.hasAttribute(kPresetMixRoleAttr)
        && xml.hasAttribute(kPresetFamilyAttr)
        && xml.hasAttribute(kPresetTagsAttr)
        && xml.hasAttribute(kPresetNominalPeakDbAttr);
}

void writePresetMetadataAttributes(juce::XmlElement& root, const PresetMetadataSnapshot& metadata)
{
    root.setAttribute(kPresetMixRoleAttr, metadata.mixRole);
    root.setAttribute(kPresetFamilyAttr, metadata.family);
    root.setAttribute(kPresetTagsAttr, metadata.tags);
    root.setAttribute(kPresetNominalPeakDbAttr, static_cast<double>(metadata.nominalPeakDb));
}

void writeCanonicalModMatrixXml(juce::XmlElement& parent, const modmatrix::MatrixState& state)
{
    auto* matEl = parent.createNewChildElement("ModMatrix");
    matEl->setAttribute("pbRange", state.pitchBendRange);
    matEl->setAttribute("lfo2Rate", static_cast<double>(state.lfo2Rate));
    matEl->setAttribute("lfo2Wave", state.lfo2Wave);

    for (int slotIndex = 0; slotIndex < modmatrix::ModulationMatrix::getNumSlots(); ++slotIndex)
    {
        const auto& slot = state.slots[static_cast<std::size_t>(slotIndex)];
        auto* slotEl = matEl->createNewChildElement("Slot");
        slotEl->setAttribute("idx", slotIndex);
        slotEl->setAttribute("src", static_cast<int>(slot.source));
        slotEl->setAttribute("dst", static_cast<int>(slot.destination));
        slotEl->setAttribute("amt", static_cast<double>(juce::jlimit(-1.0f, 1.0f, slot.amount)));
    }
}

bool hasCompleteCanonicalModMatrix(const juce::XmlElement& xml)
{
    const auto* matEl = xml.getChildByName("ModMatrix");
    if (matEl == nullptr
        || !matEl->hasAttribute("pbRange")
        || !matEl->hasAttribute("lfo2Rate")
        || !matEl->hasAttribute("lfo2Wave"))
    {
        return false;
    }

    std::array<bool, modmatrix::kMaxSlots> seen {};
    int slotCount = 0;
    for (auto* slotEl : matEl->getChildWithTagNameIterator("Slot"))
    {
        if (!slotEl->hasAttribute("idx")
            || !slotEl->hasAttribute("src")
            || !slotEl->hasAttribute("dst")
            || !slotEl->hasAttribute("amt"))
        {
            return false;
        }

        const int idx = slotEl->getIntAttribute("idx", -1);
        if (idx < 0 || idx >= modmatrix::kMaxSlots || seen[static_cast<std::size_t>(idx)])
            return false;
        seen[static_cast<std::size_t>(idx)] = true;
        ++slotCount;
    }

    return slotCount == modmatrix::kMaxSlots;
}

bool hasCanonicalFxAttributes(const juce::XmlElement& xml)
{
    static constexpr const char* kFxAttrs[] = {
        "fx_sat_drive", "fx_sat_mix",
        "fx_transient_attack", "fx_transient_sustain", "fx_transient_mix",
        "fx_comp_threshold", "fx_comp_ratio", "fx_comp_attack", "fx_comp_release", "fx_comp_makeup", "fx_comp_mix",
        "fx_eq_low_freq", "fx_eq_low_gain", "fx_eq_mid_freq", "fx_eq_mid_gain", "fx_eq_mid_q", "fx_eq_high_freq", "fx_eq_high_gain",
        "fx_chorus_rate", "fx_chorus_depth", "fx_chorus_delay", "fx_chorus_mix",
        "fx_delay_time", "fx_delay_feedback", "fx_delay_mix",
        "fx_reverb_size", "fx_reverb_damping", "fx_reverb_width", "fx_reverb_mix",
        "fx_limiter_threshold", "fx_limiter_release",
        "fx_cab_mix",
        kFxSatEnable, kFxTransientEnable, kFxCompEnable, kFxReverbEnable,
        kFxEqEnable, kFxChorusEnable, kFxDelayEnable, kFxLimiterEnable, kFxCabinetEnable
    };

    return std::all_of(std::begin(kFxAttrs), std::end(kFxAttrs),
                       [&xml](const char* attr) { return xml.hasAttribute(attr); });
}

bool hasCanonicalInstrumentAttributes(const juce::XmlElement& xml)
{
    static constexpr const char* kAttrs[] = {
        "level", "tune", kPresetStringBrightnessAttr, "attack", "decay", "sustain", "release",
        kPresetBodyAmountAttr, "drive", kPresetAttackBrightnessAttr, "stereo_width",
        kPresetPickPositionAttr, kPresetLowPassHzAttr, "pan", "output",
        kQualityMode, kDelaySync, kDelayDivision
    };

    return std::all_of(std::begin(kAttrs), std::end(kAttrs),
                       [&xml](const char* attr) { return xml.hasAttribute(attr); });
}

bool hasCanonicalIdentityAttributes(const juce::XmlElement& xml, const bool requiresFactoryIndex)
{
    if (!xml.hasAttribute("name")
        || !xml.hasAttribute(kPresetLegacyInstrumentIndexAttr)
        || !xml.hasAttribute(kPresetInstrumentIndexAttr)
        || xml.getIntAttribute(kPresetSynthIndexAttr, -1) != kSynthIndex)
    {
        return false;
    }

    if (requiresFactoryIndex
        && (!xml.hasAttribute(kPresetLegacyFactoryIndexAttr) || !xml.hasAttribute(kPresetFactoryIndexAttr)))
    {
        return false;
    }

    return true;
}

bool shouldRewritePresetXml(const juce::XmlElement& xml)
{
    const bool isFactoryPreset = xml.hasTagName("FactoryPreset");
    const auto formatVersion = xml.getIntAttribute(kPresetFormatVersionAttr, 0);
    return isLegacyPresetStorage(xml)
        || formatVersion < kCurrentPresetFormatVersion
        || !hasCanonicalIdentityAttributes(xml, isFactoryPreset)
        || !hasCanonicalInstrumentAttributes(xml)
        || !hasCanonicalFxAttributes(xml)
        || !hasCompleteMetadata(xml)
        || !hasCompleteCanonicalModMatrix(xml);
}

void writeFxSettingsAttributes(juce::XmlElement& root, const mgs::GlobalFxSettings& f)
{
    root.setAttribute("fx_sat_drive", static_cast<double>(f.satDrive));
    root.setAttribute("fx_sat_mix", static_cast<double>(f.satMix));
    root.setAttribute("fx_transient_attack", static_cast<double>(f.transientAttack));
    root.setAttribute("fx_transient_sustain", static_cast<double>(f.transientSustain));
    root.setAttribute("fx_transient_mix", static_cast<double>(f.transientMix));
    root.setAttribute("fx_comp_threshold", static_cast<double>(f.compThreshold));
    root.setAttribute("fx_comp_ratio", static_cast<double>(f.compRatio));
    root.setAttribute("fx_comp_attack", static_cast<double>(f.compAttack));
    root.setAttribute("fx_comp_release", static_cast<double>(f.compRelease));
    root.setAttribute("fx_comp_makeup", static_cast<double>(f.compMakeup));
    root.setAttribute("fx_comp_mix", static_cast<double>(f.compMix));
    root.setAttribute("fx_eq_low_freq", static_cast<double>(f.eqLowFreq));
    root.setAttribute("fx_eq_low_gain", static_cast<double>(f.eqLowGain));
    root.setAttribute("fx_eq_mid_freq", static_cast<double>(f.eqMidFreq));
    root.setAttribute("fx_eq_mid_gain", static_cast<double>(f.eqMidGain));
    root.setAttribute("fx_eq_mid_q", static_cast<double>(f.eqMidQ));
    root.setAttribute("fx_eq_high_freq", static_cast<double>(f.eqHighFreq));
    root.setAttribute("fx_eq_high_gain", static_cast<double>(f.eqHighGain));
    root.setAttribute("fx_chorus_rate", static_cast<double>(f.chorusRate));
    root.setAttribute("fx_chorus_depth", static_cast<double>(f.chorusDepth));
    root.setAttribute("fx_chorus_delay", static_cast<double>(f.chorusDelay));
    root.setAttribute("fx_chorus_mix", static_cast<double>(f.chorusMix));
    root.setAttribute("fx_delay_time", static_cast<double>(f.delayTime));
    root.setAttribute("fx_delay_feedback", static_cast<double>(f.delayFeedback));
    root.setAttribute("fx_delay_mix", static_cast<double>(f.delayMix));
    root.setAttribute("fx_reverb_size", static_cast<double>(f.reverbSize));
    root.setAttribute("fx_reverb_damping", static_cast<double>(f.reverbDamping));
    root.setAttribute("fx_reverb_width", static_cast<double>(f.reverbWidth));
    root.setAttribute("fx_reverb_mix", static_cast<double>(f.reverbMix));
    root.setAttribute("fx_limiter_threshold", static_cast<double>(f.limiterThreshold));
    root.setAttribute("fx_limiter_release", static_cast<double>(f.limiterRelease));
    root.setAttribute("fx_cab_mix", static_cast<double>(f.cabMix));
    root.setAttribute(kFxSatEnable, f.saturatorOn);
    root.setAttribute(kFxTransientEnable, f.transientOn);
    root.setAttribute(kFxCompEnable, f.compressorOn);
    root.setAttribute(kFxReverbEnable, f.reverbOn);
    root.setAttribute(kFxEqEnable, f.eqOn);
    root.setAttribute(kFxChorusEnable, f.chorusOn);
    root.setAttribute(kFxDelayEnable, f.delayOn);
    root.setAttribute(kFxLimiterEnable, f.limiterOn);
    root.setAttribute(kFxCabinetEnable, f.cabinetOn);
}

void readFxSettingsAttributes(const juce::XmlElement& xml, mgs::GlobalFxSettings& f)
{
    auto rd = [&](const char* name, float& value)
    {
        if (!xml.hasAttribute(name))
            return;

        const auto raw = xml.getDoubleAttribute(name);
        if (std::isfinite(raw))
            value = static_cast<float>(raw);
    };

    rd("fx_sat_drive", f.satDrive);
    rd("fx_sat_mix", f.satMix);
    rd("fx_transient_attack", f.transientAttack);
    rd("fx_transient_sustain", f.transientSustain);
    rd("fx_transient_mix", f.transientMix);
    rd("fx_comp_threshold", f.compThreshold);
    rd("fx_comp_ratio", f.compRatio);
    rd("fx_comp_attack", f.compAttack);
    rd("fx_comp_release", f.compRelease);
    rd("fx_comp_makeup", f.compMakeup);
    rd("fx_comp_mix", f.compMix);
    rd("fx_eq_low_freq", f.eqLowFreq);
    rd("fx_eq_low_gain", f.eqLowGain);
    rd("fx_eq_mid_freq", f.eqMidFreq);
    rd("fx_eq_mid_gain", f.eqMidGain);
    rd("fx_eq_mid_q", f.eqMidQ);
    rd("fx_eq_high_freq", f.eqHighFreq);
    rd("fx_eq_high_gain", f.eqHighGain);
    rd("fx_chorus_rate", f.chorusRate);
    rd("fx_chorus_depth", f.chorusDepth);
    rd("fx_chorus_delay", f.chorusDelay);
    rd("fx_chorus_mix", f.chorusMix);
    rd("fx_delay_time", f.delayTime);
    rd("fx_delay_feedback", f.delayFeedback);
    rd("fx_delay_mix", f.delayMix);
    rd("fx_reverb_size", f.reverbSize);
    rd("fx_reverb_damping", f.reverbDamping);
    rd("fx_reverb_width", f.reverbWidth);
    rd("fx_reverb_mix", f.reverbMix);
    rd("fx_limiter_threshold", f.limiterThreshold);
    rd("fx_limiter_release", f.limiterRelease);
    rd("fx_cab_mix", f.cabMix);

    f.saturatorOn  = static_cast<bool>(xml.getBoolAttribute(kFxSatEnable, f.saturatorOn));
    f.transientOn  = static_cast<bool>(xml.getBoolAttribute(kFxTransientEnable, f.transientOn));
    f.compressorOn = static_cast<bool>(xml.getBoolAttribute(kFxCompEnable, f.compressorOn));
    f.reverbOn     = static_cast<bool>(xml.getBoolAttribute(kFxReverbEnable, f.reverbOn));
    f.eqOn         = static_cast<bool>(xml.getBoolAttribute(kFxEqEnable, f.eqOn));
    f.chorusOn     = static_cast<bool>(xml.getBoolAttribute(kFxChorusEnable, f.chorusOn));
    f.delayOn      = static_cast<bool>(xml.getBoolAttribute(kFxDelayEnable, f.delayOn));
    f.limiterOn    = static_cast<bool>(xml.getBoolAttribute(kFxLimiterEnable, f.limiterOn));
    f.cabinetOn    = static_cast<bool>(xml.getBoolAttribute(kFxCabinetEnable, f.cabinetOn));
}

void writeFxStateProperties(juce::ValueTree& state, const mgs::GlobalFxSettings& fx)
{
    state.setProperty("sat_drive", fx.satDrive, nullptr);
    state.setProperty("sat_mix", fx.satMix, nullptr);
    state.setProperty("transient_attack", fx.transientAttack, nullptr);
    state.setProperty("transient_sustain", fx.transientSustain, nullptr);
    state.setProperty("transient_mix", fx.transientMix, nullptr);
    state.setProperty("comp_threshold", fx.compThreshold, nullptr);
    state.setProperty("comp_ratio", fx.compRatio, nullptr);
    state.setProperty("comp_attack", fx.compAttack, nullptr);
    state.setProperty("comp_release", fx.compRelease, nullptr);
    state.setProperty("comp_makeup", fx.compMakeup, nullptr);
    state.setProperty("comp_mix", fx.compMix, nullptr);
    state.setProperty("eq_low_freq", fx.eqLowFreq, nullptr);
    state.setProperty("eq_low_gain", fx.eqLowGain, nullptr);
    state.setProperty("eq_mid_freq", fx.eqMidFreq, nullptr);
    state.setProperty("eq_mid_gain", fx.eqMidGain, nullptr);
    state.setProperty("eq_mid_q", fx.eqMidQ, nullptr);
    state.setProperty("eq_high_freq", fx.eqHighFreq, nullptr);
    state.setProperty("eq_high_gain", fx.eqHighGain, nullptr);
    state.setProperty("chorus_rate", fx.chorusRate, nullptr);
    state.setProperty("chorus_depth", fx.chorusDepth, nullptr);
    state.setProperty("chorus_delay", fx.chorusDelay, nullptr);
    state.setProperty("chorus_mix", fx.chorusMix, nullptr);
    state.setProperty("delay_time", fx.delayTime, nullptr);
    state.setProperty("delay_feedback", fx.delayFeedback, nullptr);
    state.setProperty("delay_mix", fx.delayMix, nullptr);
    state.setProperty("reverb_size", fx.reverbSize, nullptr);
    state.setProperty("reverb_damping", fx.reverbDamping, nullptr);
    state.setProperty("reverb_width", fx.reverbWidth, nullptr);
    state.setProperty("reverb_mix", fx.reverbMix, nullptr);
    state.setProperty("limiter_threshold", fx.limiterThreshold, nullptr);
    state.setProperty("limiter_release", fx.limiterRelease, nullptr);
    state.setProperty("cab_mix", fx.cabMix, nullptr);
    state.setProperty(kFxSatEnable, fx.saturatorOn, nullptr);
    state.setProperty(kFxTransientEnable, fx.transientOn, nullptr);
    state.setProperty(kFxCompEnable, fx.compressorOn, nullptr);
    state.setProperty(kFxReverbEnable, fx.reverbOn, nullptr);
    state.setProperty(kFxEqEnable, fx.eqOn, nullptr);
    state.setProperty(kFxChorusEnable, fx.chorusOn, nullptr);
    state.setProperty(kFxDelayEnable, fx.delayOn, nullptr);
    state.setProperty(kFxLimiterEnable, fx.limiterOn, nullptr);
    state.setProperty(kFxCabinetEnable, fx.cabinetOn, nullptr);
}

mgs::GlobalFxSettings readFxStateProperties(const juce::ValueTree& state,
                                            mgs::GlobalFxSettings fallback)
{
    auto fx = fallback;
    fx.satDrive         = readFiniteStateFloat(state, "sat_drive", fx.satDrive);
    fx.satMix           = readFiniteStateFloat(state, "sat_mix", fx.satMix);
    fx.transientAttack  = readFiniteStateFloat(state, "transient_attack", fx.transientAttack);
    fx.transientSustain = readFiniteStateFloat(state, "transient_sustain", fx.transientSustain);
    fx.transientMix     = readFiniteStateFloat(state, "transient_mix", fx.transientMix);
    fx.compThreshold    = readFiniteStateFloat(state, "comp_threshold", fx.compThreshold);
    fx.compRatio        = readFiniteStateFloat(state, "comp_ratio", fx.compRatio);
    fx.compAttack       = readFiniteStateFloat(state, "comp_attack", fx.compAttack);
    fx.compRelease      = readFiniteStateFloat(state, "comp_release", fx.compRelease);
    fx.compMakeup       = readFiniteStateFloat(state, "comp_makeup", fx.compMakeup);
    fx.compMix          = readFiniteStateFloat(state, "comp_mix", fx.compMix);
    fx.eqLowFreq        = readFiniteStateFloat(state, "eq_low_freq", fx.eqLowFreq);
    fx.eqLowGain        = readFiniteStateFloat(state, "eq_low_gain", fx.eqLowGain);
    fx.eqMidFreq        = readFiniteStateFloat(state, "eq_mid_freq", fx.eqMidFreq);
    fx.eqMidGain        = readFiniteStateFloat(state, "eq_mid_gain", fx.eqMidGain);
    fx.eqMidQ           = readFiniteStateFloat(state, "eq_mid_q", fx.eqMidQ);
    fx.eqHighFreq       = readFiniteStateFloat(state, "eq_high_freq", fx.eqHighFreq);
    fx.eqHighGain       = readFiniteStateFloat(state, "eq_high_gain", fx.eqHighGain);
    fx.chorusRate       = readFiniteStateFloat(state, "chorus_rate", fx.chorusRate);
    fx.chorusDepth      = readFiniteStateFloat(state, "chorus_depth", fx.chorusDepth);
    fx.chorusDelay      = readFiniteStateFloat(state, "chorus_delay", fx.chorusDelay);
    fx.chorusMix        = readFiniteStateFloat(state, "chorus_mix", fx.chorusMix);
    fx.delayTime        = readFiniteStateFloat(state, "delay_time", fx.delayTime);
    fx.delayFeedback    = readFiniteStateFloat(state, "delay_feedback", fx.delayFeedback);
    fx.delayMix         = readFiniteStateFloat(state, "delay_mix", fx.delayMix);
    fx.reverbSize       = readFiniteStateFloat(state, "reverb_size", fx.reverbSize);
    fx.reverbDamping    = readFiniteStateFloat(state, "reverb_damping", fx.reverbDamping);
    fx.reverbWidth      = readFiniteStateFloat(state, "reverb_width", fx.reverbWidth);
    fx.reverbMix        = readFiniteStateFloat(state, "reverb_mix", fx.reverbMix);
    fx.limiterThreshold = readFiniteStateFloat(state, "limiter_threshold", fx.limiterThreshold);
    fx.limiterRelease   = readFiniteStateFloat(state, "limiter_release", fx.limiterRelease);
    fx.cabMix           = readFiniteStateFloat(state, "cab_mix", fx.cabMix);
    fx.saturatorOn      = static_cast<bool>(state.getProperty(kFxSatEnable, fx.saturatorOn));
    fx.transientOn      = static_cast<bool>(state.getProperty(kFxTransientEnable, fx.transientOn));
    fx.compressorOn     = static_cast<bool>(state.getProperty(kFxCompEnable, fx.compressorOn));
    fx.reverbOn         = static_cast<bool>(state.getProperty(kFxReverbEnable, fx.reverbOn));
    fx.eqOn             = static_cast<bool>(state.getProperty(kFxEqEnable, fx.eqOn));
    fx.chorusOn         = static_cast<bool>(state.getProperty(kFxChorusEnable, fx.chorusOn));
    fx.delayOn          = static_cast<bool>(state.getProperty(kFxDelayEnable, fx.delayOn));
    fx.limiterOn        = static_cast<bool>(state.getProperty(kFxLimiterEnable, fx.limiterOn));
    fx.cabinetOn        = static_cast<bool>(state.getProperty(kFxCabinetEnable, fx.cabinetOn));
    return fx;
}

std::unique_ptr<juce::XmlElement> createPresetXml(const juce::String& tagName,
                                                  const PresetPersistenceState& state)
{
    auto root = std::make_unique<juce::XmlElement>(tagName);
    root->setAttribute("name", state.name);
    root->setAttribute(kPresetFormatVersionAttr, kCurrentPresetFormatVersion);
    root->setAttribute(kPresetSynthIndexAttr, kSynthIndex);
    root->setAttribute(kPresetLegacyInstrumentIndexAttr, state.instrIndex);
    root->setAttribute(kPresetInstrumentIndexAttr, state.instrIndex);
    if (state.presetIndex.has_value())
    {
        root->setAttribute(kPresetLegacyFactoryIndexAttr, *state.presetIndex);
        root->setAttribute(kPresetFactoryIndexAttr, *state.presetIndex);
    }

    root->setAttribute("level", static_cast<double>(state.settings.level));
    root->setAttribute("tune", static_cast<double>(state.settings.tuneSemitones));
    root->setAttribute(kPresetStringBrightnessAttr, static_cast<double>(state.settings.tone.stringBrightness));
    root->setAttribute("attack", static_cast<double>(state.settings.envelope.attackSeconds));
    root->setAttribute("decay", static_cast<double>(state.settings.envelope.decaySeconds));
    root->setAttribute("sustain", static_cast<double>(state.settings.envelope.sustainLevel));
    root->setAttribute("release", static_cast<double>(state.settings.envelope.releaseSeconds));
    root->setAttribute(kPresetBodyAmountAttr, static_cast<double>(state.settings.resonance.bodyAmount));
    root->setAttribute("drive", static_cast<double>(state.settings.resonance.driveAmount));
    root->setAttribute(kPresetAttackBrightnessAttr, static_cast<double>(state.settings.performance.attackBrightness));
    root->setAttribute("stereo_width", static_cast<double>(state.settings.spatial.stereoWidth));
    root->setAttribute(kPresetPickPositionAttr, static_cast<double>(state.settings.performance.pickPosition));
    root->setAttribute(kPresetLowPassHzAttr, static_cast<double>(state.settings.tone.lowPassHz));
    root->setAttribute("pan", static_cast<double>(state.settings.spatial.pan));
    root->setAttribute("output", state.outputBus);
    root->setAttribute(kQualityMode, static_cast<double>(state.qualityMode));
    root->setAttribute(kDelaySync, static_cast<double>(state.delaySync));
    root->setAttribute(kDelayDivision, static_cast<double>(state.delayDivision));
    root->setAttribute(kPlayMode, static_cast<double>(state.playMode));
    root->setAttribute(kPalmMute, static_cast<double>(state.palmMute));
    writeFxSettingsAttributes(*root, state.fxSettings);
    writePresetMetadataAttributes(*root, state.metadata);
    writeCanonicalModMatrixXml(*root, state.modMatrixState);
    return root;
}

void migrateLegacyStateXml(juce::XmlElement& element)
{
    if (element.hasAttribute("id"))
        element.setAttribute("id", migrateLegacyInstrParamId(element.getStringAttribute("id")));

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
        migrateLegacyStateXml(*child);
}
} // namespace

// =============================================================================
auto GuitarSynthAudioProcessor::createBusLayout() -> BusesProperties
{
    BusesProperties buses;
    buses = buses.withOutput("Master", juce::AudioChannelSet::stereo(), true);
    for (int i = 0; i < kNumAuxOutputs; ++i)
        buses = buses.withOutput("Instr " + juce::String(i + 1) + " Out",
                                 juce::AudioChannelSet::stereo(), false);
    return buses;
}

// =============================================================================
GuitarSynthAudioProcessor::GuitarSynthAudioProcessor()
    : AudioProcessor(createBusLayout()),
      parameters(*this, &undoManager, juce::Identifier("MGS_PARAMS"), createParameterLayout()),
      factoryPresetBanks(mgs::getFactoryPresetBanks())
{
    undoManager.setMaxNumberOfStoredUnits(100, 30);
    resolveParameterPointers();
    currentPresetIndices.fill(0);
    currentUserPresetFiles.fill(juce::File{});

    loadFactoryOverrides();
    for (int i = 0; i < mgs::kNumInstruments; ++i)
    {
        const auto& bank = factoryPresetBanks[static_cast<std::size_t>(i)];
        instrumentFxStates[static_cast<std::size_t>(i)] = bank.empty()
            ? mgs::GlobalFxSettings{}
            : bank[0].fx;

        if (!bank.empty())
            applyInstrPresetSettings(i, bank[0].settings, false);
    }

    parameters.addParameterListener(kSelectedInstr,      this);
    parameters.addParameterListener("velocity_curve",    this);
    parameters.addParameterListener("pitch_bend_range",  this);
    cachedSelectedInstrumentIndex = getSelectedInstrIndex();
    pendingSelectedInstrumentIndex.store(cachedSelectedInstrumentIndex);
    applyFxToParams(cachedSelectedInstrumentIndex,
                    instrumentFxStates[static_cast<std::size_t>(cachedSelectedInstrumentIndex)],
                    false);
}

GuitarSynthAudioProcessor::~GuitarSynthAudioProcessor()
{
    cancelPendingUpdate();
    parameters.removeParameterListener(kSelectedInstr,     this);
    parameters.removeParameterListener("velocity_curve",   this);
    parameters.removeParameterListener("pitch_bend_range", this);
}

// =============================================================================
void GuitarSynthAudioProcessor::applyInstrPresetSettings(int instrIndex, const mgs::InstrSettings& s, bool notifyHost)
{
    setParamValueInternal(makeInstrParamId(instrIndex, "level"), s.level, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, "tune"), s.tuneSemitones, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, kInstrStringBrightnessSuffix), s.tone.stringBrightness, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, "attack"), s.envelope.attackSeconds, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, "decay"), s.envelope.decaySeconds, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, "sustain"), s.envelope.sustainLevel, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, "release"), s.envelope.releaseSeconds, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, kInstrBodyAmountSuffix), s.resonance.bodyAmount, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, "drive"), s.resonance.driveAmount, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, kInstrAttackBrightnessSuffix), s.performance.attackBrightness, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, "stereo_width"), s.spatial.stereoWidth, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, kInstrPickPositionSuffix), s.performance.pickPosition, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, kInstrLowPassHzSuffix), s.tone.lowPassHz, notifyHost);
    setParamValueInternal(makeInstrParamId(instrIndex, "pan"), s.spatial.pan, notifyHost);
}

// =============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
GuitarSynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto& banks = mgs::getFactoryPresetBanks();
    const auto outputChoices = makeOutputChoices();
    const auto qualityChoices = makeQualityChoices();
    const auto delaySyncChoices = makeDelaySyncChoices();
    const auto delayDivisionChoices = makeDelayDivisionChoices();

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kOutputGain, "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f), -3.0f));

    juce::StringArray instrChoices;
    for (int i = 0; i < mgs::kNumInstruments; ++i)
        instrChoices.add(mgs::getInstrName(i));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kSelectedInstr, "Selected Instrument", instrChoices, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kQualityMode, "Quality Mode", qualityChoices,
        static_cast<int>(QualityMode::Live)));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLfoRate, "LFO Rate",
        juce::NormalisableRange<float>(0.05f, 12.0f, 0.0001f), 2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLfoDepth, "LFO Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kLfoWave, "LFO Wave",
        juce::StringArray{ "Sine", "Triangle", "Saw", "Square" }, 0));

    // Macros
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroCorps, "Macro Corps",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroBrillance, "Macro Brillance",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroGain, "Macro Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroEspace, "Macro Espace",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));

    // FX: Compressor
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

    // FX: Saturator
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kSatDrive, "Sat Drive",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.01f), 1.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kSatMix, "Sat Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.10f));

    // FX: Transient
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kTransientAttack, "Transient Attack",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0001f), 0.05f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kTransientSustain, "Transient Sustain",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kTransientMix, "Transient Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.3f));

    // FX: Chorus
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusRate,  "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 8.0f, 0.001f), 1.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusDepth, "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusDelay, "Chorus Delay",
        juce::NormalisableRange<float>(1.0f, 30.0f, 0.01f), 8.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusMix,   "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));

    // FX: Reverb
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbSize,    "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.45f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbDamping, "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.55f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbWidth,   "Reverb Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.85f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbMix,     "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.20f));

    // FX: Parametric EQ
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqLowFreq, "EQ Low Freq",
        juce::NormalisableRange<float>(40.0f, 800.0f, 0.1f, 0.4f), 200.0f));
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
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqHighFreq, "EQ High Freq",
        juce::NormalisableRange<float>(1000.0f, 16000.0f, 0.1f, 0.4f), 5000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqHighGain, "EQ High Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));

    // FX: Stereo Delay
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kDelayTime, "Delay Time",
        juce::NormalisableRange<float>(10.0f, 1500.0f, 0.1f, 0.4f), 300.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kDelayFeedback, "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.30f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kDelayMix, "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kDelaySync, "Delay Sync", delaySyncChoices, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kDelayDivision, "Delay Division", delayDivisionChoices, 1));

    // FX: Output Limiter
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLimiterThreshold, "Limiter Threshold",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.01f), -0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLimiterRelease, "Limiter Release",
        juce::NormalisableRange<float>(5.0f, 200.0f, 0.1f), 50.0f));

    // FX: Cabinet Simulation (electric family only)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCabMix, "Cabinet Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));

    // Performance: Strum timing
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kStrumSpread, "Strum Spread",
        juce::NormalisableRange<float>(1.0f, 100.0f, 0.1f), 15.0f));

    // FX enable toggles (true = active)
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxSatEnable, "FX Saturator Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxTransientEnable, "FX Transient Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxCompEnable, "FX Compressor Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxReverbEnable, "FX Reverb Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxEqEnable, "FX EQ Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxChorusEnable, "FX Chorus Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxDelayEnable, "FX Delay Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxLimiterEnable, "FX Limiter Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(kFxCabinetEnable, "FX Cabinet Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>("fx_lock", "FX Lock", false));

    // Per-instrument parameters (9 instruments x 14 + output)
    for (int b = 0; b < mgs::kNumInstruments; ++b)
    {
        const auto& def = banks[static_cast<std::size_t>(b)][0].settings;
        const auto prefix = juce::String(mgs::getInstrName(b)) + " ";

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, "level"), prefix + "Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.level));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, "tune"), prefix + "Tune",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), def.tuneSemitones));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, kInstrStringBrightnessSuffix), prefix + "String Brightness",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.tone.stringBrightness));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, "attack"), prefix + "Attack",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.0001f), def.envelope.attackSeconds));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, "decay"), prefix + "Decay",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.001f), def.envelope.decaySeconds));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, "sustain"), prefix + "Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.envelope.sustainLevel));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, "release"), prefix + "Release",
            juce::NormalisableRange<float>(0.01f, 5.0f, 0.0001f), def.envelope.releaseSeconds));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, kInstrBodyAmountSuffix), prefix + "Body Amount",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.resonance.bodyAmount));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, "drive"), prefix + "Drive Amount",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.resonance.driveAmount));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, kInstrAttackBrightnessSuffix), prefix + "Attack Brightness",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.performance.attackBrightness));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, "stereo_width"), prefix + "Stereo Width",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.spatial.stereoWidth));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, kInstrPickPositionSuffix), prefix + "Pick Position",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), def.performance.pickPosition));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, kInstrLowPassHzSuffix), prefix + "Low Pass Hz",
            juce::NormalisableRange<float>(120.0f, 16000.0f, 0.0f, 0.28f), def.tone.lowPassHz));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makeInstrParamId(b, "pan"), prefix + "Pan",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), def.spatial.pan));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            makeInstrParamId(b, kInstrOutputSuffix), prefix + "Output",
            outputChoices, 0));
    }

    // Arpeggiator
    layout.add(std::make_unique<juce::AudioParameterBool>("arp_enable", "Arp Enable", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "arp_mode", "Arp Mode",
        juce::StringArray{ "Up", "Down", "UpDown", "Random" }, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "arp_rate", "Arp Rate",
        juce::StringArray{ "1/4", "1/8", "1/8T", "1/16", "1/16T" }, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "arp_octaves", "Arp Octaves", 1, 4, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "arp_gate", "Arp Gate",
        juce::NormalisableRange<float>(0.1f, 1.0f, 0.01f), 0.75f));
    layout.add(std::make_unique<juce::AudioParameterBool>("arp_hold", "Arp Hold", false));

    // Riff Generator (Phase 4 — replaces/extends arpeggiator)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "riff_style", "Riff Style",
        juce::StringArray{ "Off", "Rock", "Blues", "Metal", "Folk", "Jazz", "Funk", "Ambient" }, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        "riff_intensity", "Riff Intensity", 0, 7, 3));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "riff_gate", "Riff Gate",
        juce::NormalisableRange<float>(0.3f, 1.0f, 0.01f), 0.75f));
    layout.add(std::make_unique<juce::AudioParameterBool>("riff_hold", "Riff Hold", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "reverb_type", "Reverb Type",
        juce::StringArray{ "Plate", "Hall", "Room", "Chamber" }, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "velocity_curve", "Velocity Curve",
        juce::StringArray{ "Linear", "Soft", "Softer", "Hard", "Harder", "Fixed", "Touch" }, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kPlayMode, "Play Mode",
        juce::StringArray{ "Poly", "Mono Retrig", "Mono Legato" }, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>("mono_mode", "Mono Mode", false));

    layout.add(std::make_unique<juce::AudioParameterInt>("pitch_bend_range", "Pitch Bend Range", 1, 24, 2));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kPalmMute, "Palm Mute",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));

    return layout;
}

juce::String GuitarSynthAudioProcessor::makeInstrParamId(int instrIndex, const juce::String& suffix)
{
    return "instr_" + juce::String(instrIndex) + "_" + suffix;
}

// =============================================================================
void GuitarSynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    preparedSampleRate = std::max(1.0, sampleRate);
    const int scratchSamples = juce::jmax(32768, samplesPerBlock);
    resetRealtimeModulationState();
    modulationMatrix.lfo2.reset();
    voiceAgeCounter = 0;

    for (int instrIndex = 0; instrIndex < mgs::kNumInstruments; ++instrIndex)
    {
        for (int poolSlot = 0; poolSlot < kVoicePoolSize; ++poolSlot)
        {
            if (!voicePool[static_cast<std::size_t>(instrIndex)][static_cast<std::size_t>(poolSlot)])
                voicePool[static_cast<std::size_t>(instrIndex)][static_cast<std::size_t>(poolSlot)] =
                    mgs::createVoiceForInstrument(instrIndex);
            voicePoolInUse[static_cast<std::size_t>(instrIndex)][static_cast<std::size_t>(poolSlot)]
                .store(false, std::memory_order_release);
        }
    }

    for (auto& slot : voices)
        clearVoice(slot);
    for (auto& slot : dyingVoices)
        clearDyingVoice(slot);
    for (auto& stack : legatoStacks)
        stack.clear();
    arpRandomState = static_cast<std::uint32_t>(
        0x9e3779b9u ^ static_cast<std::uint32_t>(std::round(preparedSampleRate))
        ^ static_cast<std::uint32_t>(juce::Time::getMillisecondCounter()));

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

    compCache = CompressorCache{};
    satDryBuffer.setSize(static_cast<int>(spec.numChannels),
                         static_cast<int>(spec.maximumBlockSize), false, true, true);
    compDryBuffer.setSize(static_cast<int>(spec.numChannels),
                          static_cast<int>(spec.maximumBlockSize), false, true, true);
    mainDryBuffer.setSize(static_cast<int>(spec.numChannels),
                          static_cast<int>(spec.maximumBlockSize), false, true, true);
    reverbWetBuffer.setSize(static_cast<int>(spec.numChannels),
                            static_cast<int>(spec.maximumBlockSize), false, true, true);
    satOversamplingMono.initProcessing(static_cast<size_t>(scratchSamples));
    satOversamplingStereo.initProcessing(static_cast<size_t>(scratchSamples));
    satOversamplingMono.reset();
    satOversamplingStereo.reset();
    transientFastEnv = { 0.0f, 0.0f };
    transientSlowEnv = { 0.0f, 0.0f };
    saturatorPrevInput = { 0.0f, 0.0f };
    lfoPhase = 0.0f;
    outputGainCurrent = juce::Decibels::decibelsToGain(readCachedParamValue(globalParamRefs.outputGain, -3.0f));
    lfoRateCurrent = readCachedParamValue(globalParamRefs.lfoRate, 2.0f);
    lfoDepthCurrent = clamp01(readCachedParamValue(globalParamRefs.lfoDepth, 0.0f));
    satDriveCurrent = readCachedParamValue(globalParamRefs.satDrive, 1.5f);
    satMixCurrent = clamp01(readCachedParamValue(globalParamRefs.satMix, 0.1f));
    clipLatched.store(false, std::memory_order_relaxed);
    lastKnownHostTempoBpm.store(120.0f, std::memory_order_relaxed);
    for (auto& level : mainMeterLevels)
        level.store(0.0f, std::memory_order_relaxed);
    for (auto& level : auxMeterLevels)
        level.store(0.0f, std::memory_order_relaxed);

    // Phase 3 — Initialize strum state
    strumState.reset();
    strumState.sampleRate = preparedSampleRate;
    strumState.strumSpreadMs = readCachedParamValue(globalParamRefs.strumSpread, 15.0f);
    strumState.strumDirection = 1;
    strumState.strumEnabled = true;
    strumState.strumSampleDelay = static_cast<int>(preparedSampleRate * strumState.strumSpreadMs / 1000.0);

    chorus.prepare(preparedSampleRate, scratchSamples);
    chorus.reset();

    reverb.prepare(preparedSampleRate, scratchSamples);
    reverb.reset();

    convReverb.prepare(preparedSampleRate, scratchSamples);
    convReverb.reset();

    eq.prepare(preparedSampleRate);
    eq.reset();

    stereoDelay.prepare(preparedSampleRate, scratchSamples);
    stereoDelay.reset();

    limiter.prepare(preparedSampleRate);
    limiter.reset();

    cabSim.prepare(preparedSampleRate);
    cabSim.reset();
}

void GuitarSynthAudioProcessor::releaseResources()
{
    for (auto& slot : voices)
        clearVoice(slot);
    for (auto& slot : dyingVoices)
        clearDyingVoice(slot);
    resetRealtimeModulationState();
    modulationMatrix.lfo2.reset();
    voiceAgeCounter = 0;
    activeVoiceCountAtomic.store(0, std::memory_order_relaxed);
    satDryBuffer.setSize(0, 0);
    compDryBuffer.setSize(0, 0);
    mainDryBuffer.setSize(0, 0);
    reverbWetBuffer.setSize(0, 0);
    clipLatched.store(false, std::memory_order_relaxed);
}

bool GuitarSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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
void GuitarSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto outputBusCount = getBusCount(false);
    for (int busIndex = 0; busIndex < outputBusCount; ++busIndex)
        getBusBuffer(buffer, false, busIndex).clear();

    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    auto mainBuffer = getBusBuffer(buffer, false, 0);
    const int numSamples = mainBuffer.getNumSamples();
    const double sr = preparedSampleRate;

    if (mainDryBuffer.getNumChannels() < mainBuffer.getNumChannels()
        || mainDryBuffer.getNumSamples() < numSamples)
    {
        mainBuffer.clear();
        return;
    }
    mainDryBuffer.clear();

    const auto blockState = buildGlobalBlockState();
    lastKnownHostTempoBpm.store(blockState.hostBpm, std::memory_order_relaxed);
    std::array<InstrSnapshot, mgs::kNumInstruments> instrSnapshots {};
    for (int instrIndex = 0; instrIndex < mgs::kNumInstruments; ++instrIndex)
        instrSnapshots[static_cast<std::size_t>(instrIndex)] = buildInstrSnapshot(instrIndex, blockState);

    float lfo1Val = 0.0f;
    switch (blockState.lfoWave)
    {
        case 1: lfo1Val = 1.0f - 4.0f * std::abs(lfoPhase - 0.5f); break;
        case 2: lfo1Val = lfoPhase * 2.0f - 1.0f; break;
        case 3: lfo1Val = lfoPhase < 0.5f ? 1.0f : -1.0f; break;
        default: lfo1Val = mgs::fastSin(lfoPhase); break;
    }

    modmatrix::ModContext baseModContext;
    baseModContext.lfo1 = lfo1Val;
    baseModContext.lfo2 = modulationMatrix.lfo2.tickBlock(static_cast<float>(preparedSampleRate), numSamples);
    baseModContext.modWheel = modulationMatrix.modWheelValue;
    baseModContext.aftertouch = modulationMatrix.aftertouchValue;
    baseModContext.pitchBend = modulationMatrix.pitchBendValue;
    cachedModResult = modulationMatrix.process(baseModContext);

    // A1 – sample-accurate rendering helper
    std::array<float, 2048> voiceTmpL{};
    std::array<float, 2048> voiceTmpR{};

    auto renderRange = [&](int startSample, int blockLen)
    {
        if (blockLen <= 0) return;

        for (auto& slot : voices)
        {
            if (!slot.voice || !slot.voice->isActive())
                continue;

            {
                const auto& snapshot = instrSnapshots[static_cast<std::size_t>(slot.instrIndex)];
                auto voiceContext = baseModContext;
                voiceContext.envelope = slot.voice->getEnvelopeLevel();
                voiceContext.velocity = slot.noteVelocity;
                const auto voiceModResult = modulationMatrix.process(voiceContext);

                mgs::VoiceModulation voiceMod;
                voiceMod.cutoffMul = voiceModResult.cutoffMul;
                voiceMod.resonanceAdd = voiceModResult.resonance;
                voiceMod.panAdd = voiceModResult.pan;
                voiceMod.levelMul = voiceModResult.levelMul;
                voiceMod.pitchSemi = voiceModResult.pitchSemi;
                voiceMod.attackScale = voiceModResult.attackScale;
                voiceMod.decayScale = voiceModResult.decayScale;

                slot.voice->setPitchBendFactor(pitchBend.pitchBendFactor);
                slot.voice->setVoiceModulation(voiceMod, sr);
                slot.voice->setPalmMute(getPalmMuteAmountForInstrument(slot.instrIndex));
                slot.voice->updateFilter(snapshot.settings.tone.lowPassHz * voiceModResult.cutoffMul,
                                         snapshot.settings.tone.stringBrightness,
                                         sr);
            }
            int targetBus = juce::jlimit(0, outputBusCount - 1,
                instrSnapshots[static_cast<std::size_t>(slot.instrIndex)].outputBus);
            if (targetBus > 0 && getChannelCountOfBus(false, targetBus) <= 0)
                targetBus = 0;

            juce::AudioBuffer<float> auxBuffer;
            const bool hasAuxStem = targetBus > 0;
            if (hasAuxStem)
                auxBuffer = getBusBuffer(buffer, false, targetBus);
            const auto auxNumCh = auxBuffer.getNumChannels();
            const auto mainNumCh = mainDryBuffer.getNumChannels();

            for (int s = 0; s < blockLen; s += static_cast<int>(voiceTmpL.size()))
            {
                const int chunkLen = juce::jmin(static_cast<int>(voiceTmpL.size()), blockLen - s);
                std::fill_n(voiceTmpL.data(), static_cast<std::size_t>(chunkLen), 0.0f);
                std::fill_n(voiceTmpR.data(), static_cast<std::size_t>(chunkLen), 0.0f);

                slot.voice->renderBlock(voiceTmpL.data(), voiceTmpR.data(), chunkLen, sr);

                if (mainNumCh >= 2)
                {
                    auto* dstL = mainDryBuffer.getWritePointer(0);
                    auto* dstR = mainDryBuffer.getWritePointer(1);
                    for (int i = 0; i < chunkLen; ++i)
                    {
                        dstL[startSample + s + i] += voiceTmpL[static_cast<std::size_t>(i)];
                        dstR[startSample + s + i] += voiceTmpR[static_cast<std::size_t>(i)];
                    }
                }
                else if (mainNumCh == 1)
                {
                    auto* dst = mainDryBuffer.getWritePointer(0);
                    for (int i = 0; i < chunkLen; ++i)
                        dst[startSample + s + i] += (voiceTmpL[static_cast<std::size_t>(i)]
                                                   + voiceTmpR[static_cast<std::size_t>(i)]) * 0.5f;
                }

                if (hasAuxStem)
                {
                    if (auxNumCh >= 2)
                    {
                        auto* dstL = auxBuffer.getWritePointer(0);
                        auto* dstR = auxBuffer.getWritePointer(1);
                        for (int i = 0; i < chunkLen; ++i)
                        {
                            dstL[startSample + s + i] += voiceTmpL[static_cast<std::size_t>(i)];
                            dstR[startSample + s + i] += voiceTmpR[static_cast<std::size_t>(i)];
                        }
                    }
                    else if (auxNumCh == 1)
                    {
                        auto* dst = auxBuffer.getWritePointer(0);
                        for (int i = 0; i < chunkLen; ++i)
                            dst[startSample + s + i] += (voiceTmpL[static_cast<std::size_t>(i)]
                                                       + voiceTmpR[static_cast<std::size_t>(i)]) * 0.5f;
                    }
                }
            }

            if (!slot.voice->isActive())
                clearVoice(slot);
        }

        // C1 – render dying voices (stolen voices doing a quick crossfade out)
        for (auto& dv : dyingVoices)
        {
            if (dv.voice == nullptr)
                continue;
            if (!dv.voice->isActive())
            {
                clearDyingVoice(dv);
                continue;
            }
            {
                auto voiceContext = baseModContext;
                voiceContext.envelope = dv.voice->getEnvelopeLevel();
                voiceContext.velocity = dv.noteVelocity;
                const auto voiceModResult = modulationMatrix.process(voiceContext);

                mgs::VoiceModulation voiceMod;
                voiceMod.cutoffMul = voiceModResult.cutoffMul;
                voiceMod.resonanceAdd = voiceModResult.resonance;
                voiceMod.panAdd = voiceModResult.pan;
                voiceMod.levelMul = voiceModResult.levelMul;
                voiceMod.pitchSemi = voiceModResult.pitchSemi;
                voiceMod.attackScale = voiceModResult.attackScale;
                voiceMod.decayScale = voiceModResult.decayScale;

                dv.voice->setPitchBendFactor(pitchBend.pitchBendFactor);
                dv.voice->setVoiceModulation(voiceMod, sr);
                dv.voice->setPalmMute(getPalmMuteAmountForInstrument(dv.instrIndex));
                dv.voice->updateFilter(instrSnapshots[static_cast<std::size_t>(dv.instrIndex)].settings.tone.lowPassHz * voiceModResult.cutoffMul,
                                       instrSnapshots[static_cast<std::size_t>(dv.instrIndex)].settings.tone.stringBrightness,
                                       sr);
            }
            int targetBus = juce::jlimit(0, outputBusCount - 1, dv.outputBus);
            if (targetBus > 0 && getChannelCountOfBus(false, targetBus) <= 0)
                targetBus = 0;
            juce::AudioBuffer<float> auxBuffer;
            const bool hasAuxStem = targetBus > 0;
            if (hasAuxStem)
                auxBuffer = getBusBuffer(buffer, false, targetBus);
            const auto auxNumCh = auxBuffer.getNumChannels();
            const auto mainNumCh = mainDryBuffer.getNumChannels();
            for (int s = 0; s < blockLen; s += static_cast<int>(voiceTmpL.size()))
            {
                const int chunkLen = juce::jmin(static_cast<int>(voiceTmpL.size()), blockLen - s);
                std::fill_n(voiceTmpL.data(), static_cast<std::size_t>(chunkLen), 0.0f);
                std::fill_n(voiceTmpR.data(), static_cast<std::size_t>(chunkLen), 0.0f);
                dv.voice->renderBlock(voiceTmpL.data(), voiceTmpR.data(), chunkLen, sr);
                if (mainNumCh >= 2)
                {
                    auto* dstL = mainDryBuffer.getWritePointer(0);
                    auto* dstR = mainDryBuffer.getWritePointer(1);
                    for (int i = 0; i < chunkLen; ++i)
                    {
                        dstL[startSample + s + i] += voiceTmpL[static_cast<std::size_t>(i)];
                        dstR[startSample + s + i] += voiceTmpR[static_cast<std::size_t>(i)];
                    }
                }
                else if (mainNumCh == 1)
                {
                    auto* dst = mainDryBuffer.getWritePointer(0);
                    for (int i = 0; i < chunkLen; ++i)
                        dst[startSample + s + i] += (voiceTmpL[static_cast<std::size_t>(i)]
                                                   + voiceTmpR[static_cast<std::size_t>(i)]) * 0.5f;
                }

                if (hasAuxStem)
                {
                    if (auxNumCh >= 2)
                    {
                        auto* dstL = auxBuffer.getWritePointer(0);
                        auto* dstR = auxBuffer.getWritePointer(1);
                        for (int i = 0; i < chunkLen; ++i)
                        {
                            dstL[startSample + s + i] += voiceTmpL[static_cast<std::size_t>(i)];
                            dstR[startSample + s + i] += voiceTmpR[static_cast<std::size_t>(i)];
                        }
                    }
                    else if (auxNumCh == 1)
                    {
                        auto* dst = auxBuffer.getWritePointer(0);
                        for (int i = 0; i < chunkLen; ++i)
                            dst[startSample + s + i] += (voiceTmpL[static_cast<std::size_t>(i)]
                                                       + voiceTmpR[static_cast<std::size_t>(i)]) * 0.5f;
                    }
                }
            }
            if (!dv.voice->isActive())
                clearDyingVoice(dv);
        }
    };

    // Arpeggiator / Riff Generator — consumes note-on/off state and leaves CC/pitch events
    // in midiMessages for the main sample-accurate MIDI pass. This avoids building a
    // temporary MidiBuffer in the audio thread.
    const bool riffEnabled = std::round(getParamValue("riff_style")) > 0;
    const bool sequencerConsumesIncomingNotes = riffEnabled
        ? processRiffGenerator(midiMessages, numSamples, blockState.selectedInstrIndex)
        : processArpeggiator(midiMessages, numSamples, blockState.selectedInstrIndex);

    // Phase 3 — Update strum timing parameters from block state
    strumState.sampleRate = preparedSampleRate;
    strumState.strumSpreadMs = readCachedParamValue(globalParamRefs.strumSpread, 15.0f);
    strumState.strumSampleDelay = static_cast<int>(preparedSampleRate * strumState.strumSpreadMs / 1000.0);
    strumState.samplesUntilNextStrumNote = juce::jmax(0, strumState.samplesUntilNextStrumNote);

    // A1 – interleave MIDI events with rendering (sample-accurate)
    int currentSample = 0;

    int collectedEventCount = 0;
    for (const auto metadata : midiMessages)
    {
        if (collectedEventCount >= kMaxMidiEventsPerBlock)
            break;
        collectedMidiEvents[static_cast<std::size_t>(collectedEventCount++)] =
            { metadata.samplePosition, metadata.getMessage() };
    }

    for (int i = 1; i < collectedEventCount; ++i)
    {
        const auto value = collectedMidiEvents[static_cast<std::size_t>(i)];
        int j = i - 1;
        while (j >= 0 && collectedMidiEvents[static_cast<std::size_t>(j)].samplePosition > value.samplePosition)
        {
            collectedMidiEvents[static_cast<std::size_t>(j + 1)] = collectedMidiEvents[static_cast<std::size_t>(j)];
            --j;
        }
        collectedMidiEvents[static_cast<std::size_t>(j + 1)] = value;
    }

    auto refreshBlockModContext = [&]()
    {
        baseModContext.modWheel = modulationMatrix.modWheelValue;
        baseModContext.aftertouch = modulationMatrix.aftertouchValue;
        baseModContext.pitchBend = modulationMatrix.pitchBendValue;
        cachedModResult = modulationMatrix.process(baseModContext);
    };

    auto triggerNextPendingStrum = [&]() -> bool
    {
        int strumNote = -1;
        float strumVel = 0.0f;
        int strumInstr = 0;
        int strumChannel = 1;
        uint32_t chordHash = 0;
        if (!strumState.popPending(strumNote, strumVel, strumInstr, strumChannel, chordHash))
            return false;

        if (strumNote >= 0)
        {
            triggerNoteOn(strumInstr,
                          strumChannel,
                          strumNote,
                          applyVelocityCurve(strumVel, velocityCurve),
                          instrSnapshots[static_cast<std::size_t>(strumInstr)],
                          baseModContext,
                          chordHash);
        }

        if (strumState.pendingCount > 0)
            strumState.samplesUntilNextStrumNote = strumState.strumSampleDelay;

        return true;
    };

    auto advanceToSample = [&](int targetSample)
    {
        targetSample = juce::jlimit(currentSample, numSamples, targetSample);
        while (strumState.pendingCount > 0 && currentSample < targetSample)
        {
            const int samplesAvailable = targetSample - currentSample;
            if (strumState.samplesUntilNextStrumNote > samplesAvailable)
            {
                renderRange(currentSample, samplesAvailable);
                currentSample = targetSample;
                strumState.samplesUntilNextStrumNote -= samplesAvailable;
                return;
            }

            const int renderSamples = juce::jmax(0, strumState.samplesUntilNextStrumNote);
            renderRange(currentSample, renderSamples);
            currentSample += renderSamples;

            if (!triggerNextPendingStrum())
                break;
        }

        if (currentSample < targetSample)
        {
            renderRange(currentSample, targetSample - currentSample);
            currentSample = targetSample;
        }
    };

    auto handleNonNoteMidi = [&](const juce::MidiMessage& msg)
    {
        modulationMatrix.handleMidiMessage(msg);
        refreshBlockModContext();

        const auto midiChannel = msg.getChannel();
        if (msg.isController() && msg.getControllerNumber() == 120)
        {
            panicAllVoices();
            refreshBlockModContext();
        }
        else if (msg.isController() && msg.getControllerNumber() == 121)
        {
            resetRealtimeModulationState();
            refreshBlockModContext();
        }
        else if (msg.isAllNotesOff())
            releaseVoices(midiChannel, false);
        else if (msg.isAllSoundOff())
            panicAllVoices();
        else if (msg.isController() && msg.getControllerNumber() == 64)  // A2 – sustain pedal
        {
            sustainPedalDown = msg.getControllerValue() >= 64;
            if (!sustainPedalDown)
            {
                for (auto& slot : voices)
                    if (slot.voice && slot.voice->isActive() && !slot.keyDown
                        && !slot.voice->isReleasing())
                        slot.voice->noteOff();
            }
        }
        else if (msg.isController())
            handleMidiCC(msg.getControllerNumber(), msg.getControllerValue(), blockState.selectedInstrIndex);
        else if (msg.isPitchWheel())
            pitchBend.setPitchWheel(msg.getPitchWheelValue());
    };

    // Phase 3 — Process all MIDI events with strum timing for simultaneous note-ons.
    for (int idx = 0; idx < collectedEventCount;)
    {
        const int eventSample = juce::jlimit(0, numSamples - 1, collectedMidiEvents[static_cast<std::size_t>(idx)].samplePosition);
        advanceToSample(eventSample);

        int groupEnd = idx;
        while (groupEnd < collectedEventCount
               && collectedMidiEvents[static_cast<std::size_t>(groupEnd)].samplePosition
                    == collectedMidiEvents[static_cast<std::size_t>(idx)].samplePosition)
            ++groupEnd;

        int noteOnCount = 0;
        int noteOffCount = 0;

        for (int eventIndex = idx; eventIndex < groupEnd; ++eventIndex)
        {
            const auto& msg = collectedMidiEvents[static_cast<std::size_t>(eventIndex)].message;
            if (msg.isNoteOn())
            {
                if (sequencerConsumesIncomingNotes)
                    continue;
                if (noteOnCount < kMaxMidiEventsPerBlock)
                    noteOnEvents[static_cast<std::size_t>(noteOnCount++)] =
                        { msg.getChannel(), msg.getNoteNumber(), msg.getFloatVelocity() };
                continue;
            }

            if (msg.isNoteOff())
            {
                if (sequencerConsumesIncomingNotes)
                    continue;
                if (noteOffCount < kMaxMidiEventsPerBlock)
                    noteOffEvents[static_cast<std::size_t>(noteOffCount++)] =
                        { msg.getChannel(), msg.getNoteNumber(), msg.getFloatVelocity() };
                continue;
            }

            handleNonNoteMidi(msg);
        }

        for (int i = 0; i < noteOffCount; ++i)
        {
            const auto& evt = noteOffEvents[static_cast<std::size_t>(i)];
            triggerNoteOff(evt.midiChannel, evt.midiNote);
        }

        if (noteOnCount > 0)
        {
            if (strumState.strumEnabled && noteOnCount > 1)
            {
                int notesInChord = 0;
                std::array<int, StrumState::kMaxStrumNotes> chordNotes = {};
                std::array<float, StrumState::kMaxStrumNotes> chordVelocities = {};
                const int midiChannel = noteOnEvents[0].midiChannel;

                for (int i = 0; i < noteOnCount; ++i)
                {
                    if (notesInChord >= StrumState::kMaxStrumNotes)
                        break;

                    const auto& evt = noteOnEvents[static_cast<std::size_t>(i)];
                    chordNotes[notesInChord] = evt.midiNote;
                    chordVelocities[notesInChord] = evt.velocity;
                    ++notesInChord;
                }

                // Sort notes: bass to treble
                for (int i = 0; i < notesInChord - 1; ++i)
                {
                    for (int j = i + 1; j < notesInChord; ++j)
                    {
                        if (chordNotes[i] > chordNotes[j])
                        {
                            std::swap(chordNotes[i], chordNotes[j]);
                            std::swap(chordVelocities[i], chordVelocities[j]);
                        }
                    }
                }

                // Compute chord hash for RNG coordination
                uint32_t hash = 0x12345678u;
                for (int i = 0; i < notesInChord; ++i)
                    hash ^= static_cast<uint32_t>(chordNotes[i]) + 0x9e3779b9u + (hash << 6) + (hash >> 2);

                // A3 – encode strum direction in bit 31 so rake inverts with strum (up vs down stroke)
                if (strumState.strumDirection < 0)
                    hash ^= 0x80000000u;

                const bool wasPending = strumState.pendingCount > 0;
                const bool bassToTreble = strumState.strumDirection >= 0;
                const int firstIndex = bassToTreble ? 0 : (notesInChord - 1);

                triggerNoteOn(blockState.selectedInstrIndex,
                              midiChannel,
                              chordNotes[firstIndex],
                              applyVelocityCurve(chordVelocities[firstIndex], velocityCurve),
                              instrSnapshots[static_cast<std::size_t>(blockState.selectedInstrIndex)],
                              baseModContext,
                              hash);

                // Store remaining notes for strum
                if (notesInChord > 1)
                {
                    if (bassToTreble)
                    {
                        for (int i = 1; i < notesInChord; ++i)
                            strumState.pushPending(chordNotes[i], chordVelocities[i],
                                                   blockState.selectedInstrIndex, midiChannel, hash);
                    }
                    else
                    {
                        for (int i = notesInChord - 2; i >= 0; --i)
                            strumState.pushPending(chordNotes[i], chordVelocities[i],
                                                   blockState.selectedInstrIndex, midiChannel, hash);
                    }

                    if (!wasPending && strumState.pendingCount > 0)
                        strumState.samplesUntilNextStrumNote = strumState.strumSampleDelay;
                }
            }
            else
            {
                for (int i = 0; i < noteOnCount; ++i)
                {
                    const auto& evt = noteOnEvents[static_cast<std::size_t>(i)];
                    triggerNoteOn(blockState.selectedInstrIndex,
                                  evt.midiChannel,
                                  evt.midiNote,
                                  applyVelocityCurve(evt.velocity, velocityCurve),
                                  instrSnapshots[static_cast<std::size_t>(blockState.selectedInstrIndex)],
                                  baseModContext);
                }
            }
        }

        idx = groupEnd;
    }

    advanceToSample(numSamples);

    midiMessages.clear();

    if (mainBuffer.getNumChannels() > 0 && mainBuffer.getNumSamples() > 0)
    {
        for (int ch = 0; ch < juce::jmin(mainBuffer.getNumChannels(), mainDryBuffer.getNumChannels()); ++ch)
            mainBuffer.copyFrom(ch, 0, mainDryBuffer, ch, 0, numSamples);
        processMasterFxChain(mainBuffer, blockState);
    }

    updateOutputMeters(buffer, mainBuffer);

    // Update active voice count for UI display
    {
        int vc = 0;
        for (const auto& slot : voices)
            if (slot.voice && slot.voice->isActive())
                ++vc;
        activeVoiceCountAtomic.store(vc, std::memory_order_relaxed);
    }
}

// =============================================================================
juce::AudioProcessorEditor* GuitarSynthAudioProcessor::createEditor()
{
    return new GuitarSynthAudioProcessorEditor(*this);
}

double GuitarSynthAudioProcessor::getTailLengthSeconds() const
{
    const auto selectedInstr = getSelectedInstrIndex();
    const auto releaseSec = juce::jlimit(0.01f, 5.0f,
        getParamValue(makeInstrParamId(selectedInstr, "release")));
    const auto reverbEnabled = getParamValue(kFxReverbEnable) >= 0.5f;
    const auto delayEnabled = getParamValue(kFxDelayEnable) >= 0.5f;

    double tailSeconds = releaseSec + 0.2;
    if (reverbEnabled)
    {
        const auto reverbMix = clamp01(getParamValue(kReverbMix));
        const auto reverbSize = clamp01(getParamValue(kReverbSize));
        tailSeconds = juce::jmax(tailSeconds, 0.6 + reverbSize * 11.0 + reverbMix * 5.0);
    }

    if (delayEnabled)
    {
        double delayTimeSec = juce::jlimit(0.01, 1.5, static_cast<double>(getParamValue(kDelayTime)) * 0.001);
        if (getParamValue(kDelaySync) >= 0.5f)
        {
            const auto bpm = juce::jlimit(20.0, 240.0, readHostTempoBpm());
            const auto division = juce::jlimit(0, 5, static_cast<int>(std::round(getParamValue(kDelayDivision))));
            static constexpr double kDivisionBeats[] = { 1.0, 0.5, 0.75, 1.0 / 3.0, 0.25, 0.375 };
            delayTimeSec = (60.0 / bpm) * kDivisionBeats[static_cast<std::size_t>(division)];
        }

        const auto feedback = juce::jlimit(0.0f, 0.95f, getParamValue(kDelayFeedback));
        const auto repeatFactor = juce::jlimit(1.0, 10.0, 1.0 + static_cast<double>(feedback) * 10.0);
        tailSeconds = juce::jmax(tailSeconds, delayTimeSec * repeatFactor + 0.25);
    }

    return juce::jlimit(0.1, 30.0, tailSeconds);
}

// =============================================================================
int GuitarSynthAudioProcessor::getNumPrograms()
{
    const int sel = getSelectedInstrIndex();
    return static_cast<int>(factoryPresetBanks[static_cast<std::size_t>(sel)].size());
}

int GuitarSynthAudioProcessor::getCurrentProgram()
{
    return currentPresetIndices[static_cast<std::size_t>(getSelectedInstrIndex())];
}

void GuitarSynthAudioProcessor::setCurrentProgram(int index) { applyFactoryPreset(index); }

const juce::String GuitarSynthAudioProcessor::getProgramName(int index)
{
    const int sel = getSelectedInstrIndex();
    const auto& bank = factoryPresetBanks[static_cast<std::size_t>(sel)];
    if (index < 0 || index >= static_cast<int>(bank.size())) return {};
    return juce::String(juce::CharPointer_UTF8(bank[static_cast<std::size_t>(index)].name.c_str()));
}

void GuitarSynthAudioProcessor::changeProgramName(int, const juce::String&) {}

// =============================================================================
// =============================================================================
// Arpeggiator
// =============================================================================
bool GuitarSynthAudioProcessor::processArpeggiator(const juce::MidiBuffer& midi, int numSamples, int instrIdx)
{
    const bool arpEnabled = getParamValue("arp_enable") >= 0.5f;
    if (!arpEnabled)
        return false;

    const int  arpMode    = static_cast<int>(std::round(getParamValue("arp_mode")));
    const int  arpRateIdx = static_cast<int>(std::round(getParamValue("arp_rate")));
    const int  arpOctaves = static_cast<int>(std::round(getParamValue("arp_octaves")));
    const float arpGate   = juce::jlimit(0.05f, 1.0f, getParamValue("arp_gate"));
    const bool arpHold    = getParamValue("arp_hold") >= 0.5f;

    // Intercept MIDI note-on/off for the arp state. Non-note events stay in the
    // original buffer and are handled later by the main MIDI pass.
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            arpState.add(note);
        }
        else if (msg.isNoteOff())
        {
            const int note = msg.getNoteNumber();
            arpState.remove(note);
            if (arpHold && arpState.empty()) { /* hold last set */ }
        }
    }

    if (arpState.empty())
    {
        if (arpState.noteIsOn && arpState.lastTriggeredNote >= 0)
        {
            triggerNoteOff(arpState.lastTriggeredChannel, arpState.lastTriggeredNote);
            arpState.noteIsOn = false;
        }
        arpState.currentStep   = 0;
        arpState.currentOctave = 0;
        arpState.phaseAccum    = 0.0;
        arpState.gateCountdown = 0;
        return true;
    }

    const double bpm = lastKnownHostTempoBpm.load(std::memory_order_relaxed) > 0.0
                           ? static_cast<double>(lastKnownHostTempoBpm.load(std::memory_order_relaxed))
                           : 120.0;
    const double beatSamples = preparedSampleRate * 60.0 / bpm;
    static constexpr double kRateFactors[] = { 1.0, 0.5, 1.0/3.0, 0.25, 1.0/6.0 };
    const double rateFactor = kRateFactors[juce::jlimit(0, 4, arpRateIdx)];
    const double stepDurationSamples = beatSamples * rateFactor;

    // Gate note-off countdown
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

    arpState.phaseAccum += static_cast<double>(numSamples);

    while (arpState.phaseAccum >= stepDurationSamples)
    {
        arpState.phaseAccum -= stepDurationSamples;

        if (arpState.noteIsOn && arpState.lastTriggeredNote >= 0)
        {
            triggerNoteOff(arpState.lastTriggeredChannel, arpState.lastTriggeredNote);
            arpState.noteIsOn = false;
        }

        if (arpState.empty())
            break;

        const int numNotes = arpState.size();
        int stepIdx = arpState.currentStep;

        if (arpMode == 0) // Up
        {
            stepIdx = arpState.currentStep % numNotes;
            arpState.currentOctave = arpState.currentStep / numNotes;
            if (++arpState.currentStep >= numNotes * arpOctaves)
                arpState.currentStep = 0;
        }
        else if (arpMode == 1) // Down
        {
            const int total = numNotes * arpOctaves;
            const int pos = total - 1 - arpState.currentStep;
            stepIdx = pos % numNotes;
            arpState.currentOctave = pos / numNotes;
            if (++arpState.currentStep >= total) arpState.currentStep = 0;
        }
        else if (arpMode == 2) // Up-Down
        {
            const int total = numNotes * arpOctaves;
            const int period = total > 1 ? (2 * total - 2) : 1;
            int pos = arpState.currentStep % period;
            if (pos >= total) pos = 2 * total - 2 - pos;
            stepIdx = pos % numNotes;
            arpState.currentOctave = pos / numNotes;
            if (++arpState.currentStep >= period) arpState.currentStep = 0;
        }
        else // Random
        {
            stepIdx = nextArpRandomInt(numNotes);
            arpState.currentOctave = nextArpRandomInt(juce::jmax(1, arpOctaves));
        }

        const int baseNote   = arpState.get(stepIdx);
        const int noteToPlay = juce::jlimit(0, 127, baseNote + arpState.currentOctave * 12);

        const auto blockState = buildGlobalBlockState();
        const auto snap = buildInstrSnapshot(instrIdx, blockState);
        modmatrix::ModContext modCtx;
        modCtx.lfo1 = 0.0f;
        modCtx.lfo2 = 0.0f;

        arpState.lastTriggeredNote    = noteToPlay;
        arpState.lastTriggeredChannel = 1;
        triggerNoteOn(instrIdx, 1, noteToPlay, applyVelocityCurve(0.8f, velocityCurve), snap, modCtx);
        arpState.noteIsOn      = true;
        arpState.gateCountdown = juce::jmax(1, static_cast<int>(stepDurationSamples * static_cast<double>(arpGate)));
    }

    return true;
}

bool GuitarSynthAudioProcessor::processRiffGenerator(const juce::MidiBuffer& midi, int numSamples, int instrIdx)
{
    // Check if riff is enabled
    const int riffStyleIdx = static_cast<int>(std::round(getParamValue("riff_style")));
    if (riffStyleIdx == 0) // Off
        return false;

    // Get riff parameters
    const auto style = mgs::riff::RiffStyle(riffStyleIdx);
    const int intensity = static_cast<int>(std::round(getParamValue("riff_intensity")));
    const float gate = juce::jlimit(0.3f, 1.0f, getParamValue("riff_gate"));
    const bool holdEnabled = getParamValue("riff_hold") >= 0.5f;

    // Intercept MIDI notes into riffState. Non-note events stay in midi and are
    // handled later by the main MIDI pass.
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            auto& held = riffState.heldNotes;
            if (std::find(held.begin(), held.end(), note) == held.end())
            {
                if (riffState.heldCount < 16)
                {
                    held[static_cast<std::size_t>(riffState.heldCount)] = note;
                    ++riffState.heldCount;
                }
            }
            if (riffState.heldCount > 0)
                riffState.rootNote = held[0];
        }
        else if (msg.isNoteOff())
        {
            const int note = msg.getNoteNumber();
            auto& held = riffState.heldNotes;
            for (int i = 0; i < riffState.heldCount; ++i)
            {
                if (held[i] == note)
                {
                    for (int j = i; j < riffState.heldCount - 1; ++j)
                        held[j] = held[j + 1];
                    held[static_cast<std::size_t>(riffState.heldCount - 1)] = -1;
                    --riffState.heldCount;
                    break;
                }
            }
            if (!holdEnabled && riffState.heldCount == 0)
            {
                if (riffState.stepIndex > 0)
                {
                    riffState.reset();
                }
            }
        }
    }

    if (riffState.heldCount == 0)
    {
        riffState.reset();
        return true;
    }

    // Tempo sync
    const double bpm = lastKnownHostTempoBpm.load(std::memory_order_relaxed) > 0.0
                           ? static_cast<double>(lastKnownHostTempoBpm.load(std::memory_order_relaxed))
                           : 120.0;
    const double beatSamples = preparedSampleRate * 60.0 / bpm;
    static constexpr double kRateFactors[] = { 1.0, 0.5, 1.0/3.0, 0.25, 1.0/6.0 };
    const int arpRateIdx = static_cast<int>(std::round(getParamValue("arp_rate")));
    const double rateFactor = kRateFactors[juce::jlimit(0, 4, arpRateIdx)];
    const float stepDurationSamples = juce::jmax(1.0f, static_cast<float>(beatSamples * rateFactor));

    // Update riff style and params from APVTS
    riffState.style = style;
    riffState.intensity = intensity;
    riffState.gate = gate;
    riffState.holdEnabled = holdEnabled;

    // Update chord hash from strum state
    riffState.chordHash = 0;
    if (strumState.strumDirection < 0)
        riffState.chordHash ^= 0x80000000u;

    // Gate countdown
    for (int s = 0; s < numSamples; ++s)
    {
        if (riffState.gateCountdown > 0)
        {
            --riffState.gateCountdown;
            if (riffState.gateCountdown == 0 && arpState.lastTriggeredNote >= 0)
            {
                triggerNoteOff(arpState.lastTriggeredChannel, arpState.lastTriggeredNote);
                arpState.lastTriggeredNote = -1;
            }
        }
    }

    riffState.phaseAccum += static_cast<float>(numSamples);

    mgs::riff::RiffGenerator rg;
    const int maxRiffSteps = juce::jlimit(1, 1024, numSamples + 2);
    int riffStepsProcessed = 0;
    while (riffState.phaseAccum >= stepDurationSamples && riffStepsProcessed < maxRiffSteps)
    {
        riffState.phaseAccum -= stepDurationSamples;
        ++riffStepsProcessed;

        // Note off previous note
        if (arpState.lastTriggeredNote >= 0)
        {
            triggerNoteOff(arpState.lastTriggeredChannel, arpState.lastTriggeredNote);
            arpState.lastTriggeredNote = -1;
        }

        if (riffState.heldCount == 0)
            break;

        // Evaluate current pattern step
        uint8_t articulation = 0;
        float velocity = 0.8f;
        uint8_t octaveShift = 0;

        int noteToPlay = rg.evaluateStep(
            static_cast<float>(preparedSampleRate),
            riffState,
            articulation,
            velocity,
            octaveShift
        );

        const mgs::riff::RiffStep* step = rg.getCurrentStep(riffState);
        if (noteToPlay < 0 && step == nullptr)
        {
            riffState.reset();
            break;
        }

        if (noteToPlay >= 0)
        {
            const auto blockState = buildGlobalBlockState();
            const auto snap = buildInstrSnapshot(instrIdx, blockState);
            modmatrix::ModContext modCtx;
            modCtx.lfo1 = 0.0f;
            modCtx.lfo2 = 0.0f;

            riffState.currentArticulation = articulation;

            triggerNoteOn(instrIdx, 1, noteToPlay, applyVelocityCurve(velocity, velocityCurve),
                          snap, modCtx, riffState.chordHash);
            arpState.lastTriggeredNote = noteToPlay;
            arpState.lastTriggeredChannel = 1;
            riffState.gateCountdown = juce::jmax(1, static_cast<int>(stepDurationSamples * static_cast<double>(gate)));
        }

        // Advance step
        if (step)
            rg.advanceStep(riffState, step);
    }

    if (riffStepsProcessed >= maxRiffSteps)
        riffState.phaseAccum = 0.0f;

    return true;
}

void GuitarSynthAudioProcessor::randomizePreset(float amount)
{
    undoManager.beginNewTransaction();
    auto& rng = juce::Random::getSystemRandom();
    const int sel = getSelectedInstrIndex();
    static constexpr const char* kGlobalSuffixes[] = {
        "lfo_rate", "lfo_depth",
        "macro_corps", "macro_brillance", "macro_gain", "macro_espace",
        "reverb_size", "reverb_damping", "reverb_mix",
        "chorus_rate", "chorus_depth", "chorus_mix",
        "delay_time", "delay_feedback", "delay_mix"
    };
    for (auto* paramId : kGlobalSuffixes)
    {
        if (auto* p = parameters.getParameter(paramId))
        {
            const float cur = p->getValue();
            const float delta = (rng.nextFloat() * 2.0f - 1.0f) * amount;
            p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, cur + delta));
        }
    }
    static constexpr const char* kInstrSuffixes[] = {
        "string_brightness", "body_amount", "attack_brightness",
        "attack", "decay", "sustain", "release", "drive", "pick_position"
    };
    for (auto* suffix : kInstrSuffixes)
    {
        const auto paramId = makeInstrParamId(sel, suffix);
        if (auto* p = parameters.getParameter(paramId))
        {
            const float cur = p->getValue();
            const float delta = (rng.nextFloat() * 2.0f - 1.0f) * amount;
            p->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, cur + delta));
        }
    }
}

void GuitarSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    storeCurrentInstrumentFxState(cachedSelectedInstrumentIndex >= 0
        ? cachedSelectedInstrumentIndex
        : getSelectedInstrIndex());

    auto state = parameters.copyState();
    setStateParameterValue(state, kPlayMode, static_cast<int>(getPlayMode()));
    setStateParameterValue(state, "mono_mode",
                           getPlayMode() == PlayMode::Poly ? 0.0f : 1.0f);
    for (;;)
    {
        auto existingFxState = state.getChildWithName(kInstrumentFxState);
        if (!existingFxState.isValid())
            break;
        state.removeChild(existingFxState, nullptr);
    }

    juce::ValueTree fxStates(kInstrumentFxState);
    for (int i = 0; i < mgs::kNumInstruments; ++i)
    {
        juce::ValueTree fxState(kInstrumentFxEntry);
        fxState.setProperty("index", i, nullptr);
        writeFxStateProperties(fxState, instrumentFxStates[static_cast<std::size_t>(i)]);
        fxStates.appendChild(fxState, nullptr);
    }
    state.appendChild(fxStates, nullptr);

    for (int i = 0; i < mgs::kNumInstruments; ++i)
    {
        auto si = static_cast<std::size_t>(i);
        state.setProperty("pi_" + juce::String(i), currentPresetIndices[si], nullptr);
        if (currentUserPresetFiles[si].existsAsFile())
            state.setProperty("upf_" + juce::String(i),
                              currentUserPresetFiles[si].getFullPathName(), nullptr);
    }
    if (auto xml = state.createXml())
    {
        modulationMatrix.saveToXml(*xml);
        saveMidiLearnToXml(*xml);
        copyXmlToBinary(*xml, destData);
    }
}

void GuitarSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto xmlState = getXmlFromBinary(data, sizeInBytes);
    if (xmlState == nullptr || !xmlState->hasTagName(parameters.state.getType()))
        return;

    auto migratedXml = std::make_unique<juce::XmlElement>(*xmlState);
    migrateLegacyStateXml(*migratedXml);
    modulationMatrix.loadFromXml(*migratedXml);
    loadMidiLearnFromXml(*migratedXml);
    auto restoredState = juce::ValueTree::fromXml(*migratedXml);

    float restoredPlayMode = 0.0f;
    if (!readStateParameterValue(restoredState, kPlayMode, restoredPlayMode))
    {
        float legacyMonoMode = 0.0f;
        const auto migratedPlayMode = readStateParameterValue(restoredState, "mono_mode", legacyMonoMode)
            && legacyMonoMode >= 0.5f
                ? static_cast<float>(static_cast<int>(PlayMode::MonoLegato))
                : static_cast<float>(static_cast<int>(PlayMode::Poly));
        setStateParameterValue(restoredState, kPlayMode, migratedPlayMode);
    }

    parameters.replaceState(restoredState);
    sanitizeAllParameterValues();

    for (int i = 0; i < mgs::kNumInstruments; ++i)
    {
        const auto& bank = factoryPresetBanks[static_cast<std::size_t>(i)];
        instrumentFxStates[static_cast<std::size_t>(i)] = bank.empty()
            ? mgs::GlobalFxSettings{}
            : bank[0].fx;
    }

    const auto fxStates = restoredState.getChildWithName(kInstrumentFxState);
    if (fxStates.isValid())
    {
        for (const auto fxState : fxStates)
        {
            const int instrumentIndex = juce::jlimit(0, mgs::kNumInstruments - 1,
                static_cast<int>(fxState.getProperty("index", 0)));
            instrumentFxStates[static_cast<std::size_t>(instrumentIndex)] =
                sanitizeFxSettingsForInstrument(instrumentIndex, mgs::maskUnavailableFx(
                    instrumentIndex,
                    readFxStateProperties(fxState,
                                          instrumentFxStates[static_cast<std::size_t>(instrumentIndex)])));
        }
    }
    else
    {
        const int selectedInstrumentIndex = getSelectedInstrIndex();
        instrumentFxStates[static_cast<std::size_t>(selectedInstrumentIndex)] =
            sanitizeFxSettingsForInstrument(selectedInstrumentIndex,
                                            mgs::maskUnavailableFx(selectedInstrumentIndex, snapshotFxSettings()));
    }

    currentUserPresetFiles.fill(juce::File{});
    for (int i = 0; i < mgs::kNumInstruments; ++i)
    {
        auto si = static_cast<std::size_t>(i);
        const int maxIdx = static_cast<int>(factoryPresetBanks[si].size()) - 1;
        currentPresetIndices[si] = juce::jlimit(
            0, juce::jmax(0, maxIdx),
            static_cast<int>(restoredState.getProperty("pi_" + juce::String(i), 0)));

        auto userPath = restoredState.getProperty("upf_" + juce::String(i), "").toString();
        if (userPath.isNotEmpty())
        {
            juce::File f(userPath);
            if (f.existsAsFile()) currentUserPresetFiles[si] = f;
        }
    }

    cachedSelectedInstrumentIndex = getSelectedInstrIndex();
    pendingSelectedInstrumentIndex.store(cachedSelectedInstrumentIndex);
    applyFxToParams(cachedSelectedInstrumentIndex,
                    instrumentFxStates[static_cast<std::size_t>(cachedSelectedInstrumentIndex)],
                    false);
}

// =============================================================================
juce::StringArray GuitarSynthAudioProcessor::getFactoryPresetNames() const
{
    const int sel = getSelectedInstrIndex();
    const auto& bank = factoryPresetBanks[static_cast<std::size_t>(sel)];
    juce::StringArray names;
    for (const auto& p : bank)
        names.add(juce::String(juce::CharPointer_UTF8(p.name.c_str())));
    return names;
}

int GuitarSynthAudioProcessor::getCurrentFactoryPresetIndex() const noexcept
{
    return currentPresetIndices[static_cast<std::size_t>(getSelectedInstrIndex())];
}

bool GuitarSynthAudioProcessor::isCurrentPresetUser() const noexcept
{
    return currentUserPresetFiles[static_cast<std::size_t>(getSelectedInstrIndex())].existsAsFile();
}

juce::File GuitarSynthAudioProcessor::getCurrentUserPresetFile() const noexcept
{
    return currentUserPresetFiles[static_cast<std::size_t>(getSelectedInstrIndex())];
}

void GuitarSynthAudioProcessor::applyFactoryPreset(int presetIndex)
{
    undoManager.beginNewTransaction();
    const int sel = getSelectedInstrIndex();
    const auto& bank = factoryPresetBanks[static_cast<std::size_t>(sel)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size()))
        return;

    const auto& preset = bank[static_cast<std::size_t>(presetIndex)];
    applyInstrPresetSettings(sel, preset.settings, false);
    setParamValueInternal(makeInstrParamId(sel, kInstrOutputSuffix), static_cast<float>(preset.outputBus), false);
    setParamValueInternal(kPlayMode, juce::jlimit(0.0f, 2.0f, preset.playMode), false);
    setParamValueInternal("mono_mode", preset.playMode >= 0.5f ? 1.0f : 0.0f, false);
    setParamValueInternal(kPalmMute, juce::jlimit(0.0f, 1.0f, preset.palmMute), false);

    // --- Apply FX settings from factory preset (skipped when FX Lock is active) ---
    const bool fxLocked = getParamValue("fx_lock") >= 0.5f;
    if (!fxLocked)
    {
            instrumentFxStates[static_cast<std::size_t>(sel)] =
            sanitizeFxSettingsForInstrument(sel, mgs::maskUnavailableFx(sel, preset.fx));
        applyFxToParams(sel, instrumentFxStates[static_cast<std::size_t>(sel)], false);
    } // end if (!fxLocked)
    else
    {
        storeCurrentInstrumentFxState(sel);
    }

    currentPresetIndices[static_cast<std::size_t>(sel)] = presetIndex;
    currentUserPresetFiles[static_cast<std::size_t>(sel)] = juce::File{};
    currentCollectionPresetIndex = -1;
    updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
}

bool GuitarSynthAudioProcessor::saveFactoryPreset(int presetIndex)
{
    const int sel = getSelectedInstrIndex();
    auto& bank = factoryPresetBanks[static_cast<std::size_t>(sel)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size()))
        return false;

    storeCurrentInstrumentFxState(sel);

    auto& preset = bank[static_cast<std::size_t>(presetIndex)];
    preset.settings = captureInstrSettingsFromParams(sel);
    preset.fx = sanitizeFxSettingsForInstrument(sel, mgs::maskUnavailableFx(sel, instrumentFxStates[static_cast<std::size_t>(sel)]));
    preset.outputBus = captureInstrOutputBusFromParams(sel);
    preset.playMode = juce::jlimit(0.0f, 2.0f, getParamValue(kPlayMode));
    preset.palmMute = juce::jlimit(0.0f, 1.0f, getParamValue(kPalmMute));

    auto dir = getFactoryOverridesDirectory()
                   .getChildFile("instr_" + juce::String(sel));
    dir.createDirectory();
    auto file = dir.getChildFile(juce::String(presetIndex) + ".xml");

    PresetPersistenceState state;
    state.name = juce::String(juce::CharPointer_UTF8(preset.name.c_str()));
    state.instrIndex = sel;
    state.presetIndex = presetIndex;
    state.settings = preset.settings;
    state.fxSettings = preset.fx;
    state.outputBus = preset.outputBus;
    state.modMatrixState = {};
    state.qualityMode = 0.0f;
    state.delaySync = 0.0f;
    state.delayDivision = 1.0f;
    state.playMode = getParamValue(kPlayMode);
    state.palmMute = getParamValue(kPalmMute);
    state.metadata = makeFactoryPresetMetadata(preset.metadata);

    auto root = createPresetXml("FactoryPreset", state);
    return root->writeTo(file);
}

// =============================================================================
// Scene (collection) presets
// =============================================================================
juce::StringArray GuitarSynthAudioProcessor::getCollectionPresetNames() const
{
    // Internal scene snapshots are kept for factory workflows, not exposed in the current editor UI.
    juce::StringArray names;
    for (const auto& cp : mgs::getFactoryPresets())
        names.add(juce::String(juce::CharPointer_UTF8(cp.name.c_str())));
    return names;
}

void GuitarSynthAudioProcessor::applyCollectionPreset(int sceneIndex)
{
    const auto& scenes = mgs::getFactoryPresets();
    if (sceneIndex < 0 || sceneIndex >= static_cast<int>(scenes.size()))
        return;

    const auto& scene = scenes[static_cast<std::size_t>(sceneIndex)];

    for (int i = 0; i < mgs::kNumInstruments; ++i)
    {
        const auto& s = scene.settings[static_cast<std::size_t>(i)];
        applyInstrPresetSettings(i, s, false);
    }

    currentCollectionPresetIndex = sceneIndex;
}

void GuitarSynthAudioProcessor::loadFactoryOverrides()
{
    auto baseDir = getFactoryOverridesDirectory();
    if (!baseDir.isDirectory()) return;

    for (int i = 0; i < mgs::kNumInstruments; ++i)
    {
        auto si   = static_cast<std::size_t>(i);
        auto& bank = factoryPresetBanks[si];
        auto dir  = baseDir.getChildFile("instr_" + juce::String(i));
        if (!dir.isDirectory()) continue;

        for (int p = 0; p < static_cast<int>(bank.size()); ++p)
        {
            auto file = dir.getChildFile(juce::String(p) + ".xml");
            if (!file.existsAsFile()) continue;

            auto xml = juce::XmlDocument::parse(file);
            if (xml == nullptr || !xml->hasTagName("FactoryPreset")) continue;

            const bool needsRewrite = shouldRewritePresetXml(*xml);

            auto& s = bank[static_cast<std::size_t>(p)].settings;
            s.level          = static_cast<float>(xml->getDoubleAttribute("level",        s.level));
            s.tuneSemitones  = static_cast<float>(xml->getDoubleAttribute("tune",         s.tuneSemitones));
            s.tone.stringBrightness = getDoubleAttributeWithLegacy(*xml, kPresetStringBrightnessAttr, kLegacyPresetStringBrightnessAttr, s.tone.stringBrightness);
            s.envelope.attackSeconds = static_cast<float>(xml->getDoubleAttribute("attack",       s.envelope.attackSeconds));
            s.envelope.decaySeconds = static_cast<float>(xml->getDoubleAttribute("decay",        s.envelope.decaySeconds));
            s.envelope.sustainLevel = static_cast<float>(xml->getDoubleAttribute("sustain",      s.envelope.sustainLevel));
            s.envelope.releaseSeconds = static_cast<float>(xml->getDoubleAttribute("release",      s.envelope.releaseSeconds));
            s.resonance.bodyAmount = getDoubleAttributeWithLegacy(*xml, kPresetBodyAmountAttr, kLegacyPresetBodyAttr, s.resonance.bodyAmount);
            s.resonance.driveAmount = static_cast<float>(xml->getDoubleAttribute("drive",        s.resonance.driveAmount));
            s.performance.attackBrightness = getDoubleAttributeWithLegacy(*xml, kPresetAttackBrightnessAttr, kLegacyPresetAttackBrightnessAttr, s.performance.attackBrightness);
            s.spatial.stereoWidth = static_cast<float>(xml->getDoubleAttribute("stereo_width", s.spatial.stereoWidth));
            s.performance.pickPosition = getDoubleAttributeWithLegacy(*xml, kPresetPickPositionAttr, kLegacyPresetPickPositionAttr, s.performance.pickPosition);
            s.tone.lowPassHz = getDoubleAttributeWithLegacy(*xml, kPresetLowPassHzAttr, kLegacyPresetLowPassHzAttr, s.tone.lowPassHz);
            s.spatial.pan = static_cast<float>(xml->getDoubleAttribute("pan",          s.spatial.pan));
            readFxSettingsAttributes(*xml, bank[static_cast<std::size_t>(p)].fx);
            bank[static_cast<std::size_t>(p)].metadata = [&]
            {
                auto metadata = bank[static_cast<std::size_t>(p)].metadata;
                const auto xmlMetadata = readPresetMetadataFromXml(*xml, makeFactoryPresetMetadata(metadata));
                metadata.mixRole = xmlMetadata.mixRole.toStdString();
                metadata.familyLabel = xmlMetadata.family.toStdString();
                metadata.tags = splitTags(xmlMetadata.tags);
                metadata.nominalPeakDb = xmlMetadata.nominalPeakDb;
                return metadata;
            }();
            bank[static_cast<std::size_t>(p)].outputBus = juce::jlimit(0, kNumAuxOutputs,
                xml->getIntAttribute("output", bank[static_cast<std::size_t>(p)].outputBus));
            bank[static_cast<std::size_t>(p)].playMode = juce::jlimit(0.0f, 2.0f,
                static_cast<float>(xml->getDoubleAttribute("play_mode", bank[static_cast<std::size_t>(p)].playMode)));
            bank[static_cast<std::size_t>(p)].palmMute = juce::jlimit(0.0f, 1.0f,
                static_cast<float>(xml->getDoubleAttribute("palm_mute", bank[static_cast<std::size_t>(p)].palmMute)));

            if (needsRewrite)
            {
                PresetPersistenceState state;
                state.name = xml->getStringAttribute("name", juce::String(juce::CharPointer_UTF8(bank[static_cast<std::size_t>(p)].name.c_str())));
                state.instrIndex = i;
                state.presetIndex = p;
                state.settings = s;
                state.fxSettings = bank[static_cast<std::size_t>(p)].fx;
                state.outputBus = bank[static_cast<std::size_t>(p)].outputBus;
                state.modMatrixState = {};
                state.qualityMode = 0.0f;
                state.delaySync = 0.0f;
                state.delayDivision = 1.0f;
                state.playMode = bank[static_cast<std::size_t>(p)].playMode;
                state.palmMute = bank[static_cast<std::size_t>(p)].palmMute;
                state.metadata = makeFactoryPresetMetadata(bank[static_cast<std::size_t>(p)].metadata);
                auto normalizedXml = createPresetXml("FactoryPreset", state);
                normalizedXml->writeTo(file);
            }
        }
    }
}

// =============================================================================
juce::File GuitarSynthAudioProcessor::getFactoryOverridesDirectory()
{
    const auto preferred = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("MusiqueGuitarSynth")
                               .getChildFile("FactoryOverrides");
    return findWritableDirectory(preferred, "MusiqueGuitarSynth/FactoryOverrides");
}

juce::File GuitarSynthAudioProcessor::getUserPresetsDirectory(int instrIndex)
{
    const auto preferred = musique::preset::nativeUserPresetsDirectoryForSynth(6, instrIndex);
    return findWritableDirectory(preferred,
                                 "MusiqueGuitarSynth/Presets/instr_" + juce::String(juce::jmax(0, instrIndex)));
}

juce::Array<juce::File> GuitarSynthAudioProcessor::scanUserPresets() const
{
    juce::Array<juce::File> results;
    auto dir = getUserPresetsDirectory(getSelectedInstrIndex());
    if (dir.isDirectory())
        dir.findChildFiles(results, juce::File::findFiles, false, "*.xml");
    results.sort();
    return results;
}

void GuitarSynthAudioProcessor::updateOutputMeters(juce::AudioBuffer<float>& fullBuffer,
                                                   const juce::AudioBuffer<float>& mainBuffer)
{
    for (int channel = 0; channel < 2; ++channel)
    {
        float peak = 0.0f;
        if (channel < mainBuffer.getNumChannels())
            peak = mainBuffer.getMagnitude(channel, 0, mainBuffer.getNumSamples());
        mainMeterLevels[static_cast<std::size_t>(channel)].store(peak, std::memory_order_relaxed);
        if (peak >= 0.999f)
            clipLatched.store(true, std::memory_order_relaxed);
    }

    for (int auxIndex = 0; auxIndex < kNumAuxOutputs; ++auxIndex)
    {
        float peak = 0.0f;
        const int busIndex = auxIndex + 1;
        if (busIndex < getBusCount(false) && getChannelCountOfBus(false, busIndex) > 0)
        {
            auto auxBuffer = getBusBuffer(fullBuffer, false, busIndex);
            for (int channel = 0; channel < auxBuffer.getNumChannels(); ++channel)
                peak = juce::jmax(peak, auxBuffer.getMagnitude(channel, 0, auxBuffer.getNumSamples()));
        }
        auxMeterLevels[static_cast<std::size_t>(auxIndex)].store(peak, std::memory_order_relaxed);
    }
}

bool GuitarSynthAudioProcessor::saveUserPreset(const juce::String& name)
{
    if (name.isEmpty()) return false;
    const int sel  = getSelectedInstrIndex();
    storeCurrentInstrumentFxState(sel);
    const auto settings = captureInstrSettingsFromParams(sel);
    const auto modMatrixState = modulationMatrix.captureState();

    // Snapshot current APVTS values for this instrument only
    auto file = getUserPresetsDirectory(sel).getChildFile(
        juce::File::createLegalFileName(name) + ".xml");

    PresetPersistenceState state;
    state.name = name;
    state.instrIndex = sel;
    state.settings = settings;
    state.fxSettings = instrumentFxStates[static_cast<std::size_t>(sel)];
    state.outputBus = captureInstrOutputBusFromParams(sel);
    state.modMatrixState = modMatrixState;
    state.qualityMode = getParamValue(kQualityMode);
    state.delaySync = getParamValue(kDelaySync);
    state.delayDivision = getParamValue(kDelayDivision);
    state.playMode = getParamValue(kPlayMode);
    state.palmMute = getParamValue(kPalmMute);
    state.metadata = makeUserPresetMetadata(sel);

    auto root = createPresetXml("GuitarPreset", state);

    if (root->writeTo(file))
    {
        writePresetManifest(file, name, sel, mgs::getInstrName(sel));
        currentUserPresetFiles[static_cast<std::size_t>(sel)] = file;
        currentPresetIndices[static_cast<std::size_t>(sel)]   = -1;
        currentCollectionPresetIndex = -1;
        return true;
    }
    return false;
}

bool GuitarSynthAudioProcessor::updateUserPreset(const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    const int sel = getSelectedInstrIndex();
    storeCurrentInstrumentFxState(sel);
    const auto settings = captureInstrSettingsFromParams(sel);
    const auto modMatrixState = modulationMatrix.captureState();

    PresetPersistenceState state;
    state.name = file.getFileNameWithoutExtension();
    state.instrIndex = sel;
    state.settings = settings;
    state.fxSettings = instrumentFxStates[static_cast<std::size_t>(sel)];
    state.outputBus = captureInstrOutputBusFromParams(sel);
    state.modMatrixState = modMatrixState;
    state.qualityMode = getParamValue(kQualityMode);
    state.delaySync = getParamValue(kDelaySync);
    state.delayDivision = getParamValue(kDelayDivision);
    state.playMode = getParamValue(kPlayMode);
    state.palmMute = getParamValue(kPalmMute);
    state.metadata = makeUserPresetMetadata(sel);

    auto root = createPresetXml("GuitarPreset", state);

    if (root->writeTo(file))
    {
        writePresetManifest(file, file.getFileNameWithoutExtension(), sel, mgs::getInstrName(sel));
        currentUserPresetFiles[static_cast<std::size_t>(sel)] = file;
        currentPresetIndices[static_cast<std::size_t>(sel)]   = -1;
        currentCollectionPresetIndex = -1;
        return true;
    }
    return false;
}

bool GuitarSynthAudioProcessor::deleteUserPreset(const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    const int sel = getSelectedInstrIndex();
    if (currentUserPresetFiles[static_cast<std::size_t>(sel)] == file)
        currentUserPresetFiles[static_cast<std::size_t>(sel)] = juce::File{};
    musique::preset::manifestFileForPresetFile(file).deleteFile();
    return file.deleteFile();
}

bool GuitarSynthAudioProcessor::writePresetManifest(const juce::File& presetFile,
                                                    const juce::String& presetName,
                                                    int instrIndex,
                                                    const juce::String& sourceModel) const
{
    const auto identity = musique::preset::getSynthIdentity(6);
    if (!identity.isValid())
        return false;

    musique::preset::PresetManifest manifest;
    manifest.synthId = identity.synthId;
    manifest.synthType = identity.synthType;
    manifest.instrumentIndex = juce::jlimit(0, mgs::kNumInstruments - 1, instrIndex);
    manifest.instrumentName = mgs::getInstrName(manifest.instrumentIndex);
    manifest.presetName = presetName;
    manifest.xmlRootTag = identity.xmlRootTag;
    manifest.sourceModel = sourceModel;
    manifest.createdAt = juce::Time::getCurrentTime().toISO8601(true);
    manifest.sourcePath = presetFile.getFullPathName();
    manifest.validationVersion = 1;

    return musique::preset::saveManifestToFile(
        musique::preset::manifestFileForPresetFile(presetFile),
        manifest);
}

bool GuitarSynthAudioProcessor::loadUserPreset(const juce::File& file)
{
    undoManager.beginNewTransaction();
    if (!file.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || !xml->hasTagName("GuitarPreset")) return false;
    const bool needsRewrite = shouldRewritePresetXml(*xml);

    const auto synthIdentity = musique::preset::getSynthIdentity(6);
    const int currentInstrIndex = getSelectedInstrIndex();
    const bool hasExplicitInstrIndex = xml->hasAttribute(synthIdentity.instrumentAttrName)
        || xml->hasAttribute("instrument_index")
        || xml->hasAttribute("instrumentIndex")
        || xml->hasAttribute("index")
        || xml->hasAttribute("piano")
        || xml->hasAttribute("inst")
        || xml->hasAttribute("instr")
        || xml->hasAttribute("instrIndex");
    const int requestedInstrIndex = hasExplicitInstrIndex
        ? musique::preset::readInstrumentIndexFromXml(*xml, synthIdentity)
        : currentInstrIndex;
    const int sel = juce::jlimit(0, mgs::kNumInstruments - 1, requestedInstrIndex);

    if (sel != currentInstrIndex)
        setParamValueInternal(kSelectedInstr, static_cast<float>(sel), false);

    const auto currentSettings = captureInstrSettingsFromParams(sel);
    int warningCount = 0;
    mgs::InstrSettings s = currentSettings;
    s.level = sanitizeParameterValue(makeInstrParamId(sel, "level"),
        readFiniteXmlFloat(*xml, "level", nullptr, currentSettings.level), currentSettings.level, &warningCount);
    s.tuneSemitones = sanitizeParameterValue(makeInstrParamId(sel, "tune"),
        readFiniteXmlFloat(*xml, "tune", nullptr, currentSettings.tuneSemitones), currentSettings.tuneSemitones, &warningCount);
    s.tone.stringBrightness = sanitizeParameterValue(makeInstrParamId(sel, kInstrStringBrightnessSuffix),
        getDoubleAttributeWithLegacy(*xml, kPresetStringBrightnessAttr, kLegacyPresetStringBrightnessAttr, currentSettings.tone.stringBrightness),
        currentSettings.tone.stringBrightness, &warningCount);
    s.envelope.attackSeconds = sanitizeParameterValue(makeInstrParamId(sel, "attack"),
        readFiniteXmlFloat(*xml, "attack", nullptr, currentSettings.envelope.attackSeconds), currentSettings.envelope.attackSeconds, &warningCount);
    s.envelope.decaySeconds = sanitizeParameterValue(makeInstrParamId(sel, "decay"),
        readFiniteXmlFloat(*xml, "decay", nullptr, currentSettings.envelope.decaySeconds), currentSettings.envelope.decaySeconds, &warningCount);
    s.envelope.sustainLevel = sanitizeParameterValue(makeInstrParamId(sel, "sustain"),
        readFiniteXmlFloat(*xml, "sustain", nullptr, currentSettings.envelope.sustainLevel), currentSettings.envelope.sustainLevel, &warningCount);
    s.envelope.releaseSeconds = sanitizeParameterValue(makeInstrParamId(sel, "release"),
        readFiniteXmlFloat(*xml, "release", nullptr, currentSettings.envelope.releaseSeconds), currentSettings.envelope.releaseSeconds, &warningCount);
    s.resonance.bodyAmount = sanitizeParameterValue(makeInstrParamId(sel, kInstrBodyAmountSuffix),
        getDoubleAttributeWithLegacy(*xml, kPresetBodyAmountAttr, kLegacyPresetBodyAttr, currentSettings.resonance.bodyAmount),
        currentSettings.resonance.bodyAmount, &warningCount);
    s.resonance.driveAmount = sanitizeParameterValue(makeInstrParamId(sel, "drive"),
        readFiniteXmlFloat(*xml, "drive", nullptr, currentSettings.resonance.driveAmount), currentSettings.resonance.driveAmount, &warningCount);
    s.performance.attackBrightness = sanitizeParameterValue(makeInstrParamId(sel, kInstrAttackBrightnessSuffix),
        getDoubleAttributeWithLegacy(*xml, kPresetAttackBrightnessAttr, kLegacyPresetAttackBrightnessAttr, currentSettings.performance.attackBrightness),
        currentSettings.performance.attackBrightness, &warningCount);
    s.spatial.stereoWidth = sanitizeParameterValue(makeInstrParamId(sel, "stereo_width"),
        readFiniteXmlFloat(*xml, "stereo_width", nullptr, currentSettings.spatial.stereoWidth), currentSettings.spatial.stereoWidth, &warningCount);
    s.performance.pickPosition = sanitizeParameterValue(makeInstrParamId(sel, kInstrPickPositionSuffix),
        getDoubleAttributeWithLegacy(*xml, kPresetPickPositionAttr, kLegacyPresetPickPositionAttr, currentSettings.performance.pickPosition),
        currentSettings.performance.pickPosition, &warningCount);
    s.tone.lowPassHz = sanitizeParameterValue(makeInstrParamId(sel, kInstrLowPassHzSuffix),
        getDoubleAttributeWithLegacy(*xml, kPresetLowPassHzAttr, kLegacyPresetLowPassHzAttr, currentSettings.tone.lowPassHz),
        currentSettings.tone.lowPassHz, &warningCount);
    s.spatial.pan = sanitizeParameterValue(makeInstrParamId(sel, "pan"),
        readFiniteXmlFloat(*xml, "pan", nullptr, currentSettings.spatial.pan), currentSettings.spatial.pan, &warningCount);
    const int outputBus = juce::jlimit(0, kNumAuxOutputs,
        xml->getIntAttribute("output", captureInstrOutputBusFromParams(sel)));
    const float currentQualityMode = getParamValue(kQualityMode);
    const float currentDelaySync = getParamValue(kDelaySync);
    const float currentDelayDivision = getParamValue(kDelayDivision);
    const float currentPlayMode = getParamValue(kPlayMode);
    const float currentPalmMute = getParamValue(kPalmMute);
    const float qualityMode = xml->hasAttribute(kQualityMode)
        ? sanitizeParameterValue(kQualityMode,
                                 readFiniteXmlFloat(*xml, kQualityMode, nullptr, currentQualityMode),
                                 currentQualityMode,
                                 &warningCount)
        : currentQualityMode;
    const float delaySync = xml->hasAttribute(kDelaySync)
        ? sanitizeParameterValue(kDelaySync,
                                 readFiniteXmlFloat(*xml, kDelaySync, nullptr, currentDelaySync),
                                 currentDelaySync,
                                 &warningCount)
        : currentDelaySync;
    const float delayDivision = xml->hasAttribute(kDelayDivision)
        ? sanitizeParameterValue(kDelayDivision,
                                 readFiniteXmlFloat(*xml, kDelayDivision, nullptr, currentDelayDivision),
                                 currentDelayDivision,
                                 &warningCount)
        : currentDelayDivision;
    const float playMode = xml->hasAttribute(kPlayMode)
        ? sanitizeParameterValue(kPlayMode,
                                 readFiniteXmlFloat(*xml, kPlayMode, nullptr, currentPlayMode),
                                 currentPlayMode,
                                 &warningCount)
        : currentPlayMode;
    const float palmMute = xml->hasAttribute(kPalmMute)
        ? sanitizeParameterValue(kPalmMute,
                                 readFiniteXmlFloat(*xml, kPalmMute, nullptr, currentPalmMute),
                                 currentPalmMute,
                                 &warningCount)
        : currentPalmMute;
    auto modMatrixState = modulationMatrix.captureState();
    const bool hasModMatrixState = xml->getChildByName("ModMatrix") != nullptr;
    if (hasModMatrixState)
        modmatrix::ModulationMatrix::loadStateFromXml(*xml, modMatrixState);
    auto presetFx = instrumentFxStates[static_cast<std::size_t>(sel)];
    readFxSettingsAttributes(*xml, presetFx);
    presetFx = sanitizeFxSettingsForInstrument(sel, mgs::maskUnavailableFx(sel, presetFx));
    const auto metadata = readPresetMetadataFromXml(*xml, makeUserPresetMetadata(sel));

    applyInstrPresetSettings(sel, s, false);
    setParamValueInternal(makeInstrParamId(sel, kInstrOutputSuffix), static_cast<float>(outputBus), false);
    setParamValueInternal(kQualityMode, qualityMode, false);
    setParamValueInternal(kDelaySync, delaySync, false);
    setParamValueInternal(kDelayDivision, delayDivision, false);
    setParamValueInternal(kPlayMode, playMode, false);
    setParamValueInternal(kPalmMute, palmMute, false);
    instrumentFxStates[static_cast<std::size_t>(sel)] = presetFx;
    applyFxToParams(sel, presetFx, false);
    if (hasModMatrixState)
        modulationMatrix.applyState(modMatrixState);

    if (needsRewrite)
    {
        PresetPersistenceState state;
        state.name = xml->getStringAttribute("name", file.getFileNameWithoutExtension());
        state.instrIndex = sel;
        state.settings = s;
        state.fxSettings = presetFx;
        state.outputBus = outputBus;
        state.modMatrixState = modMatrixState;
        state.qualityMode = qualityMode;
        state.delaySync = delaySync;
        state.delayDivision = delayDivision;
        state.playMode = playMode;
        state.palmMute = palmMute;
        state.metadata = metadata;
        auto normalizedXml = createPresetXml("GuitarPreset", state);
        normalizedXml->writeTo(file);
    }

    writePresetManifest(file,
                        xml->getStringAttribute("name", file.getFileNameWithoutExtension()),
                        sel,
                        mgs::getInstrName(sel));

    currentUserPresetFiles[static_cast<std::size_t>(sel)] = file;
    currentPresetIndices[static_cast<std::size_t>(sel)]   = -1;
    currentCollectionPresetIndex = -1;
    updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
    return true;
}

// =============================================================================
int GuitarSynthAudioProcessor::getSelectedInstrIndex() const
{
    return juce::jlimit(0, mgs::kNumInstruments - 1,
                        static_cast<int>(std::round(getParamValue(kSelectedInstr))));
}

GuitarSynthAudioProcessor::QualityMode GuitarSynthAudioProcessor::getQualityMode() const noexcept
{
    return getParamValue(kQualityMode) >= 0.5f ? QualityMode::Studio : QualityMode::Live;
}

bool GuitarSynthAudioProcessor::isDelaySyncEnabled() const noexcept
{
    return getParamValue(kDelaySync) >= 0.5f;
}

int GuitarSynthAudioProcessor::getDelayDivisionIndex() const noexcept
{
    return juce::jlimit(0, 5, static_cast<int>(std::round(getParamValue(kDelayDivision))));
}

float GuitarSynthAudioProcessor::getLastKnownHostTempoBpm() const noexcept
{
    return lastKnownHostTempoBpm.load(std::memory_order_relaxed);
}

float GuitarSynthAudioProcessor::getMainMeterLevel(int channel) const noexcept
{
    return mainMeterLevels[static_cast<std::size_t>(juce::jlimit(0, 1, channel))].load(std::memory_order_relaxed);
}

float GuitarSynthAudioProcessor::getAuxMeterLevel(int auxIndex) const noexcept
{
    return auxMeterLevels[static_cast<std::size_t>(juce::jlimit(0, kNumAuxOutputs - 1, auxIndex))].load(std::memory_order_relaxed);
}

bool GuitarSynthAudioProcessor::isClipLatched() const noexcept
{
    return clipLatched.load(std::memory_order_relaxed);
}

void GuitarSynthAudioProcessor::clearClipLatch() noexcept
{
    clipLatched.store(false, std::memory_order_relaxed);
}

modmatrix::ModSlot GuitarSynthAudioProcessor::getModMatrixSlot(int index) const
{
    const int safeIndex = juce::jlimit(0, modmatrix::ModulationMatrix::getNumSlots() - 1, index);
    return modulationMatrix.getSlot(safeIndex);
}

void GuitarSynthAudioProcessor::setModMatrixSlot(int index,
                                                 modmatrix::Source source,
                                                 modmatrix::Destination destination,
                                                 float amount)
{
    const int safeIndex = juce::jlimit(0, modmatrix::ModulationMatrix::getNumSlots() - 1, index);
    modulationMatrix.setSlot(safeIndex, source, destination, amount);
    updateHostDisplay(juce::AudioProcessor::ChangeDetails().withParameterInfoChanged(true));
}

float GuitarSynthAudioProcessor::getModMatrixLfo2Rate() const noexcept
{
    return modulationMatrix.lfo2.getRate();
}

int GuitarSynthAudioProcessor::getModMatrixLfo2Wave() const noexcept
{
    return modulationMatrix.lfo2.getWave();
}

void GuitarSynthAudioProcessor::setModMatrixLfo2Rate(float rateHz)
{
    modulationMatrix.lfo2.setRate(juce::jlimit(0.05f, 12.0f, rateHz));
}

void GuitarSynthAudioProcessor::setModMatrixLfo2Wave(int waveformIndex)
{
    modulationMatrix.lfo2.setWave(juce::jlimit(0, 3, waveformIndex));
}

float GuitarSynthAudioProcessor::getParamValue(const juce::String& paramId) const
{
    if (const auto* raw = parameters.getRawParameterValue(paramId))
        return raw->load();
    return 0.0f;
}

float GuitarSynthAudioProcessor::sanitizeParameterValue(const juce::String& paramId, float value, float fallback, int* warningCount) const
{
    if (!std::isfinite(value))
    {
        if (warningCount != nullptr)
            ++(*warningCount);
        value = fallback;
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

void GuitarSynthAudioProcessor::setParamValueInternal(const juce::String& paramId, float value, bool notifyHost)
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
    }
}

void GuitarSynthAudioProcessor::setParamValue(const juce::String& paramId, float value)
{
    setParamValueInternal(paramId, value, true);
}

void GuitarSynthAudioProcessor::sanitizeAllParameterValues()
{
    static constexpr const char* kGlobalParamIds[] = {
        kOutputGain, kSelectedInstr, kQualityMode, kDelaySync, kDelayDivision,
        kLfoRate, kLfoDepth, kLfoWave,
        kMacroCorps, kMacroBrillance, kMacroGain, kMacroEspace,
        kCompThreshold, kCompRatio, kCompAttack, kCompRelease, kCompMakeup, kCompMix,
        kSatDrive, kSatMix, kTransientAttack, kTransientSustain, kTransientMix,
        kChorusRate, kChorusDepth, kChorusDelay, kChorusMix,
        kReverbSize, kReverbDamping, kReverbWidth, kReverbMix,
        kEqLowFreq, kEqLowGain, kEqMidFreq, kEqMidGain, kEqMidQ, kEqHighFreq, kEqHighGain,
        kDelayTime, kDelayFeedback, kDelayMix,
        kLimiterThreshold, kLimiterRelease, kCabMix,
        kFxSatEnable, kFxTransientEnable, kFxCompEnable, kFxReverbEnable, kFxEqEnable,
        kFxChorusEnable, kFxDelayEnable, kFxLimiterEnable, kFxCabinetEnable, "fx_lock",
        "reverb_type", "velocity_curve", kPlayMode, "mono_mode", "pitch_bend_range", kPalmMute
    };

    for (const auto* paramId : kGlobalParamIds)
        setParamValueInternal(paramId, getParamValue(paramId), false);

    static constexpr const char* kInstrParamSuffixes[] = {
        "level", "tune", kInstrStringBrightnessSuffix, "attack", "decay", "sustain", "release",
        kInstrBodyAmountSuffix, "drive", kInstrAttackBrightnessSuffix, "stereo_width",
        kInstrPickPositionSuffix, kInstrLowPassHzSuffix, "pan", kInstrOutputSuffix
    };

    for (int instrIndex = 0; instrIndex < mgs::kNumInstruments; ++instrIndex)
        for (const auto* suffix : kInstrParamSuffixes)
        {
            const auto paramId = makeInstrParamId(instrIndex, suffix);
            setParamValueInternal(paramId, getParamValue(paramId), false);
        }
}

float GuitarSynthAudioProcessor::readCachedParamValue(const std::atomic<float>* raw, float fallback) const noexcept
{
    return raw != nullptr ? raw->load(std::memory_order_relaxed) : fallback;
}

void GuitarSynthAudioProcessor::resolveParameterPointers()
{
    auto resolveRaw = [this](const juce::String& paramId) -> std::atomic<float>*
    {
        return parameters.getRawParameterValue(paramId);
    };

    globalParamRefs.outputGain = resolveRaw(kOutputGain);
    globalParamRefs.selectedInstr = resolveRaw(kSelectedInstr);
    globalParamRefs.qualityMode = resolveRaw(kQualityMode);
    globalParamRefs.delaySync = resolveRaw(kDelaySync);
    globalParamRefs.delayDivision = resolveRaw(kDelayDivision);
    globalParamRefs.lfoRate = resolveRaw(kLfoRate);
    globalParamRefs.lfoDepth = resolveRaw(kLfoDepth);
    globalParamRefs.lfoWave = resolveRaw(kLfoWave);
    globalParamRefs.macroCorps = resolveRaw(kMacroCorps);
    globalParamRefs.macroBrillance = resolveRaw(kMacroBrillance);
    globalParamRefs.macroGain = resolveRaw(kMacroGain);
    globalParamRefs.macroEspace = resolveRaw(kMacroEspace);
    globalParamRefs.compThreshold = resolveRaw(kCompThreshold);
    globalParamRefs.compRatio = resolveRaw(kCompRatio);
    globalParamRefs.compAttack = resolveRaw(kCompAttack);
    globalParamRefs.compRelease = resolveRaw(kCompRelease);
    globalParamRefs.compMakeup = resolveRaw(kCompMakeup);
    globalParamRefs.compMix = resolveRaw(kCompMix);
    globalParamRefs.satDrive = resolveRaw(kSatDrive);
    globalParamRefs.satMix = resolveRaw(kSatMix);
    globalParamRefs.transientAttack = resolveRaw(kTransientAttack);
    globalParamRefs.transientSustain = resolveRaw(kTransientSustain);
    globalParamRefs.transientMix = resolveRaw(kTransientMix);
    globalParamRefs.chorusRate = resolveRaw(kChorusRate);
    globalParamRefs.chorusDepth = resolveRaw(kChorusDepth);
    globalParamRefs.chorusDelay = resolveRaw(kChorusDelay);
    globalParamRefs.chorusMix = resolveRaw(kChorusMix);
    globalParamRefs.reverbSize = resolveRaw(kReverbSize);
    globalParamRefs.reverbDamping = resolveRaw(kReverbDamping);
    globalParamRefs.reverbWidth = resolveRaw(kReverbWidth);
    globalParamRefs.reverbMix = resolveRaw(kReverbMix);
    globalParamRefs.eqLowFreq = resolveRaw(kEqLowFreq);
    globalParamRefs.eqLowGain = resolveRaw(kEqLowGain);
    globalParamRefs.eqMidFreq = resolveRaw(kEqMidFreq);
    globalParamRefs.eqMidGain = resolveRaw(kEqMidGain);
    globalParamRefs.eqMidQ = resolveRaw(kEqMidQ);
    globalParamRefs.eqHighFreq = resolveRaw(kEqHighFreq);
    globalParamRefs.eqHighGain = resolveRaw(kEqHighGain);
    globalParamRefs.delayTime = resolveRaw(kDelayTime);
    globalParamRefs.delayFeedback = resolveRaw(kDelayFeedback);
    globalParamRefs.delayMix = resolveRaw(kDelayMix);
    globalParamRefs.limiterThreshold = resolveRaw(kLimiterThreshold);
    globalParamRefs.limiterRelease = resolveRaw(kLimiterRelease);
    globalParamRefs.cabMix = resolveRaw(kCabMix);
    globalParamRefs.strumSpread = resolveRaw(kStrumSpread);
    globalParamRefs.fxSatEnable = resolveRaw(kFxSatEnable);
    globalParamRefs.fxTransientEnable = resolveRaw(kFxTransientEnable);
    globalParamRefs.fxCompEnable = resolveRaw(kFxCompEnable);
    globalParamRefs.fxReverbEnable = resolveRaw(kFxReverbEnable);
    globalParamRefs.fxEqEnable = resolveRaw(kFxEqEnable);
    globalParamRefs.fxChorusEnable = resolveRaw(kFxChorusEnable);
    globalParamRefs.fxDelayEnable = resolveRaw(kFxDelayEnable);
    globalParamRefs.fxLimiterEnable = resolveRaw(kFxLimiterEnable);
    globalParamRefs.fxCabinetEnable = resolveRaw(kFxCabinetEnable);
    globalParamRefs.fxLock = resolveRaw("fx_lock");

    for (int instrIndex = 0; instrIndex < mgs::kNumInstruments; ++instrIndex)
    {
        auto& refs = instrParamRefs[static_cast<std::size_t>(instrIndex)];
        refs.level = resolveRaw(makeInstrParamId(instrIndex, "level"));
        refs.tune = resolveRaw(makeInstrParamId(instrIndex, "tune"));
        refs.stringBrightness = resolveRaw(makeInstrParamId(instrIndex, kInstrStringBrightnessSuffix));
        refs.attack = resolveRaw(makeInstrParamId(instrIndex, "attack"));
        refs.decay = resolveRaw(makeInstrParamId(instrIndex, "decay"));
        refs.sustain = resolveRaw(makeInstrParamId(instrIndex, "sustain"));
        refs.release = resolveRaw(makeInstrParamId(instrIndex, "release"));
        refs.bodyAmount = resolveRaw(makeInstrParamId(instrIndex, kInstrBodyAmountSuffix));
        refs.drive = resolveRaw(makeInstrParamId(instrIndex, "drive"));
        refs.attackBrightness = resolveRaw(makeInstrParamId(instrIndex, kInstrAttackBrightnessSuffix));
        refs.stereoWidth = resolveRaw(makeInstrParamId(instrIndex, "stereo_width"));
        refs.pickPosition = resolveRaw(makeInstrParamId(instrIndex, kInstrPickPositionSuffix));
        refs.lowPassHz = resolveRaw(makeInstrParamId(instrIndex, kInstrLowPassHzSuffix));
        refs.pan = resolveRaw(makeInstrParamId(instrIndex, "pan"));
        refs.output = resolveRaw(makeInstrParamId(instrIndex, kInstrOutputSuffix));
    }
}

double GuitarSynthAudioProcessor::readHostTempoBpm() const
{
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
            if (auto bpm = position->getBpm())
                return juce::jlimit(20.0, 240.0, *bpm);
    }

    return juce::jlimit(20.0, 240.0, static_cast<double>(lastKnownHostTempoBpm.load(std::memory_order_relaxed)));
}

GuitarSynthAudioProcessor::GlobalBlockState GuitarSynthAudioProcessor::buildGlobalBlockState() const
{
    GlobalBlockState state;
    state.selectedInstrIndex = juce::jlimit(0, mgs::kNumInstruments - 1,
        static_cast<int>(std::round(readCachedParamValue(globalParamRefs.selectedInstr, 0.0f))));
    state.qualityMode = juce::jlimit(0, 1,
        static_cast<int>(std::round(readCachedParamValue(globalParamRefs.qualityMode, 0.0f))));
    state.delayDivision = juce::jlimit(0, 5,
        static_cast<int>(std::round(readCachedParamValue(globalParamRefs.delayDivision, 1.0f))));
    state.lfoWave = juce::jlimit(0, 3,
        static_cast<int>(std::round(readCachedParamValue(globalParamRefs.lfoWave, 0.0f))));
    state.lfoRate = juce::jlimit(0.05f, 12.0f, readCachedParamValue(globalParamRefs.lfoRate, 2.0f));
    state.lfoDepth = clamp01(readCachedParamValue(globalParamRefs.lfoDepth, 0.0f));
    state.hostBpm = static_cast<float>(readHostTempoBpm());
    state.outputGainDb = juce::jlimit(-24.0f, 12.0f, readCachedParamValue(globalParamRefs.outputGain, -3.0f));
    state.macroCorps = clamp01(readCachedParamValue(globalParamRefs.macroCorps, 0.5f));
    state.macroBrillance = clamp01(readCachedParamValue(globalParamRefs.macroBrillance, 0.5f));
    state.macroGain = clamp01(readCachedParamValue(globalParamRefs.macroGain, 0.5f));
    state.macroEspace = clamp01(readCachedParamValue(globalParamRefs.macroEspace, 0.5f));
    state.fxLock = readCachedParamValue(globalParamRefs.fxLock, 0.0f) >= 0.5f;
    state.fx.satDrive = readCachedParamValue(globalParamRefs.satDrive, 1.5f);
    state.fx.satMix = readCachedParamValue(globalParamRefs.satMix, 0.1f);
    state.fx.saturatorOn = readCachedParamValue(globalParamRefs.fxSatEnable, 1.0f) >= 0.5f;
    state.fx.transientAttack = readCachedParamValue(globalParamRefs.transientAttack, 0.05f);
    state.fx.transientSustain = readCachedParamValue(globalParamRefs.transientSustain, 0.0f);
    state.fx.transientMix = readCachedParamValue(globalParamRefs.transientMix, 0.3f);
    state.fx.transientOn = readCachedParamValue(globalParamRefs.fxTransientEnable, 1.0f) >= 0.5f;
    state.fx.eqLowFreq = readCachedParamValue(globalParamRefs.eqLowFreq, 200.0f);
    state.fx.eqLowGain = readCachedParamValue(globalParamRefs.eqLowGain, 0.0f);
    state.fx.eqMidFreq = readCachedParamValue(globalParamRefs.eqMidFreq, 1000.0f);
    state.fx.eqMidGain = readCachedParamValue(globalParamRefs.eqMidGain, 0.0f);
    state.fx.eqMidQ = readCachedParamValue(globalParamRefs.eqMidQ, 1.0f);
    state.fx.eqHighFreq = readCachedParamValue(globalParamRefs.eqHighFreq, 5000.0f);
    state.fx.eqHighGain = readCachedParamValue(globalParamRefs.eqHighGain, 0.0f);
    state.fx.eqOn = readCachedParamValue(globalParamRefs.fxEqEnable, 1.0f) >= 0.5f;
    state.fx.compThreshold = readCachedParamValue(globalParamRefs.compThreshold, -19.0f);
    state.fx.compRatio = readCachedParamValue(globalParamRefs.compRatio, 3.0f);
    state.fx.compAttack = readCachedParamValue(globalParamRefs.compAttack, 10.0f);
    state.fx.compRelease = readCachedParamValue(globalParamRefs.compRelease, 120.0f);
    state.fx.compMakeup = readCachedParamValue(globalParamRefs.compMakeup, 0.0f);
    state.fx.compMix = readCachedParamValue(globalParamRefs.compMix, 1.0f);
    state.fx.compressorOn = readCachedParamValue(globalParamRefs.fxCompEnable, 1.0f) >= 0.5f;
    state.fx.chorusRate = readCachedParamValue(globalParamRefs.chorusRate, 1.5f);
    state.fx.chorusDepth = readCachedParamValue(globalParamRefs.chorusDepth, 0.0f);
    state.fx.chorusDelay = readCachedParamValue(globalParamRefs.chorusDelay, 8.0f);
    state.fx.chorusMix = readCachedParamValue(globalParamRefs.chorusMix, 0.0f);
    state.fx.chorusOn = readCachedParamValue(globalParamRefs.fxChorusEnable, 1.0f) >= 0.5f;
    state.fx.delayTime = readCachedParamValue(globalParamRefs.delayTime, 300.0f);
    state.fx.delayFeedback = readCachedParamValue(globalParamRefs.delayFeedback, 0.3f);
    state.fx.delayMix = readCachedParamValue(globalParamRefs.delayMix, 0.0f);
    state.fx.delayOn = readCachedParamValue(globalParamRefs.fxDelayEnable, 1.0f) >= 0.5f;
    state.delaySyncToHost = readCachedParamValue(globalParamRefs.delaySync, 0.0f) >= 0.5f;
    state.fx.reverbSize = readCachedParamValue(globalParamRefs.reverbSize, 0.45f);
    state.fx.reverbDamping = readCachedParamValue(globalParamRefs.reverbDamping, 0.55f);
    state.fx.reverbWidth = readCachedParamValue(globalParamRefs.reverbWidth, 0.85f);
    state.fx.reverbMix = readCachedParamValue(globalParamRefs.reverbMix, 0.2f);
    state.fx.reverbOn = readCachedParamValue(globalParamRefs.fxReverbEnable, 1.0f) >= 0.5f;
    state.fx.limiterThreshold = readCachedParamValue(globalParamRefs.limiterThreshold, -0.3f);
    state.fx.limiterRelease = readCachedParamValue(globalParamRefs.limiterRelease, 50.0f);
    state.fx.limiterOn = readCachedParamValue(globalParamRefs.fxLimiterEnable, 1.0f) >= 0.5f;
    state.fx.cabMix = readCachedParamValue(globalParamRefs.cabMix, 0.0f);
    state.fx.cabinetOn = readCachedParamValue(globalParamRefs.fxCabinetEnable, 1.0f) >= 0.5f;
    state.fx = sanitizeFxSettingsForInstrument(state.selectedInstrIndex, state.fx);
    return state;
}

GuitarSynthAudioProcessor::InstrSnapshot GuitarSynthAudioProcessor::buildInstrSnapshot(int instrIndex, const GlobalBlockState& blockState) const
{
    InstrSnapshot snapshot;
    if (instrIndex < 0 || instrIndex >= mgs::kNumInstruments)
        return snapshot;

    const auto& refs = instrParamRefs[static_cast<std::size_t>(instrIndex)];
    snapshot.settings.level = clamp01(readCachedParamValue(refs.level, 0.8f));
    snapshot.settings.tuneSemitones = juce::jlimit(-24.0f, 24.0f, readCachedParamValue(refs.tune, 0.0f));
    snapshot.settings.tone.stringBrightness = clamp01(readCachedParamValue(refs.stringBrightness, 0.5f));
    snapshot.settings.envelope.attackSeconds = juce::jlimit(0.0f, 2.0f, readCachedParamValue(refs.attack, 0.003f));
    snapshot.settings.envelope.decaySeconds = juce::jlimit(0.1f, 10.0f, readCachedParamValue(refs.decay, 3.0f));
    snapshot.settings.envelope.sustainLevel = clamp01(readCachedParamValue(refs.sustain, 0.4f));
    snapshot.settings.envelope.releaseSeconds = juce::jlimit(0.01f, 5.0f, readCachedParamValue(refs.release, 0.4f));
    snapshot.settings.resonance.bodyAmount = clamp01(readCachedParamValue(refs.bodyAmount, 0.4f));
    snapshot.settings.resonance.driveAmount = clamp01(readCachedParamValue(refs.drive, 0.0f));
    snapshot.settings.performance.attackBrightness = clamp01(readCachedParamValue(refs.attackBrightness, 0.5f));
    snapshot.settings.spatial.stereoWidth = clamp01(readCachedParamValue(refs.stereoWidth, 0.4f));
    snapshot.settings.performance.pickPosition = clamp01(readCachedParamValue(refs.pickPosition, 0.5f));
    snapshot.settings.tone.lowPassHz = juce::jlimit(120.0f, 16000.0f, readCachedParamValue(refs.lowPassHz, 8000.0f));
    snapshot.settings.spatial.pan = juce::jlimit(-1.0f, 1.0f, readCachedParamValue(refs.pan, 0.0f));
    snapshot.outputBus = juce::jlimit(0, kNumAuxOutputs,
        static_cast<int>(std::round(readCachedParamValue(refs.output, 0.0f))));
    applyPerformanceMacros(instrIndex, snapshot.settings, blockState);
    return snapshot;
}

 mgs::InstrSettings GuitarSynthAudioProcessor::captureInstrSettingsFromParams(int instrIndex) const
{
    mgs::InstrSettings settings;
    if (instrIndex < 0 || instrIndex >= mgs::kNumInstruments)
        return settings;

    settings.level = sanitizeParameterValue(makeInstrParamId(instrIndex, "level"),
        getParamValue(makeInstrParamId(instrIndex, "level")), settings.level);
    settings.tuneSemitones = sanitizeParameterValue(makeInstrParamId(instrIndex, "tune"),
        getParamValue(makeInstrParamId(instrIndex, "tune")), settings.tuneSemitones);
    settings.tone.stringBrightness = sanitizeParameterValue(makeInstrParamId(instrIndex, kInstrStringBrightnessSuffix),
        getParamValue(makeInstrParamId(instrIndex, kInstrStringBrightnessSuffix)), settings.tone.stringBrightness);
    settings.envelope.attackSeconds = sanitizeParameterValue(makeInstrParamId(instrIndex, "attack"),
        getParamValue(makeInstrParamId(instrIndex, "attack")), settings.envelope.attackSeconds);
    settings.envelope.decaySeconds = sanitizeParameterValue(makeInstrParamId(instrIndex, "decay"),
        getParamValue(makeInstrParamId(instrIndex, "decay")), settings.envelope.decaySeconds);
    settings.envelope.sustainLevel = sanitizeParameterValue(makeInstrParamId(instrIndex, "sustain"),
        getParamValue(makeInstrParamId(instrIndex, "sustain")), settings.envelope.sustainLevel);
    settings.envelope.releaseSeconds = sanitizeParameterValue(makeInstrParamId(instrIndex, "release"),
        getParamValue(makeInstrParamId(instrIndex, "release")), settings.envelope.releaseSeconds);
    settings.resonance.bodyAmount = sanitizeParameterValue(makeInstrParamId(instrIndex, kInstrBodyAmountSuffix),
        getParamValue(makeInstrParamId(instrIndex, kInstrBodyAmountSuffix)), settings.resonance.bodyAmount);
    settings.resonance.driveAmount = sanitizeParameterValue(makeInstrParamId(instrIndex, "drive"),
        getParamValue(makeInstrParamId(instrIndex, "drive")), settings.resonance.driveAmount);
    settings.performance.attackBrightness = sanitizeParameterValue(makeInstrParamId(instrIndex, kInstrAttackBrightnessSuffix),
        getParamValue(makeInstrParamId(instrIndex, kInstrAttackBrightnessSuffix)), settings.performance.attackBrightness);
    settings.spatial.stereoWidth = sanitizeParameterValue(makeInstrParamId(instrIndex, "stereo_width"),
        getParamValue(makeInstrParamId(instrIndex, "stereo_width")), settings.spatial.stereoWidth);
    settings.performance.pickPosition = sanitizeParameterValue(makeInstrParamId(instrIndex, kInstrPickPositionSuffix),
        getParamValue(makeInstrParamId(instrIndex, kInstrPickPositionSuffix)), settings.performance.pickPosition);
    settings.tone.lowPassHz = sanitizeParameterValue(makeInstrParamId(instrIndex, kInstrLowPassHzSuffix),
        getParamValue(makeInstrParamId(instrIndex, kInstrLowPassHzSuffix)), settings.tone.lowPassHz);
    settings.spatial.pan = sanitizeParameterValue(makeInstrParamId(instrIndex, "pan"),
        getParamValue(makeInstrParamId(instrIndex, "pan")), settings.spatial.pan);
    return settings;
}

int GuitarSynthAudioProcessor::captureInstrOutputBusFromParams(int instrIndex) const
{
    if (instrIndex < 0 || instrIndex >= mgs::kNumInstruments)
        return 0;

    return juce::jlimit(0, kNumAuxOutputs,
        static_cast<int>(std::round(getParamValue(makeInstrParamId(instrIndex, kInstrOutputSuffix)))));
}

void GuitarSynthAudioProcessor::applyPerformanceMacros(int instrIndex, mgs::InstrSettings& s, const GlobalBlockState& blockState) const
{
    const auto corps     = (blockState.macroCorps - 0.5f) * 2.0f;
    const auto brillance = (blockState.macroBrillance - 0.5f) * 2.0f;
    const auto gain      = (blockState.macroGain - 0.5f) * 2.0f;
    const auto espace    = (blockState.macroEspace - 0.5f) * 2.0f;

    const auto family = mgs::getFamily(instrIndex);

    float corpsDepth = 0.20f;
    float decayMulDepth = 0.25f;
    float brightDepth = 0.18f;
    float cutoffOctaves = 0.40f;
    float attackDepth = 0.12f;
    float driveDepth = 0.20f;
    float levelDepth = 0.06f;
    float widthDepth = 0.22f;
    float releaseMulDepth = 0.30f;

    switch (family)
    {
        case mgs::Family::Acoustique:
            corpsDepth = 0.24f;
            decayMulDepth = 0.18f;
            brightDepth = 0.14f;
            cutoffOctaves = 0.26f;
            attackDepth = 0.09f;
            driveDepth = 0.05f;
            levelDepth = 0.04f;
            widthDepth = 0.12f;
            releaseMulDepth = 0.16f;
            break;
        case mgs::Family::Electrique:
            corpsDepth = 0.18f;
            decayMulDepth = 0.22f;
            brightDepth = 0.18f;
            cutoffOctaves = 0.34f;
            attackDepth = 0.12f;
            driveDepth = 0.24f;
            levelDepth = 0.05f;
            widthDepth = 0.16f;
            releaseMulDepth = 0.22f;
            break;
        case mgs::Family::Electronique:
            corpsDepth = 0.12f;
            decayMulDepth = 0.28f;
            brightDepth = 0.20f;
            cutoffOctaves = 0.48f;
            attackDepth = 0.08f;
            driveDepth = 0.10f;
            levelDepth = 0.05f;
            widthDepth = 0.28f;
            releaseMulDepth = 0.34f;
            break;
    }

    s.resonance.bodyAmount = clamp01(s.resonance.bodyAmount + corps * corpsDepth);
    s.envelope.decaySeconds = juce::jlimit(0.1f, 10.0f, s.envelope.decaySeconds * (1.0f + corps * decayMulDepth));
    s.tone.stringBrightness = clamp01(s.tone.stringBrightness + brillance * brightDepth);
    s.tone.lowPassHz = juce::jlimit(120.0f, 16000.0f, s.tone.lowPassHz * std::pow(2.0f, brillance * cutoffOctaves));
    s.performance.attackBrightness = clamp01(s.performance.attackBrightness + brillance * attackDepth);
    s.resonance.driveAmount = clamp01(s.resonance.driveAmount + gain * driveDepth);
    s.level = clamp01(s.level + gain * levelDepth);
    s.spatial.stereoWidth = clamp01(s.spatial.stereoWidth + espace * widthDepth);
    s.envelope.releaseSeconds = juce::jlimit(0.01f, 5.0f, s.envelope.releaseSeconds * (1.0f + espace * releaseMulDepth));

    if (family == mgs::Family::Acoustique)
    {
        s.spatial.stereoWidth = juce::jlimit(0.0f, 0.72f, s.spatial.stereoWidth);
        s.resonance.driveAmount = juce::jlimit(0.0f, 0.32f, s.resonance.driveAmount);
        s.envelope.releaseSeconds = juce::jlimit(0.01f, 1.8f, s.envelope.releaseSeconds);
    }
    else if (family == mgs::Family::Electrique)
    {
        s.spatial.stereoWidth = juce::jlimit(0.0f, 0.65f, s.spatial.stereoWidth);
        s.performance.attackBrightness = juce::jlimit(0.0f, 0.90f, s.performance.attackBrightness);
    }
    else if (family == mgs::Family::Electronique)
    {
        s.performance.pickPosition = clamp01(s.performance.pickPosition + brillance * 0.08f);
        s.envelope.releaseSeconds = juce::jlimit(0.05f, 5.0f, s.envelope.releaseSeconds);
    }
}

GuitarSynthAudioProcessor::PlayMode GuitarSynthAudioProcessor::getPlayMode() const noexcept
{
    const auto playModeValue = juce::jlimit(0, 2, static_cast<int>(std::round(getParamValue(kPlayMode))));
    return static_cast<PlayMode>(playModeValue);
}

float GuitarSynthAudioProcessor::getPalmMuteAmountForInstrument(int instrIndex) const noexcept
{
    if (mgs::getFamily(instrIndex) == mgs::Family::Electronique)
        return 0.0f;
    return clamp01(getParamValue(kPalmMute));
}

int GuitarSynthAudioProcessor::findFreeVoice() const
{
    for (int i = 0; i < kMaxVoices; ++i)
        if (!voices[static_cast<std::size_t>(i)].voice || !voices[static_cast<std::size_t>(i)].voice->isActive())
            return i;

    int oldest = 0;
    uint64_t oldestAge = UINT64_MAX;
    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voices[static_cast<std::size_t>(i)].voice && voices[static_cast<std::size_t>(i)].voice->isReleasing()
            && voices[static_cast<std::size_t>(i)].activationAge < oldestAge)
        {
            oldest = i;
            oldestAge = voices[static_cast<std::size_t>(i)].activationAge;
        }
    }
    if (oldestAge < UINT64_MAX) return oldest;

    oldestAge = UINT64_MAX;
    for (int i = 0; i < kMaxVoices; ++i)
    {
        if (voices[static_cast<std::size_t>(i)].activationAge < oldestAge)
        {
            oldest = i;
            oldestAge = voices[static_cast<std::size_t>(i)].activationAge;
        }
    }
    return oldest;
}

int GuitarSynthAudioProcessor::nextArpRandomInt(const int upperExclusive) noexcept
{
    if (upperExclusive <= 1)
        return 0;
    arpRandomState = arpRandomState * 1664525u + 1013904223u;
    return static_cast<int>((arpRandomState >> 1) % static_cast<std::uint32_t>(upperExclusive));
}

int GuitarSynthAudioProcessor::acquireVoicePoolSlot(int instrIndex)
{
    if (instrIndex < 0 || instrIndex >= mgs::kNumInstruments)
        return -1;

    for (int poolSlot = 0; poolSlot < kVoicePoolSize; ++poolSlot)
    {
        auto& inUse = voicePoolInUse[static_cast<std::size_t>(instrIndex)][static_cast<std::size_t>(poolSlot)];
        bool expected = false;
        if (inUse.compare_exchange_strong(expected, true, std::memory_order_acq_rel, std::memory_order_acquire))
            return poolSlot;
    }

    return -1;
}

void GuitarSynthAudioProcessor::clearVoice(VoiceSlot& slot)
{
    // Voice slots are audio-thread owned during processing. Lifecycle calls may
    // clear them only when the host has stopped the processor.
    if (slot.voice)
        slot.voice->reset();
    if (slot.poolSlot >= 0 && slot.instrIndex >= 0 && slot.instrIndex < mgs::kNumInstruments)
        voicePoolInUse[static_cast<std::size_t>(slot.instrIndex)][static_cast<std::size_t>(slot.poolSlot)]
            .store(false, std::memory_order_release);
    slot.voice = nullptr;
    slot.midiNote = -1;
    slot.instrIndex = 0;
    slot.poolSlot = -1;
    slot.midiChannel = 1;
    slot.outputBus = 0;
    slot.noteVelocity = 0.0f;
    slot.keyDown = false;
    slot.activationAge = 0;
}

void GuitarSynthAudioProcessor::clearDyingVoice(DyingVoiceSlot& slot)
{
    // Dying voice slots follow the same ownership contract as active voices.
    if (slot.voice)
        slot.voice->reset();
    if (slot.poolSlot >= 0 && slot.instrIndex >= 0 && slot.instrIndex < mgs::kNumInstruments)
        voicePoolInUse[static_cast<std::size_t>(slot.instrIndex)][static_cast<std::size_t>(slot.poolSlot)]
            .store(false, std::memory_order_release);
    slot.voice = nullptr;
    slot.instrIndex = 0;
    slot.poolSlot = -1;
    slot.outputBus = 0;
    slot.noteVelocity = 0.0f;
    slot.activationAge = 0;
}

void GuitarSynthAudioProcessor::releaseVoices(int midiChannel, bool immediate)
{
    for (auto& stack : legatoStacks) stack.clear();
    sustainPedalDown = false;
    strumState.removePendingNotes(midiChannel, -1);
    for (auto& slot : voices)
    {
        if (!slot.voice || !slot.voice->isActive())
            continue;
        if (midiChannel != 0 && slot.midiChannel != midiChannel)
            continue;

        slot.keyDown = false;
        if (immediate)
        {
            clearVoice(slot);
            continue;
        }

        if (!slot.voice->isReleasing())
            slot.voice->noteOff();
    }
}

void GuitarSynthAudioProcessor::panicAllVoices()
{
    resetRealtimeModulationState();
    for (auto& stack : legatoStacks) stack.clear();
    strumState.clearPending();
    for (auto& slot : dyingVoices)
        clearDyingVoice(slot);
    sustainPedalDown = false;
    for (auto& slot : voices)
        clearVoice(slot);
}

void GuitarSynthAudioProcessor::resetRealtimeModulationState() noexcept
{
    pitchBend.reset();
    modulationMatrix.resetMidiSources();
}

void GuitarSynthAudioProcessor::triggerNoteOn(int instrIndex, int midiChannel, int midiNote, float velocity,
                                              const InstrSnapshot& snapshot, const modmatrix::ModContext& baseModContext,
                                              const uint32_t chordHash)
{
    if (instrIndex < 0 || instrIndex >= mgs::kNumInstruments) return;
    if (preparedSampleRate <= 0.0) return;
    if (midiChannel < 1 || midiChannel > 16) return;

    const auto playMode = getPlayMode();
    const bool monoMode = playMode != PlayMode::Poly;
    const bool monoLegato = playMode == PlayMode::MonoLegato
        && mgs::getCharacteristics(instrIndex).legatoEnabled;

    bool repeatedNote = false;
    for (auto& slot : voices)
    {
        if (slot.voice && slot.voice->isActive() && !slot.voice->isReleasing()
            && slot.instrIndex == instrIndex && slot.midiChannel == midiChannel
            && slot.midiNote == midiNote)
        {
            repeatedNote = true;
            slot.keyDown = false;
            slot.voice->forceQuickRelease();
        }
    }

    // In mono legato, retarget the currently active voice instead of retriggering.
    if (monoLegato && !repeatedNote)
    {
        legatoStacks[static_cast<std::size_t>(instrIndex)].push(midiNote);

        for (auto& slot : voices)
        {
            if (slot.voice && slot.voice->isActive() && !slot.voice->isReleasing()
                && slot.instrIndex == instrIndex && slot.midiChannel == midiChannel)
            {
                slot.voice->setTargetNote(midiNote, preparedSampleRate);
                slot.midiNote = midiNote;
                slot.keyDown = true;
                slot.noteVelocity = velocity;
                slot.activationAge = ++voiceAgeCounter;
                return;
            }
        }
    }

    // Mono retrigger for non-legato instruments or when no eligible voice was found.
    if (monoMode)
    {
        for (auto& slot : voices)
        {
            if (!slot.voice || !slot.voice->isActive())
                continue;
            if (slot.instrIndex != instrIndex || slot.midiChannel != midiChannel)
                continue;

            clearVoice(slot);
        }
    }

    const int voiceIdx = findFreeVoice();
    auto& v = voices[static_cast<std::size_t>(voiceIdx)];

    // C1 – crossfade voice stealing: preserve active voices for a quick fade out
    if (v.voice && v.voice->isActive() && !v.voice->isReleasing())
    {
        v.voice->forceQuickRelease();
        const int targetBus = juce::jlimit(0, kNumAuxOutputs, v.outputBus);
        DyingVoiceSlot* dyingSlot = nullptr;
        for (auto& candidate : dyingVoices)
        {
        if (candidate.voice == nullptr)
        {
            dyingSlot = &candidate;
            break;
            }
        }
        if (dyingSlot == nullptr)
        {
            dyingSlot = &dyingVoices[0];
            for (auto& candidate : dyingVoices)
                if (candidate.activationAge < dyingSlot->activationAge)
                    dyingSlot = &candidate;
            clearDyingVoice(*dyingSlot);
        }

        dyingSlot->voice = v.voice;
        dyingSlot->instrIndex = v.instrIndex;
        dyingSlot->poolSlot = v.poolSlot;
        dyingSlot->outputBus = targetBus;
        dyingSlot->noteVelocity = v.noteVelocity;
        dyingSlot->activationAge = ++voiceAgeCounter;
        v.voice = nullptr;
    }
    else
    {
        clearVoice(v);
    }

    const int poolSlot = acquireVoicePoolSlot(instrIndex);
    jassert(poolSlot >= 0);
    if (poolSlot < 0)
        return;

    v.midiNote = midiNote;
    v.instrIndex = instrIndex;
    v.poolSlot = poolSlot;
    v.midiChannel = midiChannel;
    v.outputBus = snapshot.outputBus;
    v.noteVelocity = velocity;
    v.keyDown = true;
    v.activationAge = ++voiceAgeCounter;
    v.voice = voicePool[static_cast<std::size_t>(instrIndex)][static_cast<std::size_t>(poolSlot)].get();

    auto settings = snapshot.settings;
    auto voiceContext = baseModContext;
    voiceContext.velocity = velocity;
    const auto voiceModResult = modulationMatrix.process(voiceContext);
    settings.tone.lowPassHz = juce::jlimit(120.0f, 16000.0f, settings.tone.lowPassHz * voiceModResult.cutoffMul);
    settings.envelope.attackSeconds = juce::jlimit(0.0f, 2.0f, settings.envelope.attackSeconds * voiceModResult.attackScale);
    settings.envelope.decaySeconds = juce::jlimit(0.1f, 10.0f, settings.envelope.decaySeconds * voiceModResult.decayScale);
    settings.spatial.pan = juce::jlimit(-1.0f, 1.0f, settings.spatial.pan + voiceModResult.pan);
    settings.level = juce::jlimit(0.0f, 1.0f, settings.level * voiceModResult.levelMul);
    settings.resonance.bodyAmount = juce::jlimit(0.0f, 1.0f, settings.resonance.bodyAmount * std::clamp(1.0f + voiceModResult.resonance * 0.5f, 0.25f, 2.0f));
    if (repeatedNote)
    {
        const auto family = mgs::getFamily(instrIndex);
        const float palmMuteAmount = getPalmMuteAmountForInstrument(instrIndex);
        const float repeatAttackBoost = family == mgs::Family::Acoustique ? 0.16f
                                      : family == mgs::Family::Electrique ? 0.12f
                                                                          : 0.08f;
        const float attackScale = family == mgs::Family::Acoustique ? 0.55f
                                : family == mgs::Family::Electrique ? 0.62f
                                                                    : 0.78f;
        const float decayScale = palmMuteAmount > 0.001f ? 0.78f : 0.92f;
        settings.performance.attackBrightness = juce::jlimit(0.0f, 1.0f,
            settings.performance.attackBrightness + repeatAttackBoost * (1.0f - palmMuteAmount * 0.35f));
        settings.envelope.attackSeconds = juce::jlimit(0.0f, 2.0f, settings.envelope.attackSeconds * attackScale);
        settings.envelope.decaySeconds = juce::jlimit(0.1f, 10.0f, settings.envelope.decaySeconds * decayScale);
        if (palmMuteAmount > 0.001f)
        {
            settings.resonance.bodyAmount = juce::jlimit(0.0f, 1.0f, settings.resonance.bodyAmount * (1.0f - palmMuteAmount * 0.35f));
            settings.tone.lowPassHz = juce::jlimit(120.0f, 16000.0f, settings.tone.lowPassHz * (1.0f - palmMuteAmount * 0.22f));
        }
    }
    v.voice->noteOn(midiNote, velocity, preparedSampleRate, settings, chordHash);
    v.voice->setPitchBendFactor(pitchBend.pitchBendFactor);
    v.voice->setVoiceModulation({ 1.0f, voiceModResult.resonance, voiceModResult.pan, voiceModResult.levelMul,
                                  voiceModResult.pitchSemi, voiceModResult.attackScale, voiceModResult.decayScale },
                                preparedSampleRate);
    v.voice->setPalmMute(getPalmMuteAmountForInstrument(instrIndex));
}

void GuitarSynthAudioProcessor::triggerNoteOff(int midiChannel, int midiNote)
{
    strumState.removePendingNotes(midiChannel, midiNote);

    // B3 – remove from all per-instrument legato stacks
    for (auto& stack : legatoStacks)
        stack.removeLatest(midiNote);

    VoiceSlot* targetVoice = nullptr;
    uint64_t newestAge = 0;

    for (auto& slot : voices)
    {
        if (slot.voice && slot.voice->isActive() && !slot.voice->isReleasing()
            && slot.keyDown && slot.midiNote == midiNote
            && slot.midiChannel == midiChannel
            && slot.activationAge >= newestAge)
        {
            newestAge = slot.activationAge;
            targetVoice = &slot;
        }
    }

    if (targetVoice == nullptr)
        return;

    const int instrIdxOfTarget = targetVoice->instrIndex;
    const bool monoLegato = getPlayMode() == PlayMode::MonoLegato
        && mgs::getCharacteristics(instrIdxOfTarget).legatoEnabled;

    // B3 – legato: retarget to previous held note if stack not empty
    if (monoLegato && !legatoStacks[instrIdxOfTarget].empty())
    {
        const int prevNote = legatoStacks[instrIdxOfTarget].back();
        targetVoice->voice->setTargetNote(prevNote, preparedSampleRate);
        targetVoice->midiNote = prevNote;
        return;
    }

    targetVoice->keyDown = false;

    // A2 – sustain pedal: hold the voice active until pedal is released
    if (sustainPedalDown)
        return;

    targetVoice->voice->noteOff();
}

// =============================================================================
// FLkey Mini CC page system
// =============================================================================

namespace
{
    struct CCSlot {
        const char* paramId;        // global param, or nullptr for per-instrument
        const char* instrSuffix;    // per-instrument suffix (when paramId == nullptr)
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
        "CHORUS/LIMIT"  // 6
    };

    // 7 pages x 8 knobs
    static const CCSlot kCCPages[][kKnobsPerPage] = {
        // Page 0 - Macros & Master
        { { "macro_corps",      nullptr }, { "macro_brillance", nullptr },
          { "macro_gain",       nullptr }, { "macro_espace",    nullptr },
          { "lfo_rate",         nullptr }, { "lfo_depth",       nullptr },
          { "reverb_mix",       nullptr }, { "output_gain",     nullptr } },

        // Page 1 - Envelope (per-instrument)
        { { nullptr, "attack" },       { nullptr, "decay"  },
          { nullptr, "sustain" },      { nullptr, "release" },
          { nullptr, "body_amount" },  { nullptr, "level" },
          { nullptr, "tune" },         { nullptr, "string_brightness" } },

        // Page 2 - Tone (per-instrument + global)
        { { nullptr, "drive" },                 { nullptr, "attack_brightness" },
          { nullptr, "stereo_width" },          { nullptr, "pick_position" },
          { nullptr, "low_pass_hz" },           { nullptr, "pan" },
          { "sat_drive", nullptr },             { "sat_mix",   nullptr } },

        // Page 3 - Reverb & Delay
        { { "reverb_size",     nullptr }, { "reverb_damping",  nullptr },
          { "reverb_width",    nullptr }, { "reverb_mix",      nullptr },
          { "delay_time",      nullptr }, { "delay_feedback",  nullptr },
          { "delay_mix",       nullptr }, { "cab_mix",         nullptr } },

        // Page 4 - Dynamics (Compressor + Transient)
        { { "comp_threshold", nullptr }, { "comp_ratio",  nullptr },
          { "comp_attack",    nullptr }, { "comp_release", nullptr },
          { "comp_makeup",    nullptr }, { "comp_mix",     nullptr },
          { "transient_attack", nullptr }, { "transient_sustain", nullptr } },

        // Page 5 - EQ
        { { "eq_low_freq",  nullptr }, { "eq_low_gain",  nullptr },
          { "eq_mid_freq",  nullptr }, { "eq_mid_gain",  nullptr },
          { "eq_mid_q",     nullptr }, { "eq_high_freq", nullptr },
          { "eq_high_gain", nullptr }, { "transient_mix", nullptr } },

        // Page 6 - Chorus & Limiter
        { { "chorus_rate",       nullptr }, { "chorus_depth",      nullptr },
          { "chorus_delay",      nullptr }, { "chorus_mix",        nullptr },
          { "limiter_threshold", nullptr }, { "limiter_release",   nullptr },
          { "cab_mix",           nullptr }, { "output_gain",       nullptr } }
    };
}

const char* GuitarSynthAudioProcessor::getCCPageName(int page) noexcept
{
    if (page >= 0 && page < kNumCCPages)
        return kCCPageNames[page];
    return "???";
}

void GuitarSynthAudioProcessor::handleMidiCC(int ccNumber, int ccValue, int instrIndex)
{
    const float normalisedCc = static_cast<float>(ccValue) / 127.0f;

    // --- MIDI Learn intercept ---
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

    // --- Apply learned CC mappings before FLkey pages ---
    if (ccNumber >= 0 && ccNumber < static_cast<int>(midiLearnParamSnapshot.size()))
    {
        if (auto* param = midiLearnParamSnapshot[static_cast<std::size_t>(ccNumber)].load(std::memory_order_acquire))
        {
            queueParamUpdate(param, normalisedCc);
            return;
        }
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

    // --- CC 67: Palm mute (standard MIDI soft-pedal / hold-2) ---
    if (ccNumber == 67)
    {
        if (auto* param = parameters.getParameter(kPalmMute))
            queueParamUpdate(param, normalisedCc);
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
    else if (slot.instrSuffix != nullptr)
        paramId = makeInstrParamId(instrIndex, slot.instrSuffix);
    else
        return;

    if (auto* param = parameters.getParameter(paramId))
        queueParamUpdate(param, normalisedCc);
}

// =============================================================================
// RT-safe deferred parameter update — called from audio thread (handleMidiCC)
// =============================================================================
void GuitarSynthAudioProcessor::rebuildMidiLearnSnapshot()
{
    for (auto& slot : midiLearnParamSnapshot)
        slot.store(nullptr, std::memory_order_relaxed);

    for (const auto& [cc, paramId] : midiLearnMap)
    {
        if (cc >= 0 && cc < static_cast<int>(midiLearnParamSnapshot.size()))
            midiLearnParamSnapshot[static_cast<std::size_t>(cc)].store(parameters.getParameter(paramId), std::memory_order_release);
    }
}
void GuitarSynthAudioProcessor::queueParamUpdate(juce::RangedAudioParameter* param, float normalisedValue)
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
void GuitarSynthAudioProcessor::processMasterFxChain(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    processGlobalTransient(mainBuffer, blockState);
    processGlobalSaturator(mainBuffer, blockState);
    processGlobalEQ(mainBuffer, blockState);
    processGlobalCabinet(mainBuffer, blockState);
    processGlobalCompressor(mainBuffer, blockState);
    processGlobalChorus(mainBuffer, blockState);
    applyGlobalLfo(mainBuffer, blockState);
    processGlobalDelay(mainBuffer, blockState);
    processGlobalReverb(mainBuffer, blockState);

    const float targetGain = juce::Decibels::decibelsToGain(blockState.outputGainDb);
    const int numSamples = mainBuffer.getNumSamples();
    const float gainStep = numSamples > 0 ? (targetGain - outputGainCurrent) / static_cast<float>(numSamples) : 0.0f;
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
    processGlobalLimiter(mainBuffer, blockState);
}

void GuitarSynthAudioProcessor::updateGlobalEffectParameters(const GlobalBlockState& blockState)
{
    const auto threshold = blockState.fx.compThreshold;
    const auto ratio     = blockState.fx.compRatio;
    const auto attack    = blockState.fx.compAttack;
    const auto release   = blockState.fx.compRelease;

    if (threshold != compCache.threshold) { compressor.setThreshold(threshold); compCache.threshold = threshold; }
    if (ratio     != compCache.ratio)     { compressor.setRatio(ratio);          compCache.ratio     = ratio; }
    if (attack    != compCache.attack)    { compressor.setAttack(attack);         compCache.attack    = attack; }
    if (release   != compCache.release)   { compressor.setRelease(release);       compCache.release   = release; }
}

void GuitarSynthAudioProcessor::processGlobalTransient(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    if (!mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Transient)
        || !blockState.fx.transientOn)
        return;
    const auto mix     = clamp01(blockState.fx.transientMix);
    const auto attack  = juce::jlimit(-1.0f, 1.0f, blockState.fx.transientAttack);
    const auto sustain = juce::jlimit(-1.0f, 1.0f, blockState.fx.transientSustain);

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
            const auto g = juce::jlimit(0.2f, 4.0f,
                1.0f + attack * std::max(0.0f, transient) * 7.0f
                     + sustain * std::max(0.0f, -transient) * 5.0f);
            data[i] = dry + (dry * g - dry) * mix;
        }
    }
}

void GuitarSynthAudioProcessor::processGlobalSaturator(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    if (!mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Saturator) && satMixCurrent <= 0.0001f)
        return;
    const bool effectActive = mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Saturator)
        && blockState.fx.saturatorOn;
    const auto targetMix = effectActive ? clamp01(blockState.fx.satMix) : 0.0f;
    const auto targetDrive = juce::jlimit(1.0f, 16.0f, blockState.fx.satDrive);
    if (targetMix <= 0.0001f && satMixCurrent <= 0.0001f)
        return;

    const int numSamples = mainBuffer.getNumSamples();
    const int numChannels = mainBuffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;
    const float mixStep = (targetMix - satMixCurrent) / static_cast<float>(numSamples);
    const float driveStep = (targetDrive - satDriveCurrent) / static_cast<float>(numSamples);

    if (blockState.qualityMode == static_cast<int>(QualityMode::Studio))
    {
        if (satDryBuffer.getNumChannels() < numChannels || satDryBuffer.getNumSamples() < numSamples)
        {
            jassertfalse;
            return;
        }

        for (int ch = 0; ch < numChannels; ++ch)
            satDryBuffer.copyFrom(ch, 0, mainBuffer, ch, 0, numSamples);

        juce::dsp::AudioBlock<float> block(mainBuffer);
        auto& oversampler = numChannels == 1 ? satOversamplingMono : satOversamplingStereo;
        auto osBlock = oversampler.processSamplesUp(block);
        const float norm = 1.0f / std::max(0.0001f, std::tanh(targetDrive));
        for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
        {
            auto* data = osBlock.getChannelPointer(ch);
            for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
                data[i] = std::tanh(data[i] * targetDrive) * norm;
        }
        oversampler.processSamplesDown(block);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* wet = mainBuffer.getWritePointer(ch);
            const auto* dry = satDryBuffer.getReadPointer(ch);
            float mix = satMixCurrent;
            for (int i = 0; i < numSamples; ++i)
            {
                wet[i] = dry[i] + (wet[i] - dry[i]) * mix;
                mix += mixStep;
            }
            saturatorPrevInput[static_cast<std::size_t>(juce::jlimit(0, 1, ch))] = dry[numSamples - 1] * targetDrive;
        }

        satDriveCurrent = targetDrive;
        satMixCurrent = targetMix;
        return;
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = mainBuffer.getWritePointer(ch);
        auto& prevInput = saturatorPrevInput[static_cast<std::size_t>(juce::jlimit(0, 1, ch))];
        float drive = satDriveCurrent;
        float mix = satMixCurrent;
        for (int i = 0; i < numSamples; ++i)
        {
            const auto norm = 1.0f / std::max(0.0001f, std::tanh(drive));
            const auto dry = data[i];
            const auto driven = dry * drive;
            const float wet = mgs::fx::adaaTanh(driven, prevInput) * norm;
            data[i] = dry + (wet - dry) * mix;
            prevInput = driven;
            drive += driveStep;
            mix += mixStep;
        }
    }

    satDriveCurrent = targetDrive;
    satMixCurrent = targetMix;
}

void GuitarSynthAudioProcessor::processGlobalCabinet(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    if (!mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Cabinet)
        || !blockState.fx.cabinetOn)
        return;

    const auto mix = clamp01(blockState.fx.cabMix);
    if (mix <= 0.0001f) return;

    const auto family = mgs::getFamily(blockState.selectedInstrIndex);
    const auto chars = mgs::getCharacteristics(blockState.selectedInstrIndex);
    mgs::fx::CabinetSim::Params p;
    p.mix = mix;
    if (family == mgs::Family::Electrique)
    {
        switch (blockState.selectedInstrIndex)
        {
            case 3: // Clean
                p.highPassHz = 72.0f;
                p.bodyHz = 330.0f;
                p.bodyGainDb = 1.8f;
                p.bodyQ = 0.75f;
                p.presenceHz = 2800.0f;
                p.presenceGainDb = 1.6f;
                p.presenceQ = 1.0f;
                p.lowPassHz = 6100.0f;
                p.lowPassQ = 0.70f;
                break;
            case 4: // Crunch
                p.highPassHz = 86.0f;
                p.bodyHz = 420.0f;
                p.bodyGainDb = 3.2f;
                p.bodyQ = 0.95f;
                p.presenceHz = 2400.0f;
                p.presenceGainDb = 2.6f;
                p.presenceQ = 1.15f;
                p.lowPassHz = 4950.0f;
                p.lowPassQ = 0.78f;
                break;
            case 5: // Lead
            default:
                p.highPassHz = 98.0f;
                p.bodyHz = 470.0f;
                p.bodyGainDb = 2.4f;
                p.bodyQ = 0.90f;
                p.presenceHz = 3000.0f;
                p.presenceGainDb = 3.3f;
                p.presenceQ = 1.25f;
                p.lowPassHz = 4300.0f;
                p.lowPassQ = 0.82f;
                break;
        }
    }
    else
    {
        p.highPassHz = 75.0f + chars.output.drive * 20.0f;
        p.bodyHz = 360.0f;
        p.bodyGainDb = 2.0f + chars.body.resonance * 2.0f;
        p.bodyQ = 0.8f;
        p.presenceHz = 2500.0f;
        p.presenceGainDb = 1.8f;
        p.presenceQ = 1.1f;
        p.lowPassHz = 5200.0f - chars.output.drive * 900.0f;
        p.lowPassQ = 0.74f;
    }

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() >= 2 ? mainBuffer.getWritePointer(1) : nullptr;
    cabSim.process(left, right, mainBuffer.getNumSamples(), p);
}

void GuitarSynthAudioProcessor::processGlobalCompressor(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    if (!mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Compressor)
        || !blockState.fx.compressorOn)
        return;
    const auto mix = clamp01(blockState.fx.compMix);
    const auto makeupGain = juce::Decibels::decibelsToGain(blockState.fx.compMakeup);

    if (mix <= 0.0001f)
        return;

    updateGlobalEffectParameters(blockState);
    if (compDryBuffer.getNumChannels() < mainBuffer.getNumChannels()
        || compDryBuffer.getNumSamples() < mainBuffer.getNumSamples())
    {
        jassertfalse;
        return;
    }
    for (int ch = 0; ch < mainBuffer.getNumChannels(); ++ch)
        compDryBuffer.copyFrom(ch, 0, mainBuffer, ch, 0, mainBuffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(mainBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);
    mainBuffer.applyGain(makeupGain);

    if (mix < 0.9999f)
    {
        for (int ch = 0; ch < mainBuffer.getNumChannels(); ++ch)
        {
            auto* wet = mainBuffer.getWritePointer(ch);
            const auto* dry = compDryBuffer.getReadPointer(ch);
            for (int i = 0; i < mainBuffer.getNumSamples(); ++i)
                wet[i] = dry[i] + (wet[i] - dry[i]) * mix;
        }
    }
}

void GuitarSynthAudioProcessor::applyGlobalLfo(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    const auto numCh = mainBuffer.getNumChannels();
    const auto numSamples = mainBuffer.getNumSamples();
    if (numCh <= 0 || numSamples <= 0) return;

    const float rateHz = juce::jlimit(0.05f, 12.0f, blockState.lfoRate)
                       * cachedModResult.lfo1RateMul;
    const float targetDepth = clamp01(blockState.lfoDepth);
    if (targetDepth <= 0.0001f && lfoDepthCurrent <= 0.0001f) return;

    const int wave = juce::jlimit(0, 3, blockState.lfoWave);
    constexpr float kTremDepth = 0.70f;
    constexpr float kPanDepth  = 0.20f;

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = numCh > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    const float rateStep = numSamples > 0 ? (rateHz - lfoRateCurrent) / static_cast<float>(numSamples) : 0.0f;
    const float depthStep = numSamples > 0 ? (targetDepth - lfoDepthCurrent) / static_cast<float>(numSamples) : 0.0f;
    float rateCurrent = lfoRateCurrent;
    float depthCurrent = lfoDepthCurrent;

    for (int i = 0; i < numSamples; ++i)
    {
        float lfo = 0.0f;
        switch (wave)
        {
            case 1: lfo = 1.0f - 4.0f * std::abs(lfoPhase - 0.5f); break;
            case 2: lfo = lfoPhase * 2.0f - 1.0f; break;
            case 3: lfo = lfoPhase < 0.5f ? 1.0f : -1.0f; break;
            default: lfo = mgs::fastSin(lfoPhase); break;
        }

        const float tremAmt = depthCurrent * kTremDepth;
        const float trem = 1.0f - tremAmt * 0.5f + lfo * tremAmt * 0.5f;

        if (right != nullptr)
        {
            const float pan = lfo * depthCurrent * kPanDepth;
            const float gL = std::sqrt(0.5f * (1.0f - pan)) * trem;
            const float gR = std::sqrt(0.5f * (1.0f + pan)) * trem;
            left[i]  *= gL;
            right[i] *= gR;
        }
        else
        {
            left[i] *= trem;
        }

        const float phaseInc = rateCurrent / static_cast<float>(juce::jmax(1.0, preparedSampleRate));
        lfoPhase += phaseInc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        rateCurrent += rateStep;
        depthCurrent += depthStep;
    }

    lfoRateCurrent = rateHz;
    lfoDepthCurrent = targetDepth;
}

void GuitarSynthAudioProcessor::processGlobalChorus(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    const bool effectActive = mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Chorus)
        && blockState.fx.chorusOn;

    mgs::fx::StereoChorus::Params cp;
    cp.rateHz = juce::jlimit(0.1f, 8.0f, blockState.fx.chorusRate);
    cp.depth  = clamp01(blockState.fx.chorusDepth);
    cp.mix    = effectActive ? clamp01(blockState.fx.chorusMix) : 0.0f;

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    chorus.process(left, right, mainBuffer.getNumSamples(), cp);
}

void GuitarSynthAudioProcessor::processGlobalReverb(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    const bool effectActive = mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Reverb)
        && blockState.fx.reverbOn;

    const int reverbType = static_cast<int>(std::round(getParamValue("reverb_type")));
    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    const int numSamples = mainBuffer.getNumSamples();

    if (reverbType >= 1 && effectActive)
    {
        // Convolution reverb (Hall / Room / Chamber)
        convReverb.setType(reverbType,
                           blockState.fx.reverbSize,
                           blockState.fx.reverbDamping,
                           blockState.fx.reverbWidth);
        const float mix = clamp01(blockState.fx.reverbMix);
        if (reverbWetBuffer.getNumChannels() < mainBuffer.getNumChannels()
            || reverbWetBuffer.getNumSamples() < numSamples)
        {
            return;
        }

        reverbWetBuffer.copyFrom(0, 0, left, numSamples);
        auto* wetL = reverbWetBuffer.getWritePointer(0);
        float* wetR = wetL;
        if (right != nullptr && reverbWetBuffer.getNumChannels() > 1)
        {
            reverbWetBuffer.copyFrom(1, 0, right, numSamples);
            wetR = reverbWetBuffer.getWritePointer(1);
        }

        convReverb.processBlock(wetL, wetR, numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            left[i]  = left[i]  * (1.0f - mix) + wetL[i] * mix;
            if (right) right[i] = right[i] * (1.0f - mix) + wetR[i] * mix;
        }
    }
    else
    {
        // Dattorro Plate reverb (type == 0 or not active)
        mgs::fx::DattorroPlateReverb::Params rp;
        rp.decay   = clamp01(blockState.fx.reverbSize);
        rp.damping = clamp01(blockState.fx.reverbDamping);
        rp.width   = clamp01(blockState.fx.reverbWidth);
        rp.mix     = effectActive ? clamp01(blockState.fx.reverbMix) : 0.0f;
        reverb.process(left, right, numSamples, rp);
    }
}

void GuitarSynthAudioProcessor::processGlobalEQ(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    if (!mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Eq)
        || !blockState.fx.eqOn)
        return;

    mgs::fx::ParametricEQ3Band::Params ep;
    ep.lowFreq   = blockState.fx.eqLowFreq;
    ep.lowGainDb = blockState.fx.eqLowGain;
    ep.midFreq   = juce::jlimit(200.0f, 8000.0f,
                                blockState.fx.eqMidFreq * std::exp2(cachedModResult.eqMidFreqAdd));
    ep.midGainDb = juce::jlimit(-12.0f, 12.0f,
                                blockState.fx.eqMidGain + cachedModResult.eqMidGainAdd);
    ep.midQ      = blockState.fx.eqMidQ;
    ep.highFreq  = blockState.fx.eqHighFreq;
    ep.highGainDb = blockState.fx.eqHighGain;

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    eq.process(left, right, mainBuffer.getNumSamples(), ep);
}

void GuitarSynthAudioProcessor::processGlobalDelay(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    const bool effectActive = mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Delay)
        && blockState.fx.delayOn;

    mgs::fx::StereoDelay::Params dp;
    dp.timeMs   = blockState.fx.delayTime;
    dp.feedback = blockState.fx.delayFeedback;
    dp.mix      = effectActive ? clamp01(blockState.fx.delayMix) : 0.0f;
    dp.syncToBpm = blockState.delaySyncToHost;
    dp.bpm = blockState.hostBpm;
    dp.noteDiv = blockState.delayDivision;

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    stereoDelay.process(left, right, mainBuffer.getNumSamples(), dp);
}

void GuitarSynthAudioProcessor::processGlobalLimiter(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState)
{
    if (!mgs::isFxAvailable(blockState.selectedInstrIndex, mgs::GlobalFxSlot::Limiter)
        || !blockState.fx.limiterOn)
        return;

    mgs::fx::OutputLimiter::Params lp;
    lp.thresholdDb = blockState.fx.limiterThreshold;
    lp.releaseMs   = blockState.fx.limiterRelease;

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() > 1 ? mainBuffer.getWritePointer(1) : nullptr;
    limiter.process(left, right, mainBuffer.getNumSamples(), lp);
}

// =============================================================================
mgs::GlobalFxSettings GuitarSynthAudioProcessor::sanitizeFxSettings(const mgs::GlobalFxSettings& fx) const
{
    auto sanitized = fx;
    sanitized.satDrive = juce::jlimit(1.0f, 16.0f, sanitized.satDrive);
    sanitized.satMix = clamp01(sanitized.satMix);
    sanitized.transientAttack = juce::jlimit(-1.0f, 1.0f, sanitized.transientAttack);
    sanitized.transientSustain = juce::jlimit(-1.0f, 1.0f, sanitized.transientSustain);
    sanitized.transientMix = clamp01(sanitized.transientMix);
    sanitized.eqLowFreq = juce::jlimit(40.0f, 800.0f, sanitized.eqLowFreq);
    sanitized.eqLowGain = juce::jlimit(-12.0f, 12.0f, sanitized.eqLowGain);
    sanitized.eqMidFreq = juce::jlimit(200.0f, 8000.0f, sanitized.eqMidFreq);
    sanitized.eqMidGain = juce::jlimit(-12.0f, 12.0f, sanitized.eqMidGain);
    sanitized.eqMidQ = juce::jlimit(0.1f, 10.0f, sanitized.eqMidQ);
    sanitized.eqHighFreq = juce::jlimit(1000.0f, 16000.0f, sanitized.eqHighFreq);
    sanitized.eqHighGain = juce::jlimit(-12.0f, 12.0f, sanitized.eqHighGain);
    sanitized.compThreshold = juce::jlimit(-60.0f, 0.0f, sanitized.compThreshold);
    sanitized.compRatio = juce::jlimit(1.0f, 20.0f, sanitized.compRatio);
    sanitized.compAttack = juce::jlimit(0.1f, 100.0f, sanitized.compAttack);
    sanitized.compRelease = juce::jlimit(5.0f, 500.0f, sanitized.compRelease);
    sanitized.compMakeup = juce::jlimit(0.0f, 24.0f, sanitized.compMakeup);
    sanitized.compMix = clamp01(sanitized.compMix);
    sanitized.chorusRate = juce::jlimit(0.1f, 8.0f, sanitized.chorusRate);
    sanitized.chorusDepth = clamp01(sanitized.chorusDepth);
    sanitized.chorusDelay = juce::jlimit(1.0f, 30.0f, sanitized.chorusDelay);
    sanitized.chorusMix = clamp01(sanitized.chorusMix);
    sanitized.delayTime = juce::jlimit(10.0f, 1500.0f, sanitized.delayTime);
    sanitized.delayFeedback = juce::jlimit(0.0f, 0.95f, sanitized.delayFeedback);
    sanitized.delayMix = clamp01(sanitized.delayMix);
    sanitized.reverbSize = clamp01(sanitized.reverbSize);
    sanitized.reverbDamping = clamp01(sanitized.reverbDamping);
    sanitized.reverbWidth = clamp01(sanitized.reverbWidth);
    sanitized.reverbMix = clamp01(sanitized.reverbMix);
    sanitized.limiterThreshold = juce::jlimit(-12.0f, 0.0f, sanitized.limiterThreshold);
    sanitized.limiterRelease = juce::jlimit(5.0f, 200.0f, sanitized.limiterRelease);
    sanitized.cabMix = clamp01(sanitized.cabMix);
    return sanitized;
}

mgs::GlobalFxSettings GuitarSynthAudioProcessor::sanitizeFxSettingsForInstrument(int instrIndex,
                                                                                 const mgs::GlobalFxSettings& fx) const
{
    auto sanitized = sanitizeFxSettings(fx);
    const auto family = mgs::getFamily(instrIndex);

    switch (family)
    {
        case mgs::Family::Acoustique:
            sanitized.satMix = juce::jmin(sanitized.satMix, 0.18f);
            sanitized.transientMix = juce::jmin(sanitized.transientMix, 0.45f);
            sanitized.compMix = juce::jmin(sanitized.compMix, 0.82f);
            sanitized.chorusMix = juce::jmin(sanitized.chorusMix, 0.18f);
            sanitized.delayMix = juce::jmin(sanitized.delayMix, 0.26f);
            sanitized.delayFeedback = juce::jmin(sanitized.delayFeedback, 0.58f);
            sanitized.reverbMix = juce::jmin(sanitized.reverbMix, 0.30f);
            sanitized.cabMix = 0.0f;
            sanitized.cabinetOn = false;
            break;

        case mgs::Family::Electrique:
            sanitized.transientMix = juce::jmin(sanitized.transientMix, 0.55f);
            sanitized.compMix = juce::jmin(sanitized.compMix, 0.92f);
            switch (instrIndex)
            {
                case 3: // Clean
                    sanitized.satMix = juce::jmin(sanitized.satMix, 0.28f);
                    sanitized.chorusMix = juce::jmin(sanitized.chorusMix, 0.32f);
                    sanitized.delayMix = juce::jmin(sanitized.delayMix, 0.24f);
                    sanitized.delayFeedback = juce::jmin(sanitized.delayFeedback, 0.52f);
                    sanitized.reverbMix = juce::jmin(sanitized.reverbMix, 0.24f);
                    sanitized.cabMix = juce::jmin(sanitized.cabMix, 0.42f);
                    break;
                case 4: // Crunch
                    sanitized.satMix = juce::jmin(sanitized.satMix, 0.46f);
                    sanitized.chorusMix = juce::jmin(sanitized.chorusMix, 0.18f);
                    sanitized.delayMix = juce::jmin(sanitized.delayMix, 0.22f);
                    sanitized.delayFeedback = juce::jmin(sanitized.delayFeedback, 0.48f);
                    sanitized.reverbMix = juce::jmin(sanitized.reverbMix, 0.20f);
                    sanitized.cabMix = juce::jmin(sanitized.cabMix, 0.72f);
                    break;
                case 5: // Lead
                default:
                    sanitized.satMix = juce::jmin(sanitized.satMix, 0.58f);
                    sanitized.chorusMix = juce::jmin(sanitized.chorusMix, 0.22f);
                    sanitized.delayMix = juce::jmin(sanitized.delayMix, 0.30f);
                    sanitized.delayFeedback = juce::jmin(sanitized.delayFeedback, 0.56f);
                    sanitized.reverbMix = juce::jmin(sanitized.reverbMix, 0.22f);
                    sanitized.cabMix = juce::jmin(sanitized.cabMix, 0.84f);
                    break;
            }
            break;

        case mgs::Family::Electronique:
            sanitized.satMix = juce::jmin(sanitized.satMix, 0.32f);
            sanitized.transientMix = juce::jmin(sanitized.transientMix, 0.65f);
            sanitized.compMix = juce::jmin(sanitized.compMix, 1.0f);
            sanitized.chorusMix = juce::jmin(sanitized.chorusMix, 0.44f);
            sanitized.delayMix = juce::jmin(sanitized.delayMix, 0.42f);
            sanitized.delayFeedback = juce::jmin(sanitized.delayFeedback, 0.72f);
            sanitized.reverbMix = juce::jmin(sanitized.reverbMix, 0.40f);
            sanitized.cabMix = 0.0f;
            sanitized.cabinetOn = false;
            break;
    }

    return sanitized;
}

mgs::GlobalFxSettings GuitarSynthAudioProcessor::snapshotFxSettings() const
{
    mgs::GlobalFxSettings fx;
    fx.satDrive         = getParamValue(kSatDrive);
    fx.satMix           = getParamValue(kSatMix);
    fx.saturatorOn      = getParamValue(kFxSatEnable) >= 0.5f;
    fx.transientAttack  = getParamValue(kTransientAttack);
    fx.transientSustain = getParamValue(kTransientSustain);
    fx.transientMix     = getParamValue(kTransientMix);
    fx.transientOn      = getParamValue(kFxTransientEnable) >= 0.5f;
    fx.eqLowFreq        = getParamValue(kEqLowFreq);
    fx.eqLowGain        = getParamValue(kEqLowGain);
    fx.eqMidFreq         = getParamValue(kEqMidFreq);
    fx.eqMidGain         = getParamValue(kEqMidGain);
    fx.eqMidQ            = getParamValue(kEqMidQ);
    fx.eqHighFreq        = getParamValue(kEqHighFreq);
    fx.eqHighGain        = getParamValue(kEqHighGain);
    fx.eqOn             = getParamValue(kFxEqEnable) >= 0.5f;
    fx.compThreshold     = getParamValue(kCompThreshold);
    fx.compRatio         = getParamValue(kCompRatio);
    fx.compAttack        = getParamValue(kCompAttack);
    fx.compRelease       = getParamValue(kCompRelease);
    fx.compMakeup        = getParamValue(kCompMakeup);
    fx.compMix           = getParamValue(kCompMix);
    fx.compressorOn      = getParamValue(kFxCompEnable) >= 0.5f;
    fx.chorusRate        = getParamValue(kChorusRate);
    fx.chorusDepth       = getParamValue(kChorusDepth);
    fx.chorusDelay       = getParamValue(kChorusDelay);
    fx.chorusMix         = getParamValue(kChorusMix);
    fx.chorusOn          = getParamValue(kFxChorusEnable) >= 0.5f;
    fx.delayTime         = getParamValue(kDelayTime);
    fx.delayFeedback     = getParamValue(kDelayFeedback);
    fx.delayMix          = getParamValue(kDelayMix);
    fx.delayOn           = getParamValue(kFxDelayEnable) >= 0.5f;
    fx.reverbSize        = getParamValue(kReverbSize);
    fx.reverbDamping     = getParamValue(kReverbDamping);
    fx.reverbWidth       = getParamValue(kReverbWidth);
    fx.reverbMix         = getParamValue(kReverbMix);
    fx.reverbOn          = getParamValue(kFxReverbEnable) >= 0.5f;
    fx.limiterThreshold  = getParamValue(kLimiterThreshold);
    fx.limiterRelease    = getParamValue(kLimiterRelease);
    fx.limiterOn         = getParamValue(kFxLimiterEnable) >= 0.5f;
    fx.cabMix            = getParamValue(kCabMix);
    fx.cabinetOn         = getParamValue(kFxCabinetEnable) >= 0.5f;
    return sanitizeFxSettingsForInstrument(getSelectedInstrIndex(), fx);
}

void GuitarSynthAudioProcessor::applyFxToParams(int instrIndex,
                                                const mgs::GlobalFxSettings& fx,
                                                bool notifyHost)
{
    const auto masked = sanitizeFxSettingsForInstrument(instrIndex, mgs::maskUnavailableFx(instrIndex, fx));

    setParamValueInternal(kSatDrive, masked.satDrive, notifyHost);
    setParamValueInternal(kSatMix, masked.satMix, notifyHost);
    setParamValueInternal(kTransientAttack, masked.transientAttack, notifyHost);
    setParamValueInternal(kTransientSustain, masked.transientSustain, notifyHost);
    setParamValueInternal(kTransientMix, masked.transientMix, notifyHost);
    setParamValueInternal(kEqLowFreq, masked.eqLowFreq, notifyHost);
    setParamValueInternal(kEqLowGain, masked.eqLowGain, notifyHost);
    setParamValueInternal(kEqMidFreq, masked.eqMidFreq, notifyHost);
    setParamValueInternal(kEqMidGain, masked.eqMidGain, notifyHost);
    setParamValueInternal(kEqMidQ, masked.eqMidQ, notifyHost);
    setParamValueInternal(kEqHighFreq, masked.eqHighFreq, notifyHost);
    setParamValueInternal(kEqHighGain, masked.eqHighGain, notifyHost);
    setParamValueInternal(kCompThreshold, masked.compThreshold, notifyHost);
    setParamValueInternal(kCompRatio, masked.compRatio, notifyHost);
    setParamValueInternal(kCompAttack, masked.compAttack, notifyHost);
    setParamValueInternal(kCompRelease, masked.compRelease, notifyHost);
    setParamValueInternal(kCompMakeup, masked.compMakeup, notifyHost);
    setParamValueInternal(kCompMix, masked.compMix, notifyHost);
    setParamValueInternal(kChorusRate, masked.chorusRate, notifyHost);
    setParamValueInternal(kChorusDepth, masked.chorusDepth, notifyHost);
    setParamValueInternal(kChorusDelay, masked.chorusDelay, notifyHost);
    setParamValueInternal(kChorusMix, masked.chorusMix, notifyHost);
    setParamValueInternal(kDelayTime, masked.delayTime, notifyHost);
    setParamValueInternal(kDelayFeedback, masked.delayFeedback, notifyHost);
    setParamValueInternal(kDelayMix, masked.delayMix, notifyHost);
    setParamValueInternal(kReverbSize, masked.reverbSize, notifyHost);
    setParamValueInternal(kReverbDamping, masked.reverbDamping, notifyHost);
    setParamValueInternal(kReverbWidth, masked.reverbWidth, notifyHost);
    setParamValueInternal(kReverbMix, masked.reverbMix, notifyHost);
    setParamValueInternal(kLimiterThreshold, masked.limiterThreshold, notifyHost);
    setParamValueInternal(kLimiterRelease, masked.limiterRelease, notifyHost);
    setParamValueInternal(kCabMix, masked.cabMix, notifyHost);

    setParamValueInternal(kFxSatEnable, masked.saturatorOn ? 1.0f : 0.0f, notifyHost);
    setParamValueInternal(kFxTransientEnable, masked.transientOn ? 1.0f : 0.0f, notifyHost);
    setParamValueInternal(kFxCompEnable, masked.compressorOn ? 1.0f : 0.0f, notifyHost);
    setParamValueInternal(kFxReverbEnable, masked.reverbOn ? 1.0f : 0.0f, notifyHost);
    setParamValueInternal(kFxEqEnable, masked.eqOn ? 1.0f : 0.0f, notifyHost);
    setParamValueInternal(kFxChorusEnable, masked.chorusOn ? 1.0f : 0.0f, notifyHost);
    setParamValueInternal(kFxDelayEnable, masked.delayOn ? 1.0f : 0.0f, notifyHost);
    setParamValueInternal(kFxLimiterEnable, masked.limiterOn ? 1.0f : 0.0f, notifyHost);
    setParamValueInternal(kFxCabinetEnable, masked.cabinetOn ? 1.0f : 0.0f, notifyHost);
}

void GuitarSynthAudioProcessor::storeCurrentInstrumentFxState(int instrIndex)
{
    if (instrIndex < 0 || instrIndex >= mgs::kNumInstruments)
        return;

    instrumentFxStates[static_cast<std::size_t>(instrIndex)] = snapshotFxSettings();
}

void GuitarSynthAudioProcessor::restoreInstrumentFxState(int instrIndex, bool notifyHost)
{
    if (instrIndex < 0 || instrIndex >= mgs::kNumInstruments)
        return;

    applyFxToParams(instrIndex, instrumentFxStates[static_cast<std::size_t>(instrIndex)], notifyHost);
}

bool GuitarSynthAudioProcessor::isFxAvailableForCurrentInstrument(mgs::GlobalFxSlot slot) const
{
    return mgs::isFxAvailable(getSelectedInstrIndex(), slot);
}

void GuitarSynthAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == kSelectedInstr)
    {
        pendingSelectedInstrumentIndex.store(juce::jlimit(
            0, mgs::kNumInstruments - 1, static_cast<int>(std::round(newValue))));
        triggerAsyncUpdate();
        return;
    }

    if (parameterID == "velocity_curve")
    {
        velocityCurve = intToVelocityCurve(static_cast<int>(std::round(newValue)));
        return;
    }

    if (parameterID == "pitch_bend_range")
    {
        pitchBend.bendSemitones = juce::jlimit(1.0f, 24.0f, newValue);
        pitchBend.updateFactor();
        return;
    }
}

void GuitarSynthAudioProcessor::handleAsyncUpdate()
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

    // --- Deferred instrument FX restore ---
    const int newInstrumentIndex = pendingSelectedInstrumentIndex.load();
    if (newInstrumentIndex == cachedSelectedInstrumentIndex)
        return;

    storeCurrentInstrumentFxState(cachedSelectedInstrumentIndex);
    cachedSelectedInstrumentIndex = newInstrumentIndex;

    if (getParamValue("fx_lock") < 0.5f)
        restoreInstrumentFxState(newInstrumentIndex, false);
}

void GuitarSynthAudioProcessor::flushPendingAsyncUpdatesForTests()
{
    handleAsyncUpdate();
}
// =============================================================================
// MIDI Learn
// =============================================================================
void GuitarSynthAudioProcessor::midiLearnArm(const juce::String& paramId)
{
    juce::ScopedLock sl(midiLearnLock);
    auto* param = parameters.getParameter(paramId);
    midiLearnArmedParamId = param != nullptr ? paramId : juce::String{};
    midiLearnArmedParam.store(param, std::memory_order_release);
    midiLearnArmed.store(param != nullptr, std::memory_order_release);
}

void GuitarSynthAudioProcessor::midiLearnClear(int ccNumber)
{
    juce::ScopedLock sl(midiLearnLock);
    midiLearnMap.erase(ccNumber);
    rebuildMidiLearnSnapshot();
}

void GuitarSynthAudioProcessor::midiLearnClearAll()
{
    juce::ScopedLock sl(midiLearnLock);
    midiLearnMap.clear();
    rebuildMidiLearnSnapshot();
    midiLearnArmed.store(false, std::memory_order_release);
    midiLearnArmedParam.store(nullptr, std::memory_order_release);
    midiLearnArmedParamId = {};
}

std::vector<std::pair<int, juce::String>> GuitarSynthAudioProcessor::getMidiLearnMappings() const
{
    juce::ScopedLock sl(midiLearnLock);
    std::vector<std::pair<int, juce::String>> result;
    result.reserve(midiLearnMap.size());
    for (const auto& [cc, paramId] : midiLearnMap)
        result.emplace_back(cc, paramId);
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    return result;
}

void GuitarSynthAudioProcessor::saveMidiLearnToXml(juce::XmlElement& xml) const
{
    juce::ScopedLock sl(midiLearnLock);
    if (midiLearnMap.empty())
        return;
    auto* mlNode = xml.createNewChildElement("MidiLearn");
    for (const auto& [cc, paramId] : midiLearnMap)
    {
        auto* mapNode = mlNode->createNewChildElement("Map");
        mapNode->setAttribute("cc", cc);
        mapNode->setAttribute("param", paramId);
    }
}

void GuitarSynthAudioProcessor::loadMidiLearnFromXml(const juce::XmlElement& xml)
{
    juce::ScopedLock sl(midiLearnLock);
    midiLearnMap.clear();
    const auto* mlNode = xml.getChildByName("MidiLearn");
    if (mlNode == nullptr)
    {
        rebuildMidiLearnSnapshot();
        return;
    }
    for (int i = 0; i < mlNode->getNumChildElements(); ++i)
    {
        const auto* mapNode = mlNode->getChildElement(i);
        if (mapNode && mapNode->hasAttribute("cc") && mapNode->hasAttribute("param"))
            midiLearnMap[mapNode->getIntAttribute("cc")] = mapNode->getStringAttribute("param");
    }
    rebuildMidiLearnSnapshot();
}

// =============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarSynthAudioProcessor();
}
