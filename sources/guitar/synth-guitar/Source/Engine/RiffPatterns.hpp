// filepath: synth-guitar/Source/Engine/RiffPatterns.hpp
#pragma once
#include <cstdint>

// =============================================================================
// RiffPatterns.hpp — Guitar pattern tables by style
//
// Each style defines several patterns. Each pattern is an array of steps.
// Each step: { noteOffset, articulation, duration }
//
// noteOffset: semitones from root chord note (-1 = rest, -99 = use previous note)
// articulation: 0=strum, 1=arp, 2=hybrid, 3=hammerOn, 4=pullOff, 5=slide, 6=bend, 7=tap
// duration: steps on 16th-note grid (1 = 16th, 2 = 8th, 4 = quarter, 8 = half)
//
// Patterns are selected deterministically from chordNotes + intensity using hash.
// =============================================================================

namespace mgs {
namespace riff {

enum class Articulation : uint8_t {
    Strum     = 0,  // normal pluck with rake
    Arp       = 1,  // arpeggio style (spacing)
    Hybrid    = 2,  // pluck + legato
    HammerOn  = 3,  // legato upward (no new pluck)
    PullOff   = 4,  // legato downward (no new pluck)
    Slide     = 5,  // portamento glide
    Bend      = 6,  // pitch bend on sustained note
    Tap       = 7   // very short attack + high velocity
};

enum class RiffStyle : uint8_t {
    Off      = 0,
    Rock     = 1,
    Blues    = 2,
    Metal    = 3,
    Folk     = 4,
    Jazz     = 5,
    Funk     = 6,
    Ambient  = 7
};

struct RiffStep {
    int8_t  noteOffset;     // semitones from root (-1=rest, -99=hold previous)
    uint8_t articulation;   // Articulation enum value
    uint8_t duration;       // steps on 16th grid (1=16th, 2=8th, 4=quarter)
};

// -----------------------------------------------------------------------------
// ROCK PATTERNS
// -----------------------------------------------------------------------------
namespace rock {

// I - IV - V power chord patterns (root=0, no 3rd = power chord)
constexpr RiffStep kPatternPowerChunk[][8] = {
    // Pattern A: steady 8th downstrokes
    {
        { 0,  0, 2 }, { -1, 0, 1 }, { 4,  0, 1 }, { -1, 0, 1 },
        { 0,  0, 2 }, { -1, 0, 1 }, { 4,  0, 1 }, { -1, 0, 1 }
    },
    // Pattern B: syncopated
    {
        { 0,  0, 1 }, { -1, 0, 1 }, { 4,  0, 1 }, { 0,  0, 1 },
        { 3,  0, 1 }, { -1, 0, 1 }, { 4,  0, 1 }, { -1, 0, 1 }
    },
    // Pattern C: palm-mute chug
    {
        { 0,  0, 1 }, { -1, 0, 1 }, { 0,  0, 1 }, { -1, 0, 1 },
        { 0,  0, 1 }, { -1, 0, 1 }, { 0,  0, 1 }, { -1, 0, 1 }
    }
};

// Melodic lead pattern (root + 5th + octave)
constexpr RiffStep kPatternLead[][8] = {
    {
        { 0,  0, 2 }, { -1, 0, 1 }, { 7,  0, 1 }, { -1, 0, 1 },
        { 12, 0, 2 }, { -1, 0, 1 }, { 7,  0, 1 }, { -1, 0, 1 }
    },
    {
        { 0,  0, 1 }, { 7,  0, 1 }, { 12, 0, 1 }, { -1, 0, 1 },
        { 0,  0, 1 }, { 7,  0, 1 }, { 12, 0, 1 }, { -1, 0, 1 }
    }
};

// Triplet feel (16th triplet grid)
constexpr RiffStep kPatternTriplet[][6] = {
    {
        { 0,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 },
        { 0,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 }
    }
};

// Accented downstroke (beats 1 and 3)
constexpr RiffStep kPatternDownAccented[][8] = {
    {
        { 0,  0, 4 }, { -1, 0, 1 }, { 4,  0, 2 }, { -1, 0, 1 },
        { 0,  0, 4 }, { -1, 0, 1 }, { 4,  0, 2 }, { -1, 0, 1 }
    }
};

constexpr int kNumPowerChunk = 3;
constexpr int kNumLead = 2;
constexpr int kNumTriplet = 1;
constexpr int kNumDownAccented = 1;

} // namespace rock

// -----------------------------------------------------------------------------
// BLUES PATTERNS
// -----------------------------------------------------------------------------
namespace blues {

// Basic 12-bar blues shuffle (root + bend on beat 2&4)
constexpr RiffStep kPatternShuffle[][8] = {
    // Pattern A: shuffle with bends on 2+
    {
        { 0,  0, 2 }, { -1, 0, 1 }, { 0,  6, 1 }, { -1, 0, 1 },
        { 0,  0, 2 }, { -1, 0, 1 }, { 0,  6, 1 }, { -1, 0, 1 }
    },
    // Pattern B: call-response
    {
        { 0,  0, 2 }, { -1, 0, 1 }, { 0,  0, 1 }, { 3,  0, 1 },
        { 0,  0, 2 }, { -1, 0, 1 }, { 0,  0, 1 }, { 3,  0, 1 }
    }
};

// Boogie woogie left hand (quarter note rhythm)
constexpr RiffStep kPatternBoogie[][4] = {
    {
        { 0,  0, 1 }, { -1, 0, 1 }, { 4,  0, 1 }, { -1, 0, 1 }
    },
    {
        { 0,  0, 1 }, { 4,  0, 1 }, { 0,  0, 1 }, { 4,  0, 1 }
    }
};

// Turnaround (bar 11-12 typical blues ending)
constexpr RiffStep kPatternTurnaround[][8] = {
    {
        { 0,  0, 1 }, { 0,  0, 1 }, { 0,  0, 1 }, { -99, 0, 1 },
        { 1,  0, 1 }, { 1,  0, 1 }, { 1,  0, 1 }, { -99, 0, 1 }
    }
};

// Bend pattern (hold + bend + release)
constexpr RiffStep kPatternBendRelease[][4] = {
    {
        { 0,  0, 3 }, { 0,  6, 1 }, { 0,  0, 2 }, { -1, 0, 1 }
    },
    {
        { 0,  0, 2 }, { -1, 0, 1 }, { 0,  6, 1 }, { 0,  0, 2 }
    }
};

// Hammer-on / pull-off phrase
constexpr RiffStep kPatternHammerPull[][8] = {
    {
        { 0,  0, 2 }, { 2,  3, 1 }, { 0,  4, 2 }, { -1, 0, 1 },
        { 0,  0, 2 }, { 2,  3, 1 }, { 0,  4, 2 }, { -1, 0, 1 }
    }
};

constexpr int kNumShuffle = 2;
constexpr int kNumBoogie = 2;
constexpr int kNumTurnaround = 1;
constexpr int kNumBendRelease = 2;
constexpr int kNumHammerPull = 1;

} // namespace blues

// -----------------------------------------------------------------------------
// METAL PATTERNS
// -----------------------------------------------------------------------------
namespace metal {

// Downstroke chug (palm-muted power chords)
constexpr RiffStep kPatternChug[][8] = {
    {
        { 0,  0, 1 }, { -1, 0, 1 }, { 0,  0, 1 }, { -1, 0, 1 },
        { 0,  0, 1 }, { -1, 0, 1 }, { 0,  0, 1 }, { -1, 0, 1 }
    },
    {
        { 0,  0, 1 }, { -1, 0, 1 }, { 4,  0, 1 }, { -1, 0, 1 },
        { 0,  0, 1 }, { -1, 0, 1 }, { 4,  0, 1 }, { -1, 0, 1 }
    }
};

// Gallop (3 16ths + 1 8th — very common metal)
constexpr RiffStep kPatternGallop[][4] = {
    {
        { 0,  0, 1 }, { 0,  0, 1 }, { 0,  0, 1 }, { 4,  0, 2 }
    }
};

// Sweep picking arpeggio (up then pull-off down)
constexpr RiffStep kPatternSweepArp[][8] = {
    {
        { 0,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 }, { 12, 0, 1 },
        { 7,  4, 1 }, { 4,  4, 1 }, { 0,  4, 1 }, { -1, 0, 2 }
    }
};

// Thrash gallop variant
constexpr RiffStep kPatternThrashGallop[][8] = {
    {
        { 0,  0, 1 }, { 0,  0, 1 }, { 4,  0, 1 }, { -1, 0, 1 },
        { 0,  0, 1 }, { 0,  0, 1 }, { 4,  0, 1 }, { -1, 0, 1 }
    }
};

constexpr int kNumChug = 2;
constexpr int kNumGallop = 1;
constexpr int kNumSweepArp = 1;
constexpr int kNumThrashGallop = 1;

} // namespace metal

// -----------------------------------------------------------------------------
// FOLK PATTERNS
// -----------------------------------------------------------------------------
namespace folk {

// Carter Family style (https://en.wikipedia.org/wiki/Carter_family)
constexpr RiffStep kPatternCarter[][8] = {
    {
        { 0,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 }, { -1, 0, 1 },
        { 0,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 }, { -1, 0, 1 }
    },
    {
        { 0,  0, 1 }, { 7,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 },
        { 0,  0, 1 }, { 7,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 }
    }
};

// Travis pick (alternating bass + treble)
constexpr RiffStep kPatternTravis[][8] = {
    {
        { 0,  0, 1 }, { 7,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 },
        { 0,  0, 1 }, { 7,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 }
    }
};

// Flamenco strum
constexpr RiffStep kPatternFlamenco[][8] = {
    {
        { 0,  0, 1 }, { -1, 0, 1 }, { 4,  0, 1 }, { -1, 0, 1 },
        { 7,  0, 1 }, { 4,  0, 1 }, { 0,  0, 1 }, { -1, 0, 1 }
    }
};

constexpr int kNumCarter = 2;
constexpr int kNumTravis = 1;
constexpr int kNumFlamenco = 1;

} // namespace folk

// -----------------------------------------------------------------------------
// JAZZ PATTERNS
// -----------------------------------------------------------------------------
namespace jazz {

// Comp rhythm (shell voicing: root + 3rd + 7th)
constexpr RiffStep kPatternCompHalf[][4] = {
    {
        { 0,  0, 2 }, { -1, 0, 2 }, { 4,  0, 2 }, { -1, 0, 2 }
    },
    {
        { 0,  0, 1 }, { 3,  0, 1 }, { 4,  0, 1 }, { 3,  0, 1 }
    }
};

// Bebop line
constexpr RiffStep kPatternBebop[][8] = {
    {
        { 0,  0, 1 }, { 2,  0, 1 }, { 4,  0, 1 }, { 7,  0, 1 },
        { 9,  0, 1 }, { 7,  0, 1 }, { 4,  0, 1 }, { 2,  0, 1 }
    }
};

constexpr int kNumCompHalf = 2;
constexpr int kNumBebop = 1;

} // namespace jazz

// -----------------------------------------------------------------------------
// FUNK PATTERNS
// -----------------------------------------------------------------------------
namespace funk {

// 16th note funk (syncopated, short staccato)
constexpr RiffStep kPatternFunk16[][8] = {
    {
        { 0,  0, 1 }, { -1, 0, 1 }, { 0,  0, 1 }, { 4,  0, 1 },
        { 0,  0, 1 }, { -1, 0, 1 }, { 0,  0, 1 }, { 4,  0, 1 }
    },
    {
        { 0,  0, 1 }, { 3,  0, 1 }, { 0,  0, 1 }, { 4,  0, 1 },
        { 0,  0, 1 }, { 3,  0, 1 }, { 0,  0, 1 }, { 4,  0, 1 }
    }
};

// Ghost note funk
constexpr RiffStep kPatternGhost[][8] = {
    {
        { 0,  0, 1 }, { -1, 0, 1 }, { 0,  0, 1 }, { -1, 0, 1 },
        { 4,  0, 1 }, { -1, 0, 1 }, { 7,  0, 1 }, { -1, 0, 1 }
    }
};

constexpr int kNumFunk16 = 2;
constexpr int kNumGhost = 1;

} // namespace funk

// -----------------------------------------------------------------------------
// AMBIENT PATTERNS
// -----------------------------------------------------------------------------
namespace ambient {

// Pad-like long sustained chords with slow attack
constexpr RiffStep kPatternPadLong[][4] = {
    {
        { 0,  0, 4 }, { 4,  0, 4 }, { 7,  0, 4 }, { -1, 0, 4 }
    }
};

// Bell-like arp (spaced, reverb-friendly)
constexpr RiffStep kPatternBellArp[][6] = {
    {
        { 0,  1, 2 }, { -1, 0, 1 }, { 4,  1, 2 }, { -1, 0, 1 },
        { 7,  1, 2 }, { -1, 0, 1 }
    }
};

// Texture cluster (dense, overlapping)
constexpr RiffStep kPatternCluster[][4] = {
    {
        { 0,  0, 1 }, { 1,  0, 1 }, { 4,  0, 1 }, { 5,  0, 1 }
    }
};

constexpr int kNumPadLong = 1;
constexpr int kNumBellArp = 1;
constexpr int kNumCluster = 1;

} // namespace ambient

// -----------------------------------------------------------------------------
// Style → pattern table index
// -----------------------------------------------------------------------------
struct PatternInfo {
    const RiffStep* data;
    uint8_t count;
    uint8_t stepsPerPattern;
};

inline const PatternInfo* getPatternsForStyle(RiffStyle style) {
    switch (style) {
        case RiffStyle::Rock: {
            static const PatternInfo s_rock[] = {
                { &rock::kPatternPowerChunk[0][0], rock::kNumPowerChunk, 8 },
                { &rock::kPatternLead[0][0], rock::kNumLead, 8 },
                { &rock::kPatternTriplet[0][0], rock::kNumTriplet, 6 },
            };
            return s_rock;
        }
        case RiffStyle::Blues: {
            static const PatternInfo s_blues[] = {
                { &blues::kPatternShuffle[0][0], blues::kNumShuffle, 8 },
                { &blues::kPatternBoogie[0][0], blues::kNumBoogie, 4 },
                { &blues::kPatternBendRelease[0][0], blues::kNumBendRelease, 4 },
                { &blues::kPatternHammerPull[0][0], blues::kNumHammerPull, 8 },
            };
            return s_blues;
        }
        case RiffStyle::Metal: {
            static const PatternInfo s_metal[] = {
                { &metal::kPatternChug[0][0], metal::kNumChug, 8 },
                { &metal::kPatternGallop[0][0], metal::kNumGallop, 4 },
                { &metal::kPatternSweepArp[0][0], metal::kNumSweepArp, 8 },
                { &metal::kPatternThrashGallop[0][0], metal::kNumThrashGallop, 8 },
            };
            return s_metal;
        }
        case RiffStyle::Folk: {
            static const PatternInfo s_folk[] = {
                { &folk::kPatternCarter[0][0], folk::kNumCarter, 8 },
                { &folk::kPatternTravis[0][0], folk::kNumTravis, 8 },
                { &folk::kPatternFlamenco[0][0], folk::kNumFlamenco, 8 },
            };
            return s_folk;
        }
        case RiffStyle::Jazz: {
            static const PatternInfo s_jazz[] = {
                { &jazz::kPatternCompHalf[0][0], jazz::kNumCompHalf, 4 },
                { &jazz::kPatternBebop[0][0], jazz::kNumBebop, 8 },
            };
            return s_jazz;
        }
        case RiffStyle::Funk: {
            static const PatternInfo s_funk[] = {
                { &funk::kPatternFunk16[0][0], funk::kNumFunk16, 8 },
                { &funk::kPatternGhost[0][0], funk::kNumGhost, 8 },
            };
            return s_funk;
        }
        case RiffStyle::Ambient: {
            static const PatternInfo s_ambient[] = {
                { &ambient::kPatternPadLong[0][0], ambient::kNumPadLong, 4 },
                { &ambient::kPatternBellArp[0][0], ambient::kNumBellArp, 6 },
                { &ambient::kPatternCluster[0][0], ambient::kNumCluster, 4 },
            };
            return s_ambient;
        }
        default:
            return nullptr;
    }
}

inline constexpr int getPatternCategoryCount(RiffStyle style) {
    switch (style) {
        case RiffStyle::Rock:    return 3;
        case RiffStyle::Blues:   return 4;
        case RiffStyle::Metal:    return 4;
        case RiffStyle::Folk:    return 3;
        case RiffStyle::Jazz:    return 2;
        case RiffStyle::Funk:    return 2;
        case RiffStyle::Ambient: return 3;
        default: return 0;
    }
}

} // namespace riff
} // namespace mgs