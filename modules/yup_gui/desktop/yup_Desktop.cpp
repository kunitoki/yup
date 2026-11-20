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

YUP_IMPLEMENT_SINGLETON (Desktop)

//==============================================================================

Desktop::Desktop() = default;

Desktop::~Desktop()
{
    // jassert (globalMouseListeners.isEmpty());

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

Screen::Ptr Desktop::getScreenContaining (const Rectangle<float>& bounds) const
{
    Screen::Ptr foundScreen;
    float maxArea = 0.0f;
    auto intBounds = bounds.to<int>();

    for (auto& screen : screens)
    {
        auto intersection = screen->workArea.intersection (intBounds);
        if (auto computedArea = intersection.area(); computedArea > maxArea)
        {
            maxArea = computedArea;
            foundScreen = screen;
        }
    }

    if (foundScreen == nullptr)
        return ! screens.isEmpty() ? getScreen (0) : nullptr;

    return foundScreen;
}

Screen::Ptr Desktop::getScreenContaining (Component* component) const
{
    return getScreenContaining (component->getScreenBounds());
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

    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (sendingMouseEvent)
        pendingMouseListeners.addIfNotAlreadyThere (listener);
    else
        globalMouseListeners.add (listener);
}

void Desktop::removeGlobalMouseListener (MouseListener* listener)
{
    if (listener == nullptr)
        return;

    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    pendingMouseListeners.removeFirstMatchingValue (listener);
    globalMouseListeners.remove (listener);
}

void Desktop::handleGlobalMouseDown (const MouseEvent& event)
{
    {
        ScopedValueSetter setter (sendingMouseEvent, true);
        globalMouseListeners.call (&MouseListener::mouseDown, event);
    }

    addPendingMouseListeners();
}

void Desktop::handleGlobalMouseUp (const MouseEvent& event)
{
    {
        ScopedValueSetter setter (sendingMouseEvent, true);
        globalMouseListeners.call (&MouseListener::mouseUp, event);
    }

    addPendingMouseListeners();
}

void Desktop::handleGlobalMouseMove (const MouseEvent& event)
{
    {
        ScopedValueSetter setter (sendingMouseEvent, true);
        globalMouseListeners.call (&MouseListener::mouseMove, event);
    }

    addPendingMouseListeners();
}

void Desktop::handleGlobalMouseDrag (const MouseEvent& event)
{
    {
        ScopedValueSetter setter (sendingMouseEvent, true);
        globalMouseListeners.call (&MouseListener::mouseDrag, event);
    }

    addPendingMouseListeners();
}

void Desktop::handleGlobalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    {
        ScopedValueSetter setter (sendingMouseEvent, true);
        globalMouseListeners.callChecked (ListenerList<MouseListener>::DummyBailOutChecker {},
                                          [&] (MouseListener& listener)
                                          {
                                              ignoreUnused (listener.mouseWheel (event, wheelData));
                                          });
    }

    addPendingMouseListeners();
}

void Desktop::addPendingMouseListeners()
{
    for (auto listener : pendingMouseListeners)
        globalMouseListeners.add (listener);

    pendingMouseListeners.clear();
}

//==============================================================================

std::atomic<int> Desktop::installedInputFilterCount { 0 };

void Desktop::addInputFilter (DesktopInputFilter* filter)
{
    if (filter == nullptr)
        return;

    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    if (std::find (inputFilters.begin(), inputFilters.end(), filter) != inputFilters.end())
        return;

    inputFilters.push_back (filter);
    installedInputFilterCount.fetch_add (1, std::memory_order_release);
}

void Desktop::removeInputFilter (DesktopInputFilter* filter)
{
    if (filter == nullptr)
        return;

    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED

    const auto it = std::find (inputFilters.begin(), inputFilters.end(), filter);
    if (it == inputFilters.end())
        return;

    inputFilters.erase (it);
    installedInputFilterCount.fetch_sub (1, std::memory_order_release);
}

bool Desktop::hasInputFilters() noexcept
{
    return installedInputFilterCount.load (std::memory_order_acquire) > 0;
}

bool Desktop::filterMouseEvent (DesktopInputFilter::InputEventType type, Component& rootComponent, const MouseEvent& event)
{
    // A filter may remove itself while handling an event, so the list is copied before calling
    const auto filters = inputFilters;

    for (auto* filter : filters)
    {
        if (std::find (inputFilters.begin(), inputFilters.end(), filter) == inputFilters.end())
            continue;

        if (filter->mouseEventFiltered (type, rootComponent, event))
            return true;
    }

    return false;
}

bool Desktop::filterMouseWheel (Component& rootComponent, const MouseEvent& event, const MouseWheelData& wheelData)
{
    const auto filters = inputFilters;

    for (auto* filter : filters)
    {
        if (std::find (inputFilters.begin(), inputFilters.end(), filter) == inputFilters.end())
            continue;

        if (filter->mouseWheelFiltered (rootComponent, event, wheelData))
            return true;
    }

    return false;
}

bool Desktop::filterKeyDown (Component& rootComponent, const KeyPress& keys, const Point<float>& position)
{
    const auto filters = inputFilters;

    for (auto* filter : filters)
    {
        if (std::find (inputFilters.begin(), inputFilters.end(), filter) == inputFilters.end())
            continue;

        if (filter->keyDownFiltered (rootComponent, keys, position))
            return true;
    }

    return false;
}

bool Desktop::filterKeyUp (Component& rootComponent, const KeyPress& keys, const Point<float>& position)
{
    const auto filters = inputFilters;

    for (auto* filter : filters)
    {
        if (std::find (inputFilters.begin(), inputFilters.end(), filter) == inputFilters.end())
            continue;

        if (filter->keyUpFiltered (rootComponent, keys, position))
            return true;
    }

    return false;
}

//==============================================================================

std::vector<Component*> Desktop::getTopLevelComponents() const
{
    std::vector<Component*> roots;
    roots.reserve (nativeComponents.size());

    for (const auto& [userdata, nativeComponent] : nativeComponents)
    {
        ignoreUnused (userdata);

        if (nativeComponent == nullptr || ! nativeComponent->isVisible())
            continue;

        roots.push_back (&nativeComponent->getComponent());
    }

    return roots;
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

//==============================================================================

void Desktop::registerNativeComponent (ComponentNative* nativeComponent)
{
    if (nativeComponent != nullptr)
        nativeComponents[nativeComponent] = nativeComponent;
}

void Desktop::unregisterNativeComponent (ComponentNative* nativeComponent)
{
    if (nativeComponent != nullptr)
        nativeComponents.erase (nativeComponent);
}

ComponentNative::Ptr Desktop::getNativeComponent (void* userdata) const
{
    if (userdata == nullptr)
        return nullptr;

    auto it = nativeComponents.find (userdata);
    if (it != nativeComponents.end())
        return ComponentNative::Ptr { it->second };

    return nullptr;
}

//==============================================================================

Component* Desktop::findComponentAt (const Point<float>& screenPosition, Component* componentToIgnore) const
{
    // A window that currently holds keyboard focus wins over one that merely overlaps it; SDL gives
    // no usable stacking order to rank the rest, so the first other match is kept as a fallback.
    Component* fallback = nullptr;

    for (const auto& [userdata, nativeComponent] : nativeComponents)
    {
        ignoreUnused (userdata);

        if (nativeComponent == nullptr || ! nativeComponent->isVisible())
            continue;

        auto& root = nativeComponent->getComponent();

        if (&root == componentToIgnore || ! root.isVisible())
            continue;

        if (auto* found = root.findComponentAt (root.screenToLocal (screenPosition)))
        {
            if (nativeComponent->getFocusedComponent() != nullptr)
                return found;

            if (fallback == nullptr)
                fallback = found;
        }
    }

    return fallback;
}

} // namespace yup
