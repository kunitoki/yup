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

#include "SynthInterface.h"
#include "SynthNeonEffect.h"
#include "SynthKeyboardLook.h"
#include "SynthTheme.h"
#include "SynthSettings.h"
#include "SynthThemes.h"

namespace
{
/** Collects every control under a component that shows the modulation of its parameter. */
void collectModulationTargets (yup::Component& component, std::vector<SynthModulationTarget*>& targets)
{
    for (int index = 0; index < component.getNumChildComponents(); ++index)
    {
        auto* child = component.getChildComponent (index);

        if (auto* target = dynamic_cast<SynthModulationTarget*> (child))
            targets.push_back (target);

        collectModulationTargets (*child, targets);
    }
}

} // namespace

//==============================================================================
SynthInterface::SynthInterface (RetraceProcessor& processorToEdit, int themeToWear)
    : processor (processorToEdit)
    , theme (themeToWear)
    , keyboardComponent (processor.getKeyboardState(), yup::MidiKeyboardComponent::horizontalKeyboard)
    , pitchWheelComponent (processor.getKeyboardState(), "PitchWheel")
    , modWheelComponent (processor.getKeyboardState(), "ModWheel")
{
    const auto& parameters = processor.getSynthParameters();

    keyboardComponent.setAvailableRange (36, 84); // C2 to C6
    keyboardComponent.setLowestVisibleKey (48);   // Start from C3
    keyboardComponent.setMidiChannel (1);
    keyboardComponent.setVelocity (0.7f);
    SynthKeyboardLook::apply (keyboardComponent);
    SynthTheme::applySection (keyboardComponent, SynthTheme::Section::keys);
    mainPage.addAndMakeVisible (keyboardComponent);

    // The interface takes the focus in place of the keyboard, which plays the keys that reach
    // the interface, so a click on the keys gives the focus to the interface.
    keyboardComponent.setWantsKeyboardFocus (false);
    setWantsKeyboardFocus (true);

    applyUndoSetting();

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

    for (auto* wheel : std::initializer_list<yup::Component*> { &pitchWheelComponent, &modWheelComponent })
        SynthTheme::applySection (*wheel, SynthTheme::Section::keys);

    SynthKeyboardLook::apply (pitchWheelComponent);
    SynthKeyboardLook::apply (modWheelComponent);
    mainPage.addAndMakeVisible (pitchWheelComponent);
    mainPage.addAndMakeVisible (modWheelComponent);

    header = std::make_unique<SynthHeader> (processor);
    header->onSettingsRequested = [this] (yup::Point<float> position) { showSettingsMenu (getLocalPoint (header.get(), position)); };
    addAndMakeVisible (*header);

    for (int index = 0; index < SynthExample::oscillatorCount; ++index)
    {
        auto& panel = oscillatorPanels[static_cast<std::size_t> (index)];
        panel = std::make_unique<SynthOscillatorPanel> (parameters.oscillators[static_cast<std::size_t> (index)],
                                                        processor.getPartials (index),
                                                        processor.getResources(),
                                                        waveformShader);
    }

    for (int index = 0; index < SynthExample::filterCount; ++index)
    {
        filterPanels[static_cast<std::size_t> (index)] = std::make_unique<SynthFilterPanel> (parameters.filters[static_cast<std::size_t> (index)]);
    }

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
    {
        envelopePanels[static_cast<std::size_t> (index)] = std::make_unique<SynthEnvelopePanel> (parameters.envelopes[static_cast<std::size_t> (index)], processor.getCurves(), index);
    }

    for (int index = 0; index < SynthExample::lfoCount; ++index)
    {
        lfoPanels[static_cast<std::size_t> (index)] = std::make_unique<SynthLFOPanel> (parameters.lfos[static_cast<std::size_t> (index)], processor.getCurves(), index);
    }

    const auto makeCaption = [] (const yup::String& text)
    {
        auto label = std::make_unique<yup::Label>();
        label->setText (text, yup::dontSendNotification);
        label->setFont (SynthTheme::captionFont (12.0f));
        label->setJustification (yup::Justification::centerRight);
        return label;
    };

    std::vector<SynthSection::Tab> oscillatorTabs;

    for (int index = 0; index < SynthExample::oscillatorCount; ++index)
        oscillatorTabs.push_back ({ "Osc " + yup::String (index + 1), oscillatorPanels[static_cast<std::size_t> (index)].get(), parameters.oscillators[static_cast<std::size_t> (index)].enabled });

    oscillatorSection = std::make_unique<SynthSection> (std::move (oscillatorTabs), SynthTheme::Section::oscillators);

    fmPanel = std::make_unique<SynthFMPanel> (parameters);
    modulationPage = std::make_unique<SynthModulationPage> (parameters, processor.getRoutes(), processor.getParameters());
    matrixCaption = makeCaption (yup::String::fromUTF8 ("ROW \xe2\x86\x90 COLUMN"));
    matrixSection = std::make_unique<SynthSection> (std::vector<SynthSection::Tab> { { "FM", fmPanel.get(), nullptr, false, { { &fmPanel->getResetButton(), 60.0f }, { &fmPanel->getAlgorithmButton(), 88.0f } } },
                                                                                     { "Mods", modulationPage.get() } },
                                                    SynthTheme::Section::matrix,
                                                    std::vector<SynthSection::StripControl> { { matrixCaption.get(), 110.0f } });

    // The macros drive routes, so they wear the matrix's palette.
    macroPanel = std::make_unique<SynthMacroPanel> (processor);
    macroSection = std::make_unique<SynthSection> (std::vector<SynthSection::Tab> { { "Macros", macroPanel.get() } }, SynthTheme::Section::matrix);

    std::vector<SynthSection::Tab> filterTabs;

    for (int index = 0; index < SynthExample::filterCount; ++index)
        filterTabs.push_back ({ "Filter " + yup::String (index + 1), filterPanels[static_cast<std::size_t> (index)].get(), parameters.filters[static_cast<std::size_t> (index)].enabled });

    routingSelector = std::make_unique<ParameterSegmented> (parameters.filterRouting, std::vector<ParameterSegmented::Segment> { { "Serial" }, { "Par" } });
    filterSection = std::make_unique<SynthSection> (std::move (filterTabs), SynthTheme::Section::filters, std::vector<SynthSection::StripControl> { { routingSelector.get(), 96.0f } });

    std::vector<SynthSection::Tab> envelopeTabs;
    ampCaption = makeCaption (yup::String::fromUTF8 ("\xe2\x86\x92 AMP"));

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
    {
        const auto& envelope = parameters.envelopes[static_cast<std::size_t> (index)];
        SynthSection::Tab tab { "Env " + yup::String (index + 1), envelopePanels[static_cast<std::size_t> (index)].get(), envelope.enabled, envelope.enabled == nullptr };

        if (index == 0)
            tab.stripControls.push_back ({ ampCaption.get(), 48.0f });

        envelopeTabs.push_back (std::move (tab));
    }

    envelopeSection = std::make_unique<SynthSection> (std::move (envelopeTabs), SynthTheme::Section::envelopes);

    std::vector<SynthSection::Tab> lfoTabs;

    for (int index = 0; index < SynthExample::lfoCount; ++index)
    {
        const auto& lfo = parameters.lfos[static_cast<std::size_t> (index)];
        auto& syncLed = lfoSyncLeds.emplace_back (std::make_unique<ParameterLed> (lfo.tempoSync, "Sync"));

        lfoTabs.push_back ({ "LFO " + yup::String (index + 1), lfoPanels[static_cast<std::size_t> (index)].get(), lfo.enabled, false, { { syncLed.get(), 56.0f } } });
    }

    lfoSection = std::make_unique<SynthSection> (std::move (lfoTabs), SynthTheme::Section::lfos);

    for (auto* section : { oscillatorSection.get(), macroSection.get(), matrixSection.get(), filterSection.get(), envelopeSection.get(), lfoSection.get() })
        mainPage.addAndMakeVisible (*section);

    addAndMakeVisible (mainPage);

    voicingPanel = std::make_unique<SynthVoicingPanel> (parameters);
    mainPage.addAndMakeVisible (*voicingPanel);

    collectModulationTargets (*this, modulationTargets);

    SynthThemes::applyTheme (*this, theme);
    ampCaption->setColor (yup::Label::Style::textFillColorId, SynthTheme::envelopeAccent (*this));

    // Menus and combo box lists open inside the interface, so the post effect draws them too.
    setPopupContainer (true);
    syncFromProcessor();

    // Every repaint redraws the whole interface under the post effect, so the displays
    // follow the processor at 30 Hz; dragging a control still repaints it at once.
    startTimerHz (30);
}

SynthInterface::~SynthInterface()
{
    stopTimer();
}

//==============================================================================
void SynthInterface::resized()
{
    auto bounds = getLocalBounds().reduced (SynthTheme::frameGap);
    header->setRightGroupWidth (std::round (bounds.getWidth() * SynthTheme::rightColumnShare));
    header->setBounds (bounds.removeFromTop (SynthTheme::headerHeight));

    bounds.removeFromTop (SynthTheme::frameGap);
    mainPage.setBounds (bounds);

    layoutMainPage();
}

/** Lays the instrument out as rows of panels with a right column of one width, so the
    header, FM, LFO and voicing edges line up. */
void SynthInterface::layoutMainPage()
{
    const auto gap = SynthTheme::frameGap;
    auto bounds = mainPage.getLocalBounds();
    const auto rightWidth = std::round (bounds.getWidth() * SynthTheme::rightColumnShare);

    auto bottom = bounds.removeFromBottom (SynthTheme::bottomRowHeight);
    bounds.removeFromBottom (gap);

    voicingPanel->setBounds (bottom.removeFromRight (rightWidth));
    bottom.removeFromRight (gap);

    const auto offset = mainPage.getPosition();
    wheelsTile = bottom.removeFromLeft (wheelsTileWidth);
    bottom.removeFromLeft (gap);
    keyboardTile = bottom;

    auto wheels = wheelsTile.reduced (12.0f, 8.0f).withTrimmedBottom (wheelCaptionHeight);
    pitchWheelComponent.setBounds (wheels.removeFromLeft (wheelWidth));
    modWheelComponent.setBounds (wheels.removeFromRight (wheelWidth));
    keyboardComponent.setBounds (keyboardTile.reduced (8.0f).withTrimmedTop (2.0f));

    wheelsTile = wheelsTile.withPosition (wheelsTile.getPosition() + offset);
    keyboardTile = keyboardTile.withPosition (keyboardTile.getPosition() + offset);

    auto firstRow = bounds.removeFromTop (std::round ((bounds.getHeight() - gap) * firstRowShare));
    bounds.removeFromTop (gap);

    matrixSection->setBounds (firstRow.removeFromRight (rightWidth));
    firstRow.removeFromRight (gap);
    macroSection->setBounds (firstRow.removeFromRight (macroColumnWidth));
    firstRow.removeFromRight (gap);
    oscillatorSection->setBounds (firstRow);

    lfoSection->setBounds (bounds.removeFromRight (rightWidth));
    bounds.removeFromRight (gap);
    filterSection->setBounds (bounds.removeFromLeft (std::round ((bounds.getWidth() + gap) * filterShare - gap)));
    bounds.removeFromLeft (gap);
    envelopeSection->setBounds (bounds);
}

void SynthInterface::paint (yup::Graphics& g)
{
    g.setFillColor (SynthTheme::frameColor (*this));
    g.fillAll();

    SynthTheme::paintTile (g, keyboardComponent, wheelsTile);
    SynthTheme::paintTile (g, keyboardComponent, keyboardTile);
    SynthTheme::paintWell (g, keyboardComponent, keyboardTile.reduced (6.0f));

    const auto captions = wheelsTile.reduced (12.0f, 8.0f).withTrimmedTop (wheelsTile.getHeight() - 16.0f - wheelCaptionHeight);
    g.setFillColor (SynthTheme::textSecondary (keyboardComponent));
    g.fillFittedText ("PITCH", SynthTheme::captionFont (10.5f), captions.withWidth (wheelWidth).withSizeKeepingCenter (wheelWidth + 12.0f, captions.getHeight()), yup::Justification::center);
    g.fillFittedText ("MOD", SynthTheme::captionFont (10.5f), captions.withTrimmedLeft (captions.getWidth() - wheelWidth).withSizeKeepingCenter (wheelWidth + 12.0f, captions.getHeight()), yup::Justification::center);
}

void SynthInterface::showSettingsMenu (yup::Point<float> position)
{
    auto& settings = SynthSettings::get();
    auto& neon = settings.neon;

    // Switch IDs are 1 based, a switch reached through its index in this table; theme IDs
    // start at themeBase, a theme reached through its index in SynthThemes; frame rate IDs
    // are frameRateBase plus the rate, 0 standing for vsync.
    constexpr int themeBase = 1000;
    constexpr int frameRateBase = 2000;
    constexpr int undoId = 3000;

    const std::pair<const char*, bool*> switches[] { { "Post Effect", &settings.postEffect },
                                                     { "Neon Glow", &neon.glow },
                                                     { "Chromatic Aberration", &neon.aberration },
                                                     { "Color Grade", &neon.grade },
                                                     { "Scanlines", &neon.scanlines },
                                                     { "Vignette", &neon.vignette },
                                                     { "Grain", &neon.grain },
                                                     { "Tube Curvature", &neon.curvature },
                                                     { "Phosphor Mask", &neon.phosphor } };

    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (this)
                                            .withPosition (position));

    auto themeMenu = yup::PopupMenu::create();
    const auto& themes = SynthThemes::getThemes();

    for (int index = 0; index < static_cast<int> (themes.size()); ++index)
        themeMenu->addItem (themes[static_cast<std::size_t> (index)].name, themeBase + index, true, index == theme);

    menu->addSubMenu ("Theme", themeMenu);

    auto frameRateMenu = yup::PopupMenu::create();

    for (const auto& [name, rate] : { std::pair<const char*, int> { "30 FPS", 30 }, { "60 FPS", 60 }, { "VSync", 0 } })
        frameRateMenu->addItem (name, frameRateBase + rate, true, settings.frameRate == rate);

    menu->addSubMenu ("Frame Rate", frameRateMenu);
    menu->addItem ("Undo History", undoId, true, settings.undo);
    menu->addSeparator();

    for (int index = 0; index < static_cast<int> (std::size (switches)); ++index)
    {
        const auto& [name, value] = switches[index];
        menu->addItem (name, index + 1, index == 0 || settings.postEffect, *value);

        if (index == 0)
            menu->addSeparator();
    }

    std::vector<bool*> values;

    for (const auto& entry : switches)
        values.push_back (entry.second);

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this), values] (int selectedId)
    {
        auto* interface = dynamic_cast<SynthInterface*> (safeThis.get());

        if (interface == nullptr)
            return;

        if (selectedId == undoId)
        {
            auto& undo = SynthSettings::get().undo;
            undo = ! undo;
            SynthSettings::save();
            interface->applyUndoSetting();
            return;
        }

        if (selectedId >= frameRateBase)
        {
            SynthSettings::get().frameRate = selectedId - frameRateBase;
            SynthSettings::save();
            interface->applyFrameRate();
            return;
        }

        if (selectedId >= themeBase)
        {
            if (interface->onThemeChosen != nullptr)
                interface->onThemeChosen (selectedId - themeBase);

            return;
        }

        if (! yup::isPositiveAndBelow (selectedId - 1, static_cast<int> (values.size())))
            return;

        auto* value = values[static_cast<std::size_t> (selectedId - 1)];
        *value = ! *value;
        SynthSettings::save();

        if (interface->onPostEffectChanged != nullptr)
            interface->onPostEffectChanged();
    });
}

yup::ComponentEffect::Ptr SynthInterface::createPostEffect()
{
    const auto& settings = SynthSettings::get();
    return settings.postEffect ? yup::ComponentEffect::Ptr (new SynthNeonEffect (settings.neon)) : nullptr;
}

/** Keys come up from the focused control when it does not use them: the undo shortcuts are
    the interface's, and the rest play the keyboard. */
bool SynthInterface::keyDown (const yup::KeyPress& key, const yup::Point<float>&)
{
    const auto modifiers = key.getModifiers();

    if ((modifiers.isCommandDown() || modifiers.isControlDown()) && key.getKey() == yup::KeyPress::textZKey)
    {
        // A field being typed in is finished first, which records it, so it is the step undone.
        if (auto* native = getNativeComponent(); native != nullptr && dynamic_cast<SynthFieldEditor*> (native->getFocusedComponent()) != nullptr)
            takeKeyboardFocus();

        auto& undoManager = processor.getUndoManager();

        if (modifiers.isShiftDown())
            undoManager.redo();
        else
            undoManager.undo();

        return true;
    }

    return keyboardComponent.handleNoteKey (key, true);
}

bool SynthInterface::keyUp (const yup::KeyPress& key, const yup::Point<float>&)
{
    return keyboardComponent.handleNoteKey (key, false);
}

/** Takes the focus once the interface is in a window, so the keys play it from the start. */
void SynthInterface::attachedToNative()
{
    yup::MessageManager::callAsync ([safeThis = yup::WeakReference<yup::Component> (this)]
    {
        if (auto* interface = safeThis.get())
            interface->takeKeyboardFocus();
    });
}

//==============================================================================
/** Turning undo off in the settings forgets the history. */
void SynthInterface::applyUndoSetting()
{
    auto& undoManager = processor.getUndoManager();
    const auto enabled = SynthSettings::get().undo;

    if (enabled == undoManager.isEnabled())
        return;

    undoManager.clear();
    undoManager.setEnabled (enabled);
}

void SynthInterface::applyFrameRate()
{
    auto* native = getNativeComponent();

    if (native == nullptr)
        return;

    const auto rate = SynthSettings::get().frameRate;
    native->setVsyncEnabled (rate == 0);

    if (rate > 0)
        native->setDesiredFrameRate (static_cast<float> (rate));

    appliedFrameRate = rate;
}

void SynthInterface::timerCallback()
{
    // The window may only exist once the interface is on screen, as in a plugin host.
    if (appliedFrameRate != SynthSettings::get().frameRate)
        applyFrameRate();

    updateModulation();
    syncFromProcessor();

    // A window that lost the focus no longer hears key releases, so held keys let go.
    if (auto* native = getNativeComponent(); native != nullptr && native->getFocusedComponent() == nullptr)
        keyboardComponent.releaseKeyPressNotes();

    const auto playing = processor.getNumActiveVoices() > 0;

    for (auto& panel : oscillatorPanels)
        panel->setAnimating (playing);

    for (int index = 0; index < SynthExample::lfoCount; ++index)
        lfoPanels[static_cast<std::size_t> (index)]->setPhase (processor.getLFOPhase (index));

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
        envelopePanels[static_cast<std::size_t> (index)]->setPlayhead (processor.getEnvelopePosition (index));

    header->syncFromProcessor();
}

/** Shows every route on the controls of its destination, and works out the values the
    most recent note plays, so the oscillators can draw their modulated spectra. */
void SynthInterface::updateModulation()
{
    const auto parameters = processor.getParameters();
    const auto& routes = processor.getRoutes();
    const auto& depths = processor.getSynthParameters().modulation.depths;
    const auto playing = processor.getNumActiveVoices() > 0;
    const auto count = static_cast<std::size_t> (parameters.size());

    modulatedValues.resize (count);
    arcsByParameter.resize (count);

    for (std::size_t index = 0; index < count; ++index)
    {
        modulatedValues[index] = parameters[index]->getValue();
        arcsByParameter[index].clear();
    }

    modulationLive = false;

    // Applied in slot order, each clamped to the range, as the voice applies them.
    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
    {
        const auto route = routes.getRoute (slot);
        const auto depth = depths[static_cast<std::size_t> (slot)]->getValue();

        if (! route.isActive() || ! yup::isPositiveAndBelow (route.destination, static_cast<int> (count)) || depth == 0.0f)
            continue;

        const auto destination = static_cast<std::size_t> (route.destination);
        arcsByParameter[destination].push_back ({ depth, getSourceColor (*this, route.source), isBipolarSource (route.source) });

        const auto amount = playing ? processor.getRouteAmount (slot) : 0.0f;

        if (amount == 0.0f)
            continue;

        const auto& parameter = parameters[destination];
        const auto normalized = yup::jlimit (0.0f, 1.0f, parameter->convertToNormalizedValue (modulatedValues[destination]) + amount);
        modulatedValues[destination] = parameter->convertToDenormalizedValue (normalized);
        modulationLive = true;
    }

    for (auto* target : modulationTargets)
    {
        const auto index = target->getModulationParameter()->getIndexInContainer();

        if (! yup::isPositiveAndBelow (index, static_cast<int> (count)))
            continue;

        const auto& arcs = arcsByParameter[static_cast<std::size_t> (index)];
        const auto& parameter = parameters[static_cast<std::size_t> (index)];
        const auto modulated = playing && ! arcs.empty() ? std::optional<float> (parameter->convertToNormalizedValue (modulatedValues[static_cast<std::size_t> (index)]))
                                                         : std::nullopt;

        target->showModulation (arcs, modulated);
    }
}

void SynthInterface::syncFromProcessor()
{
    const auto modulated = modulationLive ? SynthParameterReader { yup::Span<const float> (modulatedValues.data(), modulatedValues.size()) }
                                          : SynthParameterReader {};

    for (auto& panel : oscillatorPanels)
        panel->syncFromParameters (modulated);

    for (auto& panel : filterPanels)
    {
        panel->setSampleRate (processor.getSampleRate());
        panel->syncFromParameters (modulated);
    }

    for (auto& panel : envelopePanels)
        panel->syncFromParameters();

    for (auto& panel : lfoPanels)
        panel->syncFromParameters();

    fmPanel->syncFromParameters();
    modulationPage->syncFromParameters();
    macroPanel->syncFromProcessor();

    const auto info = processor.getPresetInfo();
    std::array<yup::String, SynthExample::macroCount> macroNames;

    for (int macro = 0; macro < SynthExample::macroCount; ++macro)
        macroNames[static_cast<std::size_t> (macro)] = info.getMacroName (macro);

    modulationPage->setMacroNames (macroNames);

    for (auto* section : { oscillatorSection.get(), macroSection.get(), matrixSection.get(), filterSection.get(), envelopeSection.get(), lfoSection.get() })
        section->syncFromParameters();

    for (auto& led : lfoSyncLeds)
        led->syncFromParameter();

    routingSelector->syncFromParameter();

    voicingPanel->syncFromParameters();

}
