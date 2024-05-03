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

//==============================================================================

class JUCE_API Component
{
public:
    //==============================================================================
    Component();
    Component (StringRef componentID);

    virtual ~Component();

    //==============================================================================
    virtual void setVisible (bool shouldBeVisible);
    bool isVisible() const;

    virtual void visibilityChanged();

    //==============================================================================
    virtual void setTitle (const String& title);
    String getTitle() const;

    //==============================================================================
    virtual void setSize (const Size<int>& newSize);
    Size<int> getSize() const;
    Size<int> getContentSize() const;
    int getWidth() const;
    int getHeight() const;

    Point<int> getPosition() const;
    int getX() const;
    int getY() const;

    virtual void setBounds (const Rectangle<int>& newBounds);
    Rectangle<int> getBounds() const;
    Rectangle<int> getLocalBounds() const;

    virtual void resized();

    //==============================================================================
    virtual void setFullScreen (bool shouldBeFullScreen);
    bool isFullScreen() const;

    //==============================================================================
    float getScaleDpi() const;

    //==============================================================================
    virtual void setOpacity (float opacity);
    float getOpacity() const;

    //==============================================================================
    void* getNativeHandle() const;

    //==============================================================================
    ComponentNative* getNativeComponent();

    //==============================================================================
    void addToDesktop (std::optional<float> framerateRedraw = std::nullopt);
    void removeFromDesktop();
    bool isOnDesktop() const noexcept;

    virtual void userTriedToCloseWindow();

    //==============================================================================
    void addChildComponent (Component& component);
    void addChildComponent (Component* component);

    void addAndMakeVisible (Component& component);
    void addAndMakeVisible (Component* component);

    void removeChildComponent (Component& component);
    void removeChildComponent (Component* component);

    //==============================================================================
    virtual void paint (Graphics& g, float frameRate);
    virtual void paintOverChildren (Graphics& g, float frameRate);

    //==============================================================================
    virtual void mouseDown (const MouseEvent& event);
    virtual void mouseMove (const MouseEvent& event);
    virtual void mouseDrag (const MouseEvent& event);
    virtual void mouseUp (const MouseEvent& event);

    //==============================================================================
    virtual void keyDown (const KeyPress& keys, double x, double y);
    virtual void keyUp (const KeyPress& keys, double x, double y);

private:
    void internalPaint (Graphics& g, float frameRate);
    void internalMouseDown (const MouseEvent& event);
    void internalMouseMove (const MouseEvent& event);
    void internalMouseDrag (const MouseEvent& event);
    void internalMouseUp (const MouseEvent& event);
    void internalKeyDown (const KeyPress& keys, double x, double y);
    void internalKeyUp (const KeyPress& keys, double x, double y);
    void internalResized (int width, int height);
    void internalUserTriedToCloseWindow();

    friend class ComponentNative;
    friend class WeakReference<Component>;

    String componentID, componentTitle;
    Component* parentComponent = nullptr;
    Array<Component*> children;
    Rectangle<float> boundsInParent;
    std::unique_ptr<ComponentNative> native;
    WeakReference<Component>::Master masterReference;
    uint8 opacity = 255;

    struct Options
    {
        bool isVisible    : 1;
        bool hasFrame     : 1;
        bool onDesktop    : 1;
        bool isFullScreen : 1;
    };

    union
    {
        uint32 optionsValue;
        Options options;
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Component)
};

} // namespace juce
