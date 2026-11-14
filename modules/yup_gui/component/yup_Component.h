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

#ifndef DOXYGEN
/** @internal Test-only accessor, specialized by unit tests to reach private Component state. */
template <class T>
class ComponentTestHelper;
#endif

class StyleContext;
class StyleSheet;

//==============================================================================

/**
    The Component class is the base class for all GUI components.

    It provides a common interface for all components, and is used to create and manage GUI components.
    It is a lightweight class that is used to create and manage GUI components.
 */
class YUP_API Component
    : public MouseListener
    , public GestureListener
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
        Returns the reflected type info for this component.

        Widgets override this to return their own static type info, which describes the
        properties, signals and factory used by the stylesheet, `.yui` files and tooling.
        Unregistered subclasses inherit the type info of their nearest registered base.

        @see ComponentTypeInfo, getClassTypeInfo
    */
    virtual const ComponentTypeInfo& getTypeInfo() const;

    /** Returns the static type info describing the Component base type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    //==============================================================================
    /**
        Get the ID of the component.

        @return The ID of the component.
     */
    String getComponentID() const;

    /**
        Set the ID of the component.

        The ID is matched by `#id` style selectors, so changing it restyles the component.

        @param newComponentID The new ID of the component.
     */
    void setComponentID (StringRef newComponentID);

    //==============================================================================
    /**
        Check if the component is enabled.

        @return True if the component is enabled, false otherwise.
     */
    bool isEnabled() const;

    /**
        Set the enabled state of the component.

        A disabled component, or one inside a disabled parent, receives no key presses,
        gestures, mouse enters, presses, moves, drags or double clicks. It still receives
        mouse ups and exits, so an interaction started before it got disabled can finish,
        and shows the default mouse cursor rather than its own.

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
    virtual void setVisible (bool shouldBeVisible);

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
    virtual void setTitle (const String& title);

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
        Get the position of the component in absolute screen coordinates.

        This method traverses up the parent hierarchy to calculate the component's
        absolute position on the screen.

        @return The absolute screen position of the component.
     */
    Point<float> getScreenPosition() const;

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

    /**
        Get the top left position of the component relative to its parent.

        @return The top left position of the component relative to its parent.
     */
    Point<float> getTopLeft() const;

    /**
        Set the top left position of the component relative to its parent.

        @param newTopLeft The new top left position of the component relative to its parent.
     */
    void setTopLeft (const Point<float>& newTopLeft);

    /**
        Get the bottom left position of the component relative to its parent.

        @return The bottom left position of the component relative to its parent.
     */
    Point<float> getBottomLeft() const;

    /**
        Set the bottom left position of the component relative to its parent.

        @param newBottomLeft The new bottom left position of the component relative to its parent.
     */
    void setBottomLeft (const Point<float>& newBottomLeft);

    /**
        Get the top right position of the component relative to its parent.

        @return The top right position of the component relative to its parent.
     */
    Point<float> getTopRight() const;

    /**
        Set the top right position of the component relative to its parent.

        @param newTopRight The new top right position of the component relative to its parent.
     */
    void setTopRight (const Point<float>& newTopRight);

    /**
        Get the bottom right position of the component relative to its parent.

        @return The bottom right position of the component relative to its parent.
     */
    Point<float> getBottomRight() const;

    /**
        Set the bottom right position of the component relative to its parent.

        @param newBottomRight The new bottom right position of the component relative to its parent.
     */
    void setBottomRight (const Point<float>& newBottomRight);

    /**
        Get the center position of the component relative to its parent.

        @return The center position of the component relative to its parent.
     */
    Point<float> getCenter() const;

    /**
        Set the center position of the component relative to its parent.

        @param newCenter The new center position of the component relative to its parent.
     */
    void setCenter (const Point<float>& newCenter);

    /**
        Get the center x position of the component relative to its parent.

        @return The center x position of the component relative to its parent.
     */
    float getCenterX() const;

    /**
        Set the center x position of the component relative to its parent.

        @param newCenterX The new center x position of the component relative to its parent.
     */
    void setCenterX (float newCenterX);

    /**
        Get the center y position of the component relative to its parent.

        @return The center y position of the component relative to its parent.
     */
    float getCenterY() const;

    /**
        Set the center y position of the component relative to its parent.

        @param newCenterY The new center y position of the component relative to its parent.
     */
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

        @param width The new width of the component.
        @param height The new height of the component.
     */
    void setSize (float width, float height);

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

    /**
        Called on each child when the size of this component's parent changes.

        This is called after the parent's resized() has run, so any layout the parent
        performs from resized() is already reflected in this component's bounds.

        @see resized, childBoundsChanged
     */
    virtual void parentSizeChanged();

    //==============================================================================
    /**
        Set the bounds of the component.

        @param x The new x position of the component.
        @param y The new y position of the component.
        @param width The new width of the component.
        @param height The new height of the component.
     */
    void setBounds (float x, float y, float width, float height);

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
        Get the bounds of the component in screen coordinates.

        @return The bounds of the component in screen coordinates.
     */
    Rectangle<float> getScreenBounds() const;

    /**
        Get the area of the component that is safe for interactive content.

        On mobile devices the window may extend under display cutouts (notch), the
        status bar or rounded corners: this returns the portion of the component that
        is guaranteed to be fully visible and touchable. On desktop platforms this
        usually matches the local bounds.

        @return The safe area bounds, in local component coordinates.
     */
    Rectangle<float> getSafeAreaBounds() const;

    /**
        Called when the safe area of the component changes.

        This can happen on mobile devices when the device is rotated, or when system
        bars are shown or hidden.

        @see getSafeAreaBounds
     */
    virtual void safeAreaChanged();

    //==============================================================================

    /**
        Convert a point from local coordinates to screen coordinates.

        The point goes through every ancestor, honoring component transforms, the
        parents' getLocalPointFromChild() and the effects' ComponentEffect::contentToDisplay().

        @param localPoint The point to convert.

        @return The point in screen coordinates.

        @see getParentPointFromLocal
     */
    Point<float> localToScreen (const Point<float>& localPoint) const;

    /**
        Convert a point from screen coordinates to local coordinates.

        This is the inverse of localToScreen(), and it is the same mapping used to deliver
        mouse events to this component.

        @param screenPoint The point to convert.

        @return The point in local coordinates.

        @see getLocalPointFromTopLevel
     */
    Point<float> screenToLocal (const Point<float>& screenPoint) const;

    /**
        Convert a rectangle from local coordinates to screen coordinates.

        When transforms or effects are involved, the result is the axis-aligned bounding box
        of the mapped corners.

        @param localRectangle The rectangle to convert.

        @return The rectangle in screen coordinates.
     */
    Rectangle<float> localToScreen (const Rectangle<float>& localRectangle) const;

    /**
        Convert a rectangle from screen coordinates to local coordinates.

        When transforms or effects are involved, the result is the axis-aligned bounding box
        of the mapped corners.

        @param screenRectangle The rectangle to convert.

        @return The rectangle in local coordinates.
     */
    Rectangle<float> screenToLocal (const Rectangle<float>& screenRectangle) const;

    //==============================================================================
    /**
        Maps a point in this component's local space to the local space of a direct child.

        Every input path (hit testing, mouse events, drag and drop, screenToLocal()) goes
        through this for each parent to child step, so overriding it lets a component present
        its children through a custom projection, for example a child composited by the parent
        onto a 3D surface (see setManuallyComposited()).

        The default implementation removes the child position and applies the inverse of
        the child transform.

        An override must return a point on the child's plane even when it falls outside the
        child bounds, so that a captured drag keeps tracking when the pointer leaves the child.
        Hit testing rejects points outside the child bounds by itself. Return std::nullopt only
        when the mapping is degenerate: hit testing then skips the child and the other
        conversions fall back to removing the child position.

        This runs on the message thread, while painting runs on the render thread: read a
        snapshot of the state that was last drawn (published from paint() through atomics or
        a lock), which also keeps input consistent with what is on screen.

        @param child       A direct child of this component.
        @param localPoint  The point in this component's local coordinates.

        @return The point in the child's local coordinates, or std::nullopt if the mapping is degenerate.

        @see getLocalPointFromChild, getLocalPointFromParent
     */
    virtual std::optional<Point<float>> getChildPointFromLocal (const Component& child, Point<float> localPoint) const;

    /**
        Maps a point in the local space of a direct child to this component's local space.

        This is the inverse of getChildPointFromLocal(), used by localToScreen() and everything
        built on it. Override both together. The same threading rules apply.

        The default implementation applies the child transform and adds the child position.

        @param child       A direct child of this component.
        @param childPoint  The point in the child's local coordinates.

        @return The point in this component's local coordinates, or std::nullopt if the mapping is degenerate.

        @see getChildPointFromLocal, getParentPointFromLocal
     */
    virtual std::optional<Point<float>> getLocalPointFromChild (const Component& child, Point<float> childPoint) const;

    /**
        Maps a point from the parent's local space to this component's local space.

        This applies the parent's getChildPointFromLocal() and then, if this component has an
        effect, ComponentEffect::displayToContent().

        @param parentPoint The point in the parent's local coordinates.

        @return The point in this component's local coordinates, or std::nullopt if this
                component has no parent or one of the mappings is degenerate.
     */
    std::optional<Point<float>> getLocalPointFromParent (Point<float> parentPoint) const;

    /**
        Maps a point from this component's local space to the parent's local space.

        This applies ComponentEffect::contentToDisplay() if this component has an effect,
        then the parent's getLocalPointFromChild().

        @param localPoint The point in this component's local coordinates.

        @return The point in the parent's local coordinates, or std::nullopt if this
                component has no parent or one of the mappings is degenerate.
     */
    std::optional<Point<float>> getParentPointFromLocal (Point<float> localPoint) const;

    /**
        Maps a point from the top level component's local space to this component's local space.

        This walks from the top level component down to this one through
        getLocalPointFromParent(). A step whose mapping is degenerate falls back to removing
        the component position. Points in the top level space are taken as they are, without
        applying the top level component's own transform or effect.

        This is how mouse events are made relative to the component receiving them.

        @param topLevelPoint The point in the top level component's local coordinates.

        @return The point in this component's local coordinates.
     */
    Point<float> getLocalPointFromTopLevel (Point<float> topLevelPoint) const;

    //==============================================================================

    /**
        Convert a point from another component's coordinate system to this component's local coordinates.
        This method handles all transforms in the component hierarchy.

        @param sourceComponent  The component whose coordinate system the point is in
        @param pointInSource    The point in the source component's coordinate system

        @return The point converted to this component's local coordinate system
     */
    Point<float> getLocalPoint (const Component* sourceComponent, Point<float> pointInSource) const;

    /**
        Convert a rectangle from another component's coordinate system to this component's local coordinates.
        This method handles all transforms in the component hierarchy.

        @param sourceComponent      The component whose coordinate system the rectangle is in
        @param rectangleInSource    The rectangle in the source component's coordinate system

        @return The rectangle converted to this component's local coordinate system
     */
    Rectangle<float> getLocalArea (const Component* sourceComponent, Rectangle<float> rectangleInSource) const;

    /**
        Convert a point from this component's local coordinates to another component's coordinate system.
        This method handles all transforms in the component hierarchy.

        @param targetComponent  The component whose coordinate system to convert to
        @param localPoint       The point in this component's local coordinate system

        @return The point converted to the target component's coordinate system
     */
    Point<float> getRelativePoint (const Component* targetComponent, Point<float> localPoint) const;

    /**
        Convert a rectangle from this component's local coordinates to another component's coordinate system.
        This method handles all transforms in the component hierarchy.

        @param targetComponent  The component whose coordinate system to convert to
        @param localRectangle   The rectangle in this component's local coordinate system

        @return The rectangle converted to the target component's coordinate system
     */
    Rectangle<float> getRelativeArea (const Component* targetComponent, Rectangle<float> localRectangle) const;

    //==============================================================================
    /**
        Set the transform of the component.

        The transform is applied in the component's local coordinates, before its position
        in the parent. It affects the component and all of its children, both when painting
        and when routing mouse input, so a transformed parent carries its children with it.

        @param transform The new transform of the component.
     */
    void setTransform (const AffineTransform& transform);

    /**
        Get the transform of the component.

        @return The transform of the component.
     */
    AffineTransform getTransform() const;

    /**
        Check if the component is transformed.

        @return True if the component is transformed, false otherwise.
     */
    bool isTransformed() const;

    /**
        Called when the transform of the component changes.
     */
    virtual void transformChanged();

    //==============================================================================
    /**
        Get the transform from this component's coordinate system to another component's coordinate system.
        This calculates the combined transform needed to convert coordinates from this component to the target.

        @param targetComponent  The component to get the transform to

        @return The combined transform from this component to the target component
     */
    AffineTransform getTransformToComponent (const Component* targetComponent) const;

    /**
        Get the transform from another component's coordinate system to this component's coordinate system.
        This calculates the combined transform needed to convert coordinates from the source to this component.

        @param sourceComponent  The component to get the transform from

        @return The combined transform from the source component to this component
     */
    AffineTransform getTransformFromComponent (const Component* sourceComponent) const;

    /**
        Get the transform from this component's coordinate system to screen coordinates.
        This calculates the combined transform needed to convert coordinates from this component to screen space.
        Being affine, it can't express ComponentEffect::contentToDisplay() warps, which localToScreen() does include.

        @return The combined transform from this component to screen coordinates
     */
    AffineTransform getTransformToScreen() const;

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

    /**
        Keeps the window of a desktop component at a width over height while it is resized.

        The ratio is kept for a component that is not on the desktop yet and applied to its
        window when it gets one. It constrains the whole window, so content that should keep
        its own proportions inside decorations or toolbars still has to fit itself.

        @param aspectRatio The width over height to keep, or std::nullopt to resize freely.
     */
    void setWindowAspectRatio (std::optional<float> aspectRatio);

    /**
        Returns the width over height the window keeps, or std::nullopt when it resizes freely.
     */
    std::optional<float> getWindowAspectRatio() const;

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

    /**
        Called when the opacity of the component changes.
     */
    virtual void opacityChanged();

    //==============================================================================
    /**
        Check if the component is opaque.

        An opaque component fully covers its background, allowing for rendering optimizations.
        When an area is fully covered by an opaque component (and its children), rendering
        can start from that component instead of from the root.

        @return True if the component is opaque, false otherwise.
     */
    bool isOpaque() const;

    /**
        Set the opaque state of the component.

        @param shouldBeOpaque True if the component should be opaque, false otherwise.
     */
    void setOpaque (bool shouldBeOpaque);

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

    //==============================================================================
    /**
        Lets the component paint outside its bounds by the given amounts, for a drop shadow
        or a glow.

        The margin only widens what the component paints and repaints: its bounds, layout and
        mouse hit testing are unchanged, so it is invisible to everything but drawing. Its
        paint() is clipped to getPaintBounds() instead of getLocalBounds(), and so are its
        children, and repaint() covers the margin too, so moving, resizing or hiding the
        component clears what it drew there.

        A component still paints inside its parent's area only. The margin is not painted
        for components on the desktop, nor by components with a ComponentEffect or cached to
        a texture, which render their bounds only.

        @param left    How far the component may paint left of its bounds
        @param top     How far the component may paint above its bounds
        @param right   How far the component may paint right of its bounds
        @param bottom  How far the component may paint below its bounds

        @see getPaintBounds
     */
    void setPaintMargin (float left, float top, float right, float bottom);

    /**
        Lets the component paint outside its bounds by the same amount on every side.

        @param margin  How far the component may paint outside its bounds

        @see setPaintMargin
     */
    void setPaintMargin (float margin);

    /**
        Lets the component paint outside its bounds by the given edge spacing.

        @param margin  How far the component may paint outside its bounds on each edge

        @see setPaintMargin
     */
    void setPaintMargin (const EdgeSpacing<float>& margin);

    /**
        Returns the paint margin: how far the component may paint outside its bounds on each edge.

        @see setPaintMargin
     */
    EdgeSpacing<float> getPaintMargin() const;

    /**
        Returns the area the component paints, in its local coordinates: its local bounds
        enlarged by its paint margin.

        @see setPaintMargin, getLocalBounds
     */
    Rectangle<float> getPaintBounds() const;

    //==============================================================================
    /** Enables or disables paint measurement suppression for this component.

        When suppression is enabled, component paint listeners will not receive
        paint measurement callbacks for this component, even if they request paint
        measurements.
    */
    void setPaintProfilingDisabled (bool shouldBeDisabled);

    /** Returns true if paint measurement is suppressed for this component. */
    bool isPaintProfilingDisabled() const;

    //==============================================================================
    /**
        Repaint the component.
     */
    void repaint();

    /**
        Repaint the component.

        @param rect The rectangle to repaint.
     */
    void repaint (const Rectangle<float>& rect);

    /**
        Repaint the component.
     */
    void repaint (float x, float y, float width, float height);

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

        @param nativeOptions The native flags of the component.
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

        It shows at once when the component receives the pointer's events (see
        ComponentNative::getPointerTarget()), otherwise when the pointer next reaches it.

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
        Check whether this component wants keyboard focus.

        @return True if this component wants keyboard focus, false otherwise.
     */
    bool getWantsKeyboardFocus() const;

    /**
        Set whether clicking this component can make it grab keyboard focus.

        When a component is clicked, focus handling walks from the clicked
        component up through its parents until it finds a component that both
        wants keyboard focus and has this flag enabled. This is enabled by
        default.

        @param shouldGrabFocus True if mouse clicks can grab keyboard focus.
     */
    void setClickingGrabFocus (bool shouldGrabFocus);

    /**
        Check whether clicking this component can make it grab keyboard focus.

        @return True if mouse clicks can grab keyboard focus.
     */
    bool getClickingGrabFocus() const;

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

    /**
        Called on the ancestors of a component that gained or lost the keyboard focus.

        Every component between the one whose focus changed and the top level component is
        notified, which lets a container repaint or update itself when the focus enters or
        leaves its subtree. Moving the focus between two components notifies the ancestors
        of both, so a shared ancestor is called twice: once with the component that lost
        the focus and once with the one that gained it. Use hasKeyboardFocus() on the child
        to tell the two apart.

        @param child The descendant whose focus changed. This is not necessarily a direct
                     child of this component.
        @param cause What triggered the focus change.

        @see focusGained, focusLost
     */
    virtual void focusOfChildComponentChanged (Component* child, FocusChangeType cause);

    //==============================================================================
    /**
        Check if the component has a parent component.

        @return Wheter the component has a parent.
     */
    bool hasParent() const;

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

    /** Called on the parent when one of its direct children has moved or been resized.

        A single setBounds() on the child reports one change here even though it moves
        and resizes the child, so a parent can lay itself out from this callback without
        doing the work twice.

        @param child The direct child whose bounds changed.

        @see moved, resized, parentSizeChanged
    */
    virtual void childBoundsChanged (Component* child);

    /** Called on the parent when something changing how a direct child is laid out changes.

        This happens when the child's visibility changes, or when one of its layout item properties
        (`flexGrow`, `margin`, `gridArea`, ...) is set, removed or computed differently by a
        stylesheet. Layout containers override it to lay their children out again.

        @param child The direct child whose layout changed.

        @see childrenChanged, ComponentProperty::affectsParentLayout
    */
    virtual void childLayoutChanged (Component& child);

    /** Called on a child when its index amongst its parent's children changes.

        This is the z-order position reported by getIndexOfChildComponent(), so it changes
        when toFront(), toBack(), raiseAbove(), lowerBelow(), raiseBy() or lowerBy() are
        used, or when addChildComponent() is called with a new index for a component that
        is already a child. Only the moved component is notified: the siblings that shift
        to make room for it are not.

        @param oldIndex The index the component used to have.
        @param newIndex The index the component has now.
    */
    virtual void indexInParentChildrenChanged (int oldIndex, int newIndex);

    //==============================================================================
    /**
        Get the number of child components of the component.
     */
    int getNumChildComponents() const;

    /**
        Get the child component at a given index.

        @param index The index of the child component to get.
     */
    Component* getChildComponent (int index) const;

    /**
        Returns the index of a child component, or -1 if not found.
     */
    int getIndexOfChildComponent (Component* component) const;

    /**
        Tests whether a point in local coordinates is inside this component.

        This decides which component mouse events are delivered to, so overriding it also
        overrides hit testing for the mouse: return false for the parts of the component
        that should let events through to whatever is behind them. Non-rectangular widgets
        such as rounded buttons or knobs use this to keep their corners transparent to
        clicks.

        An override can only take area away: a point outside the component's bounds never
        reaches it, whatever this returns, because the parent rejects the point before
        asking. The default implementation accepts the whole of getLocalBounds().

        @param x The x coordinate to test, relative to this component's top left.
        @param y The y coordinate to test, relative to this component's top left.

        @return True if the point counts as being inside this component.

        @see findComponentAt
     */
    virtual bool hitTest (float x, float y);

    /**
        Find the child component at a given point.

        @param p The point to find the child component at.
     */
    Component* findComponentAt (const Point<float>& p);

    /**
        Find the topmost component at a given point that is willing to receive mouse events.

        Like findComponentAt(), this walks the hierarchy depth-first preferring the
        topmost (last added) child whose bounds contain the point and whose hitTest()
        accepts it, but a component that has opted out of mouse events via
        setWantsMouseEvents() is skipped in favor of the next sibling underneath it,
        and the search bubbles up to this component itself (or returns nullptr) when
        nothing in a given subtree wants the event. Points are mapped into each child the
        same way, through its transform, effect and the parent's getChildPointFromLocal().

        @param p The point to find the component at, in this component's local coordinates.

        @returns The component that should receive the mouse event, or nullptr if none does.
     */
    Component* findComponentAtForMouseEvent (const Point<float>& p);

    /**
        Returns the top level component.
    */
    Component* getTopLevelComponent();

    /**
        Returns the component that popups opened by this component should be added to.

        This is the closest ancestor that is a popup container, or that changes how its subtree
        is presented because it is transformed or manually composited (for example onto a 3D
        surface), so the popup is presented the same way as the component that opened it.
        Without such an ancestor this is the top level component.

        @see setPopupContainer, setTransform, setManuallyComposited, PopupMenu::Options::withParentComponent
    */
    Component* getPopupParentComponent();

    //==============================================================================
    /**
        Returns the explicit property store of the component, for enumeration only.

        The store holds the explicit, store-backed values set on this component (colors,
        curated geometry and custom properties), keyed by storage id. All writes go through
        setProperty() / setColor() so that change notifications always fire; the returned set
        is therefore const.

        @return The explicit properties of the component.
     */
    const NamedValueSet& getProperties() const;

    //==============================================================================
    /**
        Returns true when getProperty() would return a defined value for this component.

        @param name The property name to test.
    */
    bool hasProperty (const Identifier& name) const;

    /**
        Sets a property on this component.

        If the component's type declares a descriptor for @p name, the value is routed through
        its setter (state-backed) or stored under its storage id (store-backed). Otherwise the
        value is stored as a custom property keyed by @p name. On an actual change, listeners are
        notified, the style is refreshed and the component repainted.

        @param name  The property name (descriptor name or a custom name).
        @param value The value to set.
    */
    void setProperty (const Identifier& name, const var& value);

    /**
        Removes an explicit, store-backed property value from this component.

        State-backed descriptor properties cannot be removed and are ignored.

        @param name The property name to remove.
    */
    void removeProperty (const Identifier& name);

    /**
        Reads a property value from this component only (no cascade).

        Returns the descriptor getter result for state-backed properties, the explicit stored
        value for store-backed properties, or an undefined var when unset.

        @param name The property name to read.
    */
    var getProperty (const Identifier& name) const;

    /** Reads a property from this component only, converted to @p T. */
    template <class T>
    T getPropertyAs (const Identifier& name) const
    {
        return VariantConverter<T>::fromVar (getProperty (name));
    }

    /**
        Finds a property value, cascading up the parent chain and falling back to the theme and
        the descriptor default for store-backed properties.

        Color properties fall back to the theme colors, including those derived from the closest
        palette, as ApplicationTheme::findColor() does. State-backed properties resolve only on
        the owning component.

        @param name The property name to find.
    */
    var findProperty (const Identifier& name) const;

    /** Finds a property with cascade, converted to @p T. */
    template <class T>
    T findPropertyAs (const Identifier& name) const
    {
        return VariantConverter<T>::fromVar (findProperty (name));
    }

    /** Notifies listeners that a state-backed property changed internally.

        Widgets call this from their own setters (Slider drag, toggle, text edit) so that the
        single change path feeds bindings, pseudo-state selectors and attribute selectors. It
        notifies ComponentListener::componentPropertyChanged() without touching the property store.

        @param name The name of the property that changed.
    */
    void notifyPropertyChanged (const Identifier& name);

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
        Set if the component wants mouse events.

        @param allowSelfMouseEvents True if the component wants mouse events on itself, false otherwise.
        @param allowChildrenMouseEvents True if the component wants mouse events on its children, false otherwise.
     */
    void setWantsMouseEvents (bool allowSelfMouseEvents, bool allowChildrenMouseEvents);

    /**
        Check if the component wants mouse events on itself.

        @return True if the component wants mouse events on itself, false otherwise.
     */
    bool doesWantSelfMouseEvents() const;

    /**
        Check if the component wants mouse events on its children.

        @return True if the component wants mouse events on its children, false otherwise.
     */
    bool doesWantChildrenMouseEvents() const;

    //==============================================================================
    /**
        Called when the mouse enters the component.

        @param event The mouse event.
     */
    void mouseEnter (const MouseEvent& event) override;

    /**
        Called when the mouse exits the component.

        @param event The mouse event.
     */
    void mouseExit (const MouseEvent& event) override;

    /**
        Called when the mouse button is pressed.

        @param event The mouse event.
     */
    void mouseDown (const MouseEvent& event) override;

    /**
        Called when the mouse is moved.

        @param event The mouse event.
     */
    void mouseMove (const MouseEvent& event) override;

    /**
        Called when the mouse is dragged.

        @param event The mouse event.
     */
    void mouseDrag (const MouseEvent& event) override;

    /**
        Called when the mouse button is released.

        @param event The mouse event.
     */
    void mouseUp (const MouseEvent& event) override;

    /**
        Called when the mouse button is double clicked.

        @param event The mouse event.
     */
    void mouseDoubleClick (const MouseEvent& event) override;

    /**
        Called when the mouse wheel is scrolled.

        The event starts at the component under the pointer and travels up its parent hierarchy.
        Return true to consume it; return false to let the next enabled parent handle it. The
        default implementation returns false.

        @param event The mouse event, with its position in this component's coordinates.
        @param wheelData The mouse wheel data.

        @returns true if this component handled the wheel event.
     */
    bool mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) override;

    //==============================================================================
    /**
        Chooses which touch and trackpad gestures this component receives.

        A gesture goes to the deepest component under where it starts that wants its type,
        looking through the parents of the component the fingers hit. Components wanting no
        gestures (the default) only see the per-finger mouse events.

        When a multi-finger pinch, rotate or pan starts on a component, the fingers involved get
        a canceled mouseUp and no further mouse events, so a component never sees both.

        @param gestureTypes The gestures wanted, combined with `|`, or GestureType::none.

        @see GestureType, GestureEvent, gesturePinch
     */
    void setWantedGestures (GestureType gestureTypes);

    /** Returns the gestures this component wants. @see setWantedGestures */
    GestureType getWantedGestures() const;

    /**
        Returns the component a gesture of a given type starting at a point goes to.

        @param position The point, relative to this component.
        @param gestureType A single gesture type.

        @return The deepest component under the point wanting the gesture, or nullptr.
     */
    Component* findGestureTargetAt (const Point<float>& position, GestureType gestureType);

    /** Called when a tap is recognized. @see GestureListener::gestureTap */
    void gestureTap (const GestureEvent& event) override;

    /** Called when a long press begins, moves or ends. @see GestureListener::gestureLongPress */
    void gestureLongPress (const GestureEvent& event) override;

    /** Called when a pinch begins, changes or ends. @see GestureListener::gesturePinch */
    void gesturePinch (const GestureEvent& event) override;

    /** Called when a rotate begins, changes or ends. @see GestureListener::gestureRotate */
    void gestureRotate (const GestureEvent& event) override;

    /** Called when a multi-finger pan begins, changes or ends. @see GestureListener::gesturePan */
    void gesturePan (const GestureEvent& event) override;

    /** Called when a swipe is recognized. @see GestureListener::gestureSwipe */
    void gestureSwipe (const GestureEvent& event) override;

    //==============================================================================
    /**
        Add a mouse listener to the component.

        Adding a listener that is already registered only updates whether it wants the events
        of nested child components.

        @param listener The mouse listener to add.
        @param wantsEventsForAllNestedChildComponents When true, the listener also receives the
               mouse events of every component inside this one, with positions relative to the
               component that received them.
     */
    void addMouseListener (MouseListener* listener, bool wantsEventsForAllNestedChildComponents = false);

    /**
        Remove a mouse listener from the component.

        @param listener The mouse listener to remove.
     */
    void removeMouseListener (MouseListener* listener);

    /**
        Add a gesture listener to the component.

        The listener receives the gestures this component wants, after the component itself.

        @param listener The gesture listener to add.
        @see setWantedGestures
     */
    void addGestureListener (GestureListener* listener);

    /**
        Remove a gesture listener from the component.

        @param listener The gesture listener to remove.
     */
    void removeGestureListener (GestureListener* listener);

    /** Add a component listener to this component. */
    void addComponentListener (ComponentListener* listener);

    /** Remove a component listener from this component. */
    void removeComponentListener (ComponentListener* listener);

    //==============================================================================
    /**
        Called when a key is pressed.

        The key goes to the component with the keyboard focus first. When that one does not
        handle it, it goes to its parent, and so on up to the top-level component, so a
        focused control never swallows the keys meant for what is around it, such as
        shortcuts or an on-screen keyboard. Invisible components are skipped, and so are
        disabled ones and everything inside a disabled parent (see isEnabled()).

        @param keys The key press.
        @param position The position of the key press.

        @returns true when the component handled the key, which stops it going further up.
     */
    virtual bool keyDown (const KeyPress& keys, const Point<float>& position);

    /**
        Called when a key is released.

        It goes up the parents like keyDown(), so a component that acted on a key press
        receives its release even after the focus moved to one of its children.

        @param keys The key press.
        @param position The position of the key press.

        @returns true when the component handled the key, which stops it going further up.
     */
    virtual bool keyUp (const KeyPress& keys, const Point<float>& position);

    /**
        Called when text is input.

        @param text The text input.
     */
    virtual void textInput (const String& text);

    /**
        Called when a key is pressed or released, whether or not it produced a keyDown().

        This reports the raw transition of a key between up and down, so it fires once per
        physical press and once per release, and auto-repeat does not retrigger it. It is
        also delivered when a key press has been consumed before reaching keyDown(), which
        makes it the reliable place to track whether a modifier or a chord key is being
        held.

        It goes up the parents like keyDown(), starting from the component with the keyboard
        focus, until a component returns true.

        @param key    The key that changed state.
        @param isDown True if the key has just gone down, false if it has just come up.

        @returns true when the component handled the change, which stops it going further up.

        @see keyDown, keyUp, modifierKeysChanged
     */
    virtual bool keyStateChanged (const KeyPress& key, bool isDown);

    /**
        Called when the state of the modifier keys changes.

        Modifiers are refreshed from mouse button events as well as key events, so this also
        fires when a click reveals a modifier that changed while the keyboard was idle.

        The default implementation passes the change up to the parent component, so an
        override consumes it unless it calls the base class version.

        @param modifiers The modifier keys that are now held down.

        @see keyStateChanged
     */
    virtual void modifierKeysChanged (const KeyModifiers& modifiers);

    //==============================================================================
    /** Makes this component hold the popups opened by the components inside it.

        Popups, such as menus and combo box lists, are normally added to the top level
        component. Inside a popup container they are added to the container instead, so they
        are presented like the rest of its subtree: through its component effect, for example
        a post-processing pass over a whole interface, and with its input mapping.

        @param shouldBePopupContainer True to hold the popups of this component's subtree.

        @see getPopupParentComponent
     */
    void setPopupContainer (bool shouldBePopupContainer);

    /** Returns true if this component holds the popups opened inside it.

        @see setPopupContainer
     */
    bool isPopupContainer() const;

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
    /** Adds a style class to this component.

        Style classes are the `.class` selectors of a stylesheet, the `class:` property of a
        `.yui` instance, and are free to be set from C++ on hand-written UIs too.

        @param styleClass The class to add.

        @see removeStyleClass, hasStyleClass, setStyleClasses, StyleSheet
    */
    void addStyleClass (const Identifier& styleClass);

    /** Removes a style class from this component.

        @param styleClass The class to remove.
    */
    void removeStyleClass (const Identifier& styleClass);

    /** Returns true if this component carries a style class.

        @param styleClass The class to look for.
    */
    bool hasStyleClass (const Identifier& styleClass) const;

    /** Replaces every style class of this component.

        @param newStyleClasses The classes the component should carry.
    */
    void setStyleClasses (const Array<Identifier>& newStyleClasses);

    /** Returns the style classes of this component, in the order they were added. */
    const Array<Identifier>& getStyleClasses() const noexcept;

    //==============================================================================
    /** Returns true while the pointer is over this component or one of its children.

        This follows the CSS semantics of `:hover`, so a container hovers while the pointer is
        over anything inside it.
    */
    bool isHovered() const noexcept;

    /** Returns true while a mouse button is held down on this component or one of its children.

        This is what the `:pressed` and `:active` pseudo-classes match.
    */
    bool isPressed() const noexcept;

    /** Returns true when this component or one of its children holds the keyboard focus.

        This is what the `:focus-within` pseudo-class matches, while `:focus` matches
        hasKeyboardFocus().
    */
    bool hasFocusWithin() const;

    //==============================================================================
    /** Attaches a stylesheet to this component and everything inside it.

        The component becomes the root of a StyleContext that matches the sheet's rules against
        the subtree, computes the values of the matched properties and keeps them up to date as
        classes, states and properties change. It works for hand-written C++ trees as well as
        for the ones built from a `.yui` file. Passing nullptr detaches the sheet and drops
        everything it computed.

        Nesting is allowed: the closest attached context up the parent chain styles a component.

        @param newStyleSheet The sheet to attach, or nullptr to detach.

        @see getStyleSheet, getStyleContext, StyleSheet, StyleContext
    */
    void setStyleSheet (ReferenceCountedObjectPtr<StyleSheet> newStyleSheet);

    /** Returns the stylesheet attached to this component with setStyleSheet(), or nullptr. */
    ReferenceCountedObjectPtr<StyleSheet> getStyleSheet() const;

    /** Returns the style context rooted at this component, or nullptr when none is attached. */
    StyleContext* getStyleContext() const noexcept;

    /** Returns the closest style context attached to this component or one of its parents. */
    StyleContext* findStyleContext() const noexcept;

    /** Returns the values a stylesheet or an animation computed for this component, or nullptr.

        @see StyleState
    */
    const StyleState* getStyleState() const noexcept;

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
    /** Gives this component and everything inside it a palette of its own.

        The theme draws the component and its children from the closest palette up the
        parent chain instead of the theme's own: ApplicationTheme::findColor() resolves the
        palette derived colors from it, and ApplicationTheme::getPalette (const Component&)
        returns it. Colors set with setColor() still take precedence. Popup menus opened
        relative to a component take its palette too.

        This lets windows, or several plugin editors in one process, wear different palettes
        with a single global theme.

        @param newPalette The palette, or std::nullopt to follow the parent's again.

        @see findPalette, ApplicationTheme::setPalette
     */
    void setPalette (std::optional<ThemePalette> newPalette);

    /** Returns the palette set on this component with setPalette(), or nullptr. */
    const ThemePalette* getPalette() const;

    /** Returns the closest palette set with setPalette() on this component or a parent, or nullptr. */
    const ThemePalette* findPalette() const;

    //==============================================================================
    /** Sets a component effect that is applied after the component and its children are rendered.

        The component subtree is first rendered to an offscreen GPU texture, then the effect's
        apply() method is called to composite the result back onto the main Graphics context.
        Pass nullptr to remove the effect.

        @param effect The effect to apply, or nullptr to remove.
        @see ComponentEffect
     */
    void setComponentEffect (ComponentEffect::Ptr effect);

    /** Returns the current component effect, or nullptr if none is set. */
    ComponentEffect::Ptr getComponentEffect() const;

    //==============================================================================
    /** Enables or disables cached-to-texture rendering for this component and its subtree.

        When enabled, the component subtree is rendered once into an offscreen GPU texture that
        is reused across subsequent frames. The cache is automatically invalidated when the component
        is repainted, resized, or its children change.

        This improves performance for complex components whose content changes infrequently.

        @param shouldCache Whether to enable cached-to-texture rendering.
     */
    void setCachedToTexture (bool shouldCache);

    /** Returns true if cached-to-texture rendering is enabled. */
    bool isCachedToTexture() const;

    //==============================================================================
    /** Creates a CPU-side Image containing a snapshot of this component's current
        appearance, including all visible children.

        The component tree is rendered offscreen, then the GPU pixels are read back to
        CPU memory. The returned Image has both GPU texture and CPU pixel data populated.

        @param ctx            The GraphicsContext to use for offscreen rendering.
        @param includeEffects When true and a component effect is active, the effect is
                              applied to the snapshot. When false, the raw subtree
                              rendering is captured.
        @return An Image containing the rendered snapshot, or an empty Image on failure.
        @see snapshotToTexture
     */
    Image snapshotToImage (GraphicsContext& ctx, bool includeEffects = true);

    /** Creates a GPU texture snapshot of this component's current appearance,
        including all visible children.

        Like snapshotToImage but returns a GPU texture without reading pixels back to
        CPU memory. This is faster when the snapshot is only needed for on-screen
        compositing (e.g. via Graphics::drawTexture).

        @param ctx            The GraphicsContext to use for offscreen rendering.
        @param includeEffects When true and a component effect is active, the effect is
                              applied to the snapshot.
        @return A GpuTexture containing the rendered snapshot, or nullptr on failure.
        @see snapshotToImage
     */
    GpuTexture::Ptr snapshotToTexture (GraphicsContext& ctx, bool includeEffects = true);

    //==============================================================================
    /** Takes this component out of the automatic compositing into its parent.

        When enabled, the normal child painting pass no longer draws this component: whoever
        presents it (usually the parent) calls renderToTexture() and composites the texture
        itself, for example mapped onto a 3D mesh. The component stays visible, hit-testable
        and focusable, so keyboard, hover, capture, wheel and drag and drop keep working
        through the usual dispatch. Override the parent's getChildPointFromLocal() and
        getLocalPointFromChild() to route input to it.

        Repainting anything inside this component repaints the whole parent, since the texture
        can end up anywhere in it.

        @param shouldBeManuallyComposited True to composite this component manually.

        @see renderToTexture, getChildPointFromLocal
     */
    void setManuallyComposited (bool shouldBeManuallyComposited);

    /** Returns true if this component is composited manually instead of by its parent.

        @see setManuallyComposited
     */
    bool isManuallyComposited() const;

    /** Renders this component's subtree, including its effect, into a texture.

        The texture is owned by the component and reused across calls: it is rendered again
        only when something in the subtree repainted, or the size or scale changed, since the
        last call. This is meant to be called from the parent's paint() for a component marked
        with setManuallyComposited().

        The texture is sized in device pixels, i.e. the component size multiplied by @p scale.
        Pass g.getContextScale() to keep the result sharp on high-density displays, or a larger
        value when the surface it is mapped onto is magnified on screen.

        @param ctx   The GraphicsContext to render with, usually g.getGraphicsContext().
        @param scale The number of texture pixels per logical unit of the component.

        @return The texture holding the subtree, or nullptr if the component is empty or
                rendering failed.

        @see setManuallyComposited
     */
    GpuTexture::Ptr renderToTexture (GraphicsContext& ctx, float scale = 1.0f);

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

protected:
    /** @internal This is used by subclasses to simplify sending notifications. */
    template <class F>
    void sendChangeNotification (NotificationType notification, F&& function)
    {
        if (notification == dontSendNotification)
            return;

        auto notificationSender = [function = std::forward<F> (function), bailOutChecker = BailOutChecker (this)]
        {
            if (bailOutChecker.shouldBailOut())
                return;

            function();
        };

        if (notification == sendNotificationAsync || ! MessageManager::getInstance()->isThisTheMessageThread())
            MessageManager::callAsync (std::move (notificationSender));
        else
            function();
    }

private:
    Identifier resolvePropertyStorageKey (const Identifier& name) const;
    StyleState& getOrCreateStyleState();
    void markStyleDirty();
    void styleStateFlagChanged (const Identifier& name);
    void setHoveredChain (bool isNowHovered);
    void setPressedChain (bool isNowPressed);
    void clearHoveredInSubtree();
    void clearPressedInSubtree();
    void internalRefreshDisplay (double lastFrameTimeSeconds);
    void internalRepaint();
    void internalRepaint (const Rectangle<float>& rect);
    void internalPaint (Graphics& g, const RectangleList<float>& repaintRegions, bool renderContinuous);
    void internalMouseEnter (const MouseEvent& event);
    void internalMouseExit (const MouseEvent& event);
    void internalMouseDown (const MouseEvent& event);
    void internalMouseMove (const MouseEvent& event);
    void internalMouseDrag (const MouseEvent& event);
    void internalMouseUp (const MouseEvent& event);
    void internalMouseDoubleClick (const MouseEvent& event);
    void internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData);
    void internalGesture (const GestureEvent& event);
    bool internalKeyDown (const KeyPress& keys, const Point<float>& position);
    bool internalKeyUp (const KeyPress& keys, const Point<float>& position);
    void internalTextInput (const String& text);
    bool internalKeyStateChanged (const KeyPress& keys, bool isDown);
    void internalModifierKeysChanged (const KeyModifiers& modifiers);
    void internalResized (int width, int height);
    void internalMoved (int xpos, int ypos);
    void internalFocusChanged (bool gotFocus);
    void internalFocusOfComponentChanged (FocusChangeType cause);
    void internalDisplayChanged();
    void internalContentScaleChanged (float dpiScale);
    void internalSafeAreaChanged();
    void internalUserTriedToCloseWindow();
    void internalHierarchyChanged();
    void internalVisibilityChanged();
    void internalAttachedToNative();
    void internalDetachedFromNative();

    void takeKeyboardFocus (FocusChangeType cause);
    void handleKeyboardFocusFromClick();

    using KeyHandler = bool (Component::*) (const KeyPress&, const Point<float>&);
    bool dispatchKey (const KeyPress& keys, const Point<float>& position, KeyHandler handler);

    void updateMouseCursor();

    template <class... MethodArgs, class... Args>
    void callMouseListeners (const BailOutChecker& bailOutChecker, void (MouseListener::*callback) (MethodArgs...), const Args&... args)
    {
        struct NestedBailOutChecker
        {
            bool shouldBailOut() const { return source.shouldBailOut() || parent.shouldBailOut(); }

            BailOutChecker source, parent;
        };

        mouseListeners.callChecked (bailOutChecker, callback, args...);

        for (auto* parent = parentComponent; parent != nullptr && ! bailOutChecker.shouldBailOut(); parent = parent->parentComponent)
        {
            const NestedBailOutChecker nestedChecker { bailOutChecker, BailOutChecker (parent) };

            parent->nestedMouseListeners.callChecked (nestedChecker, callback, args...);

            if (nestedChecker.shouldBailOut())
                return;
        }
    }

    void callMouseWheelListeners (const BailOutChecker& bailOutChecker,
                                  const MouseEvent& event,
                                  const MouseWheelData& wheelData)
    {
        const auto call = [&] (MouseListener& listener)
        {
            ignoreUnused (listener.mouseWheel (event, wheelData));
        };

        mouseListeners.callChecked (bailOutChecker, call);

        for (auto* parent = parentComponent; parent != nullptr && ! bailOutChecker.shouldBailOut(); parent = parent->parentComponent)
        {
            struct NestedBailOutChecker
            {
                bool shouldBailOut() const { return source.shouldBailOut() || parent.shouldBailOut(); }

                BailOutChecker source, parent;
            };

            const NestedBailOutChecker nestedChecker { bailOutChecker, BailOutChecker (parent) };
            parent->nestedMouseListeners.callChecked (nestedChecker, call);

            if (nestedChecker.shouldBailOut())
                return;
        }
    }

    void sendMoved();
    void sendResized();
    void sendChildBoundsChangedToParent();

    bool hasOpaqueChildCoveringArea (const Rectangle<float>& area);
    AffineTransform getTransformToTopLevelComponent() const;
    Point<float> getTopLevelScreenOrigin() const;
    void applyPaintState (Graphics& g, const RectangleList<float>& clipRegion) const;
    void paintSubtree (Graphics& g, const RectangleList<float>& clipRegion, float opacity, bool renderContinuous);
    void paintChildrenAndOverChildren (Graphics& g, const RectangleList<float>& clipRegion, bool renderContinuous);
    GpuCanvas::Ptr renderSubtreeOffscreen (GraphicsContext& ctx, float opacity, bool renderContinuous, float scale, GpuCanvas::Ptr reuseCanvas = nullptr, const std::optional<RectangleList<float>>& dirtyRegion = std::nullopt);
    std::optional<RectangleList<float>> takeEffectDirtyRegion (bool renderContinuous);
    GpuCanvas::Ptr renderSnapshotOffscreen (GraphicsContext& ctx, bool includeEffects, float scale, GpuCanvas::Ptr reuseCanvas = nullptr);

#if YUP_ENABLE_COMPONENT_PAINT_DEBUGGING
    void paintDebugOverlay (Graphics& g, const RectangleList<float>& boundsToRedraw);
#endif

    friend class ComponentNative;
    friend class SDLComponentNative;
    friend class StyleContext;
    friend class WeakReference<Component>;

    template <class T>
    friend class ComponentTestHelper;

    using ComponentListenerList = ListenerList<ComponentListener, Array<WeakReference<ComponentListener>>>;
    using MouseListenerList = ListenerList<MouseListener, Array<WeakReference<MouseListener>>>;
    using GestureListenerList = ListenerList<GestureListener, Array<WeakReference<GestureListener>>>;

    String componentID, componentTitle;
    Component* parentComponent = nullptr;
    Array<Component*> children;
    Rectangle<float> boundsInParent;
    AffineTransform transform;
    EdgeSpacing<float> paintMargin;
    ComponentNative::Ptr native;
    WeakReference<Component>::Master masterReference;
    MouseListenerList mouseListeners;
    MouseListenerList nestedMouseListeners;
    GestureListenerList gestureListeners;
    ComponentListenerList componentListeners;
    ComponentStyle::Ptr style;
    std::unique_ptr<ThemePalette> palette;
    std::unique_ptr<StyleState> styleState;
    std::unique_ptr<StyleContext> styleContext;
    Array<Identifier> styleClasses;
    NamedValueSet properties;
    MouseCursor mouseCursor;
    GestureType wantedGestures = GestureType::none;
    ComponentEffect::Ptr componentEffect;
    std::optional<float> windowAspectRatio;
    GpuCanvas::Ptr cachedTextureCanvas;
    GpuCanvas::Ptr effectOffscreenCanvas;
    RectangleList<float> effectDirtyRegion;
    static constexpr int maximumDirtyRectangles = 8;
    GpuCanvas::Ptr presentedCanvas;
    float contentScale = 1.0f;
    uint8 opacity = 255;
    bool suppressChildBoundsChanged = false;
    std::atomic_bool isRepainting { false };
    std::atomic_bool subtreeDirty { true };

    struct InternalFlags
    {
        bool isVisible : 1;
        bool isDisabled : 1;
        bool hasFrame : 1;
        bool onDesktop : 1;
        bool isFullScreen : 1;
        bool isTransparent : 1;
        bool unclippedRendering : 1;
        bool wantsKeyboardFocus : 1;
        bool clickingDoesNotGrabFocus : 1;
        bool blockSelfMouseEvents : 1;
        bool blockChildrenMouseEvents : 1;
        bool paintProfilingDisabled : 1;
        bool cachedToTexture : 1;
        bool cachedTextureStale : 1;
        bool paintAsOffscreenRoot : 1;
        bool manuallyComposited : 1;
        bool popupContainer : 1;
        bool isHovered : 1;
        bool isPressed : 1;
    };

    union
    {
        uint64 flagsValue;
        InternalFlags flags;
    };

#if YUP_ENABLE_COMPONENT_PAINT_DEBUGGING
    Color debugColor = Color::opaqueRandom();
    int counter = 2;
#endif

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Component)
};

} // namespace yup
