#pragma once

#include "PianoDefs.h"
#include "../../../Shared/ModulationMatrix.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace mps
{

struct PresetPerformanceState
{
    float macroWarmth = 0.5f;
    float macroBrillance = 0.5f;
    float macroExpression = 0.5f;
    float macroResonance = 0.5f;
    float lfoRate = 1.0f;
    float lfoDepth = 0.0f;
    int lfoWave = 0;
    LfoDestination lfoDestination = LfoDestination::Off;
    float pitchBendRange = 2.0f;
    int velocityCurve = 0;
    bool monoMode = false;
    bool tremoloSync = false;
    int modWheelTarget = 1;
    modmatrix::MatrixState modMatrixState {};
};

struct PresetMetadata
{
    std::string intent;
    std::string tags;
    std::string family;
    std::string mixRole;
};

struct InstrumentPreset
{
    std::string      name;
    PianoSettings    settings;
    GlobalFxSettings fx;
    int outputBus = 0;
    PresetPerformanceState performance {};
    PresetMetadata metadata {};
};

bool isSupportedMixRole(std::string_view mixRole) noexcept;
std::string_view getPresetBrowserTier(std::string_view mixRole) noexcept;

const std::array<std::vector<InstrumentPreset>, kNumPianos>& getFactoryPresetBanks();

} // namespace mps
