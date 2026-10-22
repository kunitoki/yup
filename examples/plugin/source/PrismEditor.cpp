/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "PrismEditor.h"
#include "ui/SynthTheme.h"

#include <type_traits>

namespace
{
/** The PRISM logo, with the viewBox cropped to the drawing so it fills the header slot.
    Its #FFFFFF stroke is replaced by the palette text color when parsed. */
constexpr const char* synthLogoSvg = R"svg(
<svg xmlns="http://www.w3.org/2000/svg" width="236" height="122" viewBox="10 66 236 122" fill="none">
  <g stroke="#FFFFFF" stroke-linejoin="miter" stroke-linecap="square" transform="translate(0 37)">
    <path d="M14 100 q4 -7 8 0 t8 0 t8 0 t8 0 t8 0 t8 0 t8 0 t8 0 t8 0 L91 100" stroke-width="2.2" stroke-linecap="round"/>
    <path d="M128 36 L191.5 146 L64.5 146 Z" stroke-width="3"/>
    <path d="M91 100 L160.3 92" stroke-width="1.5" stroke-opacity="0.5"/>
    <g stroke-linecap="butt">
      <path d="M160.3 92 L242 66"  stroke-width="2.5" stroke-opacity="1"/>
      <path d="M160.3 92 L242 80"  stroke-width="2.2" stroke-opacity="0.8"/>
      <path d="M160.3 92 L242 94"  stroke-width="2"   stroke-opacity="0.6"/>
      <path d="M160.3 92 L242 108" stroke-width="1.7" stroke-opacity="0.42"/>
      <path d="M160.3 92 L242 122" stroke-width="1.5" stroke-opacity="0.26"/>
    </g>
  </g>
</svg>
)svg";

yup::Font getSynthFont (float height)
{
    return yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (height);
}

/** Keeps every control under a component out of the keyboard focus, so the computer
    keyboard keeps playing the on-screen keyboard while the mouse tweaks them. Sliders,
    combo boxes and buttons all take the focus on click whenever they want it. */
void keepFocusOnKeyboard (yup::Component& component, const yup::Component& keyboard)
{
    for (int index = 0; index < component.getNumChildComponents(); ++index)
    {
        auto* child = component.getChildComponent (index);

        if (child == &keyboard)
            continue;

        child->setWantsKeyboardFocus (false);
        keepFocusOnKeyboard (*child, keyboard);
    }
}
} // namespace

//==============================================================================
PrismEditor::PrismEditor (PrismProcessor& processorToEdit)
    : processor (processorToEdit)
    , keyboardComponent (processor.getKeyboardState(), yup::MidiKeyboardComponent::horizontalKeyboard)
    , pitchWheelComponent (processor.getKeyboardState(), "PitchWheel")
    , modWheelComponent (processor.getKeyboardState(), "ModWheel")
{
    if (auto palette = yup::ThemePalette::fromString ("https://coolors.co/011627-f71735-41ead4-fdfffc-ff9f1c"); palette.wasOk())
        yup::ApplicationTheme::getGlobalTheme()->setPalette (palette.getReference());

    const auto& parameters = processor.getSynthParameters();
    const auto controlFont = getSynthFont (10.0f);

    keyboardComponent.setAvailableRange (36, 84); // C2 to C6
    keyboardComponent.setLowestVisibleKey (48);   // Start from C3
    keyboardComponent.setMidiChannel (1);
    keyboardComponent.setVelocity (0.7f);
    keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::whiteKeyPressedColorId, SynthTheme::accent());
    keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::blackKeyPressedColorId, SynthTheme::accentDim());
    keyboardComponent.setColor (yup::MidiKeyboardComponent::Style::keyOutlineColorId, SynthTheme::panelBorder());
    mainPage.addAndMakeVisible (keyboardComponent);

    // The wheels follow the processor's keyboard state, host MIDI included; the keyboard state
    // only plays notes, so their own moves go to the processor through its MIDI queue.
    pitchWheelComponent.onValueChanged = [this] (double value)
    {
        processor.sendMidiFromEditor (yup::MidiMessage::pitchWheel (1, 8192 + static_cast<int> (value * 8191.0)));
    };
    modWheelComponent.onValueChanged = [this] (double value)
    {
        processor.sendMidiFromEditor (yup::MidiMessage::controllerEvent (1, 1, static_cast<int> (value * 127.0)));
    };

    const auto addWheel = [this] (auto& wheel)
    {
        using Style = typename std::decay_t<decltype (wheel)>::Style;

        wheel.setColor (Style::bodyTopColorId, SynthTheme::panelBackground());
        wheel.setColor (Style::bodyBottomColorId, SynthTheme::displayBackground());
        wheel.setColor (Style::outlineColorId, SynthTheme::panelBorder());
        wheel.setColor (Style::gripColorId, SynthTheme::accentDim());
        wheel.setColor (Style::gripOverColorId, SynthTheme::accent());
        wheel.setColor (Style::gripDownColorId, SynthTheme::accent());
        mainPage.addAndMakeVisible (wheel);
    };
    addWheel (pitchWheelComponent);
    addWheel (modWheelComponent);

    logo.parseSVG (yup::String (synthLogoSvg).replace ("#FFFFFF", SynthTheme::textPrimary().toString()));

    titleLabel.setText ("P R I S M   /   SPECTRAL SYNTH", yup::dontSendNotification);
    titleLabel.setFont (getSynthFont (20.0f));
    titleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textPrimary());
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("Sculpt harmonics. Scatter phases. Play the spectrum.", yup::dontSendNotification);
    subtitleLabel.setFont (getSynthFont (12.0f));
    subtitleLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary());
    addAndMakeVisible (subtitleLabel);

    loadLabel.setFont (getSynthFont (11.0f));
    loadLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::textSecondary());
    mainPage.addAndMakeVisible (loadLabel);

    voiceLabel.setFont (getSynthFont (11.0f));
    voiceLabel.setColor (yup::Label::Style::textFillColorId, SynthTheme::accent());
    addAndMakeVisible (voiceLabel);

    for (int index = 0; index < SynthExample::oscillatorCount; ++index)
    {
        auto& panel = oscillatorPanels[static_cast<std::size_t> (index)];
        panel = std::make_unique<SynthOscillatorPanel> (yup::String ("OSC ") + yup::String (index + 1),
                                                        parameters.oscillators[static_cast<std::size_t> (index)],
                                                        processor.getPartials (index),
                                                        processor.getResources(),
                                                        waveformShader,
                                                        controlFont);
    }

    for (int index = 0; index < SynthExample::filterCount; ++index)
    {
        filterPanels[static_cast<std::size_t> (index)] = std::make_unique<SynthFilterPanel> (yup::String ("FILTER ") + yup::String (index + 1),
                                                                                             parameters.filters[static_cast<std::size_t> (index)],
                                                                                             controlFont);
    }

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
    {
        envelopePanels[static_cast<std::size_t> (index)] = std::make_unique<SynthEnvelopePanel> (yup::String ("ENV ") + yup::String (index + 1) + (index == 0 ? " / AMP" : ""),
                                                                                                 parameters.envelopes[static_cast<std::size_t> (index)],
                                                                                                 processor.getCurves(),
                                                                                                 index,
                                                                                                 controlFont);
    }

    for (int index = 0; index < SynthExample::lfoCount; ++index)
    {
        lfoPanels[static_cast<std::size_t> (index)] = std::make_unique<SynthLFOPanel> (yup::String ("LFO ") + yup::String (index + 1),
                                                                                       parameters.lfos[static_cast<std::size_t> (index)],
                                                                                       processor.getCurves(),
                                                                                       index,
                                                                                       controlFont);
    }

    const auto panelsOf = [] (auto& array)
    {
        std::vector<yup::Component*> result;

        for (auto& panel : array)
            result.push_back (panel.get());

        return result;
    };

    const auto numbered = [] (const yup::String& prefix, int count)
    {
        yup::StringArray names;

        for (int index = 1; index <= count; ++index)
            names.add (prefix + yup::String (index));

        return names;
    };

    setupTabs (oscillatorTabs, numbered ("OSC ", SynthExample::oscillatorCount), panelsOf (oscillatorPanels));
    setupTabs (filterTabs, numbered ("F", SynthExample::filterCount), panelsOf (filterPanels));
    setupTabs (envelopeTabs, numbered ("ENV ", SynthExample::envelopeCount), panelsOf (envelopePanels));
    setupTabs (lfoTabs, numbered ("LFO ", SynthExample::lfoCount), panelsOf (lfoPanels));

    fmPanel = std::make_unique<SynthFMPanel> (parameters.fm, controlFont);
    modulationPage = std::make_unique<SynthModulationPage> (parameters, processor.getRoutes(), processor.getParameters(), controlFont);
    setupTabs (matrixTabs, { "FM", "MODS" }, { fmPanel.get(), modulationPage.get() });

    mainPage.onMouseDown = [this] { keyboardComponent.takeKeyboardFocus(); };
    addAndMakeVisible (mainPage);

    SynthTheme::styleButton (randomizeButton, SynthTheme::panelBackground());
    randomizeButton.onClick = [this]
    {
        processor.randomize();
        presetBar->setCustomPatchName ("Random");
    };
    addAndMakeVisible (randomizeButton);

    SynthTheme::styleButton (clearButton, SynthTheme::panelBackground());
    clearButton.onClick = [this]
    {
        processor.getKeyboardState().allNotesOff (0); // Turn off all notes on all channels
        processor.requestAllNotesOff();
    };
    addAndMakeVisible (clearButton);

    volumeKnob = std::make_unique<ParameterKnob> ("VOLUME", parameters.volume, controlFont);
    addAndMakeVisible (*volumeKnob);

    modeChoice = std::make_unique<ParameterChoice> ("VOICE MODE", parameters.playMode, controlFont);
    mainPage.addAndMakeVisible (*modeChoice);

    glideKnob = std::make_unique<ParameterKnob> ("GLIDE", parameters.glide, controlFont);
    mainPage.addAndMakeVisible (*glideKnob);

    qualityChoice = std::make_unique<ParameterChoice> ("QUALITY", parameters.quality, controlFont);
    mainPage.addAndMakeVisible (*qualityChoice);

    routingChoice = std::make_unique<ParameterChoice> ("FILTERS", parameters.filterRouting, controlFont);
    mainPage.addAndMakeVisible (*routingChoice);

    unisonKnob = std::make_unique<ParameterKnob> ("UNISON", parameters.unisonVoices, controlFont);
    unisonDetuneKnob = std::make_unique<ParameterKnob> ("DETUNE", parameters.unisonDetune, controlFont);
    unisonSpreadKnob = std::make_unique<ParameterKnob> ("SPREAD", parameters.unisonSpread, controlFont);

    for (auto* knob : { unisonKnob.get(), unisonDetuneKnob.get(), unisonSpreadKnob.get() })
        mainPage.addAndMakeVisible (*knob);

    presetBar = std::make_unique<SynthPresetBar> (processor);
    mainPage.addAndMakeVisible (*presetBar);


    keepFocusOnKeyboard (*this, keyboardComponent);
    syncFromProcessor();

    setSize (getPreferredSize().to<float>());
    startTimerHz (60);
}

PrismEditor::~PrismEditor()
{
    stopTimer();
}

//==============================================================================
void PrismEditor::resized()
{
    auto bounds = getLocalBounds().reduced (outerInset);

    auto header = bounds.removeFromTop (headerHeight);

    volumeKnob->setBounds (header.removeFromRight (64.0f));
    header.removeFromRight (spacing);
    clearButton.setBounds (header.removeFromRight (actionButtonWidth).reduced (0.0f, buttonInset));
    header.removeFromRight (spacing);
    randomizeButton.setBounds (header.removeFromRight (actionButtonWidth).reduced (0.0f, buttonInset));
    header.removeFromRight (spacing * 2.0f);
    voiceLabel.setBounds (header.removeFromRight (110.0f));

    const auto logoHeight = header.getHeight() - 8.0f;
    logoArea = header.removeFromLeft (logoHeight * logoAspect).withSizeKeepingCenter (logoHeight * logoAspect, logoHeight);
    header.removeFromLeft (spacing * 1.5f);

    const auto textArea = header.reduced (0.0f, 6.0f);
    titleLabel.setBounds (textArea.withHeight (textArea.getHeight() * 0.55f));
    subtitleLabel.setBounds (textArea.withTrimmedTop (textArea.getHeight() * 0.55f));

    bounds.removeFromTop (spacing);

    mainPage.setBounds (bounds);

    layoutMainPage();
}

/** Lays the main page out from the bottom up: keyboard, performance row, shaping, operators. */
void PrismEditor::layoutMainPage()
{
    auto bounds = mainPage.getLocalBounds();

    auto keyboardRow = bounds.removeFromBottom (yup::jmin (keyboardHeight, mainPage.proportionOfHeight (0.12f)));
    pitchWheelComponent.setBounds (keyboardRow.removeFromLeft (wheelWidth).reduced (0.0f, 4.0f));
    keyboardRow.removeFromLeft (spacing);
    modWheelComponent.setBounds (keyboardRow.removeFromLeft (wheelWidth).reduced (0.0f, 4.0f));
    keyboardRow.removeFromLeft (spacing * 2.0f);
    keyboardComponent.setBounds (keyboardRow);
    bounds.removeFromBottom (spacing);

    auto performance = bounds.removeFromBottom (58.0f);
    modeChoice->setBounds (performance.removeFromLeft (170.0f).reduced (4.0f, 9.0f));
    glideKnob->setBounds (performance.removeFromLeft (64.0f));
    qualityChoice->setBounds (performance.removeFromLeft (80.0f).reduced (4.0f, 9.0f));
    routingChoice->setBounds (performance.removeFromLeft (100.0f).reduced (4.0f, 9.0f));

    for (auto* knob : { unisonKnob.get(), unisonDetuneKnob.get(), unisonSpreadKnob.get() })
        knob->setBounds (performance.removeFromLeft (64.0f));

    performance.removeFromLeft (spacing);
    presetBar->setBounds (performance.removeFromLeft (360.0f).reduced (4.0f, 15.0f));
    performance.removeFromLeft (spacing);
    loadLabel.setBounds (performance);
    bounds.removeFromBottom (spacing);

    const auto columnWidth = (bounds.getWidth() - spacing * 2.0f) / 3.0f;

    auto shapingRow = bounds.removeFromBottom (shapingRowHeight + tabHeight);
    layoutTabbedSection (filterTabs, shapingRow.removeFromLeft (columnWidth));
    shapingRow.removeFromLeft (spacing);
    layoutTabbedSection (envelopeTabs, shapingRow.removeFromLeft (columnWidth));
    shapingRow.removeFromLeft (spacing);
    layoutTabbedSection (lfoTabs, shapingRow);
    bounds.removeFromBottom (spacing);

    // The FM and modulation tabs share the oscillators' tab row, so the panels line up.
    layoutTabbedSection (matrixTabs, bounds.removeFromRight (columnWidth));
    bounds.removeFromRight (spacing);
    layoutTabbedSection (oscillatorTabs, bounds);
}

void PrismEditor::setupTabs (TabGroup& group, const yup::StringArray& names, std::vector<yup::Component*> panels)
{
    group.panels = std::move (panels);

    for (int index = 0; index < static_cast<int> (group.panels.size()); ++index)
    {
        auto button = std::make_unique<yup::ToggleButton>();
        button->setButtonText (names[index]);
        SynthTheme::styleToggle (*button);
        button->onClick = [this, &group, index] { selectTab (group, index); };
        mainPage.addAndMakeVisible (*button);
        group.buttons.push_back (std::move (button));

        mainPage.addChildComponent (*group.panels[static_cast<std::size_t> (index)]);
    }

    selectTab (group, 0);
}

void PrismEditor::selectTab (TabGroup& group, int index)
{
    group.selected = index;

    for (std::size_t tab = 0; tab < group.panels.size(); ++tab)
    {
        const auto shown = static_cast<int> (tab) == index;
        group.buttons[tab]->setToggleState (shown, yup::dontSendNotification);
        group.panels[tab]->setVisible (shown);
    }
}

void PrismEditor::layoutTabbedSection (TabGroup& group, yup::Rectangle<float> area)
{
    auto tabs = area.removeFromTop (tabHeight);
    const auto tabWidth = yup::jmin (72.0f, tabs.getWidth() / static_cast<float> (group.buttons.size()));

    for (auto& button : group.buttons)
    {
        button->setBounds (tabs.removeFromLeft (tabWidth).reduced (2.0f, 0.0f));
    }

    area.removeFromTop (2.0f);

    for (auto* panel : group.panels)
        panel->setBounds (area);
}

void PrismEditor::paint (yup::Graphics& g)
{
    g.setFillColor (SynthTheme::windowBackground());
    g.fillAll();

    logo.paint (g, logoArea);
}

void PrismEditor::mouseDown (const yup::MouseEvent&)
{
    keyboardComponent.takeKeyboardFocus();
}

//==============================================================================
void PrismEditor::timerCallback()
{
    syncFromProcessor();


    for (int index = 0; index < SynthExample::lfoCount; ++index)
        lfoPanels[static_cast<std::size_t> (index)]->setPhase (processor.getLFOPhase (index));

    loadLabel.setText (yup::String (processor.getLoadPercentage(), 1) + "% AUDIO / "
                           + yup::String (processor.getXRunCount()) + " OVERRUNS",
                       yup::dontSendNotification);
    voiceLabel.setText (yup::String (processor.getNumActiveVoices()) + " / " + yup::String (SynthExample::voiceCount) + " VOICES",
                        yup::dontSendNotification);
}

/** Shows every parameter and partial change, wherever it came from. */
void PrismEditor::syncFromProcessor()
{
    for (auto& panel : oscillatorPanels)
        panel->syncFromParameters();

    for (auto& panel : filterPanels)
        panel->syncFromParameters();

    for (auto& panel : envelopePanels)
        panel->syncFromParameters();

    for (auto& panel : lfoPanels)
        panel->syncFromParameters();

    fmPanel->syncFromParameters();
    modulationPage->syncFromParameters();

    volumeKnob->syncFromParameter();
    modeChoice->syncFromParameter();
    glideKnob->syncFromParameter();
    qualityChoice->syncFromParameter();
    routingChoice->syncFromParameter();

    for (auto* knob : { unisonKnob.get(), unisonDetuneKnob.get(), unisonSpreadKnob.get() })
        knob->syncFromParameter();
    glideKnob->setEnabled (yup::roundToInt (processor.getSynthParameters().playMode->getValue()) != static_cast<int> (SynthPlayMode::poly));

    presetBar->syncFromProcessor();
}
