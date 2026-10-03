#pragma once
#include "AmpModel.h"
#include "DspUtils.h"
#include <juce_dsp/juce_dsp.h>
#include <memory>

namespace hz
{
class HZMachineDSP : public AmpModel
{
public:
    void prepare(double sampleRate, int maxBlock) override
    {
        sr = sampleRate;
        using OS = juce::dsp::Oversampling<float>;
        os2 = std::make_unique<OS>(1, 1, OS::filterHalfBandPolyphaseIIR, true, true);
        os4 = std::make_unique<OS>(1, 2, OS::filterHalfBandPolyphaseIIR, true, true);
        os2->initProcessing((size_t) maxBlock);
        os4->initProcessing((size_t) maxBlock);
        envDecay = (float) std::exp(-1.0 / (0.04 * sr));
        atkC = (float) (1.0 - std::exp(-1.0 / (0.0004 * sr)));
        relC = (float) (1.0 - std::exp(-1.0 / (0.026 * sr)));
        lastOs = -1;
        first = true;
        reset();
    }

    void reset() override
    {
        for (auto* b : { &hpf, &dT, &dTone, &aT, &lp1, &cpl, &bass, &mid, &tre, &pres }) b->reset();
        sag.grDb = 0; env = 0; gateG = 0;
        if (os2) { os2->reset(); os4->reset(); }
    }

    void setParams(const AmpParams& np) override { p = np; }

    int getLatencySamples(int q) const override
    {
        auto* o = q ? os4.get() : os2.get();
        return o ? (int) std::ceil(o->getLatencyInSamples()) : 0;
    }

    void process(float* x, int n) override
    {
        const int osIdx = p[OS] > 0.5f ? 1 : 0;
        auto& os = osIdx ? *os4 : *os2;
        const double fs = sr * (osIdx ? 4.0 : 2.0);
        const auto& M = kModes[std::clamp((int) std::lround(p[MODE]), 0, 3)];
        updateCoefs(fs, osIdx, M);

        // base-rate input stage: gain -> HPF -> gate
        const float thr = dbToLin(p[GATE]);
        for (int i = 0; i < n; ++i)
        {
            float s = hpf.process(x[i] * sIn.getNextValue());
            const float a = std::fabs(s);
            env = a > env ? a : env * envDecay;
            const float t = env > thr ? 1.f : 0.f;
            gateG += (t - gateG) * (t > gateG ? atkC : relC);
            x[i] = s * gateG;
        }

        float* ch[1] = { x };
        juce::dsp::AudioBlock<float> blk(ch, 1, (size_t) n);
        auto up = os.processSamplesUp(blk);
        float* d = up.getChannelPointer(0);
        const size_t m = up.getNumSamples();
        for (size_t j = 0; j < m; ++j)
        {
            float v = d[j];
            // HZ DRIVE: tight high-pass -> gain -> clip -> tone -> level
            float dl = dT.process(v) * sDG.getNextValue();
            dl = dTone.process(shape(dl, .1f, 3.f)) * sDL.getNextValue();
            const float mix = sDm.getNextValue();
            v = v * (1.f - mix) + dl * mix;
            // preamp: two cascaded gain stages
            v = aT.process(v) * sG1.getNextValue();
            v = lp1.process(shape(v, M.a, M.ws)) * sG2.getNextValue();
            v = cpl.process(shape(v, M.a * .6f, M.ws * .8f));
            // tone stack, sag, power stage, presence
            v = tre.process(mid.process(bass.process(v)));
            v = pres.process(shape(sag.process(v), 0.f, 1.6f));
            d[j] = v;
        }
        os.processSamplesDown(blk);
    }

private:
    struct Mode { float m, a, ws; };
    static constexpr Mode kModes[4] = { {.5f, .04f, 1.2f}, {1.f, .1f, 2.f}, {1.7f, .16f, 3.f}, {2.3f, .26f, 3.4f} };

    void updateCoefs(double fs, int osIdx, const Mode& M)
    {
        using T = Biquad;
        auto b = [&](float v) { return (v - 5.f) * 3.f; };
        hpf.design(T::HP, sr, p[HPF], .7);
        dT.design(T::HP, fs, 60 + p[D_TIGHT] * 65, .7);
        dTone.design(T::LP, fs, 1500 + p[D_TONE] * 750, .7);
        aT.design(T::HP, fs, 40 + p[TIGHT] * 45, .7);
        lp1.design(T::LP, fs, 7000, .6);
        cpl.design(T::HP, fs, 30, .7);
        bass.design(T::LowShelf, fs, 110, .7, b(p[BASS]));
        mid.design(T::Peak, fs, 750, .8, b(p[MID]) * 1.2f);
        tre.design(T::HighShelf, fs, 3500, .7, b(p[TREBLE]));
        pres.design(T::HighShelf, fs, 4500, .7, b(p[PRES]) * 1.1f);
        sag.prepare(fs);
        sag.set(-8.f - p[SAG] * 3.f, 1.5f + p[SAG] * .8f, .005f, .15f);

        if (osIdx != lastOs)
        {
            sIn.reset(sr, .02);
            for (auto* s : { &sDG, &sDL, &sDm, &sG1, &sG2 }) s->reset(fs, .02);
            lastOs = osIdx;
        }
        sIn.setTargetValue(dbToLin(p[INGAIN]) * .5f);
        sDG.setTargetValue(1.f + p[D_DRIVE] * 5.f);
        sDL.setTargetValue(p[D_LEVEL] / 10.f * 1.2f);
        sDm.setTargetValue(p[DRVON] > .5f ? 1.f : 0.f);
        sG1.setTargetValue(1.f + p[GAIN] * M.m * 2.2f);
        sG2.setTargetValue(.6f + p[GAIN] * M.m * .9f);
        if (first)
        {
            for (auto* s : { &sIn, &sDG, &sDL, &sDm, &sG1, &sG2 }) s->setCurrentAndTargetValue(s->getTargetValue());
            first = false;
        }
    }

    double sr = 48000;
    AmpParams p{};
    std::unique_ptr<juce::dsp::Oversampling<float>> os2, os4;
    Biquad hpf, dT, dTone, aT, lp1, cpl, bass, mid, tre, pres;
    Comp sag;
    juce::SmoothedValue<float> sIn, sDG, sDL, sDm, sG1, sG2;
    float env = 0, gateG = 0, envDecay = .999f, atkC = .05f, relC = .001f;
    int lastOs = -1;
    bool first = true;
};
} // namespace hz
