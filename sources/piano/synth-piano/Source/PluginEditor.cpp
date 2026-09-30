#include "PluginEditor.h"
#include "BinaryData.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace lay
{
    constexpr int W = 1180;
    constexpr int H = 800;
}

namespace
{
using namespace std::literals;

constexpr const char* kRightPanelSectionLabels[4] = {
    "PLAY", "MOTION", "SPACE", "ARP"
};

constexpr int kMinEditorW = 834;
constexpr int kMinEditorH = 579;

juce::String familyDisplayName(const mps::Family family)
{
    switch (family)
    {
        case mps::Family::Concert:  return "Concert";
        case mps::Family::Vintage:  return "Vintage / Character";
        case mps::Family::Electric: return "Electric / Piano-adjacent";
    }

    return "Piano";
}

std::string_view presetRoleFilterTarget(const int roleFilterId) noexcept
{
    if (roleFilterId == 2)
        return "reference"sv;
    if (roleFilterId == 3)
        return "close"sv;
    if (roleFilterId == 4)
        return "featured"sv;
    if (roleFilterId == 5)
        return "warm"sv;
    if (roleFilterId == 6)
        return "cut"sv;
    if (roleFilterId == 7)
        return "groove"sv;
    if (roleFilterId == 8)
        return "character"sv;

    return {} ;
}

bool presetMatchesBrowserTier(const std::string_view mixRole, const int bankViewId) noexcept
{
    switch (bankViewId)
    {
        case 2: return mps::getPresetBrowserTier(mixRole) == "dry"sv;
        case 3: return mps::getPresetBrowserTier(mixRole) == "featured"sv;
        case 4: return mps::getPresetBrowserTier(mixRole) == "mix"sv;
        default: return true;
    }
}

juce::String presetTierBadge(const std::string_view mixRole)
{
    const auto tier = mps::getPresetBrowserTier(mixRole);
    if (tier.empty())
        return "USER";
    if (tier == "dry"sv)
        return "DRY";
    if (tier == "featured"sv)
        return "FTR";
    if (tier == "mix"sv)
        return "MIX";
    if (tier == "signature"sv)
        return "SIG";
    return "USR";
}

juce::String titleCaseRole(const std::string_view role)
{
    if (role.empty())
        return {};

    auto text = juce::String(role.data(), static_cast<int>(role.size())).replaceCharacter('-', ' ');
    if (text.isNotEmpty())
        text = text.substring(0, 1).toUpperCase() + text.substring(1);
    return text;
}

void glazePianoChrome(juce::Graphics& g,
                      juce::Rectangle<float> area,
                      float radius,
                      float intensity)
{
    const auto pianoTop = juce::Colour(0xff49505A).withAlpha(0.030f * intensity);
    const auto pianoMid = juce::Colour(0xff2A3038).withAlpha(0.060f * intensity);
    const auto pianoBottom = juce::Colour(0xff0E1319).withAlpha(0.155f * intensity);

    juce::ColourGradient glaze(pianoTop, area.getCentreX(), area.getY(),
                               pianoBottom, area.getCentreX(), area.getBottom(), false);
    glaze.addColour(0.42, pianoMid);
    g.setGradientFill(glaze);
    g.fillRoundedRectangle(area, radius);

    auto sheen = area.reduced(2.4f).withHeight(juce::jmax(8.0f, area.getHeight() * 0.16f));
    juce::ColourGradient highlight(juce::Colours::white.withAlpha(0.028f * intensity), sheen.getCentreX(), sheen.getY(),
                                   juce::Colours::transparentWhite, sheen.getCentreX(), sheen.getBottom(), false);
    g.setGradientFill(highlight);
    g.fillRoundedRectangle(sheen, juce::jmax(0.0f, radius - 2.0f));

    {
        juce::Graphics::ScopedSaveState scopedState(g);
        auto brushedArea = area.reduced(6.0f);
        g.reduceClipRegion(brushedArea.toNearestInt());

        auto addCloud = [&](juce::Point<float> centre, float radiusX, float radiusY, juce::Colour colour)
        {
            juce::ColourGradient cloud(colour, centre.x, centre.y,
                                       juce::Colours::transparentBlack, centre.x + radiusX, centre.y + radiusY, true);
            g.setGradientFill(cloud);
            g.fillEllipse(centre.x - radiusX, centre.y - radiusY, radiusX * 2.0f, radiusY * 2.0f);
        };

        addCloud({ brushedArea.getX() + brushedArea.getWidth() * 0.28f,
                   brushedArea.getY() + brushedArea.getHeight() * 0.18f },
                 brushedArea.getWidth() * 0.34f, brushedArea.getHeight() * 0.16f,
                 juce::Colours::white.withAlpha(0.008f * intensity));
        addCloud({ brushedArea.getX() + brushedArea.getWidth() * 0.72f,
                   brushedArea.getY() + brushedArea.getHeight() * 0.32f },
                 brushedArea.getWidth() * 0.26f, brushedArea.getHeight() * 0.18f,
                 juce::Colours::white.withAlpha(0.005f * intensity));
        addCloud({ brushedArea.getCentreX(),
                   brushedArea.getBottom() - brushedArea.getHeight() * 0.10f },
                 brushedArea.getWidth() * 0.42f, brushedArea.getHeight() * 0.14f,
                 juce::Colours::black.withAlpha(0.040f * intensity));
    }

    g.setColour(juce::Colour(0xff58606C).withAlpha(0.07f * intensity));
    g.drawRoundedRectangle(area.reduced(1.0f), juce::jmax(0.0f, radius - 1.0f), 0.9f);
}

struct PianoLayoutMetrics
{
    enum class HeaderLayoutMode
    {
        Wide,
        Medium,
        Narrow
    };

    HeaderLayoutMode headerMode = HeaderLayoutMode::Medium;
    bool compact = false;
    bool ultraCompact = false;
    bool roomy = false;
    int outerMargin = 24;
    int gutter = 16;
    int headerH = 96;
    int selectorH = 92;
    int kbH = 108;
    int contentW = 0;
    int contentX = 0;
    int selectorY = 0;
    int bodyY = 0;
    int bodyH = 0;
    int kbY = 0;
    int colW = 0;
    int col1X = 0;
    int col2X = 0;
    int col3X = 0;
};

struct HeaderLayoutSpec
{
    PianoLayoutMetrics::HeaderLayoutMode mode = PianoLayoutMetrics::HeaderLayoutMode::Wide;
    bool usesResponsiveRows = false;
    bool usesCompactCaptions = false;
    int controlH = 36;
    int actionButtonH = 24;
    int statusButtonH = 24;
    int gainSize = 34;
    int navW = 26;
    int quickGap = 8;
    int actionGap = 8;
    juce::Rectangle<int> summaryRow;
    juce::Rectangle<int> quickRow;
    juce::Rectangle<int> browserRow;
    juce::Rectangle<int> statusPrimaryRow;
    juce::Rectangle<int> statusSecondaryRow;
    juce::Rectangle<int> statusMeterRow;
};

PianoLayoutMetrics::HeaderLayoutMode chooseHeaderLayoutMode(const int width) noexcept
{
    if (width >= 1440)
        return PianoLayoutMetrics::HeaderLayoutMode::Wide;
    if (width >= 1120)
        return PianoLayoutMetrics::HeaderLayoutMode::Medium;
    return PianoLayoutMetrics::HeaderLayoutMode::Narrow;
}

juce::String headerLayoutModeName(const PianoLayoutMetrics::HeaderLayoutMode mode)
{
    switch (mode)
    {
        case PianoLayoutMetrics::HeaderLayoutMode::Wide:   return "wide";
        case PianoLayoutMetrics::HeaderLayoutMode::Medium: return "medium";
        case PianoLayoutMetrics::HeaderLayoutMode::Narrow: return "narrow";
    }

    return "medium";
}

float layoutDensity(const bool compact, const bool roomy)
{
    if (compact)
        return -1.0f;
    if (roomy)
        return 1.0f;
    return 0.0f;
}

int interpolateGap(const float density, const int compactValue, const int normalValue, const int roomyValue)
{
    if (density <= 0.0f)
    {
        return juce::roundToInt(juce::jmap(density,
                                           -1.0f,
                                           0.0f,
                                           static_cast<float>(compactValue),
                                           static_cast<float>(normalValue)));
    }

    return juce::roundToInt(juce::jmap(density,
                                       0.0f,
                                       1.0f,
                                       static_cast<float>(normalValue),
                                       static_cast<float>(roomyValue)));
}

PianoLayoutMetrics computeLayoutMetrics(int width, int height)
{
    PianoLayoutMetrics layout;
    layout.headerMode = chooseHeaderLayoutMode(width);
    layout.ultraCompact = width <= 900 || height < 640;
    layout.compact = layout.ultraCompact || width < 1120 || height < 700;
    layout.roomy = width > 1600 || height > 940;
    const float density = layoutDensity(layout.compact, layout.roomy);
    layout.outerMargin = layout.ultraCompact ? 12 : (layout.compact ? 16 : 24);
    layout.gutter = layout.ultraCompact ? 8 : interpolateGap(density, 10, 16, 22);
    switch (layout.headerMode)
    {
        case PianoLayoutMetrics::HeaderLayoutMode::Wide:
            layout.headerH = layout.ultraCompact ? 84 : (layout.compact ? 88 : 96);
            break;
        case PianoLayoutMetrics::HeaderLayoutMode::Medium:
            layout.headerH = layout.ultraCompact ? 132 : (layout.compact ? 148 : 154);
            break;
        case PianoLayoutMetrics::HeaderLayoutMode::Narrow:
            layout.headerH = layout.ultraCompact ? 136 : (layout.compact ? 158 : 162);
            break;
    }
    layout.selectorH = layout.ultraCompact ? 54 : (layout.compact ? 62 : 68);
    layout.kbH = layout.ultraCompact
        ? juce::jlimit(62, 72, static_cast<int>(height * 0.125f))
        : layout.compact
        ? juce::jlimit(72, 96, static_cast<int>(height * 0.14f))
        : juce::jlimit(84, 126, static_cast<int>(height * (layout.roomy ? 0.145f : 0.16f)));

    const int maxContentW = juce::jmin(juce::jmax(0, width - layout.outerMargin * 2), 1680);
    layout.contentW = layout.ultraCompact ? maxContentW : juce::jmax(900, maxContentW);
    layout.contentX = juce::jmax(layout.outerMargin, (width - layout.contentW) / 2);

    const int selectorGap = layout.ultraCompact ? 6 : 8;
    const int bodyKeyboardGap = layout.ultraCompact ? 8 : 10;
    layout.selectorY = layout.outerMargin + layout.headerH + selectorGap;
    layout.kbY = height - layout.kbH - layout.outerMargin;
    layout.bodyY = layout.selectorY + layout.selectorH + layout.gutter;
    const int availableBodyH = juce::jmax(0, layout.kbY - layout.bodyY - bodyKeyboardGap);
    layout.bodyH = layout.ultraCompact ? availableBodyH : juce::jmax(250, availableBodyH);
    if (layout.bodyY + layout.bodyH > layout.kbY - bodyKeyboardGap)
        layout.bodyH = availableBodyH;

    layout.colW = juce::jmax(1, (layout.contentW - layout.gutter * 2) / 3);
    layout.col1X = layout.contentX;
    layout.col2X = layout.col1X + layout.colW + layout.gutter;
    layout.col3X = layout.col2X + layout.colW + layout.gutter;
    return layout;
}

template <typename HeaderZonesT>
HeaderLayoutSpec computeHeaderLayoutSpec(const PianoLayoutMetrics& layout,
                                         const HeaderZonesT& headerZones)
{
    HeaderLayoutSpec spec;
    spec.mode = layout.headerMode;
    spec.usesResponsiveRows = spec.mode != PianoLayoutMetrics::HeaderLayoutMode::Wide;
    spec.usesCompactCaptions = spec.usesResponsiveRows;

    switch (spec.mode)
    {
        case PianoLayoutMetrics::HeaderLayoutMode::Wide:
            spec.controlH = layout.ultraCompact ? 28 : (layout.compact ? 34 : 36);
            spec.actionButtonH = layout.ultraCompact ? 20 : (layout.compact ? 22 : 24);
            spec.statusButtonH = layout.ultraCompact ? 20 : (layout.compact ? 22 : 24);
            spec.gainSize = layout.ultraCompact ? 24 : (layout.compact ? 30 : 34);
            spec.navW = layout.ultraCompact ? 22 : 26;
            spec.quickGap = layout.ultraCompact ? 5 : (layout.compact ? 6 : 8);
            spec.actionGap = layout.ultraCompact ? 4 : (layout.compact ? 6 : 8);
            spec.summaryRow = headerZones.presetPrimaryRow.reduced(0, 1);
            spec.quickRow = headerZones.presetPrimaryRow.reduced(0, 1);
            spec.browserRow = headerZones.presetSecondaryRow.reduced(0, 1);
            spec.statusPrimaryRow = headerZones.statusPrimaryRow.reduced(0, 1);
            spec.statusSecondaryRow = headerZones.statusSecondaryRow.reduced(0, 1)
                .withTrimmedRight(spec.gainSize + (layout.compact ? 24 : 26));
            spec.statusMeterRow = spec.statusSecondaryRow;
            break;
        case PianoLayoutMetrics::HeaderLayoutMode::Medium:
            spec.controlH = layout.ultraCompact ? 26 : 30;
            spec.actionButtonH = layout.ultraCompact ? 20 : 22;
            spec.statusButtonH = layout.ultraCompact ? 18 : 20;
            spec.gainSize = layout.ultraCompact ? 22 : 24;
            spec.navW = layout.ultraCompact ? 22 : 24;
            spec.quickGap = layout.ultraCompact ? 5 : 6;
            spec.actionGap = layout.ultraCompact ? 4 : 5;
            spec.summaryRow = headerZones.presetZone.reduced(0, 1);
            spec.quickRow = headerZones.fullWidthPrimaryRow.reduced(0, 1);
            spec.browserRow = headerZones.fullWidthSecondaryRow.reduced(0, 1);
            spec.statusPrimaryRow = headerZones.statusPrimaryRow.reduced(0, 1);
            spec.statusSecondaryRow = headerZones.statusSecondaryRow.reduced(0, 1);
            break;
        case PianoLayoutMetrics::HeaderLayoutMode::Narrow:
            spec.controlH = layout.ultraCompact ? 24 : 28;
            spec.actionButtonH = layout.ultraCompact ? 18 : 20;
            spec.statusButtonH = layout.ultraCompact ? 18 : 20;
            spec.gainSize = layout.ultraCompact ? 20 : 22;
            spec.navW = layout.ultraCompact ? 20 : 22;
            spec.quickGap = layout.ultraCompact ? 4 : 6;
            spec.actionGap = layout.ultraCompact ? 3 : 4;
            spec.summaryRow = headerZones.presetZone.reduced(0, 1);
            spec.quickRow = headerZones.fullWidthPrimaryRow.reduced(0, 1);
            spec.browserRow = headerZones.fullWidthSecondaryRow.reduced(0, 1);
            spec.statusPrimaryRow = headerZones.statusPrimaryRow.reduced(0, 1);
            spec.statusSecondaryRow = headerZones.statusSecondaryRow.reduced(0, 1);
            break;
    }

    return spec;
}

using PianoEnvUiProfile = synthui::InstrumentUiProfile<15>;

const synthui::MacroLabelProfile<4>& macroLabelsForFamily(const mps::Family family)
{
    static const synthui::MacroLabelProfile<4> concert = { "Timbre", "Presence", "Body", "Pedal Res" };
    static const synthui::MacroLabelProfile<4> vintage = { "Wear", "Detune", "Percussion", "Room" };
    static const synthui::MacroLabelProfile<4> electric = { "Bark", "Tine/Reed", "Amp Color", "Motion" };

    switch (family)
    {
        case mps::Family::Concert: return concert;
        case mps::Family::Vintage: return vintage;
        case mps::Family::Electric: return electric;
    }
    return concert;
}

const PianoEnvUiProfile& envProfileForPiano(const int pianoIndex)
{
    static const PianoEnvUiProfile concert = {{
        { "Level", "Output volume", "Sets the piano output volume" },
        { "Tune", "Fine tuning", "Adjusts the overall pitch of the piano" },
        { "Hammer", "Hammer hardness", "Hardens or softens the hammer attack" },
        { "Attack", "Attack time", "Controls how fast the note starts" },
        { "Decay", "Decay time", "Shortens or lengthens the initial decay" },
        { "Sustain", "Sustain level", "Holds more of the note body" },
        { "Release", "Release time", "Controls the fade-out after release" },
        { "Brightness", "Tonal brightness", "Opens or darkens the piano spectrum" },
        { "String Res", "String resonance", "Sets the sympathetic string resonance" },
        { "Soundboard", "Soundboard amount", "Adds the soundboard contribution" },
        { "Damping", "String damping", "Dampens high frequencies and shortens the note" },
        { "Character", "Model character", "Shifts the piano timbre character" },
        { "Low Pass", "Low-pass filter", "Darkens or opens the high end" },
        { "Pan", "Pan position", "Places the piano in the stereo field" },
        { "Output", "Output routing", "Routes the piano to the master or an auxiliary output" }
    }};

    static const PianoEnvUiProfile vintage = {{
        { "Level", "Output volume", "Sets the vintage piano output volume" },
        { "Tune", "Fine tuning", "Adjusts the overall pitch of the vintage model" },
        { "Mechanics", "Mechanical noise", "Adds mechanical wear and noise to the attack" },
        { "Attack", "Attack time", "Controls how fast the note starts" },
        { "Decay", "Decay time", "Shortens or lengthens the main decay" },
        { "Sustain", "Sustain level", "Holds the note body longer" },
        { "Release", "Release time", "Controls the note end after release" },
        { "Tone", "Tonal brightness", "Brightens or darkens the worn piano" },
        { "Sympathy", "Sympathetic resonance", "Adds string and body halo" },
        { "Case Tone", "Case colour", "Sets the contribution of the case and mechanical parts" },
        { "Wear", "String damping", "Mutes the model more for a shorter sound" },
        { "Age", "Age character", "Varies the age and character of the piano" },
        { "Tone Cut", "Low-pass filter", "Darkens or opens the high end" },
        { "Pan", "Pan position", "Places the piano in the stereo field" },
        { "Output", "Output routing", "Routes the piano to the master or an auxiliary output" }
    }};

    static const PianoEnvUiProfile electric = {{
        { "Level", "Output volume", "Sets the electric piano output volume" },
        { "Tune", "Fine tuning", "Adjusts the overall pitch of the electric model" },
        { "Strike", "Strike hardness", "Hardens or softens the tine or reed strike" },
        { "Attack", "Attack time", "Controls how fast the sound arrives" },
        { "Decay", "Decay time", "Shortens or lengthens the decay" },
        { "Sustain", "Sustain level", "Holds more of the note body" },
        { "Release", "Release time", "Controls the note end after release" },
        { "Tone", "Tonal brightness", "Opens or darkens the tine or reed output" },
        { "Pickup", "Pickup colour", "Sets the pickup and electric sensing feel" },
        { "Amp Body", "Amp body", "Adds amp body and cabinet character" },
        { "Tone Damp", "Tone damping", "Dampens the highs and shortens the tail" },
        { "Reed/Tine", "Model character", "Varies the character between reed, tine and variants" },
        { "Tone Cut", "Low-pass filter", "Darkens or opens the electric output" },
        { "Pan", "Pan position", "Places the electric piano in the stereo field" },
        { "Output", "Output routing", "Routes the piano to the master or an auxiliary output" }
    }};

    switch (mps::getFamily(pianoIndex))
    {
        case mps::Family::Concert:  return concert;
        case mps::Family::Vintage:  return vintage;
        case mps::Family::Electric: return electric;
    }
    return concert;
}

mps::GlobalFxSlot fxSlotForTab(const int tabIndex)
{
    switch (tabIndex)
    {
        case 0: return mps::GlobalFxSlot::Reverb;
        case 1: return mps::GlobalFxSlot::Saturator;
        case 2: return mps::GlobalFxSlot::Transient;
        case 3: return mps::GlobalFxSlot::Compressor;
        case 4: return mps::GlobalFxSlot::Eq;
        case 5: return mps::GlobalFxSlot::Chorus;
        case 6: return mps::GlobalFxSlot::Delay;
        case 7: return mps::GlobalFxSlot::Limiter;
        default: return mps::GlobalFxSlot::Limiter;
    }
}

juce::String effectDetailTitleForTab(const int tabIndex)
{
    switch (tabIndex)
    {
        case 0: return "Reverb Parameters";
        case 1: return "Saturation Parameters";
        case 2: return "Transient Parameters";
        case 3: return "Compressor Parameters";
        case 4: return "EQ Parameters";
        case 5: return "Chorus Parameters";
        case 6: return "Delay Parameters";
        case 7: return "Limiter Parameters";
        default: break;
    }

    return "FX Parameters";
}

juce::String trimNumericString(juce::String text)
{
    if (!text.containsChar('.'))
        return text;

    while (text.endsWithChar('0'))
        text = text.dropLastCharacters(1);

    if (text.endsWithChar('.'))
        text = text.dropLastCharacters(1);

    return text;
}

juce::String formatNumber(double value, int decimals = 2)
{
    if (std::abs(value) < 0.0005)
        value = 0.0;

    return trimNumericString(juce::String(value, decimals));
}

juce::String formatPercent01(double value)
{
    return juce::String(juce::roundToInt(juce::jlimit(0.0, 1.0, value) * 100.0)) + "%";
}

juce::String formatSignedPercent(double value, const double maxAbs = 1.0)
{
    const auto ratio = maxAbs > 0.0 ? juce::jlimit(-1.0, 1.0, value / maxAbs) : 0.0;
    const auto percent = juce::roundToInt(ratio * 100.0);
    return juce::String(percent >= 0 ? "+" : "") + juce::String(percent) + "%";
}

juce::String formatCenteredMacroPercent(double value)
{
    return formatSignedPercent(value - 0.5, 0.5);
}

juce::String formatFrequency(double value)
{
    if (value >= 1000.0)
        return formatNumber(value / 1000.0, value >= 10000.0 ? 0 : 1) + " kHz";

    return juce::String(juce::roundToInt(value)) + " Hz";
}

juce::String formatTime(double value)
{
    if (value < 1.0)
        return juce::String(juce::roundToInt(value * 1000.0)) + " ms";

    return formatNumber(value, value >= 10.0 ? 1 : 2) + " s";
}

juce::String formatMilliseconds(double value)
{
    return formatNumber(value, value < 10.0 ? 1 : 0) + " ms";
}

juce::String formatPan(double value)
{
    const auto clamped = juce::jlimit(-1.0, 1.0, value);
    const auto amount = juce::roundToInt(std::abs(clamped) * 100.0);
    if (amount <= 1)
        return "Center";

    return juce::String(clamped < 0.0 ? "L" : "R") + juce::String(amount);
}

juce::String formatSemitones(double value)
{
    return formatNumber(value, std::abs(value - std::round(value)) < 0.001 ? 0 : 1) + " st";
}

juce::String formatDecibels(double value)
{
    return formatNumber(value, std::abs(value) < 10.0 ? 1 : 0) + " dB";
}

juce::String formatRatio(double value)
{
    return formatNumber(value, value < 10.0 ? 1 : 0) + ":1";
}

juce::String formatDrive(double value)
{
    return formatNumber(value, value < 10.0 ? 1 : 0) + "x";
}

juce::String formatQ(double value)
{
    return "Q " + formatNumber(value, value < 1.0 ? 2 : 1);
}

void setSliderFormatter(juce::Slider& slider, std::function<juce::String(double)> formatter)
{
    slider.textFromValueFunction = [formatter = std::move(formatter)](double value)
    {
        return formatter(value);
    };
}
}

const std::array<PianoSynthAudioProcessorEditor::CtrlDef,
                 PianoSynthAudioProcessorEditor::kEnvN>
    PianoSynthAudioProcessorEditor::kEnvCtrls = {{
        { "Level",      "level" },
        { "Tune",       "tune" },
        { "Hammer",     "hammer_hardness" },
        { "Attack",     "attack" },
        { "Decay",      "decay" },
        { "Sustain",    "sustain" },
        { "Release",    "release" },
        { "Brightness", "tone_brightness" },
        { "String Res", "string_resonance" },
        { "Soundboard", "soundboard_amount" },
        { "Damping",    "damping" },
        { "Character",  "model_character" },
        { "Low Pass",   "low_pass_hz" },
        { "Pan",        "pan" },
        { "Output",     "output" }
    }};

const std::array<PianoSynthAudioProcessorEditor::FxDef,
                 PianoSynthAudioProcessorEditor::kMacroTotal>
    PianoSynthAudioProcessorEditor::kMacroCtrls = {{
        { "Tone",       "macro_warmth" },
        { "Color",      "macro_brillance" },
        { "Expression", "macro_expression" },
        { "Resonance",  "macro_resonance" }
    }};

const std::array<PianoSynthAudioProcessorEditor::FxDef,
                 PianoSynthAudioProcessorEditor::kFxN>
    PianoSynthAudioProcessorEditor::kFxCtrls = {{
        { "Size",       "reverb_size" },
        { "Damping",    "reverb_damping" },
        { "Width",      "reverb_width" },
        { "Mix",        "reverb_mix" },
        { "Predelay",   "reverb_predelay" },
        { "Drive",      "sat_drive" },
        { "Mix",        "sat_mix" },
        { "Attack",     "transient_attack" },
        { "Sustain",    "transient_sustain" },
        { "Mix",        "transient_mix" },
        { "Threshold",  "comp_threshold" },
        { "Ratio",      "comp_ratio" },
        { "Attack",     "comp_attack" },
        { "Release",    "comp_release" },
        { "Makeup",     "comp_makeup" },
        { "Mix",        "comp_mix" },
        { "Low Freq",   "eq_low_freq" },
        { "Low Gain",   "eq_low_gain" },
        { "Mid Freq",   "eq_mid_freq" },
        { "Mid Gain",   "eq_mid_gain" },
        { "Mid Q",      "eq_mid_q" },
        { "High Freq",  "eq_high_freq" },
        { "High Gain",  "eq_high_gain" },
        { "Rate",       "chorus_rate" },
        { "Depth",      "chorus_depth" },
        { "Mix",        "chorus_mix" },
        { "Time",       "delay_time" },
        { "Feedback",   "delay_feedback" },
        { "Mix",        "delay_mix" },
        { "Threshold",  "limiter_threshold" },
        { "Release",    "limiter_release" }
    }};

const char* PianoSynthAudioProcessorEditor::kFxTabNames[kFxTabs] = {
    "REVERB", "SATURATION", "TRANSIENT", "COMPRESSOR",
    "EQ", "CHORUS", "DELAY", "LIMITER"
};

const char* PianoSynthAudioProcessorEditor::kFxRackSummaries[kFxTabs] = {
    "Room and bloom",
    "Harmonic drive",
    "Attack contour",
    "Dynamics glue",
    "Tone shaping",
    "Stereo motion",
    "Echo repeats",
    "Output ceiling"
};

const char* PianoSynthAudioProcessorEditor::kFxBypassParamIds[kFxTabs] = {
    "fx_tab7_en",
    "fx_tab0_en",
    "fx_tab1_en",
    "fx_tab2_en",
    "fx_tab3_en",
    "fx_tab4_en",
    "fx_tab5_en",
    "fx_tab6_en"
};

// =============================================================================
// Tooltip texts  (index order: envDials[0..13], lfoRate, lfoDepth,
//                 macroDials[0..3], fxDials[0..30], gainDial)
// =============================================================================
const char* PianoSynthAudioProcessorEditor::kTooltipsShort[kTooltipCount] = {
    // env 0-14
    "Piano output volume (0 - 100 %)",
    "Fine tuning in semitones (-24 to +24)",
    "Hammer hardness - harder = brighter",
    "Envelope attack time (0 - 2 s)",
    "Decay time after attack (0.1 - 10 s)",
    "Envelope sustain level (0 - 100 %)",
    "Release time after key release",
    "Tonal brightness - harmonic content",
    "Sympathetic resonance of neighbouring strings",
    "Soundboard resonance amount",
    "String damping - higher = drier",
    "Synthesis model character",
    "Low-pass filter cutoff (120 - 18 000 Hz)",
    "Piano stereo position (-1 left, +1 right)",
    "Output routing (Master or Out 1-4)",
    // lfo 15-16
    "LFO speed (0.05 - 12 Hz)",
    "LFO modulation depth",
    // macro 17-20
    "Macro Tone - global warmth of the sound",
    "Macro Color - global brightness of the sound",
    "Macro Expression - sensitivity and dynamics",
    "Macro Resonance - string and soundboard resonance",
    // fx 21-51
    "Reverb room size",
    "Reverb high-frequency damping",
    "Reverb stereo width",
    "Reverb dry/wet mix",
    "Pre-delay before reverb (0 - 100 ms)",
    "Harmonic saturation amount",
    "Saturation dry/wet mix",
    "Transient attack boost",
    "Transient sustain control",
    "Transient shaper dry/wet mix",
    "Compressor threshold (-60 to 0 dB)",
    "Compression ratio (1:1 to 20:1)",
    "Compressor attack time (0.1 - 100 ms)",
    "Compressor release time (5 - 500 ms)",
    "Compressor makeup gain (0 - 24 dB)",
    "Compressor dry/wet mix",
    "Bass filter frequency - Low Shelf",
    "Bass filter gain (dB)",
    "Mid filter frequency - Peak",
    "Mid filter gain (dB)",
    "Mid filter Q factor - bandwidth",
    "High filter frequency - High Shelf",
    "High filter gain (dB)",
    "Chorus modulation speed (0.1 - 5 Hz)",
    "Chorus modulation depth",
    "Chorus dry/wet mix",
    "Delay time (1 - 2000 ms)",
    "Delay feedback (0 - 95 %)",
    "Delay dry/wet mix",
    "Output limiter threshold (-12 to 0 dB)",
    "Limiter release time (1 - 200 ms)",
    // gain 52
    "Master output gain (-24 to +12 dB)"
};

const char* PianoSynthAudioProcessorEditor::kTooltipsNovice[kTooltipCount] = {
    // env 0-14
    "Level - Selected piano volume. Turn up for louder, down for softer.",
    "Tune - Fine-tune the piano in semitones. Useful to match other instruments.",
    "Hammer - Hardness of the hammer striking the string. Harder = brighter and more percussive, softer = felt-like.",
    "Attack - How fast the sound rises at the start. Short = sharp attack, long = gentle entry.",
    "Decay - How long the sound fades after the initial peak before sustain.",
    "Sustain - Level held while the key is pressed. 100% = no decay.",
    "Release - Time for the sound to fade when you release the key. Short = clean, long = resonant.",
    "Brightness - Makes the sound brighter or darker. Higher = more high harmonics.",
    "String Res - Sympathetic resonance: neighbouring strings vibrate in sympathy. Adds richness.",
    "Soundboard - Soundboard intensity. Higher = fuller and more woody sound.",
    "Damping - String damping. Higher = drier and shorter, lower = longer sustain.",
    "Character - Sound personality of the model. Explore to find the colour you like.",
    "Low Pass - Low-pass filter: right = open sound, left = closed and warm.",
    "Pan - Moves sound between left and right. Centre = equal on both sides.",
    "Output - Routes this piano either to the main mix or to one of the auxiliary stem outputs.",
    // lfo 15-16
    "LFO Rate - Speed of the automatic oscillation. Slow = gentle wave, fast = vibrato.",
    "LFO Depth - Amount of LFO modulation. At zero, LFO has no audible effect.",
    // macro 17-20
    "Tone - Macro that adjusts global warmth. Ideal for quickly softening the piano sound.",
    "Color - Macro that enhances brightness. Gives more presence and clarity to the sound.",
    "Expression - Dynamics macro: makes playing more expressive and velocity-responsive.",
    "Resonance - Macro that increases string and soundboard resonance. More alive and full sound.",
    // fx 21-51
    "Size - Reverb room size. Larger = cavernous space, smaller = intimate room.",
    "Damping - Absorbs highs in the reverb. Higher = darker and softer reverb tail.",
    "Width - Reverb stereo width. Full = immersive, reduced = more centred.",
    "Mix - Dry and reverb blend. 100% = all reverb, 0% = dry signal only.",
    "Predelay - Delay before reverb starts. Separates the direct sound from the reverb tail.",
    "Drive - Harmonic saturation. A little = warmth, a lot = pronounced distortion.",
    "Mix - Clean and saturated signal blend. Keep some dry for clarity.",
    "Attack - Enhances the attack transient. Turn up for more snap, down to soften.",
    "Sustain - Body of the sound after the attack. Turn up for more sustain, down for a shorter sound.",
    "Mix - Original signal and transient effect blend. Adjust to stay natural.",
    "Threshold - Compressor threshold. Lower = more signal compressed, higher = less action.",
    "Ratio - Compression strength. 2:1 subtle, 10:1 pronounced. Controls dynamics.",
    "Attack - Compressor reaction. Fast = controls peaks, slow = lets transients through.",
    "Release - Compressor release time. Too short = pumping, too long = crushing.",
    "Makeup - Compensation gain. Restore the volume lost through compression.",
    "Mix - Dry/compressed blend. 50% gives a natural parallel compression.",
    "Low Freq - Bass filter frequency. Choose which bass range to boost or cut.",
    "Low Gain - Bass filter gain in dB. Positive = boost, negative = cut.",
    "Mid Freq - Mid filter frequency. Target the frequencies to adjust.",
    "Mid Gain - Mid filter gain in dB. Positive = boost, negative = cut.",
    "Mid Q - Mid filter bandwidth. Narrow = surgical, wide = musical.",
    "High Freq - High filter frequency. Choose from where to boost or cut.",
    "High Gain - High filter gain in dB. Positive = sparkle, negative = softness.",
    "Rate - Chorus speed. Slow = subtle wave, fast = perceptible modulation.",
    "Depth - Chorus depth. Higher = more movement and stereo richness.",
    "Mix - Dry signal and chorus blend. Adjust for a natural effect.",
    "Time - Delay time in ms. Short = slap-back, long = distinct echoes.",
    "Feedback - Delay repetitions. Higher = more repeats before fading out.",
    "Mix - Dry signal and delay blend. Adjust for the desired effect.",
    "Threshold - Limiter threshold. Prevents the signal from exceeding this level. Protects your speakers.",
    "Release - Limiter release. Short = reactive, long = more transparent.",
    // gain 52
    "Gain - Final output volume of the synth. Set to match the level in your mix."
};

juce::Colour PianoSynthAudioProcessorEditor::familyColour(int familyIndex)
{
    switch (familyIndex)
    {
        case 0: return juce::Colour(0xff1565C0);
        case 1: return juce::Colour(0xffD97A23);
        case 2: return juce::Colour(0xff0E9687);
        default: return juce::Colour(0xff1565C0);
    }
}

juce::Colour PianoSynthAudioProcessorEditor::pianoCatColour(int pianoIndex)
{
    static constexpr juce::uint32 colours[mps::kNumPianos] = {
        0xff2D6AC7,
        0xff4E84D3,
        0xff769EE0,
        0xffD97A23,
        0xffE2A045,
        0xff1A8C84,
        0xff17A49A,
        0xff3DBDB3
    };

    return juce::Colour(colours[static_cast<std::size_t>(juce::jlimit(0, mps::kNumPianos - 1, pianoIndex))]);
}

int PianoSynthAudioProcessorEditor::selectedPianoFromParam() const
{
    if (auto* raw = proc.getAPVTS().getRawParameterValue("selected_piano"))
        return juce::jlimit(0, mps::kNumPianos - 1,
                            static_cast<int>(std::round(raw->load())));
    return 0;
}

PianoSynthAudioProcessorEditor::VisualLayoutSnapshot
PianoSynthAudioProcessorEditor::computeVisualLayoutSnapshot(int width, int height) const
{
    const auto layout = computeLayoutMetrics(width, height);

    VisualLayoutSnapshot snapshot;
    snapshot.compact = layout.compact;
    snapshot.ultraCompact = layout.ultraCompact;
    snapshot.roomy = layout.roomy;
    snapshot.headerH = layout.headerH;
    snapshot.headerLayoutModeName = headerLayoutModeName(layout.headerMode);
    snapshot.contentX = layout.contentX;
    snapshot.contentW = layout.contentW;
    snapshot.selectorY = layout.selectorY;
    snapshot.selectorH = layout.selectorH;
    snapshot.bodyY = layout.bodyY;
    snapshot.bodyH = layout.bodyH;
    snapshot.kbY = layout.kbY;
    snapshot.kbH = layout.kbH;
    snapshot.col1X = layout.col1X;
    snapshot.col2X = layout.col2X;
    snapshot.col3X = layout.col3X;
    snapshot.colW = layout.colW;
    snapshot.headerZones = computeHeaderZones(layout.headerH);
    snapshot.headerBounds = snapshot.headerZones.headerBounds;
    snapshot.selectorPanelBounds = { layout.contentX, layout.selectorY, layout.contentW, layout.selectorH - 8 };
    return snapshot;
}

#if defined(UWDEVST_PIANO_TEST_BUILD)
PianoSynthAudioProcessorEditor::LayoutSnapshot
PianoSynthAudioProcessorEditor::captureLayoutSnapshotForTests() const
{
    const auto layout = computeVisualLayoutSnapshot(getWidth(), getHeight());

    LayoutSnapshot snapshot;
    snapshot.compact = layout.compact;
    snapshot.editorBounds = getLocalBounds();
    snapshot.headerBounds = layout.headerBounds;
    snapshot.selectorPanelBounds = layout.selectorPanelBounds;
    snapshot.headerLayoutModeName = layout.headerLayoutModeName;
    snapshot.modelSelectorBounds = modelSelector.getBounds();
    snapshot.rightPanelBounds = { layout.col3X, layout.bodyY, layout.colW, layout.bodyH };
    snapshot.currentPresetSummaryBounds = currentPresetSummaryLabel.getBounds();
    snapshot.currentPresetMetaBounds = currentPresetMetaLabel.getBounds();
    snapshot.quickTierBounds = quickTierSelector.getBounds();
    snapshot.quickRoleBounds = quickRoleSelector.getBounds();
    snapshot.quickReferenceBounds = quickReferenceBtn.getBounds();
    snapshot.quickMixBounds = quickMixBtn.getBounds();
    snapshot.quickCinematicBounds = quickCinematicBtn.getBounds();
    snapshot.presetSearchBounds = presetSearch.getBounds();
    snapshot.prevPresetBounds = prevPresetBtn.getBounds();
    snapshot.presetBoxBounds = presetBox.getBounds();
    snapshot.nextPresetBounds = nextPresetBtn.getBounds();
    snapshot.savePresetBounds = savePresetBtn.getBounds();
    snapshot.saveAsPresetBounds = saveAsPresetBtn.getBounds();
    snapshot.deletePresetBounds = deletePresetBtn.getBounds();
    snapshot.importPresetBounds = importPresetsBtn.getBounds();
    snapshot.randBounds = randButton.getBounds();
    snapshot.midiLearnBounds = midiLearnButton.getBounds();
    snapshot.undoBounds = undoButton.getBounds();
    snapshot.redoBounds = redoButton.getBounds();
    snapshot.monoBounds = monoModeButton.getBounds();
    snapshot.tooltipModeBounds = tooltipModeBtn.getBounds();
    snapshot.voiceCountBounds = voiceCountLabel.getBounds();
    snapshot.gainBounds = gainDial.getBounds();
    snapshot.midiCCBounds = midiCCPageLabel.getBounds();
    snapshot.keyboardBounds = keyboard != nullptr ? keyboard->getBounds() : juce::Rectangle<int>();
    snapshot.octaveDownBounds = octaveDownBtn.getBounds();
    snapshot.octaveUpBounds = octaveUpBtn.getBounds();
    snapshot.envVisualBounds = envVisual.getBounds();
    snapshot.lfoVisualBounds = lfoVisual.isVisible() ? lfoVisual.getBounds() : juce::Rectangle<int>();
    snapshot.lowPassBounds = envDials[12].getBounds();
    snapshot.velocitySelectorBounds = velocityCurveSelector.getBounds();
    snapshot.lfoDestinationBounds = lfoDestinationSelector.isVisible() ? lfoDestinationSelector.getBounds()
                                                                       : juce::Rectangle<int>();
    snapshot.delayDivisionBounds = delayNoteDivSelector.isVisible() ? delayNoteDivSelector.getBounds()
                                                                    : juce::Rectangle<int>();
    snapshot.arpModeBounds = arpModeSelector.isVisible() ? arpModeSelector.getBounds()
                                                         : juce::Rectangle<int>();
    snapshot.arpRateBounds = arpRateSelector.isVisible() ? arpRateSelector.getBounds()
                                                         : juce::Rectangle<int>();
    snapshot.currentPresetSummaryVisible = currentPresetSummaryLabel.isVisible();
    snapshot.quickTierVisible = quickTierSelector.isVisible();
    snapshot.quickRoleVisible = quickRoleSelector.isVisible();
    snapshot.midiCCVisible = midiCCPageLabel.isVisible();
    snapshot.currentPresetSummaryText = currentPresetSummaryLabel.getText();
    snapshot.currentPresetMetaText = currentPresetMetaLabel.getText();
    snapshot.modMatrixToggleBounds = lfoAdvancedButton.getBounds();
    snapshot.fxLockBounds = fxLockButton.getBounds();
    snapshot.fxLockVisible = fxLockButton.isVisible();
    snapshot.modMatrixToggleVisible = lfoAdvancedButton.isVisible();
    snapshot.lfoChorusItemEnabled = lfoDestinationSelector.isItemEnabled(4);
    snapshot.lfoChorusItemText = lfoDestinationSelector.getItemText(
        lfoDestinationSelector.indexOfItemId(4));
    snapshot.fxUnavailableText = fxUnavailableLbl.getText();
    snapshot.fxDetailTitleText = fxDetailTitle.getText();
    snapshot.delayDivisionText = delayNoteDivSelector.getText();
    snapshot.aftertouchStatusText = aftertouchStatusLabel.getText();
    snapshot.modAftertouchSourceText = modRows[0].srcCombo.getItemText(
        modRows[0].srcCombo.indexOfItemId(static_cast<int>(modmatrix::Source::Aftertouch) + 1));
    snapshot.lowPassTextFor10000 = const_cast<juce::Slider&>(envDials[12]).getTextFromValue(10000.0);
    snapshot.lowPassTextFor9050 = const_cast<juce::Slider&>(envDials[12]).getTextFromValue(9050.0);
    snapshot.reverbShapeControlsEnabled = fxDials[0].isEnabled()
        && fxDials[1].isEnabled()
        && fxDials[2].isEnabled();
    snapshot.tremoloSyncVisible = tremoloSyncButton.isVisible();
    snapshot.aftertouchStatusVisible = aftertouchStatusLabel.isVisible();
    for (int i = 0; i < kEnvN; ++i)
    {
        snapshot.envLabelText[static_cast<std::size_t>(i)] = envLabels[static_cast<std::size_t>(i)].getText();
        snapshot.envControlEnabled[static_cast<std::size_t>(i)] = envDials[static_cast<std::size_t>(i)].isEnabled();
    }

    for (const auto& tab : rightPanelTabs)
    {
        if (!tab.isVisible())
            continue;

        snapshot.rightPanelTabBounds = snapshot.rightPanelTabBounds.isEmpty()
            ? tab.getBounds()
            : snapshot.rightPanelTabBounds.getUnion(tab.getBounds());
    }

    auto includeQuickShortcut = [&snapshot](const juce::Button& button)
    {
        if (!button.isVisible() || button.getBounds().isEmpty())
            return;

        snapshot.quickShortcutVisible = true;
        snapshot.quickShortcutBounds = snapshot.quickShortcutBounds.isEmpty()
            ? button.getBounds()
            : snapshot.quickShortcutBounds.getUnion(button.getBounds());
    };

    includeQuickShortcut(quickReferenceBtn);
    includeQuickShortcut(quickMixBtn);
    includeQuickShortcut(quickCinematicBtn);

    auto includeArpBounds = [&snapshot](const juce::Component& component)
    {
        if (!component.isVisible() || component.getBounds().isEmpty())
            return;

        snapshot.arpBlockVisible = true;
        snapshot.arpBlockBounds = snapshot.arpBlockBounds.isEmpty()
            ? component.getBounds()
            : snapshot.arpBlockBounds.getUnion(component.getBounds());
    };

    if (arpHoldButton.isVisible() || arpModeSelector.isVisible() || arpRateSelector.isVisible()
        || arpOctavesDial.isVisible() || arpGateDial.isVisible())
    {
        includeArpBounds(arpHoldButton);
        includeArpBounds(arpModeSelector);
        includeArpBounds(arpRateSelector);
        includeArpBounds(arpOctavesDial);
        includeArpBounds(arpGateDial);
    }

    auto includeVisibleBounds = [&snapshot](const juce::Component& component)
    {
        if (!component.isVisible() || component.getBounds().isEmpty())
            return;

        snapshot.modMatrixViewportBounds = snapshot.modMatrixViewportBounds.isEmpty()
            ? component.getBounds()
            : snapshot.modMatrixViewportBounds.getUnion(component.getBounds());
    };

    includeVisibleBounds(modMatrixTitle);
    for (const auto& row : modRows)
    {
        includeVisibleBounds(row.srcCombo);
        includeVisibleBounds(row.dstCombo);
        includeVisibleBounds(row.amtSlider);
    }

    return snapshot;
}

void PianoSynthAudioProcessorEditor::setRightPanelSectionForTests(int sectionIndex)
{
    switchRightPanelSection(sectionIndex);
}

void PianoSynthAudioProcessorEditor::setFxTabForTests(int tabIndex)
{
    switchEffectTab(tabIndex);
}
#endif

juce::StringArray PianoSynthAudioProcessorEditor::hostGetFactoryNames()
{
    return proc.getFactoryPresetNames();
}

juce::String PianoSynthAudioProcessorEditor::hostFormatFactoryPresetLabel(int presetIndex,
                                                                          const juce::String& displayName) const
{
    const int pianoIdx = proc.getSelectedPianoIndex();
    const auto& banks = proc.getFactoryPresetBanksDirect();
    if (pianoIdx < 0 || pianoIdx >= static_cast<int>(banks.size()))
        return displayName;

    const auto& bank = banks[static_cast<std::size_t>(pianoIdx)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size()))
        return displayName;

    const auto mixRole = std::string_view(bank[static_cast<std::size_t>(presetIndex)].metadata.mixRole);
    return "[" + presetTierBadge(mixRole) + "] " + displayName;
}

juce::String PianoSynthAudioProcessorEditor::hostFactoryPresetSearchText(int presetIndex,
                                                                         const juce::String& displayName) const
{
    const int pianoIdx = proc.getSelectedPianoIndex();
    const auto& banks = proc.getFactoryPresetBanksDirect();
    if (pianoIdx < 0 || pianoIdx >= static_cast<int>(banks.size()))
        return displayName;

    const auto& bank = banks[static_cast<std::size_t>(pianoIdx)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size()))
        return displayName;

    const auto& metadata = bank[static_cast<std::size_t>(presetIndex)].metadata;
    const auto mixRole = std::string_view(metadata.mixRole);
    const auto tier = mps::getPresetBrowserTier(mixRole);
    return displayName
        + " "
        + juce::String(metadata.family)
        + " "
        + juce::String(metadata.intent)
        + " "
        + juce::String(metadata.mixRole)
        + " "
        + juce::String(tier.data(), static_cast<int>(tier.size()))
        + " "
        + juce::String(metadata.tags);
}

bool PianoSynthAudioProcessorEditor::hostShouldIncludeFactoryPreset(int presetIndex) const
{
    const int bankViewSel = presetFamilyFilter.getSelectedId(); // 1=All, 2=Natural, 3=Signature
    const int roleSel = presetRoleFilter.getSelectedId();       // 1=All, 2=Natural, 3=Signature

    if (bankViewSel == 1 && roleSel == 1)
        return true;

    const int pianoIdx = proc.getSelectedPianoIndex();
    const auto& banks = proc.getFactoryPresetBanksDirect();
    if (pianoIdx < 0 || pianoIdx >= static_cast<int>(banks.size()))
        return true;
    const auto& bank = banks[static_cast<std::size_t>(pianoIdx)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size()))
        return true;

    const auto& meta = bank[static_cast<std::size_t>(presetIndex)].metadata;
    const auto mixRole = std::string_view(meta.mixRole);

    if (!presetMatchesBrowserTier(mixRole, bankViewSel))
        return false;

    if (roleSel != 1)
    {
        const auto wanted = presetRoleFilterTarget(roleSel);
        if (wanted.empty() || mixRole != wanted)
            return false;
    }

    return true;
}

juce::Array<juce::File> PianoSynthAudioProcessorEditor::hostScanUserPresets()
{
    return proc.scanUserPresets();
}

bool PianoSynthAudioProcessorEditor::hostIsUserPreset()
{
    return proc.isCurrentPresetUser();
}

juce::File PianoSynthAudioProcessorEditor::hostCurrentUserFile()
{
    return proc.getCurrentUserPresetFile();
}

int PianoSynthAudioProcessorEditor::hostCurrentFactoryIdx()
{
    return proc.getCurrentFactoryPresetIndex();
}

void PianoSynthAudioProcessorEditor::hostApplyFactory(int idx)
{
    proc.applyFactoryPreset(idx);
}

void PianoSynthAudioProcessorEditor::hostLoadUser(const juce::File& f)
{
    proc.loadUserPreset(f);
}

bool PianoSynthAudioProcessorEditor::hostSaveUser(const juce::String& name)
{
    return proc.saveUserPreset(name);
}

void PianoSynthAudioProcessorEditor::hostUpdateUser(const juce::File& f)
{
    proc.updateUserPreset(f);
}

void PianoSynthAudioProcessorEditor::hostSaveFactory(int idx)
{
    proc.saveFactoryPreset(idx);
}

void PianoSynthAudioProcessorEditor::hostDeleteUser(const juce::File& f)
{
    proc.deleteUserPreset(f);
}

juce::File PianoSynthAudioProcessorEditor::hostGetUserPresetsDir()
{
    return PianoSynthAudioProcessor::getUserPresetsDirectory(proc.getSelectedPianoIndex());
}

juce::File PianoSynthAudioProcessorEditor::hostGetUserPresetsDirForIndex(int instrumentIndex)
{
    return PianoSynthAudioProcessor::getUserPresetsDirectory(instrumentIndex);
}

juce::String PianoSynthAudioProcessorEditor::hostPresetInstrumentAttr() const
{
    return "piano_index";
}

void PianoSynthAudioProcessorEditor::syncQuickBrowserState()
{
    quickTierSelector.setSelectedId(presetFamilyFilter.getSelectedId(), juce::dontSendNotification);
    quickRoleSelector.setSelectedId(presetRoleFilter.getSelectedId(), juce::dontSendNotification);
}

void PianoSynthAudioProcessorEditor::resetToPlaySectionAfterPresetChange()
{
    switchRightPanelSection(0);
}

void PianoSynthAudioProcessorEditor::applyQuickPresetShortcut(int tierId, int roleId)
{
    presetFamilyFilter.setSelectedId(tierId, juce::dontSendNotification);
    presetRoleFilter.setSelectedId(roleId, juce::dontSendNotification);
    syncQuickBrowserState();
    refreshPresetList();

    if (presetBox.getNumItems() == 0 && roleId != 1)
    {
        presetRoleFilter.setSelectedId(1, juce::dontSendNotification);
        syncQuickBrowserState();
        refreshPresetList();
    }

    resetToPlaySectionAfterPresetChange();
    syncCurrentPresetSummary();
}

void PianoSynthAudioProcessorEditor::syncCurrentPresetSummary()
{
    const int pianoIndex = selectedPianoFromParam();
    const auto family = mps::getFamily(pianoIndex);
    const auto pianoName = juce::String(mps::getPianoName(pianoIndex));
    auto presetName = presetBox.getText().trim();
    if (presetName.isEmpty())
        presetName = pianoName;

    juce::String metaText = familyDisplayName(family) + " / " + pianoName;
    if (hostIsUserPreset())
    {
        metaText = "[USER] " + metaText + " / Custom";
    }
    else
    {
        const int presetIndex = proc.getCurrentFactoryPresetIndex();
        const auto& banks = proc.getFactoryPresetBanksDirect();
        if (pianoIndex >= 0 && pianoIndex < static_cast<int>(banks.size()))
        {
            const auto& bank = banks[static_cast<std::size_t>(pianoIndex)];
            if (presetIndex >= 0 && presetIndex < static_cast<int>(bank.size()))
            {
                const auto& metadata = bank[static_cast<std::size_t>(presetIndex)].metadata;
                const auto mixRole = std::string_view(metadata.mixRole);
                metaText = "[" + presetTierBadge(mixRole) + "] "
                    + familyDisplayName(family)
                    + " / "
                    + juce::String(metadata.family)
                    + " / "
                    + titleCaseRole(mixRole);
            }
        }
    }

    currentPresetSummaryLabel.setText(presetName, juce::dontSendNotification);
    currentPresetMetaLabel.setText(metaText, juce::dontSendNotification);

    const auto summaryKey = presetName + "|" + metaText;
    if (cachedPresetSummaryKey.isNotEmpty() && cachedPresetSummaryKey != summaryKey)
        resetToPlaySectionAfterPresetChange();
    cachedPresetSummaryKey = summaryKey;
}

PianoSynthAudioProcessorEditor::PianoSynthAudioProcessorEditor(
    PianoSynthAudioProcessor& processor)
    : CommonSynthEditor(processor,
                        processor.getAPVTS(),
                        processor.getKeyboardState(),
                        juce::Colour(0xff1565C0),
                        36, 84, 38.0f)
    , proc(processor)
{
    familySelectorLbl.setText("PIANO TYPE", juce::dontSendNotification);
    modelSelectorLbl.setText("MODEL", juce::dontSendNotification);

    familySelector.addItem("CONCERT", 1);
    familySelector.addItem("VINTAGE / CHARACTER", 2);
    familySelector.addItem("ELECTRIC / PIANO-ADJACENT", 3);

    familySelector.onChange = [this]
    {
        const int familyIndex = juce::jlimit(0, mps::kNumFamilies - 1,
                                             familySelector.getSelectedId() - 1);
        activeFamilyIndex = familyIndex;
        rebuildModelSelectorForFamily(activeFamilyIndex);
        if (const int selectedId = modelSelector.getSelectedId(); selectedId > 0)
            pianoSelector.setSelectedId(selectedId);
    };

    modelSelector.onChange = [this]
    {
        if (const int selectedId = modelSelector.getSelectedId(); selectedId > 0)
            pianoSelector.setSelectedId(selectedId);
    };

    for (int familyIndex = 0; familyIndex < mps::kNumFamilies; ++familyIndex)
    {
        auto& tab = familyTabs[static_cast<std::size_t>(familyIndex)];
        tab.configure(familyIndex, familySelector.getItemText(familyIndex), familyColour(familyIndex));
        tab.onClicked = [this](int idx)
        {
            familySelector.setSelectedId(idx + 1, juce::sendNotificationSync);
        };
        tab.setVisible(false);
        addChildComponent(tab);
    }

    for (int pianoIndex = 0; pianoIndex < mps::kNumPianos; ++pianoIndex)
    {
        auto& card = presetCards[static_cast<std::size_t>(pianoIndex)];
        card.configure(pianoIndex, juce::String(juce::CharPointer_UTF8(mps::getPianoName(pianoIndex))), pianoCatColour(pianoIndex));
        card.onClicked = [this](int idx) { pianoSelector.setSelectedId(idx + 1); };
        addChildComponent(card);
    }

    pianoSelector.setVisible(false);
    addChildComponent(pianoSelector);
    for (int pianoIndex = 0; pianoIndex < mps::kNumPianos; ++pianoIndex)
        pianoSelector.addItem(juce::String(juce::CharPointer_UTF8(mps::getPianoName(pianoIndex))), pianoIndex + 1);
    selPianoAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "selected_piano", pianoSelector);
    pianoSelector.onChange = [this]
    {
        rebuildPianoAttachments();
        syncSelectionUiFromPiano();
    };

    for (int i = 0; i < kEnvN; ++i)
    {
        const auto index = static_cast<std::size_t>(i);
        switch (i)
        {
            case 0:
            case 2:
            case 5:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                setupDial(envDials[index], accent_);
                break;
            case 1:
                setupDial(envDials[index], accent_);
                break;
            case 3:
            case 4:
            case 6:
                setupDial(envDials[index], accent_);
                break;
            case 12:
                setupGrandDial(envDials[index], accent_, {});
                break;
            case 13:
                setupDial(envDials[index], accent_);
                break;
            default:
                setupDial(envDials[index], accent_);
                break;
        }

        addAndMakeVisible(envDials[index]);
        envLabels[index].setText(kEnvCtrls[index].label, juce::dontSendNotification);
        envLabels[index].setJustificationType(juce::Justification::centred);
        envLabels[index].setFont(juce::Font(juce::FontOptions{}.withHeight(10.4f)));
        envLabels[index].setColour(juce::Label::textColourId, synthcol::textSec.withAlpha(0.84f));
        addAndMakeVisible(envLabels[index]);
    }

    // --- Tooltip mode button (replaces inline tooltips) ---
    tooltipModeBtn.setButtonText("HELP: SHORT");
    tooltipModeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    tooltipModeBtn.setColour(juce::TextButton::textColourOffId, accent_);
    tooltipModeBtn.onClick = [this] { cycleTooltipMode(); };
    addAndMakeVisible(tooltipModeBtn);
    applyTooltips();

    envVisual.setAccent(accent_);
    envVisual.bindAdsr(&envDials[3], &envDials[4], &envDials[5], &envDials[6]);
    addAndMakeVisible(envVisual);

    lfoVisual.setAccent(accent_);
    lfoVisual.setTitle("LFO MOD");
    setupSmallDial(lfoRateDial, accent_);
    setupSmallDial(lfoDepthDial, accent_);
    addChildComponent(lfoRateDial);
    addChildComponent(lfoDepthDial);
    addChildComponent(lfoWaveSelector);
    lfoWaveSelector.addItem("SINE", 1);
    lfoWaveSelector.addItem("TRI", 2);
    lfoWaveSelector.addItem("SAW", 3);
    lfoWaveSelector.addItem("SQR", 4);
    lfoDestinationLabel.setText("DESTINATION", juce::dontSendNotification);
    lfoDestinationLabel.setJustificationType(juce::Justification::centredLeft);
    lfoDestinationLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    lfoDestinationLabel.setColour(juce::Label::textColourId, synthcol::textDim);
    lfoDestinationLabel.setMinimumHorizontalScale(0.75f);
    lfoDestinationSelector.addItem("OFF", 1);
    lfoDestinationSelector.addItem("TREMOLO", 2);
    lfoDestinationSelector.addItem("AUTO PAN", 3);
    lfoDestinationSelector.addItem("CHORUS", 4);
    lfoAdvancedButton.setButtonText("MOD MATRIX");
    lfoAdvancedButton.setClickingTogglesState(true);
    lfoAdvancedButton.onClick = [this]
    {
        advancedMotionVisible = lfoAdvancedButton.getToggleState();
        lfoAdvancedButton.setButtonText(advancedMotionVisible ? "MOD MATRIX ON" : "MOD MATRIX");
        syncFxAvailability();
        resized();
    };
    lfoRateAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "lfo_rate", lfoRateDial);
    lfoDepthAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "lfo_depth", lfoDepthDial);
    lfoWaveAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "lfo_wave", lfoWaveSelector);
    lfoDestinationAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "lfo_destination", lfoDestinationSelector);
    lfoVisual.bindRateDepth(&lfoRateDial, &lfoDepthDial);
    lfoVisual.setWaveformIndex(juce::jmax(0, lfoWaveSelector.getSelectedId() - 1));
    lfoWaveSelector.onChange = [this]
    {
        lfoVisual.setWaveformIndex(juce::jmax(0, lfoWaveSelector.getSelectedId() - 1));
    };
    lfoDestinationSelector.onChange = [this]
    {
        applyTooltips();
        syncFxAvailability();
        resized();
    };
    lfoVisual.onWaveformChanged = [this](int waveformIndex)
    {
        lfoWaveSelector.setSelectedId(waveformIndex + 1, juce::sendNotificationSync);
    };
    addAndMakeVisible(lfoVisual);
    addChildComponent(lfoDestinationLabel);
    addChildComponent(lfoDestinationSelector);
    addAndMakeVisible(lfoAdvancedButton);

    // --- Mod Matrix Panel (H3) ---
    modMatrixTitle.setText("MOD MATRIX", juce::dontSendNotification);
    modMatrixTitle.setJustificationType(juce::Justification::centredLeft);
    modMatrixTitle.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f).withStyle("Bold")));
    modMatrixTitle.setColour(juce::Label::textColourId, accent_);
    addChildComponent(modMatrixTitle);

    juce::StringArray mmSrcNames;
    for (int s = 0; s < static_cast<int>(modmatrix::Source::Count); ++s)
        mmSrcNames.add(modmatrix::getSourceName(static_cast<modmatrix::Source>(s)));
    juce::StringArray mmDstNames;
    for (int d = 0; d < static_cast<int>(modmatrix::Destination::Count); ++d)
        mmDstNames.add(modmatrix::getDestinationName(static_cast<modmatrix::Destination>(d)));
    for (int i = 0; i < 8; ++i)
    {
        auto& r = modRows[static_cast<std::size_t>(i)];
        for (int j = 0; j < mmSrcNames.size(); ++j) r.srcCombo.addItem(mmSrcNames[j], j + 1);
        for (int j = 0; j < mmDstNames.size(); ++j) r.dstCombo.addItem(mmDstNames[j], j + 1);
        r.amtSlider.setRange(-1.0, 1.0, 0.01);
        r.amtSlider.setSliderStyle(juce::Slider::LinearBar);
        r.amtSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        const int ri = i;
        r.srcCombo.onChange = [this, ri] { flushModRow(ri); };
        r.dstCombo.onChange = [this, ri] { flushModRow(ri); };
        r.amtSlider.onValueChange = [this, ri] { flushModRow(ri); };
        addChildComponent(r.srcCombo);
        addChildComponent(r.dstCombo);
        addChildComponent(r.amtSlider);
    }

    for (int i = 0; i < kMacroTotal; ++i)
    {
        const auto index = static_cast<std::size_t>(i);
        macroAtt[index] = std::make_unique<SliderAttach>(proc.getAPVTS(), kMacroCtrls[index].paramId, macroDials[index]);
        setupDial(macroDials[index], accent_);
        addAndMakeVisible(macroDials[index]);
        macroLbls[index].setText(kMacroCtrls[index].label, juce::dontSendNotification);
        macroLbls[index].setJustificationType(juce::Justification::centred);
        macroLbls[index].setFont(juce::Font(juce::FontOptions{}.withHeight(10.4f)));
        macroLbls[index].setColour(juce::Label::textColourId, synthcol::textSec.withAlpha(0.84f));
        addAndMakeVisible(macroLbls[index]);
    }

    for (int sectionIndex = 0; sectionIndex < kRightPanelSections; ++sectionIndex)
    {
        auto& tab = rightPanelTabs[static_cast<std::size_t>(sectionIndex)];
        tab.configure(sectionIndex, kRightPanelSectionLabels[sectionIndex], accent_);
        tab.setSelected(sectionIndex == activeRightPanelSection);
        tab.onClicked = [this](int idx) { switchRightPanelSection(idx); };
        addAndMakeVisible(tab);
    }

    for (int i = 0; i < kFxN; ++i)
    {
        const auto index = static_cast<std::size_t>(i);
        fxAtt[index] = std::make_unique<SliderAttach>(proc.getAPVTS(), kFxCtrls[index].paramId, fxDials[index]);
        setupSmallDial(fxDials[index], accent_);
        addChildComponent(fxDials[index]);
        fxLbls[index].setText(kFxCtrls[index].label, juce::dontSendNotification);
        fxLbls[index].setJustificationType(juce::Justification::centred);
        fxLbls[index].setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
        fxLbls[index].setColour(juce::Label::textColourId, synthcol::textSec.withAlpha(0.84f));
        addChildComponent(fxLbls[index]);
    }

    for (int tabIndex = 0; tabIndex < kFxTabs; ++tabIndex)
    {
        auto& item = fxRackItems[static_cast<std::size_t>(tabIndex)];
        item.configure(tabIndex, kFxTabNames[tabIndex], kFxRackSummaries[tabIndex], accent_);
        item.onClicked = [this](int clickedIndex) { switchEffectTab(clickedIndex); };
        addAndMakeVisible(item);
    }

    for (int tabIndex = 0; tabIndex < kFxTabs; ++tabIndex)
    {
        auto& button = fxBypassBtns[static_cast<std::size_t>(tabIndex)];
        button.setButtonText("ON");
        button.setClickingTogglesState(true);
        button.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(button);
        fxBypassAtts[static_cast<std::size_t>(tabIndex)] = std::make_unique<BtnAttach>(
            proc.getAPVTS(), kFxBypassParamIds[tabIndex], button);
        button.onClick = [this] { syncFxRackState(); };
    }

    fxDetailTitle.setJustificationType(juce::Justification::centredLeft);
    fxDetailTitle.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    fxDetailTitle.setColour(juce::Label::textColourId, synthcol::textSec);
    addAndMakeVisible(fxDetailTitle);

    fxUnavailableLbl.setText("Unavailable for this model", juce::dontSendNotification);
    fxUnavailableLbl.setJustificationType(juce::Justification::centred);
    fxUnavailableLbl.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    fxUnavailableLbl.setColour(juce::Label::textColourId, synthcol::textDim.withAlpha(0.55f));
    addChildComponent(fxUnavailableLbl);

    // --- Reverb type selector (Plate / Hall / Room / Chamber) ---
    reverbTypeSelector.addItem("Plate",   1);
    reverbTypeSelector.addItem("Hall",    2);
    reverbTypeSelector.addItem("Room",    3);
    reverbTypeSelector.addItem("Chamber", 4);
    reverbTypeSelector.setTooltip("Reverb algorithm: Plate (classic), Hall, Room or Chamber convolution");
    reverbTypeAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "reverb_type", reverbTypeSelector);
    reverbTypeSelector.onChange = [this]
    {
        applyTooltips();
        syncFxAvailability();
        resized();
    };
    addChildComponent(reverbTypeSelector);
    reverbTypeLabel.setText("TYPE", juce::dontSendNotification);
    reverbTypeLabel.setJustificationType(juce::Justification::centredLeft);
    reverbTypeLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    addChildComponent(reverbTypeLabel);

    delaySyncButton.setButtonText("SYNC HOST");
    addChildComponent(delaySyncButton);
    delaySyncAtt = std::make_unique<BtnAttach>(proc.getAPVTS(), "delay_sync", delaySyncButton);

    delayNoteDivLabel.setText("DIVISION", juce::dontSendNotification);
    delayNoteDivLabel.setJustificationType(juce::Justification::centredLeft);
    delayNoteDivLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    delayNoteDivLabel.setColour(juce::Label::textColourId, synthcol::textDim);
    addChildComponent(delayNoteDivLabel);

    delayNoteDivSelector.addItem("1/4", 1);
    delayNoteDivSelector.addItem("1/8", 2);
    delayNoteDivSelector.addItem("DOTTED 1/8", 3);
    delayNoteDivSelector.addItem("TRIPLET 1/8", 4);
    delayNoteDivSelector.addItem("1/16", 5);
    delayNoteDivSelector.addItem("DOTTED 1/16", 6);
    addChildComponent(delayNoteDivSelector);
    delayNoteDivAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "delay_division", delayNoteDivSelector);

    fxLockButton.setButtonText("FX LOCK");
    addAndMakeVisible(fxLockButton);
    fxLockAtt = std::make_unique<BtnAttach>(proc.getAPVTS(), "fx_lock", fxLockButton);

    // --- Performance: velocity curve ---
    velocityCurveSelector.addItem("Linear",  1);
    velocityCurveSelector.addItem("Soft",    2);
    velocityCurveSelector.addItem("Softer",  3);
    velocityCurveSelector.addItem("Hard",    4);
    velocityCurveSelector.addItem("Harder",  5);
    velocityCurveSelector.addItem("Fixed",   6);
    velocityCurveSelector.addItem("Touch",   7);
    velocityCurveSelector.setTooltip("Velocity Curve — Linear: direct, Soft: more dynamics a low velocities, Hard: requires stronger playing, Fixed: ignores velocity");
    velocityCurveAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "velocity_curve", velocityCurveSelector);
    addAndMakeVisible(velocityCurveSelector);
    velocityCurveLabel.setText("VELOCITY", juce::dontSendNotification);
    velocityCurveLabel.setJustificationType(juce::Justification::centredLeft);
    velocityCurveLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    velocityCurveLabel.setColour(juce::Label::textColourId, synthcol::textDim);
    velocityCurveLabel.setMinimumHorizontalScale(0.75f);
    addAndMakeVisible(velocityCurveLabel);

    // --- Performance: pitch bend range ---
    pitchBendRangeDial.setSliderStyle(juce::Slider::LinearBar);
    pitchBendRangeDial.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 22);
    pitchBendRangeDial.setTooltip("Pitch Bend Range (semitones) — 2: standard, 12-24: for Rhodes/Clavinet expression");
    pitchBendRangeAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "pitch_bend_range", pitchBendRangeDial);
    addAndMakeVisible(pitchBendRangeDial);
    pitchBendRangeLabel.setText("BEND", juce::dontSendNotification);
    pitchBendRangeLabel.setJustificationType(juce::Justification::centredLeft);
    pitchBendRangeLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    pitchBendRangeLabel.setColour(juce::Label::textColourId, synthcol::textDim);
    pitchBendRangeLabel.setMinimumHorizontalScale(0.75f);
    addAndMakeVisible(pitchBendRangeLabel);

    // --- Mono mode ---
    monoModeButton.setButtonText("MONO");
    monoModeButton.setClickingTogglesState(true);
    monoModeButton.setTooltip("Mono mode — each new note mutes previous note");
    monoModeAtt = std::make_unique<BtnAttach>(proc.getAPVTS(), "mono_mode", monoModeButton);
    addAndMakeVisible(monoModeButton);

    tremoloSyncButton.setButtonText("TREM SYNC");
    tremoloSyncButton.setClickingTogglesState(true);
    tremoloSyncButton.setTooltip("Electric only: sync tremolo rate to host BPM");
    tremoloSyncAtt = std::make_unique<BtnAttach>(proc.getAPVTS(), "tremolo_sync", tremoloSyncButton);
    addChildComponent(tremoloSyncButton);

    aftertouchStatusLabel.setText("AFTERTOUCH -> LEVEL (+25% max)", juce::dontSendNotification);
    aftertouchStatusLabel.setJustificationType(juce::Justification::centredLeft);
    aftertouchStatusLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.5f).withStyle("Bold")));
    aftertouchStatusLabel.setMinimumHorizontalScale(0.72f);
    addChildComponent(aftertouchStatusLabel);

    // --- Preset randomization (M4) ---
    randButton.setButtonText("RAND");
    randButton.setTooltip("Randomize synthesis parameters ±15%");
    randButton.onClick = [this] { proc.randomizePreset(); };
    addAndMakeVisible(randButton);

    // --- Undo / Redo ---
    undoButton.setButtonText(juce::CharPointer_UTF8("\xe2\x86\xba")); // ↺
    undoButton.setTooltip("Undo last parameter change (Ctrl+Z)");
    undoButton.onClick = [this] { proc.getUndoManager().undo(); };
    addAndMakeVisible(undoButton);

    redoButton.setButtonText(juce::CharPointer_UTF8("\xe2\x86\xbb")); // ↻
    redoButton.setTooltip("Redo last undone change (Ctrl+Shift+Z)");
    redoButton.onClick = [this] { proc.getUndoManager().redo(); };
    addAndMakeVisible(redoButton);

    // --- Voice count label (M1) ---
    voiceCountLabel.setText("Voices: 0", juce::dontSendNotification);
    voiceCountLabel.setJustificationType(juce::Justification::centredLeft);
    voiceCountLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.5f).withStyle("Bold")));
    voiceCountLabel.setColour(juce::Label::textColourId, synthcol::textSec);
    addAndMakeVisible(voiceCountLabel);

    // --- Arpeggiator controls ---
    arpModeSelector.addItem("Up",      1);
    arpModeSelector.addItem("Down",    2);
    arpModeSelector.addItem("Up-Down", 3);
    arpModeSelector.addItem("Random",  4);
    arpModeSelector.setTooltip("Arpeggiator direction");
    arpModeAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "arp_mode", arpModeSelector);
    addChildComponent(arpModeSelector);
    arpModeLabel.setText("MODE", juce::dontSendNotification);
    arpModeLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    arpModeLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(arpModeLabel);

    arpRateSelector.addItem("1/4",   1);
    arpRateSelector.addItem("1/8",   2);
    arpRateSelector.addItem("1/8T",  3);
    arpRateSelector.addItem("1/16",  4);
    arpRateSelector.addItem("1/16T", 5);
    arpRateSelector.setTooltip("Arpeggiator rate (relative to host BPM)");
    arpRateAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "arp_rate", arpRateSelector);
    addChildComponent(arpRateSelector);
    arpRateLabel.setText("RATE", juce::dontSendNotification);
    arpRateLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    arpRateLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(arpRateLabel);

    arpOctavesDial.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    arpOctavesDial.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 36, 14);
    arpOctavesDial.setTooltip("Arpeggiator range in octaves (1-4)");
    arpOctavesAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "arp_octaves", arpOctavesDial);
    addChildComponent(arpOctavesDial);
    arpOctavesLabel.setText("OCT", juce::dontSendNotification);
    arpOctavesLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    arpOctavesLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(arpOctavesLabel);

    arpGateDial.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    arpGateDial.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 36, 14);
    arpGateDial.setTooltip("Arpeggiator gate (note length fraction)");
    arpGateAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "arp_gate", arpGateDial);
    addChildComponent(arpGateDial);
    arpGateLabel.setText("GATE", juce::dontSendNotification);
    arpGateLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    arpGateLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(arpGateLabel);

    arpHoldButton.setButtonText("HOLD");
    arpHoldButton.setTooltip("Hold: keep arpeggiating after keys released");
    arpHoldAtt = std::make_unique<BtnAttach>(proc.getAPVTS(), "arp_hold", arpHoldButton);
    addChildComponent(arpHoldButton);

    currentPresetSummaryLabel.setText("Velvet Stern", juce::dontSendNotification);
    currentPresetSummaryLabel.setJustificationType(juce::Justification::centredLeft);
    currentPresetSummaryLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(15.0f).withStyle("Bold")));
    currentPresetSummaryLabel.setMinimumHorizontalScale(0.75f);
    addAndMakeVisible(currentPresetSummaryLabel);

    currentPresetMetaLabel.setText("[DRY] Concert / Steinway D / Reference", juce::dontSendNotification);
    currentPresetMetaLabel.setJustificationType(juce::Justification::centredLeft);
    currentPresetMetaLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.6f).withStyle("Bold")));
    currentPresetMetaLabel.setMinimumHorizontalScale(0.72f);
    addAndMakeVisible(currentPresetMetaLabel);

    quickTierSelector.addItem("Any Preset", 1);
    quickTierSelector.addItem("Dry", 2);
    quickTierSelector.addItem("Featured", 3);
    quickTierSelector.addItem("Mix", 4);
    quickTierSelector.setSelectedId(1, juce::dontSendNotification);
    quickTierSelector.setTooltip("Quick preset focus for the curated factory tiers.");
    quickTierSelector.onChange = [this]
    {
        presetFamilyFilter.setSelectedId(quickTierSelector.getSelectedId(), juce::dontSendNotification);
        refreshPresetList();
        resetToPlaySectionAfterPresetChange();
    };
    addAndMakeVisible(quickTierSelector);

    quickRoleSelector.addItem("Any Role", 1);
    quickRoleSelector.addItem("Reference", 2);
    quickRoleSelector.addItem("Close", 3);
    quickRoleSelector.addItem("Featured", 4);
    quickRoleSelector.addItem("Warm", 5);
    quickRoleSelector.addItem("Cut", 6);
    quickRoleSelector.addItem("Groove", 7);
    quickRoleSelector.addItem("Character", 8);
    quickRoleSelector.setSelectedId(1, juce::dontSendNotification);
    quickRoleSelector.setTooltip("Role focus for curated factory presets.");
    quickRoleSelector.onChange = [this]
    {
        presetRoleFilter.setSelectedId(quickRoleSelector.getSelectedId(), juce::dontSendNotification);
        refreshPresetList();
        resetToPlaySectionAfterPresetChange();
    };
    addAndMakeVisible(quickRoleSelector);

    quickReferenceBtn.setButtonText("DRY");
    quickReferenceBtn.setTooltip("Jump to the reference dry preset.");
    quickReferenceBtn.onClick = [this] { applyQuickPresetShortcut(2, 2); };
    addAndMakeVisible(quickReferenceBtn);

    quickMixBtn.setButtonText("MIX");
    quickMixBtn.setTooltip("Show the mix-ready curated presets.");
    quickMixBtn.onClick = [this] { applyQuickPresetShortcut(4, 1); };
    addAndMakeVisible(quickMixBtn);

    quickCinematicBtn.setButtonText("ALL");
    quickCinematicBtn.setTooltip("Show all preset entries.");
    quickCinematicBtn.onClick = [this] { applyQuickPresetShortcut(1, 1); };
    addAndMakeVisible(quickCinematicBtn);

    // --- Preset browser filters ---
    presetFamilyFilter.addItem("All Presets", 1);
    presetFamilyFilter.addItem("Dry", 2);
    presetFamilyFilter.addItem("Featured", 3);
    presetFamilyFilter.addItem("Mix", 4);
    presetFamilyFilter.setSelectedId(1, juce::dontSendNotification);
    presetFamilyFilter.setTooltip("Filter the browser by product tier");
    presetFamilyFilter.onChange = [this]
    {
        syncQuickBrowserState();
        refreshPresetList();
    };
    addAndMakeVisible(presetFamilyFilter);

    presetRoleFilter.addItem("All Roles", 1);
    presetRoleFilter.addItem("Reference", 2);
    presetRoleFilter.addItem("Close", 3);
    presetRoleFilter.addItem("Featured", 4);
    presetRoleFilter.addItem("Warm", 5);
    presetRoleFilter.addItem("Cut", 6);
    presetRoleFilter.addItem("Groove", 7);
    presetRoleFilter.addItem("Character", 8);
    presetRoleFilter.setSelectedId(1, juce::dontSendNotification);
    presetRoleFilter.setTooltip("Filter the browser by explicit preset role");
    presetRoleFilter.onChange = [this]
    {
        syncQuickBrowserState();
        refreshPresetList();
    };
    addAndMakeVisible(presetRoleFilter);

    presetFilterLabel.setText("BROWSER:", juce::dontSendNotification);
    presetFilterLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.5f).withStyle("Bold")));
    presetFilterLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetFilterLabel);

    // --- MIDI Learn button ---
    midiLearnButton.setButtonText("MIDI LEARN");
    midiLearnButton.setClickingTogglesState(true);
    midiLearnButton.setTooltip("Toggle MIDI Learn: arm a parameter then move a controller CC");
    midiLearnButton.onClick = [this]
    {
        const bool on = midiLearnButton.getToggleState();
        if (on)
            showMidiLearnPanel(true);
        else
            showMidiLearnPanel(false);
    };
    addAndMakeVisible(midiLearnButton);
    midiLearnPanel.clearAllBtn.onClick = [this] { proc.midiLearnClearAll(); refreshMidiLearnPanel(); };
    midiLearnPanel.addAndMakeVisible(midiLearnPanel.clearAllBtn);

    // --- MIDI CC page indicator (FLkey Mini) ---
    midiCCPageLabel.setText("MIDI CC: MACROS", juce::dontSendNotification);
    midiCCPageLabel.setJustificationType(juce::Justification::centred);
    midiCCPageLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.5f).withStyle("Bold")));
    midiCCPageLabel.setColour(juce::Label::textColourId, accent_);
    midiCCPageLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1a1a1a));
    midiCCPageLabel.setColour(juce::Label::outlineColourId, accent_.withAlpha(0.35f));
    addAndMakeVisible(midiCCPageLabel);

    applyStaticValueFormatters();
    presetSearch.setTextToShowWhenEmpty("Search preset, role, tier...", synthcol::textDim.withAlpha(0.70f));

    rebuildPianoAttachments();
    syncSelectionUiFromPiano();
    syncQuickBrowserState();
    syncCurrentPresetSummary();
    syncFxAvailability();
    switchEffectTab(0);

    backgroundImage_ = juce::ImageCache::getFromMemory(
        BinaryData::fond_piano_png, BinaryData::fond_piano_pngSize);

    applyPianoTheme(selectedPianoFromParam());
    initCommon();

    setWantsKeyboardFocus(true);
    startTimerHz(30);
    setResizable(true, true);
    setResizeLimits(kMinEditorW, kMinEditorH, 2560, 1600);
    setSize(lay::W, lay::H);
}

// FIX 4.1: Quick Piano Switch via keyboard shortcuts
bool PianoSynthAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    const auto ctrl = juce::ModifierKeys::commandModifier;
    const auto ctrlShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;

    if (key == juce::KeyPress('z', ctrl, 0))
    {
        proc.getUndoManager().undo();
        return true;
    }
    if (key == juce::KeyPress('z', ctrlShift, 0))
    {
        proc.getUndoManager().redo();
        return true;
    }
    
    // Quick Piano Switch: 1 = Concert, 2 = Vintage, 3 = Electric
    if (key == juce::KeyPress('1', 0, 0))
    {
        rebuildModelSelectorForFamily(0);
        return true;
    }
    if (key == juce::KeyPress('2', 0, 0))
    {
        rebuildModelSelectorForFamily(1);
        return true;
    }
    if (key == juce::KeyPress('3', 0, 0))
    {
        rebuildModelSelectorForFamily(2);
        return true;
    }
    
    return false;
}

void PianoSynthAudioProcessorEditor::timerCallback()
{
    rebuildPianoAttachments();
    syncSelectionUiFromPiano();
    syncQuickBrowserState();
    syncFxAvailability();
    syncPresetBox();
    syncCurrentPresetSummary();

    // H3: sync mod matrix rows when panel is visible
    if (advancedMotionVisible)
    {
        syncModMatrixUi();
        if (activeRightPanelSection == 1)
            lfoVisual.setVisible(false);
    }

    // M1: update voice count display
    voiceCountLabel.setText("Voices: " + juce::String(proc.getActiveVoiceCount()),
                            juce::dontSendNotification);

    // Undo/Redo button state
    undoButton.setEnabled(proc.getUndoManager().canUndo());
    redoButton.setEnabled(proc.getUndoManager().canRedo());

    // MIDI Learn: refresh panel when visible and learn state changes
    if (midiLearnPanelVisible)
    {
        if (!proc.isMidiLearnActive() && midiLearnButton.getToggleState())
            refreshMidiLearnPanel();
    }

    const int page = proc.getMidiCCPage();
    if (page != cachedMidiCCPage)
    {
        cachedMidiCCPage = page;
        midiCCPageLabel.setText("MIDI CC: " + juce::String(PianoSynthAudioProcessor::getCCPageName(page)),
                                juce::dontSendNotification);
    }
}

void PianoSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    paintBackground(g);

    const auto layout = computeLayoutMetrics(getWidth(), getHeight());
    const auto headerZones = computeHeaderZones(layout.headerH);
    const auto headerSpec = computeHeaderLayoutSpec(layout, headerZones);
    const auto selectorRect = juce::Rectangle<float>(static_cast<float>(layout.contentX),
                                                     static_cast<float>(layout.selectorY),
                                                     static_cast<float>(layout.contentW),
                                                     static_cast<float>(layout.selectorH - 8));
    const auto col1Rect = juce::Rectangle<float>(static_cast<float>(layout.col1X),
                                                 static_cast<float>(layout.bodyY),
                                                 static_cast<float>(layout.colW),
                                                 static_cast<float>(layout.bodyH));
    const auto col2Rect = juce::Rectangle<float>(static_cast<float>(layout.col2X),
                                                 static_cast<float>(layout.bodyY),
                                                 static_cast<float>(layout.colW),
                                                 static_cast<float>(layout.bodyH));
    const auto col3Rect = juce::Rectangle<float>(static_cast<float>(layout.col3X),
                                                 static_cast<float>(layout.bodyY),
                                                 static_cast<float>(layout.colW),
                                                 static_cast<float>(layout.bodyH));
    const auto keyboardRect = juce::Rectangle<float>(static_cast<float>(layout.contentX),
                                                     static_cast<float>(layout.kbY),
                                                     static_cast<float>(layout.contentW),
                                                     static_cast<float>(layout.kbH));
    const auto statusMeterRow = headerSpec.statusMeterRow;
    const auto voiceLoad = juce::jlimit(0.0f, 1.0f,
                                        static_cast<float>(proc.getActiveVoiceCount())
                                            / static_cast<float>(PianoSynthAudioProcessor::kMaxVoices));
    const auto outputLoad = static_cast<float>(juce::jmap(gainDial.getValue(), -24.0, 12.0, 0.0, 1.0));

    auto paintHeaderLane = [&](juce::Rectangle<float> area, juce::Colour tint)
    {
        constexpr float laneRadius = 7.5f;
        const auto laneTop = juce::Colour(0xff171C23).interpolatedWith(tint, 0.16f);
        const auto laneMid = juce::Colour(0xff11161D).interpolatedWith(tint, 0.10f);
        const auto laneBottom = juce::Colour(0xff0B1016).interpolatedWith(tint, 0.06f);
        juce::ColourGradient laneGrad(laneTop, area.getCentreX(), area.getY(),
                                      laneBottom, area.getCentreX(), area.getBottom(), false);
        laneGrad.addColour(0.45, laneMid);
        g.setGradientFill(laneGrad);
        g.fillRoundedRectangle(area, laneRadius);

        auto sheen = area.reduced(1.0f).withHeight(juce::jmax(9.0f, area.getHeight() * 0.30f));
        juce::ColourGradient sheenGrad(juce::Colours::white.withAlpha(0.018f), sheen.getCentreX(), sheen.getY(),
                                       juce::Colours::transparentWhite, sheen.getCentreX(), sheen.getBottom(), false);
        g.setGradientFill(sheenGrad);
        g.fillRoundedRectangle(sheen, juce::jmax(0.0f, laneRadius - 1.0f));

        g.setColour(juce::Colours::white.withAlpha(0.026f));
        g.drawRoundedRectangle(area.reduced(0.5f), laneRadius, 0.8f);
        g.setColour(juce::Colours::black.withAlpha(0.18f));
        g.drawRoundedRectangle(area.expanded(0.25f), laneRadius + 0.4f, 0.8f);
    };

    auto paintLaneDivider = [&](const juce::Rectangle<int>& zone, const int dividerY)
    {
        const float x = static_cast<float>(zone.getX() + 14);
        const float right = static_cast<float>(zone.getRight() - 14);
        if (right <= x)
            return;

        const float y = static_cast<float>(dividerY);
        g.setColour(juce::Colours::white.withAlpha(0.024f));
        g.drawLine(x, y, right, y, 0.8f);
        g.setColour(juce::Colours::black.withAlpha(0.18f));
        g.drawLine(x, y + 1.0f, right, y + 1.0f, 0.9f);
    };

    auto paintSelectorPanel = [&](juce::Rectangle<float> area)
    {
        constexpr float radius = 8.0f;
        g.setColour(juce::Colours::black.withAlpha(0.16f));
        g.fillRoundedRectangle(area.translated(0.0f, 3.0f), radius);

        juce::ColourGradient panelGrad(juce::Colour(0xff161B22), area.getCentreX(), area.getY(),
                                       juce::Colour(0xff0B1016), area.getCentreX(), area.getBottom(), false);
        panelGrad.addColour(0.48, juce::Colour(0xff10151C));
        g.setGradientFill(panelGrad);
        g.fillRoundedRectangle(area, radius);

        auto sheen = area.reduced(1.0f).withHeight(juce::jmax(10.0f, area.getHeight() * 0.36f));
        juce::ColourGradient sheenGrad(juce::Colours::white.withAlpha(0.018f), sheen.getCentreX(), sheen.getY(),
                                       juce::Colours::transparentWhite, sheen.getCentreX(), sheen.getBottom(), false);
        g.setGradientFill(sheenGrad);
        g.fillRoundedRectangle(sheen, juce::jmax(0.0f, radius - 1.0f));

        g.setColour(juce::Colour(0xff69717C).withAlpha(0.10f));
        g.drawRoundedRectangle(area.reduced(0.5f), radius, 0.8f);

        auto titleArea = juce::Rectangle<int>(static_cast<int>(area.getX()) + 14,
                                              static_cast<int>(area.getY()) + 8,
                                              static_cast<int>(area.getWidth()) - 28,
                                              16);
        g.setColour(synthcol::text.withAlpha(0.90f));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(12.0f).withStyle("Bold")));
        g.drawText("Quick Piano Select", titleArea, juce::Justification::centredLeft);

        auto titleTrace = juce::Rectangle<float>(area.getX() + 14.0f, area.getY() + 26.0f,
                                                 juce::jmin(92.0f, area.getWidth() * 0.16f), 1.6f);
        juce::ColourGradient traceGrad(accent_.withAlpha(0.24f), titleTrace.getX(), titleTrace.getCentreY(),
                                       juce::Colours::transparentBlack, titleTrace.getRight(), titleTrace.getCentreY(), false);
        g.setGradientFill(traceGrad);
        g.fillRoundedRectangle(titleTrace, 0.8f);
    };

    paintHeader(g, layout.headerH);
    paintHeaderLane(headerZones.presetZone.toFloat().reduced(1.0f, 1.0f), accent_.withAlpha(0.12f));
    paintHeaderLane(headerZones.statusZone.toFloat().reduced(1.0f, 1.0f), accent_.withAlpha(0.10f));
    if (headerZones.responsiveRows)
    {
        if (!headerZones.fullWidthPrimaryRow.isEmpty())
            paintHeaderLane(headerZones.fullWidthPrimaryRow.toFloat().reduced(1.0f, 1.0f), accent_.withAlpha(0.11f));
        if (!headerZones.fullWidthSecondaryRow.isEmpty())
            paintHeaderLane(headerZones.fullWidthSecondaryRow.toFloat().reduced(1.0f, 1.0f), accent_.withAlpha(0.09f));
    }
    else
    {
        paintLaneDivider(headerZones.presetZone, headerZones.presetSecondaryRow.getY() - 1);
        paintLaneDivider(headerZones.statusZone, headerZones.statusSecondaryRow.getY() - 1);
    }

    paintSelectorPanel(selectorRect);
    paintCard(g, layout.col1X, layout.bodyY, layout.colW, layout.bodyH, "Core Response");
    paintCard(g, layout.col2X, layout.bodyY, layout.colW, layout.bodyH, "Piano Voice");
    paintCard(g, layout.col3X, layout.bodyY, layout.colW, layout.bodyH, "Play / Space");
    glazePianoChrome(g, col1Rect.reduced(2.0f, 2.0f), 10.0f, 0.94f);
    glazePianoChrome(g, col2Rect.reduced(2.0f, 2.0f), 10.0f, 0.94f);
    glazePianoChrome(g, col3Rect.reduced(2.0f, 2.0f), 10.0f, 0.94f);

    const int meterY = statusMeterRow.getY() + 8;
    const int meterLeft = statusMeterRow.getRight() - 108;
    if (!headerSpec.usesResponsiveRows && meterLeft >= statusMeterRow.getX() + 164)
    {
        paintMeterBar(g, { meterLeft, meterY, 50, 8 }, voiceLoad, accent_);
        paintMeterBar(g, { meterLeft + 56, meterY, 50, 8 }, outputLoad, accent_.brighter(0.22f));
        g.setColour(synthcol::textDim);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
        g.drawText("V", juce::Rectangle<int>(meterLeft - 10, meterY - 2, 10, 12), juce::Justification::centredLeft);
        g.drawText("G", juce::Rectangle<int>(meterLeft + 46, meterY - 2, 10, 12), juce::Justification::centredLeft);
    }

    g.setColour(accent_.withAlpha(0.12f));
    g.drawLine(static_cast<float>(layout.contentX + 18), static_cast<float>(layout.kbY - 8),
               static_cast<float>(layout.contentX + layout.contentW - 18), static_cast<float>(layout.kbY - 8),
               1.0f);

    paintKeyboardDock(g, layout.contentX, layout.kbY, layout.contentW, layout.kbH);
    glazePianoChrome(g, keyboardRect.reduced(2.0f, 2.0f), 11.0f, 1.02f);
}

void PianoSynthAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const auto layout = computeLayoutMetrics(w, getHeight());
    const float gapDensity = layoutDensity(layout.compact, layout.roomy);
    const auto headerZones = computeHeaderZones(layout.headerH);
    const auto headerSpec = computeHeaderLayoutSpec(layout, headerZones);
    const bool ultraCompact = layout.ultraCompact;

    auto centeredY = [] (const juce::Rectangle<int>& area, const int height)
    {
        return area.getY() + juce::jmax(0, (area.getHeight() - height) / 2);
    };

    quickReferenceBtn.setButtonText(headerSpec.usesCompactCaptions ? "NAT" : "NATURAL");
    quickMixBtn.setButtonText(headerSpec.usesCompactCaptions ? "SIG" : "SIGNATURE");
    quickCinematicBtn.setButtonText("ALL");
    midiLearnButton.setButtonText(headerSpec.usesCompactCaptions ? "MIDI" : "MIDI LEARN");
    presetSearch.setTextToShowWhenEmpty(ultraCompact ? "Search..." : "Search preset, role, tier...",
                                        synthcol::textDim.withAlpha(0.70f));
    tooltipModeBtn.setButtonText(headerSpec.usesCompactCaptions
        ? "HELP"
        : tooltipMode == TooltipMode::Off ? "HELP: OFF"
                                          : tooltipMode == TooltipMode::Short ? "HELP: SHORT"
                                                                              : "HELP: GUIDED");

    auto layoutBrowserRow = [&](const juce::Rectangle<int>& row)
    {
        const bool wide = headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Wide;
        const int searchMinW = ultraCompact ? 94
                                    : wide ? (layout.compact ? 120 : 136)
                                    : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 150 : 132);
        int searchW = ultraCompact ? juce::jlimit(94, 118, row.getWidth() / 5)
                           : wide ? juce::jlimit(layout.compact ? 120 : 136,
                                          layout.compact ? 166 : 196,
                                          row.getWidth() / 4)
                           : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 164 : 146);
        const int saveW = ultraCompact ? 44
                               : wide ? (layout.compact ? 52 : 60)
                               : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 52 : 48);
        const int saveAsW = ultraCompact ? 54
                                 : wide ? (layout.compact ? 64 : 74)
                                 : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 62 : 58);
        const int deleteW = ultraCompact ? 48
                                 : wide ? (layout.compact ? 56 : 66)
                                 : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 56 : 52);
        const int importW = ultraCompact ? 50
                                 : wide ? (layout.compact ? 58 : 68)
                                 : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 56 : 52);
        const int randW = ultraCompact ? 42
                               : wide ? (layout.compact ? 54 : 62)
                               : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 50 : 46);
        const int midiLearnW = ultraCompact ? 42
                                    : wide ? (layout.compact ? 76 : 86)
                                    : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 50 : 46);
        const int actionBlockW = saveW + saveAsW + deleteW + importW + randW + midiLearnW + headerSpec.actionGap * 5;
        const int navGap = headerSpec.usesResponsiveRows ? 4 : 8;
        const int actionY = centeredY(row, headerSpec.actionButtonH);
        auto browserArea = row.withTrimmedRight(actionBlockW + (headerSpec.usesResponsiveRows ? 10 : 12)).reduced(0, 1);
        const int browserControlH = juce::jmax(ultraCompact ? 20 : 22, headerSpec.controlH - 1);
        const int presetMinW = ultraCompact ? 150
                                    : wide ? (layout.compact ? 190 : 250)
                                    : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 220 : 170);
        int availableForPreset = browserArea.getWidth() - searchW - headerSpec.navW * 2 - navGap * 2;
        if (availableForPreset < presetMinW && searchW > searchMinW)
        {
            const int reclaim = juce::jmin(presetMinW - availableForPreset, searchW - searchMinW);
            searchW -= reclaim;
            availableForPreset += reclaim;
        }

        const int browserY = centeredY(browserArea, browserControlH) + 1;
        int x = browserArea.getX();
        presetSearch.setBounds(x, browserY, searchW, browserControlH); x += searchW + navGap;
        prevPresetBtn.setBounds(x, browserY, headerSpec.navW, browserControlH); x += headerSpec.navW + 4;
        const int presetW = juce::jmax(0, browserArea.getRight() - x - headerSpec.navW - 4);
        presetBox.setBounds(x, browserY, presetW, browserControlH); x += presetW + 4;
        nextPresetBtn.setBounds(x, browserY, headerSpec.navW, browserControlH);

        int actionX = row.getRight() - actionBlockW;
        savePresetBtn.setBounds(actionX, actionY, saveW, headerSpec.actionButtonH); actionX += saveW + headerSpec.actionGap;
        saveAsPresetBtn.setBounds(actionX, actionY, saveAsW, headerSpec.actionButtonH); actionX += saveAsW + headerSpec.actionGap;
        deletePresetBtn.setBounds(actionX, actionY, deleteW, headerSpec.actionButtonH); actionX += deleteW + headerSpec.actionGap;
        importPresetsBtn.setBounds(actionX, actionY, importW, headerSpec.actionButtonH); actionX += importW + headerSpec.actionGap;
        randButton.setBounds(actionX, actionY, randW, headerSpec.actionButtonH); actionX += randW + headerSpec.actionGap;
        midiLearnButton.setBounds(actionX, actionY, midiLearnW, headerSpec.actionButtonH);
    };

    if (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Wide)
    {
        const int statusReserve = headerSpec.gainSize + (layout.compact ? 24 : 26);
        const int gainSlotX = headerZones.statusZone.getRight() - statusReserve;
        const int gainX = gainSlotX + juce::jmax(0, (statusReserve - headerSpec.gainSize) / 2);
        const int gainY = headerZones.statusZone.getY()
                        + juce::jmax(0, (headerZones.statusZone.getHeight() - headerSpec.gainSize) / 2)
                        + (layout.compact ? 1 : 2);
        gainDial.setBounds(gainX, gainY, headerSpec.gainSize, headerSpec.gainSize);

        auto summaryArea = headerSpec.summaryRow;
        const int summaryW = juce::jlimit(layout.compact ? 240 : 280,
                                          layout.compact ? 340 : 420,
                                          headerSpec.summaryRow.getWidth() / 3);
        auto summaryBlock = summaryArea.removeFromLeft(summaryW);
        currentPresetSummaryLabel.setBounds(summaryBlock.removeFromTop(summaryBlock.getHeight() / 2));
        currentPresetMetaLabel.setBounds(summaryBlock);

        const int quickBtnW = layout.compact ? 72 : 84;
        const int quickTierW = layout.compact ? 112 : 126;
        const int quickRoleW = layout.compact ? 116 : 136;
        auto quickArea = summaryArea.reduced(0, 2);
        const int quickY = centeredY(quickArea, headerSpec.controlH);
        const int shortcutBlockW = quickBtnW * 3 + headerSpec.quickGap * 2;
        quickReferenceBtn.setBounds(quickArea.getX(), quickY, quickBtnW, headerSpec.controlH);
        quickMixBtn.setBounds(quickReferenceBtn.getRight() + headerSpec.quickGap, quickY, quickBtnW, headerSpec.controlH);
        quickCinematicBtn.setBounds(quickMixBtn.getRight() + headerSpec.quickGap, quickY, quickBtnW, headerSpec.controlH);
        quickTierSelector.setBounds(quickArea.getRight() - quickRoleW - quickTierW - headerSpec.quickGap,
                                    quickY,
                                    quickTierW,
                                    headerSpec.controlH);
        quickRoleSelector.setBounds(quickTierSelector.getRight() + headerSpec.quickGap,
                                    quickY,
                                    quickRoleW,
                                    headerSpec.controlH);
        if (quickTierSelector.getX() < quickCinematicBtn.getRight() + 12)
        {
            const int remainingW = juce::jmax(0, quickArea.getWidth() - shortcutBlockW - headerSpec.quickGap * 3);
            const int adaptiveTierW = juce::jlimit(92, quickTierW, remainingW / 2);
            const int adaptiveRoleW = juce::jmax(104, remainingW - adaptiveTierW);
            quickTierSelector.setBounds(quickArea.getRight() - adaptiveRoleW - adaptiveTierW - headerSpec.quickGap,
                                        quickY,
                                        adaptiveTierW,
                                        headerSpec.controlH);
            quickRoleSelector.setBounds(quickTierSelector.getRight() + headerSpec.quickGap,
                                        quickY,
                                        adaptiveRoleW,
                                        headerSpec.controlH);
        }

        layoutBrowserRow(headerSpec.browserRow);

        auto statusSecondaryRow = headerSpec.statusSecondaryRow;
        const int statusY = centeredY(statusSecondaryRow, headerSpec.statusButtonH);
        int statusX = statusSecondaryRow.getX();
        const int undoRedoW = layout.compact ? 24 : 26;
        undoButton.setBounds(statusX, statusY, undoRedoW, headerSpec.statusButtonH); statusX += undoRedoW + 2;
        redoButton.setBounds(statusX, statusY, undoRedoW, headerSpec.statusButtonH); statusX += undoRedoW + 8;
        monoModeButton.setBounds(statusX, statusY, 58, headerSpec.statusButtonH); statusX += 66;
        tooltipModeBtn.setBounds(statusX, statusY, 90, headerSpec.statusButtonH); statusX += 98;
        const int voiceLabelMaxRight = gainDial.getX() - 8;
        const int voiceLabelW = juce::jmin(layout.compact ? 78 : 88,
                                           juce::jmax(44, voiceLabelMaxRight - statusX));
        voiceCountLabel.setBounds(statusX, statusY, voiceLabelW, headerSpec.statusButtonH);
        statusX += voiceLabelW + 8;
        const int midiWidth = juce::jmax(0, voiceLabelMaxRight - statusX);
        midiCCPageLabel.setVisible(true);
        midiCCPageLabel.setBounds(statusX, statusY, midiWidth, headerSpec.statusButtonH);
    }
    else
    {
        auto summaryBlock = headerSpec.summaryRow.reduced(6, 2);
        currentPresetSummaryLabel.setBounds(summaryBlock.removeFromTop(summaryBlock.getHeight() / 2));
        currentPresetMetaLabel.setBounds(summaryBlock);

        const int quickBtnW = ultraCompact ? 48 : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 62 : 56);
        const int quickTierW = ultraCompact ? 96 : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 124 : 112);
        auto quickArea = headerSpec.quickRow.reduced(ultraCompact ? 4 : 8, 1);
        const int quickY = centeredY(quickArea, headerSpec.controlH);
        quickReferenceBtn.setBounds(quickArea.getX(), quickY, quickBtnW, headerSpec.controlH);
        quickMixBtn.setBounds(quickReferenceBtn.getRight() + headerSpec.quickGap, quickY, quickBtnW, headerSpec.controlH);
        quickCinematicBtn.setBounds(quickMixBtn.getRight() + headerSpec.quickGap, quickY, quickBtnW, headerSpec.controlH);

        const int shortcutBlockW = quickBtnW * 3 + headerSpec.quickGap * 2;
        const int remainingW = juce::jmax(0, quickArea.getWidth() - shortcutBlockW - headerSpec.quickGap * 3);
        const int adaptiveTierW = juce::jlimit(ultraCompact ? 84
                                               : headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 108 : 92,
                                               quickTierW,
                                               remainingW / 2);
        const int adaptiveRoleW = juce::jmax(ultraCompact ? 104
                                             : headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 132 : 116,
                                             remainingW - adaptiveTierW);
        quickTierSelector.setBounds(quickArea.getRight() - adaptiveRoleW - adaptiveTierW - headerSpec.quickGap,
                                    quickY,
                                    adaptiveTierW,
                                    headerSpec.controlH);
        quickRoleSelector.setBounds(quickTierSelector.getRight() + headerSpec.quickGap,
                                    quickY,
                                    adaptiveRoleW,
                                    headerSpec.controlH);

        layoutBrowserRow(headerSpec.browserRow);

        auto statusPrimaryRow = headerSpec.statusPrimaryRow.reduced(ultraCompact ? 2 : 4, 0);
        const int primaryY = centeredY(statusPrimaryRow, headerSpec.statusButtonH);
        int primaryRight = statusPrimaryRow.getRight();
        gainDial.setBounds(primaryRight - headerSpec.gainSize,
                           statusPrimaryRow.getY() + juce::jmax(0, (statusPrimaryRow.getHeight() - headerSpec.gainSize) / 2),
                           headerSpec.gainSize,
                           headerSpec.gainSize);
        primaryRight -= headerSpec.gainSize + (ultraCompact ? 4 : 6);
        const int voicesW = ultraCompact ? 66 : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 80 : 76);
        voiceCountLabel.setBounds(primaryRight - voicesW, primaryY, voicesW, headerSpec.statusButtonH);
        primaryRight -= voicesW + (ultraCompact ? 4 : 6);
        const int helpW = ultraCompact ? 48 : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 60 : 54);
        tooltipModeBtn.setBounds(primaryRight - helpW, primaryY, helpW, headerSpec.statusButtonH);

        auto statusSecondaryRow = headerSpec.statusSecondaryRow.reduced(ultraCompact ? 2 : 4, 0);
        const int statusY = centeredY(statusSecondaryRow, headerSpec.statusButtonH);
        int statusX = statusSecondaryRow.getX();
        const int undoRedoW = ultraCompact ? 18 : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 22 : 20);
        const int monoW = ultraCompact ? 42 : (headerSpec.mode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 52 : 46);
        undoButton.setBounds(statusX, statusY, undoRedoW, headerSpec.statusButtonH); statusX += undoRedoW + 2;
        redoButton.setBounds(statusX, statusY, undoRedoW, headerSpec.statusButtonH); statusX += undoRedoW + 6;
        monoModeButton.setBounds(statusX, statusY, monoW, headerSpec.statusButtonH); statusX += monoW + 6;

        arpModeLabel.setVisible(false); arpModeLabel.setBounds(0, 0, 0, 0);
        arpRateLabel.setVisible(false); arpRateLabel.setBounds(0, 0, 0, 0);
        arpOctavesLabel.setVisible(false); arpOctavesLabel.setBounds(0, 0, 0, 0);
        arpGateLabel.setVisible(false); arpGateLabel.setBounds(0, 0, 0, 0);
        arpGateDial.setVisible(false); arpGateDial.setBounds(0, 0, 0, 0);
        arpOctavesDial.setVisible(false); arpOctavesDial.setBounds(0, 0, 0, 0);
        arpRateSelector.setVisible(false); arpRateSelector.setBounds(0, 0, 0, 0);
        arpModeSelector.setVisible(false); arpModeSelector.setBounds(0, 0, 0, 0);
        const int midiWidth = juce::jmax(0, gainDial.getX() - statusX - 6);
        midiCCPageLabel.setVisible(true);
        midiCCPageLabel.setBounds(statusX, statusY, midiWidth, headerSpec.statusButtonH);
    }

    presetFilterLabel.setVisible(false);
    presetFilterLabel.setBounds(0, 0, 0, 0);
    presetFamilyFilter.setVisible(false);
    presetFamilyFilter.setBounds(0, 0, 0, 0);
    presetRoleFilter.setVisible(false);
    presetRoleFilter.setBounds(0, 0, 0, 0);

    const int selPad = ultraCompact ? 8 : (layout.compact ? 12 : 14);
    const int selectorInnerX = layout.contentX + selPad;
    const int selectorInnerW = layout.contentW - selPad * 2;
    const int selectorTopY = layout.selectorY + (ultraCompact ? 23 : (layout.compact ? 30 : 32));
    const int selectorRowH = ultraCompact ? 22 : (layout.compact ? 23 : 26);
    const int selectorGap = ultraCompact ? 8 : (layout.compact ? 10 : 12);
    const int tabsZoneW = static_cast<int>(selectorInnerW * (ultraCompact ? 0.58f : (layout.compact ? 0.62f : 0.66f)));
    const int comboZoneW = selectorInnerW - tabsZoneW - selectorGap;
    const int tabGap = interpolateGap(gapDensity, 6, 8, 10);
    const int tabW = (tabsZoneW - tabGap * (mps::kNumFamilies - 1)) / mps::kNumFamilies;
    for (int familyIndex = 0; familyIndex < mps::kNumFamilies; ++familyIndex)
    {
        auto& tab = familyTabs[static_cast<std::size_t>(familyIndex)];
        tab.setBounds(selectorInnerX + familyIndex * (tabW + tabGap), selectorTopY, tabW, selectorRowH);
        tab.setVisible(true);
        tab.setSelected(familyIndex == activeFamilyIndex);
    }

    familySelectorLbl.setVisible(false);
    familySelectorLbl.setBounds(0, 0, 0, 0);
    familySelector.setVisible(false);
    familySelector.setBounds(0, 0, 0, 0);
    pianoSelector.setVisible(false);
    pianoSelector.setBounds(0, 0, 0, 0);
    modelSelectorLbl.setVisible(false);
    modelSelectorLbl.setBounds(0, 0, 0, 0);
    modelSelector.setBounds(selectorInnerX + tabsZoneW + selectorGap, selectorTopY, comboZoneW, selectorRowH);
    for (auto& card : presetCards)
    {
        card.setVisible(false);
        card.setBounds(0, 0, 0, 0);
    }

    const int cPad = ultraCompact ? 9 : (layout.compact ? 13 : 16);
    const int knobGapX = ultraCompact ? 5 : interpolateGap(gapDensity, 7, 10, 12);
    const int knobGapY = ultraCompact ? 5 : interpolateGap(gapDensity, 8, 10, 12);
    const int knobW = (layout.colW - cPad * 2 - knobGapX * 2) / 3;
    const int lblH = ultraCompact ? 10 : (layout.compact ? 12 : 14);
    const int graphTargetH = ultraCompact ? 70 : (layout.compact ? 86 : (layout.roomy ? 178 : 126));
    const int knobH = juce::jlimit(ultraCompact ? 36 : (layout.compact ? 50 : 58),
                                   ultraCompact ? 62 : (layout.roomy ? 98 : 84),
                                   (layout.bodyH - graphTargetH - cPad * 2 - lblH * 3 - knobGapY * 3) / 3);
    const int protectedKeyboardTop = layout.kbY - (ultraCompact ? 8 : (layout.compact ? 14 : 18));

    const int toneIdx[] = { 7, 8, 9, 10, 11, 13 };
    const int col2StartY = layout.bodyY + cPad + (ultraCompact ? 14 : 18);
    for (int i = 0; i < 6; ++i)
    {
        const int row = i / 3;
        const int col = i % 3;
        const int xk = layout.col2X + cPad + col * (knobW + knobGapX);
        const int yk = col2StartY + row * (knobH + lblH + knobGapY);
        const auto si = static_cast<std::size_t>(toneIdx[i]);
        envLabels[si].setBounds(xk, yk, knobW, lblH);
        envDials[si].setBounds(xk, yk + lblH, knobW, knobH);
    }

    const int cutoffSize = juce::jlimit(ultraCompact ? 50 : (layout.compact ? 66 : 72),
                                        ultraCompact ? 78 : (layout.roomy ? 116 : 104),
                                        knobH + (ultraCompact ? 10 : (layout.compact ? 14 : 18)));
    const int cutoffX = layout.col2X + (layout.colW - cutoffSize) / 2;
    const int cutoffY = col2StartY + 2 * (knobH + lblH + knobGapY) + (ultraCompact ? 5 : (layout.compact ? 6 : 10));
    envLabels[12].setBounds(cutoffX, cutoffY, cutoffSize, lblH);
    envDials[12].setBounds(cutoffX, cutoffY + lblH, cutoffSize, cutoffSize);
    envDials[12].setTextBoxStyle(juce::Slider::TextBoxBelow, false,
                                 juce::jmax(cutoffSize + (ultraCompact ? 12 : 18),
                                            ultraCompact ? 72 : 116),
                                 ultraCompact ? 16 : 20);

    const int outputSize = juce::jlimit(ultraCompact ? 46 : (layout.compact ? 54 : 62),
                                        ultraCompact ? 66 : (layout.roomy ? 90 : 78),
                                        juce::jmax(ultraCompact ? 46 : (layout.compact ? 54 : 62), knobW - 20));
    const int outputX = layout.col2X + layout.colW - cPad - outputSize;
    envLabels[14].setBounds(outputX, cutoffY, outputSize, lblH);
    envDials[14].setBounds(outputX, cutoffY + lblH, outputSize, outputSize);

    const int sourceInnerX = layout.col1X + cPad;
    const int sourceInnerW = layout.colW - cPad * 2;
    const int sourceTopY = layout.bodyY + cPad + (ultraCompact ? 16 : 24);
    const int sourceBottomY = protectedKeyboardTop - (ultraCompact ? 6 : 8);
    const int envH = juce::jlimit(ultraCompact ? 78 : (layout.compact ? 132 : 154),
                                  ultraCompact ? 128 : (layout.roomy ? 250 : 212),
                                  static_cast<int>((sourceBottomY - sourceTopY) * (ultraCompact ? 0.42f : 0.47f)));
    envVisual.setVisible(true);
    envVisual.setBounds(sourceInnerX, sourceTopY, sourceInnerW, envH);

    const int sourceControlsY = envVisual.getBottom() + (ultraCompact ? 6 : (layout.compact ? 8 : 10));
    const int adsrGapX = ultraCompact ? 5 : interpolateGap(gapDensity, 6, 8, 10);
    const int adsrGapY = ultraCompact ? 5 : interpolateGap(gapDensity, 8, 10, 12);
    const int remainingH = sourceBottomY - sourceControlsY;
    const int adsrW = (sourceInnerW - adsrGapX * 3) / 4;
    const int adsrKnobH = juce::jlimit(ultraCompact ? 32 : (layout.compact ? 44 : 50),
                                       ultraCompact ? 56 : (layout.roomy ? 78 : 64),
                                       juce::jmax(ultraCompact ? 32 : (layout.compact ? 44 : 50),
                                                  (remainingH - lblH * 4 - adsrGapY * 2) / 2));
    const int secondaryGapX = ultraCompact ? 6 : interpolateGap(gapDensity, 8, 10, 12);
    const int secondaryW = (sourceInnerW - secondaryGapX * 2) / 3;
    const int smallSourceKnobH = juce::jlimit(ultraCompact ? 30 : (layout.compact ? 42 : 48),
                                              ultraCompact ? 52 : (layout.roomy ? 76 : 62),
                                              juce::jmax(ultraCompact ? 30 : (layout.compact ? 42 : 48),
                                                         remainingH - adsrKnobH - lblH * 2 - adsrGapY - 4));

    auto layoutSourceDial = [this, lblH](int paramIndex, int xk, int yk, int width, int height)
    {
        auto si = static_cast<std::size_t>(paramIndex);
        envLabels[si].setBounds(xk, yk, width, lblH);
        envDials[si].setBounds(xk, yk + lblH, width, height);
    };

    layoutSourceDial(3, sourceInnerX, sourceControlsY, adsrW, adsrKnobH);
    layoutSourceDial(4, sourceInnerX + (adsrW + adsrGapX), sourceControlsY, adsrW, adsrKnobH);
    layoutSourceDial(5, sourceInnerX + 2 * (adsrW + adsrGapX), sourceControlsY, adsrW, adsrKnobH);
    layoutSourceDial(6, sourceInnerX + 3 * (adsrW + adsrGapX), sourceControlsY, adsrW, adsrKnobH);

    const int sourceTailY = sourceControlsY + lblH + adsrKnobH + adsrGapY;
    layoutSourceDial(2, sourceInnerX, sourceTailY, secondaryW, smallSourceKnobH);
    layoutSourceDial(0, sourceInnerX + secondaryW + secondaryGapX, sourceTailY, secondaryW, smallSourceKnobH);
    layoutSourceDial(1, sourceInnerX + 2 * (secondaryW + secondaryGapX), sourceTailY, secondaryW, smallSourceKnobH);

    lfoRateDial.setVisible(false);
    lfoRateDial.setBounds(0, 0, 0, 0);
    lfoDepthDial.setVisible(false);
    lfoDepthDial.setBounds(0, 0, 0, 0);
    lfoWaveSelector.setVisible(false);
    lfoWaveSelector.setBounds(0, 0, 0, 0);
    lfoRateDial.setVisible(false);
    lfoDepthDial.setVisible(false);
    lfoWaveSelector.setVisible(false);

    lfoDestinationLabel.setVisible(false);
    lfoDestinationLabel.setBounds(0, 0, 0, 0);
    lfoDestinationSelector.setVisible(false);
    lfoDestinationSelector.setBounds(0, 0, 0, 0);
    lfoDestinationLabel.setVisible(false);
    lfoDestinationSelector.setVisible(false);
    lfoVisual.setVisible(false);
    lfoVisual.setBounds(0, 0, 0, 0);
    lfoAdvancedButton.setVisible(false);
    lfoAdvancedButton.setBounds(0, 0, 0, 0);
    modMatrixTitle.setVisible(false);
    modMatrixTitle.setBounds(0, 0, 0, 0);
    for (auto& row : modRows)
    {
        row.srcCombo.setVisible(false);
        row.dstCombo.setVisible(false);
        row.amtSlider.setVisible(false);
        row.srcCombo.setVisible(false);
        row.srcCombo.setBounds(0, 0, 0, 0);
        row.dstCombo.setVisible(false);
        row.dstCombo.setBounds(0, 0, 0, 0);
        row.amtSlider.setVisible(false);
        row.amtSlider.setBounds(0, 0, 0, 0);
    }

    const int col3StartY = layout.bodyY + cPad + (ultraCompact ? 20 : 28);
    const int rightTabGap = ultraCompact ? 4 : interpolateGap(gapDensity, 6, 8, 10);
    const int rightTabH = ultraCompact ? 20 : (layout.compact ? 23 : 26);
    const int rightTabW = (layout.colW - cPad * 2 - rightTabGap * (kRightPanelSections - 1)) / kRightPanelSections;
    for (int sectionIndex = 0; sectionIndex < kRightPanelSections; ++sectionIndex)
    {
        auto& tab = rightPanelTabs[static_cast<std::size_t>(sectionIndex)];
        tab.setBounds(layout.col3X + cPad + sectionIndex * (rightTabW + rightTabGap),
                      col3StartY,
                      rightTabW,
                      rightTabH);
        tab.setSelected(sectionIndex == activeRightPanelSection);
    }
    const int sectionContentY = col3StartY + rightTabH + (ultraCompact ? 8 : (layout.compact ? 10 : 12));

    const int macroGap = ultraCompact ? 5 : interpolateGap(gapDensity, 9, 12, 14);
    const int macroW = (layout.colW - cPad * 2 - macroGap * 3) / 4;
    const int macroH = juce::jlimit(ultraCompact ? 32 : (layout.compact ? 48 : 56),
                                    ultraCompact ? 44 : (layout.roomy ? 84 : 72),
                                    juce::jmin(macroW, knobH + (ultraCompact ? 0 : (layout.compact ? 0 : 6))));
    for (int i = 0; i < kMacroVisible; ++i)
    {
        auto index = static_cast<std::size_t>(i);
        macroLbls[index].setVisible(activeRightPanelSection == 0);
        macroDials[index].setVisible(activeRightPanelSection == 0);
        if (activeRightPanelSection == 0)
        {
            const int xk = layout.col3X + cPad + i * (macroW + macroGap);
            macroLbls[index].setBounds(xk, sectionContentY, macroW, lblH);
            macroDials[index].setBounds(xk, sectionContentY + lblH, macroW, macroH);
        }
        else
        {
            macroLbls[index].setVisible(false);
            macroLbls[index].setBounds(0, 0, 0, 0);
            macroDials[index].setVisible(false);
            macroDials[index].setBounds(0, 0, 0, 0);
        }
    }

    velocityCurveLabel.setVisible(activeRightPanelSection == 0);
    velocityCurveSelector.setVisible(activeRightPanelSection == 0);
    pitchBendRangeLabel.setVisible(activeRightPanelSection == 0);
    pitchBendRangeDial.setVisible(activeRightPanelSection == 0);
    aftertouchStatusLabel.setVisible(false);
    aftertouchStatusLabel.setBounds(0, 0, 0, 0);
    tremoloSyncButton.setVisible(false);
    tremoloSyncButton.setBounds(0, 0, 0, 0);
    if (activeRightPanelSection == 0)
    {
        const int perfLabelY = sectionContentY + macroH + lblH + (ultraCompact ? 7 : (layout.compact ? 12 : 14));
        const int perfGap = ultraCompact ? 8 : (layout.compact ? 10 : 12);
        const int perfW = (layout.colW - cPad * 2 - perfGap) / 2;
        velocityCurveLabel.setBounds(layout.col3X + cPad, perfLabelY, perfW, 15);
        velocityCurveSelector.setBounds(layout.col3X + cPad, perfLabelY + (ultraCompact ? 15 : 18), perfW,
                                        ultraCompact ? 22 : (layout.compact ? 26 : 28));
        pitchBendRangeLabel.setBounds(velocityCurveSelector.getRight() + perfGap, perfLabelY, perfW, 15);
        pitchBendRangeDial.setBounds(velocityCurveSelector.getRight() + perfGap, perfLabelY + (ultraCompact ? 15 : 18),
                                     perfW, ultraCompact ? 22 : (layout.compact ? 26 : 28));

        const bool electricFamily = mps::getFamily(selectedPianoFromParam()) == mps::Family::Electric;
        const int perfFooterY = velocityCurveSelector.getBottom() + (ultraCompact ? 5 : (layout.compact ? 8 : 10));
        const int perfFooterH = ultraCompact ? 18 : (layout.compact ? 24 : 26);
        const int tremoloW = electricFamily ? (ultraCompact ? 88 : (layout.compact ? 100 : 110)) : 0;
        const int footerGap = electricFamily ? (ultraCompact ? 6 : 8) : 0;
        aftertouchStatusLabel.setVisible(true);
        aftertouchStatusLabel.setBounds(layout.col3X + cPad,
                                        perfFooterY,
                                        layout.colW - cPad * 2 - tremoloW - footerGap,
                                        perfFooterH);
        if (electricFamily)
        {
            tremoloSyncButton.setVisible(true);
            tremoloSyncButton.setBounds(aftertouchStatusLabel.getRight() + footerGap,
                                        perfFooterY,
                                        tremoloW,
                                        perfFooterH);
        }

        const int motionY = perfFooterY + perfFooterH + (ultraCompact ? 4 : (layout.compact ? 8 : 10));
        const int motionH = juce::jmax(ultraCompact ? 64 : 84, protectedKeyboardTop - motionY - (ultraCompact ? 5 : 8));
        lfoVisual.setVisible(true);
        lfoVisual.setBounds(layout.col3X + cPad, motionY, layout.colW - cPad * 2, motionH);
    }
    else
    {
        velocityCurveLabel.setVisible(false);
        velocityCurveLabel.setBounds(0, 0, 0, 0);
        velocityCurveSelector.setVisible(false);
        velocityCurveSelector.setBounds(0, 0, 0, 0);
        pitchBendRangeLabel.setVisible(false);
        pitchBendRangeLabel.setBounds(0, 0, 0, 0);
        pitchBendRangeDial.setVisible(false);
        pitchBendRangeDial.setBounds(0, 0, 0, 0);
    }

    if (activeRightPanelSection == 1)
    {
        const int motionLabelW = juce::jlimit(ultraCompact ? 42 : (layout.compact ? 74 : 82),
                                              ultraCompact ? 58 : (layout.compact ? 104 : 116),
                                              (layout.colW - cPad * 2) / 3);
        const int motionSelectorX = layout.col3X + cPad + motionLabelW + (ultraCompact ? 6 : 8);
        const int motionSelectorW = juce::jmax(0, layout.col3X + layout.colW - cPad - motionSelectorX);
        lfoDestinationLabel.setVisible(true);
        lfoDestinationSelector.setVisible(true);
        lfoDestinationLabel.setText(ultraCompact ? "DEST" : "DESTINATION", juce::dontSendNotification);
        lfoDestinationLabel.setBounds(layout.col3X + cPad, sectionContentY + 4, motionLabelW, 14);
        lfoDestinationSelector.setBounds(motionSelectorX, sectionContentY, motionSelectorW,
                                         ultraCompact ? 22 : (layout.compact ? 24 : 26));

        const int motionBodyY = sectionContentY + (ultraCompact ? 30 : (layout.compact ? 36 : 40));
        const int buttonReserve = ultraCompact ? 24 : 30;
        const int motionBottom = protectedKeyboardTop - buttonReserve - (ultraCompact ? 6 : 8);

        if (advancedMotionVisible)
        {
            modMatrixTitle.setVisible(true);
            modMatrixTitle.setText("MOD MATRIX", juce::dontSendNotification);
            modMatrixTitle.setBounds(layout.col3X + cPad, motionBodyY, layout.colW - cPad * 2, 14);

            int rowY = modMatrixTitle.getBottom() + 4;
            const int rowH = ultraCompact ? 18 : (layout.compact ? 20 : 22);
            const int rowGap = ultraCompact ? 3 : 4;
            const int srcW = juce::jlimit(ultraCompact ? 54 : 64, ultraCompact ? 70 : 80, (layout.colW - cPad * 2) / 4);
            const int dstW = juce::jlimit(ultraCompact ? 62 : 74, ultraCompact ? 80 : 92, (layout.colW - cPad * 2) / 3);
            const int amtW = juce::jmax(70, layout.colW - cPad * 2 - srcW - dstW - rowGap * 2);
            int visibleRowCount = 0;
            for (auto& row : modRows)
            {
                if (rowY + rowH > motionBottom)
                    break;
                row.srcCombo.setVisible(true);
                row.dstCombo.setVisible(true);
                row.amtSlider.setVisible(true);
                row.srcCombo.setBounds(layout.col3X + cPad, rowY, srcW, rowH);
                row.dstCombo.setBounds(layout.col3X + cPad + srcW + rowGap, rowY, dstW, rowH);
                row.amtSlider.setBounds(layout.col3X + cPad + srcW + rowGap + dstW + rowGap, rowY, amtW, rowH);
                rowY += rowH + rowGap;
                ++visibleRowCount;
            }
            // Show how many rows are off-screen so the user knows to resize the window
            const int hiddenRowCount = static_cast<int>(modRows.size()) - visibleRowCount;
            modMatrixTitle.setText(hiddenRowCount > 0
                ? "MOD MATRIX (+" + juce::String(hiddenRowCount) + " off-screen)"
                : "MOD MATRIX",
                juce::dontSendNotification);
            lfoVisual.setVisible(false);
            lfoVisual.setBounds(0, 0, 0, 0);
            lfoAdvancedButton.setVisible(true);
            const int advW = ultraCompact ? 104 : 118;
            lfoAdvancedButton.setBounds(layout.col3X + layout.colW - cPad - advW, layout.kbY - (ultraCompact ? 22 : 24),
                                        advW, ultraCompact ? 18 : 20);
        }
        else
        {
            lfoVisual.setVisible(true);
            lfoVisual.setBounds(layout.col3X + cPad,
                                motionBodyY,
                                layout.colW - cPad * 2,
                                juce::jmax(ultraCompact ? 58 : 84, motionBottom - motionBodyY));
            lfoAdvancedButton.setVisible(true);
            const int advW = ultraCompact ? 104 : 118;
            lfoAdvancedButton.setBounds(layout.col3X + layout.colW - cPad - advW, layout.kbY - (ultraCompact ? 22 : 24),
                                        advW, ultraCompact ? 18 : 20);
        }
    }
    else if (activeRightPanelSection != 0)
    {
        lfoVisual.setVisible(false);
        lfoVisual.setBounds(0, 0, 0, 0);
    }

    // ── Section 3: ARP ──────────────────────────────────────────────────
    if (activeRightPanelSection == 3)
    {
        const int arpGap = layout.compact ? 8 : 10;
        const int arpLblH = lblH;
        const int arpComboH = layout.compact ? 24 : 26;
        const int innerW = layout.colW - cPad * 2;
        const int arpComboW = (innerW - arpGap) / 2;
        int arpY = sectionContentY;

        // Row 1: Mode and Rate selectors side by side
        arpModeLabel.setVisible(true);
        arpRateLabel.setVisible(true);
        arpModeSelector.setVisible(true);
        arpRateSelector.setVisible(true);
        arpModeLabel.setBounds(layout.col3X + cPad, arpY, arpComboW, arpLblH);
        arpRateLabel.setBounds(layout.col3X + cPad + arpComboW + arpGap, arpY, arpComboW, arpLblH);
        arpY += arpLblH;
        arpModeSelector.setBounds(layout.col3X + cPad, arpY, arpComboW, arpComboH);
        arpRateSelector.setBounds(layout.col3X + cPad + arpComboW + arpGap, arpY, arpComboW, arpComboH);
        arpY += arpComboH + arpGap * 2;

        // Row 2: Octaves knob + Gate knob + Hold button
        const int arpKnobSz = juce::jlimit(layout.compact ? 48 : 54,
                                            layout.compact ? 68 : 76,
                                            juce::jmin(innerW / 3 - 4, layout.compact ? 64 : 72));
        const int holdW = juce::jmax(44, innerW - arpKnobSz * 2 - arpGap * 2);
        const int knobRowTotalW = arpKnobSz * 2 + arpGap + holdW + arpGap;
        const int knobRowX = layout.col3X + cPad + juce::jmax(0, (innerW - knobRowTotalW) / 2);
        arpOctavesLabel.setVisible(true);
        arpGateLabel.setVisible(true);
        arpOctavesDial.setVisible(true);
        arpGateDial.setVisible(true);
        arpHoldButton.setVisible(true);
        arpOctavesLabel.setBounds(knobRowX, arpY, arpKnobSz, arpLblH);
        arpGateLabel.setBounds(knobRowX + arpKnobSz + arpGap, arpY, arpKnobSz, arpLblH);
        arpY += arpLblH;
        arpOctavesDial.setBounds(knobRowX, arpY, arpKnobSz, arpKnobSz);
        arpGateDial.setBounds(knobRowX + arpKnobSz + arpGap, arpY, arpKnobSz, arpKnobSz);
        arpHoldButton.setBounds(knobRowX + arpKnobSz * 2 + arpGap * 2,
                                arpY + (arpKnobSz - arpComboH) / 2,
                                holdW, arpComboH);
    }
    else
    {
        arpHoldButton.setVisible(false);
        arpHoldButton.setBounds(0, 0, 0, 0);
        arpModeSelector.setVisible(false);
        arpModeSelector.setBounds(0, 0, 0, 0);
        arpModeLabel.setVisible(false);
        arpModeLabel.setBounds(0, 0, 0, 0);
        arpRateSelector.setVisible(false);
        arpRateSelector.setBounds(0, 0, 0, 0);
        arpRateLabel.setVisible(false);
        arpRateLabel.setBounds(0, 0, 0, 0);
        arpOctavesDial.setVisible(false);
        arpOctavesDial.setBounds(0, 0, 0, 0);
        arpOctavesLabel.setVisible(false);
        arpOctavesLabel.setBounds(0, 0, 0, 0);
        arpGateDial.setVisible(false);
        arpGateDial.setBounds(0, 0, 0, 0);
        arpGateLabel.setVisible(false);
        arpGateLabel.setBounds(0, 0, 0, 0);
    }

    fxLockButton.setVisible(activeRightPanelSection == 2);
    if (activeRightPanelSection == 2)
    {
        const bool fxAvail = mps::isFxAvailable(selectedPianoFromParam(), fxSlotForTab(activeFxTab));
        fxDetailTitle.setVisible(fxAvail);
        fxUnavailableLbl.setVisible(!fxAvail);
    }
    else
    {
        fxDetailTitle.setVisible(false);
        fxUnavailableLbl.setVisible(false);
    }
    delaySyncButton.setVisible(false);
    delayNoteDivLabel.setVisible(false);
    delayNoteDivSelector.setVisible(false);

    const int fxHeaderY = sectionContentY;
    if (activeRightPanelSection == 2)
    {
        fxLockButton.setBounds(layout.col3X + layout.colW - cPad - 82, fxHeaderY, 82, 20);
    }
    else
    {
        fxLockButton.setVisible(false);
        fxLockButton.setBounds(0, 0, 0, 0);
    }

    const int fxAreaY = fxHeaderY + 24;
    const int fxAreaH = juce::jmax(80, protectedKeyboardTop - fxAreaY - 10);
    constexpr int kBypassW = 34;
    const int rackGap = layout.compact ? 10 : 12;
    constexpr int kRackRowGap = 4;
    const int rackTotalW = juce::jlimit(layout.compact ? 96 : 108,
                                        layout.compact ? 120 : (layout.roomy ? 144 : 130),
                                        layout.colW / 2 - 16);
    const int rackItemW = rackTotalW - kBypassW - 6;
    const int rackRowH = juce::jlimit(layout.compact ? 18 : 20,
                                      layout.roomy ? 30 : 26,
                                      (fxAreaH - kRackRowGap * (kFxTabs - 1)) / kFxTabs);

    // Count visible FX tabs so we pack rows without gaps
    const int pianoIdx_fx = selectedPianoFromParam();
    int visibleFxTabs = 0;
    for (int t = 0; t < kFxTabs; ++t)
        if (mps::isFxAvailable(pianoIdx_fx, fxSlotForTab(t)))
            ++visibleFxTabs;
    const int visCount = juce::jmax(1, visibleFxTabs);
    const int rackBlockH = visCount * rackRowH + kRackRowGap * (visCount - 1);
    const int rackStartY = fxAreaY + juce::jmax(0, (fxAreaH - rackBlockH) / 2);

    int rackCurY = rackStartY;
    for (int t = 0; t < kFxTabs; ++t)
    {
        const bool vis = activeRightPanelSection == 2 && mps::isFxAvailable(pianoIdx_fx, fxSlotForTab(t));
        if (vis)
        {
            fxRackItems[static_cast<std::size_t>(t)].setBounds(layout.col3X + cPad, rackCurY, rackItemW, rackRowH);
            fxBypassBtns[static_cast<std::size_t>(t)].setBounds(layout.col3X + cPad + rackItemW + 6,
                                                                rackCurY + juce::jmax(0, (rackRowH - 20) / 2),
                                                                kBypassW, 20);
            rackCurY += rackRowH + kRackRowGap;
        }
        else
        {
            fxRackItems[static_cast<std::size_t>(t)].setVisible(false);
            fxRackItems[static_cast<std::size_t>(t)].setBounds(0, 0, 0, 0);
            fxBypassBtns[static_cast<std::size_t>(t)].setVisible(false);
            fxBypassBtns[static_cast<std::size_t>(t)].setBounds(0, 0, 0, 0);
        }
        fxRackItems[static_cast<std::size_t>(t)].setVisible(vis);
        fxBypassBtns[static_cast<std::size_t>(t)].setVisible(vis);
    }

    const int detailX = layout.col3X + cPad + rackTotalW + rackGap;
    const int detailAvailableW = layout.col3X + layout.colW - cPad - detailX;
    const int detailW = ultraCompact ? juce::jmax(0, detailAvailableW)
                                     : juce::jmax(layout.compact ? 132 : 160, detailAvailableW);
    fxDetailTitle.setBounds(detailX, fxAreaY, detailW, 14);
    fxUnavailableLbl.setBounds(detailX, fxAreaY + 20, detailW, 36);

    const bool showDelayOptions = activeFxTab == kDelayFxTab;
    const bool showReverbOptions = activeFxTab == 0;
    const int visibleFxCount = [&]()
    {
        int count = 0;
        for (int slot = 0; slot < kFxPerTab; ++slot)
            if (kFxTabMap[activeFxTab][slot] >= 0)
                ++count;
        return juce::jmax(1, count);
    }();
    int detailCols = 1;
    if (visibleFxCount > 1)
    {
        const int singleColumnThreshold = layout.compact ? 140 : 170;
        const int twoColumnThreshold = layout.compact ? 220 : 260;
        if (detailW < singleColumnThreshold)
            detailCols = 1;
        else if (visibleFxCount <= 4 || detailW < twoColumnThreshold)
            detailCols = 2;
        else
            detailCols = 3;
        detailCols = juce::jmin(detailCols, visibleFxCount);
    }
    const int detailRows = juce::jmax(1, (visibleFxCount + detailCols - 1) / detailCols);
    const int detailGapX = layout.compact ? 5 : 8;
    const int detailGapY = layout.compact ? 6 : 8;
    const int detailLabelH = lblH;
    const int delayOptionBlockH  = showDelayOptions  ? 42 : 0;
    const int reverbOptionBlockH = showReverbOptions ? 34 : 0;
    const int detailGridY = fxAreaY + 20;
    const int detailAvailH = juce::jmax(60, layout.kbY - detailGridY - 10 - delayOptionBlockH - reverbOptionBlockH);
    const int detailKnobW = (detailW - detailGapX * (detailCols - 1)) / detailCols;
    const int detailKnobMaxFromSpace = juce::jmax(20,
        (detailAvailH - detailGapY * juce::jmax(0, detailRows - 1)) / detailRows - detailLabelH);
    const int detailKnobH = juce::jlimit(layout.compact ? 42 : 48,
                                         layout.roomy ? 120 : 100,
                                         juce::jmin(detailKnobW, detailKnobMaxFromSpace));
    const int detailRowStride = detailLabelH + detailKnobH + detailGapY;
    const int detailStartY = detailGridY;

    int visibleIndex = 0;
    for (int slot = 0; slot < kFxPerTab; ++slot)
    {
        const int fxIndex = kFxTabMap[activeFxTab][slot];
        if (fxIndex < 0)
            continue;

        const int row = visibleIndex / detailCols;
        const int col = visibleIndex % detailCols;
        const int xk = detailX + col * (detailKnobW + detailGapX);
        const int yk = detailStartY + row * detailRowStride;
        fxLbls[static_cast<std::size_t>(fxIndex)].setVisible(activeRightPanelSection == 2);
        fxDials[static_cast<std::size_t>(fxIndex)].setVisible(activeRightPanelSection == 2);
        if (activeRightPanelSection == 2)
        {
            fxLbls[static_cast<std::size_t>(fxIndex)].setBounds(xk, yk, detailKnobW, detailLabelH);
            fxDials[static_cast<std::size_t>(fxIndex)].setBounds(xk, yk + detailLabelH, detailKnobW, detailKnobH);
            fxDials[static_cast<std::size_t>(fxIndex)].setTextBoxStyle(
                juce::Slider::TextBoxBelow, false, detailKnobW, 16);
        }
        ++visibleIndex;
    }

    for (int fxIndex = 0; fxIndex < kFxN; ++fxIndex)
    {
        if (!fxDials[static_cast<std::size_t>(fxIndex)].isVisible())
        {
            fxLbls[static_cast<std::size_t>(fxIndex)].setVisible(false);
            fxLbls[static_cast<std::size_t>(fxIndex)].setBounds(0, 0, 0, 0);
            fxDials[static_cast<std::size_t>(fxIndex)].setVisible(false);
            fxDials[static_cast<std::size_t>(fxIndex)].setBounds(0, 0, 0, 0);
        }
    }

    if (showDelayOptions && activeRightPanelSection == 2)
    {
        const int optionsY = layout.kbY - 34;
        const int syncW = ultraCompact ? 44 : (layout.compact ? 90 : 96);
        delaySyncButton.setVisible(true);
        delayNoteDivLabel.setVisible(!ultraCompact);
        delayNoteDivSelector.setVisible(true);
        delaySyncButton.setBounds(detailX, optionsY, syncW, ultraCompact ? 22 : 24);
        if (ultraCompact)
        {
            delayNoteDivLabel.setBounds(0, 0, 0, 0);
            const int divSelectorX = delaySyncButton.getRight() + 4;
            delayNoteDivSelector.setBounds(divSelectorX, optionsY - 2,
                                           juce::jmax(0, detailX + detailW - divSelectorX),
                                           26);
        }
        else
        {
            const int divLabelW = layout.compact ? 56 : 62;
            delayNoteDivLabel.setBounds(delaySyncButton.getRight() + 10, optionsY + 2, divLabelW, 18);
            const int divSelectorX = delayNoteDivLabel.getRight() + 6;
            delayNoteDivSelector.setBounds(divSelectorX, optionsY - 2,
                                           juce::jmax(104, detailX + detailW - divSelectorX), 28);
        }
    }
    else
    {
        delaySyncButton.setVisible(false);
        delaySyncButton.setBounds(0, 0, 0, 0);
        delayNoteDivLabel.setVisible(false);
        delayNoteDivLabel.setBounds(0, 0, 0, 0);
        delayNoteDivSelector.setVisible(false);
        delayNoteDivSelector.setBounds(0, 0, 0, 0);
    }

    if (showReverbOptions && activeRightPanelSection == 2)
    {
        const int optionsY = layout.kbY - 28 - (showDelayOptions ? 42 : 0);
        const int typeLabelW = layout.compact ? 36 : 40;
        const int typeSelectorW = juce::jmax(120, detailW - typeLabelW - 6);
        reverbTypeLabel.setVisible(true);
        reverbTypeSelector.setVisible(true);
        reverbTypeLabel.setBounds(detailX, optionsY + 3, typeLabelW, 22);
        reverbTypeSelector.setBounds(detailX + typeLabelW + 4, optionsY - 1, typeSelectorW, 28);
    }
    else
    {
        reverbTypeLabel.setVisible(false);
        reverbTypeLabel.setBounds(0, 0, 0, 0);
        reverbTypeSelector.setVisible(false);
        reverbTypeSelector.setBounds(0, 0, 0, 0);
    }

    if (keyboard != nullptr)
    {
        const int keyboardInsetLeft = ultraCompact ? 56 : 58;
        const int keyboardInsetTop = ultraCompact ? 5 : (layout.compact ? 6 : 8);
        const int keyboardH = juce::jmax(ultraCompact ? 42 : 36, layout.kbH - keyboardInsetTop * 2);
        const int keyboardCenterY = layout.kbY + keyboardInsetTop + keyboardH / 2;
        keyboard->setBounds(layout.contentX + keyboardInsetLeft, layout.kbY + keyboardInsetTop,
                            layout.contentW - keyboardInsetLeft - 4, keyboardH);
        const int octaveButtonW = ultraCompact ? 22 : 24;
        const int octaveButtonH = ultraCompact ? 24 : 26;
        octaveDownBtn.setBounds(layout.contentX + 5, keyboardCenterY - octaveButtonH / 2,
                                octaveButtonW, octaveButtonH);
        octaveUpBtn.setBounds(layout.contentX + 30, keyboardCenterY - octaveButtonH / 2,
                              octaveButtonW, octaveButtonH);
    }

    auto applyLbl = [layout](juce::Label& label)
    {
        label.setFont(juce::Font(juce::FontOptions{}.withHeight(layout.ultraCompact ? 10.2f : (layout.compact ? 11.5f : 12.5f)).withStyle("Bold")));
        label.setColour(juce::Label::textColourId, synthcol::textSec);
    };
    for (auto& label : envLabels) applyLbl(label);
    for (auto& label : macroLbls) applyLbl(label);
    for (auto& label : fxLbls) applyLbl(label);
    applyLbl(velocityCurveLabel);
    applyLbl(pitchBendRangeLabel);
    applyLbl(lfoDestinationLabel);
    applyLbl(aftertouchStatusLabel);
    applyLbl(arpModeLabel);
    applyLbl(arpRateLabel);
    applyLbl(arpOctavesLabel);
    applyLbl(arpGateLabel);
    const float summaryFontH = layout.headerMode == PianoLayoutMetrics::HeaderLayoutMode::Wide
        ? (layout.compact ? 13.0f : 15.0f)
        : (layout.ultraCompact ? 11.3f : (layout.headerMode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 12.8f : 12.2f));
    const float metaFontH = layout.headerMode == PianoLayoutMetrics::HeaderLayoutMode::Wide
        ? (layout.compact ? 9.6f : 10.6f)
        : (layout.ultraCompact ? 8.4f : (layout.headerMode == PianoLayoutMetrics::HeaderLayoutMode::Medium ? 9.4f : 8.9f));
    currentPresetSummaryLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(summaryFontH).withStyle("Bold")));
    currentPresetMetaLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(metaFontH).withStyle("Bold")));
}

void PianoSynthAudioProcessorEditor::applyEnvValueFormatters()
{
    envDials[12].setTextValueSuffix({});
    envDials[12].setNumDecimalPlacesToDisplay(0);
    envDials[14].setTextValueSuffix({});
    envDials[14].setNumDecimalPlacesToDisplay(0);
    setSliderFormatter(envDials[0], [](double value) { return formatPercent01(value); });
    setSliderFormatter(envDials[1], [](double value) { return formatSemitones(value); });
    setSliderFormatter(envDials[2], [](double value) { return formatPercent01(value); });
    setSliderFormatter(envDials[3], [](double value) { return formatTime(value); });
    setSliderFormatter(envDials[4], [](double value) { return formatTime(value); });
    setSliderFormatter(envDials[5], [](double value) { return formatPercent01(value); });
    setSliderFormatter(envDials[6], [](double value) { return formatTime(value); });
    setSliderFormatter(envDials[7], [](double value) { return formatPercent01(value); });
    setSliderFormatter(envDials[8], [](double value) { return formatPercent01(value); });
    setSliderFormatter(envDials[9], [](double value) { return formatPercent01(value); });
    setSliderFormatter(envDials[10], [](double value) { return formatPercent01(value); });
    setSliderFormatter(envDials[11], [](double value) { return formatPercent01(value); });
    setSliderFormatter(envDials[12], [](double value) { return formatFrequency(value); });
    setSliderFormatter(envDials[13], [](double value) { return formatPan(value); });
    setSliderFormatter(envDials[14], [](double value)
    {
        const auto outputIndex = juce::jlimit(0, PianoSynthAudioProcessor::kNumAuxOutputs,
            static_cast<int>(std::round(value)));
        return outputIndex == 0 ? juce::String("Master") : "Out " + juce::String(outputIndex);
    });
}

void PianoSynthAudioProcessorEditor::applyStaticValueFormatters()
{
    setSliderFormatter(gainDial, [](double value) { return formatDecibels(value); });
    setSliderFormatter(lfoRateDial, [](double value) { return formatNumber(value, value < 1.0 ? 2 : 1) + " Hz"; });
    setSliderFormatter(lfoDepthDial, [](double value) { return formatPercent01(value); });

    for (auto& dial : macroDials)
        setSliderFormatter(dial, [](double value) { return formatCenteredMacroPercent(value); });

    setSliderFormatter(fxDials[0], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[1], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[2], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[3], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[4], [](double value) { return formatMilliseconds(value); });
    setSliderFormatter(fxDials[5], [](double value) { return formatDrive(value); });
    setSliderFormatter(fxDials[6], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[7], [](double value) { return formatSignedPercent(value); });
    setSliderFormatter(fxDials[8], [](double value) { return formatSignedPercent(value); });
    setSliderFormatter(fxDials[9], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[10], [](double value) { return formatDecibels(value); });
    setSliderFormatter(fxDials[11], [](double value) { return formatRatio(value); });
    setSliderFormatter(fxDials[12], [](double value) { return formatMilliseconds(value); });
    setSliderFormatter(fxDials[13], [](double value) { return formatMilliseconds(value); });
    setSliderFormatter(fxDials[14], [](double value) { return formatDecibels(value); });
    setSliderFormatter(fxDials[15], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[16], [](double value) { return formatFrequency(value); });
    setSliderFormatter(fxDials[17], [](double value) { return formatDecibels(value); });
    setSliderFormatter(fxDials[18], [](double value) { return formatFrequency(value); });
    setSliderFormatter(fxDials[19], [](double value) { return formatDecibels(value); });
    setSliderFormatter(fxDials[20], [](double value) { return formatQ(value); });
    setSliderFormatter(fxDials[21], [](double value) { return formatFrequency(value); });
    setSliderFormatter(fxDials[22], [](double value) { return formatDecibels(value); });
    setSliderFormatter(fxDials[23], [](double value) { return formatNumber(value, value < 1.0 ? 2 : 1) + " Hz"; });
    setSliderFormatter(fxDials[24], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[25], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[26], [](double value) { return formatMilliseconds(value); });
    setSliderFormatter(fxDials[27], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[28], [](double value) { return formatPercent01(value); });
    setSliderFormatter(fxDials[29], [](double value) { return formatDecibels(value); });
    setSliderFormatter(fxDials[30], [](double value) { return formatMilliseconds(value); });

    setSliderFormatter(pitchBendRangeDial, [](double value) { return formatSemitones(value); });
}

void PianoSynthAudioProcessorEditor::rebuildPianoAttachments()
{
    const int pianoIndex = selectedPianoFromParam();
    if (pianoIndex == cachedAttachPianoIdx)
        return;
    cachedAttachPianoIdx = pianoIndex;
    const auto& profile = envProfileForPiano(pianoIndex);
    for (int i = 0; i < kEnvN; ++i)
    {
        envAttach[static_cast<std::size_t>(i)] = std::make_unique<SliderAttach>(
            proc.getAPVTS(),
            PianoSynthAudioProcessor::makePianoParamId(pianoIndex, kEnvCtrls[static_cast<std::size_t>(i)].suffix),
            envDials[static_cast<std::size_t>(i)]);
    }
    // Re-apply dial formatters — SliderAttach::setNormalisableRange
    // resets textFromValueFunction when the parameter range has no custom valueToText.
    for (int i = 0; i < kEnvN; ++i)
    {
        const auto index = static_cast<std::size_t>(i);
        switch (i)
        {
            case 0:
            case 2:
            case 5:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                setupDial(envDials[index], accent_);
                break;
            case 1:
                setupDial(envDials[index], accent_);
                break;
            case 3:
            case 4:
            case 6:
                setupDial(envDials[index], accent_);
                break;
            case 12:
                setupGrandDial(envDials[index], accent_, {});
                break;
            case 13:
                setupDial(envDials[index], accent_);
                break;
            default:
                setupDial(envDials[index], accent_);
                break;
        }
    }
    applyEnvValueFormatters();
    synthui::applyLabelProfile(profile, envLabels);
    applyTooltips();
    syncConditionalUiState();
}

void PianoSynthAudioProcessorEditor::rebuildModelSelectorForFamily(int familyIndex, int preferredPiano)
{
    modelSelector.clear(juce::dontSendNotification);
    const int familyStart = mps::kFamilyStart[familyIndex];
    const int familyCount = mps::kFamilySize[familyIndex];

    int selectedId = 0;
    for (int offset = 0; offset < familyCount; ++offset)
    {
        const int pianoIndex = familyStart + offset;
        const int itemId = pianoIndex + 1;
        modelSelector.addItem(juce::String(juce::CharPointer_UTF8(mps::getPianoName(pianoIndex))), itemId);
        if (preferredPiano == pianoIndex)
            selectedId = itemId;
    }

    if (selectedId == 0)
        selectedId = familyStart + 1;

    modelSelector.setSelectedId(selectedId, juce::dontSendNotification);
}

void PianoSynthAudioProcessorEditor::syncSelectionUiFromPiano()
{
    const int pianoIndex = selectedPianoFromParam();
    const int familyIndex = static_cast<int>(mps::getFamily(pianoIndex));

    if (activeFamilyIndex != familyIndex)
    {
        activeFamilyIndex = familyIndex;
        rebuildModelSelectorForFamily(activeFamilyIndex, pianoIndex);
    }
    else if (modelSelector.indexOfItemId(pianoIndex + 1) < 0)
    {
        rebuildModelSelectorForFamily(activeFamilyIndex, pianoIndex);
    }

    familySelector.setSelectedId(activeFamilyIndex + 1, juce::dontSendNotification);
    modelSelector.setSelectedId(pianoIndex + 1, juce::dontSendNotification);
    for (int idx = 0; idx < mps::kNumFamilies; ++idx)
        familyTabs[static_cast<std::size_t>(idx)].setSelected(idx == activeFamilyIndex);

    if (std::any_of(presetCards.begin(), presetCards.end(),
                    [](const auto& card) { return card.isVisible(); }))
    {
        for (int idx = 0; idx < mps::kNumPianos; ++idx)
            presetCards[static_cast<std::size_t>(idx)].setSelected(idx == pianoIndex);
    }

    if (cachedPianoIdx != pianoIndex)
    {
        cachedPianoIdx = pianoIndex;
        const bool hasActiveLfoRouting = static_cast<int>(std::round(
            proc.getAPVTS().getRawParameterValue("lfo_destination")->load())) != 0;
        if (hasActiveLfoRouting)
            advancedMotionVisible = true;

        lfoAdvancedButton.setToggleState(advancedMotionVisible, juce::dontSendNotification);
        resetToPlaySectionAfterPresetChange();
        applyPianoTheme(pianoIndex);
        refreshPresetList();
    }

    syncCurrentPresetSummary();
}

void PianoSynthAudioProcessorEditor::syncFxAvailability()
{
    const int pianoIndex = selectedPianoFromParam();
    const auto family = mps::getFamily(pianoIndex);
    const auto& macroLabels = macroLabelsForFamily(family);
    synthui::applyMacroLabelProfile(macroLabels, macroLbls);

    const bool hasActiveLfoRouting = static_cast<int>(std::round(
        proc.getAPVTS().getRawParameterValue("lfo_destination")->load())) != 0;
    if (hasActiveLfoRouting)
    {
        advancedMotionVisible = true;
    }
    lfoAdvancedButton.setToggleState(advancedMotionVisible, juce::dontSendNotification);
    lfoAdvancedButton.setButtonText(advancedMotionVisible ? "MOD MATRIX ON" : "MOD MATRIX");

    int firstAvailableFxTab = -1;
    int availabilityMask = 0;
    for (int tabIndex = 0; tabIndex < kFxTabs; ++tabIndex)
    {
        const bool available = mps::isFxAvailable(pianoIndex, fxSlotForTab(tabIndex));
        if (available && firstAvailableFxTab < 0)
            firstAvailableFxTab = tabIndex;
        if (available)
            availabilityMask |= (1 << tabIndex);
        fxRackItems[static_cast<std::size_t>(tabIndex)].setVisible(available);
        fxBypassBtns[static_cast<std::size_t>(tabIndex)].setVisible(available);
        fxBypassBtns[static_cast<std::size_t>(tabIndex)].setEnabled(available);
    }

    int targetFxTab = activeFxTab;
    if (firstAvailableFxTab >= 0 && !mps::isFxAvailable(pianoIndex, fxSlotForTab(activeFxTab)))
        targetFxTab = firstAvailableFxTab;

    const bool fxStateChanged = cachedFxAvailabilityPiano != pianoIndex
        || cachedFxAvailabilityMask != availabilityMask
        || targetFxTab != activeFxTab;

    cachedFxAvailabilityPiano = pianoIndex;
    cachedFxAvailabilityMask = availabilityMask;

    if (fxStateChanged)
        switchEffectTab(targetFxTab);
    else
        syncFxRackState();

    syncConditionalUiState();
}

void PianoSynthAudioProcessorEditor::syncFxRackState()
{
    const int pianoIndex = selectedPianoFromParam();
    for (int tabIndex = 0; tabIndex < kFxTabs; ++tabIndex)
    {
        auto& rackItem = fxRackItems[static_cast<std::size_t>(tabIndex)];
        const bool available = mps::isFxAvailable(pianoIndex, fxSlotForTab(tabIndex));
        auto& bypass = fxBypassBtns[static_cast<std::size_t>(tabIndex)];
        rackItem.setSelected(available && tabIndex == activeFxTab);
        rackItem.setEnabledState(available && bypass.getToggleState());
    }
}

void PianoSynthAudioProcessorEditor::syncConditionalUiState()
{
    syncPerformanceAffordances();
    syncMotionRoutingAvailability();
    syncInstrumentControlAvailability();
    syncFxContextMessaging();
    syncReverbDetailAvailability();
}

void PianoSynthAudioProcessorEditor::syncPerformanceAffordances()
{
    const int pianoIndex = selectedPianoFromParam();
    const bool electricFamily = mps::getFamily(pianoIndex) == mps::Family::Electric;
    const auto aftertouchText = electricFamily
        ? juce::String("AFTERTOUCH IN MOD MATRIX | stage pressure source")
        : juce::String("AFTERTOUCH AVAILABLE IN MOD MATRIX");
    aftertouchStatusLabel.setText(aftertouchText, juce::dontSendNotification);

    tremoloSyncButton.setVisible(activeRightPanelSection == 0 && electricFamily);
    tremoloSyncButton.setEnabled(electricFamily);
    tremoloSyncButton.setAlpha(electricFamily ? 1.0f : 0.45f);

    const auto tremoloTooltip = tooltipMode == TooltipMode::Off
        ? juce::String()
        : electricFamily
            ? (tooltipMode == TooltipMode::Short
                ? juce::String("Trem Sync: lock electric tremolo to host BPM.")
                : juce::String("Trem Sync: for electric models only, lock the internal tremolo rate to the host tempo."))
            : (tooltipMode == TooltipMode::Short
                ? juce::String("Trem Sync: electric models only.")
                : juce::String("Trem Sync: stored for preset recall, but only electric models use host-synced tremolo in v1."));
    tremoloSyncButton.setTooltip(tremoloTooltip);

    const auto aftertouchTooltip = tooltipMode == TooltipMode::Off
        ? juce::String()
        : tooltipMode == TooltipMode::Short
            ? juce::String("Aftertouch: available as a mod matrix source only.")
            : juce::String("Aftertouch: channel pressure no longer changes level implicitly and only acts through explicit mod matrix routing.");
    aftertouchStatusLabel.setTooltip(aftertouchTooltip);
}

void PianoSynthAudioProcessorEditor::syncInstrumentControlAvailability()
{
    const int pianoIndex = selectedPianoFromParam();
    const auto& profile = envProfileForPiano(pianoIndex);
    const auto sharedTooltipMode = tooltipMode == TooltipMode::Short ? synthui::TooltipMode::Short
                                : tooltipMode == TooltipMode::Novice ? synthui::TooltipMode::Novice
                                                                     : synthui::TooltipMode::Off;

    for (int i = 0; i < kEnvN; ++i)
    {
        const auto index = static_cast<std::size_t>(i);
        const bool available = PianoSynthAudioProcessor::isInstrumentControlAvailableForPiano(
            pianoIndex,
            kEnvCtrls[index].suffix);
        const auto baseTooltip = synthui::tooltipForMode(profile[index], sharedTooltipMode);
        const auto tooltip = sharedTooltipMode == synthui::TooltipMode::Off
            ? juce::String()
            : available
            ? baseTooltip
            : baseTooltip + (baseTooltip.isNotEmpty() ? "\n" : juce::String())
                + "Not used on this model in v1.";
        const auto labelText = available
            ? juce::String(profile[index].label)
            : juce::String(profile[index].label) + " N/A";

        envDials[index].setEnabled(available);
        envLabels[index].setEnabled(available);
        envLabels[index].setText(labelText, juce::dontSendNotification);
        envDials[index].setAlpha(available ? 1.0f : 0.45f);
        envLabels[index].setAlpha(available ? 1.0f : 0.50f);
        envDials[index].setTooltip(tooltip);
        envLabels[index].setTooltip(tooltip);
    }
}

void PianoSynthAudioProcessorEditor::syncMotionRoutingAvailability()
{
    const int pianoIndex = selectedPianoFromParam();
    const bool electricFamily = mps::getFamily(pianoIndex) == mps::Family::Electric;
    const bool hasActiveLfoRouting = static_cast<int>(std::round(
        proc.getAPVTS().getRawParameterValue("lfo_destination")->load())) != 0;
    const bool chorusAvailable = PianoSynthAudioProcessor::isLfoDestinationAvailableForPiano(
        pianoIndex,
        mps::LfoDestination::ChorusMotion);
    const bool motionSectionVisible = activeRightPanelSection == 1;
    const bool playSectionVisible = activeRightPanelSection == 0;

    lfoDestinationSelector.changeItemText(4, chorusAvailable ? "CHORUS" : "CHORUS (N/A)");
    lfoDestinationSelector.setItemEnabled(4, chorusAvailable);
    lfoAdvancedButton.setVisible(motionSectionVisible);

    const bool showMotionControls = motionSectionVisible && (electricFamily || advancedMotionVisible || hasActiveLfoRouting);
    const bool showLfo = playSectionVisible || showMotionControls;
    lfoVisual.setVisible(showLfo);
    lfoRateDial.setVisible(false);
    lfoDepthDial.setVisible(false);
    lfoWaveSelector.setVisible(false);
    lfoDestinationLabel.setVisible(showMotionControls);
    lfoDestinationSelector.setVisible(showMotionControls);

    const int destId = lfoDestinationSelector.getSelectedId();
    lfoVisual.setTitle(destId == 4 && chorusAvailable ? "LFO > CHORUS" : "LFO MOD");

    const auto baseTooltip = tooltipMode == TooltipMode::Short
        ? juce::String("Destination: choose where the LFO is sent.")
        : tooltipMode == TooltipMode::Novice
            ? juce::String("Destination: route the LFO to tremolo, auto-pan or chorus-style movement.")
            : juce::String();
    const auto tooltip = chorusAvailable
        ? baseTooltip
        : tooltipMode == TooltipMode::Off
            ? juce::String()
            : baseTooltip + (baseTooltip.isNotEmpty() ? "\n" : juce::String())
                + "Chorus motion is unavailable on this model.";
    lfoDestinationSelector.setTooltip(tooltip);
    lfoDestinationLabel.setTooltip(tooltip);
}

void PianoSynthAudioProcessorEditor::syncFxContextMessaging()
{
    const int pianoIndex = selectedPianoFromParam();
    const auto slot = fxSlotForTab(activeFxTab);
    const bool fxAvailable = mps::isFxAvailable(pianoIndex, slot);
    const auto fxName = juce::String(kFxTabNames[activeFxTab]);
    const auto pianoName = juce::String(mps::getPianoName(pianoIndex));

    fxUnavailableLbl.setText(fxName + " is not available on " + pianoName + ".", juce::dontSendNotification);
    fxUnavailableLbl.setTooltip(fxAvailable
        ? juce::String()
        : juce::String("This effect slot stays in the UI for preset recall, but this piano model does not use it."));
}

void PianoSynthAudioProcessorEditor::syncReverbDetailAvailability()
{
    const bool reverbTabActive = activeFxTab == 0
        && mps::isFxAvailable(selectedPianoFromParam(), fxSlotForTab(activeFxTab));
    const bool plateReverb = reverbTypeSelector.getSelectedId() <= 1;
    const bool enableShapeControls = !reverbTabActive || plateReverb;

    for (int fxIndex = 0; fxIndex < 3; ++fxIndex)
    {
        const auto index = static_cast<std::size_t>(fxIndex);
        fxDials[index].setEnabled(enableShapeControls);
        fxLbls[index].setEnabled(enableShapeControls);
        fxDials[index].setAlpha(enableShapeControls ? 1.0f : 0.42f);
        fxLbls[index].setAlpha(enableShapeControls ? 1.0f : 0.55f);
    }

    if (reverbTabActive)
    {
        fxDetailTitle.setText(plateReverb
                                  ? "Reverb Parameters"
                                  : "Reverb Parameters (Mix + Predelay for Hall/Room/Chamber)",
                              juce::dontSendNotification);
    }

    const char* const* src = tooltipMode == TooltipMode::Novice ? kTooltipsNovice
                             : tooltipMode == TooltipMode::Short ? kTooltipsShort
                                                                 : nullptr;
    for (int fxIndex = 0; fxIndex < 3; ++fxIndex)
    {
        const auto index = static_cast<std::size_t>(fxIndex);
        const auto baseTooltip = src != nullptr
            ? juce::String(src[kEnvN + 2 + kMacroTotal + fxIndex])
            : juce::String();
        const auto tooltip = src == nullptr
            ? juce::String()
            : plateReverb
            ? baseTooltip
            : baseTooltip + (baseTooltip.isNotEmpty() ? "\n" : juce::String())
                + "Plate only in v1. Hall, Room and Chamber currently use Mix and Predelay only.";
        fxDials[index].setTooltip(tooltip);
        fxLbls[index].setTooltip(tooltip);
    }
}

void PianoSynthAudioProcessorEditor::switchEffectTab(int tabIndex)
{
    const int pianoIndex = selectedPianoFromParam();
    const int previousTab = activeFxTab;
    activeFxTab = juce::jlimit(0, kFxTabs - 1, tabIndex);
    const bool needsRelayout = activeFxTab != previousTab || cachedFxLayoutPiano != pianoIndex;
    cachedFxLayoutPiano = pianoIndex;

    for (int fxIndex = 0; fxIndex < kFxN; ++fxIndex)
    {
        fxDials[static_cast<std::size_t>(fxIndex)].setVisible(false);
        fxLbls[static_cast<std::size_t>(fxIndex)].setVisible(false);
    }

    for (int slot = 0; slot < kFxPerTab; ++slot)
    {
        const int fxIndex = kFxTabMap[activeFxTab][slot];
        if (fxIndex >= 0 && mps::isFxAvailable(pianoIndex, fxSlotForTab(activeFxTab)))
        {
            fxDials[static_cast<std::size_t>(fxIndex)].setVisible(true);
            fxLbls[static_cast<std::size_t>(fxIndex)].setVisible(true);
        }
    }

    const bool showDelayOptions  = activeFxTab == kDelayFxTab;
    const bool showReverbOptions = activeFxTab == 0;
    delaySyncButton.setVisible(showDelayOptions);
    delayNoteDivLabel.setVisible(showDelayOptions);
    delayNoteDivSelector.setVisible(showDelayOptions);
    reverbTypeSelector.setVisible(showReverbOptions);
    reverbTypeLabel.setVisible(showReverbOptions);
    fxDetailTitle.setText(effectDetailTitleForTab(activeFxTab), juce::dontSendNotification);

    const bool fxAvailableForModel = mps::isFxAvailable(pianoIndex, fxSlotForTab(activeFxTab));
    fxUnavailableLbl.setVisible(!fxAvailableForModel);
    fxDetailTitle.setVisible(fxAvailableForModel);

    syncFxRackState();
    syncFxContextMessaging();
    syncReverbDetailAvailability();
    if (needsRelayout)
    {
        resized();
        repaint();
    }
}

void PianoSynthAudioProcessorEditor::switchRightPanelSection(int sectionIndex)
{
    if (sectionIndex < 0 || sectionIndex >= kRightPanelSections || activeRightPanelSection == sectionIndex)
        return;

    activeRightPanelSection = sectionIndex;
    resized();
    repaint();
}

void PianoSynthAudioProcessorEditor::applyPianoTheme(int pianoIndex)
{
    const auto accent = pianoCatColour(pianoIndex);
    const auto headerTint = juce::Colour(0xff2C333B);
    const auto panelBase = juce::Colour(0xff11161B);
    const auto panelCavity = juce::Colour(0xff080E14);
    const auto panelHeader = juce::Colour(0xff1D232A);
    const auto keyboardTint = juce::Colour(0xff090D12);
    const auto controlBg = juce::Colour(0xff151A20).withAlpha(0.97f);
    const auto readoutBg = controlBg.interpolatedWith(juce::Colour(0xff080B10), 0.46f).withAlpha(0.985f);
    const auto controlText = synthcol::textSec.withAlpha(0.96f);
    const auto knobAccent = accent.interpolatedWith(juce::Colour(0xffAEBBC9), 0.20f);

    setAccentTheme(accent);
    setChromePalette(headerTint, panelBase, panelCavity, panelHeader, keyboardTint);
    lnf_.setColour(SynthLookAndFeel::knobBezelColourId, juce::Colour(0xff1D232B));
    lnf_.setColour(SynthLookAndFeel::knobCollarColourId, juce::Colour(0xff0B1016));
    lnf_.setColour(SynthLookAndFeel::knobGlowColourId, accent.withAlpha(0.74f));
    lnf_.setColour(SynthLookAndFeel::knobCapAccentColourId,
                   accent.interpolatedWith(juce::Colour(0xff9EAFC1), 0.20f));

    envVisual.setAccent(accent);
    lfoVisual.setAccent(accent);

    auto styleDial = [&](juce::Slider& dial)
    {
        dial.setColour(juce::Slider::rotarySliderFillColourId, knobAccent);
        dial.setColour(juce::Slider::textBoxTextColourId, controlText);
        dial.setColour(juce::Slider::textBoxBackgroundColourId, readoutBg);
        dial.setColour(juce::Slider::textBoxOutlineColourId, accent.withAlpha(0.24f));
    };
    auto styleCombo = [&](juce::ComboBox& combo)
    {
        combo.setColour(juce::ComboBox::backgroundColourId, controlBg);
        combo.setColour(juce::ComboBox::outlineColourId, accent.withAlpha(0.30f));
        combo.setColour(juce::ComboBox::textColourId, controlText);
        combo.setColour(juce::ComboBox::arrowColourId, accent.brighter(0.18f));
    };
    auto styleHeaderButton = [&](juce::Button& button)
    {
        button.setColour(juce::TextButton::buttonColourId, controlBg);
        button.setColour(juce::TextButton::buttonOnColourId,
                         accent.withAlpha(0.18f).interpolatedWith(controlBg, 0.62f));
        button.setColour(juce::TextButton::textColourOffId, controlText);
        button.setColour(juce::TextButton::textColourOnId, controlText);
    };

    for (auto& dial : envDials)
        styleDial(dial);
    for (auto& dial : macroDials)
        styleDial(dial);
    for (auto& dial : fxDials)
        styleDial(dial);
    styleDial(lfoRateDial);
    styleDial(lfoDepthDial);
    styleDial(gainDial);

    styleCombo(presetBox);
    styleCombo(quickTierSelector);
    styleCombo(quickRoleSelector);
    styleCombo(presetFamilyFilter);
    styleCombo(presetRoleFilter);
    styleCombo(reverbTypeSelector);
    styleCombo(modelSelector);
    styleCombo(lfoWaveSelector);
    styleCombo(lfoDestinationSelector);
    styleCombo(velocityCurveSelector);
    styleCombo(delayNoteDivSelector);

    for (auto& row : modRows)
    {
        styleCombo(row.srcCombo);
        styleCombo(row.dstCombo);
        row.amtSlider.setColour(juce::Slider::trackColourId, accent.withAlpha(0.80f));
        row.amtSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff171C22));
        row.amtSlider.setColour(juce::Slider::thumbColourId, knobAccent.brighter(0.05f));
    }

    pitchBendRangeDial.setColour(juce::Slider::trackColourId, accent.withAlpha(0.82f));
    pitchBendRangeDial.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff171C22));
    pitchBendRangeDial.setColour(juce::Slider::thumbColourId, knobAccent.brighter(0.05f));
    pitchBendRangeDial.setColour(juce::Slider::textBoxTextColourId, controlText);
    pitchBendRangeDial.setColour(juce::Slider::textBoxBackgroundColourId, readoutBg);
    pitchBendRangeDial.setColour(juce::Slider::textBoxOutlineColourId, accent.withAlpha(0.38f));

    presetFilterLabel.setColour(juce::Label::textColourId, controlText.withAlpha(0.70f));
    currentPresetSummaryLabel.setColour(juce::Label::textColourId, synthcol::text.withAlpha(0.96f));
    currentPresetMetaLabel.setColour(juce::Label::textColourId, controlText.withAlpha(0.76f));
    presetSearch.setColour(juce::TextEditor::backgroundColourId, controlBg);
    presetSearch.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2A3039));
    presetSearch.setColour(juce::TextEditor::focusedOutlineColourId, accent.withAlpha(0.48f));
    presetSearch.setColour(juce::TextEditor::textColourId, controlText);
    presetSearch.setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.20f));
    presetSearch.setColour(juce::TextEditor::highlightedTextColourId, synthcol::text);

    styleHeaderButton(prevPresetBtn);
    styleHeaderButton(nextPresetBtn);
    styleHeaderButton(quickReferenceBtn);
    styleHeaderButton(quickMixBtn);
    styleHeaderButton(quickCinematicBtn);
    styleHeaderButton(savePresetBtn);
    styleHeaderButton(saveAsPresetBtn);
    styleHeaderButton(deletePresetBtn);
    styleHeaderButton(importPresetsBtn);
    styleHeaderButton(randButton);
    styleHeaderButton(undoButton);
    styleHeaderButton(redoButton);
    styleHeaderButton(midiLearnButton);
    styleHeaderButton(arpHoldButton);
    styleCombo(arpModeSelector);
    styleCombo(arpRateSelector);
    setupSmallDial(arpOctavesDial, accent_);
    setupSmallDial(arpGateDial, accent_);
    styleHeaderButton(tooltipModeBtn);

    monoModeButton.setColour(juce::ToggleButton::tickColourId, accent);
    tremoloSyncButton.setColour(juce::ToggleButton::tickColourId, accent);
    lfoAdvancedButton.setColour(juce::ToggleButton::tickColourId, accent);
    delaySyncButton.setColour(juce::ToggleButton::tickColourId, accent);
    fxLockButton.setColour(juce::ToggleButton::tickColourId, accent);
    for (auto& button : fxBypassBtns)
        button.setColour(juce::ToggleButton::tickColourId, accent);

    modMatrixTitle.setColour(juce::Label::textColourId, accent.brighter(0.22f));
    fxDetailTitle.setColour(juce::Label::textColourId, accent.brighter(0.28f));
    velocityCurveLabel.setColour(juce::Label::textColourId, controlText.withAlpha(0.92f));
    pitchBendRangeLabel.setColour(juce::Label::textColourId, controlText.withAlpha(0.92f));
    aftertouchStatusLabel.setColour(juce::Label::textColourId, controlText.withAlpha(0.88f));
    aftertouchStatusLabel.setColour(juce::Label::backgroundColourId, readoutBg);
    aftertouchStatusLabel.setColour(juce::Label::outlineColourId, accent.withAlpha(0.24f));
    lfoDestinationLabel.setColour(juce::Label::textColourId, controlText.withAlpha(0.92f));
    delayNoteDivLabel.setColour(juce::Label::textColourId, controlText.withAlpha(0.82f));
    fxUnavailableLbl.setColour(juce::Label::textColourId, controlText.withAlpha(0.60f));

    voiceCountLabel.setColour(juce::Label::textColourId, controlText);
    voiceCountLabel.setColour(juce::Label::backgroundColourId, readoutBg);
    voiceCountLabel.setColour(juce::Label::outlineColourId, accent.withAlpha(0.24f));
    midiCCPageLabel.setColour(juce::Label::textColourId, accent.brighter(0.12f));
    midiCCPageLabel.setColour(juce::Label::backgroundColourId, readoutBg);
    midiCCPageLabel.setColour(juce::Label::outlineColourId, accent.withAlpha(0.34f));

    for (int sectionIndex = 0; sectionIndex < kRightPanelSections; ++sectionIndex)
    {
        auto& tab = rightPanelTabs[static_cast<std::size_t>(sectionIndex)];
        tab.configure(sectionIndex, kRightPanelSectionLabels[sectionIndex], accent);
        tab.setSelected(sectionIndex == activeRightPanelSection);
    }
    for (auto& rackItem : fxRackItems)
        rackItem.setAccent(accent);

    syncFxRackState();
    repaint();
}

// =============================================================================
// Mod Matrix UI — sync from processor + flush to processor
// =============================================================================
void PianoSynthAudioProcessorEditor::syncModMatrixUi()
{
    for (int i = 0; i < 8; ++i)
    {
        const auto& slot = proc.getModulationMatrix().getSlot(i);
        auto& r = modRows[static_cast<std::size_t>(i)];
        r.srcCombo .setSelectedId(static_cast<int>(slot.source)      + 1, juce::dontSendNotification);
        r.dstCombo .setSelectedId(static_cast<int>(slot.destination)  + 1, juce::dontSendNotification);
        r.amtSlider.setValue(static_cast<double>(slot.amount),             juce::dontSendNotification);
    }
}

void PianoSynthAudioProcessorEditor::flushModRow(int i)
{
    auto& r    = modRows[static_cast<std::size_t>(i)];
    proc.getModulationMatrix().setSlot(i,
        static_cast<modmatrix::Source>(r.srcCombo.getSelectedId() - 1),
        static_cast<modmatrix::Destination>(r.dstCombo.getSelectedId() - 1),
        static_cast<float>(r.amtSlider.getValue()));
}

// =============================================================================
// =============================================================================
// MIDI Learn Panel
// =============================================================================
void PianoSynthAudioProcessorEditor::MidiLearnPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1A1F28));
    g.setColour(juce::Colour(0xff3A4050));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 6.0f, 1.5f);

    g.setColour(juce::Colour(0xffC0C8D8));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.5f).withStyle("Bold")));
    g.drawText("MIDI LEARN MAPPINGS", getLocalBounds().removeFromTop(28).reduced(8, 0),
               juce::Justification::centredLeft);

    if (mappings.empty())
    {
        g.setColour(juce::Colour(0xff6070A0));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
        g.drawText("No mappings yet. Arm a parameter via MIDI LEARN\nthen move a CC controller.",
                   getLocalBounds().reduced(8, 32), juce::Justification::topLeft);
    }
}

void PianoSynthAudioProcessorEditor::MidiLearnPanel::resized()
{
    clearAllBtn.setBounds(getWidth() - 72, 4, 68, 20);

    int y = 32;
    for (std::size_t i = 0; i < clearBtns.size(); ++i)
    {
        clearBtns[i]->setBounds(getWidth() - 50, y, 44, 18);
        y += 22;
    }
}

void PianoSynthAudioProcessorEditor::showMidiLearnPanel(bool visible)
{
    midiLearnPanelVisible = visible;
    midiLearnButton.setToggleState(visible, juce::dontSendNotification);

    if (visible)
    {
        refreshMidiLearnPanel();
        addAndMakeVisible(midiLearnPanel);
        midiLearnPanel.toFront(false);
        const auto layout = computeVisualLayoutSnapshot(getWidth(), getHeight());
        const int panelW = 280;
        const int panelH = juce::jmax(80, 36 + static_cast<int>(midiLearnPanel.mappings.size()) * 22 + 4);
        midiLearnPanel.setBounds(layout.headerBounds.getRight() - panelW - 4,
                                 layout.headerBounds.getBottom() + 4,
                                 panelW, panelH);
    }
    else
    {
        midiLearnPanel.setVisible(false);
        proc.midiLearnArm({});
    }
}

void PianoSynthAudioProcessorEditor::refreshMidiLearnPanel()
{
    midiLearnPanel.mappings = proc.getMidiLearnMappings();
    midiLearnPanel.clearBtns.clear();

    for (const auto& kv : midiLearnPanel.mappings)
    {
        const int cc = kv.first;
        auto btn = std::make_unique<juce::TextButton>("X");
        btn->onClick = [this, cc] { proc.midiLearnClear(cc); refreshMidiLearnPanel(); };
        midiLearnPanel.addAndMakeVisible(*btn);
        midiLearnPanel.clearBtns.push_back(std::move(btn));
    }
    midiLearnPanel.resized();
    midiLearnPanel.repaint();
}

// Tooltip mode cycling
// =============================================================================
void PianoSynthAudioProcessorEditor::cycleTooltipMode()
{
    const auto layout = computeLayoutMetrics(getWidth(), getHeight());

    switch (tooltipMode)
    {
        case TooltipMode::Off:    tooltipMode = TooltipMode::Short;  break;
        case TooltipMode::Short:  tooltipMode = TooltipMode::Novice; break;
        case TooltipMode::Novice: tooltipMode = TooltipMode::Off;    break;
    }

    switch (tooltipMode)
    {
        case TooltipMode::Off:
        case TooltipMode::Short:
        case TooltipMode::Novice:
            tooltipModeBtn.setButtonText(layout.headerMode == PianoLayoutMetrics::HeaderLayoutMode::Wide
                ? (tooltipMode == TooltipMode::Off ? "HELP: OFF"
                                                  : tooltipMode == TooltipMode::Short ? "HELP: SHORT"
                                                                                      : "HELP: GUIDED")
                : "HELP");
            break;
    }

    tooltipWindow.setVisible(tooltipMode != TooltipMode::Off);
    applyTooltips();
}

// =============================================================================
// Apply tooltips according to current mode
// =============================================================================
void PianoSynthAudioProcessorEditor::applyTooltips()
{
    const char** src = nullptr;
    if (tooltipMode == TooltipMode::Short)  src = kTooltipsShort;
    if (tooltipMode == TooltipMode::Novice) src = kTooltipsNovice;

    const int pianoIndex = cachedAttachPianoIdx >= 0 ? cachedAttachPianoIdx : selectedPianoFromParam();
    const auto family = mps::getFamily(pianoIndex);
    const auto& profile = envProfileForPiano(pianoIndex);
    const auto sharedTooltipMode = tooltipMode == TooltipMode::Short ? synthui::TooltipMode::Short
                                 : tooltipMode == TooltipMode::Novice ? synthui::TooltipMode::Novice
                                                                      : synthui::TooltipMode::Off;
    auto modeTooltip = [this](const juce::String& shortText, const juce::String& guidedText) -> juce::String
    {
        switch (tooltipMode)
        {
            case TooltipMode::Short:  return shortText;
            case TooltipMode::Novice: return guidedText;
            case TooltipMode::Off:    break;
        }

        return {};
    };
    auto setModeTooltip = [&modeTooltip](juce::SettableTooltipClient& component,
                                         const juce::String& shortText,
                                         const juce::String& guidedText)
    {
        component.setTooltip(modeTooltip(shortText, guidedText));
    };

    synthui::applyTooltipProfile(profile, envDials, sharedTooltipMode);
    for (int i = 0; i < kEnvN; ++i)
        envLabels[static_cast<std::size_t>(i)].setTooltip(
            synthui::tooltipForMode(profile[static_cast<std::size_t>(i)], sharedTooltipMode));

    int idx = kEnvN;

    lfoRateDial .setTooltip(src ? juce::String(src[idx])     : juce::String()); ++idx;
    lfoDepthDial.setTooltip(src ? juce::String(src[idx])     : juce::String()); ++idx;

    std::array<juce::String, kMacroTotal> macroShort;
    std::array<juce::String, kMacroTotal> macroGuided;
    switch (family)
    {
        case mps::Family::Concert:
            macroShort = { "Timbre: shift from felt softness to open concert brightness.",
                           "Presence: push the upper register forward.",
                           "Body: add wood weight and low-mid resonance.",
                           "Pedal Res: increase pedal-style bloom and sustain." };
            macroGuided = { "Timbre: moves the concert piano between softer felt tone and brighter concert projection.",
                            "Presence: adds more upper harmonic focus so the piano speaks more clearly in a mix.",
                            "Body: thickens the core tone with more cabinet and soundboard weight.",
                            "Pedal Res: raises the sense of pedal resonance and sympathetic sustain around the notes." };
            break;
        case mps::Family::Vintage:
            macroShort = { "Wear: add age, grit and imperfect mechanics.",
                           "Detune: widen the pitch drift for older piano character.",
                           "Percussion: emphasize hammer knock and dry attack.",
                           "Room: add cabinet and room halo around the note." };
            macroGuided = { "Wear: increases the aged, worn and slightly unstable behaviour of the vintage piano.",
                            "Detune: spreads pitch variation for a less restored and more nostalgic tuning feel.",
                            "Percussion: brings out attack noise and woody impact for a drier, more percussive response.",
                            "Room: adds a small ambient halo so the old piano feels less close-miked and more lived-in." };
            break;
        case mps::Family::Electric:
            macroShort = { "Bark: drive the electric piano into a harder bite.",
                           "Tine/Reed: move the pickup voice between tine and reed colour.",
                           "Amp Color: shape cabinet warmth and amp edge.",
                           "Motion: add animated stereo movement and modulation feel." };
            macroGuided = { "Bark: increases the aggressive bite that appears when the electric piano is played harder.",
                            "Tine/Reed: shifts the instrument colour toward tine shimmer or reed-like nasal bite.",
                            "Amp Color: changes how much amp and cabinet tone colours the electric piano signal.",
                            "Motion: adds movement through modulation and stereo animation for a more alive stage sound." };
            break;
    }

    for (int i = 0; i < kMacroTotal; ++i, ++idx)
    {
        const auto tooltip = modeTooltip(macroShort[static_cast<std::size_t>(i)],
                                         macroGuided[static_cast<std::size_t>(i)]);
        macroDials[static_cast<std::size_t>(i)].setTooltip(tooltip);
        macroLbls[static_cast<std::size_t>(i)].setTooltip(tooltip);
    }

    for (int i = 0; i < kFxN; ++i, ++idx)
        fxDials[(size_t)i].setTooltip(src ? juce::String(src[idx]) : juce::String());

    gainDial.setTooltip(src ? juce::String(src[idx]) : juce::String());

    setModeTooltip(tooltipModeBtn,
                   "Help: cycle tooltip detail level.",
                   "Help: switch between no tooltips, short descriptions and guided descriptions.");
setModeTooltip(currentPresetSummaryLabel,
                   "Current piano: the active preset name.",
                   "Current piano: the loaded preset name. Then use DRY, MIX and ALL for quick filtering.");
    setModeTooltip(currentPresetMetaLabel,
                   "Current piano meta: tier, family and role.",
                   "Current piano meta: shows the browser tier plus the family and musical role of the current preset.");
    setModeTooltip(quickTierSelector,
                   "Preset scope: all, dry, featured or mix.",
                   "Preset scope: narrow to dry, featured, mix-ready or show all entries.");
    setModeTooltip(quickRoleSelector,
                   "Role: reference, close, featured, warm, cut, groove or character.",
                   "Role: select one of the curated factory preset roles.");
    setModeTooltip(quickReferenceBtn,
                   "Dry: jump to the reference preset.",
                   "Dry: jump to the dry reference preset for immediate playing.");
    setModeTooltip(quickMixBtn,
                   "Mix: show mix-ready presets.",
                   "Mix: show the warm, cut and groove presets for the current piano.");
    setModeTooltip(quickCinematicBtn,
                   "All: show all available presets.",
                   "All: clear scope and role filters to show all factory entries.");
    setModeTooltip(monoModeButton,
                   "Mono: one note at a time.",
                   "Mono: each new note steals the previous one for single-voice lead playing.");
    setModeTooltip(randButton,
                   "Rand: randomize the current preset slightly.",
                   "Rand: create a controlled variation of the current preset without changing the instrument model.");
    setModeTooltip(voiceCountLabel,
                   "Voices: current active polyphony.",
                   "Voices: shows how many notes are currently sounding so you can watch sustain and pedal density.");
    setModeTooltip(midiCCPageLabel,
                   "MIDI CC: current mapped control page.",
                   "MIDI CC: shows which hardware control page is currently mapped to the piano.");

    setModeTooltip(modelSelector,
                   "Model: choose the current piano model.",
                   "Model: choose the exact piano voice inside the selected family.");
    setModeTooltip(familySelector,
                   "Family: choose Concert, Vintage / Character or Electric / Piano-adjacent.",
                   "Family: switch the modeled piano/keys family, then choose a specific model on the right.");

    setModeTooltip(familyTabs[0],
                   "Concert: refined grand-style acoustic pianos.",
                   "Concert: refined acoustic piano models with longer bloom, wider body and cleaner projection.");
    setModeTooltip(familyTabs[1],
                   "Vintage / Character: Bastringue and prepared piano colours.",
                   "Vintage / Character: aged or prepared models with more character, imperfection and mechanical flavour.");
    setModeTooltip(familyTabs[2],
                   "Electric / Piano-adjacent: tine, reed and clavinet colours.",
                   "Electric / Piano-adjacent: electric piano and keyboard-adjacent models with pickup tone, bark and stage character.");

    setModeTooltip(lfoWaveSelector,
                   "Wave: choose the LFO shape.",
                   "Wave: choose the modulation shape that drives tremolo, auto-pan or chorus movement.");
    setModeTooltip(lfoDestinationSelector,
                   "Destination: choose where the LFO is sent.",
                   "Destination: route the LFO to tremolo, auto-pan or chorus-style movement.");
    setModeTooltip(lfoDestinationLabel,
                   "Destination: current LFO target.",
                   "Destination: the current destination that receives LFO movement.");
    setModeTooltip(lfoAdvancedButton,
                   "Mod Matrix: open deeper motion routing.",
                   "Mod Matrix: reveal deeper routing so motion sources can reach additional targets.");

    setModeTooltip(rightPanelTabs[0],
                   "Play: macros and direct performance shaping.",
                   "Play: the main performance page with macros, velocity response and the fastest sound-shaping controls.");
    setModeTooltip(rightPanelTabs[1],
                   "Motion: deeper routing and modulation.",
                   "Motion: assign modulation sources to destinations when you need more detailed movement control.");
    setModeTooltip(rightPanelTabs[2],
                   "Space: select and edit the effect rack.",
                   "Space: choose an effect module in the rack, then edit its parameters once the core piano is already working.");

    setModeTooltip(velocityCurveSelector,
                   "Velocity: choose the playing response curve.",
                   "Velocity: changes how your keyboard dynamics translate into piano volume and tone.");
    setModeTooltip(velocityCurveLabel,
                   "Velocity: current response curve.",
                   "Velocity: the active response curve used to interpret your playing strength.");
    setModeTooltip(pitchBendRangeDial,
                   "Bend: semitone range for pitch bend.",
                   "Bend: sets how far the pitch wheel can move the piano up or down in semitones.");
    setModeTooltip(pitchBendRangeLabel,
                   "Bend: current pitch wheel range.",
                   "Bend: current pitch bend depth applied by the wheel.");

    setModeTooltip(fxLockButton,
                   "FX Lock: keep the FX setup while changing model.",
                   "FX Lock: preserve the current effect chain and settings when switching to another piano model.");
    setModeTooltip(delaySyncButton,
                   "Sync Host: lock delay time to tempo.",
                   "Sync Host: sync the delay to the project tempo instead of using free milliseconds.");
    setModeTooltip(delayNoteDivSelector,
                   "Division: choose the synced delay note value.",
                   "Division: choose the rhythmic note value used when delay sync is active.");
    setModeTooltip(delayNoteDivLabel,
                   "Division: current synced delay value.",
                   "Division: current tempo-synced note length used by the delay.");

    for (int tabIndex = 0; tabIndex < kFxTabs; ++tabIndex)
    {
        const auto fxName = juce::String(kFxTabNames[tabIndex]);
        const auto fxSummary = juce::String(kFxRackSummaries[tabIndex]);
        setModeTooltip(fxRackItems[static_cast<std::size_t>(tabIndex)],
                       fxName + ": " + fxSummary + ". Click to edit.",
                       fxName + ": " + fxSummary + ". Select this rack module to expose its detailed controls.");
        setModeTooltip(fxBypassBtns[static_cast<std::size_t>(tabIndex)],
                       fxName + " On/Off: bypass this module.",
                       fxName + " On/Off: bypass this module while keeping its settings ready in the rack.");
    }

    syncConditionalUiState();
}
