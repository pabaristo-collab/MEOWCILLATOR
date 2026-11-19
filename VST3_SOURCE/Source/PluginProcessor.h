#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include "SynthSoundBank.h"
#include "DrumSoundBank.h"

class MeowcillatorProcessor final : public juce::AudioProcessor, private juce::Timer {
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
  static juce::StringArray grooveNames(), kitNames();
  static int rhythmBpm(int index);
  // MIDI Learn: 27 native parameters + 18 performance actions.
  static constexpr int MidiParameterCount=27;
  static constexpr int MidiTargetCount=57; // Appended independent PAD FX targets; legacy CC IDs unchanged.
  static constexpr int FxTimeMidiTarget=53;
  static constexpr int FxSpeedMidiTarget=54;
  static constexpr int PadReverbMidiTarget=55;
  static constexpr int PadCrunchMidiTarget=56;
  static const char* midiTargetName(int index);
  int findMidiTarget(const juce::String& id) const;
  void beginMidiLearn(const juce::String& id, bool relative=false);
  void forgetMidiMapping(const juce::String& id);
  void clearMidiMappings();
  void cancelMidiLearn();
  juce::var midiMappingState() const;
  int pendingMidiLearn() const { return learnTarget.load(std::memory_order_acquire); }
  std::atomic<unsigned> midiMapRevision{0};
  void clearMidiOverride(const juce::String& id);
  // Read the immediate realtime MIDI override, not the delayed host-facing APVTS mirror.
  float valueForUi(const char* id) const { return parameterForAudio(id); }

  juce::AudioProcessorValueTreeState apvts;
  std::atomic<int> program{0};
  std::atomic<int> editorWidth{1200},editorHeight{750};
  std::atomic<float> padX{.5f},padY{.5f},audioLevel{0.f};
  std::atomic<bool> padDown{false},play{false};
  std::atomic<bool> requestPanic{false}; // Native one-click panic; handled only in audio callback.
  std::atomic<int> currentStep{0};
  // Keyboard notes trigger the native synth and animate the original X/Y surface.
  // Five playable on-screen keyboard keys and external MIDI both use the same native engine.
  std::array<std::atomic<bool>,128> uiKeyboardNotes{};
  std::array<bool,128> previousUiKeyboardNotes{};
  // Incoming MIDI monitor for DAW routing diagnosis. GUI reads atomics only.
  std::atomic<unsigned> incomingMidiCount{0};
  std::atomic<int> incomingMidiType{0}; // 1=NOTE, 2=CC, 3=PITCH
  std::atomic<int> incomingMidiChannel{0};
  std::atomic<int> incomingMidiNumber{0};
  std::atomic<int> incomingMidiValue{0};
  std::atomic<int> midiVisualNote{-1};
  std::atomic<bool> midiVisualActive{false};
  std::atomic<float> midiVisualY{.52f};
  // Each pad uses an atomic hit counter: rapid WebView clicks are never
  // discarded merely because they arrive in the same audio block.
  std::array<std::atomic<unsigned>,8> drumPadRequests{};
  // Audio-driven hit monitors: pattern, mouse and external MIDI use the same
  // realtime counters. The WebView only reads them at GUI frame rate.
  std::array<std::atomic<unsigned>,8> drumHitCounts{};
  std::array<std::atomic<int>,8> drumHitVelocities{};
  // Lightweight realtime-safe mono audio tap used ONLY by the GUI visualizer.
  // Display thread performs analysis; audio callback only stores samples.
  std::array<std::atomic<float>, 256> visualTap{};
  std::atomic<unsigned> visualWriteHead{0};
  // The original synthesizer retains its feline presets; obsolete cat-effect voices are removed.
private:
  // Audio thread only touches atomics and fixed-size arrays (no allocation/GUI calls).
  // Mapping encoding: 1..2048 for CC channel+number, 4097..6144 for note trigger;
  // 8193..8208 for 14-bit pitch wheel by channel. Zero means unassigned.
  std::array<std::atomic<int>,MidiTargetCount> midiMappings{};
  std::array<std::atomic<float>,MidiParameterCount> midiRealtimeValue{};
  std::array<std::atomic<bool>,MidiParameterCount> midiRealtimeEnabled{};
  std::array<std::atomic<bool>,MidiParameterCount> midiUpdatePending{};
  std::array<std::atomic<bool>,MidiTargetCount> midiTriggerHigh{};
  // Appended MIDI Learn target for delay time; previous target indices are stable.
  std::atomic<float> midiFxTimeValue{290.f};
  std::atomic<float> midiFxSpeedValue{1.2f};
  std::atomic<bool> midiFxSpeedEnabled{false},midiFxSpeedUpdatePending{false};
  std::atomic<bool> midiFxTimeEnabled{false},midiFxTimeUpdatePending{false};
  std::atomic<float> midiPadReverbValue{.16f},midiPadCrunchValue{.12f};
  std::atomic<bool> midiPadReverbEnabled{false},midiPadReverbUpdatePending{false};
  std::atomic<bool> midiPadCrunchEnabled{false},midiPadCrunchUpdatePending{false};
  std::atomic<int> learnTarget{-1};
  std::atomic<bool> learnRelative{false};
  std::atomic<unsigned> lastLearnedCode{0};
  void handleMidiMapping(const juce::MidiMessage& msg);
  void applyMidiTarget(int target, float normalized, bool noteMessage=false);
  bool mappedControlNote(const juce::MidiMessage& msg) const;
  void timerCallback() override; // host parameter notifications on message thread
  float parameterForAudio(const char* id) const;
  using Voice=meow::SynthVoice;
  std::array<Voice,24> voices{};
  std::array<bool,128> midiNotes{};          // ARP notes (physical or latched by HOLD)
  std::array<bool,128> physicalMidiNotes{};  // Actual physical key state, not latched
  std::array<bool,128> soundingInputNotes{}; // Direct audio voices started from MIDI
  std::array<bool,128> forwardedInputNotes{};// MIDI output Note On requiring Note Off
 struct ScheduledOff{int channel=10,note=0,samples=0;bool active=false;};
 std::array<ScheduledOff,24> pendingOffs{};
  double sr=44100., drumPhase=0., arpPhase=0.;
  bool lastPadDown=false,lastHoldMode=false,lastArpMode=false,arpHadSource=false,lastPlay=false;
  int heldMidiCount=0;
  int lastGroove=-1,lastPadCC=-1;
  int lastPadNote=-1,lastDirectPadNote=-1,lastPadMidiNote=-1,lastArpNote=-1,lastArpMidiNote=-1;
  int arpStep=0,arpGateSamples=0,stepIndex=0;
  float prevHz=220.f,midiExpressionY=.52f;
  meow::DrumSynth drumEngine;    // 11 untouched sequenced patterns
  meow::DrumSynth padEngine;     // independent manually played pad FX bus
  std::array<float,16384> padVerbL{},padVerbR{};
  int padVerbPosition=0;float padVerbDampL=0.f,padVerbDampR=0.f;
  uint32_t randState=0x8941725u;
  unsigned scopeDivider=0, scopeWrite=0;
  float delayL[96000]{}, delayR[96000]{}; int delayIndex=0;float delaySamplesSmoothed=0.f,fxLfoPhase=0.f;
  int noteFromPad(float x,int key,int octave,int scale) const;
  void noteOn(int,float,float,int,float);
  void noteOff(int,int=-1);
  void fireDrums(int,int,juce::MidiBuffer&,int,bool);
  void strikeDrumPad(int,float,juce::MidiBuffer&,int,bool);
  void reportDrumHit(int pad,float velocity);
  void fireArp(juce::MidiBuffer&,int,int,float,bool);
  float renderVoice(Voice&,int,float,float,float);
  float noise();
  static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeowcillatorProcessor)
};
