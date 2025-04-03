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
    String getComponentID() const;

    //==============================================================================
    bool isEnabled() const;
    virtual void setEnabled (bool shouldBeEnabled);
    virtual void enablementChanged();

    //==============================================================================
    bool isVisible() const;
    virtual void setVisible (bool shouldBeVisible);
    virtual void visibilityChanged();

    //==============================================================================
    String getTitle() const;
    virtual void setTitle (const String& title);

    //==============================================================================
    virtual void setPosition (const Point<float>& newPosition);
    Point<float> getPosition() const;
    float getX() const;
    float getY() const;
    virtual void moved();

    //==============================================================================
    virtual void setSize (const Size<float>& newSize);
    Size<float> getSize() const;
    float getWidth() const;
    float getHeight() const;
    virtual void setBounds (const Rectangle<float>& newBounds);
    Rectangle<float> getBounds() const;
    Rectangle<float> getLocalBounds() const;
    Rectangle<float> getBoundsRelativeToAncestor() const;
    float proportionOfWidth (float proportion) const;
    float proportionOfHeight (float proportion) const;
    virtual void resized();

    //==============================================================================
    virtual void setFullScreen (bool shouldBeFullScreen);
    bool isFullScreen() const;

    //==============================================================================
    float getScaleDpi() const;
    virtual void contentScaleChanged (float dpiScale);

    //==============================================================================
    float getOpacity() const;
    virtual void setOpacity (float opacity);

    //==============================================================================
    virtual void enableRenderingUnclipped (bool shouldBeEnabled);
    bool isRenderingUnclipped() const;

    void repaint();
    void repaint (const Rectangle<float>& rect);

    //==============================================================================
    void* getNativeHandle() const;

    //==============================================================================
    ComponentNative* getNativeComponent();
    const ComponentNative* getNativeComponent() const;

    //==============================================================================
    bool isOnDesktop() const;
    void addToDesktop (const ComponentNative::Options& nativeOptions, void* parent);
    void removeFromDesktop();

    virtual void userTriedToCloseWindow();

    //==============================================================================
    Component* getParentComponent();
    const Component* getParentComponent() const;

    template <class T>
    T* getParentComponentOfType()
    {
        auto parent = parentComponent;
        while (parent != nullptr)
        {
            if (auto foundParent = dynamic_cast<T*> (parent))
                return foundParent;

            parent = parent->getParentComponent();
        }

        return nullptr;
    }

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
    void setMouseCursor (const MouseCursor& cursorType);
    virtual MouseCursor getMouseCursor() const;

    //==============================================================================
    void setWantsKeyboardFocus (bool wantsFocus);

    void takeFocus();
    void leaveFocus();
    bool hasFocus() const;

    virtual void focusGained();
    virtual void focusLost();

    //==============================================================================
    void setColor (const Identifier& colorId, const std::optional<Color>& color);
    std::optional<Color> getColor (const Identifier& colorId) const;
    std::optional<Color> findColor (const Identifier& colorId) const;

    //==============================================================================
    NamedValueSet& getProperties();
    const NamedValueSet& getProperties() const;

    //==============================================================================
    virtual void paint (Graphics& g);
    virtual void paintOverChildren (Graphics& g);

    //==============================================================================
    virtual void refreshDisplay (double lastFrameTimeSeconds);

    //==============================================================================
    virtual void mouseEnter (const MouseEvent& event);
    virtual void mouseExit (const MouseEvent& event);
    virtual void mouseDown (const MouseEvent& event);
    virtual void mouseMove (const MouseEvent& event);
    virtual void mouseDrag (const MouseEvent& event);
    virtual void mouseUp (const MouseEvent& event);
    virtual void mouseDoubleClick (const MouseEvent& event);
    virtual void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData);

    //==============================================================================
    void addMouseListener (MouseListener* listener);
    void removeMouseListener (MouseListener* listener);

    //==============================================================================
    virtual void keyDown (const KeyPress& keys, const Point<float>& position);
    virtual void keyUp (const KeyPress& keys, const Point<float>& position);
    virtual void textInput (const String& text);

    //==============================================================================
    template <class F, class... Args>
    auto createSafeCallback (F&& func, Args&&... args)
    {
        return [weakThis = WeakReference<Component> (this),
                func = std::forward<F> (func),
                args = std::forward_as_tuple (std::forward<Args> (args)...)]
        {
            if (weakThis.get() != nullptr)
                std::apply (func, args);
        };
    }

private:
    void internalRefreshDisplay (double lastFrameTimeSeconds);
    void internalPaint (Graphics& g, const Rectangle<float>& repaintArea, bool renderContinuous);
    void internalMouseEnter (const MouseEvent& event);
    void internalMouseExit (const MouseEvent& event);
    void internalMouseDown (const MouseEvent& event);
    void internalMouseMove (const MouseEvent& event);
    void internalMouseDrag (const MouseEvent& event);
    void internalMouseUp (const MouseEvent& event);
    void internalMouseDoubleClick (const MouseEvent& event);
    void internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData);
    void internalKeyDown (const KeyPress& keys, const Point<float>& position);
    void internalKeyUp (const KeyPress& keys, const Point<float>& position);
    void internalTextInput (const String& text);
    void internalMoved (int xpos, int ypos);
    void internalResized (int width, int height);
    void internalContentScaleChanged (float dpiScale);
    void internalUserTriedToCloseWindow();

    void updateMouseCursor();

    friend class ComponentNative;
    friend class GLFWComponentNative;
    friend class SDL2ComponentNative;
    friend class WeakReference<Component>;

    using MouseListenerList = ListenerList<MouseListener, Array<WeakReference<MouseListener>>>;

    String componentID, componentTitle;
    Component* parentComponent = nullptr;
    Array<Component*> children;
    Rectangle<float> boundsInParent;
    std::unique_ptr<ComponentNative> native;
    WeakReference<Component>::Master masterReference;
    MouseListenerList mouseListeners;
    NamedValueSet properties;
    MouseCursor mouseCursor;
    uint8 opacity = 255;

    struct Options
    {
        bool isVisible : 1;
        bool isDisabled : 1;
        bool hasFrame : 1;
        bool onDesktop : 1;
        bool isFullScreen : 1;
        bool unclippedRendering : 1;
        bool wantsKeyboardFocus : 1;
        bool wantsTextInput : 1;
    };

    union
    {
        uint32 optionsValue;
        Options options;
    };

#if YUP_ENABLE_COMPONENT_REPAINT_DEBUGGING
    Color debugColor = Color::opaqueRandom();
    int counter = 2;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Component)
};

} // namespace yup
