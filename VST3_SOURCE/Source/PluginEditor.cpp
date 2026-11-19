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
    setResizeLimits(700, 540, 1700, 1450);
    setSize(processor.editorWidth.load(), processor.editorHeight.load());
    browser.goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz(25);
}

MeowcillatorEditor::~MeowcillatorEditor(){ stopTimer(); processor.padDown.store(false); }

void MeowcillatorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff080808));
}
void MeowcillatorEditor::resized(){ browser.setBounds(getLocalBounds()); }

void MeowcillatorEditor::nativeMessage(const juce::var& msg)
{
    auto* obj=msg.getDynamicObject(); if(obj==nullptr)return;
    const juce::String type=obj->getProperty("type").toString();
    if(type=="ready") { browserReady=true; sendState(); return; }
    if(type=="midiLearn") { processor.beginMidiLearn(obj->getProperty("target").toString(),(bool)obj->getProperty("relative")); sendState(); return; }
    if(type=="midiForget") { processor.forgetMidiMapping(obj->getProperty("target").toString()); sendState(); return; }
    if(type=="midiClearAll") { processor.clearMidiMappings(); sendState(); return; }
    if(type=="resize") {
        // Only the JUCE editor/host owns its window size; never resize via DOM.
        const int w=juce::jlimit(700,1700,(int)obj->getProperty("width"));
        const int h=juce::jlimit(540,1450,(int)obj->getProperty("height"));
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
    if(type=="cat") {
        processor.triggerCat.store(juce::jlimit(0,5,(int)obj->getProperty("index")));
        return;
    }
    if(type=="preset") {
        processor.setCurrentProgram(juce::jlimit(0,36,(int)obj->getProperty("index")));
        return;
    }
    if(type=="transport") {
        processor.play.store((bool)obj->getProperty("on"));
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
    juce::DynamicObject::Ptr data=new juce::DynamicObject();
    juce::DynamicObject::Ptr params=new juce::DynamicObject();
    const char* ids[]={"osc","drums","bpm","glide","cutoff","resonance","fxmix","drive","feedback",
       "gate","scale","key","octave","groove","kit","effect","rate","arpType","color","motion","motionPower",
       "arp","hold","drumsOn"};
    for(const auto* id:ids)
        if(const auto* p=processor.apvts.getRawParameterValue(id))params->setProperty(id,(double)p->load());
    data->setProperty("parameters",juce::var(params.get()));
    data->setProperty("preset",processor.getCurrentProgram());
    data->setProperty("midiMappings",processor.midiMappingState());
    data->setProperty("midiLearning",juce::String(MeowcillatorProcessor::midiTargetName(processor.pendingMidiLearn())));
    browser.emitEventIfBrowserIsVisible("meowState",juce::var(data.get()));
}

void MeowcillatorEditor::timerCallback()
{
    if(!browserReady || !browser.isVisible())return;
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
    browser.emitEventIfBrowserIsVisible("meowFrame",juce::var(frame.get()));
    const auto revision=processor.midiMapRevision.load(std::memory_order_relaxed);
    if(revision!=lastMidiRevision){lastMidiRevision=revision;sendState();}
    else if(++stateCounter%8==0)sendState();
}
