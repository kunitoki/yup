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

void yup_glfwWindowClose (GLFWwindow* window);
void yup_glfwWindowPos (GLFWwindow* window, int xpos, int ypos);
void yup_glfwWindowSize (GLFWwindow* window, int width, int height);
void yup_glfwWindowFocus (GLFWwindow* window, int focused);
void yup_glfwMouseMove (GLFWwindow* window, double x, double y);
void yup_glfwMousePress (GLFWwindow* window, int button, int action, int mods);
void yup_glfwMouseScroll (GLFWwindow* window, double xoffset, double yoffset);
void yup_glfwKeyPress (GLFWwindow* window, int key, int scancode, int action, int mods);

//==============================================================================

MouseEvent::Buttons toMouseButton (int buttons) noexcept
{
    return static_cast<MouseEvent::Buttons> (buttons + 1);
}

MouseEvent toMouseEvent (int buttons, int modifiers, double x, double y) noexcept
{
    return { toMouseButton (buttons), modifiers, { static_cast<float> (x), static_cast<float> (y) } };
}

MouseEvent toMouseEvent (int buttons, int modifiers, const Point<float>& p) noexcept
{
    return { toMouseButton (buttons), modifiers, p };
}

//==============================================================================

int keyToModifier (int key)
{
    int mod = 0;

    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
        mod = GLFW_MOD_CONTROL;

    else if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
        mod = GLFW_MOD_SHIFT;

    else if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
        mod = GLFW_MOD_ALT;

    else if (key == GLFW_KEY_LEFT_SUPER || key == GLFW_KEY_RIGHT_SUPER)
        mod = GLFW_MOD_SUPER;

    return mod;
}

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

class GLFWComponentNative final
    : public ComponentNative
    , public Timer
    , public Thread
    , public AsyncUpdater
{
public:
    //==============================================================================

    GLFWComponentNative (Component& component, bool continuousRepaint, std::optional<float> framerateRedraw);
    ~GLFWComponentNative() override;

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
    bool isContinuousRepaintingEnabled() const override;
    void enableContinuousRepainting (bool shouldBeEnabled) override;
    bool isAtomicModeEnabled() const override;
    void enableAtomicMode (bool shouldBeEnabled) override;
    bool isWireframeEnabled() const override;
    void enableWireframe (bool shouldBeEnabled) override;

    //==============================================================================

    float getScaleDpi() const override;
    float getCurrentFrameRate() const override;

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
    void handlePaint (Graphics& g, float frameRate);
    void handleMouseMoveOrDrag (const Point<float>& localPosition);
    void handleMouseDown (const Point<float>& localPosition, MouseEvent::Buttons button, KeyModifiers modifiers);
    void handleMouseUp (const Point<float>& localPosition, MouseEvent::Buttons button, KeyModifiers modifiers);
    void handleMouseWheel (const Point<float>& localPosition, const MouseWheelData& wheelData);
    void handleKeyDown (const KeyPress& keys, const Point<float>& position);
    void handleKeyUp (const KeyPress& keys, const Point<float>& position);
    void handleMoved (int xpos, int ypos);
    void handleResized (int width, int height);
    void handleFocusChanged (bool gotFocus);
    void handleUserTriedToCloseWindow();

    //==============================================================================
    Point<float> getScaledCursorPosition() const;

private:
    void updateComponentUnderMouse (const MouseEvent& event);
    void triggerRenderingUpdate();
    void renderContext();

    GLFWwindow* window = nullptr;
    String windowTitle;

    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;

    Rectangle<int> screenBounds = { 0, 0, 1, 1 };
    Rectangle<int> lastScreenBounds = { 0, 0, 1, 1 };
    Point<float> lastMouseMovePosition = { -1.0f, -1.0f };
    Point<float> lastMouseDownPosition = { -1.0f, -1.0f };

    WeakReference<Component> lastComponentClicked;
    WeakReference<Component> lastComponentFocused;
    WeakReference<Component> lastComponentUnderMouse;

    MouseEvent::Buttons currentMouseButtons = MouseEvent::noButtons;
    KeyModifiers currentKeyModifiers;

    float desiredFrameRate = 60.0f;
    std::atomic<float> currentFrameRate = 0.0f;

    int currentWidth = 0;
    int currentHeight = 0;

    WaitableEvent renderEvent { true };
    WaitableEvent commandEvent;
    bool renderAtomicMode = false;
    bool renderWireframe = false;
    std::atomic<bool> renderContinuous = false;

   #if JUCE_MAC
    id<MTLDevice> gpu = nil;
    id<MTLCommandQueue> queue = nil;
    CAMetalLayer* swapchain = nullptr;
   #endif
};

//==============================================================================

GLFWComponentNative::GLFWComponentNative (Component& component, bool continuousRepaint, std::optional<float> framerateRedraw)
    : ComponentNative (component)
    , Thread ("YUP Render Thread")
    , screenBounds (component.getBounds().to<int>())
    , desiredFrameRate (framerateRedraw.value_or (60.0f))
    , renderContinuous (continuousRepaint)
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

    window = glfwCreateWindow (jmax (1, screenBounds.getWidth()),
                               jmax (1, screenBounds.getHeight()),
                               component.getTitle().toRawUTF8(),
                               monitor,
                               nullptr);

    glfwSetWindowPos (window, screenBounds.getX(), screenBounds.getY());

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
    glfwSwapInterval (0);
   #endif

    glfwSetWindowUserPointer (window, this);

    glfwSetWindowCloseCallback (window, yup_glfwWindowClose);
    glfwSetWindowSizeCallback (window, yup_glfwWindowSize);
    glfwSetWindowPosCallback (window, yup_glfwWindowPos);
    glfwSetWindowFocusCallback (window, yup_glfwWindowFocus);
    glfwSetCursorPosCallback (window, yup_glfwMouseMove);
    glfwSetMouseButtonCallback (window, yup_glfwMousePress);
    glfwSetScrollCallback (window, yup_glfwMouseScroll);
    glfwSetKeyCallback (window, yup_glfwKeyPress);

   #if JUCE_EMSCRIPTEN && RIVE_WEBGL
    startTimerHz (desiredFrameRate);
   #else
    startThread (Priority::high);
   #endif
}

GLFWComponentNative::~GLFWComponentNative()
{
    jassert (window != nullptr);

   #if JUCE_EMSCRIPTEN && RIVE_WEBGL
    stopTimer();
   #else
    signalThreadShouldExit();
    renderEvent.signal();
    commandEvent.signal();
    stopThread(-1);
   #endif

    glfwSetWindowUserPointer (window, nullptr);
    glfwDestroyWindow (window);
    window = nullptr;
}

//==============================================================================

void GLFWComponentNative::setTitle (const String& title)
{
    jassert (window != nullptr);

    if (windowTitle != title)
    {
        glfwSetWindowTitle (window, title.toRawUTF8());

        windowTitle = title;
    }
}

String GLFWComponentNative::getTitle() const
{
   #if !(JUCE_EMSCRIPTEN && RIVE_WEBGL)
    jassert (window != nullptr);

    if (auto title = glfwGetWindowTitle (window))
        return String::fromUTF8 (title);
   #endif

    return windowTitle;
}

//==============================================================================

void GLFWComponentNative::setVisible (bool shouldBeVisible)
{
    jassert (window != nullptr);

    if (shouldBeVisible)
        glfwShowWindow (window);
    else
        glfwHideWindow (window);
}

bool GLFWComponentNative::isVisible() const
{
    jassert (window != nullptr);

    return false;
}

//==============================================================================

void GLFWComponentNative::setSize (const Size<int>& size)
{
    jassert (window != nullptr);

   #if JUCE_EMSCRIPTEN && RIVE_WEBGL
    double devicePixelRatio = emscripten_get_device_pixel_ratio();
    glfwSetWindowSize (window, static_cast<int> (size.getWidth() * devicePixelRatio), static_cast<int> (size.getHeight() * devicePixelRatio));

    EM_ASM (
    {
        var canvas = document.getElementById("canvas");
        canvas.style = "width:" + $0 + "px; height:" + $1 + "px;";
    }, size.getWidth(), size.getHeight());

   #else
    glfwSetWindowSize (window, size.getWidth(), size.getHeight());

   #endif

   screenBounds = screenBounds.withSize (size);
}

Size<int> GLFWComponentNative::getSize() const
{
    jassert (window != nullptr);

    int width = 0, height = 0;
    glfwGetWindowSize (window, &width, &height);
    return { width, height };
}

Size<int> GLFWComponentNative::getContentSize() const
{
    jassert (window != nullptr);

    int width = 0, height = 0;
    glfwGetFramebufferSize (window, &width, &height);
    return { width, height };
}

Point<int> GLFWComponentNative::getPosition() const
{
    return screenBounds.getPosition();
}

void GLFWComponentNative::setPosition (const Point<int>& newPosition)
{
    glfwSetWindowPos (window, newPosition.getX(), newPosition.getY());

    screenBounds = screenBounds.withPosition (newPosition * getScaleDpi());
}

Rectangle<int> GLFWComponentNative::getBounds() const
{
    return screenBounds;
}

void GLFWComponentNative::setBounds (const Rectangle<int>& newBounds)
{
    jassert (window != nullptr);

    int leftMargin = 0, topMargin = 0, rightMargin = 0, bottomMargin = 0;

   #if JUCE_EMSCRIPTEN && RIVE_WEBGL
    const double devicePixelRatio = emscripten_get_device_pixel_ratio();
    glfwSetWindowSize (window,
        static_cast<int> (newBounds.getWidth() * devicePixelRatio),
        static_cast<int> (newBounds.getHeight() * devicePixelRatio));

    emscripten_set_element_css_size("#canvas", newBounds.getWidth(), newBounds.getHeight());

   #else
    if (! isFullScreen() && glfwGetWindowAttrib (window, GLFW_DECORATED) != 0)
        glfwGetWindowFrameSize (window, &leftMargin, &topMargin, &rightMargin, &bottomMargin);

    glfwSetWindowSize (window,
        newBounds.getWidth() - leftMargin - rightMargin,
        newBounds.getHeight() - topMargin - bottomMargin);

   #endif

    glfwSetWindowPos (window, newBounds.getX() + leftMargin, newBounds.getY() + topMargin);

    screenBounds = newBounds * getScaleDpi();
}

//==============================================================================

void GLFWComponentNative::setFullScreen (bool shouldBeFullScreen)
{
    jassert (window != nullptr);

    if (shouldBeFullScreen)
    {
        lastScreenBounds = screenBounds / getScaleDpi();

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

        setBounds (lastScreenBounds);
    }
}

bool GLFWComponentNative::isFullScreen() const
{
    return window != nullptr && glfwGetWindowMonitor (window) != nullptr;
}

//==============================================================================

void GLFWComponentNative::setOpacity (float opacity)
{
    jassert (window != nullptr);
    jassert (isPositiveAndBelow (opacity, 1.0f));

    glfwSetWindowOpacity (window, opacity);
}

float GLFWComponentNative::getOpacity() const
{
    return window ? glfwGetWindowOpacity (window) : 1.0f;
}

//==============================================================================

void GLFWComponentNative::setFocusedComponent (Component* comp)
{
    if (lastComponentFocused != nullptr)
        ;

    lastComponentFocused = comp;
}

Component* GLFWComponentNative::getFocusedComponent() const
{
    return lastComponentFocused;
}

//==============================================================================

bool GLFWComponentNative::isContinuousRepaintingEnabled() const
{
    return renderContinuous;
}

void GLFWComponentNative::enableContinuousRepainting (bool shouldBeEnabled)
{
    renderContinuous = shouldBeEnabled;
}

bool GLFWComponentNative::isAtomicModeEnabled() const
{
    return renderAtomicMode;
}

void GLFWComponentNative::enableAtomicMode (bool shouldBeEnabled)
{
    renderAtomicMode = shouldBeEnabled;
}

bool GLFWComponentNative::isWireframeEnabled() const
{
    return renderWireframe;
}

void GLFWComponentNative::enableWireframe (bool shouldBeEnabled)
{
    renderWireframe = shouldBeEnabled;
}

//==============================================================================

float GLFWComponentNative::getScaleDpi() const
{
    return context->dpiScale (getNativeHandle());
}

float GLFWComponentNative::getCurrentFrameRate() const
{
    return currentFrameRate.load (std::memory_order_relaxed);
}

//==============================================================================

Point<float> GLFWComponentNative::getScaledCursorPosition() const
{
    double x, y;
    glfwGetCursorPos (window, &x, &y);

    const float dpiScale = getScaleDpi();
    x *= dpiScale;
    y *= dpiScale;

    return { static_cast<float> (x), static_cast<float> (y) };
}

//==============================================================================

rive::Factory* GLFWComponentNative::getFactory()
{
    return context->factory();
}

//==============================================================================

void* GLFWComponentNative::getNativeHandle() const
{
    jassert (window != nullptr);

   #if JUCE_MAC
    return (__bridge void*) glfwGetCocoaWindow (window);

   #elif JUCE_WINDOWS
    return glfwGetWin32Window (window);

   #elif JUCE_LINUX
    return glfwGetX11Window (window);

   #else
    return nullptr;

   #endif
}

//==============================================================================

void GLFWComponentNative::run()
{
    const double maxFrameTimeSeconds = 1.0 / static_cast<double> (desiredFrameRate);
    const double maxFrameTimeMs = maxFrameTimeSeconds * 1000.0;

    double fpsMeasureStartTimeSeconds = Time::getMillisecondCounterHiRes() / 1000.0;
    uint64_t frameCounter = 0;

    while (! threadShouldExit())
    {
        double frameStartTimeSeconds = Time::getMillisecondCounterHiRes() / 1000.0;

        // Trigger and wait for rendering
        renderEvent.reset();
        triggerAsyncUpdate();
        renderEvent.wait (maxFrameTimeMs);

        // Measure spent time
        double currentTimeSeconds = Time::getMillisecondCounterHiRes() / 1000.0;
        double timeSpentSeconds = currentTimeSeconds - frameStartTimeSeconds;

        // Wait for a stable frame time
        if (renderContinuous)
        {
            const double secondsToWait = maxFrameTimeSeconds - timeSpentSeconds;
            if (secondsToWait > 0.0f)
            {
                const auto waitUntilMs = (currentTimeSeconds + secondsToWait) * 1000.0;

                while (Time::getMillisecondCounterHiRes() + 2.0 < waitUntilMs)
                    Thread::sleep (1);

                while (Time::getMillisecondCounterHiRes() < waitUntilMs)
                    Thread::sleep (0);
            }
        }
        else
        {
            commandEvent.wait();
        }

        // Measure current framerate
        ++frameCounter;

        const double timeSinceFpsMeasure = currentTimeSeconds - fpsMeasureStartTimeSeconds;
        if (timeSinceFpsMeasure >= 1.0)
        {
            double currentFps = frameCounter / timeSinceFpsMeasure;
            currentFrameRate.store (currentFps, std::memory_order_relaxed);

            fpsMeasureStartTimeSeconds = currentTimeSeconds;
            frameCounter = 0;
        }
    }
}

void GLFWComponentNative::handleAsyncUpdate()
{
    if (! isThreadRunning())
        return;

    renderContext();

    renderEvent.signal();
}

void GLFWComponentNative::timerCallback()
{
    renderContext();
}

//==============================================================================

void GLFWComponentNative::renderContext()
{
    auto [width, height] = getContentSize();

    if (currentWidth != width || currentHeight != height)
    {
        currentWidth = width;
        currentHeight = height;

        context->onSizeChanged (getNativeHandle(), width, height, 0);
        renderer = context->makeRenderer (width, height);
    }

    jassert (context != nullptr);
    jassert (renderer != nullptr);

    context->begin (
    {
        .renderTargetWidth = static_cast<uint32_t> (width),
        .renderTargetHeight = static_cast<uint32_t> (height),
        .clearColor = 0xff404040,
        .msaaSampleCount = 0,
        .disableRasterOrdering = renderAtomicMode,
        .wireframe = renderWireframe,
        .fillsDisabled = false,
        .strokesDisabled = false,
    });

    Graphics g (*context, *renderer);
    handlePaint (g, desiredFrameRate);

    context->end (getNativeHandle());

    context->tick();
}

//==============================================================================

void GLFWComponentNative::triggerRenderingUpdate()
{
    if (renderContinuous)
        return;

    commandEvent.signal();
}

//==============================================================================

void GLFWComponentNative::handlePaint (Graphics& g, float frameRate)
{
    component.internalPaint (g, frameRate);
}

//==============================================================================

void GLFWComponentNative::handleMouseMoveOrDrag (const Point<float>& localPosition)
{
    const auto event = MouseEvent()
        .withButtons (currentMouseButtons)
        .withModifiers (currentKeyModifiers)
        .withPosition (localPosition);

    if (lastComponentClicked != nullptr)
    {
        lastComponentClicked->internalMouseDrag (event
            .withSourceComponent (lastComponentClicked)
            //.withSourcePosition (lastMouseDownPosition)
        );
    }
    else
    {
        updateComponentUnderMouse (event);

        if (lastComponentUnderMouse != nullptr)
            lastComponentUnderMouse->internalMouseMove (event);
    }

    lastMouseMovePosition = localPosition;

    triggerRenderingUpdate();
}

void GLFWComponentNative::handleMouseDown (const Point<float>& localPosition, MouseEvent::Buttons button, KeyModifiers modifiers)
{
    currentMouseButtons = static_cast<MouseEvent::Buttons> (currentMouseButtons | button);
    currentKeyModifiers = modifiers;

    //DBG ("handleMouseDown: " << globalPosition);

    const auto event = MouseEvent()
        .withButtons (currentMouseButtons)
        .withModifiers (currentKeyModifiers)
        .withPosition (localPosition);

    if (lastComponentClicked == nullptr)
    {
        if (auto child = component.findComponentAt (localPosition))
        {
            lastComponentClicked = child;

            DBG ("handleMouseDown: " << lastComponentClicked->getTitle());
        }
    }

    if (lastComponentClicked != nullptr)
    {
        lastMouseDownPosition = localPosition;

        lastComponentClicked->internalMouseDown (event
            .withSourceComponent (lastComponentClicked)
            //.withSourcePosition (lastMouseDownPosition)
        );
    }

    lastMouseMovePosition = localPosition;

    triggerRenderingUpdate();
}

void GLFWComponentNative::handleMouseUp (const Point<float>& localPosition, MouseEvent::Buttons button, KeyModifiers modifiers)
{
    currentMouseButtons = static_cast<MouseEvent::Buttons> (currentMouseButtons & ~button);
    currentKeyModifiers = modifiers;

    const auto event = MouseEvent()
        .withButtons (currentMouseButtons)
        .withModifiers (currentKeyModifiers)
        .withPosition (localPosition);

    if (lastComponentClicked != nullptr)
    {
        DBG ("handleMouseUp: " << lastComponentClicked->getTitle());

        lastComponentClicked->internalMouseUp (event
            .withSourceComponent (lastComponentClicked)
            //.withSourcePosition (lastMouseDownPosition)
        );
    }

    if (currentMouseButtons == MouseEvent::noButtons)
    {
        updateComponentUnderMouse (event);

        lastComponentClicked = nullptr;
    }

    lastMouseMovePosition = localPosition;

    triggerRenderingUpdate();
}

//==============================================================================

void GLFWComponentNative::handleMouseWheel (const Point<float>& localPosition, const MouseWheelData& wheelData)
{
    const auto event = MouseEvent()
        .withButtons (currentMouseButtons)
        .withModifiers (currentKeyModifiers)
        .withPosition (localPosition);

    if (lastComponentClicked != nullptr)
    {
        DBG ("handleMouseWheel: " << lastComponentClicked->getTitle());

        lastComponentClicked->internalMouseWheel (event, wheelData);
    }
    else if (lastComponentFocused != nullptr)
    {
        DBG ("handleMouseWheel: " << lastComponentFocused->getTitle());

        lastComponentFocused->internalMouseWheel (event, wheelData);
    }

    triggerRenderingUpdate();
}

//==============================================================================

void GLFWComponentNative::handleKeyDown (const KeyPress& keys, const Point<float>& cursorPosition)
{
    currentKeyModifiers = keys.getModifiers();

    if (lastComponentFocused != nullptr)
        lastComponentFocused->internalKeyDown (keys, cursorPosition);
    else
        component.internalKeyDown (keys, cursorPosition);

    triggerRenderingUpdate();
}

void GLFWComponentNative::handleKeyUp (const KeyPress& keys, const Point<float>& cursorPosition)
{
    currentKeyModifiers = keys.getModifiers();

    if (lastComponentFocused != nullptr)
        lastComponentFocused->internalKeyUp (keys, cursorPosition);
    else
        component.internalKeyUp (keys, cursorPosition);

    triggerRenderingUpdate();
}

//==============================================================================

void GLFWComponentNative::handleMoved (int xpos, int ypos)
{
    //DBG ("handleMoved: " << xpos << ", " << ypos);
    //DBG ("  size:      " << getBounds());

    component.internalMoved (xpos, ypos);

    screenBounds = screenBounds.withPosition (xpos, ypos);
}

void GLFWComponentNative::handleResized (int width, int height)
{
    //DBG ("handleResized: " << width << ", " << height);
    //DBG ("  size:        " << getBounds());

    component.internalResized (width, height);

    screenBounds = screenBounds.withSize (width, height);

    triggerRenderingUpdate();
}

void GLFWComponentNative::handleFocusChanged (bool gotFocus)
{
    DBG ("handleFocusChanged: " << (gotFocus ? 1 : 0));

    triggerRenderingUpdate();
}

void GLFWComponentNative::handleUserTriedToCloseWindow()
{
    component.internalUserTriedToCloseWindow();
}

//==============================================================================

void GLFWComponentNative::updateComponentUnderMouse (const MouseEvent& event)
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

std::unique_ptr<ComponentNative> ComponentNative::createFor (Component& component, bool continuousRepaint, std::optional<float> framerateRedraw)
{
    return std::make_unique<GLFWComponentNative> (component, continuousRepaint, framerateRedraw);
}

//==============================================================================

void yup_glfwWindowClose (GLFWwindow* window)
{
    auto* nativeComponent = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    nativeComponent->handleUserTriedToCloseWindow();
}

void yup_glfwWindowPos (GLFWwindow* window, int xpos, int ypos)
{
    auto* nativeComponent = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    nativeComponent->handleMoved (xpos, ypos);
}

void yup_glfwWindowSize (GLFWwindow* window, int width, int height)
{
    auto* nativeComponent = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    nativeComponent->handleResized (width, height);
}

void yup_glfwWindowFocus (GLFWwindow* window, int focused)
{
    auto* nativeComponent = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    nativeComponent->handleFocusChanged (focused != 0);
}

void yup_glfwMouseMove (GLFWwindow* window, double x, double y)
{
    auto* nativeComponent = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    float dpiScale = nativeComponent->getScaleDpi();
    x *= dpiScale;
    y *= dpiScale;

    nativeComponent->handleMouseMoveOrDrag ({ static_cast<float> (x), static_cast<float> (y) });
}

void yup_glfwMousePress (GLFWwindow* window, int button, int action, int mods)
{
    auto* nativeComponent = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    auto cursorPosition = nativeComponent->getScaledCursorPosition();

    if (action == GLFW_PRESS)
        nativeComponent->handleMouseDown (cursorPosition, toMouseButton (button), toKeyModifiers (mods));
    else
        nativeComponent->handleMouseUp (cursorPosition, toMouseButton (button), toKeyModifiers (mods));
}

void yup_glfwMouseScroll (GLFWwindow* window, double xoffset, double yoffset)
{
    auto* nativeComponent = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    auto cursorPosition = nativeComponent->getScaledCursorPosition();

    nativeComponent->handleMouseWheel (cursorPosition, { static_cast<float> (xoffset), static_cast<float> (yoffset) });
}

void yup_glfwKeyPress (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* nativeComponent = static_cast<GLFWComponentNative*> (glfwGetWindowUserPointer (window));

    auto cursorPosition = nativeComponent->getScaledCursorPosition();

    if (action == GLFW_PRESS)
    {
        mods |= keyToModifier (key);
        nativeComponent->handleKeyDown (toKeyPress (key, scancode, mods), cursorPosition);
    }
    else
    {
        mods &= ~keyToModifier (key);
        nativeComponent->handleKeyUp (toKeyPress (key, scancode, mods), cursorPosition);
    }
}

//==============================================================================

void yup_glfwMonitorCallback (GLFWmonitor* monitor, int event)
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
       #if !(JUCE_EMSCRIPTEN && RIVE_WEBGL)
        glfwSetMonitorUserPointer (monitor, display.get());
       #endif

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

void yup_glfwErrorCallback (int code, const char* message)
{
    DBG ("GLFW Error: " << code << " - " << message);
}

void YUPApplication::staticInitialisation()
{
    glfwSetErrorCallback (yup_glfwErrorCallback);

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

    glfwSetMonitorCallback (yup_glfwMonitorCallback);
}

void YUPApplication::staticFinalisation()
{
    glfwTerminate();
}

} // namespace yup
