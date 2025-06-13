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

/**
    The Component class is the base class for all GUI components.

    It provides a common interface for all components, and is used to create and manage GUI components.
    It is a lightweight class that is used to create and manage GUI components.
 */
class YUP_API Component
{
public:
    //==============================================================================
    /**
        Constructor for the Component class.

        @param componentID The ID of the component.
     */
    Component();

    /**
        Constructor for the Component class.

        @param componentID The ID of the component.
     */
    Component (StringRef componentID);

    /** Destructor for the Component class. */
    virtual ~Component();

    //==============================================================================
    /**
        Get the ID of the component.

        @return The ID of the component.
     */
    String getComponentID() const;

    //==============================================================================
    /**
        Check if the component is enabled.

        @return True if the component is enabled, false otherwise.
     */
    bool isEnabled() const;

    /**
        Set the enabled state of the component.

        @param shouldBeEnabled True if the component should be enabled, false otherwise.
     */
    void setEnabled (bool shouldBeEnabled);

    /**
        Called when the enabled state of the component changes.
     */
    virtual void enablementChanged();

    //==============================================================================
    /**
        Check if the component is visible.

        @return True if the component is visible, false otherwise.
     */
    bool isVisible() const;

    /**
        Set the visible state of the component.

        @param shouldBeVisible True if the component should be visible, false otherwise.
     */
    void setVisible (bool shouldBeVisible);

    /**
        Check if the component is showing.

        A component is showing if it is visible and all of its parents are also visible.

        @return True if the component is showing, false otherwise.
     */
    bool isShowing() const;

    /**
        Called when the visible state of the component changes.
     */
    virtual void visibilityChanged();

    //==============================================================================
    /**
        Get the title of the component.

        @return The title of the component.
     */
    String getTitle() const;

    /**
        Set the title of the component.

        @param title The new title of the component.
     */
    void setTitle (const String& title);

    //==============================================================================
    /**
        Get the position of the component relative to its parent.

        @return The position of the component relative to its parent.
     */
    Point<float> getPosition() const;

    /**
        Set the position of the component relative to its parent.

        @param newPosition The new position of the component relative to its parent.
     */
    void setPosition (const Point<float>& newPosition);

    /**
        Get the x position of the component relative to its parent.

        @return The x position of the component relative to its parent.
     */
    float getX() const;

    /**
        Get the y position of the component relative to its parent.

        @return The y position of the component relative to its parent.
     */
    float getY() const;

    /**
        Get the left position of the component relative to its parent.

        @return The left position of the component relative to its parent.
     */
    float getLeft() const;

    /**
        Get the top position of the component relative to its parent.

        @return The top position of the component relative to its parent.
     */
    float getTop() const;

    /**
        Get the right position of the component relative to its parent.

        @return The right position of the component relative to its parent.
     */
    float getRight() const;

    /**
        Get the bottom position of the component relative to its parent.

        @return The bottom position of the component relative to its parent.
     */
    float getBottom() const;

    Point<float> getTopLeft() const;
    void setTopLeft (const Point<float>& newTopLeft);

    Point<float> getBottomLeft() const;
    void setBottomLeft (const Point<float>& newBottomLeft);

    Point<float> getTopRight() const;
    void setTopRight (const Point<float>& newTopRight);

    Point<float> getBottomRight() const;
    void setBottomRight (const Point<float>& newBottomRight);

    Point<float> getCenter() const;
    void setCenter (const Point<float>& newCenter);

    float getCenterX() const;
    void setCenterX (float newCenterX);

    float getCenterY() const;
    void setCenterY (float newCenterY);

    /**
        Called when the position of the component changes.
     */
    virtual void moved();

    //==============================================================================
    /**
        Get the size of the component.
     */
    Size<float> getSize() const;

    /**
        Set the size of the component.

        @param newSize The new size of the component.
     */
    void setSize (const Size<float>& newSize);

    /**
        Get the width of the component.

        @return The width of the component.
     */
    float getWidth() const;

    /**
        Get the height of the component.

        @return The height of the component.
     */
    float getHeight() const;

    /**
        Get the width of the component as a proportion of the parent's width.

        @param proportion The proportion of the parent's width to get.

        @return The width of the component as a proportion of the parent's width.
     */
    float proportionOfWidth (float proportion) const;

    /**
        Get the height of the component as a proportion of the parent's height.

        @param proportion The proportion of the parent's height to get.

        @return The height of the component as a proportion of the parent's height.
     */
    float proportionOfHeight (float proportion) const;

    /**
        Called when the size of the component changes.
     */
    virtual void resized();

    //==============================================================================
    /**
        Set the bounds of the component.

        @param newBounds The new bounds of the component.
     */
    void setBounds (const Rectangle<float>& newBounds);

    /**
        Get the bounds of the component, relative to its parent.

        @return The bounds of the component.
     */
    Rectangle<float> getBounds() const;

    /**
        Get the bounds of the component in screen coordinates.

        @return The bounds of the component in screen coordinates.
     */
    Rectangle<float> getScreenBounds() const;

    /**
        Get the local bounds of the component.

        The local bounds is the same as getBounds() but with the position set to (0, 0).

        @return The local bounds of the component.
     */
    Rectangle<float> getLocalBounds() const;

    /**
        Get the bounds of the component relative to the top level component.

        @return The bounds of the component relative to the top level component.
     */
    Rectangle<float> getBoundsRelativeToTopLevelComponent() const;

    /**
        Get the position of the component in absolute screen coordinates.

        This method traverses up the parent hierarchy to calculate the component's
        absolute position on the screen.

        @return The absolute screen position of the component.
     */
    Point<float> getScreenPosition() const;

    //==============================================================================

    void setTransform (const AffineTransform& transform);

    AffineTransform getTransform() const;

    bool isTransformed() const;

    virtual void transformChanged();

    //==============================================================================
    /**
        Set the full screen state of the component.

        @param shouldBeFullScreen True if the component should be full screen, false otherwise.
     */
    void setFullScreen (bool shouldBeFullScreen);

    /**
        Check if the component is full screen.
     */
    bool isFullScreen() const;

    //==============================================================================
    /**
        Called when the component changes display.
     */
    virtual void displayChanged();

    //==============================================================================
    /**
        Get the scale factor of the component.

        @return The scale factor of the component.
     */
    float getScaleDpi() const;

    /**
        Called when the content scale of the component changes.

        @param dpiScale The new scale factor of the component.
     */
    virtual void contentScaleChanged (float dpiScale);

    //==============================================================================
    /**
        Get the opacity of the component.

        @return The opacity of the component.
     */
    float getOpacity() const;

    /**
        Set the opacity of the component.

        @param opacity The new opacity of the component.
     */
    void setOpacity (float opacity);

    //==============================================================================
    /**
        Enable or disable unclipped rendering for the component.

        @param shouldBeEnabled True if the component should be rendered unclipped, false otherwise.
     */
    virtual void enableRenderingUnclipped (bool shouldBeEnabled);

    /**
        Check if the component is rendering unclipped.

        @return True if the component is rendering unclipped, false otherwise.
     */
    bool isRenderingUnclipped() const;

    /**
        Repaint the component.
     */
    void repaint();

    /**
        Repaint the component.

        @param rect The rectangle to repaint.
     */
    void repaint (const Rectangle<float>& rect);

    //==============================================================================
    /**
        Get the native handle of the component.

        @return The native handle of the component.
     */
    void* getNativeHandle() const;

    /**
        Get the native component of the component.

        @return The native component of the component.
     */
    ComponentNative* getNativeComponent();

    /**
        Get the native component of the component.

        @return The native component of the component.
     */
    const ComponentNative* getNativeComponent() const;

    /** Called when this component is attached to a native window.

        Override this to perform any setup required when the component gets a native window.
    */
    virtual void attachedToNative();

    /** Called when this component is detached from its native window.

        Override this to perform any cleanup when the component loses its native window.
    */
    virtual void detachedFromNative();

    //==============================================================================
    /**
        Check if the component is on the desktop.

        @return True if the component is on the desktop, false otherwise.
     */
    bool isOnDesktop() const;

    /**
        Add the component to the desktop.

        @param nativeOptions The native options of the component.
        @param parent The parent of the component.
     */
    void addToDesktop (const ComponentNative::Options& nativeOptions, void* parent = nullptr);

    /**
        Remove the component from the desktop.
     */
    void removeFromDesktop();

    /**
        Called when the user tries to close the window.
     */
    virtual void userTriedToCloseWindow();

    //==============================================================================
    /**
        Bring the component to the front.
     */
    void toFront (bool shouldGainKeyboardFocus);

    /**
        Bring the component to the back.
     */
    void toBack();

    /** Raises this component above the specified component in the z-order.

        @param component The component to raise above.
    */
    void raiseAbove (Component* component);

    /** Lowers this component below the specified component in the z-order.

        @param component The component to lower below.
    */
    void lowerBelow (Component* component);

    /** Raises this component by a number of positions in the z-order.

        @param indexToRaise The number of positions to raise by.
    */
    void raiseBy (int indexToRaise);

    /** Lowers this component by a number of positions in the z-order.

        @param indexToLower The number of positions to lower by.
    */
    void lowerBy (int indexToLower);

    //==============================================================================
    /**
        Set the mouse cursor of the component.

        @param cursorType The new mouse cursor of the component.
     */
    void setMouseCursor (const MouseCursor& cursorType);

    /**
        Get the mouse cursor of the component.

        @return The mouse cursor of the component.
     */
    virtual MouseCursor getMouseCursor() const;

    //==============================================================================
    /**
        Set if the component wants keyboard focus.

        @param wantsFocus True if the component wants keyboard focus, false otherwise.
     */
    void setWantsKeyboardFocus (bool wantsFocus);

    /**
        Take the focus.
     */
    void takeKeyboardFocus();

    /**
        Leave the focus.
     */
    void leaveKeyboardFocus();

    /**
        Check if the component has focus.

        @return True if the component has focus, false otherwise.
     */
    bool hasKeyboardFocus() const;

    /**
        Called when the component gains focus.
     */
    virtual void focusGained();

    /**
        Called when the component loses focus.
     */
    virtual void focusLost();

    //==============================================================================
    /**
        Get the parent of the component.

        @return The parent of the component.
     */
    Component* getParentComponent();

    /**
        Get the parent of the component.

        @return The parent of the component.
     */
    const Component* getParentComponent() const;

    /**
        Get the parent of the component of a given type.

        @return The parent of the component of the given type.
     */
    template <class T>
    T* getParentComponentWithType()
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

    /**
        Get the parent of the component of a given type.

        @return The parent of the component of the given type.
     */
    template <class T>
    const T* getParentComponentWithType() const
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
    /** Add a child component to the component.

        @param component The child component to add.
     */
    void addChildComponent (Component& component, int index = -1);

    /** Add a child component to the component.

        @param component The child component to add.
     */
    void addChildComponent (Component* component, int index = -1);

    /** Add a child component to the component and make it visible.

        @param component The child component to add.
     */
    void addAndMakeVisible (Component& component, int index = -1);

    /** Add a child component to the component and make it visible.

        @param component The child component to add.
     */
    void addAndMakeVisible (Component* component, int index = -1);

    /** Remove a child component from the component.

        @param component The child component to remove.
     */
    void removeChildComponent (Component& component);

    /** Remove a child component from the component.

        @param component The child component to remove.
     */
    void removeChildComponent (Component* component);

    /**
        Remove a child component from the component at a specified index.

        @param index The child component index to remove.
     */
    void removeChildComponent (int index);

    /** Removes all child components from this component.

        The removed components will be deleted if not referenced elsewhere.
    */
    void removeAllChildren();

    /** Called when the parent hierarchy of this component changes.

        This happens when the component is added to or removed from a parent component.
    */
    virtual void parentHierarchyChanged();

    /** Called when the child components of this component change.

        This happens when child components are added or removed.
    */
    virtual void childrenChanged();

    //==============================================================================
    /**
        Get the number of child components of the component.
     */
    int getNumChildComponents() const;

    /**
        Get the child component at a given index.

        @param index The index of the child component to get.
     */
    Component* getComponentAt (int index) const;

    /**
        Returns the index of a child component, or -1 if not found.
     */
    int getIndexOfChildComponent (Component* component) const;

    /**
        Find the child component at a given point.

        @param p The point to find the child component at.
     */
    Component* findComponentAt (const Point<float>& p);

    /**
        Returns the top level component.
    */
    Component* getTopLevelComponent();

    //==============================================================================
    /**
        Get the properties of the component.

        @return The properties of the component.
     */
    NamedValueSet& getProperties();

    /**
        Get the properties of the component.

        @return The properties of the component.
     */
    const NamedValueSet& getProperties() const;

    //==============================================================================
    /**
        Paint the component.

        @param g The graphics context to paint the component on.
     */
    virtual void paint (Graphics& g);

    /**
        Paint the component over its children.

        @param g The graphics context to paint the component on.
     */
    virtual void paintOverChildren (Graphics& g);

    //==============================================================================
    /**
        Refresh the display of the component.

        @param lastFrameTimeSeconds The time since the last frame.
     */
    virtual void refreshDisplay (double lastFrameTimeSeconds);

    //==============================================================================
    /**
        Called when the mouse enters the component.

        @param event The mouse event.
     */
    virtual void mouseEnter (const MouseEvent& event);

    /**
        Called when the mouse exits the component.

        @param event The mouse event.
     */
    virtual void mouseExit (const MouseEvent& event);

    /**
        Called when the mouse button is pressed.

        @param event The mouse event.
     */
    virtual void mouseDown (const MouseEvent& event);

    /**
        Called when the mouse is moved.

        @param event The mouse event.
     */
    virtual void mouseMove (const MouseEvent& event);

    /**
        Called when the mouse is dragged.

        @param event The mouse event.
     */
    virtual void mouseDrag (const MouseEvent& event);

    /**
        Called when the mouse button is released.

        @param event The mouse event.
     */
    virtual void mouseUp (const MouseEvent& event);

    /**
        Called when the mouse button is double clicked.

        @param event The mouse event.
     */
    virtual void mouseDoubleClick (const MouseEvent& event);

    /**
        Called when the mouse wheel is scrolled.

        @param event The mouse event.
        @param wheelData The mouse wheel data.
     */
    virtual void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData);

    //==============================================================================
    /**
        Add a mouse listener to the component.

        @param listener The mouse listener to add.
     */
    void addMouseListener (MouseListener* listener);

    /**
        Remove a mouse listener from the component.

        @param listener The mouse listener to remove.
     */
    void removeMouseListener (MouseListener* listener);

    //==============================================================================
    /**
        Called when a key is pressed.

        @param keys The key press.
        @param position The position of the key press.
     */
    virtual void keyDown (const KeyPress& keys, const Point<float>& position);

    /**
        Called when a key is released.

        @param keys The key press.
        @param position The position of the key press.
     */
    virtual void keyUp (const KeyPress& keys, const Point<float>& position);

    /**
        Called when text is input.

        @param text The text input.
     */
    virtual void textInput (const String& text);

    //==============================================================================

    /** Sets the style for this component.

        @param newStyle The new style to apply to the component.
    */
    void setStyle (ComponentStyle::Ptr newStyle);

    /** Returns the current style of this component.

        @return The component's current style.
    */
    ComponentStyle::Ptr getStyle() const;

    /** Called when the component's style changes.

        Override this to perform custom actions when the style is modified.
    */
    virtual void styleChanged();

    //==============================================================================
    /** Set a color for the component.

        @param colorId The identifier of the color to set.
        @param color The color to set.
     */
    void setColor (const Identifier& colorId, const std::optional<Color>& color);

    /** Get the color of the component.

        @param colorId The identifier of the color to get.
     */
    std::optional<Color> getColor (const Identifier& colorId) const;

    /** Find the color of the component.

        @param colorId The identifier of the color to find.

        @return The color of the component.
     */
    std::optional<Color> findColor (const Identifier& colorId) const;

    //==============================================================================
    /** A bail out checker for the component. */
    class BailOutChecker
    {
    public:
        /** Constructor for the BailOutChecker class.

            @param component The component to check.
         */
        explicit BailOutChecker (Component* component)
            : componentWeak (component)
        {
        }

        /** Copy and move constructors and assignment operators for the BailOutChecker class. */
        BailOutChecker (const BailOutChecker& other) = default;
        BailOutChecker (BailOutChecker&& other) = default;
        BailOutChecker& operator= (const BailOutChecker& other) = default;
        BailOutChecker& operator= (BailOutChecker&& other) = default;

        /** Check if the component should bail out.

            @return True if the component should bail out, false otherwise.
         */
        bool shouldBailOut() const
        {
            return componentWeak.get() == nullptr;
        }

    private:
        WeakReference<Component> componentWeak;
    };

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
    void internalResized (int width, int height);
    void internalMoved (int xpos, int ypos);
    void internalFocusChanged (bool gotFocus);
    void internalDisplayChanged();
    void internalContentScaleChanged (float dpiScale);
    void internalUserTriedToCloseWindow();
    void internalHierarchyChanged();

    void updateMouseCursor();

    friend class ComponentNative;
    friend class SDL2ComponentNative;
    friend class WeakReference<Component>;

    using MouseListenerList = ListenerList<MouseListener, Array<WeakReference<MouseListener>>>;

    String componentID, componentTitle;
    Component* parentComponent = nullptr;
    Array<Component*> children;
    Rectangle<float> boundsInParent;
    AffineTransform transform;
    ComponentNative::Ptr native;
    WeakReference<Component>::Master masterReference;
    MouseListenerList mouseListeners;
    ComponentStyle::Ptr style;
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
        bool isRepainting : 1;
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

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Component)
};

} // namespace yup
