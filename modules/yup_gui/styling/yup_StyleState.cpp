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

//==============================================================================

bool StyleState::setComputedValue (const Identifier& storageId, const var& value)
{
    return computedValues.set (storageId, value);
}

bool StyleState::removeComputedValue (const Identifier& storageId)
{
    return computedValues.remove (storageId);
}

const var* StyleState::getComputedValue (const Identifier& storageId) const
{
    return computedValues.getVarPointer (storageId);
}

bool StyleState::clearComputedValues()
{
    if (computedValues.size() == 0)
        return false;

    computedValues.clear();
    return true;
}

//==============================================================================

bool StyleState::setAnimatedValue (const Identifier& storageId, const var& value)
{
    return animatedValues.set (storageId, value);
}

bool StyleState::removeAnimatedValue (const Identifier& storageId)
{
    return animatedValues.remove (storageId);
}

const var* StyleState::getAnimatedValue (const Identifier& storageId) const
{
    return animatedValues.getVarPointer (storageId);
}

bool StyleState::clearAnimatedValues()
{
    if (animatedValues.size() == 0)
        return false;

    animatedValues.clear();
    return true;
}

const var* StyleState::getStyleValue (const Identifier& storageId) const
{
    if (const auto* animated = animatedValues.getVarPointer (storageId))
        return animated;

    return computedValues.getVarPointer (storageId);
}

//==============================================================================

bool StyleState::setStylePalette (std::optional<ThemePalette> newPalette)
{
    if (! newPalette.has_value())
    {
        if (stylePalette == nullptr)
            return false;

        stylePalette = nullptr;
        return true;
    }

    if (stylePalette != nullptr && *stylePalette == *newPalette)
        return false;

    stylePalette = std::make_unique<ThemePalette> (std::move (*newPalette));
    return true;
}

//==============================================================================

bool StyleState::setShadows (std::vector<Shadow> newShadows)
{
    if (shadows == newShadows)
        return false;

    shadows = std::move (newShadows);
    return true;
}

void StyleState::paintShadows (Graphics& g, Rectangle<float> bounds, float cornerRadius) const
{
    const auto savedState = g.saveState();

    for (auto it = shadows.rbegin(); it != shadows.rend(); ++it)
    {
        g.setFeather (it->blur);
        g.setFillColor (it->color);
        g.fillRoundedRect (bounds.translated (it->offset).enlarged (it->spread), jmax (0.0f, cornerRadius + it->spread));
    }
}

//==============================================================================

bool StyleState::isEmpty() const noexcept
{
    return computedValues.size() == 0 && animatedValues.size() == 0 && stylePalette == nullptr && shadows.empty();
}

} // namespace yup
