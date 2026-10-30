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

#include "SynthOscillatorPanel.h"
#include "SynthUndoHistory.h"
#include "SynthTheme.h"

SynthOscillatorPanel::SynthOscillatorPanel (const SynthOscillatorParameters& parametersToEdit,
                                            SynthPartials& partialsToEdit,
                                            const SynthOscillatorResources& resources,
                                            std::shared_ptr<SynthWaveformShader> waveformShader)
    : parameters (parametersToEdit)
    , partials (partialsToEdit)
    , editor (parametersToEdit, partialsToEdit, resources, std::move (waveformShader))
    , waveformChoice ({}, parametersToEdit.waveform, SynthTheme::valueFont (13.5f))
    , frequencyModeSelector (parametersToEdit.frequencyMode, { { "Ratio" }, { "Fixed" } })
    , pitchField ({}, parametersToEdit.ratio)
    , fineField ("Fine", parametersToEdit.fine)
    , freeRunningLed (parametersToEdit.freeRunning, "Free")
    , syncModeChoice ({}, parametersToEdit.syncMode, SynthTheme::valueFont (13.5f))
    , syncRatioField ({}, parametersToEdit.syncRatio)
    , levelKnob ("LEVEL", parametersToEdit.level, SynthTheme::captionFont (12.0f))
    , panKnob ("PAN", parametersToEdit.pan, SynthTheme::captionFont (12.0f))
    , phaseKnob ("PHASE", parametersToEdit.phase, SynthTheme::captionFont (12.0f))
    , sends { SynthSliderField ("To F1", parametersToEdit.toFilter1, SynthSliderField::Look::bar),
              SynthSliderField ("To F2", parametersToEdit.toFilter2, SynthSliderField::Look::bar),
              SynthSliderField ("Direct", parametersToEdit.direct, SynthSliderField::Look::bar) }
    , spectrum { SynthSliderField ("Ridges", parametersToEdit.ridgeSpacing, SynthSliderField::Look::boxed),
                 SynthSliderField ("Color", parametersToEdit.color, SynthSliderField::Look::boxed),
                 SynthSliderField ("Dispersion", parametersToEdit.dispersion, SynthSliderField::Look::boxed),
                 SynthSliderField ("Squeeze", parametersToEdit.squeeze, SynthSliderField::Look::boxed),
                 SynthSliderField ("Squash", parametersToEdit.squash, SynthSliderField::Look::boxed),
                 SynthSliderField ("Tilt", parametersToEdit.tilt, SynthSliderField::Look::boxed, true),
                 SynthSliderField ("Odd / Even", parametersToEdit.oddEven, SynthSliderField::Look::boxed, true),
                 SynthSliderField ("Formant", parametersToEdit.formant, SynthSliderField::Look::boxed),
                 SynthSliderField ("F. Pos", parametersToEdit.formantPosition, SynthSliderField::Look::boxed),
                 SynthSliderField ("Scatter", parametersToEdit.scatter, SynthSliderField::Look::boxed) }
    , drawTools (makeDrawToolSegments(), true)
    , partialsTools (makePartialsToolSegments(), true)
{
    captionFont = SynthTheme::captionFont (11.5f);

    setCachedToTexture (true);

    setOpaque (false);

    addAndMakeVisible (editor);

    // Picking a preset drops any edited partials, otherwise the oscillator would keep
    // playing the edited shape while the combo box claims something else.
    waveformChoice.onChange = [this] { editor.revertToPreset (true); };

    partialsButton.setButtonText ("Partials");
    SynthTheme::styleRaisedToggle (partialsButton);
    partialsButton.onClick = [this] { showEditingPartials (partialsButton.getToggleState()); };

    // In the order of WaveformDrawTool and PartialsTool.
    drawTools.setSelectedIndex (0);
    drawTools.onSelect = [this] (int index) { editor.setDrawTool (static_cast<WaveformDrawTool> (index)); };

    partialsTools.setSelectedIndex (0);
    partialsTools.onSelect = [this] (int index) { editor.setPartialsTool (static_cast<PartialsTool> (index)); };

    toolsMenuButton.onClick = [this] { showToolsMenu(); };

    SynthTheme::styleRaisedButton (resetButton);
    resetButton.onClick = [this] { resetShape(); };

    for (auto* control : std::initializer_list<yup::Component*> { &waveformChoice, &frequencyModeSelector, &pitchField, &fineField, &freeRunningLed,
                                                                  &syncModeChoice, &syncRatioField, &partialsButton, &resetButton, &levelKnob, &panKnob, &phaseKnob,
                                                                  &drawTools, &toolsMenuButton })
        addAndMakeVisible (*control);

    addChildComponent (partialsTools);

    for (auto& field : sends)
        addAndMakeVisible (field);

    for (auto& field : spectrum)
        addAndMakeVisible (field);

    shownValues = parameters.read();
    partials.fillValues (shownValues);
    showModes (shownValues);
}

void SynthOscillatorPanel::showModes (const SynthOscillatorValues& values)
{
    pitchField.setParameter (values.frequencyMode == SynthFrequencyMode::fixed ? parameters.fixedHz : parameters.ratio);
    syncRatioField.setEnabled (values.syncMode != yup::SyncMode::none);
}

void SynthOscillatorPanel::showEditingPartials (bool shouldEdit)
{
    editor.setEditingPartials (shouldEdit);
    drawTools.setVisible (! shouldEdit);
    partialsTools.setVisible (shouldEdit);
}

/** Opens the operations on the whole cycle, the grid and the undo history. */
void SynthOscillatorPanel::showToolsMenu()
{
    // Operation IDs are 1 based in the order of PartialsOperation; grid IDs are gridBase
    // plus the divisions, 0 standing for none.
    constexpr int gridBase = 100;
    constexpr int undoId = 200;
    constexpr int redoId = 201;

    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (getPopupParentComponent())
                                            .withRelativePosition (&toolsMenuButton, yup::PopupMenu::Placement::toRight()));

    const char* const operations[] { "Invert", "Reverse", "Mirror", "Odd Harmonics Only", "Even Harmonics Only", "Randomize Phases" };

    for (int index = 0; index < static_cast<int> (std::size (operations)); ++index)
    {
        menu->addItem (operations[index], index + 1);

        if (index == 2)
            menu->addSeparator();
    }

    menu->addSeparator();

    auto gridMenu = yup::PopupMenu::create();

    for (const auto divisions : { 0, 4, 8, 16, 32 })
        gridMenu->addItem (divisions == 0 ? yup::String ("Off") : yup::String (divisions), gridBase + divisions, true, editor.getGridDivisions() == divisions);

    menu->addSubMenu ("Grid", gridMenu, ! editor.isEditingPartials());
    menu->addSeparator();
    auto* history = SynthUndoHistory::find (*this);
    menu->addItem ("Undo", undoId, history != nullptr && history->canUndo());
    menu->addItem ("Redo", redoId, history != nullptr && history->canRedo());

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this)] (int selectedId)
    {
        auto* panel = dynamic_cast<SynthOscillatorPanel*> (safeThis.get());

        if (panel == nullptr || selectedId <= 0)
            return;

        auto& editor = panel->editor;

        auto* history = SynthUndoHistory::find (*panel);

        if (selectedId == undoId && history != nullptr)
            history->undo();
        else if (selectedId == redoId && history != nullptr)
            history->redo();
        else if (selectedId >= gridBase)
            editor.setGridDivisions (selectedId - gridBase);
        else
            editor.applyOperation (static_cast<PartialsOperation> (selectedId - 1));
    });
}

std::vector<SynthSegmented::Segment> SynthOscillatorPanel::makeDrawToolSegments()
{
    const auto polyline = &SynthSegmented::makePolyline;

    yup::Path brush;
    brush.moveTo (0.0f, 0.7f);
    brush.cubicTo (0.2f, -0.1f, 0.45f, 1.1f, 0.65f, 0.45f);
    brush.cubicTo (0.75f, 0.15f, 0.9f, 0.2f, 1.0f, 0.35f);

    yup::Path arc;
    arc.moveTo (0.0f, 1.0f);
    arc.cubicTo (0.05f, -0.3f, 0.95f, -0.3f, 1.0f, 1.0f);

    // In the order of WaveformDrawTool.
    return { { "Brush", brush },
             { "Line", polyline ({ { 0.0f, 1.0f }, { 1.0f, 0.0f } }) },
             { "Arc", arc },
             { "Zigzag", polyline ({ { 0.0f, 1.0f }, { 0.25f, 0.0f }, { 0.5f, 1.0f }, { 0.75f, 0.0f }, { 1.0f, 1.0f } }) },
             { "Steps", polyline ({ { 0.0f, 1.0f }, { 0.33f, 1.0f }, { 0.33f, 0.5f }, { 0.67f, 0.5f }, { 0.67f, 0.0f }, { 1.0f, 0.0f } }) },
             { "Noise", polyline ({ { 0.0f, 0.5f }, { 0.12f, 0.1f }, { 0.25f, 0.8f }, { 0.37f, 0.3f }, { 0.5f, 1.0f }, { 0.62f, 0.0f }, { 0.75f, 0.7f }, { 0.87f, 0.2f }, { 1.0f, 0.6f } }) } };
}

std::vector<SynthSegmented::Segment> SynthOscillatorPanel::makePartialsToolSegments()
{
    yup::Path magnitude;
    yup::Path phase;

    for (const auto& [x, top] : { std::pair { 0.1f, 0.0f }, { 0.37f, 0.4f }, { 0.63f, 0.6f }, { 0.9f, 0.75f } })
        magnitude.addLine ({ x, 1.0f }, { x, top });

    for (const auto& [x, end] : { std::pair { 0.1f, 0.0f }, { 0.37f, 0.8f }, { 0.63f, 0.25f }, { 0.9f, 1.0f } })
        phase.addLine ({ x, 0.5f }, { x, end });

    // In the order of PartialsTool.
    return { { "Magnitude", magnitude }, { "Phase", phase } };
}

/** Returns the drawn partials to the waveform and every spectral modifier to its default, as one undo step. */
void SynthOscillatorPanel::resetShape()
{
    SynthUndoHistory::beginStep (*this);
    editor.revertToPreset (true);

    for (auto& field : spectrum)
    {
        const auto& parameter = field.getModulationParameter();

        if (parameter->getValue() != parameter->getDefaultValue())
            ParameterGesture (*this, parameter).setNormalized (parameter->convertToNormalizedValue (parameter->getDefaultValue()), true);
    }

    syncFromParameters();
}

void SynthOscillatorPanel::syncFromParameters (SynthParameterReader modulated)
{
    waveformChoice.syncFromParameter();
    frequencyModeSelector.syncFromParameter();
    pitchField.syncFromParameter();
    fineField.syncFromParameter();
    freeRunningLed.syncFromParameter();
    syncModeChoice.syncFromParameter();
    syncRatioField.syncFromParameter();

    for (auto* knob : { &levelKnob, &panKnob, &phaseKnob })
        knob->syncFromParameter();

    for (auto& field : sends)
        field.syncFromParameter();

    for (auto& field : spectrum)
        field.syncFromParameter();

    // One generation bump per frame, however many mouse events the drag produced.
    editor.commitPendingEdits();

    // While the partials are drawn by hand the wave follows the hand, not the modulation.
    if (editor.isEditingPartials())
        modulated = {};

    auto values = parameters.read (modulated);
    partials.fillValues (values);

    // A hidden tab catches up once it shows, as its values still differ from the shown ones.
    if (values == shownValues || ! isShowing())
        return;

    shownValues = values;
    editor.refresh (modulated);
    showModes (values);
}

void SynthOscillatorPanel::resized()
{
    const auto gap = SynthTheme::controlGap;
    auto bounds = getLocalBounds();

    auto spectrumArea = bounds.removeFromRight (std::round (bounds.getWidth() * spectrumShare));
    bounds.removeFromRight (SynthTheme::panelPadding);

    const auto rows = 5;
    const auto columnWidth = (spectrumArea.getWidth() - gap) * 0.5f;
    const auto rowHeight = (spectrumArea.getHeight() - gap * static_cast<float> (rows - 1)) / static_cast<float> (rows);

    for (int index = 0; index < static_cast<int> (spectrum.size()); ++index)
    {
        const auto column = static_cast<float> (index % 2);
        const auto row = static_cast<float> (index / 2);
        spectrum[static_cast<std::size_t> (index)].setBounds ({ spectrumArea.getX() + column * (columnWidth + gap),
                                                                spectrumArea.getY() + row * (rowHeight + gap),
                                                                columnWidth,
                                                                rowHeight });
    }

    auto controls = bounds.removeFromBottom (controlsHeight);
    bounds.removeFromBottom (gap);

    displayArea = bounds;
    auto display = bounds;
    topBar = display.removeFromTop (barHeight);
    bottomBar = display.removeFromBottom (barHeight);
    toolRail = display.removeFromLeft (railWidth).withTrimmedLeft (1.0f);
    editor.setBounds (display.withTrimmedRight (1.0f));

    auto rail = toolRail.reduced (2.0f, 4.0f);
    toolsMenuButton.setBounds (rail.removeFromBottom (toolHeight));
    rail.removeFromBottom (4.0f);
    drawTools.setBounds (rail.withHeight (yup::jmin (rail.getHeight(), toolHeight * 6.0f)));
    partialsTools.setBounds (rail.withHeight (yup::jmin (rail.getHeight(), toolHeight * 2.0f)));

    auto top = topBar.reduced (5.0f, 4.0f);
    waveformChoice.setBounds (top.removeFromLeft (130.0f));
    top.removeFromLeft (gap);
    frequencyModeSelector.setBounds (top.removeFromLeft (110.0f));
    top.removeFromLeft (gap);
    pitchField.setBounds (top.removeFromLeft (84.0f));
    top.removeFromLeft (gap);
    fineField.setBounds (top.removeFromLeft (96.0f));

    auto bottom = bottomBar.reduced (8.0f, 4.0f);
    freeRunningLed.setBounds (bottom.removeFromLeft (52.0f));
    bottom.removeFromLeft (gap * 2.0f);
    syncCaptionArea = bottom.removeFromLeft (34.0f);
    syncModeChoice.setBounds (bottom.removeFromLeft (84.0f));
    bottom.removeFromLeft (gap);
    syncRatioField.setBounds (bottom.removeFromLeft (64.0f));
    resetButton.setBounds (bottom.removeFromRight (64.0f));
    bottom.removeFromRight (gap);
    partialsButton.setBounds (bottom.removeFromRight (78.0f));

    auto knobs = controls.removeFromLeft (190.0f);
    levelKnob.setBounds (knobs.removeFromLeft (72.0f));
    panKnob.setBounds (knobs.removeFromLeft (59.0f).reduced (0.0f, 8.0f));
    phaseKnob.setBounds (knobs.reduced (0.0f, 8.0f));

    controls.removeFromLeft (gap * 2.0f);
    seamX = controls.getX();
    controls.removeFromLeft (gap * 2.0f);

    auto sendArea = controls.reduced (0.0f, 12.0f);
    const auto sendHeight = sendArea.getHeight() / static_cast<float> (sends.size());

    for (auto& field : sends)
        field.setBounds (sendArea.removeFromTop (sendHeight));
}

void SynthOscillatorPanel::paint (yup::Graphics& g)
{
    SynthTheme::paintWell (g, *this, displayArea, SynthTheme::wellCorner + 1.0f);

    for (const auto& bar : { topBar, bottomBar })
    {
        g.setFillColor (SynthTheme::stripColor (*this).withAlpha (0.6f));
        g.fillRect (bar.reduced (1.0f, 0.0f));
    }

    g.setFillColor (SynthTheme::stripColor (*this).withAlpha (0.6f));
    g.fillRect (toolRail);

    g.setFillColor (yup::Colors::black.withAlpha (0.7f));
    g.fillRect (toolRail.getRight() - 1.0f, toolRail.getY(), 1.0f, toolRail.getHeight());
    g.fillRect (topBar.getX() + 1.0f, topBar.getBottom() - 1.0f, topBar.getWidth() - 2.0f, 1.0f);
    g.fillRect (bottomBar.getX() + 1.0f, bottomBar.getY(), bottomBar.getWidth() - 2.0f, 1.0f);

    g.setFillColor (SynthTheme::textSecondary (*this));
    g.fillFittedText ("SYNC", captionFont, syncCaptionArea, yup::Justification::centerLeft);

    const auto controlsTop = displayArea.getBottom() + SynthTheme::controlGap + 10.0f;
    SynthTheme::paintSeam (g, *this, { seamX, controlsTop }, { seamX, getHeight() - 10.0f });
}
