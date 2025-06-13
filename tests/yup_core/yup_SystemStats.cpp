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

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

TEST (SystemStats, YUPVersion)
{
    auto version = SystemStats::getYUPVersion();
    EXPECT_FALSE (version.isEmpty());
}

TEST (SystemStats, OperatingSystemType)
{
    auto systemType = SystemStats::getOperatingSystemType();

#if YUP_WINDOWS
    EXPECT_TRUE (systemType & SystemStats::OperatingSystemType::Windows);
#elif YUP_MAC
    EXPECT_TRUE (systemType & SystemStats::OperatingSystemType::MacOSX);
#elif YUP_LINUX
    EXPECT_TRUE (systemType & SystemStats::OperatingSystemType::Linux);
#elif YUP_ANDROID
    EXPECT_TRUE (systemType & SystemStats::OperatingSystemType::Android);
#elif YUP_IOS
    EXPECT_TRUE (systemType & SystemStats::OperatingSystemType::iOS);
#elif YUP_EMSCRIPTEN
    EXPECT_TRUE (systemType & SystemStats::OperatingSystemType::WebBrowser);
#elif YUP_WASM
    EXPECT_TRUE (systemType & SystemStats::OperatingSystemType::WASM);
#else
    ignoreUnused (systemType);
#endif
}

TEST (SystemStatsTests, GetOperatingSystemName)
{
    String osName = SystemStats::getOperatingSystemName();
    EXPECT_FALSE (osName.isEmpty());
}

TEST (SystemStatsTests, IsOperatingSystem64Bit)
{
    bool is64Bit = SystemStats::isOperatingSystem64Bit();
    if constexpr (sizeof (void*) == 8)
        EXPECT_TRUE (is64Bit);
    else
        EXPECT_FALSE (is64Bit);
}

TEST (SystemStatsTests, GetEnvironmentVariable)
{
    String existingVar = SystemStats::getEnvironmentVariable ("PATH", "default");
    EXPECT_FALSE (existingVar.isEmpty());

    String nonExistingVar = SystemStats::getEnvironmentVariable ("NON_EXISTENT_VAR", "default");
    EXPECT_EQ (nonExistingVar, "default");
}

TEST (SystemStatsTests, DISABLED_SetAndRemoveEnvironmentVariable)
{
    String varName = "YUP_TEST_ENV_VAR";
    String varValue = "YUP_TEST_VALUE";

    bool setResult = SystemStats::setEnvironmentVariable (varName, varValue);
    EXPECT_TRUE (setResult);

    String fetchedValue = SystemStats::getEnvironmentVariable (varName, "");
    EXPECT_EQ (fetchedValue, varValue);

    bool removeResult = SystemStats::removeEnvironmentVariable (varName);
    EXPECT_TRUE (removeResult);

    String afterRemoval = SystemStats::getEnvironmentVariable (varName, "");
    EXPECT_EQ (afterRemoval, "");
}

TEST (SystemStatsTests, GetEnvironmentVariables)
{
    auto envVars = SystemStats::getEnvironmentVariables();
    EXPECT_GT (envVars.size(), 0);
}

TEST (SystemStatsTests, DISABLED_UserAndComputerInfo)
{
    String logonName = SystemStats::getLogonName();
    EXPECT_FALSE (logonName.isEmpty());

    String fullUserName = SystemStats::getFullUserName();
    EXPECT_FALSE (fullUserName.isEmpty());

    String computerName = SystemStats::getComputerName();
    EXPECT_FALSE (computerName.isEmpty());
}

TEST (SystemStatsTests, DISABLED_LocaleInfo)
{
    String userLanguage = SystemStats::getUserLanguage();
    EXPECT_FALSE (userLanguage.isEmpty());
    EXPECT_GE (userLanguage.length(), 2);

    String userRegion = SystemStats::getUserRegion();
    EXPECT_FALSE (userRegion.isEmpty());
    EXPECT_GT (userRegion.length(), 0);

    String displayLanguage = SystemStats::getDisplayLanguage();
    EXPECT_FALSE (displayLanguage.isEmpty());
    EXPECT_GE (displayLanguage.length(), 2);
}

TEST (SystemStatsTests, DISABLED_DeviceInfo)
{
    String deviceDescription = SystemStats::getDeviceDescription();
    EXPECT_TRUE (deviceDescription.isNotEmpty());

#if ! YUP_WASM
    String deviceManufacturer = SystemStats::getDeviceManufacturer();
    EXPECT_TRUE (deviceManufacturer.isNotEmpty());
#endif
}

TEST (SystemStatsTests, GetUniqueDeviceID)
{
    String deviceID = SystemStats::getUniqueDeviceID();
    EXPECT_TRUE (deviceID.isNotEmpty());

    String deviceID2 = SystemStats::getUniqueDeviceID();
    EXPECT_EQ (deviceID, deviceID2);
}

TEST (SystemStatsTests, GetMachineIdentifiers)
{
    auto identifiers = SystemStats::getMachineIdentifiers (SystemStats::MachineIdFlags::uniqueId);
    EXPECT_FALSE (identifiers.isEmpty());
}

TEST (SystemStatsTests, DISABLED_CpuInfo)
{
    int numCpus = SystemStats::getNumCpus();
    EXPECT_GT (numCpus, 0);

    int numPhysicalCpus = SystemStats::getNumPhysicalCpus();
    EXPECT_GT (numPhysicalCpus, 0);

#if ! YUP_WASM
    int cpuSpeed = SystemStats::getCpuSpeedInMegahertz();
    EXPECT_GT (cpuSpeed, 0);

    String cpuVendor = SystemStats::getCpuVendor();
    EXPECT_TRUE (cpuVendor.isNotEmpty());

    String cpuModel = SystemStats::getCpuModel();
    EXPECT_TRUE (cpuModel.isNotEmpty());
#endif
}

TEST (SystemStatsTests, CpuFeatures)
{
    EXPECT_NO_THROW (SystemStats::hasMMX());
    EXPECT_NO_THROW (SystemStats::hasSSE());
    EXPECT_NO_THROW (SystemStats::hasSSE2());
    EXPECT_NO_THROW (SystemStats::hasSSE3());
    EXPECT_NO_THROW (SystemStats::hasSSSE3());
    EXPECT_NO_THROW (SystemStats::hasSSE41());
    EXPECT_NO_THROW (SystemStats::hasSSE42());
    EXPECT_NO_THROW (SystemStats::hasAVX());
    EXPECT_NO_THROW (SystemStats::hasAVX2());
    EXPECT_NO_THROW (SystemStats::hasNeon());
}

TEST (SystemStatsTests, MemoryInfo)
{
#if ! YUP_WASM
    int memorySize = SystemStats::getMemorySizeInMegabytes();
    EXPECT_GT (memorySize, 0);
#endif

    int pageSize = SystemStats::getPageSize();
    EXPECT_GT (pageSize, 0);
}

TEST (SystemStatsTests, GetStackBacktrace)
{
#if ! YUP_WASM
    String backtrace = SystemStats::getStackBacktrace();
    EXPECT_TRUE (backtrace.isNotEmpty());
#endif
}

TEST (SystemStatsTests, SetApplicationCrashHandler)
{
    static bool handlerCalled = false;
    auto crashHandler = [] (void*)
    {
        handlerCalled = true;
    };

    SystemStats::setApplicationCrashHandler (crashHandler);
    EXPECT_NO_THROW (SystemStats::setApplicationCrashHandler (crashHandler));
}

TEST (SystemStatsTests, IsRunningInAppExtensionSandbox)
{
    bool isSandboxed = SystemStats::isRunningInAppExtensionSandbox();
    EXPECT_TRUE (isSandboxed == true || isSandboxed == false);
}

#if YUP_MAC
TEST (SystemStatsTests, IsAppSandboxEnabled)
{
    bool isSandboxEnabled = SystemStats::isAppSandboxEnabled();
    EXPECT_TRUE (isSandboxEnabled == true || isSandboxEnabled == false);
}
#endif
