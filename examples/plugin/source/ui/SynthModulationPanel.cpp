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

#include "SynthModulationPanel.h"
#include "SynthRouteEdits.h"

//==============================================================================
class SynthModulationPanel::Row : public yup::Component
{
public:
    Row (SynthModulationPanel& panelToEditIn, int slotToEdit, SynthModulationSource sourceToShow)
        : panel (panelToEditIn)
        , slot (slotToEdit)
        , source (sourceToShow)
        , depthField ({}, panel.depths[static_cast<std::size_t> (slot)], SynthSliderField::Look::bar, true)
    {
        setOpaque (false);
        depthField.setTrackSunken (true);

        removeButton.setTitle ("Remove route");
        removeButton.onClick = [this]
        {
            SynthRouteEdits::clear (panel, panel.routes, slot, panel.depths[static_cast<std::size_t> (slot)]);
        };

        addAndMakeVisible (depthField);
        addAndMakeVisible (removeButton);
    }

    void syncFromParameter() { depthField.syncFromParameter(); }

    void resized() override
    {
        auto bounds = getLocalBounds();
        removeButton.setBounds (bounds.removeFromRight (buttonSize).withSizeKeepingCenter (buttonSize, buttonSize));
        bounds.removeFromRight (SynthTheme::controlGap);
        depthField.setBounds (bounds.removeFromRight (depthWidth));
        nameArea = bounds.withTrimmedLeft (dotSize + SynthTheme::controlGap);
    }

    void paint (yup::Graphics& g) override
    {
        const auto dot = yup::Rectangle<float> (0.0f, (getHeight() - dotSize) * 0.5f, dotSize, dotSize);
        g.setFillColor (getSourceColor (*this, source));
        g.fillRoundedRect (dot, 2.0f);

        g.setFillColor (SynthTheme::textPrimary (*this));
        g.fillFittedText (panel.sourceNames[static_cast<int> (source)], panel.textFont, nameArea, yup::Justification::centerLeft);
    }

private:
    static constexpr float depthWidth = 110.0f;
    static constexpr float buttonSize = 20.0f;
    static constexpr float dotSize = 8.0f;

    SynthModulationPanel& panel;
    int slot;
    SynthModulationSource source;
    SynthSliderField depthField;
    SynthIconButton removeButton { "RemoveRoute", YUP_ICON_XMARK, 11.0f };
    yup::Rectangle<float> nameArea;
};

//==============================================================================
SynthModulationPanel::SynthModulationPanel (SynthRoutes& routesToEdit, const Depths& depthParameters)
    : routes (routesToEdit)
    , depths (depthParameters)
    , sourceNames (getSynthModulationSourceShortNames())
{
    titleFont = SynthTheme::captionFont (12.5f);
    textFont = SynthTheme::valueFont (12.5f);

    setOpaque (false);
    setVisible (false);
}

SynthModulationPanel::~SynthModulationPanel() = default;

void SynthModulationPanel::show (SynthModulationTarget& target)
{
    control = dynamic_cast<yup::Component*> (&target);
    parameter = target.getModulationParameter();

    if (control.get() == nullptr || parameter == nullptr)
        return;

    shownRoutes.clear();
    rebuild (findRoutes());
    setVisible (true);
    toFront (false);
}

void SynthModulationPanel::close()
{
    if (! isOpen())
        return;

    setVisible (false);
    rows.clear();
    shownRoutes.clear();
    control = nullptr;
    parameter = nullptr;
}

void SynthModulationPanel::setMacroNames (const std::array<yup::String, SynthExample::macroCount>& macroNames)
{
    const auto first = static_cast<int> (SynthModulationSource::macro1);

    for (std::size_t macro = 0; macro < macroNames.size(); ++macro)
        sourceNames.set (first + static_cast<int> (macro), macroNames[macro]);

    repaint();
}

void SynthModulationPanel::syncFromRoutes()
{
    if (! isOpen())
        return;

    if (auto* shown = control.get(); shown == nullptr || ! shown->isShowing())
    {
        close();
        return;
    }

    // Rows are rebuilt here, a frame after a remove button was clicked, never inside its click.
    if (auto current = findRoutes(); current != shownRoutes)
        rebuild (current);

    for (auto& row : rows)
        row->syncFromParameter();
}

SynthModulationPanel::Routes SynthModulationPanel::findRoutes() const
{
    Routes found;
    const auto index = parameter->getIndexInContainer();

    for (int slot = 0; slot < SynthExample::modulationSlots; ++slot)
        if (const auto route = routes.getRoute (slot); route.isActive() && route.destination == index)
            found.emplace_back (slot, route.source);

    return found;
}

void SynthModulationPanel::rebuild (const Routes& routesToShow)
{
    shownRoutes = routesToShow;
    rows.clear();

    for (const auto& [slot, source] : shownRoutes)
        addAndMakeVisible (*rows.emplace_back (std::make_unique<Row> (*this, slot, source)));

    placeCard();
}

void SynthModulationPanel::placeCard()
{
    auto* shown = control.get();
    auto* parent = getParentComponent();

    if (shown == nullptr || parent == nullptr)
        return;

    const auto padding = SynthTheme::panelPadding;
    const auto lines = static_cast<float> (yup::jmax<std::size_t> (1, rows.size()) + 1);
    const auto height = padding * 2.0f + lineHeight * lines;
    const auto target = parent->getLocalArea (shown, shown->getLocalBounds());
    const auto bounds = getLocalBounds();

    auto x = target.getRight() + cardGap;
    pointsLeft = x + cardWidth <= bounds.getRight() - padding;

    if (! pointsLeft)
        x = target.getX() - cardGap - cardWidth;

    const auto y = yup::jlimit (bounds.getY() + padding, yup::jmax (bounds.getY() + padding, bounds.getBottom() - padding - height), target.getY());
    card = { yup::jmax (bounds.getX() + padding, x), y, cardWidth, height };

    const auto pointerInset = SynthTheme::panelCorner + pointerSize;
    pointerY = yup::jlimit (card.getY() + pointerInset, card.getBottom() - pointerInset, target.getCenterY());

    auto content = card.reduced (padding);
    content.removeFromTop (lineHeight);

    for (auto& row : rows)
        row->setBounds (content.removeFromTop (lineHeight).reduced (0.0f, 1.0f));

    repaint();
}

void SynthModulationPanel::resized()
{
    placeCard();
}

void SynthModulationPanel::paint (yup::Graphics& g)
{
    if (card.isEmpty() || parameter == nullptr)
        return;

    SynthTheme::paintDropShadow (g, card);
    SynthTheme::paintTile (g, *this, card);
    g.setStrokeColor (SynthTheme::panelBorder (*this));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (card.reduced (0.5f), SynthTheme::panelCorner);

    // A notch on the edge facing the control, pointing at it; its base covers the border there.
    const auto edge = pointsLeft ? card.getX() : card.getRight();
    const auto outwards = pointsLeft ? -1.0f : 1.0f;
    const auto tip = yup::Point<float> (edge + outwards * pointerSize, pointerY);
    const auto base = edge - outwards * 1.5f;

    yup::Path notch;
    notch.addTriangle (base, pointerY - pointerSize, tip.getX(), tip.getY(), base, pointerY + pointerSize);
    g.setFillColor (SynthTheme::stripColor (*this));
    g.fillPath (notch);

    yup::Path outline;
    outline.moveTo (edge + outwards * 0.5f, pointerY - pointerSize);
    outline.lineTo (tip.getX(), tip.getY());
    outline.lineTo (edge + outwards * 0.5f, pointerY + pointerSize);
    g.strokePath (outline);

    auto content = card.reduced (SynthTheme::panelPadding);
    g.setFillColor (SynthTheme::textSecondary (*this));
    g.fillFittedText (parameter->getName().toUpperCase(), titleFont, content.removeFromTop (lineHeight), yup::Justification::centerLeft);

    if (rows.empty())
        g.fillFittedText ("Drag a modulation handle onto this control", textFont, content.removeFromTop (lineHeight), yup::Justification::centerLeft);
}

void SynthModulationPanel::mouseDown (const yup::MouseEvent& event)
{
    if (! card.contains (event.getPosition()))
        close();
}
