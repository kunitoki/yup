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
/** Defines types of stroke caps used in graphical contexts.

    This enumeration specifies the style of caps that could be applied to the ends of lines in a graphical drawing context.
    Each cap style provides a different appearance for the ends of lines.
*/
enum class StrokeCap : unsigned int
{
    Butt = 0,   ///< A butt cap displays the end of the line exactly at the end point with no extension.
    Round = 1,  ///< A round cap extends the line with a half-circle that is centered at the end point of the line.
    Square = 2  ///< A square cap extends the line by adding a square outline that projects beyond the end point.
};

} // namespace yup
