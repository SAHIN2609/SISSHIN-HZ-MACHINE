#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <optional>
#include <vector>

class HZEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HZEditor(HZProcessor&);
    ~HZEditor() override;
    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url) const;
    void timerCallback() override;

    HZProcessor& proc;
    // Declaration order matters: relays outlive the browser, which outlives the attachments.
    std::vector<std::unique_ptr<juce::WebSliderRelay>> relays;
    std::unique_ptr<juce::WebBrowserComponent> browser;
    std::vector<std::unique_ptr<juce::WebSliderParameterAttachment>> attachments;
};
