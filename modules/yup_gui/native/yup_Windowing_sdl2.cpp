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

#ifndef YUP_WINDOWING_LOGGING
#define YUP_WINDOWING_LOGGING 1
#endif

#if YUP_WINDOWING_LOGGING
#define YUP_WINDOWING_LOG(textToWrite) YUP_DBG (textToWrite)
#else
#define YUP_WINDOWING_LOG(textToWrite) \
    {                                  \
    }
#endif

//==============================================================================

std::atomic_flag SDL2ComponentNative::isInitialised = ATOMIC_FLAG_INIT;

//==============================================================================

SDL2ComponentNative::SDL2ComponentNative (Component& component,
                                          const Options& options,
                                          void* parent)
    : ComponentNative (component, options.flags)
    , Thread ("YUP Render Thread")
    , parentWindow (parent)
    , currentGraphicsApi (getGraphicsContextApi (options.graphicsApi))
    , clearColor (options.clearColor.value_or (Colors::black))
    , screenBounds (component.getBounds().to<int>())
    , doubleClickTime (options.doubleClickTime.value_or (RelativeTime::milliseconds (200)))
    , desiredFrameRate (options.framerateRedraw.value_or (60.0f))
    , shouldRenderContinuous (options.flags.test (renderContinuous))
    , updateOnlyWhenFocused (options.updateOnlyWhenFocused)
{
    incReferenceCount();

    Desktop::getInstance()->registerNativeComponent (this);

    SDL_AddEventWatch (eventDispatcher, this);

    // Setup window hints and get flags
    windowFlags = setContextWindowHints (currentGraphicsApi);

    if (options.flags.test (resizableWindow))
        windowFlags |= SDL_WINDOW_RESIZABLE;

    if (component.isVisible())
        windowFlags |= SDL_WINDOW_SHOWN;
    else
        windowFlags |= SDL_WINDOW_HIDDEN;

    if (options.flags.test (allowHighDensityDisplay))
        windowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;

    if (! options.flags.test (decoratedWindow))
        windowFlags |= SDL_WINDOW_BORDERLESS;

    SDL_SetHint (SDL_HINT_ORIENTATIONS, "Portrait PortraitUpsideDown LandscapeLeft LandscapeRight");
    SDL_SetHint (SDL_HINT_MOUSE_DOUBLE_CLICK_TIME, String (doubleClickTime.inMilliseconds()).toRawUTF8());
    SDL_SetHint (SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    // Create the window, renderer and parent it
    window = SDL_CreateWindow (component.getTitle().toRawUTF8(),
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               1,
                               1,
                               windowFlags);
    if (window == nullptr)
        return; // TODO - raise something ?

    SDL_SetWindowData (window, "self", this);

    if (parent != nullptr)
        setNativeParent (parent, window);

    if (currentGraphicsApi == GraphicsContext::OpenGL)
    {
        windowContext = SDL_GL_CreateContext (window);
        if (windowContext == nullptr)
            return; // TODO - raise something ?

        SDL_GL_MakeCurrent (window, windowContext);
    }

    // Create the rendering context
    GraphicsContext::Options graphicsOptions;
    graphicsOptions.retinaDisplay = options.flags.test (allowHighDensityDisplay);
    graphicsOptions.loaderFunction = SDL_GL_GetProcAddress;
    context = GraphicsContext::createContext (currentGraphicsApi, graphicsOptions);
    if (context == nullptr)
        return; // TODO - raise something ?

    // Resize after callbacks are in place
    setBounds (
        { screenBounds.getX(),
          screenBounds.getY(),
          jmax (1, screenBounds.getWidth()),
          jmax (1, screenBounds.getHeight()) });

    // Start the rendering
    startRendering();
}

SDL2ComponentNative::~SDL2ComponentNative()
{
    // Remove event watch
    SDL_DelEventWatch (eventDispatcher, this);

    // Unregister this component from the desktop
    Desktop::getInstance()->unregisterNativeComponent (this);

    // Stop the rendering
    stopRendering();

    // Destroy the window
    if (window != nullptr)
    {
        SDL_SetWindowData (window, "self", nullptr);
        SDL_DestroyWindow (window);
        window = nullptr;
    }
}

//==============================================================================

void SDL2ComponentNative::setTitle (const String& title)
{
    if (windowTitle == title)
        return;

    if (window != nullptr)
        SDL_SetWindowTitle (window, title.toRawUTF8());

    windowTitle = title;
}

String SDL2ComponentNative::getTitle() const
{
#if ! (YUP_EMSCRIPTEN && RIVE_WEBGL)
    if (window == nullptr)
        return {};

    if (auto title = SDL_GetWindowTitle (window))
        return String::fromUTF8 (title);
#endif

    return windowTitle;
}

//==============================================================================

void SDL2ComponentNative::setVisible (bool shouldBeVisible)
{
    if (window == nullptr)
        return;

    if (shouldBeVisible)
        SDL_ShowWindow (window);
    else
        SDL_HideWindow (window);
}

bool SDL2ComponentNative::isVisible() const
{
    return window != nullptr && (SDL_GetWindowFlags (window) & SDL_WINDOW_SHOWN) != 0;
}

//==============================================================================

Size<int> SDL2ComponentNative::getContentSize() const
{
    const auto dpiScale = getScaleDpi();

    const auto width = static_cast<int> (screenBounds.getWidth() * dpiScale);
    const auto height = static_cast<int> (screenBounds.getHeight() * dpiScale);

    return { width, height };
}

//==============================================================================

void SDL2ComponentNative::setSize (const Size<int>& newSize)
{
    if (window == nullptr)
        return;

    screenBounds = screenBounds.withSize (newSize);

    if (auto currentSize = getSize(); currentSize != newSize)
        SDL_SetWindowSize (window, jmax (1, newSize.getWidth()), jmax (1, newSize.getHeight()));
}

Size<int> SDL2ComponentNative::getSize() const
{
    int width = 0, height = 0;

    if (window != nullptr)
        SDL_GetWindowSize (window, &width, &height);

    return { width, height };
}

void SDL2ComponentNative::setPosition (const Point<int>& newPosition)
{
    if (window == nullptr)
        return;

    screenBounds = screenBounds.withPosition (newPosition);

    if (auto currentPosition = getPosition(); currentPosition != newPosition)
        SDL_SetWindowPosition (window, newPosition.getX(), newPosition.getY());
}

Point<int> SDL2ComponentNative::getPosition() const
{
    int x = 0, y = 0;

    if (window != nullptr)
        SDL_GetWindowPosition (window, &x, &y);

    return { x, y };
}

void SDL2ComponentNative::setBounds (const Rectangle<int>& newBounds)
{
#if YUP_ANDROID
    screenBounds = Rectangle<int> (0, 0, getSize());

#else
    if (window == nullptr)
        return;

    auto adjustedBounds = newBounds;
    int leftMargin = 0, topMargin = 0, rightMargin = 0, bottomMargin = 0;

#if YUP_EMSCRIPTEN && RIVE_WEBGL
    const double devicePixelRatio = emscripten_get_device_pixel_ratio();
    SDL_SetWindowSize (window,
                       static_cast<int> (newBounds.getWidth() * devicePixelRatio),
                       static_cast<int> (newBounds.getHeight() * devicePixelRatio));

    emscripten_set_element_css_size ("#canvas",
                                     jmax (0, newBounds.getWidth()),
                                     jmax (0, newBounds.getHeight()));

#else
    if (! isFullScreen() && isDecorated())
        SDL_GetWindowBordersSize (window, &leftMargin, &topMargin, &rightMargin, &bottomMargin);

    adjustedBounds.translate (leftMargin, topMargin);
    adjustedBounds.setSize ({ jmax (1, adjustedBounds.getWidth() - leftMargin - rightMargin),
                              jmax (1, adjustedBounds.getHeight() - topMargin - bottomMargin) });

    if (auto currentSize = getSize(); currentSize != adjustedBounds.getSize())
        SDL_SetWindowSize (window, adjustedBounds.getWidth(), adjustedBounds.getHeight());

#endif

    if (auto currentPosition = getPosition(); currentPosition != adjustedBounds.getPosition())
        SDL_SetWindowPosition (window, adjustedBounds.getX(), adjustedBounds.getY());

    screenBounds = newBounds;

#endif
}

Rectangle<int> SDL2ComponentNative::getBounds() const
{
    return screenBounds;
}

//==============================================================================

void SDL2ComponentNative::setFullScreen (bool shouldBeFullScreen)
{
    if (window == nullptr)
        return;

    if (shouldBeFullScreen)
    {
#if YUP_EMSCRIPTEN
        emscripten_request_fullscreen ("#canvas", false);
#else
        lastScreenBounds = screenBounds;

        SDL_SetWindowFullscreen (window, SDL_WINDOW_FULLSCREEN); // SDL_SetWindowDisplayMode
#endif
    }
    else
    {
#if YUP_EMSCRIPTEN
        emscripten_exit_fullscreen();
#else
        SDL_RestoreWindow (window);
        SDL_SetWindowSize (window, component.getWidth(), component.getHeight());
        SDL_SetWindowPosition (window, component.getX(), component.getY());

        setBounds (lastScreenBounds);
#endif
    }
}

bool SDL2ComponentNative::isFullScreen() const
{
    return window != nullptr && (SDL_GetWindowFlags (window) & SDL_WINDOW_FULLSCREEN) != 0;
}

//==============================================================================

bool SDL2ComponentNative::isDecorated() const
{
    return window != nullptr && (SDL_GetWindowFlags (window) & SDL_WINDOW_BORDERLESS) == 0;
}

//==============================================================================

void SDL2ComponentNative::setOpacity (float opacity)
{
    if (window != nullptr)
        SDL_SetWindowOpacity (window, jlimit (0.0f, 1.0f, opacity));
}

float SDL2ComponentNative::getOpacity() const
{
    float opacity = 1.0f;

    if (window != nullptr)
        SDL_GetWindowOpacity (window, &opacity);

    return opacity;
}

//==============================================================================

void SDL2ComponentNative::setFocusedComponent (Component* comp)
{
    auto compBailOut = Component::BailOutChecker (comp);

    if (lastComponentFocused != nullptr)
    {
        auto focusBailOut = Component::BailOutChecker (lastComponentFocused.get());

        lastComponentFocused->focusLost();

        if (! focusBailOut.shouldBailOut())
            lastComponentFocused->repaint();
    }

    if (compBailOut.shouldBailOut())
        return;

    lastComponentFocused = comp;

    if (lastComponentFocused != nullptr)
    {
        auto focusBailOut = Component::BailOutChecker (lastComponentFocused.get());

        lastComponentFocused->focusGained();

        if (! focusBailOut.shouldBailOut())
            lastComponentFocused->repaint();
    }

    if (window != nullptr)
    {
        if ((SDL_GetWindowFlags (window) & SDL_WINDOW_INPUT_FOCUS) == 0) // SDL_WINDOW_MOUSE_FOCUS
            SDL_SetWindowInputFocus (window);
    }
}

Component* SDL2ComponentNative::getFocusedComponent() const
{
    return lastComponentFocused;
}

//==============================================================================

bool SDL2ComponentNative::isContinuousRepaintingEnabled() const
{
    return shouldRenderContinuous;
}

void SDL2ComponentNative::enableContinuousRepainting (bool shouldBeEnabled)
{
    shouldRenderContinuous = shouldBeEnabled;
}

bool SDL2ComponentNative::isAtomicModeEnabled() const
{
    return renderAtomicMode;
}

void SDL2ComponentNative::enableAtomicMode (bool shouldBeEnabled)
{
    renderAtomicMode = shouldBeEnabled;

    repaint();
}

bool SDL2ComponentNative::isWireframeEnabled() const
{
    return renderWireframe;
}

void SDL2ComponentNative::enableWireframe (bool shouldBeEnabled)
{
    renderWireframe = shouldBeEnabled;

    repaint();
}

//==============================================================================

void SDL2ComponentNative::repaint()
{
    currentRepaintAreas.clearQuick();

    currentRepaintAreas.add (Rectangle<float>().withSize (getSize().to<float>()));
}

void SDL2ComponentNative::repaint (const Rectangle<float>& rect)
{
    currentRepaintAreas.add (rect);
}

const RectangleList<float>& SDL2ComponentNative::getRepaintAreas() const
{
    return currentRepaintAreas;
}

//==============================================================================

float SDL2ComponentNative::getScaleDpi() const
{
    return context != nullptr ? context->dpiScale (getNativeHandle()) : 1.0f;
}

float SDL2ComponentNative::getCurrentFrameRate() const
{
    return currentFrameRate.load (std::memory_order_relaxed);
}

float SDL2ComponentNative::getDesiredFrameRate() const
{
    return desiredFrameRate;
}

//==============================================================================

Point<float> SDL2ComponentNative::getCursorPosition() const
{
    int x = 0, y = 0;

    SDL_GetMouseState (&x, &y);

    return {
        static_cast<float> (x),
        static_cast<float> (y)
    };
}

//==============================================================================

rive::Factory* SDL2ComponentNative::getFactory()
{
    return context ? context->factory() : nullptr;
}

//==============================================================================

void* SDL2ComponentNative::getNativeHandle() const
{
    return getNativeWindowHandle (window);
}

//==============================================================================

void SDL2ComponentNative::run()
{
    const double maxFrameTimeSeconds = 1.0 / static_cast<double> (desiredFrameRate);
    const double maxFrameTimeMs = maxFrameTimeSeconds * 1000.0;

    while (! threadShouldExit())
    {
        double frameStartTimeSeconds = yup::Time::getMillisecondCounterHiRes() / 1000.0;

        // Trigger and wait for rendering
        renderEvent.reset();
        cancelPendingUpdate();
        triggerAsyncUpdate();
        renderEvent.wait (maxFrameTimeMs - 4.0);

        if (threadShouldExit())
            break;

        // Measure spent time and cap the framerate
        double currentTimeSeconds = yup::Time::getMillisecondCounterHiRes() / 1000.0;
        double timeSpentSeconds = currentTimeSeconds - frameStartTimeSeconds;

        const double secondsToWait = maxFrameTimeSeconds - timeSpentSeconds;
        if (secondsToWait > 0.0)
        {
            const auto waitUntilMs = (currentTimeSeconds + secondsToWait) * 1000.0;

            while (yup::Time::getMillisecondCounterHiRes() < waitUntilMs - 4.0)
                std::this_thread::sleep_for (std::chrono::microseconds (1000));

            while (yup::Time::getMillisecondCounterHiRes() < waitUntilMs - 2.0)
                std::this_thread::sleep_for (std::chrono::microseconds (500));

            while (yup::Time::getMillisecondCounterHiRes() < waitUntilMs)
                std::this_thread::sleep_for (std::chrono::microseconds (10));
        }
    }
}

void SDL2ComponentNative::handleAsyncUpdate()
{
    if (! isThreadRunning() || ! isInitialised.test_and_set())
        return;

    renderContext();

    renderEvent.signal();
}

void SDL2ComponentNative::timerCallback()
{
    renderContext();
}

//==============================================================================

void SDL2ComponentNative::renderContext()
{
    YUP_PROFILE_NAMED_INTERNAL_TRACE (RenderContext);

    if (context == nullptr)
        return;

    const auto contentSize = getContentSize();
    auto contentWidth = contentSize.getWidth();
    auto contentHeight = contentSize.getHeight();

    if (contentWidth == 0 || contentHeight == 0)
        return;

    if (currentContentWidth != contentWidth || currentContentHeight != contentHeight)
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (ResizeRenderer);

        currentContentWidth = contentWidth;
        currentContentHeight = contentHeight;

        context->onSizeChanged (getNativeHandle(), contentWidth, contentHeight, 0);
        renderer = context->makeRenderer (contentWidth, contentHeight);

        repaint();
    }

    auto renderContinuous = shouldRenderContinuous.load (std::memory_order_relaxed);
    auto currentTimeSeconds = yup::Time::getMillisecondCounterHiRes() / 1000.0;

    const auto measureFramesPerSeconds = ErasedScopeGuard ([&]
    {
        ++frameRateCounter;

        const double timeSinceFpsMeasure = currentTimeSeconds - frameRateStartTimeSeconds;
        if (timeSinceFpsMeasure >= 1.0)
        {
            const double currentFps = static_cast<double> (frameRateCounter) / timeSinceFpsMeasure;
            currentFrameRate.store (currentFps, std::memory_order_relaxed);

            frameRateStartTimeSeconds = currentTimeSeconds;
            frameRateCounter = 0;
        }
    });

    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (RefreshDisplay);

        component.internalRefreshDisplay (currentTimeSeconds - lastRenderTimeSeconds);
        lastRenderTimeSeconds = currentTimeSeconds;
    }

    if (renderContinuous)
        repaint();
    else if (currentRepaintAreas.isEmpty())
        return;

    auto renderFrame = [&]
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (RenderFrame);

        // Setup frame description
        const auto loadAction = (renderContinuous)
                                  ? rive::gpu::LoadAction::clear
                                  : rive::gpu::LoadAction::preserveRenderTarget;

        rive::gpu::RenderContext::FrameDescriptor frameDescriptor;
        frameDescriptor.renderTargetWidth = static_cast<uint32_t> (currentContentWidth);
        frameDescriptor.renderTargetHeight = static_cast<uint32_t> (currentContentHeight);
        frameDescriptor.loadAction = loadAction;
        frameDescriptor.clearColor = clearColor.getARGB();
        frameDescriptor.disableRasterOrdering = renderAtomicMode;
        frameDescriptor.wireframe = renderWireframe;
        frameDescriptor.fillsDisabled = false;
        frameDescriptor.strokesDisabled = false;
        frameDescriptor.clockwiseFillOverride = true;

        {
            YUP_PROFILE_NAMED_INTERNAL_TRACE (ContextBegin);

            // Begin context drawing
            context->begin (frameDescriptor);
        }

        // Repaint components hierarchy
        if (renderer != nullptr)
        {
            const auto dpiScale = getScaleDpi();

            for (auto& repaintArea : currentRepaintAreas)
            {
                YUP_PROFILE_NAMED_INTERNAL_TRACE (InternalPaint);

                Graphics g (*context, *renderer, dpiScale);
                component.internalPaint (g, repaintArea, renderContinuous);
            }
        }

        // Finish context drawing
        {
            YUP_PROFILE_NAMED_INTERNAL_TRACE (ContextEnd);

            context->end (getNativeHandle());
            context->tick();
        }
    };

    renderFrame();

    // Swap buffers
    if (window != nullptr && currentGraphicsApi == GraphicsContext::OpenGL)
        SDL_GL_SwapWindow (window);

    // Clear repainted areas
    currentRepaintAreas.clearQuick();
}

//==============================================================================

void SDL2ComponentNative::startRendering()
{
    lastRenderTimeSeconds = yup::Time::getMillisecondCounterHiRes() / 1000.0;
    frameRateStartTimeSeconds = lastRenderTimeSeconds;
    frameRateCounter = 0;

    if constexpr (renderDrivenByTimer)
    {
        if (! isTimerRunning())
            startTimerHz (desiredFrameRate);
    }
    else
    {
        if (! isThreadRunning())
            startThread (Priority::high);
    }

    repaint();
}

void SDL2ComponentNative::stopRendering()
{
    if constexpr (renderDrivenByTimer)
    {
        if (isTimerRunning())
            stopTimer();
    }
    else
    {
        if (isThreadRunning())
        {
            signalThreadShouldExit();
            notify();
            renderEvent.signal();
            stopThread (-1);
        }
    }
}

bool SDL2ComponentNative::isRendering() const
{
    if constexpr (renderDrivenByTimer)
        return isTimerRunning();
    else
        return isThreadRunning();
}

//==============================================================================

void SDL2ComponentNative::handleMouseMoveOrDrag (const Point<float>& position)
{
    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (position);

    if (lastMouseDownPosition)
        event = event.withLastMouseDownPosition (*lastMouseDownPosition);

    if (lastMouseDownTime)
        event = event.withLastMouseDownTime (*lastMouseDownTime);

    if (lastComponentClicked != nullptr)
    {
        event = event.withSourceComponent (lastComponentClicked);

        lastComponentClicked->internalMouseDrag (event.withRelativePositionTo (lastComponentClicked));
    }
    else
    {
        updateComponentUnderMouse (event);

        if (lastComponentUnderMouse != nullptr)
            lastComponentUnderMouse->internalMouseMove (event.withRelativePositionTo (lastComponentUnderMouse));
    }

    lastMouseMovePosition = position;
}

void SDL2ComponentNative::handleMouseDown (const Point<float>& position, MouseEvent::Buttons button, KeyModifiers modifiers)
{
    currentMouseButtons = static_cast<MouseEvent::Buttons> (currentMouseButtons | button);
    currentKeyModifiers = modifiers;

    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (position);

    if (lastComponentClicked == nullptr)
    {
        if (auto child = component.findComponentAt (position))
            lastComponentClicked = child;
    }

    if (lastComponentClicked != nullptr)
    {
        const auto currentMouseDownTime = yup::Time::getCurrentTime();

        event = event.withSourceComponent (lastComponentClicked);

        lastComponentClicked->internalMouseDown (event.withRelativePositionTo (lastComponentClicked));

        lastMouseDownPosition = position;
        lastMouseDownTime = currentMouseDownTime;
    }

    lastMouseMovePosition = position;
}

void SDL2ComponentNative::handleMouseUp (const Point<float>& position, MouseEvent::Buttons button, KeyModifiers modifiers)
{
    currentMouseButtons = static_cast<MouseEvent::Buttons> (currentMouseButtons & ~button);
    currentKeyModifiers = modifiers;

    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (position);

    if (lastMouseDownPosition)
        event = event.withLastMouseDownPosition (*lastMouseDownPosition);

    if (lastMouseDownTime)
        event = event.withLastMouseDownTime (*lastMouseDownTime);

    if (lastComponentClicked != nullptr)
    {
        const auto currentMouseDownTime = yup::Time::getCurrentTime();

        event = event.withSourceComponent (lastComponentClicked);

        if (lastMouseUpTime
            && *lastMouseUpTime > yup::Time()
            && currentMouseDownTime - *lastMouseUpTime < doubleClickTime)
        {
            lastComponentClicked->internalMouseDoubleClick (event.withRelativePositionTo (lastComponentClicked));
        }

        lastComponentClicked->internalMouseUp (event.withRelativePositionTo (lastComponentClicked));

        lastMouseUpTime = currentMouseDownTime;
    }

    if (currentMouseButtons == MouseEvent::noButtons)
    {
        updateComponentUnderMouse (event);

        lastComponentClicked = nullptr;
    }

    lastMouseMovePosition = position;

    if (isMouseOutsideWindow (window))
        handleFocusChanged (false);
}

//==============================================================================

void SDL2ComponentNative::handleMouseWheel (const Point<float>& position, const MouseWheelData& wheelData)
{
    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (position);

    if (lastMouseDownPosition)
        event = event.withLastMouseDownPosition (*lastMouseDownPosition);

    if (lastMouseDownTime)
        event = event.withLastMouseDownTime (*lastMouseDownTime);

    updateComponentUnderMouse (event);

    if (lastComponentClicked != nullptr)
    {
        event = event.withSourceComponent (lastComponentClicked);

        lastComponentClicked->internalMouseWheel (event.withRelativePositionTo (lastComponentClicked), wheelData);
    }
    else if (lastComponentFocused != nullptr)
    {
        lastComponentFocused->internalMouseWheel (event.withRelativePositionTo (lastComponentFocused), wheelData);
    }
    else if (lastComponentUnderMouse != nullptr)
    {
        lastComponentUnderMouse->internalMouseWheel (event.withRelativePositionTo (lastComponentUnderMouse), wheelData);
    }
}

//==============================================================================

void SDL2ComponentNative::handleMouseEnter (const Point<float>& position)
{
    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (position);

    updateComponentUnderMouse (event);

    if (lastComponentUnderMouse != nullptr)
    {
        event = event.withSourceComponent (lastComponentUnderMouse);

        lastComponentUnderMouse->mouseEnter (event.withRelativePositionTo (lastComponentUnderMouse));
    }
}

void SDL2ComponentNative::handleMouseLeave (const Point<float>& position)
{
    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (position);

    if (lastComponentUnderMouse != nullptr)
    {
        event = event.withSourceComponent (lastComponentUnderMouse);

        lastComponentUnderMouse->mouseExit (event.withRelativePositionTo (lastComponentUnderMouse));
    }

    updateComponentUnderMouse (event);
}

//==============================================================================

void SDL2ComponentNative::handleKeyDown (const KeyPress& keys, const Point<float>& position)
{
    currentKeyModifiers = keys.getModifiers();
    keyState.set (keys.getKey(), 1);

    if (lastComponentFocused != nullptr)
        lastComponentFocused->internalKeyDown (keys, position); // TODO: remove position
    else
        component.internalKeyDown (keys, position);
}

void SDL2ComponentNative::handleKeyUp (const KeyPress& keys, const Point<float>& position)
{
    currentKeyModifiers = keys.getModifiers();
    keyState.set (keys.getKey(), 0);

    if (lastComponentFocused != nullptr)
        lastComponentFocused->internalKeyUp (keys, position); // TODO: remove position
    else
        component.internalKeyUp (keys, position);
}

void SDL2ComponentNative::handleTextInput (const String& textInput)
{
    if (lastComponentFocused != nullptr)
        lastComponentFocused->internalTextInput (textInput);
    else
        component.internalTextInput (textInput);
}

//==============================================================================

void SDL2ComponentNative::handleMoved (int xpos, int ypos)
{
    YUP_PROFILE_INTERNAL_TRACE();

    if (internalBoundsChange)
        return;

    component.internalMoved (xpos, ypos);

    screenBounds = screenBounds.withPosition (xpos, ypos);

    if (parentWindow != nullptr)
    {
        auto preventBoundsChange = ScopedValueSetter<bool> (internalBoundsChange, true);

        auto nativeWindowPos = getNativeWindowPosition (parentWindow);
        setPosition (nativeWindowPos.getTopLeft());
    }
}

void SDL2ComponentNative::handleResized (int width, int height)
{
    YUP_PROFILE_INTERNAL_TRACE();

    component.internalResized (width, height);

    screenBounds = screenBounds.withSize (width, height);

    if (parentWindow != nullptr)
    {
        auto preventBoundsChange = ScopedValueSetter<bool> (internalBoundsChange, true);

        auto nativeWindowPos = getNativeWindowPosition (parentWindow);
        setPosition (nativeWindowPos.getTopLeft());
    }

    repaint();
}

void SDL2ComponentNative::handleFocusChanged (bool gotFocus)
{
    YUP_PROFILE_INTERNAL_TRACE();

    if (gotFocus)
    {
        SDL_StartTextInput();

        if (! isRendering())
            startRendering();

        component.internalFocusChanged (true);
    }
    else
    {
        component.internalFocusChanged (false);

        lastComponentClicked = nullptr;
        lastMouseDownPosition.reset();
        lastMouseDownTime.reset();

        SDL_StopTextInput();

        if (updateOnlyWhenFocused)
        {
            if (isRendering())
                stopRendering();
        }
    }
}

void SDL2ComponentNative::handleMinimized()
{
    stopRendering();
}

void SDL2ComponentNative::handleMaximized()
{
    repaint();
}

void SDL2ComponentNative::handleRestored()
{
    repaint();
}

void SDL2ComponentNative::handleExposed()
{
    repaint();
}

void SDL2ComponentNative::handleContentScaleChanged()
{
    YUP_PROFILE_INTERNAL_TRACE();

    component.internalContentScaleChanged (getScaleDpi());

    handleResized (screenBounds.getWidth(), screenBounds.getHeight());
}

void SDL2ComponentNative::handleDisplayChanged()
{
    YUP_PROFILE_INTERNAL_TRACE();

    component.internalDisplayChanged();
}

void SDL2ComponentNative::handleUserTriedToCloseWindow()
{
    YUP_PROFILE_INTERNAL_TRACE();

    component.internalUserTriedToCloseWindow();
}

//==============================================================================

void SDL2ComponentNative::updateComponentUnderMouse (const MouseEvent& event)
{
    Component* child = component.findComponentAt (event.getPosition());
    if (child != nullptr)
    {
        if (lastComponentUnderMouse == nullptr)
        {
            child->internalMouseEnter (event.withRelativePositionTo (child));
        }
        else if (lastComponentUnderMouse != child)
        {
            lastComponentUnderMouse->internalMouseExit (event.withRelativePositionTo (lastComponentUnderMouse));
            child->internalMouseEnter (event.withRelativePositionTo (child));
        }
    }
    else
    {
        if (lastComponentUnderMouse != nullptr)
            lastComponentUnderMouse->internalMouseExit (event.withRelativePositionTo (lastComponentUnderMouse));
    }

    lastComponentUnderMouse = child;
}

//==============================================================================

void SDL2ComponentNative::handleWindowEvent (const SDL_WindowEvent& windowEvent)
{
    YUP_PROFILE_INTERNAL_TRACE();

    switch (windowEvent.event)
    {
        case SDL_WINDOWEVENT_CLOSE:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_CLOSE");
            component.internalUserTriedToCloseWindow();
            break;

        case SDL_WINDOWEVENT_RESIZED:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_RESIZED " << windowEvent.data1 << " " << windowEvent.data2);
            break;

        case SDL_WINDOWEVENT_SIZE_CHANGED:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_SIZE_CHANGED " << windowEvent.data1 << " " << windowEvent.data2);
            handleResized (windowEvent.data1, windowEvent.data2);
            break;

        case SDL_WINDOWEVENT_MOVED:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_MOVED " << windowEvent.data1 << " " << windowEvent.data2);
            handleMoved (windowEvent.data1, windowEvent.data2);
            break;

        case SDL_WINDOWEVENT_ENTER:
        {
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_ENTER");
            int x = 0, y = 0;
            SDL_GetMouseState (&x, &y);
            handleMouseEnter ({ x, y });
            break;
        }

        case SDL_WINDOWEVENT_LEAVE:
        {
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_LEAVE");
            int x = 0, y = 0;
            SDL_GetMouseState (&x, &y);
            handleMouseLeave ({ x, y });
            break;
        }

        case SDL_WINDOWEVENT_SHOWN:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_SHOWN");
            // repaint();
            break;

        case SDL_WINDOWEVENT_HIDDEN:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_HIDDEN");
            break;

        case SDL_WINDOWEVENT_MINIMIZED:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_MINIMIZED");
            handleMinimized();
            break;

        case SDL_WINDOWEVENT_MAXIMIZED:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_MAXIMIZED");
            handleMaximized();
            break;

        case SDL_WINDOWEVENT_RESTORED:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_RESTORED");
            handleRestored();
            break;

        case SDL_WINDOWEVENT_EXPOSED:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_EXPOSED");
            repaint();
            break;

        case SDL_WINDOWEVENT_FOCUS_GAINED:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_FOCUS_GAINED");
            handleFocusChanged (true);
            break;

        case SDL_WINDOWEVENT_FOCUS_LOST:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_FOCUS_LOST");
            handleFocusChanged (false);
            break;

        case SDL_WINDOWEVENT_TAKE_FOCUS:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_TAKE_FOCUS");
            break;

        case SDL_WINDOWEVENT_DISPLAY_CHANGED:
            YUP_WINDOWING_LOG ("SDL_WINDOWEVENT_DISPLAY_CHANGED");
            handleContentScaleChanged();
            break;
    }
}

//==============================================================================

void SDL2ComponentNative::handleEvent (SDL_Event* event)
{
    YUP_PROFILE_INTERNAL_TRACE();

    switch (event->type)
    {
        case SDL_WINDOWEVENT:
        {
            if (event->window.windowID == SDL_GetWindowID (window))
                handleWindowEvent (event->window);

            break;
        }

        case SDL_RENDER_TARGETS_RESET:
        {
            YUP_WINDOWING_LOG ("SDL_RENDER_TARGETS_RESET");
            break;
        }

        case SDL_RENDER_DEVICE_RESET:
        {
            YUP_WINDOWING_LOG ("SDL_RENDER_DEVICE_RESET");
            break;
        }

        case SDL_MOUSEMOTION:
        {
            //YUP_WINDOWING_LOG ("SDL_MOUSEMOTION " << event->motion.x << " " << event->motion.y);

            auto cursorPosition = Point<float> { static_cast<float> (event->motion.x), static_cast<float> (event->motion.y) };

            if (event->window.windowID == SDL_GetWindowID (window))
                handleMouseMoveOrDrag (cursorPosition);

            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        {
            YUP_WINDOWING_LOG ("SDL_MOUSEBUTTONDOWN " << event->button.x << " " << event->button.y);

            auto cursorPosition = Point<float> { static_cast<float> (event->button.x), static_cast<float> (event->button.y) };

            if (event->button.windowID == SDL_GetWindowID (window))
                handleMouseDown (cursorPosition, toMouseButton (event->button.button), KeyModifiers (SDL_GetModState()));
            else
                ; // TODO - when opening a window in mouse down, mouse up is sent to the other window

            break;
        }

        case SDL_MOUSEBUTTONUP:
        {
            YUP_WINDOWING_LOG ("SDL_MOUSEBUTTONUP " << event->button.x << " " << event->button.y);

            auto cursorPosition = Point<float> { static_cast<float> (event->button.x), static_cast<float> (event->button.y) };

            if (event->button.windowID == SDL_GetWindowID (window))
                handleMouseUp (cursorPosition, toMouseButton (event->button.button), KeyModifiers (SDL_GetModState()));
            else
                ; // TODO - when opening a window in mouse down, mouse up is sent to the other window

            break;
        }

        case SDL_MOUSEWHEEL:
        {
            YUP_WINDOWING_LOG ("SDL_MOUSEWHEEL " << event->wheel.x << " " << event->wheel.y);

            auto cursorPosition = getCursorPosition();

            if (event->wheel.windowID == SDL_GetWindowID (window))
                handleMouseWheel (cursorPosition, { static_cast<float> (event->wheel.x), static_cast<float> (event->wheel.y) });

            break;
        }

        case SDL_KEYDOWN:
        {
            auto cursorPosition = getCursorPosition();
            auto modifiers = toKeyModifiers (event->key.keysym.mod);

            if (event->key.windowID == SDL_GetWindowID (window))
                handleKeyDown (toKeyPress (event->key.keysym.sym, event->key.keysym.scancode, modifiers), cursorPosition);

            break;
        }

        case SDL_KEYUP:
        {
            auto cursorPosition = getCursorPosition();
            auto modifiers = toKeyModifiers (event->key.keysym.mod);

            if (event->key.windowID == SDL_GetWindowID (window))
                handleKeyUp (toKeyPress (event->key.keysym.sym, event->key.keysym.scancode, modifiers), cursorPosition);

            break;
        }

        case SDL_TEXTINPUT:
        {
            YUP_WINDOWING_LOG ("SDL_TEXTINPUT");

            // auto cursorPosition = getCursorPosition();
            // auto modifiers = toKeyModifiers (getKeyModifiers());

            if (event->text.windowID == SDL_GetWindowID (window))
                handleTextInput (String::fromUTF8 (event->text.text));

            break;
        }

        case SDL_TEXTEDITING:
        {
            YUP_WINDOWING_LOG ("SDL_TEXTEDITING");

            // auto cursorPosition = getCursorPosition();
            // auto modifiers = toKeyModifiers (getKeyModifiers());

            //if (event->text.windowID == SDL_GetWindowID (window))
            //    handleTextInput (String::fromUTF8 (event->text.text));

            break;
        }

        default:
            break;
    }
}

//==============================================================================

int SDL2ComponentNative::eventDispatcher (void* userdata, SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_QUIT:
        {
            YUP_WINDOWING_LOG ("SDL_QUIT");
            break;
        }

        default:
        {
            if (auto nativeComponent = Desktop::getInstance()->getNativeComponent (userdata))
                dynamic_cast<SDL2ComponentNative*> (nativeComponent.get())->handleEvent (event);

            break;
        }
    }

    return 0;
}

//==============================================================================

ComponentNative::Ptr ComponentNative::createFor (Component& component,
                                                 const Options& options,
                                                 void* parent)
{
    return ComponentNative::Ptr (ReferenceCountedObjectAdopt, new SDL2ComponentNative (component, options, parent));
}

//==============================================================================

namespace
{

int displayEventDispatcher (void* userdata, SDL_Event* event)
{
    auto desktop = static_cast<Desktop*> (userdata);

    if (event->type == SDL_DISPLAYEVENT)
    {
        switch (event->display.event)
        {
            case SDL_DISPLAYEVENT_CONNECTED:
                desktop->handleScreenConnected (event->display.display);
                break;

            case SDL_DISPLAYEVENT_DISCONNECTED:
                desktop->handleScreenDisconnected (event->display.display);
                break;

            case SDL_DISPLAYEVENT_ORIENTATION:
                desktop->handleScreenOrientationChanged (event->display.display);
                break;

#if ! YUP_EMSCRIPTEN
            case SDL_DISPLAYEVENT_MOVED:
                desktop->handleScreenMoved (event->display.display);
                break;
#endif

            default:
                break;
        }

        return 0;
    }

    switch (event->type)
    {
        case SDL_MOUSEMOTION:
        {
            int x = 0, y = 0;
            SDL_GetGlobalMouseState (&x, &y);
            auto cursorPosition = Point<float> { static_cast<float> (x), static_cast<float> (y) };
            auto keyModifiers = toKeyModifiers (SDL_GetModState());

            MouseEvent mouseEvent (
                static_cast<MouseEvent::Buttons> (event->motion.state),
                keyModifiers,
                cursorPosition);

            // Call drag handler if any mouse buttons are pressed, otherwise call move handler
            if (event->motion.state != 0)
                desktop->handleGlobalMouseDrag (mouseEvent);
            else
                desktop->handleGlobalMouseMove (mouseEvent);

            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        {
            int x = 0, y = 0;
            SDL_GetGlobalMouseState (&x, &y);
            auto cursorPosition = Point<float> { static_cast<float> (x), static_cast<float> (y) };
            auto button = toMouseButton (event->button.button);
            auto keyModifiers = toKeyModifiers (SDL_GetModState());

            MouseEvent mouseEvent (
                button,
                keyModifiers,
                cursorPosition);

            desktop->handleGlobalMouseDown (mouseEvent);
            break;
        }

        case SDL_MOUSEBUTTONUP:
        {
            int x = 0, y = 0;
            SDL_GetGlobalMouseState (&x, &y);
            auto cursorPosition = Point<float> { static_cast<float> (x), static_cast<float> (y) };
            auto button = toMouseButton (event->button.button);
            auto keyModifiers = toKeyModifiers (SDL_GetModState());

            MouseEvent mouseEvent (
                button,
                keyModifiers,
                cursorPosition);

            desktop->handleGlobalMouseUp (mouseEvent);
            break;
        }

        case SDL_MOUSEWHEEL:
        {
            int x = 0, y = 0;
            SDL_GetGlobalMouseState (&x, &y);
            auto cursorPosition = Point<float> { static_cast<float> (x), static_cast<float> (y) };
            auto keyModifiers = toKeyModifiers (SDL_GetModState());
            auto mouseWheelData = MouseWheelData { static_cast<float> (event->wheel.x), static_cast<float> (event->wheel.y) };

            MouseEvent mouseEvent (
                MouseEvent::noButtons,
                keyModifiers,
                cursorPosition);

            desktop->handleGlobalMouseWheel (mouseEvent, mouseWheelData);
            break;
        }

        default:
            break;
    }

    return 0;
}

} // namespace

//==============================================================================

void Desktop::updateScreens()
{
    const int numScreens = SDL_GetNumVideoDisplays();
    for (int i = 0; i < numScreens; ++i)
    {
        SDL_Rect bounds;
        if (SDL_GetDisplayBounds (i, &bounds) != 0)
            continue;

        auto screen = std::make_unique<Screen>();
        screen->name = String::fromUTF8 (SDL_GetDisplayName (i));
        screen->isPrimary = (i == 0);
        screen->virtualPosition = Point<int> (bounds.x, bounds.y);
        screen->workArea = Rectangle<int> (bounds.x, bounds.y, bounds.w, bounds.h);

        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI (i, &ddpi, &hdpi, &vdpi) == 0)
        {
            screen->physicalSizeMillimeters = Size<int> (
                static_cast<int> (bounds.w * 25.4f / hdpi),
                static_cast<int> (bounds.h * 25.4f / vdpi));
        }

        screen->contentScaleX = hdpi / 96.0f; // Assuming 96 DPI as standard
        screen->contentScaleY = vdpi / 96.0f;

        screens.add (screen.release());
    }
}

void Desktop::setMouseCursor (const MouseCursor& cursorToSet)
{
    static const auto cursors = []
    {
        return std::unordered_map<MouseCursor::Type, SDL_Cursor*> {
            { MouseCursor::Default, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_ARROW) },
            { MouseCursor::Text, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_IBEAM) },
            { MouseCursor::Wait, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_WAIT) },
            { MouseCursor::WaitArrow, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_WAITARROW) },
            { MouseCursor::Hand, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_HAND) },
            { MouseCursor::Crosshair, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_CROSSHAIR) },
            { MouseCursor::Crossbones, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_NO) },
            { MouseCursor::ResizeLeftRight, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_SIZEWE) },
            { MouseCursor::ResizeUpDown, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_SIZENS) },
            { MouseCursor::ResizeTopLeftRightBottom, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_SIZENWSE) },
            { MouseCursor::ResizeBottomLeftRightTop, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_SIZENESW) },
            { MouseCursor::ResizeAll, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_ARROW) }
        };
    }();

    currentMouseCursor = cursorToSet;

    if (cursorToSet.getType() == MouseCursor::None)
    {
        SDL_ShowCursor (SDL_DISABLE);
    }
    else
    {
        auto it = cursors.find (cursorToSet.getType());
        if (it != cursors.end())
            SDL_SetCursor (it->second);

        SDL_ShowCursor (SDL_ENABLE);
    }
}

Point<float> Desktop::getCurrentMouseLocation() const
{
    int x = 0, y = 0;

    SDL_GetGlobalMouseState (&x, &y);

    return { static_cast<float> (x), static_cast<float> (y) };
}

void Desktop::setCurrentMouseLocation (const Point<float>& location)
{
    SDL_WarpMouseGlobal (static_cast<int> (location.getX()), static_cast<int> (location.getY()));
}

//==============================================================================

void initialiseYup_Windowing()
{
    // Do not install signal handlers
    SDL_SetHint (SDL_HINT_NO_SIGNAL_HANDLERS, "1");

    // Initialise SDL
    SDL_SetMainReady();
    if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        YUP_DBG ("Error initialising SDL: " << SDL_GetError());

        jassertfalse;
        YUPApplicationBase::quit();

        return;
    }

    // Update available displays
    Desktop::getInstance()->updateScreens();
    SDL_AddEventWatch (displayEventDispatcher, Desktop::getInstance());

    // Set the default theme now in all platforms except ios
#if ! YUP_IOS
    ApplicationTheme::setGlobalTheme (createThemeVersion1());
#endif

    // Inject the event loop
    MessageManager::getInstance()->registerEventLoopCallback ([]
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (EventLoop);

        constexpr double timeoutInterval = 1.0 / 60.0; // TODO
        auto timeoutDetector = TimeoutDetector (timeoutInterval);

        SDL_Event event;
        while (SDL_PollEvent (&event))
        {
            if (MessageManager::getInstance()->hasStopMessageBeenSent())
                return;

            if (timeoutDetector.hasTimedOut())
                break;
        }

        if (! timeoutDetector.hasTimedOut())
            Thread::sleep (1);
    });

    // Set the default theme on ios
#if YUP_IOS
    {
        const MessageManagerLock mmLock;
        ApplicationTheme::setGlobalTheme (createThemeVersion1());
    }
#endif

    SDL2ComponentNative::isInitialised.test_and_set();
}

void shutdownYup_Windowing()
{
    SDL2ComponentNative::isInitialised.clear();

    // Shutdown desktop
    SDL_DelEventWatch (displayEventDispatcher, Desktop::getInstance());
    Desktop::getInstance()->deleteInstance();

    // Unregister theme
    {
        const MessageManagerLock mmLock;
        ApplicationTheme::setGlobalTheme (nullptr);
    }

    // Unregister event loop
    MessageManager::getInstance()->registerEventLoopCallback (nullptr);

    // Quit SDL
    SDL_Quit();
}

} // namespace yup
