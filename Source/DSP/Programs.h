#pragma once
// The 99 factory programs: title, application, the effect in each processor and the routing, as listed in the
// manual (p.40-41). The manual gives no knob values, so every program starts from the effect's default knobs;
// the Serial / Parallel factory bank only changes the Mix settings (see the Utilities chapter).
#include <string>
#include <cstring>
#include "Catalog.h"
#include "Engine.h"

namespace nfpe
{
struct ProgramData
{
    int routing = Cascade;
    int fx[2] = { StudioDelay, VocalHall };
    float knob[2][3] = { { 0.5f, 0.35f, 1.0f }, { 0.08f, 0.5f, 0.5f } };
    float mix[2] = { 0.28f, 0.30f };
    bool bypass[2] = { false, false };
    float bpm[2] = { 120.0f, 120.0f };
};

struct FactoryRow { const char* title; int app; const char* p1; const char* p2; int routing; int bypassMask; };
inline const char* applicationName(int a)
{
    static const char* n[5] = { "Vocal", "Live Sound", "Instrumental", "Drums & Percussion", "Special" };
    return n[a < 0 ? 0 : (a > 4 ? 4 : a)];
}

inline const FactoryRow* factoryRows()
{
    // app: 0 Vocal, 1 Live Sound, 2 Instrumental, 3 Drums & Percussion, 4 Special; bypassMask: 1 = P1 bypassed, 2 = P2 bypassed
    static const FactoryRow r[99] =
    {
        {"BIG POP VOCAL",0,"Studio Delay","Vocal Hall",2,0},{"POP VOCALS",0,"Tape Delay","Studio Reverb",3,0},{"RETROVERB",0,"Tape Delay","Vocal Plate",2,0},
        {"CHORUS + PLATE",0,"Chorus","Plate",4,0},{"VOCAL HALL+PLATE",0,"Vocal Hall","Vocal Plate",4,0},{"EXTRA ROOMY",1,"Room","Small Hall",2,0},
        {"TUNED ROOM",1,"Detune","Room",2,0},{"ECHOVERB",1,"Digital Delay","Small Hall",2,0},{"QUICKIE",1,"Small Hall","Digital Delay",2,0},
        {"ACOUSTIC CHAMBER",2,"Tape Delay","Chamber",2,1},{"AMBIENT CHAMBER",2,"Ambience","Chamber",4,0},{"ACOUSTIC BLISS",2,"Detune","Large Hall",4,0},
        {"GUITAR WORLD",2,"Chorus","Modulated Delay",2,0},{"SPRINGTIME",2,"Tremolo/Pan","Spring",2,1},{"GIANT SNARE",3,"Studio Delay","Drum Hall",2,0},
        {"SMOOTH DRM PLATE",3,"Compressor","Drum Plate",2,0},{"RICH DRUM HALL",3,"Compressor","Drum Hall",2,0},{"BIG TIME",4,"Digital Delay","Large Hall",4,0},
        {"VOICE OF EVIL",4,"Pitch Shifter","Reverse Reverb",2,0},{"WAYBACK MACHINE",4,"Reverse Delay","Reverse Reverb",2,0},{"BIG VOCAL VERB",0,"Large Plate","Vocal Hall",2,0},
        {"TWO ROOMS",0,"Small Hall","Vocal Hall",1,0},{"BIG EASY",0,"Chorus","Vocal Hall",2,0},{"NASHVILLE",0,"Vocal Plate","Room",4,0},
        {"LA-LA LAND",0,"Compressor","Vocal Hall",2,0},{"SMOOTH VOCALS",0,"Compressor","Small Hall",2,0},{"COMPRESSED VOX",0,"Compressor","Vocal Hall",2,0},
        {"NO \"S\" VOCALS",0,"De-Esser","Vocal Plate",2,0},{"DE-ESSER+COMPR",0,"De-Esser","Compressor",2,0},{"COMPRESSOR",0,"Compressor","Compressor",4,2},
        {"DE-ESSER",0,"De-Esser","De-Esser",4,2},{"STUDIO DELAY",0,"Studio Delay","Vocal Hall",2,2},{"60'S DELAY",0,"Tape Delay","Spring",2,0},
        {"SLAPBACK",0,"Detune","Tape Delay",2,1},{"DELAY + VERB",0,"Studio Delay","Vocal Hall",1,0},{"VOCAL CHORUS",0,"Chorus","Vocal Hall",2,2},
        {"CHORAL HALL",0,"Chorus","Large Hall",2,0},{"TAPE CHORUS",0,"Tape Delay","Chorus",2,0},{"CORE US",0,"Chorus","Chorus",1,0},
        {"DOUBLE TRACKS",0,"Detune","Chorus",2,0},{"SHIFTED ROOM",0,"Pitch Shift","Room Reverb",3,0},{"LIKE THAT!",0,"Pitch Shifter","Vocal Hall",4,0},
        {"PLATE + HALL",1,"Vocal Plate","Small Hall",1,0},{"VOCAL + INST 1",1,"Vocal Hall","Studio",4,0},{"VOCAL + INST 2",1,"Vocal Plate","Detune",4,0},
        {"VOCAL + INST 3",1,"Vocal Hall","Small Hall",4,0},{"GET SMALL",1,"Small Hall","Small Plate",4,0},{"ALIVE N' KICKING",1,"Small Hall","Studio",4,0},
        {"MUY GRANDE",1,"Drum Hall","Digital Delay",4,0},{"BIGGER BAND",1,"Chorus","Studio Delay",2,0},{"TAPS",1,"Chorus","Tape Delay",2,0},
        {"TAP CHAMBER",1,"Pong Delay","Chamber",2,0},{"SPACED OUT",1,"Large Plate","Modulated Delay",2,0},{"DUAL HALLS",2,"Large Hall","Small Hall",1,0},
        {"TRIED AND TRUE",2,"Studio","Small Hall",4,0},{"MORE & MORE",2,"Large Plate","Arena",4,0},{"SPRING STUDIO",2,"Spring","Studio",4,0},
        {"HIT ME",2,"Tape Delay","Studio",2,0},{"PRECISION DELAY",2,"Digital Delay","Small Hall",2,0},{"MODULATED DELAY",2,"Chorus","Modulated Delay",2,0},
        {"ROCK LEAD",2,"Tape Delay","Small Plate",4,0},{"PONG VERB",2,"Small Plate","Pong Delay",2,0},{"MOD-U-VERB",2,"Vocal Plate","Chorus",2,0},
        {"ROTO-VERB",2,"Drum Hall","Rotary Speaker",2,0},{"ROTARY",2,"Rotary","Small Room",4,0},{"HAM 'N EGGS",2,"Tape Delay","Rotary",2,0},
        {"VIBROLUX",2,"Rotary Speaker","Spring",1,0},{"SIDE BY SIDE",2,"Tremolo","Panner",4,0},{"WOBBLERS",2,"Chorus","Small Plate",4,0},
        {"RHODES PANNER",2,"Phaser","Tremolo/Panner",4,0},{"BREWED AWAKENING",2,"Flanger","Tape Delay",1,0},{"JET FLANGE",2,"Flanger","Arena",2,0},
        {"PHASE GATE",2,"Gated","Phaser",4,0},{"PHASED PLATE",2,"Phaser","Small Plate",2,0},{"PHAME",2,"Phaser","Large Plate",1,0},
        {"PHASER TASER",2,"Phaser","Digital Delay",2,0},{"DUALING GUITARS",2,"Pitch Shifter","Pitch Shifter",4,0},{"THE LOW DOWN",2,"Pitch Shift","Modulated Delay",1,0},
        {"AND THEN SOME",2,"Reverse Delay","Tremolo",2,0},{"SQUISHED KICK",3,"Compressor","Drum Hall",2,0},{"GATED SNARE",3,"Gated Reverb","Gated Reverb",3,0},
        {"SNARE DELAY",3,"Studio Delay","Drum Plate",2,0},{"DRUM DELAY",3,"Tape Delay","Drum Hall",2,0},{"DRUM ECHO",3,"Tape Delay","Small Hall",2,0},
        {"TEMPO GATE",3,"Studio Delay","Gated",2,0},{"FLANGE DRUMS",3,"Studio Delay","Flanger",2,1},{"TOM-TOM",3,"Studio Delay","Drum Hall",4,0},
        {"SNARE + TOMS",3,"Drum Hall","Room",4,0},{"DRUM PLATE/HALL",3,"Drum Plate","Drum Hall",4,0},{"VIBRATO",4,"Vibrato","Vibrato",4,0},
        {"VIBRA-SLAP",4,"Vibrato","Digital Delay",1,0},{"SPIN CYCLE",4,"Rotary","Tremolo/Panner",2,0},{"ROLE REVERSAL",4,"TremoloPan","Reverse",2,0},
        {"GET SOME",4,"Flanger","Reverse Delay",3,0},{"REWIND",4,"Chamber","Reverse",2,0},{"BACK AT YA",4,"Reverse Delay","Reverse",4,0},
        {"DELAY WARS",4,"Pong Delay","Modulated Delay",4,0},{"PITCH OUT",4,"Pitch Shifter","Pong Delay",2,0},{"HUH?",4,"Pitch Shifter","Vocal Plate",2,0},
    };
    return r;
}

// Name used in the manual's list -> effect id (the list uses a few short names), plus a knob 3 override for the Tremolo / Panner variants.
inline int effectFromName(const char* name, float* knob3Override)
{
    *knob3Override = -1.0f;
    const struct { const char* n; int fx; float k3; } m[] =
    {
        {"Plate", SmallPlate, -1}, {"Studio Reverb", Studio, -1}, {"Room Reverb", Room, -1}, {"Small Room", Room, -1},
        {"Modulated Delay", ModDelay, -1}, {"Rotary Speaker", Rotary, -1}, {"Pitch Shifter", PitchShift, -1},
        {"Tremolo", TremoloPan, 0.0f}, {"Panner", TremoloPan, 1.0f}, {"Tremolo/Panner", TremoloPan, 0.65f}, {"TremoloPan", TremoloPan, 0.65f},
        {"Gated Reverb", Gated, -1}, {"Reverse", ReverseReverb, -1}, {"Reverse Reverb", ReverseReverb, -1},
    };
    for (const auto& e : m) if (std::strcmp(e.n, name) == 0) { *knob3Override = e.k3; return e.fx; }
    for (int i = 0; i < kNumEffects; ++i) if (std::strcmp(effectInfo(i).name, name) == 0) return i;
    return Studio;
}

inline float defaultMix(int fx, bool parallelBank)
{
    if (effectInfo(fx).wetOnly || parallelBank) return 1.0f;
    if (isReverbType(fx)) return 0.30f;
    if (fx >= StudioDelay && fx <= ReverseDelay) return 0.28f;
    return 0.5f;
}

inline ProgramData factoryProgram(int index, bool parallelBank)
{
    const FactoryRow& row = factoryRows()[index < 0 ? 0 : (index > 98 ? 98 : index)];
    ProgramData d;
    d.routing = row.routing - 1;
    const char* names[2] = { row.p1, row.p2 };
    for (int p = 0; p < 2; ++p)
    {
        float k3;
        d.fx[p] = effectFromName(names[p], &k3);
        for (int i = 0; i < 3; ++i) d.knob[p][i] = effectInfo(d.fx[p]).def[i];
        if (k3 >= 0.0f) d.knob[p][2] = k3;
        d.mix[p] = defaultMix(d.fx[p], parallelBank);
        d.bypass[p] = (row.bypassMask >> p) & 1;
        d.bpm[p] = 120.0f;
    }
    return d;
}

inline std::string factoryTitle(int index)
{
    std::string s = factoryRows()[index < 0 ? 0 : (index > 98 ? 98 : index)].title, o;
    bool up = true;
    for (char c : s)
    {
        const bool letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        if (letter) { o += up ? (char) std::toupper((unsigned char) c) : (char) std::tolower((unsigned char) c); up = false; }
        else { o += c; up = (c != '\'' && !(c >= '0' && c <= '9')); }
    }
    return o;
}
}
