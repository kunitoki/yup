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

namespace juce
{

void Logger::outputDebugString (const String& text)
{
#if JUCE_EMSCRIPTEN
    if (juce_isRunningUnderBrowser())
    {
        EM_ASM ({ console.log (UTF8ToString ($0)); }, text.toRawUTF8());
        return;
    }
#endif

    std::fprintf (stderr, "%.*s", text.length(), text.toRawUTF8());
}

//==============================================================================
SystemStats::OperatingSystemType SystemStats::getOperatingSystemType()
{
#if JUCE_EMSCRIPTEN
    return WebBrowser;
#else
    return WASM;
#endif
}

String SystemStats::getOperatingSystemName()
{
#if JUCE_EMSCRIPTEN
    char* platform = reinterpret_cast<char*> (EM_ASM_PTR ({
        var str = navigator.platform || "unknown";
        var lengthBytes = lengthBytesUTF8 (str) + 1;
        var ptr = _malloc (lengthBytes);
        stringToUTF8 (str, ptr, lengthBytes);
        return ptr;
    }));

    String platformString (platform ? platform : "unknown");
    free (platform);

    return platformString;
#else
    return "WASM";
#endif

}

bool SystemStats::isOperatingSystem64Bit()
{
    return sizeof(void*) == 8;
}

String SystemStats::getUniqueDeviceID()
{
#if JUCE_EMSCRIPTEN
    char* deviceInfo = reinterpret_cast<char*> (EM_ASM_PTR ({
        var info = navigator.userAgent || "";
        info += navigator.platform || "";
        info += navigator.language || "";

        var lengthBytes = lengthBytesUTF8 (info) + 1;
        var ptr = _malloc (lengthBytes);
        stringToUTF8 (info, ptr, lengthBytes);
        return ptr;
    }));

    String infoString (deviceInfo ? deviceInfo : "");
    free (deviceInfo);

    return String (static_cast<uint64_t> (infoString.hashCode64()));
#else
    return {};
#endif
}

String SystemStats::getDeviceDescription()
{
#if JUCE_EMSCRIPTEN
    char* userAgent = reinterpret_cast<char*> (EM_ASM_PTR ({
        var str = navigator.userAgent || "unknown";
        var lengthBytes = lengthBytesUTF8 (str) + 1;
        var ptr = _malloc (lengthBytes);
        stringToUTF8 (str, ptr, lengthBytes);
        return ptr;
    }));

    String userAgentString (userAgent ? userAgent : "unknown");

    free (userAgent);

    return userAgentString;
#else
    return "WASM VM";
#endif
}

String SystemStats::getDeviceManufacturer() { return {}; }

String SystemStats::getCpuVendor() { return {}; }

String SystemStats::getCpuModel() { return {}; }

int SystemStats::getCpuSpeedInMegahertz() { return 0; }

int SystemStats::getMemorySizeInMegabytes()
{
#if JUCE_EMSCRIPTEN
    int memoryMB = EM_ASM_INT ({
        if ("deviceMemory" in navigator)
            return navigator.deviceMemory * 1024;
        return 0;
    });

    return memoryMB * 1024; // Convert GB to MB
#else
    return 0;
#endif
}

int SystemStats::getPageSize()
{
    return 65536;
}

String SystemStats::getLogonName() { return {}; }

String SystemStats::getFullUserName() { return {}; }

String SystemStats::getComputerName() { return {}; }

String SystemStats::getUserLanguage()
{
#if JUCE_EMSCRIPTEN
    char* language = reinterpret_cast<char*> (EM_ASM_PTR ({
        var str = navigator.language || "";
        var lengthBytes = lengthBytesUTF8 (str) + 1;
        var ptr = _malloc (lengthBytes);
        stringToUTF8 (str, ptr, lengthBytes);
        return ptr;
    }));

    String languageString (language ? language : "");
    free (language);

    return languageString;
#else
    return {};
#endif
}

String SystemStats::getUserRegion()
{
#if JUCE_EMSCRIPTEN
    char* locale = reinterpret_cast<char*> (EM_ASM_PTR ({
        var str = "";
        if (typeof Intl !== 'undefined' && Intl.DateTimeFormat)
        {
            var options = Intl.DateTimeFormat().resolvedOptions();
            if (options.locale)
                str = options.locale;
        }

        var lengthBytes = lengthBytesUTF8 (str) + 1;
        var ptr = _malloc (lengthBytes);
        stringToUTF8 (str, ptr, lengthBytes);
        return ptr;
    }));

    String localeString (locale ? locale : "");
    free (locale);

    return localeString;
#else
    return {};
#endif
}

String SystemStats::getDisplayLanguage()
{
    return getUserLanguage();
}

//==============================================================================
void CPUInformation::initialise() noexcept
{
#if JUCE_EMSCRIPTEN
    int hwConcurrency = EM_ASM_INT ({
        if ("hardwareConcurrency" in navigator)
            return navigator.hardwareConcurrency;
        return 1;
    });

    numLogicalCPUs = hwConcurrency > 0 ? hwConcurrency : 1;
    numPhysicalCPUs = numLogicalCPUs; // Physical core info isn't available
#else
    numLogicalCPUs = 1;
    numPhysicalCPUs = 1;
#endif
}

//==============================================================================
uint32 juce_millisecondsSinceStartup() noexcept
{
#if JUCE_EMSCRIPTEN
    if (juce_isRunningUnderBrowser())
        return static_cast<uint32> (emscripten_get_now());
#endif

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::steady_clock::now() - juce_getTimeSinceStartupFallback());

    return static_cast<uint32>(elapsed.count());
}

int64 Time::getHighResolutionTicks() noexcept
{
#if JUCE_EMSCRIPTEN
    if (juce_isRunningUnderBrowser())
        return static_cast<int64> (emscripten_get_now() * 1000.0);
#endif

    return static_cast<int64> (juce_millisecondsSinceStartup() * 1000.0);
}

int64 Time::getHighResolutionTicksPerSecond() noexcept
{
    return 1000000; // (microseconds)
}

double Time::getMillisecondCounterHiRes() noexcept
{
#if JUCE_EMSCRIPTEN
    if (juce_isRunningUnderBrowser())
        return emscripten_get_now();
#endif

    return static_cast<double> (juce_millisecondsSinceStartup());
}

bool Time::setSystemTimeToThisTime() const
{
    return false;
}

JUCE_API bool JUCE_CALLTYPE juce_isRunningUnderDebugger() noexcept
{
    return false;
}

} // namespace juce
