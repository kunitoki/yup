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

bool StyleState::isEmpty() const noexcept
{
    return computedValues.size() == 0 && stylePalette == nullptr;
}

} // namespace yup
