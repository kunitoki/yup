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

const Identifier DocumentWindow::Style::backgroundColorId = "documentWindowBackground";

//==============================================================================

DocumentWindow::DocumentWindow (const ComponentNative::Options& options, const std::optional<Color>& backgroundColor)
{
    addToDesktop (options, nullptr);

    if (backgroundColor)
        setColor (Style::backgroundColorId, *backgroundColor);
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
        // Center using pixel window dimensions so the math works correctly
        // even if SDL reports the work area in screen pixels (e.g. RDP).
        const auto dpiScale = screen->contentScaleX;
        const auto pixelW = static_cast<int> (size.getWidth() * dpiScale);
        const auto pixelH = static_cast<int> (size.getHeight() * dpiScale);
        auto bounds = screen->workArea.centeredRectangleWithSize (Size<int> (pixelW, pixelH));

        // Keep position from the pixel-space centering, but
        // use the original logical size for the window.
        bounds.setSize (size);

        YUP_MODULE_DBG (GUI_WINDOWING, "DocumentWindow: centreWithSize workArea=" << screen->workArea.toString() << " size=" << size.toString() << " contentScale=" << dpiScale << " -> bounds=" << bounds.toString());
        setBounds (bounds.to<float>());
    }
    else
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "DocumentWindow: centreWithSize no screen found, setting size only");
        setSize (size.to<float>());
    }
}

//==============================================================================

void DocumentWindow::paint (Graphics& g)
{
    g.setFillColor (findColor (Style::backgroundColorId).value_or (Colors::dimgray));
    g.fillAll();
}

//==============================================================================

void DocumentWindow::userTriedToCloseWindow()
{
    jassertfalse; // Must implement this method to decide what to do when the window is closing !
}

} // namespace yup
