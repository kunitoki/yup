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
    EM_ASM({ console.log(UTF8ToString($0)); }, text.toRawUTF8());
#else
    std::printf ("%.*s", text.length(), text.toRawUTF8());
#endif
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
    auto navigator = emscripten::val::global("navigator");

    String platform{ navigator["platform"].as<std::string>().c_str() };

    if (platform.isEmpty())
        platform = "unknown";

    return platform;
#else
    return "WASM";
#endif
}

bool SystemStats::isOperatingSystem64Bit()
{
    return sizeof(void*) == 8;
}

String SystemStats::getDeviceDescription()
{
#if JUCE_EMSCRIPTEN
    auto navigator = emscripten::val::global("navigator");
    return { navigator["userAgent"].as<std::string>().c_str() };
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
    auto navigator = emscripten::val::global("navigator");
    if (! navigator.hasOwnProperty("deviceMemory"))
        return 0;

    double deviceMemoryGB = navigator["deviceMemory"].as<double>();
    return static_cast<int>(deviceMemoryGB * 1024); // Convert GB to MB
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
    auto navigator = emscripten::val::global("navigator");
    if (! navigator.hasOwnProperty("language"))
        return {};

    return { navigator["language"].as<std::string>().c_str() };
#else
    return {};
#endif
}

String SystemStats::getUserRegion()
{
#if JUCE_EMSCRIPTEN
    auto intl = emscripten::val::global("Intl");
    if (intl.hasOwnProperty("DateTimeFormat"))
    {
        auto localeOptions = intl.call<emscripten::val>("DateTimeFormat").call<emscripten::val>("resolvedOptions");
        if (localeOptions.hasOwnProperty("locale"))
            return { localeOptions["locale"].as<std::string>().c_str() };
    }
    return {};
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
    auto navigator = emscripten::val::global("navigator");
    if (navigator.hasOwnProperty("hardwareConcurrency"))
    {
        numLogicalCPUs = navigator["hardwareConcurrency"].as<int>();
        numPhysicalCPUs = numLogicalCPUs; // Physical core info isn't available
        return;
    }
#endif

    numLogicalCPUs = 1;
    numPhysicalCPUs = 1;
}

//==============================================================================
uint32 juce_millisecondsSinceStartup() noexcept
{
    return static_cast<uint32> (emscripten_get_now());
}

int64 Time::getHighResolutionTicks() noexcept
{
    return static_cast<int64> (emscripten_get_now() * 1000.0);
}

int64 Time::getHighResolutionTicksPerSecond() noexcept
{
    return 1000000; // (microseconds)
}

double Time::getMillisecondCounterHiRes() noexcept
{
    return emscripten_get_now();
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
