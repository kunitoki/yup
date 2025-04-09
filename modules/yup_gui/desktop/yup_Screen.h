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
/** Represents a screen device.

    This class encapsulates information about a screen, including its physical size,
    content scale, virtual position, work area, name, and whether it is the primary screen.

    @see Desktop
*/
class JUCE_API Screen : public ReferenceCountedObject
{
public:
    //==============================================================================
    /** A pointer to a Display object. */
    using Ptr = ReferenceCountedObjectPtr<const Screen>;
    using Array = ReferenceCountedArray<Screen>;

    //==============================================================================
    /** Default constructor. */
    Screen() = default;

    //==============================================================================
    /** The physical size of the screen in millimeters. */
    Size<int> physicalSizeMillimeters;

    /** The content scale of the screen. */
    float contentScaleX = 1.0f;
    float contentScaleY = 1.0f;

    /** The virtual position of the screen. */
    Point<int> virtualPosition;

    /** The work area of the screen. */
    Rectangle<int> workArea;

    /** The name of the screen. */
    String name;

    /** Whether the screen is the primary screen. */
    bool isPrimary = false;
};

} // namespace yup
