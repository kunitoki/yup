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
    struct decoratedWindowTag;
    struct resizableWindowTag;
    struct skipTaskbarTag;
    struct renderContinuousTag;
    struct allowHighDensityDisplayTag;

public:
    //==============================================================================
    using Flags = FlagSet<uint32,
        decoratedWindowTag,
        resizableWindowTag,
        skipTaskbarTag,
        renderContinuousTag,
        allowHighDensityDisplayTag>;

    static inline constexpr Flags noFlags = Flags();
    static inline constexpr Flags decoratedWindow = Flags::declareValue<decoratedWindowTag>();
    static inline constexpr Flags resizableWindow = Flags::declareValue<resizableWindowTag>();
    static inline constexpr Flags skipTaskbar = Flags::declareValue<skipTaskbarTag>();
    static inline constexpr Flags renderContinuous = Flags::declareValue<renderContinuousTag>();
    static inline constexpr Flags allowHighDensityDisplay = Flags::declareValue<allowHighDensityDisplayTag>();
    static inline constexpr Flags defaultFlags = decoratedWindow | resizableWindow | allowHighDensityDisplay;

    //==============================================================================
    /** Configuration options for creating a native component. */
    struct Options
    {
        /** Default constructor, initializes the options with default values. */
        constexpr Options() noexcept = default;

        Options& withFlags (Flags newFlags) noexcept
        {
            flags = newFlags;
            return *this;
        }

        Options& withDecoration (bool shouldHaveDecoration) noexcept
        {
            if (shouldHaveDecoration)
                flags |= decoratedWindow;
            else
                flags &= ~decoratedWindow;
            return *this;
        }

        Options& withResizableWindow (bool shouldAllowResizing) noexcept
        {
            if (shouldAllowResizing)
                flags |= resizableWindow;
            else
                flags &= ~resizableWindow;
            return *this;
        }

        Options& withRenderContinuous (bool shouldRenderContinuous) noexcept
        {
            if (shouldRenderContinuous)
                flags |= renderContinuous;
            else
                flags &= ~renderContinuous;
            return *this;
        }

        Options& withSkipTaskbar (bool shouldSkipTaskbar) noexcept
        {
            if (shouldSkipTaskbar)
                flags |= skipTaskbar;
            else
                flags &= ~skipTaskbar;
            return *this;
        }

        Options& withAllowedHighDensityDisplay (bool shouldAllowHighDensity) noexcept
        {
            if (shouldAllowHighDensity)
                flags |= allowHighDensityDisplay;
            else
                flags &= ~allowHighDensityDisplay;
            return *this;
        }

        Options& withGraphicsApi (std::optional<GraphicsContext::Api> newGraphicsApi) noexcept
        {
            graphicsApi = newGraphicsApi;
            return *this;
        }

        Options& withFramerateRedraw (std::optional<float> newFramerateRedraw) noexcept
        {
            framerateRedraw = newFramerateRedraw;
            return *this;
        }

        Options& withClearColor (std::optional<Color> newClearColor) noexcept
        {
            clearColor = newClearColor;
            return *this;
        }

        Options& withDoubleClickTime (std::optional<RelativeTime> newDoubleClickTime) noexcept
        {
            doubleClickTime = newDoubleClickTime;
            return *this;
        }

        Options& withUpdateOnlyFocused (bool onlyWhenFocused) noexcept
        {
            updateOnlyWhenFocused = onlyWhenFocused;
            return *this;
        }

        Flags flags = defaultFlags;                      ///<
        std::optional<GraphicsContext::Api> graphicsApi; ///<
        std::optional<float> framerateRedraw;            ///<
        std::optional<Color> clearColor;                 ///<
        std::optional<RelativeTime> doubleClickTime;     ///<
        bool updateOnlyWhenFocused = false;              ///<
    };

    //==============================================================================
    ComponentNative (Component& newComponent, const Flags& newFlags);
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
    virtual bool isDecorated() const = 0;

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
    virtual void repaint() = 0;
    virtual void repaint (const Rectangle<float>& rect) = 0;
    virtual const RectangleList<float>& getRepaintAreas() const = 0;

    //==============================================================================
    virtual float getScaleDpi() const = 0;

    //==============================================================================
    virtual float getCurrentFrameRate() const = 0;
    virtual float getDesiredFrameRate() const = 0;

    //==============================================================================
    virtual void* getNativeHandle() const = 0;

    //==============================================================================
    virtual rive::Factory* getFactory() = 0;

    //==============================================================================
    static std::unique_ptr<ComponentNative> createFor (Component& component,
                                                       const Options& options,
                                                       void* parent);

protected:
    Component& component;
    Flags flags;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentNative)
};

} // namespace yup
