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

namespace
{

//==============================================================================

constexpr uint32 sdlDefaultSubsystems = SDL_INIT_VIDEO | SDL_INIT_EVENTS;

String getSDLVersionString (int version)
{
    return String (SDL_VERSIONNUM_MAJOR (version)) + "."
         + String (SDL_VERSIONNUM_MINOR (version)) + "."
         + String (SDL_VERSIONNUM_MICRO (version));
}

//==============================================================================

bool displayEventDispatcher (void* userdata, SDL_Event* event)
{
    if (auto* messageManager = MessageManager::getInstanceWithoutCreating();
        messageManager != nullptr && ! messageManager->isThisTheMessageThread())
    {
        MessageManager::callAsync ([eventCopy = *event]() mutable
        {
            if (auto* desktop = Desktop::getInstanceWithoutCreating())
                displayEventDispatcher (desktop, &eventCopy);
        });

        return true;
    }

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

            const SDL_Point pt { static_cast<int> (x), static_cast<int> (y) };
            const auto displayScale = getDisplayUnitsPerPoint (SDL_GetDisplayForPoint (&pt));

            auto cursorPosition = Point<float> { x / displayScale, y / displayScale };
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

            const SDL_Point pt { static_cast<int> (x), static_cast<int> (y) };
            const auto displayScale = getDisplayUnitsPerPoint (SDL_GetDisplayForPoint (&pt));

            auto cursorPosition = Point<float> { x / displayScale, y / displayScale };
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

            const SDL_Point pt { static_cast<int> (x), static_cast<int> (y) };
            const auto displayScale = getDisplayUnitsPerPoint (SDL_GetDisplayForPoint (&pt));

            auto cursorPosition = Point<float> { x / displayScale, y / displayScale };
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

            const SDL_Point pt { static_cast<int> (x), static_cast<int> (y) };
            const auto displayScale = getDisplayUnitsPerPoint (SDL_GetDisplayForPoint (&pt));

            auto cursorPosition = Point<float> { x / displayScale, y / displayScale };
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

YUP_API void YUP_CALLTYPE initialiseYup_Windowing()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: initialising windowing");

    const int compiledVersion = SDL_VERSION;
    const int linkedVersion = SDL_GetVersion();

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: compiled version=" << getSDLVersionString (compiledVersion) << ", linked version=" << getSDLVersionString (linkedVersion));

    // Do not install signal handlers
    SDL_SetHint (SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: disabled SDL signal handlers");

    // Initialise SDL
    SDL_SetMainReady();
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: main marked ready");

    const auto alreadyInitialised = SDL_WasInit (sdlDefaultSubsystems);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: requested subsystems=" << String::toHexString (static_cast<int> (sdlDefaultSubsystems)) << ", already initialised=" << String::toHexString (static_cast<int> (alreadyInitialised)));

    if ((alreadyInitialised & sdlDefaultSubsystems) != sdlDefaultSubsystems)
    {
        if (! SDL_InitSubSystem (sdlDefaultSubsystems))
        {
            YUP_MODULE_DBG (GUI_WINDOWING, "SDL: error initialising SDL: " << SDL_GetError());

            jassertfalse;
            YUPApplicationBase::quit();

            return;
        }

        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: subsystems initialised");
    }
    else
    {
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: subsystems were already initialised");
    }

    // Update available displays
    Desktop::getInstance()->updateScreens();
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: updated screens");

    SDL_AddEventWatch (displayEventDispatcher, Desktop::getInstance());
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: registered display event watch");

    // Set the default theme now in all platforms except ios
#if ! YUP_IOS
    ApplicationTheme::setGlobalTheme (createThemeVersion1());
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: registered default theme");
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
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: registered default theme");
    }
#endif

    SDLComponentNative::isInitialised.test_and_set();
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: windowing initialised");
}

//==============================================================================

YUP_API void YUP_CALLTYPE shutdownYup_Windowing()
{
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: shutting down windowing");

    SDLComponentNative::isInitialised.clear();

    // Shutdown desktop
    if (auto desktop = Desktop::getInstanceWithoutCreating())
    {
        SDL_RemoveEventWatch (displayEventDispatcher, desktop);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unregistered display event watch");

        desktop->deleteInstance();
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: deleted desktop instance");
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

    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unregistered default theme");

    // Unregister event loop
    if (messageManager != nullptr)
    {
        messageManager->registerEventLoopCallback (nullptr);
        YUP_MODULE_DBG (GUI_WINDOWING, "SDL: unregistered event loop callback");
    }

    // Quit only the subsystems YUP initialised.
    SDL_QuitSubSystem (sdlDefaultSubsystems);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: subsystems quit");

#if YUP_STANDALONE_APPLICATION
    std::atexit (&SDL_Quit);
    YUP_MODULE_DBG (GUI_WINDOWING, "SDL: registered SDL_Quit at exit");
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
