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

static constexpr uint32 sdlDefaultSubsystems = SDL_INIT_VIDEO | SDL_INIT_EVENTS;

static String getSDLVersionString (int version)
{
    return String (SDL_VERSIONNUM_MAJOR (version)) + "."
         + String (SDL_VERSIONNUM_MINOR (version)) + "."
         + String (SDL_VERSIONNUM_MICRO (version));
}

//==============================================================================

std::atomic_flag SDL2ComponentNative::isInitialised = ATOMIC_FLAG_INIT;
int SDL2ComponentNative::mouseCaptureRequestCount = 0;
uint32_t SDL2ComponentNative::lastCapturedMouseButtonState = 0;
bool SDL2ComponentNative::popupDismissalCheckPending = false;

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
    , shouldCaptureMouse (options.flags.test (captureMouse))
{
    incReferenceCount();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: constructing native component: component=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (&component))) << ", parent=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (parent))) << ", bounds=" << component.getBounds().toString() << ", visible=" << String (component.isVisible() ? "true" : "false") << ", renderContinuous=" << String (shouldRenderContinuous ? "true" : "false") << ", updateOnlyWhenFocused=" << String (updateOnlyWhenFocused ? "true" : "false") << ", desiredFrameRate=" << String (desiredFrameRate));

    Desktop::getInstance()->registerNativeComponent (this);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: registered native component");

    SDL_AddEventWatch (eventDispatcher, this);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: registered window event watch");

    // Setup window hints and get flags
    windowFlags = setContextWindowHints (currentGraphicsApi);

    if (options.flags.test (resizableWindow))
        windowFlags |= SDL_WINDOW_RESIZABLE;

    if (! component.isVisible())
        windowFlags |= SDL_WINDOW_HIDDEN;

    if (options.flags.test (allowHighDensityDisplay))
        windowFlags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

    if (options.flags.test (temporaryWindow))
        windowFlags |= SDL_WINDOW_ALWAYS_ON_TOP; // SDL_WINDOW_POPUP_MENU | SDL_WINDOW_UTILITY

    if (! options.flags.test (decoratedWindow))
        windowFlags |= SDL_WINDOW_BORDERLESS;

    SDL_SetHint (SDL_HINT_ORIENTATIONS, "Portrait PortraitUpsideDown LandscapeLeft LandscapeRight");
    SDL_SetHint (SDL_HINT_MOUSE_DOUBLE_CLICK_TIME, String (doubleClickTime.inMilliseconds()).toRawUTF8());
    SDL_SetHint (SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    // Create the window, renderer and parent it
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: creating window: title=" << component.getTitle() << ", flags=" << String::toHexString (static_cast<int> (windowFlags)) << ", parent=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (parent))));

#if YUP_WINDOWS
    if (parent != nullptr)
    {
        // Register a plain window class to avoid triggering SDL's WndProc during creation
        // (SDL will subclass it afterwards when wrapping the existing HWND).
        static const wchar_t childWindowClass[] = L"YUPChildWindow";
        static bool childWindowClassRegistered = false;

        if (! childWindowClassRegistered)
        {
            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof (WNDCLASSEXW);
            wc.lpfnWndProc = DefWindowProcW;
            wc.hInstance = GetModuleHandleW (nullptr);
            wc.hCursor = LoadCursorW (nullptr, IDC_ARROW);
            wc.lpszClassName = childWindowClass;
            childWindowClassRegistered = RegisterClassExW (&wc) != 0;
        }

        DWORD style = WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

        if (options.flags.test (decoratedWindow))
            style |= WS_CAPTION;

        if (component.isVisible())
            style |= WS_VISIBLE;

        HWND childHwnd = CreateWindowExW (0,
                                          childWindowClass,
                                          component.getTitle().toWideCharPointer(),
                                          style,
                                          0,
                                          0,
                                          1,
                                          1,
                                          reinterpret_cast<HWND> (parent),
                                          nullptr,
                                          GetModuleHandleW (nullptr),
                                          nullptr);

        if (childHwnd == nullptr)
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unable to create child window");
            return;
        }

        SDL_PropertiesID windowProps = SDL_CreateProperties();
        SDL_SetPointerProperty (windowProps, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, reinterpret_cast<void*> (childHwnd));
        window = SDL_CreateWindowWithProperties (windowProps);
        SDL_DestroyProperties (windowProps);

        if (window == nullptr)
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unable to wrap child window with SDL: " << SDL_GetError());
            DestroyWindow (childHwnd);
            return;
        }
    }
    else
#endif
    {
        window = SDL_CreateWindow (component.getTitle().toRawUTF8(), 1, 1, windowFlags);
        if (window == nullptr)
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unable to create heavyweight window: " << SDL_GetError());
            return; // TODO - raise something ?
        }
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: created window: id=" << static_cast<int64> (SDL_GetWindowID (window)) << ", window=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (window))));

#if ! YUP_WINDOWS
    if (parent != nullptr)
    {
        setNativeParent (parent, window);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: set native parent");
    }
#endif

    if (currentGraphicsApi == GraphicsContext::OpenGL || currentGraphicsApi == GraphicsContext::OpenGLES)
    {
        windowContext = SDL_GL_CreateContext (window);
        if (windowContext == nullptr)
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unable to create GL context: " << SDL_GetError());
            return; // TODO - raise something ?
        }

        SDL_GL_MakeCurrent (window, windowContext);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: created GL context");
    }

    // Create the rendering context
    GraphicsContext::Options graphicsOptions;
    graphicsOptions.retinaDisplay = options.flags.test (allowHighDensityDisplay);
    graphicsOptions.loaderFunction = [] (const char* name) -> void*
    {
        return reinterpret_cast<void*> (SDL_GL_GetProcAddress (name));
    };
    context = GraphicsContext::createContext (currentGraphicsApi, graphicsOptions);
    if (context == nullptr)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unable to create YUP GraphicsContext");
        return; // TODO - raise something ?
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: created YUP GraphicsContext");

    // Resize after callbacks are in place
    setBounds (
        { screenBounds.getX(),
          screenBounds.getY(),
          jmax (1, screenBounds.getWidth()),
          jmax (1, screenBounds.getHeight()) });

    // Check mouse capture
    if (shouldCaptureMouse && isVisible())
        updateMouseCapture (true);

    // Start the rendering
    startRendering();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: native component constructed: window=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (window))) << ", context=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (context.get()))) << ", rendering=" << String (isRendering() ? "true" : "false"));
}

SDL2ComponentNative::~SDL2ComponentNative()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: destroying native component: window=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (window))) << ", context=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (context.get()))) << ", rendering=" << String (isRendering() ? "true" : "false"));

    updateMouseCapture (false);

    // Stop the rendering first, before touching any SDL resources
    stopRendering();

    // Cancel any pending async update that may have been scheduled by the render thread
    cancelPendingUpdate();

    // Remove event watch
    SDL_RemoveEventWatch (eventDispatcher, this);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unregistered window event watch");

    // Unregister this component from the desktop
    Desktop::getInstance()->unregisterNativeComponent (this);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unregistered native component");

    // Destroy graphics resources before the SDL window
    renderer.reset();
    context.reset();

    // Destroy the window
    if (window != nullptr)
    {
        SDL_DestroyWindow (window);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: destroyed window");
        window = nullptr;
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: native component destroyed");
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
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setVisible skipped: window is null, visible=" << String (shouldBeVisible ? "true" : "false"));
        return;
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setVisible " << String (shouldBeVisible ? "true" : "false") << ", currentFlags=" << String::toHexString (static_cast<int> (SDL_GetWindowFlags (window))));

    if (shouldBeVisible)
    {
        SDL_ShowWindow (window);
        repaint();
        updateMouseCapture (true);
    }
    else
    {
        updateMouseCapture (false);
        SDL_HideWindow (window);
    }
}

bool SDL2ComponentNative::isVisible() const
{
    return window != nullptr && (SDL_GetWindowFlags (window) & SDL_WINDOW_HIDDEN) == 0;
}

//==============================================================================

void SDL2ComponentNative::toFront()
{
    if (window != nullptr && isVisible())
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: raise window");
        SDL_RaiseWindow (window);
    }
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
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setSize skipped: window is null, size=" << newSize.toString());
        return;
    }

    screenBounds = screenBounds.withSize (newSize);

    if (auto currentSize = getSize(); currentSize != newSize)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setSize " << currentSize.toString() << " -> " << newSize.toString());
        SDL_SetWindowSize (window, jmax (1, newSize.getWidth()), jmax (1, newSize.getHeight()));
    }
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
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setPosition skipped: window is null, position=" << newPosition.toString());
        return;
    }

    screenBounds = screenBounds.withPosition (newPosition);

    if (auto currentPosition = getPosition(); currentPosition != newPosition)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setPosition " << currentPosition.toString() << " -> " << newPosition.toString());
        SDL_SetWindowPosition (window, newPosition.getX(), newPosition.getY());
    }
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
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setBounds skipped: window is null, bounds=" << newBounds.toString());
        return;
    }

    auto adjustedBounds = newBounds;
    int leftMargin = 0, topMargin = 0, rightMargin = 0, bottomMargin = 0;

#if YUP_EMSCRIPTEN && RIVE_WEBGL
    //const double devicePixelRatio = emscripten_get_device_pixel_ratio();
    //SDL_SetWindowSize (window,
    //                   jmax (0, (int) (newBounds.getWidth() * devicePixelRatio)),
    //                   jmax (0, (int) (newBounds.getHeight() * devicePixelRatio)));

    SDL_SetWindowSize (window,
                       jmax (0, newBounds.getWidth()),
                       jmax (0, newBounds.getHeight()));

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
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setBounds size " << currentSize.toString() << " -> " << adjustedBounds.getSize().toString() << ", requested=" << newBounds.toString() << ", margins=" << leftMargin << "," << topMargin << "," << rightMargin << "," << bottomMargin);
        SDL_SetWindowSize (window, adjustedBounds.getWidth(), adjustedBounds.getHeight());
    }

#endif

    if (auto currentPosition = getPosition(); currentPosition != adjustedBounds.getPosition())
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setBounds position " << currentPosition.toString() << " -> " << adjustedBounds.getPosition().toString() << ", requested=" << newBounds.toString());
        SDL_SetWindowPosition (window, adjustedBounds.getX(), adjustedBounds.getY());
    }

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
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setFullScreen skipped: window is null, fullScreen=" << String (shouldBeFullScreen ? "true" : "false"));
        return;
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: setFullScreen " << String (shouldBeFullScreen ? "true" : "false") << ", current=" << String (isFullScreen() ? "true" : "false"));

    if (shouldBeFullScreen)
    {
#if YUP_EMSCRIPTEN
        emscripten_request_fullscreen ("#canvas", false);
#else
        lastScreenBounds = screenBounds;

        SDL_SetWindowFullscreen (window, true); // SDL_SetWindowFullscreenMode
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
    if (window != nullptr)
        return SDL_GetWindowOpacity (window);

    return 1.0f;
}

//==============================================================================

void SDL2ComponentNative::setFocusedComponent (Component* comp)
{
    const auto focusNativeWindowIfNeeded = [&]
    {
        if (window != nullptr && isVisible() && lastComponentFocused != nullptr)
        {
            SDL_RaiseWindow (window);

            focusNativeWindow (getNativeHandle());
        }
    };

    if (lastComponentFocused == comp)
    {
        focusNativeWindowIfNeeded();
        return;
    }

    auto compBailOut = Component::BailOutChecker (comp);

    // Check if we need to stop text input for the previously focused component
    Component* previousComponent = lastComponentFocused.get();
    WeakReference<Component> previousComponentWeak (previousComponent);
    bool previousWantsTextInput = previousComponent != nullptr && dynamic_cast<TextInputTarget*> (previousComponent) != nullptr;

    if (lastComponentFocused != nullptr)
    {
        auto focusBailOut = Component::BailOutChecker (lastComponentFocused.get());

        lastComponentFocused->focusLost();

        if (! focusBailOut.shouldBailOut())
            lastComponentFocused->repaint();
    }

    if (compBailOut.shouldBailOut())
    {
        lastComponentFocused = nullptr;

        if (previousWantsTextInput && previousComponentWeak.get() != nullptr)
            stopTextInput (*previousComponentWeak.get());

        return;
    }

    lastComponentFocused = comp;

    // Check if the newly focused component needs text input
    Component* newComponent = lastComponentFocused.get();
    bool newWantsTextInput = newComponent != nullptr && dynamic_cast<TextInputTarget*> (newComponent) != nullptr;

    // Stop text input if the previous component had it but the new one doesn't
    if (previousWantsTextInput && ! newWantsTextInput && previousComponentWeak.get() != nullptr)
        stopTextInput (*previousComponentWeak.get());

    if (lastComponentFocused != nullptr)
    {
        auto focusBailOut = Component::BailOutChecker (lastComponentFocused.get());

        lastComponentFocused->focusGained();

        if (! focusBailOut.shouldBailOut())
            lastComponentFocused->repaint();
    }

    focusNativeWindowIfNeeded();
}

Component* SDL2ComponentNative::getFocusedComponent() const
{
    return hasNativeKeyboardFocus() ? lastComponentFocused.get() : nullptr;
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
    float x = 0, y = 0;

    SDL_GetMouseState (&x, &y);

    return { x, y };
}

//==============================================================================

rive::Factory* SDL2ComponentNative::getFactory()
{
    return context ? context->factory() : nullptr;
}

//==============================================================================

GraphicsContext* SDL2ComponentNative::getGraphicsContext()
{
    return context.get();
}

//==============================================================================

void* SDL2ComponentNative::getNativeHandle() const
{
    return getNativeWindowHandle (window);
}

//==============================================================================

void SDL2ComponentNative::startTextInput (Component& component)
{
    if (window == nullptr)
        return;

    auto* target = dynamic_cast<TextInputTarget*> (std::addressof (component));
    if (target == nullptr)
        return;

    if (currentTextInputComponent != nullptr && currentTextInputComponent != std::addressof (component))
        SDL_StopTextInput (window);

    currentTextInputComponent = std::addressof (component);

    SDL_StartTextInput (window);

    updateTextInputRect (component);
}

void SDL2ComponentNative::updateTextInputRect (Component& component)
{
    if (window == nullptr || currentTextInputComponent != std::addressof (component))
        return;

    auto* target = dynamic_cast<TextInputTarget*> (std::addressof (component));
    if (target == nullptr)
        return;

    SDL_Rect sdlRect;
    auto textRect = target->getTextInputRect();
    sdlRect.x = static_cast<int> (textRect.getX());
    sdlRect.y = static_cast<int> (textRect.getY());
    sdlRect.w = static_cast<int> (textRect.getWidth());
    sdlRect.h = static_cast<int> (textRect.getHeight());
    SDL_SetTextInputArea (window, &sdlRect, 0);
}

void SDL2ComponentNative::stopTextInput (Component& component)
{
    if (window == nullptr)
        return;

    if (currentTextInputComponent == std::addressof (component))
    {
        currentTextInputComponent = nullptr;

        SDL_StopTextInput (window);
    }
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
    if (window != nullptr) // WIN ONLY: currentMouseButtons != MouseEvent::noButtons
    {
        int windowX = 0, windowY = 0;
        SDL_GetWindowPosition (window, &windowX, &windowY);

        float mouseX = 0.0f, mouseY = 0.0f;
        SDL_GetGlobalMouseState (&mouseX, &mouseY);

        const auto cursorPosition = Point<float> { mouseX - static_cast<float> (windowX),
                                                   mouseY - static_cast<float> (windowY) };

        if (lastMouseMovePosition != cursorPosition)
            handleMouseMoveOrDrag (cursorPosition);
    }

    pollCapturedMouseState();

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

    if (contentWidth == 0 || contentHeight == 0 || ! isVisible())
        return;

    if (currentContentWidth != contentWidth || currentContentHeight != contentHeight)
    {
        YUP_PROFILE_NAMED_INTERNAL_TRACE (ResizeRenderer);

        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: resize render target " << currentContentWidth << "x" << currentContentHeight << " -> " << contentWidth << "x" << contentHeight << ", dpiScale=" << getScaleDpi());

        currentContentWidth = contentWidth;
        currentContentHeight = contentHeight;

        context->onSizeChanged (getNativeHandle(), contentWidth, contentHeight, 0);
        renderer = context->makeRenderer (contentWidth, contentHeight);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: renderer " << String (renderer != nullptr ? "created" : "creation failed"));

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

        {
            const auto repaintComponents = [&]
            {
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
            };

            if (PaintProfiler::hasRegisteredComponents() && PaintProfiler::getInstance().isEnabled())
            {
                PaintProfiler::getInstance().beginFrame();
                const auto endFrameGuard = ErasedScopeGuard ([&]
                {
                    PaintProfiler::getInstance().endFrame();
                });

                repaintComponents();
            }
            else
            {
                repaintComponents();
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
    if (window != nullptr && (currentGraphicsApi == GraphicsContext::OpenGL || currentGraphicsApi == GraphicsContext::OpenGLES))
        SDL_GL_SwapWindow (window);

    // Clear repainted areas
    currentRepaintAreas.clearQuick();
}

//==============================================================================

void SDL2ComponentNative::startRendering()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: startRendering requested: timerDriven=" << String (renderDrivenByTimer ? "true" : "false") << ", alreadyRendering=" << String (isRendering() ? "true" : "false") << ", desiredFrameRate=" << String (desiredFrameRate));

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

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: startRendering completed: rendering=" << String (isRendering() ? "true" : "false"));
}

void SDL2ComponentNative::stopRendering()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: stopRendering requested: rendering=" << String (isRendering() ? "true" : "false"));

    if constexpr (renderDrivenByTimer)
    {
        if (isTimerRunning())
        {
            stopTimer();
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: stopped render timer");
        }
    }
    else
    {
        if (isThreadRunning())
        {
            signalThreadShouldExit();
            notify();
            renderEvent.signal();
            stopThread (-1);
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: stopped render thread");
        }
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: stopRendering completed: rendering=" << String (isRendering() ? "true" : "false"));
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
    currentMouseButtons = static_cast<MouseEvent::Buttons> (toMouseButtons (SDL_GetMouseState (nullptr, nullptr)) | button);
    currentKeyModifiers = modifiers;

    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (position);

    if (currentMouseButtons == button)
        lastComponentClicked = findComponentForMouseEvent (position);

    if (lastComponentClicked != nullptr)
    {
        const auto currentMouseDownTime = yup::Time::getCurrentTime();

        event = event.withSourceComponent (lastComponentClicked);

        lastComponentClicked->internalMouseDown (event.withRelativePositionTo (lastComponentClicked));

        lastMouseDownPosition = position;
        lastMouseDownTime = currentMouseDownTime;

#if YUP_WINDOWS
        SetCapture (reinterpret_cast<HWND> (getNativeHandle()));
#endif
    }

    lastMouseMovePosition = position;
}

void SDL2ComponentNative::handleMouseUp (const Point<float>& position, MouseEvent::Buttons button, KeyModifiers modifiers)
{
    currentMouseButtons = static_cast<MouseEvent::Buttons> (toMouseButtons (SDL_GetMouseState (nullptr, nullptr)) & ~button);
    currentKeyModifiers = modifiers;

    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (position);

    if (lastMouseDownPosition)
        event = event.withLastMouseDownPosition (*lastMouseDownPosition);

    if (lastMouseDownTime)
        event = event.withLastMouseDownTime (*lastMouseDownTime);

    auto nativeBailOut = Component::BailOutChecker (&component);

    if (auto* clickedComponent = lastComponentClicked.get())
    {
        const auto currentMouseDownTime = yup::Time::getCurrentTime();
        auto clickedComponentBailOut = Component::BailOutChecker (clickedComponent);

        event = event.withSourceComponent (clickedComponent);

        if (lastMouseUpTime
            && *lastMouseUpTime > yup::Time()
            && currentMouseDownTime - *lastMouseUpTime < doubleClickTime)
        {
            clickedComponent->internalMouseDoubleClick (event.withRelativePositionTo (clickedComponent));
        }

        if (! clickedComponentBailOut.shouldBailOut())
            clickedComponent->internalMouseUp (event.withRelativePositionTo (clickedComponent));

        lastMouseUpTime = currentMouseDownTime;
    }

    if (nativeBailOut.shouldBailOut())
        return;

    if (currentMouseButtons == MouseEvent::noButtons)
    {
#if YUP_WINDOWS
        ReleaseCapture();
#endif

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
    if (currentMouseButtons != MouseEvent::noButtons)
        return;

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
    if (currentMouseButtons != MouseEvent::noButtons)
        return;

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
    if (! hasNativeKeyboardFocus() || currentTextInputComponent == nullptr)
        return;

    currentTextInputComponent->internalTextInput (textInput);
}

void SDL2ComponentNative::handleItemsDropped (const Point<float>& position, const DragAndDropData& data)
{
    if (Component* target = findComponentForMouseEvent (position))
        target->internalItemsDropped (data, position);
}

//==============================================================================

void SDL2ComponentNative::handleMoved (int xpos, int ypos)
{
    YUP_PROFILE_INTERNAL_TRACE();

    if (internalBoundsChange)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleMoved ignored during internal bounds change: " << xpos << " " << ypos);
        return;
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleMoved " << screenBounds.getX() << " " << screenBounds.getY() << " -> " << xpos << " " << ypos << ", parent=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (parentWindow))));

    if (context == nullptr)
        return;

    if (parentWindow != nullptr)
    {
        auto preventBoundsChange = ScopedValueSetter<bool> (internalBoundsChange, true);

#if YUP_MAC
        auto nativeWindowPos = getNativeWindowPosition (parentWindow);
#else
        auto nativeWindowPos = Rectangle<int> (0, 0, 1, 1);
#endif

        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: parent window position sync after move: " << nativeWindowPos.toString());
        setPosition (nativeWindowPos.getTopLeft());

        xpos = nativeWindowPos.getX();
        ypos = nativeWindowPos.getY();
    }

    component.internalMoved (xpos, ypos);

    screenBounds = screenBounds.withPosition (xpos, ypos);
}

void SDL2ComponentNative::handleResized (int width, int height)
{
    YUP_PROFILE_INTERNAL_TRACE();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleResized " << screenBounds.getWidth() << "x" << screenBounds.getHeight() << " -> " << width << "x" << height << ", parent=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (parentWindow))));

    if (context == nullptr)
        return;

    component.internalResized (width, height);

    screenBounds = screenBounds.withSize (width, height);

    if (parentWindow != nullptr)
    {
        auto preventBoundsChange = ScopedValueSetter<bool> (internalBoundsChange, true);

#if YUP_MAC
        auto nativeWindowPos = getNativeWindowPosition (parentWindow);
#else
        auto nativeWindowPos = Rectangle<int> (0, 0, 1, 1);
#endif

        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: parent window position sync after resize: " << nativeWindowPos.toString());
        setPosition (nativeWindowPos.getTopLeft());
    }

    if (dynamic_cast<PopupMenu*> (&component) == nullptr)
        PopupMenu::dismissAllPopups();

    repaint();
}

void SDL2ComponentNative::handleFocusChanged (bool gotFocus)
{
    YUP_PROFILE_INTERNAL_TRACE();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleFocusChanged " << String (gotFocus ? "true" : "false") << ", rendering=" << String (isRendering() ? "true" : "false"));

    if (gotFocus)
    {
        if (! isRendering())
            startRendering();

        component.internalFocusChanged (true);

        // Re-notify the focused widget so it restarts its caret and text input.
        // This pairs with the focusLost() call in the gotFocus=false branch below.
        if (lastComponentFocused != nullptr && lastComponentFocused.get() != std::addressof (component))
        {
            auto focusBailOut = Component::BailOutChecker (lastComponentFocused.get());

            lastComponentFocused->focusGained();

            if (! focusBailOut.shouldBailOut())
                lastComponentFocused->repaint();
        }
    }
    else
    {
        // Properly notify the focused widget so it stops its caret and text input
        // via relinquishTextInput(), keeping textInputActive in sync with SDL's state.
        if (lastComponentFocused != nullptr && lastComponentFocused.get() != std::addressof (component))
        {
            auto focusBailOut = Component::BailOutChecker (lastComponentFocused.get());

            lastComponentFocused->focusLost();

            if (! focusBailOut.shouldBailOut())
                lastComponentFocused->repaint();
        }
        else if (currentTextInputComponent != nullptr)
        {
            currentTextInputComponent = nullptr;
            SDL_StopTextInput (window);
        }

        component.internalFocusChanged (false);

        lastComponentClicked = nullptr;
        lastMouseDownPosition.reset();
        lastMouseDownTime.reset();

        if (updateOnlyWhenFocused)
        {
            if (isRendering())
                stopRendering();
        }

        triggerPopupDismissalCheck();
    }
}

bool SDL2ComponentNative::hasNativeKeyboardFocus() const
{
    return window != nullptr && (SDL_GetWindowFlags (window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void SDL2ComponentNative::handleMinimized()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleMinimized");
    PopupMenu::dismissAllPopups();

    stopRendering();
}

void SDL2ComponentNative::handleMaximized()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleMaximized");
    repaint();
}

void SDL2ComponentNative::handleRestored()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleRestored");
    repaint();
}

void SDL2ComponentNative::handleExposed()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleExposed");
    repaint();
}

void SDL2ComponentNative::handleContentScaleChanged()
{
    YUP_PROFILE_INTERNAL_TRACE();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleContentScaleChanged dpiScale=" << getScaleDpi());

    component.internalContentScaleChanged (getScaleDpi());

    handleResized (screenBounds.getWidth(), screenBounds.getHeight());
}

void SDL2ComponentNative::handleDisplayChanged()
{
    YUP_PROFILE_INTERNAL_TRACE();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: handleDisplayChanged");

    component.internalDisplayChanged();
}

void SDL2ComponentNative::handleUserTriedToCloseWindow()
{
    YUP_PROFILE_INTERNAL_TRACE();

    component.internalUserTriedToCloseWindow();
}

//==============================================================================

Component* SDL2ComponentNative::findComponentForMouseEvent (const Point<float>& position)
{
    Component* child = component.findComponentAt (position);
    if (child == nullptr)
        return nullptr;

    Component* current = child;
    while (current != nullptr)
    {
        if (current->doesWantSelfMouseEvents())
        {
            Component* parent = current->getParentComponent();
            while (parent != nullptr)
            {
                if (! parent->doesWantChildrenMouseEvents())
                    return parent;

                parent = parent->getParentComponent();
            }

            return current;
        }

        current = current->getParentComponent();
    }

    return nullptr;
}

void SDL2ComponentNative::updateComponentUnderMouse (const MouseEvent& event)
{
    Component* child = findComponentForMouseEvent (event.getPosition());

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

    switch (windowEvent.type)
    {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_CLOSE_REQUESTED");
            component.internalUserTriedToCloseWindow();
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_RESIZED " << windowEvent.data1 << " " << windowEvent.data2);
            break;

        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED " << windowEvent.data1 << " " << windowEvent.data2);
            handleResized (windowEvent.data1, windowEvent.data2);
            break;

        case SDL_EVENT_WINDOW_MOVED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_MOVED " << windowEvent.data1 << " " << windowEvent.data2);
            handleMoved (windowEvent.data1, windowEvent.data2);
            break;

        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_MOUSE_ENTER");
            float x = 0.0f, y = 0.0f;
            SDL_GetMouseState (&x, &y);
            handleMouseEnter ({ x, y });
            break;
        }

        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_MOUSE_LEAVE");
            float x = 0.0f, y = 0.0f;
            SDL_GetMouseState (&x, &y);
            handleMouseLeave ({ x, y });
            break;
        }

        case SDL_EVENT_WINDOW_SHOWN:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_SHOWN");
            if (firstDisplay)
            {
                firstDisplay = false;

                const auto size = getSize();
                handleResized (size.getWidth(), size.getHeight());
            }
            break;

        case SDL_EVENT_WINDOW_HIDDEN:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_HIDDEN");
            break;

        case SDL_EVENT_WINDOW_MINIMIZED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_MINIMIZED");
            handleMinimized();
            break;

        case SDL_EVENT_WINDOW_MAXIMIZED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_MAXIMIZED");
            handleMaximized();
            break;

        case SDL_EVENT_WINDOW_RESTORED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_RESTORED");
            handleRestored();
            break;

        case SDL_EVENT_WINDOW_EXPOSED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_EXPOSED");
            repaint();
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_FOCUS_GAINED");
            handleFocusChanged (true);
            break;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_FOCUS_LOST");
            handleFocusChanged (false);
            break;

        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_DISPLAY_CHANGED");
            handleContentScaleChanged();
            break;

        default:
            break;
    }
}

//==============================================================================

void SDL2ComponentNative::handleEvent (SDL_Event* event)
{
    YUP_PROFILE_INTERNAL_TRACE();

    if (event->type >= SDL_EVENT_WINDOW_FIRST && event->type <= SDL_EVENT_WINDOW_LAST)
    {
        if (event->window.windowID == SDL_GetWindowID (window))
            handleWindowEvent (event->window);

        return;
    }

    switch (event->type)
    {
        case SDL_EVENT_RENDER_TARGETS_RESET:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_RENDER_TARGETS_RESET");
            break;
        }

        case SDL_EVENT_RENDER_DEVICE_RESET:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_RENDER_DEVICE_RESET");
            break;
        }

            /*
        case SDL_EVENT_MOUSE_MOTION:
        {
            //YUP_MODULE_DBG (GUI_WINDOWING, "SDL_MOUSEMOTION " << event->motion.x << " " << event->motion.y);

            auto cursorPosition = Point<float> { static_cast<float> (event->motion.x), static_cast<float> (event->motion.y) };

            if (event->motion.windowID == SDL_GetWindowID (window))
                handleMouseMoveOrDrag (cursorPosition);

            break;
        }
        */

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_MOUSE_BUTTON_DOWN " << event->button.x << " " << event->button.y);

            auto cursorPosition = Point<float> { static_cast<float> (event->button.x), static_cast<float> (event->button.y) };

            if (event->button.windowID == SDL_GetWindowID (window))
                handleMouseDown (cursorPosition, toMouseButton (event->button.button), KeyModifiers (SDL_GetModState()));

            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_MOUSE_BUTTON_UP " << event->button.x << " " << event->button.y);

            auto cursorPosition = Point<float> { static_cast<float> (event->button.x), static_cast<float> (event->button.y) };

            if (event->button.windowID == SDL_GetWindowID (window))
                handleMouseUp (cursorPosition, toMouseButton (event->button.button), KeyModifiers (SDL_GetModState()));

            else if (lastComponentClicked != nullptr)
                handleMouseUp (cursorPosition, toMouseButton (event->button.button), KeyModifiers (SDL_GetModState()));

            break;
        }

        case SDL_EVENT_MOUSE_WHEEL:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_MOUSE_WHEEL " << event->wheel.x << " " << event->wheel.y);

            auto cursorPosition = getCursorPosition();

            if (event->wheel.windowID == SDL_GetWindowID (window))
                handleMouseWheel (cursorPosition, { static_cast<float> (event->wheel.x), static_cast<float> (event->wheel.y) });

            break;
        }

        case SDL_EVENT_KEY_DOWN:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_KEY_DOWN " << (int) (event->key.key) << " " << event->key.scancode);

            auto cursorPosition = getCursorPosition();
            auto modifiers = toKeyModifiers (event->key.mod);

            if (event->key.windowID == SDL_GetWindowID (window))
                handleKeyDown (toKeyPress (event->key.key, event->key.scancode, modifiers), cursorPosition);

            break;
        }

        case SDL_EVENT_KEY_UP:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_KEY_UP " << (int) (event->key.key) << " " << event->key.scancode);

            auto cursorPosition = getCursorPosition();
            auto modifiers = toKeyModifiers (event->key.mod);

            if (event->key.windowID == SDL_GetWindowID (window))
                handleKeyUp (toKeyPress (event->key.key, event->key.scancode, modifiers), cursorPosition);

            break;
        }

        case SDL_EVENT_TEXT_INPUT:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_TEXT_INPUT " << String::fromUTF8 (event->text.text));

            // auto cursorPosition = getCursorPosition();
            // auto modifiers = toKeyModifiers (getKeyModifiers());

            if (event->text.windowID == SDL_GetWindowID (window))
                handleTextInput (String::fromUTF8 (event->text.text));

            break;
        }

        case SDL_EVENT_TEXT_EDITING:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_TEXT_EDITING");

            // auto cursorPosition = getCursorPosition();
            // auto modifiers = toKeyModifiers (getKeyModifiers());

            //if (event->text.windowID == SDL_GetWindowID (window))
            //    handleTextInput (String::fromUTF8 (event->text.text));

            break;
        }

        case SDL_EVENT_DROP_BEGIN:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_DROP_BEGIN");

            if (event->drop.windowID == SDL_GetWindowID (window))
            {
                SDL_RaiseWindow (window);

                pendingDroppedFiles.clear();
                pendingDroppedText.clear();
            }

            break;
        }

        case SDL_EVENT_DROP_FILE:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_DROP_FILE " << String::fromUTF8 (event->drop.data));

            if (event->drop.windowID == SDL_GetWindowID (window))
                pendingDroppedFiles.add (File (String::fromUTF8 (event->drop.data)));

            break;
        }

        case SDL_EVENT_DROP_TEXT:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_DROP_TEXT " << String::fromUTF8 (event->drop.data));

            if (event->drop.windowID == SDL_GetWindowID (window))
                pendingDroppedText += String::fromUTF8 (event->drop.data);

            break;
        }

        case SDL_EVENT_DROP_COMPLETE:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_DROP_COMPLETE");

            if (event->drop.windowID == SDL_GetWindowID (window))
            {
                auto data = DragAndDropData()
                                .withFiles (pendingDroppedFiles)
                                .withText (pendingDroppedText);

                if (! data.isEmpty())
                    handleItemsDropped (getCursorPosition(), data);

                pendingDroppedFiles.clear();
                pendingDroppedText.clear();
            }

            break;
        }

        default:
            break;
    }
}

//==============================================================================

bool SDL2ComponentNative::eventDispatcher (void* userdata, SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_EVENT_QUIT:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_QUIT");
            break;
        }

        default:
        {
            if (auto component = Desktop::getInstance()->getNativeComponent (userdata))
            {
                if (auto nativeComponent = dynamic_cast<SDL2ComponentNative*> (component.get()))
                    nativeComponent->handleEvent (event);
                else
                    YUP_MODULE_DBG (GUI_WINDOWING, "Received event for unknown component");
            }

            break;
        }
    }

    return true;
}

//==============================================================================

void SDL2ComponentNative::triggerPopupDismissalCheck()
{
    if (popupDismissalCheckPending)
        return;

    popupDismissalCheckPending = true;

    if (! MessageManager::callAsync ([]
    {
        dismissPopupsIfNoNativeWindowHasFocus();
    }))
    {
        popupDismissalCheckPending = false;
    }
}

void SDL2ComponentNative::dismissPopupsIfNoNativeWindowHasFocus()
{
    popupDismissalCheckPending = false;

    if (anyNativeWindowHasKeyboardFocus())
        return;

    PopupMenu::dismissAllPopups();
}

bool SDL2ComponentNative::anyNativeWindowHasKeyboardFocus()
{
    auto* desktop = Desktop::getInstanceWithoutCreating();

    if (desktop == nullptr)
        return false;

    for (const auto& [userdata, nativeComponent] : desktop->nativeComponents)
    {
        ignoreUnused (userdata);

        if (auto* sdlNativeComponent = dynamic_cast<SDL2ComponentNative*> (nativeComponent))
        {
            if (sdlNativeComponent->hasNativeKeyboardFocus())
                return true;
        }
    }

    return false;
}

bool SDL2ComponentNative::anyNativeWindowContains (Point<float> screenPosition)
{
    auto* desktop = Desktop::getInstanceWithoutCreating();

    if (desktop == nullptr)
        return false;

    for (const auto& [userdata, nativeComponent] : desktop->nativeComponents)
    {
        ignoreUnused (userdata);

        if (nativeComponent != nullptr
            && nativeComponent->isVisible()
            && nativeComponent->getBounds().to<float>().contains (screenPosition))
        {
            return true;
        }
    }

    return false;
}

//==============================================================================

void SDL2ComponentNative::updateMouseCapture (bool shouldBeActive)
{
    if (! shouldCaptureMouse)
        shouldBeActive = false;

    if (shouldBeActive == mouseCaptureActive)
        return;

    if (shouldBeActive)
    {
        mouseCaptureActive = requestMouseCapture();
        return;
    }

    releaseMouseCapture();
    mouseCaptureActive = false;
}

void SDL2ComponentNative::pollCapturedMouseState()
{
    if (mouseCaptureRequestCount <= 0 || SDL_WasInit (SDL_INIT_VIDEO) == 0)
        return;

    float x = 0.0f;
    float y = 0.0f;
    const auto currentButtons = SDL_GetGlobalMouseState (&x, &y);
    const auto hadButtonsDown = lastCapturedMouseButtonState != 0;
    const auto hasButtonsDown = currentButtons != 0;

    lastCapturedMouseButtonState = currentButtons;

    if (hadButtonsDown || ! hasButtonsDown)
        return;

    if (anyNativeWindowContains ({ x, y }))
        return;

    MessageManager::callAsync ([]
    {
        PopupMenu::dismissAllPopups();
    });
}

bool SDL2ComponentNative::requestMouseCapture()
{
    if (SDL_WasInit (SDL_INIT_VIDEO) == 0)
        return false;

    const bool shouldEnableCapture = mouseCaptureRequestCount == 0;

    if (shouldEnableCapture && ! SDL_CaptureMouse (true))
        return false;

    ++mouseCaptureRequestCount;
    lastCapturedMouseButtonState = SDL_GetGlobalMouseState (nullptr, nullptr);

    if (shouldEnableCapture)
        YUP_MODULE_DBG (GUI_WINDOWING, "Enabled SDL Mouse Capture");

    return true;
}

void SDL2ComponentNative::releaseMouseCapture()
{
    if (mouseCaptureRequestCount <= 0)
        return;

    --mouseCaptureRequestCount;

    if (mouseCaptureRequestCount == 0 && SDL_WasInit (SDL_INIT_VIDEO) != 0)
    {
        SDL_CaptureMouse (false);
        lastCapturedMouseButtonState = 0;

        YUP_MODULE_DBG (GUI_WINDOWING, "Disabled SDL Mouse Capture");
    }
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

bool displayEventDispatcher (void* userdata, SDL_Event* event)
{
    auto desktop = static_cast<Desktop*> (userdata);

    if (event->type >= SDL_EVENT_DISPLAY_FIRST && event->type <= SDL_EVENT_DISPLAY_LAST)
    {
        switch (event->type)
        {
            case SDL_EVENT_DISPLAY_ADDED:
                desktop->handleScreenConnected (static_cast<int> (event->display.displayID));
                break;

            case SDL_EVENT_DISPLAY_REMOVED:
                desktop->handleScreenDisconnected (static_cast<int> (event->display.displayID));
                break;

            case SDL_EVENT_DISPLAY_ORIENTATION:
                desktop->handleScreenOrientationChanged (static_cast<int> (event->display.displayID));
                break;

#if ! YUP_EMSCRIPTEN
            case SDL_EVENT_DISPLAY_MOVED:
                desktop->handleScreenMoved (static_cast<int> (event->display.displayID));
                break;
#endif

            default:
                break;
        }

        return true;
    }

    switch (event->type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        {
            float x = 0.0f, y = 0.0f;
            SDL_GetGlobalMouseState (&x, &y);
            auto cursorPosition = Point<float> { x, y };
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

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            float x = 0.0f, y = 0.0f;
            SDL_GetGlobalMouseState (&x, &y);
            auto cursorPosition = Point<float> { x, y };
            auto button = toMouseButton (event->button.button);
            auto keyModifiers = toKeyModifiers (SDL_GetModState());

            MouseEvent mouseEvent (
                button,
                keyModifiers,
                cursorPosition);

            desktop->handleGlobalMouseDown (mouseEvent);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            float x = 0.0f, y = 0.0f;
            SDL_GetGlobalMouseState (&x, &y);
            auto cursorPosition = Point<float> { x, y };
            auto button = toMouseButton (event->button.button);
            auto keyModifiers = toKeyModifiers (SDL_GetModState());

            MouseEvent mouseEvent (
                button,
                keyModifiers,
                cursorPosition);

            desktop->handleGlobalMouseUp (mouseEvent);
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL:
        {
            float x = 0.0f, y = 0.0f;
            SDL_GetGlobalMouseState (&x, &y);
            auto cursorPosition = Point<float> { x, y };
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

    return true;
}

} // namespace

//==============================================================================

void Desktop::updateScreens()
{
    int numScreens = 0;
    SDL_DisplayID* displays = SDL_GetDisplays (&numScreens);
    if (displays == nullptr)
        return;

    const SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();

    for (int i = 0; i < numScreens; ++i)
    {
        const SDL_DisplayID displayID = displays[i];

        SDL_Rect bounds;
        if (! SDL_GetDisplayBounds (displayID, &bounds))
            continue;

        auto screen = std::make_unique<Screen>();
        screen->name = String::fromUTF8 (SDL_GetDisplayName (displayID));
        screen->isPrimary = (displayID == primaryDisplay);
        screen->virtualPosition = Point<int> (bounds.x, bounds.y);
        screen->workArea = Rectangle<int> (bounds.x, bounds.y, bounds.w, bounds.h);

        const float contentScale = SDL_GetDisplayContentScale (displayID);
        screen->contentScaleX = contentScale > 0.0f ? contentScale : 1.0f;
        screen->contentScaleY = screen->contentScaleX;

        screens.add (screen.release());
    }

    SDL_free (displays);
}

void Desktop::setMouseCursor (const MouseCursor& cursorToSet)
{
    static const auto cursors = []
    {
        return std::unordered_map<MouseCursor::Type, SDL_Cursor*> {
            { MouseCursor::Default, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_DEFAULT) },
            { MouseCursor::Text, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_TEXT) },
            { MouseCursor::Wait, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_WAIT) },
            { MouseCursor::WaitArrow, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_PROGRESS) },
            { MouseCursor::Hand, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_POINTER) },
            { MouseCursor::Crosshair, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_CROSSHAIR) },
            { MouseCursor::Crossbones, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_NOT_ALLOWED) },
            { MouseCursor::ResizeLeftRight, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_EW_RESIZE) },
            { MouseCursor::ResizeUpDown, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_NS_RESIZE) },
            { MouseCursor::ResizeTopLeftRightBottom, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_NWSE_RESIZE) },
            { MouseCursor::ResizeBottomLeftRightTop, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_NESW_RESIZE) },
            { MouseCursor::ResizeAll, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_MOVE) }
        };
    }();

    currentMouseCursor = cursorToSet;

    if (cursorToSet.getType() == MouseCursor::None)
    {
        SDL_HideCursor();
    }
    else
    {
        auto it = cursors.find (cursorToSet.getType());
        if (it != cursors.end())
            SDL_SetCursor (it->second);

        SDL_ShowCursor();
    }
}

Point<float> Desktop::getCurrentMouseLocation() const
{
    float x = 0.0f, y = 0.0f;

    SDL_GetGlobalMouseState (&x, &y);

    return { x, y };
}

void Desktop::setCurrentMouseLocation (const Point<float>& location)
{
    SDL_WarpMouseGlobal (location.getX(), location.getY());
}

//==============================================================================

YUP_API void YUP_CALLTYPE initialiseYup_Windowing()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: initialising windowing");

    const int compiledVersion = SDL_VERSION;
    const int linkedVersion = SDL_GetVersion();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: compiled version=" << getSDLVersionString (compiledVersion) << ", linked version=" << getSDLVersionString (linkedVersion));

    // Do not install signal handlers
    SDL_SetHint (SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: disabled SDL signal handlers");

    // Initialise SDL
    SDL_SetMainReady();
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: SDL main marked ready");

    const auto alreadyInitialised = SDL_WasInit (sdlDefaultSubsystems);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: requested subsystems=" << String::toHexString (static_cast<int> (sdlDefaultSubsystems)) << ", already initialised=" << String::toHexString (static_cast<int> (alreadyInitialised)));

    if ((alreadyInitialised & sdlDefaultSubsystems) != sdlDefaultSubsystems)
    {
        if (! SDL_InitSubSystem (sdlDefaultSubsystems))
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: error initialising SDL: " << SDL_GetError());

            jassertfalse;
            YUPApplicationBase::quit();

            return;
        }

        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: SDL subsystems initialised");
    }
    else
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: SDL subsystems were already initialised");
    }

    // Update available displays
    Desktop::getInstance()->updateScreens();
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: updated screens");

    SDL_AddEventWatch (displayEventDispatcher, Desktop::getInstance());
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: registered display event watch");

    // Set the default theme now in all platforms except ios
#if ! YUP_IOS
    ApplicationTheme::setGlobalTheme (createThemeVersion1());
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: registered default theme");
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

#if ! YUP_WASM
        if (! timeoutDetector.hasTimedOut())
            Thread::sleep (1);
#endif
    });

    // Set the default theme on ios
#if YUP_IOS
    {
        const MessageManagerLock mmLock;
        ApplicationTheme::setGlobalTheme (createThemeVersion1());
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: registered default theme");
    }
#endif

    SDL2ComponentNative::isInitialised.test_and_set();
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: windowing initialised");
}

YUP_API void YUP_CALLTYPE shutdownYup_Windowing()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: shutting down windowing");

    SDL2ComponentNative::isInitialised.clear();

    // Shutdown desktop
    if (auto desktop = Desktop::getInstanceWithoutCreating())
    {
        SDL_RemoveEventWatch (displayEventDispatcher, desktop);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unregistered display event watch");

        desktop->deleteInstance();
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: deleted desktop instance");
    }

    auto messageManager = MessageManager::getInstanceWithoutCreating();

    // Unregister theme
    if (messageManager == nullptr)
    {
        ApplicationTheme::setGlobalTheme (nullptr);
    }
    else
    {
        const MessageManagerLock mmLock;
        ApplicationTheme::setGlobalTheme (nullptr);
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unregistered default theme");

    // Unregister event loop
    if (messageManager != nullptr)
    {
        messageManager->registerEventLoopCallback (nullptr);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: unregistered event loop callback");
    }

    // Quit only the subsystems YUP initialised.
    SDL_QuitSubSystem (sdlDefaultSubsystems);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: SDL subsystems quit");

#if YUP_STANDALONE_APPLICATION
    std::atexit (&SDL_Quit);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL2: registered SDL_Quit at exit");
#endif
}

//==============================================================================
std::atomic_int ScopedYupInitialiser_Windowing::numScopedInitInstances = 0;

ScopedYupInitialiser_Windowing::ScopedYupInitialiser_Windowing()
{
    if (numScopedInitInstances.fetch_add (1) == 0)
        initialiseYup_Windowing();
}

ScopedYupInitialiser_Windowing::~ScopedYupInitialiser_Windowing()
{
    if (numScopedInitInstances.fetch_add (-1) == 1)
        shutdownYup_Windowing();
}

} // namespace yup
