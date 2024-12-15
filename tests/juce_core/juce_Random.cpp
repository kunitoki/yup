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

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

using namespace juce;

TEST (RandomTests, RandomNumbers)
{
    Random& r = Random::getSystemRandom();

    for (int i = 2000; --i >= 0;)
    {
        EXPECT_TRUE (r.nextDouble() >= 0.0 && r.nextDouble() < 1.0);
        EXPECT_TRUE (r.nextFloat() >= 0.0f && r.nextFloat() < 1.0f);
        EXPECT_TRUE (r.nextInt (5) >= 0 && r.nextInt (5) < 5);
        EXPECT_TRUE (r.nextInt (1) == 0);

        int n = r.nextInt (50) + 1;
        EXPECT_TRUE (r.nextInt (n) >= 0 && r.nextInt (n) < n);

        n = r.nextInt (0x7ffffffe) + 1;
        EXPECT_TRUE (r.nextInt (n) >= 0 && r.nextInt (n) < n);
    }
}
