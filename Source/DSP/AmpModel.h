#pragma once
#include "../Params.h"
#include <array>

namespace hz
{
using AmpParams = std::array<float, NUM_PARAMS>;

// Everything from input to power amp (gate, pedals, preamp, amp, power stage).
// Cabinet and output stage live in the processor. A neural model can replace
// HZMachineDSP behind this interface without touching the rest of the plugin.
class AmpModel
{
public:
    virtual ~AmpModel() = default;
    virtual void prepare(double sampleRate, int maxBlock) = 0;
    virtual void reset() = 0;
    virtual void setParams(const AmpParams& p) = 0;
    virtual void process(float* mono, int numSamples) = 0; // in place
    virtual int getLatencySamples(int qualityIndex) const = 0;
};

// Placeholder for the future ONNX amp model (link ONNX Runtime, load a model in
// loadModel(), run inference in process()). Currently a pass-through.
class HZMachineNeural : public AmpModel
{
public:
    bool loadModel(const char* /*onnxPath*/) { return false; }
    void prepare(double, int) override {}
    void reset() override {}
    void setParams(const AmpParams&) override {}
    void process(float*, int) override {}
    int getLatencySamples(int) const override { return 0; }
};
} // namespace hz
