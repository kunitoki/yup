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

namespace yup
{

//==============================================================================
/** Defines where a stroke is drawn relative to the edge of its path.

    A centered stroke straddles the edge, half inside and half outside. Inside
    and outside strokes keep the whole width on one side, so for example a
    border drawn inside never grows past the shape's bounds.
*/
enum class StrokePosition : unsigned int
{
    Inside = 0, ///< The stroke is drawn entirely inside the path.
    Center = 1, ///< The stroke is centered on the path's edge.
    Outside = 2 ///< The stroke is drawn entirely outside the path.
};

} // namespace yup
