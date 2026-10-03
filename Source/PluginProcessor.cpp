#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace hz;

HZProcessor::HZProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
    for (auto& d : paramDefs()) raw.push_back(apvts.getRawParameterValue(d.id));
    apvts.addParameterListener("cab", this);
    apvts.addParameterListener("os", this);
}

HZProcessor::~HZProcessor()
{
    cancelPendingUpdate();
    apvts.removeParameterListener("cab", this);
    apvts.removeParameterListener("os", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout HZProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : paramDefs())
    {
        const juce::ParameterID pid { d.id, 1 };
        if (d.isInt)
            layout.add(std::make_unique<juce::AudioParameterInt>(pid, d.name, (int) d.mn, (int) d.mx, (int) d.def));
        else
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                pid, d.name, juce::NormalisableRange<float>(d.mn, d.mx, d.step), d.def));
    }
    return layout;
}

bool HZProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    return (o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo()) && i == o;
}

static juce::AudioBuffer<float> makeIR(int cab, double sr)
{
    static const float lpF[] = { 6500.f, 5200.f, 4300.f }, fr[] = { 125.f, 100.f, 82.f },
                       tau[] = { .004f, .006f, .008f }, len[] = { .02f, .03f, .04f };
    cab = juce::jlimit(0, 2, cab);
    const int n = (int) (sr * len[cab]);
    juce::AudioBuffer<float> b(1, n);
    auto* d = b.getWritePointer(0);
    uint32_t seed = 7;
    float lp = 0;
    const float a = (float) std::exp(-2 * kPi * lpF[cab] / sr);
    for (int i = 0; i < n; ++i)
    {
        const float t = (float) (i / sr);
        seed = seed * 1664525u + 1013904223u;
        const float nz = (float) seed / 2147483648.f - 1.f;
        lp = nz * (1.f - a) + lp * a;
        d[i] = lp * 3.f * std::exp(-t / tau[cab]) + std::sin(2.f * (float) kPi * fr[cab] * t) * std::exp(-t / .01f) * .6f;
    }
    d[0] += .6f;
    return b;
}

void HZProcessor::loadCab() // message thread only: loading allocates
{
    const int cab = (int) std::lround(raw[CAB]->load());
    if (cab == loadedCab) return;
    loadedCab = cab;
    conv.loadImpulseResponse(makeIR(cab, sr), sr, juce::dsp::Convolution::Stereo::no,
                             juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::yes);
}

void HZProcessor::handleAsyncUpdate()
{
    loadCab();
    setLatencySamples(amp.getLatencySamples(raw[OS]->load() > .5f ? 1 : 0));
}

void HZProcessor::prepareToPlay(double sampleRate, int blockSize)
{
    sr = sampleRate;
    amp.prepare(sr, blockSize);
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 1 };
    conv.prepare(spec);
    conv.reset();
    mono.assign((size_t) blockSize, 0.f);
    dry.assign((size_t) blockSize, 0.f);
    limiter.prepare(sr);
    limiter.set(-3.f, 20.f, .002f, .08f);
    ohpf.reset(); olpf.reset();
    sTotal.reset(sr, .02); sWet.reset(sr, .02);
    sTotal.setCurrentAndTargetValue(0.f);
    sWet.setCurrentAndTargetValue(1.f);
    loadedCab = -1;
    loadCab();
    setLatencySamples(amp.getLatencySamples(raw[OS]->load() > .5f ? 1 : 0));
}

void HZProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nIn = getTotalNumInputChannels(), nOut = getTotalNumOutputChannels();
    if (n > (int) mono.size()) { buffer.clear(); return; } // host exceeded the prepared block size

    float* m = mono.data();
    float* dr = dry.data();
    float inPeak = 0.f;
    for (int i = 0; i < n; ++i)
    {
        float s = 0.f;
        for (int c = 0; c < nIn; ++c) s += buffer.getReadPointer(c)[i];
        m[i] = nIn > 0 ? s / (float) nIn : 0.f;
        inPeak = std::max(inPeak, std::fabs(m[i]));
    }

    AmpParams p;
    for (int i = 0; i < NUM_PARAMS; ++i) p[(size_t) i] = raw[(size_t) i]->load();

    amp.setParams(p);
    amp.process(m, n);

    // cabinet
    std::copy(m, m + n, dr);
    float* chans[1] = { m };
    juce::dsp::AudioBlock<float> blk(chans, 1, (size_t) n);
    conv.process(juce::dsp::ProcessContextReplacing<float>(blk));

    // output stage
    ohpf.design(Biquad::HP, sr, p[OHPF], .7);
    olpf.design(Biquad::LP, sr, p[OLPF], .7);
    const float master = std::pow(p[MASTER] / 10.f, 2.f) * 1.6f;
    sTotal.setTargetValue(master * dbToLin(p[OUT]));
    sWet.setTargetValue(p[CABMIX] / 100.f);

    float outPeak = 0.f;
    for (int i = 0; i < n; ++i)
    {
        const float w = sWet.getNextValue();
        float y = m[i] * w + dr[i] * (1.f - w) * .6f;
        y = olpf.process(ohpf.process(y)) * sTotal.getNextValue();
        y = std::clamp(limiter.process(y), -1.f, 1.f);
        m[i] = y;
        outPeak = std::max(outPeak, std::fabs(y));
    }

    for (int c = 0; c < nOut; ++c) std::copy(m, m + n, buffer.getWritePointer(c));

    inLevel.store(std::max(inPeak, inLevel.load() * .9f));
    outLevel.store(std::max(outPeak, outLevel.load() * .9f));
}

juce::AudioProcessorEditor* HZProcessor::createEditor() { return new HZEditor(*this); }

void HZProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void HZProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HZProcessor(); }
