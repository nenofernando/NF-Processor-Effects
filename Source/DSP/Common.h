#pragma once
// JUCE-free DSP building blocks shared by every NF Processor Effect algorithm.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace nfpe
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float dbToGain(float db) { return std::pow(10.0f, db * 0.05f); }
inline float gainToDb(float g) { return 20.0f * std::log10(std::max(g, 1.0e-9f)); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float expMap(float t, float lo, float hi) { return lo * std::pow(hi / lo, t); }
inline float flushDenormal(float x) { return std::fabs(x) < 1.0e-18f ? 0.0f : x; }
inline float softClip(float x) { return std::tanh(x); }

// One-pole low-pass (hp = input minus low-pass).
struct OnePole
{
    float a = 1.0f, z = 0.0f;
    void setCutoff(float fc, double sr)
    {
        fc = clampf(fc, 5.0f, (float) (sr * 0.49));
        a = 1.0f - std::exp((float) (-kTwoPi * fc / sr));
    }
    float lp(float x) { z += a * (x - z); z = flushDenormal(z); return z; }
    float hp(float x) { return x - lp(x); }
    void reset() { z = 0.0f; }
};

// RBJ biquad (direct form II transposed).
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0.0f; }
    void setLowpass(float f, float q, double sr) { coeffs(f, q, sr, 0); }
    void setHighpass(float f, float q, double sr) { coeffs(f, q, sr, 1); }
    void setBandpass(float f, float q, double sr) { coeffs(f, q, sr, 2); }
    float process(float x)
    {
        const float y = b0 * x + z1;
        z1 = flushDenormal(b1 * x - a1 * y + z2);
        z2 = flushDenormal(b2 * x - a2 * y);
        return y;
    }
private:
    void coeffs(float f, float q, double sr, int type)
    {
        f = clampf(f, 10.0f, (float) (sr * 0.49));
        const double w = kTwoPi * f / sr, c = std::cos(w), s = std::sin(w), alpha = s / (2.0 * std::max(0.05f, q));
        const double a0 = 1.0 + alpha;
        double nb0, nb1, nb2;
        if (type == 0)      { nb0 = (1 - c) * 0.5; nb1 = 1 - c; nb2 = (1 - c) * 0.5; }
        else if (type == 1) { nb0 = (1 + c) * 0.5; nb1 = -(1 + c); nb2 = (1 + c) * 0.5; }
        else                { nb0 = alpha; nb1 = 0; nb2 = -alpha; }   // band-pass, 0 dB peak
        b0 = (float) (nb0 / a0); b1 = (float) (nb1 / a0); b2 = (float) (nb2 / a0);
        a1 = (float) (-2.0 * c / a0); a2 = (float) ((1.0 - alpha) / a0);
    }
};

// Circular delay line with 4-point Hermite interpolation. read(d): d = delay in samples (>= 1).
class DelayLine
{
public:
    void init(int maxDelaySamples)
    {
        int n = 16;
        while (n < maxDelaySamples + 8) n <<= 1;
        buf.assign((size_t) n, 0.0f);
        mask = n - 1; w = 0; maxDelay = n - 4;
    }
    void clear() { std::fill(buf.begin(), buf.end(), 0.0f); }
    int capacity() const { return maxDelay; }
    inline void push(float x) { w = (w + 1) & mask; buf[(size_t) w] = x; }
    inline float at(int d) const { return buf[(size_t) ((w - d) & mask)]; }
    inline float read(float d) const
    {
        d = d < 1.0f ? 1.0f : (d > (float) (maxDelay - 2) ? (float) (maxDelay - 2) : d);
        const int i = (int) d;
        const float f = d - (float) i;
        const float ym1 = at(i - 1), y0 = at(i), y1 = at(i + 1), y2 = at(i + 2);
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
        return ((c3 * f + c2) * f + c1) * f + y0;
    }
    inline float readLinear(float d) const
    {
        d = d < 1.0f ? 1.0f : (d > (float) (maxDelay - 2) ? (float) (maxDelay - 2) : d);
        const int i = (int) d;
        const float f = d - (float) i;
        return at(i) + (at(i + 1) - at(i)) * f;
    }
private:
    std::vector<float> buf;
    int mask = 15, w = 0, maxDelay = 8;
};

// Schroeder all-pass with an integer delay.
class Allpass
{
public:
    void init(int len) { line.init(len + 4); delay = std::max(1, len); }
    void clear() { line.clear(); }
    inline float process(float x, float g)
    {
        const float d = line.at(delay);
        const float v = x + g * d;
        line.push(v);
        return d - g * v;
    }
private:
    DelayLine line;
    int delay = 1;
};

// First-order all-pass (phaser stages, spring dispersion).
struct Allpass1
{
    float x1 = 0.0f, y1 = 0.0f;
    inline float process(float x, float a)
    {
        const float y = a * x + x1 - a * y1;
        x1 = x; y1 = flushDenormal(y);
        return y;
    }
    void reset() { x1 = y1 = 0.0f; }
};

struct Lfo
{
    double phase = 0.0, inc = 0.0;
    void setRate(double hz, double sr) { inc = hz / sr; }
    void setPhase(double p) { phase = p - std::floor(p); }
    inline void advance() { phase += inc; if (phase >= 1.0) phase -= 1.0; }
    inline float sine(double offset = 0.0) const { return (float) std::sin(kTwoPi * (phase + offset)); }
    inline float tri(double offset = 0.0) const
    {
        double p = phase + offset; p -= std::floor(p);
        return (float) (p < 0.5 ? 4.0 * p - 1.0 : 3.0 - 4.0 * p);
    }
};

// Exponential one-pole smoother.
struct Smooth
{
    float v = 0.0f, coeff = 0.01f;
    void setTime(float ms, double sr) { coeff = 1.0f - std::exp((float) (-1.0 / (std::max(0.05f, ms) * 0.001 * sr))); }
    inline float next(float target) { v += coeff * (target - v); return v; }
    void snap(float x) { v = x; }
};

// Peak/envelope follower with separate attack / release.
struct Follower
{
    float v = 0.0f, ca = 0.0f, cr = 0.0f;
    void set(float attackMs, float releaseMs, double sr)
    {
        ca = std::exp((float) (-1.0 / (std::max(0.01f, attackMs) * 0.001 * sr)));
        cr = std::exp((float) (-1.0 / (std::max(0.01f, releaseMs) * 0.001 * sr)));
    }
    inline float process(float x)
    {
        x = std::fabs(x);
        const float c = x > v ? ca : cr;
        v = x + c * (v - x);
        return v = flushDenormal(v);
    }
    void reset() { v = 0.0f; }
};

// Small deterministic noise source (never allocates, never depends on global state).
struct Noise
{
    uint32_t s = 22222u;
    inline float next() { s = s * 1664525u + 1013904223u; return (float) (int32_t) s * (1.0f / 2147483648.0f); }
};
}
