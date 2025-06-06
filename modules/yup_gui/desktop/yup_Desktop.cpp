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

int Desktop::getNumScreens() const
{
    return screens.size();
}

//==============================================================================

Screen::Ptr Desktop::getScreen (int screenIndex) const
{
    if (isPositiveAndBelow (screenIndex, screens.size()))
        return screens.getUnchecked (screenIndex);

    return nullptr;
}

//==============================================================================

Span<const Screen* const> Desktop::getScreens() const
{
    return { screens.data(), static_cast<size_t> (screens.size()) };
}

//==============================================================================

Screen::Ptr Desktop::getPrimaryScreen() const
{
    return ! screens.isEmpty() ? getScreen (0) : nullptr;
}

//==============================================================================

Screen::Ptr Desktop::getScreenContainingMouseCursor() const
{
    return getScreenContaining (getCurrentMouseLocation());
}

//==============================================================================

Screen::Ptr Desktop::getScreenContaining (const Point<float>& location) const
{
    for (auto& screen : screens)
    {
        if (screen->workArea.contains (location.to<int>()))
            return screen;
    }

    return ! screens.isEmpty() ? getScreen (0) : nullptr;
}

//==============================================================================

MouseCursor Desktop::getMouseCursor() const
{
    return currentMouseCursor.value_or (MouseCursor (MouseCursor::Default));
}

//==============================================================================

void Desktop::addGlobalMouseListener (MouseListener* listener)
{
    if (listener == nullptr)
        return;

    // Remove any existing weak reference to this listener first
    removeGlobalMouseListener (listener);

    // Add the new weak reference
    globalMouseListeners.push_back (WeakReference<MouseListener> (listener));
}

void Desktop::removeGlobalMouseListener (MouseListener* listener)
{
    if (listener == nullptr)
        return;

    globalMouseListeners.erase (
        std::remove_if (globalMouseListeners.begin(), globalMouseListeners.end(),
            [listener] (const WeakReference<MouseListener>& ref)
            {
                return ref.get() == listener || ref.get() == nullptr;
            }),
        globalMouseListeners.end());
}

void Desktop::handleGlobalMouseDown (const MouseEvent& event)
{
    auto it = globalMouseListeners.begin();
    while (it != globalMouseListeners.end())
    {
        auto* listener = it->get();
        if (listener == nullptr)
        {
            it = globalMouseListeners.erase (it);
        }
        else
        {
            listener->mouseDown (event);
            ++it;
        }
    }
}

void Desktop::handleGlobalMouseUp (const MouseEvent& event)
{
    auto it = globalMouseListeners.begin();
    while (it != globalMouseListeners.end())
    {
        auto* listener = it->get();
        if (listener == nullptr)
        {
            it = globalMouseListeners.erase (it);
        }
        else
        {
            listener->mouseUp (event);
            ++it;
        }
    }
}

void Desktop::handleGlobalMouseMove (const MouseEvent& event)
{
    auto it = globalMouseListeners.begin();
    while (it != globalMouseListeners.end())
    {
        auto* listener = it->get();
        if (listener == nullptr)
        {
            it = globalMouseListeners.erase (it);
        }
        else
        {
            listener->mouseMove (event);
            ++it;
        }
    }
}

void Desktop::handleGlobalMouseDrag (const MouseEvent& event)
{
    auto it = globalMouseListeners.begin();
    while (it != globalMouseListeners.end())
    {
        auto* listener = it->get();
        if (listener == nullptr)
        {
            it = globalMouseListeners.erase (it);
        }
        else
        {
            listener->mouseDrag (event);
            ++it;
        }
    }
}

//==============================================================================

void Desktop::handleScreenConnected (int screenIndex)
{
    updateScreens();
}

void Desktop::handleScreenDisconnected (int screenIndex)
{
    updateScreens();
}

void Desktop::handleScreenMoved (int screenIndex)
{
    updateScreens();
}

void Desktop::handleScreenOrientationChanged (int screenIndex)
{
    updateScreens();
}

} // namespace yup
