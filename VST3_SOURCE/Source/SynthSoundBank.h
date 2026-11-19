#pragma once
// MEOWCILLATOR 0.9.14. Native, allocation-free, 37 genuinely differentiated
// patch voices. No samples, no WebAudio, no UI calls in the audio thread.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace meow {
constexpr float pi2 = 6.2831853071795864769f;
enum class Wave : uint8_t { Sin, Saw, Pulse, Tri, Fold, Glass, Reed, Chip, Formant, Noise };
struct Patch {
 Wave a,b; float blend, sub, noise, fm, detune, width;
 float attack, release, cutoff, drive, vibratoHz, vibratoCents, pitchSweep;
 float tremoloHz, tremoloDepth; int shift;
};
// Patch order is exactly presetLabels[37] in PluginProcessor.cpp. Every patch
// has its own waveform pair, envelope, spectrum, modulation and/or pitch motion.
inline constexpr std::array<Patch,37> patches{{
//   A                    B                    blend sub noise fm detune width  attack release cutoff drive vibratoHz vibCt sweep tremHz tremD shift
 {Wave::Formant,Wave::Saw,  .30f,.18f,.02f,2.4f,  4.f,.52f, .020f,.39f,.40f,.32f,5.3f,19.f,  9.f,5.3f,.12f, 0}, // 01 ARISTO MEOW
 {Wave::Noise, Wave::Pulse,.43f,.08f,.75f,0.0f,  0.f,.24f, .001f,.10f,.63f,.88f,0.f,0.f, -5.f,0.f,0.f,-12}, // 02 HARDCORE BARK
 {Wave::Pulse, Wave::Saw,  .38f,.42f,.02f,.15f, 13.f,.43f, .004f,.19f,.77f,.77f,5.8f,8.f,0.f,0.f,0.f,0}, // 03 CAT FUZZ LEAD
 {Wave::Saw,   Wave::Reed, .32f,.31f,.01f,.4f,  7.f,.49f, .002f,.18f,.85f,.52f,0.f,0.f,-1.f,0.f,0.f,0}, // 04 BITTEN GUITAR
 {Wave::Glass, Wave::Fold, .43f,.11f,.04f,4.8f,11.f,.46f, .012f,.95f,1.35f,.49f,7.9f,31.f,4.f,0.f,0.f,12}, // 05 FEEDBACK QUEEN
 {Wave::Noise, Wave::Chip, .62f,.08f,.65f,2.2f,  5.f,.31f, .001f,.13f,.91f,.81f,0.f,0.f,-8.f,0.f,0.f,0}, // 06 NOISE RIFF
 {Wave::Formant,Wave::Sin, .62f,.31f,.11f,1.1f,  8.f,.50f, .34f,1.38f,.38f,.06f,4.4f,11.f,0.f,0.f,0.f,0}, // 07 COLD CAVE CHOIR
 {Wave::Saw,   Wave::Tri, .42f,.31f,.03f,.2f,  9.f,.49f, .27f,1.67f,.28f,.17f,.26f,14.f,0.f,0.f,0.f,0}, // 08 VAMPIRE PAD
 {Wave::Sin,   Wave::Glass,.60f,.22f,.23f,1.2f,17.f,.51f, .45f,2.14f,.20f,.03f,.13f,8.f,0.f,0.f,0.f,12}, // 09 MIDNIGHT MIST
 {Wave::Reed,  Wave::Sin, .41f,.53f,.01f,.6f,  5.f,.52f, .026f,.70f,.68f,.13f,5.f,3.f,0.f,4.9f,.17f,0}, // 10 SAD CAT ORGAN
 {Wave::Glass, Wave::Sin, .25f,.06f,.01f,6.9f,  0.f,.50f, .001f,1.12f,1.55f,.11f,0.f,0.f,0.f,0.f,0.f,12}, // 11 NEON GHOST
 {Wave::Sin,   Wave::Tri, .28f,.45f,.08f,.08f, 3.f,.50f, .56f,2.50f,.15f,.02f,.09f,23.f,0.f,0.f,0.f,-12},// 12 SLOW MOTION
 {Wave::Pulse, Wave::Saw, .36f,.72f,.0f,.1f,  4.f,.34f, .003f,.20f,.48f,.30f,0.f,0.f,1.5f,0.f,0.f,-12},// 13 POST-PUNK BASS
 {Wave::Tri,   Wave::Sin, .43f,.82f,.0f,.1f,  0.f,.52f, .005f,.29f,.16f,.08f,.44f,4.f,0.f,0.f,0.f,-12},// 14 COLD BASS
 {Wave::Glass, Wave::Reed,.38f,.24f,.01f,3.2f,  9.f,.50f, .003f,1.14f,1.30f,.05f,3.7f,6.f,0.f,0.f,0.f,12},// 15 CHORUS CHIME
 {Wave::Sin,   Wave::Glass,.65f,.09f,.0f,8.2f,  5.f,.50f, .001f,.74f,1.80f,.09f,0.f,0.f,10.f,0.f,0.f,12},// 16 CHIME RIFF
 {Wave::Saw,   Wave::Pulse,.50f,.51f,.01f,0.f, 3.f,.28f, .002f,.16f,.78f,.40f,0.f,0.f,0.f,0.f,0.f,-12},// 17 DEAD DISCO
 {Wave::Reed,  Wave::Tri, .41f,.30f,.16f,.1f, 11.f,.49f, .21f,1.25f,.46f,.12f,.55f,17.f,0.f,0.f,0.f,0}, //18 DUSTY STRINGS
 {Wave::Chip,  Wave::Fold,.36f,.10f,.19f,5.7f, 23.f,.17f, .001f,.18f,1.10f,.88f,0.f,0.f,-7.f,0.f,0.f,0}, // 19 CYBER HATE
 {Wave::Pulse, Wave::Tri, .26f,.64f,.01f,1.5f,  5.f,.23f, .001f,.22f,.52f,.68f,8.f,7.f,2.f,0.f,0.f,-12}, // 20 EBM MACHINE
 {Wave::Formant,Wave::Fold,.61f,.16f,.08f,3.7f, 12.f,.42f, .009f,.28f,.73f,.72f,6.8f,12.f,-1.5f,0.f,0.f,-12},//21 METAL THROAT
 {Wave::Glass, Wave::Pulse,.44f,.12f,.01f,5.6f,14.f,.45f, .012f,.45f,1.38f,.54f,8.5f,35.f,12.f,0.f,0.f,12},//22 RUSTY ALARM
 {Wave::Sin,   Wave::Saw, .34f,.10f,.02f,1.1f,  7.f,.50f, .058f,1.28f,.95f,.32f,2.1f,260.f,0.f,2.1f,.33f,0},//23 FACTORY SIREN
 {Wave::Chip,  Wave::Pulse,.49f,.12f,.0f,0.f, 16.f,.12f, .001f,.13f,1.30f,.15f,8.f,0.f,-6.f,0.f,0.f,12},//24 MECHANICAL NYAN
 {Wave::Sin,   Wave::Tri, .15f,.87f,.0f,.05f,  0.f,.50f, .003f,.38f,.11f,.15f,0.f,0.f,4.f,0.f,0.f,-12},//25 PURR 808
 {Wave::Tri,   Wave::Noise,.12f,.35f,.20f,0.f, 5.f,.50f, .015f,.45f,.30f,.13f,4.7f,21.f,0.f,0.f,0.f,0}, //26 VINYL CAT
 {Wave::Sin,   Wave::Pulse,.15f,.93f,.0f,0.f, 0.f,.48f, .001f,.29f,.18f,.31f,0.f,0.f,5.f,0.f,0.f,-24},//27 BOOM BASS
 {Wave::Tri,   Wave::Glass,.34f,.33f,.06f,1.2f, 14.f,.52f, .019f,.46f,.27f,.17f,1.4f,23.f,-.5f,0.f,0.f,0}, //28 TAPE KEYS
 {Wave::Reed,  Wave::Glass,.54f,.22f,.13f,2.6f,10.f,.49f, .034f,.88f,.34f,.23f,.71f,18.f,0.f,0.f,0.f,0}, //29 LO-FI CHORD
 {Wave::Glass, Wave::Fold,.64f,.08f,.01f,9.5f,  8.f,.5f, .001f,.99f,.88f,.45f,0.f,0.f,9.f,0.f,0.f,12}, //30 MEMPHIS BELL
 {Wave::Chip,  Wave::Pulse,.32f,.05f,.0f,0.f,  0.f,.19f, .001f,.19f,1.75f,.08f,0.f,0.f,3.f,0.f,0.f,12}, //31 8-BIT NYAN
 {Wave::Fold,  Wave::Chip, .53f,.07f,.16f,7.4f, 26.f,.36f, .001f,.24f,.97f,.90f,13.f,50.f,-4.f,0.f,0.f,0},//32 CIRCUIT BENT
 {Wave::Glass, Wave::Pulse,.45f,.14f,.03f,4.8f,  0.f,.40f, .001f,.33f,1.22f,.62f,7.8f,28.f,22.f,0.f,0.f,12},//33 LASER MEOW
 {Wave::Formant,Wave::Noise,.45f,.16f,.38f,3.0f,  3.f,.45f, .09f,.88f,.36f,.24f,2.3f,15.f,-2.f,0.f,0.f,0}, //34 RADIO GHOST
 {Wave::Noise, Wave::Chip, .62f,.08f,.86f,9.0f,  9.f,.13f, .001f,.10f,1.60f,.59f,0.f,0.f,-12.f,0.f,0.f,12},//35 GLITCH NOISE
 {Wave::Chip,  Wave::Fold, .40f,.18f,.07f,3.0f, 22.f,.17f, .002f,.24f,.82f,.62f,6.5f,34.f,7.f,0.f,0.f,0}, //36 BORKED CONSOLE
 {Wave::Reed,  Wave::Saw,  .54f,.53f,.00f,.1f,  7.f,.50f, .048f,.56f,.73f,.07f,5.6f,5.f,0.f,5.6f,.16f,0}, //37 ACCORDION CAT
}};
struct SynthVoice {
 bool active=false, release=false; int note=60,source=0;
 float phase=0.f,subphase=0.f,phaseB=0.f,phaseM=0.f,env=0.f,vel=0.f,y=.5f,
  filter=0.f, filter2=0.f, prevInput=0.f, currentHz=440.f,targetHz=440.f,
  age=0.f, pitchEnv=1.f, noiseHeld=0.f; int decim=0,patchIndex=-1; uint32_t rng=0x4512u;
};
inline float random01(SynthVoice& v){v.rng^=v.rng<<13;v.rng^=v.rng>>17;v.rng^=v.rng<<5;return float(v.rng&65535u)/32768.f-1.f;}
inline float waveform(Wave k,float phase,float modPhase,float width,float fm,float n){
 const float s=std::sin(pi2*phase);
 switch(k){
 case Wave::Sin:return s;
 case Wave::Saw:return 2.f*phase-1.f;
 case Wave::Pulse:return phase<width?1.f:-1.f;
 case Wave::Tri:return 1.f-4.f*std::abs(phase-.5f);
 case Wave::Fold:return std::sin(pi2*phase+std::sin(pi2*modPhase)*fm)*.8f+.20f*std::sin(pi2*phase*3.f);
 case Wave::Glass:return std::sin(pi2*phase+std::sin(pi2*modPhase)*fm);
 case Wave::Reed:return .58f*(2.f*phase-1.f)+.27f*s+.15f*std::sin(pi2*phase*3.f);
 case Wave::Chip:return std::round((phase<width?1.f:-1.f)*3.f)/3.f + .15f*std::round(s*4.f)/4.f;
 case Wave::Formant:return .44f*s+.36f*std::sin(2.f*pi2*phase)+.20f*std::sin(3.f*pi2*phase);
 case Wave::Noise:return n;
 }
 return 0.f;
}
inline constexpr std::array<float,37> patchGain{{
 1.7f,2.1f,.49f,.63f,.91f,1.2f,2.4f,1.17f,1.65f,1.17f,
 1.02f,1.48f,.61f,.85f,1.30f,.90f,.76f,1.42f,.52f,.52f,
 1.04f,.85f,.72f,.67f,.83f,.78f,.64f,.83f,.83f,1.02f,
 .61f,.70f,.90f,2.50f,2.10f,.53f,1.25f
}};
inline float renderSynth(SynthVoice& v,int patch,float cutoff,float res,float glide,float sr){
 const int index=std::clamp(patch,0,36);
 const auto& p=patches[static_cast<std::size_t>(index)];
 const float fs=std::max(8000.f,sr);
 if(v.patchIndex!=index){
  v.targetHz=440.f*std::exp2((float(v.note+p.shift)-69.f)/12.f);
  v.patchIndex=index;
 }
 const float target=v.targetHz;
 const float slide=glide<=.002f?1.f:std::min(1.f,1.f/(fs*(.002f+glide*.48f)));
 v.currentHz+=(target-v.currentHz)*slide;
 const float vib=std::sin(pi2*p.vibratoHz*v.age)*(p.vibratoCents/1200.f);
 const float semis=p.pitchSweep*v.pitchEnv/12.f;
 const float hz=std::max(15.f,v.currentHz*std::exp2(semis+vib));
 const float t=1.f/fs;
 v.age+=t;
 v.pitchEnv*=1.f-std::min(1.f,10.f*t);
 v.phase+=hz*t;v.phase-=std::floor(v.phase);
 const float detuned=hz*(1.f+p.detune*.0005778f);
 v.phaseB+=detuned*t;v.phaseB-=std::floor(v.phaseB);
 v.phaseM+=hz*t*(1.37f+p.fm*.12f);v.phaseM-=std::floor(v.phaseM);
 v.subphase+=hz*.5f*t;v.subphase-=std::floor(v.subphase);
 const float n=random01(v);
 float a=waveform(p.a,v.phase,v.phaseM,p.width,p.fm,n);
 float b=waveform(p.b,v.phaseB,v.phaseM,.5f,p.fm*.71f,n);
 const float sub=std::sin(pi2*v.subphase);
 float raw=(1.f-p.blend)*a+p.blend*b+p.sub*.34f*sub+p.noise*.32f*n;
 if(index==0){ // meow-like moving formants in addition to the pitch contour
  const float formant=std::sin(pi2*430.f*v.age)*(0.24f+0.35f*std::sin(pi2*1.7f*v.age));
  raw=.78f*raw+.22f*formant;
 }
 if(index==33)raw*=.50f+.5f*std::sin(pi2*17.f*v.age); // detuned radio AM
 if(index==31||index==34||index==35){ // distinctive clocked glitch / bit-hold
  if(++v.decim>= (index==34?11:index==31?7:5)){v.decim=0;v.noiseHeld=raw;}
  raw=v.noiseHeld;
 }
 if(p.tremoloHz>0)raw*=1.f-p.tremoloDepth*(.5f-.5f*std::sin(pi2*p.tremoloHz*v.age));
 if(p.drive>0.001f)raw=std::tanh(raw*(1.f+p.drive*3.f))*.89f;
 const float attack=std::max(.001f,p.attack);
 if(v.release){const float decay=std::min(1.f,t/std::max(.025f,p.release));v.env*=1.f-decay;}
 else {const float inc=std::min(1.f,t/attack);v.env+=(1.f-v.env)*inc;}
 if(v.release&&v.env<.00025f){v.active=false;return 0.f;}
 const float cutoffHz=std::clamp(cutoff*p.cutoff*(.43f+.94f*v.y)+280.f*v.pitchEnv,55.f,fs*.45f);
 const float alpha=std::clamp(pi2*cutoffHz/fs,0.001f,.89f);
 v.filter+=(raw-v.filter)*alpha;
 v.filter2+=(v.filter-v.filter2)*alpha;
 const float high=raw-v.filter,band=v.filter-v.filter2;
 float shaped=v.filter2;
 if(p.a==Wave::Glass||p.a==Wave::Fold)shaped=0.68f*v.filter+0.32f*band;
 if(p.a==Wave::Noise)shaped=.38f*high+.22f*band+.40f*raw;
 if(p.a==Wave::Formant)shaped=.55f*v.filter+.45f*band;
 shaped+=res*.12f*raw;
 const float peak=(index==1||index==34?.12f:index==24||index==26?.17f:p.attack>.2f?.24f:.21f);
 return shaped*v.env*v.vel*peak*patchGain[static_cast<std::size_t>(index)];
}
} // namespace meow
