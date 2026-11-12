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
#include "ParameterGesture.h"
#include "SynthNeonEffect.h"
#include "SynthRouteEdits.h"
#include "SynthKeyboardLook.h"
#include "SynthTheme.h"
#include "SynthSettings.h"
#include "SynthThemes.h"

#include "../dsp/SynthPatch.h"
#include "../mcp/RetraceMCPServer.h"

namespace
{

//==============================================================================
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

/** True when a source swings both ways, an envelope switched to bipolar included. */
bool isRouteBipolar (SynthModulationSource source, const SynthParameters& parameters)
{
    if (const auto envelope = getSourceEnvelope (source); envelope >= 0)
    {
        const auto& polarity = parameters.envelopes[static_cast<std::size_t> (envelope)].polarity;
        return polarity != nullptr && polarity->getValue() > 0.5f;
    }

    return isBipolarSource (source);
}

/** The post effect switches of the settings menu, by their menu names. */
std::array<std::pair<const char*, bool*>, 10> getViewSwitches (SynthViewOptions& options)
{
    auto& neon = options.neon;

    return { { { "Post Effect", &options.postEffect },
               { "Neon Glow", &neon.glow },
               { "Chromatic Aberration", &neon.aberration },
               { "Color Grade", &neon.grade },
               { "Scanlines", &neon.scanlines },
               { "Vignette", &neon.vignette },
               { "Grain", &neon.grain },
               { "Tube Curvature", &neon.curvature },
               { "Phosphor Mask", &neon.phosphor },
               { "Glass Reflection", &neon.reflection } } };
}

} // namespace

//==============================================================================
SynthInterface::SynthInterface (RetraceProcessor& processorToEdit, yup::AudioProcessorEditor& ownerEditor, const SynthViewOptions& optionsToShow)
    : processor (processorToEdit)
    , editor (ownerEditor)
    , theme (optionsToShow.getThemeIndex())
    , viewOptions (optionsToShow)
    , keyboardComponent (processor.getKeyboardState(), yup::MidiKeyboardComponent::horizontalKeyboard)
    , pitchWheelComponent (processor.getKeyboardState(), "PitchWheel")
    , modWheelComponent (processor.getKeyboardState(), "ModWheel")
    , keyboardToolbar (processor.getTuning(), keyboardComponent)
{
    setPopupContainer (true);

    wheelCaptionFont = SynthTheme::captionFont (10.5f);

    const auto& parameters = processor.getSynthParameters();

    keyboardComponent.setAvailableRange (24, 96); // C1 to C7
    keyboardComponent.setVisibleRange (36, 84);   // C2 to C6
    keyboardComponent.setMidiChannel (1);
    keyboardComponent.setVelocity (0.7f);
    SynthKeyboardLook::apply (keyboardComponent);
    SynthTheme::applySection (keyboardComponent, SynthTheme::Section::keys);
    mainPage.addAndMakeVisible (keyboardComponent);

    keyboardComponent.setWantsKeyboardFocus (false);
    setWantsKeyboardFocus (true);

    applyUndoSetting();

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

    for (auto* handle : { &pitchBendHandle, &modWheelHandle })
        mainPage.addAndMakeVisible (*handle);

    SynthTheme::applySection (keyboardToolbar, SynthTheme::Section::keys);
    mainPage.addAndMakeVisible (keyboardToolbar);

    header = std::make_unique<SynthHeader> (processor);
    header->onSettingsRequested = [this] (yup::Point<float> position) { showSettingsMenu (getLocalPoint (header.get(), position)); };
    header->onBrowserToggled = [this] { setBrowserOpen (! isBrowserOpen()); };
    header->onUndoRequested = [this] (bool redo) { undoStep (redo); };
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
        filterPanels[static_cast<std::size_t> (index)] = std::make_unique<SynthFilterPanel> (parameters.filters[static_cast<std::size_t> (index)], waveformShader);

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
        envelopePanels[static_cast<std::size_t> (index)] = std::make_unique<SynthEnvelopePanel> (parameters.envelopes[static_cast<std::size_t> (index)], processor.getCurves(), index, waveformShader);

    for (int index = 0; index < SynthExample::lfoCount; ++index)
        lfoPanels[static_cast<std::size_t> (index)] = std::make_unique<SynthLFOPanel> (parameters.lfos[static_cast<std::size_t> (index)], processor.getCurves(), index, waveformShader);

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
    effectsPage = std::make_unique<SynthEffectsPage> (parameters.effects, processor.getEffectChain(), processor.getCurves(), waveformShader);

    const auto gridCaption = yup::String::fromUTF8 ("ROW \xe2\x86\x90 COLUMN");
    matrixCaption = makeCaption (gridCaption);
    modulationCaption = makeCaption (gridCaption);
    matrixSection = std::make_unique<SynthSection> (std::vector<SynthSection::Tab> { { "FM", fmPanel.get(), nullptr, false, { { matrixCaption.get(), 110.0f }, { &fmPanel->getResetButton(), 60.0f }, { &fmPanel->getAlgorithmButton(), 88.0f } } },
                                                                                     { "Mods", modulationPage.get(), nullptr, false, { { modulationCaption.get(), 110.0f }, { &modulationPage->getResetButton(), 60.0f } } },
                                                                                     { "EFX", effectsPage.get() } },
                                                    SynthTheme::Section::matrix);

    macroPanel = std::make_unique<SynthMacroPanel> (processor);
    macroSection = std::make_unique<SynthSection> (std::vector<SynthSection::Tab> { { "Macros", macroPanel.get() } }, SynthTheme::Section::macros);

    std::vector<SynthSection::Tab> filterTabs;

    for (int index = 0; index < SynthExample::filterCount; ++index)
        filterTabs.push_back ({ "Filter " + yup::String (index + 1), filterPanels[static_cast<std::size_t> (index)].get(), parameters.filters[static_cast<std::size_t> (index)].enabled });

    routingSelector = std::make_unique<ParameterSegmented> (parameters.filterRouting, std::vector<ParameterSegmented::Segment> { { "Serial" }, { "Par" } });
    swapFiltersButton.setTitle ("Swap filters");
    swapFiltersButton.onClick = [this]
    {
        swapFilters();
    };
    filterSection = std::make_unique<SynthSection> (std::move (filterTabs), SynthTheme::Section::filters, std::vector<SynthSection::StripControl> { { routingSelector.get(), 96.0f }, { &swapFiltersButton, 28.0f } });

    std::vector<SynthSection::Tab> envelopeTabs;

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
    {
        const auto& envelope = parameters.envelopes[static_cast<std::size_t> (index)];
        auto& panel = *envelopePanels[static_cast<std::size_t> (index)];
        envelopeTabs.push_back ({ "Env " + yup::String (index + 1), &panel, envelope.enabled, envelope.enabled == nullptr, { { &panel.getSyncLed(), 56.0f } }, &panel.getModulationHandle() });
    }

    envelopeSection = std::make_unique<SynthSection> (std::move (envelopeTabs), SynthTheme::Section::envelopes);

    std::vector<SynthSection::Tab> lfoTabs;

    for (int index = 0; index < SynthExample::lfoCount; ++index)
    {
        const auto& lfo = parameters.lfos[static_cast<std::size_t> (index)];
        auto& syncLed = lfoSyncLeds.emplace_back (std::make_unique<ParameterLed> (lfo.tempoSync, "Sync"));

        auto& panel = *lfoPanels[static_cast<std::size_t> (index)];
        lfoTabs.push_back ({ "LFO " + yup::String (index + 1), &panel, lfo.enabled, false, { { syncLed.get(), 56.0f } }, &panel.getModulationHandle() });
    }

    lfoSection = std::make_unique<SynthSection> (std::move (lfoTabs), SynthTheme::Section::lfos);

    for (auto* section : { oscillatorSection.get(), macroSection.get(), matrixSection.get(), filterSection.get(), envelopeSection.get(), lfoSection.get() })
        mainPage.addAndMakeVisible (*section);

    addAndMakeVisible (mainPage);

    voicingPanel = std::make_unique<SynthVoicingPanel> (parameters);
    mainPage.addAndMakeVisible (*voicingPanel);

    presetBrowser = std::make_unique<SynthPresetBrowser> (processor);
    presetBrowser->onPresetChosen = [this] { setBrowserOpen (false); };
    SynthTheme::applySection (*presetBrowser, SynthTheme::Section::header);
    mainPage.addChildComponent (*presetBrowser);

    collectModulationTargets (*this, modulationTargets);

    modulationPanel = std::make_unique<SynthModulationPanel> (processor.getRoutes(), parameters.modulation.depths);
    addChildComponent (*modulationPanel);

    SynthThemes::applyTheme (*this, theme);

    panelsByParameter.resize (processor.getParameters().size());

    const auto watchState = [] (auto getState)
    {
        auto shown = getState();
        return [getState = std::move (getState), shown = std::move (shown)]() mutable
        {
            const auto state = getState();
            return std::exchange (shown, state) != state;
        };
    };

    auto& curves = processor.getCurves();
    const auto curvesGeneration = [&curves] { return curves.getGeneration(); };

    for (int index = 0; index < SynthExample::oscillatorCount; ++index)
    {
        auto* panel = oscillatorPanels[static_cast<std::size_t> (index)].get();
        auto partialsMoved = watchState ([&partials = processor.getPartials (index)] { return partials.getGeneration(); });

        addPanelSync (
            *panel,
            parameters.oscillators[static_cast<std::size_t> (index)].getParameters(),
            [this, panel] { panel->syncFromParameters (getModulatedReader()); },
            [panel, partialsMoved]() mutable { return partialsMoved() || panel->hasPendingEdits(); },
            true);
    }

    for (int index = 0; index < SynthExample::filterCount; ++index)
    {
        auto* panel = filterPanels[static_cast<std::size_t> (index)].get();
        addPanelSync (
            *panel,
            parameters.filters[static_cast<std::size_t> (index)].getParameters(),
            [this, panel] { panel->syncFromParameters (getModulatedReader()); },
            {},
            true);
    }

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
    {
        auto* panel = envelopePanels[static_cast<std::size_t> (index)].get();
        addPanelSync (
            *panel,
            parameters.envelopes[static_cast<std::size_t> (index)].getParameters(),
            [this, panel] { panel->syncFromParameters (getModulatedReader()); },
            watchState (curvesGeneration),
            true);
    }

    for (int index = 0; index < SynthExample::lfoCount; ++index)
    {
        auto* panel = lfoPanels[static_cast<std::size_t> (index)].get();
        addPanelSync (
            *panel,
            parameters.lfos[static_cast<std::size_t> (index)].getParameters(),
            [panel] { panel->syncFromParameters(); },
            watchState (curvesGeneration));
    }

    auto fmParameters = parameters.fm.getParameters();

    for (const auto& oscillator : parameters.oscillators)
        fmParameters.insert (fmParameters.end(), { oscillator.level, oscillator.enabled });

    addPanelSync (*fmPanel, fmParameters, [this]
    {
        fmPanel->syncFromParameters();
    });

    auto& routes = processor.getRoutes();
    addPanelSync (
        *modulationPage,
        parameters.modulation.getParameters(),
        [this] { modulationPage->syncFromParameters(); },
        watchState ([&routes] { return routes.getGeneration(); }));

    addPanelSync (
        *effectsPage,
        parameters.effects.getParameters(),
        [this] { effectsPage->syncFromParameters(); },
        [this, curvesMoved = watchState (curvesGeneration)]() mutable { return curvesMoved() || effectsPage->hasPendingOrderChange(); });

    for (const auto& parameter : processor.getParameters())
    {
        const auto index = static_cast<std::size_t> (parameter->getIndexInContainer());
        parameterAttachments.push_back (std::make_unique<yup::AudioParameterAttachment> (editor, parameter, [this, index]
        {
            parametersChanged = true;

            for (const auto panel : panelsByParameter[index])
                panelSyncs[panel].dirty = true;
        }, false));
    }

    syncControls();
    syncMacroNames();
}

SynthInterface::~SynthInterface()
{
}

//==============================================================================
void SynthInterface::resized()
{
    auto frameBounds = getLocalBounds().reduced (SynthTheme::frameGap);
    header->setRightGroupWidth (std::round (frameBounds.getWidth() * SynthTheme::rightColumnShare));
    header->setBounds (frameBounds.removeFromTop (SynthTheme::headerHeight));

    frameBounds.removeFromTop (SynthTheme::frameGap);
    mainPage.setBounds (frameBounds);

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
    auto handles = wheels.removeFromTop (wheelHandleSize + 4.0f).withTrimmedBottom (4.0f);
    pitchBendHandle.setBounds (handles.removeFromLeft (wheelWidth).withSizeKeepingCenter (wheelHandleSize, wheelHandleSize));
    modWheelHandle.setBounds (handles.removeFromRight (wheelWidth).withSizeKeepingCenter (wheelHandleSize, wheelHandleSize));
    pitchWheelComponent.setBounds (wheels.removeFromLeft (wheelWidth));
    modWheelComponent.setBounds (wheels.removeFromRight (wheelWidth));

    auto keys = keyboardTile.reduced (8.0f);
    keyboardToolbar.setBounds (keys.removeFromTop (keyboardToolbarHeight));
    keys.removeFromTop (6.0f);
    keysWell = keys.enlarged (2.0f);
    keyboardComponent.setBounds (keys.withTrimmedTop (2.0f));

    wheelsTile = wheelsTile.withPosition (wheelsTile.getPosition() + offset);
    keyboardTile = keyboardTile.withPosition (keyboardTile.getPosition() + offset);
    keysWell = keysWell.withPosition (keysWell.getPosition() + offset);

    presetBrowser->setBounds (bounds);

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

    modulationPanel->setBounds (getLocalBounds());
}

//==============================================================================
void SynthInterface::refreshDisplay (double)
{
    const auto playing = processor.getNumActiveVoices() > 0;
    const auto routeGeneration = processor.getRoutes().getGeneration();
    const auto routesChanged = std::exchange (shownRouteGeneration, routeGeneration) != routeGeneration;
    const auto parametersMoved = std::exchange (parametersChanged, false);

    if (parametersMoved || routesChanged || (modulationShown && (playing || wasPlaying)))
        updateModulation();

    wasPlaying = playing;

    if (parametersMoved)
        syncControls();

    syncPanels (std::exchange (modulationWasLive, modulationLive) || modulationLive);
    syncMacroNames();
    modulationPanel->syncFromRoutes();

    if (dragSource.has_value() && ! getDraggedSource().has_value())
        clearDrag();

    if (auto* native = getNativeComponent(); native != nullptr && native->getFocusedComponent() == nullptr)
        keyboardComponent.releaseKeyPressNotes();

    for (auto& panel : oscillatorPanels)
        panel->setAnimating (playing);

    const auto sampleRate = processor.getSampleRate();
    const auto transport = processor.getTransport();

    for (auto& panel : filterPanels)
    {
        panel->setAnimating (playing);
        panel->setSampleRate (sampleRate);
    }

    for (auto& panel : envelopePanels)
    {
        panel->setAnimating (playing);
        panel->setTransport (transport);
    }

    for (auto& panel : lfoPanels)
    {
        panel->setAnimating (playing);
        panel->setTransport (transport);
    }

    for (int index = 0; index < SynthExample::lfoCount; ++index)
        lfoPanels[static_cast<std::size_t> (index)]->setPhase (processor.getLFOPhase (index));

    for (int index = 0; index < SynthExample::envelopeCount; ++index)
        envelopePanels[static_cast<std::size_t> (index)]->setPlayhead (processor.getEnvelopePosition (index));

    macroPanel->syncFromProcessor();
    header->syncFromProcessor();

    if (isBrowserOpen())
        presetBrowser->syncFromProcessor();
}

//==============================================================================
void SynthInterface::paint (yup::Graphics& g)
{
    g.setFillColor (SynthTheme::frameColor (*this));
    g.fillAll();

    SynthTheme::paintTile (g, keyboardComponent, wheelsTile);
    SynthTheme::paintTile (g, keyboardComponent, keyboardTile);
    SynthTheme::paintWell (g, keyboardComponent, keysWell);

    const auto captions = wheelsTile.reduced (12.0f, 8.0f).withTrimmedTop (wheelsTile.getHeight() - 16.0f - wheelCaptionHeight);
    g.setFillColor (SynthTheme::textSecondary (keyboardComponent));
    g.fillFittedText ("PITCH", wheelCaptionFont, captions.withWidth (wheelWidth).withSizeKeepingCenter (wheelWidth + 12.0f, captions.getHeight()), yup::Justification::center);
    g.fillFittedText ("MOD", wheelCaptionFont, captions.withTrimmedLeft (captions.getWidth() - wheelWidth).withSizeKeepingCenter (wheelWidth + 12.0f, captions.getHeight()), yup::Justification::center);
}

void SynthInterface::paintOverChildren (yup::Graphics& g)
{
    if (! dragSource.has_value())
        return;

    constexpr auto corner = 6.0f;
    const auto color = getSourceColor (*this, *dragSource);
    const auto areas = getDropAreas();

    // Everything but the controls the source can reach recedes
    yup::Path dimmed;
    dimmed.setUsingNonZeroWinding (false);
    dimmed.addRectangle (getLocalBounds());

    for (const auto& [target, area] : areas)
        dimmed.addRoundedRectangle (area, corner);

    g.setFillColor (yup::Colors::black.withAlpha (SynthTheme::isDark (*this) ? 0.45f : 0.25f));
    g.fillPath (dimmed);

    for (const auto& [target, area] : areas)
    {
        const auto hovered = target == dropTarget;
        const auto strokeWidth = hovered ? 2.5f : 1.5f;

        g.setFillColor (color.withAlpha (hovered ? 0.3f : 0.1f));
        g.fillRoundedRect (area, corner);

        g.setStrokeColor (color.withAlpha (hovered ? 1.0f : 0.75f));
        g.setStrokeWidth (strokeWidth);
        g.strokeRoundedRect (area.reduced (strokeWidth * 0.5f), corner - strokeWidth * 0.5f);
    }
}

//==============================================================================
void SynthInterface::attachedToNative()
{
    yup::MessageManager::callAsync ([safeThis = yup::WeakReference<yup::Component> (this)]
    {
        if (auto* interface = dynamic_cast<SynthInterface*> (safeThis.get()))
        {
            interface->applyFrameRate();
            interface->takeKeyboardFocus();
        }
    });
}

//==============================================================================
std::vector<std::pair<SynthModulationTarget*, yup::Rectangle<float>>> SynthInterface::getDropAreas() const
{
    constexpr auto margin = 3.0f;
    constexpr auto gap = 2.0f;

    std::vector<std::pair<SynthModulationTarget*, yup::Rectangle<float>>> areas;

    for (auto* target : modulationTargets)
    {
        auto* component = dynamic_cast<yup::Component*> (target);

        if (component != nullptr && component->isShowing() && canRoute (*dragSource, *target))
            areas.emplace_back (target, getLocalArea (component, component->getLocalBounds()).enlarged (margin));
    }

    for (std::size_t i = 0; i < areas.size(); ++i)
    {
        for (std::size_t j = i + 1; j < areas.size(); ++j)
        {
            auto& a = areas[i].second;
            auto& b = areas[j].second;
            const auto overlap = a.intersection (b);

            if (overlap.isEmpty())
                continue;

            auto& first = (overlap.getWidth() <= overlap.getHeight() ? a.getCenterX() <= b.getCenterX() : a.getCenterY() <= b.getCenterY()) ? a : b;
            auto& second = &first == &a ? b : a;

            if (overlap.getWidth() <= overlap.getHeight())
            {
                const auto middle = overlap.getCenterX();
                first = first.withTrimmedRight (first.getRight() - middle + gap * 0.5f);
                second = second.withTrimmedLeft (middle + gap * 0.5f - second.getX());
            }
            else
            {
                const auto middle = overlap.getCenterY();
                first = first.withTrimmedBottom (first.getBottom() - middle + gap * 0.5f);
                second = second.withTrimmedTop (middle + gap * 0.5f - second.getY());
            }
        }
    }

    return areas;
}

std::optional<SynthModulationSource> SynthInterface::getDraggedSource() const
{
    auto* manager = yup::DragAndDropManager::getInstanceWithoutCreating();

    if (manager == nullptr || ! manager->isDragging() || manager->getCurrentDragData().getText() != SynthModulationHandle::dragText)
        return std::nullopt;

    auto* handle = dynamic_cast<SynthModulationHandle*> (manager->getCurrentDragSourceComponent());

    if (handle == nullptr || handle->getParentComponentWithType<SynthInterface>() != this)
        return std::nullopt;

    return handle->getSource();
}

bool SynthInterface::canRoute (SynthModulationSource source, const SynthModulationTarget& target) const
{
    const auto parameter = target.getModulationParameter();

    if (parameter == nullptr || SynthPatch::findRouteDestination (processor.getParameters(), parameter->getID()) < 0)
        return false;

    const auto& routes = processor.getRoutes();
    return routes.findRoute (source, parameter->getID()) < 0 && routes.findFreeSlot() >= 0;
}

SynthModulationTarget* SynthInterface::findTargetAt (yup::Point<float> position) const
{
    for (auto* target : modulationTargets)
        if (auto* component = dynamic_cast<yup::Component*> (target); component != nullptr && component->isShowing()
                                                                      && getLocalArea (component, component->getLocalBounds()).contains (position))
            return target;

    return nullptr;
}

void SynthInterface::hoverDrag (yup::Point<float> position)
{
    const auto source = isBrowserOpen() ? std::nullopt : getDraggedSource();
    auto* target = source.has_value() ? findTargetAt (position) : nullptr;

    if (target != nullptr && ! canRoute (*source, *target))
        target = nullptr;

    if (source == dragSource && target == dropTarget)
        return;

    dragSource = source;
    dropTarget = target;
    repaint();
}

void SynthInterface::clearDrag()
{
    if (! dragSource.has_value() && dropTarget == nullptr)
        return;

    dragSource.reset();
    dropTarget = nullptr;
    repaint();
}

bool SynthInterface::isInterestedInDragSource (const yup::DragAndDropSourceDetails& details)
{
    return details.data.getText() == SynthModulationHandle::dragText && getDraggedSource().has_value();
}

void SynthInterface::itemDragEnter (const yup::DragAndDropSourceDetails& details)
{
    hoverDrag (details.localPosition);
}

void SynthInterface::itemDragMove (const yup::DragAndDropSourceDetails& details)
{
    hoverDrag (details.localPosition);
}

void SynthInterface::itemDragExit (const yup::DragAndDropSourceDetails&)
{
    if (dropTarget == nullptr)
        return;

    dropTarget = nullptr;
    repaint();
}

bool SynthInterface::itemDropped (const yup::DragAndDropSourceDetails& details)
{
    hoverDrag (details.localPosition);

    const auto source = dragSource;
    auto* target = dropTarget;
    clearDrag();

    if (! source.has_value() || target == nullptr)
        return false;

    auto* component = dynamic_cast<yup::Component*> (target);
    auto& routes = processor.getRoutes();
    const auto slot = routes.findFreeSlot();
    const auto parameter = target->getModulationParameter();
    const auto destination = SynthPatch::findRouteDestination (processor.getParameters(), parameter->getID());

    if (component == nullptr || slot < 0 || destination < 0)
        return false;

    SynthRouteEdits::write (*component, routes, slot, { static_cast<int> (*source), destination, parameter->getID() });

    const auto& depth = processor.getSynthParameters().modulation.depths[static_cast<std::size_t> (slot)];
    ParameterGesture (*component, depth).setNormalized (depth->convertToNormalizedValue (initialDepth), true);

    return true;
}

//==============================================================================
void SynthInterface::setBrowserOpen (bool shouldBeOpen)
{
    if (shouldBeOpen == isBrowserOpen())
        return;

    modulationPanel->close();

    for (auto* section : { oscillatorSection.get(), macroSection.get(), matrixSection.get(), filterSection.get(), envelopeSection.get(), lfoSection.get() })
        section->setVisible (! shouldBeOpen);

    presetBrowser->setVisible (shouldBeOpen);
    header->setBrowserOpen (shouldBeOpen);

    if (shouldBeOpen)
        presetBrowser->focusList();
    else
        takeKeyboardFocus();
}

//==============================================================================
void SynthInterface::showModulationPanel (SynthModulationTarget& target)
{
    const auto parameter = target.getModulationParameter();

    if (isBrowserOpen() || parameter == nullptr || SynthPatch::findRouteDestination (processor.getParameters(), parameter->getID()) < 0)
        return;

    modulationPanel->show (target);
}

//==============================================================================
void SynthInterface::showSettingsMenu (yup::Point<float> position)
{
    constexpr int themeBase = 1000;
    constexpr int frameRateBase = 2000;
    constexpr int undoId = 3000;
    constexpr int landscapesId = 3001;
    constexpr int mcpServerId = 3002;
    constexpr int mcpCopyUrlId = 3003;
    constexpr int saveDefaultsId = 3004;
    constexpr int audioSettingsId = 3005;
    constexpr int scaleBase = 4000;

    auto* standaloneHost = yup::StandalonePluginHost::getInstance();

    const auto& settings = SynthSettings::get();
    auto shown = processor.getViewOptions();
    const auto switches = getViewSwitches (shown);

    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (this)
                                            .withPosition (position));

    auto scaleMenu = yup::PopupMenu::create();
    const auto currentScale = getScalePercent != nullptr ? getScalePercent() : 100;

    for (const auto percent : { 70, 80, 90, 100, 110, 120, 130 })
        scaleMenu->addItem (yup::String (percent) + "%", scaleBase + percent, true, percent == currentScale);

    menu->addSubMenu ("Scale", scaleMenu);

    auto frameRateMenu = yup::PopupMenu::create();

    for (const auto& [name, rate] : { std::pair<const char*, int> { "30 FPS", 30 }, { "60 FPS", 60 }, { "VSync", 0 } })
        frameRateMenu->addItem (name, frameRateBase + rate, true, shown.frameRate == rate);

    menu->addSubMenu ("Framerate", frameRateMenu);

    auto themeMenu = yup::PopupMenu::create();
    const auto& themes = SynthThemes::getThemes();

    for (int index = 0; index < static_cast<int> (themes.size()); ++index)
        themeMenu->addItem (themes[static_cast<std::size_t> (index)].name, themeBase + index, true, index == shown.getThemeIndex());

    menu->addSubMenu ("Theme", themeMenu);

    auto effectsMenu = yup::PopupMenu::create();
    effectsMenu->addItem ("Landscape Displays", landscapesId, true, shown.landscapes);
    effectsMenu->addItem ("Post Processing", 1, true, shown.postEffect);

    auto individualMenu = yup::PopupMenu::create();

    for (int index = 1; index < static_cast<int> (switches.size()); ++index)
    {
        const auto& [name, value] = switches[static_cast<std::size_t> (index)];
        individualMenu->addItem (name, index + 1, shown.postEffect, *value);
    }

    effectsMenu->addSubMenu ("Individual Effects", individualMenu, shown.postEffect);
    menu->addSubMenu ("Effects", effectsMenu);

    if (standaloneHost != nullptr)
    {
        menu->addSeparator();
        menu->addItem ("Audio Settings...", audioSettingsId);
    }

    menu->addSeparator();

    auto* mcpServer = processor.getMCPServer();
    const auto mcpUrl = mcpServer != nullptr ? mcpServer->getUrl() : yup::String();
    auto mcpMenu = yup::PopupMenu::create();
    mcpMenu->addItem ("MCP Server", mcpServerId, mcpServer != nullptr, settings.mcpServer);
    mcpMenu->addItem (mcpUrl.isNotEmpty() ? "Copy " + mcpUrl : yup::String ("Copy MCP Server URL"), mcpCopyUrlId, mcpUrl.isNotEmpty(), false);
    menu->addSubMenu ("MCP", mcpMenu);

    menu->addSeparator();
    menu->addItem ("Undo History", undoId, true, settings.undo);
    menu->addItem ("Save as Default", saveDefaultsId);

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this)] (int selectedId)
    {
        auto* interface = dynamic_cast<SynthInterface*> (safeThis.get());

        if (interface == nullptr)
            return;

        if (selectedId >= scaleBase)
        {
            if (interface->onScalePercentRequested != nullptr)
                interface->onScalePercentRequested (selectedId - scaleBase);

            return;
        }

        if (selectedId == undoId)
        {
            auto& undo = SynthSettings::get().undo;
            undo = ! undo;
            SynthSettings::save();
            interface->applyUndoSetting();
            return;
        }

        if (selectedId == mcpServerId)
        {
            auto& mcpServer = SynthSettings::get().mcpServer;
            mcpServer = ! mcpServer;
            SynthSettings::save();
            RetraceMCPServer::setEnabledEverywhere (mcpServer);
            return;
        }

        if (selectedId == mcpCopyUrlId)
        {
            if (auto* server = interface->processor.getMCPServer())
                yup::SystemClipboard::copyTextToClipboard (server->getUrl());

            return;
        }

        if (selectedId == saveDefaultsId)
        {
            SynthSettings::get().defaults = interface->processor.getViewOptions();
            SynthSettings::save();
            return;
        }

        if (selectedId == audioSettingsId)
        {
            if (auto* host = yup::StandalonePluginHost::getInstance())
                host->showAudioSettings();

            return;
        }

        interface->changeViewOptions ([selectedId] (SynthViewOptions& options)
        {
            if (selectedId == landscapesId)
                options.landscapes = ! options.landscapes;
            else if (selectedId >= frameRateBase)
                options.frameRate = selectedId - frameRateBase;
            else if (selectedId >= themeBase)
                options.theme = SynthThemes::getThemes()[static_cast<std::size_t> (selectedId - themeBase)].name;
            else if (const auto switches = getViewSwitches (options); yup::isPositiveAndBelow (selectedId - 1, static_cast<int> (switches.size())))
                *switches[static_cast<std::size_t> (selectedId - 1)].second = ! *switches[static_cast<std::size_t> (selectedId - 1)].second;
        });
    });
}

//==============================================================================
void SynthInterface::changeViewOptions (const std::function<void (SynthViewOptions&)>& change)
{
    auto options = processor.getViewOptions();
    change (options);
    processor.setViewOptions (options);

    if (onViewOptionsChanged != nullptr)
        onViewOptionsChanged();
}

void SynthInterface::setViewOptions (const SynthViewOptions& newOptions)
{
    viewOptions = newOptions;
    applyFrameRate();
    repaint();
}

//==============================================================================
yup::ComponentEffect::Ptr SynthInterface::createPostEffect (const SynthViewOptions& options)
{
    return options.postEffect ? yup::ComponentEffect::Ptr (new SynthNeonEffect (options.neon)) : nullptr;
}

//==============================================================================
bool SynthInterface::keyDown (const yup::KeyPress& key, const yup::Point<float>&)
{
    if (key.getKey() == yup::KeyPress::escapeKey && modulationPanel->isOpen())
    {
        modulationPanel->close();
        return true;
    }

    if (key.getKey() == yup::KeyPress::escapeKey && isBrowserOpen())
    {
        setBrowserOpen (false);
        return true;
    }

    const auto modifiers = key.getModifiers();

    if ((modifiers.isCommandDown() || modifiers.isControlDown()) && key.getKey() == yup::KeyPress::textZKey)
    {
        undoStep (modifiers.isShiftDown());
        return true;
    }

    return keyboardComponent.handleNoteKey (key, true);
}

bool SynthInterface::keyUp (const yup::KeyPress& key, const yup::Point<float>&)
{
    return keyboardComponent.handleNoteKey (key, false);
}

//==============================================================================
void SynthInterface::applyUndoSetting()
{
    auto& undoManager = processor.getUndoManager();
    const auto enabled = SynthSettings::get().undo;

    if (enabled == undoManager.isEnabled())
        return;

    undoManager.clear();
    undoManager.setEnabled (enabled);
}

void SynthInterface::undoStep (bool redo)
{
    // A field being typed in is finished first, which records it, so it is the step undone.
    if (auto* native = getNativeComponent(); native != nullptr && dynamic_cast<SynthFieldEditor*> (native->getFocusedComponent()) != nullptr)
        takeKeyboardFocus();

    auto& undoManager = processor.getUndoManager();

    if (redo)
        undoManager.redo();
    else
        undoManager.undo();
}

//==============================================================================
void SynthInterface::applyFrameRate()
{
    auto* native = getNativeComponent();

    if (native == nullptr)
        return;

    const auto rate = viewOptions.frameRate;

    if (rate == appliedFrameRate)
        return;

    native->setVsyncEnabled (rate == 0);

    // VSync (rate 0) drops the fixed rate so pacing follows the display refresh; 30/60 fix it.
    native->setDesiredFrameRate (rate > 0 ? std::optional<float> (static_cast<float> (rate)) : std::nullopt);

    appliedFrameRate = rate;
}

//==============================================================================
void SynthInterface::updateModulation()
{
    const auto parameters = processor.getParameters();
    const auto& routes = processor.getRoutes();
    const auto& synthParameters = processor.getSynthParameters();
    const auto& depths = synthParameters.modulation.depths;
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
    modulationShown = false;

    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
    {
        const auto route = routes.getRoute (slot);
        const auto depth = depths[static_cast<std::size_t> (slot)]->getValue();

        if (! route.isActive() || ! yup::isPositiveAndBelow (route.destination, static_cast<int> (count)) || depth == 0.0f)
            continue;

        const auto destination = static_cast<std::size_t> (route.destination);
        arcsByParameter[destination].push_back ({ depth, getSourceColor (*this, route.source), isRouteBipolar (route.source, synthParameters) });
        modulationShown = true;

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

//==============================================================================
void SynthInterface::addPanelSync (yup::Component& component,
                                   yup::Span<const yup::AudioParameter::Ptr> parameters,
                                   std::function<void()> sync,
                                   std::function<bool()> poll,
                                   bool followsModulation)
{
    const auto index = panelSyncs.size();
    panelSyncs.push_back ({ &component, std::move (sync), std::move (poll), followsModulation });

    for (const auto& parameter : parameters)
        panelsByParameter[static_cast<std::size_t> (parameter->getIndexInContainer())].push_back (index);
}

void SynthInterface::syncPanels (bool modulationMoved)
{
    for (auto& panel : panelSyncs)
    {
        if (panel.poll != nullptr && panel.poll())
            panel.dirty = true;

        if (panel.followsModulation && modulationMoved)
            panel.dirty = true;

        if (! panel.dirty || ! panel.component->isShowing())
            continue;

        panel.dirty = false;
        panel.sync();
    }
}

void SynthInterface::syncControls()
{
    for (auto* section : { oscillatorSection.get(), macroSection.get(), matrixSection.get(), filterSection.get(), envelopeSection.get(), lfoSection.get() })
        section->syncFromParameters();

    for (auto& led : lfoSyncLeds)
        led->syncFromParameter();

    routingSelector->syncFromParameter();
    voicingPanel->syncFromParameters();
}

void SynthInterface::syncMacroNames()
{
    auto info = processor.getPresetInfo();

    if (info == shownPresetInfo)
        return;

    std::array<yup::String, SynthExample::macroCount> macroNames;

    for (int macro = 0; macro < SynthExample::macroCount; ++macro)
        macroNames[static_cast<std::size_t> (macro)] = info.getMacroName (macro);

    modulationPage->setMacroNames (macroNames);
    modulationPanel->setMacroNames (macroNames);
    shownPresetInfo = std::move (info);
}

//==============================================================================
void SynthInterface::swapFilters()
{
    static_assert (SynthExample::filterCount == 2);

    const auto& filters = processor.getSynthParameters().filters;

    auto& control = swapFiltersButton;
    SynthUndoHistory::beginStep (control);

    for (const auto& [first, second] : { std::pair { filters[0].type, filters[1].type },
                                         std::pair { filters[0].cutoff, filters[1].cutoff },
                                         std::pair { filters[0].resonance, filters[1].resonance },
                                         std::pair { filters[0].drive, filters[1].drive },
                                         std::pair { filters[0].keytrack, filters[1].keytrack },
                                         std::pair { filters[0].mix, filters[1].mix },
                                         std::pair { filters[0].enabled, filters[1].enabled } })
    {
        const auto firstValue = first->getNormalizedValue();
        ParameterGesture (control, first).setNormalized (second->getNormalizedValue(), true);
        ParameterGesture (control, second).setNormalized (firstValue, true);
    }

    auto& routes = processor.getRoutes();
    std::vector<std::pair<int, SynthRouteEdits::RouteState>> moved;

    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
    {
        auto state = SynthRouteEdits::read (routes, slot);
        const auto other = state.destinationID.startsWith ("flt1.") ? "flt2." : state.destinationID.startsWith ("flt2.") ? "flt1."
                                                                                                                         : nullptr;

        if (state.destination < 0 || other == nullptr)
            continue;

        state.destinationID = yup::String (other) + state.destinationID.substring (5);
        state.destination = SynthPatch::findRouteDestination (processor.getParameters(), state.destinationID);
        moved.emplace_back (slot, state);
    }

    for (const auto& [slot, state] : moved)
        SynthRouteEdits::write (control, routes, slot, state, true);
}

//==============================================================================
SynthParameterReader SynthInterface::getModulatedReader() const
{
    return modulationLive ? SynthParameterReader { yup::Span<const float> (modulatedValues.data(), modulatedValues.size()) }
                          : SynthParameterReader {};
}
