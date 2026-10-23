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

#include <yup_gui/yup_gui.h>

#include <functional>

//==============================================================================
/** A page of the instrument: paints nothing and hands clicks on its background back. */
class SynthPage : public yup::Component
{
public:
    /** Construct a page. */
    SynthPage()
    {
        setOpaque (false);
    }

    /** Called when the page itself, not one of its children, is clicked. */
    std::function<void()> onMouseDown;

    void mouseDown (const yup::MouseEvent&) override
    {
        if (onMouseDown != nullptr)
            onMouseDown();
    }
};
