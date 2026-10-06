#pragma once
// The 16 reverbs: an 8-line feedback delay network (halls, plates, rooms, chamber, ambience, studio, arena),
// a gated reverb, a reverse reverb and a spring reverb. All algorithms are our own; the manual describes the
// character of each program, not its internals.
#include "Common.h"
#include "Catalog.h"

namespace nfpe
{
struct ReverbConfig
{
    float size;               // multiplies the delay-line lengths
    float rtMin, rtMax;       // RT60 (s) at Decay 0 / 99
    float ap;                 // input diffusion (all-pass coefficient)
    float er, erSize;         // early reflection level / time scale
    float dampLo, dampHi;     // loop damping cut-off (Hz) at Liveliness 0 / 99
    float mod;                // delay modulation depth (samples at 48 kHz)
    float preMax;             // ms at Pre Delay 99
};

inline const ReverbConfig& reverbConfig(int fx)
{
    static const ReverbConfig c[16] =
    {
        /* Small Hall  */ { 1.20f, 0.6f,  6.0f, 0.62f, 0.35f, 0.7f, 2500, 9000, 6, 100 },
        /* Large Hall  */ { 2.00f, 1.0f, 14.0f, 0.62f, 0.30f, 1.2f, 2200, 8500, 8, 200 },
        /* Small Plate */ { 0.80f, 0.4f,  4.5f, 0.72f, 0.00f, 1.0f, 4000, 14000, 5, 100 },
        /* Large Plate */ { 1.10f, 0.6f,  9.0f, 0.72f, 0.00f, 1.0f, 3500, 14000, 6, 200 },
        /* Room        */ { 0.45f, 0.15f, 2.2f, 0.55f, 0.70f, 0.45f, 1800, 7000, 2, 100 },
        /* Chamber     */ { 0.90f, 0.4f,  5.0f, 0.60f, 0.25f, 0.8f, 2500, 8000, 5, 100 },
        /* Gated       */ { 1.30f, 1.6f,  1.6f, 0.70f, 0.20f, 0.9f, 6000, 6000, 3, 200 },
        /* Reverse     */ { 1.30f, 1.0f,  1.0f, 0.70f, 0.00f, 1.0f, 6000, 6000, 3, 200 },
        /* Vocal Hall  */ { 1.80f, 1.0f, 12.0f, 0.50f, 0.30f, 1.1f, 2000, 8000, 8, 200 },
        /* Vocal Plate */ { 1.00f, 0.5f,  8.0f, 0.60f, 0.00f, 1.0f, 3500, 13000, 6, 200 },
        /* Drum Hall   */ { 1.50f, 0.7f,  8.0f, 0.75f, 0.30f, 1.0f, 3000, 10000, 6, 100 },
        /* Drum Plate  */ { 0.90f, 0.4f,  5.0f, 0.78f, 0.00f, 1.0f, 4500, 15000, 5, 100 },
        /* Ambience    */ { 0.30f, 0.2f,  0.9f, 0.60f, 0.80f, 0.5f, 3000, 9000, 2, 100 },
        /* Studio      */ { 0.60f, 0.25f, 3.0f, 0.60f, 0.50f, 0.55f, 2500, 9000, 2, 100 },
        /* Arena       */ { 3.00f, 2.0f, 20.0f, 0.60f, 0.20f, 2.0f, 1200, 4000, 10, 200 },
        /* Spring      */ { 1.00f, 0.4f,  4.0f, 0.60f, 0.00f, 1.0f, 3000, 3000, 0, 200 },
    };
    return c[fx < 0 ? 0 : (fx > 15 ? 15 : fx)];
}

class FdnReverb
{
public:
    static constexpr int N = 8;

    void prepare(double sampleRate)
    {
        sr = sampleRate; scale = (float) (sr / 48000.0);
        const int maxLen = (int) (2087 * 3.2f * scale) + 80;
        for (int i = 0; i < N; ++i) line[i].init(maxLen);
        preL.init((int) (0.26 * sr)); preR.init((int) (0.26 * sr));
        er.init((int) (0.30 * sr));
        static const int apL[4] = { 113, 163, 241, 337 }, apR[4] = { 127, 181, 263, 359 };
        for (int i = 0; i < 4; ++i) { diffL[i].init((int) (apL[i] * scale) + 2); diffR[i].init((int) (apR[i] * scale) + 2); }
        for (int i = 0; i < N; ++i) { lfo[i].setPhase((double) i / N); lfo[i].setRate(0.11 + 0.037 * i, sr); }
        outHp.a = 0.0f; hpL.setCutoff(35.0f, sr); hpR.setCutoff(35.0f, sr);
        reset();
    }
    void reset()
    {
        for (int i = 0; i < N; ++i) { line[i].clear(); damp[i].reset(); }
        preL.clear(); preR.clear(); er.clear();
        for (int i = 0; i < 4; ++i) { diffL[i].clear(); diffR[i].clear(); }
        hpL.reset(); hpR.reset();
    }
    // Called when the reverb type changes (no allocation).
    void configure(float size)
    {
        static const int base[N] = { 1031, 1153, 1301, 1447, 1597, 1753, 1913, 2087 };
        for (int i = 0; i < N; ++i) len[i] = (float) ((int) (base[i] * size * scale) | 1);
        modDepth = 0.0f;
    }
    void setModulation(float samplesAt48k) { modDepth = samplesAt48k * scale; }
    // rt: RT60 in seconds. live01: 0..1. All set once per block.
    void setDynamic(float rt, float dampHz, float apCoef, float erLevel, float erSize, float preMs)
    {
        rt = clampf(rt, 0.05f, 40.0f);
        for (int i = 0; i < N; ++i)
        {
            g[i] = std::min(0.9995f, std::pow(10.0f, -3.0f * len[i] / (rt * (float) sr)));
            damp[i].setCutoff(dampHz, sr);
        }
        ap = clampf(apCoef, 0.0f, 0.85f);
        erGain = erLevel; erScale = erSize; preSamples = std::min(preMs * 0.001f * (float) sr, (float) (preL.capacity() - 4));
        // level compensation: the steady-state level of a feedback reverb grows with sqrt(RT60)
        outGain = 0.42f * clampf(std::sqrt(1.5f / rt), 0.45f, 2.2f);
    }
    // wet output only
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, bool monoIn)
    {
        static const float erTimesL[8] = { 7, 13, 19, 29, 37, 47, 59, 71 }, erTimesR[8] = { 5, 11, 17, 23, 31, 43, 53, 67 };
        static const float erGains[8] = { 0.8f, 0.7f, 0.62f, 0.55f, 0.48f, 0.40f, 0.33f, 0.27f };
        const float msToSamp = 0.001f * (float) sr;
        for (int s = 0; s < n; ++s)
        {
            float xl = inL[s], xr = monoIn ? inL[s] : inR[s];
            if (monoIn) { xl = xr = 0.5f * (inL[s] + inR[s]); }
            preL.push(xl); preR.push(xr);
            float pl = preSamples < 1.0f ? xl : preL.read(preSamples);
            float pr = preSamples < 1.0f ? xr : preR.read(preSamples);
            er.push(0.5f * (pl + pr));
            float dl = pl, dr = pr;
            for (int i = 0; i < 4; ++i) { dl = diffL[i].process(dl, ap); dr = diffR[i].process(dr, ap); }

            float y[N];
            for (int i = 0; i < N; ++i)
            {
                const float d = len[i] + modDepth * lfo[i].sine();
                lfo[i].advance();
                y[i] = damp[i].lp(line[i].read(d)) * g[i];
            }
            // output taps (before the feedback matrix)
            float oL = (y[0] - y[1] + y[2] - y[3] + y[4] - y[5] + y[6] - y[7]);
            float oR = (y[0] + y[1] - y[2] - y[3] + y[4] + y[5] - y[6] - y[7]);
            // Walsh-Hadamard (orthonormal)
            for (int h = 1; h < N; h <<= 1)
                for (int i = 0; i < N; i += h << 1)
                    for (int j = i; j < i + h; ++j) { const float a = y[j], b = y[j + h]; y[j] = a + b; y[j + h] = a - b; }
            const float norm = 0.35355339f;
            for (int i = 0; i < N; ++i)
            {
                const float in = (i & 1) ? dr : dl;
                line[i].push(flushDenormal(y[i] * norm + in * 0.5f));
            }
            float wl = oL * outGain, wr = oR * outGain;
            if (erGain > 0.0f)
            {
                float el = 0.0f, ey = 0.0f;
                for (int t = 0; t < 8; ++t) { el += er.read(erTimesL[t] * erScale * msToSamp + 1.0f) * erGains[t]; ey += er.read(erTimesR[t] * erScale * msToSamp + 1.0f) * erGains[t]; }
                wl += el * erGain * 0.45f; wr += ey * erGain * 0.45f;
            }
            outL[s] = hpL.hp(wl); outR[s] = hpR.hp(wr);
        }
    }
private:
    double sr = 48000.0; float scale = 1.0f;
    DelayLine line[N], preL, preR, er;
    Allpass diffL[4], diffR[4];
    OnePole damp[N], hpL, hpR, outHp;
    Lfo lfo[N];
    float len[N] = { 1031, 1153, 1301, 1447, 1597, 1753, 1913, 2087 }, g[N] = {};
    float ap = 0.6f, erGain = 0.0f, erScale = 1.0f, preSamples = 0.0f, outGain = 0.4f, modDepth = 0.0f;
};

// Spring reverb: band-limited input -> two dispersive spring lines (chain of all-passes inside a feedback loop).
class SpringReverb
{
public:
    static constexpr int kStages = 80;
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (int j = 0; j < 2; ++j) { d[j].init((int) (0.09 * sr)); }
        inHp.setCutoff(150.0f, sr); inLp.setCutoff(4800.0f, sr); inLp2.setCutoff(4800.0f, sr);
        for (int j = 0; j < 2; ++j) loopLp[j].setCutoff(3200.0f, sr);
        pre.init((int) (0.26 * sr));
        reset();
    }
    void reset()
    {
        for (int j = 0; j < 2; ++j) { d[j].clear(); for (auto& a : ap[j]) a.reset(); loopLp[j].reset(); fbState[j] = 0.0f; }
        inHp.reset(); inLp.reset(); inLp2.reset(); pre.clear();
    }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float preMs, float rt, float boing)
    {
        const float delays[2] = { (float) (0.031 * sr), (float) (0.037 * sr) };
        const float loopMs = 0.050f;   // loop time including the dispersion (approx.)
        const float fb = std::min(0.97f, std::pow(10.0f, -3.0f * loopMs / clampf(rt, 0.2f, 8.0f)));
        const float drive = 1.0f + 3.0f * boing, a = -0.68f + 0.1f * boing;
        const float preS = std::min(preMs * 0.001f * (float) sr, (float) (pre.capacity() - 4));
        for (int s = 0; s < n; ++s)
        {
            pre.push(0.5f * (inL[s] + inR[s]));
            float x = preS < 1.0f ? 0.5f * (inL[s] + inR[s]) : pre.read(preS);
            x = inLp2.lp(inLp.lp(inHp.hp(x)));
            x = std::tanh(x * drive) / drive;
            float o[2];
            for (int j = 0; j < 2; ++j)
            {
                d[j].push(flushDenormal(x + fb * fbState[j]));
                const float wob = 1.0f + 3.0f * boing * noise.next() * 0.02f;
                float y = d[j].read(delays[j] * wob);
                for (int k = 0; k < kStages; ++k) y = ap[j][k].process(y, a);
                y = loopLp[j].lp(y);
                fbState[j] = flushDenormal(y);
                o[j] = y;
            }
            outL[s] = o[0] * 1.6f; outR[s] = o[1] * 1.6f;
        }
    }
private:
    double sr = 48000.0;
    DelayLine d[2], pre;
    Allpass1 ap[2][kStages];
    OnePole inHp, inLp, inLp2, loopLp[2];
    float fbState[2] = { 0, 0 };
    Noise noise;
};

// One object for all 16 reverb programs.
class ReverbFx
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        fdn.prepare(sr); spring.prepare(sr);
        revSize = (int) (2.0 * 1.8 * sr) + 64; revMask = 1; while (revMask < revSize) revMask <<= 1;
        revL.assign((size_t) revMask, 0.0f); revR.assign((size_t) revMask, 0.0f); revMask -= 1;
        scratchL.assign(16384, 0.0f); scratchR.assign(16384, 0.0f);
        env.set(1.0f, 30.0f, sr);
        reset(0);
    }
    void reset(int fx)
    {
        type = fx;
        fdn.reset(); spring.reset(); env.reset();
        std::fill(revL.begin(), revL.end(), 0.0f); std::fill(revR.begin(), revR.end(), 0.0f);
        revW = 0; revCount = 0; for (auto& p : players) p.active = false;
        holdLeft = 0; gate = 0.0f;
        const auto& c = reverbConfig(fx);
        fdn.configure(c.size);
        fdn.setModulation(c.mod);
    }
    int currentType() const { return type; }

    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3)
    {
        const auto& c = reverbConfig(type);
        const float preMs = k1 * c.preMax;
        int done = 0;
        while (done < n)
        {
            const int m = std::min(n - done, (int) scratchL.size());
            const float *iL = inL + done, *iR = inR + done;
            float *oL = outL + done, *oR = outR + done;
            if (type == Spring) spring.process(iL, iR, oL, oR, m, preMs, 0.4f + k2 * 5.0f, k3);
            else if (type == Gated) processGated(iL, iR, oL, oR, m, c, preMs, k2, k3);
            else if (type == ReverseReverb) processReverse(iL, iR, oL, oR, m, c, preMs, k2, k3);
            else
            {
                float rt = expMap(k2, c.rtMin, c.rtMax), er = c.er, damp = expMap(k3, c.dampLo, c.dampHi);
                if (type == Ambience) { rt = 0.15f + 0.75f * k2; er = c.er * (1.2f - 0.7f * k2); }
                fdn.setDynamic(rt, damp, c.ap, er, c.erSize, preMs);
                fdn.process(iL, iR, oL, oR, m, false);
            }
            // loudness trim per program (measured with the snare audition sound, so all reverbs sit at about -2 dB wet/dry energy)
            static const float trimDb[16] = { -1.1f, 3.5f, -5.4f, 1.2f, -1.1f, 1.2f, -0.6f, 1.2f, 2.5f, -1.3f, 1.5f, 1.7f, -5.9f, -2.0f, 3.3f, -1.5f };
            const float tg = dbToGain(trimDb[type]);
            for (int s = 0; s < m; ++s) { oL[s] *= tg; oR[s] *= tg; }
            done += m;
        }
    }
private:
    void processGated(const float* iL, const float* iR, float* oL, float* oR, int n, const ReverbConfig& c, float preMs, float k2, float k3)
    {
        fdn.setDynamic(c.rtMin, c.dampLo, 0.35f + 0.4f * k3, c.er, c.erSize, preMs);
        fdn.process(iL, iR, oL, oR, n, true);
        const int hold = (int) ((0.020f + k2 * 0.580f) * (float) sr);
        for (int s = 0; s < n; ++s)
        {
            const float lvl = env.process(std::max(std::fabs(iL[s]), std::fabs(iR[s])));
            if (lvl > 0.02f) holdLeft = hold;
            const bool open = holdLeft > 0;
            if (holdLeft > 0) --holdLeft;
            const float target = open ? 1.0f : 0.0f;
            gate += (target - gate) * (target > gate ? 0.02f : 0.0007f * 48000.0f / (float) sr * 6.0f);
            oL[s] *= gate; oR[s] *= gate;
        }
    }
    void processReverse(const float* iL, const float* iR, float* oL, float* oR, int n, const ReverbConfig& c, float preMs, float k2, float k3)
    {
        const float T = 0.10f + k2 * 1.5f;                       // segment length (s)
        const int Ti = std::max(64, (int) (T * (float) sr));
        fdn.setDynamic(T * 1.1f, c.dampLo, 0.35f + 0.4f * k3, 0.0f, c.erSize, preMs);
        fdn.process(iL, iR, scratchL.data(), scratchR.data(), n, true);
        for (int s = 0; s < n; ++s)
        {
            revW = (revW + 1) & revMask;
            revL[(size_t) revW] = scratchL[(size_t) s]; revR[(size_t) revW] = scratchR[(size_t) s];
            if (--revCount <= 0)
            {
                revCount += std::max(1, Ti / 2);
                for (auto& p : players) if (!p.active) { p.active = true; p.t = 0; p.T = Ti; p.start = revW; break; }
            }
            float l = 0.0f, r = 0.0f;
            for (auto& p : players)
            {
                if (!p.active) continue;
                const float w = 0.5f - 0.5f * std::cos((float) (kTwoPi * (double) p.t / (double) p.T));
                const int idx = (p.start - p.t) & revMask;      // newest sample first, then walking back in time
                l += revL[(size_t) idx] * w; r += revR[(size_t) idx] * w;
                if (++p.t >= p.T) p.active = false;
            }
            oL[s] = l; oR[s] = r;
        }
    }
    struct Player { bool active = false; int t = 0, T = 1, start = 0; };
    double sr = 48000.0;
    int type = 0;
    FdnReverb fdn; SpringReverb spring;
    std::vector<float> revL, revR, scratchL, scratchR;
    int revSize = 0, revMask = 0, revW = 0, revCount = 0;
    Player players[4];
    Follower env; int holdLeft = 0; float gate = 0.0f;
};
}
