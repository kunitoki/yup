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
/** Represents a display device.

    This class encapsulates information about a display, including its physical size,
    content scale, virtual position, work area, name, and whether it is the primary display.

    @see Desktop
*/
class JUCE_API Display : public ReferenceCountedObject
{
public:
    //==============================================================================
    /** A pointer to a Display object. */
    using Ptr = ReferenceCountedObjectPtr<const Display>;
    using Array = ReferenceCountedArray<Display>;

    //==============================================================================
    /** Default constructor. */
    Display() = default;

    //==============================================================================
    /** The physical size of the display in millimeters. */
    Size<int> physicalSizeMillimeters;

    /** The content scale of the display. */
    float contentScaleX = 1.0f;
    float contentScaleY = 1.0f;

    /** The virtual position of the display. */
    Point<int> virtualPosition;

    /** The work area of the display. */
    Rectangle<int> workArea;

    /** The name of the display. */
    String name;

    /** Whether the display is the primary display. */
    bool isPrimary = false;
};

} // namespace yup
