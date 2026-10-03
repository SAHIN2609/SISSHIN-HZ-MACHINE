#pragma once
#include <algorithm>
#include <cmath>

namespace hz
{
constexpr double kPi = 3.14159265358979323846;

// Allocation-free biquad (RBJ cookbook), safe to retune on the audio thread.
struct Biquad
{
    enum Type { LP, HP, LowShelf, HighShelf, Peak };
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }
    inline float process(float x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }
    void set(double nb0, double nb1, double nb2, double a0, double na1, double na2)
    {
        b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0;
    }
    void design(Type t, double sr, double f, double q, double gainDb = 0)
    {
        f = std::clamp(f, 10.0, sr * 0.45);
        const double w = 2 * kPi * f / sr, cw = std::cos(w), sw = std::sin(w);
        const double A = std::pow(10.0, gainDb / 40.0);
        const double al = sw / (2 * q);
        const double sa = 2 * std::sqrt(A) * (sw / 2 * std::sqrt(2.0)); // shelf slope S = 1
        switch (t)
        {
            case LP:   set((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + al, -2 * cw, 1 - al); break;
            case HP:   set((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al); break;
            case Peak: set(1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A); break;
            case LowShelf:
                set(A * ((A + 1) - (A - 1) * cw + sa), 2 * A * ((A - 1) - (A + 1) * cw),
                    A * ((A + 1) - (A - 1) * cw - sa), (A + 1) + (A - 1) * cw + sa,
                    -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - sa);
                break;
            case HighShelf:
                set(A * ((A + 1) + (A - 1) * cw + sa), -2 * A * ((A - 1) + (A + 1) * cw),
                    A * ((A + 1) + (A - 1) * cw - sa), (A + 1) - (A - 1) * cw + sa,
                    2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - sa);
                break;
        }
    }
};

// Simple feed-forward compressor, used for amp "sag" and the output limiter.
struct Comp
{
    float thDb = -10, ratio = 2, atk = .005f, rel = .15f, aC = 0, rC = 0, grDb = 0;
    double sr = 48000;
    void prepare(double s) { sr = s; update(); }
    void set(float th, float r, float a, float rl) { thDb = th; ratio = r; atk = a; rel = rl; update(); }
    void update() { aC = (float) std::exp(-1.0 / (atk * sr)); rC = (float) std::exp(-1.0 / (rel * sr)); }
    inline float process(float x)
    {
        const float lvl = 20.f * std::log10(std::fabs(x) + 1e-9f);
        const float target = lvl > thDb ? (lvl - thDb) * (1.f - 1.f / ratio) : 0.f;
        const float c = target > grDb ? aC : rC;
        grDb = target + (grDb - target) * c;
        return x * std::exp(-grDb * 0.11512925f);
    }
};

inline float dbToLin(float db) { return std::pow(10.f, db / 20.f); }

// Asymmetric tanh waveshaper; 'a' adds even harmonics, 'k' sets hardness.
inline float shape(float x, float a, float k)
{
    x = std::clamp(x, -1.f, 1.f);
    return (std::tanh(k * x + a) - std::tanh(a)) * 0.8f;
}
} // namespace hz
