#pragma once
// Four independently voiced synthesised drum machines. The sequencer itself
// lives in RhythmPatterns.h and is deliberately not touched.
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace meow {
struct DrumSynth {
 float kEnv=0,sEnv=0,hEnv=0,oEnv=0,kSweep=0;
 float clapEnv=0,lowTomEnv=0,highTomEnv=0,crashEnv=0;
 float lowTomPhase=0,highTomPhase=0,clapAge=0,crashLP=0;
 float kPhase=0,kSubPhase=0,sPhase=0,metalPhase=0;
 float snareLP=0,hatLP=0,kToneLP=0,sAge=0;
 uint32_t seed=0x5831F1Cu;
 void reset(){*this=DrumSynth{};}
 float noise(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return float(seed&65535u)/32768.f-1.f;}
 void kick(float accent){kEnv=accent;kSweep=1.f;kPhase=0;kSubPhase=0;}
 void snare(float accent){sEnv=accent;sPhase=0;sAge=0;}
 void hat(float accent){hEnv=accent;}
 void openHat(float accent){oEnv=accent;}
 void clap(float accent){clapEnv=accent;clapAge=0;}
 void lowTom(float accent){lowTomEnv=accent;lowTomPhase=0;}
 void highTom(float accent){highTomEnv=accent;highTomPhase=0;}
 void crash(float accent){crashEnv=accent;}
 // One stereo-independent mono sample; each kit has its own oscillator,
 // pitch envelope, noise spectrum, transient and release behaviour.
 float render(int kit,float sr){
  const float fs=std::max(8000.f,sr),dt=1.f/fs;
  const int k=std::clamp(kit,0,3);
  const float n=noise();
  float kickOut=0, snareOut=0, hatOut=0;
  static constexpr float decayK[4]={.145f,.42f,.115f,.285f};
  static constexpr float decayS[4]={.12f,.22f,.23f,.20f};
  static constexpr float decayH[4]={.031f,.074f,.052f,.079f};
  static constexpr float decayO[4]={.22f,.34f,.28f,.41f};
  static constexpr float base[4]={57.f,43.f,80.f,49.f};
  static constexpr float sweep[4]={145.f,95.f,225.f,90.f};
  const float f=base[k]+sweep[k]*kSweep*kSweep;
  kPhase+=f*dt;kPhase-=std::floor(kPhase);
  kSubPhase+=f*.5f*dt;kSubPhase-=std::floor(kSubPhase);
  const float sine=std::sin(6.283185307f*kPhase);
  if(k==0){ // XEROX PUNK: punchy analog kick + noisy click
   kickOut=(.57f*sine+.12f*n*kSweep)*kEnv;
  }else if(k==1){ // COLD MACHINE: deep 808/sub with stable long tail
   kickOut=(.68f*sine+.22f*std::sin(6.283185307f*kSubPhase)+.02f*n*kSweep)*kEnv;
  }else if(k==2){ // INDUSTRIAL METAL: overdriven FM/metallic hit
   kickOut=.56f*std::tanh(3.6f*sine+1.3f*std::sin(6.283185307f*3.7f*kPhase)+n*.47f*kSweep)*kEnv;
  }else{ // DUSTY BOOM BAP: smooth filtered fundamental + tape sub
   float sum=.64f*sine+.20f*std::sin(6.283185307f*kSubPhase);
   kToneLP+=(sum-kToneLP)*.36f;
   kickOut=.54f*std::tanh(kToneLP*1.9f)*kEnv;
  }
  sPhase+=((k==2?380.f:k==1?205.f:k==3?160.f:190.f)*dt);
  sPhase-=std::floor(sPhase);
  snareLP+=(n-snareLP)*(k==0?.15f:k==1?.48f:k==2?.37f:.06f);
  if(k==0){ // punk: snappy highpass white noise + membrane
   snareOut=(.28f*(n-snareLP)+.11f*std::sin(6.283185307f*sPhase))*sEnv;
  }else if(k==1){ // cold drum machine: electronic ring + bright hiss
   snareOut=(.25f*(n-snareLP)+.18f*std::sin(6.283185307f*sPhase))*sEnv;
  }else if(k==2){ // harsh snare + metallic clang overtone
   const float clang=std::sin(6.283185307f*sPhase+2.f*std::sin(6.283185307f*2.34f*sPhase));
   snareOut=(.31f*(n-snareLP)+.22f*clang)*sEnv;
  }else{ // dusty clap: 3 burst edges and filtered noise
   const float clap=(sAge<.008f||(.018f<sAge&&sAge<.027f)||(.036f<sAge&&sAge<.048f))?1.55f:.68f;
   snareOut=(.38f*snareLP*clap+.06f*std::sin(6.283185307f*sPhase))*sEnv;
  }
  sAge+=dt;
  hatLP+=(n-hatLP)*(k==3?.08f:k==1?.71f:k==2?.41f:.28f);
  const float hh= hEnv+oEnv*(k==3?.75f:.64f);
  if(k==0)hatOut=(n-hatLP)*hh*.095f;
  else if(k==1){
   metalPhase+=1345.f*dt;metalPhase-=std::floor(metalPhase);
   hatOut=(.55f*(n-hatLP)+.45f*std::sin(6.283185307f*metalPhase*7.f))*hh*.115f;
  }else if(k==2){
   metalPhase+=2933.f*dt;metalPhase-=std::floor(metalPhase);
   const float clang=std::sin(6.283185307f*metalPhase*3.8f)+.5f*std::sin(6.283185307f*metalPhase*7.2f);
   hatOut=(.43f*(n-hatLP)+.57f*clang)*hh*.14f;
  }else hatOut=(hatLP*.54f+(n-hatLP)*.19f)*hh*.085f;
  // Dedicated additional drum-pad voices. The legacy sequencer never triggers
  // these, so its established 11 rhythms remain byte-for-byte identical.
  const float clapBursts=(clapAge<.009f||(.019f<clapAge&&clapAge<.029f)||(.039f<clapAge&&clapAge<.052f))?1.f:.44f;
  const float clapOut=clapEnv*clapBursts*(k==1?.22f:k==2?.37f:k==3?.27f:.30f)*(k==3?snareLP:n-hatLP);
  const float lowHz=(k==1?91.f:k==2?145.f:k==3?105.f:123.f);
  const float highHz=(k==1?169.f:k==2?229.f:k==3?190.f:206.f);
  lowTomPhase+=lowHz*dt;lowTomPhase-=std::floor(lowTomPhase);
  highTomPhase+=highHz*dt;highTomPhase-=std::floor(highTomPhase);
  const float tomLow=lowTomEnv*.38f*(std::sin(6.283185307f*lowTomPhase)+n*.08f);
  const float tomHigh=highTomEnv*.33f*(std::sin(6.283185307f*highTomPhase)+n*.10f);
  crashLP+=(n-crashLP)*(k==1?.36f:k==3?.14f:.29f);
  const float crashOut=crashEnv*(n-crashLP)*(k==2?.27f:.19f);
  clapAge+=dt;
  clapEnv*=1.f-std::min(1.f,dt/(k==3?.20f:.12f));
  lowTomEnv*=1.f-std::min(1.f,dt/(k==1?.36f:.22f));
  highTomEnv*=1.f-std::min(1.f,dt/(k==1?.28f:.17f));
  crashEnv*=1.f-std::min(1.f,dt/(k==1?.90f:k==3?.70f:.46f));
  // Distinct release curves for each bank; independent closed/open hats.
  kEnv*=1.f-std::min(1.f,dt/decayK[k]);
  sEnv*=1.f-std::min(1.f,dt/decayS[k]);
  hEnv*=1.f-std::min(1.f,dt/decayH[k]);
  oEnv*=1.f-std::min(1.f,dt/decayO[k]);
  kSweep*=1.f-std::min(1.f,dt/(k==2?.026f:k==3?.068f:.046f));
  static constexpr float kitGain[4]={1.f,.60f,.92f,.90f};
  return std::clamp((kickOut+snareOut+hatOut+clapOut+tomLow+tomHigh+crashOut)*kitGain[k],-.86f,.86f);
 }
};
} // namespace meow
