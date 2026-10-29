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

#include "SynthFMPanel.h"
#include "../dsp/SynthAlgorithms.h"
#include "SynthTheme.h"
#include "SynthUndoHistory.h"

namespace
{
/** Shows a normalized amount as a whole percentage inside the knob's well. */
void showPercentInWell (ParameterKnob& knob, const yup::AudioParameter::Ptr& parameter)
{
    knob.getKnob().setShowsValueInWell (true);
    knob.getKnob().getValueText = [parameter]
    {
        return yup::String (yup::roundToInt (parameter->getNormalizedValue() * 100.0f));
    };
}
} // namespace

SynthFMPanel::SynthFMPanel (const SynthParameters& parametersToEdit)
    : parameters (parametersToEdit)
{
    captionFont = SynthTheme::captionFont (11.5f);

    setCachedToTexture (true);

    setOpaque (false);

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
        {
            const auto& parameter = parameters.fm.get (carrier, modulator);
            auto& knob = knobs[static_cast<std::size_t> (carrier)][static_cast<std::size_t> (modulator)];
            knob = std::make_unique<ParameterKnob> (yup::String(), parameter, SynthTheme::captionFont (11.0f));
            showPercentInWell (*knob, parameter);

            if (carrier == modulator)
                knob->getKnob().setSourceAccent (1);

            addAndMakeVisible (*knob);
        }

        const auto& level = parameters.oscillators[static_cast<std::size_t> (carrier)].level;
        auto& levelKnob = levelKnobs[static_cast<std::size_t> (carrier)];
        levelKnob = std::make_unique<ParameterKnob> (yup::String(), level, SynthTheme::captionFont (11.0f));
        showPercentInWell (*levelKnob, level);
        addAndMakeVisible (*levelKnob);
    }

    addAndMakeVisible (graph);

    SynthTheme::styleRaisedButton (algorithmButton);
    algorithmButton.onClick = [this] { showAlgorithmMenu(); };

    SynthTheme::styleRaisedButton (resetButton);
    resetButton.onClick = [this] { resetMatrix(); };

    syncFromParameters();
}

void SynthFMPanel::syncFromParameters()
{
    for (auto& row : knobs)
        for (auto& knob : row)
            knob->syncFromParameter();

    for (auto& knob : levelKnobs)
        knob->syncFromParameter();

    graph.setState (readState());
}

SynthFMGraph::State SynthFMPanel::readState() const
{
    SynthFMGraph::State state;

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        const auto index = static_cast<std::size_t> (carrier);
        const auto& oscillator = parameters.oscillators[index];

        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
            state.amounts[index][static_cast<std::size_t> (modulator)] = parameters.fm.get (carrier, modulator)->getNormalizedValue();

        state.levels[index] = oscillator.level->getNormalizedValue();
        state.enabled[index] = oscillator.enabled->getValue() > 0.5f;
    }

    return state;
}

int SynthFMPanel::findAlgorithm() const
{
    const auto state = readState();
    return findSynthAlgorithm (state.amounts, state.levels);
}

void SynthFMPanel::showAlgorithmMenu()
{
    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (getPopupParentComponent())
                                            .withRelativePosition (&algorithmButton));

    const auto current = findAlgorithm();

    for (int index = 0; index < static_cast<int> (getSynthAlgorithms().size()); ++index)
        menu->addItem ("Algorithm " + yup::String (index + 1), index + 1, true, index == current);

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this)] (int selectedId)
    {
        if (auto* panel = dynamic_cast<SynthFMPanel*> (safeThis.get()); panel != nullptr && selectedId > 0)
            panel->applyAlgorithm (selectedId - 1);
    });
}

/** Wires the links and outputs of an algorithm, as one undo step.

    Links already on keep their amount and new ones start at newLinkAmount; operators
    already heard keep their level. Feedback is left as it is, and every operator is
    switched on, since every algorithm uses all four.
*/
void SynthFMPanel::applyAlgorithm (int index)
{
    const auto& algorithm = getSynthAlgorithms()[static_cast<std::size_t> (index)];

    const auto isLink = [&algorithm] (int carrier, int modulator)
    {
        return std::any_of (algorithm.links.begin(), algorithm.links.end(), [&] (const SynthAlgorithm::Link& link)
        {
            return link.modulator == modulator + 1 && link.carrier == carrier + 1;
        });
    };

    const auto isOutput = [&algorithm] (int op)
    {
        return std::find (algorithm.outputs.begin(), algorithm.outputs.end(), op + 1) != algorithm.outputs.end();
    };

    const auto set = [this] (const yup::AudioParameter::Ptr& parameter, float normalized)
    {
        ParameterGesture (*this, parameter).setNormalized (normalized, true);
    };

    SynthUndoHistory::beginStep (*this);

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
        {
            if (modulator == carrier)
                continue;

            const auto& parameter = parameters.fm.get (carrier, modulator);
            const auto current = parameter->getNormalizedValue();

            set (parameter, isLink (carrier, modulator) ? (current > 0.0f ? current : parameter->convertToNormalizedValue (newLinkAmount)) : 0.0f);
        }

        const auto& oscillator = parameters.oscillators[static_cast<std::size_t> (carrier)];
        const auto level = oscillator.level->getNormalizedValue();

        set (oscillator.level, isOutput (carrier) ? (level > 0.0f ? level : newOutputLevel) : 0.0f);
        set (oscillator.enabled, 1.0f);
    }

    syncFromParameters();
}

/** Returns every amount of the matrix, feedback included, to its default, as one undo step. */
void SynthFMPanel::resetMatrix()
{
    SynthUndoHistory::beginStep (*this);

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
        {
            const auto& parameter = parameters.fm.get (carrier, modulator);
            ParameterGesture (*this, parameter).setNormalized (parameter->convertToNormalizedValue (parameter->getDefaultValue()), true);
        }
    }

    syncFromParameters();
}

void SynthFMPanel::resized()
{
    auto bounds = getLocalBounds();
    auto graphArea = bounds.removeFromBottom (std::round (bounds.getHeight() * graphShare));
    bounds.removeFromBottom (SynthTheme::controlGap);
    bounds.removeFromTop (labelSize);
    bounds.removeFromLeft (labelSize);
    grid = bounds;

    const auto columns = static_cast<float> (SynthExample::oscillatorCount + 1);
    const auto rows = static_cast<float> (SynthExample::oscillatorCount);
    const auto cellWidth = grid.getWidth() / columns;
    const auto cellHeight = grid.getHeight() / rows;
    const auto size = yup::jmin (cellWidth, cellHeight) - SynthTheme::controlGap;

    const auto cellAt = [&] (int row, int column)
    {
        return yup::Rectangle<float> { grid.getX() + cellWidth * static_cast<float> (column),
                                       grid.getY() + cellHeight * static_cast<float> (row),
                                       cellWidth,
                                       cellHeight }
            .withSizeKeepingCenter (size, size);
    };

    SynthFMGraph::Row nodePositions {};

    for (int carrier = 0; carrier < SynthExample::oscillatorCount; ++carrier)
    {
        for (int modulator = 0; modulator < SynthExample::oscillatorCount; ++modulator)
            knobs[static_cast<std::size_t> (carrier)][static_cast<std::size_t> (modulator)]->setBounds (cellAt (carrier, modulator));

        levelKnobs[static_cast<std::size_t> (carrier)]->setBounds (cellAt (carrier, SynthExample::oscillatorCount));
        nodePositions[static_cast<std::size_t> (carrier)] = grid.getX() + cellWidth * (static_cast<float> (carrier) + 0.5f) - graphArea.getX();
    }

    graph.setBounds (graphArea);
    graph.setNodePositions (nodePositions);
}

void SynthFMPanel::paint (yup::Graphics& g)
{
    const auto count = SynthExample::oscillatorCount;
    const auto cellWidth = grid.getWidth() / static_cast<float> (count + 1);
    const auto cellHeight = grid.getHeight() / static_cast<float> (count);

    g.setFillColor (SynthTheme::textSecondary (*this));

    for (int index = 0; index < count; ++index)
    {
        const auto offset = static_cast<float> (index);
        g.fillFittedText ("OP " + yup::String (index + 1), captionFont, { grid.getX() + cellWidth * offset, 0.0f, cellWidth, labelSize }, yup::Justification::center);
        g.fillFittedText (yup::String (index + 1), captionFont, { 0.0f, grid.getY() + cellHeight * offset, labelSize, cellHeight }, yup::Justification::centerLeft);
    }

    const auto outX = grid.getX() + cellWidth * static_cast<float> (count);
    g.fillFittedText ("OUT", captionFont, { outX, 0.0f, cellWidth, labelSize }, yup::Justification::center);

    SynthTheme::paintSeam (g, *this, { outX, grid.getY() + 4.0f }, { outX, grid.getBottom() - 4.0f });
}
