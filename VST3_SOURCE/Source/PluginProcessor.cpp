#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

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
static const std::uint16_t kickMask[11]={0x9129,0x1111,0x2549,0x9595,0x5555,0xF3DF,0x5555,0x0901,0xA549,0xA929,0xD555};
static const std::uint16_t snareMask[11]={0x1010,0x1010,0x1090,0x1010,0x5150,0x5555,0x5450,0x1010,0x1090,0x1490,0x5450};
static const std::uint16_t hatMask[11]={0x5555,0x2222,0x7777,0xFFFF,0x5555,0xFFDF,0x5555,0x2222,0x7777,0xD7D7,0xFFFF};
float pitchHz(int n){return 440.f*std::pow(2.f,(float(n)-69.f)/12.f);}
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
 f("glide","GLIDE",0,1,.12f); f("cutoff","CUTOFF",150,10000,6000,1.f);f("resonance","RESONANCE",0,1,.35f);
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
 :AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),apvts(*this,nullptr,"MEOWCILLATOR_STATE",makeParameters()){}
void MeowcillatorProcessor::setCurrentProgram(int i){program=juce::jlimit(0,36,i);}
const juce::String MeowcillatorProcessor::getProgramName(int i){return presetLabels[juce::jlimit(0,36,i)];}
void MeowcillatorProcessor::prepareToPlay(double sampleRate,int){sr=sampleRate;for(auto& v:voices)v={};for(auto& s:catVoices)s={};drumPhase=arpPhase=0;stepIndex=0;lastPadDown=false;lastPadNote=lastArpNote=-1;kickEnv=snareEnv=hatEnv=0;juce::zeromem(delayL,sizeof(delayL));juce::zeromem(delayR,sizeof(delayR));delayIndex=0;}
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
 slot->currentHz=glide>.01f?prevHz:slot->targetHz;prevHz=slot->targetHz;
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
 const int s=stepIndex%16;bool k=(kickMask[groove]>>s)&1u, sn=(snareMask[groove]>>s)&1u,h=(hatMask[groove]>>s)&1u;
 if(k)kickEnv=1.f;if(sn)snareEnv=1.f;if(h)hatEnv=.6f;
 // Independent drum MIDI on channel 10. The synth is never routed to this channel.
 auto add=[&](int note,int vel){if(send){midi.addEvent(juce::MidiMessage::noteOn(10,note,(juce::uint8)vel),sample);for(auto& off:pendingOffs)if(!off.active){off={10,note,juce::jmax(1,int(sr*.05)),true};break;}}};
 if(k)add(36,107);if(sn)add(38,102);if(h)add(42,74);
 juce::ignoreUnused(kit);
 ++stepIndex;currentStep=stepIndex%16;
}
void MeowcillatorProcessor::fireArp(juce::MidiBuffer& midi,int sample,int mode,float glide,bool send){
 std::array<int,132> chord{};int count=0;
 if(lastPadNote>=0){for(int add:{0,3,7,12})chord[size_t(count++)]=juce::jmin(127,lastPadNote+add);}
 else for(int n=0;n<128;n++)if(midiNotes[size_t(n)])chord[size_t(count++)]=n;
 if(!count)return;
 if(lastArpNote>=0){noteOff(lastArpNote,2);if(send)midi.addEvent(juce::MidiMessage::noteOff(1,lastArpNote),sample);}
 int idx=arpStep++%count;
 if(mode==1)idx=count-1-idx;else if(mode==2)idx=int((randState>>5)%uint32_t(count));
 lastArpNote=chord[size_t(idx)];noteOn(lastArpNote,.84f,padY.load(),2,glide);if(send)midi.addEvent(juce::MidiMessage::noteOn(1,lastArpNote,(juce::uint8)108),sample);
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
 auto v=[&](const char* id){return apvts.getRawParameterValue(id)->load();};
 int patch=program.load(),groove=juce::jlimit(0,10,int(v("groove"))),kit=int(v("kit")),key=int(v("key")),oct=int(v("octave"))-2,scale=int(v("scale"));
 bool drumsOn=v("drumsOn")>.5f,arp=v("arp")>.5f,hold=v("hold")>.5f;
 bool sendDrums=v("midiDrums")>.5f,sendSynth=v("midiSynth")>.5f,sendCats=v("midiCats")>.5f;
 double bpm=v("bpm");float glide=v("glide"),oscVol=v("osc"),drumVol=v("drums"),cutoff=v("cutoff"),res=v("resonance"),gate=v("gate");
 int rate=int(v("rate")),arpType=int(v("arpType")),fx=int(v("effect"));
 const float drive=v("drive"),fxmix=v("fxmix"),feedback=v("feedback");
 juce::MidiBuffer input=midi;juce::MidiBuffer output;auto it=input.begin(),end=input.end();
 auto addSynth=[&](const juce::MidiMessage& m,int index){if(sendSynth)output.addEvent(m,index);};
 if(lastHoldMode&&!hold){midiNotes.fill(false);if(!padDown.load())lastPadNote=-1;}lastHoldMode=hold;
 int catTrig=triggerCat.exchange(-1);
 if(catTrig>=0&&catTrig<6){triggerCatVoice(catTrig);if(sendCats){output.addEvent(juce::MidiMessage::noteOn(16,60+catTrig,(juce::uint8)112),0);output.addEvent(juce::MidiMessage::noteOff(16,60+catTrig),juce::jmin(1,frames-1));}}
 float maxRms=0;
 for(int i=0;i<frames;i++){
  for(auto& off:pendingOffs)if(off.active&&--off.samples<=0){output.addEvent(juce::MidiMessage::noteOff(off.channel,off.note),i);off.active=false;}
  while(it!=end&&(*it).samplePosition<=i){auto msg=(*it).getMessage();const int ch=msg.getChannel();
   if(ch==16&&msg.isNoteOn()&&msg.getNoteNumber()>=60&&msg.getNoteNumber()<66){triggerCatVoice(msg.getNoteNumber()-60);}
   else if(ch==10&&msg.isNoteOn()){if(msg.getNoteNumber()==36)kickEnv=1;else if(msg.getNoteNumber()==38)snareEnv=1;else if(msg.getNoteNumber()==42)hatEnv=.6f;}
   else if(msg.isNoteOn()){midiNotes[size_t(msg.getNoteNumber())]=true;if(!arp)noteOn(msg.getNoteNumber(),msg.getFloatVelocity(),.52f,1,glide);}
   else if(msg.isNoteOff()){if(!hold)midiNotes[size_t(msg.getNoteNumber())]=false;if(!arp&&!hold)noteOff(msg.getNoteNumber(),1);}
   ++it;
  }
  const bool down=padDown.load();const int padNote=noteFromPad(padX.load(),key,oct,scale);
  if(down&&(!lastPadDown||padNote!=lastPadNote)){
    if(lastPadDown&&lastPadNote>=0){if(!arp)noteOff(lastPadNote,0);addSynth(juce::MidiMessage::noteOff(1,lastPadNote),i);}
    lastPadNote=padNote;if(!arp)noteOn(padNote,.91f,padY.load(),0,glide);
    addSynth(juce::MidiMessage::noteOn(1,padNote,(juce::uint8)110),i);
  }
  if(!down&&lastPadDown&&!hold){if(!arp)noteOff(lastPadNote,0);if(lastPadNote>=0)addSynth(juce::MidiMessage::noteOff(1,lastPadNote),i);lastPadNote=-1;if(lastArpNote>=0){noteOff(lastArpNote,2);if(sendSynth)output.addEvent(juce::MidiMessage::noteOff(1,lastArpNote),i);lastArpNote=-1;}}
  lastPadDown=down;
  if(!arp&&lastArpNote>=0){noteOff(lastArpNote,2);if(sendSynth)output.addEvent(juce::MidiMessage::noteOff(1,lastArpNote),i);lastArpNote=-1;}
  if(arp){arpPhase+=bpm*double(1<<rate)/(60.*sr);
   if(arpPhase>=1.){arpPhase-=1.;fireArp(output,i,arpType,glide,sendSynth);arpGateSamples=juce::jmax(1,int(sr*60./bpm/double(1<<rate)*gate));}
  }
  if(arp&&arpGateSamples>0&&--arpGateSamples==0&&lastArpNote>=0){noteOff(lastArpNote,2);if(sendSynth)output.addEvent(juce::MidiMessage::noteOff(1,lastArpNote),i);lastArpNote=-1;}
  if(play.load()){drumPhase+=bpm*4./(60.*sr);if(drumPhase>=1.){drumPhase-=1.;if(drumsOn)fireDrums(groove,kit,output,i,sendDrums);}}
  float synth=0;for(auto& voice:voices)if(voice.active)synth+=renderVoice(voice,patch,cutoff,res,glide);
  synth*=oscVol;
  kickPhase+=float((kit==1?90.:kit==2?72.:114.)/sr);if(kickPhase>=1)kickPhase-=1;
  float k=std::sin(twoPi*kickPhase)*kickEnv*(kit==2?.55f:.39f);kickEnv*=.99923f;
  float s=noise()*snareEnv*(kit==3?.13f:.20f);snareEnv*=.9955f;
  float h=noise()*hatEnv*(kit==1?.025f:.05f);hatEnv*=.972f;
  const float cats=renderCatVoices();
  float v0=synth+drumVol*(k+s+h);v0=std::tanh(v0*(1.f+drive*2.f));
  const int length=juce::jlimit(1,95999,int(sr*(fx==4?.09:.29)));
  const int delayed=(delayIndex+96000-length)%96000;
  float left=delayL[delayed],right=delayR[delayed];delayL[delayIndex]=v0+feedback*right;delayR[delayIndex]=v0+feedback*left;delayIndex=(delayIndex+1)%96000;
  float wet=(fx==0?0.f:fxmix*.5f*(left+right));float outL=std::tanh(v0+wet+cats),outR=std::tanh(v0+wet+cats);
  outL=juce::jlimit(-.98f,.98f,outL);outR=juce::jlimit(-.98f,.98f,outR);
  buffer.setSample(0,i,outL);buffer.setSample(1,i,outR);maxRms=juce::jmax(maxRms,juce::jmax(std::abs(outL),std::abs(outR)));
 }
 audioLevel=audioLevel.load()*.74f+maxRms*.26f;
 midi.swapWith(output);

}
void MeowcillatorProcessor::getStateInformation(juce::MemoryBlock& b){
 auto tree=apvts.copyState();tree.setProperty("program",program.load(),nullptr);
 if(auto xml=tree.createXml())copyXmlToBinary(*xml,b);
}
void MeowcillatorProcessor::setStateInformation(const void* d,int n){
 auto xml=getXmlFromBinary(d,n);if(!xml)return;auto tree=juce::ValueTree::fromXml(*xml);
 if(!tree.isValid())return;program=juce::jlimit(0,36,int(tree.getProperty("program",0)));
 apvts.replaceState(tree);
}
juce::AudioProcessorEditor* MeowcillatorProcessor::createEditor(){return new MeowcillatorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MeowcillatorProcessor();}
