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


#include "yup_YuiToolGuides.h"
#include "yup_YuiToolSupport.h"

#include <algorithm>
#include <cmath>
#include <iterator>

using namespace yup;

namespace
{

const Color boundsColor (0xff3794ff);
const Color distanceColor (0xffffa500);

// The components of an interface written at a place of the file it was loaded from, which are many for what a
// component declared in the file holds: one for each time it is used
void collectWrittenAt (YuiDocument& document, Component& component, YuiToolPlace place, std::vector<WeakReference<Component>>& found)
{
    const auto sourceMap = document.getSourceMap();

    if (const auto element = document.getBuilder().findElementFor (&component); element.isValid() && sourceMap != nullptr)
    {
        YuiDiagnostic probe;
        probe.element = element;

        const auto located = sourceMap->locate (probe);

        if (located.source.isEmpty() && located.line == place.line && located.column == place.column)
            found.emplace_back (&component);
    }

    for (int index = 0; index < component.getNumChildComponents(); ++index)
        collectWrittenAt (document, *component.getChildComponent (index), place, found);
}

bool isInside (const Component& component, const Component& root)
{
    for (const auto* held = &component; held != nullptr; held = held->getParentComponent())
        if (held == &root)
            return true;

    return false;
}

String describeLength (float length)
{
    return std::abs (length - std::round (length)) < 0.05f ? String (roundToInt (length)) : String (length, 1);
}

} // namespace

//==============================================================================

YuiToolGuides::YuiToolGuides()
{
    setOpaque (false);
}

void YuiToolGuides::setDocument (YuiDocument* newDocument, const File& newInput)
{
    document = newDocument;
    input = newInput;

    // What was picked of an interface that is gone leaves the caret to say what is shown
    picked.erase (std::remove_if (picked.begin(), picked.end(), [] (const WeakReference<Component>& component)
    {
        return component == nullptr;
    }), picked.end());

    hasPicked = hasPicked && ! picked.empty();

    findAtCaret();
    repaint();
}

void YuiToolGuides::setView (Point<float> newOrigin, float zoom)
{
    origin = newOrigin;
    pixel = 1.0f / jmax (0.05f, zoom);

    repaint();
}

void YuiToolGuides::setCaret (YuiToolPlace newCaret)
{
    if (newCaret.line != caret.line || newCaret.column != caret.column)
        hasPicked = false;

    caret = newCaret;

    findAtCaret();
    repaint();
}

void YuiToolGuides::setPicked (const std::vector<Component*>& components)
{
    picked.assign (components.begin(), components.end());
    hasPicked = true;

    repaint();
}

void YuiToolGuides::findAtCaret()
{
    atCaret.clear();

    auto* root = document != nullptr ? document->getRoot() : nullptr;

    if (root == nullptr || caret.line < 1 || ! input.existsAsFile())
        return;

    // A document knows its components by where their name is written, which is where the one the caret is in starts
    const YuiToolIndex index (input.loadFileAsString());

    if (const auto* block = index.findComponentAt (index.getOffset (caret)))
        collectWrittenAt (*document, *root, index.getPlace (block->start), atCaret);
}

//==============================================================================

float YuiToolGuides::getLabelWidth (const String& text) const
{
    // As wide as the text most likely is, which is then fitted in it
    return (static_cast<float> (text.length()) * 6.5f + 8.0f) * pixel;
}

void YuiToolGuides::paintLabel (Graphics& g, const Font& font, const String& text, Point<float> topLeft)
{
    const Size<float> size (getLabelWidth (text), 14.0f * pixel);
    const Rectangle<float> box (jlimit (0.0f, jmax (0.0f, getWidth() - size.getWidth()), topLeft.getX()),
                                jlimit (0.0f, jmax (0.0f, getHeight() - size.getHeight()), topLeft.getY()),
                                size.getWidth(),
                                size.getHeight());

    g.setFillColor (Color (0xff000000).withAlpha (0.75f));
    g.fillRect (box);
    g.setFillColor (Color (0xffffffff));
    g.fillFittedText (text, font, box, Justification::center);
}

void YuiToolGuides::paintDistance (Graphics& g, const Font& font, Point<float> from, float length, bool isAcross)
{
    if (length < 0.5f)
        return;

    const auto text = describeLength (length);

    g.setStrokeColor (distanceColor);
    g.strokeLine (from, isAcross ? from.translated (length, 0.0f) : from.translated (0.0f, length));

    // Next to the middle of the line: under one going across, right of one going down
    paintLabel (g, font, text, isAcross ? from.translated ((length - getLabelWidth (text)) * 0.5f, 3.0f * pixel)
                                        : from.translated (3.0f * pixel, length * 0.5f - 7.0f * pixel));
}

void YuiToolGuides::paint (Graphics& g)
{
    const auto* root = document != nullptr ? document->getRoot() : nullptr;

    if (root == nullptr)
        return;

    // The lines and the texts keep their size on the screen, whatever the zoom the interface is seen at
    const auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (11.0f * pixel);
    std::vector<Rectangle<float>> areas;

    g.setStrokeWidth (pixel);

    for (const auto& shown : hasPicked ? picked : atCaret)
    {
        const auto* component = shown.get();

        if (component == nullptr || ! isInside (*component, *root))
            continue;

        const auto place = findYuiToolOrigin (*component, *root) - origin;
        const auto bounds = component->getLocalBounds().translated (place);

        areas.push_back (bounds);

        if (const auto* parent = component->getParentComponent(); parent != nullptr && component != root)
        {
            const auto held = parent->getLocalBounds().translated (findYuiToolOrigin (*parent, *root) - origin);

            areas.push_back (held);
            paintDistance (g, font, { held.getX(), bounds.getCenterY() }, bounds.getX() - held.getX(), true);
            paintDistance (g, font, { bounds.getRight(), bounds.getCenterY() }, held.getRight() - bounds.getRight(), true);
            paintDistance (g, font, { bounds.getCenterX(), held.getY() }, bounds.getY() - held.getY(), false);
            paintDistance (g, font, { bounds.getCenterX(), bounds.getBottom() }, held.getBottom() - bounds.getBottom(), false);
        }

        g.setStrokeColor (boundsColor);

        if (! component->getPaintMargin().isZero())
        {
            const float dots[] { 2.0f * pixel, 2.0f * pixel };
            Path outline;

            outline.addRectangle (component->getPaintBounds().translated (place));
            g.strokePath (outline.createDashed ({ dots, std::size (dots) }));
        }

        g.strokeRect (bounds);

        auto name = describeYuiToolName (*component);

        name << "  " << describeLength (bounds.getWidth()) << " x " << describeLength (bounds.getHeight());

        // Above the component, or inside it where there is no room above
        const auto room = 16.0f * pixel;

        paintLabel (g, font, name, { bounds.getX(), bounds.getY() >= room ? bounds.getY() - room : bounds.getY() + 2.0f * pixel });
    }

    // Only what changed under the guides is painted again: once what they show moved, they are painted whole
    if (areas != painted)
    {
        painted = std::move (areas);

        MessageManager::callAsync ([guides = WeakReference<Component> (this)]
        {
            if (auto* shown = guides.get())
                shown->repaint();
        });
    }
}

void YuiToolGuides::mouseDown (const MouseEvent& event)
{
    auto* root = document != nullptr ? document->getRoot() : nullptr;

    if (root == nullptr)
        return;

    // What a widget made for itself is picked as the widget
    auto* hit = root->findComponentAt (event.getPosition() + origin);

    while (hit != nullptr && hit != root && ! document->getBuilder().findElementFor (hit).isValid())
        hit = hit->getParentComponent();

    // With the shift key what is shown stays, and the component is added to it or taken away
    auto chosen = event.getModifiers().isShiftDown() ? (hasPicked ? picked : atCaret) : Components();

    if (hit != nullptr)
    {
        const auto known = std::find_if (chosen.begin(), chosen.end(), [hit] (const WeakReference<Component>& component)
        {
            return component == hit;
        });

        if (known != chosen.end())
            chosen.erase (known);
        else
            chosen.emplace_back (hit);
    }

    picked = std::move (chosen);
    hasPicked = true;

    repaint();
}
