// Developer tool: renders the editor to a PNG. Usage: NFPESnapshot out.png [width] [program 1..99]
#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::File out(argc > 1 ? argv[1] : "snapshot.png");
    const int w = argc > 2 ? juce::String(argv[2]).getIntValue() : 1200;
    NFPEAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    if (argc > 3) proc.loadProgram(juce::String(argv[3]).getIntValue() - 1);
    {   // audio smoke test through processBlock (stereo and mono layouts, every factory program)
        juce::Random rnd(7); float worst = 0.0f; bool finite = true;
        for (int prog = 0; prog < 99; ++prog)
        {
            proc.loadProgram(prog);
            for (int ch = 1; ch <= 2; ++ch)
            {
                juce::AudioBuffer<float> b(ch, 512); juce::MidiBuffer midi;
                for (int blk = 0; blk < 40; ++blk)
                {
                    for (int c = 0; c < ch; ++c) for (int i = 0; i < 512; ++i) b.setSample(c, i, blk < 5 ? rnd.nextFloat() * 0.5f - 0.25f : 0.0f);
                    if (blk == 0) proc.triggerAudition();
                    proc.processBlock(b, midi);
                    for (int c = 0; c < ch; ++c) for (int i = 0; i < 512; ++i) { const float v = b.getSample(c, i); if (!std::isfinite(v)) finite = false; worst = std::max(worst, std::fabs(v)); }
                }
            }
        }
        std::printf("processBlock smoke test: finite=%d peak=%.2f\n", (int) finite, worst);
    }
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
    ed->setSize(w, w / 3);
    juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
    auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
    juce::PNGImageFormat png;
    out.deleteFile();
    juce::FileOutputStream os(out);
    png.writeImageToStream(img, os);
    return 0;
}
