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
/** Represents the desktop environment, providing access to display information and management.

    This class encapsulates functionality related to the desktop environment, including access to multiple displays
    connected to the system. It allows querying and management of different display properties through the `Display` objects.
*/
class JUCE_API Desktop
{
public:
    //==============================================================================
    /** Destructor for the Desktop class. */
    ~Desktop();

    //==============================================================================
    /** Returns the number of displays connected to the system.

        @return The number of available displays.
    */
    int getNumDisplays() const;

    /** Retrieves a pointer to the `Display` object at the specified index.

        @param displayIndex The zero-based index of the display to retrieve.

        @return A pointer to the `Display` object, or nullptr if the index is out of range.
    */
    Display* getDisplay (int displayIndex) const;

    /** Retrieves a pointer to the primary `Display` object.

        The primary display is typically the main screen of the system where applications are initially displayed.

        @return A pointer to the primary `Display` object.
    */
    Display* getPrimaryDisplay() const;

    //==============================================================================
    JUCE_DECLARE_SINGLETON (Desktop, true)

private:
    friend class YUPApplication;

    void updateDisplays();

    Desktop();

    OwnedArray<Display> displays;
};

} // namespace yup
