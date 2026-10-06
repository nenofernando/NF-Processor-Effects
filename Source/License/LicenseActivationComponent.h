#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLicenseManager.h"

/** Popup de ativação centralizado - fundo semi-transparente cobre a janela
    toda (dá pra ver o plugin por trás, mas fica claro que precisa ativar),
    com um cartão bem visível no meio (campo de chave + botão + status).
    Feedback do usuário: uma barra fina no topo passava despercebida - isso
    aqui é bem mais difícil de ignorar. Some sozinho quando a ativação der
    certo.

    Janelas baixas (plug-ins estilo rack, ex: NF D-42, 76 px de altura no
    mínimo): o cartão de 220 px não cabia - cortava o título e a mensagem de
    erro, e parecia que "clicar em ativar não faz nada". Abaixo de 240 px de
    altura o cartão vira UMA linha (título | chave | botão) com a mensagem de
    status logo abaixo, sempre dentro da janela. */
class LicenseActivationComponent : public juce::Component
{
public:
    explicit LicenseActivationComponent (NFLicenseManager& managerIn) : manager (managerIn)
    {
        titleLabel.setText ("Ative sua licenca", juce::dontSendNotification);
        titleLabel.setJustificationType (juce::Justification::centred);
        titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (titleLabel);

        keyEditor.setTextToShowWhenEmpty ("XXXXX-XXXXX-XXXXX-XXXXX", juce::Colours::grey);
        keyEditor.setJustification (juce::Justification::centred);
        keyEditor.onReturnKey = [this] { attemptActivation(); };
        addAndMakeVisible (keyEditor);

        activateButton.setButtonText ("Activate License");
        activateButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff7c3aed));
        activateButton.onClick = [this] { attemptActivation(); };
        addAndMakeVisible (activateButton);

        statusLabel.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (statusLabel);

        applyFonts (false);
    }

    void paint (juce::Graphics& g) override
    {
        // Fundo escurecido semi-transparente - o plugin continua visível
        // por baixo, só fica claro que a interação está bloqueada.
        g.fillAll (juce::Colour (0x70000000));

        auto card = cardBounds();
        const float radius = isCompact() ? 10.0f : 14.0f;
        g.setColour (juce::Colour (0xf2141416));
        g.fillRoundedRectangle (card.toFloat(), radius);
        g.setColour (juce::Colour (0xff7c3aed));
        g.drawRoundedRectangle (card.toFloat(), radius, 1.5f);
    }

    void resized() override
    {
        const bool compact = isCompact();
        applyFonts (compact);

        if (! compact)
        {
            auto area = cardBounds().reduced (28, 24);
            titleLabel.setBounds (area.removeFromTop (32));
            area.removeFromTop (16);
            keyEditor.setBounds (area.removeFromTop (36));
            area.removeFromTop (14);
            activateButton.setBounds (area.removeFromTop (38));
            area.removeFromTop (12);
            statusLabel.setBounds (area.removeFromTop (24));
            return;
        }

        // Layout de uma linha: [título] [campo da chave] [botão], status embaixo.
        auto area = cardBounds().reduced (12, 5);
        const int statusH = area.getHeight() >= 46 ? 16 : 0;
        auto statusRow = area.removeFromBottom (statusH);
        if (statusH > 0)
            area.removeFromBottom (2);

        const int rowH = juce::jmin (34, area.getHeight());
        auto row = area.withSizeKeepingCentre (area.getWidth(), rowH);
        titleLabel.setBounds (row.removeFromLeft (juce::jmin (170, row.getWidth() / 4)));
        activateButton.setBounds (row.removeFromRight (juce::jmin (140, row.getWidth() / 4)));
        keyEditor.setBounds (row.reduced (8, 0));
        statusLabel.setBounds (statusRow);
    }

private:
    bool isCompact() const { return getHeight() < 240; }

    juce::Rectangle<int> cardBounds() const
    {
        if (! isCompact())
            return getLocalBounds().withSizeKeepingCentre (360, 220);

        const int w = juce::jmin (getWidth() - 16, 780);
        const int h = juce::jmin (getHeight() - 8, 100);
        return getLocalBounds().withSizeKeepingCentre (juce::jmax (w, 100), juce::jmax (h, 40));
    }

    void applyFonts (bool compact)
    {
        if (compact == fontsAreCompact)
            return;
        fontsAreCompact = compact;
        titleLabel.setFont (juce::Font (juce::FontOptions (compact ? 16.0f : 20.0f, juce::Font::bold)));
        keyEditor.setFont (juce::Font (juce::FontOptions (compact ? 15.0f : 16.0f)));
        statusLabel.setFont (juce::Font (juce::FontOptions (compact ? 12.0f : 13.0f)));
    }

    /** Mensagens do servidor em português claro (o código cru - "product_not_licensed" - não ajuda o cliente). */
    static juce::String friendlyError (const juce::String& code)
    {
        if (code == "invalid_license")
            return juce::String::fromUTF8 ("Chave n" "\xc3\xa3" "o encontrada. Confira e tente de novo.");
        if (code == "license_revoked")
            return juce::String::fromUTF8 ("Esta licen" "\xc3\xa7" "a foi desativada. Fale com o suporte.");
        if (code == "product_not_licensed")
            return juce::String::fromUTF8 ("Esta chave n" "\xc3\xa3" "o " "\xc3\xa9" " deste plugin.");
        if (code == "no_activation_slots")
            return juce::String::fromUTF8 ("Esta chave j" "\xc3\xa1" " est" "\xc3\xa1" " ativa no m" "\xc3\xa1" "ximo de computadores.");
        return code;
    }

    void attemptActivation()
    {
        auto key = keyEditor.getText().trim();
        if (key.isEmpty())
        {
            statusLabel.setText ("Digite a chave de licenca.", juce::dontSendNotification);
            statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
            return;
        }

        activateButton.setEnabled (false);
        statusLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        statusLabel.setText ("Verificando...", juce::dontSendNotification);

        // SafePointer em vez de capturar `this` cru: a resposta pode chegar
        // depois desta tela (ex: janela do editor) ja ter sido fechada.
        juce::Component::SafePointer<LicenseActivationComponent> safeThis (this);
        manager.activateAsync (key, [safeThis] (bool success, juce::String errorMessage)
        {
            auto* self = safeThis.getComponent();
            if (self == nullptr)
                return;

            self->activateButton.setEnabled (true);

            if (success)
            {
                self->statusLabel.setColour (juce::Label::textColourId, juce::Colours::limegreen);
                self->statusLabel.setText ("Ativado!", juce::dontSendNotification);
                if (self->onActivated)
                    self->onActivated();
            }
            else
            {
                self->statusLabel.setColour (juce::Label::textColourId, juce::Colours::orangered);
                self->statusLabel.setText (friendlyError (errorMessage), juce::dontSendNotification);
            }
        });
    }

public:
    std::function<void()> onActivated;

private:
    NFLicenseManager& manager;
    juce::Label titleLabel, statusLabel;
    juce::TextEditor keyEditor;
    juce::TextButton activateButton;
    bool fontsAreCompact = true;   // força a primeira applyFonts() a aplicar
};
