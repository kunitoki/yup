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

class Component;

//==============================================================================
class JUCE_API ComponentNative
{
public:
    //==============================================================================
    ComponentNative (Component& newComponent);
    virtual ~ComponentNative();

    //==============================================================================
    virtual void setTitle (const String& title) = 0;
    virtual String getTitle() const = 0;

    //==============================================================================
    virtual void setVisible (bool shouldBeVisible) = 0;
    virtual bool isVisible() const = 0;

    //==============================================================================
    virtual void setSize (const Size<int>& newSize) = 0;
    virtual Size<int> getSize() const = 0;
    virtual Size<int> getContentSize() const = 0;

    virtual Point<int> getPosition() const = 0;
    virtual void setPosition (const Point<int>& newPosition) = 0;

    virtual Rectangle<int> getBounds() const = 0;
    virtual void setBounds (const Rectangle<int>& newBounds) = 0;

    //==============================================================================
    virtual void setFullScreen (bool shouldBeFullScreen) = 0;
    virtual bool isFullScreen() const = 0;

    //==============================================================================
    virtual void setOpacity (float opacity) = 0;
    virtual float getOpacity() const = 0;

    //==============================================================================
    virtual void setFocusedComponent (Component* comp) = 0;
    virtual Component* getFocusedComponent() const = 0;

    //==============================================================================
    virtual bool isContinuousRepaintingEnabled() const = 0;
    virtual void enableContinuousRepainting (bool shouldBeEnabled) = 0;
    virtual bool isAtomicModeEnabled() const = 0;
    virtual void enableAtomicMode (bool shouldBeEnabled) = 0;
    virtual bool isWireframeEnabled() const = 0;
    virtual void enableWireframe (bool shouldBeEnabld) = 0;

    //==============================================================================
    virtual float getScaleDpi() const = 0;

    //==============================================================================
    virtual float getCurrentFrameRate() const = 0;

    //==============================================================================
    virtual void* getNativeHandle() const = 0;

    //==============================================================================
    virtual rive::Factory* getFactory() = 0;

    //==============================================================================
    static std::unique_ptr<ComponentNative> createFor (Component& component,
                                                       bool continuousRepaint,
                                                       std::optional<float> framerateRedraw);

protected:
    Component& component;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentNative)
};

} // namespace yup
