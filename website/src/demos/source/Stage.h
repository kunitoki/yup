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

#pragma once

#include <array>
#include <atomic>

//==============================================================================
/**
    The window filling the page canvas, showing one demo at a time.

    The page picks the demo through requestDemo(), which may be called from any
    thread; the request is applied on the next frame.
*/
class Stage : public yup::DocumentWindow
{
public:
    //==============================================================================
    Stage()
        : yup::DocumentWindow (yup::ComponentNative::Options().withVSync (true), Demo::fieldColor)
    {
        for (auto* demo : demos)
            addChildComponent (demo);

        showDemo (0);
    }

    //==============================================================================
    /** Asks for the demo at the given index, in the order the page lists them. */
    static void requestDemo (int index) { requestedDemo.store (index, std::memory_order_relaxed); }

    /** Tells whether the page shows the demo full screen; opening it focuses its main widget. */
    static void requestOpen (bool isOpen) { requestedOpen.store (isOpen, std::memory_order_relaxed); }

    //==============================================================================
    void refreshDisplay (double) override
    {
        const auto index = requestedDemo.load (std::memory_order_relaxed);
        if (index != currentDemo && yup::isPositiveAndBelow (index, static_cast<int> (demos.size())))
            showDemo (index);

        const auto open = requestedOpen.load (std::memory_order_relaxed);
        if (open && ! wasOpen && yup::isPositiveAndBelow (currentDemo, static_cast<int> (demos.size())))
            demos[static_cast<std::size_t> (currentDemo)]->focusMainComponent();

        wasOpen = open;
    }

    void resized() override
    {
        for (auto* demo : demos)
            demo->setBounds (getLocalBounds());
    }

    void userTriedToCloseWindow() override
    {
        yup::YUPApplication::getInstance()->systemRequestedQuit();
    }

private:
    //==============================================================================
    void showDemo (int index)
    {
        currentDemo = index;

        for (std::size_t i = 0; i < demos.size(); ++i)
            demos[i]->setVisible (static_cast<int> (i) == index);
    }

    //==============================================================================
    static inline std::atomic<int> requestedDemo { 0 };
    static inline std::atomic<bool> requestedOpen { false };

    Facet facet;
    Keys keys;
    Reel reel;
    Layers layers;
    Ripple ripple;
    Start start;
    std::array<Demo*, 6> demos { &facet, &keys, &reel, &layers, &ripple, &start };
    int currentDemo = -1;
    bool wasOpen = false;
};
