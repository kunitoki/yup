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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                 yup_core
    vendor:             yup
    version:            7.0.12
    name:               YUP core classes
    description:        The essential set of basic YUP classes, as required by all the other YUP modules.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       zlib
    osxFrameworks:      Cocoa Foundation IOKit Security
    iosFrameworks:      Foundation UIKit
    iosSimFrameworks:   Foundation UIKit
    linuxLibs:          rt dl pthread
    androidLibs:        log android
    androidSearchpaths: {ANDROID_NDK}/sources/android/cpufeatures
    mingwLibs:          ws2_32 uuid wininet version kernel32 user32 wsock32 advapi32 ole32 oleaut32 imm32 comdlg32 shlwapi rpcrt4 winmm

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_CORE_H_INCLUDED

//==============================================================================
#ifdef _MSC_VER
#pragma warning(push)
// Disable warnings for long class names, padding, and undefined preprocessor definitions.
#pragma warning(disable : 4251 4786 4668 4820)
#ifdef __INTEL_COMPILER
#pragma warning(disable : 1125)
#endif
#endif

#include "system/yup_TargetPlatform.h"

//==============================================================================
/** Config: YUP_FORCE_DEBUG

    Normally, YUP_DEBUG is set to 1 or 0 based on compiler and project settings,
    but if you define this value, you can override this to force it to be true or false.
*/
#ifndef YUP_FORCE_DEBUG
//#define YUP_FORCE_DEBUG 0
#endif

//==============================================================================
/** Config: YUP_LOG_ASSERTIONS

    If this flag is enabled, the jassert and jassertfalse macros will always use Logger::writeToLog()
    to write a message when an assertion happens.

    Enabling it will also leave this turned on in release builds. When it's disabled,
    however, the jassert and jassertfalse macros will not be compiled in a
    release build.

    @see jassert, jassertfalse, Logger
*/
#ifndef YUP_LOG_ASSERTIONS
#if YUP_ANDROID
#define YUP_LOG_ASSERTIONS 1
#else
#define YUP_LOG_ASSERTIONS 0
#endif
#endif

//==============================================================================
/** Config: YUP_CHECK_MEMORY_LEAKS

    Enables a memory-leak check for certain objects when the app terminates. See the LeakedObjectDetector
    class and the YUP_LEAK_DETECTOR macro for more details about enabling leak checking for specific classes.
*/
#if YUP_DEBUG && ! defined(YUP_CHECK_MEMORY_LEAKS)
#define YUP_CHECK_MEMORY_LEAKS 1
#endif

//==============================================================================
/** Config: YUP_DONT_AUTOLINK_TO_WIN32_LIBRARIES

    In a Windows build, this can be used to stop the required system libs being
    automatically added to the link stage.
*/
#ifndef YUP_DONT_AUTOLINK_TO_WIN32_LIBRARIES
#define YUP_DONT_AUTOLINK_TO_WIN32_LIBRARIES 0
#endif

/** Config: YUP_USE_CURL
    Enables http/https support via libcurl (Linux only). Enabling this will add an additional
    run-time dynamic dependency to libcurl.

    If you disable this then https/ssl support will not be available on Linux.
*/
#ifndef YUP_USE_CURL
#define YUP_USE_CURL 1
#endif

/** Config: YUP_LOAD_CURL_SYMBOLS_LAZILY
    If enabled, YUP will load libcurl lazily when required (for example, when WebInputStream
    is used). Enabling this flag may also help with library dependency errors as linking
    libcurl at compile-time may instruct the linker to hard depend on a specific version
    of libcurl. It's also useful if you want to limit the amount of YUP dependencies and
    you are not using WebInputStream or the URL classes.
*/
#ifndef YUP_LOAD_CURL_SYMBOLS_LAZILY
#define YUP_LOAD_CURL_SYMBOLS_LAZILY 1
#endif

/** Config: YUP_CATCH_UNHANDLED_EXCEPTIONS
    If enabled, this will add some exception-catching code to forward unhandled exceptions
    to your YUPApplicationBase::unhandledException() callback.
*/
#ifndef YUP_CATCH_UNHANDLED_EXCEPTIONS
#define YUP_CATCH_UNHANDLED_EXCEPTIONS 0
#endif

/** Config: YUP_ALLOW_STATIC_NULL_VARIABLES
    If disabled, this will turn off dangerous static globals like String::empty, var::null, etc
    which can cause nasty order-of-initialisation problems if they are referenced during static
    constructor code.
*/
#ifndef YUP_ALLOW_STATIC_NULL_VARIABLES
#define YUP_ALLOW_STATIC_NULL_VARIABLES 0
#endif

/** Config: YUP_STRICT_REFCOUNTEDPOINTER
    If enabled, this will make the ReferenceCountedObjectPtr class stricter about allowing
    itself to be cast directly to a raw pointer. By default this is disabled, for compatibility
    with old code, but if possible, you should always enable it to improve code safety!
*/
#ifndef YUP_STRICT_REFCOUNTEDPOINTER
#define YUP_STRICT_REFCOUNTEDPOINTER 0
#endif

/** Config: YUP_ENABLE_ALLOCATION_HOOKS
    If enabled, this will add global allocation functions with built-in assertions, which may
    help when debugging allocations in unit tests.
*/
#ifndef YUP_ENABLE_ALLOCATION_HOOKS
#define YUP_ENABLE_ALLOCATION_HOOKS 0
#endif

/** Config: YUP_PROFILING_CATEGORIES
    If enabled, this will add global profiling categories to the profiler. The "yup" category should
    always be defined, only additional categories should be provided (note the first comma).
    Format of the categories is like:
    ```
        #define YUP_PROFILING_CATEGORIES \
            , perfetto::Category ("custom1") \
            , perfetto::Category ("custom2")
    ```
*/
#ifndef YUP_PROFILING_CATEGORIES
#define YUP_PROFILING_CATEGORIES
#endif

/** Config: YUP_PROFILING_FILE_PREFIX
    If provided, it will be used as prefix for profilation files generated. By default it will use "yup-profile".
*/
#ifndef YUP_PROFILING_FILE_PREFIX
#define YUP_PROFILING_FILE_PREFIX "yup-profile"
#endif

#ifndef YUP_STRING_UTF_TYPE
#define YUP_STRING_UTF_TYPE 8
#endif

//==============================================================================
//==============================================================================

#if YUP_CORE_INCLUDE_NATIVE_HEADERS
#include "native/yup_BasicNativeHeaders.h"
#endif

#if YUP_WINDOWS
#undef small
#endif

#include "system/yup_StandardHeader.h"

namespace yup
{
class StringRef;
class MemoryBlock;
class File;
class InputStream;
class OutputStream;
class DynamicObject;
class FileInputStream;
class FileOutputStream;
class XmlElement;

extern YUP_API bool YUP_CALLTYPE yup_isRunningUnderDebugger() noexcept;
extern YUP_API void YUP_CALLTYPE logAssertion (const char* file, int line) noexcept;
extern YUP_API void YUP_CALLTYPE logAssertion (const wchar_t* file, int line) noexcept;
} // namespace yup

#include "misc/yup_EnumHelpers.h"
#include "memory/yup_Memory.h"
#include "maths/yup_MathsFunctions.h"
#include "memory/yup_ByteOrder.h"
#include "memory/yup_Atomic.h"
#include "text/yup_CharacterFunctions.h"

YUP_BEGIN_IGNORE_WARNINGS_MSVC (4514 4996)

#include "text/yup_CharPointer_UTF8.h"
#include "text/yup_CharPointer_UTF16.h"
#include "text/yup_CharPointer_UTF32.h"
#include "text/yup_CharPointer_ASCII.h"

YUP_END_IGNORE_WARNINGS_MSVC

#include "misc/yup_MetaProgramming.h"
#include "text/yup_String.h"
#include "text/yup_StringRef.h"
#include "logging/yup_Logger.h"
#include "misc/yup_Functional.h"
#include "containers/yup_Span.h"
#include "memory/yup_LeakedObjectDetector.h"
#include "memory/yup_ContainerDeletePolicy.h"
#include "memory/yup_HeapBlock.h"
#include "memory/yup_MemoryBlock.h"
#include "memory/yup_ReferenceCountedObject.h"
#include "memory/yup_ScopedPointer.h"
#include "memory/yup_OptionalScopedPointer.h"
#include "containers/yup_Optional.h"
#include "containers/yup_Enumerate.h"
#include "containers/yup_ScopedValueSetter.h"
#include "memory/yup_Singleton.h"
#include "memory/yup_WeakReference.h"
#include "threads/yup_ScopedLock.h"
#include "threads/yup_CriticalSection.h"
#include "maths/yup_Range.h"
#include "maths/yup_NormalisableRange.h"
#include "maths/yup_StatisticsAccumulator.h"
#include "containers/yup_ElementComparator.h"
#include "containers/yup_ArrayAllocationBase.h"
#include "containers/yup_ArrayBase.h"
#include "containers/yup_Array.h"
#include "containers/yup_LinkedListPointer.h"
#include "misc/yup_ScopeGuard.h"
#include "threads/yup_ThreadValueInitialiser.h"
#include "containers/yup_ListenerList.h"
#include "containers/yup_OwnedArray.h"
#include "containers/yup_ReferenceCountedArray.h"
#include "containers/yup_SortedSet.h"
#include "containers/yup_SparseSet.h"
#include "containers/yup_AbstractFifo.h"
#include "containers/yup_SingleThreadedAbstractFifo.h"
#include "text/yup_NewLine.h"
#include "text/yup_StringPool.h"
#include "text/yup_Identifier.h"
#include "text/yup_StringArray.h"
#include "text/yup_StringPairArray.h"
#include "system/yup_SystemStats.h"
#include "memory/yup_HeavyweightLeakedObjectDetector.h"
#include "cryptography/yup_SHA1.h"
#include "text/yup_TextDiff.h"
#include "text/yup_LocalisedStrings.h"
#include "text/yup_Base64.h"
#include "misc/yup_FlagSet.h"
#include "misc/yup_Result.h"
#include "misc/yup_ResultValue.h"
#include "misc/yup_Uuid.h"
#include "misc/yup_ConsoleApplication.h"
#include "containers/yup_Variant.h"
#include "containers/yup_NamedValueSet.h"
#include "javascript/yup_JSON.h"
#include "containers/yup_DynamicObject.h"
#include "containers/yup_HashMap.h"
#include "containers/yup_FixedSizeFunction.h"
#include "time/yup_RelativeTime.h"
#include "time/yup_Time.h"
#include "time/yup_TimeoutDetector.h"
#include "streams/yup_InputStream.h"
#include "streams/yup_OutputStream.h"
#include "streams/yup_BufferedInputStream.h"
#include "streams/yup_MemoryInputStream.h"
#include "streams/yup_MemoryOutputStream.h"
#include "streams/yup_SubregionStream.h"
#include "streams/yup_InputSource.h"
#include "files/yup_File.h"
#include "files/yup_DirectoryIterator.h"
#include "files/yup_RangedDirectoryIterator.h"
#include "files/yup_FileInputStream.h"
#include "files/yup_FileOutputStream.h"
#include "files/yup_FileSearchPath.h"
#include "files/yup_MemoryMappedFile.h"
#include "files/yup_TemporaryFile.h"
#include "files/yup_FileFilter.h"
#include "files/yup_WildcardFileFilter.h"
#include "streams/yup_FileInputSource.h"
#include "logging/yup_FileLogger.h"
#include "javascript/yup_JSONUtils.h"
#include "serialisation/yup_Serialisation.h"
#include "javascript/yup_JSONSerialisation.h"
#include "javascript/yup_Javascript.h"
#include "maths/yup_BigInteger.h"
#include "maths/yup_Expression.h"
#include "maths/yup_Random.h"
#include "misc/yup_RuntimePermissions.h"
#include "misc/yup_WindowsRegistry.h"
#include "threads/yup_ChildProcess.h"
#include "threads/yup_DynamicLibrary.h"
#include "threads/yup_InterProcessLock.h"
#include "threads/yup_Process.h"
#include "threads/yup_SpinLock.h"
#include "threads/yup_WaitableEvent.h"
#include "threads/yup_Thread.h"
#include "threads/yup_HighResolutionTimer.h"
#include "threads/yup_ThreadLocalValue.h"
#include "threads/yup_ThreadPool.h"
#include "threads/yup_TimeSliceThread.h"
#include "threads/yup_ReadWriteLock.h"
#include "threads/yup_ScopedReadLock.h"
#include "threads/yup_ScopedWriteLock.h"
#include "network/yup_IPAddress.h"
#include "network/yup_MACAddress.h"
#include "network/yup_NamedPipe.h"
#include "network/yup_Socket.h"
#include "network/yup_URL.h"
#include "network/yup_WebInputStream.h"
#include "streams/yup_URLInputSource.h"
#include "time/yup_PerformanceCounter.h"
#include "unit_tests/yup_UnitTest.h"
#include "xml/yup_XmlDocument.h"
#include "xml/yup_XmlElement.h"
#include "zip/yup_GZIPCompressorOutputStream.h"
#include "zip/yup_GZIPDecompressorInputStream.h"
#include "zip/yup_ZipFile.h"
#include "containers/yup_PropertySet.h"
#include "memory/yup_SharedResourcePointer.h"
#include "memory/yup_AllocationHooks.h"
#include "memory/yup_Reservoir.h"
#include "files/yup_AndroidDocument.h"
#include "files/yup_Watchdog.h"
#include "streams/yup_AndroidDocumentInputSource.h"

#include "detail/yup_CallbackListenerList.h"

#if YUP_CORE_INCLUDE_OBJC_HELPERS && (YUP_MAC || YUP_IOS)
#include "native/yup_CFHelpers_apple.h"
#include "native/yup_ObjCHelpers_apple.h"
#endif

#if YUP_CORE_INCLUDE_COM_SMART_PTR && YUP_WINDOWS
#include "native/yup_ComSmartPtr_windows.h"
#endif

#if YUP_CORE_INCLUDE_JNI_HELPERS && YUP_ANDROID
#include <jni.h>
#include "native/yup_JNIHelpers_android.h"
#endif

#if YUP_UNIT_TESTS
#include "unit_tests/yup_UnitTestCategories.h"
#endif

#if YUP_ENABLE_PROFILING
YUP_BEGIN_IGNORE_WARNINGS_MSVC (4267)
#include <perfetto.h>
YUP_END_IGNORE_WARNINGS_MSVC
#endif

#include "profiling/yup_Profiler.h"

#ifndef DOXYGEN
namespace yup
{
/*
    As the very long class names here try to explain, the purpose of this code is to cause
    a linker error if not all of your compile units are consistent in the options that they
    enable before including YUP headers. The reason this is important is that if you have
    two cpp files, and one includes the yup headers with debug enabled, and another does so
    without that, then each will be generating code with different class layouts, and you'll
    get subtle and hard-to-track-down memory corruption!
 */
#if YUP_DEBUG
struct YUP_API this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_debug_mode
{
    this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_debug_mode() noexcept;
};

static this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_debug_mode compileUnitMismatchSentinel;
#else
struct YUP_API this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_release_mode
{
    this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_release_mode() noexcept;
};

static this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_release_mode compileUnitMismatchSentinel;
#endif
} // namespace yup
#endif

YUP_END_IGNORE_WARNINGS_MSVC

// In DLL builds, need to disable this warnings for other modules
#if defined(YUP_DLL_BUILD) || defined(YUP_DLL)
YUP_IGNORE_MSVC (4251)
#endif
