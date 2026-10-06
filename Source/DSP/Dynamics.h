#pragma once
// The two dynamics programs: De-Esser (split-band compression of the sibilant region) and Compressor
// (feed-forward RMS detector, soft knee). Both are wet-only: the Mix knob works as a level control.
#include "Common.h"
#include "Catalog.h"

namespace nfpe
{
class DeEsserFx
{
public:
    void prepare(double sampleRate) { sr = sampleRate; env.set(0.2f, 25.0f, sr); gs.setTime(1.0f, sr); reset(); }
    void reset() { bp[0].reset(); bp[1].reset(); env.reset(); gs.snap(1.0f); }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const float ratio = ratioFromKnob(k1);
        const float f = deEssFreqHz((int) std::lround(k2 * 40.0f));
        const float bw = deEssBwOctaves((int) std::lround(k3 * 5.0f));
        // Q from the bandwidth in octaves
        const float q = 1.0f / (2.0f * std::sinh(0.34657359f * bw * (float) (kTwoPi * f / sr) / std::sin((float) (kTwoPi * f / sr))));
        bp[0].setBandpass(f, q, sr); bp[1].setBandpass(f, q, sr);
        const float thr = dbToGain(-32.0f);
        for (int s = 0; s < n; ++s)
        {
            const float bl = bp[0].process(inL[s]), br = bp[1].process(inR[s]);
            const float lvl = env.process(std::max(std::fabs(bl), std::fabs(br)));
            float g = 1.0f;
            if (lvl > thr && ratio > 1.0f)
            {
                const float overDb = gainToDb(lvl / thr);
                const float grDb = overDb * (1.0f - 1.0f / ratio);
                g = dbToGain(-grDb);
            }
            g = gs.next(g);
            outL[s] = inL[s] - bl * (1.0f - g);
            outR[s] = inR[s] - br * (1.0f - g);
        }
    }
private:
    double sr = 48000.0; Biquad bp[2]; Follower env; Smooth gs;
};

class CompressorFx
{
public:
    void prepare(double sampleRate) { sr = sampleRate; rmsCoef = std::exp((float) (-1.0 / (0.010 * sr))); reset(); }
    void reset() { ms = 0.0f; grDb = 0.0f; }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const float thr = -70.0f + k1 * 70.0f, ratio = ratioFromKnob(k2), makeup = dbToGain(k3 * 12.0f);
        const float slope = 1.0f - 1.0f / ratio, knee = 8.0f;
        const float rel = std::exp((float) (-1.0 / (0.12 * sr)));
        for (int s = 0; s < n; ++s)
        {
            const float p = 0.5f * (inL[s] * inL[s] + inR[s] * inR[s]);
            ms = flushDenormal(p + rmsCoef * (ms - p));
            const float levelDb = 10.0f * std::log10(std::max(ms, 1.0e-12f));
            const float over = levelDb - thr;
            float target = 0.0f;                                  // gain reduction in dB (>= 0)
            if (over > knee * 0.5f) target = over * slope;
            else if (over > -knee * 0.5f) { const float x = over + knee * 0.5f; target = slope * x * x / (2.0f * knee); }
            const float attack = std::exp((float) (-1.0 / ((0.010 / (1.0 + std::max(0.0f, target) / 6.0f)) * sr)));
            grDb = target > grDb ? target + attack * (grDb - target) : target + rel * (grDb - target);
            grDb = flushDenormal(grDb);
            const float g = dbToGain(-grDb) * makeup;
            outL[s] = inL[s] * g; outR[s] = inR[s] * g;
        }
        lastGr = grDb;
    }
    float gainReductionDb() const { return lastGr; }
private:
    double sr = 48000.0; float rmsCoef = 0.0f, ms = 0.0f, grDb = 0.0f, lastGr = 0.0f;
};
}
