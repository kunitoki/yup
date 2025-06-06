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
class SDL2ComponentNative final
    : public ComponentNative
    , public Timer
    , public Thread
    , public AsyncUpdater
{
#if (JUCE_EMSCRIPTEN && RIVE_WEBGL) && ! defined(__EMSCRIPTEN_PTHREADS__)
    static constexpr bool renderDrivenByTimer = true;
#else
    static constexpr bool renderDrivenByTimer = false;
#endif

public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<SDL2ComponentNative>;

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
    void setSize (const Size<int>& newSize) override;
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
    void repaint() override;
    void repaint (const Rectangle<float>& rect) override;
    const RectangleList<float>& getRepaintAreas() const override;

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
    void handleMouseMoveOrDrag (const Point<float>& position);
    void handleMouseDown (const Point<float>& position, MouseEvent::Buttons button, KeyModifiers modifiers);
    void handleMouseUp (const Point<float>& position, MouseEvent::Buttons button, KeyModifiers modifiers);
    void handleMouseWheel (const Point<float>& position, const MouseWheelData& wheelData);
    void handleMouseEnter (const Point<float>& position);
    void handleMouseLeave (const Point<float>& position);
    void handleKeyDown (const KeyPress& keys, const Point<float>& position);
    void handleKeyUp (const KeyPress& keys, const Point<float>& position);
    void handleTextInput (const String& textInput);
    void handleMoved (int xpos, int ypos);
    void handleResized (int width, int height);
    void handleFocusChanged (bool gotFocus);
    void handleMinimized();
    void handleMaximized();
    void handleRestored();
    void handleExposed();
    void handleContentScaleChanged();
    void handleDisplayChanged();
    void handleUserTriedToCloseWindow();

    //==============================================================================
    void handleWindowEvent (const SDL_WindowEvent& windowEvent);

    //==============================================================================
    void handleEvent (SDL_Event* event);
    static int eventDispatcher (void* userdata, SDL_Event* event);

    //==============================================================================
    static std::atomic_flag isInitialised;

private:
    void updateComponentUnderMouse (const MouseEvent& event);
    void renderContext();

    void startRendering();
    void stopRendering();
    bool isRendering() const;

    SDL_Window* window = nullptr;
    SDL_GLContext windowContext = nullptr;

    void* parentWindow = nullptr;
    String windowTitle;
    uint32 windowFlags = 0;

    GraphicsContext::Api currentGraphicsApi;

    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<rive::Renderer> renderer;

    Color clearColor;
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

    RectangleList<float> currentRepaintAreas;

    float desiredFrameRate = 60.0f;
    std::atomic<float> currentFrameRate = 0.0f;
    double frameRateStartTimeSeconds = 0.0;
    uint64_t frameRateCounter = 0;

    int currentContentWidth = 0;
    int currentContentHeight = 0;
    bool internalBoundsChange = false;

    WaitableEvent renderEvent { true };
    std::atomic<bool> shouldRenderContinuous = false;
    double lastRenderTimeSeconds = 0.0;
    bool renderAtomicMode = false;
    bool renderWireframe = false;
    bool updateOnlyWhenFocused = false;
};

} // namespace yup
