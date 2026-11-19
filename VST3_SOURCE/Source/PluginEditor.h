#pragma once
#include "PluginProcessor.h"
#include <JuceHeader.h>
#include <optional>

// JUCE WebView2 hosts the exact MEOWCILLATOR 0.9.5 responsive artwork.
// Mouse/MIDI parameters and spectrum are exchanged with native JUCE DSP.
class MeowcillatorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit MeowcillatorEditor(MeowcillatorProcessor&);
    ~MeowcillatorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    MeowcillatorProcessor& processor;
    juce::WebBrowserComponent browser;
    juce::dsp::FFT fft{8};
    std::array<float,512> fftBuffer{};
    bool browserReady=false;
    unsigned stateCounter=0;
    juce::WebBrowserComponent::Options browserOptions();
    void nativeMessage(const juce::var&);
    void sendState();
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeowcillatorEditor)
};
