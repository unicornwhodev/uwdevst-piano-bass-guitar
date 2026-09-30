// filepath: synth-guitar/Source/Engine/RiffGenerator.cpp
#include "RiffGenerator.hpp"
#include "GuitarVoice.h"
#include <algorithm>
#include <cstring>

namespace mgs {
namespace riff {

// -----------------------------------------------------------------------------
// Pattern selection — deterministic from chord hash + intensity
// -----------------------------------------------------------------------------
uint32_t RiffGenerator::selectPatternHash(const RiffState& state) {
    uint32_t h = state.chordHash;
    h ^= static_cast<uint32_t>(state.intensity * 37);
    h ^= static_cast<uint32_t>(state.rootNote * 17);
    return h;
}

// -----------------------------------------------------------------------------
// Check if step is accented (beat 1 or 3)
// -----------------------------------------------------------------------------
bool RiffGenerator::isAccentedStep(int stepIndex, int stepsPerBar) {
    stepsPerBar = std::max(1, stepsPerBar);
    int beat = stepIndex % stepsPerBar;
    return (beat == 0 || beat == stepsPerBar / 2);
}

// -----------------------------------------------------------------------------
// Compute velocity: higher intensity = louder, accented beats = louder
// -----------------------------------------------------------------------------
float RiffGenerator::computeVelocity(int intensity, int stepInBar, bool isAccented) {
    (void) stepInBar;
    float base = 0.4f + (static_cast<float>(intensity) / 7.0f) * 0.55f;
    if (isAccented)
        base *= 1.35f;
    return std::clamp(base, 0.1f, 1.0f);
}

int RiffGenerator::getCurrentPatternLength(const RiffState& state)
{
    if (state.style == RiffStyle::Off)
        return 0;

    const PatternInfo* categories = getPatternsForStyle(state.style);
    if (!categories)
        return 0;

    const int catCount = getPatternCategoryCount(state.style);
    if (catCount <= 0)
        return 0;

    if (state.patternCategory >= static_cast<uint8_t>(catCount))
        return 0;

    const auto& cat = categories[state.patternCategory];
    if (cat.count == 0 || cat.data == nullptr || cat.stepsPerPattern == 0)
        return 0;

    return static_cast<int>(cat.stepsPerPattern);
}

// -----------------------------------------------------------------------------
// Get current step from state
// -----------------------------------------------------------------------------
const RiffStep* RiffGenerator::getCurrentStep(const RiffState& state) const {
    if (state.style == RiffStyle::Off)
        return nullptr;

    const PatternInfo* categories = getPatternsForStyle(state.style);
    if (!categories)
        return nullptr;

    int catCount = getPatternCategoryCount(state.style);
    if (catCount <= 0)
        return nullptr;

    if (state.patternCategory >= static_cast<uint8_t>(catCount))
        return nullptr;

    const auto& cat = categories[state.patternCategory];
    if (cat.count == 0 || cat.data == nullptr || cat.stepsPerPattern == 0)
        return nullptr;

    if (state.patternIndex >= cat.count)
        return nullptr;

    uint8_t idx = state.patternIndex;

    const RiffStep* pattern = cat.data + (idx * cat.stepsPerPattern);
    int stepInPattern = state.stepIndex % cat.stepsPerPattern;
    return pattern + stepInPattern;
}

// -----------------------------------------------------------------------------
// Advance step to next position
// -----------------------------------------------------------------------------
void RiffGenerator::advanceStep(RiffState& state, const RiffStep* step) {
    if (!step)
        return;

    const int stepsInPattern = getCurrentPatternLength(state);
    if (stepsInPattern <= 0)
        return;

    state.stepIndex++;
    if (state.stepIndex >= stepsInPattern)
    {
        state.stepIndex = 0;
        ++state.barCount;
    }
}

// -----------------------------------------------------------------------------
// Compute which pattern to use for current chord
// -----------------------------------------------------------------------------
void RiffGenerator::computePatternForChord(RiffState& state, int numNotes, const int* notes) {
    if (state.style == RiffStyle::Off)
        return;

    const PatternInfo* categories = getPatternsForStyle(state.style);
    if (!categories)
    {
        state.patternCategory = 0;
        state.patternIndex = 0;
        state.stepIndex = 0;
        state.barCount = 0;
        return;
    }

    int catCount = getPatternCategoryCount(state.style);
    if (catCount <= 0)
    {
        state.patternCategory = 0;
        state.patternIndex = 0;
        state.stepIndex = 0;
        state.barCount = 0;
        return;
    }

    uint32_t h = selectPatternHash(state);

    // Use hash to select category based on number of notes (more notes = lead or comp)
    uint8_t catIdx = static_cast<uint8_t>(h % static_cast<uint32_t>(catCount));
    state.patternCategory = catIdx;

    // Pattern index within category is also hash-derived
    const auto& cat = categories[catIdx];
    if (cat.count == 0 || cat.data == nullptr || cat.stepsPerPattern == 0)
    {
        state.patternCategory = 0;
        state.patternIndex = 0;
        state.stepIndex = 0;
        state.barCount = 0;
        return;
    }

    state.patternIndex = static_cast<uint8_t>((h / 13) % cat.count);
    state.stepIndex = 0;
    state.barCount = 0;

    // Root note = first held note
    if (numNotes > 0 && notes != nullptr) {
        state.rootNote = notes[0];
    }
}

// -----------------------------------------------------------------------------
// Evaluate step — main sequencing function
// Returns: MIDI note to fire (-1 = no note)
// Fills: articulation, velocity, octaveShift
// -----------------------------------------------------------------------------
int RiffGenerator::evaluateStep(float sampleRate, const RiffState& state,
                                 uint8_t& outArticulation, float& outVelocity,
                                 uint8_t& outOctaveShift) {
    (void) sampleRate;
    if (state.style == RiffStyle::Off)
        return -1;

    if (state.heldCount == 0)
        return -1;

    // Get current step
    const RiffStep* step = getCurrentStep(state);
    if (!step)
        return -1;

    // Handle rest
    if (step->noteOffset == -1)
        return -1;

    // Handle hold previous note
    if (step->noteOffset == -99) {
        outArticulation = state.currentArticulation;
        outVelocity = 0.0f; // no new note, hold previous
        outOctaveShift = 0;
        return -1;
    }

    // Compute actual MIDI note
    int root = state.rootNote;
    if (root < 0) root = 60;
    int midiNote = root + step->noteOffset;
    midiNote += state.currentOctaveShift * 12;
    midiNote = std::clamp(midiNote, 0, 127);

    // Determine articulation
    uint8_t articulation = step->articulation;
    outArticulation = articulation;

    // Compute velocity
    const int stepsInPattern = std::max(1, getCurrentPatternLength(state));
    bool accented = isAccentedStep(state.stepIndex, stepsInPattern);
    outVelocity = computeVelocity(state.intensity, state.stepIndex, accented);

    // Apply gate (if gate < 1.0, reduce velocity for softer steps)
    if (state.gate < 1.0f) {
        outVelocity *= state.gate;
    }

    outOctaveShift = 0;

    return midiNote;
}

} // namespace riff
} // namespace mgs
