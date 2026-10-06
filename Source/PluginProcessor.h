#pragma once
#include <JuceHeader.h>
#include "DSP/Engine.h"
#include "DSP/Audition.h"
#include "DSP/Programs.h"
#include "License/NFLicenseManager.h"

namespace nfpe_ids
{
inline juce::String fx(int p)     { return "fx" + juce::String(p + 1); }
inline juce::String knob(int p, int i) { return "k" + juce::String(p + 1) + juce::String(i + 1); }
inline juce::String bypass(int p) { return "bypass" + juce::String(p + 1); }
inline juce::String bpm(int p)    { return "bpm" + juce::String(p + 1); }
inline juce::String mix(int p)    { return "mix" + juce::String(p + 1); }
}

class NFPEAudioProcessor final : public juce::AudioProcessor
{
public:
    NFLicenseManager licenseManager { "NF_PROCESSOR_EFFECTS" };   // only enforced when built with NF_LICENSE_ENFORCE (CMake option NFPE_LICENSE)
    NFPEAudioProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::AudioProcessorValueTreeState apvts;

    // ---- programs (message thread) ----
    static constexpr int kNumPrograms = 99;                      // per bank; slots 0..98 factory, 99..197 user
    nfpe::ProgramData captureProgram() const;
    void applyProgram(const nfpe::ProgramData&);
    void loadProgram(int slot);                                  // 0..98 factory, 99..197 user
    bool storeUserProgram(int userSlot, const juce::String& name);   // userSlot 0..98
    juce::String programTitle(int slot) const;
    bool userProgramExists(int userSlot) const;
    static juce::File userProgramsDirectory();
    int currentProgramSlot() const { return (int) apvts.state.getProperty("program", 0); }
    juce::String currentProgramName() const { return apvts.state.getProperty("presetName", "Big Pop Vocal").toString(); }
    void setProgramNameOnly(const juce::String& n) { apvts.state.setProperty("presetName", n, nullptr); }
    const nfpe::ProgramData& loadedProgram() const { return loaded; }
    void markLoaded() { loaded = captureProgram(); }
    // selecting a different effect loads that effect's default knob positions (like the hardware)
    void selectEffect(int p, int fxIndex);
    void setParam(const juce::String& id, float plainValue);
    float getParam(const juce::String& id) const { return apvts.getRawParameterValue(id)->load(); }
    void triggerAudition() { auditionRequest.store(1); }
    int auditionMode() const { return (int) apvts.state.getProperty("auditionMode", 0); }   // 0 = all samples in turn, 1..5 = one sample

    // meters / indicators read by the editor
    std::atomic<float> peakL { 0.0f }, peakR { 0.0f };

private:
    // cached parameter pointers: processBlock never builds strings or looks parameters up by name
    struct RawParams
    {
        std::atomic<float> *routing, *bypassMode, *hostSync, *input, *outMode;
        std::atomic<float> *fx[2], *knob[2][3], *mix[2], *bypass[2], *bpm[2];
    } raw {};
    nfpe::Engine engine;
    nfpe::Audition audition;
    nfpe::ProgramData loaded;
    std::atomic<int> auditionRequest { 0 };
    int auditionNext = 0;
    std::vector<float> monoScratch;
    juce::SmoothedValue<float> inputGain;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NFPEAudioProcessor)
};
