#pragma once
#include "PluginProcessor.h"
#include <JuceHeader.h>
#include <functional>
#include <memory>
#include <vector>

// Pure JUCE. No WebView2, JavaScript, browser process, event bridge, or HTML.
class MeowcillatorEditor final : public juce::AudioProcessorEditor,
                                private juce::MidiKeyboardState::Listener,
                                private juce::Timer
{
public:
    explicit MeowcillatorEditor(MeowcillatorProcessor&);
    ~MeowcillatorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment=juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment=juce::AudioProcessorValueTreeState::ButtonAttachment;
    class NativeStyle;
    class Knob;
    class XYPad;
    class LearnButton;
    class LearnCombo;
    MeowcillatorProcessor& proc;
    std::unique_ptr<NativeStyle> style;
    std::unique_ptr<XYPad> xy;
    juce::MidiKeyboardState keyboardState;
    juce::MidiKeyboardComponent keyboard{keyboardState,juce::MidiKeyboardComponent::horizontalKeyboard};
    struct KnobEntry { std::unique_ptr<Knob> component; std::unique_ptr<SliderAttachment> attachment; juce::String id; juce::Rectangle<float> base; };
    struct SelectEntry { std::unique_ptr<LearnCombo> component; std::unique_ptr<ComboAttachment> attachment; juce::String id; juce::Rectangle<float> base; };
    std::vector<KnobEntry> knobs;
    std::vector<SelectEntry> choices;
    struct ToggleEntry { std::unique_ptr<LearnButton> button; std::unique_ptr<ButtonAttachment> attachment; juce::Rectangle<float> base; };
    std::vector<ToggleEntry> toggles;
    juce::ComboBox presetSelect;
    std::unique_ptr<LearnButton> transport;
    juce::TextButton panic{"PANIC"};
    juce::Label info;
    juce::Label keyboardLabel;
    std::array<std::unique_ptr<LearnButton>,5> pads;
    std::array<unsigned,5> previousDrumHits{};
    std::array<float,5> padGlow{};
    juce::Image cat;
    int lastPreset=-1;
    bool stoppedByUser=false;
    void addKnob(const char* id,const char* label,float x,float y,float w,float h);
    void addChoice(const char* id,float x,float y,float w,float h);
    void addToggle(const char* id,const char* title,float x,float y,float w,float h);
    void handleNoteOn(juce::MidiKeyboardState*,int channel,int note,float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*,int channel,int note,float velocity) override;
    void timerCallback() override;
    void updateTransportLabel();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeowcillatorEditor)
};
