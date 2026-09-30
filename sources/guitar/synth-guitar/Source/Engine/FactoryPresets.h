#pragma once

#include "GuitarDefs.h"
#include <array>
#include <string>
#include <vector>

namespace mgs
{

struct PresetMetadata
{
    std::string mixRole = "production";
    std::vector<std::string> tags { "guitar", "factory" };
    std::string familyLabel = "guitar";
    float nominalPeakDb = -12.0f;
};

struct InstrumentPreset
{
    std::string   name;
    InstrSettings settings;
    GlobalFxSettings fx;
    int outputBus = 0;
    float playMode = 0.0f;
    float palmMute = 0.0f;
    PresetMetadata metadata {};
};

// A "collection" preset stores one InstrSettings per instrument — a generated scene snapshot.
struct CollectionPreset
{
    std::string                             name;
    std::array<InstrSettings, kNumInstruments> settings;
};

// Returns one curated factory bank per instrument. The actual bank sizes are the source of truth.
const std::array<std::vector<InstrumentPreset>, kNumInstruments>& getFactoryPresetBanks();

// Returns the generated style × variation scene presets.
const std::vector<CollectionPreset>& getFactoryPresets();

} // namespace mgs
