// filepath: synth-guitar/Source/Engine/RiffGenerator.hpp
#pragma once
#include "RiffPatterns.hpp"
#include <array>

namespace mgs {

class GuitarVoice; // forward

namespace riff {

// =============================================================================
// RiffState — replaces/extends ArpState in PluginProcessor
// =============================================================================
struct RiffState {
    // Notes currently held (same as old ArpState::heldNotes)
    std::array<int, 16> heldNotes = { -1, -1, -1, -1, -1, -1, -1, -1,
                                      -1, -1, -1, -1, -1, -1, -1, -1 };
    int heldCount = 0;

    // Riff sequencer state
    int stepIndex = 0;           // current step within the pattern
    int barCount = 0;            // how many bars we've played (pattern length wraps)
    int currentOctaveShift = 0;  // current octave offset (-2 to +2 typical)

    // Timing
    float phaseAccum = 0.0f;          // phase accumulator for tempo sync
    int gateCountdown = 0;           // samples until next note should fire

    // Riff generation params (set from UI/audio thread)
    RiffStyle style = RiffStyle::Off;
    int intensity = 0;           // 0-7
    float gate = 0.75f;         // 0.3-1.0
    bool holdEnabled = false;

    // Chord info for pattern hashing
    uint32_t chordHash = 0;      // same as PluginProcessor chordHash
    int rootNote = 60;          // MIDI root of current chord

    // Pattern selection (deterministic from chordNotes + intensity)
    uint8_t patternCategory = 0; // which category (powerChunk/lead/triplet etc)
    uint8_t patternIndex = 0;    // which pattern within that category

    // Articulation override for current step
    uint8_t currentArticulation = 0;

    void reset() {
        stepIndex = 0;
        barCount = 0;
        currentOctaveShift = 0;
        phaseAccum = 0.0f;
        gateCountdown = 0;
        currentArticulation = 0;
    }
};

// =============================================================================
// RiffGenerator — processes riff patterns and generates note events
// =============================================================================
class RiffGenerator {
public:
    RiffGenerator() = default;

    // Evaluate whether to fire a new note at the current sample
    // Returns: note to fire (-1 = no note), or -1 for rest
    // Fills out articulation + velocity for the returned note
    int evaluateStep(float sampleRate, const RiffState& state,
                    uint8_t& outArticulation, float& outVelocity,
                    uint8_t& outOctaveShift);

    // Compute pattern for current chord (call when chord changes)
    void computePatternForChord(RiffState& state, int numNotes, const int* notes);

    // Get current step info
    const RiffStep* getCurrentStep(const RiffState& state) const;

    // Advance one step
    static void advanceStep(RiffState& state, const RiffStep* step);

    // Current pattern length for loop/accent decisions.
    static int getCurrentPatternLength(const RiffState& state);

    // Compute velocity from intensity and beat position
    static float computeVelocity(int intensity, int stepInBar, bool isAccented);

private:
    // Select pattern deterministically from chordNotes hash + intensity
    static uint32_t selectPatternHash(const RiffState& state);

    // Check if a step is accented (beat 1 or 3 of bar)
    static bool isAccentedStep(int stepIndex, int stepsPerBar);
};

} // namespace riff
} // namespace mgs
