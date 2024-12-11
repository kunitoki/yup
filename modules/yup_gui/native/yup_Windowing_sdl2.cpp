/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c): return { KeyPress::xxx, modifiers, sc }; - kunitoki@gmail.com

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

MouseEvent::Buttons toMouseButton (Uint8 sdlButton) noexcept
{
    switch (sdlButton)
    {
        case SDL_BUTTON_LEFT:
            return MouseEvent::Buttons::leftButton;

        case SDL_BUTTON_RIGHT:
            return MouseEvent::Buttons::rightButton;

        case SDL_BUTTON_MIDDLE:
            return MouseEvent::Buttons::middleButton;

        default:
            return MouseEvent::Buttons::noButtons;
    }
}

//==============================================================================

KeyModifiers toKeyModifiers (Uint16 sdlMod) noexcept
{
    int modifiers;

    if (sdlMod & KMOD_CTRL)
        modifiers |= KeyModifiers::controlMask;

    if (sdlMod & KMOD_SHIFT)
        modifiers |= KeyModifiers::shiftMask;

    if (sdlMod & KMOD_ALT)
        modifiers |= KeyModifiers::altMask;

    if (sdlMod & KMOD_GUI)
        modifiers |= KeyModifiers::superMask;

    return modifiers;
}

// clang-format off
KeyPress toKeyPress (SDL_Keycode key, SDL_Scancode scancode, KeyModifiers modifiers) noexcept
{
    const char32_t sc = static_cast<char32_t> (scancode);

    switch (key)
    {
    case SDLK_SPACE:            return { KeyPress::spaceKey, modifiers, sc };
    //case SDLK_APOSTROPHE:       return { KeyPress::apostropheKey, modifiers, sc };
    case SDLK_COMMA:            return { KeyPress::commaKey, modifiers, sc };
    case SDLK_MINUS:            return { KeyPress::minusKey, modifiers, sc };
    case SDLK_PERIOD:           return { KeyPress::periodKey, modifiers, sc };
    case SDLK_SLASH:            return { KeyPress::slashKey, modifiers, sc };
    case SDLK_0:                return { KeyPress::number0Key, modifiers, sc };
    case SDLK_1:                return { KeyPress::number1Key, modifiers, sc };
    case SDLK_2:                return { KeyPress::number2Key, modifiers, sc };
    case SDLK_3:                return { KeyPress::number3Key, modifiers, sc };
    case SDLK_4:                return { KeyPress::number4Key, modifiers, sc };
    case SDLK_5:                return { KeyPress::number5Key, modifiers, sc };
    case SDLK_6:                return { KeyPress::number6Key, modifiers, sc };
    case SDLK_7:                return { KeyPress::number7Key, modifiers, sc };
    case SDLK_8:                return { KeyPress::number8Key, modifiers, sc };
    case SDLK_9:                return { KeyPress::number9Key, modifiers, sc };
    case SDLK_SEMICOLON:        return { KeyPress::semicolonKey, modifiers, sc };
    case SDLK_EQUALS:           return { KeyPress::equalKey, modifiers, sc };
    case SDLK_a:                return { KeyPress::textAKey, modifiers, sc };
    case SDLK_b:                return { KeyPress::textBKey, modifiers, sc };
    case SDLK_c:                return { KeyPress::textCKey, modifiers, sc };
    case SDLK_d:                return { KeyPress::textDKey, modifiers, sc };
    case SDLK_e:                return { KeyPress::textEKey, modifiers, sc };
    case SDLK_f:                return { KeyPress::textFKey, modifiers, sc };
    case SDLK_g:                return { KeyPress::textGKey, modifiers, sc };
    case SDLK_h:                return { KeyPress::textHKey, modifiers, sc };
    case SDLK_i:                return { KeyPress::textIKey, modifiers, sc };
    case SDLK_j:                return { KeyPress::textJKey, modifiers, sc };
    case SDLK_k:                return { KeyPress::textKKey, modifiers, sc };
    case SDLK_l:                return { KeyPress::textLKey, modifiers, sc };
    case SDLK_m:                return { KeyPress::textMKey, modifiers, sc };
    case SDLK_n:                return { KeyPress::textNKey, modifiers, sc };
    case SDLK_o:                return { KeyPress::textOKey, modifiers, sc };
    case SDLK_p:                return { KeyPress::textPKey, modifiers, sc };
    case SDLK_q:                return { KeyPress::textQKey, modifiers, sc };
    case SDLK_r:                return { KeyPress::textRKey, modifiers, sc };
    case SDLK_s:                return { KeyPress::textSKey, modifiers, sc };
    case SDLK_t:                return { KeyPress::textTKey, modifiers, sc };
    case SDLK_u:                return { KeyPress::textUKey, modifiers, sc };
    case SDLK_v:                return { KeyPress::textVKey, modifiers, sc };
    case SDLK_w:                return { KeyPress::textWKey, modifiers, sc };
    case SDLK_x:                return { KeyPress::textXKey, modifiers, sc };
    case SDLK_y:                return { KeyPress::textYKey, modifiers, sc };
    case SDLK_z:                return { KeyPress::textZKey, modifiers, sc };
    case SDLK_LEFTBRACKET:      return { KeyPress::leftBracketKey, modifiers, sc };
    case SDLK_BACKSLASH:        return { KeyPress::backslashKey, modifiers, sc };
    case SDLK_RIGHTBRACKET:     return { KeyPress::rightBracketKey, modifiers, sc };
    //case SDLK_GRAVE_ACCENT:     return { KeyPress::graveAccentKey, modifiers, sc };
    //case SDLK_WORLD_1:          return { KeyPress::world1Key, modifiers, sc };
    //case SDLK_WORLD_2:          return { KeyPress::world2Key, modifiers, sc };
    case SDLK_ESCAPE:           return { KeyPress::escapeKey, modifiers, sc };
    case SDLK_RETURN:           return { KeyPress::enterKey, modifiers, sc };
    case SDLK_TAB:              return { KeyPress::tabKey, modifiers, sc };
    case SDLK_BACKSPACE:        return { KeyPress::backspaceKey, modifiers, sc };
    case SDLK_INSERT:           return { KeyPress::insertKey, modifiers, sc };
    case SDLK_DELETE:           return { KeyPress::deleteKey, modifiers, sc };
    case SDLK_RIGHT:            return { KeyPress::rightKey, modifiers, sc };
    case SDLK_LEFT:             return { KeyPress::leftKey, modifiers, sc };
    case SDLK_DOWN:             return { KeyPress::downKey, modifiers, sc };
    case SDLK_UP:               return { KeyPress::upKey, modifiers, sc };
    case SDLK_PAGEUP:           return { KeyPress::pageUpKey, modifiers, sc };
    case SDLK_PAGEDOWN:         return { KeyPress::pageDownKey, modifiers, sc };
    case SDLK_HOME:             return { KeyPress::homeKey, modifiers, sc };
    case SDLK_END:              return { KeyPress::endKey, modifiers, sc };
    case SDLK_CAPSLOCK:         return { KeyPress::capsLockKey, modifiers, sc };
    case SDLK_SCROLLLOCK:       return { KeyPress::scrollLockKey, modifiers, sc };
    case SDLK_NUMLOCKCLEAR:     return { KeyPress::numLockKey, modifiers, sc };
    case SDLK_PRINTSCREEN:      return { KeyPress::printScreenKey, modifiers, sc };
    case SDLK_PAUSE:            return { KeyPress::pauseKey, modifiers, sc };
    case SDLK_F1:               return { KeyPress::f1Key, modifiers, sc };
    case SDLK_F2:               return { KeyPress::f2Key, modifiers, sc };
    case SDLK_F3:               return { KeyPress::f3Key, modifiers, sc };
    case SDLK_F4:               return { KeyPress::f4Key, modifiers, sc };
    case SDLK_F5:               return { KeyPress::f5Key, modifiers, sc };
    case SDLK_F6:               return { KeyPress::f6Key, modifiers, sc };
    case SDLK_F7:               return { KeyPress::f7Key, modifiers, sc };
    case SDLK_F8:               return { KeyPress::f8Key, modifiers, sc };
    case SDLK_F9:               return { KeyPress::f9Key, modifiers, sc };
    case SDLK_F10:              return { KeyPress::f10Key, modifiers, sc };
    case SDLK_F11:              return { KeyPress::f11Key, modifiers, sc };
    case SDLK_F12:              return { KeyPress::f12Key, modifiers, sc };
    case SDLK_F13:              return { KeyPress::f13Key, modifiers, sc };
    case SDLK_F14:              return { KeyPress::f14Key, modifiers, sc };
    case SDLK_F15:              return { KeyPress::f15Key, modifiers, sc };
    case SDLK_F16:              return { KeyPress::f16Key, modifiers, sc };
    case SDLK_F17:              return { KeyPress::f17Key, modifiers, sc };
    case SDLK_F18:              return { KeyPress::f18Key, modifiers, sc };
    case SDLK_F19:              return { KeyPress::f19Key, modifiers, sc };
    case SDLK_F20:              return { KeyPress::f20Key, modifiers, sc };
    case SDLK_F21:              return { KeyPress::f21Key, modifiers, sc };
    case SDLK_F22:              return { KeyPress::f22Key, modifiers, sc };
    case SDLK_F23:              return { KeyPress::f23Key, modifiers, sc };
    case SDLK_F24:              return { KeyPress::f24Key, modifiers, sc };
    //case SDLK_F25:              return { KeyPress::f25Key, modifiers, sc };
    case SDLK_KP_0:             return { KeyPress::kp0Key, modifiers, sc };
    case SDLK_KP_1:             return { KeyPress::kp1Key, modifiers, sc };
    case SDLK_KP_2:             return { KeyPress::kp2Key, modifiers, sc };
    case SDLK_KP_3:             return { KeyPress::kp3Key, modifiers, sc };
    case SDLK_KP_4:             return { KeyPress::kp4Key, modifiers, sc };
    case SDLK_KP_5:             return { KeyPress::kp5Key, modifiers, sc };
    case SDLK_KP_6:             return { KeyPress::kp6Key, modifiers, sc };
    case SDLK_KP_7:             return { KeyPress::kp7Key, modifiers, sc };
    case SDLK_KP_8:             return { KeyPress::kp8Key, modifiers, sc };
    case SDLK_KP_9:             return { KeyPress::kp9Key, modifiers, sc };
    case SDLK_KP_DECIMAL:       return { KeyPress::kpDecimalKey, modifiers, sc };
    case SDLK_KP_DIVIDE:        return { KeyPress::kpDivideKey, modifiers, sc };
    case SDLK_KP_POWER:         return { KeyPress::kpMultiplyKey, modifiers, sc };
    case SDLK_KP_MINUS:         return { KeyPress::kpSubtractKey, modifiers, sc };
    case SDLK_KP_PLUS:          return { KeyPress::kpAddKey, modifiers, sc };
    case SDLK_KP_ENTER:         return { KeyPress::kpEnterKey, modifiers, sc };
    case SDLK_KP_EQUALS:        return { KeyPress::kpEqualKey, modifiers, sc };
    case SDLK_LSHIFT:           return { KeyPress::leftShiftKey, modifiers, sc };
    case SDLK_LCTRL:            return { KeyPress::leftControlKey, modifiers, sc };
    case SDLK_LALT:             return { KeyPress::leftAltKey, modifiers, sc };
    case SDLK_LGUI:             return { KeyPress::leftSuperKey, modifiers, sc };
    case SDLK_RSHIFT:           return { KeyPress::rightShiftKey, modifiers, sc };
    case SDLK_RCTRL:            return { KeyPress::rightControlKey, modifiers, sc };
    case SDLK_RALT:             return { KeyPress::rightAltKey, modifiers, sc };
    case SDLK_RGUI:             return { KeyPress::rightSuperKey, modifiers, sc };
    case SDLK_MENU:             return { KeyPress::menuKey, modifiers, sc };

    default:
        break;
    }

    return {};
}

// clang-format on

//==============================================================================

void* getNativeWindowHandle (SDL_Window* window)
{
    if (window == nullptr)
        return nullptr;

    SDL_SysWMinfo wmInfo;
    SDL_VERSION (&wmInfo.version);
    if (SDL_GetWindowWMInfo (window, &wmInfo))
    {
#if JUCE_MAC
        return (__bridge void*) wmInfo.info.cocoa.window; // NSWindow*

#elif JUCE_IOS
        return (__bridge void*) wmInfo.info.uikit.window; // UIWindow*

#elif JUCE_WINDOWS
        return wmInfo.info.win.window; // HWND

#elif JUCE_LINUX
        return reinterpret_cast<void*> (wmInfo.info.x11.window); // X11 Window

#elif JUCE_ANDROID
        return reinterpret_cast<void*> (wmInfo.info.android.window); // ANativeWindow*

#endif
    }

    return nullptr;
}

Rectangle<int> getNativeWindowPosition (void* nativeDisplay, void* nativeWindow)
{
#if JUCE_WINDOWS
    RECT windowRect;

    GetWindowRect (reinterpret_cast<HWND> (nativeWindow), &windowRect);

    return {
        windowRect.left,
        windowRect.top,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top
    };

#elif JUCE_MAC
    NSView* view = reinterpret_cast<NSView*> (nativeWindow);
    NSRect viewRect = [view convertRect:[view bounds] toView:nil];

    NSRect windowRect = [[view window] convertRectToScreen:viewRect];
    windowRect.origin.y = CGDisplayBounds (CGMainDisplayID()).size.height - (windowRect.origin.y + windowRect.size.height);

    return {
        static_cast<int> (windowRect.origin.x),
        static_cast<int> (windowRect.origin.y),
        static_cast<int> (windowRect.size.width),
        static_cast<int> (windowRect.size.height)
    };

#elif JUCE_LINUX
    return {};

#else
    return {};

#endif
}

void setNativeParent (void* nativeDisplay, void* nativeWindow, SDL_Window* window)
{
#if JUCE_WINDOWS
    HWND hpar = reinterpret_cast<HWND> (nativeWindow);
    HWND hwnd = reinterpret_cast<HWND> (getNativeWindowHandle (window));
    SetParent (hwnd, hpar);

    long style = GetWindowLong (hwnd, GWL_STYLE);
    style &= ~WS_POPUP;
    style |= WS_CHILDWINDOW;
    SetWindowLong (hwnd, GWL_STYLE, style);

    SetWindowPos (hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

#elif JUCE_MAC
    NSWindow* parentWindow = [reinterpret_cast<NSView*> (nativeWindow) window];
    NSWindow* currentWindow = reinterpret_cast<NSWindow*> (getNativeWindowHandle (window));
    [parentWindow addChildWindow:currentWindow ordered:NSWindowAbove];

#elif JUCE_LINUX

#else

#endif
}

//==============================================================================

GraphicsContext::Api getGraphicsContextApi (const std::optional<GraphicsContext::Api>& forceContextApi)
{
    GraphicsContext::Api desiredApi;

#if JUCE_MAC || JUCE_IOS
#if YUP_RIVE_USE_METAL
    desiredApi = forceContextApi.value_or (GraphicsContext::Metal);
#elif YUP_RIVE_USE_OPENGL
    desiredApi = forceContextApi.value_or (GraphicsContext::OpenGL);
#endif

#elif JUCE_WINDOWS
#if YUP_RIVE_USE_D3D
    desiredApi = forceContextApi.value_or (GraphicsContext::Direct3D);
#elif YUP_RIVE_USE_OPENGL
    desiredApi = forceContextApi.value_or (GraphicsContext::OpenGL);
#endif

#elif JUCE_LINUX
    desiredApi = forceContextApi.value_or (GraphicsContext::OpenGL);

#else
    desiredApi = forceContextApi.value_or (GraphicsContext::OpenGL);

#endif

    return desiredApi;
}

Uint32 setContextWindowHints (GraphicsContext::Api desiredApi)
{
    if (desiredApi == GraphicsContext::Metal)
    {
        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "metal");

        return SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI;
    }

    if (desiredApi == GraphicsContext::Direct3D)
    {
        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "direct3d11");

        return SDL_WINDOW_ALLOW_HIGHDPI;
    }

    if (desiredApi == GraphicsContext::OpenGL)
    {
#if defined(ANGLE) || defined(JUCE_ANDROID) || defined(JUCE_EMSCRIPTEN)
        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengles2");

        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute (SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

        return SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
#else
        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengl");

        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        return SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
#endif
    }

    return SDL_WINDOW_ALLOW_HIGHDPI;
}

//==============================================================================

class SDL2ComponentNative final
    : public ComponentNative
    , public Timer
    , public Thread
    , public AsyncUpdater
{
public:
    static std::atomic_flag isInitialised;

    //==============================================================================

    SDL2ComponentNative (Component& component,
                         const Options& options,
                         void* parent);

    ~SDL2ComponentNative() override;

    //==============================================================================

    void setTitle (const String& title) override;
    String getTitle() const override;

    //==============================================================================

    void setVisible (bool shouldBeVisible) override;
    bool isVisible() const override;

    //==============================================================================

    void setSize (const Size<int>& size) override;
    Size<int> getSize() const override;
    Size<int> getContentSize() const override;

    Point<int> getPosition() const override;
    void setPosition (const Point<int>& newPosition) override;

    Rectangle<int> getBounds() const override;
    void setBounds (const Rectangle<int>& newBounds) override;

    //==============================================================================

    void setFullScreen (bool shouldBeFullScreen) override;
    bool isFullScreen() const override;

    //==============================================================================

    bool isDecorated() const override;

    //==============================================================================
    bool isContinuousRepaintingEnabled() const override;
    void enableContinuousRepainting (bool shouldBeEnabled) override;
    bool isAtomicModeEnabled() const override;
    void enableAtomicMode (bool shouldBeEnabled) override;
    bool isWireframeEnabled() const override;
    void enableWireframe (bool shouldBeEnabled) override;

    //==============================================================================
    void repaint (const Rectangle<float>& rect) override;
    Rectangle<float> getRepaintArea() const override;

    //==============================================================================

    float getScaleDpi() const override;
    float getCurrentFrameRate() const override;
    float getDesiredFrameRate() const override;

    //==============================================================================

    void setOpacity (float opacity) override;
    float getOpacity() const override;

    //==============================================================================

    void setFocusedComponent (Component* comp) override;
    Component* getFocusedComponent() const override;

    //==============================================================================

    rive::Factory* getFactory() override;

    //==============================================================================

    void* getNativeHandle() const override;

    //==============================================================================

    void run() override;
    void handleAsyncUpdate() override;
    void timerCallback() override;

    //==============================================================================
    Point<float> getCursorPosition() const;

    //==============================================================================
    void handleMouseMoveOrDrag (const Point<float>& localPosition);
    void handleMouseDown (const Point<float>& localPosition, MouseEvent::Buttons button, KeyModifiers modifiers);
    void handleMouseUp (const Point<float>& localPosition, MouseEvent::Buttons button, KeyModifiers modifiers);
    void handleMouseWheel (const Point<float>& localPosition, const MouseWheelData& wheelData);
    void handleKeyDown (const KeyPress& keys, const Point<float>& position);
    void handleKeyUp (const KeyPress& keys, const Point<float>& position);
    void handleMoved (int xpos, int ypos);
    void handleResized (int width, int height);
    void handleFocusChanged (bool gotFocus);
    void handleContentScaleChanged (float xscale, float yscale);
    void handleUserTriedToCloseWindow();

    //==============================================================================
    void handleWindowEvent (const SDL_WindowEvent& windowEvent);

    //==============================================================================
    void handleEvent (SDL_Event* event);
    static int eventDispatcher (void* userdata, SDL_Event* event);

private:
    void updateComponentUnderMouse (const MouseEvent& event);
    void triggerRenderingUpdate();
    void renderContext();

    void startRendering();
    void stopRendering();

    SDL_Window* window = nullptr;
    SDL_Renderer* windowRenderer = nullptr;
    SDL_GLContext windowContext = nullptr;

    void* parentWindow = nullptr;
    String windowTitle;

    GraphicsContext::Api currentGraphicsApi;

    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;

    float currentScaleDpi = 1.0f;
    Rectangle<int> screenBounds = { 0, 0, 1, 1 };
    Rectangle<int> lastScreenBounds = { 0, 0, 1, 1 };
    Point<float> lastMouseMovePosition = { -1.0f, -1.0f };
    std::optional<Point<float>> lastMouseDownPosition;
    std::optional<juce::Time> lastMouseDownTime;

    WeakReference<Component> lastComponentClicked;
    WeakReference<Component> lastComponentFocused;
    WeakReference<Component> lastComponentUnderMouse;

    HashMap<int, char> keyState;
    MouseEvent::Buttons currentMouseButtons = MouseEvent::noButtons;
    KeyModifiers currentKeyModifiers;

    RelativeTime doubleClickTime;

    float desiredFrameRate = 60.0f;
    std::atomic<float> currentFrameRate = 0.0f;

    int currentContentWidth = 0;
    int currentContentHeight = 0;

    WaitableEvent renderEvent { true };
    WaitableEvent commandEvent;
    std::atomic<bool> shouldRenderContinuous = false;
    bool renderAtomicMode = false;
    bool renderWireframe = false;
    int forcedRedraws = 0;
    static constexpr int defaultForcedRedraws = 2;

    Rectangle<float> currentRepaintArea;
};

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
    , screenBounds (component.getBounds().to<int>())
    , doubleClickTime (options.doubleClickTime.value_or (RelativeTime::milliseconds (200)))
    , desiredFrameRate (options.framerateRedraw.value_or (60.0f))
    , shouldRenderContinuous (options.flags.test (renderContinuous))
{
    SDL_AddEventWatch (eventDispatcher, this);

    // Setup window hints
    auto windowFlags = setContextWindowHints (currentGraphicsApi);
    windowFlags |= SDL_WINDOW_RESIZABLE;

    if (component.isVisible())
        windowFlags |= SDL_WINDOW_SHOWN;
    else
        windowFlags |= SDL_WINDOW_HIDDEN;

    if (! options.flags.test (decoratedWindow))
        windowFlags |= SDL_WINDOW_BORDERLESS;

    // Create the window, renderer and parent it
    window = SDL_CreateWindow (component.getTitle().toRawUTF8(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1, 1, windowFlags);
    if (window == nullptr)
        return; // TODO - raise something ?

    windowRenderer = SDL_CreateRenderer (window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    SDL_SetWindowData (window, "self", this);

    if (parent != nullptr)
        setNativeParent (nullptr, parent, window);

    // Create the rendering context
    context = GraphicsContext::createContext (currentGraphicsApi, GraphicsContext::Options {});
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
    // Stop the rendering
    stopRendering();

    // Destroy the renderer and window
    if (windowRenderer != nullptr)
        SDL_DestroyRenderer (windowRenderer);

    if (window != nullptr)
    {
        SDL_SetWindowData (window, "self", nullptr);
        SDL_DestroyWindow (window);
    }

    window = nullptr;
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
#if ! (JUCE_EMSCRIPTEN && RIVE_WEBGL)
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

void SDL2ComponentNative::setSize (const Size<int>& size)
{
    setBounds (screenBounds.withSize (size));
}

Size<int> SDL2ComponentNative::getSize() const
{
    int width = 0, height = 0;

    if (window != nullptr)
        SDL_GetWindowSize (window, &width, &height);

    return { width, height };
}

Size<int> SDL2ComponentNative::getContentSize() const
{
    int width = 0, height = 0;

    if (windowRenderer != nullptr)
        SDL_GetRendererOutputSize (windowRenderer, &width, &height);

    return { width, height };
}

Point<int> SDL2ComponentNative::getPosition() const
{
    return screenBounds.getPosition();
}

void SDL2ComponentNative::setPosition (const Point<int>& newPosition)
{
    if (window == nullptr || screenBounds.getPosition() == newPosition)
        return;

    SDL_SetWindowPosition (window, newPosition.getX(), newPosition.getY());

    screenBounds = screenBounds.withPosition (newPosition);
}

Rectangle<int> SDL2ComponentNative::getBounds() const
{
    return screenBounds;
}

void SDL2ComponentNative::setBounds (const Rectangle<int>& newBounds)
{
#if JUCE_ANDROID
    screenBounds = Rectangle<int> (0, 0, getSize());

#else
    if (window == nullptr)
        return;

    int leftMargin = 0, topMargin = 0, rightMargin = 0, bottomMargin = 0;

#if JUCE_EMSCRIPTEN && RIVE_WEBGL
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

    SDL_SetWindowSize (window,
                       jmax (1, newBounds.getWidth() - leftMargin - rightMargin),
                       jmax (1, newBounds.getHeight() - topMargin - bottomMargin));

#endif

    //setPosition (newBounds.getPosition().translated (leftMargin, topMargin));
    SDL_SetWindowPosition (window, newBounds.getX() + leftMargin, newBounds.getY() + topMargin);

    screenBounds = newBounds;

#endif
}

//==============================================================================

void SDL2ComponentNative::setFullScreen (bool shouldBeFullScreen)
{
    if (window == nullptr)
        return;

    if (shouldBeFullScreen)
    {
#if JUCE_EMSCRIPTEN
        emscripten_request_fullscreen ("#canvas", false);
#else
        lastScreenBounds = screenBounds;

        SDL_SetWindowFullscreen (window, SDL_WINDOW_FULLSCREEN); // SDL_SetWindowDisplayMode
#endif
    }
    else
    {
#if JUCE_EMSCRIPTEN
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
    if (lastComponentFocused != nullptr)
        ; // TODO

    lastComponentFocused = comp;
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

    component.repaint();
}

bool SDL2ComponentNative::isWireframeEnabled() const
{
    return renderWireframe;
}

void SDL2ComponentNative::enableWireframe (bool shouldBeEnabled)
{
    renderWireframe = shouldBeEnabled;

    component.repaint();
}

//==============================================================================

void SDL2ComponentNative::repaint (const Rectangle<float>& rect)
{
    if (! currentRepaintArea.isEmpty())
        currentRepaintArea = currentRepaintArea.smallestContainingRectangle (rect);
    else
        currentRepaintArea = rect;

    triggerRenderingUpdate();
}

Rectangle<float> SDL2ComponentNative::getRepaintArea() const
{
    return currentRepaintArea;
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

    double fpsMeasureStartTimeSeconds = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    uint64_t frameCounter = 0;

    while (! threadShouldExit())
    {
        double frameStartTimeSeconds = juce::Time::getMillisecondCounterHiRes() / 1000.0;

        // Trigger and wait for rendering
        renderEvent.reset();
        triggerAsyncUpdate();
        renderEvent.wait (maxFrameTimeMs);

        // Wait for any repaint command
        if (! shouldRenderContinuous)
        {
            while (! commandEvent.wait (10.0f))
                currentFrameRate.store (0.0f, std::memory_order_relaxed);
        }

        // Measure spent time and cap the framerate
        double currentTimeSeconds = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        double timeSpentSeconds = currentTimeSeconds - frameStartTimeSeconds;

        const double secondsToWait = maxFrameTimeSeconds - timeSpentSeconds;
        if (secondsToWait > 0.0f)
        {
            const auto waitUntilMs = (currentTimeSeconds + secondsToWait) * 1000.0;

            while (juce::Time::getMillisecondCounterHiRes() < waitUntilMs - 2.0)
                Thread::sleep (1);

            while (juce::Time::getMillisecondCounterHiRes() < waitUntilMs)
                Thread::sleep (0);
        }

        // Measure current framerate
        ++frameCounter;

        const double timeSinceFpsMeasure = currentTimeSeconds - fpsMeasureStartTimeSeconds;
        if (timeSinceFpsMeasure >= 1.0)
        {
            const double currentFps = static_cast<double> (frameCounter) / timeSinceFpsMeasure;
            currentFrameRate.store (currentFps, std::memory_order_relaxed);

            fpsMeasureStartTimeSeconds = currentTimeSeconds;
            frameCounter = 0;
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
    auto [contentWidth, contentHeight] = getContentSize();
    if (context == nullptr || contentWidth == 0 || contentHeight == 0)
        return;

    auto renderContinuous = shouldRenderContinuous.load (std::memory_order_relaxed);

    if (currentContentWidth != contentWidth || currentContentHeight != contentHeight)
    {
        currentContentWidth = contentWidth;
        currentContentHeight = contentHeight;

        context->onSizeChanged (getNativeHandle(), contentWidth, contentHeight, 0);
        renderer = context->makeRenderer (contentWidth, contentHeight);

        repaint (Rectangle<float> (0, 0, contentWidth, contentHeight));
        forcedRedraws = defaultForcedRedraws;
    }

    if (parentWindow != nullptr)
    {
        auto nativeWindowPos = getNativeWindowPosition (nullptr, parentWindow);
        setPosition (nativeWindowPos.getTopLeft());
    }

    if (! renderContinuous && currentRepaintArea.isEmpty())
        return;

    const auto loadAction = renderContinuous
                              ? rive::gpu::LoadAction::clear
                              : rive::gpu::LoadAction::preserveRenderTarget;

    // Begin context drawing
    rive::gpu::RenderContext::FrameDescriptor frameDescriptor;
    frameDescriptor.renderTargetWidth = static_cast<uint32_t> (contentWidth);
    frameDescriptor.renderTargetHeight = static_cast<uint32_t> (contentHeight);
    frameDescriptor.loadAction = loadAction;
    frameDescriptor.clearColor = 0xff000000;
    frameDescriptor.msaaSampleCount = 0;
    frameDescriptor.disableRasterOrdering = renderAtomicMode;
    frameDescriptor.wireframe = renderWireframe;
    frameDescriptor.fillsDisabled = false;
    frameDescriptor.strokesDisabled = false;
    context->begin (frameDescriptor);

    // Repaint components hierarchy
    if (renderer != nullptr)
    {
        Graphics g (*context, *renderer, currentScaleDpi);
        component.internalPaint (g, desiredFrameRate);
    }

    // Finish context drawing
    context->end (getNativeHandle());
    context->tick();

    // Swap buffers
    if (window != nullptr && currentGraphicsApi == GraphicsContext::OpenGL)
        SDL_GL_SwapWindow (window);

    if (! renderContinuous)
    {
        if (forcedRedraws > 0)
            --forcedRedraws;
        else
            currentRepaintArea = {};
    }
}

//==============================================================================

void SDL2ComponentNative::triggerRenderingUpdate()
{
    if (shouldRenderContinuous)
        return;

    forcedRedraws = defaultForcedRedraws;
    commandEvent.signal();
}

//==============================================================================

void SDL2ComponentNative::startRendering()
{
#if (JUCE_EMSCRIPTEN && RIVE_WEBGL) && ! defined(__EMSCRIPTEN_PTHREADS__)
    startTimerHz (desiredFrameRate);
#else
    startThread (Priority::high);
#endif
}

void SDL2ComponentNative::stopRendering()
{
#if (JUCE_EMSCRIPTEN && RIVE_WEBGL) && ! defined(__EMSCRIPTEN_PTHREADS__)
    stopTimer();
#else
    signalThreadShouldExit();
    notify();
    renderEvent.signal();
    commandEvent.signal();
    stopThread (-1);
#endif
}

//==============================================================================

void SDL2ComponentNative::handleMouseMoveOrDrag (const Point<float>& localPosition)
{
    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (localPosition);

    if (lastMouseDownPosition)
        event = event.withLastMouseDownPosition (*lastMouseDownPosition);

    if (lastMouseDownTime)
        event = event.withLastMouseDownTime (*lastMouseDownTime);

    if (lastComponentClicked != nullptr)
    {
        event = event.withSourceComponent (lastComponentClicked);

        lastComponentClicked->internalMouseDrag (event);
    }
    else
    {
        updateComponentUnderMouse (event);

        if (lastComponentUnderMouse != nullptr)
            lastComponentUnderMouse->internalMouseMove (event);
    }

    lastMouseMovePosition = localPosition;
}

void SDL2ComponentNative::handleMouseDown (const Point<float>& localPosition, MouseEvent::Buttons button, KeyModifiers modifiers)
{
    currentMouseButtons = static_cast<MouseEvent::Buttons> (currentMouseButtons | button);
    currentKeyModifiers = modifiers;

    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (localPosition);

    if (lastComponentClicked == nullptr)
    {
        if (auto child = component.findComponentAt (localPosition))
            lastComponentClicked = child;
    }

    if (lastComponentClicked != nullptr)
    {
        const auto currentMouseDownTime = juce::Time::getCurrentTime();

        event = event.withSourceComponent (lastComponentClicked);

        if (lastMouseDownTime
            && lastMouseDownPosition
            && *lastMouseDownTime > juce::Time()
            && currentMouseDownTime - *lastMouseDownTime < doubleClickTime)
        {
            event = event.withLastMouseDownPosition (*lastMouseDownPosition);
            event = event.withLastMouseDownTime (*lastMouseDownTime);

            lastComponentClicked->internalMouseDoubleClick (event);
        }
        else
        {
            lastComponentClicked->internalMouseDown (event);
        }

        lastMouseDownPosition = localPosition;
        lastMouseDownTime = currentMouseDownTime;
    }

    lastMouseMovePosition = localPosition;
}

void SDL2ComponentNative::handleMouseUp (const Point<float>& localPosition, MouseEvent::Buttons button, KeyModifiers modifiers)
{
    currentMouseButtons = static_cast<MouseEvent::Buttons> (currentMouseButtons & ~button);
    currentKeyModifiers = modifiers;

    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (localPosition);

    if (lastMouseDownPosition)
        event = event.withLastMouseDownPosition (*lastMouseDownPosition);

    if (lastMouseDownTime)
        event = event.withLastMouseDownTime (*lastMouseDownTime);

    if (lastComponentClicked != nullptr)
    {
        event = event.withSourceComponent (lastComponentClicked);

        lastComponentClicked->internalMouseUp (event);
    }

    if (currentMouseButtons == MouseEvent::noButtons)
    {
        updateComponentUnderMouse (event);

        lastComponentClicked = nullptr;
    }

    lastMouseMovePosition = localPosition;
    lastMouseDownPosition.reset();
    lastMouseDownTime.reset();
}

//==============================================================================

void SDL2ComponentNative::handleMouseWheel (const Point<float>& localPosition, const MouseWheelData& wheelData)
{
    auto event = MouseEvent()
                     .withButtons (currentMouseButtons)
                     .withModifiers (currentKeyModifiers)
                     .withPosition (localPosition);

    if (lastMouseDownPosition)
        event = event.withLastMouseDownPosition (*lastMouseDownPosition);

    if (lastMouseDownTime)
        event = event.withLastMouseDownTime (*lastMouseDownTime);

    updateComponentUnderMouse (event);

    if (lastComponentClicked != nullptr)
    {
        event = event.withSourceComponent (lastComponentClicked);

        lastComponentClicked->internalMouseWheel (event, wheelData);
    }
    else if (lastComponentFocused != nullptr)
    {
        lastComponentFocused->internalMouseWheel (event, wheelData);
    }
    else if (lastComponentUnderMouse != nullptr)
    {
        lastComponentUnderMouse->internalMouseWheel (event, wheelData);
    }
}

//==============================================================================

void SDL2ComponentNative::handleKeyDown (const KeyPress& keys, const Point<float>& cursorPosition)
{
    currentKeyModifiers = keys.getModifiers();
    keyState.set (keys.getKey(), 1);

    if (lastComponentFocused != nullptr)
        lastComponentFocused->internalKeyDown (keys, cursorPosition);
    else
        component.internalKeyDown (keys, cursorPosition);
}

void SDL2ComponentNative::handleKeyUp (const KeyPress& keys, const Point<float>& cursorPosition)
{
    currentKeyModifiers = keys.getModifiers();
    keyState.set (keys.getKey(), 0);

    if (lastComponentFocused != nullptr)
        lastComponentFocused->internalKeyUp (keys, cursorPosition);
    else
        component.internalKeyUp (keys, cursorPosition);
}

//==============================================================================

void SDL2ComponentNative::handleMoved (int xpos, int ypos)
{
    component.internalMoved (xpos, ypos);

    screenBounds = screenBounds.withPosition (xpos, ypos);
}

void SDL2ComponentNative::handleResized (int width, int height)
{
    component.internalResized (width, height);

    screenBounds = screenBounds.withSize (width, height);
    currentScaleDpi = getScaleDpi();

    triggerRenderingUpdate();
}

void SDL2ComponentNative::handleFocusChanged (bool gotFocus)
{
    //DBG ("handleFocusChanged: " << (gotFocus ? 1 : 0));
}

void SDL2ComponentNative::handleContentScaleChanged (float xscale, float yscale)
{
    int width = screenBounds.getWidth();
    int height = screenBounds.getHeight();

    if (window != nullptr)
        SDL_GetWindowSize (window, &width, &height);

    handleResized (width, height);
}

void SDL2ComponentNative::handleUserTriedToCloseWindow()
{
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
            child->internalMouseEnter (event);
        }
        else if (lastComponentUnderMouse != child)
        {
            lastComponentUnderMouse->internalMouseExit (event);
            child->internalMouseEnter (event);
        }
    }
    else
    {
        if (lastComponentUnderMouse)
        {
            lastComponentUnderMouse->internalMouseExit (event);
        }
    }

    lastComponentUnderMouse = child;
}

//==============================================================================

std::unique_ptr<ComponentNative> ComponentNative::createFor (Component& component,
                                                             const Options& options,
                                                             void* parent)
{
    return std::make_unique<SDL2ComponentNative> (component, options, parent);
}

//==============================================================================

void SDL2ComponentNative::handleWindowEvent (const SDL_WindowEvent& windowEvent)
{
    switch (windowEvent.event)
    {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            handleResized (windowEvent.data1, windowEvent.data2);
            break;

        case SDL_WINDOWEVENT_MOVED:
            handleMoved (windowEvent.data1, windowEvent.data2);
            break;

        case SDL_WINDOWEVENT_FOCUS_GAINED:
            handleFocusChanged (true);
            break;

        case SDL_WINDOWEVENT_FOCUS_LOST:
            handleFocusChanged (false);
            break;
    }
}

//==============================================================================

void SDL2ComponentNative::handleEvent (SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_QUIT:
        {
            component.internalUserTriedToCloseWindow();
            break;
        }

        case SDL_WINDOWEVENT:
        {
            handleWindowEvent (event->window);
            break;
        }

        case SDL_MOUSEMOTION:
        {
            handleMouseMoveOrDrag ({ static_cast<float> (event->motion.x), static_cast<float> (event->motion.y) });
            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        {
            auto cursorPosition = Point<float> { static_cast<float> (event->button.x), static_cast<float> (event->button.y) };

            handleMouseDown (cursorPosition, toMouseButton (event->button.button), KeyModifiers());

            break;
        }

        case SDL_MOUSEBUTTONUP:
        {
            auto cursorPosition = Point<float> { static_cast<float> (event->button.x), static_cast<float> (event->button.y) };

            handleMouseUp (cursorPosition, toMouseButton (event->button.button), KeyModifiers());

            break;
        }

        case SDL_MOUSEWHEEL:
        {
            auto cursorPosition = getCursorPosition();

            handleMouseWheel (cursorPosition, { static_cast<float> (event->wheel.x), static_cast<float> (event->wheel.y) });

            break;
        }

        case SDL_KEYDOWN:
        {
            auto cursorPosition = getCursorPosition();
            auto modifiers = toKeyModifiers (event->key.keysym.mod);

            handleKeyDown (toKeyPress (event->key.keysym.sym, event->key.keysym.scancode, modifiers), cursorPosition);

            break;
        }

        case SDL_KEYUP:
        {
            auto cursorPosition = getCursorPosition();
            auto modifiers = toKeyModifiers (event->key.keysym.mod);

            handleKeyDown (toKeyPress (event->key.keysym.sym, event->key.keysym.scancode, modifiers), cursorPosition);

            break;
        }

        default:
            break;
    }
}

//==============================================================================

int SDL2ComponentNative::eventDispatcher (void* userdata, SDL_Event* event)
{
    static_cast<SDL2ComponentNative*> (userdata)->handleEvent (event);
    return 0;
}

//==============================================================================

void Desktop::updateDisplays()
{
    const int numDisplays = SDL_GetNumVideoDisplays();
    for (int i = 0; i < numDisplays; ++i)
    {
        SDL_Rect bounds;
        if (SDL_GetDisplayBounds (i, &bounds) != 0)
            continue;

        auto display = std::make_unique<Display>();
        display->virtualPosition = Point<int> (bounds.x, bounds.y);
        display->workArea = Rectangle<int> (bounds.x, bounds.y, bounds.w, bounds.h);

        float ddpi, hdpi, vdpi;
        if (SDL_GetDisplayDPI (i, &ddpi, &hdpi, &vdpi) == 0)
        {
            display->physicalSizeMillimeters = Size<int> (
                static_cast<int> (bounds.w * 25.4f / hdpi),
                static_cast<int> (bounds.h * 25.4f / vdpi));
        }

        display->contentScaleX = hdpi / 96.0f; // Assuming 96 DPI as standard
        display->contentScaleY = vdpi / 96.0f;

        display->name = String (SDL_GetDisplayName (i));
        display->isPrimary = (i == 0);

        displays.add (display.release());
    }
}

//==============================================================================

void initialiseYup_Windowing()
{
    // Initialise SDL2
    if (SDL_Init (SDL_INIT_VIDEO) != 0)
    {
        DBG ("Error initialising SDL");
        return; // quit !
    }

    // Update available displays
    Desktop::getInstance()->updateDisplays();

    // Allow SDL to poll events
    auto loopCallback = []
    {
        SDL_PollEvent (nullptr);
    };

    MessageManager::getInstance()->registerEventLoopCallback (loopCallback);

    SDL2ComponentNative::isInitialised.test_and_set();
}

void shutdownYup_Windowing()
{
    SDL2ComponentNative::isInitialised.clear();

    MessageManager::getInstance()->registerEventLoopCallback (nullptr);

    Desktop::getInstance()->deleteInstance();

    SDL_Quit();
}

} // namespace yup
