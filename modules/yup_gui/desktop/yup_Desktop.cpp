/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

JUCE_IMPLEMENT_SINGLETON (Desktop)

//==============================================================================

Desktop::Desktop()
{
}

Desktop::~Desktop()
{
    clearSingletonInstance();
}

//==============================================================================

int Desktop::getNumDisplays() const
{
    return displays.size();
}

//==============================================================================

Display::Ptr Desktop::getDisplay (int displayIndex) const
{
    if (isPositiveAndBelow (displayIndex, displays.size()))
        return displays.getUnchecked (displayIndex);

    return nullptr;
}

//==============================================================================

Span<const Display* const> Desktop::getDisplays() const
{
    return { displays.data(), static_cast<size_t> (displays.size()) };
}

//==============================================================================

Display::Ptr Desktop::getPrimaryDisplay() const
{
    return ! displays.isEmpty() ? getDisplay (0) : nullptr;
}

//==============================================================================

Display::Ptr Desktop::getDisplayContainingMouseCursor() const
{
    return getDisplayContaining (getCurrentMouseLocation());
}

//==============================================================================

Display::Ptr Desktop::getDisplayContaining (const Point<float>& location) const
{
    for (auto& display : displays)
    {
        if (display->workArea.contains (location.to<int>()))
            return display;
    }

    return ! displays.isEmpty() ? getDisplay (0) : nullptr;
}

//==============================================================================

MouseCursor Desktop::getMouseCursor() const
{
    return currentMouseCursor.value_or (MouseCursor (MouseCursor::Default));
}

//==============================================================================

void Desktop::handleDisplayConnected (int displayIndex)
{
    updateDisplays();
}

void Desktop::handleDisplayDisconnected (int displayIndex)
{
    updateDisplays();
}

void Desktop::handleDisplayMoved (int displayIndex)
{
    updateDisplays();
}

void Desktop::handleDisplayOrientationChanged (int displayIndex)
{
    updateDisplays();
}

} // namespace yup
