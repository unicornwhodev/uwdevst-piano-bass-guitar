#include "PluginEditor.h"
#include "BinaryData.h"

#include <cmath>

namespace lay
{
    constexpr int W = 1180;
    constexpr int H = 800;
}

namespace
{
constexpr const char* kRightPanelSectionLabels[3] = { "MACRO & LFO", "MOD MATRIX", "FX" };

juce::StringArray makeOutputChoices()
{
    juce::StringArray outputs;
    outputs.add("Master");
    for (int i = 0; i < GuitarSynthAudioProcessor::kNumAuxOutputs; ++i)
        outputs.add("Out " + juce::String(i + 1));
    return outputs;
}

using EnvUiProfile = synthui::InstrumentUiProfile<14>;

struct GuitarLayoutMetrics
{
    bool compact = false;
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

GuitarLayoutMetrics computeLayoutMetrics(int width, int height)
{
    GuitarLayoutMetrics layout;
    layout.compact = width < 1120 || height < 700;
    layout.roomy = width > 1600 || height > 940;
    const float density = layoutDensity(layout.compact, layout.roomy);
    layout.outerMargin = layout.compact ? 16 : 24;
    layout.gutter = interpolateGap(density, 10, 14, 20);
    layout.headerH = layout.compact ? 116 : 128;  // 3-row header (audit refonte)
    layout.selectorH = layout.compact ? 82 : 86;
    layout.kbH = layout.compact
        ? juce::jlimit(72, 96, static_cast<int>(height * 0.14f))
        : juce::jlimit(84, 126, static_cast<int>(height * (layout.roomy ? 0.145f : 0.16f)));

    const int maxContentW = juce::jmin(width - layout.outerMargin * 2, 1680);
    layout.contentW = juce::jmax(920, maxContentW);
    layout.contentX = (width - layout.contentW) / 2;

    layout.selectorY = layout.outerMargin + layout.headerH + 8;
    layout.kbY = height - layout.kbH - layout.outerMargin;
    layout.bodyY = layout.selectorY + layout.selectorH + layout.gutter;
    layout.bodyH = juce::jmax(250, layout.kbY - layout.bodyY - 10);

    layout.colW = (layout.contentW - layout.gutter * 2) / 3;
    layout.col1X = layout.contentX;
    layout.col2X = layout.col1X + layout.colW + layout.gutter;
    layout.col3X = layout.col2X + layout.colW + layout.gutter;
    return layout;
}

const synthui::MacroLabelProfile<4>& macroLabelsForFamily(const mgs::Family family)
{
    static const synthui::MacroLabelProfile<4> acoustique   = { "Body", "Sparkle", "Warmth", "Air" };
    static const synthui::MacroLabelProfile<4> electrique   = { "Tone", "Bite", "Gain", "Room" };
    static const synthui::MacroLabelProfile<4> electronique = { "Texture", "Motion", "Edge", "Room" };

    switch (family)
    {
        case mgs::Family::Acoustique:   return acoustique;
        case mgs::Family::Electrique:   return electrique;
        case mgs::Family::Electronique: return electronique;
    }
    return acoustique;
}

juce::String trimValueString(double value, int decimals = 2)
{
    auto text = juce::String(value, decimals);
    while (text.contains(".") && (text.endsWith("0") || text.endsWith(".")))
    {
        if (text.endsWith("."))
        {
            text = text.dropLastCharacters(1);
            break;
        }
        text = text.dropLastCharacters(1);
    }
    return text;
}

double parseUiNumber(const juce::String& text)
{
    const auto trimmed = text.trim();
    if (trimmed.isEmpty())
        return 0.0;

    juce::String filtered;
    bool seenDecimal = false;
    bool seenSign = false;
    for (auto ch : trimmed)
    {
        if (juce::CharacterFunctions::isDigit(ch))
        {
            filtered << juce::String::charToString(ch);
            continue;
        }

        if (ch == '.' && !seenDecimal)
        {
            filtered << ".";
            seenDecimal = true;
            continue;
        }

        if ((ch == '-' || ch == '+') && filtered.isEmpty() && !seenSign)
        {
            filtered << juce::String::charToString(ch);
            seenSign = true;
        }
    }

    return filtered.isNotEmpty() ? filtered.getDoubleValue() : 0.0;
}

juce::String formatPercent01(double value)
{
    return juce::String(juce::roundToInt(juce::jlimit(0.0, 1.0, value) * 100.0)) + "%";
}

juce::String formatSignedPercent(double value)
{
    const auto rounded = juce::roundToInt(value * 100.0);
    if (std::abs(rounded) < 1)
        return "0%";
    return juce::String(rounded > 0 ? "+" : "") + juce::String(rounded) + "%";
}

juce::String formatSemitones(double value)
{
    if (std::abs(value) < 0.005)
        return "0 st";

    const auto body = trimValueString(value, std::abs(value) < 10.0 ? 1 : 0);
    return juce::String(value > 0.0 ? "+" : "") + body + " st";
}

juce::String formatTimeSeconds(double seconds)
{
    if (seconds < 1.0)
        return juce::String(juce::roundToInt(seconds * 1000.0)) + " ms";

    return trimValueString(seconds, seconds < 10.0 ? 2 : 1) + " s";
}

juce::String formatMilliseconds(double milliseconds)
{
    if (milliseconds >= 1000.0)
        return trimValueString(milliseconds / 1000.0, milliseconds < 10000.0 ? 2 : 1) + " s";

    return juce::String(juce::roundToInt(milliseconds)) + " ms";
}

juce::String formatFrequency(double hz)
{
    if (hz >= 1000.0)
        return trimValueString(hz / 1000.0, hz < 10000.0 ? 1 : 1) + " kHz";

    return juce::String(juce::roundToInt(hz)) + " Hz";
}

double parseFrequencyText(const juce::String& text)
{
    const auto lowered = text.toLowerCase();
    const auto value = parseUiNumber(lowered);
    return lowered.containsChar('k') ? value * 1000.0 : value;
}

juce::String formatPan(double value)
{
    const auto pan = juce::jlimit(-1.0, 1.0, value);
    const auto amount = juce::roundToInt(std::abs(pan) * 100.0);
    if (amount < 2)
        return "Center";
    return juce::String(pan < 0.0 ? "L " : "R ") + juce::String(amount) + "%";
}

juce::String formatDecibels(double value)
{
    const auto body = trimValueString(value, std::abs(value) < 10.0 ? 1 : 0);
    return body + " dB";
}

juce::String formatRatio(double value)
{
    return trimValueString(value, value < 10.0 ? 1 : 0) + ":1";
}

juce::String formatMultiplier(double value)
{
    return trimValueString(value, value < 10.0 ? 1 : 0) + "x";
}

void paintHeaderCaption(juce::Graphics& g, juce::Rectangle<int> area,
                        const juce::String& text, juce::Colour accent, juce::Justification just)
{
    auto band = area.withHeight(12).translated(0, -1).toFloat();
    band.setX(static_cast<float>(area.getX()));
    band.setWidth(static_cast<float>(area.getWidth()));
    g.setColour(juce::Colours::black.withAlpha(0.12f));
    g.fillRoundedRectangle(band.translated(0.0f, 1.0f), 4.0f);
    g.setColour(accent.withAlpha(0.08f));
    g.fillRoundedRectangle(band, 4.0f);
    g.setColour(synthcol::textDim.withAlpha(0.85f));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withStyle("Bold")));
    g.drawText(text, area.removeFromTop(12), just);
}

void paintKeyboardMarkers(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour accent)
{
    auto markerBand = area.reduced(72.0f, 8.0f);
    markerBand = markerBand.removeFromTop(10.0f);
    static constexpr std::array<const char*, 4> labels { "OPEN", "5TH", "7TH", "12TH" };
    static constexpr std::array<float, 4> positions { 0.08f, 0.34f, 0.48f, 0.73f };

    for (std::size_t i = 0; i < labels.size(); ++i)
    {
        const float x = markerBand.getX() + markerBand.getWidth() * positions[i];
        auto dot = juce::Rectangle<float>(x - 2.5f, markerBand.getY() + 1.0f, 5.0f, 5.0f);
        g.setColour(accent.withAlpha(i == 3 ? 0.40f : 0.24f));
        g.fillEllipse(dot);
        g.setColour(synthcol::textDim.withAlpha(0.48f));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.5f).withStyle("Bold")));
        g.drawText(labels[i],
                   juce::Rectangle<int>(static_cast<int>(x - 18.0f), static_cast<int>(markerBand.getBottom() - 1.0f), 40, 10),
                   juce::Justification::centred);
    }
}

const EnvUiProfile& envProfileForInstrument(const int instrIndex)
{
    static const EnvUiProfile acoustique = {{
        { "Level", "Output volume", "Sets the acoustic guitar output level" },
        { "Tune", "Fine tuning", "Adjusts overall guitar pitch in semitones" },
        { "String Tone", "String brightness", "Opens or darkens the string timbre" },
        { "Attack", "Attack time", "Controls note onset speed" },
        { "Decay", "Decay time", "Shortens or lengthens the decay" },
        { "Sustain", "Sustain level", "Holds the note level higher while held" },
        { "Release", "Release time", "Controls fade-out after key release" },
        { "Body", "Acoustic body", "Adjusts body resonance amount" },
        { "Warmth", "Warmth / drive", "Adds harmonic warmth without amp distortion" },
        { "Pick Snap", "Attack brightness", "Emphasises pick or fingernail snap" },
        { "Width", "Stereo width", "Widens the guitar in the stereo field" },
        { "Pick Position", "Pick position", "Moves the plucking point along the string" },
        { "Air Cut", "Low-pass filter", "Darkens or opens the high frequencies" },
        { "Pan", "Pan", "Places the guitar left or right in the mix" }
    }};

    static const EnvUiProfile electrique = {{
        { "Level", "Output volume", "Sets the electric guitar output level" },
        { "Tune", "Fine tuning", "Adjusts overall model pitch in semitones" },
        { "Pickup Brightness", "Pickup colour", "Brightens or darkens the pickup character" },
        { "Attack", "Attack time", "Controls note onset speed" },
        { "Decay", "Decay time", "Controls the descent before sustain" },
        { "Sustain", "Sustain level", "Extends the typical guitar sustain" },
        { "Release", "Release time", "Controls fade-out after key release" },
        { "Cab Resonance", "Body / cab", "Blends body resonance or speaker cabinet feel" },
        { "Amp Drive", "Amp drive", "Pushes the preamp into saturation" },
        { "Attack Bite", "Attack bite", "Adds edge and bite to the pick attack" },
        { "Width", "Stereo width", "Widens the guitar in the mix" },
        { "Pickup Position", "Pickup position", "Moves pickup blend between bridge and neck" },
        { "Tone Cut", "Low-pass filter", "Closes or opens the overall tone" },
        { "Pan", "Pan", "Places the guitar left or right in the mix" }
    }};

    static const EnvUiProfile electronique = {{
        { "Level", "Output volume", "Sets the guitar-synth texture output level" },
        { "Tune", "Fine tuning", "Adjusts overall hybrid model pitch" },
        { "Texture", "Harmonic texture", "Opens or darkens the harmonic content" },
        { "Attack", "Attack time", "Controls texture onset speed" },
        { "Decay", "Decay time", "Shortens or lengthens the decay" },
        { "Sustain", "Sustain level", "Maintains a more stable sound bed" },
        { "Release", "Release time", "Lets the texture fade after key release" },
        { "Resonance", "Body resonance", "Adds body or synthetic resonance feel" },
        { "Drive", "Drive", "Adds grain and harmonic density" },
        { "Attack Edge", "Attack edge", "Emphasises attack and transient presence" },
        { "Spread", "Stereo spread", "Spreads the texture wider in the space" },
        { "Excitation Position", "Excitation position", "Moves the excitation point on the virtual string" },
        { "Filter", "Low-pass filter", "Darkens or opens the texture" },
        { "Pan", "Pan", "Places the instrument left or right in the mix" }
    }};

    switch (mgs::getFamily(instrIndex))
    {
        case mgs::Family::Acoustique:   return acoustique;
        case mgs::Family::Electrique:   return electrique;
        case mgs::Family::Electronique: return electronique;
    }
    return acoustique;
}

juce::String instrumentDisplayName(int instrIndex)
{
    switch (instrIndex)
    {
        case 0: return "Folk Steel";
        case 1: return "12-String";
        case 2: return "Flamenco";
        case 3: return "Clean";
        case 4: return "Crunch";
        case 5: return "Lead";
        case 6: return "Synth Guitar";
        case 7: return "E-Guitar Pad";
        case 8: return "Guitar Ambient";
        default: break;
    }

    return juce::String(juce::CharPointer_UTF8(mgs::getInstrName(instrIndex)));
}

bool isTexturePresetMetadata(const mgs::PresetMetadata& metadata)
{
    if (metadata.familyLabel == "hybrid" || metadata.mixRole == "texture")
        return true;

    return std::find(metadata.tags.begin(), metadata.tags.end(), "texture") != metadata.tags.end();
}

bool hasPresetTag(const mgs::PresetMetadata& metadata, const char* tag)
{
    return std::find(metadata.tags.begin(), metadata.tags.end(), tag) != metadata.tags.end();
}

void drawStatusChip(juce::Graphics& g, juce::Rectangle<int> area,
                    const juce::String& text, juce::Colour fill, juce::Colour outline)
{
    auto r = area.toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.24f));
    g.fillRoundedRectangle(r.translated(0.0f, 2.0f), 6.0f);

    juce::ColourGradient chipGrad(fill.brighter(0.08f), r.getCentreX(), r.getY(),
                                  fill.darker(0.18f), r.getCentreX(), r.getBottom(), false);
    chipGrad.addColour(0.46, fill);
    g.setGradientFill(chipGrad);
    g.fillRoundedRectangle(r, 6.0f);

    auto sheen = r.withHeight(r.getHeight() * 0.48f);
    juce::ColourGradient sheenGrad(juce::Colours::white.withAlpha(0.07f), sheen.getCentreX(), sheen.getY(),
                                   juce::Colours::transparentWhite, sheen.getCentreX(), sheen.getBottom(), false);
    g.setGradientFill(sheenGrad);
    g.fillRoundedRectangle(sheen, 6.0f);

    g.setColour(outline.withAlpha(0.88f));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);
    g.setColour(synthcol::text);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f).withStyle("Bold")));
    g.drawText(text, area, juce::Justification::centred);
}

void drawMeterBar(juce::Graphics& g, juce::Rectangle<int> area, float level, juce::Colour colour)
{
    auto r = area.toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.fillRoundedRectangle(r.translated(0.0f, 2.0f), 4.0f);

    juce::ColourGradient bgGrad(synthcol::bg.brighter(0.05f), r.getCentreX(), r.getY(),
                                synthcol::surface.darker(0.18f), r.getCentreX(), r.getBottom(), false);
    bgGrad.addColour(0.46, synthcol::surface.withAlpha(0.92f));
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(synthcol::border.withAlpha(0.42f));
    g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

    const auto clamped = juce::jlimit(0.0f, 1.0f, level);
    const int segments = 12;
    const float gap = 2.0f;
    const float segW = (r.getWidth() - gap * (segments - 1)) / static_cast<float>(segments);
    const int active = static_cast<int>(std::round(clamped * static_cast<float>(segments)));
    for (int i = 0; i < segments; ++i)
    {
        auto seg = juce::Rectangle<float>(r.getX() + i * (segW + gap), r.getY() + 1.5f,
                                          segW, r.getHeight() - 3.0f);
        const bool lit = i < active;
        g.setColour(lit ? colour.withAlpha(0.90f) : synthcol::border.withAlpha(0.34f));
        g.fillRoundedRectangle(seg, 1.2f);
        if (lit)
        {
            g.setColour(colour.brighter(0.35f).withAlpha(0.26f));
            g.fillRoundedRectangle(seg.expanded(0.5f, 0.2f), 1.4f);
        }
    }
}

void glazeGuitarChrome(juce::Graphics& g,
                       juce::Rectangle<float> area,
                       juce::Colour accent,
                       float radius,
                       float intensity)
{
    const auto woodTop = juce::Colour(0xff7A593F).withAlpha(0.030f * intensity);
    const auto woodMid = juce::Colour(0xff5B4031).withAlpha(0.046f * intensity);
    const auto woodBottom = juce::Colour(0xff171316).withAlpha(0.105f * intensity);

    juce::ColourGradient warm(woodTop, area.getCentreX(), area.getY(),
                              woodBottom, area.getCentreX(), area.getBottom(), false);
    warm.addColour(0.34, woodTop.interpolatedWith(woodMid, 0.55f));
    warm.addColour(0.64, woodMid);
    g.setGradientFill(warm);
    g.fillRoundedRectangle(area, radius);

    auto grainArea = area.reduced(8.0f, 6.0f);
    for (int i = 1; i <= 4; ++i)
    {
        const float y = grainArea.getY() + grainArea.getHeight() * (0.12f + 0.18f * static_cast<float>(i));
        g.setColour(juce::Colours::white.withAlpha(0.007f * intensity));
        g.drawHorizontalLine(static_cast<int>(std::round(y)),
                             grainArea.getX(),
                             grainArea.getRight());
        g.setColour(juce::Colours::black.withAlpha(0.020f * intensity));
        g.drawHorizontalLine(static_cast<int>(std::round(y + 1.0f)),
                             grainArea.getX() + 10.0f,
                             grainArea.getRight() - 8.0f);
    }

    auto sheen = area.reduced(2.4f).withHeight(juce::jmax(8.0f, area.getHeight() * 0.16f));
    juce::ColourGradient highlight(juce::Colours::white.withAlpha(0.020f * intensity), sheen.getCentreX(), sheen.getY(),
                                   juce::Colours::transparentWhite, sheen.getCentreX(), sheen.getBottom(), false);
    g.setGradientFill(highlight);
    g.fillRoundedRectangle(sheen, juce::jmax(0.0f, radius - 2.0f));

    g.setColour(accent.withAlpha(0.028f * intensity));
    g.drawRoundedRectangle(area.reduced(1.0f), juce::jmax(0.0f, radius - 1.0f), 0.9f);
}

void paintControlWell(juce::Graphics& g,
                      juce::Rectangle<float> area,
                      juce::Colour tint,
                      juce::Colour accent,
                      float radius)
{
    juce::ColourGradient wellGrad(juce::Colour(0xff1A2026).interpolatedWith(tint, 0.16f), area.getCentreX(), area.getY(),
                                  juce::Colour(0xff101419).interpolatedWith(tint, 0.08f), area.getCentreX(), area.getBottom(), false);
    wellGrad.addColour(0.52, juce::Colour(0xff151A20).interpolatedWith(tint, 0.12f));
    g.setGradientFill(wellGrad);
    g.fillRoundedRectangle(area, radius);

    auto sheen = area.reduced(8.0f, 5.0f).removeFromTop(juce::jmax(8.0f, area.getHeight() * 0.20f));
    juce::ColourGradient sheenGrad(juce::Colours::white.withAlpha(0.016f), sheen.getCentreX(), sheen.getY(),
                                   juce::Colours::transparentWhite, sheen.getCentreX(), sheen.getBottom(), false);
    g.setGradientFill(sheenGrad);
    g.fillRoundedRectangle(sheen, juce::jmax(0.0f, radius - 4.0f));

    g.setColour(accent.withAlpha(0.055f));
    g.drawRoundedRectangle(area.reduced(0.7f), radius, 0.9f);
    g.setColour(juce::Colours::black.withAlpha(0.24f));
    g.drawRoundedRectangle(area.expanded(0.1f), radius + 0.3f, 0.9f);
}
}

const std::array<GuitarSynthAudioProcessorEditor::CtrlDef,
                 GuitarSynthAudioProcessorEditor::kEnvN>
    GuitarSynthAudioProcessorEditor::kEnvCtrls = {{
        { "Level",        "level" },
        { "Tune",         "tune" },
        { "Brightness",   "string_brightness" },
        { "Attack",       "attack" },
        { "Decay",        "decay" },
        { "Sustain",      "sustain" },
        { "Release",      "release" },
        { "Body Amount",  "body_amount" },
        { "Drive",        "drive" },
        { "Attack Bite",  "attack_brightness" },
        { "Stereo Width", "stereo_width" },
        { "Pickup Position", "pick_position" },
        { "Tone Cut",     "low_pass_hz" },
        { "Pan",          "pan" }
    }};

const std::array<GuitarSynthAudioProcessorEditor::FxDef,
                 GuitarSynthAudioProcessorEditor::kMacroTotal>
    GuitarSynthAudioProcessorEditor::kMacroCtrls = {{
        { "Tone",    "macro_corps" },
        { "Bite",    "macro_brillance" },
        { "Gain",    "macro_gain" },
        { "Room",    "macro_espace" }
    }};

const std::array<GuitarSynthAudioProcessorEditor::FxDef,
                 GuitarSynthAudioProcessorEditor::kFxN>
    GuitarSynthAudioProcessorEditor::kFxCtrls = {{
        { "Size",       "reverb_size" },
        { "Damping",    "reverb_damping" },
        { "Width",      "reverb_width" },
        { "Mix",        "reverb_mix" },
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
        { "Delay",      "chorus_delay" },
        { "Mix",        "chorus_mix" },
        { "Time",       "delay_time" },
        { "Feedback",   "delay_feedback" },
        { "Mix",        "delay_mix" },
        { "Threshold",  "limiter_threshold" },
        { "Release",    "limiter_release" },
        { "Mix",        "cab_mix" }
    }};

const char* GuitarSynthAudioProcessorEditor::kFxTabNames[kFxTabs] = {
    "REVERB", "SATURATION", "TRANSIENT", "COMPRESSOR", "EQ",
    "CHORUS", "DELAY", "LIMITER", "CABINET"
};

const char* GuitarSynthAudioProcessorEditor::kFxRackSummaries[kFxTabs] = {
    "Space and width",
    "Harmonic drive",
    "Attack contour",
    "Dynamics control",
    "Tone balancing",
    "Stereo motion",
    "Echo repeats",
    "Output ceiling",
    "Speaker colour"
};

const char* GuitarSynthAudioProcessorEditor::kFxBypassParamIds[kFxTabs] = {
    "fx_reverb_en",
    "fx_tab0_en",
    "fx_tab1_en",
    "fx_tab2_en",
    "fx_eq_en",
    "fx_chorus_en",
    "fx_delay_en",
    "fx_limiter_en",
    "fx_cabinet_en"
};

// =============================================================================
// Tooltip texts  (index order: envDials[0..13], lfoRate, lfoDepth,
//                 macroDials[0..3], fxDials[0..31], gainDial)
// =============================================================================
const char* GuitarSynthAudioProcessorEditor::kTooltipsShort[kTooltipCount] = {
    // env 0-13
    "Guitar output volume (0 - 100 %)",
    "Fine tuning in semitones (-24 to +24)",
    "String brightness - harmonic content",
    "Envelope attack time (0 - 2 s)",
    "Decay time after peak (0.1 - 10 s)",
    "Envelope sustain level (0 - 100 %)",
    "Release time after key release",
    "Body resonance intensity",
    "Harmonic saturation of the signal",
    "Attack brightness - harmonic content at note start",
    "Stereo width of the sound",
    "Pick / finger position along the string",
    "Low-pass filter cutoff frequency (120 - 18 000 Hz)",
    "Guitar stereo position (-1 left, +1 right)",
    // lfo 14-15
    "LFO speed (0.05 - 12 Hz)",
    "LFO modulation depth",
    // macro 16-19
    "Macro Tone - overall body and warmth",
    "Macro Bite - attack liveliness",
    "Macro Gain - overall signal power",
    "Macro Room - stereo width and depth",
    // fx 20-51
    "Reverb room size",
    "High-frequency damping in the reverb",
    "Reverb stereo width",
    "Reverb dry / wet balance",
    "Harmonic saturation intensity",
    "Saturation dry / wet balance",
    "Transient attack emphasis",
    "Transient sustain control",
    "Transient shaper dry / wet balance",
    "Compressor threshold (-60 to 0 dB)",
    "Compression ratio (1:1 to 20:1)",
    "Compressor attack time (0.1 - 100 ms)",
    "Compressor release time (5 - 500 ms)",
    "Compressor makeup gain (0 - 24 dB)",
    "Compressor dry / wet balance",
    "Low shelf filter frequency",
    "Low shelf gain (dB)",
    "Mid peak filter frequency",
    "Mid peak gain (dB)",
    "Mid filter Q factor - bandwidth",
    "High shelf filter frequency",
    "High shelf gain (dB)",
    "Chorus modulation speed (0.1 - 5 Hz)",
    "Chorus modulation depth",
    "Chorus modulation delay",
    "Chorus dry / wet balance",
    "Delay time (1 - 2000 ms)",
    "Delay feedback (0 - 95 %)",
    "Delay dry / wet balance",
    "Output limiter threshold (-12 to 0 dB)",
    "Limiter release time (1 - 200 ms)",
    "Cabinet dry / wet balance",
    // gain 52
    "Global output gain (-24 to +12 dB)"
};

const char* GuitarSynthAudioProcessorEditor::kTooltipsNovice[kTooltipCount] = {
    // env 0-13
    "Level - Selected guitar volume. Turn up for louder, down for softer.",
    "Tune - Fine-tunes the guitar in semitones. Useful when matching other instruments.",
    "Brightness - String brightness. Higher = metallic and clear, lower = matte and soft.",
    "Attack - Speed at which the sound rises at the start. Short = sharp attack, long = soft entry.",
    "Decay - How long the sound descends after the initial peak before sustain.",
    "Sustain - Level held while you hold the key. 100% = no decay.",
    "Release - Time for the sound to fade after releasing the key. Short = tight, long = resonant.",
    "Body Amount - Body resonance intensity. Higher = fuller, more woody sound.",
    "Drive - Adds harmonic saturation. Simulates a pushed amp or an aggressive guitar.",
    "Atk Bright - Brightness at note start. Higher = more percussive and cutting attack.",
    "Stereo Width - Widens the sound in the stereo field. 0 = mono, 1 = full stereo.",
    "Pickup Position - Simulates pickup position. Toward 0 = bridge (bright), toward 1 = neck (warm).",
    "Low Pass - Low-pass filter frequency. Lower to soften the sound, higher to let highs through.",
    "Pan - Position in the stereo field. -1 = hard left, 0 = centre, +1 = hard right.",
    // lfo 14-15
    "LFO Rate - Low-frequency oscillator speed. Controls vibrato or tremolo rate.",
    "LFO Depth - Modulation intensity. Higher = more pronounced effect.",
    // macro 16-19
    "Macro Tone - Controls overall warmth. Turn right for a rounder, warmer sound.",
    "Macro Bite - Controls attack liveliness. Higher = more percussive and present.",
    "Macro Gain - Overall signal power. Useful to boost the signal before effects.",
    "Macro Room - Stereo width and depth. Higher = more spatial and immersive.",
    // fx 20-51
    "Reverb Size - Simulated room size. Large = long tail, small = short ambience.",
    "Reverb Damping - High-frequency absorption in the reverb. Higher = darker reverb.",
    "Reverb Width - Reverb stereo opening. 0 = mono, 1 = wide.",
    "Reverb Mix - Balance between direct and reverb. 0% = dry, 100% = fully wet.",
    "Sat Drive - Saturation amount. Simulates the warmth of a tube amp.",
    "Sat Mix - Balance between clean and saturated signal.",
    "Transient Attack - Emphasises the initial hit. Useful for fingerpicking or strumming.",
    "Transient Sustain - Controls sustain after the attack. Lower = shorter, more percussive.",
    "Transient Mix - Transient shaper balance. 0% = no effect.",
    "Comp Threshold - Level above which the compressor acts. Lower = more compression.",
    "Comp Ratio - Compression strength. 2:1 = gentle, 10:1 = heavy, 20:1 = near-limiter.",
    "Comp Attack - Compressor reaction speed. Short = preserves transients, long = smooth.",
    "Comp Release - Compressor release time. Short = pumping, long = natural.",
    "Comp Makeup - Gain after compression to compensate for volume reduction.",
    "Comp Mix - Parallel compressor balance. 50% = parallel compression (NY style).",
    "EQ Low Freq - Low shelf filter cutoff. Typical: 80-300 Hz.",
    "EQ Low Gain - Boosts or cuts bass. Positive = more bass, negative = less.",
    "EQ Mid Freq - Mid peak centre. Adjust to target body (400 Hz) or presence (2 kHz).",
    "EQ Mid Gain - Boosts or cuts midrange. Useful for sculpting guitar character.",
    "EQ Mid Q - Mid bandwidth. Low Q = wide and gentle, high Q = narrow and precise.",
    "EQ High Freq - High shelf filter frequency. Typical: 3-12 kHz.",
    "EQ High Gain - Boosts or cuts highs. Positive = more brilliance, negative = darker.",
    "Chorus Rate - Chorus modulation speed. Slow = gentle ripple, fast = vibrato.",
    "Chorus Depth - Modulation intensity. Higher = more pronounced effect.",
    "Chorus Delay - Chorus base delay. Affects character (short = flanging, long = chorus).",
    "Chorus Mix - Balance between dry and chorused signal. 50% is a good starting point.",
    "Delay Time - Time between repeats. Short = slapback, long = spaced echoes.",
    "Delay Feedback - Number of repeats. Higher = more repeats (watch the feedback!).",
    "Delay Mix - Balance between direct and echo. Light for rhythm, more for atmosphere.",
    "Limiter Threshold - Final limiter ceiling. Prevents the signal from exceeding this level.",
    "Limiter Release - Limiter release speed. Short = transparent, long = smooth.",
    "Cab Mix - Speaker simulation. Adds the colour of a guitar cabinet.",
    // gain 52
    "Output Gain - Final plugin output volume. Adjust to match the level in your mix."
};

juce::Colour GuitarSynthAudioProcessorEditor::familyColour(int familyIndex)
{
    switch (familyIndex)
    {
        case 0: return juce::Colour(0xffC8873A);
        case 1: return juce::Colour(0xff2BA89A);
        case 2: return juce::Colour(0xff5F80D9);
        default: return juce::Colour(0xff2BA89A);
    }
}

juce::Colour GuitarSynthAudioProcessorEditor::instrCatColour(int instrIndex)
{
    static constexpr juce::uint32 colours[mgs::kNumInstruments] = {
        0xffC8873A, 0xffD29A52, 0xffE0B65C,
        0xff2BA89A, 0xff1FA7C6, 0xff5EC7B8,
        0xff5F80D9, 0xff7A6EE8, 0xffA95FD6
    };

    return juce::Colour(colours[static_cast<std::size_t>(juce::jlimit(0, mgs::kNumInstruments - 1, instrIndex))]);
}

GuitarSynthAudioProcessorEditor::InstrumentPalette GuitarSynthAudioProcessorEditor::paletteForInstrument(int instrIndex)
{
    auto makePalette = [](juce::Colour accent,
                          juce::Colour panelBase,
                          juce::Colour panelCavity,
                          juce::Colour panelHeader,
                          juce::Colour controlBg,
                          juce::Colour knobRing,
                          juce::Colour knobGlow) -> InstrumentPalette
    {
        return {
            accent,
            panelBase,
            panelCavity,
            panelHeader,
            controlBg,
            knobRing,
            knobGlow,
            juce::Colour(0xffE8EEF3).interpolatedWith(accent, 0.08f),
            juce::Colour(0xffAAB7C5).interpolatedWith(accent, 0.18f)
        };
    };

    switch (juce::jlimit(0, mgs::kNumInstruments - 1, instrIndex))
    {
        case 0: return makePalette(juce::Colour(0xffC8873A), juce::Colour(0xff6A4423), juce::Colour(0xff4C3220),
                                   juce::Colour(0xff8B5A2C), juce::Colour(0xff251C18), juce::Colour(0xffD89B4A),
                                   juce::Colour(0xffE0A85E));
        case 1: return makePalette(juce::Colour(0xffD29A52), juce::Colour(0xff75502C), juce::Colour(0xff563922),
                                   juce::Colour(0xff9E6D34), juce::Colour(0xff2A201A), juce::Colour(0xffE3B464),
                                   juce::Colour(0xffE9BE74));
        case 2: return makePalette(juce::Colour(0xffE0B65C), juce::Colour(0xff74471F), juce::Colour(0xff5E341D),
                                   juce::Colour(0xffA9692A), juce::Colour(0xff2B1B16), juce::Colour(0xffF0C76B),
                                   juce::Colour(0xffF2D17E));
        case 3: return makePalette(juce::Colour(0xff2BA89A), juce::Colour(0xff1C4A48), juce::Colour(0xff153736),
                                   juce::Colour(0xff2A6C68), juce::Colour(0xff182325), juce::Colour(0xff42C6B7),
                                   juce::Colour(0xff56D9CD));
        case 4: return makePalette(juce::Colour(0xff1FA7C6), juce::Colour(0xff164454), juce::Colour(0xff123540),
                                   juce::Colour(0xff1B657A), juce::Colour(0xff16242A), juce::Colour(0xff38C1DF),
                                   juce::Colour(0xff51D5EE));
        case 5: return makePalette(juce::Colour(0xff5EC7B8), juce::Colour(0xff1E4950), juce::Colour(0xff16353A),
                                   juce::Colour(0xff2C6E72), juce::Colour(0xff172428), juce::Colour(0xff7ADFD1),
                                   juce::Colour(0xff8BE9DC));
        case 6: return makePalette(juce::Colour(0xff5F80D9), juce::Colour(0xff243B67), juce::Colour(0xff1B2B4C),
                                   juce::Colour(0xff38508D), juce::Colour(0xff171E29), juce::Colour(0xff7B9CF0),
                                   juce::Colour(0xff8AA8FF));
        case 7: return makePalette(juce::Colour(0xff7A6EE8), juce::Colour(0xff302C6F), juce::Colour(0xff231F52),
                                   juce::Colour(0xff4B43A0), juce::Colour(0xff191A2A), juce::Colour(0xff998FFF),
                                   juce::Colour(0xffADA6FF));
        case 8: return makePalette(juce::Colour(0xffA95FD6), juce::Colour(0xff442C63), juce::Colour(0xff321F4C),
                                   juce::Colour(0xff6A3E8D), juce::Colour(0xff1E1926), juce::Colour(0xffC07EEF),
                                   juce::Colour(0xffD39CFF));
        default: break;
    }

    return makePalette(juce::Colour(0xff2BA89A), juce::Colour(0xff1C4A48), juce::Colour(0xff153736),
                       juce::Colour(0xff2A6C68), juce::Colour(0xff182325), juce::Colour(0xff42C6B7),
                       juce::Colour(0xff56D9CD));
}

int GuitarSynthAudioProcessorEditor::selectedInstrFromParam() const
{
    if (auto* raw = proc.getAPVTS().getRawParameterValue("selected_instr"))
        return juce::jlimit(0, mgs::kNumInstruments - 1,
                            static_cast<int>(std::round(raw->load())));
    return 0;
}

juce::StringArray GuitarSynthAudioProcessorEditor::hostGetFactoryNames()
{
    return proc.getFactoryPresetNames();
}

juce::Array<juce::File> GuitarSynthAudioProcessorEditor::hostScanUserPresets()
{
    return proc.scanUserPresets();
}

bool GuitarSynthAudioProcessorEditor::hostIsUserPreset()
{
    return proc.isCurrentPresetUser();
}

juce::File GuitarSynthAudioProcessorEditor::hostCurrentUserFile()
{
    return proc.getCurrentUserPresetFile();
}

int GuitarSynthAudioProcessorEditor::hostCurrentFactoryIdx()
{
    return proc.getCurrentFactoryPresetIndex();
}

void GuitarSynthAudioProcessorEditor::hostApplyFactory(int idx)
{
    proc.applyFactoryPreset(idx);
}

void GuitarSynthAudioProcessorEditor::hostLoadUser(const juce::File& f)
{
    proc.loadUserPreset(f);
}

bool GuitarSynthAudioProcessorEditor::hostSaveUser(const juce::String& name)
{
    return proc.saveUserPreset(name);
}

void GuitarSynthAudioProcessorEditor::hostUpdateUser(const juce::File& f)
{
    proc.updateUserPreset(f);
}

void GuitarSynthAudioProcessorEditor::hostSaveFactory(int idx)
{
    proc.saveFactoryPreset(idx);
}

void GuitarSynthAudioProcessorEditor::hostDeleteUser(const juce::File& f)
{
    proc.deleteUserPreset(f);
}

juce::File GuitarSynthAudioProcessorEditor::hostGetUserPresetsDir()
{
    return GuitarSynthAudioProcessor::getUserPresetsDirectory(proc.getSelectedInstrIndex());
}

juce::File GuitarSynthAudioProcessorEditor::hostGetUserPresetsDirForIndex(int instrumentIndex)
{
    return GuitarSynthAudioProcessor::getUserPresetsDirectory(instrumentIndex);
}

juce::String GuitarSynthAudioProcessorEditor::hostPresetInstrumentAttr() const
{
    return "instrIndex";
}

juce::String GuitarSynthAudioProcessorEditor::hostFormatFactoryPresetLabel(int presetIndex,
                                                                           const juce::String& displayName) const
{
    const int instrIdx = proc.getSelectedInstrIndex();
    const auto& banks = mgs::getFactoryPresetBanks();
    if (instrIdx < 0 || instrIdx >= mgs::kNumInstruments)
        return displayName;

    const auto& bank = banks[static_cast<std::size_t>(instrIdx)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size()))
        return displayName;

    const auto& metadata = bank[static_cast<std::size_t>(presetIndex)].metadata;
    juce::String usageLabel;
    if (hasPresetTag(metadata, "muted"))
        usageLabel = "Muted";
    else if (hasPresetTag(metadata, "dry"))
        usageLabel = "Dry";
    else if (hasPresetTag(metadata, "writer"))
        usageLabel = "Writer";
    else if (hasPresetTag(metadata, "showcase"))
        usageLabel = "Showcase";

    juce::String roleLabel;
    if (metadata.mixRole == "pluck")
        roleLabel = "Picked";
    else if (metadata.mixRole == "rhythm")
        roleLabel = "Rhythm";
    else if (metadata.mixRole == "lead")
        roleLabel = "Lead";
    else if (metadata.mixRole == "texture")
        roleLabel = "Texture";

    juce::StringArray parts;
    if (usageLabel.isNotEmpty())
        parts.add(usageLabel);
    if (roleLabel.isNotEmpty() && roleLabel != usageLabel)
        parts.add(roleLabel);
    parts.add(displayName);
    return parts.joinIntoString(" / ");
}

juce::String GuitarSynthAudioProcessorEditor::hostFactoryPresetSearchText(int presetIndex,
                                                                          const juce::String& displayName) const
{
    const int instrIdx = proc.getSelectedInstrIndex();
    const auto& banks = mgs::getFactoryPresetBanks();
    if (instrIdx < 0 || instrIdx >= mgs::kNumInstruments)
        return displayName;

    const auto& bank = banks[static_cast<std::size_t>(instrIdx)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size()))
        return displayName;

    const auto& metadata = bank[static_cast<std::size_t>(presetIndex)].metadata;
    juce::StringArray searchable;
    searchable.add(displayName);
    searchable.add(juce::String(juce::CharPointer_UTF8(metadata.mixRole.c_str())));
    searchable.add(juce::String(juce::CharPointer_UTF8(metadata.familyLabel.c_str())));
    for (const auto& tag : metadata.tags)
        searchable.add(juce::String(tag));
    return searchable.joinIntoString(" ");
}

GuitarSynthAudioProcessorEditor::GuitarSynthAudioProcessorEditor(
    GuitarSynthAudioProcessor& processor)
    : CommonSynthEditor(processor,
                        processor.getAPVTS(),
                        processor.getKeyboardState(),
                        juce::Colour(0xff2BA89A),
                        36, 84, 38.0f)
    , proc(processor)
{
    familySelectorLbl.setText("FAMILY", juce::dontSendNotification);
    modelSelectorLbl.setText("MODEL", juce::dontSendNotification);
    modelSelector.setComponentID("modelSelector");

    familySelector.addItem("ACOUSTIC", 1);
    familySelector.addItem("ELECTRIC", 2);
    familySelector.addItem("HYBRID", 3);

    familySelector.onChange = [this]
    {
        const int fi = juce::jlimit(0, mgs::kNumFamilies - 1,
                                    familySelector.getSelectedId() - 1);
        activeFamilyIndex = fi;
        rebuildModelSelectorForFamily(activeFamilyIndex);
        const int selectedId = modelSelector.getSelectedId();
        if (selectedId > 0)
            instrSelector.setSelectedId(selectedId);
    };

    modelSelector.onChange = [this]
    {
        const int selectedId = modelSelector.getSelectedId();
        if (selectedId > 0)
            instrSelector.setSelectedId(selectedId);
    };

    static const char* kFamilyNames[] = { "ACOUSTIC", "ELECTRIC", "HYBRID" };
    for (int f = 0; f < mgs::kNumFamilies; ++f)
    {
        familyTabs[static_cast<std::size_t>(f)].configure(f, kFamilyNames[f], familyColour(f));
        familyTabs[static_cast<std::size_t>(f)].onClicked = [this](int idx) { familySelector.setSelectedId(idx + 1, juce::sendNotificationSync); };
        familyTabs[static_cast<std::size_t>(f)].setVisible(false);
        addChildComponent(familyTabs[static_cast<std::size_t>(f)]);
    }

    instrSelector.setVisible(false);
    addChildComponent(instrSelector);
    for (int i = 0; i < mgs::kNumInstruments; ++i)
        instrSelector.addItem(instrumentDisplayName(i), i + 1);
    selInstrAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "selected_instr", instrSelector);
    instrSelector.onChange = [this] {
        rebuildInstrAttachments();
        syncSelectionUiFromInstr();
    };

    for (int i = 0; i < kEnvN; ++i)
    {
        const auto si = static_cast<std::size_t>(i);
        switch (i)
        {
            case 0:
            case 2:
            case 5:
            case 7:
            case 9:
            case 10:
            case 11:
                setupDial(envDials[si], accent_);
                break;
            case 1:
                setupDial(envDials[si], accent_);
                break;
            case 3:
            case 4:
            case 6:
                setupDial(envDials[si], accent_);
                break;
            case 8:
                setupDial(envDials[si], accent_);
                break;
            case 12:
                // No suffix: textFromValueFunction (formatFrequency) already appends "Hz"/"kHz",
                // and JUCE appends getTextValueSuffix() after the custom formatter's text, so
                // passing " Hz" here produced a doubled unit ("X kHz Hz").
                setupGrandDial(envDials[si], accent_, {});
                break;
            case 13:
                setupDial(envDials[si], accent_);
                break;
            default:
                setupDial(envDials[si], accent_);
                break;
        }

        addAndMakeVisible(envDials[si]);
        envLabels[si].setText(kEnvCtrls[si].label, juce::dontSendNotification);
        envLabels[si].setJustificationType(juce::Justification::centred);
        envLabels[si].setFont(juce::Font(juce::FontOptions{}.withHeight(10.4f)));
        envLabels[si].setColour(juce::Label::textColourId, synthcol::textSec.withAlpha(0.84f));
        addAndMakeVisible(envLabels[si]);
    }

    envVisual.setAccent(accent_);
    envVisual.bindAdsr(&envDials[3], &envDials[4], &envDials[5], &envDials[6]);
    addAndMakeVisible(envVisual);

    lfoVisual.setAccent(accent_);
    lfoVisual.setTitle("LFO");
    setupSmallDial(lfoRateDial, accent_);
    setupSmallDial(lfoDepthDial, accent_);
    addChildComponent(lfoRateDial);
    addChildComponent(lfoDepthDial);
    addChildComponent(lfoWaveSelector);
    lfoWaveSelector.addItem("SINE", 1);
    lfoWaveSelector.addItem("TRI", 2);
    lfoWaveSelector.addItem("SAW", 3);
    lfoWaveSelector.addItem("SQR", 4);
    lfoRateAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "lfo_rate", lfoRateDial);
    lfoDepthAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "lfo_depth", lfoDepthDial);
    lfoWaveAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "lfo_wave", lfoWaveSelector);
    lfoVisual.bindRateDepth(&lfoRateDial, &lfoDepthDial);
    lfoVisual.setWaveformIndex(juce::jmax(0, lfoWaveSelector.getSelectedId() - 1));
    lfoWaveSelector.onChange = [this] {
        lfoVisual.setWaveformIndex(juce::jmax(0, lfoWaveSelector.getSelectedId() - 1));
    };
    lfoVisual.onWaveformChanged = [this](int wi) {
        lfoWaveSelector.setSelectedId(wi + 1, juce::sendNotificationSync);
    };
    addAndMakeVisible(lfoVisual);

    qualitySelector.addItem("SAT LIVE", 1);
    qualitySelector.addItem("SAT STUDIO", 2);
    qualityAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "quality_mode", qualitySelector);
    qualitySelector.setTooltip("Saturator quality mode");
    qualitySelector.setComponentID("qualitySelector");
    addAndMakeVisible(qualitySelector);

    outputSelector.addItemList(makeOutputChoices(), 1);
    outputSelector.setTooltip("Per-instrument output bus");
    outputSelector.setComponentID("outputSelector");
    addAndMakeVisible(outputSelector);

    fxLockButton.setButtonText("FX LOCK");
    fxLockButton.setClickingTogglesState(true);
    fxLockButton.setTooltip("Keep the current FX rack when switching model or factory preset");
    fxLockButton.setComponentID("fxLockButton");
    fxLockAtt = std::make_unique<BtnAttach>(proc.getAPVTS(), "fx_lock", fxLockButton);
    addAndMakeVisible(fxLockButton);

    for (int i = 0; i < kMacroTotal; ++i)
    {
        const auto si = static_cast<std::size_t>(i);
        macroAtt[si] = std::make_unique<SliderAttach>(proc.getAPVTS(), kMacroCtrls[si].paramId, macroDials[si]);
        setupDial(macroDials[si], accent_);
        addAndMakeVisible(macroDials[si]);
        macroLbls[si].setText(kMacroCtrls[si].label, juce::dontSendNotification);
        macroLbls[si].setJustificationType(juce::Justification::centred);
        macroLbls[si].setFont(juce::Font(juce::FontOptions{}.withHeight(10.4f)));
        macroLbls[si].setColour(juce::Label::textColourId, synthcol::textSec.withAlpha(0.84f));
        addAndMakeVisible(macroLbls[si]);
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
        const auto si = static_cast<std::size_t>(i);
        fxAtt[si] = std::make_unique<SliderAttach>(proc.getAPVTS(), kFxCtrls[si].paramId, fxDials[si]);
        setupSmallDial(fxDials[si], accent_);
        addChildComponent(fxDials[si]);
        fxLbls[si].setText(kFxCtrls[si].label, juce::dontSendNotification);
        fxLbls[si].setJustificationType(juce::Justification::centred);
        fxLbls[si].setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
        fxLbls[si].setColour(juce::Label::textColourId, synthcol::textSec.withAlpha(0.84f));
        addChildComponent(fxLbls[si]);
    }

    for (int t = 0; t < kFxTabs; ++t)
    {
        auto& item = fxRackItems[static_cast<std::size_t>(t)];
        item.configure(t, kFxTabNames[t], kFxRackSummaries[t], accent_);
        item.onClicked = [this](int ti) { switchEffectTab(ti); };
        addAndMakeVisible(item);
    }

    for (int t = 0; t < kFxTabs; ++t)
    {
        auto& btn = fxBypassBtns[static_cast<std::size_t>(t)];
        btn.setButtonText("ON");
        btn.setClickingTogglesState(true);
        btn.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(btn);
        fxBypassAtts[static_cast<std::size_t>(t)] = std::make_unique<BtnAttach>(
            proc.getAPVTS(), kFxBypassParamIds[t], btn);
        btn.onClick = [this] { syncFxRackState(); };
    }

    fxDetailTitle.setJustificationType(juce::Justification::centredLeft);
    fxDetailTitle.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    fxDetailTitle.setColour(juce::Label::textColourId, synthcol::textSec);
    addAndMakeVisible(fxDetailTitle);

    delaySyncLabel.setText("SYNC", juce::dontSendNotification);
    delayDivisionLabel.setText("DIV", juce::dontSendNotification);
    delaySyncSelector.addItem("OFF", 1);
    delaySyncSelector.addItem("HOST", 2);
    delayDivisionSelector.addItem("1/4", 1);
    delayDivisionSelector.addItem("1/8", 2);
    delayDivisionSelector.addItem("1/8D", 3);
    delayDivisionSelector.addItem("1/8T", 4);
    delayDivisionSelector.addItem("1/16", 5);
    delayDivisionSelector.addItem("1/16D", 6);
    delaySyncAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "delay_sync", delaySyncSelector);
    delayDivisionAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "delay_division", delayDivisionSelector);
    addChildComponent(delaySyncLabel);
    addChildComponent(delayDivisionLabel);
    addChildComponent(delaySyncSelector);
    addChildComponent(delayDivisionSelector);

    // --- Reverb type selector ---
    reverbTypeLabel.setText("Type", juce::dontSendNotification);
    reverbTypeSelector.addItem("Plate",   1);
    reverbTypeSelector.addItem("Hall",    2);
    reverbTypeSelector.addItem("Room",    3);
    reverbTypeSelector.addItem("Chamber", 4);
    reverbTypeSelector.setTooltip("Reverb algorithm type");
    reverbTypeAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "reverb_type", reverbTypeSelector);
    addChildComponent(reverbTypeLabel);
    addChildComponent(reverbTypeSelector);

    // --- Tooltip mode button ---
    tooltipModeBtn.setButtonText("TIP: SHORT");
    tooltipModeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2A2A32));
    tooltipModeBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffBBBBCC));
    tooltipModeBtn.onClick = [this] { cycleTooltipMode(); };
    tooltipModeBtn.setTooltip("Cycle tooltip detail level");
    addAndMakeVisible(tooltipModeBtn);
    applyTooltips();

    // --- MIDI CC page indicator (FLkey Mini) ---
    midiCCPageLabel.setText("CC: ---", juce::dontSendNotification);
    midiCCPageLabel.setJustificationType(juce::Justification::centredLeft);
    midiCCPageLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(12.0f).withStyle("Bold")));
    midiCCPageLabel.setColour(juce::Label::textColourId, juce::Colour(0xffBBBBCC));
    midiCCPageLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    midiCCPageLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    midiCCPageLabel.setTooltip("Current MIDI CC page");
    addAndMakeVisible(midiCCPageLabel);

    modLfo2RateLabel.setText("LFO2 RATE", juce::dontSendNotification);
    modLfo2WaveLabel.setText("LFO2 WAVE", juce::dontSendNotification);
    setupSmallDial(modLfo2RateDial, accent_);
    modLfo2WaveSelector.addItem("SINE", 1);
    modLfo2WaveSelector.addItem("TRI", 2);
    modLfo2WaveSelector.addItem("SAW", 3);
    modLfo2WaveSelector.addItem("SQR", 4);
    addChildComponent(modLfo2RateLabel);
    addChildComponent(modLfo2WaveLabel);
    addChildComponent(modLfo2RateDial);
    addChildComponent(modLfo2WaveSelector);

    for (int slotIndex = 0; slotIndex < kModSlots; ++slotIndex)
    {
        auto& label = modSlotLabels[static_cast<std::size_t>(slotIndex)];
        auto& src = modSourceBoxes[static_cast<std::size_t>(slotIndex)];
        auto& dst = modDestBoxes[static_cast<std::size_t>(slotIndex)];
        auto& amt = modAmountSliders[static_cast<std::size_t>(slotIndex)];

        label.setText("S" + juce::String(slotIndex + 1), juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        addChildComponent(label);

        for (int sourceIndex = 0; sourceIndex < modmatrix::kSourceCount; ++sourceIndex)
            src.addItem(modmatrix::getSourceName(static_cast<modmatrix::Source>(sourceIndex)), sourceIndex + 1);
        for (int destIndex = 0; destIndex < modmatrix::kDestCount; ++destIndex)
            dst.addItem(modmatrix::getDestinationName(static_cast<modmatrix::Destination>(destIndex)), destIndex + 1);

        src.onChange = [this, slotIndex, &src, &dst, &amt]
        {
            proc.setModMatrixSlot(slotIndex,
                                  static_cast<modmatrix::Source>(juce::jmax(0, src.getSelectedId() - 1)),
                                  static_cast<modmatrix::Destination>(juce::jmax(0, dst.getSelectedId() - 1)),
                                  static_cast<float>(amt.getValue()));
        };
        dst.onChange = [this, slotIndex, &src, &dst, &amt]
        {
            proc.setModMatrixSlot(slotIndex,
                                  static_cast<modmatrix::Source>(juce::jmax(0, src.getSelectedId() - 1)),
                                  static_cast<modmatrix::Destination>(juce::jmax(0, dst.getSelectedId() - 1)),
                                  static_cast<float>(amt.getValue()));
        };

        setupSmallDial(amt, accent_);
        amt.setRange(-1.0, 1.0, 0.01);
        amt.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        amt.setDoubleClickReturnValue(true, 0.0);
        amt.setTooltip("Mod amount");
        amt.onValueChange = [this, slotIndex, &src, &dst, &amt]
        {
            proc.setModMatrixSlot(slotIndex,
                                  static_cast<modmatrix::Source>(juce::jmax(0, src.getSelectedId() - 1)),
                                  static_cast<modmatrix::Destination>(juce::jmax(0, dst.getSelectedId() - 1)),
                                  static_cast<float>(amt.getValue()));
            amt.setTooltip("Mod amount: " + formatSignedPercent(amt.getValue()));
        };

        addChildComponent(src);
        addChildComponent(dst);
        addChildComponent(amt);
    }

    modLfo2RateDial.setRange(0.05, 12.0, 0.01);
    modLfo2RateDial.onValueChange = [this]
    {
        proc.setModMatrixLfo2Rate(static_cast<float>(modLfo2RateDial.getValue()));
    };
    modLfo2WaveSelector.onChange = [this]
    {
        proc.setModMatrixLfo2Wave(juce::jmax(0, modLfo2WaveSelector.getSelectedId() - 1));
    };

    // --- Undo / Redo ---
    // charToString (not a raw "\u..." literal) builds the string from the codepoint directly,
    // so it isn't run through MSVC's narrow-literal execution-charset re-encoding (cp1252 can't
    // represent these glyphs and silently substitutes a wrong character \u2014 same pattern already
    // used below for octaveDownBtn/octaveUpBtn).
    undoButton.setButtonText(juce::String::charToString(0x21B6));
    undoButton.setTooltip("Undo (Ctrl+Z)");
    undoButton.onClick = [this] { proc.getUndoManager().undo(); };
    addAndMakeVisible(undoButton);
    redoButton.setButtonText(juce::String::charToString(0x21B7));
    redoButton.setTooltip("Redo (Ctrl+Shift+Z)");
    redoButton.onClick = [this] { proc.getUndoManager().redo(); };
    addAndMakeVisible(redoButton);

    // --- MIDI Learn ---
    midiLearnButton.setButtonText("MIDI LEARN");
    midiLearnButton.setTooltip("Show/hide MIDI CC learn panel");
    midiLearnButton.onClick = [this] { showMidiLearnPanel(!midiLearnPanelVisible); };
    addAndMakeVisible(midiLearnButton);
    midiLearnPanel.onClearMapping = [this](int cc) { proc.midiLearnClear(cc); refreshMidiLearnPanel(); };
    midiLearnPanel.onClearAll = [this] { proc.midiLearnClearAll(); refreshMidiLearnPanel(); };
    addChildComponent(midiLearnPanel);

    // --- RAND button ---
    randButton.setButtonText("RAND");
    randButton.setTooltip("Randomize current preset (sound-design utility)");
    randButton.onClick = [this] { proc.randomizePreset(); };
    addAndMakeVisible(randButton);

    // --- Voice count ---
    voiceCountLabel.setText("0 V", juce::dontSendNotification);
    voiceCountLabel.setJustificationType(juce::Justification::centredRight);
    voiceCountLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(12.0f).withStyle("Bold")));
    voiceCountLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    voiceCountLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(voiceCountLabel);

    // --- Arpeggiator ---
    arpEnableButton.setButtonText("ARP");
    arpEnableButton.setTooltip("Enable arpeggiator");
    arpEnableButton.setClickingTogglesState(true);
    addAndMakeVisible(arpEnableButton);
    arpEnableAtt = std::make_unique<BtnAttach>(proc.getAPVTS(), "arp_enable", arpEnableButton);

    arpModeSelector.addItem("Up",     1);
    arpModeSelector.addItem("Down",   2);
    arpModeSelector.addItem("UpDown", 3);
    arpModeSelector.addItem("Random", 4);
    arpModeSelector.setTooltip("Arpeggio direction");
    addAndMakeVisible(arpModeSelector);
    arpModeAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "arp_mode", arpModeSelector);

    arpRateSelector.addItem("1/4",   1);
    arpRateSelector.addItem("1/8",   2);
    arpRateSelector.addItem("1/8T",  3);
    arpRateSelector.addItem("1/16",  4);
    arpRateSelector.addItem("1/16T", 5);
    arpRateSelector.setTooltip("Arpeggio rate");
    addAndMakeVisible(arpRateSelector);
    arpRateAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "arp_rate", arpRateSelector);

    arpOctavesDial.setRange(1.0, 4.0, 1.0);
    arpOctavesDial.setSliderStyle(juce::Slider::LinearHorizontal);
    arpOctavesDial.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 20, 14);
    arpOctavesDial.setTooltip("Arpeggio range in octaves");
    addAndMakeVisible(arpOctavesDial);
    arpOctavesAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "arp_octaves", arpOctavesDial);

    arpGateDial.setRange(0.1, 1.0, 0.01);
    arpGateDial.setSliderStyle(juce::Slider::LinearHorizontal);
    arpGateDial.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    arpGateDial.setTooltip("Arpeggio gate (note length)");
    addAndMakeVisible(arpGateDial);
    arpGateAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "arp_gate", arpGateDial);

    arpHoldButton.setButtonText("HLD");
    arpHoldButton.setTooltip("Hold: keep playing when keys are released");
    arpHoldButton.setClickingTogglesState(true);
    addAndMakeVisible(arpHoldButton);
    arpHoldAtt = std::make_unique<BtnAttach>(proc.getAPVTS(), "arp_hold", arpHoldButton);

    arpModeLabel.setText("Mode", juce::dontSendNotification);
    arpRateLabel.setText("Rate", juce::dontSendNotification);
    arpOctavesLabel.setText("Oct", juce::dontSendNotification);
    arpGateLabel.setText("Gate", juce::dontSendNotification);

    // Initially hidden until arp is enabled
    arpModeSelector.setVisible(false);
    arpRateSelector.setVisible(false);
    arpOctavesDial.setVisible(false);
    arpGateDial.setVisible(false);
    arpHoldButton.setVisible(false);

    // --- Riff Generator (Phase 4) ---
    riffStyleSelector.addItem("Off",    1);
    riffStyleSelector.addItem("Rock",   2);
    riffStyleSelector.addItem("Blues",  3);
    riffStyleSelector.addItem("Metal",  4);
    riffStyleSelector.addItem("Folk",   5);
    riffStyleSelector.addItem("Jazz",   6);
    riffStyleSelector.addItem("Funk",   7);
    riffStyleSelector.addItem("Ambient", 8);
    riffStyleSelector.setTooltip("Riff generation style");
    addAndMakeVisible(riffStyleSelector);
    riffStyleAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "riff_style", riffStyleSelector);

    riffIntensityDial.setRange(0, 7, 1);
    riffIntensityDial.setSliderStyle(juce::Slider::LinearHorizontal);
    riffIntensityDial.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 20, 14);
    riffIntensityDial.setTooltip("Riff intensity (dynamics + articulation variation)");
    addAndMakeVisible(riffIntensityDial);
    riffIntensityAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "riff_intensity", riffIntensityDial);

    riffGateDial.setRange(0.3, 1.0, 0.01);
    riffGateDial.setSliderStyle(juce::Slider::LinearHorizontal);
    riffGateDial.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    riffGateDial.setTooltip("Riff gate (note length)");
    addAndMakeVisible(riffGateDial);
    riffGateAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "riff_gate", riffGateDial);

    riffHoldButton.setButtonText("HLD");
    riffHoldButton.setTooltip("Hold: keep playing when keys are released");
    riffHoldButton.setClickingTogglesState(true);
    addAndMakeVisible(riffHoldButton);
    riffHoldAtt = std::make_unique<BtnAttach>(proc.getAPVTS(), "riff_hold", riffHoldButton);

    riffStyleLabel.setText("Style", juce::dontSendNotification);
    riffIntensityLabel.setText("Int", juce::dontSendNotification);
    riffGateLabel.setText("Gate", juce::dontSendNotification);

    // Riff controls visible only when style != Off
    riffStyleSelector.setVisible(true);
    riffIntensityDial.setVisible(true);
    riffGateDial.setVisible(true);
    riffHoldButton.setVisible(true);

    // --- Velocity Curve ---
    velocityCurveLabel.setText("Vel Curve", juce::dontSendNotification);
    velocityCurveLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(velocityCurveLabel);
    velocityCurveSelector.addItem("Linear",  1);
    velocityCurveSelector.addItem("Soft",    2);
    velocityCurveSelector.addItem("Softer",  3);
    velocityCurveSelector.addItem("Hard",    4);
    velocityCurveSelector.addItem("Harder",  5);
    velocityCurveSelector.addItem("Fixed",   6);
    velocityCurveSelector.addItem("Touch",   7);
    velocityCurveSelector.setTooltip("Velocity response curve");
    addAndMakeVisible(velocityCurveSelector);
    velocityCurveSelector.setComponentID("velocityCurveSelector");
    velocityCurveAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "velocity_curve", velocityCurveSelector);

    // --- Play Mode ---
    playModeLabel.setText("Play", juce::dontSendNotification);
    playModeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(playModeLabel);
    playModeSelector.addItem("Poly", 1);
    playModeSelector.addItem("Mono Retrig", 2);
    playModeSelector.addItem("Mono Legato", 3);
    playModeSelector.setTooltip("Keyboard play mode");
    playModeSelector.setComponentID("playModeSelector");
    addAndMakeVisible(playModeSelector);
    playModeAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "play_mode", playModeSelector);

    // --- Palm Mute ---
    palmMuteLabel.setText("Mute", juce::dontSendNotification);
    palmMuteLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(palmMuteLabel);
    palmMuteDial.setRange(0.0, 1.0, 0.0001);
    palmMuteDial.setSliderStyle(juce::Slider::LinearHorizontal);
    palmMuteDial.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 34, 14);
    palmMuteDial.setTooltip("Palm mute amount");
    palmMuteDial.setComponentID("palmMuteDial");
    addAndMakeVisible(palmMuteDial);
    palmMuteAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "palm_mute", palmMuteDial);

    // --- Pitch Bend Range ---
    pitchBendRangeLabel.setText("PB", juce::dontSendNotification);
    pitchBendRangeLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(pitchBendRangeLabel);
    pitchBendRangeDial.setRange(1.0, 24.0, 1.0);
    pitchBendRangeDial.setSliderStyle(juce::Slider::LinearHorizontal);
    pitchBendRangeDial.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 20, 14);
    pitchBendRangeDial.setTooltip("Pitch bend range in semitones");
    pitchBendRangeDial.setComponentID("pitchBendRangeDial");
    addAndMakeVisible(pitchBendRangeDial);
    pitchBendRangeAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "pitch_bend_range", pitchBendRangeDial);

    // --- Preset filters ---
    presetTierFilter.addItem("All Lanes", 1);
    presetTierFilter.addItem("Core", 2);
    presetTierFilter.addItem("Texture", 3);
    presetTierFilter.setSelectedId(2, juce::dontSendNotification);
    presetTierFilter.onChange = [this] { refreshPresetList(); };
    presetTierFilter.setComponentID("presetTierFilter");
    addAndMakeVisible(presetTierFilter);

    presetUseFilter.addItem("All Uses", 1);
    presetUseFilter.addItem("Writer", 2);
    presetUseFilter.addItem("Dry", 3);
    presetUseFilter.addItem("Muted", 4);
    presetUseFilter.addItem("Showcase", 5);
    presetUseFilter.setSelectedId(1, juce::dontSendNotification);
    presetUseFilter.onChange = [this] { refreshPresetList(); };
    presetUseFilter.setComponentID("presetUseFilter");
    addAndMakeVisible(presetUseFilter);

    presetRoleFilter.addItem("All Roles", 1);
    presetRoleFilter.addItem("Showcase", 2);
    presetRoleFilter.addItem("lead", 3);
    presetRoleFilter.addItem("rhythm", 4);
    presetRoleFilter.addItem("picked", 5);
    presetRoleFilter.addItem("texture", 6);
    presetRoleFilter.setSelectedId(1, juce::dontSendNotification);
    presetRoleFilter.onChange = [this] { refreshPresetList(); };
    presetRoleFilter.setComponentID("presetRoleFilter");
    addAndMakeVisible(presetRoleFilter);

    setWantsKeyboardFocus(true);

    rebuildInstrAttachments();
    syncSelectionUiFromInstr();
    syncFxAvailability();
    syncAdvancedModUi();
    switchEffectTab(0);

    backgroundImage_ = juce::ImageCache::getFromMemory(
        BinaryData::fond_guitar_png, BinaryData::fond_guitar_pngSize);

    applyInstrumentTheme(selectedInstrFromParam());
    initCommon();

    startTimerHz(12);
    setResizable(true, true);
    setResizeLimits(960, 700, 2560, 1600);
    presetSearch.setTooltip("Search presets");
    presetBox.setComponentID("presetBox");
    presetBox.setTooltip("Current preset");
    prevPresetBtn.setTooltip("Previous preset");
    nextPresetBtn.setTooltip("Next preset");
    savePresetBtn.setTooltip("Save the current preset");
    saveAsPresetBtn.setTooltip("Save the current preset as a new file");
    deletePresetBtn.setTooltip("Delete the selected user preset");
    importPresetsBtn.setTooltip("Import presets from disk");
    octaveDownBtn.setButtonText(juce::String::charToString(0x25C0));
    octaveUpBtn.setButtonText(juce::String::charToString(0x25B6));
    octaveDownBtn.setTooltip("Shift the keyboard down by one octave");
    octaveUpBtn.setTooltip("Shift the keyboard up by one octave");
    gainDial.setTooltip("Global output gain");

    applyValueFormatters();
    setSize(lay::W, lay::H);
}

void GuitarSynthAudioProcessorEditor::applyValueFormatters()
{
    const auto parsePercent01Text = [](const juce::String& text)
    {
        return juce::jlimit(0.0, 1.0, parseUiNumber(text) / 100.0);
    };

    const auto parseSignedPercentText = [](const juce::String& text)
    {
        return juce::jlimit(-1.0, 1.0, parseUiNumber(text) / 100.0);
    };

    const auto parseSecondsText = [](const juce::String& text)
    {
        const auto lowered = text.toLowerCase();
        const auto value = parseUiNumber(lowered);
        if (lowered.contains("ms"))
            return value / 1000.0;
        return value;
    };

    const auto parseMillisecondsText = [](const juce::String& text)
    {
        const auto lowered = text.toLowerCase();
        const auto value = parseUiNumber(lowered);
        if (lowered.containsChar('s') && !lowered.contains("ms"))
            return value * 1000.0;
        return value;
    };

    const auto parsePanText = [](const juce::String& text)
    {
        const auto lowered = text.toLowerCase();
        if (lowered.contains("center") || lowered == "c")
            return 0.0;

        const auto amount = juce::jlimit(0.0, 100.0, parseUiNumber(lowered));
        if (lowered.containsChar('l'))
            return -(amount / 100.0);
        if (lowered.containsChar('r'))
            return amount / 100.0;
        return juce::jlimit(-1.0, 1.0, parseUiNumber(lowered));
    };

    const auto setTextboxContrast = [this](juce::Slider& slider)
    {
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.96f));
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff11161C).withAlpha(0.92f));
        slider.setColour(juce::Slider::textBoxOutlineColourId, accent_.withAlpha(0.24f));
    };

    for (auto& slider : envDials) setTextboxContrast(slider);
    for (auto& slider : macroDials) setTextboxContrast(slider);
    for (auto& slider : fxDials) setTextboxContrast(slider);
    for (auto& slider : modAmountSliders) setTextboxContrast(slider);
    setTextboxContrast(lfoRateDial);
    setTextboxContrast(lfoDepthDial);
    setTextboxContrast(modLfo2RateDial);

    envDials[0].textFromValueFunction = [](double v) { return formatPercent01(v); };
    envDials[0].valueFromTextFunction = parsePercent01Text;
    envDials[1].textFromValueFunction = [](double v) { return formatSemitones(v); };
    envDials[1].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(-24.0, 24.0, parseUiNumber(text)); };
    envDials[2].textFromValueFunction = [](double v) { return formatPercent01(v); };
    envDials[2].valueFromTextFunction = parsePercent01Text;
    envDials[3].textFromValueFunction = [](double v) { return formatTimeSeconds(v); };
    envDials[3].valueFromTextFunction = parseSecondsText;
    envDials[4].textFromValueFunction = [](double v) { return formatTimeSeconds(v); };
    envDials[4].valueFromTextFunction = parseSecondsText;
    envDials[5].textFromValueFunction = [](double v) { return formatPercent01(v); };
    envDials[5].valueFromTextFunction = parsePercent01Text;
    envDials[6].textFromValueFunction = [](double v) { return formatTimeSeconds(v); };
    envDials[6].valueFromTextFunction = parseSecondsText;
    envDials[7].textFromValueFunction = [](double v) { return formatPercent01(v); };
    envDials[7].valueFromTextFunction = parsePercent01Text;
    envDials[8].textFromValueFunction = [](double v) { return formatPercent01(v); };
    envDials[8].valueFromTextFunction = parsePercent01Text;
    envDials[9].textFromValueFunction = [](double v) { return formatPercent01(v); };
    envDials[9].valueFromTextFunction = parsePercent01Text;
    envDials[10].textFromValueFunction = [](double v) { return formatPercent01(v); };
    envDials[10].valueFromTextFunction = parsePercent01Text;
    envDials[11].textFromValueFunction = [](double v) { return formatPercent01(v); };
    envDials[11].valueFromTextFunction = parsePercent01Text;
    envDials[12].textFromValueFunction = [](double v) { return formatFrequency(v); };
    envDials[12].valueFromTextFunction = parseFrequencyText;
    envDials[13].textFromValueFunction = [](double v) { return formatPan(v); };
    envDials[13].valueFromTextFunction = parsePanText;

    lfoRateDial.textFromValueFunction = [](double v) { return trimValueString(v, v < 10.0 ? 2 : 1) + " Hz"; };
    lfoRateDial.valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(0.05, 12.0, parseUiNumber(text)); };
    lfoDepthDial.textFromValueFunction = [](double v) { return formatPercent01(v); };
    lfoDepthDial.valueFromTextFunction = parsePercent01Text;

    for (auto& slider : macroDials)
    {
        slider.textFromValueFunction = [](double v) { return formatPercent01(v); };
        slider.valueFromTextFunction = parsePercent01Text;
    }

    fxDials[0].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[0].valueFromTextFunction = parsePercent01Text;
    fxDials[1].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[1].valueFromTextFunction = parsePercent01Text;
    fxDials[2].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[2].valueFromTextFunction = parsePercent01Text;
    fxDials[3].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[3].valueFromTextFunction = parsePercent01Text;
    fxDials[4].textFromValueFunction = [](double v) { return formatMultiplier(v); };
    fxDials[4].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(1.0, 16.0, parseUiNumber(text)); };
    fxDials[5].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[5].valueFromTextFunction = parsePercent01Text;
    fxDials[6].textFromValueFunction = [](double v) { return formatSignedPercent(v); };
    fxDials[6].valueFromTextFunction = parseSignedPercentText;
    fxDials[7].textFromValueFunction = [](double v) { return formatSignedPercent(v); };
    fxDials[7].valueFromTextFunction = parseSignedPercentText;
    fxDials[8].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[8].valueFromTextFunction = parsePercent01Text;
    fxDials[9].textFromValueFunction = [](double v) { return formatDecibels(v); };
    fxDials[9].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(-60.0, 0.0, parseUiNumber(text)); };
    fxDials[10].textFromValueFunction = [](double v) { return formatRatio(v); };
    fxDials[10].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(1.0, 20.0, parseUiNumber(text)); };
    fxDials[11].textFromValueFunction = [](double v) { return formatMilliseconds(v); };
    fxDials[11].valueFromTextFunction = parseMillisecondsText;
    fxDials[12].textFromValueFunction = [](double v) { return formatMilliseconds(v); };
    fxDials[12].valueFromTextFunction = parseMillisecondsText;
    fxDials[13].textFromValueFunction = [](double v) { return formatDecibels(v); };
    fxDials[13].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(0.0, 24.0, parseUiNumber(text)); };
    fxDials[14].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[14].valueFromTextFunction = parsePercent01Text;
    fxDials[15].textFromValueFunction = [](double v) { return formatFrequency(v); };
    fxDials[15].valueFromTextFunction = parseFrequencyText;
    fxDials[16].textFromValueFunction = [](double v) { return formatDecibels(v); };
    fxDials[16].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(-12.0, 12.0, parseUiNumber(text)); };
    fxDials[17].textFromValueFunction = [](double v) { return formatFrequency(v); };
    fxDials[17].valueFromTextFunction = parseFrequencyText;
    fxDials[18].textFromValueFunction = [](double v) { return formatDecibels(v); };
    fxDials[18].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(-12.0, 12.0, parseUiNumber(text)); };
    fxDials[19].textFromValueFunction = [](double v) { return "Q " + trimValueString(v, v < 10.0 ? 2 : 1); };
    fxDials[19].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(0.1, 10.0, parseUiNumber(text)); };
    fxDials[20].textFromValueFunction = [](double v) { return formatFrequency(v); };
    fxDials[20].valueFromTextFunction = parseFrequencyText;
    fxDials[21].textFromValueFunction = [](double v) { return formatDecibels(v); };
    fxDials[21].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(-12.0, 12.0, parseUiNumber(text)); };
    fxDials[22].textFromValueFunction = [](double v) { return trimValueString(v, v < 10.0 ? 2 : 1) + " Hz"; };
    fxDials[22].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(0.1, 8.0, parseUiNumber(text)); };
    fxDials[23].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[23].valueFromTextFunction = parsePercent01Text;
    fxDials[24].textFromValueFunction = [](double v) { return formatMilliseconds(v); };
    fxDials[24].valueFromTextFunction = parseMillisecondsText;
    fxDials[25].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[25].valueFromTextFunction = parsePercent01Text;
    fxDials[26].textFromValueFunction = [](double v) { return formatMilliseconds(v); };
    fxDials[26].valueFromTextFunction = parseMillisecondsText;
    fxDials[27].textFromValueFunction = [](double v) { return juce::String(juce::roundToInt(v * 100.0)) + "%"; };
    fxDials[27].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(0.0, 0.95, parseUiNumber(text) / 100.0); };
    fxDials[28].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[28].valueFromTextFunction = parsePercent01Text;
    fxDials[29].textFromValueFunction = [](double v) { return formatDecibels(v); };
    fxDials[29].valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(-12.0, 0.0, parseUiNumber(text)); };
    fxDials[30].textFromValueFunction = [](double v) { return formatMilliseconds(v); };
    fxDials[30].valueFromTextFunction = parseMillisecondsText;
    fxDials[31].textFromValueFunction = [](double v) { return formatPercent01(v); };
    fxDials[31].valueFromTextFunction = parsePercent01Text;

    modLfo2RateDial.textFromValueFunction = [](double v) { return trimValueString(v, v < 10.0 ? 2 : 1) + " Hz"; };
    modLfo2RateDial.valueFromTextFunction = [](const juce::String& text) { return juce::jlimit(0.05, 12.0, parseUiNumber(text)); };

    palmMuteDial.textFromValueFunction = [](double v) { return formatPercent01(v); };
    palmMuteDial.valueFromTextFunction = parsePercent01Text;
    pitchBendRangeDial.textFromValueFunction = [](double v) { return formatSemitones(v); };
    pitchBendRangeDial.valueFromTextFunction = [](const juce::String& text)
    {
        return juce::jlimit(1.0, 24.0, parseUiNumber(text));
    };

    for (auto& slider : modAmountSliders)
    {
        slider.textFromValueFunction = [](double v) { return formatSignedPercent(v); };
        slider.valueFromTextFunction = parseSignedPercentText;
    }

    const auto refreshTextBox = [](juce::Slider& slider)
    {
        slider.updateText();
        slider.repaint();
    };
    for (auto& slider : envDials) refreshTextBox(slider);
    for (auto& slider : macroDials) refreshTextBox(slider);
    for (auto& slider : fxDials) refreshTextBox(slider);
    for (auto& slider : modAmountSliders) refreshTextBox(slider);
    refreshTextBox(lfoRateDial);
    refreshTextBox(lfoDepthDial);
    refreshTextBox(modLfo2RateDial);
    refreshTextBox(palmMuteDial);
    refreshTextBox(pitchBendRangeDial);
}




void GuitarSynthAudioProcessorEditor::timerCallback()
{
    rebuildInstrAttachments();
    syncPresetBox();
    if (activeRightPanelSection == 1)
        syncAdvancedModUi();

    const int page = proc.getMidiCCPage();
    if (page != cachedMidiCCPage)
    {
        cachedMidiCCPage = page;
        updateMidiCCPageLabel();
        repaint(midiCCPageLabel.getBounds());
    }

    // Undo/redo button states
    const bool canUndo = proc.getUndoManager().canUndo();
    const bool canRedo = proc.getUndoManager().canRedo();
    if (undoButton.isEnabled() != canUndo) undoButton.setEnabled(canUndo);
    if (redoButton.isEnabled() != canRedo) redoButton.setEnabled(canRedo);

    // Arpeggiator detail controls visibility
    {
        const auto* arpEnRaw = proc.getAPVTS().getRawParameterValue("arp_enable");
        const bool arpOn = arpEnRaw && *arpEnRaw >= 0.5f;
        arpModeSelector.setVisible(arpOn);
        arpRateSelector.setVisible(arpOn);
        arpOctavesDial.setVisible(arpOn);
        arpGateDial.setVisible(arpOn);
        arpHoldButton.setVisible(arpOn);
    }

    // Voice count display
    const int vc = proc.getActiveVoiceCount();
    if (vc != cachedVoiceCount.load(std::memory_order_relaxed))
    {
        cachedVoiceCount.store(vc, std::memory_order_relaxed);
        voiceCountLabel.setText(juce::String(vc) + " V", juce::dontSendNotification);
    }
}

void GuitarSynthAudioProcessorEditor::updateMidiCCPageLabel()
{
    const int page = juce::jlimit(0, GuitarSynthAudioProcessor::kNumCCPages - 1, proc.getMidiCCPage());
    const auto fullText = juce::String("CC: ") + GuitarSynthAudioProcessor::getCCPageName(page);
    const auto compactText = juce::String("CC ") + juce::String(page + 1)
        + "/" + juce::String(GuitarSynthAudioProcessor::kNumCCPages);

    const bool useCompactText = midiCCPageLabel.getWidth() > 0 && midiCCPageLabel.getWidth() < 70;
    midiCCPageLabel.setText(useCompactText ? compactText : fullText, juce::dontSendNotification);
    midiCCPageLabel.setTooltip("Current MIDI CC page: " + fullText);
}

bool GuitarSynthAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
    {
        proc.getUndoManager().undo();
        return true;
    }
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier
                                   | juce::ModifierKeys::shiftModifier, 0))
    {
        proc.getUndoManager().redo();
        return true;
    }
    return false;
}

bool GuitarSynthAudioProcessorEditor::hostShouldIncludeFactoryPreset(int presetIndex) const
{
    const int instrIdx = proc.getSelectedInstrIndex();
    const auto& banks = mgs::getFactoryPresetBanks();
    if (instrIdx < 0 || instrIdx >= mgs::kNumInstruments) return true;
    const auto& bank = banks[static_cast<std::size_t>(instrIdx)];
    if (presetIndex < 0 || presetIndex >= static_cast<int>(bank.size())) return true;

    const auto& meta = bank[static_cast<std::size_t>(presetIndex)].metadata;

    const int tierId = presetTierFilter.getSelectedId();
    if (tierId == 2 && isTexturePresetMetadata(meta))
        return false;
    if (tierId == 3 && !isTexturePresetMetadata(meta))
        return false;

    const int useId = presetUseFilter.getSelectedId();
    if (useId == 2 && !hasPresetTag(meta, "writer"))
        return false;
    if (useId == 3 && !hasPresetTag(meta, "dry"))
        return false;
    if (useId == 4 && !hasPresetTag(meta, "muted"))
        return false;
    if (useId == 5 && !hasPresetTag(meta, "showcase"))
        return false;

    const int roleId = presetRoleFilter.getSelectedId();
    if (roleId > 1)
    {
        // combo IDs: 2=showcase, 3=lead, 4=rhythm, 5=pluck, 6=texture
        static const char* kRoleValues[] = { "", "texture", "lead", "rhythm", "pluck", "texture" };
        if (roleId == 2)
        {
            if (!hasPresetTag(meta, "showcase"))
                return false;
        }
        else if (meta.mixRole != kRoleValues[roleId - 1])
            return false;
    }

    return true;
}

void GuitarSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    paintBackground(g);

    const auto layout = computeLayoutMetrics(getWidth(), getHeight());
    const auto headerZones = computeHeaderZones(layout.headerH);
    const auto headerRect = juce::Rectangle<float>(static_cast<float>(layout.contentX),
                                                   static_cast<float>(layout.outerMargin),
                                                   static_cast<float>(layout.contentW),
                                                   static_cast<float>(layout.headerH));
    const auto selectorRect = juce::Rectangle<float>(static_cast<float>(layout.contentX),
                                                     static_cast<float>(layout.outerMargin + layout.headerH + 8),
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
    const int gainSize = layout.compact ? 40 : 46;
    const int statusRightTrim = gainSize + 10 + (layout.compact ? 12 : 14);
    const auto statusPrimaryRow = headerZones.statusPrimaryRow.withTrimmedRight(statusRightTrim);
    const auto chromeAccent = activePalette.panelHeader.isTransparent()
        ? accent_
        : activePalette.panelHeader.interpolatedWith(accent_, 0.42f);

    paintHeader(g, layout.headerH);
    glazeGuitarChrome(g, headerRect.reduced(1.5f, 1.5f), chromeAccent, 13.0f, 0.86f);

    g.setColour(activePalette.panelHeader.withAlpha(0.07f));
    g.fillRoundedRectangle(statusPrimaryRow.toFloat().withWidth(
                               juce::jmin(120.0f, static_cast<float>(statusPrimaryRow.getWidth()) * 0.34f)),
                           7.0f);

    const auto gainAnchor = gainDial.getBounds().toFloat().expanded(8.0f, 7.0f).translated(0.0f, -1.0f);
    g.setColour(juce::Colours::black.withAlpha(0.14f));
    g.fillRoundedRectangle(gainAnchor.translated(0.0f, 2.0f), 9.0f);
    g.setColour(activePalette.panelHeader.withAlpha(0.08f));
    g.fillRoundedRectangle(gainAnchor, 9.0f);
    g.setColour(chromeAccent.withAlpha(0.22f));
    g.drawRoundedRectangle(gainAnchor.reduced(0.5f), 9.0f, 0.9f);

    paintCard(g, layout.contentX, layout.outerMargin + layout.headerH + 8,
              layout.contentW, layout.selectorH - 8, "Instrument / Model");
    paintCard(g, layout.col1X, layout.bodyY, layout.colW, layout.bodyH, "Source & Envelope");
    paintCard(g, layout.col2X, layout.bodyY, layout.colW, layout.bodyH, "Timbre & Shape");
    paintCard(g, layout.col3X, layout.bodyY, layout.colW, layout.bodyH, "Macros / Mod / FX");
    glazeGuitarChrome(g, selectorRect.reduced(2.0f, 2.0f), chromeAccent, 10.0f, 0.62f);
    glazeGuitarChrome(g, col1Rect.reduced(2.0f, 2.0f), chromeAccent, 10.0f, 0.52f);
    glazeGuitarChrome(g, col2Rect.reduced(2.0f, 2.0f), chromeAccent, 10.0f, 0.52f);
    glazeGuitarChrome(g, col3Rect.reduced(2.0f, 2.0f), chromeAccent, 10.0f, 0.52f);

    if (envDials[12].isVisible())
    {
        auto toneWell = envDials[12].getBounds().toFloat().expanded(layout.compact ? 16.0f : 20.0f,
                                                                    layout.compact ? 14.0f : 18.0f);
        toneWell.setY(static_cast<float>(envLabels[12].getY()) - 6.0f);
        toneWell.setHeight(toneWell.getHeight() + 8.0f);
        paintControlWell(g, toneWell, activePalette.panelCavity, chromeAccent, 14.0f);
    }

    // L/R meter — relocated to statusPrimaryRow right edge (refonte 3-row header).
    // Outputs occupy ~200 px at the row left, meter sits in the remaining space before the gain reserve.
    const int meterBarW = 24;
    const int meterBarGap = 4;
    const int meterTotalW = meterBarW * 2 + meterBarGap;
    const int meterRight = statusPrimaryRow.getRight() - 4;
    const int meterLeft = meterRight - meterTotalW;
    const int meterY = statusPrimaryRow.getY() + juce::jmax(0, (statusPrimaryRow.getHeight() - 8) / 2);
    if (meterLeft >= statusPrimaryRow.getX() + 210)
    {
        drawMeterBar(g, { meterLeft, meterY, meterBarW, 8 }, proc.getMainMeterLevel(0), accent_);
        drawMeterBar(g, { meterLeft + meterBarW + meterBarGap, meterY, meterBarW, 8 },
                     proc.getMainMeterLevel(1), accent_.brighter(0.22f));
    }

    g.setColour(accent_.withAlpha(0.12f));
    g.drawLine(static_cast<float>(layout.contentX + 18), static_cast<float>(layout.kbY - 8),
               static_cast<float>(layout.contentX + layout.contentW - 18), static_cast<float>(layout.kbY - 8),
               1.0f);

    paintKeyboardDock(g, layout.contentX, layout.kbY, layout.contentW, layout.kbH);
    glazeGuitarChrome(g, keyboardRect.reduced(2.0f, 2.0f), chromeAccent, 11.0f, 0.78f);
    paintKeyboardMarkers(g, keyboardRect, accent_);
}

void GuitarSynthAudioProcessorEditor::resized()
{
    const auto layout = computeLayoutMetrics(getWidth(), getHeight());
    const float gapDensity = layoutDensity(layout.compact, layout.roomy);
    // ============================================================================
    // 3-row header layout (refonte audit) :
    //   Preset zone (left)            | Status zone (right)
    //   R1 : search + nav + filters   | R1 : quality + output
    //   R2 : preset actions           | R2 : fxLock + tooltip + midiCC + voiceCount
    //   R3 : performance controls     | R3 : ARP (en/mode/rate/oct/gate/hold) + rand
    //   Gain dial : centred vertically along the right edge of the status zone
    // ============================================================================
    const auto headerZones = computeHeaderZones(layout.headerH);
    const int gainSize = layout.compact ? 40 : 46;
    const int gainMargin = 10;
    const int statusRightTrim = gainSize + gainMargin + (layout.compact ? 12 : 14);
    const int statusVertOffset = layout.compact ? 0 : 1;

    // Gain (right edge of status zone, full-height vertical centre)
    const int gainX = headerZones.statusZone.getRight() - gainSize - gainMargin;
    const int gainY = headerZones.statusZone.getY()
                    + juce::jmax(0, (headerZones.statusZone.getHeight() - gainSize) / 2)
                    - statusVertOffset;
    gainDial.setBounds(gainX, gainY, gainSize, gainSize);

    // ---- Preset zone, R1 : search + prev + presetBox + next + 3 filtres ------
    const int rowH1 = layout.compact ? 26 : 28;
    const auto presetR1 = headerZones.presetPrimaryRow.reduced(0, 1);
    const int r1Y = presetR1.getY() + juce::jmax(0, (presetR1.getHeight() - rowH1) / 2);
    const int navW = layout.compact ? 22 : 24;
    const int searchW = layout.compact ? 52 : 110;
    const int searchGap = layout.compact ? 4 : 6;
    const int navGap = layout.compact ? 3 : 4;
    const int presetNextGap = layout.compact ? 3 : 4;
    const int filterGap = layout.compact ? 3 : 6;
    const int tierFilterW = layout.compact ? 64 : 68;
    const int useFilterW = layout.compact ? 70 : 76;
    const int roleFilterW = layout.compact ? 70 : 76;
    const int filterLeadGap = layout.compact ? 4 : 8;
    const int filterReserve = tierFilterW + useFilterW + roleFilterW + filterGap * 2 + filterLeadGap;
    int x = presetR1.getX();
    presetSearch.setBounds(x, r1Y, searchW, rowH1); x += searchW + searchGap;
    prevPresetBtn.setBounds(x, r1Y, navW, rowH1); x += navW + navGap;
    // presetBox fills the space between nextPresetBtn and the filter combos.
    const int presetBoxAvailableW = presetR1.getRight() - filterReserve - x - navW - presetNextGap;
    const int presetBoxMinW = layout.compact ? 44 : 72;
    const int presetBoxW = presetBoxAvailableW >= presetBoxMinW
        ? presetBoxAvailableW
        : juce::jmax(0, presetBoxAvailableW);
    presetBox.setBounds(x, r1Y, presetBoxW, rowH1); x += presetBoxW + presetNextGap;
    nextPresetBtn.setBounds(x, r1Y, navW, rowH1);
    int fx = presetR1.getRight() - roleFilterW;
    presetRoleFilter.setBounds(fx, r1Y, roleFilterW, rowH1); fx -= useFilterW + filterGap;
    presetUseFilter.setBounds(fx, r1Y, useFilterW, rowH1); fx -= tierFilterW + filterGap;
    presetTierFilter.setBounds(fx, r1Y, tierFilterW, rowH1);

    // ---- Preset zone, R2 : actions (undo, redo, save, saveAs, delete, import, midiLearn) ----
    const int rowH2 = layout.compact ? 22 : 24;
    const auto presetR2 = headerZones.presetSecondaryRow.reduced(0, 1);
    const int r2Y = presetR2.getY() + juce::jmax(0, (presetR2.getHeight() - rowH2) / 2);
    const int undoW = layout.compact ? 26 : 28;
    const int saveW = layout.compact ? 56 : 64;
    const int saveAsW = layout.compact ? 66 : 78;
    const int deleteW = layout.compact ? 62 : 70;
    const int importW = layout.compact ? 62 : 70;
    const int midiLearnW = layout.compact ? 80 : 90;
    const int actionGap = 6;
    int ax = presetR2.getX();
    undoButton.setBounds(ax, r2Y, undoW, rowH2); ax += undoW + 4;
    redoButton.setBounds(ax, r2Y, undoW, rowH2); ax += undoW + actionGap;
    savePresetBtn.setBounds(ax, r2Y, saveW, rowH2); ax += saveW + actionGap;
    saveAsPresetBtn.setBounds(ax, r2Y, saveAsW, rowH2); ax += saveAsW + actionGap;
    deletePresetBtn.setBounds(ax, r2Y, deleteW, rowH2); ax += deleteW + actionGap;
    importPresetsBtn.setBounds(ax, r2Y, importW, rowH2); ax += importW + actionGap;
    midiLearnButton.setBounds(ax, r2Y, midiLearnW, rowH2);

    // MIDI Learn panel overlay (anchored to action row)
    if (midiLearnPanelVisible)
    {
        const int panelW = 300;
        const int panelH = 200;
        midiLearnPanel.setBounds(midiLearnButton.getX(),
                                 midiLearnButton.getBottom() + 4,
                                 panelW, panelH);
    }

    // ---- Preset zone, R3 : performance (velocity, playMode, palmMute, pitchBend) ----
    const auto presetR3 = headerZones.presetTertiaryRow.reduced(0, 1);
    const int rowH3 = juce::jmin(layout.compact ? 20 : 24, presetR3.getHeight());
    const int r3Y = presetR3.getY() + juce::jmax(0, (presetR3.getHeight() - rowH3) / 2);
    velocityCurveLabel.setText(layout.compact ? "Vel" : "Vel Curve", juce::dontSendNotification);
    playModeLabel.setText("Play", juce::dontSendNotification);
    palmMuteLabel.setText("Mute", juce::dontSendNotification);
    pitchBendRangeLabel.setText("PB", juce::dontSendNotification);
    const auto perfLabelFont = juce::Font(juce::FontOptions{}
                                              .withHeight(layout.compact ? 9.6f : 11.4f)
                                              .withStyle("Bold"));
    for (auto* label : { &velocityCurveLabel, &playModeLabel, &palmMuteLabel, &pitchBendRangeLabel })
    {
        label->setFont(perfLabelFont);
        label->setMinimumHorizontalScale(0.72f);
    }

    const int perfGap  = layout.compact ? 5 : 8;
    const int vcLblW   = layout.compact ? 22 : 56;
    const int vcW      = layout.compact ? 66 : 76;
    const int playLblW = layout.compact ? 24 : 30;
    const int playW    = layout.compact ? 58 : 88;
    const int muteLblW = layout.compact ? 28 : 36;
    const int muteW    = layout.compact ? 62 : 80;
    const int pbLblW   = layout.compact ? 20 : 24;
    const int pbW      = layout.compact ? 50 : 72;
    int perfX = presetR3.getX();
    velocityCurveLabel.setBounds(perfX, r3Y, vcLblW, rowH3); perfX += vcLblW + 2;
    velocityCurveSelector.setBounds(perfX, r3Y, vcW, rowH3); perfX += vcW + perfGap;
    playModeLabel.setBounds(perfX, r3Y, playLblW, rowH3); perfX += playLblW + 2;
    playModeSelector.setBounds(perfX, r3Y, playW, rowH3); perfX += playW + perfGap;
    palmMuteLabel.setBounds(perfX, r3Y, muteLblW, rowH3); perfX += muteLblW + 2;
    palmMuteDial.setBounds(perfX, r3Y, muteW, rowH3); perfX += muteW + perfGap;
    pitchBendRangeLabel.setBounds(perfX, r3Y, pbLblW, rowH3); perfX += pbLblW + 2;
    pitchBendRangeDial.setBounds(perfX, r3Y, pbW, rowH3);

    // ---- Status zone, R1 : quality + output ---------------------------------
    const int statusH1 = layout.compact ? 22 : 24;
    const auto statusR1 = headerZones.statusPrimaryRow.reduced(0, 1).withTrimmedRight(statusRightTrim);
    const int sR1Y = statusR1.getY() + juce::jmax(0, (statusR1.getHeight() - statusH1) / 2);
    const int qualityW = layout.compact ? 90 : 102;
    const int outputW  = layout.compact ? 80 : 92;
    int sx = statusR1.getX();
    qualitySelector.setBounds(sx, sR1Y, qualityW, statusH1); sx += qualityW + 8;
    outputSelector.setBounds(sx, sR1Y, outputW, statusH1);

    // ---- Status zone, R2 : fxLock + tooltip + midiCC (fill) + voiceCount ----
    const int statusH2 = layout.compact ? 22 : 24;
    const auto statusR2 = headerZones.statusSecondaryRow.reduced(0, 1).withTrimmedRight(statusRightTrim);
    const int sR2Y = statusR2.getY() + juce::jmax(0, (statusR2.getHeight() - statusH2) / 2);
    const int fxLockW  = layout.compact ? 76 : 88;
    const int tooltipW = layout.compact ? 78 : 90;
    const int voiceW   = layout.compact ? 36 : 42;
    const int statusGap = layout.compact ? 4 : 6;
    sx = statusR2.getX();
    fxLockButton.setBounds(sx, sR2Y, fxLockW, statusH2); sx += fxLockW + statusGap;
    tooltipModeBtn.setBounds(sx, sR2Y, tooltipW, statusH2); sx += tooltipW + statusGap;
    const int voiceX = statusR2.getRight() - voiceW;
    const int midiCCRight = voiceX - statusGap;
    midiCCPageLabel.setBounds(sx, sR2Y, juce::jmax(0, midiCCRight - sx), statusH2);
    voiceCountLabel.setBounds(voiceX, sR2Y, voiceW, statusH2);
    updateMidiCCPageLabel();

    // ---- Status zone, R3 : ARP + rand --------------------------------------
    const int statusH3 = layout.compact ? 22 : 24;
    const auto statusR3 = headerZones.statusTertiaryRow.reduced(0, 1).withTrimmedRight(statusRightTrim);
    const int sR3Y = statusR3.getY() + juce::jmax(0, (statusR3.getHeight() - statusH3) / 2);
    const int arpGap   = layout.compact ? 3 : 4;
    const int arpEnW   = layout.compact ? 26 : 34;
    const int arpModeW = layout.compact ? 42 : 52;
    const int arpRateW = layout.compact ? 34 : 44;
    const int arpOctW  = layout.compact ? 34 : 48;
    const int arpGateW = layout.compact ? 34 : 42;
    const int arpHldW  = layout.compact ? 22 : 28;
    const int randW    = layout.compact ? 42 : 46;
    sx = statusR3.getX();
    arpEnableButton.setBounds(sx, sR3Y, arpEnW, statusH3); sx += arpEnW + arpGap;
    arpModeSelector.setBounds(sx, sR3Y, arpModeW, statusH3); sx += arpModeW + arpGap;
    arpRateSelector.setBounds(sx, sR3Y, arpRateW, statusH3); sx += arpRateW + arpGap;
    arpOctavesDial.setBounds(sx, sR3Y, arpOctW, statusH3); sx += arpOctW + arpGap;
    arpGateDial.setBounds(sx, sR3Y, arpGateW, statusH3); sx += arpGateW + arpGap;
    arpHoldButton.setBounds(sx, sR3Y, arpHldW, statusH3);
    randButton.setBounds(statusR3.getRight() - randW, sR3Y, randW, statusH3);

    // Riff Generator layout
    const int riffStyleW = layout.compact ? 54 : 62;
    const int riffIntW   = layout.compact ? 40 : 48;
    const int riffGateW  = layout.compact ? 36 : 40;
    const int riffHldW   = layout.compact ? 24 : 28;
    sx = statusR3.getX();
    riffStyleSelector.setBounds(sx, sR3Y, riffStyleW, statusH3); sx += riffStyleW + arpGap;
    riffIntensityDial.setBounds(sx, sR3Y, riffIntW, statusH3); sx += riffIntW + arpGap;
    riffGateDial.setBounds(sx, sR3Y, riffGateW, statusH3); sx += riffGateW + arpGap;
    riffHoldButton.setBounds(sx, sR3Y, riffHldW, statusH3);

    const int selPad = layout.compact ? 12 : 14;
    const int selectorInnerX = layout.contentX + selPad;
    const int selectorInnerW = layout.contentW - selPad * 2;
    const int selectorTopY = layout.selectorY + (layout.compact ? 26 : 28);
    const int selectorRowH = layout.compact ? 21 : 23;
    const int selectorGap = layout.compact ? 8 : 10;
    const int tabsZoneW = static_cast<int>(selectorInnerW * (layout.compact ? 0.58f : 0.59f));
    const int comboZoneW = selectorInnerW - tabsZoneW - selectorGap;
    const int tabGap = interpolateGap(gapDensity, 6, 8, 10);
    const int tabW = (tabsZoneW - tabGap * (mgs::kNumFamilies - 1)) / mgs::kNumFamilies;
    for (int familyIndex = 0; familyIndex < mgs::kNumFamilies; ++familyIndex)
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
    instrSelector.setVisible(false);
    instrSelector.setBounds(0, 0, 0, 0);
    modelSelectorLbl.setVisible(false);
    modelSelectorLbl.setBounds(0, 0, 0, 0);
    modelSelector.setBounds(selectorInnerX + tabsZoneW + selectorGap, selectorTopY, comboZoneW, selectorRowH);

    const int cPad = layout.compact ? 13 : 16;
    const int knobGapX = interpolateGap(gapDensity, 7, 10, 12);
    const int knobGapY = interpolateGap(gapDensity, 8, 10, 12);
    const int knobW = (layout.colW - cPad * 2 - knobGapX * 2) / 3;
    const int lblH = layout.compact ? 12 : 14;
    const int graphTargetH = layout.compact ? 86 : (layout.roomy ? 178 : 126);
    const int knobH = juce::jlimit(layout.compact ? 50 : 58,
                                   layout.roomy ? 98 : 84,
                                   (layout.bodyH - graphTargetH - cPad * 2 - lblH * 3 - knobGapY * 3) / 3);
    const int protectedKeyboardTop = layout.kbY - (layout.compact ? 14 : 18);

    const int sourceInnerX = layout.col1X + cPad;
    const int sourceInnerW = layout.colW - cPad * 2;
    const int sourceTopY = layout.bodyY + cPad + 28;
    const int sourceBottomY = protectedKeyboardTop - 8;
    const int envH = juce::jlimit(layout.compact ? 138 : 164,
                                  layout.roomy ? 262 : 224,
                                  static_cast<int>((sourceBottomY - sourceTopY) * 0.52f));
    envVisual.setVisible(true);
    envVisual.setBounds(sourceInnerX, sourceTopY, sourceInnerW, envH);

    const int sourceControlsY = envVisual.getBottom() + (layout.compact ? 8 : 10);
    const int adsrGapX = interpolateGap(gapDensity, 6, 8, 10);
    const int adsrGapY = interpolateGap(gapDensity, 8, 10, 12);
    const int remainingH = sourceBottomY - sourceControlsY;
    const int adsrW = (sourceInnerW - adsrGapX * 3) / 4;
    const int adsrKnobH = juce::jlimit(layout.compact ? 44 : 50,
                                       layout.roomy ? 78 : 64,
                                       juce::jmax(layout.compact ? 44 : 50,
                                                  (remainingH - lblH * 4 - adsrGapY * 2) / 2));
    const int secondaryGapX = interpolateGap(gapDensity, 8, 10, 12);
    const int secondaryW = (sourceInnerW - secondaryGapX * 2) / 3;
    const int smallSourceKnobH = juce::jlimit(layout.compact ? 42 : 48,
                                              layout.roomy ? 76 : 62,
                                              juce::jmax(layout.compact ? 42 : 48,
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
    layoutSourceDial(9, sourceInnerX, sourceTailY, secondaryW, smallSourceKnobH);
    layoutSourceDial(0, sourceInnerX + secondaryW + secondaryGapX, sourceTailY, secondaryW, smallSourceKnobH);
    layoutSourceDial(1, sourceInnerX + 2 * (secondaryW + secondaryGapX), sourceTailY, secondaryW, smallSourceKnobH);

    const int toneIdx[] = { 7, 8, 10, 2, 13, 11 };
    const int col2StartY = layout.bodyY + cPad + 28;
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
    const int cutoffSize = juce::jlimit(layout.compact ? 66 : 72,
                                        layout.roomy ? 116 : 104,
                                        knobH + (layout.compact ? 14 : 18));
    const int cutoffX = layout.col2X + (layout.colW - cutoffSize) / 2;
    const int cutoffY = col2StartY + 2 * (knobH + lblH + knobGapY) + (layout.compact ? 6 : 10);
    envLabels[12].setBounds(cutoffX, cutoffY, cutoffSize, lblH);
    envDials[12].setBounds(cutoffX, cutoffY + lblH, cutoffSize, cutoffSize);

    lfoRateDial.setVisible(false);
    lfoDepthDial.setVisible(false);
    lfoWaveSelector.setVisible(false);
    lfoRateDial.setVisible(false);
    lfoRateDial.setBounds(0, 0, 0, 0);
    lfoDepthDial.setVisible(false);
    lfoDepthDial.setBounds(0, 0, 0, 0);
    lfoWaveSelector.setVisible(false);
    lfoWaveSelector.setBounds(0, 0, 0, 0);
    lfoVisual.setVisible(false);
    lfoVisual.setBounds(0, 0, 0, 0);

    const int col3StartY = layout.bodyY + cPad + 28;
    const int rightTabGap = interpolateGap(gapDensity, 6, 8, 10);
    const int rightTabH = layout.compact ? 23 : 26;
    // Width proportional to label length ("MACRO & LFO" is over 5x longer than "FX") so the
    // longest label isn't squeezed while the shortest carries needless empty space.
    const int rightTabsTotalW = layout.colW - cPad * 2 - rightTabGap * (kRightPanelSections - 1);
    int rightTabLabelLen[kRightPanelSections];
    int labelLenSum = 0;
    for (int i = 0; i < kRightPanelSections; ++i)
    {
        rightTabLabelLen[i] = static_cast<int>(juce::String(kRightPanelSectionLabels[i]).length());
        labelLenSum += rightTabLabelLen[i];
    }
    int rightTabX = layout.col3X + cPad;
    int rightTabWUsed = 0;
    for (int sectionIndex = 0; sectionIndex < kRightPanelSections; ++sectionIndex)
    {
        const bool isLast = sectionIndex == kRightPanelSections - 1;
        const int rightTabW = isLast
                                   ? rightTabsTotalW - rightTabWUsed
                                   : juce::jmax(36, (rightTabsTotalW * rightTabLabelLen[sectionIndex]) / labelLenSum);
        auto& tab = rightPanelTabs[static_cast<std::size_t>(sectionIndex)];
        tab.setBounds(rightTabX, col3StartY, rightTabW, rightTabH);
        tab.setSelected(sectionIndex == activeRightPanelSection);
        rightTabX += rightTabW + rightTabGap;
        rightTabWUsed += rightTabW;
    }
    const int sectionContentY = col3StartY + rightTabH + (layout.compact ? 10 : 12);

    const int macroGap = interpolateGap(gapDensity, 8, 10, 12);
    const int macroW = (layout.colW - cPad * 2 - macroGap * 3) / 4;
    const int macroH = juce::jlimit(layout.compact ? 48 : 54,
                                    layout.roomy ? 84 : 72,
                                    juce::jmin(macroW, knobH - (layout.compact ? 6 : 10)));
    for (int i = 0; i < kMacroVisible; ++i)
    {
        const auto si = static_cast<std::size_t>(i);
        macroLbls[si].setVisible(activeRightPanelSection == 0);
        macroDials[si].setVisible(activeRightPanelSection == 0);
        if (activeRightPanelSection == 0)
        {
            const int xk = layout.col3X + cPad + i * (macroW + macroGap);
            macroLbls[si].setBounds(xk, sectionContentY, macroW, lblH);
            macroDials[si].setBounds(xk, sectionContentY + lblH, macroW, macroH);
        }
        else
        {
            macroLbls[si].setVisible(false);
            macroLbls[si].setBounds(0, 0, 0, 0);
            macroDials[si].setVisible(false);
            macroDials[si].setBounds(0, 0, 0, 0);
        }
    }

    if (activeRightPanelSection == 0)
    {
        const int lfoVisualY = sectionContentY + macroH + lblH + (layout.compact ? 14 : 16);
        const int lfoVisualH = juce::jmax(104, protectedKeyboardTop - lfoVisualY - 8);
        lfoVisual.setVisible(true);
        lfoVisual.setBounds(layout.col3X + cPad, lfoVisualY, layout.colW - cPad * 2, lfoVisualH);
    }

    modLfo2RateLabel.setVisible(false);
    modLfo2WaveLabel.setVisible(false);
    modLfo2RateDial.setVisible(false);
    modLfo2WaveSelector.setVisible(false);
    modLfo2RateLabel.setVisible(false);
    modLfo2RateLabel.setBounds(0, 0, 0, 0);
    modLfo2WaveLabel.setVisible(false);
    modLfo2WaveLabel.setBounds(0, 0, 0, 0);
    modLfo2RateDial.setVisible(false);
    modLfo2RateDial.setBounds(0, 0, 0, 0);
    modLfo2WaveSelector.setVisible(false);
    modLfo2WaveSelector.setBounds(0, 0, 0, 0);
    for (int slotIndex = 0; slotIndex < kModSlots; ++slotIndex)
    {
        modSlotLabels[static_cast<std::size_t>(slotIndex)].setVisible(false);
        modSourceBoxes[static_cast<std::size_t>(slotIndex)].setVisible(false);
        modDestBoxes[static_cast<std::size_t>(slotIndex)].setVisible(false);
        modAmountSliders[static_cast<std::size_t>(slotIndex)].setVisible(false);
        modSlotLabels[static_cast<std::size_t>(slotIndex)].setVisible(false);
        modSlotLabels[static_cast<std::size_t>(slotIndex)].setBounds(0, 0, 0, 0);
        modSourceBoxes[static_cast<std::size_t>(slotIndex)].setVisible(false);
        modSourceBoxes[static_cast<std::size_t>(slotIndex)].setBounds(0, 0, 0, 0);
        modDestBoxes[static_cast<std::size_t>(slotIndex)].setVisible(false);
        modDestBoxes[static_cast<std::size_t>(slotIndex)].setBounds(0, 0, 0, 0);
        modAmountSliders[static_cast<std::size_t>(slotIndex)].setVisible(false);
        modAmountSliders[static_cast<std::size_t>(slotIndex)].setBounds(0, 0, 0, 0);
    }
    if (activeRightPanelSection == 1)
    {
        const int modAreaX = layout.col3X + cPad;
        const int modAreaW = layout.colW - cPad * 2;
        const int matrixY = sectionContentY;
        const int controlGap = layout.compact ? 5 : 6;
        const int modAreaH = juce::jmax(80, protectedKeyboardTop - matrixY - 10);
        const int footerReserve = layout.compact ? 86 : 96;
        const int rowGap = layout.compact ? 4 : 5;
        const int matrixRowsH = juce::jmax(96, modAreaH - footerReserve);
        const int rowH = juce::jlimit(20, 28,
                                      (matrixRowsH - rowGap * juce::jmax(0, kModSlots - 1)) / kModSlots);
        const int amountSize = juce::jlimit(layout.compact ? 22 : 24,
                                            layout.compact ? 28 : 30,
                                            rowH + 2);
        const int labelW = 18;
        const int comboAreaW = modAreaW - labelW - amountSize - controlGap * 3;
        const int sourceW = juce::jmax(74, comboAreaW / 2);
        const int destW = juce::jmax(74, comboAreaW - sourceW);
        const int comboH = juce::jlimit(20, 22, rowH);
        const int maxRowBottom = matrixY + matrixRowsH;
        int visibleSlotCount = 0;
        for (int slotIndex = 0; slotIndex < kModSlots; ++slotIndex)
        {
            const int rowY = matrixY + slotIndex * (rowH + rowGap);
            if (rowY + rowH > maxRowBottom)
                break;
            const int comboY = rowY + juce::jmax(0, (rowH - comboH) / 2);
            const int amountY = rowY + juce::jmax(0, (rowH - amountSize) / 2);
            modSlotLabels[static_cast<std::size_t>(slotIndex)].setBounds(modAreaX, rowY + juce::jmax(0, (rowH - 16) / 2), labelW, 16);
            modSourceBoxes[static_cast<std::size_t>(slotIndex)].setBounds(modAreaX + labelW + controlGap, comboY, sourceW, comboH);
            modDestBoxes[static_cast<std::size_t>(slotIndex)].setBounds(modAreaX + labelW + controlGap * 2 + sourceW, comboY, destW, comboH);
            modAmountSliders[static_cast<std::size_t>(slotIndex)].setBounds(modAreaX + modAreaW - amountSize, amountY, amountSize, amountSize);
            ++visibleSlotCount;
        }
        for (int slotIndex = 0; slotIndex < kModSlots; ++slotIndex)
        {
            const bool vis = slotIndex < visibleSlotCount;
            modSlotLabels[static_cast<std::size_t>(slotIndex)].setVisible(vis);
            modSourceBoxes[static_cast<std::size_t>(slotIndex)].setVisible(vis);
            modDestBoxes[static_cast<std::size_t>(slotIndex)].setVisible(vis);
            modAmountSliders[static_cast<std::size_t>(slotIndex)].setVisible(vis);
        }
        const int footerY = matrixY + visibleSlotCount * (rowH + rowGap) + 10;
        if (footerY + 40 <= protectedKeyboardTop)
        {
            const int footerLabelW = modAreaW / 2 - controlGap;
            modLfo2RateLabel.setBounds(modAreaX, footerY, footerLabelW, 14);
            modLfo2WaveLabel.setBounds(modAreaX + footerLabelW + controlGap, footerY, footerLabelW, 14);
            modLfo2RateDial.setBounds(modAreaX, footerY + 12, footerLabelW, juce::jmin(70, modAreaW / 2));
            modLfo2WaveSelector.setBounds(modAreaX + footerLabelW + controlGap, footerY + 18, footerLabelW, 22);
            modLfo2RateLabel.setVisible(true);
            modLfo2WaveLabel.setVisible(true);
            modLfo2RateDial.setVisible(true);
            modLfo2WaveSelector.setVisible(true);
        }
    }

    const int fxAreaY = sectionContentY + 20;
    const int fxAreaH = juce::jmax(180, protectedKeyboardTop - fxAreaY - 10);
    constexpr int kBypassW = 34;
    constexpr int kRackGap = 14;
    constexpr int kRackRowGap = 4;
    const int rackTotalW = juce::jlimit(layout.compact ? 108 : 118,
                                        layout.roomy ? 162 : 146,
                                        layout.colW / 2 - 6);
    const int rackItemW = rackTotalW - kBypassW - 6;
    const int rackRowH = juce::jlimit(layout.compact ? 18 : 20,
                                      layout.roomy ? 30 : 26,
                                      (fxAreaH - kRackRowGap * (kFxTabs - 1)) / kFxTabs);
    const int instrIdxFx = selectedInstrFromParam();
    int availableFxTabs = 0;
    for (int t = 0; t < kFxTabs; ++t)
        if (isFxTabAvailable(t, instrIdxFx))
            ++availableFxTabs;
    const int visCount = juce::jmax(1, availableFxTabs);
    const int rackBlockH = visCount * rackRowH + kRackRowGap * (visCount - 1);
    const int rackStartY = fxAreaY + juce::jmax(0, (fxAreaH - rackBlockH) / 2);
    int rackCurY = rackStartY;
    for (int t = 0; t < kFxTabs; ++t)
    {
        const bool vis = activeRightPanelSection == 2 && isFxTabAvailable(t, instrIdxFx);
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

    const int detailX = layout.col3X + cPad + rackTotalW + kRackGap;
    const int detailW = layout.colW - cPad * 2 - rackTotalW - kRackGap;
    fxDetailTitle.setVisible(activeRightPanelSection == 2);
    fxDetailTitle.setBounds(detailX, fxAreaY, detailW, 16);
    for (int i = 0; i < kFxN; ++i)
    {
        fxDials[static_cast<std::size_t>(i)].setVisible(false);
        fxLbls[static_cast<std::size_t>(i)].setVisible(false);
        fxDials[static_cast<std::size_t>(i)].setVisible(false);
        fxDials[static_cast<std::size_t>(i)].setBounds(0, 0, 0, 0);
        fxLbls[static_cast<std::size_t>(i)].setVisible(false);
        fxLbls[static_cast<std::size_t>(i)].setBounds(0, 0, 0, 0);
    }
    delaySyncLabel.setVisible(false);
    delayDivisionLabel.setVisible(false);
    delaySyncSelector.setVisible(false);
    delayDivisionSelector.setVisible(false);
    delaySyncLabel.setVisible(false);
    delaySyncLabel.setBounds(0, 0, 0, 0);
    delayDivisionLabel.setVisible(false);
    delayDivisionLabel.setBounds(0, 0, 0, 0);
    delaySyncSelector.setVisible(false);
    delaySyncSelector.setBounds(0, 0, 0, 0);
    delayDivisionSelector.setVisible(false);
    delayDivisionSelector.setBounds(0, 0, 0, 0);
    reverbTypeLabel.setVisible(false);
    reverbTypeLabel.setBounds(0, 0, 0, 0);
    reverbTypeSelector.setVisible(false);
    reverbTypeSelector.setBounds(0, 0, 0, 0);
    if (activeRightPanelSection == 2)
    {
        const int detailTop = fxAreaY + 26;
        const int detailH = juce::jmax(80, fxAreaY + fxAreaH - detailTop);
        fxDetailTitle.setText(juce::String("DETAIL: ") + kFxTabNames[activeFxTab], juce::dontSendNotification);
        int visibleCount = 0;
        for (int slot = 0; slot < kFxPerTab; ++slot)
            if (kFxTabMap[activeFxTab][slot] >= 0)
                ++visibleCount;
        const int detailCols = visibleCount <= 1 ? 1 : (visibleCount <= 4 ? 2 : 3);
        const int detailRows = juce::jmax(1, (visibleCount + detailCols - 1) / detailCols);
        const int fxGapX = layout.compact ? 8 : 12;
        const int fxGapY = layout.compact ? 8 : 12;
        const int detailLabelH = lblH;
        const int detailDialW = detailCols > 0
            ? (detailW - fxGapX * (detailCols - 1)) / detailCols
            : detailW;
        const int detailDialH = juce::jlimit(layout.compact ? 44 : 54,
                                             layout.roomy ? 120 : 94,
                                             (detailH - detailLabelH * detailRows - fxGapY * juce::jmax(0, detailRows - 1)) / detailRows);
        int visibleIndex = 0;
        for (int slot = 0; slot < kFxPerTab; ++slot)
        {
            const int fxIndex = kFxTabMap[activeFxTab][slot];
            if (fxIndex < 0)
                continue;
            const int row = visibleIndex / detailCols;
            const int col = visibleIndex % detailCols;
            const int xk = detailX + col * (detailDialW + fxGapX);
            const int yk = detailTop + row * (detailDialH + detailLabelH + fxGapY);
            const auto si = static_cast<std::size_t>(fxIndex);
            fxLbls[si].setBounds(xk, yk, detailDialW, detailLabelH);
            fxDials[si].setBounds(xk, yk + detailLabelH, detailDialW, detailDialH);
            fxLbls[si].setVisible(true);
            fxDials[si].setVisible(true);
            ++visibleIndex;
        }
        if (activeFxTab == 6)
        {
            const int controlsY = detailTop + detailRows * (detailDialH + detailLabelH + fxGapY) + 4;
            const int comboW = (detailW - fxGapX) / 2;
            delaySyncLabel.setBounds(detailX, controlsY, comboW, 14);
            delayDivisionLabel.setBounds(detailX + comboW + fxGapX, controlsY, comboW, 14);
            delaySyncSelector.setBounds(detailX, controlsY + 14, comboW, 22);
            delayDivisionSelector.setBounds(detailX + comboW + fxGapX, controlsY + 14, comboW, 22);
            delaySyncLabel.setVisible(true);
            delayDivisionLabel.setVisible(true);
            delaySyncSelector.setVisible(true);
            delayDivisionSelector.setVisible(true);
        }
        if (activeFxTab == 0)
        {
            const int controlsY = detailTop + detailRows * (detailDialH + detailLabelH + fxGapY) + 4;
            reverbTypeLabel.setBounds(detailX, controlsY, detailW, 14);
            reverbTypeSelector.setBounds(detailX, controlsY + 14, detailW, 22);
            reverbTypeLabel.setVisible(true);
            reverbTypeSelector.setVisible(true);
        }
    }

    const int keyboardInsetLeft = layout.compact ? 76 : 78;
    const int keyboardInsetTop = layout.compact ? 6 : 8;
    const int keyboardH = juce::jmax(36, layout.kbH - keyboardInsetTop * 2);
    const int keyboardCenterY = layout.kbY + keyboardInsetTop + keyboardH / 2;
    const int keyboardInsetRight = 18; // matches the accent-line margin drawn above the keyboard dock
    keyboard->setBounds(layout.contentX + keyboardInsetLeft, layout.kbY + keyboardInsetTop,
                        layout.contentW - keyboardInsetLeft - keyboardInsetRight, keyboardH);
    octaveDownBtn.setBounds(layout.contentX + 6, keyboardCenterY - 14, 20, 26);
    octaveUpBtn.setBounds(layout.contentX + 30, keyboardCenterY - 14, 20, 26);

    auto applyLbl = [this, layout](juce::Label& label)
    {
        label.setFont(juce::Font(juce::FontOptions{}.withHeight(layout.compact ? 11.0f : 12.0f).withStyle("Bold")));
        const auto colour = activePalette.textMuted.isTransparent()
            ? synthcol::textSec
            : activePalette.textMuted.withAlpha(0.98f);
        label.setColour(juce::Label::textColourId, colour);
    };
    for (auto& label : envLabels) applyLbl(label);
    for (auto& label : macroLbls) applyLbl(label);
    for (auto& label : fxLbls) applyLbl(label);
    for (auto& label : modSlotLabels) applyLbl(label);
    applyLbl(delaySyncLabel);
    applyLbl(delayDivisionLabel);
    applyLbl(modLfo2RateLabel);
    applyLbl(modLfo2WaveLabel);
}

GuitarSynthAudioProcessorEditor::VisualLayoutSnapshot
GuitarSynthAudioProcessorEditor::computeVisualLayoutSnapshot(int width, int height) const
{
    const auto layout = computeLayoutMetrics(width, height);

    VisualLayoutSnapshot snapshot;
    snapshot.compact = layout.compact;
    snapshot.roomy = layout.roomy;
    snapshot.headerH = layout.headerH;
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

// =============================================================================
// MIDI Learn Panel
// =============================================================================
void GuitarSynthAudioProcessorEditor::showMidiLearnPanel(bool visible)
{
    midiLearnPanelVisible = visible;
    midiLearnPanel.setVisible(visible);
    if (visible)
        refreshMidiLearnPanel();
    resized();
}

void GuitarSynthAudioProcessorEditor::refreshMidiLearnPanel()
{
    midiLearnPanel.mappings = proc.getMidiLearnMappings();
    midiLearnPanel.resized();
    midiLearnPanel.repaint();
}

void GuitarSynthAudioProcessorEditor::MidiLearnPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e).withAlpha(0.96f));
    g.setColour(juce::Colour(0xff27AE60));
    g.drawRect(getLocalBounds(), 1);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(13.0f).withStyle("Bold")));
    g.drawText("MIDI CC MAPPINGS", getLocalBounds().removeFromTop(26).reduced(8, 0),
               juce::Justification::centredLeft);
    if (mappings.empty())
    {
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
        g.setColour(juce::Colour(0xff888888));
        g.drawText("No mappings. Arm a parameter and move a CC.",
                   getLocalBounds().withTrimmedTop(40).reduced(8, 0),
                   juce::Justification::centredLeft);
    }
}

void GuitarSynthAudioProcessorEditor::MidiLearnPanel::resized()
{
    clearAllBtn.setBounds(getWidth() - 80, 4, 74, 20);
    clearBtns.clear();
    int y = 30;
    for (int i = 0; i < static_cast<int>(mappings.size()); ++i)
    {
        auto& btn = clearBtns.emplace_back(std::make_unique<juce::TextButton>("X"));
        btn->setBounds(getWidth() - 30, y, 24, 18);
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff802020));
        const int cc = mappings[static_cast<std::size_t>(i)].first;
        btn->onClick = [this, cc] { if (onClearMapping) onClearMapping(cc); };
        addAndMakeVisible(*btn);
        y += 22;
    }
    addAndMakeVisible(clearAllBtn);
    clearAllBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff802020));
    clearAllBtn.onClick = [this] { if (onClearAll) onClearAll(); };
}

#if defined(UWDEVST_GUITAR_TEST_BUILD)
GuitarSynthAudioProcessorEditor::LayoutSnapshot
GuitarSynthAudioProcessorEditor::captureLayoutSnapshotForTests() const
{
    const auto layout = computeVisualLayoutSnapshot(getWidth(), getHeight());

    LayoutSnapshot snapshot;
    snapshot.compact = layout.compact;
    snapshot.editorBounds = getLocalBounds();
    snapshot.headerBounds = layout.headerBounds;
    snapshot.selectorPanelBounds = layout.selectorPanelBounds;
    snapshot.presetPerformanceRowBounds = layout.headerZones.presetTertiaryRow.reduced(0, 1);
    snapshot.modelSelectorBounds = modelSelector.getBounds();
    snapshot.presetSearchBounds = presetSearch.getBounds();
    snapshot.presetBoxBounds = presetBox.getBounds();
    snapshot.nextPresetBounds = nextPresetBtn.getBounds();
    snapshot.presetTierFilterBounds = presetTierFilter.getBounds();
    snapshot.presetUseFilterBounds = presetUseFilter.getBounds();
    snapshot.presetRoleFilterBounds = presetRoleFilter.getBounds();
    snapshot.qualitySelectorBounds = qualitySelector.getBounds();
    snapshot.outputSelectorBounds = outputSelector.getBounds();
    snapshot.midiCCPageLabelBounds = midiCCPageLabel.getBounds();
    snapshot.voiceCountLabelBounds = voiceCountLabel.getBounds();
    snapshot.gainBounds = gainDial.getBounds();
    snapshot.velocityCurveSelectorBounds = velocityCurveSelector.getBounds();
    snapshot.playModeSelectorBounds = playModeSelector.getBounds();
    snapshot.palmMuteBounds = palmMuteDial.getBounds();
    snapshot.pitchBendRangeLabelBounds = pitchBendRangeLabel.getBounds();
    snapshot.pitchBendRangeBounds = pitchBendRangeDial.getBounds();
    snapshot.keyboardBounds = keyboard != nullptr ? keyboard->getBounds() : juce::Rectangle<int>();
    snapshot.octaveDownBounds = octaveDownBtn.getBounds();
    snapshot.octaveUpBounds = octaveUpBtn.getBounds();
    snapshot.fxLockBounds = fxLockButton.getBounds();
    snapshot.fxLockVisible = fxLockButton.isVisible();
    snapshot.modMatrixToggleBounds = rightPanelTabs[1].getBounds();
    snapshot.midiCCPageText = midiCCPageLabel.getText();
    snapshot.velocityCurveText = velocityCurveSelector.getText();
    auto& palmMuteSlider = const_cast<juce::Slider&>(palmMuteDial);
    snapshot.palmMuteText = palmMuteSlider.getTextFromValue(palmMuteSlider.getValue());
    snapshot.pitchBendRangeLabelText = pitchBendRangeLabel.getText();
    auto& airCutDial = const_cast<juce::Slider&>(envDials[12]);
    snapshot.airCutText = airCutDial.getTextFromValue(airCutDial.getValue());

    for (const auto& tab : rightPanelTabs)
    {
        if (!tab.isVisible())
            continue;

        snapshot.rightPanelTabsBounds = snapshot.rightPanelTabsBounds.isEmpty()
            ? tab.getBounds()
            : snapshot.rightPanelTabsBounds.getUnion(tab.getBounds());
    }

    auto includeVisibleBounds = [&snapshot](const juce::Component& component)
    {
        if (!component.isVisible() || component.getBounds().isEmpty())
            return;

        snapshot.modMatrixContentBounds = snapshot.modMatrixContentBounds.isEmpty()
            ? component.getBounds()
            : snapshot.modMatrixContentBounds.getUnion(component.getBounds());
    };

    includeVisibleBounds(modLfo2RateLabel);
    includeVisibleBounds(modLfo2WaveLabel);
    includeVisibleBounds(modLfo2RateDial);
    includeVisibleBounds(modLfo2WaveSelector);
    for (int slotIndex = 0; slotIndex < kModSlots; ++slotIndex)
    {
        includeVisibleBounds(modSlotLabels[static_cast<std::size_t>(slotIndex)]);
        includeVisibleBounds(modSourceBoxes[static_cast<std::size_t>(slotIndex)]);
        includeVisibleBounds(modDestBoxes[static_cast<std::size_t>(slotIndex)]);
        includeVisibleBounds(modAmountSliders[static_cast<std::size_t>(slotIndex)]);
    }

    return snapshot;
}

void GuitarSynthAudioProcessorEditor::setRightPanelSectionForTests(int sectionIndex)
{
    switchRightPanelSection(sectionIndex);
}
#endif

void GuitarSynthAudioProcessorEditor::switchEffectTab(int tabIndex)
{
    if (tabIndex < 0 || tabIndex >= kFxTabs)
        return;

    const int instrIdx = selectedInstrFromParam();
    if (!isFxTabAvailable(tabIndex, instrIdx))
        return;

    if (activeFxTab == tabIndex)
    {
        syncFxRackState();
        return;
    }

    activeFxTab = tabIndex;
    fxDetailTitle.setText(juce::String("DETAIL: ") + kFxTabNames[tabIndex], juce::dontSendNotification);
    syncFxRackState();
    resized();
    repaint();
}

void GuitarSynthAudioProcessorEditor::switchRightPanelSection(int sectionIndex)
{
    sectionIndex = juce::jlimit(0, kRightPanelSections - 1, sectionIndex);
    if (activeRightPanelSection == sectionIndex)
        return;

    activeRightPanelSection = sectionIndex;
    if (activeRightPanelSection == 2)
        syncFxAvailability();
    if (activeRightPanelSection == 1)
        syncAdvancedModUi();

    resized();
    repaint();
}

void GuitarSynthAudioProcessorEditor::syncFxRackState()
{
    const int instrIdx = selectedInstrFromParam();
    for (int t = 0; t < kFxTabs; ++t)
    {
        const bool available = isFxTabAvailable(t, instrIdx);
        auto& rackItem = fxRackItems[static_cast<std::size_t>(t)];
        auto& bypass = fxBypassBtns[static_cast<std::size_t>(t)];
        rackItem.setEnabled(available);
        rackItem.setSelected(available && t == activeFxTab);
        rackItem.setEnabledState(available && bypass.getToggleState());
    }
}

void GuitarSynthAudioProcessorEditor::applyInstrumentTheme(int instrIndex)
{
    activePalette = paletteForInstrument(instrIndex);
    const auto catC = activePalette.accent;
    const auto controlText = activePalette.textHi;
    const auto mutedText = activePalette.textMuted;
    const auto controlBg = activePalette.controlBg;
    const auto readoutBg = controlBg.interpolatedWith(juce::Colour(0xff0B0F14), 0.46f).withAlpha(0.96f);
    const auto knobRingTint = activePalette.knobRing.interpolatedWith(activePalette.panelHeader, 0.18f);
    const auto knobBezelTint = activePalette.panelBase.interpolatedWith(catC.darker(0.22f), 0.12f);
    const auto knobCollarTint = activePalette.panelCavity.interpolatedWith(activePalette.panelHeader, 0.24f);
    const auto knobGlowTint = activePalette.knobGlow.interpolatedWith(activePalette.panelHeader.brighter(0.04f), 0.26f);

    setAccentTheme(catC);
    setChromePalette(activePalette.panelHeader,
                     activePalette.panelBase,
                     activePalette.panelCavity,
                     activePalette.panelHeader,
                     activePalette.panelCavity.interpolatedWith(activePalette.panelBase, 0.45f));
    lnf_.setColour(SynthLookAndFeel::knobBezelColourId, knobBezelTint);
    lnf_.setColour(SynthLookAndFeel::knobCollarColourId, knobCollarTint);
    lnf_.setColour(SynthLookAndFeel::knobGlowColourId, knobGlowTint);
    lnf_.setColour(SynthLookAndFeel::knobCapAccentColourId,
                   juce::Colour(0xffD7DEE7).interpolatedWith(knobGlowTint, 0.16f));

    envVisual.setAccent(catC);
    lfoVisual.setAccent(catC);

    auto styleDial = [&](juce::Slider& dial)
    {
        dial.setColour(juce::Slider::rotarySliderFillColourId, knobRingTint);
        dial.setColour(juce::Slider::textBoxTextColourId, controlText);
        dial.setColour(juce::Slider::textBoxBackgroundColourId, readoutBg);
        dial.setColour(juce::Slider::textBoxOutlineColourId, catC.withAlpha(0.44f));
    };

    auto styleCombo = [&](juce::ComboBox& combo)
    {
        combo.setColour(juce::ComboBox::outlineColourId, catC.withAlpha(0.42f));
        combo.setColour(juce::ComboBox::backgroundColourId, controlBg);
        combo.setColour(juce::ComboBox::textColourId, controlText);
        combo.setColour(juce::ComboBox::arrowColourId, catC.brighter(0.18f));
    };

    auto styleLabel = [&](juce::Label& label, juce::Colour colour)
    {
        label.setColour(juce::Label::textColourId, colour);
    };

    for (auto& dial : envDials)
        styleDial(dial);

    for (auto& dial : macroDials)
        styleDial(dial);

    for (auto& dial : fxDials)
        styleDial(dial);

    for (auto& dial : modAmountSliders)
        styleDial(dial);

    styleDial(modLfo2RateDial);
    styleDial(gainDial);
    styleDial(palmMuteDial);
    styleDial(pitchBendRangeDial);
    styleCombo(qualitySelector);
    styleCombo(outputSelector);
    styleCombo(modelSelector);
    styleCombo(velocityCurveSelector);
    styleCombo(playModeSelector);
    styleCombo(lfoWaveSelector);
    styleCombo(modLfo2WaveSelector);
    styleCombo(delaySyncSelector);
    styleCombo(delayDivisionSelector);
    styleCombo(arpModeSelector);
    styleCombo(arpRateSelector);
    styleCombo(presetTierFilter);
    styleCombo(presetUseFilter);
    styleCombo(presetRoleFilter);
    for (auto& box : modSourceBoxes)
        styleCombo(box);
    for (auto& box : modDestBoxes)
        styleCombo(box);

    tooltipModeBtn.setColour(juce::TextButton::buttonColourId, controlBg);
    tooltipModeBtn.setColour(juce::TextButton::buttonOnColourId, catC.withAlpha(0.18f).interpolatedWith(controlBg, 0.62f));
    tooltipModeBtn.setColour(juce::TextButton::textColourOffId, controlText);
    tooltipModeBtn.setColour(juce::TextButton::textColourOnId, controlText);
    fxLockButton.setColour(juce::ToggleButton::textColourId, controlText);
    fxLockButton.setColour(juce::ToggleButton::tickColourId, catC);
    midiCCPageLabel.setColour(juce::Label::textColourId, controlText);
    midiCCPageLabel.setColour(juce::Label::backgroundColourId, readoutBg);
    midiCCPageLabel.setColour(juce::Label::outlineColourId, catC.withAlpha(0.34f));
    for (auto& tab : rightPanelTabs)
        tab.setAccent(catC);

    styleLabel(fxDetailTitle, catC.brighter(0.26f));

    for (auto& rackItem : fxRackItems)
        rackItem.setAccent(catC);

    for (auto& btn : fxBypassBtns)
    {
        btn.setColour(juce::ToggleButton::textColourId, controlText);
        btn.setColour(juce::ToggleButton::tickColourId, catC);
        btn.repaint();
    }

    for (auto& label : envLabels)
        styleLabel(label, mutedText.withAlpha(0.98f));

    for (auto& label : macroLbls)
        styleLabel(label, mutedText.withAlpha(0.98f));

    for (auto& label : fxLbls)
        styleLabel(label, mutedText.withAlpha(0.98f));

    for (auto& label : modSlotLabels)
        styleLabel(label, mutedText.withAlpha(0.94f));

    styleLabel(delaySyncLabel, mutedText.withAlpha(0.96f));
    styleLabel(delayDivisionLabel, mutedText.withAlpha(0.96f));
    styleLabel(modLfo2RateLabel, mutedText.withAlpha(0.96f));
    styleLabel(modLfo2WaveLabel, mutedText.withAlpha(0.96f));
    styleLabel(velocityCurveLabel, mutedText.withAlpha(0.96f));
    styleLabel(playModeLabel, mutedText.withAlpha(0.96f));
    styleLabel(palmMuteLabel, mutedText.withAlpha(0.96f));
    styleLabel(pitchBendRangeLabel, mutedText.withAlpha(0.96f));
    styleLabel(arpModeLabel, mutedText.withAlpha(0.96f));
    styleLabel(arpRateLabel, mutedText.withAlpha(0.96f));
    styleLabel(arpOctavesLabel, mutedText.withAlpha(0.96f));
    styleLabel(arpGateLabel, mutedText.withAlpha(0.96f));

    syncFxRackState();
    repaint();
}

void GuitarSynthAudioProcessorEditor::refreshUiForTesting()
{
    rebuildInstrAttachments();
    syncSelectionUiFromInstr();
    syncFxAvailability();
    refreshPresetList();
    if (activeRightPanelSection == 1)
        syncAdvancedModUi();
    resized();
}

void GuitarSynthAudioProcessorEditor::syncAdvancedModUi()
{
    for (int slotIndex = 0; slotIndex < kModSlots; ++slotIndex)
    {
        const auto slot = proc.getModMatrixSlot(slotIndex);
        auto& sourceBox = modSourceBoxes[static_cast<std::size_t>(slotIndex)];
        auto& destBox = modDestBoxes[static_cast<std::size_t>(slotIndex)];
        auto& amountSlider = modAmountSliders[static_cast<std::size_t>(slotIndex)];
        const auto sourceId = static_cast<int>(slot.source) + 1;
        const auto destId = static_cast<int>(slot.destination) + 1;
        if (sourceBox.getSelectedId() != sourceId)
            sourceBox.setSelectedId(sourceId, juce::dontSendNotification);
        if (destBox.getSelectedId() != destId)
            destBox.setSelectedId(destId, juce::dontSendNotification);
        if (std::abs(amountSlider.getValue() - slot.amount) > 1.0e-6)
            amountSlider.setValue(slot.amount, juce::dontSendNotification);
        const auto tooltip = "Mod amount: " + formatSignedPercent(slot.amount);
        if (amountSlider.getTooltip() != tooltip)
            amountSlider.setTooltip(tooltip);
    }

    const auto lfo2Rate = proc.getModMatrixLfo2Rate();
    const auto lfo2WaveId = proc.getModMatrixLfo2Wave() + 1;
    if (std::abs(modLfo2RateDial.getValue() - lfo2Rate) > 1.0e-6)
        modLfo2RateDial.setValue(lfo2Rate, juce::dontSendNotification);
    if (modLfo2WaveSelector.getSelectedId() != lfo2WaveId)
        modLfo2WaveSelector.setSelectedId(lfo2WaveId, juce::dontSendNotification);
}

bool GuitarSynthAudioProcessorEditor::isFxTabAvailable(int tabIndex, int instrIndex) const
{
    switch (tabIndex)
    {
        case 0: return mgs::isFxAvailable(instrIndex, mgs::GlobalFxSlot::Reverb);
        case 1: return mgs::isFxAvailable(instrIndex, mgs::GlobalFxSlot::Saturator);
        case 2: return mgs::isFxAvailable(instrIndex, mgs::GlobalFxSlot::Transient);
        case 3: return mgs::isFxAvailable(instrIndex, mgs::GlobalFxSlot::Compressor);
        case 4: return mgs::isFxAvailable(instrIndex, mgs::GlobalFxSlot::Eq);
        case 5: return mgs::isFxAvailable(instrIndex, mgs::GlobalFxSlot::Chorus);
        case 6: return mgs::isFxAvailable(instrIndex, mgs::GlobalFxSlot::Delay);
        case 7: return mgs::isFxAvailable(instrIndex, mgs::GlobalFxSlot::Limiter);
        case 8: return mgs::isFxAvailable(instrIndex, mgs::GlobalFxSlot::Cabinet);
        default: return true;
    }
}

int GuitarSynthAudioProcessorEditor::firstAvailableFxTab(int instrIndex) const
{
    for (int tabIndex = 0; tabIndex < kFxTabs; ++tabIndex)
        if (isFxTabAvailable(tabIndex, instrIndex))
            return tabIndex;
    return 0;
}

void GuitarSynthAudioProcessorEditor::syncFxAvailability()
{
    const int instrIdx = selectedInstrFromParam();
    for (int tabIndex = 0; tabIndex < kFxTabs; ++tabIndex)
    {
        const bool available = isFxTabAvailable(tabIndex, instrIdx);
        const auto tooltip = available
            ? juce::String()
            : juce::String(kFxTabNames[tabIndex]) + " is not available for this model";
        fxBypassBtns[static_cast<std::size_t>(tabIndex)].setEnabled(available);
        fxBypassBtns[static_cast<std::size_t>(tabIndex)].setButtonText(available ? "ON" : "OFF");
        fxBypassBtns[static_cast<std::size_t>(tabIndex)].setTooltip(tooltip);
    }

    if (!isFxTabAvailable(activeFxTab, instrIdx))
        switchEffectTab(firstAvailableFxTab(instrIdx));
    else
        syncFxRackState();
}

void GuitarSynthAudioProcessorEditor::rebuildInstrAttachments()
{
    const auto instrIdx = selectedInstrFromParam();
    if (instrIdx == cachedInstrIdx)
        return;

    cachedInstrIdx = instrIdx;
    const auto family = mgs::getFamily(instrIdx);
    const auto& profile = envProfileForInstrument(instrIdx);
    const auto& macroLabels = macroLabelsForFamily(family);

    for (auto& attachment : envAttach)
        attachment.reset();
    outputAtt.reset();

    for (int i = 0; i < kEnvN; ++i)
    {
        const auto si = static_cast<std::size_t>(i);
        auto id = GuitarSynthAudioProcessor::makeInstrParamId(cachedInstrIdx, kEnvCtrls[si].suffix);
        envAttach[si] = std::make_unique<SliderAttach>(proc.getAPVTS(), id, envDials[si]);
    }
    outputAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(),
                                                 GuitarSynthAudioProcessor::makeInstrParamId(cachedInstrIdx, "output"),
                                                 outputSelector);

    for (int i = 0; i < kEnvN; ++i)
    {
        const auto si = static_cast<std::size_t>(i);
        switch (i)
        {
            case 0:
            case 2:
            case 5:
            case 7:
            case 9:
            case 10:
            case 11:
                setupDial(envDials[si], accent_);
                break;
            case 1:
                setupDial(envDials[si], accent_);
                break;
            case 3:
            case 4:
            case 6:
                setupDial(envDials[si], accent_);
                break;
            case 8:
                setupDial(envDials[si], accent_);
                break;
            case 12:
                // No suffix: textFromValueFunction (formatFrequency) already appends "Hz"/"kHz",
                // and JUCE appends getTextValueSuffix() after the custom formatter's text, so
                // passing " Hz" here produced a doubled unit ("X kHz Hz").
                setupGrandDial(envDials[si], accent_, {});
                break;
            case 13:
                setupDial(envDials[si], accent_);
                break;
            default:
                setupDial(envDials[si], accent_);
                break;
        }
    }

    // Re-apply our custom textFromValueFunction/valueFromTextFunction: constructing the
    // SliderAttach objects above resets each slider to the parameter's own default text
    // formatter, silently discarding whatever was set in applyValueFormatters() (this is what
    // made every env dial show a raw, unformatted parameter value instead of "56%"/"7.8 kHz"/etc).
    applyValueFormatters();

    synthui::applyLabelProfile(profile, envLabels);
    synthui::applyMacroLabelProfile(macroLabels, macroLbls);

    applyInstrumentTheme(instrIdx);
    activeFamilyIndex = static_cast<int>(family);
    syncSelectionUiFromInstr();
    syncFxAvailability();
    refreshPresetList();
    applyTooltips();
}

void GuitarSynthAudioProcessorEditor::rebuildModelSelectorForFamily(int familyIndex, int preferredInstr)
{
    familyIndex = juce::jlimit(0, mgs::kNumFamilies - 1, familyIndex);
    modelSelector.clear(juce::dontSendNotification);

    const int first = mgs::kFamilyStart[familyIndex];
    const int count = mgs::kFamilySize[familyIndex];

    for (int i = 0; i < count; ++i)
        modelSelector.addItem(instrumentDisplayName(first + i), first + i + 1);

    int target = preferredInstr;
    if (target < first || target >= first + count)
        target = first;

    modelSelector.setSelectedId(target + 1, juce::dontSendNotification);
}

void GuitarSynthAudioProcessorEditor::syncSelectionUiFromInstr()
{
    const int instrIndex = selectedInstrFromParam();
    const int familyIndex = static_cast<int>(mgs::getFamily(instrIndex));

    activeFamilyIndex = familyIndex;

    if (familySelector.getSelectedId() != familyIndex + 1)
        familySelector.setSelectedId(familyIndex + 1, juce::dontSendNotification);

    rebuildModelSelectorForFamily(familyIndex, instrIndex);

    for (int tabIndex = 0; tabIndex < mgs::kNumFamilies; ++tabIndex)
        familyTabs[static_cast<std::size_t>(tabIndex)].setSelected(tabIndex == familyIndex);

    bool presetFilterChanged = false;
    const bool textureInstrument = mgs::getFamily(instrIndex) == mgs::Family::Electronique;
    if (textureInstrument && presetTierFilter.getSelectedId() == 2)
    {
        presetTierFilter.setSelectedId(3, juce::dontSendNotification);
        presetFilterChanged = true;
    }
    else if (!textureInstrument && presetTierFilter.getSelectedId() == 3)
    {
        presetTierFilter.setSelectedId(2, juce::dontSendNotification);
        presetFilterChanged = true;
    }

    const bool palmMuteAvailable = mgs::getFamily(instrIndex) != mgs::Family::Electronique;
    palmMuteLabel.setEnabled(palmMuteAvailable);
    palmMuteDial.setEnabled(palmMuteAvailable);
    palmMuteLabel.setAlpha(palmMuteAvailable ? 1.0f : 0.55f);
    palmMuteDial.setAlpha(palmMuteAvailable ? 1.0f : 0.55f);
    palmMuteDial.setTooltip(palmMuteAvailable
                                ? "Palm mute amount"
                                : "Palm mute is reserved for the core guitar lane");

    if (presetFilterChanged)
        refreshPresetList();
}

// =============================================================================
// Tooltip mode cycling
// =============================================================================
void GuitarSynthAudioProcessorEditor::cycleTooltipMode()
{
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
            tooltipModeBtn.setButtonText(tooltipMode == TooltipMode::Off ? "TIP: OFF"
                                                                         : tooltipMode == TooltipMode::Short ? "TIP: SHORT"
                                                                                                              : "TIP: NOVICE");
            break;
    }

    tooltipWindow.setVisible(tooltipMode != TooltipMode::Off);
    applyTooltips();
}

// =============================================================================
// Apply tooltips according to current mode
// =============================================================================
void GuitarSynthAudioProcessorEditor::applyTooltips()
{
    const char** src = nullptr;
    if (tooltipMode == TooltipMode::Short)  src = kTooltipsShort;
    if (tooltipMode == TooltipMode::Novice) src = kTooltipsNovice;

    const auto& profile = envProfileForInstrument(cachedInstrIdx >= 0 ? cachedInstrIdx : selectedInstrFromParam());
    const auto sharedTooltipMode = tooltipMode == TooltipMode::Short ? synthui::TooltipMode::Short
                                 : tooltipMode == TooltipMode::Novice ? synthui::TooltipMode::Novice
                                                                      : synthui::TooltipMode::Off;
    synthui::applyTooltipProfile(profile, envDials, sharedTooltipMode);

    int idx = kEnvN;

    lfoRateDial .setTooltip(src ? juce::String(src[idx])     : juce::String()); ++idx;
    lfoDepthDial.setTooltip(src ? juce::String(src[idx])     : juce::String()); ++idx;

    for (int i = 0; i < kMacroTotal; ++i, ++idx)
        macroDials[static_cast<std::size_t>(i)].setTooltip(src ? juce::String(src[idx]) : juce::String());

    for (int i = 0; i < kFxN; ++i, ++idx)
        fxDials[static_cast<std::size_t>(i)].setTooltip(src ? juce::String(src[idx]) : juce::String());

    gainDial.setTooltip(src ? juce::String(src[idx]) : juce::String());
}
