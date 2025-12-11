/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#if ! YUP_WASM

TEST (DynamicLibraryTests, OpenSystemLibrary)
{
    DynamicLibrary lib;

    // Try to open a system library
#if YUP_MAC || YUP_IOS
    EXPECT_TRUE (lib.open ("/usr/lib/libSystem.dylib"));
#elif YUP_LINUX || YUP_ANDROID
    EXPECT_TRUE (lib.open ("libc.so.6") || lib.open ("libc.so"));
#elif YUP_WINDOWS
    EXPECT_TRUE (lib.open ("kernel32.dll"));
#endif
}

TEST (DynamicLibraryTests, OpenNonExistent)
{
    DynamicLibrary lib;

    // Should fail to open non-existent library
    EXPECT_FALSE (lib.open ("/nonexistent/library.so"));
}

TEST (DynamicLibraryTests, Close)
{
    DynamicLibrary lib;

    // Close without opening should not crash
    EXPECT_NO_THROW (lib.close());

#if YUP_MAC || YUP_IOS
    lib.open ("/usr/lib/libSystem.dylib");
#elif YUP_LINUX || YUP_ANDROID
    lib.open ("libc.so.6");
#endif

    // Close after opening
    EXPECT_NO_THROW (lib.close());

    // Close again should not crash
    EXPECT_NO_THROW (lib.close());
}

TEST (DynamicLibraryTests, GetFunction)
{
    DynamicLibrary lib;

    // Getting function from unopened library should return nullptr
    EXPECT_EQ (lib.getFunction ("some_function"), nullptr);

#if YUP_MAC || YUP_IOS
    if (lib.open ("/usr/lib/libSystem.dylib"))
    {
        // Try to get a known function
        auto func = lib.getFunction ("malloc");
        EXPECT_NE (func, nullptr);

        // Try to get non-existent function
        func = lib.getFunction ("nonexistent_function_12345");
        EXPECT_EQ (func, nullptr);
    }
#elif YUP_LINUX || YUP_ANDROID
    if (lib.open ("libc.so.6") || lib.open ("libc.so"))
    {
        // Try to get a known function
        auto func = lib.getFunction ("malloc");
        EXPECT_NE (func, nullptr);

        // Try to get non-existent function
        func = lib.getFunction ("nonexistent_function_12345");
        EXPECT_EQ (func, nullptr);
    }
#endif
}

TEST (DynamicLibraryTests, ReopenAfterClose)
{
    DynamicLibrary lib;

#if YUP_MAC || YUP_IOS
    EXPECT_TRUE (lib.open ("/usr/lib/libSystem.dylib"));
    lib.close();
    EXPECT_TRUE (lib.open ("/usr/lib/libSystem.dylib"));
#elif YUP_LINUX || YUP_ANDROID
    if (lib.open ("libc.so.6") || lib.open ("libc.so"))
    {
        lib.close();
        EXPECT_TRUE (lib.open ("libc.so.6") || lib.open ("libc.so"));
    }
#endif
}

TEST (DynamicLibraryTests, OpenEmptyString)
{
    DynamicLibrary lib;

    // Opening with empty string loads current process symbols
    EXPECT_TRUE (lib.open (""));

    // Should be able to get functions from current process
    auto func = lib.getFunction ("malloc");
    EXPECT_NE (func, nullptr);
}

#endif // ! YUP_WASM
