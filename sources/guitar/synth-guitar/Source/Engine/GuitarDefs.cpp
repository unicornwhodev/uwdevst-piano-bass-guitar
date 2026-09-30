#include "GuitarDefs.h"
#include <algorithm>

namespace mgs
{
namespace
{

constexpr InstrCharacteristics makeInstrCharacteristics(
    const SynthMode synthMode,
    const float stringDecaySeconds,
    const float stringBrightness,
    const float stringInharmonicity,
    const float excitationBrightness,
    const float excitationDecaySeconds,
    const float excitationPosition,
    const float bodySizeRatio,
    const float bodyResonance,
    const float bodyBrightness,
    const float outputDrive,
    const float outputPitchFollow,
    const float outputStereoWidth,
    const bool  legatoEnabled = false)
{
    return {
        synthMode,
        { stringDecaySeconds, stringBrightness, stringInharmonicity },
        { excitationBrightness, excitationDecaySeconds, excitationPosition },
        { bodySizeRatio, bodyResonance, bodyBrightness },
        { outputDrive, outputPitchFollow, outputStereoWidth },
        legatoEnabled
    };
}

// -------------------------------------------------------------------------
// Names
// -------------------------------------------------------------------------
constexpr const char* kNames[kNumInstruments] = {
    "Folk Steel",         // 0  ACOUSTIQUE
    "12 Cordes",          // 1
    "Flamenca",           // 2
    "Clean",              // 3  \xC3\x89LECTRIQUE
    "Crunch",             // 4
    "Lead",               // 5
    "Synth Guitar",       // 6  \xC3\x89LECTRONIQUE
    "E-Guitar Pad",       // 7
    "Guitar Ambient"      // 8
};

constexpr const char* kShortNames[kNumInstruments] = {
    "FOLK", "12CD", "FLAM",
    "CLN",  "CRNC", "LEAD",
    "SYNG", "EPAD", "GAMB"
};

constexpr const char* kFamilyNames[kNumFamilies] = {
    "ACOUSTIQUE",
    "\xC3\x89LECTRIQUE",
    "SYNTH\xC3\x88SE & TEXTURES"  // ex "ÉLECTRONIQUE" — audit ETAPE_17 §17.1.b (Pad identity)
};

  constexpr FxAvailability FX(bool reverb,
                bool saturator,
                bool transient,
                bool compressor,
                bool eq,
                bool chorus,
                bool delay,
                bool limiter,
                bool cabinet)
  {
    return FxAvailability{ reverb, saturator, transient, compressor, eq, chorus, delay, limiter, cabinet };
  }

  // FX(reverb, saturator, transient, compressor, eq, chorus, delay, limiter, cabinet)
  // Audit ETAPE_17 §17.1.d/e : chorus enabled on Folk/Crunch/Lead, delay enabled on Flamenca
  constexpr std::array<FxAvailability, kNumInstruments> kFxAvailability = {{
    FX(true,  false, true,  true,  true,  true,  true,  true,  false),  // 0 Folk Steel    (chorus on)
    FX(true,  false, true,  true,  true,  true,  true,  true,  false),  // 1 12 Cordes
    FX(true,  false, true,  false, true,  false, true,  true,  false),  // 2 Flamenca      (delay on)
    FX(true,  true,  true,  true,  true,  true,  true,  true,  true),   // 3 Clean
    FX(true,  true,  true,  true,  true,  true,  true,  true,  true),   // 4 Crunch        (chorus on)
    FX(true,  true,  true,  true,  true,  true,  true,  true,  true),   // 5 Lead          (chorus on)
    FX(true,  true,  false, true,  true,  true,  true,  true,  false),  // 6 Synth Guitar
    FX(true,  false, false, true,  true,  true,  true,  true,  false),  // 7 E-Guitar Pad
    FX(true,  false, false, false, true,  true,  true,  true,  false)   // 8 Guitar Ambient
  }};

// -------------------------------------------------------------------------
// Characteristics
// -------------------------------------------------------------------------
constexpr InstrCharacteristics kChars[kNumInstruments] = {
    // Folk Steel — warm steel-string, prominent body
    // NOTE: Steel string inharmonicity ~0.0015-0.003 (physically accurate)
    makeInstrCharacteristics(SynthMode::KarplusStrong,
                             3.5f, 0.55f, 0.0020f,  // FIXED: 0.0005 → 0.002 (steel realistic)
                             0.60f, 0.008f, 0.15f,
                             1.2f, 0.30f, 0.45f,    // bodyResonance 0.20→0.30 (plan audit)
                             0.0f, 1.0f, 0.30f),
    // 12 Cordes — doubled strings, natural chorus, bright
    // Doubled courses have slightly higher inharmonicity due to coupled vibration
    makeInstrCharacteristics(SynthMode::KarplusStrong,
                             3.0f, 0.65f, 0.0020f,  // FIXED: 0.0008 → 0.002 (steel realistic)
                             0.65f, 0.010f, 0.13f,
                             1.3f, 0.30f, 0.50f,    // bodyResonance 0.20→0.30 (plan audit)
                             0.0f, 1.0f, 0.60f),
    // Flamenca — nylon, bright, percussive attack, short sustain
    // Nylon string inharmonicity ~0.0002 (much softer material = less stiffness)
    makeInstrCharacteristics(SynthMode::KarplusStrong,
                             1.8f, 0.70f, 0.0002f,  // KEEP: correct for nylon
                             0.80f, 0.005f, 0.08f,
                             1.0f, 0.45f, 0.55f,
                             0.0f, 1.0f, 0.25f),
    // Clean — clear electric, bell-like, slight body, no drive
    makeInstrCharacteristics(SynthMode::KarplusStrong,
                             4.0f, 0.50f, 0.0004f,
                             0.55f, 0.006f, 0.18f,
                             0.3f, 0.15f, 0.50f,
                             0.0f, 1.0f, 0.35f, true),
    // Crunch — mild overdrive, punchy
    makeInstrCharacteristics(SynthMode::KarplusStrong,
                             3.5f, 0.55f, 0.0005f,
                             0.60f, 0.007f, 0.20f,
                             0.2f, 0.10f, 0.55f,
                             0.35f, 1.0f, 0.30f, true),
    // Lead — high gain, bright sustained, compressed
    // Drive reduced 0.70 → 0.45 (audit ETAPE_17 §17.1.a) to preserve articulation/polyphony
    makeInstrCharacteristics(SynthMode::KarplusStrong,
                             5.0f, 0.60f, 0.0006f,
                             0.50f, 0.005f, 0.22f,
                             0.1f, 0.05f, 0.60f,
                             0.45f, 1.0f, 0.25f, true),
    // Synth Guitar — heavily processed, synth-like
    makeInstrCharacteristics(SynthMode::Hybrid,
                             4.5f, 0.45f, 0.001f,
                             0.40f, 0.015f, 0.25f,
                             0.85f, 0.18f, 0.55f,
                             0.20f, 1.0f, 0.50f),
    // E-Guitar Pad — ambient electric, articulated texture, mid presence
    // DIFFERENTIATED: brighter, faster, more midrange for texture work
    makeInstrCharacteristics(SynthMode::Pad,
                             8.0f, 0.40f, 0.0003f,   // decay 8s, brightness 0.40 (up from 0.35)
                             0.25f, 0.020f, 0.35f,   // excitation decay faster (0.020 vs 0.030)
                             0.35f, 0.20f, 0.40f,    // body smaller (0.35 vs 0.4)
                             0.10f, 1.0f, 0.70f),    // drive 0.10 for slight grit
    // Guitar Ambient — cathedral-like space, pure ambient, no mid bite
    // DIFFERENTIATED: darker, longer, very spatial, no drive
    makeInstrCharacteristics(SynthMode::Pad,
                             12.0f, 0.25f, 0.0004f,  // decay 12s (up from 10), brightness 0.25 (darker)
                             0.20f, 0.080f, 0.40f,   // excitation decay slower (0.080 vs 0.050)
                             0.55f, 0.25f, 0.25f,    // body larger (0.55 vs 0.5), stereo 0.25 (darker)
                             0.0f, 1.0f, 0.85f)      // drive 0.0 (no grit), stereo width 0.85 (very wide)
};

// -------------------------------------------------------------------------
// Default settings
// -------------------------------------------------------------------------
constexpr InstrSettings kDefaults[kNumInstruments] = {
    // Folk Steel
    { 0.80f, 0.0f, 0.50f, 0.003f, 3.0f, 0.10f, 0.40f,
      0.55f, 0.0f, 0.55f, 0.35f, 0.50f, 8000.0f, 0.0f },
    // 12 Cordes
    { 0.78f, 0.0f, 0.60f, 0.004f, 2.8f, 0.10f, 0.45f,
      0.50f, 0.0f, 0.60f, 0.55f, 0.55f, 9000.0f, 0.0f },
    // Flamenca
    { 0.82f, 0.0f, 0.65f, 0.001f, 1.5f, 0.05f, 0.25f,
      0.45f, 0.0f, 0.75f, 0.30f, 0.60f, 10000.0f, 0.0f },
    // Clean
    { 0.80f, 0.0f, 0.48f, 0.002f, 3.5f, 0.25f, 0.35f,
      0.15f, 0.0f, 0.50f, 0.30f, 0.50f, 8000.0f, 0.0f },
    // Crunch
    { 0.78f, 0.0f, 0.55f, 0.002f, 3.0f, 0.30f, 0.30f,
      0.10f, 0.40f, 0.55f, 0.30f, 0.55f, 9000.0f, 0.0f },
    // Lead
    // Envelope tightened (decay 4.0→3.0, sustain 0.50→0.40, release 0.50→0.30)
    // and driveAmount 0.70→0.45 — audit ETAPE_07/08 (sustain accumulation)
    { 0.75f, 0.0f, 0.60f, 0.003f, 3.0f, 0.40f, 0.30f,
      0.05f, 0.45f, 0.45f, 0.25f, 0.60f, 10000.0f, 0.0f },
    // Synth Guitar
    { 0.78f, 0.0f, 0.50f, 0.010f, 4.0f, 0.40f, 0.50f,
      0.0f, 0.25f, 0.40f, 0.50f, 0.55f, 7000.0f, 0.0f },
    // E-Guitar Pad — DIFFERENTIATED: brighter, faster attack, more midrange
    { 0.72f, 0.0f, 0.40f, 0.040f, 5.0f, 0.50f, 1.20f,   // brightness 0.40, attack 0.040, decay 5s
      0.20f, 0.10f, 0.25f, 0.65f, 0.40f, 6000.0f, 0.0f },  // drive 0.10, stereo 0.65, LP 6000
    // Guitar Ambient — DIFFERENTIATED: darker, longer, pure spatial
    { 0.68f, 0.0f, 0.25f, 0.20f, 10.0f, 0.45f, 2.50f,  // brightness 0.25, attack 0.20, decay 10s
      0.25f, 0.00f, 0.20f, 0.85f, 0.35f, 3500.0f, 0.0f }   // drive 0.00, stereo 0.85, LP 3500
};

// -------------------------------------------------------------------------
// Descriptions (French, UTF-8)
// -------------------------------------------------------------------------
constexpr const char* kDescriptions[kNumInstruments] = {
    // Folk Steel
    "La guitare folk \xC3\xa0 cordes acier est l'instrument embl\xC3\xa9matique du "
    "folk, du country et de la chanson. Son corps Dreadnought projette "
    "un son puissant et \xC3\xa9quilibr\xC3\xa9, riche en harmoniques m\xC3\xa9""diums. "
    "Les cordes acier offrent une brillance naturelle et une attaque "
    "franche, id\xC3\xa9""ale pour le fingerpicking comme pour le strumming.",

    // 12 Cordes
    "La guitare douze cordes poss\xC3\xa8""de six paires de cordes accord\xC3\xa9""es "
    "\xC3\xa0 l'unisson ou \xC3\xa0 l'octave. Ce doublage cr\xC3\xa9""e un chorus naturel "
    "riche et scintillant, une signature sonore reconnaissable entre "
    "toutes. De Led Zeppelin \xC3\xa0 The Eagles, la douze cordes ajoute "
    "une dimension orchestrale aux arrangements de guitare.",

    // Flamenca
    "La guitare flamenca est taill\xC3\xa9""e dans du cypr\xC3\xa8s et de l'\xC3\xa9pic\xC3\xa9""a "
    "pour un son percussif, vif et brillant. Ses cordes en nylon "
    "produisent un timbre clair et incisif, parfait pour les rasgueados "
    "et les picados. L'attaque franche et le sustain court donnent "
    "\xC3\xa0 chaque note une pr\xC3\xa9""cision et une \xC3\xa9nergie remarquables.",

    // Clean
    "Le son clean \xC3\xa9lectrique est la voix pure de la guitare amplifi\xC3\xa9""e, "
    "sans distorsion ni saturation. Cristallin et pr\xC3\xa9""cis, il r\xC3\xa9v\xC3\xa8le "
    "chaque nuance du jeu. Des accords jazz de Wes Montgomery aux "
    "arp\xC3\xa8ges funky de Nile Rodgers, le son clean reste le fondement "
    "de nombreux styles musicaux.",

    // Crunch
    "Le crunch est ce territoire magique entre le clean et la saturation "
    "totale. L'ampli r\xC3\xa9pond au toucher du guitariste : doux pour le "
    "clean, fort pour la saturation. Ce son dynamique et expressif "
    "est le c\xC5\x93ur du rock classique, du blues \xC3\xa9lectrique et du "
    "rhythm'n'blues.",

    // Lead
    "Le son lead satur\xC3\xa9 est l'arme du soliste. Gain \xC3\xa9lev\xC3\xa9, sustain "
    "infini, harmoniques chantantes : chaque note se prolonge et hurle. "
    "De Hendrix \xC3\xa0 Satriani, en passant par Gilmour et Van Halen, "
    "le lead guitar d\xC3\xa9""finit l'\xC3\xa9motion et la puissance du rock, "
    "du metal et du blues \xC3\xa9lectrique.",

    // Synth Guitar
    "La synth guitar fusionne la guitare \xC3\xa9lectrique avec le monde de "
    "la synth\xC3\xa8se sonore. Filtres dynamiques, oscillateurs num\xC3\xa9riques "
    "et traitements granulaires transforment le signal en textures "
    "uniques. Utilis\xC3\xa9""e par des artistes comme Pat Metheny et Robert "
    "Fripp, elle repousse les limites de l'instrument.",

    // E-Guitar Pad
    "Le pad de guitare \xC3\xa9lectrique cr\xC3\xa9""e des nappes sonores \xC3\xa0 partir "
    "de cordes amplifi\xC3\xa9""es, trait\xC3\xa9""es par des effets de delay, reverb "
    "et modulation. Le r\xC3\xa9sultat est une texture douce et enveloppante, "
    "suspendue dans le temps. Id\xC3\xa9""al pour les ambiances cin\xC3\xa9matiques "
    "et les paysages sonores \xC3\xa9th\xC3\xa9r\xC3\xa9s.",

    // Guitar Ambient
    "La guitare ambient transforme l'instrument en g\xC3\xa9n\xC3\xa9rateur de "
    "paysages sonores. Volume swell, reverb infinie, delays multiples : "
    "les notes se dissolvent en nappes atmosph\xC3\xa9riques. Pionniers "
    "comme Brian Eno et Daniel Lanois ont d\xC3\xa9montr\xC3\xa9 que la guitare "
    "peut cr\xC3\xa9""er des mondes sonores d'une beaut\xC3\xa9 hypnotique."
};

} // anonymous namespace

// =========================================================================
// Accessor implementations
// =========================================================================
Family getFamily(int instrIndex)
{
    int idx = std::clamp(instrIndex, 0, kNumInstruments - 1);
    if (idx < kFamilyStart[1]) return Family::Acoustique;
    if (idx < kFamilyStart[2]) return Family::Electrique;
    return Family::Electronique;
}

int getFamilyStartIndex(Family family)
{
    return kFamilyStart[static_cast<int>(family)];
}

const char* getFamilyName(int familyIndex)
{
    return kFamilyNames[std::clamp(familyIndex, 0, kNumFamilies - 1)];
}

const char* getInstrName(int instrIndex)
{
    return kNames[std::clamp(instrIndex, 0, kNumInstruments - 1)];
}

const char* getInstrShortName(int instrIndex)
{
    return kShortNames[std::clamp(instrIndex, 0, kNumInstruments - 1)];
}

const InstrCharacteristics& getCharacteristics(int instrIndex)
{
    return kChars[std::clamp(instrIndex, 0, kNumInstruments - 1)];
}

InstrSettings getDefaultSettings(int instrIndex)
{
    return kDefaults[std::clamp(instrIndex, 0, kNumInstruments - 1)];
}

const char* getInstrDescription(int instrIndex)
{
    return kDescriptions[std::clamp(instrIndex, 0, kNumInstruments - 1)];
}

const FxAvailability& getFxAvailability(int instrIndex)
{
  return kFxAvailability[std::clamp(instrIndex, 0, kNumInstruments - 1)];
}

bool isFxAvailable(int instrIndex, GlobalFxSlot slot)
{
  const auto& availability = getFxAvailability(instrIndex);
  switch (slot)
  {
    case GlobalFxSlot::Reverb:    return availability.reverb;
    case GlobalFxSlot::Saturator: return availability.saturator;
    case GlobalFxSlot::Transient: return availability.transient;
    case GlobalFxSlot::Compressor:return availability.compressor;
    case GlobalFxSlot::Eq:        return availability.eq;
    case GlobalFxSlot::Chorus:    return availability.chorus;
    case GlobalFxSlot::Delay:     return availability.delay;
    case GlobalFxSlot::Limiter:   return availability.limiter;
    case GlobalFxSlot::Cabinet:   return availability.cabinet;
    default:                      return true;
  }
}

GlobalFxSettings maskUnavailableFx(int instrIndex, const GlobalFxSettings& fx)
{
  auto masked = fx;
  const auto& availability = getFxAvailability(instrIndex);
  masked.reverbOn     = availability.reverb     && masked.reverbOn;
  masked.saturatorOn  = availability.saturator  && masked.saturatorOn;
  masked.transientOn  = availability.transient  && masked.transientOn;
  masked.compressorOn = availability.compressor && masked.compressorOn;
  masked.eqOn         = availability.eq         && masked.eqOn;
  masked.chorusOn     = availability.chorus     && masked.chorusOn;
  masked.delayOn      = availability.delay      && masked.delayOn;
  masked.limiterOn    = availability.limiter    && masked.limiterOn;
  masked.cabinetOn    = availability.cabinet    && masked.cabinetOn;
  return masked;
}

} // namespace mgs
