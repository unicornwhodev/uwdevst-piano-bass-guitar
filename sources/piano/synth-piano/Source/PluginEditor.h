#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

#include "PluginProcessor.h"
#include "../../Shared/SynthCommon.h"

// =============================================================================
// Piano Synth editor — inherits CommonSynthEditor from Shared/SynthCommon.h
// =============================================================================
class PianoSynthAudioProcessorEditor : public CommonSynthEditor,
                                       private juce::Timer
{
public:
    explicit PianoSynthAudioProcessorEditor(PianoSynthAudioProcessor&);

    // --- CommonSynthEditor pure virtuals ---
    juce::String            pluginNamespace()  const override { return {}; }
    juce::String            pluginTitle()      const override { return "UWdeVST Piano"; }
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
    bool                    hostShouldIncludeFactoryPreset(int presetIndex) const override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

#if defined(UWDEVST_PIANO_TEST_BUILD)
    struct LayoutSnapshot
    {
        bool compact = false;
        juce::Rectangle<int> editorBounds;
        juce::Rectangle<int> headerBounds;
        juce::Rectangle<int> selectorPanelBounds;
        juce::Rectangle<int> modelSelectorBounds;
        juce::Rectangle<int> rightPanelTabBounds;
        juce::Rectangle<int> rightPanelBounds;
        juce::Rectangle<int> currentPresetSummaryBounds;
        juce::Rectangle<int> currentPresetMetaBounds;
        juce::Rectangle<int> quickTierBounds;
        juce::Rectangle<int> quickRoleBounds;
        juce::Rectangle<int> quickShortcutBounds;
        juce::Rectangle<int> quickReferenceBounds;
        juce::Rectangle<int> quickMixBounds;
        juce::Rectangle<int> quickCinematicBounds;
        juce::Rectangle<int> presetSearchBounds;
        juce::Rectangle<int> prevPresetBounds;
        juce::Rectangle<int> presetBoxBounds;
        juce::Rectangle<int> nextPresetBounds;
        juce::Rectangle<int> savePresetBounds;
        juce::Rectangle<int> saveAsPresetBounds;
        juce::Rectangle<int> deletePresetBounds;
        juce::Rectangle<int> importPresetBounds;
        juce::Rectangle<int> randBounds;
        juce::Rectangle<int> midiLearnBounds;
        juce::Rectangle<int> undoBounds;
        juce::Rectangle<int> redoBounds;
        juce::Rectangle<int> monoBounds;
        juce::Rectangle<int> tooltipModeBounds;
        juce::Rectangle<int> voiceCountBounds;
        juce::Rectangle<int> gainBounds;
        juce::Rectangle<int> midiCCBounds;
        juce::Rectangle<int> keyboardBounds;
        juce::Rectangle<int> octaveDownBounds;
        juce::Rectangle<int> octaveUpBounds;
        juce::Rectangle<int> envVisualBounds;
        juce::Rectangle<int> lfoVisualBounds;
        juce::Rectangle<int> lowPassBounds;
        juce::Rectangle<int> velocitySelectorBounds;
        juce::Rectangle<int> lfoDestinationBounds;
        juce::Rectangle<int> delayDivisionBounds;
        juce::Rectangle<int> arpModeBounds;
        juce::Rectangle<int> arpRateBounds;
        juce::Rectangle<int> arpBlockBounds;
        juce::Rectangle<int> modMatrixToggleBounds;
        juce::Rectangle<int> modMatrixViewportBounds;
        juce::Rectangle<int> fxLockBounds;
        bool fxLockVisible = false;
        bool modMatrixToggleVisible = false;
        bool currentPresetSummaryVisible = false;
        bool quickTierVisible = false;
        bool quickRoleVisible = false;
        bool quickShortcutVisible = false;
        bool midiCCVisible = false;
        bool arpBlockVisible = false;
        bool lfoChorusItemEnabled = false;
        bool reverbShapeControlsEnabled = true;
        bool tremoloSyncVisible = false;
        bool aftertouchStatusVisible = false;
        juce::String headerLayoutModeName;
        juce::String currentPresetSummaryText;
        juce::String currentPresetMetaText;
        juce::String lfoChorusItemText;
        juce::String fxUnavailableText;
        juce::String fxDetailTitleText;
        juce::String delayDivisionText;
        juce::String aftertouchStatusText;
        juce::String modAftertouchSourceText;
        juce::String lowPassTextFor10000;
        juce::String lowPassTextFor9050;
        std::array<juce::String, 15> envLabelText {};
        std::array<bool, 15> envControlEnabled {};
    };

    LayoutSnapshot captureLayoutSnapshotForTests() const;
    void setRightPanelSectionForTests(int sectionIndex);
    void setFxTabForTests(int tabIndex);
#endif

private:
    using APVTS          = juce::AudioProcessorValueTreeState;
    using SliderAttach   = APVTS::SliderAttachment;
    using ComboBoxAttach = APVTS::ComboBoxAttachment;
    using BtnAttach      = APVTS::ButtonAttachment;

    struct CtrlDef { const char* label; const char* suffix; };
    struct FxDef   { const char* label; const char* paramId; };

    void timerCallback() override;
    void rebuildPianoAttachments();
    void rebuildModelSelectorForFamily(int familyIndex, int preferredPiano = -1);
    void syncSelectionUiFromPiano();
    void syncCurrentPresetSummary();
    void syncQuickBrowserState();
    void syncFxAvailability();
    void syncFxRackState();
    void syncConditionalUiState();
    void syncPerformanceAffordances();
    void syncInstrumentControlAvailability();
    void syncMotionRoutingAvailability();
    void syncFxContextMessaging();
    void syncReverbDetailAvailability();
    void applyEnvValueFormatters();
    void applyStaticValueFormatters();
    void applyQuickPresetShortcut(int tierId, int roleId);
    void resetToPlaySectionAfterPresetChange();
    void switchEffectTab(int tabIndex);
    void switchRightPanelSection(int sectionIndex);
    void applyPianoTheme(int pianoIndex);
    int  selectedPianoFromParam() const;

    static juce::Colour familyColour(int familyIndex);
    static juce::Colour pianoCatColour(int pianoIndex);

    struct VisualLayoutSnapshot
    {
        bool compact = false;
        bool ultraCompact = false;
        bool roomy = false;
        int headerH = 0;
        juce::String headerLayoutModeName;
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

    PianoSynthAudioProcessor& proc;

    static constexpr int kEnvN         = 15;
    static constexpr int kFxN          = 31;
    static constexpr int kMacroTotal   = 4;
    static constexpr int kMacroVisible = 4;
    static constexpr int kFxPerTab     = 7;
    static constexpr int kFxTabs       = 8;
    static constexpr int kDelayFxTab   = 6;
    static constexpr int kRightPanelSections = 4;

    std::array<SynthFamilyTab,  mps::kNumFamilies> familyTabs;
    std::array<SynthPresetCard, mps::kNumPianos>   presetCards;

    juce::ComboBox pianoSelector;
    std::unique_ptr<ComboBoxAttach> selPianoAtt;

    std::array<juce::Slider, kEnvN> envDials;
    std::array<juce::Label,  kEnvN> envLabels;
    std::array<std::unique_ptr<SliderAttach>, kEnvN> envAttach;
    EnvelopeDisplay envVisual;
    LfoModulationDisplay lfoVisual;
    juce::Slider lfoRateDial, lfoDepthDial;
    juce::ComboBox lfoWaveSelector;
    juce::ComboBox lfoDestinationSelector;
    juce::Label lfoDestinationLabel;
    juce::ToggleButton lfoAdvancedButton;
    std::unique_ptr<SliderAttach> lfoRateAtt, lfoDepthAtt;
    std::unique_ptr<ComboBoxAttach> lfoWaveAtt, lfoDestinationAtt;

    std::array<juce::Slider, kMacroTotal> macroDials;
    std::array<juce::Label,  kMacroTotal> macroLbls;
    std::array<std::unique_ptr<SliderAttach>, kMacroTotal> macroAtt;
    std::array<SynthEffectTab, kRightPanelSections> rightPanelTabs;

    std::array<juce::Slider, kFxN> fxDials;
    std::array<juce::Label,  kFxN> fxLbls;
    std::array<std::unique_ptr<SliderAttach>, kFxN> fxAtt;
    std::array<SynthFxRackItem, kFxTabs> fxRackItems;
    std::array<juce::ToggleButton, kFxTabs> fxBypassBtns;
    std::array<std::unique_ptr<BtnAttach>, kFxTabs> fxBypassAtts;
    juce::Label fxDetailTitle;
    juce::Label fxUnavailableLbl;
    juce::ComboBox reverbTypeSelector;
    juce::Label reverbTypeLabel;
    std::unique_ptr<ComboBoxAttach> reverbTypeAtt;
    juce::ToggleButton delaySyncButton;
    juce::ComboBox delayNoteDivSelector;
    juce::Label delayNoteDivLabel;
    std::unique_ptr<BtnAttach> delaySyncAtt;
    std::unique_ptr<ComboBoxAttach> delayNoteDivAtt;
    juce::ToggleButton fxLockButton;
    std::unique_ptr<BtnAttach> fxLockAtt;

    // ── Arpégiateur ────────────────────────────────────────────────────
    juce::ToggleButton arpHoldButton;
    juce::ComboBox     arpModeSelector;
    juce::ComboBox     arpRateSelector;
    juce::Slider       arpOctavesDial;
    juce::Slider       arpGateDial;
    juce::Label        arpModeLabel, arpRateLabel, arpOctavesLabel, arpGateLabel;
    std::unique_ptr<BtnAttach>      arpHoldAtt;
    std::unique_ptr<ComboBoxAttach> arpModeAtt, arpRateAtt;
    std::unique_ptr<SliderAttach>   arpOctavesAtt, arpGateAtt;

    // ── Preset filter ──────────────────────────────────────────────────
    juce::Label currentPresetSummaryLabel;
    juce::Label currentPresetMetaLabel;
    juce::ComboBox quickTierSelector;
    juce::ComboBox quickRoleSelector;
    juce::TextButton quickReferenceBtn;
    juce::TextButton quickMixBtn;
    juce::TextButton quickCinematicBtn;
    juce::ComboBox presetFamilyFilter;   // Concert / Vintage / Electric / All
    juce::ComboBox presetRoleFilter;     // All / solo / layer / ambient / cinematic / ...
    juce::Label presetFilterLabel;

    // ── MIDI CC page indicator (FLkey Mini) ────────────────────────────
    juce::Label midiCCPageLabel;
    int cachedMidiCCPage = -1;

    // ── MIDI Learn ─────────────────────────────────────────────────────
    juce::TextButton midiLearnButton;
    bool midiLearnPanelVisible = false;

    struct MidiLearnPanel : public juce::Component
    {
        MidiLearnPanel() { setInterceptsMouseClicks(true, true); }
        // Filled by editor with current mappings
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
    juce::Label    velocityCurveLabel;
    juce::ComboBox velocityCurveSelector;
    juce::Label    pitchBendRangeLabel;
    juce::Slider   pitchBendRangeDial;
    std::unique_ptr<ComboBoxAttach> velocityCurveAtt;
    std::unique_ptr<SliderAttach>   pitchBendRangeAtt;
    juce::ToggleButton monoModeButton;
    std::unique_ptr<BtnAttach> monoModeAtt;
    juce::ToggleButton tremoloSyncButton;
    std::unique_ptr<BtnAttach> tremoloSyncAtt;
    juce::Label aftertouchStatusLabel;
    juce::TextButton   randButton;
    juce::TextButton   undoButton, redoButton;
    juce::Label        voiceCountLabel;

    // ── Tooltip mode system ────────────────────────────────────────────
    enum class TooltipMode { Off, Short, Novice };
    TooltipMode tooltipMode = TooltipMode::Short;

    juce::TooltipWindow tooltipWindow { this, 600 };
    juce::TextButton    tooltipModeBtn;

    void cycleTooltipMode();
    void applyTooltips();
    juce::String cachedPresetSummaryKey;

    static constexpr int kTooltipCount = kEnvN + 2 + kMacroTotal + kFxN + 1; // env+lfo+macro+fx+gain
    static const char* kTooltipsShort[kTooltipCount];
    static const char* kTooltipsNovice[kTooltipCount];

    // ── Mod Matrix Panel (visible when advancedMotionVisible) ──────────
    struct ModMatrixRow {
        juce::ComboBox srcCombo;
        juce::ComboBox dstCombo;
        juce::Slider   amtSlider;
    };
    std::array<ModMatrixRow, 8> modRows;
    juce::Label modMatrixTitle;

    void syncModMatrixUi();
    void flushModRow(int rowIndex);

    int activeFamilyIndex    = 0;
    int activeFxTab           = 0;
    int activeRightPanelSection = 0;
    int cachedPianoIdx        = -1;
    int cachedAttachPianoIdx  = -1;
    int cachedFxAvailabilityPiano = -1;
    int cachedFxAvailabilityMask = -1;
    int cachedFxLayoutPiano = -1;
    bool advancedMotionVisible = false;

    static constexpr int kFxTabMap[kFxTabs][kFxPerTab] = {
        {  0,  1,  2,  3,  4, -1, -1 },
        {  5,  6, -1, -1, -1, -1, -1 },
        {  7,  8,  9, -1, -1, -1, -1 },
        { 10, 11, 12, 13, 14, 15, -1 },
        { 16, 17, 18, 19, 20, 21, 22 },
        { 23, 24, 25, -1, -1, -1, -1 },
        { 26, 27, 28, -1, -1, -1, -1 },
        { 29, 30, -1, -1, -1, -1, -1 }
    };

    static const std::array<CtrlDef, kEnvN>       kEnvCtrls;
    static const std::array<FxDef,   kMacroTotal> kMacroCtrls;
    static const std::array<FxDef,   kFxN>        kFxCtrls;

    static const char* kFxRackSummaries[kFxTabs];
    static const char* kFxBypassParamIds[kFxTabs];
    static const char* kFxTabNames[kFxTabs];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoSynthAudioProcessorEditor)
};
