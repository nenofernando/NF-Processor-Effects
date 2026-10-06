#pragma once
// EffectSlot: one of the two processors (any of the 32 effects). Engine: the two slots plus the four routings.
#include "Common.h"
#include "Catalog.h"
#include "Reverbs.h"
#include "Delays.h"
#include "Modulation.h"
#include "Dynamics.h"

namespace nfpe
{
class EffectSlot
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        reverb.prepare(sr); delay.prepare(sr); chorus.prepare(sr); flanger.prepare(sr); phaser.prepare(sr);
        trem.prepare(sr); rotary.prepare(sr); vibrato.prepare(sr); pitch.prepare(sr); detune.prepare(sr);
        deess.prepare(sr); comp.prepare(sr);
        setType(type);
    }
    void setType(int fx)
    {
        type = fx;
        if (isReverbType(fx)) reverb.reset(fx);
        else if (fx >= StudioDelay && fx <= ReverseDelay) delay.reset(fx);
        else switch (fx)
        {
            case DeEsser: deess.reset(); break;       case Compressor: comp.reset(); break;
            case Chorus: chorus.reset(); break;       case Flanger: flanger.reset(); break;
            case Phaser: phaser.reset(); break;       case TremoloPan: trem.reset(); break;
            case Rotary: rotary.reset(); break;       case Vibrato: vibrato.reset(); break;
            case PitchShift: pitch.reset(); break;    case Detune: detune.reset(); break;
            default: break;
        }
    }
    int getType() const { return type; }
    // Processes n samples; the output is the wet (processed) signal only.
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, const float* k, double quarterMs)
    {
        if (isReverbType(type)) reverb.process(inL, inR, outL, outR, n, k[0], k[1], k[2]);
        else if (type <= ReverseDelay) delay.process(inL, inR, outL, outR, n, k[0], k[1], k[2], quarterMs);
        else switch (type)
        {
            case DeEsser:    deess.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
            case Compressor: comp.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
            case Chorus:     chorus.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
            case Flanger:    flanger.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
            case Phaser:     phaser.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
            case TremoloPan: trem.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
            case Rotary:     rotary.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
            case Vibrato:    vibrato.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
            case PitchShift: pitch.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
            default:         detune.process(inL, inR, outL, outR, n, k[0], k[1], k[2]); break;
        }
    }
    float compressorGainReductionDb() const { return comp.gainReductionDb(); }
private:
    double sr = 48000.0; int type = 0;
    ReverbFx reverb; DelayFx delay; ChorusFx chorus; FlangerFx flanger; PhaserFx phaser; TremoloPanFx trem;
    RotaryFx rotary; VibratoFx vibrato; PitchShiftFx pitch; DetuneFx detune; DeEsserFx deess; CompressorFx comp;
};

enum Routing { DualMono = 0, Cascade, DualStereo, MonoSplit };

struct EngineParams
{
    int routing = Cascade;
    bool bypass[2] = { false, false };
    bool bypassMute = false;      // true: a bypassed processor outputs silence instead of the dry signal
    int fx[2] = { 0, 0 };
    float knob[2][3] = {};
    float mix[2] = { 0.3f, 0.3f };
    double quarterMs[2] = { 500.0, 500.0 };
};

class Engine
{
public:
    void prepare(double sampleRate, int maxBlock)
    {
        sr = sampleRate; block = maxBlock;
        for (int p = 0; p < 2; ++p)
        {
            slot[p].prepare(sr);
            wetL[p].assign((size_t) maxBlock, 0.0f); wetR[p].assign((size_t) maxBlock, 0.0f);
            inL[p].assign((size_t) maxBlock, 0.0f); inR[p].assign((size_t) maxBlock, 0.0f);
            outTmpL[p].assign((size_t) maxBlock, 0.0f); outTmpR[p].assign((size_t) maxBlock, 0.0f);
            mixS[p].setTime(15.0f, sr); actS[p].setTime(6.0f, sr); fadeS[p].setTime(8.0f, sr);
            mixS[p].snap(0.3f); actS[p].snap(1.0f); fadeS[p].snap(1.0f); lastFx[p] = -1;
        }
    }
    void reset() { for (int p = 0; p < 2; ++p) { slot[p].setType(slot[p].getType()); lastFx[p] = -1; } }

    // L/R are processed in place. n <= maxBlock.
    void process(float* L, float* R, int n, const EngineParams& e)
    {
        float dryL[2], dryR[2];
        (void) dryL; (void) dryR;
        // input to each processor
        for (int p = 0; p < 2; ++p)
        {
            if (e.fx[p] != lastFx[p]) { slot[p].setType(e.fx[p]); lastFx[p] = e.fx[p]; fadeS[p].snap(0.0f); }
        }
        const int r = e.routing;
        for (int s = 0; s < n; ++s)
        {
            switch (r)
            {
                case DualMono:  inL[0][(size_t) s] = inR[0][(size_t) s] = L[s]; inL[1][(size_t) s] = inR[1][(size_t) s] = R[s]; break;
                case MonoSplit: inL[0][(size_t) s] = inR[0][(size_t) s] = L[s]; inL[1][(size_t) s] = inR[1][(size_t) s] = R[s]; break;
                default:        inL[0][(size_t) s] = inL[1][(size_t) s] = L[s]; inR[0][(size_t) s] = inR[1][(size_t) s] = R[s]; break;
            }
        }
        if (r == Cascade)
        {
            runSlot(0, e, n);
            std::vector<float>& bL = outTmpL[0]; std::vector<float>& bR = outTmpR[0];
            ensure(bL, bR, n);
            blend(0, e, n, inL[0].data(), inR[0].data(), bL.data(), bR.data());
            // processor 2 takes the output of processor 1
            std::copy(bL.begin(), bL.begin() + n, inL[1].begin()); std::copy(bR.begin(), bR.begin() + n, inR[1].begin());
            runSlot(1, e, n);
            blend(1, e, n, inL[1].data(), inR[1].data(), L, R);
            return;
        }
        runSlot(0, e, n); runSlot(1, e, n);
        std::vector<float>& aL = outTmpL[0]; std::vector<float>& aR = outTmpR[0];
        std::vector<float>& bL = outTmpL[1]; std::vector<float>& bR = outTmpR[1];
        ensure(aL, aR, n); ensure(bL, bR, n);
        blend(0, e, n, inL[0].data(), inR[0].data(), aL.data(), aR.data());
        blend(1, e, n, inL[1].data(), inR[1].data(), bL.data(), bR.data());
        if (r == DualMono)
        {
            for (int s = 0; s < n; ++s) { L[s] = aL[(size_t) s]; R[s] = bR[(size_t) s]; }
            return;
        }
        // parallel routings: the two processors are summed, the dry signal counted once
        for (int s = 0; s < n; ++s)
        {
            const float dryA = (r == MonoSplit) ? 0.5f * L[s] : L[s], dryB = (r == MonoSplit) ? 0.5f * R[s] : R[s];
            (void) dryA; (void) dryB;
            L[s] = sumParallel(aL[(size_t) s], bL[(size_t) s], inL[0][(size_t) s], inL[1][(size_t) s], e, r, 0);
            R[s] = sumParallel(aR[(size_t) s], bR[(size_t) s], inR[0][(size_t) s], inR[1][(size_t) s], e, r, 1);
        }
    }
    float compressorGrDb(int p) const { return slot[p].compressorGainReductionDb(); }
private:
    // dry weight for parallel routings: the dry signal appears once, scaled by the least-wet active processor
    float sumParallel(float a, float b, float dryA, float dryB, const EngineParams& e, int r, int ch)
    {
        (void) ch;
        const bool aOut = !(e.bypass[0] && e.bypassMute), bOut = !(e.bypass[1] && e.bypassMute);
        const float ma = effMix(0, e), mb = effMix(1, e);
        float wetPart = 0.0f, dryPart = 0.0f;
        if (r == DualStereo)
        {
            // a and b already contain their own dry share: remove it and add it back once
            const float da = aOut ? dryA * (1.0f - ma) : 0.0f, db = bOut ? dryB * (1.0f - mb) : 0.0f;
            wetPart = (aOut ? a - da : 0.0f) + (bOut ? b - db : 0.0f);
            float w = 1.0f; bool any = false;
            if (aOut) { w = std::min(w, 1.0f - ma); any = true; }
            if (bOut) { w = std::min(w, 1.0f - mb); any = true; }
            dryPart = any ? 0.5f * (dryA + dryB) * w : 0.0f;
        }
        else
        {
            const float da = aOut ? dryA * (1.0f - ma) : 0.0f, db = bOut ? dryB * (1.0f - mb) : 0.0f;
            wetPart = (aOut ? a - da : 0.0f) + (bOut ? b - db : 0.0f);
            dryPart = 0.5f * (da + db);
        }
        return wetPart + dryPart;
    }
    float effMix(int p, const EngineParams& e) const
    {
        const auto& info = effectInfo(e.fx[p]);
        if (e.bypass[p]) return 0.0f;
        return info.wetOnly ? 1.0f : e.mix[p];
    }
    void ensure(std::vector<float>& a, std::vector<float>& b, int n) { if ((int) a.size() < n) a.assign((size_t) block, 0.0f); if ((int) b.size() < n) b.assign((size_t) block, 0.0f); }
    void runSlot(int p, const EngineParams& e, int n)
    {
        // keep running while the bypass cross-fade is audible, then stop (and clear) the processor
        const bool active = !e.bypass[p];
        if (!active && actS[p].v < 0.0005f) { std::fill(wetL[p].begin(), wetL[p].begin() + n, 0.0f); std::fill(wetR[p].begin(), wetR[p].begin() + n, 0.0f); wasActive[p] = false; return; }
        if (active && !wasActive[p]) { slot[p].setType(e.fx[p]); wasActive[p] = true; }
        slot[p].process(inL[p].data(), inR[p].data(), wetL[p].data(), wetR[p].data(), n, e.knob[p], e.quarterMs[p]);
    }
    // output of processor p given its input (dry) and its wet signal: dry/wet or level, with smooth bypass and effect-change fades
    void blend(int p, const EngineParams& e, int n, const float* dL, const float* dR, float* oL, float* oR)
    {
        const auto& info = effectInfo(e.fx[p]);
        const float mixT = e.mix[p];
        const float actT = e.bypass[p] ? 0.0f : 1.0f;
        for (int s = 0; s < n; ++s)
        {
            const float m = mixS[p].next(mixT), a = actS[p].next(actT), f = fadeS[p].next(1.0f);
            const float wl = wetL[p][(size_t) s] * f, wr = wetR[p][(size_t) s] * f;
            float pl, pr;
            if (info.wetOnly) { pl = wl * m; pr = wr * m; }
            else { pl = dL[s] * (1.0f - m) + wl * m; pr = dR[s] * (1.0f - m) + wr * m; }
            const float byL = e.bypassMute ? 0.0f : dL[s], byR = e.bypassMute ? 0.0f : dR[s];
            oL[s] = byL + (pl - byL) * a;
            oR[s] = byR + (pr - byR) * a;
        }
    }
    double sr = 48000.0; int block = 512;
    EffectSlot slot[2];
    std::vector<float> wetL[2], wetR[2], inL[2], inR[2], outTmpL[2], outTmpR[2];
    Smooth mixS[2], actS[2], fadeS[2];
    int lastFx[2] = { -1, -1 }; bool wasActive[2] = { true, true };
};
}
