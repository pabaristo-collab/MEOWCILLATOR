#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>

class MeowcillatorProcessor final : public juce::AudioProcessor {
public:
  MeowcillatorProcessor();
  void prepareToPlay(double, int) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout&) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "MEOWCILLATOR by pablo aristo"; }
  bool acceptsMidi() const override {return true;}
  bool producesMidi() const override {return true;}
  bool isMidiEffect() const override {return false;}
  double getTailLengthSeconds() const override {return 1.5;}
  int getNumPrograms() override {return 37;}
  int getCurrentProgram() override {return program.load();}
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int) override;
  void changeProgramName(int,const juce::String&) override {}
  void getStateInformation(juce::MemoryBlock&) override;
  void setStateInformation(const void*,int) override;
  static juce::StringArray grooveNames(), kitNames(), catNames();
  static int rhythmBpm(int index);
  juce::AudioProcessorValueTreeState apvts;
  std::atomic<int> program{0};
  std::atomic<float> padX{.5f},padY{.5f},audioLevel{0.f};
  std::atomic<bool> padDown{false},play{false};
  std::atomic<int> triggerCat{-1}, lastCat{-1}, currentStep{0};
  // Six internal synthesized feline voices: no external audio files.
private:
  struct Voice{bool active=false,release=false;int note=60,source=0;float phase=0,subphase=0,env=0,vel=0,y=.5f,filter=0,currentHz=440,targetHz=440;};
  std::array<Voice,24> voices{};
  struct CatVoice { bool active=false; int kind=0; float age=0,phase=0,filter=0; };
  std::array<CatVoice,12> catVoices{};
  std::array<bool,128> midiNotes{};
 struct ScheduledOff{int channel=10,note=0,samples=0;bool active=false;};
 std::array<ScheduledOff,24> pendingOffs{};
  double sr=44100., drumPhase=0., arpPhase=0.;
  bool lastPadDown=false,lastHoldMode=false;int lastPadNote=-1,lastArpNote=-1,arpStep=0,arpGateSamples=0,stepIndex=0;
  float prevHz=220.f;
  float kickEnv=0,snareEnv=0,hatEnv=0,kickPhase=0,snareFilter=0;
  uint32_t randState=0x8941725u;
  float delayL[96000]{}, delayR[96000]{}; int delayIndex=0;
  int noteFromPad(float x,int key,int octave,int scale) const;
  void noteOn(int,float,float,int,float);
  void noteOff(int,int=-1);
  void fireDrums(int,int,juce::MidiBuffer&,int,bool);
  void fireArp(juce::MidiBuffer&,int,int,float,bool);
  void triggerCatVoice(int);
  float renderCatVoices();
  float renderVoice(Voice&,int,float,float,float);
  float noise();
  static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeowcillatorProcessor)
};
