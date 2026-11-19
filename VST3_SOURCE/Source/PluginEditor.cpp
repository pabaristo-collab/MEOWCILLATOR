#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

void PedalLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float pos,float start,float end,juce::Slider&){
 const float diameter=juce::jmin(float(width),float(height))-13.f;
 auto centre=juce::Point<float>{float(x)+float(width)/2.f,float(y)+float(height)/2.f};
 auto bounds=juce::Rectangle<float>(diameter,diameter).withCentre(centre);
 g.setColour(juce::Colour(0xff040404));g.fillEllipse(bounds.expanded(4));
 for(int i=0;i<=12;i++){float a=start+(end-start)*float(i)/12.f;auto p1=centre+juce::Point<float>{std::sin(a),-std::cos(a)}*(diameter*.54f);auto p2=centre+juce::Point<float>{std::sin(a),-std::cos(a)}*(diameter*.61f);g.setColour(i%3==0?juce::Colour(0xffded7cd):juce::Colour(0xff857d74));g.drawLine(p1.x,p1.y,p2.x,p2.y,1.25f);}
 juce::ColourGradient gr(juce::Colour(0xff54514e),centre.x-diameter*.25f,centre.y-diameter*.38f,juce::Colour(0xff080909),centre.x+diameter*.3f,centre.y+diameter*.4f,false);
 g.setGradientFill(gr);g.fillEllipse(bounds);
 g.setColour(juce::Colour(0xff88817b));g.drawEllipse(bounds,1.15f);
 for(int i=0;i<8;i++){g.setColour(juce::Colours::white.withAlpha(.03f));g.drawEllipse(bounds.reduced(float(i+4)),.65f);}
 const float angle=start+pos*(end-start),dx=std::sin(angle),dy=-std::cos(angle);
 const auto from=centre+juce::Point<float>{dx,dy}*diameter*.12f;const auto to=centre+juce::Point<float>{dx,dy}*diameter*.41f;
 g.setColour(juce::Colours::white);g.drawLine(from.x,from.y,to.x,to.y,3.f);g.setColour(accent.withAlpha(.48f));g.drawEllipse(bounds.expanded(2),1.f);
}

MeowcillatorEditor::MeowcillatorEditor(MeowcillatorProcessor& p):AudioProcessorEditor(&p),proc(p){
 setResizable(true,true);setResizeLimits(840,840,1120,1120);
 if(auto* constrainer=getConstrainer())constrainer->setFixedAspectRatio(1.0);
 padArea={302,249,502,476};setSize(980,980);
 cat=juce::ImageFileFormat::loadFrom(BinaryData::cat_png,BinaryData::cat_pngSize);
 wear=juce::ImageFileFormat::loadFrom(BinaryData::rat_wear_png,BinaryData::rat_wear_pngSize);
 for(int i=0;i<37;i++)presets.addItem(proc.getProgramName(i),i+1);presets.setSelectedId(proc.getCurrentProgram()+1,juce::dontSendNotification);
 addAndMakeVisible(presets);addAndMakeVisible(prevPreset);addAndMakeVisible(nextPreset);
 presets.onChange=[this]{proc.setCurrentProgram(presets.getSelectedId()-1);repaint();};
 prevPreset.onClick=[this]{presets.setSelectedId(1+((presets.getSelectedId()+35)%37));};
 nextPreset.onClick=[this]{presets.setSelectedId(1+(presets.getSelectedId()%37));};
 addAndMakeVisible(playButton);playButton.onClick=[this]{proc.play=!proc.play.load();};
 addAndMakeVisible(helpButton);helpButton.onClick=[] {juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"MEOWCILLATOR · QUICK START","XY PAD: move to play. ARP + HOLD latch musical notes. RHYTHM sets starting BPM; BPM remains editable. CAT SOUNDS: six synthesized feline effects; no external files. MIDI OUT: synth channel 1, drums channel 10, cats channel 16. Render WAV in REAPER.");};
 dial("osc","VOL",{55,302,91,110});dial("glide","GLIDE",{160,302,91,110});
 dial("gate","GATE",{162,670,91,110});
 dial("cutoff","CUTOFF",{848,312,91,107});dial("resonance","RESO",{958,312,91,107});
 dial("fxmix","MIX",{838,493,69,104});dial("drive","DRIVE",{925,493,69,104});dial("feedback","FEEDBACK",{1012,493,69,104});
 dial("drums","DRUM LVL",{744,931,82,103});dial("bpm","BPM",{845,931,93,103});dial("motionPower","MOTION",{726,763,69,80});
 combo("key",{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},{51,443,92,28});
 combo("scale",{"MIN PENT","MAJOR","MINOR","BLUES","CHROMATIC"},{152,443,111,28});
 combo("octave",{"-2","-1","0","+1","+2"},{51,495,211,29});
 combo("rate",{"1/4","1/8","1/16","1/32"},{51,738,96,30});
 combo("arpType",{"UP","DOWN","RANDOM"},{51,793,110,30});
 combo("effect",{"OFF","TAPE DELAY","BIT CRUSH","SATURATOR","SPACE / ECHO"},{839,457,233,29});
 combo("groove",MeowcillatorProcessor::grooveNames(),{49,953,294,34});
 combo("kit",MeowcillatorProcessor::kitNames(),{362,953,289,34});
 combo("color",{"BLACK CAT","ORANGE CAT","BLUSH","ICE","ACID"},{334,794,142,28});
 combo("motion",{"LIQUID","PULSE","SPECTRUM","GLITCH"},{493,794,213,28});
 toggle("arp","ARP",{187,551,80,29});toggle("hold","HOLD",{167,793,99,30});
 toggle("drumsOn","DRUMS",{45,1037,93,27});
 toggle("midiSynth","MIDI SYNTH",{143,1037,120,27});
 toggle("midiDrums","MIDI DRUMS",{272,1037,120,27});
 toggle("midiCats","MIDI CATS",{397,1037,114,27});
 for(int i=0;i<6;i++){
  sampleTriggers[size_t(i)].setButtonText(MeowcillatorProcessor::catNames()[i]);sampleTriggers[size_t(i)].onClick=[this,i]{proc.triggerCat=i;};
  addAndMakeVisible(sampleTriggers[size_t(i)]);
 }
 resized();
 startTimerHz(30);
}
MeowcillatorEditor::~MeowcillatorEditor(){proc.padDown=false;stopTimer();for(auto& p:dials)p.second->setLookAndFeel(nullptr);}
void MeowcillatorEditor::dial(const juce::String& id,const juce::String&,juce::Rectangle<int> r){
 auto s=std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,juce::Slider::TextBoxBelow);
 s->setLookAndFeel(&skin);s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,r.getWidth(),19);
 s->setColour(juce::Slider::textBoxTextColourId,juce::Colours::white);
 s->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 baseBounds[s.get()]=r;addAndMakeVisible(*s);
 sliderAttach.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts,id,*s));dials[id]=std::move(s);
}
void MeowcillatorEditor::combo(const juce::String& id,juce::StringArray items,juce::Rectangle<int> r){
 auto c=std::make_unique<juce::ComboBox>();for(int i=0;i<items.size();i++)c->addItem(items[i],i+1);
 c->setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff080909));
 c->setColour(juce::ComboBox::textColourId,juce::Colours::white);
 c->setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff91857b));
 baseBounds[c.get()]=r;addAndMakeVisible(*c);
 comboAttach.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts,id,*c));
 if(id=="groove")c->onChange=[this,cptr=c.get()] {
  const int index=cptr->getSelectedId()-1;if(index<0)return;
  if(auto* param=proc.apvts.getParameter("bpm"))param->setValueNotifyingHost(param->convertTo0to1(float(MeowcillatorProcessor::rhythmBpm(index))));
 };
 combos[id]=std::move(c);
}
void MeowcillatorEditor::toggle(const juce::String& id,const juce::String& label,juce::Rectangle<int> r){
 auto b=std::make_unique<juce::ToggleButton>(label);baseBounds[b.get()]=r;b->setColour(juce::ToggleButton::textColourId,juce::Colours::white);addAndMakeVisible(*b);
 toggleAttach.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts,id,*b));toggles[id]=std::move(b);
}
juce::Colour MeowcillatorEditor::accent() const{
 const int i=int(proc.apvts.getRawParameterValue("color")->load());
 switch(i){case 0:return juce::Colours::white;case 1:return juce::Colour(0xffff8b28);case 2:return juce::Colour(0xffff88c9);case 3:return juce::Colour(0xff69e4ff);case 4:return juce::Colour(0xffcbff57);default:return juce::Colours::white;}
}
void MeowcillatorEditor::drawText(juce::Graphics& g,const juce::String& s,juce::Rectangle<int> r,float size,bool bold,juce::Colour color) const {
 g.setColour(color);g.setFont(juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain)));
 g.drawFittedText(s,r,juce::Justification::centred,1);
}
void MeowcillatorEditor::drawPanel(juce::Graphics& g,juce::Rectangle<int> r,const juce::String& title) const {
 g.setColour(juce::Colour(0xe30a0a0a));g.fillRoundedRectangle(r.toFloat(),12.f);
 g.setColour(juce::Colour(0xff91867b));g.drawRoundedRectangle(r.toFloat(),12.f,1.4f);
 if(title.isNotEmpty()){
  g.setColour(juce::Colour(0xffe3dacd));auto tag=juce::Rectangle<int>(r.getX()+12,r.getY()+9,juce::jmin(184,r.getWidth()-24),27);
  g.fillRect(tag);drawText(g,title,tag,18,true,juce::Colour(0xff141414));
 }
}
void MeowcillatorEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff0b0c0b));
 const float sx=float(getWidth())/1120.f,sy=float(getHeight())/1120.f;
 g.addTransform(juce::AffineTransform::scale(sx,sy));
 auto a=accent();skin.accent=a;
 g.setColour(juce::Colour(0xff141310));g.fillRoundedRectangle(juce::Rectangle<float>(0.f,0.f,1120.f,1120.f).reduced(4),32.f);
 if(wear.isValid()){g.setOpacity(.68f);g.drawImageWithin(wear,5,5,1110,1110,juce::RectanglePlacement::stretchToFit);g.setOpacity(1.f);}
 g.setColour(juce::Colour(0xffaaa094));g.drawRoundedRectangle(juce::Rectangle<float>(0.f,0.f,1120.f,1120.f).reduced(5),31.f,2.4f);
 for(auto p:{juce::Point<float>{29,29},juce::Point<float>{1090,29},juce::Point<float>{29,1089},juce::Point<float>{1090,1089}}){g.setColour(juce::Colour(0xff333331));g.fillEllipse(p.x-10,p.y-10,20,20);g.setColour(juce::Colour(0xffc5bdb3));g.drawLine(p.x-5,p.y,p.x+5,p.y,2.f);g.drawLine(p.x,p.y-5,p.x,p.y+5,2.f);}
 // Header: branded worn-metal faceplate. Old English if installed by the OS, sans fallback.
 drawText(g,"DIY",{46,28,83,26},23,true,juce::Colours::white);drawText(g,"SYNTH PUNK",{44,53,164,26},18,true,juce::Colours::white);
 g.setFont(juce::Font(juce::FontOptions("Old English Text MT",57.f,juce::Font::bold)));g.setColour(juce::Colours::white);
 g.drawFittedText("MEOW",282,17,224,79,juce::Justification::centredRight,1);
 drawText(g,"CILLATOR",{496,17,339,74},60,true,juce::Colours::white);
 g.setFont(juce::Font(juce::FontOptions("Old English Text MT",23.f,juce::Font::plain)));g.drawFittedText("by pablo aristo",377,91,362,30,juce::Justification::centred,1);
 drawText(g,"MAKE NOISE.",{857,26,208,29},20,true,juce::Colours::white);drawText(g,"HAVE FUN.",{859,56,208,29},20,true,a);
 drawPanel(g,{31,134,259,101},"PRESET");drawPanel(g,{302,134,502,101},"");drawPanel(g,{817,134,267,101},"MIDI / HELP");
 drawPanel(g,{31,249,255,289},"SOUND");drawPanel(g,{31,549,255,284},"ARP / SEQ");
 drawPanel(g,{817,249,267,190},"FILTER");drawPanel(g,{817,446,267,200},"FX");drawPanel(g,{817,656,267,179},"CAT SOUNDS");
 drawPanel(g,{302,737,502,98},"PAD / MOOD");drawPanel(g,{31,844,1053,239},"⛓ DRUM MACHINE");
 // X/Y touchscreen with reactive contour field and the untouched black cat logo.
 padArea={302,249,502,476};g.setColour(juce::Colour(0xff030304));g.fillRoundedRectangle(padArea.toFloat(),20.f);
 g.setColour(a);g.drawRoundedRectangle(padArea.toFloat().reduced(3),20.f,2.5f);
 auto inner=padArea.reduced(20);
 for(int i=0;i<22;i++){
  float radius=75.f+i*7.f;juce::Path path;
  auto e=proc.audioLevel.load(),power=proc.apvts.getRawParameterValue("motionPower")->load();
  for(int k=0;k<=110;k++){
   float theta=juce::MathConstants<float>::twoPi*float(k)/110.f;
   const int motionMode=int(proc.apvts.getRawParameterValue("motion")->load());
   const float time=float(juce::Time::getMillisecondCounterHiRes()*.0006);
   float undulate=std::sin(theta*(motionMode==2?17.f:8.f)+time+i*.2f)*(2.f+e*15.f)*power;
   if(motionMode==1)undulate+=std::sin(time*3.f-i*.65f)*e*16.f*power;
   if(motionMode==3)undulate+=(std::sin(theta*25.f+time*2.f)>0.75f?e*30.f:0.f)*power;
   float x=inner.getCentreX()+std::cos(theta)*(radius+undulate);
   float y=inner.getCentreY()+std::sin(theta)*(radius+undulate);
   if(!k)path.startNewSubPath(x,y);else path.lineTo(x,y);
  }
  path.closeSubPath();g.setColour(a.withAlpha(.032f+(i%4==0?.07f:0.f)+e*.08f));g.strokePath(path,juce::PathStrokeType(.75f));
 }
 if(cat.isValid()){
  g.setOpacity(.93f);g.drawImageWithin(cat,inner.getX()+70,inner.getY()+62,inner.getWidth()-140,inner.getHeight()-124,juce::RectanglePlacement::centred|juce::RectanglePlacement::onlyReduceInSize,false);g.setOpacity(1.f);
 }
 drawText(g,"X / Y PERFORMANCE PAD",{343,264,421,21},12,true,juce::Colour(0xffded8ce));
 drawText(g,"DRAG TO PLAY · X=PITCH  Y=EXPRESSION",{334,692,435,20},11,false,juce::Colour(0xffdfd2c2));
 // Instrument status screen
 g.setColour(juce::Colour(0xff030303));g.fillRoundedRectangle({314.f,141.f,480.f,89.f},9.f);
 g.setColour(a.withAlpha(.75f));g.drawRoundedRectangle({314.f,141.f,480.f,89.f},9.f,1.f);
 auto get=[&](const char* id){return proc.apvts.getRawParameterValue(id)->load();};
 drawText(g,"P"+juce::String(proc.program.load()+1).paddedLeft('0',2)+"  "+proc.getProgramName(proc.program.load()),{332,147,444,29},21,true,juce::Colours::white);
 drawText(g,MeowcillatorProcessor::grooveNames()[int(get("groove"))]+" · KIT "+MeowcillatorProcessor::kitNames()[int(get("kit"))],{325,177,459,20},12,false,a);
 drawText(g,"BPM "+juce::String(int(get("bpm")))+"   ARP "+(get("arp")>.5f?"ON":"OFF")+"   HOLD "+(get("hold")>.5f?"ON":"OFF")+"   STEP "+juce::String(proc.currentStep.load()+1),{329,202,450,19},13,false,juce::Colour(0xffe6dbce));
 // Labels next to hardware controls
 for(const auto& q:std::array<std::pair<const char*,juce::Rectangle<int>>,12>{{
  {"VOL",{55,398,90,23}},{"GLIDE",{162,398,90,23}},{"GATE",{162,766,91,19}},
  {"CUTOFF",{842,405,100,21}},{"RESO",{948,405,103,21}},
  {"MIX",{838,580,69,17}},{"DRIVE",{925,580,69,17}},{"FEEDBACK",{997,580,91,17}},
  {"BPM",{840,920,74,22}},{"DRUM LVL",{740,921,105,22}},
  {"MOTION POWER",{695,747,101,19}},{"ARP RATE",{51,711,99,20}}
 }})drawText(g,q.first,q.second,12,true,juce::Colours::white);
 drawText(g,"KEY",{52,423,92,16},11,true);drawText(g,"SCALE",{152,423,110,16},11,true);
 drawText(g,"OCTAVE",{51,474,211,17},12,true);
 drawText(g,"FILTER / FX / DRUMS · MIDI CH 1 / 10 / 16",{535,1038,512,22},11,false,juce::Colour(0xffe2dbd1));
 drawText(g,"SYNTH CAT SOUNDS · INTERNAL DSP",{827,686,244,14},10,false,a);
 drawText(g,"RHYTHM STYLE",{48,926,292,20},12,true);drawText(g,"DRUM SOUND",{359,926,298,20},12,true);
}
void MeowcillatorEditor::resized(){
 const float factor=float(getWidth())/1120.f;
 auto scaled=[factor](juce::Rectangle<int> r){return juce::Rectangle<int>(juce::roundToInt(r.getX()*factor),juce::roundToInt(r.getY()*factor),juce::roundToInt(r.getWidth()*factor),juce::roundToInt(r.getHeight()*factor));};
 presets.setBounds(scaled({75,183,173,36}));prevPreset.setBounds(scaled({44,184,27,35}));nextPreset.setBounds(scaled({253,184,27,35}));
 playButton.setBounds(scaled({955,956,116,39}));helpButton.setBounds(scaled({1035,184,33,34}));
 for(auto& entry:baseBounds)entry.first->setBounds(scaled(entry.second));
 for(int i=0;i<6;i++){
  const int row=i/3,col=i%3,x=832+col*83,y=710+row*57;
  sampleTriggers[size_t(i)].setBounds(scaled({x,y,77,50}));
 }
}
void MeowcillatorEditor::timerCallback(){
 skin.accent=accent();playButton.setButtonText(proc.play.load()?"■ STOP":"▶ PLAY");
 repaint();
}
void MeowcillatorEditor::padAt(juce::Point<float> p){
 proc.padX=juce::jlimit(0.f,.9999f,(p.x-float(padArea.getX()))/float(padArea.getWidth()));
 proc.padY=1.f-juce::jlimit(0.f,1.f,(p.y-float(padArea.getY()))/float(padArea.getHeight()));
}
void MeowcillatorEditor::mouseDown(const juce::MouseEvent& e){auto p=e.position*(1120.f/float(getWidth()));if(padArea.contains(p.toInt())){padAt(p);proc.padDown=true;}}
void MeowcillatorEditor::mouseDrag(const juce::MouseEvent& e){if(proc.padDown.load())padAt(e.position*(1120.f/float(getWidth())));}
void MeowcillatorEditor::mouseUp(const juce::MouseEvent&){proc.padDown=false;}
