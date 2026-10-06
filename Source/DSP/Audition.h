#pragma once
// The five built-in audition sounds (stick click, snare, kick, female "Doo", acoustic guitar arpeggio).
// They are synthesised at start-up (no recordings are shipped): each one is a short, dry, mono test signal.
#include "Common.h"

namespace nfpe
{
class Audition
{
public:
    static constexpr int kNumSamples = 5;
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        make(0, stick()); make(1, snare()); make(2, kick()); make(3, vocal()); make(4, guitar());
        pos = (size_t) -1;
    }
    void trigger(int index) { current = index < 0 ? 0 : (index >= kNumSamples ? kNumSamples - 1 : index); pos = 0; }
    bool playing() const { return pos != (size_t) -1; }
    // adds the sound to the two channels
    void addTo(float* L, float* R, int n)
    {
        if (pos == (size_t) -1) return;
        const auto& d = data[current];
        for (int i = 0; i < n && pos < d.size(); ++i, ++pos) { L[i] += d[pos]; R[i] += d[pos]; }
        if (pos >= d.size()) pos = (size_t) -1;
    }
private:
    void make(int i, std::vector<float> v)
    {
        float peak = 1.0e-6f;
        for (float x : v) peak = std::max(peak, std::fabs(x));
        for (float& x : v) x *= 0.7f / peak;
        data[i] = std::move(v);
    }
    std::vector<float> buf(double seconds) const { return std::vector<float>((size_t) (seconds * sr) + 1, 0.0f); }
    std::vector<float> stick()
    {
        auto v = buf(0.25); Noise nz; Biquad bp; bp.setBandpass(3800.0f, 1.6f, sr);
        for (size_t i = 0; i < v.size(); ++i)
        {
            const float t = (float) (i / sr);
            v[i] = bp.process(nz.next()) * std::exp(-t / 0.006f) * 3.0f + std::sin((float) (kTwoPi * 1250.0 * t)) * std::exp(-t / 0.012f) * 0.6f;
        }
        return v;
    }
    std::vector<float> snare()
    {
        auto v = buf(0.45); Noise nz; Biquad hp; hp.setHighpass(1500.0f, 0.7f, sr);
        for (size_t i = 0; i < v.size(); ++i)
        {
            const float t = (float) (i / sr);
            v[i] = hp.process(nz.next()) * std::exp(-t / 0.09f) * 0.9f + std::sin((float) (kTwoPi * (190.0 + 60.0 * std::exp(-t / 0.02)) * t)) * std::exp(-t / 0.07f) * 0.8f;
        }
        return v;
    }
    std::vector<float> kick()
    {
        auto v = buf(0.6); double ph = 0.0; Noise nz; Biquad lp; lp.setLowpass(3000.0f, 0.7f, sr);
        for (size_t i = 0; i < v.size(); ++i)
        {
            const double t = (double) i / sr;
            const double f = 48.0 + 90.0 * std::exp(-t / 0.035);
            ph += f / sr;
            v[i] = (float) (std::sin(kTwoPi * ph) * std::exp(-t / 0.17)) + lp.process(nz.next()) * (float) std::exp(-t / 0.004) * 0.35f;
        }
        return v;
    }
    std::vector<float> vocal()   // female "Doo": sawtooth source through the formants of /u/
    {
        auto v = buf(1.7); Biquad f1, f2, f3; f1.setBandpass(320.0f, 5.0f, sr); f2.setBandpass(850.0f, 7.0f, sr); f3.setBandpass(2500.0f, 9.0f, sr);
        double ph = 0.0; Noise nz; Biquad hp; hp.setHighpass(3000.0f, 0.7f, sr);
        for (size_t i = 0; i < v.size(); ++i)
        {
            const double t = (double) i / sr;
            const double f0 = 262.0 * (1.0 + 0.012 * std::sin(kTwoPi * 5.4 * t) * std::min(1.0, t / 0.4));
            ph += f0 / sr; if (ph >= 1.0) ph -= 1.0;
            const float src = (float) (2.0 * ph - 1.0);
            float env = (float) std::min(1.0, t / 0.05) * (float) std::min(1.0, (1.7 - t) / 0.45);
            const float y = f1.process(src) * 1.0f + f2.process(src) * 0.55f + f3.process(src) * 0.12f;
            const float dcons = hp.process(nz.next()) * (float) std::exp(-t / 0.02) * 0.4f;   // the "D"
            v[i] = (y * env + dcons) * 0.5f;
        }
        return v;
    }
    std::vector<float> guitar()  // Karplus-Strong arpeggio: E3 B3 E4 G4 B4 E5
    {
        auto v = buf(3.6);
        const double notes[6] = { 164.81, 246.94, 329.63, 392.0, 493.88, 659.26 };
        Noise nz;
        for (int k = 0; k < 6; ++k)
        {
            const int len = std::max(2, (int) (sr / notes[k])); std::vector<float> ring((size_t) len);
            for (auto& x : ring) x = nz.next();
            size_t idx = 0; const size_t start = (size_t) (k * 0.26 * sr);
            for (size_t i = start; i < v.size(); ++i)
            {
                const float cur = ring[idx], nxt = ring[(idx + 1) % (size_t) len];
                const float y = 0.5f * (cur + nxt) * 0.9965f;
                ring[idx] = y; idx = (idx + 1) % (size_t) len;
                v[i] += cur * 0.6f;
            }
        }
        return v;
    }
    double sr = 48000.0;
    std::vector<float> data[kNumSamples];
    int current = 0; size_t pos = (size_t) -1;
};
}
