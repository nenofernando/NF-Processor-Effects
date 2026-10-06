#include "PluginEditor.h"
#include "NFPEBinaryData.h"
#include "License/NFUpdateCheck.h"
#ifndef JucePlugin_VersionString
 #define JucePlugin_VersionString "0.0.0-test"
#endif

using namespace nfpe;

namespace
{
constexpr float kBaseW = 1200.0f, kBaseH = 400.0f;
constexpr int kDefaultWidth = 990;                       // 990 x 330: a bit larger than the 810 x 270 family default (owner's request, easier to read)
const juce::Colour kBlue(0xff1d4f9f), kDarkBlue(0xff123a7a), kLine(0xff9fb8e6), kWhite(0xfff2f6ff), kGreen(0xff3fe06a), kRed(0xffff4a3d), kCyan(0xff5fe0ff);
constexpr float kProcX[2] = { 38.0f, 608.0f };   // inset so the frames never touch the bottom screws

juce::Font fontPx(float px, bool bold = true) { return juce::Font(juce::FontOptions(px, bold ? juce::Font::bold : juce::Font::plain)); }

void text(juce::Graphics& g, const juce::String& t, float x, float y, float size, juce::Colour c, juce::Justification j = juce::Justification::centredLeft, float w = 300.0f, bool bold = true, float kern = 0.0f)
{
    g.setColour(c);
    g.setFont(fontPx(size, bold).withExtraKerningFactor(kern));
    juce::Rectangle<float> r(x, y - size * 1.35f, w, size * 2.0f);   // y is the text baseline
    if (j.testFlags(juce::Justification::horizontallyCentred)) r.setX(x - w * 0.5f);
    else if (j.testFlags(juce::Justification::right)) r.setX(x - w);
    g.drawFittedText(t, r.toNearestInt(), j, 1, 0.85f);
}

void drawKnobBody(juce::Graphics& g, juce::Point<float> c, float r, float pos)
{
    for (int i = 0; i <= 10; ++i)
    {
        const float a = juce::degreesToRadians(-135.0f + 27.0f * (float) i);
        const juce::Point<float> d(std::sin(a), -std::cos(a));
        g.setColour(kWhite);
        g.drawLine(juce::Line<float>(c + d * (r + 7.0f * r / 24.0f), c + d * (r + 13.0f * r / 24.0f)), (i % 5 == 0 ? 2.4f : 1.4f) * r / 24.0f);
    }
    g.setColour(juce::Colour(0xff05070b)); g.fillEllipse(c.x - r - 2, c.y - r - 2, 2 * r + 4, 2 * r + 4);
    g.setColour(juce::Colour(0xff1b1e24)); g.fillEllipse(c.x - r, c.y - r, 2 * r, 2 * r);
    g.setColour(juce::Colour(0xff3a3f49)); g.drawEllipse(c.x - r, c.y - r, 2 * r, 2 * r, 2.0f * r / 24.0f);
    const float r2 = r * 0.72f;
    g.setColour(juce::Colour(0xff262a32)); g.fillEllipse(c.x - r2, c.y - r2, 2 * r2, 2 * r2);
    g.setColour(juce::Colour(0x22ffffff)); g.fillEllipse(c.x - r2 * 0.8f, c.y - r2 * 0.95f, r2 * 1.2f, r2 * 0.5f);   // soft top light
    const float a = juce::degreesToRadians(-135.0f + 270.0f * pos);
    const juce::Point<float> d(std::sin(a), -std::cos(a));
    g.setColour(juce::Colours::white);
    g.drawLine(juce::Line<float>(c + d * r * 0.25f, c + d * r * 0.92f), 3.6f * r / 24.0f);
}
}

// ---------------------------------------------------------------------------------------------------------------
NFPELookAndFeel::NFPELookAndFeel()
{
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff0b1a30));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffd6e4ff));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff2b6cd8));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(0xff0b1a30));
    setColour(juce::TooltipWindow::textColourId, juce::Colour(0xffd6e4ff));
}

void NFPELookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider&)
{
    const auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h);
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f / 1.6f;
    drawKnobBody(g, b.getCentre(), r, pos);
}

void NFPELookAndFeel::drawScrew(juce::Graphics& g, float x, float y, float rot)
{
    g.setColour(juce::Colour(0x55000000)); g.fillEllipse(x - 11.5f, y - 9.5f, 26.0f, 26.0f);
    g.setColour(juce::Colour(0xff3b4148)); g.fillEllipse(x - 10.5f, y - 10.5f, 21.0f, 21.0f);
    juce::ColourGradient sg(juce::Colour(0xfff4f6f8), x - 4.0f, y - 5.0f, juce::Colour(0xff6b737c), x + 8.0f, y + 9.0f, true);
    sg.addColour(0.45, juce::Colour(0xffb9c0c8));
    g.setGradientFill(sg); g.fillEllipse(x - 9.2f, y - 9.2f, 18.4f, 18.4f);
    g.setColour(juce::Colour(0xff4a5159)); g.drawEllipse(x - 9.2f, y - 9.2f, 18.4f, 18.4f, 0.8f);
    juce::Path cross;
    cross.addRectangle(-1.9f, -6.6f, 3.8f, 13.2f); cross.addRectangle(-6.6f, -1.9f, 13.2f, 3.8f);
    g.setColour(juce::Colour(0xf21d2126)); g.fillPath(cross, juce::AffineTransform::rotation(juce::degreesToRadians(rot)).translated(x, y));
    juce::Path inner;
    inner.addRectangle(-1.2f, -5.6f, 2.4f, 11.2f); inner.addRectangle(-5.6f, -1.2f, 11.2f, 2.4f);
    g.setColour(juce::Colour(0xcc0b0d10)); g.fillPath(inner, juce::AffineTransform::rotation(juce::degreesToRadians(rot)).translated(x, y));
    g.setColour(juce::Colours::white.withAlpha(0.35f)); g.fillEllipse(x - 5.6f, y - 6.6f, 4.8f, 4.8f);
}

void NFPELookAndFeel::drawLed(juce::Graphics& g, float x, float y, juce::Colour c, bool on, float r)
{
    if (on) { g.setColour(c.withAlpha(0.28f)); g.fillEllipse(x - r * 1.7f, y - r * 1.7f, r * 3.4f, r * 3.4f); }
    g.setColour(on ? c : juce::Colour(0xff0b2250)); g.fillEllipse(x - r, y - r, 2 * r, 2 * r);
    if (on) { g.setColour(juce::Colours::white.withAlpha(0.55f)); g.fillEllipse(x - r * 0.45f, y - r * 0.6f, r * 0.7f, r * 0.55f); }
}

void HwButton::paintButton(juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    const float s = r.getHeight() / 30.0f;
    g.setColour(juce::Colour(0xff0d1118)); g.fillRoundedRectangle(r, 5.0f * s);
    g.setColour(juce::Colour(0xff556070)); g.drawRoundedRectangle(r, 5.0f * s, 1.5f * s);
    auto face = r.reduced(3.0f * s);
    const bool on = getToggleState() || down;
    juce::Colour fc = on ? juce::Colour(0xff4a5568) : (over ? juce::Colour(0xff3a4250) : juce::Colour(0xff2a303b));
    g.setGradientFill(juce::ColourGradient(fc.brighter(on ? 0.0f : 0.15f), 0.0f, face.getY(), fc.darker(0.25f), 0.0f, face.getBottom(), false));
    g.fillRoundedRectangle(face, 3.0f * s);
    g.setColour(dimmed ? juce::Colour(0xff7a8294) : juce::Colour(0xffdfe6f5));
    g.setFont(fontPx(face.getHeight() * 0.58f).withExtraKerningFactor(0.04f));
    g.drawFittedText(getButtonText(), face.toNearestInt(), juce::Justification::centred, 1, 0.8f);
}

void Rocker::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(1.0f);
    const float s = r.getWidth() / 46.0f;
    g.setColour(juce::Colour(0xff0d1118)); g.fillRoundedRectangle(r, 6.0f * s);
    g.setColour(juce::Colour(0xff556070)); g.drawRoundedRectangle(r, 6.0f * s, 1.5f * s);
    const float cx = r.getCentreX(), h = r.getHeight();
    auto tri = [&](bool up, bool lit)
    {
        juce::Path p; const float y0 = up ? r.getY() + h * 0.12f : r.getBottom() - h * 0.12f, dy = up ? h * 0.17f : -h * 0.17f;
        p.addTriangle(cx, y0, cx - 11.0f * s, y0 + dy, cx + 11.0f * s, y0 + dy);
        g.setColour(lit ? juce::Colour(0xff5fe0ff) : kWhite); g.fillPath(p);
    };
    tri(true, pressed && pressedUp); tri(false, pressed && !pressedUp);
}

void SevenSeg::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float s = r.getHeight() / 62.0f;
    g.setColour(juce::Colour(0xff050806)); g.fillRoundedRectangle(r, 6.0f * s);
    g.setColour(juce::Colour(0xff222233)); g.drawRoundedRectangle(r.reduced(1.0f), 6.0f * s, 2.0f * s);
    auto bitsFor = [](juce::juce_wchar ch) -> int
    {   // bit 0..6 = a b c d e f g
        switch (ch)
        {
            case '0': return 0b0111111; case '1': return 0b0000110; case '2': return 0b1011011; case '3': return 0b1001111; case '4': return 0b1100110;
            case '5': return 0b1101101; case '6': return 0b1111101; case '7': return 0b0000111; case '8': return 0b1111111; case '9': return 0b1101111;
            case 'A': return 0b1110111; case 'b': return 0b1111100; case 'C': return 0b0111001; case 'c': return 0b1011000; case 'd': return 0b1011110;
            case 'E': return 0b1111001; case 'F': return 0b1110001; case 'H': return 0b1110110; case 'h': return 0b1110100; case 'L': return 0b0111000;
            case 'n': return 0b1010100; case 'o': return 0b1011100; case 'P': case 'p': return 0b1110011; case 'r': return 0b1010000; case 'S': return 0b1101101;
            case 't': return 0b1111000; case 'U': return 0b0111110; case 'u': return 0b0011100; case '-': return 0b1000000; case 'I': return 0b0000110;
            default: return 0;
        }
    };
    const float cellW = r.getWidth() * 0.40f, cellH = r.getHeight() * 0.68f, t = cellH * 0.13f;
    const float x0 = r.getCentreX() - cellW - 2.0f * s, y0 = r.getCentreY() - cellH * 0.5f - 1.0f * s;
    for (int d = 0; d < 2; ++d)
    {
        const juce::juce_wchar ch = d < text.length() ? text[d] : ' ';
        const int bits = bitsFor(ch);
        const float x = x0 + (float) d * (cellW + 4.0f * s);
        const juce::Rectangle<float> seg[7] = {
            { x + t, y0, cellW - 2 * t, t }, { x + cellW - t, y0 + t, t, cellH * 0.5f - t * 1.5f }, { x + cellW - t, y0 + cellH * 0.5f + t * 0.5f, t, cellH * 0.5f - t * 1.5f },
            { x + t, y0 + cellH - t, cellW - 2 * t, t }, { x, y0 + cellH * 0.5f + t * 0.5f, t, cellH * 0.5f - t * 1.5f }, { x, y0 + t, t, cellH * 0.5f - t * 1.5f },
            { x + t, y0 + cellH * 0.5f - t * 0.5f, cellW - 2 * t, t } };
        for (int k = 0; k < 7; ++k)
        {
            const bool lit = (bits >> k) & 1;
            g.setColour(lit ? kGreen : juce::Colour(0xff06150b));
            g.fillRoundedRectangle(seg[k], t * 0.4f);
            if (lit) { g.setColour(kGreen.withAlpha(0.22f)); g.fillRoundedRectangle(seg[k].expanded(t * 0.5f), t); }
        }
    }
    g.setColour(dot ? kGreen : juce::Colour(0xff06150b));
    g.fillEllipse(r.getRight() - 8.0f * s, r.getBottom() - 10.0f * s, 4.5f * s, 4.5f * s);
}

void ProgramKnob::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f / 1.6f;
    drawKnobBody(g, b.getCentre(), r, angle);
}

void PresetBar::paint(juce::Graphics& g)
{
    const float s = (float) getHeight() / 29.0f;
    auto r = getLocalBounds().toFloat().reduced(1.0f * s);
    g.setColour(juce::Colour(0x50000000)); g.fillRoundedRectangle(r.translated(0.0f, 2.0f * s), 8.0f * s);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff0b1a30), 0.0f, r.getY(), juce::Colour(0xff050d1a), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle(r, 8.0f * s);
    g.setColour(juce::Colour(0xff0a0f14)); g.drawRoundedRectangle(r, 8.0f * s, 1.6f * s);
    g.setColour(juce::Colour(0xff2fd4ff).withAlpha(0.35f)); g.drawRoundedRectangle(r.reduced(1.8f * s), 6.5f * s, 0.9f * s);
    g.setColour(kCyan);
    const float cy = r.getCentreY(), ax = 13.0f * s, w = 5.0f * s, h = 6.0f * s;
    juce::Path left, right;
    left.addTriangle(r.getX() + ax + w, cy - h, r.getX() + ax + w, cy + h, r.getX() + ax - w, cy);
    right.addTriangle(r.getRight() - ax - w, cy - h, r.getRight() - ax - w, cy + h, r.getRight() - ax + w, cy);
    g.fillPath(left); g.fillPath(right);
    g.setFont(fontPx(15.0f * s));
    g.drawFittedText(name, juce::Rectangle<float>(r.getX() + 28.0f * s, r.getY(), r.getWidth() - 56.0f * s, r.getHeight()).toNearestInt(), juce::Justification::centred, 1);
}

// ---------------------------------------------------------------------------------------------------------------
namespace
{
int stepsFor(const KnobDef& d)
{
    switch (d.kind)
    {
        case kMs: return (int) d.b;           case kDelayTime: return 72;       case kFeedback: return 100;
        case kDbNeg: return 70;               case kRatio: return 99;           case kDeEssFreq: return 40;
        case kDeEssBw: return 5;              case kUpDown: return 1;           case kCents: return 48;
        case kGainDb: return 12;              default: return (int) (d.b - d.a);
    }
}
}

NFPEAudioProcessorEditor::NFPEAudioProcessorEditor(NFPEAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
#ifdef NF_LICENSE_ENFORCE
      , licenseOverlay(p.licenseManager)
#endif
{
    setLookAndFeel(&look);
    logoImage = juce::ImageCache::getFromMemory(NFPEBinaryData::logo_nf_audio_tools_official_png, NFPEBinaryData::logo_nf_audio_tools_official_pngSize).convertedToFormat(juce::Image::ARGB).createCopy();
    {   // same look as the printed lettering of the panel: the grey letters become crisp white, the waveform stays vivid blue
        juce::Image::BitmapData bd(logoImage, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < bd.height; ++y)
            for (int x = 0; x < bd.width; ++x)
            {
                auto* px = reinterpret_cast<juce::PixelARGB*>(bd.getPixelPointer(x, y));
                juce::PixelARGB c = *px; c.unpremultiply();
                const float a = c.getAlpha() / 255.0f;
                const juce::Colour col(c.getRed(), c.getGreen(), c.getBlue());
                const float alpha = juce::jmin(1.0f, a * 1.25f);
                juce::Colour out = col.getSaturation() > 0.35f ? juce::Colour::fromHSV(col.getHue(), 0.75f, 1.0f, alpha) : juce::Colours::white.withAlpha(alpha);
                *px = out.getPixelARGB();   // already premultiplied
            }
    }
    logoGlow = juce::Image();
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(3.0);
    getConstrainer()->setSizeLimits(405, 135, 1800, 600);
    setSize(kDefaultWidth, kDefaultWidth / 3);   // always opens at 990 x 330; a click on the NF logo brings it back after resizing

    addAndMakeVisible(logoButton);
    logoButton.onClick = [this] { setSize(kDefaultWidth, kDefaultWidth / 3); };
    addAndMakeVisible(menuButton); menuButton.onClick = [this] { showMainMenu(); };
    addAndMakeVisible(presetBar);
    presetBar.onPrev = [this] { stepPreset(-1); };
    presetBar.onNext = [this] { stepPreset(+1); };
    presetBar.onMenu = [this] { showProgramMenu(); };
    presetBar.setPresetName(processor.currentProgramName());

    auto setupKnob = [this](juce::Slider& s, double def)
    {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setDoubleClickReturnValue(true, def);
    };
    auto& a = processor.apvts;
    setupKnob(inputKnob, 0.0);
    inputA = std::make_unique<SA>(a, "input", inputKnob);
    inputKnob.onValueChange = [this] { showValue(juce::roundToInt(inputKnob.getValue())); };
    for (int q = 0; q < 2; ++q)
    {
        setupKnob(mixKnob[q], 0.3);
        mixA[q] = std::make_unique<SA>(a, nfpe_ids::mix(q), mixKnob[q]);
        mixKnob[q].onValueChange = [this, q] { showValue(juce::roundToInt(mixKnob[q].getValue() * 99.0)); };
        for (int i = 0; i < 3; ++i)
        {
            setupKnob(procKnob[q][i], 0.5);
            procA[q][i] = std::make_unique<SA>(a, nfpe_ids::knob(q, i), procKnob[q][i]);
            procKnob[q][i].onValueChange = [this, q, i]
            {
                const int fx = juce::roundToInt(processor.getParam(nfpe_ids::fx(q)));
                showValue(knobDisplayInt(fx, i, (float) procKnob[q][i].getValue()));
            };
        }
        addAndMakeVisible(rocker[q]);
        rocker[q].onStep = [this, q](int d) { processor.selectEffect(q, juce::roundToInt(processor.getParam(nfpe_ids::fx(q))) + d); storeMode = false; };
        addAndMakeVisible(bypassBtn[q]); bypassBtn[q].setClickingTogglesState(true);
        bypassA[q] = std::make_unique<BA>(a, nfpe_ids::bypass(q), bypassBtn[q]);
        addAndMakeVisible(tempoBtn[q]);
        tempoBtn[q].onClick = [this, q] { tapTempo(q); };
    }
    addAndMakeVisible(routingBtn);
    routingBtn.onClick = [this]
    {
        storeMode = false;
        const int r = (juce::roundToInt(processor.getParam("routing")) + 1) % 4;
        processor.setParam("routing", (float) r);
        showValueText("r" + juce::String(r + 1));
    };
    addAndMakeVisible(storeBtn); storeBtn.onClick = [this] { onStorePressed(); };
    addAndMakeVisible(auditionBtn); auditionBtn.onClick = [this] { onAuditionPressed(); };
    addAndMakeVisible(seg);
    addAndMakeVisible(programKnob);
    programKnob.onStep = [this](int d) { stepProgram(d); };
    programKnob.onPush = [this]
    {
        if (storeMode) return;
        processor.loadProgram(pendingSlot);
    };
    pendingSlot = processor.currentProgramSlot();
    for (int q = 0; q < 2; ++q) applyKnobSteps(q);

#ifdef NF_LICENSE_ENFORCE
    addChildComponent(licenseOverlay);
    licenseOverlay.setVisible(!processor.licenseManager.isActivated());
    licenseOverlay.onActivated = [this] { licenseOverlay.setVisible(false); };
    licenseOverlay.setLookAndFeel(&juce::LookAndFeel::getDefaultLookAndFeel());
#endif
    startTimerHz(30);
}

NFPEAudioProcessorEditor::~NFPEAudioProcessorEditor() { stopTimer(); setLookAndFeel(nullptr); }

juce::Rectangle<int> NFPEAudioProcessorEditor::scaleBounds(juce::Rectangle<float> b) const
{
    return { juce::roundToInt(offsetX + b.getX() * layoutScale), juce::roundToInt(offsetY + b.getY() * layoutScale),
             juce::roundToInt(b.getWidth() * layoutScale), juce::roundToInt(b.getHeight() * layoutScale) };
}

void NFPEAudioProcessorEditor::resized()
{
    layoutScale = juce::jmin(getWidth() / kBaseW, getHeight() / kBaseH);
    offsetX = (getWidth() - kBaseW * layoutScale) * 0.5f;
    offsetY = (getHeight() - kBaseH * layoutScale) * 0.5f;
    auto knobBox = [&](juce::Slider& s, float cx, float cy, float r) { const float b = r * 3.2f; s.setBounds(scaleBounds({ cx - b * 0.5f, cy - b * 0.5f, b, b })); };
    knobBox(inputKnob, 122, 158, 28);
    knobBox(mixKnob[0], 204, 158, 22); knobBox(mixKnob[1], 278, 158, 22);
    routingBtn.setBounds(scaleBounds({ 238, 92, 86, 32 }));
    storeBtn.setBounds(scaleBounds({ 912, 84, 72, 30 })); auditionBtn.setBounds(scaleBounds({ 994, 84, 84, 30 }));
    seg.setBounds(scaleBounds({ 918, 128, 92, 62 }));
    { const float b = 34 * 3.2f; programKnob.setBounds(scaleBounds({ 1130 - b * 0.5f, 162 - b * 0.5f, b, b })); }
    for (int q = 0; q < 2; ++q)
    {
        const float x = kProcX[q];
        rocker[q].setBounds(scaleBounds({ x + 16, 278, 46, 92 }));
        tempoBtn[q].setBounds(scaleBounds({ x + 76, 316, 70, 28 }));
        bypassBtn[q].setBounds(scaleBounds({ x + 176, 316, 70, 28 }));
        for (int i = 0; i < 3; ++i) knobBox(procKnob[q][i], x + 300 + 90.0f * i, 341, 24);
    }
    logoButton.setBounds(scaleBounds({ 24, 1, 108, 60 }));
    menuButton.setBounds(scaleBounds({ 1120, 18, 30, 27 }));
    presetBar.setBounds(scaleBounds({ 858, 14, 233, 31 }));
#ifdef NF_LICENSE_ENFORCE
    licenseOverlay.setBounds(getLocalBounds());
#endif
}

void NFPEAudioProcessorEditor::applyKnobSteps(int p)
{
    const int fx = juce::roundToInt(processor.getParam(nfpe_ids::fx(p)));
    const auto& info = effectInfo(fx);
    for (int i = 0; i < 3; ++i)
    {
        const int steps = stepsFor(info.knob[i]);
        procKnob[p][i].setRange(0.0, 1.0, steps > 0 ? 1.0 / steps : 0.0);
        procKnob[p][i].setDoubleClickReturnValue(true, info.def[i]);
        procKnob[p][i].setTooltip(juce::String(info.knob[i].name));
    }
    tempoBtn[p].dimmed = !info.usesTempo;
    lastFx[p] = fx;
}

void NFPEAudioProcessorEditor::showValue(int v)
{
    // values of 100 or more show the last two digits with the dot lit (the manual's trick: 115 -> "15" + dot)
    holdText = juce::String(v >= 100 ? v - 100 : v).paddedLeft('0', 2);
    holdUntil = juce::Time::getMillisecondCounter() + 1400;
    holdDot = v >= 100;
}
void NFPEAudioProcessorEditor::showValueText(const juce::String& t) { holdText = t; holdDot = false; holdUntil = juce::Time::getMillisecondCounter() + 1400; }

void NFPEAudioProcessorEditor::stepProgram(int delta)
{
    const int total = 2 * NFPEAudioProcessor::kNumPrograms;
    if (storeMode) pendingSlot = NFPEAudioProcessor::kNumPrograms + ((pendingSlot - NFPEAudioProcessor::kNumPrograms + delta + 99) % 99 + 99) % 99;
    else pendingSlot = ((pendingSlot + delta) % total + total) % total;
    holdUntil = 0;
    if (!storeMode && processor.getParam("autoLoad") >= 0.5f) processor.loadProgram(pendingSlot);
}

void NFPEAudioProcessorEditor::onStorePressed()
{
    if (!storeMode)
    {
        storeMode = true;
        pendingSlot = std::max(NFPEAudioProcessor::kNumPrograms, processor.currentProgramSlot());
        return;
    }
    const int user = pendingSlot - NFPEAudioProcessor::kNumPrograms;
    juce::String name = processor.currentProgramName();
    if (processor.currentProgramSlot() != pendingSlot) name = name.isEmpty() ? "User " + juce::String(user + 1) : name;
    if (!processor.storeUserProgram(user, name))
        juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "NF Processor Effects", "Could not save the program.");
    storeMode = false;
    showValueText("St");
}

void NFPEAudioProcessorEditor::onAuditionPressed()
{
    if (storeMode) { storeMode = false; return; }
    if (juce::ModifierKeys::currentModifiers.isShiftDown() || juce::ModifierKeys::currentModifiers.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addSectionHeader("AUDITION SOUND");
        const char* names[6] = { "All samples in turn", "Drum stick click", "Snare drum", "Kick drum", "Female vocal \"Doo\"", "Acoustic guitar arpeggio" };
        for (int i = 0; i < 6; ++i) m.addItem(1 + i, names[i], true, processor.auditionMode() == i);
        juce::Component::SafePointer<NFPEAudioProcessorEditor> safe(this);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&auditionBtn), [safe](int r)
        { if (safe != nullptr && r > 0) safe->processor.apvts.state.setProperty("auditionMode", r - 1, nullptr); });
        return;
    }
    processor.triggerAudition();
}

void NFPEAudioProcessorEditor::tapTempo(int p)
{
    storeMode = false;
    if (!effectInfo(juce::roundToInt(processor.getParam(nfpe_ids::fx(p)))).usesTempo) return;
    const auto now = juce::Time::getMillisecondCounter();
    const auto dt = now - lastTap[p];
    lastTap[p] = now;
    if (dt > 200 && dt < 2000)
    {
        const float bpm = juce::jlimit(30.0f, 300.0f, 60000.0f / (float) dt);
        processor.setParam(nfpe_ids::bpm(p), bpm);
        showValue(juce::roundToInt(bpm));
    }
}

juce::String NFPEAudioProcessorEditor::programDisplayText(bool& dot, bool& blinkOff) const
{
    const bool phase = ((juce::Time::getMillisecondCounter() / 350) & 1) != 0;
    blinkOff = false; dot = false;
    const int N = NFPEAudioProcessor::kNumPrograms;
    if (storeMode)
    {
        if (phase) { dot = false; return "St"; }
        dot = true; return juce::String(pendingSlot - N + 1).paddedLeft('0', 2);
    }
    if (juce::Time::getMillisecondCounter() < holdUntil) { dot = holdDot; return holdText; }
    const int slot = pendingSlot != processor.currentProgramSlot() ? pendingSlot : processor.currentProgramSlot();
    dot = slot >= N;
    if (pendingSlot != processor.currentProgramSlot() && phase) blinkOff = true;
    return juce::String(slot % N + 1).paddedLeft('0', 2);
}

bool NFPEAudioProcessorEditor::anyEdits() const
{
    const auto c = processor.captureProgram(), &l = processor.loadedProgram();
    if (c.routing != l.routing) return true;
    for (int p = 0; p < 2; ++p)
    {
        if (c.fx[p] != l.fx[p] || c.bypass[p] != l.bypass[p] || std::fabs(c.mix[p] - l.mix[p]) > 0.002f || std::fabs(c.bpm[p] - l.bpm[p]) > 0.05f) return true;
        for (int i = 0; i < 3; ++i) if (std::fabs(c.knob[p][i] - l.knob[p][i]) > 0.002f) return true;
    }
    return false;
}

void NFPEAudioProcessorEditor::timerCallback()
{
    for (int q = 0; q < 2; ++q)
        if (juce::roundToInt(processor.getParam(nfpe_ids::fx(q))) != lastFx[q]) applyKnobSteps(q);
    programKnob.angle = (float) pendingSlot / (float) (2 * NFPEAudioProcessor::kNumPrograms - 1);
    presetBar.setPresetName(processor.currentProgramName());
    bool dot = false, blinkOff = false;
    juce::String t = programDisplayText(dot, blinkOff);
    seg.set(blinkOff ? juce::String() : t, blinkOff ? false : dot);

    // repaint the panel only when something visible changed
    juce::String key;
    key << processor.getParam("routing") << '|' << processor.getParam("bank") << '|' << (int) anyEdits();
    for (int q = 0; q < 2; ++q)
    {
        key << '|' << processor.getParam(nfpe_ids::fx(q)) << ',' << processor.getParam(nfpe_ids::bypass(q)) << ',' << processor.getParam(nfpe_ids::bpm(q));
        for (int i = 0; i < 3; ++i) key << ',' << juce::roundToInt(processor.getParam(nfpe_ids::knob(q, i)) * 1000.0f);
        const double qMs = 60000.0 / processor.getParam(nfpe_ids::bpm(q));
        key << ',' << (int) (std::fmod((double) juce::Time::getMillisecondCounter(), qMs) < qMs * 0.15);
    }
    auto lvl = [](float pk) { const float db = juce::Decibels::gainToDecibels(pk, -90.0f); return (db > -42.0f) + (db > -30.0f) + (db > -18.0f) + (db > -9.0f) + (db > -3.0f) + (db > -0.3f); };
    key << '|' << lvl(processor.peakL.load()) << lvl(processor.peakR.load()) << (int) storeMode;
    const auto h = (size_t) key.hashCode64();
    if (h != lastHash) { lastHash = h; repaint(); }
}

// ---------------------------------------------------------------------------------------------------------------
void NFPEAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0b0c0e));
    juce::Graphics::ScopedSaveState state(g);
    g.addTransform(juce::AffineTransform::scale(layoutScale).translated(offsetX, offsetY));

    // chassis
    g.setColour(kBlue); g.fillRoundedRectangle(0, 0, kBaseW, kBaseH, 12.0f);
    g.setColour(juce::Colour(0xff6f95d6)); g.fillRect(0.0f, 0.0f, kBaseW, 6.0f);
    g.setColour(kDarkBlue); g.fillRect(0.0f, kBaseH - 6.0f, kBaseW, 6.0f);
    NFPELookAndFeel::drawScrew(g, 18, 18, 25); NFPELookAndFeel::drawScrew(g, 1182, 18, -35);
    NFPELookAndFeel::drawScrew(g, 18, 382, 70); NFPELookAndFeel::drawScrew(g, 1182, 382, -10);

    // header
    g.setOpacity(1.0f);   // drawImage uses the current colour's alpha as opacity (the last screw highlight left it at 0.35)
    if (logoImage.isValid()) g.drawImage(logoImage, juce::Rectangle<float>(24, 1, 108, 60), juce::RectanglePlacement::centred);
    g.setColour(kWhite); g.drawLine(142, 16, 142, 46, 2.0f);
    text(g, "NF PROCESSOR EFFECTS", 156, 34, 26, kWhite, juce::Justification::centredLeft, 400, true, 0.08f);
    text(g, "DUAL REVERB / EFFECTS PROCESSOR", 156, 50, 13, juce::Colour(0xffcfe0ff), juce::Justification::centredLeft, 400, true, 0.12f);

    auto section = [&](float x, float y, float w, float h, const juce::String& title)
    {
        g.setColour(kLine); g.drawRoundedRectangle(x, y, w, h, 8.0f, 1.6f);
        juce::GlyphArrangement ga; ga.addLineOfText(fontPx(17).withExtraKerningFactor(0.1f), title, 0.0f, 0.0f);
        const float tw = ga.getBoundingBox(0, -1, true).getWidth() + 16.0f;
        g.setColour(kBlue); g.fillRect(x + 12, y - 11, tw, 22.0f);
        text(g, title, x + 20, y + 6, 17, kWhite, juce::Justification::centredLeft, tw + 20, true, 0.1f);
    };
    section(18, 66, 326, 168, "INPUT / ROUTING");
    section(358, 66, 520, 168, "REVERB");
    section(896, 66, 286, 168, "PROGRAM");
    text(g, "DELAY / EFFECTS", 640, 88, 17, kWhite, juce::Justification::centredLeft, 220, true, 0.1f);
    g.setColour(kLine); g.drawLine(630, 96, 630, 226, 1.0f);

    // input meters
    auto lvl = [](float pk) { const float db = juce::Decibels::gainToDecibels(pk, -90.0f); return (db > -42.0f) + (db > -30.0f) + (db > -18.0f) + (db > -9.0f) + (db > -3.0f) + (db > -0.3f); };
    const int lv[2] = { lvl(processor.peakL.load()), lvl(processor.peakR.load()) };
    for (int c = 0; c < 2; ++c)
        for (int r = 0; r < 6; ++r)
            NFPELookAndFeel::drawLed(g, 40.0f + c * 20.0f, 208.0f - r * 18.0f, r == 5 ? kRed : (r >= 3 ? juce::Colour(0xffffd23f) : kGreen), r < lv[c]);
    text(g, "INPUT", 122, 214, 15, kWhite, juce::Justification::centred, 100);
    text(g, "MIX 1", 204, 214, 15, kWhite, juce::Justification::centred, 100);
    text(g, "MIX 2", 278, 214, 15, kWhite, juce::Justification::centred, 100);

    // routing diagram
    {
        const int routing = juce::roundToInt(processor.getParam("routing"));
        g.setColour(juce::Colour(0xff0b3a8a)); g.fillRoundedRectangle(152, 88, 74, 36, 4.0f);
        g.setColour(kLine); g.drawRoundedRectangle(152, 88, 74, 36, 4.0f, 1.2f);
        g.setColour(kWhite);
        auto box = [&](float x, float y) { g.drawRect(x, y, 16.0f, 9.0f, 1.8f); };
        switch (routing)
        {
            case 0: box(170, 92); box(170, 108); g.drawLine(160, 96.5f, 170, 96.5f, 1.6f); g.drawLine(186, 96.5f, 196, 96.5f, 1.6f); g.drawLine(160, 112.5f, 170, 112.5f, 1.6f); g.drawLine(186, 112.5f, 196, 112.5f, 1.6f); break;
            case 1: box(164, 100); box(186, 100); g.drawLine(158, 104.5f, 164, 104.5f, 1.6f); g.drawLine(180, 104.5f, 186, 104.5f, 1.6f); g.drawLine(202, 104.5f, 208, 104.5f, 1.6f); break;
            case 2: box(176, 92); box(176, 108); g.drawLine(160, 96.5f, 176, 96.5f, 1.6f); g.drawLine(160, 112.5f, 176, 112.5f, 1.6f); g.drawLine(192, 96.5f, 204, 96.5f, 1.6f); g.drawLine(192, 112.5f, 204, 112.5f, 1.6f); g.drawLine(204, 96.5f, 204, 112.5f, 1.6f); break;
            default: box(176, 92); box(176, 108); g.drawLine(160, 96.5f, 176, 96.5f, 1.6f); g.drawLine(160, 112.5f, 176, 112.5f, 1.6f); g.drawLine(192, 96.5f, 204, 96.5f, 1.6f); g.drawLine(192, 112.5f, 204, 112.5f, 1.6f); g.drawLine(204, 96.5f, 204, 112.5f, 1.6f); g.drawLine(160, 96.5f, 160, 112.5f, 0.0f); break;
        }
        text(g, juce::String(routing + 1), 214, 112, 16, kWhite, juce::Justification::centredLeft, 14);
    }

    // effect matrix
    const int fxSel[2] = { juce::roundToInt(processor.getParam(nfpe_ids::fx(0))), juce::roundToInt(processor.getParam(nfpe_ids::fx(1))) };
    const float colX[4] = { 368, 490, 640, 758 };
    for (int idx = 0; idx < kNumEffects; ++idx)
    {
        const int col = idx / 8, row = idx % 8;
        const float x = colX[col], y = 102.0f + (float) row * 16.6f;
        const bool a = fxSel[0] == idx, b = fxSel[1] == idx;
        NFPELookAndFeel::drawLed(g, x + 6, y - 5, kGreen, a, 5.0f);
        NFPELookAndFeel::drawLed(g, x + 22, y - 5, kRed, b, 5.0f);
        text(g, effectInfo(idx).name, x + 34, y + 2, 15.0f, (a || b) ? juce::Colours::white : juce::Colour(0xffd6e4ff), juce::Justification::centredLeft, 118, a || b);
    }

    // program section
    text(g, anyEdits() ? "EDITED" : (processor.currentProgramSlot() >= NFPEAudioProcessor::kNumPrograms ? "USER PROGRAM" : "FACTORY PROGRAM"), 964, 212, 13, juce::Colour(0xffcfe0ff), juce::Justification::centred, 140);
    NFPELookAndFeel::drawLed(g, 1070, 100, kRed, anyEdits() && !storeMode, 4.5f);
    text(g, "PUSH TO LOAD", 1130, 224, 13, juce::Colour(0xffcfe0ff), juce::Justification::centred, 140);

    // processors
    for (int q = 0; q < 2; ++q)
    {
        const float x = kProcX[q];
        const int fx = fxSel[q];
        const auto& info = effectInfo(fx);
        const juce::Colour led = q == 0 ? kGreen : kRed;
        section(x, 262, 554, 126, "PROCESSOR " + juce::String(q + 1));
        text(g, "EFFECT", x + 39, 332, 11.5f, juce::Colour(0xffcfe0ff), juce::Justification::centred, 60);
        text(g, "SELECT", x + 39, 345, 11.5f, juce::Colour(0xffcfe0ff), juce::Justification::centred, 60);
        g.setColour(juce::Colour(0xff07101f)); g.fillRoundedRectangle(x + 76, 276, 150, 30, 6.0f);
        g.setColour(juce::Colour(0xff0a0f14)); g.drawRoundedRectangle(x + 76, 276, 150, 30, 6.0f, 2.0f);
        NFPELookAndFeel::drawLed(g, x + 90, 291, led, true);
        text(g, info.name, x + 104, 298, 19, q == 0 ? juce::Colour(0xff8cf0a4) : juce::Colour(0xffff9b92), juce::Justification::centredLeft, 118);
        const bool byp = processor.getParam(nfpe_ids::bypass(q)) >= 0.5f;
        NFPELookAndFeel::drawLed(g, x + 258, 330, kRed, byp, 5.5f);
        if (info.usesTempo)
        {
            const double qMs = 60000.0 / processor.getParam(nfpe_ids::bpm(q));
            NFPELookAndFeel::drawLed(g, x + 161, 330, led, std::fmod((double) juce::Time::getMillisecondCounter(), qMs) < qMs * 0.15 || processor.getParam("hostSync") >= 0.5f, 5.5f);
        }
        const auto saved = processor.loadedProgram();
        for (int i = 0; i < 3; ++i)
        {
            const float kx = x + 300.0f + 90.0f * (float) i;
            const float k = processor.getParam(nfpe_ids::knob(q, i));
            g.setColour(juce::Colour(0xff07101f)); g.fillRoundedRectangle(kx - 42, 276, 84, 22, 5.0f);
            g.setColour(juce::Colour(0xff0a0f14)); g.drawRoundedRectangle(kx - 42, 276, 84, 22, 5.0f, 1.5f);
            text(g, juce::String(knobText(fx, i, k)), kx, 293, 15.5f, kCyan, juce::Justification::centred, 80);
            text(g, info.knob[i].name, kx, 382, 14, kWhite, juce::Justification::centred, 90);
            const bool changed = saved.fx[q] != fx || std::fabs(saved.knob[q][i] - k) > 0.002f;
            NFPELookAndFeel::drawLed(g, kx - 34, 353, juce::Colour(0xffffb100), changed, 4.5f);
        }
    }
    text(g, "NF AUDIO TOOLS", 600, 397, 12, kWhite, juce::Justification::centred, 300, false);
    text(g, "V" JucePlugin_VersionString, 70, 397, 11, kWhite, juce::Justification::centredLeft, 80, false);
}

// ---------------------------------------------------------------------------------------------------------------
// The manuals ship inside the plug-in: they are written once to a per-user folder (rewritten if the size differs after an update) and opened in the default PDF viewer.
static void openManual(const char* data, int size, const juce::String& filename)
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("NF Audio Tools").getChildFile("NF Processor Effects").getChildFile("Manuals");
    dir.createDirectory();
    auto file = dir.getChildFile(filename);
    if (!file.existsAsFile() || file.getSize() != (juce::int64) size)
    {
        juce::TemporaryFile temp(file);
        if (temp.getFile().replaceWithData(data, (size_t) size)) temp.overwriteTargetFileWithTemporary();
    }
    if (file.existsAsFile()) file.startAsProcess();
}

void NFPEAudioProcessorEditor::showMainMenu()
{
    juce::PopupMenu m;
    auto choice = [&](int id, const juce::String& label, const juce::String& param, int value)
    { m.addItem(id, label, true, juce::roundToInt(processor.getParam(param)) == value); };
    m.addSectionHeader("MANUALS");
    m.addItem(10, "Manual - Portugues");
    m.addItem(11, "Manual - English");
    m.addSeparator();
    m.addSectionHeader("UTILITY");
    choice(1, "Output: Stereo", "outMode", 0); choice(2, "Output: Mono", "outMode", 1);
    choice(3, "Bypass: pass dry signal", "bypassMode", 0); choice(4, "Bypass: mute", "bypassMode", 1);
    choice(5, "Factory programs: Serial bank (in-line use)", "bank", 0); choice(6, "Factory programs: Parallel bank (send / return)", "bank", 1);
    m.addItem(7, "Sync delays to the host tempo", true, processor.getParam("hostSync") >= 0.5f);
    m.addItem(8, "Program AutoLoad", true, processor.getParam("autoLoad") >= 0.5f);
    m.addSeparator();
    m.addSectionHeader("AUDITION SOUND");
    const char* names[6] = { "All samples in turn", "Drum stick click", "Snare drum", "Kick drum", "Female vocal \"Doo\"", "Acoustic guitar arpeggio" };
    for (int i = 0; i < 6; ++i) m.addItem(20 + i, names[i], true, processor.auditionMode() == i);
    m.addSeparator();
    m.addItem(101, "Check for updates...");
    m.addItem(100, "About");
    juce::Component::SafePointer<NFPEAudioProcessorEditor> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&menuButton), [safe](int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& pr = safe->processor;
        switch (r)
        {
            case 1: pr.setParam("outMode", 0); break;        case 2: pr.setParam("outMode", 1); break;
            case 3: pr.setParam("bypassMode", 0); break;     case 4: pr.setParam("bypassMode", 1); break;
            case 5: pr.setParam("bank", 0); break;           case 6: pr.setParam("bank", 1); break;
            case 7: pr.setParam("hostSync", pr.getParam("hostSync") >= 0.5f ? 0.0f : 1.0f); break;
            case 8: pr.setParam("autoLoad", pr.getParam("autoLoad") >= 0.5f ? 0.0f : 1.0f); break;
            case 10: openManual(NFPEBinaryData::NF_ProcessorEffects_Manual_Portugues_pdf, NFPEBinaryData::NF_ProcessorEffects_Manual_Portugues_pdfSize, "NF_ProcessorEffects_Manual_Portugues_V" JucePlugin_VersionString ".pdf"); break;
            case 11: openManual(NFPEBinaryData::NF_ProcessorEffects_Manual_English_pdf, NFPEBinaryData::NF_ProcessorEffects_Manual_English_pdfSize, "NF_ProcessorEffects_Manual_English_V" JucePlugin_VersionString ".pdf"); break;
            case 100: safe->showAbout(); break;
            case 101: safe->checkForUpdates(); break;
            default: if (r >= 20 && r < 26) pr.apvts.state.setProperty("auditionMode", r - 20, nullptr); break;
        }
    });
}

void NFPEAudioProcessorEditor::showProgramMenu()
{
    juce::PopupMenu m;
    const int cur = processor.currentProgramSlot();
    for (int app = 0; app < 5; ++app)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < NFPEAudioProcessor::kNumPrograms; ++i)
            if (factoryRows()[i].app == app) sub.addItem(1000 + i, juce::String(i + 1).paddedLeft('0', 2) + "   " + juce::String(factoryTitle(i)), true, cur == i);
        m.addSubMenu(applicationName(app), sub);
    }
    juce::PopupMenu user;
    for (int u = 0; u < 99; ++u)
        if (processor.userProgramExists(u))
            user.addItem(1000 + NFPEAudioProcessor::kNumPrograms + u, juce::String(u + 1).paddedLeft('0', 2) + "   " + processor.programTitle(NFPEAudioProcessor::kNumPrograms + u), true, cur == NFPEAudioProcessor::kNumPrograms + u);
    if (user.getNumItems() == 0) user.addItem(1, "No user programs saved yet (use STORE)", false);
    m.addSubMenu("User programs", user);
    m.addSeparator();
    m.addItem(1, "Save Preset File...");
    m.addItem(2, "Load Preset File...");
    m.addItem(3, "Restore default");
    juce::Component::SafePointer<NFPEAudioProcessorEditor> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetBar).withMinimumWidth(presetBar.getWidth()), [safe](int r)
    {
        if (safe == nullptr || r == 0) return;
        if (r == 1) safe->savePresetFile();
        else if (r == 2) safe->loadPresetFile();
        else if (r == 3) { safe->processor.loadProgram(0); safe->pendingSlot = 0; }
        else if (r >= 1000) { safe->pendingSlot = r - 1000; safe->processor.loadProgram(r - 1000); }
    });
}

void NFPEAudioProcessorEditor::stepPreset(int direction)
{
    const int n = NFPEAudioProcessor::kNumPrograms;
    int slot = processor.currentProgramSlot();
    slot = slot >= n ? 0 : (slot + direction + n) % n;
    pendingSlot = slot;
    processor.loadProgram(slot);
}

void NFPEAudioProcessorEditor::showAbout()
{
    juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "About NF Processor Effects",
        juce::String("NF Processor Effects V") + JucePlugin_VersionString + "\nNF Audio Tools - Nenno Fernando\n\n16 reverbs, 6 delays, 8 modulation effects, 2 dynamics.\n99 factory programs and 99 user programs.");
}

void NFPEAudioProcessorEditor::savePresetFile()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("NF Audio Tools").getChildFile("NF Processor Effects").getChildFile("Presets");
    dir.createDirectory();
    chooser = std::make_unique<juce::FileChooser>("Save NF Processor Effects Preset", dir, "*.nfpepreset");
    juce::Component::SafePointer<NFPEAudioProcessorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto f = fc.getResult();
        if (f != juce::File{})
        {
            if (!f.hasFileExtension("nfpepreset")) f = f.withFileExtension("nfpepreset");
            safe->processor.apvts.state.setProperty("presetName", f.getFileNameWithoutExtension(), nullptr);
            if (auto xml = safe->processor.apvts.copyState().createXml())
            {
                xml->setAttribute("nfpePresetSignature", "NFPEPreset");
                xml->writeTo(f);
            }
        }
        safe->chooser.reset();
    });
}

void NFPEAudioProcessorEditor::loadPresetFile()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("NF Audio Tools").getChildFile("NF Processor Effects").getChildFile("Presets");
    chooser = std::make_unique<juce::FileChooser>("Load NF Processor Effects Preset", dir, "*.nfpepreset");
    juce::Component::SafePointer<NFPEAudioProcessorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto f = fc.getResult();
        if (f != juce::File{})
        {
            auto xml = juce::XmlDocument::parse(f);
            if (xml != nullptr && xml->getStringAttribute("nfpePresetSignature") == "NFPEPreset" && xml->hasTagName(safe->processor.apvts.state.getType()))
            {
                safe->processor.apvts.replaceState(juce::ValueTree::fromXml(*xml));
                safe->processor.markLoaded();
            }
            else juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "NF Processor Effects", "Invalid NF Processor Effects preset");
        }
        safe->chooser.reset();
    });
}

// Only runs when the user clicks the menu item: one request to the NF server, off the message thread.
// The callback runs after the network round trip, so the editor may be gone by then (SafePointer).
void NFPEAudioProcessorEditor::checkForUpdates()
{
    juce::Component::SafePointer<NFPEAudioProcessorEditor> safe(this);
    nfupdate::checkAsync("nf-processor-effects", JucePlugin_VersionString, [safe](nfupdate::Result r)
    {
        if (safe == nullptr) return;
        const juce::String title = "NF Processor Effects";

        if (!r.reachedServer)
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, title,
                "Could not check for updates. Please check your internet connection and try again.");
            return;
        }
        if (!r.updateAvailable)
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, title,
                juce::String("You have the latest version (V") + JucePlugin_VersionString + ").");
            return;
        }
        const auto options = juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::InfoIcon).withTitle(title)
            .withMessage("A new version is available: V" + r.latestVersion + " (you have V" + juce::String(JucePlugin_VersionString) + ").\n\nDownload it now?")
            .withButton("Download").withButton("Later");
        const auto url = r.downloadUrl;
        // AlertWindow result codes with two buttons: first = 1, second = 0.
        juce::AlertWindow::showAsync(options, [url](int button) { if (button == 1) juce::URL(url).launchInDefaultBrowser(); });
    });
}
