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

//==============================================================================
/** Initialises YUP's GUI classes.

    If you're embedding YUP into an application that uses its own event-loop rather
    than using the START_YUP_APPLICATION macro, call this function before making any
    YUP calls, to make sure things are initialised correctly.

    Note that if you're creating a YUP DLL for Windows, you may also need to call the
    Process::setCurrentModuleInstanceHandle() method.

    @see shutdownYup_GUI()
*/
YUP_API void YUP_CALLTYPE initialiseYup_GUI();

/** Clears up any static data being used by YUP's GUI classes.

    If you're embedding YUP into an application that uses its own event-loop rather
    than using the START_YUP_APPLICATION macro, call this function in your shutdown
    code to clean up any YUP objects that might be lying around.

    @see initialiseYup_GUI()
*/
YUP_API void YUP_CALLTYPE shutdownYup_GUI();

//==============================================================================
/** A utility object that helps you initialise and shutdown YUP correctly
    using an RAII pattern.

    When the first instance of this class is created, it calls initialiseYup_GUI(),
    and when the last instance is deleted, it calls shutdownYup_GUI(), so that you
    can easily be sure that as long as at least one instance of the class exists, the
    library will be initialised.

    This class is particularly handy to use at the beginning of a console app's
    main() function, because it'll take care of shutting down whenever you return
    from the main() call.

    Be careful with your threading though - to be safe, you should always make sure
    that these objects are created and deleted on the message thread.

    @tags{Events}
*/
class YUP_API ScopedYupInitialiser_GUI final
{
   public:
    /** The constructor simply calls initialiseYup_GUI(). */
    ScopedYupInitialiser_GUI();

    /** The destructor simply calls shutdownYup_GUI(). */
    ~ScopedYupInitialiser_GUI();

    YUP_DECLARE_NON_COPYABLE(ScopedYupInitialiser_GUI)
    YUP_DECLARE_NON_MOVEABLE(ScopedYupInitialiser_GUI)
};

//==============================================================================
/**
    To start a YUP app, use this macro: START_YUP_APPLICATION (AppSubClass) where
    AppSubClass is the name of a class derived from YUPApplication or YUPApplicationBase.

    See the YUPApplication and YUPApplicationBase class documentation for more details.
*/
#if DOXYGEN
#define START_YUP_APPLICATION(AppClass)
#else
#if YUP_WINDOWS && !defined(_CONSOLE)
#define YUP_MAIN_FUNCTION                                                      \
    YUP_BEGIN_IGNORE_WARNINGS_MSVC(28251)                                      \
    int __stdcall WinMain(struct HINSTANCE__*, struct HINSTANCE__*, char*, int) \
        YUP_END_IGNORE_WARNINGS_MSVC
#define YUP_MAIN_FUNCTION_ARGS
#elif YUP_ANDROID && YUP_MODULE_AVAILABLE_yup_gui
#define YUP_MAIN_FUNCTION extern "C" int SDL_main(int argc, char* argv[])
#define YUP_MAIN_FUNCTION_ARGS argc, (const char**)argv
#else
#define YUP_MAIN_FUNCTION int main(int argc, char* argv[])
#define YUP_MAIN_FUNCTION_ARGS argc, (const char**)argv
#endif

#if YUP_IOS

#define YUP_CREATE_APPLICATION_DEFINE(AppClass)                \
    YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wmissing-prototypes") \
    yup::YUPApplicationBase* yup_CreateApplication()         \
    {                                                           \
        return new AppClass();                                  \
    }                                                           \
    void* yup_GetIOSCustomDelegateClass()                      \
    {                                                           \
        return nullptr;                                         \
    }                                                           \
    YUP_END_IGNORE_WARNINGS_GCC_LIKE

#define YUP_CREATE_APPLICATION_DEFINE_CUSTOM_DELEGATE(AppClass, DelegateClass) \
    YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wmissing-prototypes")                 \
    yup::YUPApplicationBase* yup_CreateApplication()                         \
    {                                                                           \
        return new AppClass();                                                  \
    }                                                                           \
    void* yup_GetIOSCustomDelegateClass()                                      \
    {                                                                           \
        return [DelegateClass class];                                           \
    }                                                                           \
    YUP_END_IGNORE_WARNINGS_GCC_LIKE

#define YUP_MAIN_FUNCTION_DEFINITION                                                    \
    YUP_MAIN_FUNCTION                                                                   \
    {                                                                                    \
        yup::YUPApplicationBase::createInstance = &yup_CreateApplication;             \
        yup::YUPApplicationBase::iOSCustomDelegate = yup_GetIOSCustomDelegateClass(); \
        return yup::YUPApplicationBase::main(YUP_MAIN_FUNCTION_ARGS);                 \
    }

#else

#define YUP_CREATE_APPLICATION_DEFINE(AppClass)         \
    yup::YUPApplicationBase* yup_CreateApplication(); \
    yup::YUPApplicationBase* yup_CreateApplication()  \
    {                                                    \
        return new AppClass();                           \
    }

#define YUP_MAIN_FUNCTION_DEFINITION                                        \
    YUP_MAIN_FUNCTION                                                       \
    {                                                                        \
        yup::YUPApplicationBase::createInstance = &yup_CreateApplication; \
        return yup::YUPApplicationBase::main(YUP_MAIN_FUNCTION_ARGS);     \
    }

#endif

#if YupPlugin_Build_Standalone
#if YUP_USE_CUSTOM_PLUGIN_STANDALONE_APP
#define START_YUP_APPLICATION(AppClass) YUP_CREATE_APPLICATION_DEFINE(AppClass)
#if YUP_IOS
#define START_YUP_APPLICATION_WITH_CUSTOM_DELEGATE(AppClass, DelegateClass) YUP_CREATE_APPLICATION_DEFINE_CUSTOM_DELEGATE(AppClass, DelegateClass)
#endif
#else
#define START_YUP_APPLICATION(AppClass) static_assert(false, "You are trying to use START_YUP_APPLICATION in an audio plug-in. Define YUP_USE_CUSTOM_PLUGIN_STANDALONE_APP=1 if you want to use a custom standalone target app.");
#if YUP_IOS
#define START_YUP_APPLICATION_WITH_CUSTOM_DELEGATE(AppClass, DelegateClass) static_assert(false, "You are trying to use START_YUP_APPLICATION in an audio plug-in. Define YUP_USE_CUSTOM_PLUGIN_STANDALONE_APP=1 if you want to use a custom standalone target app.");
#endif
#endif
#else

#define START_YUP_APPLICATION(AppClass)                        \
    YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wmissing-prototypes") \
    YUP_CREATE_APPLICATION_DEFINE(AppClass)                    \
    YUP_MAIN_FUNCTION_DEFINITION                               \
    YUP_END_IGNORE_WARNINGS_GCC_LIKE

#if YUP_IOS
/**
   You can instruct YUP to use a custom iOS app delegate class instead of YUP's default
   app delegate. For YUP to work you must pass all messages to YUP's internal app delegate.
   Below is an example of minimal forwarding custom delegate. Note that you are at your own
   risk if you decide to use your own delegate and subtle, hard to debug bugs may occur.

   @interface MyCustomDelegate : NSObject <UIApplicationDelegate> { NSObject<UIApplicationDelegate>* yupDelegate; } @end

   @implementation MyCustomDelegate

   -(id) init
   {
       self = [super init];
       yupDelegate = reinterpret_cast<NSObject<UIApplicationDelegate>*> ([[NSClassFromString (@"YupAppStartupDelegate") alloc] init]);
       return self;
   }

   -(void) dealloc
   {
       [yupDelegate release];
       [super dealloc];
   }

   - (void) forwardInvocation: (NSInvocation*) anInvocation
   {
       if (yupDelegate != nullptr && [yupDelegate respondsToSelector: [anInvocation selector]])
           [anInvocation invokeWithTarget: yupDelegate];
       else
           [super forwardInvocation: anInvocation];
   }

   -(BOOL) respondsToSelector: (SEL) aSelector
   {
       if (yupDelegate != nullptr && [yupDelegate respondsToSelector: aSelector])
           return YES;

       return [super respondsToSelector: aSelector];
   }
   @end
*/
#define START_YUP_APPLICATION_WITH_CUSTOM_DELEGATE(AppClass, DelegateClass) \
    YUP_CREATE_APPLICATION_DEFINE_CUSTOM_DELEGATE(AppClass, DelegateClass)  \
    YUP_MAIN_FUNCTION_DEFINITION
#endif
#endif
#endif

} // namespace yup
