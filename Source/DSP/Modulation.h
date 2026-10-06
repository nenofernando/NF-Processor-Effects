#pragma once
// Modulated / pitch effects: Chorus, Flanger, Phaser, Tremolo/Pan, Rotary, Vibrato, Pitch Shift, Detune.
// Every function returns the processed (wet) signal; the dry/wet blend is done by the engine.
#include "Common.h"

namespace nfpe
{
class ChorusFx
{
public:
    static constexpr int kVoices = 9;
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (auto& d : line) d.init((int) (0.06 * sr));
        reset();
    }
    void reset()
    {
        for (auto& d : line) d.clear();
        for (int v = 0; v < kVoices; ++v) ph[v] = (double) v / kVoices;
    }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const double rate = expMap(k1, 0.1f, 8.0f);
        const float depthMs = 0.4f + k2 * 5.5f;
        const float voices = k3 * 8.0f;                       // additional voices
        float gain[kVoices]; float norm = 0.0f;
        for (int v = 0; v < kVoices; ++v) { gain[v] = v == 0 ? 1.0f : clampf(voices - (float) (v - 1), 0.0f, 1.0f); norm += gain[v] * gain[v]; }
        norm = 1.0f / std::sqrt(std::max(1.0f, norm));
        const float msS = 0.001f * (float) sr;
        for (int s = 0; s < n; ++s)
        {
            line[0].push(inL[s]); line[1].push(inR[s]);
            float l = 0.0f, r = 0.0f;
            for (int v = 0; v < kVoices; ++v)
            {
                ph[v] += rate * (1.0 + 0.045 * v) / sr; if (ph[v] >= 1.0) ph[v] -= 1.0;
                if (gain[v] <= 0.0f) continue;
                const float centre = (11.0f + 2.1f * (float) v) * msS;
                const float m = std::sin((float) (kTwoPi * ph[v])) * depthMs * msS;
                const float a = line[0].read(centre + m), b = line[1].read(centre - m * 0.9f + 0.7f * msS);
                const float pan = (v & 1) ? 0.35f : 0.65f;   // alternate voices across the image
                l += gain[v] * (a * (1.0f - pan) * 1.4f + b * pan * 0.6f);
                r += gain[v] * (b * (1.0f - pan) * 1.4f + a * pan * 0.6f);
            }
            outL[s] = l * norm; outR[s] = r * norm;
        }
    }
private:
    double sr = 48000.0, ph[kVoices] = {};
    DelayLine line[2];
};

class FlangerFx
{
public:
    void prepare(double sampleRate) { sr = sampleRate; for (auto& d : line) d.init((int) (0.02 * sr)); reset(); }
    void reset() { for (auto& d : line) d.clear(); fb[0] = fb[1] = 0.0f; phase = 0.0; }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const double rate = expMap(k1, 0.05f, 5.0f);
        const float depth = (0.15f + 5.5f * k2) * 0.001f * (float) sr, base = 0.5f * 0.001f * (float) sr;
        const float regen = k3 * 0.9f;
        for (int s = 0; s < n; ++s)
        {
            phase += rate / sr; if (phase >= 1.0) phase -= 1.0;
            const float lfo[2] = { 0.5f + 0.5f * std::sin((float) (kTwoPi * phase)), 0.5f + 0.5f * std::sin((float) (kTwoPi * (phase + 0.25))) };
            const float in[2] = { inL[s], inR[s] };
            float o[2];
            for (int c = 0; c < 2; ++c)
            {
                line[c].push(flushDenormal(in[c] + regen * fb[c]));
                o[c] = line[c].read(base + depth * lfo[c]);
                fb[c] = o[c];
            }
            outL[s] = o[0]; outR[s] = o[1];
        }
    }
private:
    double sr = 48000.0, phase = 0.0; DelayLine line[2]; float fb[2] = {};
};

class PhaserFx
{
public:
    static constexpr int kStages = 8;
    void prepare(double sampleRate) { sr = sampleRate; reset(); }
    void reset() { for (auto& c : st) for (auto& a : c) a.reset(); fb[0] = fb[1] = 0.0f; phase = 0.0; }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const double rate = expMap(k1, 0.05f, 6.0f);
        const float range = 0.25f + 0.75f * k2, regen = k3 * 0.88f;
        for (int s = 0; s < n; ++s)
        {
            phase += rate / sr; if (phase >= 1.0) phase -= 1.0;
            const float in[2] = { inL[s], inR[s] };
            float o[2];
            for (int c = 0; c < 2; ++c)
            {
                const float lfo = 0.5f + 0.5f * std::sin((float) (kTwoPi * (phase + 0.25 * c)));
                const float f = 250.0f * std::pow(3800.0f / 250.0f, 0.5f * (1.0f - range) * 0.0f + lfo * range);
                const float t = std::tan((float) (kPi * f / sr));
                const float a = (t - 1.0f) / (t + 1.0f);
                float x = in[c] + regen * fb[c];
                for (int k = 0; k < kStages; ++k) x = st[c][k].process(x, a);
                fb[c] = flushDenormal(x);
                o[c] = x;
            }
            outL[s] = o[0]; outR[s] = o[1];
        }
    }
private:
    double sr = 48000.0, phase = 0.0; Allpass1 st[2][kStages]; float fb[2] = {};
};

class TremoloPanFx
{
public:
    void prepare(double sampleRate) { sr = sampleRate; reset(); }
    void reset() { phase = 0.0; }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const double rate = expMap(k1, 0.1f, 15.0f);
        for (int s = 0; s < n; ++s)
        {
            phase += rate / sr; if (phase >= 1.0) phase -= 1.0;
            const float a = 0.5f + 0.5f * std::sin((float) (kTwoPi * phase));
            const float b = 0.5f + 0.5f * std::sin((float) (kTwoPi * (phase + 0.5 * k3)));
            outL[s] = inL[s] * (1.0f - k2 * a);
            outR[s] = inR[s] * (1.0f - k2 * b);
        }
    }
private:
    double sr = 48000.0, phase = 0.0;
};

class VibratoFx
{
public:
    void prepare(double sampleRate) { sr = sampleRate; for (auto& d : line) d.init((int) (0.03 * sr)); reset(); }
    void reset() { for (auto& d : line) d.clear(); phase = 0.0; }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const double rate = expMap(k1, 0.2f, 12.0f);
        const float centre = 0.004f * (float) sr, depth = k2 * 0.0035f * (float) sr;
        for (int s = 0; s < n; ++s)
        {
            phase += rate / sr; if (phase >= 1.0) phase -= 1.0;
            line[0].push(inL[s]); line[1].push(inR[s]);
            outL[s] = line[0].read(centre + depth * std::sin((float) (kTwoPi * phase)));
            outR[s] = line[1].read(centre + depth * std::sin((float) (kTwoPi * (phase + 0.5 * k3))));
        }
    }
private:
    double sr = 48000.0, phase = 0.0; DelayLine line[2];
};

// Rotating speaker: crossover at 800 Hz, horn (treble) and drum (bass) rotors with their own inertia,
// Doppler through modulated delays, amplitude modulation and two virtual microphones.
class RotaryFx
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (auto& d : horn) d.init((int) (0.01 * sr));
        for (auto& d : drum) d.init((int) (0.01 * sr));
        reset();
    }
    void reset()
    {
        for (auto& d : horn) d.clear(); for (auto& d : drum) d.clear();
        hornF = 0.8f; drumF = 0.7f; hornPh = drumPh = 0.0;
        for (auto& b : xoLp) b.reset(); for (auto& b : xoHp) b.reset();
        for (auto& b : xoLp) b.setLowpass(800.0f, 0.7071f, sr); for (auto& b : xoHp) b.setHighpass(800.0f, 0.7071f, sr);
    }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const float hornTarget = expMap(k1, 0.5f, 7.2f), drumTarget = hornTarget * 0.88f;
        const float hornCoef = 1.0f - std::exp(-1.0f / (0.9f * (float) sr)), drumCoef = 1.0f - std::exp(-1.0f / (2.6f * (float) sr));
        const float doppler = k2 * 0.00055f * (float) sr, dopplerDrum = doppler * 0.35f;
        const float spread = k3 * (float) kPi;
        const float base = 0.0015f * (float) sr;
        for (int s = 0; s < n; ++s)
        {
            hornF += (hornTarget - hornF) * hornCoef; drumF += (drumTarget - drumF) * drumCoef;
            hornPh += hornF / sr; drumPh += drumF / sr;
            if (hornPh >= 1.0) hornPh -= 1.0; if (drumPh >= 1.0) drumPh -= 1.0;
            const float x = 0.5f * (inL[s] + inR[s]);
            // 4th-order Linkwitz-Riley split (two cascaded 2nd-order sections)
            const float lo = xoLp[1].process(xoLp[0].process(x)), hi = xoHp[1].process(xoHp[0].process(x));
            horn[0].push(hi); drum[0].push(lo);
            float o[2];
            for (int m = 0; m < 2; ++m)
            {
                const double off = m == 0 ? 0.0 : (double) spread / kTwoPi;
                const float ch = std::cos((float) (kTwoPi * (hornPh + off))), cd = std::cos((float) (kTwoPi * (drumPh + off)));
                const float h = horn[0].read(base + doppler * ch) * (1.0f + 0.38f * ch);
                const float d = drum[0].read(base + dopplerDrum * cd) * (1.0f + 0.16f * cd);
                o[m] = h + d;
            }
            outL[s] = o[0] * 0.9f; outR[s] = o[1] * 0.9f;
        }
    }
private:
    double sr = 48000.0, hornPh = 0.0, drumPh = 0.0;
    float hornF = 0.8f, drumF = 0.7f;
    DelayLine horn[1], drum[1];
    Biquad xoLp[2], xoHp[2];
};

// Time-domain pitch shifter (two Hann-crossfaded read heads).
class GranularShifter
{
public:
    void init(double sr, float windowMs)
    {
        win = windowMs * 0.001f * (float) sr;
        line.init((int) (win * 2.2f) + 16);
        clear();
    }
    void clear() { line.clear(); phase = 0.0f; }
    inline float process(float x, float ratio)
    {
        line.push(x);
        phase += (1.0f - ratio) / win;
        if (phase >= 1.0f) phase -= 1.0f; else if (phase < 0.0f) phase += 1.0f;
        float p2 = phase + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
        const float w1 = 0.5f - 0.5f * std::cos(6.28318531f * phase), w2 = 0.5f - 0.5f * std::cos(6.28318531f * p2);
        return line.read(phase * win + 2.0f) * w1 + line.read(p2 * win + 2.0f) * w2;
    }
private:
    DelayLine line; float win = 2400.0f, phase = 0.0f;
};

class PitchShiftFx
{
public:
    void prepare(double sampleRate) { sr = sampleRate; for (auto& g : sh) g.init(sr, 50.0f); reset(); }
    void reset() { for (auto& g : sh) g.clear(); for (auto& f : lp) f.reset(); }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const float semis = std::round(k1 * 24.0f) * (k2 >= 0.5f ? 1.0f : -1.0f);
        const float ratio = std::pow(2.0f, semis / 12.0f);
        const float cut = expMap(std::round(k3 * 55.0f) / 55.0f, 800.0f, 20000.0f);
        for (auto& f : lp) f.setLowpass(cut, 0.7071f, sr);
        for (int s = 0; s < n; ++s)
        {
            outL[s] = lp[0].process(sh[0].process(inL[s], ratio));
            outR[s] = lp[1].process(sh[1].process(inR[s], ratio));
        }
    }
private:
    double sr = 48000.0; GranularShifter sh[2]; Biquad lp[2];
};

class DetuneFx
{
public:
    void prepare(double sampleRate) { sr = sampleRate; for (auto& g : sh) g.init(sr, 40.0f); reset(); }
    void reset() { for (auto& g : sh) g.clear(); }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const float cl = std::round(k1 * 48.0f) - 24.0f, cr = std::round(k2 * 48.0f) - 24.0f;
        const float rl = std::pow(2.0f, cl / 1200.0f), rr = std::pow(2.0f, cr / 1200.0f);
        const float a = 0.5f + 0.5f * k3, b = 0.5f - 0.5f * k3;
        for (int s = 0; s < n; ++s)
        {
            const float vl = sh[0].process(inL[s], rl), vr = sh[1].process(inR[s], rr);
            outL[s] = vl * a + vr * b; outR[s] = vr * a + vl * b;
        }
    }
private:
    double sr = 48000.0; GranularShifter sh[2];
};
}
