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

void juce_glfwWindowClose (GLFWwindow* window);
void juce_glfwWindowSize (GLFWwindow* window, int width, int height);
void juce_glfwMouseMove (GLFWwindow* window, double x, double y);
void juce_glfwMousePress (GLFWwindow* window, int button, int action, int mods);
void juce_glfwKeyPress (GLFWwindow* window, int key, int scancode, int action, int mods);

//==============================================================================

MouseEvent toMouseEvent (int buttons, int modifiers, double x, double y) noexcept
{
    return { static_cast<MouseEvent::Buttons> (buttons + 1), modifiers, { static_cast<float> (x), static_cast<float> (y) } };
}

//==============================================================================

KeyModifiers toKeyModifiers (int modifiers) noexcept
{
    return { modifiers };
}

KeyPress toKeyPress (int key, int scancode, int modifiers) noexcept
{
    const char32_t sc = static_cast<char32_t> (scancode);

    switch (key)
    {
    case GLFW_KEY_SPACE:            return { KeyPress::spaceKey, modifiers, sc };
    case GLFW_KEY_APOSTROPHE:       return { KeyPress::apostropheKey, modifiers, sc };
    case GLFW_KEY_COMMA:            return { KeyPress::commaKey, modifiers, sc };
    case GLFW_KEY_MINUS:            return { KeyPress::minusKey, modifiers, sc };
    case GLFW_KEY_PERIOD:           return { KeyPress::periodKey, modifiers, sc };
    case GLFW_KEY_SLASH:            return { KeyPress::slashKey, modifiers, sc };
    case GLFW_KEY_0:                return { KeyPress::number0Key, modifiers, sc };
    case GLFW_KEY_1:                return { KeyPress::number1Key, modifiers, sc };
    case GLFW_KEY_2:                return { KeyPress::number2Key, modifiers, sc };
    case GLFW_KEY_3:                return { KeyPress::number3Key, modifiers, sc };
    case GLFW_KEY_4:                return { KeyPress::number4Key, modifiers, sc };
    case GLFW_KEY_5:                return { KeyPress::number5Key, modifiers, sc };
    case GLFW_KEY_6:                return { KeyPress::number6Key, modifiers, sc };
    case GLFW_KEY_7:                return { KeyPress::number7Key, modifiers, sc };
    case GLFW_KEY_8:                return { KeyPress::number8Key, modifiers, sc };
    case GLFW_KEY_9:                return { KeyPress::number9Key, modifiers, sc };
    case GLFW_KEY_SEMICOLON:        return { KeyPress::semicolonKey, modifiers, sc };
    case GLFW_KEY_EQUAL:            return { KeyPress::equalKey, modifiers, sc };
    case GLFW_KEY_A:                return { KeyPress::textAKey, modifiers, sc };
    case GLFW_KEY_B:                return { KeyPress::textBKey, modifiers, sc };
    case GLFW_KEY_C:                return { KeyPress::textCKey, modifiers, sc };
    case GLFW_KEY_D:                return { KeyPress::textDKey, modifiers, sc };
    case GLFW_KEY_E:                return { KeyPress::textEKey, modifiers, sc };
    case GLFW_KEY_F:                return { KeyPress::textFKey, modifiers, sc };
    case GLFW_KEY_G:                return { KeyPress::textGKey, modifiers, sc };
    case GLFW_KEY_H:                return { KeyPress::textHKey, modifiers, sc };
    case GLFW_KEY_I:                return { KeyPress::textIKey, modifiers, sc };
    case GLFW_KEY_J:                return { KeyPress::textJKey, modifiers, sc };
    case GLFW_KEY_K:                return { KeyPress::textKKey, modifiers, sc };
    case GLFW_KEY_L:                return { KeyPress::textLKey, modifiers, sc };
    case GLFW_KEY_M:                return { KeyPress::textMKey, modifiers, sc };
    case GLFW_KEY_N:                return { KeyPress::textNKey, modifiers, sc };
    case GLFW_KEY_O:                return { KeyPress::textOKey, modifiers, sc };
    case GLFW_KEY_P:                return { KeyPress::textPKey, modifiers, sc };
    case GLFW_KEY_Q:                return { KeyPress::textQKey, modifiers, sc };
    case GLFW_KEY_R:                return { KeyPress::textRKey, modifiers, sc };
    case GLFW_KEY_S:                return { KeyPress::textSKey, modifiers, sc };
    case GLFW_KEY_T:                return { KeyPress::textTKey, modifiers, sc };
    case GLFW_KEY_U:                return { KeyPress::textUKey, modifiers, sc };
    case GLFW_KEY_V:                return { KeyPress::textVKey, modifiers, sc };
    case GLFW_KEY_W:                return { KeyPress::textWKey, modifiers, sc };
    case GLFW_KEY_X:                return { KeyPress::textXKey, modifiers, sc };
    case GLFW_KEY_Y:                return { KeyPress::textYKey, modifiers, sc };
    case GLFW_KEY_Z:                return { KeyPress::textZKey, modifiers, sc };
    case GLFW_KEY_LEFT_BRACKET:     return { KeyPress::leftBracketKey, modifiers, sc };
    case GLFW_KEY_BACKSLASH:        return { KeyPress::backslashKey, modifiers, sc };
    case GLFW_KEY_RIGHT_BRACKET:    return { KeyPress::rightBracketKey, modifiers, sc };
    case GLFW_KEY_GRAVE_ACCENT:     return { KeyPress::graveAccentKey, modifiers, sc };
    case GLFW_KEY_WORLD_1:          return { KeyPress::world1Key, modifiers, sc };
    case GLFW_KEY_WORLD_2:          return { KeyPress::world2Key, modifiers, sc };

    case GLFW_KEY_ESCAPE:           return { KeyPress::escapeKey, modifiers, sc };
    case GLFW_KEY_ENTER:            return { KeyPress::enterKey, modifiers, sc };
    case GLFW_KEY_TAB:              return { KeyPress::tabKey, modifiers, sc };
    case GLFW_KEY_BACKSPACE:        return { KeyPress::backspaceKey, modifiers, sc };
    case GLFW_KEY_INSERT:           return { KeyPress::insertKey, modifiers, sc };
    case GLFW_KEY_DELETE:           return { KeyPress::deleteKey, modifiers, sc };
    case GLFW_KEY_RIGHT:            return { KeyPress::rightKey, modifiers, sc };
    case GLFW_KEY_LEFT:             return { KeyPress::leftKey, modifiers, sc };
    case GLFW_KEY_DOWN:             return { KeyPress::downKey, modifiers, sc };
    case GLFW_KEY_UP:               return { KeyPress::upKey, modifiers, sc };
    case GLFW_KEY_PAGE_UP:          return { KeyPress::pageUpKey, modifiers, sc };
    case GLFW_KEY_PAGE_DOWN:        return { KeyPress::pageDownKey, modifiers, sc };
    case GLFW_KEY_HOME:             return { KeyPress::homeKey, modifiers, sc };
    case GLFW_KEY_END:              return { KeyPress::endKey, modifiers, sc };
    case GLFW_KEY_CAPS_LOCK:        return { KeyPress::capsLockKey, modifiers, sc };
    case GLFW_KEY_SCROLL_LOCK:      return { KeyPress::scrollLockKey, modifiers, sc };
    case GLFW_KEY_NUM_LOCK:         return { KeyPress::numLockKey, modifiers, sc };
    case GLFW_KEY_PRINT_SCREEN:     return { KeyPress::printScreenKey, modifiers, sc };
    case GLFW_KEY_PAUSE:            return { KeyPress::pauseKey, modifiers, sc };
    case GLFW_KEY_F1:               return { KeyPress::f1Key, modifiers, sc };
    case GLFW_KEY_F2:               return { KeyPress::f2Key, modifiers, sc };
    case GLFW_KEY_F3:               return { KeyPress::f3Key, modifiers, sc };
    case GLFW_KEY_F4:               return { KeyPress::f4Key, modifiers, sc };
    case GLFW_KEY_F5:               return { KeyPress::f5Key, modifiers, sc };
    case GLFW_KEY_F6:               return { KeyPress::f6Key, modifiers, sc };
    case GLFW_KEY_F7:               return { KeyPress::f7Key, modifiers, sc };
    case GLFW_KEY_F8:               return { KeyPress::f8Key, modifiers, sc };
    case GLFW_KEY_F9:               return { KeyPress::f9Key, modifiers, sc };
    case GLFW_KEY_F10:              return { KeyPress::f10Key, modifiers, sc };
    case GLFW_KEY_F11:              return { KeyPress::f11Key, modifiers, sc };
    case GLFW_KEY_F12:              return { KeyPress::f12Key, modifiers, sc };
    case GLFW_KEY_F13:              return { KeyPress::f13Key, modifiers, sc };
    case GLFW_KEY_F14:              return { KeyPress::f14Key, modifiers, sc };
    case GLFW_KEY_F15:              return { KeyPress::f15Key, modifiers, sc };
    case GLFW_KEY_F16:              return { KeyPress::f16Key, modifiers, sc };
    case GLFW_KEY_F17:              return { KeyPress::f17Key, modifiers, sc };
    case GLFW_KEY_F18:              return { KeyPress::f18Key, modifiers, sc };
    case GLFW_KEY_F19:              return { KeyPress::f19Key, modifiers, sc };
    case GLFW_KEY_F20:              return { KeyPress::f20Key, modifiers, sc };
    case GLFW_KEY_F21:              return { KeyPress::f21Key, modifiers, sc };
    case GLFW_KEY_F22:              return { KeyPress::f22Key, modifiers, sc };
    case GLFW_KEY_F23:              return { KeyPress::f23Key, modifiers, sc };
    case GLFW_KEY_F24:              return { KeyPress::f24Key, modifiers, sc };
    case GLFW_KEY_F25:              return { KeyPress::f25Key, modifiers, sc };
    case GLFW_KEY_KP_0:             return { KeyPress::kp0Key, modifiers, sc };
    case GLFW_KEY_KP_1:             return { KeyPress::kp1Key, modifiers, sc };
    case GLFW_KEY_KP_2:             return { KeyPress::kp2Key, modifiers, sc };
    case GLFW_KEY_KP_3:             return { KeyPress::kp3Key, modifiers, sc };
    case GLFW_KEY_KP_4:             return { KeyPress::kp4Key, modifiers, sc };
    case GLFW_KEY_KP_5:             return { KeyPress::kp5Key, modifiers, sc };
    case GLFW_KEY_KP_6:             return { KeyPress::kp6Key, modifiers, sc };
    case GLFW_KEY_KP_7:             return { KeyPress::kp7Key, modifiers, sc };
    case GLFW_KEY_KP_8:             return { KeyPress::kp8Key, modifiers, sc };
    case GLFW_KEY_KP_9:             return { KeyPress::kp9Key, modifiers, sc };
    case GLFW_KEY_KP_DECIMAL:       return { KeyPress::kpDecimalKey, modifiers, sc };
    case GLFW_KEY_KP_DIVIDE:        return { KeyPress::kpDivideKey, modifiers, sc };
    case GLFW_KEY_KP_MULTIPLY:      return { KeyPress::kpMultiplyKey, modifiers, sc };
    case GLFW_KEY_KP_SUBTRACT:      return { KeyPress::kpSubtractKey, modifiers, sc };
    case GLFW_KEY_KP_ADD:           return { KeyPress::kpAddKey, modifiers, sc };
    case GLFW_KEY_KP_ENTER:         return { KeyPress::kpEnterKey, modifiers, sc };
    case GLFW_KEY_KP_EQUAL:         return { KeyPress::kpEqualKey, modifiers, sc };

    case GLFW_KEY_LEFT_SHIFT:       return { KeyPress::leftShiftKey, modifiers, sc };
    case GLFW_KEY_LEFT_CONTROL:     return { KeyPress::leftControlKey, modifiers, sc };
    case GLFW_KEY_LEFT_ALT:         return { KeyPress::leftAltKey, modifiers, sc };
    case GLFW_KEY_LEFT_SUPER:       return { KeyPress::leftSuperKey, modifiers, sc };

    case GLFW_KEY_RIGHT_SHIFT:      return { KeyPress::rightShiftKey, modifiers, sc };
    case GLFW_KEY_RIGHT_CONTROL:    return { KeyPress::rightControlKey, modifiers, sc };
    case GLFW_KEY_RIGHT_ALT:        return { KeyPress::rightAltKey, modifiers, sc };
    case GLFW_KEY_RIGHT_SUPER:      return { KeyPress::rightSuperKey, modifiers, sc };

    case GLFW_KEY_MENU:             return { KeyPress::menuKey, modifiers, sc };

    default:
        break;
    }

    return {};
}

//==============================================================================

class GLFWComponentNative final : public ComponentNative, public Thread, public AsyncUpdater
{
public:
    //==============================================================================

    GLFWComponentNative (Component& component, std::optional<float> framerateRedraw)
        : ComponentNative (component)
        , Thread ("YUP Render Thread")
        , desiredFrameRate (framerateRedraw.value_or (60.0f))
    {
       #if JUCE_MAC
        gpu = MTLCreateSystemDefaultDevice();
        queue = [gpu newCommandQueue];
        swapchain = [CAMetalLayer layer];
        swapchain.device = gpu;
        swapchain.opaque = YES;
       #endif

        glfwWindowHint (GLFW_VISIBLE, component.isVisible() ? GLFW_TRUE : GLFW_FALSE);
        //glfwWindowHint (GLFW_DECORATED, GLFW_FALSE);

        auto monitor = component.isFullScreen() ? glfwGetPrimaryMonitor() : nullptr;

        window = glfwCreateWindow (jmax (1, component.getWidth()),
                                   jmax (1, component.getHeight()),
                                   component.getTitle().toRawUTF8(),
                                   monitor,
                                   nullptr);

        glfwSetWindowPos (window, component.getX(), component.getY());

       #if JUCE_MAC
        NSWindow* nswindow = glfwGetCocoaWindow (window);
        nswindow.contentView.layer = swapchain;
        nswindow.contentView.wantsLayer = YES;
       #endif

        context = GraphicsContext::createContext (GraphicsContext::Options{});
        if (context == nullptr)
            return;

       #if JUCE_EMSCRIPTEN && RIVE_WEBGL
        glfwMakeContextCurrent (window);
        glfwSwapInterval (1);
       #endif

        glfwSetWindowUserPointer (window, this);

        glfwSetWindowCloseCallback (window, juce_glfwWindowClose);
        glfwSetCursorPosCallback (window, juce_glfwMouseMove);
        glfwSetMouseButtonCallback (window, juce_glfwMousePress);
        glfwSetKeyCallback (window, juce_glfwKeyPress);
        glfwSetWindowSizeCallback (window, juce_glfwWindowSize);

        startThread();
    }

    ~GLFWComponentNative()
    {
        jassert (window != nullptr);

        signalThreadShouldExit();
        renderEvent.signal();
        stopThread(-1);

        glfwSetWindowUserPointer (window, nullptr);
        glfwDestroyWindow (window);
        window = nullptr;
    }

    //==============================================================================

    void setTitle (const String& title) override
    {
        jassert (window != nullptr);

        if (windowTitle != title)
        {
            glfwSetWindowTitle (window, title.toRawUTF8());

            windowTitle = title;
        }
    }

    String getTitle() const override
    {
       #if !(JUCE_EMSCRIPTEN && RIVE_WEBGL)
        jassert (window != nullptr);

        if (auto title = glfwGetWindowTitle (window))
            return String::fromUTF8 (title);
       #endif

        return windowTitle;
    }

    //==============================================================================

    void setVisible (bool shouldBeVisible) override
    {
        jassert (window != nullptr);

        if (shouldBeVisible)
            glfwShowWindow (window);
        else
            glfwHideWindow (window);
    }

    bool isVisible() const override
    {
        jassert (window != nullptr);

        return false;
    }

    //==============================================================================

    void setSize (const Size<int>& size) override
    {
        jassert (window != nullptr);

       #if JUCE_EMSCRIPTEN && RIVE_WEBGL
        glfwSetWindowSize (window, size.getWidth() * 2, size.getHeight() * 2);

        //emscripten_set_canvas_element_size ("#canvas", size.getWidth(), size.getHeight());

        EM_ASM (
        {
            var canvas = document.getElementById("canvas");
            canvas.style = "width:" + $0 + "px;height:" + $1 + "px;";
        }, size.getWidth(), size.getHeight());

       #else
        glfwSetWindowSize (window, size.getWidth(), size.getHeight());

       #endif
    }

    Size<int> getSize() const override
    {
        jassert (window != nullptr);

        int width = 0, height = 0;
        glfwGetWindowSize (window, &width, &height);
        return { width, height };
    }

    Size<int> getContentSize() const override
    {
        jassert (window != nullptr);

        int width = 0, height = 0;
        glfwGetFramebufferSize (window, &width, &height);
        return { width, height };
    }

    void setBounds (const Rectangle<int>& newBounds) override
    {
        jassert (window != nullptr);

       #if JUCE_EMSCRIPTEN && RIVE_WEBGL
        glfwSetWindowPos (window, newBounds.getX(), newBounds.getY());
        glfwSetWindowSize (window, newBounds.getWidth() * 2, newBounds.getHeight() * 2);

        //emscripten_set_canvas_element_size ("#canvas", newBounds.getWidth(), newBounds.getHeight());

        EM_ASM (
        {
            var canvas = document.getElementById("canvas");
            canvas.style = "width:" + $0 + "px;height:" + $1 + "px;";
        }, newBounds.getWidth(), newBounds.getHeight());

       #else
        glfwSetWindowSize (window, newBounds.getWidth(), newBounds.getHeight());
        glfwSetWindowPos (window, newBounds.getX(), newBounds.getY());

       #endif
    }

    //==============================================================================

    void setFullScreen (bool shouldBeFullScreen) override
    {
        jassert (window != nullptr);

        if (shouldBeFullScreen)
        {
            auto monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode (monitor);

            glfwSetWindowMonitor (window,
                                  monitor,
                                  0,
                                  0,
                                  mode->width,
                                  mode->height,
                                  mode->refreshRate);
        }
        else
        {
            glfwSetWindowMonitor (window,
                                  nullptr,
                                  component.getX(),
                                  component.getY(),
                                  component.getWidth(),
                                  component.getHeight(),
                                  GLFW_DONT_CARE);
        }
    }

    bool isFullScreen() const override
    {
        return window != nullptr && glfwGetWindowMonitor (window) != nullptr;
    }

    //==============================================================================

    float getScaleDpi() const override
    {
        return context->dpiScale (getNativeHandle());
    }

    float getCurrentFrameRate() const override
    {
        return currentFrameRate.load (std::memory_order_relaxed);
    }

    //==============================================================================

    void setOpacity (float opacity) override
    {
        jassert (window != nullptr);
        jassert (isPositiveAndBelow (opacity, 1.0f));

        glfwSetWindowOpacity (window, opacity);
    }

    float getOpacity() const override
    {
        return window ? glfwGetWindowOpacity (window) : 1.0f;
    }

    //==============================================================================

    rive::Factory* getFactory() override
    {
        return context->factory();
    }

    //==============================================================================

    void* getNativeHandle() const override
    {
        jassert (window != nullptr);

       #if JUCE_MAC
        return glfwGetCocoaWindow (window);
       #elif JUCE_WINDOWS
        return glfwGetWin32Window (window);
       #elif JUCE_LINUX
        return glfwGetX11Window (window);
       #else
        return nullptr;
       #endif
    }

    //==============================================================================

    void run() override
    {
        const double maxFrameTime = 1.0 / static_cast<double> (desiredFrameRate);
        const double maxFrameTimeMs = maxFrameTime * 1000.0;
        const uint64 frameUpdateCounter = static_cast<uint64> (desiredFrameRate);

        double currentTime = Time::getMillisecondCounterHiRes() / 1000.0;

        while (! threadShouldExit())
        {
            // Trigger and wait for rendering
            renderEvent.reset();
            triggerAsyncUpdate();
            renderEvent.wait (maxFrameTimeMs);

            // Update framerate
            if (++frameCounter >= frameUpdateCounter)
            {
                const auto newFrameRate = static_cast<float> (frameCounter) * 0.75f
                    + currentFrameRate.load (std::memory_order_relaxed) * 0.25f;

                currentFrameRate.store (newFrameRate, std::memory_order_relaxed);
                frameCounter = 0;
            }

            // Wait for a stable frame time
            const double newTime = Time::getMillisecondCounterHiRes() / 1000.0;
            const double remainingTime = maxFrameTime - (newTime - currentTime);
            currentTime = newTime;

            if (remainingTime > 0.0f)
                Thread::sleep (roundToInt (remainingTime * 1000.0));
        }
    }

    //==============================================================================

    void handleAsyncUpdate() override
    {
        if (! isThreadRunning())
            return;

        auto [width, height] = getContentSize();

        if (currentWidth != width || currentHeight != height)
        {
            currentWidth = width;
            currentHeight = height;

            context->onSizeChanged (getNativeHandle(), width, height, 0);
            renderer = context->makeRenderer (width, height);
        }

        bool forceAtomicMode = false;
        bool wireframe = false;
        bool disableFill = false;
        bool disableStroke = false;

        jassert (context != nullptr);
        jassert (renderer != nullptr);

        context->begin (
        {
            .renderTargetWidth = static_cast<uint32_t> (width),
            .renderTargetHeight = static_cast<uint32_t> (height),
            //.loadAction = rive::pls::LoadAction::preserveRenderTarget,
            .clearColor = 0xff404040,
            .msaaSampleCount = 0,
            .disableRasterOrdering = forceAtomicMode,
            .wireframe = wireframe,
            .fillsDisabled = disableFill,
            .strokesDisabled = disableStroke,
        });

        Graphics g (*context, *renderer);
        handlePaint (g, desiredFrameRate);

        context->end (getNativeHandle());

        context->tick();

        renderEvent.signal();
    }

private:
    GLFWwindow* window = nullptr;
    String windowTitle;
    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;
    float desiredFrameRate = 60.0f;
    std::atomic<float> currentFrameRate = 0.0f;
    uint64 frameCounter = 0;
    int currentWidth = 0;
    int currentHeight = 0;
    WaitableEvent renderEvent{ true };

   #if JUCE_MAC
    id<MTLDevice> gpu = nil;
    id<MTLCommandQueue> queue = nil;
    CAMetalLayer* swapchain = nullptr;
   #endif
};

//==============================================================================

std::unique_ptr<ComponentNative> ComponentNative::createFor (Component& component, std::optional<float> framerateRedraw)
{
    return std::make_unique<GLFWComponentNative> (component, framerateRedraw);
}

//==============================================================================

void juce_glfwWindowClose (GLFWwindow* window)
{
    auto* component = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    component->handleUserTriedToCloseWindow();
}

//==============================================================================

void juce_glfwWindowSize (GLFWwindow* window, int width, int height)
{
    auto* component = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    component->handleResized (width, height);
}

//==============================================================================

void juce_glfwMouseMove (GLFWwindow* window, double x, double y)
{
    auto* component = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    const auto leftButtonDown = glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const auto middleButtonDown = glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const auto rightButtonDown = glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    float dpiScale = component->getScaleDpi();
    x *= dpiScale;
    y *= dpiScale;

    if (leftButtonDown || middleButtonDown || rightButtonDown)
    {
        int buttons = (leftButtonDown ? GLFW_MOUSE_BUTTON_LEFT : 0)
            | (middleButtonDown ? GLFW_MOUSE_BUTTON_MIDDLE : 0)
            | (rightButtonDown ? GLFW_MOUSE_BUTTON_RIGHT : 0);

        component->handleMouseDrag (toMouseEvent (buttons, 0, x, y));
    }
    else
    {
        component->handleMouseMove (toMouseEvent (0, 0, x, y));
    }
}

//==============================================================================

void juce_glfwMousePress (GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos (window, &x, &y);

    auto* component = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    float dpiScale = component->getScaleDpi();
    x *= dpiScale;
    y *= dpiScale;

    if (action == GLFW_PRESS)
        component->handleMouseDown (toMouseEvent (button, mods, x, y));
    else
        component->handleMouseUp (toMouseEvent (button, mods, x, y));
}

//==============================================================================

void juce_glfwKeyPress (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    double x = 0, y = 0;
    glfwGetCursorPos (window, &x, &y);

    auto* component = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    float dpiScale = component->getScaleDpi();
    x *= dpiScale;
    y *= dpiScale;

    if (action == GLFW_PRESS)
        component->handleKeyDown (toKeyPress (key, scancode, mods), x, y);
    else
        component->handleKeyUp (toKeyPress (key, scancode, mods), x, y);
}

//==============================================================================

void juce_glfwMonitorCallback (GLFWmonitor* monitor, int event)
{
    auto desktop = Desktop::getInstance();

    if (event == GLFW_CONNECTED)
    {
    }
    else if (event == GLFW_DISCONNECTED)
    {
    }

    desktop->updateDisplays();
}

void Desktop::updateDisplays()
{
    int count;
    GLFWmonitor** monitors = glfwGetMonitors (&count);
    auto primaryMonitor = glfwGetPrimaryMonitor();

    for (int index = 0; index < count; ++index)
    {
        auto monitor = monitors[index];
        if (monitor == nullptr)
            continue;

        auto display = std::make_unique<Display>();
        glfwSetMonitorUserPointer (monitor, display.get());

        int physicalWidth = 0, physicalHeight = 0;
        glfwGetMonitorPhysicalSize (monitor, &physicalWidth, &physicalHeight);
        display->physicalSizeMillimeters = Size<int> (physicalWidth, physicalHeight);

        int posX = 0, posY = 0;
        glfwGetMonitorPos (monitor, &posX, &posY);
        display->virtualPosition = Point<int> (posX, posY);

        int workX = 0, workY = 0, workWidth = 0, workHeight = 0;
        glfwGetMonitorWorkarea (monitor, &workX, &workY, &workWidth, &workHeight);
        display->workArea = Rectangle<int> (workX, workY, workWidth, workHeight);

        float scaleX = 1.0f, scaleY = 1.0f;
        glfwGetMonitorContentScale (monitor, &scaleX, &scaleY);
        display->contentScaleX = scaleX;
        display->contentScaleY = scaleY;

        if (auto name = glfwGetMonitorName (monitor))
            display->name = String::fromUTF8 (name);

        if (primaryMonitor == monitor)
        {
            display->isPrimary = true;

            displays.insert (0, display.release());
        }
        else
        {
            displays.add (display.release());
        }
    }
}

//==============================================================================

void juce_glfwErrorCallback (int code, const char* message)
{
    DBG ("GLFW Error: " << code << " - " << message);
}

void JUCEApplication::staticInitialisation()
{
    glfwSetErrorCallback (juce_glfwErrorCallback);

    glfwInit();

   #if JUCE_MAC || JUCE_WINDOWS
    glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint (GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
   #elif defined (ANGLE)
    glfwWindowHint (GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   #else
    glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 6);
   #endif

    Desktop::getInstance()->updateDisplays();
    glfwSetMonitorCallback (juce_glfwMonitorCallback);
}

void JUCEApplication::staticFinalisation()
{
    glfwTerminate();
}

} // namespace yup
