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

DocumentWindow::DocumentWindow (const ComponentNative::Options& options, const Color& backgroundColor)
    : backgroundColor (backgroundColor)
{
    auto finalOptions = options;

#if YUP_EMSCRIPTEN
    // This is enforced for now until we have a better way to handle dirty regions on emscripten
    finalOptions = finalOptions.withRenderContinuous (true);
#endif

    addToDesktop (finalOptions, nullptr);
}

DocumentWindow::~DocumentWindow()
{
}

//==============================================================================

void DocumentWindow::centreWithSize (const Size<int>& size)
{
    auto desktop = Desktop::getInstance();

    if (auto screen = desktop->getScreenContainingMouseCursor())
    {
        auto bounds = screen->workArea.centeredRectangleWithSize (size);
        // TODO - take into account the frame and taskbar
        setBounds (bounds.to<float>());
    }
    else
    {
        setSize (size.to<float>());
    }
}

//==============================================================================

void DocumentWindow::paint (Graphics& g)
{
    g.setFillColor (backgroundColor);
    g.fillAll();
}

//==============================================================================

void DocumentWindow::userTriedToCloseWindow()
{
    jassertfalse; // Must implement this method to decide what to do when the window is closing !
}

} // namespace yup
