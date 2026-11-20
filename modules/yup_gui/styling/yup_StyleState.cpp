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

bool StyleState::setStyleTransform (const AffineTransform& newTransform) noexcept
{
    if (styleTransform == newTransform)
        return false;

    styleTransform = newTransform;
    return true;
}

//==============================================================================

bool StyleState::setFilterOperations (std::vector<FilterEffect::Operation> newOperations)
{
    if (newOperations.empty())
        return setFilterEffect (nullptr);

    if (filterEffect == nullptr)
    {
        filterEffect = FilterEffect::Ptr (new FilterEffect (std::move (newOperations)));
        return true;
    }

    if (filterEffect->getOperations() == newOperations)
        return false;

    filterEffect->setOperations (std::move (newOperations));
    return true;
}

bool StyleState::setFilterEffect (FilterEffect::Ptr newFilter)
{
    if (filterEffect == newFilter)
        return false;

    filterEffect = std::move (newFilter);
    return true;
}

//==============================================================================

bool StyleState::setShadows (std::vector<Shadow> newShadows)
{
    if (shadows == newShadows)
        return false;

    shadows = std::move (newShadows);
    cachedShadowPaths.clear();
    return true;
}

bool StyleState::setGlows (std::vector<Shadow> newGlows)
{
    if (glows == newGlows)
        return false;

    glows = std::move (newGlows);
    cachedShadowPaths.clear();
    return true;
}

void StyleState::paintShadows (Graphics& g, Rectangle<float> bounds, float cornerRadius) const
{
    const auto totalLayers = shadows.size() + glows.size();
    if (totalLayers == 0)
        return;

    if (cachedShadowPaths.size() != totalLayers
        || cachedShadowBounds != bounds
        || ! approximatelyEqual (cachedShadowCornerRadius, cornerRadius))
    {
        cachedShadowBounds = bounds;
        cachedShadowCornerRadius = cornerRadius;
        cachedShadowPaths.clear();
        cachedShadowPaths.reserve (totalLayers);

        const auto appendReversed = [&] (const std::vector<Shadow>& layers)
        {
            for (auto it = layers.rbegin(); it != layers.rend(); ++it)
            {
                Path shadowPath;
                shadowPath.addRoundedRectangle (bounds.translated (it->offset).enlarged (it->spread),
                                                jmax (0.0f, cornerRadius + it->spread));
                cachedShadowPaths.push_back (std::move (shadowPath));
            }
        };

        appendReversed (shadows);
        appendReversed (glows);
    }

    const auto savedState = g.saveState();

    size_t index = 0;
    const auto drawReversed = [&] (const std::vector<Shadow>& layers)
    {
        for (auto it = layers.rbegin(); it != layers.rend(); ++it, ++index)
        {
            g.setFeather (it->blur);
            g.setFillColor (it->color);
            g.fillPath (cachedShadowPaths[index]);
        }
    };

    drawReversed (shadows);
    drawReversed (glows);
}

} // namespace yup
