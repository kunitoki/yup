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

#pragma once

namespace yup
{

//==============================================================================
/** This file defines miscellaneous macros for debugging, assertions, etc. */

//==============================================================================
#ifdef YUP_FORCE_DEBUG
#undef YUP_DEBUG

#if YUP_FORCE_DEBUG
#define YUP_DEBUG 1
#endif
#endif

/** This macro defines the C calling convention used as the standard for YUP calls. */
#if YUP_WINDOWS
#define YUP_CALLTYPE __stdcall
#define YUP_CDECL __cdecl
#else
#define YUP_CALLTYPE
#define YUP_CDECL
#endif

//==============================================================================
#ifndef DOXYGEN
#define YUP_JOIN_MACRO_HELPER(a, b) a##b
#define YUP_STRINGIFY_MACRO_HELPER(a) #a
#endif

/** A good old-fashioned C macro concatenation helper.
    This combines two items (which may themselves be macros) into a single string,
    avoiding the pitfalls of the ## macro operator. */
#define YUP_JOIN_MACRO(item1, item2) YUP_JOIN_MACRO_HELPER (item1, item2)

/** A handy C macro for stringifying any symbol, rather than just a macro parameter. */
#define YUP_STRINGIFY(item) YUP_STRINGIFY_MACRO_HELPER (item)

//==============================================================================
// Debugging and assertion macros
#ifndef YUP_LOG_CURRENT_ASSERTION
#if YUP_LOG_ASSERTIONS || YUP_DEBUG
#define YUP_LOG_CURRENT_ASSERTION yup::logAssertion (YUP_JOIN_MACRO (L, __FILE__), __LINE__);
#else
#define YUP_LOG_CURRENT_ASSERTION
#endif
#endif

//==============================================================================
// clang-format off
#if YUP_IOS || YUP_LINUX || YUP_BSD
/** This will try to break into the debugger if the app is currently being debugged.

    If called by an app that's not being debugged, the behaviour isn't defined - it may
    crash or not, depending on the platform.

    @see jassert()
*/
#define YUP_BREAK_IN_DEBUGGER ::kill (0, SIGTRAP);
#elif YUP_WASM
#define YUP_BREAK_IN_DEBUGGER
#elif YUP_MSVC
#ifndef __INTEL_COMPILER
#pragma intrinsic(__debugbreak)
#endif
#define YUP_BREAK_IN_DEBUGGER __debugbreak();
#elif YUP_INTEL && (YUP_GCC || YUP_CLANG || YUP_MAC)
#if YUP_NO_INLINE_ASM
#define YUP_BREAK_IN_DEBUGGER
#else
#define YUP_BREAK_IN_DEBUGGER asm ("int $3");
#endif
#elif YUP_ARM && YUP_MAC
#define YUP_BREAK_IN_DEBUGGER __builtin_debugtrap();
#elif YUP_ANDROID
#define YUP_BREAK_IN_DEBUGGER __builtin_trap();
#else
#define YUP_BREAK_IN_DEBUGGER __asm int 3;
#endif
// clang-format on

#if YUP_CLANG && defined(__has_feature) && ! defined(YUP_ANALYZER_NORETURN)
#if __has_feature(attribute_analyzer_noreturn)
inline void __attribute__ ((analyzer_noreturn)) yup_assert_noreturn()
{
}

#define YUP_ANALYZER_NORETURN yup::yup_assert_noreturn();
#endif
#endif

#ifndef YUP_ANALYZER_NORETURN
#define YUP_ANALYZER_NORETURN
#endif

/** Used to silence Wimplicit-fallthrough on Clang and GCC where available
    as there are a few places in the codebase where we need to do this
    deliberately and want to ignore the warning. */
#if YUP_CLANG
#if __has_cpp_attribute(clang::fallthrough)
#define YUP_FALLTHROUGH [[clang::fallthrough]];
#else
#define YUP_FALLTHROUGH
#endif
#elif YUP_GCC
#if __GNUC__ >= 7
#define YUP_FALLTHROUGH [[gnu::fallthrough]];
#else
#define YUP_FALLTHROUGH
#endif
#else
#define YUP_FALLTHROUGH
#endif

//==============================================================================
#if defined(__has_feature)
#define YUP_HAS_FEATURE(feature) __has_feature (feature)
#else
#define YUP_HAS_FEATURE(feature) 0
#endif

#if defined(__has_attribute)
#define YUP_HAS_ATTRIBUTE(attribute) __has_attribute (attribute)
#else
#define YUP_HAS_ATTRIBUTE(attribute) 0
#endif

#if defined(__has_builtin)
#define YUP_HAS_BUILTIN(builtin) __has_builtin (builtin)
#else
#define YUP_HAS_BUILTIN(builtin) 0
#endif

//==============================================================================
#if __cpp_lib_is_constant_evaluated >= 201811L
#define YUP_STL_FEATURE_IS_CONSTANT_EVALUATED 1
#else
#define YUP_STL_FEATURE_IS_CONSTANT_EVALUATED 0
#endif

#if defined(YUP_MSVC) || YUP_HAS_BUILTIN(__builtin_is_constant_evaluated)
#define YUP_FEATURE_BUILTIN_IS_CONSTANT_EVALUATED 1
#else
#define YUP_FEATURE_BUILTIN_IS_CONSTANT_EVALUATED 0
#endif

constexpr bool isConstantEvaluated() noexcept
{
#if YUP_STL_FEATURE_IS_CONSTANT_EVALUATED
    return std::is_constant_evaluated();
#elif YUP_FEATURE_BUILTIN_IS_CONSTANT_EVALUATED
    return __builtin_is_constant_evaluated();
#else
    return false;
#endif
}

//==============================================================================
// clang-format off
#if YUP_MSVC && ! defined(DOXYGEN)
#define YUP_BLOCK_WITH_FORCED_SEMICOLON(x) \
    __pragma (warning (push))               \
    __pragma (warning (disable : 4127))     \
    __pragma (warning (disable : 4390))     \
    do { x } while (false)                  \
    __pragma (warning (pop))
#else
/** This is the good old C++ trick for creating a macro that forces the user to put
    a semicolon after it when they use it.
*/
#define YUP_BLOCK_WITH_FORCED_SEMICOLON(x) \
    do { x } while (false)
#endif
// clang-format on

//==============================================================================
// clang-format off
#if (YUP_DEBUG && ! YUP_DISABLE_ASSERTIONS) || DOXYGEN
/** Assertion are enabled in debug unless explicitly disabled. */
#define YUP_ASSERTIONS_ENABLED 1

/** Writes a string to the standard error stream.

    Note that as well as a single string, you can use this to write multiple items as a stream, e.g.

    @code
        YUP_DBG ("foo = " << foo << "bar = " << bar);
    @endcode

    The macro is only enabled in a debug build, so be careful not to use it with expressions
    that have important side-effects!

    @see Logger::outputDebugString
*/
#define YUP_DBG(textToWrite) YUP_BLOCK_WITH_FORCED_SEMICOLON (\
    yup::String tempDbgBuf;                                    \
    tempDbgBuf << textToWrite;                                  \
    yup::Logger::outputDebugString (tempDbgBuf);)

//==============================================================================
/** This will always cause an assertion failure.
    It is only compiled in a debug build, (unless YUP_LOG_ASSERTIONS is enabled for your build).

    @see jassert
*/
#define jassertfalse YUP_BLOCK_WITH_FORCED_SEMICOLON (\
    if (! yup::isConstantEvaluated())                 \
    {                                                  \
        YUP_LOG_CURRENT_ASSERTION;                    \
        if (yup::yup_isRunningUnderDebugger())       \
            { YUP_BREAK_IN_DEBUGGER }                 \
        else                                           \
            { YUP_ANALYZER_NORETURN }                 \
    }                                                  \
    else                                               \
    {                                                  \
        YUP_ANALYZER_NORETURN                         \
    })

//==============================================================================
/** Platform-independent assertion macro.

    This macro gets turned into a no-op when you're building with debugging turned off, so be
    careful that the expression you pass to it doesn't perform any actions that are vital for the
    correct behaviour of your program!

    @see jassertfalse
*/
#define jassert(expression) YUP_BLOCK_WITH_FORCED_SEMICOLON (if (! (expression)) jassertfalse;)

/** Platform-independent assertion macro which suppresses ignored-variable
    warnings in all build modes. You should probably use a plain jassert()
    and `[[maybe_unused]]` by default.
*/
#define jassertquiet(expression) YUP_BLOCK_WITH_FORCED_SEMICOLON (if (! (expression)) jassertfalse;)

#else
//==============================================================================
/** If debugging is disabled, these dummy debug and assertion macros are used. */
#define YUP_ASSERTIONS_ENABLED 0

#define YUP_DBG(textToWrite)
#define jassertfalse YUP_BLOCK_WITH_FORCED_SEMICOLON (if (! yup::isConstantEvaluated()) YUP_LOG_CURRENT_ASSERTION;)

#if YUP_LOG_ASSERTIONS
#define jassert(expression) YUP_BLOCK_WITH_FORCED_SEMICOLON (if (! (expression)) jassertfalse;)
#define jassertquiet(expression) YUP_BLOCK_WITH_FORCED_SEMICOLON (if (! (expression)) jassertfalse;)
#else
#define jassert(expression) YUP_BLOCK_WITH_FORCED_SEMICOLON (;)
#define jassertquiet(expression) YUP_BLOCK_WITH_FORCED_SEMICOLON (if (false) (void) (expression);)
#endif

#endif
// clang-format on

#define YUP_ASSERTIONS_ENABLED_OR_LOGGED YUP_ASSERTIONS_ENABLED || YUP_LOG_ASSERTIONS

//==============================================================================
/** This is a shorthand macro for deleting a class's copy constructor and copy assignment operator.

    For example, instead of

    @code
        class MyClass
        {
            etc..

        private:
            MyClass (const MyClass&);
            MyClass& operator= (const MyClass&);
        };
    @endcode

    ..you can just write:

    @code
        class MyClass
        {
            etc..

        private:
            YUP_DECLARE_NON_COPYABLE (MyClass)
        };
    @endcode
*/
#define YUP_DECLARE_NON_COPYABLE(className) \
    className (const className&) = delete;   \
    className& operator= (const className&) = delete;

/** This is a shorthand macro for deleting a class's move constructor and
    move assignment operator. */
#define YUP_DECLARE_NON_MOVEABLE(className) \
    className (className&&) = delete;        \
    className& operator= (className&&) = delete;

/** This is a shorthand way of writing both a YUP_DECLARE_NON_COPYABLE and
    YUP_LEAK_DETECTOR macro for a class. */
#define YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(className) \
    YUP_DECLARE_NON_COPYABLE (className)                       \
    YUP_LEAK_DETECTOR (className)

/** This macro can be added to class definitions to disable the use of new/delete to
    allocate the object on the heap, forcing it to only be used as a stack or member variable. */
#define YUP_PREVENT_HEAP_ALLOCATION             \
private:                                         \
    static void* operator new (size_t) = delete; \
    static void operator delete (void*) = delete;

//==============================================================================
#if YUP_MSVC && ! defined(DOXYGEN)
#define YUP_COMPILER_WARNING(msg) __pragma (message (__FILE__ "(" YUP_STRINGIFY (__LINE__) ") : Warning: " msg))
#else
/** This macro allows you to emit a custom compiler warning message.

    Very handy for marking bits of code as "to-do" items, or for shaming
    code written by your co-workers in a way that's hard to ignore.

    GCC and Clang provide the \#warning directive, but MSVC doesn't, so this macro
    is a cross-compiler way to get the same functionality as \#warning.
*/
#define YUP_COMPILER_WARNING(msg) _Pragma (YUP_STRINGIFY (message (msg)))
#endif

//==============================================================================
#if YUP_DEBUG || DOXYGEN
/** A platform-independent way of forcing an inline function.

    Use the syntax:

    @code
      forcedinline void myfunction (int x)
    @endcode
*/
#define forcedinline inline
#else
#if YUP_MSVC
#define forcedinline __forceinline
#else
#define forcedinline inline __attribute__ ((always_inline))
#endif
#endif

#if YUP_MSVC || DOXYGEN
/** This can be placed before a stack or member variable declaration to tell the compiler
    to align it to the specified number of bytes. */
#define YUP_ALIGN(bytes) __declspec (align (bytes))
#else
#define YUP_ALIGN(bytes) __attribute__ ((aligned (bytes)))
#endif

//==============================================================================
#if YUP_ANDROID && ! defined(DOXYGEN)
#define YUP_MODAL_LOOPS_PERMITTED 0
#elif ! defined(YUP_MODAL_LOOPS_PERMITTED)
/** Some operating environments don't provide a modal loop mechanism, so this flag can be
    used to disable any functions that try to run a modal loop. */
#define YUP_MODAL_LOOPS_PERMITTED 0
#endif

//==============================================================================
#if YUP_GCC || YUP_CLANG
#define YUP_PACKED __attribute__ ((packed))
#elif ! defined(DOXYGEN)
#define YUP_PACKED
#endif

//==============================================================================
#if YUP_GCC || DOXYGEN
/** This can be appended to a function declaration to tell gcc to disable associative
    math optimisations which break some floating point algorithms. */
#define YUP_NO_ASSOCIATIVE_MATH_OPTIMISATIONS __attribute__ ((__optimize__ ("no-associative-math")))
#else
#define YUP_NO_ASSOCIATIVE_MATH_OPTIMISATIONS
#endif

} // namespace yup
