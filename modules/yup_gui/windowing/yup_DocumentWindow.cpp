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

DocumentWindow::DocumentWindow (ComponentNative::Flags flags, const Color& backgroundColor, std::optional<float> framerateRedraw)
    : backgroundColor (backgroundColor)
{
    addToDesktop (flags, framerateRedraw);
}

DocumentWindow::~DocumentWindow()
{
    removeFromDesktop();
}

//==============================================================================

void DocumentWindow::centreWithSize (const Size<int>& size)
{
    auto desktop = Desktop::getInstance();

    if (auto display = desktop->getPrimaryDisplay())
    {
        auto bounds = Rectangle<float>
        {
            jmax (0.0f, static_cast<float> (display->workArea.getWidth() - size.getWidth()) / 2),
            jmax (0.0f, static_cast<float> (display->workArea.getHeight() - size.getHeight()) / 2),
            static_cast<float> (size.getWidth()),
            static_cast<float> (size.getHeight())
        };

        // TODO - take into account the frame and taskbar

        setBounds (bounds);
    }
    else
    {
        setSize (size.to<float> ());
    }
}

//==============================================================================

void DocumentWindow::paint (Graphics& g)
{
    g.setFillColor (backgroundColor);
    g.fillAll();
}

} // namespace yup
