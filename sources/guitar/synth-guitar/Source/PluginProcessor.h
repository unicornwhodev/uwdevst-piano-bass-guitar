#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Engine/RiffGenerator.hpp"
#include "Engine/GuitarVoice.h"
#include "Engine/FactoryPresets.h"
#include "Engine/FxProcessors.h"
#include "../../Shared/PitchBendState.h"
#include "../../Shared/ModulationMatrix.h"

class GuitarSynthAudioProcessor : public juce::AudioProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener,
                                  private juce::AsyncUpdater
{
public:
    enum class QualityMode : int
    {
        Live = 0,
        Studio
    };

    enum class PlayMode : int
    {
        Poly = 0,
        MonoRetrig,
        MonoLegato
    };

    static constexpr int kNumAuxOutputs = 4;
    static constexpr int kMaxVoices     = 32;
    static constexpr int kVoicePoolSize = kMaxVoices * 2;
    static constexpr const char* kProcessorName = "UWdeVST_Guitar";

    GuitarSynthAudioProcessor();
    ~GuitarSynthAudioProcessor() override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::String makeInstrParamId(int instrIndex, const juce::String& suffix);
    static auto createBusLayout() -> BusesProperties;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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

    // ── MIDI Learn ─────────────────────────────────────────────────────
    void midiLearnArm(const juce::String& paramId);
    void midiLearnClear(int ccNumber);
    void midiLearnClearAll();
    bool isMidiLearnActive() const noexcept { return midiLearnArmed.load(std::memory_order_relaxed); }
    juce::String getMidiLearnArmedParam() const { return midiLearnArmedParamId; }
    std::vector<std::pair<int, juce::String>> getMidiLearnMappings() const;
    void saveMidiLearnToXml(juce::XmlElement& xml) const;
    void loadMidiLearnFromXml(const juce::XmlElement& xml);
    void flushPendingAsyncUpdatesForTests();

    juce::StringArray getFactoryPresetNames() const;
    int  getCurrentFactoryPresetIndex() const noexcept;
    void applyFactoryPreset(int presetIndex);
    bool saveFactoryPreset(int presetIndex);

    // Generated scene snapshots. Runtime remains one active instrument per instance.
    juce::StringArray getCollectionPresetNames() const;
    int  getCurrentCollectionPresetIndex() const noexcept { return currentCollectionPresetIndex; }
    void applyCollectionPreset(int sceneIndex);

    static juce::File getUserPresetsDirectory(int instrIndex);
    static juce::File getFactoryOverridesDirectory();
    juce::Array<juce::File> scanUserPresets() const;
    bool saveUserPreset(const juce::String& name);
    bool updateUserPreset(const juce::File& file);
    bool deleteUserPreset(const juce::File& file);
    bool loadUserPreset(const juce::File& file);
    bool isCurrentPresetUser() const noexcept;
    juce::File getCurrentUserPresetFile() const noexcept;

    int getSelectedInstrIndex() const;
    int getActiveVoiceCount() const noexcept { return activeVoiceCountAtomic.load(std::memory_order_relaxed); }
    void randomizePreset(float amount = 0.15f);
    QualityMode getQualityMode() const noexcept;
    bool isDelaySyncEnabled() const noexcept;
    int getDelayDivisionIndex() const noexcept;
    float getLastKnownHostTempoBpm() const noexcept;
    float getMainMeterLevel(int channel) const noexcept;
    float getAuxMeterLevel(int auxIndex) const noexcept;
    bool isClipLatched() const noexcept;
    void clearClipLatch() noexcept;

    modmatrix::ModSlot getModMatrixSlot(int index) const;
    void setModMatrixSlot(int index, modmatrix::Source source,
                          modmatrix::Destination destination, float amount);
    float getModMatrixLfo2Rate() const noexcept;
    int getModMatrixLfo2Wave() const noexcept;
    void setModMatrixLfo2Rate(float rateHz);
    void setModMatrixLfo2Wave(int waveformIndex);

private:
    struct VoiceSlot
    {
        mgs::GuitarVoice* voice = nullptr;
        int midiNote   = -1;
        int instrIndex = 0;
        int poolSlot = -1;
        int midiChannel = 1;
        int outputBus = 0;
        float noteVelocity = 0.0f;
        bool keyDown = false;
        uint64_t activationAge = 0;
    };

    struct DyingVoiceSlot
    {
        mgs::GuitarVoice* voice = nullptr;
        int instrIndex = 0;
        int poolSlot = -1;
        int outputBus = 0;
        float noteVelocity = 0.0f;
        uint64_t activationAge = 0;
    };

    struct InstrSnapshot
    {
        mgs::InstrSettings settings;
        int outputBus = 0;
    };

    struct GlobalBlockState
    {
        int selectedInstrIndex = 0;
        int qualityMode = 0;
        int delayDivision = 0;
        int lfoWave = 0;
        float lfoRate = 2.0f;
        float lfoDepth = 0.0f;
        float hostBpm = 120.0f;
        float outputGainDb = -3.0f;
        float macroCorps = 0.5f;
        float macroBrillance = 0.5f;
        float macroGain = 0.5f;
        float macroEspace = 0.5f;
        bool fxLock = false;
        bool delaySyncToHost = false;
        mgs::GlobalFxSettings fx;
    };

    struct GlobalParamRefs
    {
        std::atomic<float>* outputGain = nullptr;
        std::atomic<float>* selectedInstr = nullptr;
        std::atomic<float>* qualityMode = nullptr;
        std::atomic<float>* delaySync = nullptr;
        std::atomic<float>* delayDivision = nullptr;
        std::atomic<float>* lfoRate = nullptr;
        std::atomic<float>* lfoDepth = nullptr;
        std::atomic<float>* lfoWave = nullptr;
        std::atomic<float>* macroCorps = nullptr;
        std::atomic<float>* macroBrillance = nullptr;
        std::atomic<float>* macroGain = nullptr;
        std::atomic<float>* macroEspace = nullptr;
        std::atomic<float>* compThreshold = nullptr;
        std::atomic<float>* compRatio = nullptr;
        std::atomic<float>* compAttack = nullptr;
        std::atomic<float>* compRelease = nullptr;
        std::atomic<float>* compMakeup = nullptr;
        std::atomic<float>* compMix = nullptr;
        std::atomic<float>* satDrive = nullptr;
        std::atomic<float>* satMix = nullptr;
        std::atomic<float>* transientAttack = nullptr;
        std::atomic<float>* transientSustain = nullptr;
        std::atomic<float>* transientMix = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusDelay = nullptr;
        std::atomic<float>* chorusMix = nullptr;
        std::atomic<float>* reverbSize = nullptr;
        std::atomic<float>* reverbDamping = nullptr;
        std::atomic<float>* reverbWidth = nullptr;
        std::atomic<float>* reverbMix = nullptr;
        std::atomic<float>* eqLowFreq = nullptr;
        std::atomic<float>* eqLowGain = nullptr;
        std::atomic<float>* eqMidFreq = nullptr;
        std::atomic<float>* eqMidGain = nullptr;
        std::atomic<float>* eqMidQ = nullptr;
        std::atomic<float>* eqHighFreq = nullptr;
        std::atomic<float>* eqHighGain = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* limiterThreshold = nullptr;
        std::atomic<float>* limiterRelease = nullptr;
        std::atomic<float>* cabMix = nullptr;
        std::atomic<float>* strumSpread = nullptr;
        std::atomic<float>* fxSatEnable = nullptr;
        std::atomic<float>* fxTransientEnable = nullptr;
        std::atomic<float>* fxCompEnable = nullptr;
        std::atomic<float>* fxReverbEnable = nullptr;
        std::atomic<float>* fxEqEnable = nullptr;
        std::atomic<float>* fxChorusEnable = nullptr;
        std::atomic<float>* fxDelayEnable = nullptr;
        std::atomic<float>* fxLimiterEnable = nullptr;
        std::atomic<float>* fxCabinetEnable = nullptr;
        std::atomic<float>* fxLock = nullptr;
    };

    struct InstrParamRefs
    {
        std::atomic<float>* level = nullptr;
        std::atomic<float>* tune = nullptr;
        std::atomic<float>* stringBrightness = nullptr;
        std::atomic<float>* attack = nullptr;
        std::atomic<float>* decay = nullptr;
        std::atomic<float>* sustain = nullptr;
        std::atomic<float>* release = nullptr;
        std::atomic<float>* bodyAmount = nullptr;
        std::atomic<float>* drive = nullptr;
        std::atomic<float>* attackBrightness = nullptr;
        std::atomic<float>* stereoWidth = nullptr;
        std::atomic<float>* pickPosition = nullptr;
        std::atomic<float>* lowPassHz = nullptr;
        std::atomic<float>* pan = nullptr;
        std::atomic<float>* output = nullptr;
    };

    float getParamValue(const juce::String& paramId) const;
    float sanitizeParameterValue(const juce::String& paramId, float value, float fallback, int* warningCount = nullptr) const;
    void  setParamValue(const juce::String& paramId, float value);
    void  setParamValueInternal(const juce::String& paramId, float value, bool notifyHost);
    void  sanitizeAllParameterValues();
    float readCachedParamValue(const std::atomic<float>* raw, float fallback = 0.0f) const noexcept;
    void  resolveParameterPointers();
    double readHostTempoBpm() const;
    GlobalBlockState buildGlobalBlockState() const;
    InstrSnapshot buildInstrSnapshot(int instrIndex, const GlobalBlockState& blockState) const;
    mgs::InstrSettings captureInstrSettingsFromParams(int instrIndex) const;
    int captureInstrOutputBusFromParams(int instrIndex) const;
    void applyPerformanceMacros(int instrIndex, mgs::InstrSettings& s, const GlobalBlockState& blockState) const;
    PlayMode getPlayMode() const noexcept;
    float getPalmMuteAmountForInstrument(int instrIndex) const noexcept;
    int  findFreeVoice() const;
    int  acquireVoicePoolSlot(int instrIndex);
    int  nextArpRandomInt(int upperExclusive) noexcept;
    void clearVoice(VoiceSlot& slot);
    void clearDyingVoice(DyingVoiceSlot& slot);
    void releaseVoices(int midiChannel, bool immediate);
    void panicAllVoices();
    void resetRealtimeModulationState() noexcept;
    void triggerNoteOn(int instrIndex, int midiChannel, int midiNote, float velocity,
                       const InstrSnapshot& snapshot, const modmatrix::ModContext& baseModContext,
                       uint32_t chordHash = 0);
    void triggerNoteOff(int midiChannel, int midiNote);
    void handleMidiCC(int ccNumber, int ccValue, int instrIndex);
    void updateGlobalEffectParameters(const GlobalBlockState& blockState);
    mgs::GlobalFxSettings snapshotFxSettings() const;
    mgs::GlobalFxSettings sanitizeFxSettings(const mgs::GlobalFxSettings& fx) const;
    mgs::GlobalFxSettings sanitizeFxSettingsForInstrument(int instrIndex, const mgs::GlobalFxSettings& fx) const;
    void applyFxToParams(int instrIndex, const mgs::GlobalFxSettings& fx, bool notifyHost = false);
    void storeCurrentInstrumentFxState(int instrIndex);
    void restoreInstrumentFxState(int instrIndex, bool notifyHost = false);
    bool isFxAvailableForCurrentInstrument(mgs::GlobalFxSlot slot) const;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void processMasterFxChain(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void processGlobalTransient(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void processGlobalSaturator(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void processGlobalCompressor(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void applyGlobalLfo(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void processGlobalEQ(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void processGlobalCabinet(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void processGlobalChorus(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void processGlobalDelay(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void processGlobalReverb(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void processGlobalLimiter(juce::AudioBuffer<float>& mainBuffer, const GlobalBlockState& blockState);
    void loadFactoryOverrides();
    void updateOutputMeters(juce::AudioBuffer<float>& fullBuffer, const juce::AudioBuffer<float>& mainBuffer);
    void applyInstrPresetSettings(int instrIndex, const mgs::InstrSettings& s, bool notifyHost = false);
    bool writePresetManifest(const juce::File& presetFile, const juce::String& presetName,
                             int instrIndex, const juce::String& sourceModel) const;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboardState;
    std::array<std::vector<mgs::InstrumentPreset>, mgs::kNumInstruments> factoryPresetBanks;

    std::array<VoiceSlot, kMaxVoices> voices;
    std::array<std::array<std::unique_ptr<mgs::GuitarVoice>, kVoicePoolSize>, mgs::kNumInstruments> voicePool;
    std::array<std::array<std::atomic_bool, kVoicePoolSize>, mgs::kNumInstruments> voicePoolInUse {};
    std::array<DyingVoiceSlot, kMaxVoices> dyingVoices;
    uint64_t voiceAgeCounter = 0;

    juce::dsp::Compressor<float> compressor;
    mgs::fx::StereoChorus        chorus;
    mgs::fx::DattorroPlateReverb reverb;
    mgs::fx::SyntheticConvReverb convReverb;
    mgs::fx::ParametricEQ3Band   eq;
    mgs::fx::StereoDelay         stereoDelay;
    mgs::fx::OutputLimiter       limiter;
    mgs::fx::CabinetSim         cabSim;
    juce::AudioBuffer<float> satDryBuffer;
    juce::AudioBuffer<float> compDryBuffer;
    juce::AudioBuffer<float> mainDryBuffer;
    juce::AudioBuffer<float> reverbWetBuffer;
    juce::dsp::Oversampling<float> satOversamplingMono { 1, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
    juce::dsp::Oversampling<float> satOversamplingStereo { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
    static constexpr int kMaxMidiEventsPerBlock = 1024;
    struct CollectedMidiEvent
    {
        int samplePosition = 0;
        juce::MidiMessage message;
    };
    struct MidiNoteEvent
    {
        int midiChannel = 1;
        int midiNote = 0;
        float velocity = 0.0f;
    };
    struct LegatoStack
    {
        static constexpr int kCapacity = 16;
        std::array<int, kCapacity> notes {};
        int count = 0;

        void clear() noexcept
        {
            count = 0;
        }

        bool empty() const noexcept
        {
            return count <= 0;
        }

        int back() const noexcept
        {
            return count > 0 ? notes[static_cast<std::size_t>(count - 1)] : -1;
        }

        void push(int note) noexcept
        {
            if (count < kCapacity)
            {
                notes[static_cast<std::size_t>(count++)] = note;
                return;
            }

            for (int i = 1; i < kCapacity; ++i)
                notes[static_cast<std::size_t>(i - 1)] = notes[static_cast<std::size_t>(i)];
            notes.back() = note;
        }

        void removeLatest(int note) noexcept
        {
            for (int i = count - 1; i >= 0; --i)
            {
                if (notes[static_cast<std::size_t>(i)] != note)
                    continue;

                for (int j = i; j < count - 1; ++j)
                    notes[static_cast<std::size_t>(j)] = notes[static_cast<std::size_t>(j + 1)];
                --count;
                return;
            }
        }
    };
    std::array<CollectedMidiEvent, kMaxMidiEventsPerBlock> collectedMidiEvents {};
    std::array<MidiNoteEvent, kMaxMidiEventsPerBlock> noteOnEvents {};
    std::array<MidiNoteEvent, kMaxMidiEventsPerBlock> noteOffEvents {};
    std::array<float, 2> transientFastEnv = { 0.0f, 0.0f };
    std::array<float, 2> transientSlowEnv = { 0.0f, 0.0f };
    std::array<float, 2> saturatorPrevInput = { 0.0f, 0.0f };
    double preparedSampleRate = 44100.0;
    float lfoPhase = 0.0f;
    float outputGainCurrent = juce::Decibels::decibelsToGain(-3.0f);
    float lfoRateCurrent = 2.0f;
    float lfoDepthCurrent = 0.0f;
    float satDriveCurrent = 1.5f;
    float satMixCurrent = 0.1f;

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

    // Arpeggiator state (audio thread only)
    struct ArpState
    {
        static constexpr int kMaxHeldNotes = 128;
        std::array<int, kMaxHeldNotes> heldNotes {};
        int  heldNoteCount = 0;
        int  currentStep    = 0;
        int  currentOctave  = 0;
        int  direction      = 1;
        int  lastTriggeredNote = -1;
        int  lastTriggeredChannel = 1;
        double phaseAccum   = 0.0;
        bool noteIsOn       = false;
        int  gateCountdown  = 0;

        bool empty() const noexcept { return heldNoteCount <= 0; }
        int size() const noexcept { return heldNoteCount; }

        bool contains(int note) const noexcept
        {
            for (int i = 0; i < heldNoteCount; ++i)
                if (heldNotes[static_cast<std::size_t>(i)] == note)
                    return true;
            return false;
        }

        void add(int note) noexcept
        {
            if (contains(note) || heldNoteCount >= kMaxHeldNotes)
                return;

            int insertAt = heldNoteCount++;
            while (insertAt > 0 && heldNotes[static_cast<std::size_t>(insertAt - 1)] > note)
            {
                heldNotes[static_cast<std::size_t>(insertAt)] = heldNotes[static_cast<std::size_t>(insertAt - 1)];
                --insertAt;
            }
            heldNotes[static_cast<std::size_t>(insertAt)] = note;
        }

        void remove(int note) noexcept
        {
            for (int i = 0; i < heldNoteCount; ++i)
            {
                if (heldNotes[static_cast<std::size_t>(i)] != note)
                    continue;

                for (int j = i; j < heldNoteCount - 1; ++j)
                    heldNotes[static_cast<std::size_t>(j)] = heldNotes[static_cast<std::size_t>(j + 1)];
                --heldNoteCount;
                heldNotes[static_cast<std::size_t>(heldNoteCount)] = 0;
                return;
            }
        }

        int get(int index) const noexcept
        {
            if (heldNoteCount <= 0)
                return 0;
            return heldNotes[static_cast<std::size_t>(juce::jlimit(0, heldNoteCount - 1, index))];
        }
    };
    ArpState arpState;
    std::uint32_t arpRandomState = 0x9e3779b9u;
    bool processArpeggiator(const juce::MidiBuffer& midi, int numSamples, int instrIdx);
    bool processRiffGenerator(const juce::MidiBuffer& midi, int numSamples, int instrIdx);
    mgs::riff::RiffState riffState;

    // Phase 3 — Strum timing for chord coherency
    struct StrumState
    {
        static constexpr int kMaxStrumNotes = 8;
        std::array<int, kMaxStrumNotes> pendingNotes = { -1, -1, -1, -1, -1, -1, -1, -1 };
        std::array<float, kMaxStrumNotes> pendingVelocities = {};
        std::array<int, kMaxStrumNotes> pendingInstruments = {};
        std::array<int, kMaxStrumNotes> pendingChannels = {};
        std::array<uint32_t, kMaxStrumNotes> pendingChordHashes = {};
        int pendingCount = 0;

        // Strum timing
        int strumSampleDelay = 0;  // samples between each note
        int samplesUntilNextStrumNote = 0;

        // Strum settings from user
        bool strumEnabled = true;
        float strumSpreadMs = 15.0f;
        int strumDirection = 1;  // 1 = bass→treble, -1 = treble→bass

        // Sample rate for conversion
        double sampleRate = 44100.0;

        void clearPending()
        {
            pendingNotes.fill(-1);
            pendingVelocities.fill(0.0f);
            pendingInstruments.fill(0);
            pendingChannels.fill(1);
            pendingChordHashes.fill(0);
            pendingCount = 0;
            samplesUntilNextStrumNote = 0;
        }

        bool pushPending(int midiNote, float velocity, int instrument, int midiChannel, uint32_t chordHash)
        {
            if (pendingCount >= kMaxStrumNotes)
                return false;

            pendingNotes[static_cast<std::size_t>(pendingCount)] = midiNote;
            pendingVelocities[static_cast<std::size_t>(pendingCount)] = velocity;
            pendingInstruments[static_cast<std::size_t>(pendingCount)] = instrument;
            pendingChannels[static_cast<std::size_t>(pendingCount)] = midiChannel;
            pendingChordHashes[static_cast<std::size_t>(pendingCount)] = chordHash;
            ++pendingCount;
            return true;
        }

        bool popPending(int& midiNote, float& velocity, int& instrument, int& midiChannel, uint32_t& chordHash)
        {
            if (pendingCount <= 0)
                return false;

            midiNote = pendingNotes[0];
            velocity = pendingVelocities[0];
            instrument = pendingInstruments[0];
            midiChannel = pendingChannels[0];
            chordHash = pendingChordHashes[0];

            for (int i = 1; i < pendingCount; ++i)
            {
                pendingNotes[static_cast<std::size_t>(i - 1)] = pendingNotes[static_cast<std::size_t>(i)];
                pendingVelocities[static_cast<std::size_t>(i - 1)] = pendingVelocities[static_cast<std::size_t>(i)];
                pendingInstruments[static_cast<std::size_t>(i - 1)] = pendingInstruments[static_cast<std::size_t>(i)];
                pendingChannels[static_cast<std::size_t>(i - 1)] = pendingChannels[static_cast<std::size_t>(i)];
                pendingChordHashes[static_cast<std::size_t>(i - 1)] = pendingChordHashes[static_cast<std::size_t>(i)];
            }

            --pendingCount;
            pendingNotes[static_cast<std::size_t>(pendingCount)] = -1;
            pendingVelocities[static_cast<std::size_t>(pendingCount)] = 0.0f;
            pendingInstruments[static_cast<std::size_t>(pendingCount)] = 0;
            pendingChannels[static_cast<std::size_t>(pendingCount)] = 1;
            pendingChordHashes[static_cast<std::size_t>(pendingCount)] = 0;
            if (pendingCount == 0)
                samplesUntilNextStrumNote = 0;
            return true;
        }

        void removePendingNotes(int midiChannel, int midiNote)
        {
            int writeIndex = 0;
            for (int i = 0; i < pendingCount; ++i)
            {
                const bool channelMatches = midiChannel == 0 || pendingChannels[static_cast<std::size_t>(i)] == midiChannel;
                const bool noteMatches = midiNote < 0 || pendingNotes[static_cast<std::size_t>(i)] == midiNote;
                if (channelMatches && noteMatches)
                    continue;

                if (writeIndex != i)
                {
                    pendingNotes[static_cast<std::size_t>(writeIndex)] = pendingNotes[static_cast<std::size_t>(i)];
                    pendingVelocities[static_cast<std::size_t>(writeIndex)] = pendingVelocities[static_cast<std::size_t>(i)];
                    pendingInstruments[static_cast<std::size_t>(writeIndex)] = pendingInstruments[static_cast<std::size_t>(i)];
                    pendingChannels[static_cast<std::size_t>(writeIndex)] = pendingChannels[static_cast<std::size_t>(i)];
                    pendingChordHashes[static_cast<std::size_t>(writeIndex)] = pendingChordHashes[static_cast<std::size_t>(i)];
                }
                ++writeIndex;
            }

            for (int i = writeIndex; i < pendingCount; ++i)
            {
                pendingNotes[static_cast<std::size_t>(i)] = -1;
                pendingVelocities[static_cast<std::size_t>(i)] = 0.0f;
                pendingInstruments[static_cast<std::size_t>(i)] = 0;
                pendingChannels[static_cast<std::size_t>(i)] = 1;
                pendingChordHashes[static_cast<std::size_t>(i)] = 0;
            }

            pendingCount = writeIndex;
            if (pendingCount == 0)
                samplesUntilNextStrumNote = 0;
        }

        void reset()
        {
            clearPending();
            strumSampleDelay = 0;
        }
    };
    StrumState strumState;

    bool sustainPedalDown = false;
    std::array<LegatoStack, mgs::kNumInstruments> legatoStacks;

    std::array<int,        mgs::kNumInstruments> currentPresetIndices;
    std::array<juce::File, mgs::kNumInstruments> currentUserPresetFiles;
    int currentCollectionPresetIndex = -1;

    std::array<mgs::GlobalFxSettings, mgs::kNumInstruments> instrumentFxStates;
    GlobalParamRefs globalParamRefs;
    std::array<InstrParamRefs, mgs::kNumInstruments> instrParamRefs;
    std::atomic<int> pendingSelectedInstrumentIndex { 0 };
    int cachedSelectedInstrumentIndex = -1;

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

    // MIDI Learn state
    std::atomic<bool>               midiLearnArmed { false };
    juce::String                    midiLearnArmedParamId;
    juce::CriticalSection           midiLearnLock;
    std::unordered_map<int, juce::String> midiLearnMap;
    std::atomic<juce::RangedAudioParameter*> midiLearnArmedParam { nullptr };
    std::array<std::atomic<juce::RangedAudioParameter*>, 128> midiLearnParamSnapshot {};
    std::atomic<int> pendingMidiLearnCc { -1 };
    std::atomic<juce::RangedAudioParameter*> pendingMidiLearnParam { nullptr };
    std::atomic<float> pendingMidiLearnValue { 0.0f };
    void rebuildMidiLearnSnapshot();

    std::atomic<int> activeVoiceCountAtomic { 0 };
    std::array<std::atomic<float>, 2> mainMeterLevels { 0.0f, 0.0f };
    std::array<std::atomic<float>, kNumAuxOutputs> auxMeterLevels { 0.0f, 0.0f, 0.0f, 0.0f };
    std::atomic<float> lastKnownHostTempoBpm { 120.0f };
    std::atomic<bool> clipLatched { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarSynthAudioProcessor)
};
