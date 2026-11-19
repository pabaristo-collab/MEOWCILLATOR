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
static const char* feline[6]={"MEOW","PURR","YOWL","HISS","CHIRP","GROWL"};
float pitchHz(int n){return 440.f*std::pow(2.f,(float(n)-69.f)/12.f);}
static constexpr const char* midiIds[MeowcillatorProcessor::MidiTargetCount]={
 "osc","drums","bpm","glide","cutoff","resonance","fxmix","drive","feedback","gate",
 "scale","key","octave","groove","kit","effect","rate","arpType","color","motion","motionPower",
 "arp","hold","drumsOn","midiSynth","midiDrums","midiCats",
 "preset","play","padX","padY","padDown",
 "cat0","cat1","cat2","cat3","cat4","cat5",
 "presetPrev","presetNext","groovePrev","grooveNext","kitPrev","kitNext","fxToggle"
};
// Raw units for native parameters. Choice and bool values are integral.
static constexpr float minRaw[27]={0,0,40,0,150,0,0,0,0,.1f,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static constexpr float maxRaw[27]={1,1,220,1,10000,1,1,1,.85f,1,4,11,4,10,3,4,3,2,4,3,1,1,1,1,1,1,1};

}

juce::StringArray MeowcillatorProcessor::grooveNames(){juce::StringArray a;for(auto s:rhythms)a.add(s);return a;}
juce::StringArray MeowcillatorProcessor::kitNames(){juce::StringArray a;for(auto s:kits)a.add(s);return a;}
juce::StringArray MeowcillatorProcessor::catNames(){juce::StringArray a;for(auto s:feline)a.add(s);return a;}
int MeowcillatorProcessor::rhythmBpm(int i){return rhythmTempos[juce::jlimit(0,10,i)];}
juce::AudioProcessorValueTreeState::ParameterLayout MeowcillatorProcessor::makeParameters(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 auto f=[&](const char* id,const char* name,float lo,float hi,float def,float step=.001f){p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>{lo,hi,step},def));};
 auto b=[&](const char* id,const char* name,bool def){p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},name,def));};
 auto ch=[&](const char* id,const char* name,juce::StringArray opts,int def){p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},name,opts,def));};
 f("osc","SYNTH VOL",0,1,.65f); f("drums","DRUM VOL",0,1,.6f); f("bpm","BPM",40,220,132,1.f);
 f("glide","GLIDE",0,1,0.f); f("cutoff","CUTOFF",150,10000,6000,1.f);f("resonance","RESONANCE",0,1,.35f);
 f("fxmix","FX MIX",0,1,.30f);f("drive","DRIVE",0,1,.35f);f("feedback","FEEDBACK",0,.85f,.35f);
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
 b("midiSynth","SEND SYNTH MIDI CH1",true);b("midiDrums","SEND DRUM MIDI CH10",true);b("midiCats","SEND CAT MIDI CH16",true);
 return {p.begin(),p.end()};
}
MeowcillatorProcessor::MeowcillatorProcessor()
 :AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),apvts(*this,nullptr,"MEOWCILLATOR_STATE",makeParameters())
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
  midiMapRevision.fetch_add(1);
 }
}
void MeowcillatorProcessor::clearMidiMappings(){
 learnTarget.store(-1);
 for(int i=0;i<MidiTargetCount;++i){midiMappings[i].store(0);
  if(i<MidiParameterCount){midiRealtimeEnabled[i].store(false);midiUpdatePending[i].store(false);}}
 midiMapRevision.fetch_add(1);
}
void MeowcillatorProcessor::clearMidiOverride(const juce::String& id){
 const int t=findMidiTarget(id);
 if(t>=0 && t<MidiParameterCount){midiRealtimeEnabled[t].store(false);midiUpdatePending[t].store(false);}
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
 for(int t=0;t<MidiParameterCount;++t)
  if(midiUpdatePending[t].exchange(false,std::memory_order_acq_rel)){
   const float raw=midiRealtimeValue[t].load(std::memory_order_acquire);
   if(auto* p=apvts.getParameter(midiIds[t]))
    p->setValueNotifyingHost(p->convertTo0to1(raw));
  }
}
void MeowcillatorProcessor::applyMidiTarget(int t,float normalized,bool noteMessage){
 const float n=juce::jlimit(0.f,1.f,normalized);
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
  else if(t>=32&&t<=37)triggerCat.store(t-32);
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
  if(cc||bend||(note&&pending>=27)){
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
   }else{
    midiTriggerHigh[t].store(false);
    applyMidiTarget(t,1.f,true);
    midiTriggerHigh[t].store(false);
   }
  }
 }
}

void MeowcillatorProcessor::prepareToPlay(double sampleRate,int){sr=sampleRate;for(auto& v:voices)v={};for(auto& s:catVoices)s={};drumPhase=arpPhase=0;stepIndex=0;
 lastPadDown=lastHoldMode=lastArpMode=arpHadSource=false;heldMidiCount=0;lastPadNote=lastArpNote=lastDirectPadNote=lastPadMidiNote=lastArpMidiNote=-1;
 midiNotes.fill(false);physicalMidiNotes.fill(false);soundingInputNotes.fill(false);forwardedInputNotes.fill(false);
 arpGateSamples=0;arpStep=0;kickEnv=snareEnv=hatEnv=openHatEnv=kickSweep=snareNoiseLP=hatNoiseLP=0;lastGroove=-1;lastPlay=false;lastPadCC=-1;
 juce::zeromem(delayL,sizeof(delayL));juce::zeromem(delayR,sizeof(delayR));delayIndex=0;scopeDivider=scopeWrite=0;visualWriteHead.store(0);for(auto& s:visualTap)s.store(0.f);}
bool MeowcillatorProcessor::isBusesLayoutSupported(const BusesLayout& l) const{return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
float MeowcillatorProcessor::noise(){randState^=randState<<13;randState^=randState>>17;randState^=randState<<5;return float(randState&65535)/32768.f-1.f;}
int MeowcillatorProcessor::noteFromPad(float x,int key,int octave,int scale) const {
 static const std::array<std::vector<int>,5> scales={std::vector<int>{0,3,5,7,10},std::vector<int>{0,2,4,5,7,9,11},std::vector<int>{0,2,3,5,7,8,10},std::vector<int>{0,3,5,6,7,10},std::vector<int>{0,1,2,3,4,5,6,7,8,9,10,11}};
 const auto& a=scales[size_t(juce::jlimit(0,4,scale))];int n=juce::jlimit(0,int(a.size()*3)-1,int(x*float(a.size()*3)));
 return juce::jlimit(20,108,48+key+12*(n/int(a.size())-1+octave)+a[size_t(n%int(a.size()))]);
}
void MeowcillatorProcessor::noteOn(int note,float vel,float y,int source,float glide){
 Voice* slot=&voices[0];for(auto& v:voices)if(!v.active){slot=&v;break;}
 *slot={};slot->active=true;slot->source=source;slot->note=note;slot->vel=vel;slot->y=y;slot->targetHz=pitchHz(note);
 // First touch and MIDI note-ons start at pitch immediately. Only an
 // intentional pad drag with GLIDE turned up interpolates from the last note.
 const bool shouldSlide = (source == 0 && lastPadDown && glide > .015f);
 slot->currentHz=shouldSlide?prevHz:slot->targetHz;prevHz=slot->targetHz;
}
void MeowcillatorProcessor::noteOff(int note,int source){for(auto& v:voices)if(v.active&&v.note==note&&(source<0||v.source==source))v.release=true;}
float MeowcillatorProcessor::renderVoice(Voice& v,int patch,float cutoff,float resonance,float glide){
 int family=patch/6,variant=patch%6;bool accordion=patch==36;
 int shift=(family==1||family==5)?-12:(family==4?12:0);if(patch==8)shift=12;if(accordion)shift=0;
 v.targetHz=pitchHz(v.note+shift);
 float k=(glide<=.002f?1.f:1.f-std::exp(-1.f/(float(sr)*(.002f+glide*.48f))));
 v.currentHz+=(v.targetHz-v.currentHz)*k;
 v.phase+=v.currentHz/float(sr);v.phase-=std::floor(v.phase);
 float subFrequency=(family==4?.5f:accordion?2.f:1.f);
 v.subphase+=v.currentHz*subFrequency/float(sr);v.subphase-=std::floor(v.subphase);
 float saw=2.f*v.phase-1.f,sq=v.phase<.5f?1.f:-1.f,tri=1.f-4.f*std::abs(v.phase-.5f),sine=std::sin(twoPi*v.phase);
 float sub=std::sin(twoPi*v.subphase),a=accordion?(.52f*saw+.22f*sq+.26f*sub):family==0?saw:family==1?(variant%2?tri:sine):family==2?(variant<2?sq:tri):family==3?(variant%2?saw:sq):family==4?(.8f*sine+.2f*sub):variant%2?sq:tri;
 if(family==0||family==3)a=std::tanh(a*(family==0?3.4f:2.8f));
 if(family==5)a=std::floor(a*(4.f+variant))/ (4.f+variant);
 if(v.release) v.env*=.9991f;else v.env+=(1.f-v.env)*(family==1?.0013f:.005f);
 if(v.release&&v.env<.0003f){v.active=false;return 0.f;}
 float freq=juce::jlimit(80.f,float(sr*.4),cutoff*(.3f+.7f*v.y));float alpha=1.f-std::exp(-twoPi*freq/float(sr));v.filter+=(a-v.filter)*alpha;
 float air=(resonance*.18f)*a;float trim=accordion?.80f:family==1?.92f:family==3?.8f:.88f;
 return (v.filter+air)*v.env*v.vel*.17f*trim;
}
void MeowcillatorProcessor::fireDrums(int groove,int kit,juce::MidiBuffer& midi,int sample,bool send){
    const auto& pat = meowRhythms[static_cast<std::size_t>(juce::jlimit(0,10,groove))];
    const unsigned s = static_cast<unsigned>(stepIndex) & 31u;
    const auto hit = [s](std::uint32_t mask) { return ((mask >> s) & 1u) != 0u; };
    const bool k=hit(pat.kick), sn=hit(pat.snare), h=hit(pat.hat), oh=hit(pat.openHat);
    const float accent=hit(pat.accent)?1.17f:0.82f;
    if(k){kickEnv=accent; kickSweep=1.0f;}
    if(sn)snareEnv=accent;
    if(h)hatEnv=accent*(s%4==0?.80f:.55f);
    if(oh)openHatEnv=accent*.70f;
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
// Cat sounds are internally synthesized and independent of the main synth voice.
void MeowcillatorProcessor::triggerCatVoice(int i){
 if(i<0||i>5)return;
 CatVoice* slot=&catVoices[0];
 for(auto& c:catVoices)if(!c.active){slot=&c;break;}
 *slot={};slot->active=true;slot->kind=i;lastCat=i;
}
float MeowcillatorProcessor::renderCatVoices(){
 float sum=0;const float fs=float(sr);
 for(auto& c:catVoices){
  if(!c.active)continue;
  const float t=c.age;
  const float duration[6]={.76f,1.55f,1.13f,.84f,.32f,.92f};
  if(t>=duration[c.kind]){c.active=false;continue;}
  const float u=t/duration[c.kind],env=std::pow(juce::jmax(0.f,1.f-u),1.35f)*juce::jmin(1.f,t*80.f);
  float hz=230.f,signal=0.f;
  switch(c.kind){
   case 0: hz=590.f-300.f*u+75.f*std::sin(twoPi*u*2.3f);break;          // MEOW
   case 1: hz=64.f+3.f*std::sin(twoPi*t*23.f);break;                    // PURR
   case 2: hz=760.f-460.f*u+90.f*std::sin(twoPi*t*4.f);break;          // YOWL
   case 3: hz=950.f;break;                                              // HISS
   case 4: hz=1200.f-470.f*u+140.f*std::sin(twoPi*u*3.f);break;        // CHIRP
   case 5: hz=100.f-44.f*u+9.f*std::sin(twoPi*t*12.f);break;            // GROWL
  }
  c.phase+=hz/fs;c.phase-=std::floor(c.phase);
  const float sine=std::sin(twoPi*c.phase);
  const float saw=2.f*c.phase-1.f;
  const float n=noise();
  if(c.kind==0)signal=std::tanh(2.8f*(.72f*sine+.28f*saw))*(.75f+.25f*std::sin(twoPi*t*6.f));
  else if(c.kind==1)signal=(.64f*sine+.36f*n)*(.60f+.4f*std::sin(twoPi*t*25.f));
  else if(c.kind==2)signal=std::tanh((.65f*saw+.35f*sine)*4.f);
  else if(c.kind==3){c.filter+=(n-c.filter)*.16f;signal=n-c.filter;}
  else if(c.kind==4)signal=.65f*sine+.35f*saw;
  else signal=std::tanh((saw*.72f+n*.28f)*4.5f);
  sum+=signal*env*(c.kind==3?.13f:c.kind==1?.17f:.23f);
  c.age+=1.f/fs;
 }
 return juce::jlimit(-.8f,.8f,sum);
}
void MeowcillatorProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;buffer.clear();const int frames=buffer.getNumSamples();
 // Map hardware CC/pitch wheel before DSP reads parameters for this block.
 // This path is allocation-free and does not block on the WebView renderer.
 for(const auto metadata:midi)handleMidiMapping(metadata.getMessage());
 auto v=[&](const char* id){return parameterForAudio(id);};
 int patch=program.load(),groove=juce::jlimit(0,10,int(v("groove"))),kit=int(v("kit")),key=int(v("key")),oct=int(v("octave"))-2,scale=int(v("scale"));
 bool drumsOn=v("drumsOn")>.5f,arp=v("arp")>.5f,hold=v("hold")>.5f;
 bool sendDrums=v("midiDrums")>.5f,sendSynth=v("midiSynth")>.5f,sendCats=v("midiCats")>.5f;
 double bpm=v("bpm");float glide=v("glide"),oscVol=v("osc"),drumVol=v("drums"),cutoff=v("cutoff"),res=v("resonance"),gate=v("gate");
 const bool playing=play.load(std::memory_order_relaxed);
 if(groove!=lastGroove){lastGroove=groove;stepIndex=0;drumPhase=playing?1.0:0.0;}
 if(playing&&!lastPlay){stepIndex=0;drumPhase=1.0;}
 lastPlay=playing;
 int rate=int(v("rate")),arpType=int(v("arpType")),fx=int(v("effect"));
 const float drive=v("drive"),fxmix=v("fxmix"),feedback=v("feedback");
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
 int catTrig=triggerCat.exchange(-1);
 if(catTrig>=0&&catTrig<6){triggerCatVoice(catTrig);if(sendCats){output.addEvent(juce::MidiMessage::noteOn(16,60+catTrig,(juce::uint8)112),0);output.addEvent(juce::MidiMessage::noteOff(16,60+catTrig),juce::jmin(1,frames-1));}}
 const float fs=float(sr);
 // Native drum synthesis. Distinct tone envelopes across the four kits.
 const float kickDecay = std::exp(-1.f/(fs*(kit==1?.14f:kit==2?.20f:kit==3?.31f:.21f)));
 const float snareDecay = std::exp(-1.f/(fs*(kit==3?.22f:kit==1?.10f:.14f)));
 const float hatDecay = std::exp(-1.f/(fs*(kit==2?.047f:kit==3?.065f:.035f)));
 const float openHatDecay = std::exp(-1.f/(fs*(kit==3?.47f:kit==1?.22f:.31f)));
 const float sweepDecay = std::exp(-1.f/(fs*(kit==2?.039f:.055f)));
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
    endDirectPad(i);endArp(i);arpHadSource=false;
    if(sendSynth)output.addEvent(msg,i);
    ++it;continue;
   }
   if(ch==16&&msg.isNoteOn()&&msg.getNoteNumber()>=60&&msg.getNoteNumber()<66){triggerCatVoice(msg.getNoteNumber()-60);}
   else if(ch==10&&msg.isNoteOn()){if(msg.getNoteNumber()==36)kickEnv=1;else if(msg.getNoteNumber()==38)snareEnv=1;else if(msg.getNoteNumber()==42)hatEnv=.6f;else if(msg.getNoteNumber()==46)openHatEnv=.7f;}
   else if(msg.isController() && ch==1 && msg.getControllerNumber()==74){
    midiExpressionY=juce::jlimit(0.f,1.f,float(msg.getControllerValue())/127.f);
    for(auto& voice:voices)if(voice.active&&voice.source==1)voice.y=midiExpressionY;
    if(sendSynth)output.addEvent(msg,i);
   }
   else if(msg.isNoteOn()){
    const int n=msg.getNoteNumber();physicalMidiNotes[size_t(n)]=true;
    if(!midiNotes[size_t(n)]){midiNotes[size_t(n)]=true;++heldMidiCount;}
    if(!arp)startInput(n,msg.getFloatVelocity(),i);
   }
   else if(msg.isNoteOff()){
    const int n=msg.getNoteNumber();physicalMidiNotes[size_t(n)]=false;
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
  // Different drum kits have independent pitch, saturation and decay characteristics.
  const float kickBase=kit==1?48.f:kit==2?60.f:kit==3?54.f:49.f;
  const float kickRange=kit==2?155.f:kit==3?75.f:kit==1?94.f:120.f;
  kickPhase+=(kickBase+kickRange*kickSweep)/fs;
  if(kickPhase>=1.f)kickPhase-=1.f;
  const float kraw=std::sin(twoPi*kickPhase)*(kit==2?.57f:kit==3?.45f:.50f);
  const float click=noise()*kickSweep*(kit==2?.10f:kit==1?.045f:.07f);
  float k=juce::jlimit(-.65f,.65f,kraw*kickEnv+click*kickEnv);
  kickEnv*=kickDecay;kickSweep*=sweepDecay;
  // Separate snare/hat noise colors and real open-hat envelope.
  const float n=noise();
  snareNoiseLP+=(n-snareNoiseLP)*(kit==3?.045f:kit==1?.18f:.12f);
  const float snareTone=(kit==3?snareNoiseLP*1.2f:n-snareNoiseLP*.5f);
  float s=snareTone*snareEnv*(kit==2?.36f:kit==1?.22f:.30f);
  snareEnv*=snareDecay;
  hatNoiseLP+=(n-hatNoiseLP)*(kit==3?.27f:kit==1?.65f:.46f);
  const float brightNoise=n-hatNoiseLP;
  float h=brightNoise*(hatEnv*(kit==2?.095f:kit==1?.055f:.075f)
                       +openHatEnv*(kit==3?.12f:.10f));
  hatEnv*=hatDecay;openHatEnv*=openHatDecay;
  const float cats=renderCatVoices();
  float v0=synth+drumVol*(k+s+h);v0=std::tanh(v0*(1.f+drive*2.f));
  const int length=juce::jlimit(1,95999,int(sr*(fx==4?.09:.29)));
  const int delayed=(delayIndex+96000-length)%96000;
  float left=delayL[delayed],right=delayR[delayed];delayL[delayIndex]=v0+feedback*right;delayR[delayIndex]=v0+feedback*left;delayIndex=(delayIndex+1)%96000;
  float wet=(fx==0?0.f:fxmix*.5f*(left+right));float outL=std::tanh(v0+wet+cats),outR=std::tanh(v0+wet+cats);
  outL=juce::jlimit(-.98f,.98f,outL);outR=juce::jlimit(-.98f,.98f,outR);
  buffer.setSample(0,i,outL);buffer.setSample(1,i,outR);
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
 editorWidth.store(juce::jlimit(700,1700,(int)tree.getProperty("ui_width",1000)));
 editorHeight.store(juce::jlimit(540,1450,(int)tree.getProperty("ui_height",780)));
 apvts.replaceState(tree);
 for(int t=0;t<MidiTargetCount;++t){const int code=(int)tree.getProperty(juce::Identifier("midi_"+juce::String(midiIds[t])),0);
   midiMappings[t].store(code>=0&&code<=16384?code:0);
   if(t<MidiParameterCount){midiRealtimeEnabled[t].store(false);midiUpdatePending[t].store(false);}}
 learnTarget.store(-1);midiMapRevision.fetch_add(1);
}
juce::AudioProcessorEditor* MeowcillatorProcessor::createEditor(){return new MeowcillatorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MeowcillatorProcessor();}
