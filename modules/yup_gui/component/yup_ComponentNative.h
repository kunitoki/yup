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

namespace juce
{

class Component;

//==============================================================================

class JUCE_API ComponentNative
{
public:
    ComponentNative (Component& newComponent);
    virtual ~ComponentNative();

    virtual void setTitle (const String& title) = 0;
    virtual String getTitle() const = 0;

    virtual void setVisible (bool shouldBeVisible) = 0;
    virtual bool isVisible() const = 0;

    virtual void setSize (const Size<int>& newSize) = 0;
    virtual Size<int> getSize() const = 0;
    virtual Size<int> getContentSize() const = 0;

    virtual void* getNativeHandle() const = 0;

    void handleMouseMove (const MouseEvent& event);
    void handleMouseDrag (const MouseEvent& event);
    void handleMouseDown (const MouseEvent& event);
    void handleMouseUp (const MouseEvent& event);
    void handleKeyDown (const KeyPress& keys, double x, double y);
    void handleKeyUp (const KeyPress& keys, double x, double y);
    void handleUserTriedToCloseWindow();

    static std::unique_ptr<ComponentNative> createFor (Component& component);

private:
    Component& component;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentNative)
};

} // namespace juce
