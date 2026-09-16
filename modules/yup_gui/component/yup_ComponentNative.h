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
/** Describes what caused the keyboard focus to move.

    @see Component::focusOfChildComponentChanged, ComponentNative::setFocusedComponent
*/
enum class FocusChangeType
{
    focusChangedByMouseClick, /**< The focus moved because the user clicked on a component. */
    focusChangedDirectly      /**< The focus was moved programmatically. */
};

//==============================================================================

/**
    Provides platform-native window and rendering capabilities for Components.

    This class serves as an abstraction layer between the Component class and the platform-specific
    window management and rendering systems. It handles native window creation, event processing,
    and rendering pipeline setup across different platforms.

    ComponentNative objects are typically created internally by the Component class when a Component
    needs to be displayed on screen as a top-level window.

    @see Component
*/
class YUP_API ComponentNative : public ReferenceCountedObject
{
    struct decoratedWindowTag;
    struct resizableWindowTag;
    struct temporaryWindowTag;
    struct renderContinuousTag;
    struct allowHighDensityDisplayTag;
    struct captureMouseTag;
    struct vsyncTag;
    struct transparentWindowTag;
    struct nonFocusableWindowTag;
    struct alwaysOnTopWindowTag;

public:
    //==============================================================================
    /** The pointer defintion for this native component. */
    using Ptr = ReferenceCountedObjectPtr<ComponentNative>;

    //==============================================================================
    /** Type definition for window configuration flags. */
    using Flags = FlagSet<uint32,
                          decoratedWindowTag,
                          resizableWindowTag,
                          temporaryWindowTag,
                          renderContinuousTag,
                          allowHighDensityDisplayTag,
                          captureMouseTag,
                          vsyncTag,
                          transparentWindowTag,
                          nonFocusableWindowTag,
                          alwaysOnTopWindowTag>;

    /** No flags set. */
    static inline constexpr Flags noFlags = Flags();
    /** Flag to enable window decorations (title bar, borders, etc.). */
    static inline constexpr Flags decoratedWindow = Flags::declareValue<decoratedWindowTag>();
    /** Flag to enable window resizing by the user. */
    static inline constexpr Flags resizableWindow = Flags::declareValue<resizableWindowTag>();
    /** Flag to mark the native window as a temporary popup/menu-style window. */
    static inline constexpr Flags temporaryWindow = Flags::declareValue<temporaryWindowTag>();
    /** Flag to enable continuous rendering mode. */
    static inline constexpr Flags renderContinuous = Flags::declareValue<renderContinuousTag>();
    /** Flag to enable high-density display support. */
    static inline constexpr Flags allowHighDensityDisplay = Flags::declareValue<allowHighDensityDisplayTag>();
    /** Flag to capture mouse input outside the native window while the component is on the desktop. */
    static inline constexpr Flags captureMouse = Flags::declareValue<captureMouseTag>();
    /** Flag to synchronize presentation to the display refresh (vsync). */
    static inline constexpr Flags vsync = Flags::declareValue<vsyncTag>();
    /** Flag to give the window a transparent buffer, so per-pixel alpha is preserved. */
    static inline constexpr Flags transparentWindow = Flags::declareValue<transparentWindowTag>();
    /** Flag to keep the window out of the focus chain, so clicking it never steals focus. */
    static inline constexpr Flags nonFocusableWindow = Flags::declareValue<nonFocusableWindowTag>();
    /** Flag to keep the window above all others. */
    static inline constexpr Flags alwaysOnTopWindow = Flags::declareValue<alwaysOnTopWindowTag>();
    /** Default flags combining decoratedWindow, resizableWindow, and allowHighDensityDisplay. */
    static inline constexpr Flags defaultFlags = decoratedWindow | resizableWindow | allowHighDensityDisplay;

    //==============================================================================
    /** Determines how the accumulated dirty rectangles are turned into repaint work.

        When a single frame accumulates several, possibly far apart, dirty rectangles
        the way they are expanded into paint calls has a large impact on how much of
        the component hierarchy is repainted.

        @see Options::withRepaintMode
    */
    enum class RepaintMode
    {
        /** Repaint each dirty rectangle in isolation.

            The dirty rectangles are treated as one disjoint region: a parent shared by
            several dirty rectangles is painted once, clipped to the region, and only the
            parts of the hierarchy that overlap the region are repainted. Everything that
            lies between two distant dirty rectangles is left untouched. This is the default.
        */
        disjointRegions,

        /** Merge every dirty rectangle into a single bounding box and repaint it.

            Everything that falls inside the bounding box of all dirty rectangles is
            repainted, even if it lies between two distant dirty rectangles and did not
            actually change.
        */
        boundingBox
    };

    //==============================================================================
    /**
        Configuration options for creating a native component.

        This structure encapsulates all the configuration options that can be used
        when creating a new ComponentNative instance. It provides a fluent interface
        for setting options.
    */
    struct Options
    {
        /** Default constructor, initializes the options with default values. */
        constexpr Options() noexcept = default;

        /** Sets the flags for the native component.

            @param newFlags The flags to set.

            @return Reference to this Options object for method chaining.
        */
        Options& withFlags (Flags newFlags) noexcept;

        /** Sets whether the window should have decorations.

            @param shouldHaveDecoration True to enable window decorations, false to disable.

            @return Reference to this Options object for method chaining.
        */
        Options& withDecoration (bool shouldHaveDecoration) noexcept;

        /** Sets whether the window should be resizable.

            @param shouldAllowResizing True to enable window resizing, false to disable.

            @return Reference to this Options object for method chaining.
        */
        Options& withResizableWindow (bool shouldAllowResizing) noexcept;

        /** Sets whether the component should render continuously.

            @param shouldRenderContinuous True to enable continuous rendering, false to use on-demand rendering.

            @return Reference to this Options object for method chaining.
        */
        Options& withRenderContinuous (bool shouldRenderContinuous) noexcept;

        /** Sets whether high-density display should be allowed.

            @param shouldAllowHighDensity True to enable high-density display support, false to disable.

            @return Reference to this Options object for method chaining.
        */
        Options& withAllowedHighDensityDisplay (bool shouldAllowHighDensity) noexcept;

        /** Sets whether the native window should capture mouse input outside its bounds.

            @param shouldCaptureMouse True to capture mouse input, false to use normal window-local input.

            @return Reference to this Options object for method chaining.
        */
        Options& withMouseCapture (bool shouldCaptureMouse) noexcept;

        /** Sets whether presentation should be synchronized to the display refresh.

            With vsync enabled the backend's present call blocks until the display is ready, so
            frames are paced by the display rather than by `framerateRedraw`. Off by default.

            @param shouldUseVSync True to synchronize presentation to the display, false to pace frames with the software timer.

            @return Reference to this Options object for method chaining.
        */
        Options& withVSync (bool shouldUseVSync) noexcept;

        /** Sets whether the window should preserve per-pixel alpha.

            @param shouldBeTransparent True to give the window a transparent buffer, false for an opaque one.

            @return Reference to this Options object for method chaining.
        */
        Options& withTransparent (bool shouldBeTransparent) noexcept;

        /** Sets whether the window can take keyboard focus.

            @param shouldBeFocusable False to keep the window out of the focus chain, true for a normal focusable window.

            @return Reference to this Options object for method chaining.
        */
        Options& withFocusable (bool shouldBeFocusable) noexcept;

        /** Sets whether the window should stay above all others.

            @param shouldBeAlwaysOnTop True to keep the window on top, false for normal stacking.

            @return Reference to this Options object for method chaining.
        */
        Options& withAlwaysOnTop (bool shouldBeAlwaysOnTop) noexcept;

        /** Sets whether the window should be treated as a temporary popup/menu window.

            @param shouldBeTemporary True for popup/menu-style windows, false for regular windows.

            @return Reference to this Options object for method chaining.
        */
        Options& withTemporaryWindow (bool shouldBeTemporary) noexcept;

        /** Sets the graphics API to be used for rendering.

            @param newGraphicsApi The graphics API to use, or std::nullopt to use the default.

            @return Reference to this Options object for method chaining.
        */
        Options& withGraphicsApi (std::optional<GpuPlatform> newGraphicsApi) noexcept;

        /** Sets the target framerate for continuous rendering.

            @param newFramerateRedraw The target framerate, or std::nullopt to use the default.

            @return Reference to this Options object for method chaining.
        */
        Options& withFramerateRedraw (std::optional<float> newFramerateRedraw) noexcept;

        /** Sets the framerate to fall back to while the window does not have keyboard focus.

            Use it to throttle a window that is in the background, or that is covered by a modal
            window, without stopping it altogether the way withUpdateOnlyFocused() does. The rate
            is never raised above the one set with withFramerateRedraw(), and if updates are
            already limited to the focused state then that takes precedence and nothing is drawn
            at all while unfocused.

            @param newUnfocusedFramerateRedraw The unfocused target framerate, or std::nullopt to keep rendering at the normal rate.

            @return Reference to this Options object for method chaining.

            @see withFramerateRedraw, withUpdateOnlyFocused
        */
        Options& withUnfocusedFramerateRedraw (std::optional<float> newUnfocusedFramerateRedraw) noexcept;

        /** Sets the clear color used when rendering.

            @param newClearColor The clear color, or std::nullopt to use the default.

            @return Reference to this Options object for method chaining.
        */
        Options& withClearColor (std::optional<Color> newClearColor) noexcept;

        /** Sets the double-click detection time.

            @param newDoubleClickTime The maximum time between clicks to be considered a double-click, or std::nullopt to use the default.

            @return Reference to this Options object for method chaining.
        */
        Options& withDoubleClickTime (std::optional<RelativeTime> newDoubleClickTime) noexcept;

        /** Sets whether updates should only happen when the window is focused.

            @param onlyWhenFocused True to only update when focused, false to update regardless of focus state.

            @return Reference to this Options object for method chaining.
        */
        Options& withUpdateOnlyFocused (bool onlyWhenFocused) noexcept;

        /** Sets how dirty rectangles are turned into repaint work.

            @param newRepaintMode The repaint mode to use.

            @return Reference to this Options object for method chaining.

            @see RepaintMode
        */
        Options& withRepaintMode (RepaintMode newRepaintMode) noexcept;

        /** The configuration flags for the component. */
        Flags flags = defaultFlags;
        /** The graphics API to use for rendering. */
        std::optional<GpuPlatform> graphicsApi;
        /** The target framerate for continuous rendering. */
        std::optional<float> framerateRedraw;
        /** The target framerate to use while the window does not have keyboard focus. */
        std::optional<float> unfocusedFramerateRedraw;
        /** The clear color to use when rendering. */
        std::optional<Color> clearColor;
        /** The maximum time between clicks to be considered a double-click. */
        std::optional<RelativeTime> doubleClickTime;
        /** Whether updates should only happen when the window is focused. */
        bool updateOnlyWhenFocused = false;
        /** How dirty rectangles are turned into repaint work. */
        RepaintMode repaintMode = RepaintMode::disjointRegions;
    };

    //==============================================================================
    /** Constructor.

        @param newComponent The Component associated with this native component.

        @param newFlags The flags used to configure the native component.
    */
    ComponentNative (Component& newComponent, const Flags& newFlags);

    /** Destructor. */
    virtual ~ComponentNative();

    //==============================================================================
    /** Sets the window title.

        @param title The new title to set.
    */
    virtual void setTitle (const String& title) = 0;

    /** Gets the current window title.

        @return The current window title.
    */
    virtual String getTitle() const = 0;

    //==============================================================================
    /** Sets the visibility of the window.

        @param shouldBeVisible True to make the window visible, false to hide it.
    */
    virtual void setVisible (bool shouldBeVisible) = 0;

    /** Checks whether the window is currently visible.

        @return True if the window is visible, false otherwise.
    */
    virtual bool isVisible() const = 0;

    //==============================================================================
    virtual void toFront() = 0;

    //==============================================================================
    /** Sets the size of the window.

        @param newSize The new size to set.
    */
    virtual void setSize (const Size<int>& newSize) = 0;

    /** Gets the current size of the window.

        @return The current window size.
    */
    virtual Size<int> getSize() const = 0;

    /** Gets the size of the window's content area.

        @return The size of the window's content area.
    */
    virtual Size<int> getContentSize() const = 0;

    /** Gets the position of the window.

        @return The current window position.
    */
    virtual Point<int> getPosition() const = 0;

    /** Sets the position of the window.

        @param newPosition The new position to set.
    */
    virtual void setPosition (const Point<int>& newPosition) = 0;

    /** Gets the bounds of the window.

        @return The current window bounds.
    */
    virtual Rectangle<int> getBounds() const = 0;

    /** Sets the bounds of the window.

        @param newBounds The new bounds to set.
    */
    virtual void setBounds (const Rectangle<int>& newBounds) = 0;

    //==============================================================================
    /** Gets the area of the window that is safe for interactive content.

        On mobile devices this excludes areas covered by display cutouts (notch), the
        status bar or rounded corners. On desktop platforms this usually matches the
        full window bounds.

        @return The safe area bounds, in window coordinates.
    */
    virtual Rectangle<int> getSafeAreaBounds() const = 0;

    //==============================================================================
    /** Sets whether the window should be in fullscreen mode.

        @param shouldBeFullScreen True to make the window fullscreen, false to exit fullscreen.
    */
    virtual void setFullScreen (bool shouldBeFullScreen) = 0;

    /** Checks whether the window is currently in fullscreen mode.

        @return True if the window is fullscreen, false otherwise.
    */
    virtual bool isFullScreen() const = 0;

    //==============================================================================
    /** Checks whether the window has decorations.

        @return True if the window has decorations, false otherwise.
    */
    virtual bool isDecorated() const = 0;

    //==============================================================================
    /** Sets the opacity of the window.

        @param opacity The opacity value, where 0.0 is fully transparent and 1.0 is fully opaque.
    */
    virtual void setOpacity (float opacity) = 0;

    /** Gets the current opacity of the window.

        @return The current opacity value.
    */
    virtual float getOpacity() const = 0;

    //==============================================================================
    /** Sets the focused component.

        @param comp  The component to focus, or nullptr to clear focus.
        @param cause What triggered the focus change, reported to the ancestors of the
                     components losing and gaining the focus.
    */
    virtual void setFocusedComponent (Component* comp, FocusChangeType cause = FocusChangeType::focusChangedDirectly) = 0;

    /** Gets the currently focused component.

        @return The currently focused component, or nullptr if no component has focus.
    */
    virtual Component* getFocusedComponent() const = 0;

    //==============================================================================
    /** Checks whether continuous repainting is enabled.

        @return True if continuous repainting is enabled, false otherwise.
    */
    virtual bool isContinuousRepaintingEnabled() const = 0;

    /** Enables or disables continuous repainting.

        @param shouldBeEnabled True to enable continuous repainting, false to disable.
    */
    virtual void enableContinuousRepainting (bool shouldBeEnabled) = 0;

    /** Checks whether atomic mode is enabled.

        @return True if atomic mode is enabled, false otherwise.
    */
    virtual bool isAtomicModeEnabled() const = 0;

    /** Enables or disables atomic mode.

        @param shouldBeEnabled True to enable atomic mode, false to disable.
    */
    virtual void enableAtomicMode (bool shouldBeEnabled) = 0;

    /** Checks whether wireframe mode is enabled.

        @return True if wireframe mode is enabled, false otherwise.
    */
    virtual bool isWireframeEnabled() const = 0;

    /** Enables or disables wireframe mode.

        @param shouldBeEnabld True to enable wireframe mode, false to disable.
    */
    virtual void enableWireframe (bool shouldBeEnabld) = 0;

    //==============================================================================
    /** Requests a repaint of the entire component. */
    virtual void repaint() = 0;

    /** Requests a repaint of a specific area of the component.

        @param rect The area to repaint.
    */
    virtual void repaint (const Rectangle<float>& rect) = 0;

    /** Gets the list of areas that are currently scheduled for repainting.

        @return The list of areas scheduled for repainting.
    */
    virtual const RectangleList<float>& getRepaintAreas() const = 0;

    //==============================================================================
    /** Enables or disables mouse capture unconditionally, ignoring the ComponentNative::captureMouse flag.

        A drag session uses this to keep receiving mouse events while the pointer travels outside the
        window the drag started from, even when that window was not created with captureMouse.

        @param shouldBeActive True to capture mouse input outside the native window, false to release it.
    */
    virtual void setGlobalMouseCaptureActive (bool shouldBeActive) = 0;

    /** Ends the mouse gesture currently in flight, when the platform consumed the button release.

        A native drag session runs its own event loop and swallows the release that ends it, so the
        window would be left believing the button is still down. This delivers the missing mouse up
        to the component that was clicked and forgets the gesture.

        Does nothing when no button is down.
    */
    virtual void cancelCurrentMouseGesture() = 0;

    //==============================================================================
    /** Starts text input for the specified component.

        @param component The component to start text input for.
    */
    virtual void startTextInput (Component& component) = 0;

    /** Stops text input for the specified component.

        @param component The component to stop text input for.
    */
    virtual void stopTextInput (Component& component) = 0;

    /** Updates the native text input rectangle for the specified component.

        @param component The component whose text input rectangle has changed.
    */
    virtual void updateTextInputRect (Component& component) = 0;

    //==============================================================================
    /** Gets the DPI scale factor.

        @return The current DPI scale factor.
    */
    virtual float getScaleDpi() const = 0;

    //==============================================================================
    /** Gets the current framerate.

        @return The current framerate in frames per second.
    */
    virtual float getCurrentFrameRate() const = 0;

    /** Gets the desired framerate.

        This is the rate that was asked for, which is not necessarily the rate frames are being
        produced at: the window throttles itself while unfocused if an unfocused framerate was
        configured, and a frame that takes longer than its budget delays the ones after it. Use
        getCurrentFrameRate() for the rate actually being achieved.

        @return The desired framerate in frames per second.

        @see setDesiredFrameRate, getCurrentFrameRate
    */
    virtual float getDesiredFrameRate() const = 0;

    /** Sets the desired framerate.

        Takes effect on the next frame, so it is safe to call while the window is rendering. Use
        it to throttle a window that has nothing to show, for example one that is obscured by
        another window, or to raise the rate again when it becomes interesting.

        @param newFrameRate The desired framerate in frames per second.

        @see getDesiredFrameRate, Options::withUnfocusedFramerateRedraw
    */
    virtual void setDesiredFrameRate (float newFrameRate) = 0;

    //==============================================================================
    /** Gets the native handle for the component.

        @return The native handle as a void pointer.
    */
    virtual void* getNativeHandle() const = 0;

    //==============================================================================
    /** Gets the Rive factory associated with this component.

        @return The Rive factory instance.
    */
    virtual rive::Factory* getFactory() = 0;

    /** Gets the graphics context associated with this component.

        @return Pointer to the GraphicsContext, or nullptr if unavailable.
    */
    virtual GraphicsContext* getGraphicsContext() = 0;

    //==============================================================================
    /** Runs @a fn with the native GPU context made current on this thread, when
        the backend requires it.

        OpenGL contexts are thread-affine, and the windowing layer binds the
        context to a dedicated render thread. Offscreen GPU work initiated from
        other threads (e.g. component snapshots taken from the message thread)
        must run through this hook so the context is bound, and its access is
        serialized with the render thread, for the duration of @a fn.

        The default implementation simply invokes @a fn.

        @param fn The GPU work to run with the context current.
    */
    virtual void runWithGraphicsContext (const std::function<void()>& fn) { fn(); }

    //==============================================================================
    /** Returns the Component this native window displays.

        @return The root Component associated with this native component.
    */
    Component& getComponent();

    /** Returns the Component this native window displays.

        @return The root Component associated with this native component.
    */
    const Component& getComponent() const;

    //==============================================================================
    /** Creates a platform-specific ComponentNative instance.

        This factory method creates an appropriate ComponentNative implementation based on the
        current platform and the provided options.

        @param component The Component to associate with the native component.
        @param options The options to configure the native component.
        @param parent Optional pointer to a parent native window, or nullptr for a top-level window.

        @return A reference counted pointer to the created ComponentNative instance.
    */
    static ComponentNative::Ptr createFor (Component& component,
                                           const Options& options,
                                           void* parent);

protected:
    /** The Component associated with this native component. */
    Component& component;
    /** The configuration flags for this native component. */
    Flags flags;

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentNative)
};

} // namespace yup
