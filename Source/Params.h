#pragma once
#include <vector>

// Single source of truth for every parameter. The order here is the order of the
// ParamIndex enum, the APVTS layout, and the web UI's relays.
struct ParamDef { const char* id; const char* name; float mn, mx, def, step; bool isInt; };

enum ParamIndex { INGAIN, GATE, HPF, D_DRIVE, D_TONE, D_LEVEL, D_TIGHT, GAIN, TIGHT, BASS, MID, TREBLE,
                  PRES, SAG, MASTER, CABMIX, OHPF, OLPF, OUT, MODE, CAB, DRVON, OS, NUM_PARAMS };

inline const std::vector<ParamDef>& paramDefs()
{
    static const std::vector<ParamDef> d = {
        {"ingain", "Input gain", 0, 24, 6, .1f, false},
        {"gate", "Gate threshold", -80, -20, -55, 1, false},
        {"hpf", "Input HPF", 20, 300, 70, 1, false},
        {"d_drive", "HZ DRIVE Drive", 0, 10, 4, .1f, false},
        {"d_tone", "HZ DRIVE Tone", 0, 10, 5, .1f, false},
        {"d_level", "HZ DRIVE Level", 0, 10, 6, .1f, false},
        {"d_tight", "HZ DRIVE Tight", 0, 10, 6, .1f, false},
        {"gain", "Gain", 0, 10, 6, .1f, false},
        {"tight", "Tight", 0, 10, 5, .1f, false},
        {"bass", "Bass", 0, 10, 5, .1f, false},
        {"mid", "Mid", 0, 10, 6, .1f, false},
        {"treble", "Treble", 0, 10, 5, .1f, false},
        {"pres", "Presence", 0, 10, 5, .1f, false},
        {"sag", "Sag", 0, 10, 3, .1f, false},
        {"master", "Master", 0, 10, 6, .1f, false},
        {"cabmix", "IR mix", 0, 100, 100, 1, false},
        {"ohpf", "Output HPF", 20, 200, 40, 1, false},
        {"olpf", "Output LPF", 3000, 14000, 8500, 10, false},
        {"out", "Output", -24, 12, -6, .1f, false},
        {"mode", "Amp mode", 0, 3, 2, 1, true},      // 0 Clean 1 Crunch 2 Modern 3 Lead
        {"cab", "Cabinet", 0, 2, 2, 1, true},        // 0 1x12 1 2x12 2 4x12
        {"drvon", "HZ DRIVE on", 0, 1, 1, 1, true},
        {"os", "Oversampling", 0, 1, 1, 1, true},    // 0 = 2x, 1 = 4x
    };
    return d;
}
