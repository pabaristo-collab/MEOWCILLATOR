#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RhythmPatterns.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace {
constexpr float twoPi=juce::MathConstants<float>::twoPi;
static const char* presetLabels[37]={
 "ARISTO MEOW","HARDCORE BARK","CAT FUZZ LEAD","BITTEN GUITAR","FEEDBACK QUEEN","NOISE RIFF",
 "COLD CAVE CHOIR","VAMPIRE PAD","MIDNIGHT MIST","SAD CAT ORGAN","NEON GHOST","SLOW MOTION",
 "POST-PUNK BASS","COLD BASS","CHORUS CHIME","CHIME RIFF","DEAD DISCO","DUSTY STRINGS",
 "CYBER HATE","EBM MACHINE","METAL THROAT","RUSTY ALARM","FACTORY SIREN","MECHANICAL NYAN",
 "PURR 808","VINYL CAT","BOOM BASS","TAPE KEYS","LO-FI CHORD","MEMPHIS BELL",
 "8-BIT NYAN","CIRCUIT BENT","LASER MEOW","RADIO GHOST","GLITCH NOISE","BORKED CONSOLE","ACCORDION CAT"};
static const char* rhythms[11]={"POST-PUNK","DARKWAVE","GOTH ROCK","INDUSTRIAL EBM","HARDCORE PUNK","POWERVIOLENCE","D-BEAT","COLDWAVE","DEATHROCK","SYNTH PUNK","CRUST PUNK"};
static const int rhythmTempos[11]={132,118,138,126,182,220,172,110,148,160,190};
static const char* kits[4]={"XEROX PUNK","COLD MACHINE","INDUSTRIAL METAL","DUSTY BOOM BAP"};
float pitchHz(int n){return 440.f*std::pow(2.f,(float(n)-69.f)/12.f);}
static constexpr const char* midiIds[MeowcillatorProcessor::MidiTargetCount]={
 "osc","drums","bpm","glide","cutoff","resonance","fxmix","drive","feedback","gate",
 "scale","key","octave","groove","kit","effect","rate","arpType","color","motion","motionPower",
 "arp","hold","drumsOn","midiSynth","midiDrums","midiCats",
 "preset","play","padX","padY","padDown",
 "cat0","cat1","cat2","cat3","cat4","cat5",
 "presetPrev","presetNext","groovePrev","grooveNext","kitPrev","kitNext","fxToggle",
 "drumPad0","drumPad1","drumPad2","drumPad3","drumPad4","drumPad5","drumPad6","drumPad7",
 "fxTime" // New mapping appended: old MIDI assignments remain index-compatible.
};
// Raw units for native parameters. Choice and bool values are integral.
static constexpr float minRaw[27]={0,0,40,0,150,0,0,0,0,.1f,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static constexpr float maxRaw[27]={1,1,220,1,10000,1,1,1,.85f,1,4,11,4,10,3,4,3,2,4,3,1,1,1,1,1,1,1};

}

juce::StringArray MeowcillatorProcessor::grooveNames(){juce::StringArray a;for(auto s:rhythms)a.add(s);return a;}
juce::StringArray MeowcillatorProcessor::kitNames(){juce::StringArray a;for(auto s:kits)a.add(s);return a;}
int MeowcillatorProcessor::rhythmBpm(int i){return rhythmTempos[juce::jlimit(0,10,i)];}
juce::AudioProcessorValueTreeState::ParameterLayout MeowcillatorProcessor::makeParameters(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 auto f=[&](const char* id,const char* name,float lo,float hi,float def,float step=.001f){p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>{lo,hi,step},def));};
 auto b=[&](const char* id,const char* name,bool def){p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},name,def));};
 auto ch=[&](const char* id,const char* name,juce::StringArray opts,int def){p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},name,opts,def));};
 f("osc","SYNTH VOL",0,1,.65f); f("drums","DRUM VOL",0,1,.6f); f("bpm","BPM",40,220,132,1.f);
 f("glide","GLIDE",0,1,0.f); f("cutoff","CUTOFF",150,10000,6000,1.f);f("resonance","RESONANCE",0,1,.35f);
 f("fxmix","FX MIX",0,1,.30f);f("drive","DRIVE",0,1,.35f);f("feedback","FEEDBACK",0,.85f,.35f);
 f("fxTime","DELAY TIME (ms)",60.f,900.f,290.f,1.f);
 f("gate","ARP GATE",.10f,1.f,.60f);
 ch("scale","SCALE",{"MIN PENT","MAJOR","MINOR","BLUES","CHROMATIC"},0);
 ch("key","KEY",{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},0);
 ch("octave","OCTAVE",{"-2","-1","0","+1","+2"},2);
 ch("groove","RHYTHM",grooveNames(),0);ch("kit","DRUM SOUND",kitNames(),0);
 ch("effect","FX",{"OFF","TAPE DELAY","BIT CRUSH","SATURATOR","SPACE / ECHO"},0);
 ch("rate","ARP RATE",{"1/4","1/8","1/16","1/32"},2);
 ch("arpType","ARP TYPE",{"UP","DOWN","RANDOM"},0);
 ch("color","UI COLOR",{"BLACK CAT","ORANGE CAT","BLUSH","ICE","ACID"},0);
 ch("motion","PAD MOTION",{"LIQUID","PULSE","SPECTRUM","GLITCH"},0);
 f("motionPower","MOTION POWER",0,1,.74f);
 b("arp","ARP",false);b("hold","HOLD",false);b("drumsOn","DRUMS ON",true);
 b("midiSynth","SEND SYNTH MIDI CH1",true);b("midiDrums","SEND DRUM MIDI CH10",true);
 b("midiCats","LEGACY (UNUSED)",false); // Retained only for older project states.
 b("splitOut","SPLIT DRUMS TO OUTPUT 3-4",false);
 return {p.begin(),p.end()};
}
MeowcillatorProcessor::MeowcillatorProcessor()
 :AudioProcessor(BusesProperties().withOutput("SYNTH",juce::AudioChannelSet::stereo(),true)
   .withOutput("DRUMS + PADS",juce::AudioChannelSet::stereo(),true)),apvts(*this,nullptr,"MEOWCILLATOR_STATE",makeParameters())
{
 for(auto& e:midiMappings)e.store(0);
 for(auto& e:midiRealtimeEnabled)e.store(false);
 for(auto& e:midiUpdatePending)e.store(false);
 for(auto& e:midiRealtimeValue)e.store(0.f);
 startTimerHz(60); // Audio values change on MIDI arrival; host notifications stay on GUI thread.
}
void MeowcillatorProcessor::setCurrentProgram(int i){program=juce::jlimit(0,36,i);}
const juce::String MeowcillatorProcessor::getProgramName(int i){return presetLabels[juce::jlimit(0,36,i)];}
const char* MeowcillatorProcessor::midiTargetName(int index){
 return index>=0&&index<MidiTargetCount?midiIds[index]:"";
}
int MeowcillatorProcessor::findMidiTarget(const juce::String& id) const{
 for(int i=0;i<MidiTargetCount;++i)if(id==midiIds[i])return i;
 return -1;
}
void MeowcillatorProcessor::beginMidiLearn(const juce::String& id,bool relative){
 const int t=findMidiTarget(id);
 if(t>=0){learnRelative.store(relative);learnTarget.store(t,std::memory_order_release);midiMapRevision.fetch_add(1);}
}
void MeowcillatorProcessor::forgetMidiMapping(const juce::String& id){
 const int t=findMidiTarget(id);
 if(t>=0){midiMappings[t].store(0);midiTriggerHigh[t].store(false);
  if(t<MidiParameterCount){midiRealtimeEnabled[t].store(false);midiUpdatePending[t].store(false);}
  if(t==FxTimeMidiTarget){midiFxTimeEnabled.store(false);midiFxTimeUpdatePending.store(false);}
  midiMapRevision.fetch_add(1);
 }
}
void MeowcillatorProcessor::clearMidiMappings(){
 learnTarget.store(-1);
 for(int i=0;i<MidiTargetCount;++i){midiMappings[i].store(0);
  if(i<MidiParameterCount){midiRealtimeEnabled[i].store(false);midiUpdatePending[i].store(false);}}
 midiFxTimeEnabled.store(false);midiFxTimeUpdatePending.store(false);
 midiMapRevision.fetch_add(1);
}
void MeowcillatorProcessor::clearMidiOverride(const juce::String& id){
 const int t=findMidiTarget(id);
 if(t>=0 && t<MidiParameterCount){midiRealtimeEnabled[t].store(false);midiUpdatePending[t].store(false);}
 if(t==FxTimeMidiTarget){midiFxTimeEnabled.store(false);midiFxTimeUpdatePending.store(false);}
}
juce::var MeowcillatorProcessor::midiMappingState() const{
 juce::DynamicObject::Ptr data=new juce::DynamicObject();
 for(int t=0;t<MidiTargetCount;++t){const int code=midiMappings[t].load();
  if(!code)continue;
  const int ty=code>12288?3:code>8192?2:code>4096?1:0;
  const int n=ty==2?0:((code-1)&127);
  const int channel=ty==2?code-8192:((code-1)&2047)/128+1;
  juce::String label=ty==2?"Pitch Bend":ty==1?"Note "+juce::String(n):"CC "+juce::String(n)+(ty==3?" REL":"");
  data->setProperty(midiIds[t],"CH"+juce::String(channel)+" "+label);
 }
 return juce::var(data.get());
}
float MeowcillatorProcessor::parameterForAudio(const char* id) const{
 if(std::strcmp(id,"fxTime")==0 && midiFxTimeEnabled.load(std::memory_order_relaxed))return midiFxTimeValue.load(std::memory_order_relaxed);
 for(int t=0;t<MidiParameterCount;++t)
  if(std::strcmp(midiIds[t],id)==0){
   if(midiRealtimeEnabled[t].load(std::memory_order_relaxed))return midiRealtimeValue[t].load(std::memory_order_relaxed);
   break;
  }
 if(const auto* p=apvts.getRawParameterValue(id))return p->load();
 return 0.f;
}
void MeowcillatorProcessor::timerCallback(){
 // Audio thread never notifies JUCE/REAPER or allocates memory.
 if(midiFxTimeUpdatePending.exchange(false,std::memory_order_acq_rel))
  if(auto* p=apvts.getParameter("fxTime"))p->setValueNotifyingHost(p->convertTo0to1(midiFxTimeValue.load(std::memory_order_acquire)));
 for(int t=0;t<MidiParameterCount;++t)
  if(midiUpdatePending[t].exchange(false,std::memory_order_acq_rel)){
   const float raw=midiRealtimeValue[t].load(std::memory_order_acquire);
   if(auto* p=apvts.getParameter(midiIds[t]))
    p->setValueNotifyingHost(p->convertTo0to1(raw));
  }
}
void MeowcillatorProcessor::applyMidiTarget(int t,float normalized,bool noteMessage){
 const float n=juce::jlimit(0.f,1.f,normalized);
 if(t==FxTimeMidiTarget){
  midiFxTimeValue.store(60.f+n*840.f,std::memory_order_relaxed);
  midiFxTimeEnabled.store(true,std::memory_order_release);
  midiFxTimeUpdatePending.store(true,std::memory_order_release);
  return;
 }
 if(t<MidiParameterCount){
  // Buttons: toggle on each rising edge, ignoring note-off/CC release.
  if(t>=21 && t<=26){
   const bool high=noteMessage||n>.5f;
   if(high&&!midiTriggerHigh[t].load()){
    const float old=parameterForAudio(midiIds[t]);
    midiRealtimeValue[t].store(old>.5f?0.f:1.f);
    midiRealtimeEnabled[t].store(true);midiUpdatePending[t].store(true);
   }
   midiTriggerHigh[t].store(high);
  }else{
   const float raw=minRaw[t]+n*(maxRaw[t]-minRaw[t]);
   const float quantized=(t>=10&&t<=19)?std::round(raw):t==2?std::round(raw):raw;
   midiRealtimeValue[t].store(quantized,std::memory_order_relaxed);
   midiRealtimeEnabled[t].store(true,std::memory_order_release);
   midiUpdatePending[t].store(true,std::memory_order_release);
  }
  return;
 }
 if(t==27){program.store(juce::jlimit(0,36,(int)std::round(n*36.f)));return;}
 if(t==29){padX.store(n*.9999f);return;}
 if(t==30){padY.store(n);return;}
 if(t==31){padDown.store(n>.5f);return;}
 // Stateful actions trigger once per press, not on controller release.
 const bool high=noteMessage||n>.5f;
 if(high&&!midiTriggerHigh[t].load()){
  if(t==28)play.store(!play.load());
  // Former cat trigger indices 32-37 are reserved and intentionally inert.
  else if(t==38)program.store((program.load()+36)%37);
  else if(t==39)program.store((program.load()+1)%37);
  else if(t==40||t==41||t==42||t==43){
   const char* id=t<42?"groove":"kit";
   const int idx=t<42?13:14;
   const int max=t<42?11:4;
   const int cur=juce::jlimit(0,max-1,(int)parameterForAudio(id));
   const int delta=(t==40||t==42)?max-1:1;
   midiRealtimeValue[idx].store((float)((cur+delta)%max));midiRealtimeEnabled[idx].store(true);midiUpdatePending[idx].store(true);
  }
  else if(t>=45&&t<=52){drumPadRequests[static_cast<size_t>(t-45)].fetch_add(1,std::memory_order_release);}
  else if(t==44){ // FX on/off preserves the last default effect: tape delay
   const int current=(int)parameterForAudio("effect");
   midiRealtimeValue[15].store(current==0?1.f:0.f);
   midiRealtimeEnabled[15].store(true);midiUpdatePending[15].store(true);
  }
 }
 midiTriggerHigh[t].store(high);
}
bool MeowcillatorProcessor::mappedControlNote(const juce::MidiMessage& msg) const{
 if(!msg.isNoteOn()&&!msg.isNoteOff())return false;
 const int code=4097+(msg.getChannel()-1)*128+msg.getNoteNumber();
 for(int i=0;i<MidiTargetCount;++i)
  if(midiMappings[i].load(std::memory_order_relaxed)==code)return true;
 return false;
}
void MeowcillatorProcessor::handleMidiMapping(const juce::MidiMessage& msg){
 const bool cc=msg.isController(),note=msg.isNoteOn(),off=msg.isNoteOff(),bend=msg.isPitchWheel();
 if(!cc&&!note&&!off&&!bend)return;
 const int channel=msg.getChannel();
 const int code=bend?8192+channel:cc?1+(channel-1)*128+msg.getControllerNumber():4097+(channel-1)*128+msg.getNoteNumber();
 if(off){
  for(int t=0;t<MidiTargetCount;++t)
   if(midiMappings[t].load(std::memory_order_relaxed)==code)midiTriggerHigh[t].store(false);
  return;
 }
 const int pending=learnTarget.load(std::memory_order_relaxed);
 if(pending>=0 && pending<MidiTargetCount){
  // Do not capture piano notes as mappings for continuous knobs.
  if(cc||bend||(note&&pending>=27&&pending!=FxTimeMidiTarget)){
   midiMappings[pending].store(cc&&learnRelative.load()?code+12288:code,std::memory_order_release);
   learnTarget.store(-1,std::memory_order_release);
   midiMapRevision.fetch_add(1,std::memory_order_relaxed);
   return; // the learn gesture itself doesn't change the target
  }
 }
 const float n=bend?float(msg.getPitchWheelValue())/16383.f:cc?float(msg.getControllerValue())/127.f:1.f;
 for(int t=0;t<MidiTargetCount;++t){
  const int mapping=midiMappings[t].load(std::memory_order_relaxed);
  if(mapping==code){applyMidiTarget(t,n,note);continue;}
  if(cc&&mapping==code+12288){
   // Relative encoders: 1/127 and 63/65 formats both supported as directional ticks.
   const int val=msg.getControllerValue();
   const int direction=val==64||val==0?0:val<=63?1:-1;
   if(!direction)continue;
   if(t<MidiParameterCount && t<21){
    const float old=parameterForAudio(midiIds[t]);
    const float inc=(t>=10&&t<=19)?1.f: (maxRaw[t]-minRaw[t])/100.f;
    const float raw=juce::jlimit(minRaw[t],maxRaw[t],old+inc*direction);
    midiRealtimeValue[t].store(raw);midiRealtimeEnabled[t].store(true);midiUpdatePending[t].store(true);
   }else if(t==FxTimeMidiTarget){
    const float old=parameterForAudio("fxTime");
    const float raw=juce::jlimit(60.f,900.f,old+15.f*direction);
    midiFxTimeValue.store(raw);midiFxTimeEnabled.store(true);midiFxTimeUpdatePending.store(true);
   }else{
    midiTriggerHigh[t].store(false);
    applyMidiTarget(t,1.f,true);
    midiTriggerHigh[t].store(false);
   }
  }
 }
}

void MeowcillatorProcessor::prepareToPlay(double sampleRate,int){sr=sampleRate;for(auto& v:voices)v={};drumPhase=arpPhase=0;stepIndex=0;
 lastPadDown=lastHoldMode=lastArpMode=arpHadSource=false;heldMidiCount=0;lastPadNote=lastArpNote=lastDirectPadNote=lastPadMidiNote=lastArpMidiNote=-1;
 midiNotes.fill(false);physicalMidiNotes.fill(false);soundingInputNotes.fill(false);forwardedInputNotes.fill(false);
 arpGateSamples=0;arpStep=0;drumEngine.reset();for(auto& p:drumPadRequests)p.store(0);lastGroove=-1;lastPlay=false;lastPadCC=-1;
 juce::zeromem(delayL,sizeof(delayL));juce::zeromem(delayR,sizeof(delayR));delayIndex=0;delaySamplesSmoothed=float(sr)*.290f;scopeDivider=scopeWrite=0;visualWriteHead.store(0);for(auto& s:visualTap)s.store(0.f);}
bool MeowcillatorProcessor::isBusesLayoutSupported(const BusesLayout& l) const{
 if(l.getMainOutputChannelSet()!=juce::AudioChannelSet::stereo())return false;
 // Allow the auxiliary bus to be disabled by hosts that expose only stereo.
 if(l.outputBuses.size()!=2)return false;
 const auto aux=l.outputBuses[1];
 return aux==juce::AudioChannelSet::stereo() || aux==juce::AudioChannelSet::disabled();
}
float MeowcillatorProcessor::noise(){randState^=randState<<13;randState^=randState>>17;randState^=randState<<5;return float(randState&65535)/32768.f-1.f;}
int MeowcillatorProcessor::noteFromPad(float x,int key,int octave,int scale) const {
 static const std::array<std::vector<int>,5> scales={std::vector<int>{0,3,5,7,10},std::vector<int>{0,2,4,5,7,9,11},std::vector<int>{0,2,3,5,7,8,10},std::vector<int>{0,3,5,6,7,10},std::vector<int>{0,1,2,3,4,5,6,7,8,9,10,11}};
 const auto& a=scales[size_t(juce::jlimit(0,4,scale))];int n=juce::jlimit(0,int(a.size()*3)-1,int(x*float(a.size()*3)));
 return juce::jlimit(20,108,48+key+12*(n/int(a.size())-1+octave)+a[size_t(n%int(a.size()))]);
}
void MeowcillatorProcessor::noteOn(int note,float vel,float y,int source,float glide){
 Voice* slot=&voices[0];for(auto& v:voices)if(!v.active){slot=&v;break;}
 *slot={};slot->active=true;slot->source=source;slot->note=note;slot->vel=vel;slot->y=y;slot->targetHz=pitchHz(note+meow::patches[static_cast<size_t>(juce::jlimit(0,36,program.load()))].shift);
 // First touch and MIDI note-ons start at pitch immediately. Only an
 // intentional pad drag with GLIDE turned up interpolates from the last note.
 const bool shouldSlide = (source == 0 && lastPadDown && glide > .015f);
 slot->currentHz=shouldSlide?prevHz:slot->targetHz;prevHz=slot->targetHz;
 slot->patchIndex=juce::jlimit(0,36,program.load());
 slot->rng=0x4512u+static_cast<uint32_t>(note*131+source*191+program.load()*173);
}
void MeowcillatorProcessor::noteOff(int note,int source){for(auto& v:voices)if(v.active&&v.note==note&&(source<0||v.source==source))v.release=true;}
float MeowcillatorProcessor::renderVoice(Voice& v,int patch,float cutoff,float resonance,float glide){
 return meow::renderSynth(v,patch,cutoff,resonance,glide,float(sr));
}
void MeowcillatorProcessor::fireDrums(int groove,int kit,juce::MidiBuffer& midi,int sample,bool send){
    const auto& pat = meowRhythms[static_cast<std::size_t>(juce::jlimit(0,10,groove))];
    const unsigned s = static_cast<unsigned>(stepIndex) & 31u;
    const auto hit = [s](std::uint32_t mask) { return ((mask >> s) & 1u) != 0u; };
    const bool k=hit(pat.kick), sn=hit(pat.snare), h=hit(pat.hat), oh=hit(pat.openHat);
    const float accent=hit(pat.accent)?1.17f:0.82f;
    if(k)drumEngine.kick(accent);
    if(sn)drumEngine.snare(accent);
    if(h)drumEngine.hat(accent*(s%4==0?.80f:.55f));
    if(oh)drumEngine.openHat(accent*.70f);
    // Keep GM-standard drum notes on their own MIDI channel 10.
    auto add=[&](int note,int velocity){
        if(!send)return;
        midi.addEvent(juce::MidiMessage::noteOn(10,note,(juce::uint8)velocity),sample);
        for(auto& off:pendingOffs)if(!off.active){off={10,note,juce::jmax(1,int(sr*.06)),true};break;}
    };
    const int vel=juce::jlimit(1,127,int(100.f*accent));
    if(k)add(36,vel);
    if(sn)add(38,vel);
    if(h)add(42,juce::jlimit(1,127,int(74.f*accent)));
    if(oh)add(46,juce::jlimit(1,127,int(85.f*accent)));
    juce::ignoreUnused(kit);
    stepIndex=(stepIndex+1)&31;
    currentStep.store(stepIndex,std::memory_order_relaxed);
}
// Eight GM-compatible live pads, independent of the transport/sequencer.
// Pad NoteOns are emitted by this VST3 on channel 10, so REAPER can record
// output MIDI and swap the instrument for another drum plugin as requested.
void MeowcillatorProcessor::strikeDrumPad(int index,float vel,juce::MidiBuffer& midi,int sample,bool send){
 static constexpr int notes[8]={36,38,39,42,46,45,50,49};
 if(index<0||index>=8)return;
 const float gain=juce::jlimit(.05f,1.f,vel);
 switch(index){
  case 0:drumEngine.kick(gain);break;
  case 1:drumEngine.snare(gain);break;
  case 2:drumEngine.clap(gain);break;
  case 3:drumEngine.hat(gain*.8f);break;
  case 4:drumEngine.openHat(gain*.75f);break;
  case 5:drumEngine.lowTom(gain);break;
  case 6:drumEngine.highTom(gain);break;
  case 7:drumEngine.crash(gain);break;
 }
 if(send){
  const int note=notes[index];
  midi.addEvent(juce::MidiMessage::noteOn(10,note,(juce::uint8)juce::jlimit(1,127,int(gain*127.f))),sample);
  for(auto& off:pendingOffs)if(!off.active){off={10,note,juce::jmax(1,int(sr*.065)),true};break;}
 }
}
void MeowcillatorProcessor::fireArp(juce::MidiBuffer& midi,int sample,int mode,float glide,bool send){
 // A held MIDI keyboard takes priority over the X/Y pad, matching the web UI.
 std::array<int,132> chord{};int count=0;
 for(int n=0;n<128;n++)if(midiNotes[size_t(n)])chord[size_t(count++)]=n;
 if(!count && lastPadNote>=0){for(int add:{0,3,7,12})chord[size_t(count++)]=juce::jmin(127,lastPadNote+add);}
 if(!count)return;
 if(lastArpNote>=0){noteOff(lastArpNote,2);lastArpNote=-1;}
 // Close a MIDI note only if we previously emitted its Note On. Never gate
 // Note Off on the current 'sendSynth' setting: it may have changed mid-note.
 if(lastArpMidiNote>=0){midi.addEvent(juce::MidiMessage::noteOff(1,lastArpMidiNote),sample);lastArpMidiNote=-1;}
 int idx=arpStep++%count;
 if(mode==1)idx=count-1-idx;else if(mode==2)idx=int((randState>>5)%uint32_t(count));
 lastArpNote=chord[size_t(idx)];noteOn(lastArpNote,.84f,padY.load(),2,glide);
 if(send){midi.addEvent(juce::MidiMessage::noteOn(1,lastArpNote,(juce::uint8)108),sample);lastArpMidiNote=lastArpNote;}
}
void MeowcillatorProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;buffer.clear();const int frames=buffer.getNumSamples();
 // JUCE maps both VST3 output buses into a single processBlock buffer.
 // Use bus-aware views rather than assuming channels 2/3 are always active.
 auto synthBus=getBusBuffer(buffer,false,0);
 auto drumBus=getBusBuffer(buffer,false,1);
 // Map hardware CC/pitch wheel before DSP reads parameters for this block.
 // This path is allocation-free and does not block on the WebView renderer.
 for(const auto metadata:midi)handleMidiMapping(metadata.getMessage());
 auto v=[&](const char* id){return parameterForAudio(id);};
 int patch=program.load(),groove=juce::jlimit(0,10,int(v("groove"))),kit=int(v("kit")),key=int(v("key")),oct=int(v("octave"))-2,scale=int(v("scale"));
 bool drumsOn=v("drumsOn")>.5f,arp=v("arp")>.5f,hold=v("hold")>.5f;
 bool sendDrums=v("midiDrums")>.5f,sendSynth=v("midiSynth")>.5f;
 double bpm=v("bpm");float glide=v("glide"),oscVol=v("osc"),drumVol=v("drums"),cutoff=v("cutoff"),res=v("resonance"),gate=v("gate");
 const bool playing=play.load(std::memory_order_relaxed);
 const bool splitOut=v("splitOut")>.5f && drumBus.getNumChannels()==2;
 if(groove!=lastGroove){lastGroove=groove;stepIndex=0;drumPhase=playing?1.0:0.0;}
 if(playing&&!lastPlay){stepIndex=0;drumPhase=1.0;}
 lastPlay=playing;
 int rate=int(v("rate")),arpType=int(v("arpType")),fx=int(v("effect"));
 const float drive=v("drive"),fxmix=v("fxmix"),feedback=v("feedback");
 const float fxTimeMs=juce::jlimit(60.f,900.f,v("fxTime"));
 const float timeSmoothCoeff=juce::jlimit(.00001f,1.f,1.f/(float(sr)*.025f));
 juce::MidiBuffer input;input.swapWith(midi);juce::MidiBuffer output;auto it=input.begin(),end=input.end();
 // Active note bookkeeping is separate from the XY base note and from ARP.
 // This is essential: releasing HOLD must send balanced MIDI Note Offs even
 // when the pointer has been up for many audio blocks.
 auto endDirectPad=[&](int at){
  if(lastDirectPadNote>=0){noteOff(lastDirectPadNote,0);lastDirectPadNote=-1;}
  if(lastPadMidiNote>=0){output.addEvent(juce::MidiMessage::noteOff(1,lastPadMidiNote),at);lastPadMidiNote=-1;}
 };
 auto endArp=[&](int at){
  if(lastArpNote>=0){noteOff(lastArpNote,2);lastArpNote=-1;}
  if(lastArpMidiNote>=0){output.addEvent(juce::MidiMessage::noteOff(1,lastArpMidiNote),at);lastArpMidiNote=-1;}
  arpGateSamples=0;
 };
 auto endInput=[&](int n,int at){
  if(soundingInputNotes[size_t(n)]){noteOff(n,1);soundingInputNotes[size_t(n)]=false;}
  if(forwardedInputNotes[size_t(n)]){output.addEvent(juce::MidiMessage::noteOff(1,n),at);forwardedInputNotes[size_t(n)]=false;}
 };
 auto startInput=[&](int n,float velocity,int at){
  if(soundingInputNotes[size_t(n)])noteOff(n,1);
  noteOn(n,velocity,midiExpressionY,1,glide);soundingInputNotes[size_t(n)]=true;
  if(sendSynth){
   if(forwardedInputNotes[size_t(n)])output.addEvent(juce::MidiMessage::noteOff(1,n),at);
   output.addEvent(juce::MidiMessage::noteOn(1,n,(juce::uint8)juce::jlimit(1,127,int(velocity*127.f))),at);
   forwardedInputNotes[size_t(n)]=true;
  }
 };
 const bool holdJustOff=lastHoldMode&&!hold;
 const bool arpJustOn=!lastArpMode&&arp,arpJustOff=lastArpMode&&!arp;
 if(holdJustOff){
  // Keep only physically held keyboard keys; clear all previously latched keys.
  for(int n=0;n<128;++n)if(!physicalMidiNotes[size_t(n)]){
   if(midiNotes[size_t(n)]){midiNotes[size_t(n)]=false;--heldMidiCount;}
   endInput(n,0);
  }
  // A released XY pad is no longer a valid ARP source after HOLD turns off.
  if(!padDown.load(std::memory_order_relaxed))lastPadNote=-1;
  int lastHeld=-1;for(int k=127;k>=0;--k)if(physicalMidiNotes[size_t(k)]){lastHeld=k;break;}
  midiVisualNote.store(lastHeld);midiVisualActive.store(lastHeld>=0);
 }
 if(arpJustOn){
  // Transition DIRECT -> ARP: release direct voices and their MIDI output.
  endDirectPad(0);
  for(int n=0;n<128;++n)endInput(n,0);
  arpPhase=0.0;arpStep=0;arpHadSource=false; // first source pulses immediately
 }
 if(arpJustOff){
  arpHadSource=false;
  // Transition ARP -> DIRECT. Do not leave the final arpeggiated note ringing.
  endArp(0);
  for(int n=0;n<128;++n)if(midiNotes[size_t(n)])startInput(n,.84f,0);
 }
 if(holdJustOff && !padDown.load(std::memory_order_relaxed)){
  endDirectPad(0);
  // Stop an ARP note immediately when no physically held notes remain.
  if(heldMidiCount==0)endArp(0);
 }
 lastHoldMode=hold;lastArpMode=arp;
 // Accept simultaneous/repeated pad clicks without making WebView process audio.
 // Queue response at the start of the audio block. No allocations or waits.
 for(int p=0;p<8;++p){
  const auto hits=juce::jmin(8u,drumPadRequests[static_cast<size_t>(p)].exchange(0,std::memory_order_acq_rel));
  for(unsigned n=0;n<hits;++n)strikeDrumPad(p,.92f,output,0,sendDrums);
 }
  const float fs=float(sr);
 // Drum kit synthesis is isolated in DrumSoundBank.h. This intentionally
 // does not affect the 11 original 32-step rhythms or MIDI note assignments.
 float maxRms=0;
 float cachedPadX=-2.f;int cachedPadNote=-1;
 for(int i=0;i<frames;i++){
  for(auto& off:pendingOffs)if(off.active&&--off.samples<=0){output.addEvent(juce::MidiMessage::noteOff(off.channel,off.note),i);off.active=false;}
  while(it!=end&&(*it).samplePosition<=i){auto msg=(*it).getMessage();const int ch=msg.getChannel();
   if(mappedControlNote(msg)){++it;continue;}
   // REAPER sends CC120/123 on transport stop or MIDI panic. Release all
   // held and arpeggiated voices even if physical key-up events were lost.
   if(msg.isController() && (msg.getControllerNumber()==120 || msg.getControllerNumber()==123)){
    for(int n=0;n<128;++n){physicalMidiNotes[size_t(n)]=false;midiNotes[size_t(n)]=false;endInput(n,i);}
    heldMidiCount=0;lastPadNote=-1;lastPadDown=false;
    midiVisualNote.store(-1);midiVisualActive.store(false);
    endDirectPad(i);endArp(i);arpHadSource=false;
    if(sendSynth)output.addEvent(msg,i);
    ++it;continue;
   }
   if(ch==10&&msg.isNoteOn()){
    static constexpr int notes[8]={36,38,39,42,46,45,50,49};
    for(int d=0;d<8;++d)if(msg.getNoteNumber()==notes[d]){
      // Incoming MIDI is already on REAPER's input; do not echo it back.
      strikeDrumPad(d,msg.getFloatVelocity(),output,i,false);
      break;
    }
   }
   else if(msg.isController() && ch==1 && msg.getControllerNumber()==74){
    midiExpressionY=juce::jlimit(0.f,1.f,float(msg.getControllerValue())/127.f);
    midiVisualY.store(midiExpressionY,std::memory_order_relaxed);
    for(auto& voice:voices)if(voice.active&&voice.source==1)voice.y=midiExpressionY;
    if(sendSynth)output.addEvent(msg,i);
   }
   else if(msg.isNoteOn() && ch!=10){
    const int n=msg.getNoteNumber();physicalMidiNotes[size_t(n)]=true;
    midiVisualNote.store(n,std::memory_order_relaxed);midiVisualActive.store(true,std::memory_order_release);
    if(!midiNotes[size_t(n)]){midiNotes[size_t(n)]=true;++heldMidiCount;}
    if(!arp)startInput(n,msg.getFloatVelocity(),i);
   }
   else if(msg.isNoteOff() && ch!=10){
    const int n=msg.getNoteNumber();physicalMidiNotes[size_t(n)]=false;
    if(!hold && midiVisualNote.load(std::memory_order_relaxed)==n){
     int stillHeld=-1;for(int k=127;k>=0;--k)if(physicalMidiNotes[size_t(k)]){stillHeld=k;break;}
     midiVisualNote.store(stillHeld,std::memory_order_relaxed);midiVisualActive.store(stillHeld>=0,std::memory_order_release);
    }
    if(!hold){if(midiNotes[size_t(n)]){midiNotes[size_t(n)]=false;--heldMidiCount;}endInput(n,i);}
   }
   ++it;
  }
  const bool down=padDown.load(std::memory_order_relaxed);
  // Quantize the pad only while it is physically touched; HOLD stores the
  // last valid pitch after pointer-up without generating a second Note On.
  if(down){const float x=padX.load(std::memory_order_relaxed);
   if(x!=cachedPadX){cachedPadX=x;cachedPadNote=noteFromPad(x,key,oct,scale);}
   if(!lastPadDown || cachedPadNote!=lastPadNote)lastPadNote=cachedPadNote;
  }else if(!hold){lastPadNote=-1;}
  if(!arp){
   if(lastPadNote>=0 && (down||hold)){
    if(lastDirectPadNote!=lastPadNote){
     endDirectPad(i);
     lastDirectPadNote=lastPadNote;
     noteOn(lastDirectPadNote,.91f,padY.load(std::memory_order_relaxed),0,glide);
     if(sendSynth){output.addEvent(juce::MidiMessage::noteOn(1,lastDirectPadNote,(juce::uint8)110),i);lastPadMidiNote=lastDirectPadNote;}
     lastPadCC=-1;
    }
   }else endDirectPad(i);
  }else if(lastDirectPadNote>=0||lastPadMidiNote>=0)endDirectPad(i);
  // The vertical pad axis remains expressive after note-on, but doesn't
  // retrigger notes while dragging through the same quantized pitch.
  if(down){const float y=padY.load(std::memory_order_relaxed);
   for(auto& voice:voices)if(voice.active&&voice.source==0)voice.y=y;
  }
  if(down && sendSynth && ((i & 255)==0 || !lastPadDown)){
   const int cc=juce::jlimit(0,127,int(padY.load(std::memory_order_relaxed)*127.f));
   if(cc!=lastPadCC){output.addEvent(juce::MidiMessage::controllerEvent(1,74,cc),i);lastPadCC=cc;}
  }
  if(!down)lastPadCC=-1;
  lastPadDown=down;
  // Without an ARP source, its last note must stop immediately instead of
  // waiting for a scheduled gate or a later pointer event.
  if(arp){
   const bool hasSource=lastPadNote>=0||heldMidiCount>0;
   if(!hasSource){if(lastArpNote>=0||lastArpMidiNote>=0)endArp(i);arpPhase=0.;arpHadSource=false;}
   else{
    if(!arpHadSource){arpPhase=1.;arpHadSource=true;arpStep=0;}
    arpPhase+=bpm*double(1<<rate)/(60.*sr);
    if(arpPhase>=1.){
     arpPhase-=1.;fireArp(output,i,arpType,glide,sendSynth);
     arpGateSamples=juce::jmax(1,int(sr*60./bpm/double(1<<rate)*gate));
    }
    if(arpGateSamples>0&&--arpGateSamples==0)endArp(i);
   }
  }else if(lastArpNote>=0||lastArpMidiNote>=0)endArp(i);
  if(playing){drumPhase+=bpm*4./(60.*sr);if(drumPhase>=1.){drumPhase-=1.;if(drumsOn)fireDrums(groove,kit,output,i,sendDrums);}}
  float synth=0;for(auto& voice:voices)if(voice.active)synth+=renderVoice(voice,patch,cutoff,res,glide);
  synth*=oscVol;
  // Four actual drum sound engines: XEROX PUNK / COLD MACHINE /
  // INDUSTRIAL METAL / DUSTY BOOM BAP. Pattern timing remains untouched.
  const float drumsSample=drumEngine.render(kit,fs);
  // Routing is intentional: ONLY the played synthesizer (XY / incoming MIDI)
  // enters FX. The drum machine stays independent and dry.
  // Never mix drums into an FX buffer or a shared saturator/feedback path.
  const float drumsDry=drumVol*drumsSample;
  float synthLeft=synth, synthRight=synth;
  if(fx==1 || fx==4){ // TAPE DELAY / SPACE ECHO — synth-only stereo feedback
   const float goal=juce::jlimit(1.f,95998.f,float(sr)*(.001f*fxTimeMs));
   delaySamplesSmoothed+=(goal-delaySamplesSmoothed)*timeSmoothCoeff;
   float pos=float(delayIndex)+96000.f-delaySamplesSmoothed;
   if(pos>=96000.f)pos-=96000.f;
   const int idx0=int(pos);
   const int idx1=(idx0+1)%96000;
   const float frac=pos-float(idx0);
   const float tapLeft=juce::jmap(frac,delayL[idx0],delayL[idx1]);
   const float tapRight=juce::jmap(frac,delayR[idx0],delayR[idx1]);
   const float input=std::tanh(synth*(1.f+drive*2.f));
   delayL[delayIndex]=input+feedback*tapRight;
   delayR[delayIndex]=input+feedback*tapLeft;
   synthLeft=synth+fxmix*tapLeft;
   synthRight=synth+fxmix*tapRight;
  }else{
   // Clear the delay line while bypassed so switching FX can't expose an
   // old tail. This also guarantees dry drums never enter the delay memory.
   delayL[delayIndex]=0.f;delayR[delayIndex]=0.f;
   if(fx==2){ // BIT CRUSH — synth only
    const float levels=juce::jmap(drive,128.f,6.f);
    const float crushed=std::round(synth*levels)/levels;
    synthLeft=synthRight=juce::jmap(fxmix,synth,crushed);
   }else if(fx==3){ // SATURATOR — synth only
    const float distorted=std::tanh(synth*(1.f+drive*6.f));
    synthLeft=synthRight=juce::jmap(fxmix,synth,distorted);
   }
   // fx==0: a genuine FX bypass; drums stay untouched.
  }
  delayIndex=(delayIndex+1)%96000;
  // MIX (default): backward-compatible full audio in 1/2; aux muted.
  // SPLIT: synth in 1/2; dedicated drums/pads in 3/4.
  const float mainL=juce::jlimit(-.98f,.98f,synthLeft+(splitOut?0.f:drumsDry));
  const float mainR=juce::jlimit(-.98f,.98f,synthRight+(splitOut?0.f:drumsDry));
  synthBus.setSample(0,i,mainL);synthBus.setSample(1,i,mainR);
  if(drumBus.getNumChannels()==2){
   drumBus.setSample(0,i,splitOut?drumsDry:0.f);
   drumBus.setSample(1,i,splitOut?drumsDry:0.f);
  }
  const float outL=mainL+(splitOut?drumsDry:0.f), outR=mainR+(splitOut?drumsDry:0.f);
  if(++scopeDivider >= 12){scopeDivider=0;visualTap[scopeWrite].store((outL+outR)*.5f,std::memory_order_relaxed);scopeWrite=(scopeWrite+1u)&255u;visualWriteHead.store(scopeWrite,std::memory_order_release);}
  maxRms=juce::jmax(maxRms,juce::jmax(std::abs(outL),std::abs(outR)));
 }
 audioLevel=audioLevel.load()*.74f+maxRms*.26f;
 midi.swapWith(output);

}
void MeowcillatorProcessor::getStateInformation(juce::MemoryBlock& b){
 auto tree=apvts.copyState();tree.setProperty("program",program.load(),nullptr);
 for(int t=0;t<MidiTargetCount;++t)tree.setProperty(juce::Identifier("midi_"+juce::String(midiIds[t])),midiMappings[t].load(),nullptr);
 tree.setProperty("ui_width",editorWidth.load(),nullptr);
 tree.setProperty("ui_height",editorHeight.load(),nullptr);
 if(auto xml=tree.createXml())copyXmlToBinary(*xml,b);
}
void MeowcillatorProcessor::setStateInformation(const void* d,int n){
 auto xml=getXmlFromBinary(d,n);if(!xml)return;auto tree=juce::ValueTree::fromXml(*xml);
 if(!tree.isValid())return;program=juce::jlimit(0,36,int(tree.getProperty("program",0)));
 editorWidth.store(juce::jlimit(466,1208,(int)tree.getProperty("ui_width",675)));
 editorHeight.store(juce::jlimit(560,1450,(int)tree.getProperty("ui_height",810)));
 apvts.replaceState(tree);
 for(int t=0;t<MidiTargetCount;++t){const int code=(int)tree.getProperty(juce::Identifier("midi_"+juce::String(midiIds[t])),0);
   midiMappings[t].store((t>=32&&t<=37)?0:(code>=0&&code<=16384?code:0));
   if(t<MidiParameterCount){midiRealtimeEnabled[t].store(false);midiUpdatePending[t].store(false);}
  if(t==FxTimeMidiTarget){midiFxTimeEnabled.store(false);midiFxTimeUpdatePending.store(false);}}
 midiFxTimeEnabled.store(false);midiFxTimeUpdatePending.store(false);
 learnTarget.store(-1);midiMapRevision.fetch_add(1);
}
juce::AudioProcessorEditor* MeowcillatorProcessor::createEditor(){return new MeowcillatorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MeowcillatorProcessor();}
