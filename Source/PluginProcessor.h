#pragma once
#include "DSP/HZMachineDSP.h"
#include "DSP/DspUtils.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>

class HZProcessor : public juce::AudioProcessor,
                    private juce::AudioProcessorValueTreeState::Listener,
                    private juce::AsyncUpdater
{
public:
    HZProcessor();
    ~HZProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SISSHIN HZ MACHINE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> inLevel { 0.f }, outLevel { 0.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged(const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override;
    void loadCab();

    hz::HZMachineDSP amp;
    juce::dsp::Convolution conv;
    hz::Biquad ohpf, olpf;
    hz::Comp limiter;
    juce::SmoothedValue<float> sTotal, sWet;
    std::vector<std::atomic<float>*> raw;
    std::vector<float> mono, dry;
    double sr = 48000;
    int loadedCab = -1;
};
