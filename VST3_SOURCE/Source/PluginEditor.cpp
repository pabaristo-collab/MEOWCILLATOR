#include "PluginEditor.h"
#include <BinaryData.h>
#include <array>
#include <cmath>

namespace {
constexpr juce::uint32 paper=0xfff4f1e8;
constexpr juce::uint32 charcoal=0xff0b0b0d;
constexpr juce::uint32 panel=0xff171719;
constexpr juce::uint32 grey=0xff969696;
constexpr juce::uint32 accent=0xffe5e0d7;
constexpr float baseWidth=1280.f, baseHeight=800.f;
void label(juce::Graphics& g,const juce::String& t,juce::Rectangle<float> r,float size=13.f,
           juce::Justification a=juce::Justification::centredLeft,juce::Colour c=juce::Colour(paper)){
    g.setColour(c);g.setFont(juce::Font(juce::FontOptions(size,juce::Font::bold)));
    g.drawFittedText(t,r.toNearestInt(),a,1);
}
void panelRect(juce::Graphics& g,juce::Rectangle<float> r,const char* title){
    g.setColour(juce::Colour(panel));g.fillRoundedRectangle(r,9.f);
    g.setColour(juce::Colour(grey));g.drawRoundedRectangle(r,9.f,1.2f);
    g.setColour(juce::Colour(paper));g.fillRoundedRectangle(r.getX()+11.f,r.getY()+8.f,
                                                         juce::jmin(190.f,55.f+juce::String(title).length()*9.f),22.f,2.f);
    label(g,title,{r.getX()+16.f,r.getY()+9.f,r.getWidth()-32.f,20.f},13.f,
          juce::Justification::centredLeft,juce::Colour(charcoal));
}
}

class MeowcillatorEditor::NativeStyle final : public juce::LookAndFeel_V4 {
public:
    NativeStyle(){
        setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff222224));
        setColour(juce::ComboBox::textColourId,juce::Colour(paper));
        setColour(juce::ComboBox::outlineColourId,juce::Colour(grey));
        setColour(juce::TextButton::buttonColourId,juce::Colour(0xff29292b));
        setColour(juce::TextButton::buttonOnColourId,juce::Colour(paper));
        setColour(juce::TextButton::textColourOffId,juce::Colour(paper));
        setColour(juce::TextButton::textColourOnId,juce::Colour(charcoal));
        setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff222224));
        setColour(juce::PopupMenu::textColourId,juce::Colour(paper));
        setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(paper));
        setColour(juce::MidiKeyboardComponent::blackNoteColourId,juce::Colour(charcoal));
        setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,juce::Colour(0xffc9c9c9));
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) override {
        const float radius=juce::jmin(w,h)*.37f;
        const auto cx=x+w*.5f,cy=y+h*.5f;
        g.setColour(juce::Colour(0xff080809));g.fillEllipse(cx-radius-2.f,cy-radius-2.f,(radius+2.f)*2,(radius+2.f)*2);
        g.setColour(juce::Colour(0xff999999));g.drawEllipse(cx-radius-2.f,cy-radius-2.f,(radius+2.f)*2,(radius+2.f)*2,1.2f);
        for(int i=0;i<=10;++i){
            float a=start+(end-start)*i/10.f;
            g.setColour(juce::Colour(0xff777777));
            const auto p1=juce::Point<float>(cx+std::sin(a)*(radius+3.f),cy-std::cos(a)*(radius+3.f));
            const auto p2=juce::Point<float>(cx+std::sin(a)*(radius+6.f),cy-std::cos(a)*(radius+6.f));
            g.drawLine({p1,p2},1.f);
        }
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff414142),cx-radius,cy-radius,
                                                 juce::Colour(0xff080809),cx+radius,cy+radius,false));
        g.fillEllipse(cx-radius,cy-radius,radius*2.f,radius*2.f);
        const float a=start+(end-start)*pos;
        auto p1=juce::Point<float>(cx+std::sin(a)*radius*.15f,cy-std::cos(a)*radius*.15f);
        auto p2=juce::Point<float>(cx+std::sin(a)*radius*.84f,cy-std::cos(a)*radius*.84f);
        g.setColour(juce::Colour(paper));g.drawLine({p1,p2},3.1f);
    }
};

class MeowcillatorEditor::Knob final : public juce::Component {
public:
    juce::Slider slider;
    juce::String displayName;
    float defaultValue=0.f;
    std::function<void()> learn;
    std::function<void()> forget;
    std::function<void()> manualTouch;
    explicit Knob(const juce::String& name):displayName(name){
        addAndMakeVisible(slider);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,86,20);
        slider.setDoubleClickReturnValue(true,0.0); // Default value replaced by addKnob().
        slider.setPopupDisplayEnabled(false,false,nullptr);
        slider.setColour(juce::Slider::textBoxTextColourId,juce::Colour(paper));
        slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(charcoal));
        slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
        slider.addMouseListener(this,true);
    }
    void resized() override{slider.setBounds(getLocalBounds().withTrimmedTop(19));}
    void paint(juce::Graphics& g) override{
        label(g,displayName,{0.f,0.f,(float)getWidth(),20.f},12.5f,juce::Justification::centred);
    }
    void mouseDown(const juce::MouseEvent& e) override{
        if(!e.mods.isPopupMenu()){if(manualTouch)manualTouch();return;}
        juce::PopupMenu menu;menu.addItem(1,"MIDI LEARN");menu.addItem(2,"CLEAR MIDI MAPPING");
        juce::Component::SafePointer<Knob> self(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[self](int option){
            if(self){if(option==1&&self->learn)self->learn();if(option==2&&self->forget)self->forget();}
        });
    }
};

class MeowcillatorEditor::LearnButton final : public juce::TextButton {
public:
    std::function<void()> learn;
    std::function<void()> forget;
    using juce::TextButton::TextButton;
    void mouseDown(const juce::MouseEvent& e) override{
        if(e.mods.isPopupMenu()){
            juce::PopupMenu m;m.addItem(1,"MIDI LEARN");m.addItem(2,"CLEAR MIDI MAPPING");
            juce::Component::SafePointer<LearnButton> self(this);
            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[self](int n){
                if(self){if(n==1&&self->learn)self->learn();if(n==2&&self->forget)self->forget();}
            });
        }else juce::TextButton::mouseDown(e);
    }
};

class MeowcillatorEditor::LearnCombo final : public juce::ComboBox {
public:
    std::function<void()> learn,forget;
    void mouseDown(const juce::MouseEvent& e) override{
        if(e.mods.isPopupMenu()){
            juce::PopupMenu menu;menu.addItem(1,"MIDI LEARN");menu.addItem(2,"CLEAR MIDI MAPPING");
            juce::Component::SafePointer<LearnCombo> self(this);
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[self](int result){
                if(self){if(result==1 && self->learn)self->learn();if(result==2 && self->forget)self->forget();}
            });
        }else juce::ComboBox::mouseDown(e);
    }
};

class MeowcillatorEditor::XYPad final : public juce::Component {
public:
    explicit XYPad(MeowcillatorProcessor& p,juce::Image logo):processor(p),cat(std::move(logo)){
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }
    void mouseDown(const juce::MouseEvent& e) override{update(e,true);}
    void mouseDrag(const juce::MouseEvent& e) override{update(e,true);}
    void mouseUp(const juce::MouseEvent& e) override{update(e,false);}
    void mouseExit(const juce::MouseEvent& e) override{if(!e.mods.isAnyMouseButtonDown())processor.padDown.store(false);}
    void paint(juce::Graphics& g) override{
        const auto b=getLocalBounds().toFloat().reduced(2.f);
        g.setColour(juce::Colour(0xff030305));g.fillRoundedRectangle(b,11.f);
        g.setColour(juce::Colour(0xff9f9f9f));g.drawRoundedRectangle(b,11.f,1.6f);
        const float energy=juce::jlimit(0.f,1.f,processor.audioLevel.load(std::memory_order_relaxed)*2.2f);
        for(int i=0;i<12;++i){
            auto r=b.reduced(14.f+(float)i*5.f);
            g.setColour(juce::Colour(paper).withAlpha((0.025f+energy*.055f)*((i%3)==0?1.4f:.75f)));
            g.drawRoundedRectangle(r,juce::jmax(4.f,18.f-i),1.f);
        }
        if(cat.isValid()){
            const float w=juce::jmin(b.getWidth()*.72f,b.getHeight()*.72f);
            juce::Rectangle<float> cr(b.getCentreX()-w*.5f,b.getCentreY()-w*.5f,w,w);
            g.setOpacity(1.f);g.drawImage(cat,cr,juce::RectanglePlacement::centred);
        }else{
            label(g,"MEOW",b.reduced(20),58.f,juce::Justification::centred);
        }
        const bool down=processor.padDown.load(std::memory_order_relaxed);
        if(down){
            const float px=processor.padX.load(std::memory_order_relaxed);
            const float py=processor.padY.load(std::memory_order_relaxed);
            const float x=b.getX()+b.getWidth()*px;
            const float y=b.getY()+b.getHeight()*(1.f-py);
            g.setColour(juce::Colour(paper).withAlpha(.82f));g.drawEllipse(x-12.f,y-12.f,24.f,24.f,2.f);
        }
        const int midiNote=processor.midiVisualNote.load(std::memory_order_relaxed);
        if(processor.midiVisualActive.load(std::memory_order_relaxed)&&midiNote>=0){
            const float x=b.getX()+b.getWidth()*juce::jlimit(0.f,1.f,float(midiNote-12)/108.f);
            const float y=b.getY()+b.getHeight()*(1.f-processor.midiVisualY.load(std::memory_order_relaxed));
            g.setColour(juce::Colours::white.withAlpha(.7f));g.fillEllipse(x-5.f,y-5.f,10.f,10.f);
        }
        label(g,"X / Y PERFORMANCE  ·  TOUCH OR MIDI",{b.getX()+14.f,b.getY()+10.f,b.getWidth()-28.f,20.f},13.f);
    }
private:
    MeowcillatorProcessor& processor;juce::Image cat;
    void update(const juce::MouseEvent& e,bool down){
        auto b=getLocalBounds().toFloat().reduced(2.f);
        processor.padX.store(juce::jlimit(0.f,.9999f,((float)e.x-b.getX())/juce::jmax(1.f,b.getWidth())),std::memory_order_relaxed);
        processor.padY.store(juce::jlimit(0.f,1.f,1.f-((float)e.y-b.getY())/juce::jmax(1.f,b.getHeight())),std::memory_order_relaxed);
        processor.padDown.store(down,std::memory_order_release);
        repaint();
    }
};

void MeowcillatorEditor::addKnob(const char* id,const char* name,float x,float y,float w,float h){
    KnobEntry entry;
    entry.id=id;entry.base={x,y,w,h};entry.component=std::make_unique<Knob>(name);
    auto* p=proc.apvts.getParameter(id);
    jassert(p!=nullptr);
    if(p!=nullptr){
        const float v=p->convertFrom0to1(p->getDefaultValue());
        entry.component->defaultValue=v;
        entry.component->slider.setDoubleClickReturnValue(true,(double)v);
    }
    entry.component->learn=[this,id](){proc.beginMidiLearn(id);};
    entry.component->forget=[this,id](){proc.forgetMidiMapping(id);};
    addAndMakeVisible(*entry.component);
    entry.attachment=std::make_unique<SliderAttachment>(proc.apvts,id,entry.component->slider);
    // Do not replace Slider::onValueChange: SliderAttachment owns it.
    // Instead clear the controller override on the mouse-down listener.
    entry.component->manualTouch=[this,id]{proc.clearMidiOverride(id);};
    knobs.push_back(std::move(entry));
}
void MeowcillatorEditor::addChoice(const char* id,float x,float y,float w,float h){
    SelectEntry entry;entry.id=id;entry.base={x,y,w,h};entry.component=std::make_unique<LearnCombo>();
    if(auto* p=dynamic_cast<juce::AudioParameterChoice*>(proc.apvts.getParameter(id))){
        for(int i=0;i<p->choices.size();++i)entry.component->addItem(p->choices[i],i+1);
    }
    entry.component->learn=[this,id]{proc.beginMidiLearn(id);};
    entry.component->forget=[this,id]{proc.forgetMidiMapping(id);};
    entry.component->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*entry.component);
    entry.attachment=std::make_unique<ComboAttachment>(proc.apvts,id,*entry.component);
    choices.push_back(std::move(entry));
}
void MeowcillatorEditor::addToggle(const char* id,const char* title,float x,float y,float w,float h){
    ToggleEntry entry;entry.base={x,y,w,h};entry.button=std::make_unique<LearnButton>(title);
    entry.button->setClickingTogglesState(true);
    entry.button->learn=[this,id]{proc.beginMidiLearn(id);};
    entry.button->forget=[this,id]{proc.forgetMidiMapping(id);};
    addAndMakeVisible(*entry.button);
    entry.attachment=std::make_unique<ButtonAttachment>(proc.apvts,id,*entry.button);
    toggles.push_back(std::move(entry));
}
MeowcillatorEditor::MeowcillatorEditor(MeowcillatorProcessor& p)
 : juce::AudioProcessorEditor(p),proc(p),style(std::make_unique<NativeStyle>()){
    setLookAndFeel(style.get());
    cat=juce::ImageFileFormat::loadFrom(BinaryData::cat_png,BinaryData::cat_pngSize);
    xy=std::make_unique<XYPad>(proc,cat);addAndMakeVisible(*xy);
    keyboardState.addListener(this);
    keyboard.setAvailableRange(0,127);
    keyboard.setLowestVisibleKey(36);
    keyboard.setKeyWidth(25.f);
    addAndMakeVisible(keyboard);
    keyboardLabel.setText("FULL MIDI RANGE  0–127    ·    KEYBOARD / CONTROLLER    ·    DRUMS ON CH 10",juce::dontSendNotification);
    keyboardLabel.setJustificationType(juce::Justification::centredLeft);
    keyboardLabel.setColour(juce::Label::textColourId,juce::Colour(paper));
    keyboardLabel.setFont(juce::Font(juce::FontOptions(13.f,juce::Font::bold)));
    addAndMakeVisible(keyboardLabel);
    info.setJustificationType(juce::Justification::centredLeft);
    info.setColour(juce::Label::textColourId,juce::Colour(paper));
    info.setFont(juce::Font(juce::FontOptions(13.f)));
    addAndMakeVisible(info);
    addKnob("osc","VOL",618,131,97,92);
    addKnob("glide","GLIDE",730,131,97,92);
    addKnob("cutoff","CUTOFF",864,131,110,92);
    addKnob("resonance","RESO",971,131,110,92);
    addKnob("drive","DIST",1079,131,110,92);
    addKnob("gate","GATE",620,291,96,85);
    addKnob("motionPower","MOTION",726,291,96,85);
    addKnob("fxmix","MIX",891,311,83,85);
    addKnob("fxTime","TIME",968,311,83,85);
    addKnob("feedback","FDBK",1044,311,83,85);
    addKnob("fxSpeed","SPEED",1122,311,83,85);
    addKnob("bpm","BPM",1045,523,92,73);
    addKnob("drums","DRUM VOL",1141,523,92,73);
    addKnob("padReverb","PAD VERB",1007,681,95,75);
    addKnob("padCrunch","PAD CRUNCH",1110,681,95,75);
    addChoice("rate",619,262,108,28);
    addChoice("arpType",737,262,108,28);
    addToggle("arp","ARP  OFF / ON",618,376,128,32);
    addToggle("hold","HOLD  OFF / ON",754,376,128,32);
    addChoice("effect",895,281,337,27);
    addChoice("key",45,664,110,31);
    addChoice("scale",169,664,154,31);
    addChoice("octave",337,664,110,31);
    addChoice("groove",605,547,229,35);
    addChoice("kit",846,547,190,35);
    addToggle("splitOut","SPLIT AUDIO 1/2 + 3/4",619,718,214,34);
    for(int i=0;i<37;++i)presetSelect.addItem(juce::String::formatted("%02d  ",i+1)+p.getProgramName(i),i+1);
    presetSelect.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);
    presetSelect.onChange=[this]{if(presetSelect.getSelectedId()>0)proc.setCurrentProgram(presetSelect.getSelectedId()-1);};
    addAndMakeVisible(presetSelect);
    transport=std::make_unique<LearnButton>("PLAY  /  STOP");
    transport->learn=[this]{proc.beginMidiLearn("play");};
    transport->forget=[this]{proc.forgetMidiMapping("play");};
    transport->onClick=[this]{
        const bool shouldPlay=!proc.play.load(std::memory_order_acquire);
        proc.play.store(shouldPlay,std::memory_order_release);
        stoppedByUser=!shouldPlay;
        updateTransportLabel();
    };
    addAndMakeVisible(*transport);
    panic.onClick=[this]{
        proc.padDown.store(false,std::memory_order_release);
        for(auto& key:proc.uiKeyboardNotes)key.store(false,std::memory_order_release);
        proc.play.store(false,std::memory_order_release);
        proc.requestPanic.store(true,std::memory_order_release);
        updateTransportLabel();
    };
    addAndMakeVisible(panic);
    const std::array<const char*,5> names={"KICK · 36","SNARE · 38","CLAP · 39","CLOSED HH · 42","OPEN HH · 46"};
    for(int i=0;i<5;++i){
        auto b=std::make_unique<LearnButton>(names[(size_t)i]);
        b->onClick=[this,i]{proc.drumPadRequests[(size_t)i].fetch_add(1,std::memory_order_release);};
        const auto target="drumPad"+juce::String(i);
        b->learn=[this,target]{proc.beginMidiLearn(target);};
        b->forget=[this,target]{proc.forgetMidiMapping(target);};
        addAndMakeVisible(*b);pads[(size_t)i]=std::move(b);
    }
    setOpaque(true);setResizable(true,true);setResizeLimits(1024,640,1600,1000);
    if(auto* c=getConstrainer())c->setFixedAspectRatio(1.6);
    const int w=juce::jlimit(1024,1600,p.editorWidth.load());
    setSize(w,juce::roundToInt(w/1.6f));
    startTimerHz(24);updateTransportLabel();
}
MeowcillatorEditor::~MeowcillatorEditor(){
    stopTimer();keyboardState.removeListener(this);
    proc.padDown.store(false,std::memory_order_release);
    for(auto& k:proc.uiKeyboardNotes)k.store(false,std::memory_order_release);
    setLookAndFeel(nullptr);
}
void MeowcillatorEditor::handleNoteOn(juce::MidiKeyboardState*,int,int note,float){
    if(note>=0&&note<128)proc.uiKeyboardNotes[(size_t)note].store(true,std::memory_order_release);
}
void MeowcillatorEditor::handleNoteOff(juce::MidiKeyboardState*,int,int note,float){
    if(note>=0&&note<128)proc.uiKeyboardNotes[(size_t)note].store(false,std::memory_order_release);
}
void MeowcillatorEditor::updateTransportLabel(){
    const bool on=proc.play.load(std::memory_order_acquire);
    transport->setButtonText(on?"■ STOP RHYTHM":"▶ PLAY RHYTHM");
    transport->setColour(juce::TextButton::buttonColourId,on?juce::Colour(0xffdedbd2):juce::Colour(0xff343437));
    transport->setColour(juce::TextButton::textColourOffId,on?juce::Colour(charcoal):juce::Colour(paper));
}
void MeowcillatorEditor::timerCallback(){
    // All MIDI note/audio control is processed in C++, not this 24 Hz UI timer.
    updateTransportLabel();
    if(lastPreset!=proc.getCurrentProgram()){
        lastPreset=proc.getCurrentProgram();
        presetSelect.setSelectedId(lastPreset+1,juce::dontSendNotification);
    }
    for(int i=0;i<5;++i){
        auto& glow=padGlow[(size_t)i];
        const auto count=proc.drumHitCounts[(size_t)i].load(std::memory_order_relaxed);
        if(count!=previousDrumHits[(size_t)i]){previousDrumHits[(size_t)i]=count;glow=.90f;}
        else glow*=.67f;
        const auto greyValue=(juce::uint8)juce::jlimit(36.f,212.f,40.f+glow*178.f);
        auto c=juce::Colour(greyValue,greyValue,greyValue);
        pads[(size_t)i]->setColour(juce::TextButton::buttonColourId,c);
        pads[(size_t)i]->setColour(juce::TextButton::textColourOffId,glow>.48f?juce::Colour(charcoal):juce::Colour(paper));
    }
    if(xy)xy->repaint();
    const int note=proc.midiVisualNote.load(std::memory_order_relaxed);
    const auto msg=proc.pendingMidiLearn();
    info.setText(msg>=0?"MIDI LEARN ACTIVE  ·  MOVE YOUR CONTROLLER":
        juce::String("MIDI: ")+(note>=0?juce::MidiMessage::getMidiNoteName(note,true,true,3):"READY")+
        "     ·     CH 1–9, 11–16 → SYNTH     ·     CH 10 → DRUMS",juce::dontSendNotification);
}
void MeowcillatorEditor::resized(){
    const float sx=(float)getWidth()/baseWidth,sy=(float)getHeight()/baseHeight;
    auto R=[&](float x,float y,float w,float h){return juce::Rectangle<int>(juce::roundToInt(x*sx),juce::roundToInt(y*sy),juce::roundToInt(w*sx),juce::roundToInt(h*sy));};
    xy->setBounds(R(24,100,560,452));
    keyboardLabel.setBounds(R(36,562,530,23));
    keyboard.setBounds(R(36,585,530,50));
    info.setBounds(R(36,706,550,25));
    presetSelect.setBounds(R(612,469,477,30));
    transport->setBounds(R(606,679,221,35));
    panic.setBounds(R(842,718,80,34));
    for(auto& e:knobs)e.component->setBounds(R(e.base.getX(),e.base.getY(),e.base.getWidth(),e.base.getHeight()));
    for(auto& e:choices)e.component->setBounds(R(e.base.getX(),e.base.getY(),e.base.getWidth(),e.base.getHeight()));
    for(auto& e:toggles)e.button->setBounds(R(e.base.getX(),e.base.getY(),e.base.getWidth(),e.base.getHeight()));
    for(int i=0;i<5;++i)pads[(size_t)i]->setBounds(R(604.f+i*125.f,600,118,57));
    proc.editorWidth.store(getWidth(),std::memory_order_relaxed);
    proc.editorHeight.store(getHeight(),std::memory_order_relaxed);
}
void MeowcillatorEditor::paint(juce::Graphics& gr){
    gr.fillAll(juce::Colour(charcoal));
    juce::Graphics::ScopedSaveState scoped(gr); // Proportional native layout.
    auto& g=gr;
    g.addTransform(juce::AffineTransform::scale((float)getWidth()/baseWidth,(float)getHeight()/baseHeight));
    g.setColour(juce::Colour(0xff111112));g.fillRoundedRectangle({10.f,10.f,1260.f,780.f},18.f);
    g.setColour(juce::Colour(grey));g.drawRoundedRectangle({10.f,10.f,1260.f,780.f},18.f,1.2f);
    label(g,"MEOW",{32.f,22.f,320.f,54.f},46.f);
    label(g,"CILLATOR",{263.f,22.f,440.f,54.f},44.f);
    label(g,"by pablo aristo    ·    DIY SYNTH PUNK",{800.f,32.f,435.f,35.f},17.f,juce::Justification::centredRight);
    panelRect(g,{604.f,100.f,238.f,137.f},"SOUND");
    panelRect(g,{851.f,100.f,398.f,137.f},"FILTER");
    panelRect(g,{604.f,247.f,276.f,183.f},"ARP / SEQ");
    panelRect(g,{888.f,247.f,361.f,183.f},"FX");
    panelRect(g,{604.f,439.f,645.f,65.f},"PRESET / BANK");
    panelRect(g,{590.f,514.f,669.f,251.f},"DRUM MACHINE  ·  5 LIVE PADS");
    label(g,"RHYTHM",{607.f,526.f,207.f,18.f},11.f);
    label(g,"DRUM SOUND",{846.f,526.f,190.f,18.f},11.f);
    label(g,"KEY",{44.f,642.f,104.f,18.f},11.f);
    label(g,"SCALE",{168.f,642.f,134.f,18.f},11.f);
    label(g,"XY OCTAVE",{337.f,642.f,115.f,18.f},11.f);
    label(g,"MAKE NOISE.  HAVE FUN.",{24.f,747.f,560.f,24.f},17.f);
}
