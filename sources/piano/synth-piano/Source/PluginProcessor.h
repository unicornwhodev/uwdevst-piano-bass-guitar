#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Engine/PianoVoice.h"
#include "Engine/FactoryPresets.h"
#include "Engine/FxProcessors.h"
#include "../../Shared/PitchBendState.h"
#include "../../Shared/ModulationMatrix.h"

class PianoSynthAudioProcessor : public juce::AudioProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener,
                                  private juce::AsyncUpdater
{
public:
    static constexpr int kNumAuxOutputs = 4;
    static constexpr int kMaxVoices     = 32;
    static constexpr const char* kProcessorName = "UWdeVST_Piano";

    PianoSynthAudioProcessor();
    ~PianoSynthAudioProcessor() override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::String makePianoParamId(int pianoIndex, const juce::String& suffix);
    static auto createBusLayout() -> BusesProperties;
    static bool isLfoDestinationAvailableForPiano(int pianoIndex, mps::LfoDestination destination) noexcept;
    static mps::LfoDestination sanitizeLfoDestinationForPiano(int pianoIndex,
                                                              mps::LfoDestination destination) noexcept;
    static bool isInstrumentControlAvailableForPiano(int pianoIndex, const juce::String& suffix) noexcept;
    static bool usesTempoSyncedTremolo(int pianoIndex) noexcept;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
#if UWDEVST_PIANO_TEST_BUILD
    void testPublishPedalNoiseSeedForP0(std::uint64_t seed) noexcept { publishPedalNoiseRandomSeed(seed); }
#endif

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
#if defined(JucePlugin_Name)
        return JucePlugin_Name;
#else
        return kProcessorName;
#endif
    }
    bool acceptsMidi()    const override { return true; }
    bool producesMidi()   const override { return false; }
    bool isMidiEffect()   const override { return false; }
    double getTailLengthSeconds() const override;

    int  getNumPrograms() override;
    int  getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return parameters; }
    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }
    juce::UndoManager& getUndoManager() noexcept { return undoManager; }

    // ── FLkey Mini CC page system ──────────────────────────────────────
    static constexpr int kNumCCPages = 7;
    int  getMidiCCPage() const noexcept { return midiCCPage.load(std::memory_order_relaxed); }
    static const char* getCCPageName(int page) noexcept;

    // ── Arpégiateur ─────────────────────────────────────────────────────
    // (parameters: arp_enable, arp_mode, arp_rate, arp_octaves, arp_gate, arp_hold)

    // ── MIDI Learn ─────────────────────────────────────────────────────
    void midiLearnArm(const juce::String& paramId);
    void midiLearnClear(int ccNumber);
    void midiLearnClearAll();
    bool isMidiLearnActive() const noexcept { return midiLearnArmed.load(std::memory_order_relaxed); }
    juce::String getMidiLearnArmedParam() const { return midiLearnArmedParamId; }
    // Returns a snapshot of the current cc→paramId map for UI display (call on message thread)
    std::vector<std::pair<int, juce::String>> getMidiLearnMappings() const;
    // Serialisation helpers called from getState/setStateInformation
    void saveMidiLearnToXml(juce::XmlElement& xml) const;
    void loadMidiLearnFromXml(const juce::XmlElement& xml);
#if UWDEVST_PIANO_TEST_BUILD
    void flushPendingAsyncUpdatesForTests();
#endif

    juce::StringArray getFactoryPresetNames() const;
    int  getCurrentFactoryPresetIndex() const noexcept;
    const std::array<std::vector<mps::InstrumentPreset>, mps::kNumPianos>&
        getFactoryPresetBanksDirect() const noexcept { return factoryPresetBanks; }
    void applyFactoryPreset(int presetIndex);
    bool saveFactoryPreset(int presetIndex);
    void applyOfflinePreset(int pianoIndex, const mps::InstrumentPreset& preset);
    static bool importPresetXmlFile(const juce::File& file,
                                    int fallbackPianoIndex,
                                    mps::InstrumentPreset& outPreset,
                                    int& outPianoIndex,
                                    juce::String* message = nullptr);

    // User preset management
    static juce::File getLibraryRootDirectory();
    static juce::File getUserPresetsDirectory(int pianoIndex);
    static juce::File getFactoryOverridesDirectory();
    juce::Array<juce::File> scanUserPresets() const;
    bool saveUserPreset(const juce::String& name);
    bool updateUserPreset(const juce::File& file);
    bool deleteUserPreset(const juce::File& file);
    bool loadUserPreset(const juce::File& file);
    bool writePresetManifest(const juce::File& presetFile,
                             const juce::String& presetName,
                             int pianoIndex,
                             const juce::String& sourceModel = {}) const;
    bool isCurrentPresetUser() const noexcept;
    juce::File getCurrentUserPresetFile() const noexcept;

    int  getSelectedPianoIndex() const;
    int  getActiveVoiceCount() const noexcept { return activeVoiceCountAtomic.load(std::memory_order_relaxed); }
    void randomizePreset(float amount = 0.15f);

    modmatrix::ModulationMatrix&       getModulationMatrix()       noexcept { return modulationMatrix; }
    const modmatrix::ModulationMatrix& getModulationMatrix() const noexcept { return modulationMatrix; }

private:
    struct VoiceSlot
    {
        std::array<std::unique_ptr<mps::PianoVoice>, mps::kNumPianos> voiceBank;
        mps::PianoVoice* active = nullptr;
        mps::PianoVoice* dying  = nullptr;   // cross-piano steal: fading-out voice
        int dyingBus = 0;
        float activeVelocity = 0.5f;
        float dyingVelocity = 0.5f;
        int renderBus = 0;
        int previousRenderBus = 0;
        int midiNote    = -1;
        int pianoIndex  = 0;
        int midiChannel = 1;
        bool keyDown = false;
        bool deferredNoteOff = false;
        bool sostenutoCaptured = false;
        int attackBlendRemaining = 0;
        int routeFadeRemaining = 0;
        std::uint64_t noteOnOrder = 0;
    };

    float getParamValue(const juce::String& paramId) const;
    float sanitizeParameterValue(const juce::String& paramId, float value, float fallback, int* warningCount = nullptr) const;
    void  setParamValue(const juce::String& paramId, float value);
    void  setParamValueInternal(const juce::String& paramId, float value, bool notifyHost);
    void  sanitizeAllParameterValues();
    void  sanitizePianoDependentState(bool notifyHost, int pianoIndexOverride = -1);
    mps::PianoSettings snapshotPianoSettings(int pianoIndex) const;
    void applyPerformanceMacros(int pianoIndex, mps::PianoSettings& s) const;
    mps::PresetPerformanceState snapshotPerformanceState() const;
    void applyPerformanceState(const mps::PresetPerformanceState& state);
    int  findFreeVoice() const;
    int  findFreeVoiceForNote(int midiNote, int pianoIndex) const;
    void clearVoice(VoiceSlot& slot);
    void handleSustainPedal(int midiChannel, float damperValue);
    void handleSostenutoPedal(int midiChannel, bool on);
    void handleUnaCordaPedal(int midiChannel, bool on);
    void handleMidiCC(int ccNumber, int ccValue, int pianoIndex);
    void releaseVoices(int midiChannel, bool immediate);
    void panicAllVoices();
    void triggerNoteOn(int pianoIndex, int midiChannel, int midiNote, float velocity);
    void triggerNoteOff(int midiChannel, int midiNote);
    void refreshDeterministicRenderMode();
    void resetPedalNoiseRandomStream();
    void publishPedalNoiseRandomSeed(std::uint64_t seed) noexcept;
    void consumePendingPedalNoiseRandomSeed() noexcept;
    std::int64_t makeRandomSeed() const noexcept;
    std::int64_t makeDeterministicVoiceSeed(int slotIndex, int pianoIndex, int midiChannel,
                                            int midiNote, std::uint64_t noteOnOrder) const noexcept;
    void updateGlobalEffectParameters();
    void processGlobalTransient(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalSaturator(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalCompressor(juce::AudioBuffer<float>& mainBuffer);
    void applyGlobalLfo(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalEQ(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalChorus(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalDelay(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalReverb(juce::AudioBuffer<float>& mainBuffer);
    void processOutputLimiter(juce::AudioBuffer<float>& mainBuffer);
    void renderPedalNoise(juce::AudioBuffer<float>& mainBuffer);
    bool isFxAvailableForCurrentPiano(mps::GlobalFxSlot slot) const;
    void archiveLegacyPresetLibraryIfNeeded();
    void loadFactoryOverrides();
    void applyPianoPresetSettings(int pianoIndex, const mps::PianoSettings& s);
    void applyInstrumentPreset(int pianoIndex, const mps::InstrumentPreset& preset, bool preserveFxLock);
    mps::InstrumentPreset captureCurrentPresetState(int pianoIndex, const juce::String& name) const;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboardState;
    std::array<std::vector<mps::InstrumentPreset>, mps::kNumPianos> factoryPresetBanks;

    std::array<VoiceSlot, kMaxVoices> voices;

    juce::dsp::Compressor<float> compressor;
    juce::dsp::Oversampling<float> satOversampler { 2, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
    mps::fx::DattorroPlateReverb  plateReverb;
    mps::fx::SyntheticConvReverb  convReverb;
    mps::fx::ParametricEQ3Band    parametricEQ;
    mps::fx::StereoChorus         stereoChorus;
    mps::fx::StereoDelay          stereoDelay;
    mps::fx::OutputLimiter        outputLimiter;
    juce::AudioBuffer<float> fxDryBuffer;
    juce::AudioBuffer<float> reverbDryBuffer;
    juce::AudioBuffer<float> reverbConvBuffer;
    juce::AudioBuffer<float> stealScratchBuffer;
    juce::MidiBuffer arpFilteredMidi;
    juce::AudioBuffer<float> reverbPredelayBuffer;
    int reverbPredelayCapacity = 0;
    int reverbPredelayWritePos = 0;
    int lastConvReverbType = -1;
    int convReverbSwitchFadeRemaining = 0;
    std::array<float, 2> transientFastEnv = { 0.0f, 0.0f };
    std::array<float, 2> transientSlowEnv = { 0.0f, 0.0f };
    double preparedSampleRate = 44100.0;
    float lfoPhase = 0.0f;
    float outputGainCurrent = juce::Decibels::decibelsToGain(-3.0f);
    float pedalNoiseLevel = 0.0f;
    float pedalNoiseDecay = 1.0f;
    std::uint64_t pedalNoiseRandomState = 0x6a09e667f3bcc909ULL;
    std::atomic<std::uint64_t> pendingPedalNoiseSeed { 0x6a09e667f3bcc909ULL };
    std::atomic<bool> pendingPedalNoiseSeedValid { false };
    mutable std::atomic<std::uint64_t> realtimeRandomCounter { 0x9e3779b97f4a7c15ULL };
    bool offlinePresetMode = false;
    bool deterministicRenderMode = false;
    std::int64_t renderSeed = static_cast<std::int64_t>(0x5a17d37c9e5b1f4bULL);

    // Voice-count gain compensation (prevents polyphonic overload)
    float voiceCountScale      = 1.0f;
    float voiceCountSmoothCoeff = 0.001f;

    // Stolen voice crossfade (prevents clicks on voice stealing)
    static constexpr int kStealFadeSamples = 128;
    static constexpr int kRouteFadeSamples = 128;
    struct StealFade {
        std::array<float, kStealFadeSamples> left{};
        std::array<float, kStealFadeSamples> right{};
        int remaining = 0;
        int busIndex = 0;
    };
    std::array<StealFade, 4> stealFades{};

    struct CompressorCache
    {
        float threshold =  1.0f;
        float ratio     = -1.0f;
        float attack    = -1.0f;
        float release   = -1.0f;
    } compCache;

    PitchBendState pitchBend;
    modmatrix::ModulationMatrix modulationMatrix;
    modmatrix::ModResult cachedModResult;
    VelocityCurve  velocityCurve = VelocityCurve::Linear;
    std::atomic<int> midiCCPage { 0 };  // FLkey Mini CC page (0..kNumCCPages-1)

    // MIDI Learn state (message-thread writes, audio-thread reads via atomic flag)
    std::atomic<bool> midiLearnArmed { false };
    juce::String      midiLearnArmedParamId;   // message thread only
    // cc→paramId map: written on message thread (handleMidiCC deferred), read on audio thread read-only
    // Protected by a simple critical section since MIDI Learn is not in the hot path
    juce::CriticalSection midiLearnLock;
    std::unordered_map<int, juce::String> midiLearnMap;
    std::atomic<juce::RangedAudioParameter*> midiLearnArmedParam { nullptr };
    std::array<std::atomic<juce::RangedAudioParameter*>, 128> midiLearnParamSnapshot {};
    std::atomic<int> pendingMidiLearnCc { -1 };
    std::atomic<juce::RangedAudioParameter*> pendingMidiLearnParam { nullptr };
    std::atomic<float> pendingMidiLearnValue { 0.0f };
    void rebuildMidiLearnSnapshot();
    float lastNoteVelocity = 0.5f;
    mutable float currentBpm = 120.0f;
    std::atomic<int> activeVoiceCountAtomic { 0 };
    std::array<float, 16> sustainPedalPosition {};  // 0.0 = up, 1.0 = fully down
    std::array<bool, 16> sustainHoldLatched {};     // per-channel sustain hold hysteresis state
    std::array<bool, 16> sostenutoPedalDown {};       // per-channel sostenuto state
    std::array<bool, 16> unaCordaPedalDown {};        // per-channel una corda state
    std::uint64_t nextNoteOnOrder = 1;

    std::array<int,        mps::kNumPianos> currentPresetIndices;
    std::array<juce::File, mps::kNumPianos> currentUserPresetFiles;

    std::array<mps::GlobalFxSettings, mps::kNumPianos> cachedFxPerPiano;
    int lastSelectedPiano = -1;
    bool suppressConditionalSanitization = false;
    mps::GlobalFxSettings snapshotFx(int pianoIndex = -1) const;
    void restoreFx(const mps::GlobalFxSettings& fx);

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Deferred FX restore (message thread) — set from audio thread, consumed by handleAsyncUpdate
    std::atomic<int>  pendingFxRestorePiano { -1 };

    // Arpeggiator state (audio thread only)
    struct ArpState
    {
        static constexpr int kMaxHeldNotes = 128;
        std::array<int, kMaxHeldNotes> heldNotes {};
        int heldNoteCount = 0;
        int  currentStep    = 0;
        int  currentOctave  = 0;
        int  direction      = 1;          // +1 ascending / -1 descending
        int  lastTriggeredNote = -1;
        int  lastTriggeredChannel = 1;
        double phaseAccum   = 0.0;        // accumulated samples since last step
        bool noteIsOn       = false;
        int  gateCountdown  = 0;          // samples until gate note-off
        int  heldNoteForHold = -1;        // last note to sustain when arpHold is active
    };
    ArpState arpState;
    std::uint32_t arpRandomState = 0x243f6a88u;
    void processArpeggiator(juce::MidiBuffer& midi, int numSamples, int pianoIdx);
    int nextArpRandomInt(int upperExclusive) noexcept;

    // Mod wheel target cached for RT-safe reads (nullptr if param missing)
    std::atomic<float>* modWheelTargetRaw = nullptr;

    // RT-safe deferred parameter updates from MIDI CC handlers
    struct PendingParamUpdate
    {
        juce::RangedAudioParameter* param = nullptr;
        float normalisedValue = 0.0f;
    };
    static constexpr int kPendingParamQueueSize = 32;
    juce::AbstractFifo pendingParamFifo { kPendingParamQueueSize };
    std::array<PendingParamUpdate, kPendingParamQueueSize> pendingParamQueue;
    void queueParamUpdate(juce::RangedAudioParameter* param, float normalisedValue);

    void handleAsyncUpdate() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoSynthAudioProcessor)
};
