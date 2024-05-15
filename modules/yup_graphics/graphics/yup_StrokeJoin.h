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
/** Defines types of stroke joins used in graphical contexts.

    This enumeration specifies the style of joins that could be applied where two lines meet in a graphical drawing context.
    Each join type affects how the corner is shaped.
*/
enum class StrokeJoin : unsigned int
{
    Miter = 0, ///< A miter join creates a sharp corner or a clipped corner if the join angle is too sharp.
    Round = 1, ///< A round join creates a rounded corner that smooths the meeting point of the lines.
    Bevel = 2  ///< A bevel join creates a flattened corner by connecting the outer corners of the stroke.
};

} // namespace yup
