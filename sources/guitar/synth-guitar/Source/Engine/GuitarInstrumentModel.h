#pragma once

#include "GuitarDefs.h"

#include <array>

namespace mgs
{

enum class GuitarRenderEngineMode
{
    LegacyFamily = 0,
    V2,
    V2ModelOnly
};

struct GuitarInstrumentModelProfile
{
    const char* modelId = "";
    const char* targetEngineId = "";
    const char* exciterModel = "";
    const char* resonatorModel = "";
    const char* auditionFocus = "";
    SynthMode synthesisMode = SynthMode::KarplusStrong;
    float pluckFocus = 1.0f;
    float transientFocus = 1.0f;
    float bodyFocus = 1.0f;
    float bodyFeedbackFocus = 1.0f;
    float sympatheticFocus = 0.0f;
    float driveFocus = 1.0f;
    float continuousFocus = 0.0f;
    float stereoFocus = 1.0f;
    float loopFilterFocus = 0.0f;
    float inharmonicityFocus = 1.0f;
};

const std::array<GuitarInstrumentModelProfile, kNumInstruments>& getGuitarInstrumentModelProfiles();
const GuitarInstrumentModelProfile& getGuitarInstrumentModelProfile(int instrumentIndex);

GuitarRenderEngineMode getGuitarRenderEngineMode() noexcept;
void setGuitarRenderEngineMode(GuitarRenderEngineMode mode) noexcept;
const char* getGuitarRenderEngineModeName(GuitarRenderEngineMode mode) noexcept;

} // namespace mgs
