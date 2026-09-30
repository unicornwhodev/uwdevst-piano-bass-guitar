#include "PianoDefs.h"

#include <algorithm>
#include <atomic>

namespace mps
{
namespace
{
std::atomic<int> gPianoRenderEngineMode { static_cast<int>(PianoRenderEngineMode::V2) };

constexpr PianoCharacteristics makePianoCharacteristics(float inharmonicityScale,
                            float hammerHardnessBase,
                            float soundboardQ,
                            float stringDetuning,
                            int partialCount,
                            float bodyDelayRatio,
                            float bodyDamping,
                            float sympatheticSemis,
                            float decay1Ratio,
                            float decay2Ratio,
                            float sustainPlatform,
                            bool hasDampers,
                            bool isElectric,
                            float duplexScale = 0.0f,
                            float promptSoundRatio = 0.12f,
                            float afterSoundRatio  = 0.50f,
                            float railsbackScale   = 0.0f,
                            float sustainDecayCoeff = kSustainDecayCoeff)
{
  return {
    { inharmonicityScale, stringDetuning, partialCount, sympatheticSemis, hasDampers, duplexScale },
    { hammerHardnessBase },
    { soundboardQ, bodyDelayRatio, bodyDamping },
    { decay1Ratio, decay2Ratio, sustainPlatform, promptSoundRatio, afterSoundRatio, sustainDecayCoeff },
    { isElectric },
    railsbackScale
  };
}

// =========================================================================
// Names
// =========================================================================
constexpr std::array<const char*, kNumPianos> kNames = {
    // Concert
    "Steinway D", "B\xC3\xB6sendorfer Imperial", "Yamaha CFX",
    // Vintage
    "Bastringue", "Piano Pr\xC3\xA9par\xC3\xA9",
    // Electric
    "Rhodes Mark I", "Wurlitzer 200A", "Clavinet D6"
};

constexpr std::array<const char*, kNumPianos> kShortNames = {
    "STEIN", "BOSEN", "YAMAH",
    "BAST",  "PREPA",
    "RHODS", "WURL",  "CLAV"
};

constexpr std::array<const char*, kNumFamilies> kFamilyNames = {
    "CONCERT", "VINTAGE / CHARACTER", "\xC3\x89LECTRIQUES / PIANO-ADJACENT"
};

constexpr std::array<PianoInstrumentModel, kNumPianos> kPianoInstrumentModels = {{
    { "piano.steinway_d.v2", "steinway_d", Family::Concert,
      PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard,
      PianoEngineReadiness::DedicatedVoice,
      "SteinwayVoice", "felt hammer with duplex strike", "concert soundboard and sympathetic string bed",
      "balanced concert grand, duplex sheen, controlled body bloom" },

    { "piano.bosendorfer_imperial.v2", "bosendorfer_imperial", Family::Concert,
      PianoInstrumentAlgorithm::BosendorferImperialBassResonator,
      PianoEngineReadiness::DedicatedVoice,
      "BosendorferVoice", "soft Vienna hammer", "extended bass body resonator",
      "dark bass extension, long sustain, rounded upper register" },

    { "piano.yamaha_cfx.v2", "yamaha_cfx", Family::Concert,
      PianoInstrumentAlgorithm::YamahaCfxBrightScaleAction,
      PianoEngineReadiness::DedicatedVoice,
      "YamahaVoice", "fast bright action", "tight modern soundboard",
      "clear attack, bright scale, fast mid-register response" },

    { "piano.bastringue.v2", "bastringue", Family::Vintage,
      PianoInstrumentAlgorithm::BastringueHonkyTonkTackRail,
      PianoEngineReadiness::DedicatedVoice,
      "BastringueVoice", "tack rail and worn hammer", "detuned upright body",
      "honky-tonk beating, short boxy body, mechanical tack" },

    { "piano.prepared.v2", "piano_prepare", Family::Vintage,
      PianoInstrumentAlgorithm::PreparedPianoObjectBuzz,
      PianoEngineReadiness::DedicatedVoice,
      "PreparedPianoVoice", "prepared hammer and object contact", "muted string and metallic object modes",
      "metallic buzz, damped sustain, percussive object color" },

    { "piano.rhodes_mark_i.v2", "rhodes_mark_i", Family::Electric,
      PianoInstrumentAlgorithm::RhodesTinePickupBark,
      PianoEngineReadiness::DedicatedVoice,
      "RhodesVoice", "hammered tine", "tonebar pickup preamp",
      "bell tine, pickup asymmetry, velocity bark" },

    { "piano.wurlitzer_200a.v2", "wurlitzer_200a", Family::Electric,
      PianoInstrumentAlgorithm::WurlitzerReedAmpBite,
      PianoEngineReadiness::DedicatedVoice,
      "WurlitzerVoice", "reed strike", "reed bandpass and amp speaker",
      "reed bite, mid bark, asymmetric amp drive" },

    { "piano.clavinet_d6.v2", "clavinet_d6", Family::Electric,
      PianoInstrumentAlgorithm::ClavinetPickupStringSnap,
      PianoEngineReadiness::DedicatedVoice,
      "ClavinetVoice", "string hammer and key release", "pickup selector and damping",
      "snappy string attack, pickup quack, short release noise" }
}};

// =========================================================================
// Piano characteristics
// =========================================================================
constexpr std::array<PianoCharacteristics, kNumPianos> kChars = {{
    // --- Concert ---
    // Steinway D: balanced, rich, crystalline treble, strong duplex resonance
    //                    B scale hard  sbQ  detune partials body  bdamp  symp  d1   d2   sust  damp  elec  duplex prompt after rails  susDecay
    makePianoCharacteristics(1.00f, 0.51f, 13.0f, 0.000f, 32, 0.98f, 0.28f, 12.0f,
                             0.34f, 2.45f, 0.30f, true, false, 0.92f, 0.16f, 0.54f, 1.00f, 0.999840f),
    // Bosendorfer Imperial: warm, deep bass, Viennese — very long sustain
    makePianoCharacteristics(0.76f, 0.38f, 17.5f, 0.000f, 32, 0.72f, 0.16f, 12.0f,
                             0.27f, 3.35f, 0.38f, true, false, 0.42f, 0.08f, 0.66f, 0.92f, 0.999930f),
    // Yamaha CFX: bright, articulate, modern, prominent duplex
    makePianoCharacteristics(1.32f, 0.64f, 8.5f, 0.000f, 30, 1.08f, 0.38f, 12.0f,
                             0.45f, 1.70f, 0.22f, true, false, 1.08f, 0.24f, 0.38f, 0.45f, 0.999700f),
    // --- Vintage ---
    // Bastringue: detuned honky-tonk — shorter sustain (worn strings)
    makePianoCharacteristics(1.10f, 0.48f, 8.0f, 0.025f, 24, 1.00f, 0.35f, 12.0f,
                             0.45f, 1.8f, 0.22f, true, false, 0.20f, 0.08f, 0.40f, 0.55f, 0.999500f),
    // Piano Prepare: prepared piano, metallic, percussive — faster decay
    makePianoCharacteristics(1.25f, 0.65f, 6.0f, 0.020f, 24, 1.20f, 0.15f, 7.0f,
                             0.55f, 1.5f, 0.18f, false, false, 0.40f, 0.20f, 0.35f, 0.0f, 0.999200f),
    // --- Electric --- (no duplex, simple envelope)
    // Rhodes Mark I: warm tine, bell-like — moderate decay
    makePianoCharacteristics(0.15f, 0.35f, 0.0f, 0.000f, 8, 1.00f, 0.40f, 12.0f,
                             0.30f, 3.5f, 0.35f, true, true, 0.0f, 0.12f, 0.50f, 0.0f, 0.999400f),
    // Wurlitzer 200A: reedy, bark on attack — quicker decay
    makePianoCharacteristics(0.20f, 0.45f, 0.0f, 0.000f, 8, 1.00f, 0.30f, 12.0f,
                             0.40f, 2.5f, 0.28f, true, true, 0.0f, 0.12f, 0.50f, 0.0f, 0.999300f),
    // Clavinet D6: percussive, funky — fastest decay
    makePianoCharacteristics(0.08f, 0.75f, 0.0f, 0.000f, 10, 0.90f, 0.18f, 12.0f,
                             0.60f, 1.2f, 0.15f, true, true, 0.0f, 0.12f, 0.50f, 0.0f, 0.999000f),
}};

// =========================================================================
// Default settings per piano
// =========================================================================
constexpr std::array<PianoSettings, kNumPianos> kDefaults = {{
    // Steinway D
    { 0.82f, 0.0f, 0.50f, 0.0028f, 5.00f, 0.22f, 0.40f, 0.50f, 0.35f, 0.55f, 0.40f, 0.50f, 14000.0f, 0.0f },
    // Bosendorfer Imperial
    { 0.80f, 0.0f, 0.42f, 0.0034f, 3.50f, 0.28f, 0.25f, 0.42f, 0.40f, 0.65f, 0.35f, 0.55f, 11000.0f, 0.0f },
    // Yamaha CFX
    { 0.84f, 0.0f, 0.58f, 0.0021f, 4.50f, 0.20f, 0.35f, 0.60f, 0.30f, 0.48f, 0.42f, 0.48f, 16000.0f, 0.0f },
    // Bastringue
    { 0.80f, 0.0f, 0.50f, 0.0026f, 3.00f, 0.15f, 0.30f, 0.55f, 0.20f, 0.40f, 0.50f, 0.70f,  9000.0f, 0.0f },
    // Piano Prepare
    { 0.78f, 0.0f, 0.65f, 0.0012f, 0.40f, 0.08f, 0.35f, 0.45f, 0.15f, 0.30f, 0.55f, 0.75f,  8000.0f, 0.0f },
    // Rhodes Mark I
    { 0.80f, 0.0f, 0.38f, 0.003f, 4.00f, 0.30f, 0.50f, 0.35f, 0.10f, 0.00f, 0.30f, 0.50f, 10000.0f, 0.0f },
    // Wurlitzer 200A
    { 0.82f, 0.0f, 0.48f, 0.003f, 3.00f, 0.25f, 0.40f, 0.45f, 0.08f, 0.00f, 0.40f, 0.55f, 11000.0f, 0.0f },
    // Clavinet D6
    { 0.84f, 0.0f, 0.72f, 0.001f, 1.20f, 0.10f, 0.12f, 0.55f, 0.05f, 0.00f, 0.45f, 0.65f, 14000.0f, 0.0f },
}};

// =========================================================================
// Piano descriptions (100-120 words, French, UTF-8)
// Voix produit unique: ton modeste, axe "sert a" plutot que "ressemble a",
// pas de mention de concours ou d'artistes celebres.
// =========================================================================
constexpr std::array<const char*, kNumPianos> kDescriptions = {{
    // Steinway D
    "Couleur de concert polyvalente pour la production moderne. Servie comme "
    "reference centrale dans les accords ouverts, les arp\xC3\xA8ges serr\xC3\xA9s et les "
    "accompagnements clairs. Le grave reste contenu, le medium tient sa place dans "
    "le mix sans dominer, et l'aigu reste lisible sans brillance artificielle. "
    "Les presets Reference et Signature sont con\xC3\xA7us pour aller directement en "
    "production : Reference reste sec, Signature pousse un peu plus la proximit\xC3\xA9 "
    "et l'attaque. Ce n'est pas une \xC3\xA9mulation premium ; c'est une couleur stable.",

    // Bosendorfer Imperial
    "Direction de timbre plutot qu'\xC3\xA9mulation : un corps plus dense dans le "
    "grave, une rondeur plus sombre, et un sustain qui s'\xC3\xA9tend sans devenir "
    "flou. Sert surtout pour les ballades, les textures cin\xC3\xA9matographiques et "
    "les passages o\xC3\xB9 le piano doit porter une \xC3\xA9motion sans agresser. "
    "Reference reste sec et lisible ; Signature creuse le bas-medium et rallonge "
    "le release pour les prises plus enveloppantes. Le moteur ne cherche pas "
    "\xC3\xA0 \xC3\xAA""tre un Imperial haut de gamme, il en garde l'esprit.",

    // Yamaha CFX
    "Couleur de concert moderne et articulate. Le medium reste droit, l'aigu "
    "perle sans duret\xC3\xA9, et l'attaque est plus nette que sur les autres "
    "concerts. Sert pour les morceaux qui ont besoin de presence et de "
    "clart\xC3\xA9 dans un mix d\xC3\xA9j\xC3\xA0 charg\xC3\xA9, ou pour les solos de "
    "piano qui doivent couper sans harsh. Reference reste sec ; Signature "
    "ouvre un peu plus le haut du spectre et serre l'attaque pour les "
    "passages incisifs. Utile en pop, jazz moderne et cin\xC3\xA9ma.",

    // Bastringue
    "Couleur honky-tonk assum\xC3\xA9""e : cordes l\xC3\xA9g\xC3\xA8rement d\xC3\xA9saccord\xC3\xA9""es, "
    "medium plus \xC3\xA9touff\xC3\xA9, sustain court. Sert pour les ambiances saloon, "
    "le boogie, le rock fifties, et tout ce qui a besoin d'une couleur piano "
    "imm\xC3\xA9""diatement reconnaissable. Reference garde la couleur mais reste sec "
    "et propre pour la production ; Signature pousse le d\xC3\xA9saccord et "
    "raccourcit le release pour les passages les plus bruts. La couleur est "
    "exag\xC3\xA9r\xC3\xA9""e \xC3\xA0 dessein, c'est l'int\xC3\xA9r\xC3\xAA""t du mod\xC3\xA8""le.",

    // Piano Prepare
    "Couleur piano pr\xC3\xA9par\xC3\xA9 abstraite et percussive. Le timbre de chaque note "
    "est transform\xC3\xA9, les mediums deviennent m\xC3\xA9talliques, les graves plus "
    "sourds, et le sustain globalement plus court. Sert pour les ambiances "
    "contemporaines, les textures cin\xC3\xA9matographiques, et les passages qui "
    "ont besoin d'un piano non-identifiable. Reference garde les pr\xC3\xA9parations "
    "lisibles ; Signature les pousse plus loin en attaque et en dampening. "
    "Le mod\xC3\xA8""le assume d'\xC3\xAA""tre non-standard, c'est ce qu'on lui demande.",

    // Rhodes Mark I
    "Couleur Rhodes Mark I : son chaud et rond, attaque en cloche, sustain "
    "long et doux. Sert pour les ballades, le jazz, la soul, les ambiances "
    "vintage. Reference reste propre avec un son clair et neutre, pour aller "
    "dans un mix sans dominer. Signature pousse la chaleur, ajoute un "
    "l\xC3\xA9ger chorus suitcase et un tremolo discret pour les ambiances plus "
    "caract\xC3\xA9ris\xC3\xA9""es. Les deux restent dans la palette Rhodes sans tomber "
    "dans le clich\xC3\xA9 funk.",

    // Wurlitzer 200A
    "Couleur Wurlitzer 200A : son reed bark caract\xC3\xA9ristique, attaque mordante, "
    "medium plus agressif que le Rhodes. Sert pour les couleurs vintage, le "
    "rock, la pop avec \xC3\xA2me, et les solos qui doivent percer. Reference reste "
    "s\xC3\xA8""che et contr\xC3\xB4l\xC3\xA9""e, id\xC3\xA9""ale pour les accompagnements. Signature "
    "ouvre la drive, pousse le mid et le haut pour les passages plus incisifs "
    "et plus expressifs. La couleur reed reste identifiable sans \xC3\xAAtre "
    "grotesque.",

    // Clavinet D6
    "Couleur Clavinet D6 : attaque tranchante, sustain court, son percussif "
    "et funky. Sert pour les lignes funk, les riffs syncop\xC3\xA9s, les stabs et "
    "tout ce qui a besoin d'un piano qui claque. Reference reste proche du "
    "son brut de l'instrument. Signature pousse l'attaque, raccourcit "
    "le release et booste la pr\xC3\xA9sence pour les passages les plus affirm\xC3\xA9s. "
    "Les deux gardent le caract\xC3\xA8""re court et percussif qui fait l'identit\xC3\xA9 "
    "du mod\xC3\xA8le.",
}};

} // namespace

Family getFamily(const int pianoIndex)
{
    const auto idx = std::clamp(pianoIndex, 0, kNumPianos - 1);
    for (int f = kNumFamilies - 1; f > 0; --f)
        if (idx >= kFamilyStart[f]) return static_cast<Family>(f);
    return Family::Concert;
}

int getFamilyStartIndex(const Family family)
{
    return kFamilyStart[std::clamp(static_cast<int>(family), 0, kNumFamilies - 1)];
}

const char* getFamilyName(const int familyIndex)
{
    return kFamilyNames[static_cast<std::size_t>(std::clamp(familyIndex, 0, kNumFamilies - 1))];
}

const char* getPianoName(const int pianoIndex)
{
    return kNames[static_cast<std::size_t>(std::clamp(pianoIndex, 0, kNumPianos - 1))];
}

const char* getPianoShortName(const int pianoIndex)
{
    return kShortNames[static_cast<std::size_t>(std::clamp(pianoIndex, 0, kNumPianos - 1))];
}

const PianoCharacteristics& getCharacteristics(const int pianoIndex)
{
    return kChars[static_cast<std::size_t>(std::clamp(pianoIndex, 0, kNumPianos - 1))];
}

PianoSettings getDefaultSettings(const int pianoIndex)
{
    return kDefaults[static_cast<std::size_t>(std::clamp(pianoIndex, 0, kNumPianos - 1))];
}

const char* getPianoDescription(const int pianoIndex)
{
    return kDescriptions[static_cast<std::size_t>(std::clamp(pianoIndex, 0, kNumPianos - 1))];
}

// =========================================================================
// FX availability per piano
// =========================================================================
namespace {
  constexpr FxAvailability FX(bool saturator,
                bool transient,
                bool compressor,
                bool eq,
                bool chorus,
                bool delay,
                bool reverb,
                bool limiter)
  {
    return FxAvailability{ saturator, transient, compressor, eq, chorus, delay, reverb, limiter };
  }

  //                           Sat    Trans  Comp   EQ     Chor   Delay  Reverb Limit
  constexpr std::array<FxAvailability, kNumPianos> kFxAvailability = {{
    FX(false, true,  true,  true,  false, true,  true,  true),   // 0: Steinway D
    FX(false, true,  true,  true,  false, true,  true,  true),   // 1: Bösendorfer
    FX(false, true,  true,  true,  false, true,  true,  true),   // 2: Yamaha CFX
    FX(true,  true,  true,  true,  true,  true,  true,  true),   // 3: Bastringue
    FX(true,  true,  true,  true,  true,  true,  true,  true),   // 4: Piano Préparé
    FX(true,  false, true,  true,  true,  true,  true,  true),   // 5: Rhodes Mark I
    FX(true,  false, true,  true,  true,  true,  true,  true),   // 6: Wurlitzer 200A
    FX(true,  false, true,  true,  true,  true,  true,  true),   // 7: Clavinet D6
  }};
} // anon

const FxAvailability& getFxAvailability(int pianoIndex)
{
    return kFxAvailability[static_cast<std::size_t>(std::clamp(pianoIndex, 0, kNumPianos - 1))];
}

bool isFxAvailable(int pianoIndex, GlobalFxSlot slot)
{
    const auto& fx = getFxAvailability(pianoIndex);
    switch (slot)
    {
        case GlobalFxSlot::Saturator:  return fx.saturator;
        case GlobalFxSlot::Transient:  return fx.transient;
        case GlobalFxSlot::Compressor: return fx.compressor;
        case GlobalFxSlot::Eq:         return fx.eq;
        case GlobalFxSlot::Chorus:     return fx.chorus;
        case GlobalFxSlot::Delay:      return fx.delay;
        case GlobalFxSlot::Reverb:     return fx.reverb;
        case GlobalFxSlot::Limiter:    return fx.limiter;
        default:                       return true;
    }
}

GlobalFxSettings maskUnavailableFx(int pianoIndex, const GlobalFxSettings& fx)
{
    auto masked = fx;
    const auto& a = getFxAvailability(pianoIndex);
    masked.saturationEnabled = a.saturator  && masked.saturationEnabled;
    masked.transientEnabled  = a.transient  && masked.transientEnabled;
    masked.compressorEnabled = a.compressor && masked.compressorEnabled;
    masked.eqEnabled         = a.eq         && masked.eqEnabled;
    masked.chorusEnabled     = a.chorus     && masked.chorusEnabled;
    masked.delayEnabled      = a.delay      && masked.delayEnabled;
    masked.reverbEnabled     = a.reverb     && masked.reverbEnabled;
    masked.limiterEnabled    = a.limiter    && masked.limiterEnabled;
    return masked;
}

const std::array<PianoInstrumentModel, kNumPianos>& getPianoInstrumentModels() noexcept
{
    return kPianoInstrumentModels;
}

const PianoInstrumentModel& getPianoInstrumentModel(const int pianoIndex) noexcept
{
    return kPianoInstrumentModels[static_cast<std::size_t>(std::clamp(pianoIndex, 0, kNumPianos - 1))];
}

PianoInstrumentAlgorithm getPianoInstrumentAlgorithm(const int pianoIndex) noexcept
{
    return getPianoInstrumentModel(pianoIndex).algorithm;
}

const char* getPianoInstrumentAlgorithmName(const PianoInstrumentAlgorithm algorithm) noexcept
{
    switch (algorithm)
    {
        case PianoInstrumentAlgorithm::SteinwayConcertDuplexSoundboard: return "steinway_concert_duplex_soundboard";
        case PianoInstrumentAlgorithm::BosendorferImperialBassResonator: return "bosendorfer_imperial_bass_resonator";
        case PianoInstrumentAlgorithm::YamahaCfxBrightScaleAction: return "yamaha_cfx_bright_scale_action";
        case PianoInstrumentAlgorithm::BastringueHonkyTonkTackRail: return "bastringue_honky_tonk_tack_rail";
        case PianoInstrumentAlgorithm::PreparedPianoObjectBuzz: return "prepared_piano_object_buzz";
        case PianoInstrumentAlgorithm::RhodesTinePickupBark: return "rhodes_tine_pickup_bark";
        case PianoInstrumentAlgorithm::WurlitzerReedAmpBite: return "wurlitzer_reed_amp_bite";
        case PianoInstrumentAlgorithm::ClavinetPickupStringSnap: return "clavinet_pickup_string_snap";
    }

    return "unknown";
}

const char* getPianoEngineReadinessName(const PianoEngineReadiness readiness) noexcept
{
    switch (readiness)
    {
        case PianoEngineReadiness::TargetOnly: return "target_only";
        case PianoEngineReadiness::DedicatedVoice: return "dedicated_voice";
    }

    return "unknown";
}

const char* getPianoRenderEngineModeName(const PianoRenderEngineMode mode) noexcept
{
    switch (mode)
    {
        case PianoRenderEngineMode::LegacyFamily: return "legacy_family";
        case PianoRenderEngineMode::V2: return "v2";
        case PianoRenderEngineMode::V2ModelOnly: return "v2_model_only";
    }

    return "unknown";
}

PianoRenderEngineMode getPianoRenderEngineMode() noexcept
{
    const auto value = gPianoRenderEngineMode.load(std::memory_order_relaxed);
    if (value < static_cast<int>(PianoRenderEngineMode::LegacyFamily)
        || value > static_cast<int>(PianoRenderEngineMode::V2ModelOnly))
        return PianoRenderEngineMode::V2;

    return static_cast<PianoRenderEngineMode>(value);
}

void setPianoRenderEngineMode(const PianoRenderEngineMode mode) noexcept
{
    gPianoRenderEngineMode.store(static_cast<int>(mode), std::memory_order_relaxed);
}

bool isPianoDedicatedVoiceActive() noexcept
{
    return getPianoRenderEngineMode() != PianoRenderEngineMode::LegacyFamily;
}

} // namespace mps
