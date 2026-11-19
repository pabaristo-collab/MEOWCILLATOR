#include "../Source/RhythmPatterns.h"
#include "../Source/DrumSoundBank.h"
#include "../Source/SynthSoundBank.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <set>
#include <tuple>
#include <vector>
#include <algorithm>

int main(){
  // All original sequencer rows are distinct; no changes to beat data.
  static_assert(meowRhythms.size()==11,"must preserve 11 rhythms");
  std::set<std::tuple<unsigned,unsigned,unsigned,unsigned,unsigned>> patterns;
  for(const auto& p:meowRhythms)patterns.insert({p.kick,p.snare,p.hat,p.openHat,p.accent});
  assert(patterns.size()==11);
  int goodDrums=0; float globalPeak=0.f;
  for(int kit=0;kit<4;kit++)for(int pad=0;pad<5;pad++){
    meow::DrumSynth v;
    switch(pad){case 0:v.kick(0.9f);break;case 1:v.snare(0.9f);break;case 2:v.clap(0.9f);break;case 3:v.hat(0.9f);break;case 4:v.openHat(0.9f);break;}
    float peak=0.f,sum=0.f;
    for(int i=0;i<11025;++i){float sample=v.render(kit,44100.f);
      assert(std::isfinite(sample));peak=std::max(peak,std::abs(sample));sum+=sample*sample;}
    const float rms=std::sqrt(sum/11025.f);
    if(rms>0.00003f)++goodDrums;
    assert(peak<=.9301f);
    globalPeak=std::max(globalPeak,peak);
    std::printf("kit=%d pad=%d rms=%.5f peak=%.5f\n",kit,pad,rms,peak);
  }
  assert(goodDrums==20);
  // STOP must clear sequencer audio immediately; subsequent PLAY must retrigger.
  {
    meow::DrumSynth engine;
    engine.kick(1.f);
    float before=0.f;for(int i=0;i<100;++i)before+=std::abs(engine.render(0,44100.f));
    assert(before>0.01f);
    engine.reset();
    assert(engine.render(0,44100.f)==0.f);
    engine.kick(1.f);
    float after=0.f;for(int i=0;i<100;++i)after+=std::abs(engine.render(0,44100.f));
    assert(after>0.01f);
  }
  static_assert(meow::patches.size()==37,"must keep 37 sound presets");
  int audiblePresets=0;
  for(int i=0;i<37;i++){
    meow::SynthVoice v;v.active=true;v.note=60;v.currentHz=261.626f;v.targetHz=261.626f;v.vel=.86f;v.patchIndex=i;
    float sum=0.f,peak=0.f;
    for(int t=0;t<13230;++t){
      float o=meow::renderSynth(v,i,6000.f,.35f,0.f,44100.f);
      assert(std::isfinite(o));sum+=o*o;peak=std::max(peak,std::abs(o));
    }
    float rms=std::sqrt(sum/13230.f);if(rms>.00001f)++audiblePresets;
    std::printf("preset=%d rms=%.5f peak=%.5f\n",i+1,rms,peak);
  }
  assert(audiblePresets==37);
  std::printf("PASS: 11 patterns, 20 drum sounds, 37 presets, drum STOP/reset/PLAY, finite audio. Peak %.4f\n",globalPeak);
}
