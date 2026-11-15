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


namespace yup
{

namespace
{

std::optional<MouseCursor::Type> findStyleCursor (const String& name)
{
    static const std::pair<const char*, MouseCursor::Type> cursors[] = {
        { "none", MouseCursor::None },
        { "default", MouseCursor::Default },
        { "arrow", MouseCursor::Arrow },
        { "text", MouseCursor::Text },
        { "wait", MouseCursor::Wait },
        { "waitArrow", MouseCursor::WaitArrow },
        { "hand", MouseCursor::Hand },
        { "crosshair", MouseCursor::Crosshair },
        { "crossbones", MouseCursor::Crossbones },
        { "resizeLeftRight", MouseCursor::ResizeLeftRight },
        { "resizeUpDown", MouseCursor::ResizeUpDown },
        { "resizeTopLeftRightBottom", MouseCursor::ResizeTopLeftRightBottom },
        { "resizeBottomLeftRightTop", MouseCursor::ResizeBottomLeftRightTop },
        { "resizeAll", MouseCursor::ResizeAll }
    };

    return detail::findNamed (cursors, name);
}

} // namespace

//==============================================================================

void StyleEffects::applyCursor (Component& component, const String& name, const Reporter& report)
{
    // As with drawings, a cursor the application set in the meantime wins over the styled one
    const bool styled = cursor.has_value() && component.getMouseCursor() == *cursor;
    const auto type = findStyleCursor (name);

    if (name.isNotEmpty() && ! type.has_value())
        report ("cursor", "Unknown cursor '" + name + "'");

    if (! type.has_value())
    {
        if (styled)
            component.setMouseCursor (savedCursor);

        cursor.reset();
        return;
    }

    if (! styled)
        savedCursor = component.getMouseCursor();

    cursor = MouseCursor (*type);

    if (component.getMouseCursor() != *cursor)
        component.setMouseCursor (*cursor);
}

void StyleEffects::applyEffect (Component& component, const String& name, const Reporter& report)
{
    const bool styled = effectName.isNotEmpty() && component.getComponentEffect().get() == effect.get();

    if (styled && name == effectName)
        return;

    ComponentEffect::Ptr created;

    if (name.isNotEmpty() && name != "none")
    {
        created = ComponentEffectRegistry::getInstance().createEffect (name);

        if (created == nullptr)
            report ("effect", "Unknown effect '" + name + "'");
    }

    if (name.isEmpty() || (created == nullptr && name != "none"))
    {
        if (styled)
            component.setComponentEffect (std::move (savedEffect));

        effectName = {};
        effect = nullptr;
        savedEffect = nullptr;
        return;
    }

    if (! styled)
        savedEffect = component.getComponentEffect();

    effectName = name;
    effect = created;
    component.setComponentEffect (std::move (created));
}

std::vector<StyleState::Shadow> StyleEffects::applyShadows (Component& component, const String& text, const ColorEvaluator& evaluateColor, const Reporter& report)
{
    auto shadows = parseShadows (text, evaluateColor, report);
    applyPaintMargin (component, shadows);
    return shadows;
}

std::vector<StyleState::Shadow> StyleEffects::parseShadows (const String& text, const ColorEvaluator& evaluateColor, const Reporter& report)
{
    std::vector<StyleState::Shadow> shadows;

    if (text.isEmpty() || text == "none")
        return shadows;

    const auto layers = detail::tokenizeStyleValue (text);

    for (int index = 0; index < static_cast<int> (layers.size()); ++index)
    {
        const auto& tokens = layers[static_cast<size_t> (index)];

        // Up to four lengths, then a color expression taking the remaining tokens
        float parsedLengths[4] {};
        int lengths = 0;

        while (lengths < jmin (4, tokens.size() - 1))
        {
            const auto length = detail::parseStrictFloat (tokens[lengths]);
            if (! length.has_value())
                break;

            parsedLengths[lengths++] = *length;
        }

        if (lengths < 2)
        {
            report ("shadow", "shadow: Expected 'x y [blur [spread]] color', got '" + tokens.joinIntoString (" ") + "'");
            return {};
        }

        const auto color = evaluateColor (index, tokens.joinIntoString (" ", lengths));

        if (! color.has_value())
            return {};

        StyleState::Shadow shadow;
        shadow.offset = { parsedLengths[0], parsedLengths[1] };
        shadow.blur = lengths > 2 ? jmax (0.0f, parsedLengths[2]) : 0.0f;
        shadow.spread = lengths > 3 ? parsedLengths[3] : 0.0f;
        shadow.color = *color;
        shadows.push_back (shadow);
    }

    return shadows;
}

void StyleEffects::applyPaintMargin (Component& component, const std::vector<StyleState::Shadow>& shadows)
{
    // The paint margin grows to hold the shadows, keeping what the application asked for
    const auto current = component.getPaintMargin();
    const bool styled = paintMargin.has_value() && current == *paintMargin;

    if (shadows.empty())
    {
        if (styled)
            component.setPaintMargin (savedPaintMargin);

        paintMargin.reset();
        return;
    }

    if (! styled)
        savedPaintMargin = current;

    auto left = savedPaintMargin.getLeft();
    auto top = savedPaintMargin.getTop();
    auto right = savedPaintMargin.getRight();
    auto bottom = savedPaintMargin.getBottom();

    for (const auto& shadow : shadows)
    {
        const auto reach = shadow.blur + jmax (0.0f, shadow.spread);
        left = jmax (left, reach - shadow.offset.getX());
        top = jmax (top, reach - shadow.offset.getY());
        right = jmax (right, reach + shadow.offset.getX());
        bottom = jmax (bottom, reach + shadow.offset.getY());
    }

    paintMargin = EdgeSpacing<float> (left, top, right, bottom);

    if (current != *paintMargin)
        component.setPaintMargin (*paintMargin);
}

} // namespace yup
