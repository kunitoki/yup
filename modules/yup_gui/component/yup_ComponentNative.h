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
/**
    Provides platform-native window and rendering capabilities for Components.

    This class serves as an abstraction layer between the Component class and the platform-specific
    window management and rendering systems. It handles native window creation, event processing,
    and rendering pipeline setup across different platforms.

    ComponentNative objects are typically created internally by the Component class when a Component
    needs to be displayed on screen as a top-level window.

    @see Component
*/
class YUP_API ComponentNative
{
    struct decoratedWindowTag;
    struct resizableWindowTag;
    struct renderContinuousTag;
    struct allowHighDensityDisplayTag;

public:
    //==============================================================================
    /** Type definition for window configuration flags. */
    using Flags = FlagSet<uint32, decoratedWindowTag, resizableWindowTag, renderContinuousTag, allowHighDensityDisplayTag>;

    /** No flags set. */
    static inline constexpr Flags noFlags = Flags();
    /** Flag to enable window decorations (title bar, borders, etc.). */
    static inline constexpr Flags decoratedWindow = Flags::declareValue<decoratedWindowTag>();
    /** Flag to enable window resizing by the user. */
    static inline constexpr Flags resizableWindow = Flags::declareValue<resizableWindowTag>();
    /** Flag to enable continuous rendering mode. */
    static inline constexpr Flags renderContinuous = Flags::declareValue<renderContinuousTag>();
    /** Flag to enable high-density display support. */
    static inline constexpr Flags allowHighDensityDisplay = Flags::declareValue<allowHighDensityDisplayTag>();
    /** Default flags combining decoratedWindow, resizableWindow, and allowHighDensityDisplay. */
    static inline constexpr Flags defaultFlags = decoratedWindow | resizableWindow | allowHighDensityDisplay;

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

        /** Sets the graphics API to be used for rendering.

            @param newGraphicsApi The graphics API to use, or std::nullopt to use the default.

            @return Reference to this Options object for method chaining.
        */
        Options& withGraphicsApi (std::optional<GraphicsContext::Api> newGraphicsApi) noexcept;

        /** Sets the target framerate for continuous rendering.

            @param newFramerateRedraw The target framerate, or std::nullopt to use the default.

            @return Reference to this Options object for method chaining.
        */
        Options& withFramerateRedraw (std::optional<float> newFramerateRedraw) noexcept;

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

        /** The configuration flags for the component. */
        Flags flags = defaultFlags;
        /** The graphics API to use for rendering. */
        std::optional<GraphicsContext::Api> graphicsApi;
        /** The target framerate for continuous rendering. */
        std::optional<float> framerateRedraw;
        /** The clear color to use when rendering. */
        std::optional<Color> clearColor;
        /** The maximum time between clicks to be considered a double-click. */
        std::optional<RelativeTime> doubleClickTime;
        /** Whether updates should only happen when the window is focused. */
        bool updateOnlyWhenFocused = false;
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

        @param comp The component to focus, or nullptr to clear focus.
    */
    virtual void setFocusedComponent (Component* comp) = 0;

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

        @return The desired framerate in frames per second.
    */
    virtual float getDesiredFrameRate() const = 0;

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

    //==============================================================================
    /** Creates a platform-specific ComponentNative instance.

        This factory method creates an appropriate ComponentNative implementation based on the
        current platform and the provided options.

        @param component The Component to associate with the native component.
        @param options The options to configure the native component.
        @param parent Optional pointer to a parent native window, or nullptr for a top-level window.

        @return A unique_ptr to the created ComponentNative instance.
    */
    static std::unique_ptr<ComponentNative> createFor (Component& component,
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
