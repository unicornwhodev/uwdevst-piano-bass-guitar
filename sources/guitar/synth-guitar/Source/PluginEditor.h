#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

#include "PluginProcessor.h"
#include "../../Shared/SynthCommon.h"

// =============================================================================
// Guitar Synth editor — inherits CommonSynthEditor from Shared/SynthCommon.h
// =============================================================================
class GuitarSynthAudioProcessorEditor : public CommonSynthEditor,
                                        private juce::Timer
{
public:
    explicit GuitarSynthAudioProcessorEditor(GuitarSynthAudioProcessor&);

    // --- CommonSynthEditor pure virtuals ---
    juce::String            pluginNamespace()  const override { return {}; }
    juce::String            pluginTitle()      const override { return "UWdeVST Guitar"; }
    juce::StringArray       hostGetFactoryNames()              override;
    juce::Array<juce::File> hostScanUserPresets()              override;
    bool                    hostIsUserPreset()                 override;
    juce::File              hostCurrentUserFile()              override;
    int                     hostCurrentFactoryIdx()            override;
    void                    hostApplyFactory(int idx)          override;
    void                    hostLoadUser(const juce::File& f)  override;
    bool                    hostSaveUser(const juce::String& n)override;
    void                    hostUpdateUser(const juce::File& f)override;
    void                    hostSaveFactory(int idx)           override;
    void                    hostDeleteUser(const juce::File& f)override;
    juce::File              hostGetUserPresetsDir()            override;
    juce::File              hostGetUserPresetsDirForIndex(int instrumentIndex) override;
    juce::String            hostPresetInstrumentAttr() const   override;
    juce::String            hostFormatFactoryPresetLabel(int presetIndex,
                                                         const juce::String& displayName) const override;
    juce::String            hostFactoryPresetSearchText(int presetIndex,
                                                        const juce::String& displayName) const override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool hostShouldIncludeFactoryPreset(int presetIndex) const override;
    void refreshUiForTesting();

#if defined(UWDEVST_GUITAR_TEST_BUILD)
    struct LayoutSnapshot
    {
        bool compact = false;
        juce::Rectangle<int> editorBounds;
        juce::Rectangle<int> headerBounds;
        juce::Rectangle<int> selectorPanelBounds;
        juce::Rectangle<int> presetPerformanceRowBounds;
        juce::Rectangle<int> modelSelectorBounds;
        juce::Rectangle<int> presetSearchBounds;
        juce::Rectangle<int> presetBoxBounds;
        juce::Rectangle<int> nextPresetBounds;
        juce::Rectangle<int> presetTierFilterBounds;
        juce::Rectangle<int> presetUseFilterBounds;
        juce::Rectangle<int> presetRoleFilterBounds;
        juce::Rectangle<int> qualitySelectorBounds;
        juce::Rectangle<int> outputSelectorBounds;
        juce::Rectangle<int> midiCCPageLabelBounds;
        juce::Rectangle<int> voiceCountLabelBounds;
        juce::Rectangle<int> gainBounds;
        juce::Rectangle<int> velocityCurveSelectorBounds;
        juce::Rectangle<int> playModeSelectorBounds;
        juce::Rectangle<int> palmMuteBounds;
        juce::Rectangle<int> pitchBendRangeLabelBounds;
        juce::Rectangle<int> pitchBendRangeBounds;
        juce::Rectangle<int> keyboardBounds;
        juce::Rectangle<int> octaveDownBounds;
        juce::Rectangle<int> octaveUpBounds;
        juce::Rectangle<int> rightPanelTabsBounds;
        juce::Rectangle<int> fxLockBounds;
        juce::Rectangle<int> modMatrixToggleBounds;
        juce::Rectangle<int> modMatrixContentBounds;
        juce::String midiCCPageText;
        juce::String velocityCurveText;
        juce::String palmMuteText;
        juce::String pitchBendRangeLabelText;
        juce::String airCutText;
        bool fxLockVisible = false;
    };

    LayoutSnapshot captureLayoutSnapshotForTests() const;
    void setRightPanelSectionForTests(int sectionIndex);
#endif

private:
    using APVTS          = juce::AudioProcessorValueTreeState;
    using SliderAttach   = APVTS::SliderAttachment;
    using ComboBoxAttach = APVTS::ComboBoxAttachment;

    struct CtrlDef { const char* label; const char* suffix; };
    struct FxDef   { const char* label; const char* paramId; };
    struct InstrumentPalette
    {
        juce::Colour accent;
        juce::Colour panelBase;
        juce::Colour panelCavity;
        juce::Colour panelHeader;
        juce::Colour controlBg;
        juce::Colour knobRing;
        juce::Colour knobGlow;
        juce::Colour textHi;
        juce::Colour textMuted;
    };

    void timerCallback() override;
    void rebuildInstrAttachments();
    void rebuildModelSelectorForFamily(int familyIndex, int preferredInstr = -1);
    void syncSelectionUiFromInstr();
    void syncFxAvailability();
    void syncFxRackState();
    void switchEffectTab(int tabIndex);
    void switchRightPanelSection(int sectionIndex);
    void updateMidiCCPageLabel();
    void applyInstrumentTheme(int instrIndex);
    void applyValueFormatters();
    bool isFxTabAvailable(int tabIndex, int instrIndex) const;
    int  firstAvailableFxTab(int instrIndex) const;
    int  selectedInstrFromParam() const;
    void syncAdvancedModUi();

    static juce::Colour familyColour(int familyIndex);
    static juce::Colour instrCatColour(int instrIndex);
    static InstrumentPalette paletteForInstrument(int instrIndex);

    struct VisualLayoutSnapshot
    {
        bool compact = false;
        bool roomy = false;
        int headerH = 0;
        int contentX = 0;
        int contentW = 0;
        int selectorY = 0;
        int selectorH = 0;
        int bodyY = 0;
        int bodyH = 0;
        int kbY = 0;
        int kbH = 0;
        int col1X = 0;
        int col2X = 0;
        int col3X = 0;
        int colW = 0;
        HeaderZones headerZones;
        juce::Rectangle<int> headerBounds;
        juce::Rectangle<int> selectorPanelBounds;
    };

    VisualLayoutSnapshot computeVisualLayoutSnapshot(int width, int height) const;

    GuitarSynthAudioProcessor& proc;

    static constexpr int kEnvN         = 14;
    static constexpr int kFxN          = 32;
    static constexpr int kMacroTotal   = 4;
    static constexpr int kMacroVisible = 4;
    static constexpr int kFxPerTab     = 7;
    static constexpr int kFxTabs       = 9;
    static constexpr int kRightPanelSections = 3;

    std::array<SynthFamilyTab,  mgs::kNumFamilies> familyTabs;

    juce::ComboBox instrSelector;
    std::unique_ptr<ComboBoxAttach> selInstrAtt;

    std::array<juce::Slider, kEnvN> envDials;
    std::array<juce::Label,  kEnvN> envLabels;
    std::array<std::unique_ptr<SliderAttach>, kEnvN> envAttach;
    EnvelopeDisplay envVisual;
    LfoModulationDisplay lfoVisual;
    juce::Slider lfoRateDial, lfoDepthDial;
    juce::ComboBox lfoWaveSelector;
    std::unique_ptr<SliderAttach> lfoRateAtt, lfoDepthAtt;
    std::unique_ptr<ComboBoxAttach> lfoWaveAtt;

    juce::ComboBox qualitySelector;
    juce::ComboBox outputSelector;
    juce::ToggleButton fxLockButton;
    std::unique_ptr<ComboBoxAttach> qualityAtt;
    std::unique_ptr<ComboBoxAttach> outputAtt;

    std::array<juce::Slider, kMacroTotal> macroDials;
    std::array<juce::Label,  kMacroTotal> macroLbls;
    std::array<std::unique_ptr<SliderAttach>, kMacroTotal> macroAtt;

    std::array<juce::Slider, kFxN> fxDials;
    std::array<juce::Label,  kFxN> fxLbls;
    std::array<std::unique_ptr<SliderAttach>, kFxN> fxAtt;
    std::array<SynthEffectTab, kRightPanelSections> rightPanelTabs;
    std::array<SynthFxRackItem, kFxTabs> fxRackItems;
    using BtnAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<BtnAttach> fxLockAtt;
    std::array<juce::ToggleButton, kFxTabs> fxBypassBtns;
    std::array<std::unique_ptr<BtnAttach>, kFxTabs> fxBypassAtts;
    juce::Label fxDetailTitle;
    juce::ComboBox delaySyncSelector;
    juce::ComboBox delayDivisionSelector;
    juce::Label delaySyncLabel;
    juce::Label delayDivisionLabel;
    std::unique_ptr<ComboBoxAttach> delaySyncAtt;
    std::unique_ptr<ComboBoxAttach> delayDivisionAtt;

    juce::Label    reverbTypeLabel;
    juce::ComboBox reverbTypeSelector;
    std::unique_ptr<ComboBoxAttach> reverbTypeAtt;

    // ── Undo / Redo ────────────────────────────────────────────────────
    juce::TextButton undoButton, redoButton;

    // ── Preset filter ──────────────────────────────────────────────────
    juce::ComboBox presetTierFilter;
    juce::ComboBox presetUseFilter;
    juce::ComboBox presetRoleFilter;
    juce::Label    presetFilterLabel;

    // ── MIDI Learn ─────────────────────────────────────────────────────
    juce::TextButton midiLearnButton;
    bool midiLearnPanelVisible = false;

    struct MidiLearnPanel : public juce::Component
    {
        MidiLearnPanel() { setInterceptsMouseClicks(true, true); }
        std::vector<std::pair<int, juce::String>> mappings;
        std::function<void(int)>  onClearMapping;
        std::function<void()>     onClearAll;
        void paint(juce::Graphics& g) override;
        void resized() override;
        juce::TextButton clearAllBtn { "CLEAR ALL" };
        std::vector<std::unique_ptr<juce::TextButton>> clearBtns;
    };
    MidiLearnPanel midiLearnPanel;
    void showMidiLearnPanel(bool visible);
    void refreshMidiLearnPanel();

    // ── Performance controls ────────────────────────────────────────────
    juce::TextButton   randButton;
    juce::Label        voiceCountLabel;
    std::atomic<int>   cachedVoiceCount { -1 };

    // ── Arpeggiator controls ───────────────────────────────────────────
    juce::ToggleButton arpEnableButton;
    juce::ComboBox     arpModeSelector;
    juce::ComboBox     arpRateSelector;
    juce::Slider       arpOctavesDial;
    juce::Slider       arpGateDial;
    juce::ToggleButton arpHoldButton;
    juce::Label        arpModeLabel, arpRateLabel, arpOctavesLabel, arpGateLabel;
    std::unique_ptr<BtnAttach>      arpEnableAtt;
    std::unique_ptr<ComboBoxAttach> arpModeAtt;
    std::unique_ptr<ComboBoxAttach> arpRateAtt;
    std::unique_ptr<SliderAttach>   arpOctavesAtt;
    std::unique_ptr<SliderAttach>   arpGateAtt;
    std::unique_ptr<BtnAttach>      arpHoldAtt;

    // ── Riff Generator (Phase 4) ──────────────────────────────────────
    juce::ToggleButton riffEnableButton;  // reuse arp enable to toggle
    juce::ComboBox     riffStyleSelector;
    juce::Slider       riffIntensityDial;
    juce::Slider       riffGateDial;
    juce::ToggleButton riffHoldButton;
    juce::Label        riffStyleLabel, riffIntensityLabel, riffGateLabel;
    std::unique_ptr<ComboBoxAttach> riffStyleAtt;
    std::unique_ptr<SliderAttach>   riffIntensityAtt;
    std::unique_ptr<SliderAttach>   riffGateAtt;
    std::unique_ptr<BtnAttach>      riffHoldAtt;

    // ── Velocity Curve / Play Mode / Palm Mute / Pitch Bend Range ────
    juce::Label    velocityCurveLabel;
    juce::ComboBox velocityCurveSelector;
    std::unique_ptr<ComboBoxAttach> velocityCurveAtt;

    juce::Label    playModeLabel;
    juce::ComboBox playModeSelector;
    std::unique_ptr<ComboBoxAttach> playModeAtt;

    juce::Label  palmMuteLabel;
    juce::Slider palmMuteDial;
    std::unique_ptr<SliderAttach> palmMuteAtt;

    juce::Label  pitchBendRangeLabel;
    juce::Slider pitchBendRangeDial;
    std::unique_ptr<SliderAttach> pitchBendRangeAtt;

    // ── MIDI CC page indicator (FLkey Mini) ────────────────────────────
    juce::Label midiCCPageLabel;
    int cachedMidiCCPage = -1;

    // ── Tooltip mode system ────────────────────────────────────────────
    enum class TooltipMode { Off, Short, Novice };
    TooltipMode tooltipMode = TooltipMode::Short;

    juce::TooltipWindow tooltipWindow { this, 600 };
    juce::TextButton    tooltipModeBtn;

    static constexpr int kModSlots = modmatrix::ModulationMatrix::getNumSlots();
    std::array<juce::Label, kModSlots> modSlotLabels;
    std::array<juce::ComboBox, kModSlots> modSourceBoxes;
    std::array<juce::ComboBox, kModSlots> modDestBoxes;
    std::array<juce::Slider, kModSlots> modAmountSliders;
    juce::Label modLfo2RateLabel;
    juce::Label modLfo2WaveLabel;
    juce::Slider modLfo2RateDial;
    juce::ComboBox modLfo2WaveSelector;

    void cycleTooltipMode();
    void applyTooltips();

    static constexpr int kTooltipCount = kEnvN + 2 + kMacroTotal + kFxN + 1; // env+lfo+macro+fx+gain
    static const char* kTooltipsShort[kTooltipCount];
    static const char* kTooltipsNovice[kTooltipCount];

    int activeFamilyIndex = 0;
    int activeFxTab       = 0;
    int activeRightPanelSection = 0;
    int cachedInstrIdx    = -1;
    InstrumentPalette activePalette {};

    static constexpr int kFxTabMap[kFxTabs][kFxPerTab] = {
        {  0,  1,  2,  3, -1, -1, -1 },
        {  4,  5, -1, -1, -1, -1, -1 },
        {  6,  7,  8, -1, -1, -1, -1 },
        {  9, 10, 11, 12, 13, 14, -1 },
        { 15, 16, 17, 18, 19, 20, 21 },
        { 22, 23, 24, 25, -1, -1, -1 },
        { 26, 27, 28, -1, -1, -1, -1 },
        { 29, 30, -1, -1, -1, -1, -1 },
        { 31, -1, -1, -1, -1, -1, -1 }
    };

    static const std::array<CtrlDef, kEnvN>       kEnvCtrls;
    static const std::array<FxDef,   kMacroTotal> kMacroCtrls;
    static const std::array<FxDef,   kFxN>        kFxCtrls;

    static const char* kFxRackSummaries[kFxTabs];
    static const char* kFxBypassParamIds[kFxTabs];
    static const char* kFxTabNames[kFxTabs];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarSynthAudioProcessorEditor)
};
