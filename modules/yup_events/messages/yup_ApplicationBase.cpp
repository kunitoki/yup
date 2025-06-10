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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

YUPApplicationBase::CreateInstanceFunction YUPApplicationBase::createInstance = nullptr;
YUPApplicationBase* YUPApplicationBase::appInstance = nullptr;

#if YUP_IOS
void* YUPApplicationBase::iOSCustomDelegate = nullptr;
#endif

YUPApplicationBase::YUPApplicationBase()
{
    jassert (isStandaloneApp() && appInstance == nullptr);
    appInstance = this;
}

YUPApplicationBase::~YUPApplicationBase()
{
    jassert (appInstance == this);
    appInstance = nullptr;
}

void YUPApplicationBase::setApplicationReturnValue (const int newReturnValue) noexcept
{
    appReturnValue = newReturnValue;
}

// This is called on the Mac and iOS where the OS doesn't allow the stack to unwind on shutdown..
void YUPApplicationBase::appWillTerminateByForce()
{
    YUP_AUTORELEASEPOOL
    {
        {
            const std::unique_ptr<YUPApplicationBase> app (appInstance);

            if (app != nullptr)
                app->shutdownApp();
        }

        DeletedAtShutdown::deleteAll();
        MessageManager::deleteInstance();
    }
}

void YUPApplicationBase::quit()
{
    MessageManager::getInstance()->stopDispatchLoop();
}

void YUPApplicationBase::sendUnhandledException (const std::exception* const e,
                                                 const char* const sourceFile,
                                                 const int lineNumber)
{
    if (auto* app = YUPApplicationBase::getInstance())
    {
        // If you hit this assertion then the __FILE__ macro is providing a
        // relative path instead of an absolute path. On Windows this will be
        // a path relative to the build directory rather than the currently
        // running application. To fix this you must compile with the /FC flag.
        jassert (File::isAbsolutePath (sourceFile));

        app->unhandledException (e, sourceFile, lineNumber);
    }
}

//==============================================================================
#if ! (YUP_IOS || YUP_ANDROID || YUP_EMSCRIPTEN)
#define YUP_HANDLE_MULTIPLE_INSTANCES 1
#endif

#if YUP_HANDLE_MULTIPLE_INSTANCES
struct YUPApplicationBase::MultipleInstanceHandler final : public ActionListener
{
    MultipleInstanceHandler (const String& appName)
        : appLock ("juceAppLock_" + appName)
    {
    }

    bool sendCommandLineToPreexistingInstance()
    {
        if (appLock.enter (0))
            return false;

        if (auto* app = YUPApplicationBase::getInstance())
        {
            MessageManager::broadcastMessage (app->getApplicationName() + "/" + app->getCommandLineParameters());
            return true;
        }

        jassertfalse;
        return false;
    }

    void actionListenerCallback (const String& message) override
    {
        if (auto* app = YUPApplicationBase::getInstance())
        {
            auto appName = app->getApplicationName();

            if (message.startsWith (appName + "/"))
                app->anotherInstanceStarted (message.substring (appName.length() + 1));
        }
    }

private:
    InterProcessLock appLock;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultipleInstanceHandler)
};

bool YUPApplicationBase::sendCommandLineToPreexistingInstance()
{
    jassert (multipleInstanceHandler == nullptr); // this must only be called once!

    multipleInstanceHandler.reset (new MultipleInstanceHandler (getApplicationName()));
    return multipleInstanceHandler->sendCommandLineToPreexistingInstance();
}

#else
struct YUPApplicationBase::MultipleInstanceHandler
{
};
#endif

//==============================================================================
#if YUP_WINDOWS && ! defined(_CONSOLE)

String YUP_CALLTYPE YUPApplicationBase::getCommandLineParameters()
{
    return CharacterFunctions::findEndOfToken (CharPointer_UTF16 (GetCommandLineW()),
                                               CharPointer_UTF16 (L" "),
                                               CharPointer_UTF16 (L"\""))
        .findEndOfWhitespace();
}

StringArray YUP_CALLTYPE YUPApplicationBase::getCommandLineParameterArray()
{
    StringArray s;
    int argc = 0;

    if (auto argv = CommandLineToArgvW (GetCommandLineW(), &argc))
    {
        s = StringArray (argv + 1, argc - 1);
        LocalFree (argv);
    }

    return s;
}

#else

#if YUP_IOS && YUP_MODULE_AVAILABLE_yup_gui
extern int yup_iOSMain (int argc, const char* argv[], void* classPtr);
#endif

#if YUP_MAC
extern void initialiseNSApplication();
#endif

#if YUP_WINDOWS || YUP_ANDROID
const char* const* yup_argv = nullptr;
int yup_argc = 0;
#else
extern const char* const* yup_argv; // declared in yup_core
extern int yup_argc;
#endif

String YUPApplicationBase::getCommandLineParameters()
{
    String argString;

    for (const auto& arg : getCommandLineParameterArray())
    {
        const auto withQuotes = arg.containsChar (' ') && ! arg.isQuotedString()
                                  ? arg.quoted ('"')
                                  : arg;
        argString << withQuotes << ' ';
    }

    return argString.trim();
}

StringArray YUPApplicationBase::getCommandLineParameterArray()
{
    StringArray result;

    for (int i = 1; i < yup_argc; ++i)
        result.add (CharPointer_UTF8 (yup_argv[i]));

    return result;
}

int YUPApplicationBase::main (int argc, const char* argv[])
{
    YUP_AUTORELEASEPOOL
    {
        yup_argc = argc;
        yup_argv = argv;

#if YUP_MAC
        initialiseNSApplication();
#endif

#if YUP_IOS && YUP_MODULE_AVAILABLE_yup_gui
        return yup_iOSMain (argc, argv, iOSCustomDelegate);
#else

        return YUPApplicationBase::main();
#endif
    }
}

#endif

//==============================================================================
#if YUP_ANDROID
extern "C" jint JNIEXPORT yup_JNI_OnLoad (JavaVM* vm, void*);
#endif

int YUPApplicationBase::main()
{
#if YUP_ANDROID
    auto env = (JNIEnv*) SDL_AndroidGetJNIEnv();
    auto clazz = (jobject) SDL_AndroidGetActivity();
    JavaVM* vm = nullptr;

    if (env != nullptr && env->GetJavaVM (&vm) == 0)
    {
        yup_JNI_OnLoad (vm, nullptr);

        JNIClassBase::initialiseAllClasses (env, clazz);
        Thread::initialiseYUP (env, clazz);
    }
#endif

    ScopedJuceInitialiser_GUI libraryInitialiser;
    jassert (createInstance != nullptr);

    const std::unique_ptr<YUPApplicationBase> app (createInstance());
    jassert (app != nullptr);

    if (! app->initialiseApp())
        return app->shutdownApp();

    YUP_TRY
    {
        // loop until a quit message is received..
        MessageManager::getInstance()->runDispatchLoop();
    }
    YUP_CATCH_EXCEPTION

    return app->shutdownApp();
}

//==============================================================================
bool YUPApplicationBase::initialiseApp()
{
#if YUP_HANDLE_MULTIPLE_INSTANCES
    if ((! moreThanOneInstanceAllowed()) && sendCommandLineToPreexistingInstance())
    {
        YUP_DBG ("Another instance is running - quitting...");
        return false;
    }
#endif

#if YUP_WINDOWS && (! defined(_CONSOLE)) && (! YUP_MINGW)
    if (isStandaloneApp() && AttachConsole (ATTACH_PARENT_PROCESS) != 0)
    {
        // if we've launched a GUI app from cmd.exe or PowerShell, we need this to enable printf etc.
        // However, only reassign stdout, stderr, stdin if they have not been already opened by
        // a redirect or similar.
        FILE* ignore;

        if (_fileno (stdout) < 0)
            freopen_s (&ignore, "CONOUT$", "w", stdout);
        if (_fileno (stderr) < 0)
            freopen_s (&ignore, "CONOUT$", "w", stderr);
        if (_fileno (stdin) < 0)
            freopen_s (&ignore, "CONIN$", "r", stdin);
    }
#endif

    // let the app do its setting-up..
    initialise (getCommandLineParameters());

    stillInitialising = false;

    if (MessageManager::getInstance()->hasStopMessageBeenSent())
        return false;

#if YUP_HANDLE_MULTIPLE_INSTANCES
    if (auto* mih = multipleInstanceHandler.get())
        MessageManager::getInstance()->registerBroadcastListener (mih);
#endif

    return true;
}

int YUPApplicationBase::shutdownApp()
{
    jassert (YUPApplicationBase::getInstance() == this);

#if YUP_HANDLE_MULTIPLE_INSTANCES
    if (auto* mih = multipleInstanceHandler.get())
        MessageManager::getInstance()->deregisterBroadcastListener (mih);
#endif

    YUP_TRY
    {
        // give the app a chance to clean up..
        shutdown();
    }
    YUP_CATCH_EXCEPTION

    multipleInstanceHandler.reset();
    return getApplicationReturnValue();
}

} // namespace yup
