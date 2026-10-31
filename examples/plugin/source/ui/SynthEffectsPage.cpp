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

#include "SynthEffectsPage.h"
#include "ParameterChoice.h"
#include "ParameterSegmented.h"
#include "SynthTheme.h"
#include "SynthUndoHistory.h"

#include <algorithm>
#include <functional>

//==============================================================================
/** One effect's controls: its name and choices on a line, an optional display, its knobs in a grid below. */
class SynthEffectsPage::EffectPanel : public yup::Component
{
public:
    explicit EffectPanel (const yup::String& name)
    {
        setOpaque (false);

        title.setText (name.toUpperCase(), yup::dontSendNotification);
        title.setFont (SynthTheme::captionFont (13.5f));
        addAndMakeVisible (title);
    }

    template <typename Control>
    Control& addHeader (std::unique_ptr<Control> control, float width)
    {
        auto& added = *control;
        syncs.push_back ([&added] { added.syncFromParameter(); });
        addAndMakeVisible (added);
        headers.push_back ({ std::move (control), width });
        return added;
    }

    ParameterKnob& addKnob (const yup::String& caption, yup::AudioParameter::Ptr parameter)
    {
        auto& knob = *knobs.emplace_back (std::make_unique<ParameterKnob> (caption, std::move (parameter), SynthTheme::captionFont (12.0f)));
        addAndMakeVisible (knob);
        return knob;
    }

    /** Places a display between the header line and the knobs, which then sit in one row. */
    void setDisplay (yup::Component& displayToShow)
    {
        display = &displayToShow;
        addAndMakeVisible (displayToShow);
    }

    void enablementChanged() override
    {
        repaint();
    }

    void syncFromParameters()
    {
        for (auto& sync : syncs)
            sync();

        for (auto& knob : knobs)
            knob->syncFromParameter();
    }

    void resized() override
    {
        const auto gap = SynthTheme::controlGap;
        auto bounds = getLocalBounds();

        auto line = bounds.removeFromTop (lineHeight);

        for (auto& [control, width] : headers)
        {
            control->setBounds (line.removeFromRight (width));
            line.removeFromRight (gap);
        }

        title.setBounds (line);
        bounds.removeFromTop (gap);

        const auto count = static_cast<int> (knobs.size());

        if (display != nullptr)
        {
            auto knobRow = bounds.removeFromBottom (count > 0 ? yup::jmin (maximumKnobHeight, bounds.getHeight() * 0.4f) : 0.0f);
            bounds.removeFromBottom (gap);
            display->setBounds (bounds);
            bounds = knobRow;
        }

        if (count == 0)
            return;

        const auto columns = display != nullptr || count <= 4 ? count : (count + 1) / 2;
        const auto rows = (count + columns - 1) / columns;
        const auto width = bounds.getWidth() / static_cast<float> (columns);
        const auto height = yup::jmin (maximumKnobHeight, bounds.getHeight() / static_cast<float> (rows));

        for (int index = 0; index < count; ++index)
        {
            const auto column = static_cast<float> (index % columns);
            const auto row = static_cast<float> (index / columns);
            knobs[static_cast<std::size_t> (index)]->setBounds ({ bounds.getX() + column * width, bounds.getY() + row * height, width, height });
        }
    }

private:
    static constexpr float lineHeight = 24.0f;
    static constexpr float maximumKnobHeight = 72.0f;

    yup::Label title;
    yup::Component* display = nullptr;
    std::vector<std::pair<std::unique_ptr<yup::Component>, float>> headers;
    std::vector<std::function<void()>> syncs;
    std::vector<std::unique_ptr<ParameterKnob>> knobs;
};

//==============================================================================
SynthEffectsPage::SynthEffectsPage (const SynthEffectsParameters& parametersToEdit,
                                    SynthEffectChain& chainToEdit,
                                    SynthCurveState& curvesToEdit,
                                    std::shared_ptr<SynthWaveformShader> landscapeShader)
    : parameters (parametersToEdit)
    , chain (chainToEdit)
    , curves (curvesToEdit)
    , shaperEditor (std::move (landscapeShader))
    , shownOrder (chainToEdit.getOrder())
    , orderBeforeDrag (shownOrder)
{
    captionFont = SynthTheme::captionFont (12.0f);

    setCachedToTexture (true);
    setOpaque (false);

    const auto names = getSynthEffectNames();

    for (int index = 0; index < SynthExample::effectCount; ++index)
    {
        const auto type = static_cast<std::size_t> (index);
        leds[type] = std::make_unique<ParameterLed> (parameters.enabled[type]);
        addAndMakeVisible (*leds[type]);

        panels[type] = std::make_unique<EffectPanel> (names[index]);
        addChildComponent (*panels[type]);
    }

    const auto panelFor = [this] (SynthEffectType type) -> EffectPanel& { return *panels[static_cast<std::size_t> (type)]; };

    {
        const auto& p = parameters.bitcrusher;
        auto& panel = panelFor (SynthEffectType::bitcrusher);
        panel.addKnob ("BITS", p.bits);
        panel.addKnob ("RATE", p.rate);
        panel.addKnob ("DITHER", p.dither);
        panel.addKnob ("MIX", p.mix);
    }

    {
        const auto& p = parameters.chorus;
        auto& panel = panelFor (SynthEffectType::chorus);
        panel.addHeader (std::make_unique<ParameterChoice> (yup::String(), p.mode, SynthTheme::valueFont (13.5f)), 108.0f);
        chorusGenericKnobs = { &panel.addKnob ("RATE", p.rate), &panel.addKnob ("DEPTH", p.depth),
                               &panel.addKnob ("DELAY", p.delay), &panel.addKnob ("FEEDBACK", p.feedback) };
        panel.addKnob ("MIX", p.mix);
    }

    {
        const auto& p = parameters.phaser;
        auto& panel = panelFor (SynthEffectType::phaser);
        panel.addHeader (std::make_unique<ParameterSegmented> (p.stages, std::vector<ParameterSegmented::Segment> { { "4" }, { "8" }, { "12" } }), 84.0f);
        panel.addKnob ("RATE", p.rate);
        panel.addKnob ("DEPTH", p.depth);
        panel.addKnob ("CENTER", p.center);
        panel.addKnob ("FEEDBACK", p.feedback);
        panel.addKnob ("STEREO", p.stereo);
        panel.addKnob ("MIX", p.mix);
    }

    {
        const auto& p = parameters.flanger;
        auto& panel = panelFor (SynthEffectType::flanger);
        panel.addKnob ("RATE", p.rate);
        panel.addKnob ("DEPTH", p.depth);
        panel.addKnob ("DELAY", p.delay);
        panel.addKnob ("FEEDBACK", p.feedback);
        panel.addKnob ("STEREO", p.stereo);
        panel.addKnob ("MIX", p.mix);
    }

    {
        const auto& p = parameters.delay;
        auto& panel = panelFor (SynthEffectType::delay);
        panel.addHeader (std::make_unique<ParameterSegmented> (p.pingPong, std::vector<ParameterSegmented::Segment> { { "Stereo" }, { "Ping" } }), 96.0f);
        panel.addHeader (std::make_unique<ParameterLed> (p.tempoSync, "Sync"), 52.0f);
        delayTimeKnob = &panel.addKnob ("TIME", p.time);
        panel.addKnob ("FEEDBACK", p.feedback);
        panel.addKnob ("MIX", p.mix);
        panel.addKnob ("LOW CUT", p.lowCut);
        panel.addKnob ("HIGH CUT", p.highCut);
    }

    {
        const auto& p = parameters.reverb;
        auto& panel = panelFor (SynthEffectType::reverb);
        panel.addKnob ("PREDELAY", p.predelay);
        panel.addKnob ("DECAY", p.decay);
        panel.addKnob ("DAMPING", p.damping);
        panel.addKnob ("WIDTH", p.width);
        panel.addKnob ("MIX", p.mix);
    }

    {
        const auto& p = parameters.distortion;
        auto& panel = panelFor (SynthEffectType::distortion);
        panel.addHeader (std::make_unique<ParameterSegmented> (p.type, std::vector<ParameterSegmented::Segment> { { "Soft" }, { "Cubic" }, { "Hard" } }), 132.0f);
        panel.addKnob ("DRIVE", p.drive);
        panel.addKnob ("BIAS", p.bias);
        panel.addKnob ("TONE", p.tone);
        panel.addKnob ("OUTPUT", p.output);
        panel.addKnob ("MIX", p.mix);
    }

    {
        const auto& p = parameters.equalizer;
        auto& panel = panelFor (SynthEffectType::equalizer);
        panel.addKnob ("LOW", p.lowGain);
        panel.addKnob ("MID", p.midGain);
        panel.addKnob ("HIGH", p.highGain);
        panel.addKnob ("MID Q", p.midQ);
        panel.addKnob ("LOW FREQ", p.lowFrequency);
        panel.addKnob ("MID FREQ", p.midFrequency);
        panel.addKnob ("HIGH FREQ", p.highFrequency);
    }

    {
        const auto& p = parameters.compressor;
        auto& panel = panelFor (SynthEffectType::compressor);
        panel.addKnob ("THRESHOLD", p.threshold);
        panel.addKnob ("RATIO", p.ratio);
        panel.addKnob ("ATTACK", p.attack);
        panel.addKnob ("RELEASE", p.release);
        panel.addKnob ("MAKEUP", p.makeup);
        panel.addKnob ("MIX", p.mix);
    }

    {
        const auto& p = parameters.surround;
        auto& panel = panelFor (SynthEffectType::surround);
        panel.addKnob ("WIDTH", p.width);
        panel.addKnob ("DEPTH", p.depth);
        panel.addKnob ("TIME", p.time);
        panel.addKnob ("MONO BELOW", p.monoBelow);
        panel.addKnob ("MIX", p.mix);
    }

    {
        const auto& p = parameters.waveshaper;
        auto& panel = panelFor (SynthEffectType::waveshaper);

        shaperEditor.setTransfer (true);
        shaperEditor.onCurveEdited = [this] (const SynthCurveState::Curve& curve) { curves.setShaper (curve); };
        shaperEditor.onEditFinished = [this] (const SynthCurveState::Curve& before)
        {
            const auto after = curves.getShaper();

            if (after == before)
                return;

            SynthUndoHistory::record<SynthCurveState::Curve> (*this, before, after, [&curves = curves] (const SynthCurveState::Curve& curve)
            {
                curves.setShaper (curve);
            });
        };

        panel.setDisplay (shaperEditor);
        panel.addKnob ("DRIVE", p.drive);
        panel.addKnob ("BIAS", p.bias);
        panel.addKnob ("OUTPUT", p.output);
        panel.addKnob ("MIX", p.mix);
    }

    selectEffect (selected);
    syncFromParameters();
}

SynthEffectsPage::~SynthEffectsPage() = default;

//==============================================================================
void SynthEffectsPage::syncFromParameters()
{
    for (auto& led : leds)
        led->syncFromParameter();

    const auto values = parameters.read();

    delayTimeKnob->setParameter (values.delay.tempoSync ? parameters.delay.division : parameters.delay.time);

    for (auto* knob : chorusGenericKnobs)
        knob->setEnabled (values.chorus.mode == SynthChorusMode::generic);

    for (std::size_t index = 0; index < panels.size(); ++index)
    {
        panels[index]->setEnabled (values.enabled[index]);
        panels[index]->syncFromParameters();
    }

    if (const auto generation = curves.getGeneration(); generation != shownCurveGeneration)
    {
        shownCurveGeneration = generation;
        shaperEditor.setCurve (curves.getShaper());
    }

    if (const auto order = chain.getOrder(); order != shownOrder && draggedRow < 0)
    {
        shownOrder = order;
        layoutRows();
        repaint();
    }
}

void SynthEffectsPage::selectEffect (SynthEffectType type)
{
    selected = type;

    for (int index = 0; index < SynthExample::effectCount; ++index)
        panels[static_cast<std::size_t> (index)]->setVisible (index == static_cast<int> (type));

    repaint();
}

//==============================================================================
yup::Rectangle<float> SynthEffectsPage::getListBounds() const
{
    return getLocalBounds().removeFromLeft (yup::jmin (maximumListWidth, getWidth() * 0.38f));
}

yup::Rectangle<float> SynthEffectsPage::getRowBounds (int row) const
{
    const auto list = getListBounds();
    const auto height = yup::jmin (maximumRowHeight, list.getHeight() / static_cast<float> (SynthExample::effectCount));
    return { list.getX(), list.getY() + static_cast<float> (row) * height, list.getWidth(), height };
}

int SynthEffectsPage::getRowAt (float y) const
{
    const auto height = getRowBounds (0).getHeight();
    return height > 0.0f ? yup::jlimit (0, SynthExample::effectCount - 1, static_cast<int> ((y - getListBounds().getY()) / height)) : 0;
}

void SynthEffectsPage::layoutRows()
{
    for (int row = 0; row < SynthExample::effectCount; ++row)
    {
        const auto area = getRowBounds (row);
        const auto led = area.withWidth (area.getHeight());
        leds[static_cast<std::size_t> (shownOrder[static_cast<std::size_t> (row)])]->setBounds (led);
    }
}

void SynthEffectsPage::resized()
{
    layoutRows();

    auto bounds = getLocalBounds();
    bounds.removeFromLeft (getListBounds().getWidth() + SynthTheme::controlGap * 2.0f);

    for (auto& panel : panels)
        panel->setBounds (bounds);
}

void SynthEffectsPage::paint (yup::Graphics& g)
{
    const auto list = getListBounds().withHeight (getRowBounds (SynthExample::effectCount - 1).getBottom());
    SynthTheme::paintWell (g, *this, list);

    const auto names = getSynthEffectNames();

    for (int row = 0; row < SynthExample::effectCount; ++row)
    {
        const auto type = shownOrder[static_cast<std::size_t> (row)];
        auto area = getRowBounds (row);

        if (type == selected)
            SynthTheme::paintRaised (g, *this, area.reduced (2.0f), 3.0f);

        area.removeFromLeft (area.getHeight());

        // A grip on the right shows the rows can be dragged.
        auto grip = area.removeFromRight (area.getHeight() * 0.6f).withSizeKeepingCenter (8.0f, 7.0f);
        g.setFillColor (SynthTheme::textSecondary (*this).withAlpha (0.6f));

        for (int line = 0; line < 3; ++line)
            g.fillRect (grip.getX(), grip.getY() + static_cast<float> (line) * 3.0f, grip.getWidth(), 1.0f);

        g.setFillColor (type == selected ? SynthTheme::textPrimary (*this) : SynthTheme::textSecondary (*this));
        g.fillFittedText (names[static_cast<int> (type)].toUpperCase(), captionFont, area, yup::Justification::centerLeft);
    }
}

void SynthEffectsPage::mouseDown (const yup::MouseEvent& event)
{
    const auto position = event.getPosition();

    if (! getListBounds().contains (position) || position.getY() >= getRowBounds (SynthExample::effectCount - 1).getBottom())
        return;

    draggedRow = getRowAt (position.getY());
    orderBeforeDrag = shownOrder;
    selectEffect (shownOrder[static_cast<std::size_t> (draggedRow)]);
}

void SynthEffectsPage::mouseDrag (const yup::MouseEvent& event)
{
    if (draggedRow < 0)
        return;

    const auto row = getRowAt (event.getPosition().getY());

    if (row == draggedRow)
        return;

    auto order = shownOrder;
    const auto moved = order.begin() + draggedRow;

    if (row < draggedRow)
        std::rotate (order.begin() + row, moved, moved + 1);
    else
        std::rotate (moved, moved + 1, order.begin() + row + 1);

    chain.setOrder (order);

    shownOrder = order;
    draggedRow = row;
    layoutRows();
    repaint();
}

void SynthEffectsPage::mouseUp (const yup::MouseEvent&)
{
    if (std::exchange (draggedRow, -1) < 0 || shownOrder == orderBeforeDrag)
        return;

    SynthUndoHistory::record<SynthEffectOrder> (*this, orderBeforeDrag, shownOrder, [&chain = chain] (const SynthEffectOrder& order)
    {
        chain.setOrder (order);
    });
}
