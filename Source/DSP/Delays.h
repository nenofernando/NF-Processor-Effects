#pragma once
// The six delays: Studio (stereo, ducker), Digital (clean mono, ducker), Tape (wow/flutter, smear), Pong (ping-pong),
// Modulated (stereo, chorused repeats) and Reverse (backwards repeats that build up).
#include "Common.h"
#include "Catalog.h"

namespace nfpe
{
class DelayFx
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        dl.init((int) (10.3 * sr)); dr.init((int) (2.6 * sr));
        for (int i = 0; i < 2; ++i) { fbLp[i].setCutoff(7000.0f, sr); fbHp[i].setCutoff(90.0f, sr); }
        ap[0].init((int) (0.0071 * sr)); ap[1].init((int) (0.0113 * sr));
        duckEnv.set(2.0f, 60.0f, sr); duck.setTime(25.0f, sr); timeS.setTime(70.0f, sr); gainS.setTime(20.0f, sr);
        reset(StudioDelay);
    }
    void reset(int fx)
    {
        type = fx;
        dl.clear(); dr.clear();
        for (int i = 0; i < 2; ++i) { fbLp[i].reset(); fbHp[i].reset(); }
        ap[0].clear(); ap[1].clear();
        duckEnv.reset(); duck.snap(1.0f); timeS.snap(-1.0f); gainS.snap(1.0f);
        for (auto& p : players) p.active = false;
        revCount = 0; wowPhase = 0.0; lastOut = 0.0f;
    }
    void process(const float* inL, const float* inR, float* outL, float* outR, int n, float k1, float k2, float k3, double quarterMs)
    {
        const bool studioLike = type == StudioDelay || type == ModDelay;
        const float maxMs = studioLike ? 2500.0f : 5000.0f;
        float tMs = (float) quarterMs * delayRatio(std::round(k1 * 72.0f));
        if (type == PongDelay) tMs = std::min(tMs, 2500.0f);
        tMs = clampf(tMs, 5.0f, maxMs);
        const float targetSamples = tMs * 0.001f * (float) sr;
        if (timeS.v < 0.0f) timeS.snap(targetSamples);

        const bool hold = (type == StudioDelay || type == DigitalDelay || type == PongDelay) && k2 >= 0.995f;
        const float fb = hold ? 1.0f : k2 * 0.96f;
        const float inGainTarget = hold ? 0.0f : 1.0f;
        const float thr = dbToGain(-70.0f + k3 * 70.0f);
        const bool ducks = (type == StudioDelay || type == DigitalDelay) && k3 < 0.999f;
        const float smearHz = expMap(k3, 12000.0f, 1500.0f);
        if (type == TapeDelay) { fbLp[0].setCutoff(smearHz, sr); }
        if (type == ReverseDelay) { fbLp[0].setCutoff(expMap(k3, 9000.0f, 1800.0f), sr); }
        const float apG = 0.5f * k3;
        if (studioLike) { const float hz = type == ModDelay ? 9000.0f : 7000.0f; fbLp[0].setCutoff(hz, sr); fbLp[1].setCutoff(hz, sr); }
        if (type == TapeDelay) fbLp[1].setCutoff(9500.0f, sr);

        for (int s = 0; s < n; ++s)
        {
            const float tS = timeS.next(targetSamples);
            const float inG = gainS.next(inGainTarget);
            const float l = inL[s], r = inR[s], m = 0.5f * (l + r);
            const float lvl = duckEnv.process(std::max(std::fabs(l), std::fabs(r)));
            const float dk = duck.next(ducks && lvl > thr ? 0.5f : 1.0f);
            switch (type)
            {
                case StudioDelay:
                {
                    const float yl = dl.read(tS), yr = dr.read(tS);
                    dl.push(flushDenormal(l * inG + fb * fbLp[0].lp(yl)));
                    dr.push(flushDenormal(r * inG + fb * fbLp[1].lp(yr)));
                    outL[s] = yl * dk; outR[s] = yr * dk;
                    break;
                }
                case DigitalDelay:
                {
                    const float y = dl.read(tS);
                    dl.push(flushDenormal(m * inG + fb * y));
                    outL[s] = outR[s] = y * dk;
                    break;
                }
                case TapeDelay:
                {
                    wowPhase += 1.0 / sr;
                    const float wow = (float) (std::sin(kTwoPi * 0.63 * wowPhase) * 0.00035 + std::sin(kTwoPi * 6.4 * wowPhase) * 0.00006) * (float) sr;
                    const float y = dl.read(tS + wow);
                    float z = fbHp[0].hp(fbLp[0].lp(y));
                    z = ap[0].process(z, apG); z = ap[1].process(z, apG);
                    z = std::tanh(z * 1.25f) / 1.25f;
                    dl.push(flushDenormal(m * inG + fb * z));
                    const float o = fbLp[1].lp(y);
                    outL[s] = outR[s] = o;
                    break;
                }
                case PongDelay:
                {
                    const float p = 2.0f * tS * (0.15f + 0.70f * k3);
                    const float y1 = dl.read(p), y2 = dl.read(2.0f * tS);
                    dl.push(flushDenormal(m * inG + fb * y2));
                    outL[s] = y1; outR[s] = y2;
                    break;
                }
                case ModDelay:
                {
                    wowPhase += 0.45 / sr; if (wowPhase >= 1.0) wowPhase -= 1.0;
                    const float md = k3 * 0.006f * (float) sr;
                    const float ml = (float) std::sin(kTwoPi * wowPhase) * md, mr = -ml;
                    const float yl = dl.read(tS + ml), yr = dr.read(tS + mr);
                    dl.push(flushDenormal(l * inG + fb * fbLp[0].lp(yl)));
                    dr.push(flushDenormal(r * inG + fb * fbLp[1].lp(yr)));
                    outL[s] = yl; outR[s] = yr;
                    break;
                }
                default:   // ReverseDelay
                {
                    const int T = std::max(256, (int) std::min(tS, (float) (5.0 * sr)));
                    // write first (fed back from the previous output), then the players read walking back in time
                    dl.push(flushDenormal(m * inG + fb * 0.9f * fbLp[0].lp(lastOut)));
                    if (--revCount <= 0)
                    {
                        revCount += std::max(1, T / 2);
                        for (auto& p : players) if (!p.active) { p.active = true; p.t = 0; p.T = T; break; }
                    }
                    float o = 0.0f;
                    for (auto& p : players)
                    {
                        if (!p.active) continue;
                        const float w = 0.5f - 0.5f * std::cos((float) (kTwoPi * (double) p.t / (double) p.T));
                        o += dl.at(2 * p.t + 1) * w;
                        if (++p.t >= p.T) p.active = false;
                    }
                    lastOut = o;
                    outL[s] = outR[s] = o;
                    break;
                }
            }
        }
    }
private:
    struct Player { bool active = false; int t = 0, T = 1; };
    double sr = 48000.0;
    int type = StudioDelay;
    DelayLine dl, dr;
    OnePole fbLp[2], fbHp[2];
    Allpass ap[2];
    Follower duckEnv; Smooth duck, timeS, gainS;
    Player players[4]; int revCount = 0; double wowPhase = 0.0; float lastOut = 0.0f;
};
}
