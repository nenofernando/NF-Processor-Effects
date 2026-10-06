#pragma once
// The 32 effects of the processor, their knob definitions, value ranges and display formatting.
// Ranges and units follow the factory manual (reverb controls p.16, delay controls p.18, dynamics p.19, modulation p.20-22, appendix p.39).
#include <cmath>
#include <cstdio>
#include <string>

namespace nfpe
{
enum Fx
{
    SmallHall = 0, LargeHall, SmallPlate, LargePlate, Room, Chamber, Gated, ReverseReverb,
    VocalHall, VocalPlate, DrumHall, DrumPlate, Ambience, Studio, Arena, Spring,
    StudioDelay, DigitalDelay, TapeDelay, PongDelay, ModDelay, ReverseDelay, DeEsser, Compressor,
    Chorus, Flanger, Phaser, TremoloPan, Rotary, Vibrato, PitchShift, Detune,
    kNumEffects
};

enum class Io { Stereo, MonoInStereoOut, MonoWet, StereoWet };
enum KnobKind { kPlain, kMs, kDelayTime, kFeedback, kDbNeg, kRatio, kDeEssFreq, kDeEssBw, kUpDown, kCents, kGainDb };

struct KnobDef { const char* name; int kind; float a, b; };   // a..b: value range (kMs: b = max ms; kDbNeg: a..b dB)

struct EffectInfo
{
    const char* name;
    Io io;
    bool wetOnly;       // the Mix knob is a level control (no dry added)
    bool usesTempo;
    KnobDef knob[3];
    float def[3];       // knob positions (0..1) loaded when the effect is selected
};

inline const EffectInfo* effectTable()
{
    static const EffectInfo t[kNumEffects] =
    {
        { "Small Hall",   Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 100}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.10f, 0.45f, 0.55f} },
        { "Large Hall",   Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 200}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.06f, 0.50f, 0.55f} },
        { "Small Plate",  Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 100}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.05f, 0.45f, 0.60f} },
        { "Large Plate",  Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 200}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.05f, 0.50f, 0.60f} },
        { "Room",         Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 100}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.05f, 0.40f, 0.50f} },
        { "Chamber",      Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 100}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.08f, 0.45f, 0.50f} },
        { "Gated",        Io::MonoInStereoOut, false, false, { {"Pre Delay", kMs, 0, 200}, {"Decay", kPlain, 0, 60}, {"Diffusion", kPlain, 0, 99} }, {0.00f, 0.50f, 0.70f} },
        { "Reverse",      Io::MonoInStereoOut, false, false, { {"Pre Delay", kMs, 0, 200}, {"Decay", kPlain, 0, 60}, {"Diffusion", kPlain, 0, 99} }, {0.00f, 0.45f, 0.70f} },
        { "Vocal Hall",   Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 200}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.08f, 0.50f, 0.50f} },
        { "Vocal Plate",  Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 200}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.05f, 0.50f, 0.60f} },
        { "Drum Hall",    Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 100}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.05f, 0.45f, 0.60f} },
        { "Drum Plate",   Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 100}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.03f, 0.40f, 0.65f} },
        { "Ambience",     Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 100}, {"Shape", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.05f, 0.40f, 0.50f} },
        { "Studio",       Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 100}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.05f, 0.40f, 0.50f} },
        { "Arena",        Io::Stereo, false, false, { {"Pre Delay", kMs, 0, 200}, {"Decay", kPlain, 0, 99}, {"Liveliness", kPlain, 0, 99} }, {0.08f, 0.50f, 0.40f} },
        { "Spring",       Io::MonoInStereoOut, false, false, { {"Pre Delay", kMs, 0, 200}, {"Decay", kPlain, 0, 99}, {"Boing", kPlain, 0, 99} }, {0.00f, 0.50f, 0.50f} },

        { "Studio Delay",  Io::Stereo, false, true, { {"Time Range", kDelayTime, 0, 72}, {"Feedback", kFeedback, 0, 100}, {"Duck Thresh", kDbNeg, -70, 0} }, {0.50f, 0.35f, 1.00f} },
        { "Digital Delay", Io::MonoInStereoOut, false, true, { {"Time Range", kDelayTime, 0, 72}, {"Feedback", kFeedback, 0, 100}, {"Duck Thresh", kDbNeg, -70, 0} }, {0.50f, 0.35f, 1.00f} },
        { "Tape Delay",    Io::MonoInStereoOut, false, true, { {"Time Range", kDelayTime, 0, 72}, {"Feedback", kPlain, 0, 99}, {"Smear", kPlain, 0, 99} }, {0.50f, 0.35f, 0.30f} },
        { "Pong Delay",    Io::MonoInStereoOut, false, true, { {"Time Range", kDelayTime, 0, 72}, {"Feedback", kFeedback, 0, 100}, {"Tap Ratio", kPlain, 0, 23} }, {0.50f, 0.35f, 0.52f} },
        { "Mod Delay",     Io::Stereo, false, true, { {"Time Range", kDelayTime, 0, 72}, {"Feedback", kPlain, 0, 99}, {"Mod Depth", kPlain, 0, 99} }, {0.50f, 0.30f, 0.40f} },
        { "Reverse Delay", Io::MonoInStereoOut, false, true, { {"Time Range", kDelayTime, 0, 72}, {"Feedback", kPlain, 0, 99}, {"Smear", kPlain, 0, 99} }, {0.50f, 0.35f, 0.30f} },
        { "De-Esser",      Io::MonoWet, true, false, { {"Ratio", kRatio, 0, 99}, {"Freq Range", kDeEssFreq, 0, 40}, {"Bandwidth", kDeEssBw, 0, 5} }, {0.80f, 0.60f, 0.60f} },
        { "Compressor",    Io::MonoWet, true, false, { {"Threshold", kDbNeg, -70, 0}, {"Ratio", kRatio, 0, 99}, {"Gain", kGainDb, 0, 12} }, {0.60f, 0.65f, 0.25f} },
        { "Chorus",        Io::Stereo, false, false, { {"Speed", kPlain, 0, 99}, {"Depth", kPlain, 0, 99}, {"Voices", kPlain, 0, 99} }, {0.30f, 0.50f, 0.35f} },
        { "Flanger",       Io::Stereo, false, false, { {"Speed", kPlain, 0, 99}, {"Depth", kPlain, 0, 99}, {"Regen", kPlain, 0, 99} }, {0.30f, 0.60f, 0.50f} },
        { "Phaser",        Io::Stereo, false, false, { {"Speed", kPlain, 0, 99}, {"Depth", kPlain, 0, 99}, {"Regen", kPlain, 0, 99} }, {0.30f, 0.60f, 0.40f} },
        { "Tremolo/Pan",   Io::StereoWet, true, false, { {"Speed", kPlain, 0, 99}, {"Depth", kPlain, 0, 99}, {"Phase", kPlain, 0, 99} }, {0.45f, 0.70f, 0.00f} },
        { "Rotary",        Io::MonoWet, true, false, { {"Speed", kPlain, 0, 99}, {"Doppler", kPlain, 0, 99}, {"Spread", kPlain, 0, 99} }, {0.45f, 0.50f, 0.60f} },
        { "Vibrato",       Io::StereoWet, true, false, { {"Speed", kPlain, 0, 99}, {"Depth", kPlain, 0, 99}, {"Phase", kPlain, 0, 99} }, {0.45f, 0.40f, 0.00f} },
        { "Pitch Shift",   Io::Stereo, false, false, { {"Shift", kPlain, 0, 24}, {"Up/Down", kUpDown, 0, 1}, {"Cutoff Freq", kPlain, 0, 55} }, {0.50f, 0.75f, 1.00f} },
        { "Detune",        Io::Stereo, false, false, { {"Left Shift", kCents, 0, 48}, {"Right Shift", kCents, 0, 48}, {"Spread", kPlain, 0, 99} }, {0.70f, 0.30f, 0.80f} },
    };
    return t;
}
inline const EffectInfo& effectInfo(int fx) { return effectTable()[fx < 0 ? 0 : (fx >= kNumEffects ? kNumEffects - 1 : fx)]; }
inline bool isReverbType(int fx) { return fx >= 0 && fx < 16; }

// Time Range 0..72: 36 = twelve o'clock = one quarter note (appendix p.39). Returns the delay length as a fraction of a quarter note.
inline float delayRatio(float v)
{
    if (v <= 36.0f) return 0.25f + v / 48.0f;
    return 1.0f + (v - 36.0f) / 36.0f;
}
inline const char* noteName(int v)
{
    switch (v) { case 0: return "1/16"; case 4: return "1/8T"; case 6: return "1/16."; case 12: return "1/8"; case 20: return "1/4T";
                 case 24: return "1/8."; case 36: return "1/4"; case 48: return "1/2T"; case 54: return "1/4."; case 72: return "1/2"; default: return ""; }
}

inline float deEssFreqHz(int i)   // 41 steps, 1600 Hz .. 10 kHz (appendix p.39)
{
    static const float f[41] = { 1600, 1680, 1760, 1840, 1920, 2000, 2100, 2200, 2300, 2400, 2500, 2630, 2760, 2890, 3020, 3150, 3320, 3490, 3660, 3830,
                                 4000, 4200, 4400, 4600, 4800, 5000, 5260, 5520, 5780, 6040, 6300, 6640, 6980, 7320, 7660, 8000, 8400, 8800, 9200, 9600, 10000 };
    return f[i < 0 ? 0 : (i > 40 ? 40 : i)];
}
inline float deEssBwOctaves(int i)   // 1/4 octave (0) .. 1/24 octave (5)
{
    static const float b[6] = { 1.0f / 4, 1.0f / 8, 1.0f / 12, 1.0f / 16, 1.0f / 20, 1.0f / 24 };
    return b[i < 0 ? 0 : (i > 5 ? 5 : i)];
}
// 1:1 .. infinity
inline float ratioFromKnob(float k) { return k >= 0.995f ? 1.0e6f : 1.0f / (1.0f - k); }

inline int knobSteps(const KnobDef& d) { return d.kind == kUpDown ? 2 : (d.kind == kDeEssFreq ? 41 : (d.kind == kDeEssBw ? 6 : 0)); }

// Integer shown on the two-digit display while a knob is turned.
inline int knobDisplayInt(int fx, int i, float k)
{
    const KnobDef& d = effectInfo(fx).knob[i];
    switch (d.kind)
    {
        case kMs:        return (int) std::lround(k * d.b);
        case kRatio:     return (int) std::lround(k * 99.0f);
        case kDeEssFreq: return (int) std::lround(k * 40.0f) + 1;
        case kDeEssBw:   return (int) std::lround(k * 5.0f);
        case kUpDown:    return k >= 0.5f ? 1 : 0;
        case kDbNeg:     return (int) std::lround(-(d.a + k * (d.b - d.a)));
        case kFeedback:  return (int) std::lround(k * 100.0f);   // 100 = repeat hold
        default:         return (int) std::lround(d.a + k * (d.b - d.a));
    }
}

inline std::string knobText(int fx, int i, float k)
{
    const KnobDef& d = effectInfo(fx).knob[i];
    char b[48];
    const int v = knobDisplayInt(fx, i, k);
    switch (d.kind)
    {
        case kMs:        std::snprintf(b, sizeof b, "%d ms", v); break;
        case kDelayTime: { const char* n = noteName(v); if (*n) std::snprintf(b, sizeof b, "%d  %s", v, n); else std::snprintf(b, sizeof b, "%d", v); break; }
        case kFeedback:  if (v >= 100) std::snprintf(b, sizeof b, "Hold"); else std::snprintf(b, sizeof b, "%d", v); break;
        case kDbNeg:     if (v == 0) std::snprintf(b, sizeof b, "0 dB"); else std::snprintf(b, sizeof b, "-%d dB", v); break;
        case kRatio:     { const float r = ratioFromKnob(k); if (r > 1000.0f) std::snprintf(b, sizeof b, "Inf:1"); else std::snprintf(b, sizeof b, "%.1f:1", r); break; }
        case kDeEssFreq: std::snprintf(b, sizeof b, "%d Hz", (int) deEssFreqHz(v - 1)); break;
        case kDeEssBw:   { const int den[6] = { 4, 8, 12, 16, 20, 24 }; std::snprintf(b, sizeof b, "1/%d oct", den[v]); break; }
        case kUpDown:    std::snprintf(b, sizeof b, "%s", v ? "Up" : "Down"); break;
        case kCents:     std::snprintf(b, sizeof b, "%+d ct", v - 24); break;
        case kGainDb:    std::snprintf(b, sizeof b, "+%d dB", v); break;
        default:         std::snprintf(b, sizeof b, "%d", v); break;
    }
    return b;
}
}
