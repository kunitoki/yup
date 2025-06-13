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

// clang-format off
#ifdef YUP_CORE_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif
// clang-format on

#define YUP_CORE_INCLUDE_OBJC_HELPERS 1
#define YUP_CORE_INCLUDE_COM_SMART_PTR 1
#define YUP_CORE_INCLUDE_NATIVE_HEADERS 1
#define YUP_CORE_INCLUDE_JNI_HELPERS 1

#include "yup_core.h"

#include <atomic>
#include <ctime>
#include <cctype>
#include <cstdarg>
#include <locale>
#include <thread>

#if ! (YUP_ANDROID || YUP_BSD)
#include <sys/timeb.h>
#include <cwctype>
#endif

#if YUP_WINDOWS
#if YUP_MINGW
#include <ws2spi.h>
#include <cstdio>
#include <locale.h>
#else
YUP_BEGIN_IGNORE_WARNINGS_MSVC (4091)
#include <Dbghelp.h>
YUP_END_IGNORE_WARNINGS_MSVC

#if ! YUP_DONT_AUTOLINK_TO_WIN32_LIBRARIES
#pragma comment(lib, "DbgHelp.lib")
#endif
#endif

#else
#if YUP_LINUX || YUP_BSD || YUP_ANDROID
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#if YUP_WASM
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#if YUP_EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif

#if YUP_LINUX || YUP_BSD
#include <stdio.h>
#include <limits.h>
#include <langinfo.h>
#include <ifaddrs.h>
#include <semaphore.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>

#if YUP_USE_CURL
#include <curl/curl.h>
#endif
#endif

#include <pwd.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <net/if.h>
#include <sys/ioctl.h>

#if ! (YUP_WASM || (YUP_ANDROID && __ANDROID_API__ < 33))
#include <execinfo.h>
#endif

#if ! (YUP_WASM || YUP_MINGW)
#include <cxxabi.h>
#endif

extern char** environ;
#endif

#if YUP_MAC
#include <sys/stat.h>
#include <sys/mman.h>
#include <semaphore.h>
#endif

#if YUP_MAC || YUP_IOS
#include <xlocale.h>
#include <mach/mach.h>
#endif

#if YUP_ANDROID
#include <ifaddrs.h>
#include <unwind.h>
#include <dlfcn.h>
#include <android/log.h>
#endif

#undef check

//==============================================================================
#include "profiling/yup_Profiler.cpp"

//==============================================================================
#include "containers/yup_AbstractFifo.cpp"
#include "containers/yup_NamedValueSet.cpp"
#include "containers/yup_PropertySet.cpp"
#include "files/yup_DirectoryIterator.cpp"
#include "files/yup_RangedDirectoryIterator.cpp"
#include "files/yup_File.cpp"
#include "files/yup_FileInputStream.cpp"
#include "files/yup_FileOutputStream.cpp"
#include "files/yup_FileSearchPath.cpp"
#include "files/yup_TemporaryFile.cpp"
#include "logging/yup_FileLogger.cpp"
#include "logging/yup_Logger.cpp"
#include "maths/yup_BigInteger.cpp"
#include "maths/yup_Expression.cpp"
#include "maths/yup_Random.cpp"
#include "memory/yup_MemoryBlock.cpp"
#include "memory/yup_AllocationHooks.cpp"
#include "cryptography/yup_SHA1.cpp"
#include "misc/yup_RuntimePermissions.cpp"
#include "misc/yup_Result.cpp"
#include "misc/yup_Uuid.cpp"
#include "misc/yup_ConsoleApplication.cpp"
#include "misc/yup_ScopeGuard.cpp"
#include "network/yup_MACAddress.cpp"
#include "network/yup_NamedPipe.cpp"
#include "network/yup_Socket.cpp"
#include "network/yup_IPAddress.cpp"
#include "streams/yup_BufferedInputStream.cpp"
#include "streams/yup_FileInputSource.cpp"
#include "streams/yup_InputStream.cpp"
#include "streams/yup_MemoryInputStream.cpp"
#include "streams/yup_MemoryOutputStream.cpp"
#include "streams/yup_SubregionStream.cpp"
#include "system/yup_SystemStats.cpp"
#include "text/yup_CharacterFunctions.cpp"
#include "text/yup_Identifier.cpp"
#include "text/yup_LocalisedStrings.cpp"
#include "text/yup_String.cpp"
#include "streams/yup_OutputStream.cpp"
#include "text/yup_StringArray.cpp"
#include "text/yup_StringPairArray.cpp"
#include "text/yup_StringPool.cpp"
#include "text/yup_TextDiff.cpp"
#include "text/yup_Base64.cpp"
#include "threads/yup_ReadWriteLock.cpp"
#include "threads/yup_Thread.cpp"
#include "threads/yup_ThreadPool.cpp"
#include "threads/yup_TimeSliceThread.cpp"
#include "time/yup_PerformanceCounter.cpp"
#include "time/yup_RelativeTime.cpp"
#include "time/yup_Time.cpp"
#include "unit_tests/yup_UnitTest.cpp"
#include "containers/yup_Variant.cpp"
#include "javascript/yup_JSON.cpp"
#include "javascript/yup_JSONUtils.cpp"
#include "javascript/yup_Javascript.cpp"
#include "containers/yup_DynamicObject.cpp"
#include "xml/yup_XmlDocument.cpp"
#include "xml/yup_XmlElement.cpp"
#include "files/yup_FileFilter.cpp"
#include "files/yup_WildcardFileFilter.cpp"
#include "native/yup_ThreadPriorities_native.h"
#include "native/yup_PlatformTimerListener.h"

//==============================================================================
#if ! YUP_WINDOWS
#include "native/yup_SharedCode_posix.h"
#include "native/yup_NamedPipe_posix.cpp"
#if ! YUP_ANDROID || __ANDROID_API__ >= 24
#include "native/yup_IPAddress_posix.h"
#endif
#endif

//==============================================================================
#if YUP_MAC
#include "native/yup_Watchdog_mac.h"
#endif

//==============================================================================
#if YUP_MAC || YUP_IOS
#include "native/yup_Files_apple.mm"
#include "native/yup_Network_apple.mm"
#include "native/yup_Strings_apple.mm"
#include "native/yup_SharedCode_intel.h"
#include "native/yup_SystemStats_apple.mm"
#include "native/yup_Threads_apple.mm"
#include "native/yup_PlatformTimer_generic.cpp"
#include "native/yup_Process_apple.mm"

//==============================================================================
#elif YUP_WINDOWS
#include "native/yup_Files_windows.cpp"
#include "native/yup_Network_windows.cpp"
#include "native/yup_Registry_windows.cpp"
#include "native/yup_SystemStats_windows.cpp"
#include "native/yup_Threads_windows.cpp"
#include "native/yup_PlatformTimer_windows.cpp"
#include "native/yup_Watchdog_windows.h"

//==============================================================================
#elif YUP_LINUX
#include "native/yup_CommonFile_linux.cpp"
#include "native/yup_Files_linux.cpp"
#include "native/yup_Network_linux.cpp"
#if YUP_USE_CURL
#include "native/yup_Network_curl.cpp"
#endif
#include "native/yup_SystemStats_linux.cpp"
#include "native/yup_Threads_linux.cpp"
#include "native/yup_PlatformTimer_generic.cpp"
#include "native/yup_Watchdog_linux.h"

//==============================================================================
#elif YUP_BSD
#include "native/yup_CommonFile_linux.cpp"
#include "native/yup_Files_linux.cpp"
#include "native/yup_Network_linux.cpp"
#if YUP_USE_CURL
#include "native/yup_Network_curl.cpp"
#endif
#include "native/yup_SharedCode_intel.h"
#include "native/yup_SystemStats_linux.cpp"
#include "native/yup_Threads_linux.cpp"
#include "native/yup_PlatformTimer_generic.cpp"

//==============================================================================
#elif YUP_ANDROID
#include "native/yup_CommonFile_linux.cpp"
#include "native/yup_JNIHelpers_android.cpp"
#include "native/yup_Files_android.cpp"
#include "native/yup_Misc_android.cpp"
#include "native/yup_Network_android.cpp"
#include "native/yup_SystemStats_android.cpp"
#include "native/yup_Threads_android.cpp"
#include "native/yup_RuntimePermissions_android.cpp"
#include "native/yup_PlatformTimer_generic.cpp"

//==============================================================================
#elif YUP_WASM
#include "native/yup_WebAssemblyHelpers.h"
#include "native/yup_SystemStats_wasm.cpp"
#include "native/yup_Files_wasm.cpp"
#include "native/yup_Network_wasm.cpp"
#include "native/yup_Threads_wasm.cpp"
#include "native/yup_PlatformTimer_generic.cpp"
#endif

#include "files/yup_common_MimeTypes.h"
#include "files/yup_common_MimeTypes.cpp"
#include "native/yup_AndroidDocument_android.cpp"
#include "threads/yup_HighResolutionTimer.cpp"
#include "threads/yup_WaitableEvent.cpp"
#include "network/yup_URL.cpp"

#if ! YUP_WASM
#include "threads/yup_ChildProcess.cpp"
#include "network/yup_WebInputStream.cpp"
#include "streams/yup_URLInputSource.cpp"
#endif

//==============================================================================
#if YUP_UNIT_TESTS
#include "maths/yup_MathsFunctions_test.cpp"
#endif

//==============================================================================
#include <zlib/zlib.h>

#include "zip/yup_GZIPDecompressorInputStream.cpp"
#include "zip/yup_GZIPCompressorOutputStream.cpp"
#include "zip/yup_ZipFile.cpp"

//==============================================================================
#include "files/yup_Watchdog.cpp"

//==============================================================================
namespace yup
{
/*
    As the very long class names here try to explain, the purpose of this code is to cause
    a linker error if not all of your compile units are consistent in the options that they
    enable before including YUP headers. The reason this is important is that if you have
    two cpp files, and one includes the yup headers with debug enabled, and the other doesn't,
    then each will be generating code with different memory layouts for the classes, and
    you'll get subtle and hard-to-track-down memory corruption bugs!
*/
#if YUP_DEBUG
this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_debug_mode ::this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_debug_mode() noexcept
{
}
#else
this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_release_mode ::this_will_fail_to_link_if_some_of_your_compile_units_are_built_in_release_mode() noexcept
{
}
#endif
} // namespace yup
