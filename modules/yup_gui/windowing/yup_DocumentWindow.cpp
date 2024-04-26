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

namespace juce
{

void juce_glfwErrorCallback (int code, const char* message)
{
    DBG ("GLFW Error: " << code << " - " << message);
}

void juce_glfwInitialiseWindowing()
{
    static struct ScopedGlfwHandler
    {
        ScopedGlfwHandler()
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
        }

        ~ScopedGlfwHandler()
        {
            glfwTerminate();
        }

    } glfwInstance = {};
}

void juce_glfwMouseMove (GLFWwindow* window, double x, double y)
{
    //float dpiScale = s_fiddleContext->dpiScale(glfwGetCocoaWindow(s_window));
    //x *= dpiScale;
    //y *= dpiScale;

    auto* documentWindow = static_cast<juce::DocumentWindow*> (glfwGetWindowUserPointer (window));

    if (glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS
        || glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS
        || glfwGetMouseButton (window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        documentWindow->mouseDrag (0, 0, x, y);
    }
    else
    {
        documentWindow->mouseMove (0, 0, x, y);
    }
}

void juce_glfwMousePress (GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    glfwGetCursorPos (window, &x, &y);

    //float dpiScale = s_fiddleContext->dpiScale(glfwGetCocoaWindow(s_window));
    //x *= dpiScale;
    //y *= dpiScale;

    auto* documentWindow = static_cast<juce::DocumentWindow*> (glfwGetWindowUserPointer (window));

    if (action == GLFW_PRESS)
        documentWindow->mouseDown (button, mods, x, y);
    else
        documentWindow->mouseUp (button, mods, x, y);
}

void juce_glfwKeyPress (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    double x = 0, y = 0;
    glfwGetCursorPos (window, &x, &y);

    //float dpiScale = s_fiddleContext->dpiScale(glfwGetCocoaWindow(s_window));
    //x *= dpiScale;
    //y *= dpiScale;

    auto* documentWindow = static_cast<juce::DocumentWindow*> (glfwGetWindowUserPointer (window));

    if (action == GLFW_PRESS)
        documentWindow->keyDown (key, scancode, mods, x, y);
    else
        documentWindow->keyUp (key, scancode, mods, x, y);
}

struct DocumentWindow::HeavyweightWindow
{
    GLFWwindow* window = nullptr;

#if JUCE_MAC
    id<MTLDevice> gpu = nil;
    id<MTLCommandQueue> queue = nil;
    CAMetalLayer* swapchain = nullptr;
#endif
};

DocumentWindow::DocumentWindow()
    : heavyweightWindow (std::make_unique<HeavyweightWindow> ())
{
    juce_glfwInitialiseWindowing();

#if JUCE_MAC
    heavyweightWindow->gpu = MTLCreateSystemDefaultDevice();
    heavyweightWindow->queue = [heavyweightWindow->gpu newCommandQueue];
    heavyweightWindow->swapchain = [CAMetalLayer layer];
    heavyweightWindow->swapchain.device = heavyweightWindow->gpu;
    heavyweightWindow->swapchain.opaque = YES;
#endif

    heavyweightWindow->window = glfwCreateWindow (800, 800, "GLFW Metal", nullptr, nullptr);

#if JUCE_MAC
    NSWindow* nswindow = glfwGetCocoaWindow (heavyweightWindow->window);
    nswindow.contentView.layer = heavyweightWindow->swapchain;
    nswindow.contentView.wantsLayer = YES;
#endif

    glfwSetWindowUserPointer (heavyweightWindow->window, this);

    glfwSetCursorPosCallback (heavyweightWindow->window, juce_glfwMouseMove);
    glfwSetMouseButtonCallback (heavyweightWindow->window, juce_glfwMousePress);
    glfwSetKeyCallback (heavyweightWindow->window, juce_glfwKeyPress);
}

DocumentWindow::~DocumentWindow()
{
    jassert (heavyweightWindow != nullptr && heavyweightWindow->window != nullptr);

    glfwSetWindowUserPointer (heavyweightWindow->window, nullptr);

    glfwDestroyWindow (heavyweightWindow->window);
    heavyweightWindow->window = nullptr;
}

void DocumentWindow::setWindowTitle (const String& windowTitle)
{
    jassert (heavyweightWindow != nullptr && heavyweightWindow->window != nullptr);

    glfwSetWindowTitle (heavyweightWindow->window, windowTitle.toRawUTF8());
}

void DocumentWindow::setSize (int width, int height)
{
    jassert (heavyweightWindow != nullptr && heavyweightWindow->window != nullptr);

    glfwSetWindowSize (heavyweightWindow->window, width, height);
}

std::tuple<int, int> DocumentWindow::getSize() const
{
    jassert (heavyweightWindow != nullptr && heavyweightWindow->window != nullptr);

    int width = 0, height = 0;
    glfwGetFramebufferSize (heavyweightWindow->window, &width, &height);
    return std::make_tuple (width, height);
}

void DocumentWindow::setVisible (bool shouldBeVisible)
{
    jassert (heavyweightWindow != nullptr && heavyweightWindow->window != nullptr);

    if (shouldBeVisible)
    {
        // if (GL)
        //     glfwMakeContextCurrent (heavyweightWindow->window);

        glfwSetWindowAttrib (heavyweightWindow->window, GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
        glfwShowWindow (heavyweightWindow->window);
    }
    else
    {
        glfwHideWindow (heavyweightWindow->window);
    }
}

void DocumentWindow::close()
{
    jassert (heavyweightWindow != nullptr && heavyweightWindow->window != nullptr);

    glfwSetWindowShouldClose (heavyweightWindow->window, GLFW_TRUE);
}

bool DocumentWindow::shouldClose() const
{
    jassert (heavyweightWindow != nullptr && heavyweightWindow->window != nullptr);

    return glfwWindowShouldClose (heavyweightWindow->window) == 1;
}

void* DocumentWindow::nativeHandle() const
{
    jassert (heavyweightWindow != nullptr && heavyweightWindow->window != nullptr);

#if JUCE_MAC
    return glfwGetCocoaWindow (heavyweightWindow->window);
#elif JUCE_WINDOWS
    return glfwGetWin32Window (heavyweightWindow->window);
#else
    return nullptr;
#endif
}

void DocumentWindow::mouseDown(int button, int mods, double x, double y)
{
}

void DocumentWindow::mouseMove(int button, int mods, double x, double y)
{
}

void DocumentWindow::mouseUp(int button, int mods, double x, double y)
{
}

void DocumentWindow::mouseDrag(int button, int mods, double x, double y)
{
}

void DocumentWindow::keyDown(int key, int scancode, int mods, double x, double y)
{
}

void DocumentWindow::keyUp(int key, int scancode, int mods, double x, double y)
{
}

}