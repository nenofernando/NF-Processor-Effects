#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace nfpe;

namespace
{
constexpr int kChunk = 2048;
}

juce::AudioProcessorValueTreeState::ParameterLayout NFPEAudioProcessor::createParameters()
{
    using ID = juce::ParameterID;
    using F = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    const ProgramData d = factoryProgram(0, false);

    F inRange(-60.0f, 12.0f, 0.1f); inRange.setSkewForCentre(0.0f);
    p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{ "input", 1 }, "Input", inRange, 0.0f, juce::AudioParameterFloatAttributes().withLabel("dB")));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ID{ "routing", 1 }, "Routing", juce::StringArray{ "1 Dual Mono", "2 Cascade", "3 Dual Stereo", "4 Mono Split" }, d.routing));
    for (int q = 0; q < 2; ++q)
    {
        const juce::String n = "P" + juce::String(q + 1) + " ";
        p.push_back(std::make_unique<juce::AudioParameterInt>(ID{ nfpe_ids::fx(q), 1 }, n + "Effect", 0, kNumEffects - 1, d.fx[q]));
        for (int i = 0; i < 3; ++i)
            p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{ nfpe_ids::knob(q, i), 1 }, n + "Knob " + juce::String(i + 1), F(0.0f, 1.0f, 0.0f), d.knob[q][i]));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{ nfpe_ids::mix(q), 1 }, "Mix " + juce::String(q + 1), F(0.0f, 1.0f, 0.0f), d.mix[q]));
        p.push_back(std::make_unique<juce::AudioParameterBool>(ID{ nfpe_ids::bypass(q), 1 }, n + "Bypass", d.bypass[q]));
        p.push_back(std::make_unique<juce::AudioParameterFloat>(ID{ nfpe_ids::bpm(q), 1 }, n + "Tempo", F(30.0f, 300.0f, 0.1f), 120.0f, juce::AudioParameterFloatAttributes().withLabel("BPM")));
    }
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ID{ "outMode", 1 }, "Output Mode", juce::StringArray{ "Stereo", "Mono" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ID{ "bypassMode", 1 }, "Bypass Mode", juce::StringArray{ "Pass dry", "Mute" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(ID{ "bank", 1 }, "Factory Bank", juce::StringArray{ "Serial", "Parallel" }, 0));
    p.push_back(std::make_unique<juce::AudioParameterBool>(ID{ "hostSync", 1 }, "Host Tempo Sync", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>(ID{ "autoLoad", 1 }, "Program AutoLoad", true));
    return { p.begin(), p.end() };
}

NFPEAudioProcessor::NFPEAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "NF_PROCESSOR_EFFECTS_STATE", createParameters())
{
    raw.routing = apvts.getRawParameterValue("routing"); raw.bypassMode = apvts.getRawParameterValue("bypassMode");
    raw.hostSync = apvts.getRawParameterValue("hostSync"); raw.input = apvts.getRawParameterValue("input"); raw.outMode = apvts.getRawParameterValue("outMode");
    for (int q = 0; q < 2; ++q)
    {
        raw.fx[q] = apvts.getRawParameterValue(nfpe_ids::fx(q)); raw.mix[q] = apvts.getRawParameterValue(nfpe_ids::mix(q));
        raw.bypass[q] = apvts.getRawParameterValue(nfpe_ids::bypass(q)); raw.bpm[q] = apvts.getRawParameterValue(nfpe_ids::bpm(q));
        for (int i = 0; i < 3; ++i) raw.knob[q][i] = apvts.getRawParameterValue(nfpe_ids::knob(q, i));
    }
    apvts.state.setProperty("program", 0, nullptr);
    apvts.state.setProperty("presetName", juce::String(factoryTitle(0)), nullptr);
    markLoaded();
}

void NFPEAudioProcessor::prepareToPlay(double sr, int)
{
    engine.prepare(sr, kChunk);
    audition.prepare(sr);
    monoScratch.assign((size_t) kChunk * 8, 0.0f);
    inputGain.reset(sr, 0.03);
    inputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(raw.input->load(), -59.9f));
}

bool NFPEAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && in == out;
}

void NFPEAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
#ifdef NF_LICENSE_ENFORCE   // off while the plug-in is in development (the licence is added at the end, with Paulo)
    struct LicenseMuteGuard { NFLicenseManager& lm; juce::AudioBuffer<float>& b; ~LicenseMuteGuard() { if (!lm.isActivated()) b.clear(); } } licenseGuard{ licenseManager, buffer };
#endif
    const int numCh = std::min(2, buffer.getNumChannels());
    if (numCh <= 0) return;
    const int total = buffer.getNumSamples();
    float* l = buffer.getWritePointer(0);
    const bool mono = numCh == 1;
    if ((int) monoScratch.size() < total) monoScratch.assign((size_t) total, 0.0f);
    float* r = mono ? monoScratch.data() : buffer.getWritePointer(1);
    if (mono) std::copy(l, l + total, r);

    // ---- parameters -> engine ----
    EngineParams e;
    e.routing = juce::roundToInt(raw.routing->load());
    e.bypassMute = raw.bypassMode->load() >= 0.5f;
    double hostBpm = 0.0;
    if (raw.hostSync->load() >= 0.5f)
        if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) hostBpm = *b;
    for (int p = 0; p < 2; ++p)
    {
        e.fx[p] = juce::jlimit(0, kNumEffects - 1, juce::roundToInt(raw.fx[p]->load()));
        for (int i = 0; i < 3; ++i) e.knob[p][i] = raw.knob[p][i]->load();
        e.mix[p] = raw.mix[p]->load();
        e.bypass[p] = raw.bypass[p]->load() >= 0.5f;
        const double bpm = hostBpm > 1.0 ? hostBpm : (double) raw.bpm[p]->load();
        e.quarterMs[p] = 60000.0 / juce::jlimit(20.0, 400.0, bpm);
    }
    inputGain.setTargetValue(juce::Decibels::decibelsToGain(raw.input->load(), -59.9f));
    const bool monoOut = raw.outMode->load() >= 0.5f;

    if (auditionRequest.exchange(0) != 0)
    {
        const int m = auditionMode();
        int idx = m == 0 ? auditionNext : m - 1;
        if (m == 0) auditionNext = (auditionNext + 1) % Audition::kNumSamples;
        audition.trigger(idx);
    }

    float pkL = 0.0f, pkR = 0.0f;
    for (int start = 0; start < total; start += kChunk)
    {
        const int n = std::min(kChunk, total - start);
        float* cl = l + start; float* cr = r + start;
        for (int i = 0; i < n; ++i) { const float g = inputGain.getNextValue(); cl[i] *= g; cr[i] *= g; }
        audition.addTo(cl, cr, n);
        for (int i = 0; i < n; ++i) { pkL = std::max(pkL, std::fabs(cl[i])); pkR = std::max(pkR, std::fabs(cr[i])); }
        engine.process(cl, cr, n, e);
        if (monoOut) for (int i = 0; i < n; ++i) cl[i] = cr[i] = 0.5f * (cl[i] + cr[i]);
    }
    if (mono) for (int i = 0; i < total; ++i) l[i] = 0.5f * (l[i] + r[i]);
    peakL.store(std::max(pkL, peakL.load() * 0.85f)); peakR.store(std::max(pkR, peakR.load() * 0.85f));
}

// ------------------------------------------------------------------ parameters / programs
void NFPEAudioProcessor::setParam(const juce::String& id, float plainValue)
{
    if (auto* p = apvts.getParameter(id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(plainValue));
        p->endChangeGesture();
    }
}

ProgramData NFPEAudioProcessor::captureProgram() const
{
    ProgramData d;
    d.routing = juce::roundToInt(getParam("routing"));
    for (int p = 0; p < 2; ++p)
    {
        d.fx[p] = juce::roundToInt(getParam(nfpe_ids::fx(p)));
        for (int i = 0; i < 3; ++i) d.knob[p][i] = getParam(nfpe_ids::knob(p, i));
        d.mix[p] = getParam(nfpe_ids::mix(p));
        d.bypass[p] = getParam(nfpe_ids::bypass(p)) >= 0.5f;
        d.bpm[p] = getParam(nfpe_ids::bpm(p));
    }
    return d;
}

void NFPEAudioProcessor::applyProgram(const ProgramData& d)
{
    setParam("routing", (float) d.routing);
    for (int p = 0; p < 2; ++p)
    {
        setParam(nfpe_ids::fx(p), (float) d.fx[p]);
        for (int i = 0; i < 3; ++i) setParam(nfpe_ids::knob(p, i), d.knob[p][i]);
        setParam(nfpe_ids::mix(p), d.mix[p]);
        setParam(nfpe_ids::bypass(p), d.bypass[p] ? 1.0f : 0.0f);
        setParam(nfpe_ids::bpm(p), d.bpm[p]);
    }
    loaded = d;
}

void NFPEAudioProcessor::selectEffect(int p, int fxIndex)
{
    fxIndex = (fxIndex % kNumEffects + kNumEffects) % kNumEffects;
    setParam(nfpe_ids::fx(p), (float) fxIndex);
    for (int i = 0; i < 3; ++i) setParam(nfpe_ids::knob(p, i), effectInfo(fxIndex).def[i]);
    setParam(nfpe_ids::mix(p), defaultMix(fxIndex, getParam("bank") >= 0.5f));
}

juce::File NFPEAudioProcessor::userProgramsDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("NF Audio Tools").getChildFile("NF Processor Effects").getChildFile("User Programs");
    dir.createDirectory();
    return dir;
}

static juce::File userFile(int userSlot) { return NFPEAudioProcessor::userProgramsDirectory().getChildFile(juce::String(userSlot + 1).paddedLeft('0', 2) + ".nfpeprog"); }

bool NFPEAudioProcessor::userProgramExists(int userSlot) const { return userFile(userSlot).existsAsFile(); }

juce::String NFPEAudioProcessor::programTitle(int slot) const
{
    if (slot < kNumPrograms) return juce::String(factoryTitle(slot));
    const int u = slot - kNumPrograms;
    if (auto xml = juce::XmlDocument::parse(userFile(u))) return xml->getStringAttribute("name", "User " + juce::String(u + 1));
    return "User " + juce::String(u + 1) + " (empty)";
}

bool NFPEAudioProcessor::storeUserProgram(int userSlot, const juce::String& name)
{
    const ProgramData d = captureProgram();
    juce::XmlElement xml("NFPEProgram");
    xml.setAttribute("name", name);
    xml.setAttribute("routing", d.routing);
    for (int p = 0; p < 2; ++p)
    {
        const juce::String s = juce::String(p + 1);
        xml.setAttribute("fx" + s, d.fx[p]);
        for (int i = 0; i < 3; ++i) xml.setAttribute("k" + s + juce::String(i + 1), (double) d.knob[p][i]);
        xml.setAttribute("mix" + s, (double) d.mix[p]);
        xml.setAttribute("bypass" + s, d.bypass[p]);
        xml.setAttribute("bpm" + s, (double) d.bpm[p]);
    }
    juce::TemporaryFile temp(userFile(userSlot));
    if (!xml.writeTo(temp.getFile()) || !temp.overwriteTargetFileWithTemporary()) return false;
    apvts.state.setProperty("program", kNumPrograms + userSlot, nullptr);
    apvts.state.setProperty("presetName", name, nullptr);
    loaded = d;
    return true;
}

void NFPEAudioProcessor::loadProgram(int slot)
{
    slot = juce::jlimit(0, 2 * kNumPrograms - 1, slot);
    if (slot < kNumPrograms)
    {
        applyProgram(factoryProgram(slot, getParam("bank") >= 0.5f));
        apvts.state.setProperty("presetName", juce::String(factoryTitle(slot)), nullptr);
    }
    else
    {
        auto xml = juce::XmlDocument::parse(userFile(slot - kNumPrograms));
        if (xml == nullptr || !xml->hasTagName("NFPEProgram")) return;      // empty user location: nothing to load
        ProgramData d;
        d.routing = juce::jlimit(0, 3, xml->getIntAttribute("routing", 1));
        for (int p = 0; p < 2; ++p)
        {
            const juce::String s = juce::String(p + 1);
            d.fx[p] = juce::jlimit(0, kNumEffects - 1, xml->getIntAttribute("fx" + s, 0));
            for (int i = 0; i < 3; ++i) d.knob[p][i] = juce::jlimit(0.0f, 1.0f, (float) xml->getDoubleAttribute("k" + s + juce::String(i + 1), 0.5));
            d.mix[p] = juce::jlimit(0.0f, 1.0f, (float) xml->getDoubleAttribute("mix" + s, 0.3));
            d.bypass[p] = xml->getBoolAttribute("bypass" + s, false);
            d.bpm[p] = juce::jlimit(30.0f, 300.0f, (float) xml->getDoubleAttribute("bpm" + s, 120.0));
        }
        applyProgram(d);
        apvts.state.setProperty("presetName", xml->getStringAttribute("name"), nullptr);
    }
    apvts.state.setProperty("program", slot, nullptr);
}

void NFPEAudioProcessor::getStateInformation(juce::MemoryBlock& dest) { if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, dest); }
void NFPEAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        markLoaded();
    }
}
juce::AudioProcessorEditor* NFPEAudioProcessor::createEditor() { return new NFPEAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NFPEAudioProcessor(); }
