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

#include <juce_core/juce_core.h>

#include <string_view>

using namespace yup;

TEST (PlatformDefsTests, Stringify)
{
    constexpr auto x = std::string_view (YUP_STRINGIFY (abcdf));

    static_assert (x == "abcdf");
}

TEST (PlatformDefsTests, IsConstantEvaluated)
{
    constexpr auto x = []
    {
        if (yup::isConstantEvaluated())
            return 1;
        else
            return 2;
    };

    static_assert (x() == 1);

    auto y = x();
    EXPECT_EQ (y, 2);
}

TEST (PlatformDefsTests, ConstexprJassertfalse)
{
    constexpr auto x = []
    {
        if (__LINE__ == 0)
            jassertfalse;

        return true;
    }();

    static_assert (x == true);
}
