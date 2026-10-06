#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#ifdef NF_LICENSE_ENFORCE
 #include "License/LicenseActivationComponent.h"
#endif

// ---------------------------------------------------------------------------------------------------------------
// Look and feel: black rotary knobs with a white pointer and 11 printed ticks. Everything is vector, redrawn from the
// parameter value; nothing is baked into an image (layer rule for the NF family).
class NFPELookAndFeel final : public juce::LookAndFeel_V4
{
public:
    NFPELookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    static void drawScrew(juce::Graphics&, float cx, float cy, float rotationDeg);
    static void drawLed(juce::Graphics&, float cx, float cy, juce::Colour c, bool on, float r = 5.5f);
};

// Push button with a bevelled dark face and engraved text.
class HwButton final : public juce::Button
{
public:
    explicit HwButton(const juce::String& text) : juce::Button(text) { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void paintButton(juce::Graphics&, bool over, bool down) override;
    bool dimmed = false;
};

// Up / down effect select rocker (top half = next effect, bottom half = previous).
class Rocker final : public juce::Component
{
public:
    std::function<void(int)> onStep;
    Rocker() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent& e) override { pressedUp = e.y < getHeight() / 2; pressed = true; repaint(); if (onStep) onStep(pressedUp ? +1 : -1); }
    void mouseUp(const juce::MouseEvent&) override { pressed = false; repaint(); }
private:
    bool pressed = false, pressedUp = false;
};

// Two-digit seven segment display (green).
class SevenSeg final : public juce::Component
{
public:
    void set(const juce::String& t, bool dotOn) { if (t != text || dotOn != dot) { text = t; dot = dotOn; repaint(); } }
    void paint(juce::Graphics&) override;
private:
    juce::String text { "01" }; bool dot = false;
};

// Endless program selector: drag or wheel to choose, click (without dragging) to load.
class ProgramKnob final : public juce::Component
{
public:
    std::function<void(int)> onStep;
    std::function<void()> onPush;
    float angle = 0.0f;   // 0..1 shown position
    ProgramKnob() { setMouseCursor(juce::MouseCursor::UpDownResizeCursor); }
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override { acc = 0.0f; dragged = false; lastY = 0; }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        const float dy = (float) (lastY - e.getDistanceFromDragStartY());
        lastY = e.getDistanceFromDragStartY();
        acc += dy;
        while (acc >= 7.0f) { acc -= 7.0f; dragged = true; if (onStep) onStep(+1); }
        while (acc <= -7.0f) { acc += 7.0f; dragged = true; if (onStep) onStep(-1); }
    }
    void mouseUp(const juce::MouseEvent& e) override { if (!dragged && e.getDistanceFromDragStart() < 4 && onPush) onPush(); }
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        wheel += w.deltaY * 6.0f;
        while (wheel >= 1.0f) { wheel -= 1.0f; if (onStep) onStep(+1); }
        while (wheel <= -1.0f) { wheel += 1.0f; if (onStep) onStep(-1); }
    }
private:
    float acc = 0.0f, wheel = 0.0f; int lastY = 0; bool dragged = false;
};

class LogoButton final : public juce::Component
{
public:
    std::function<void()> onClick;
    LogoButton() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void mouseUp(const juce::MouseEvent& e) override { if (contains(e.getPosition()) && onClick) onClick(); }
};

class MenuButton final : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onClick;
    MenuButton() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float lineH = juce::jmax(1.6f, b.getHeight() * 0.10f);
        g.setColour(juce::Colours::white.withAlpha(0.92f));
        for (int i = 0; i < 3; ++i) g.fillRoundedRectangle(b.getX(), b.getY() + b.getHeight() * (0.20f + (float) i * 0.30f), b.getWidth(), lineH, lineH * 0.5f);
    }
    void mouseUp(const juce::MouseEvent& e) override { if (contains(e.getPosition()) && onClick) onClick(); }
};

// Preset tab: [<] name [>] (same small tab as every NF plug-in)
class PresetBar final : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onPrev, onNext, onMenu;
    PresetBar() { setMouseCursor(juce::MouseCursor::PointingHandCursor); }
    void setPresetName(const juce::String& n) { if (n != name) { name = n; repaint(); } }
    void paint(juce::Graphics&) override;
    void mouseUp(const juce::MouseEvent& e) override
    {
        if (!contains(e.getPosition())) return;
        const float s = (float) getHeight() / 29.0f;
        if (e.position.x < 28.0f * s) { if (onPrev) onPrev(); }
        else if (e.position.x > (float) getWidth() - 28.0f * s) { if (onNext) onNext(); }
        else if (onMenu) onMenu();
    }
private:
    juce::String name { "Default" };
};

class NFPEAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NFPEAudioProcessorEditor(NFPEAudioProcessor&);
    ~NFPEAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    juce::Rectangle<int> scaleBounds(juce::Rectangle<float> b) const;
    void applyKnobSteps(int p);
    void showValue(int v);
    void showValueText(const juce::String& t);
    void stepProgram(int delta);
    void onStorePressed();
    void onAuditionPressed();
    void tapTempo(int p);
    void showMainMenu();
    void showProgramMenu();
    void stepPreset(int direction);
    void showAbout();
    void savePresetFile();
    void loadPresetFile();
    juce::String programDisplayText(bool& dot, bool& blinkOff) const;
    bool anyEdits() const;

    float layoutScale = 1.0f, offsetX = 0.0f, offsetY = 0.0f;
    NFPEAudioProcessor& processor;
    NFPELookAndFeel look;
    juce::TooltipWindow tooltipWindow { this, 500 };
    juce::Image logoImage, logoGlow;
    LogoButton logoButton; MenuButton menuButton; PresetBar presetBar;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Slider inputKnob, mixKnob[2], procKnob[2][3];
    HwButton routingBtn { "ROUTING" }, storeBtn { "STORE" }, auditionBtn { "AUDITION" };
    HwButton tempoBtn[2] = { HwButton("TEMPO"), HwButton("TEMPO") }, bypassBtn[2] = { HwButton("BYPASS"), HwButton("BYPASS") };
    Rocker rocker[2];
    SevenSeg seg;
    ProgramKnob programKnob;
#ifdef NF_LICENSE_ENFORCE
    LicenseActivationComponent licenseOverlay;
#endif
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> inputA, mixA[2], procA[2][3];
    std::unique_ptr<BA> bypassA[2];

    // front-panel state
    int pendingSlot = 0;
    bool storeMode = false;
    juce::String holdText; juce::uint32 holdUntil = 0; bool holdDot = false;
    juce::uint32 lastTap[2] = { 0, 0 };
    int lastFx[2] = { -1, -1 };
    size_t lastHash = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NFPEAudioProcessorEditor)
};
