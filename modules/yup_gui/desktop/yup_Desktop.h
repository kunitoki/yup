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
/** Represents the desktop environment, providing access to screen information and management.

    This class encapsulates functionality related to the desktop environment, including
    access to multiple screens connected to the system. It allows querying and management
    of different screen properties through the `Screen` objects.
*/
class JUCE_API Desktop
{
public:
    //==============================================================================
    /** Destructor for the Desktop class. */
    ~Desktop();

    //==============================================================================
    /** Returns the number of screens connected to the system.

        @return The number of available screens.
    */
    int getNumScreens() const;

    /** Retrieves a pointer to the `Screen` object at the specified index.

        @param screenIndex The zero-based index of the screen to retrieve.

        @return A pointer to the `Screen` object, or nullptr if the index is out of range.
    */
    Screen::Ptr getScreen (int screenIndex) const;

    /** Retrieves a span of pointers to all `Screen` objects.

        @return A span of pointers to all `Screen` objects.
    */
    Span<const Screen* const> getScreens() const;

    //==============================================================================
    /** Retrieves a pointer to the primary `Screen` object.

        The primary screen is typically the main screen of the system where applications are initially screened.

        @return A pointer to the primary `Screen` object.
    */
    Screen::Ptr getPrimaryScreen() const;

    /** Retries a pointer to the screen containing the mouse cursor.

        @return A pointer to the `Screen` object which contains the mouse cursor.
    */
    Screen::Ptr getScreenContainingMouseCursor() const;

    /** Retries a pointer to the screen containing an absolute location.

        @return A pointer to the `Screen` object which contains the location.
    */
    Screen::Ptr getScreenContaining (const Point<float>& location) const;

    //==============================================================================
    /** Sets the mouse cursor to the specified cursor.

        @param cursorToSet The cursor to set.
    */
    void setMouseCursor (const MouseCursor& cursorToSet);

    /** Retrieves the current mouse cursor.

        @return The current mouse cursor.
    */
    MouseCursor getMouseCursor() const;

    /** Retrieves the current absolute mouse location.

        @return The current absolute mouse location.
    */
    Point<float> getCurrentMouseLocation() const;

    /** Retrieves the current absolute mouse location.

        @return The current absolute mouse location.
    */
    void setCurrentMouseLocation (const Point<float>& location);

    //==============================================================================
    /** Updates the list of screens. */
    void updateScreens();

    //==============================================================================
    /** @internal */
    void handleScreenConnected (int screenIndex);
    /** @internal */
    void handleScreenDisconnected (int screenIndex);
    /** @internal */
    void handleScreenMoved (int screenIndex);
    /** @internal */
    void handleScreenOrientationChanged (int screenIndex);

    //==============================================================================
    JUCE_DECLARE_SINGLETON (Desktop, false)

private:
    friend class YUPApplication;

    Desktop();

    Screen::Array screens;
    std::optional<MouseCursor> currentMouseCursor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Desktop)
};

} // namespace yup
