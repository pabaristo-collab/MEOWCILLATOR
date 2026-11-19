#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>
#include <cstring>

juce::WebBrowserComponent::Options MeowcillatorEditor::browserOptions()
{
    using Browser = juce::WebBrowserComponent;
    return Browser::Options{}
        .withBackend(Browser::Options::Backend::webview2)
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withEventListener("meow", [this](juce::var msg) { nativeMessage(msg); })
        .withResourceProvider([](const juce::String& path) -> std::optional<Browser::Resource> {
            if (path != "/" && path != "/index.html") return std::nullopt;
            Browser::Resource res;
            res.mimeType="text/html; charset=utf-8";
            const auto* p = reinterpret_cast<const std::byte*>(BinaryData::meow_ui_html);
            res.data.assign(p,p+BinaryData::meow_ui_htmlSize);
            return res;
        });
}

MeowcillatorEditor::MeowcillatorEditor(MeowcillatorProcessor& p)
    : juce::AudioProcessorEditor(p), processor(p), browser(browserOptions())
{
    setOpaque(true);
    addAndMakeVisible(browser);
    setResizable(true, true);
    // The original physical panel measures 1050 x 1261 CSS pixels. Lock the
    // host/editor bounds to that ratio: no large black gutters or clipped UI.
    constexpr double panelAspect = 16.0 / 10.0;
    setResizeLimits(960, 600, 1600, 1000);
    if(auto* bounds = getConstrainer())
        bounds->setFixedAspectRatio(panelAspect);
    // Old projects may have a stored portrait size. Do not inherit an overly
    // tall window when opening the new landscape edition.
    int desiredHeight = juce::jlimit(600, 1000, processor.editorHeight.load());
    if (processor.editorWidth.load() < 960 || processor.editorWidth.load() < processor.editorHeight.load())
        desiredHeight = 750; // 1200 x 750: wider and more readable in REAPER.
    setSize(juce::roundToInt(desiredHeight * panelAspect), desiredHeight);
    browser.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz(30); // MIDI GUI feedback up to 30 Hz; FFT/waveform remains capped at 15 Hz.
}

MeowcillatorEditor::~MeowcillatorEditor(){
 stopTimer(); processor.padDown.store(false);
 for(auto& key:processor.uiKeyboardNotes)key.store(false,std::memory_order_release);
}

void MeowcillatorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff080808));
}
void MeowcillatorEditor::resized()
{
    browser.setBounds(getLocalBounds());
    // Persist sizes changed either with the panel grip or REAPER's own handle.
    processor.editorWidth.store(getWidth(), std::memory_order_relaxed);
    processor.editorHeight.store(getHeight(), std::memory_order_relaxed);
}

void MeowcillatorEditor::nativeMessage(const juce::var& msg)
{
    auto* obj=msg.getDynamicObject(); if(obj==nullptr)return;
    const juce::String type=obj->getProperty("type").toString();
    if(type=="ready") { browserReady=true; lastStateValid=false; sendState(); return; }
    if(type=="midiLearn") { processor.beginMidiLearn(obj->getProperty("target").toString(),(bool)obj->getProperty("relative")); sendState(); return; }
    if(type=="midiForget") { processor.forgetMidiMapping(obj->getProperty("target").toString()); sendState(); return; }
    if(type=="midiClearAll") { processor.clearMidiMappings(); sendState(); return; }
    if(type=="midiCancel") { processor.cancelMidiLearn(); sendState(); return; }
    if(type=="panic") { processor.requestPanic.store(true,std::memory_order_release); processor.play.store(false,std::memory_order_release); sendState(); return; }
    if(type=="resize") {
        // Only the JUCE editor/host owns its window size; never resize via DOM.
        constexpr double panelAspect = 16.0 / 10.0;
        const int h=juce::jlimit(600,1000,(int)obj->getProperty("height"));
        const int w=juce::roundToInt(h*panelAspect);
        processor.editorWidth.store(w);
        processor.editorHeight.store(h);
        if(w!=getWidth() || h!=getHeight())setSize(w,h);
        return;
    }

    if(type=="pad") {
        processor.padX.store(juce::jlimit(0.0f,0.9999f,(float)(double)obj->getProperty("x")),std::memory_order_relaxed);
        processor.padY.store(juce::jlimit(0.0f,1.0f,(float)(double)obj->getProperty("y")),std::memory_order_relaxed);
        processor.padDown.store((bool)obj->getProperty("down"),std::memory_order_release);
        return;
    }
    if(type=="drumPad") {
        const int index=(int)obj->getProperty("index");
        if(index>=0&&index<5)processor.drumPadRequests[static_cast<size_t>(index)].fetch_add(1,std::memory_order_release);
        return;
    }
    if(type=="virtualNote"){
        const int note=(int)obj->getProperty("note");
        if(note>=0&&note<128)
            processor.uiKeyboardNotes[static_cast<size_t>(note)].store((bool)obj->getProperty("down"),std::memory_order_release);
        return;
    }
    if(type=="preset") {
        processor.setCurrentProgram(juce::jlimit(0,36,(int)obj->getProperty("index")));
        return;
    }
    if(type=="transport") {
        processor.play.store((bool)obj->getProperty("on"),std::memory_order_release);
        lastStateValid=false;sendState();
        return;
    }
    if(type=="parameter") {
        const auto id=obj->getProperty("id").toString();
        if(auto* param=processor.apvts.getParameter(id)) {
            const float value=(float)(double)obj->getProperty("value");
            processor.clearMidiOverride(id);
            param->setValueNotifyingHost(param->convertTo0to1(value));
        }
    }
}

void MeowcillatorEditor::sendState()
{
    if (!browserReady) return;
    static constexpr const char* ids[]={"osc","drums","bpm","glide","cutoff","resonance","fxmix","drive","feedback",
        "gate","scale","key","octave","groove","kit","effect","fxTime","fxSpeed","rate","arpType","color","motion","motionPower",
        "arp","hold","splitOut","padReverb","padCrunch"};
    static_assert(sizeof(ids)/sizeof(ids[0])==28, "Sync parameter count changed: review snapshot cache");
    const int preset=processor.getCurrentProgram();
    const int learning=processor.pendingMidiLearn();
    const bool playing=processor.play.load(std::memory_order_acquire);
    const auto revision=processor.midiMapRevision.load(std::memory_order_acquire);
    std::array<float,28> values{};
    bool changed=!lastStateValid || preset!=lastPresetSent || learning!=lastLearnSent || revision!=lastMidiRevision || playing!=lastPlaySent;
    for (size_t i=0;i<values.size();++i)
    {
        // Runtime MIDI CC values are applied in the audio callback, not here.
        values[i]=processor.valueForUi(ids[i]);
        if(!lastStateValid || std::abs(values[i]-lastStateValues[i]) > 0.0005f)
            changed=true;
    }
    if (!changed) return;

    juce::DynamicObject::Ptr data=new juce::DynamicObject();
    juce::DynamicObject::Ptr params=new juce::DynamicObject();
    for(size_t i=0;i<values.size();++i)
        params->setProperty(ids[i],(double)values[i]);
    data->setProperty("parameters",juce::var(params.get()));
    data->setProperty("preset",preset);
    data->setProperty("playing",playing);
    // Mapping strings are more expensive than simple values. Rebuild only on learn/mapping changes.
    if(!lastStateValid || revision!=lastMidiRevision || learning!=lastLearnSent)
    {
        data->setProperty("midiMappings",processor.midiMappingState());
        data->setProperty("midiLearning",juce::String(MeowcillatorProcessor::midiTargetName(learning)));
    }
    browser.emitEventIfBrowserIsVisible("meowState",juce::var(data.get()));
    lastStateValues=values;
    lastPresetSent=preset;
    lastPlaySent=playing;
    lastLearnSent=learning;
    lastMidiRevision=revision;
    lastStateValid=true;
}

void MeowcillatorEditor::timerCallback()
{
    if(!browserReady || !browser.isVisible())return;
    // React to actual MIDI/automation changes on this message-thread timer.
    // MIDI note/CC processing is already realtime in processBlock().
    sendState();
    // The visualizer is optional and runs at half the controller refresh rate.
    if ((++stateCounter & 1u) != 0u) return;
    const unsigned head=processor.visualWriteHead.load(std::memory_order_acquire);
    for(unsigned n=0;n<256;++n){
        const float sample=processor.visualTap[(head+n)&255u].load(std::memory_order_relaxed);
        // Hann window for a 256-point FFT, with independent playback waveform.
        fftBuffer[n]=sample*(0.5f-0.5f*std::cos(juce::MathConstants<float>::twoPi*(float)n/255.0f));
    }
    for(unsigned n=256;n<512;++n)fftBuffer[n]=0.f;
    fft.performFrequencyOnlyForwardTransform(fftBuffer.data());
    juce::Array<juce::var> wave, spectrum;
    wave.ensureStorageAllocated(256);spectrum.ensureStorageAllocated(256);
    for(int i=0;i<256;++i){
        const float sample=processor.visualTap[(head+(unsigned)i)&255u].load(std::memory_order_relaxed);
        wave.add((int)juce::jlimit(0.f,255.f,128.f+sample*115.f));
        const float magnitude=fftBuffer[juce::jmin(i,127)];
        spectrum.add((int)juce::jlimit(0.f,255.f,60.f*std::log1p(magnitude*3.5f)));
    }
    juce::DynamicObject::Ptr frame=new juce::DynamicObject();
    frame->setProperty("wave",juce::var(wave));
    frame->setProperty("fft",juce::var(spectrum));
    frame->setProperty("step",processor.currentStep.load());
    juce::Array<juce::var> padHits,padVelocities;
    for(size_t i=0;i<5;++i){
      padHits.add((int)processor.drumHitCounts[i].load(std::memory_order_acquire));
      padVelocities.add(processor.drumHitVelocities[i].load(std::memory_order_relaxed));
    }
    frame->setProperty("drumHits",juce::var(padHits));
    frame->setProperty("drumVelocities",juce::var(padVelocities));
    frame->setProperty("midiNote",processor.midiVisualNote.load(std::memory_order_relaxed));
    frame->setProperty("midiActive",processor.midiVisualActive.load(std::memory_order_relaxed));
    frame->setProperty("midiY",processor.midiVisualY.load(std::memory_order_relaxed));
    frame->setProperty("midiCount",static_cast<int>(processor.incomingMidiCount.load(std::memory_order_acquire)));
    frame->setProperty("midiType",processor.incomingMidiType.load(std::memory_order_relaxed));
    frame->setProperty("midiChannel",processor.incomingMidiChannel.load(std::memory_order_relaxed));
    frame->setProperty("midiNumber",processor.incomingMidiNumber.load(std::memory_order_relaxed));
    frame->setProperty("midiValue",processor.incomingMidiValue.load(std::memory_order_relaxed));
    browser.emitEventIfBrowserIsVisible("meowFrame",juce::var(frame.get()));

}
