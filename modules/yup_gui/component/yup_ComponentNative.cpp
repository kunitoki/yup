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

ComponentNative::Options& ComponentNative::Options::withFlags (Flags newFlags) noexcept
{
    flags = newFlags;
    return *this;
}

ComponentNative::Options& ComponentNative::Options::withDecoration (bool shouldHaveDecoration) noexcept
{
    if (shouldHaveDecoration)
        flags |= decoratedWindow;
    else
        flags &= ~decoratedWindow;
    return *this;
}

ComponentNative::Options& ComponentNative::Options::withResizableWindow (bool shouldAllowResizing) noexcept
{
    if (shouldAllowResizing)
        flags |= resizableWindow;
    else
        flags &= ~resizableWindow;
    return *this;
}

ComponentNative::Options& ComponentNative::Options::withRenderContinuous (bool shouldRenderContinuous) noexcept
{
    if (shouldRenderContinuous)
        flags |= renderContinuous;
    else
        flags &= ~renderContinuous;
    return *this;
}

ComponentNative::Options& ComponentNative::Options::withAllowedHighDensityDisplay (bool shouldAllowHighDensity) noexcept
{
    if (shouldAllowHighDensity)
        flags |= allowHighDensityDisplay;
    else
        flags &= ~allowHighDensityDisplay;
    return *this;
}

ComponentNative::Options& ComponentNative::Options::withGraphicsApi (std::optional<GraphicsContext::Api> newGraphicsApi) noexcept
{
    graphicsApi = newGraphicsApi;
    return *this;
}

ComponentNative::Options& ComponentNative::Options::withFramerateRedraw (std::optional<float> newFramerateRedraw) noexcept
{
    framerateRedraw = newFramerateRedraw;
    return *this;
}

ComponentNative::Options& ComponentNative::Options::withClearColor (std::optional<Color> newClearColor) noexcept
{
    clearColor = newClearColor;
    return *this;
}

ComponentNative::Options& ComponentNative::Options::withDoubleClickTime (std::optional<RelativeTime> newDoubleClickTime) noexcept
{
    doubleClickTime = newDoubleClickTime;
    return *this;
}

ComponentNative::Options& ComponentNative::Options::withUpdateOnlyFocused (bool onlyWhenFocused) noexcept
{
    updateOnlyWhenFocused = onlyWhenFocused;
    return *this;
}

//==============================================================================

ComponentNative::ComponentNative (Component& newComponent, const Flags& newFlags)
    : component (newComponent)
    , flags (newFlags)
{
}

ComponentNative::~ComponentNative()
{
}

} // namespace yup
