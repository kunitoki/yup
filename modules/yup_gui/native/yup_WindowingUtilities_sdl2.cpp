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

//==============================================================================

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

bool isMouseOutsideWindow (SDL_Window* window)
{
    int windowX, windowY, windowW, windowH;
    SDL_GetWindowPosition (window, &windowX, &windowY);
    SDL_GetWindowSize (window, &windowW, &windowH);

    int mouseX, mouseY;
    Uint32 mouseState = SDL_GetGlobalMouseState (&mouseX, &mouseY);

    return (mouseX < windowX || mouseX > windowX + windowW || mouseY < windowY || mouseY > windowY + windowH);
}

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

//==============================================================================

#if JUCE_LINUX
void* getNativeDisplayHandle (SDL_Window* window)
{
    if (window == nullptr)
        return nullptr;

    SDL_SysWMinfo wmInfo;
    SDL_VERSION (&wmInfo.version);
    if (SDL_GetWindowWMInfo (window, &wmInfo))
        return reinterpret_cast<void*> (wmInfo.info.x11.display); // X11 Display

    return nullptr;
}
#endif

//==============================================================================

#if !JUCE_WINDOWS && !JUCE_MAC && !JUCE_LINUX
Rectangle<int> getNativeWindowPosition (void* nativeWindow)
{
    return {};
}
#endif

//==============================================================================

void setNativeParent (void* nativeWindow, SDL_Window* window)
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
    if (! X11Functions::getInstance()->isX11Available())
        return;

    auto* display = reinterpret_cast<::Display*> (getNativeDisplayHandle (window));
    if (display == nullptr)
        return;

    X11Functions::getInstance()->XReparentWindow (
        display,
        reinterpret_cast<::Window> (getNativeWindowHandle (window)),
        reinterpret_cast<::Window> (nativeWindow),
        0,
        0);

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

//==============================================================================

Uint32 setContextWindowHints (GraphicsContext::Api desiredApi)
{
    if (desiredApi == GraphicsContext::Metal)
    {
        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "metal");

        return SDL_WINDOW_METAL;
    }

    if (desiredApi == GraphicsContext::Direct3D)
    {
        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "direct3d11");

        return 0;
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

        return SDL_WINDOW_OPENGL;
#else
        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengl");

        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        return SDL_WINDOW_OPENGL;
#endif
    }

    return 0;
}

} // namespace yup
