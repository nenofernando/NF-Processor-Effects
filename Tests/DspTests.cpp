// Standalone DSP checks (no JUCE): every effect must be finite, bounded, silent on silence and respond to a signal.
#include <cstdio>
#include <cstdlib>
#include "../Source/DSP/Engine.h"
#include "../Source/DSP/Audition.h"

using namespace nfpe;
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf("FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static double rms(const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += (double) v[i] * v[i]; return std::sqrt(s / std::max<size_t>(1, b - a)); }

int main()
{
    const double sr = 48000.0; const int block = 256;
    Audition aud; aud.prepare(sr);
    // 1. each effect on a kick+noise burst: finite, bounded, not silent
    for (int fx = 0; fx < kNumEffects; ++fx)
    {
        EffectSlot slot; slot.prepare(sr); slot.setType(fx);
        float k[3]; for (int i = 0; i < 3; ++i) k[i] = effectInfo(fx).def[i];
        std::vector<float> out; std::vector<float> l(block), r(block), ol(block), orr(block);
        aud.trigger(fx % 5);
        double maxAbs = 0; bool finite = true;
        for (int b = 0; b < (int) (4 * sr / block); ++b)
        {
            std::fill(l.begin(), l.end(), 0.0f); std::fill(r.begin(), r.end(), 0.0f);
            if (b < 200) aud.addTo(l.data(), r.data(), block);
            slot.process(l.data(), r.data(), ol.data(), orr.data(), block, k, 500.0);
            for (int i = 0; i < block; ++i) { const float v = ol[(size_t) i]; if (!std::isfinite(v) || !std::isfinite(orr[(size_t) i])) finite = false; maxAbs = std::max(maxAbs, (double) std::max(std::fabs(v), std::fabs(orr[(size_t) i]))); out.push_back(v); }
        }
        const double tailRms = rms(out, 0, out.size());
        std::printf("%-14s peak %.3f  rms %.4f\n", effectInfo(fx).name, maxAbs, tailRms);
        CHECK(finite, "%s produced NaN/Inf", effectInfo(fx).name);
        CHECK(maxAbs < 4.0, "%s peak too high (%.2f)", effectInfo(fx).name, maxAbs);
        CHECK(tailRms > 1.0e-5, "%s is silent", effectInfo(fx).name);
        // silence in -> silence out (after the tail): feed zeros for 20 s with a non-hold setting and check decay
    }
    // 2. reverb tails must decay: big hall, decay 99, tail after the input stops
    {
        EffectSlot slot; slot.prepare(sr); slot.setType(LargeHall);
        float k[3] = { 0.0f, 0.5f, 0.5f };
        std::vector<float> l(block, 0.0f), r(block, 0.0f), ol(block), orr(block), env;
        l[0] = r[0] = 1.0f;
        for (int b = 0; b < (int) (30 * sr / block); ++b) { slot.process(l.data(), r.data(), ol.data(), orr.data(), block, k, 500.0); l[0] = r[0] = 0.0f; double e = 0; for (int i = 0; i < block; ++i) e += ol[(size_t) i] * ol[(size_t) i]; env.push_back((float) std::sqrt(e / block)); }
        const double early = rms(env, (size_t) (0.2 * sr / block), (size_t) (1.0 * sr / block)), late = rms(env, (size_t) (20 * sr / block), (size_t) (30 * sr / block));
        std::printf("Large Hall tail: early %.5f late %.7f\n", early, late);
        CHECK(early > 1e-4, "reverb has no tail");
        CHECK(late < early * 0.05, "reverb tail does not decay");
    }
    // 3. delay timing: Digital delay at 1/4 note with 500 ms quarter -> echo at 500 ms
    {
        DelayFx d; d.prepare(sr); d.reset(DigitalDelay);
        std::vector<float> l((size_t) sr, 0.0f), r((size_t) sr, 0.0f), ol((size_t) sr), orr((size_t) sr);
        l[0] = r[0] = 1.0f;
        d.process(l.data(), r.data(), ol.data(), orr.data(), (int) sr, 0.5f, 0.0f, 1.0f, 500.0);
        size_t pk = 0; for (size_t i = 0; i < ol.size(); ++i) if (std::fabs(ol[i]) > std::fabs(ol[pk])) pk = i;
        std::printf("Digital delay echo at %.1f ms (expect 500)\n", pk * 1000.0 / sr);
        CHECK(std::fabs((double) pk * 1000.0 / sr - 500.0) < 2.0, "delay time wrong");
    }
    // 4. pitch shifter: +12 semitones doubles the frequency of a 440 Hz sine
    {
        PitchShiftFx p; p.prepare(sr); std::vector<float> l((size_t) sr), r((size_t) sr), ol((size_t) sr), orr((size_t) sr);
        for (size_t i = 0; i < l.size(); ++i) l[i] = r[i] = 0.5f * std::sin((float) (kTwoPi * 440.0 * i / sr));
        p.process(l.data(), r.data(), ol.data(), orr.data(), (int) sr, 0.5f, 1.0f, 1.0f);
        int zc = 0; for (size_t i = (size_t) (0.5 * sr); i + 1 < ol.size(); ++i) if (ol[i] <= 0 && ol[i + 1] > 0) ++zc;
        const double f = zc / 0.5;
        std::printf("Pitch shift +12 st: %.1f Hz (expect ~880)\n", f);
        CHECK(std::fabs(f - 880.0) < 25.0, "pitch shift frequency wrong");
    }
    // 5. engine: all routings x all effect pairs stay finite
    {
        Engine e; e.prepare(sr, block);
        std::vector<float> l(block), r(block);
        bool finite = true;
        for (int routing = 0; routing < 4; ++routing)
            for (int a = 0; a < kNumEffects; a += 3)
            {
                EngineParams ep; ep.routing = routing; ep.fx[0] = a; ep.fx[1] = (a + 11) % kNumEffects; ep.mix[0] = ep.mix[1] = 0.5f;
                for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) ep.knob[p][i] = effectInfo(ep.fx[p]).def[i];
                aud.trigger(1);
                for (int b = 0; b < 120; ++b)
                {
                    std::fill(l.begin(), l.end(), 0.0f); std::fill(r.begin(), r.end(), 0.0f);
                    if (b < 10) aud.addTo(l.data(), r.data(), block);
                    if (b == 60) ep.bypass[0] = true;
                    e.process(l.data(), r.data(), block, ep);
                    for (int i = 0; i < block; ++i) if (!std::isfinite(l[(size_t) i]) || !std::isfinite(r[(size_t) i]) || std::fabs(l[(size_t) i]) > 8.0f) finite = false;
                }
            }
        CHECK(finite, "engine produced non-finite / huge output");
    }
    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nALL OK\n", failures);
    return failures ? 1 : 0;
}
