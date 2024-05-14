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
    Point<int> getPosition() const;
    int getX() const;
    int getY() const;

    virtual void moved();

    //==============================================================================
    virtual void setSize (const Size<int>& newSize);
    Size<int> getSize() const;
    Size<int> getContentSize() const;
    int getWidth() const;
    int getHeight() const;

    virtual void setBounds (const Rectangle<int>& newBounds);
    Rectangle<int> getBounds() const;
    Rectangle<int> getLocalBounds() const;

    int proportionOfWidth (float proportion) const;
    int proportionOfHeight (float proportion) const;

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

    void insertChildComponent (Component& component, int index);
    void insertChildComponent (Component* component, int index);

    void removeChildComponent (Component& component);
    void removeChildComponent (Component* component);

    //==============================================================================
    int getNumChildComponents() const;
    Component* getComponentAt (int index) const;
    Component* findComponentAt (const Point<float>& p);

    //==============================================================================
    void toFront();
    void toBack();

    //==============================================================================
    void takeFocus();
    void leaveFocus();

    //==============================================================================
    virtual void paint (Graphics& g, float frameRate);
    virtual void paintOverChildren (Graphics& g, float frameRate);

    //==============================================================================
    virtual void mouseEnter (const MouseEvent& event);
    virtual void mouseExit (const MouseEvent& event);
    virtual void mouseDown (const MouseEvent& event);
    virtual void mouseMove (const MouseEvent& event);
    virtual void mouseDrag (const MouseEvent& event);
    virtual void mouseUp (const MouseEvent& event);
    virtual void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData);

    //==============================================================================
    virtual void keyDown (const KeyPress& keys, const Point<float>& position);
    virtual void keyUp (const KeyPress& keys, const Point<float>& position);

private:
    void internalPaint (Graphics& g, float frameRate);
    void internalMouseEnter (const MouseEvent& event);
    void internalMouseExit (const MouseEvent& event);
    void internalMouseDown (const MouseEvent& event);
    void internalMouseMove (const MouseEvent& event);
    void internalMouseDrag (const MouseEvent& event);
    void internalMouseUp (const MouseEvent& event);
    void internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData);
    void internalKeyDown (const KeyPress& keys, const Point<float>& position);
    void internalKeyUp (const KeyPress& keys, const Point<float>& position);
    void internalMoved (int xpos, int ypos);
    void internalResized (int width, int height);
    void internalUserTriedToCloseWindow();

    friend class ComponentNative;
    friend class GLFWComponentNative;
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

} // namespace yup
