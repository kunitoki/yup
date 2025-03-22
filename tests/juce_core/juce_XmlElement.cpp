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

#include <map>
#include <memory>

using namespace juce;

TEST (XmlElementTests, FloatFormatting)
{
    auto element = std::make_unique<XmlElement> ("test");
    Identifier number ("number");

    std::map<double, String> tests;
    tests[1] = "1.0";
    tests[1.1] = "1.1";
    tests[1.01] = "1.01";
    tests[0.76378] = "0.76378";
    tests[-10] = "-10.0";
    tests[10.01] = "10.01";
    tests[0.0123] = "0.0123";
    tests[-3.7e-27] = "-3.7e-27";
    tests[1e+40] = "1.0e40";
    tests[-12345678901234567.0] = "-1.234567890123457e16";
    tests[192000] = "192000.0";
    tests[1234567] = "1.234567e6";
    tests[0.00006] = "0.00006";
    tests[0.000006] = "6.0e-6";

    for (auto& test : tests)
    {
        element->setAttribute (number, test.first);
        EXPECT_EQ (element->getStringAttribute (number), test.second);
    }
}
