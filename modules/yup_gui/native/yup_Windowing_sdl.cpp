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

std::atomic_flag SDLComponentNative::isInitialised = ATOMIC_FLAG_INIT;
int SDLComponentNative::mouseCaptureRequestCount = 0;
uint32_t SDLComponentNative::lastCapturedMouseButtonState = 0;
bool SDLComponentNative::popupDismissalCheckPending = false;

//==============================================================================

SDLComponentNative::SDLComponentNative (Component& component,
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

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: constructing native component: component=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (&component))) << ", parent=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (parent))) << ", bounds=" << component.getBounds().toString() << ", visible=" << String (component.isVisible() ? "true" : "false") << ", renderContinuous=" << String (shouldRenderContinuous ? "true" : "false") << ", updateOnlyWhenFocused=" << String (updateOnlyWhenFocused ? "true" : "false") << ", desiredFrameRate=" << String (desiredFrameRate));

    Desktop::getInstance()->registerNativeComponent (this);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: registered native component");

    SDL_AddEventWatch (eventDispatcher, this);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: registered window event watch");

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
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: creating window: title=" << component.getTitle() << ", flags=" << String::toHexString (static_cast<int> (windowFlags)) << ", parent=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (parent))));

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
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unable to create child window");
            return;
        }

        SDL_PropertiesID windowProps = SDL_CreateProperties();
        SDL_SetPointerProperty (windowProps, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, reinterpret_cast<void*> (childHwnd));
        window = SDL_CreateWindowWithProperties (windowProps);
        SDL_DestroyProperties (windowProps);

        if (window == nullptr)
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unable to wrap child window with SDL: " << SDL_GetError());
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
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unable to create heavyweight window: " << SDL_GetError());
            return; // TODO - raise something ?
        }
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: created window: id=" << static_cast<int64> (SDL_GetWindowID (window)) << ", window=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (window))));

#if ! YUP_WINDOWS
    if (parent != nullptr)
    {
        setNativeParent (parent, window);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: set native parent");
    }
#endif

    if (currentGraphicsApi == GraphicsContext::OpenGL || currentGraphicsApi == GraphicsContext::OpenGLES)
    {
        windowContext = SDL_GL_CreateContext (window);
        if (windowContext == nullptr)
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unable to create GL context: " << SDL_GetError());
            return; // TODO - raise something ?
        }

        SDL_GL_MakeCurrent (window, windowContext);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: created GL context");
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
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unable to create YUP GraphicsContext");
        return; // TODO - raise something ?
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: created YUP GraphicsContext");

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

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: native component constructed: window=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (window))) << ", context=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (context.get()))) << ", rendering=" << String (isRendering() ? "true" : "false"));
}

SDLComponentNative::~SDLComponentNative()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: destroying native component: window=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (window))) << ", context=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (context.get()))) << ", rendering=" << String (isRendering() ? "true" : "false"));

    updateMouseCapture (false);

    // Stop the rendering first, before touching any SDL resources
    stopRendering();

    // Cancel any pending async update that may have been scheduled by the render thread
    cancelPendingUpdate();

    // Remove event watch
    SDL_RemoveEventWatch (eventDispatcher, this);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unregistered window event watch");

    // Unregister this component from the desktop
    Desktop::getInstance()->unregisterNativeComponent (this);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unregistered native component");

    // Destroy graphics resources before the SDL window
    renderer.reset();
    context.reset();

    // Destroy the window
    if (window != nullptr)
    {
        SDL_DestroyWindow (window);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: destroyed window");
        window = nullptr;
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: native component destroyed");
}

//==============================================================================

void SDLComponentNative::setTitle (const String& title)
{
    if (windowTitle == title)
        return;

    if (window != nullptr)
        SDL_SetWindowTitle (window, title.toRawUTF8());

    windowTitle = title;
}

String SDLComponentNative::getTitle() const
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

void SDLComponentNative::setVisible (bool shouldBeVisible)
{
    if (window == nullptr)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setVisible skipped: window is null, visible=" << String (shouldBeVisible ? "true" : "false"));
        return;
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setVisible " << String (shouldBeVisible ? "true" : "false") << ", currentFlags=" << String::toHexString (static_cast<int> (SDL_GetWindowFlags (window))));

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

bool SDLComponentNative::isVisible() const
{
    return window != nullptr && (SDL_GetWindowFlags (window) & SDL_WINDOW_HIDDEN) == 0;
}

//==============================================================================

void SDLComponentNative::toFront()
{
    if (window != nullptr && isVisible())
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: raise window");
        SDL_RaiseWindow (window);
    }
}

//==============================================================================

Size<int> SDLComponentNative::getContentSize() const
{
    // The drawable size must match the real window surface: deriving it from the
    // logical size multiplied by a scale factor would drift from the actual pixel
    // size on platforms where SDL window coordinates are physical pixels.
    if (window != nullptr)
    {
        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels (window, &width, &height);

        if (width > 0 && height > 0)
            return { width, height };
    }

    return { jmax (1, screenBounds.getWidth()), jmax (1, screenBounds.getHeight()) };
}

//==============================================================================

void SDLComponentNative::setSize (const Size<int>& newSize)
{
    if (window == nullptr)
        return;

    screenBounds = screenBounds.withSize (newSize);

    if (auto currentSize = getSize(); currentSize != newSize)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setSize " << currentSize.toString() << " -> " << newSize.toString());

        const auto scale = getWindowUnitsPerPoint (window);
        SDL_SetWindowSize (window,
                           jmax (1, roundToInt (newSize.getWidth() * scale)),
                           jmax (1, roundToInt (newSize.getHeight() * scale)));
    }
}

Size<int> SDLComponentNative::getSize() const
{
    int width = 0, height = 0;

    if (window != nullptr)
        SDL_GetWindowSize (window, &width, &height);

    const auto scale = getWindowUnitsPerPoint (window);
    return { roundToInt (width / scale), roundToInt (height / scale) };
}

void SDLComponentNative::setPosition (const Point<int>& newPosition)
{
    if (window == nullptr)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setPosition skipped: window is null, position=" << newPosition.toString());
        return;
    }

    screenBounds = screenBounds.withPosition (newPosition);

    if (auto currentPosition = getPosition(); currentPosition != newPosition)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setPosition " << currentPosition.toString() << " -> " << newPosition.toString());

        const auto scale = getWindowUnitsPerPoint (window);
        SDL_SetWindowPosition (window,
                               roundToInt (newPosition.getX() * scale),
                               roundToInt (newPosition.getY() * scale));
    }
}

Point<int> SDLComponentNative::getPosition() const
{
    int x = 0, y = 0;

    if (window != nullptr)
        SDL_GetWindowPosition (window, &x, &y);

    const auto scale = getWindowUnitsPerPoint (window);
    return { roundToInt (x / scale), roundToInt (y / scale) };
}

void SDLComponentNative::setBounds (const Rectangle<int>& newBounds)
{
#if YUP_ANDROID
    screenBounds = Rectangle<int> (0, 0, getSize());

#else
    if (window == nullptr)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setBounds skipped: window is null, bounds=" << newBounds.toString());
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
    const auto scale = getWindowUnitsPerPoint (window);

    if (! isFullScreen() && isDecorated())
    {
        // Window borders are reported in native window units
        SDL_GetWindowBordersSize (window, &leftMargin, &topMargin, &rightMargin, &bottomMargin);

        leftMargin = roundToInt (leftMargin / scale);
        topMargin = roundToInt (topMargin / scale);
        rightMargin = roundToInt (rightMargin / scale);
        bottomMargin = roundToInt (bottomMargin / scale);
    }

    adjustedBounds.translate (leftMargin, topMargin);
    adjustedBounds.setSize ({ jmax (1, adjustedBounds.getWidth() - leftMargin - rightMargin),
                              jmax (1, adjustedBounds.getHeight() - topMargin - bottomMargin) });

    if (auto currentSize = getSize(); currentSize != adjustedBounds.getSize())
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setBounds size " << currentSize.toString() << " -> " << adjustedBounds.getSize().toString() << ", requested=" << newBounds.toString() << ", margins=" << leftMargin << "," << topMargin << "," << rightMargin << "," << bottomMargin);
        SDL_SetWindowSize (window,
                           roundToInt (adjustedBounds.getWidth() * scale),
                           roundToInt (adjustedBounds.getHeight() * scale));
    }

#endif

    if (auto currentPosition = getPosition(); currentPosition != adjustedBounds.getPosition())
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setBounds position " << currentPosition.toString() << " -> " << adjustedBounds.getPosition().toString() << ", requested=" << newBounds.toString());

        const auto positionScale = getWindowUnitsPerPoint (window);
        SDL_SetWindowPosition (window,
                               roundToInt (adjustedBounds.getX() * positionScale),
                               roundToInt (adjustedBounds.getY() * positionScale));
    }

    screenBounds = newBounds;

#endif
}

Rectangle<int> SDLComponentNative::getBounds() const
{
    return screenBounds;
}

//==============================================================================

Rectangle<int> SDLComponentNative::getSafeAreaBounds() const
{
    if (SDL_Rect safeArea; window != nullptr && SDL_GetWindowSafeArea (window, &safeArea))
    {
        const auto scale = getWindowUnitsPerPoint (window);
        return { roundToInt (safeArea.x / scale),
                 roundToInt (safeArea.y / scale),
                 roundToInt (safeArea.w / scale),
                 roundToInt (safeArea.h / scale) };
    }

    return { 0, 0, getSize() };
}

//==============================================================================

void SDLComponentNative::setFullScreen (bool shouldBeFullScreen)
{
    if (window == nullptr)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setFullScreen skipped: window is null, fullScreen=" << String (shouldBeFullScreen ? "true" : "false"));
        return;
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: setFullScreen " << String (shouldBeFullScreen ? "true" : "false") << ", current=" << String (isFullScreen() ? "true" : "false"));

    if (shouldBeFullScreen)
    {
        lastScreenBounds = screenBounds;

#if YUP_EMSCRIPTEN
        emscripten_request_fullscreen ("#canvas", false);
#else
        SDL_SetWindowFullscreen (window, true); // SDL_SetWindowFullscreenMode
#endif
    }
    else
    {
#if YUP_EMSCRIPTEN
        emscripten_exit_fullscreen();
#else
        SDL_RestoreWindow (window);

        const auto scale = getWindowUnitsPerPoint (window);
        SDL_SetWindowSize (window,
                           roundToInt (component.getWidth() * scale),
                           roundToInt (component.getHeight() * scale));
        SDL_SetWindowPosition (window,
                               roundToInt (component.getX() * scale),
                               roundToInt (component.getY() * scale));

        setBounds (lastScreenBounds);
#endif
    }
}

bool SDLComponentNative::isFullScreen() const
{
    return window != nullptr && (SDL_GetWindowFlags (window) & SDL_WINDOW_FULLSCREEN) != 0;
}

//==============================================================================

bool SDLComponentNative::isDecorated() const
{
    return window != nullptr && (SDL_GetWindowFlags (window) & SDL_WINDOW_BORDERLESS) == 0;
}

//==============================================================================

void SDLComponentNative::setOpacity (float opacity)
{
    if (window != nullptr)
        SDL_SetWindowOpacity (window, jlimit (0.0f, 1.0f, opacity));
}

float SDLComponentNative::getOpacity() const
{
    if (window != nullptr)
        return SDL_GetWindowOpacity (window);

    return 1.0f;
}

//==============================================================================

void SDLComponentNative::setFocusedComponent (Component* comp)
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

Component* SDLComponentNative::getFocusedComponent() const
{
    return hasNativeKeyboardFocus() ? lastComponentFocused.get() : nullptr;
}

//==============================================================================

bool SDLComponentNative::isContinuousRepaintingEnabled() const
{
    return shouldRenderContinuous;
}

void SDLComponentNative::enableContinuousRepainting (bool shouldBeEnabled)
{
    shouldRenderContinuous = shouldBeEnabled;
}

bool SDLComponentNative::isAtomicModeEnabled() const
{
    return renderAtomicMode;
}

void SDLComponentNative::enableAtomicMode (bool shouldBeEnabled)
{
    renderAtomicMode = shouldBeEnabled;

    repaint();
}

bool SDLComponentNative::isWireframeEnabled() const
{
    return renderWireframe;
}

void SDLComponentNative::enableWireframe (bool shouldBeEnabled)
{
    renderWireframe = shouldBeEnabled;

    repaint();
}

//==============================================================================

void SDLComponentNative::repaint()
{
    currentRepaintAreas.clearQuick();

    currentRepaintAreas.add (Rectangle<float>().withSize (getSize().to<float>()));
}

void SDLComponentNative::repaint (const Rectangle<float>& rect)
{
    currentRepaintAreas.add (rect);
}

const RectangleList<float>& SDLComponentNative::getRepaintAreas() const
{
    return currentRepaintAreas;
}

//==============================================================================

float SDLComponentNative::getScaleDpi() const
{
    if (window == nullptr || (windowFlags & SDL_WINDOW_HIGH_PIXEL_DENSITY) == 0)
        return 1.0f;

    const auto scale = SDL_GetWindowDisplayScale (window);
    return scale > 0.0f ? scale : 1.0f;
}

float SDLComponentNative::getCurrentFrameRate() const
{
    return currentFrameRate.load (std::memory_order_relaxed);
}

float SDLComponentNative::getDesiredFrameRate() const
{
    return desiredFrameRate;
}

//==============================================================================

Point<float> SDLComponentNative::getCursorPosition() const
{
    float x = 0, y = 0;

    SDL_GetMouseState (&x, &y);

    return Point<float> (x, y) / getWindowUnitsPerPoint (window);
}

//==============================================================================

rive::Factory* SDLComponentNative::getFactory()
{
    return context ? context->factory() : nullptr;
}

//==============================================================================

GraphicsContext* SDLComponentNative::getGraphicsContext()
{
    return context.get();
}

//==============================================================================

void* SDLComponentNative::getNativeHandle() const
{
    return getNativeWindowHandle (window);
}

//==============================================================================

void SDLComponentNative::startTextInput (Component& component)
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

void SDLComponentNative::updateTextInputRect (Component& component)
{
    if (window == nullptr || currentTextInputComponent != std::addressof (component))
        return;

    auto* target = dynamic_cast<TextInputTarget*> (std::addressof (component));
    if (target == nullptr)
        return;

    const auto scale = getWindowUnitsPerPoint (window);

    SDL_Rect sdlRect;
    auto textRect = target->getTextInputRect();
    sdlRect.x = roundToInt (textRect.getX() * scale);
    sdlRect.y = roundToInt (textRect.getY() * scale);
    sdlRect.w = roundToInt (textRect.getWidth() * scale);
    sdlRect.h = roundToInt (textRect.getHeight() * scale);
    SDL_SetTextInputArea (window, &sdlRect, 0);
}

void SDLComponentNative::stopTextInput (Component& component)
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

void SDLComponentNative::run()
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
                std::this_thread::sleep_for (std::chrono::microseconds (25));

            while (yup::Time::getMillisecondCounterHiRes() < waitUntilMs - 2.0)
                std::this_thread::sleep_for (std::chrono::microseconds (10));

            while (yup::Time::getMillisecondCounterHiRes() < waitUntilMs)
                std::this_thread::sleep_for (std::chrono::microseconds (1));
        }
    }
}

void SDLComponentNative::handleAsyncUpdate()
{
    if (! isThreadRunning() || ! isInitialised.test_and_set())
        return;

    renderContext();

    renderEvent.signal();
}

void SDLComponentNative::timerCallback()
{
#if ! (YUP_MOBILE || YUP_EMSCRIPTEN)
    if (window != nullptr) // WIN ONLY: currentMouseButtons != MouseEvent::noButtons
    {
        int windowX = 0, windowY = 0;
        SDL_GetWindowPosition (window, &windowX, &windowY);

        float mouseX = 0.0f, mouseY = 0.0f;
        SDL_GetGlobalMouseState (&mouseX, &mouseY);

        const auto cursorPosition = Point<float> { mouseX - static_cast<float> (windowX),
                                                   mouseY - static_cast<float> (windowY) }
                                  / getWindowUnitsPerPoint (window);

        if (lastMouseMovePosition != cursorPosition)
            handleMouseMoveOrDrag (cursorPosition);
    }

    pollCapturedMouseState();
#endif

    renderContext();
}

//==============================================================================

void SDLComponentNative::renderContext()
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

        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: resize render target " << currentContentWidth << "x" << currentContentHeight << " -> " << contentWidth << "x" << contentHeight << ", dpiScale=" << getScaleDpi());

        currentContentWidth = contentWidth;
        currentContentHeight = contentHeight;

        context->onSizeChanged (getNativeHandle(), contentWidth, contentHeight, getScaleDpi(), 0);
        renderer = context->makeRenderer (contentWidth, contentHeight);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: renderer " << String (renderer != nullptr ? "created" : "creation failed"));

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

void SDLComponentNative::startRendering()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: startRendering requested: timerDriven=" << String (renderDrivenByTimer ? "true" : "false") << ", alreadyRendering=" << String (isRendering() ? "true" : "false") << ", desiredFrameRate=" << String (desiredFrameRate));

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

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: startRendering completed: rendering=" << String (isRendering() ? "true" : "false"));
}

void SDLComponentNative::stopRendering()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: stopRendering requested: rendering=" << String (isRendering() ? "true" : "false"));

    if constexpr (renderDrivenByTimer)
    {
        if (isTimerRunning())
        {
            stopTimer();
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL: stopped render timer");
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
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL: stopped render thread");
        }
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: stopRendering completed: rendering=" << String (isRendering() ? "true" : "false"));
}

bool SDLComponentNative::isRendering() const
{
    if constexpr (renderDrivenByTimer)
        return isTimerRunning();
    else
        return isThreadRunning();
}

//==============================================================================

void SDLComponentNative::handleMouseMoveOrDrag (const Point<float>& position)
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

void SDLComponentNative::handleMouseDown (const Point<float>& position, MouseEvent::Buttons button, KeyModifiers modifiers)
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

void SDLComponentNative::handleMouseUp (const Point<float>& position, MouseEvent::Buttons button, KeyModifiers modifiers)
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

void SDLComponentNative::handleMouseWheel (const Point<float>& position, const MouseWheelData& wheelData)
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

void SDLComponentNative::handleMouseEnter (const Point<float>& position)
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

void SDLComponentNative::handleMouseLeave (const Point<float>& position)
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

void SDLComponentNative::handleKeyDown (const KeyPress& keys, const Point<float>& position)
{
    currentKeyModifiers = keys.getModifiers();
    keyState.set (keys.getKey(), 1);

    if (lastComponentFocused != nullptr)
        lastComponentFocused->internalKeyDown (keys, position); // TODO: remove position
    else
        component.internalKeyDown (keys, position);
}

void SDLComponentNative::handleKeyUp (const KeyPress& keys, const Point<float>& position)
{
    currentKeyModifiers = keys.getModifiers();
    keyState.set (keys.getKey(), 0);

    if (lastComponentFocused != nullptr)
        lastComponentFocused->internalKeyUp (keys, position); // TODO: remove position
    else
        component.internalKeyUp (keys, position);
}

void SDLComponentNative::handleTextInput (const String& textInput)
{
    if (! hasNativeKeyboardFocus() || currentTextInputComponent == nullptr)
        return;

    currentTextInputComponent->internalTextInput (textInput);
}

void SDLComponentNative::handleItemsDropped (const Point<float>& position, const DragAndDropData& data)
{
    if (Component* target = findComponentForMouseEvent (position))
        target->internalItemsDropped (data, position);
}

void SDLComponentNative::handleItemsDragPosition (const Point<float>& position, const DragAndDropData& data)
{
    Component* target = findComponentForMouseEvent (position);

    if (target != nullptr)
    {
        if (lastComponentUnderDrag == nullptr)
        {
            target->internalItemDragEnter (data, position);
        }
        else if (lastComponentUnderDrag != target)
        {
            lastComponentUnderDrag->internalItemDragExit (data);
            target->internalItemDragEnter (data, position);
        }
        else
        {
            target->internalItemDragMove (data, position);
        }
    }
    else
    {
        if (lastComponentUnderDrag != nullptr)
        {
            lastComponentUnderDrag->internalItemDragExit (data);
        }
    }

    lastComponentUnderDrag = target;
}

//==============================================================================

void SDLComponentNative::handleMoved (int xpos, int ypos)
{
    YUP_PROFILE_INTERNAL_TRACE();

    if (internalBoundsChange)
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleMoved ignored during internal bounds change: " << xpos << " " << ypos);
        return;
    }

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleMoved " << screenBounds.getX() << " " << screenBounds.getY() << " -> " << xpos << " " << ypos << ", parent=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (parentWindow))));

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

        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: parent window position sync after move: " << nativeWindowPos.toString());
        setPosition (nativeWindowPos.getTopLeft());

        xpos = nativeWindowPos.getX();
        ypos = nativeWindowPos.getY();
    }

    component.internalMoved (xpos, ypos);

    screenBounds = screenBounds.withPosition (xpos, ypos);
}

void SDLComponentNative::handleResized (int width, int height)
{
    YUP_PROFILE_INTERNAL_TRACE();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleResized " << screenBounds.getWidth() << "x" << screenBounds.getHeight() << " -> " << width << "x" << height << ", parent=" << String::toHexString (static_cast<int64> (reinterpret_cast<pointer_sized_uint> (parentWindow))));

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

        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: parent window position sync after resize: " << nativeWindowPos.toString());
        setPosition (nativeWindowPos.getTopLeft());
    }

    if (dynamic_cast<PopupMenu*> (&component) == nullptr)
        PopupMenu::dismissAllPopups();

    repaint();
}

void SDLComponentNative::handleFocusChanged (bool gotFocus)
{
    YUP_PROFILE_INTERNAL_TRACE();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleFocusChanged " << String (gotFocus ? "true" : "false") << ", rendering=" << String (isRendering() ? "true" : "false"));

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

bool SDLComponentNative::hasNativeKeyboardFocus() const
{
    return window != nullptr && (SDL_GetWindowFlags (window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void SDLComponentNative::handleMinimized()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleMinimized");
    PopupMenu::dismissAllPopups();

    stopRendering();
}

void SDLComponentNative::handleMaximized()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleMaximized");
    repaint();
}

void SDLComponentNative::handleRestored()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleRestored");
    repaint();
}

void SDLComponentNative::handleExposed()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleExposed");
    repaint();
}

void SDLComponentNative::handleContentScaleChanged()
{
    YUP_PROFILE_INTERNAL_TRACE();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleContentScaleChanged dpiScale=" << getScaleDpi());

    // When the window units per point ratio changes (Windows/X11 dpi change), resize
    // the native window so the component keeps its logical size. This is a no-op on
    // platforms where SDL window coordinates are already logical points.
    if (const auto scale = getWindowUnitsPerPoint (window); scale != lastWindowUnitsPerPoint)
    {
        lastWindowUnitsPerPoint = scale;
        setSize (screenBounds.getSize());
    }

    component.internalContentScaleChanged (getScaleDpi());

    const auto currentSize = getSize();
    handleResized (currentSize.getWidth(), currentSize.getHeight());
}

void SDLComponentNative::handleDisplayChanged()
{
    YUP_PROFILE_INTERNAL_TRACE();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleDisplayChanged");

    component.internalDisplayChanged();
}

void SDLComponentNative::handleSafeAreaChanged()
{
    YUP_PROFILE_INTERNAL_TRACE();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: handleSafeAreaChanged " << getSafeAreaBounds().toString());

    component.internalSafeAreaChanged();

    repaint();
}

void SDLComponentNative::handleUserTriedToCloseWindow()
{
    YUP_PROFILE_INTERNAL_TRACE();

    component.internalUserTriedToCloseWindow();
}

//==============================================================================

Component* SDLComponentNative::findComponentForMouseEvent (const Point<float>& position)
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

void SDLComponentNative::updateComponentUnderMouse (const MouseEvent& event)
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

void SDLComponentNative::handleWindowEvent (const SDL_WindowEvent& windowEvent)
{
    YUP_PROFILE_INTERNAL_TRACE();

    switch (windowEvent.type)
    {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_CLOSE_REQUESTED");
            component.internalUserTriedToCloseWindow();
            break;

        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_DISPLAY_CHANGED");
            handleContentScaleChanged();
            break;

        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED");
            handleContentScaleChanged();
            break;

        case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_SAFE_AREA_CHANGED");
            handleSafeAreaChanged();
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_RESIZED " << windowEvent.data1 << " " << windowEvent.data2);
            //handleResized (windowEvent.data1, windowEvent.data2);
            break;

        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED " << windowEvent.data1 << " " << windowEvent.data2);
            handleContentScaleChanged();
            break;

        case SDL_EVENT_WINDOW_MOVED:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_MOVED " << windowEvent.data1 << " " << windowEvent.data2);
            const auto scale = getWindowUnitsPerPoint (window);
            handleMoved (roundToInt (windowEvent.data1 / scale), roundToInt (windowEvent.data2 / scale));
            break;
        }

        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_MOUSE_ENTER");
            handleMouseEnter (getCursorPosition());
            break;

        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_MOUSE_LEAVE");
            handleMouseLeave (getCursorPosition());
            break;

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
            // YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_WINDOW_EXPOSED");
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

        default:
            break;
    }
}

//==============================================================================

void SDLComponentNative::handleEvent (SDL_Event* event)
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

#if YUP_MOBILE || YUP_EMSCRIPTEN
        case SDL_EVENT_MOUSE_MOTION:
        {
            auto cursorPosition = Point<float> { static_cast<float> (event->motion.x), static_cast<float> (event->motion.y) }
                                / getWindowUnitsPerPoint (window);

            if (event->motion.windowID == SDL_GetWindowID (window))
                handleMouseMoveOrDrag (cursorPosition);

            break;
        }
#endif

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_MOUSE_BUTTON_DOWN " << event->button.x << " " << event->button.y);

            auto cursorPosition = Point<float> { static_cast<float> (event->button.x), static_cast<float> (event->button.y) }
                                / getWindowUnitsPerPoint (window);

            if (event->button.windowID == SDL_GetWindowID (window))
                handleMouseDown (cursorPosition, toMouseButton (event->button.button), KeyModifiers (SDL_GetModState()));

            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL_EVENT_MOUSE_BUTTON_UP " << event->button.x << " " << event->button.y);

            auto cursorPosition = Point<float> { static_cast<float> (event->button.x), static_cast<float> (event->button.y) }
                                / getWindowUnitsPerPoint (window);

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

                // Clean up any previous drag enter state
                if (lastComponentUnderDrag != nullptr)
                {
                    auto data = DragAndDropData()
                                    .withFiles (pendingDroppedFiles)
                                    .withText (pendingDroppedText);

                    lastComponentUnderDrag->internalItemDragExit (data);
                    lastComponentUnderDrag = nullptr;
                }

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

        case SDL_EVENT_DROP_POSITION:
        {
            if (event->drop.windowID == SDL_GetWindowID (window))
            {
                auto data = DragAndDropData()
                                .withFiles (pendingDroppedFiles)
                                .withText (pendingDroppedText);

                handleItemsDragPosition (Point<float> (event->drop.x, event->drop.y) / getWindowUnitsPerPoint (window), data);
            }

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

                // Clean up drag enter/exit tracking
                if (lastComponentUnderDrag != nullptr)
                {
                    lastComponentUnderDrag->internalItemDragExit (data);
                    lastComponentUnderDrag = nullptr;
                }

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

bool SDLComponentNative::eventDispatcher (void* userdata, SDL_Event* event)
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
                if (auto nativeComponent = dynamic_cast<SDLComponentNative*> (component.get()))
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

void SDLComponentNative::triggerPopupDismissalCheck()
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

void SDLComponentNative::dismissPopupsIfNoNativeWindowHasFocus()
{
    popupDismissalCheckPending = false;

    if (anyNativeWindowHasKeyboardFocus())
        return;

    PopupMenu::dismissAllPopups();
}

bool SDLComponentNative::anyNativeWindowHasKeyboardFocus()
{
    auto* desktop = Desktop::getInstanceWithoutCreating();

    if (desktop == nullptr)
        return false;

    for (const auto& [userdata, nativeComponent] : desktop->nativeComponents)
    {
        ignoreUnused (userdata);

        if (auto* sdlNativeComponent = dynamic_cast<SDLComponentNative*> (nativeComponent))
        {
            if (sdlNativeComponent->hasNativeKeyboardFocus())
                return true;
        }
    }

    return false;
}

bool SDLComponentNative::anyNativeWindowContains (Point<float> screenPosition)
{
    auto* desktop = Desktop::getInstanceWithoutCreating();

    if (desktop == nullptr)
        return false;

    for (const auto& [userdata, nativeComponent] : desktop->nativeComponents)
    {
        ignoreUnused (userdata);

        auto* sdlNativeComponent = dynamic_cast<SDLComponentNative*> (nativeComponent);
        if (sdlNativeComponent == nullptr || ! sdlNativeComponent->isVisible())
            continue;

        // Global mouse coordinates are in native window units, bounds are in points
        const auto scaledPosition = screenPosition / getWindowUnitsPerPoint (sdlNativeComponent->window);
        if (sdlNativeComponent->getBounds().to<float>().contains (scaledPosition))
            return true;
    }

    return false;
}

//==============================================================================

void SDLComponentNative::updateMouseCapture (bool shouldBeActive)
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

void SDLComponentNative::pollCapturedMouseState()
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

bool SDLComponentNative::requestMouseCapture()
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

void SDLComponentNative::releaseMouseCapture()
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
    return ComponentNative::Ptr (ReferenceCountedObjectAdopt, new SDLComponentNative (component, options, parent));
}

} // namespace yup
