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

#include <yup_gui_markup/yup_gui_markup.h>

namespace yup
{

/** Gives the markup tests access to the internal mouse entry points of a component. */
template <>
class ComponentTestHelper<YuiBuilder>
{
public:
    static void triggerMouseDown (Component& component, const MouseEvent& event)
    {
        component.internalMouseDown (event);
    }

    static void triggerMouseUp (Component& component, const MouseEvent& event)
    {
        component.internalMouseUp (event);
    }
};

} // namespace yup
