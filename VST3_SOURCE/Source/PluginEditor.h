#pragma once
#include "PluginProcessor.h"
#include <JuceHeader.h>
#include <map>

class PedalLookAndFeel final:public juce::LookAndFeel_V4{
public:
 juce::Colour accent=juce::Colour(0xffff8b28);
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};

class MeowcillatorEditor final: public juce::AudioProcessorEditor,private juce::Timer {
public:
 explicit MeowcillatorEditor(MeowcillatorProcessor&);
 ~MeowcillatorEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
 void mouseDown(const juce::MouseEvent&) override;
 void mouseDrag(const juce::MouseEvent&) override;
 void mouseUp(const juce::MouseEvent&) override;
private:
 MeowcillatorProcessor& proc;
 PedalLookAndFeel skin;
 juce::Image cat,wear;
 juce::Rectangle<int> padArea;
 juce::ComboBox presets;
 juce::TextButton prevPreset{"‹"},nextPreset{"›"},playButton{"▶ PLAY"},helpButton{"?"};
 juce::Label displayName,displayStatus;
 std::map<juce::String,std::unique_ptr<juce::Slider>> dials;
 std::map<juce::String,std::unique_ptr<juce::ComboBox>> combos;
 std::map<juce::String,std::unique_ptr<juce::ToggleButton>> toggles;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttach;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAttach;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggleAttach;
 std::map<juce::Component*,juce::Rectangle<int>> baseBounds;
 std::array<juce::TextButton,6> sampleTriggers{};
 juce::Colour accent() const;
 void drawPanel(juce::Graphics&,juce::Rectangle<int>,const juce::String&) const;
 void drawText(juce::Graphics&,const juce::String&,juce::Rectangle<int>,float,bool=false,juce::Colour=juce::Colours::white) const;
 void dial(const juce::String&,const juce::String&,juce::Rectangle<int>);
 void combo(const juce::String&,juce::StringArray,juce::Rectangle<int>);
 void toggle(const juce::String&,const juce::String&,juce::Rectangle<int>);
 void timerCallback() override;
 void padAt(juce::Point<float>);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeowcillatorEditor)
};
