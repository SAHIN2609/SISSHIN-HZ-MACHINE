#include "PluginEditor.h"
#include "BinaryData.h"

HZEditor::HZEditor(HZProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    for (auto& d : paramDefs()) relays.push_back(std::make_unique<juce::WebSliderRelay>(d.id));

    using Opt = juce::WebBrowserComponent::Options;
    auto opts = Opt{}
                    .withBackend(Opt::Backend::webview2)
                    .withWinWebView2Options(Opt::WinWebView2{}.withUserDataFolder(
                        juce::File::getSpecialLocation(juce::File::tempDirectory)))
                    .withNativeIntegrationEnabled()
                    .withResourceProvider([this](const juce::String& url) { return getResource(url); });
    for (auto& r : relays) opts = opts.withOptionsFrom(*r);

    browser = std::make_unique<juce::WebBrowserComponent>(opts);
    addAndMakeVisible(*browser);

    const auto& defs = paramDefs();
    for (size_t i = 0; i < defs.size(); ++i)
        attachments.push_back(std::make_unique<juce::WebSliderParameterAttachment>(
            *proc.apvts.getParameter(defs[i].id), *relays[i], nullptr));

    browser->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    setSize(1120, 700);
    startTimerHz(30);
}

HZEditor::~HZEditor() { stopTimer(); }

void HZEditor::resized() { browser->setBounds(getLocalBounds()); }

std::optional<juce::WebBrowserComponent::Resource> HZEditor::getResource(const juce::String& url) const
{
    auto make = [](const char* data, int size, const char* mime) {
        const auto* b = reinterpret_cast<const std::byte*>(data);
        return juce::WebBrowserComponent::Resource{ std::vector<std::byte>(b, b + size), juce::String(mime) };
    };
    const auto path = (url == "/" || url.isEmpty()) ? juce::String("/index.html") : url;
    if (path == "/index.html") return make(BinaryData::index_html, BinaryData::index_htmlSize, "text/html");
    if (path == "/js/juce/index.js") return make(BinaryData::index_js, BinaryData::index_jsSize, "text/javascript");
    return std::nullopt;
}

void HZEditor::timerCallback()
{
    auto* o = new juce::DynamicObject();
    o->setProperty("in", proc.inLevel.load());
    o->setProperty("out", proc.outLevel.load());
    browser->emitEventIfBrowserIsVisible("meters", juce::var(o));
}
