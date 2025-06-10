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

#include <optional>

using namespace yup;

class JSONUtilsTests : public ::testing::Test
{
protected:
    void expectDeepEqual (const std::optional<var>& a, const std::optional<var>& b)
    {
        const auto text = a.has_value() && b.has_value()
                            ? JSON::toString (*a) + " != " + JSON::toString (*b)
                            : String();
        EXPECT_TRUE (deepEqual (a, b)) << text;
    }

    static bool deepEqual (const std::optional<var>& a, const std::optional<var>& b)
    {
        if (a.has_value() && b.has_value())
            return JSONUtils::deepEqual (*a, *b);

        return a == b;
    }
};

TEST_F (JSONUtilsTests, JSONPointers)
{
    const auto obj = JSON::parse (R"({ "name":           "PIANO 4"
                                     , "lfoSpeed":       30
                                     , "lfoWaveform":    "triangle"
                                     , "pitchEnvelope":  { "rates": [94,67,95,60], "levels": [50,50,50,50] }
                                     })");

    expectDeepEqual (JSONUtils::setPointer (obj, "", "hello world"), var ("hello world"));
    expectDeepEqual (JSONUtils::setPointer (obj, "/lfoWaveform/foobar", "str"), std::nullopt);
    expectDeepEqual (JSONUtils::setPointer (JSON::parse (R"({"foo":0,"bar":1})"), "/foo", 2), JSON::parse (R"({"foo":2,"bar":1})"));
    expectDeepEqual (JSONUtils::setPointer (JSON::parse (R"({"foo":0,"bar":1})"), "/baz", 2), JSON::parse (R"({"foo":0,"bar":1,"baz":2})"));
    expectDeepEqual (JSONUtils::setPointer (JSON::parse (R"({"foo":{},"bar":{}})"), "/foo/bar", 2), JSON::parse (R"({"foo":{"bar":2},"bar":{}})"));
    expectDeepEqual (JSONUtils::setPointer (obj, "/pitchEnvelope/rates/01", "str"), std::nullopt);
    expectDeepEqual (JSONUtils::setPointer (obj, "/pitchEnvelope/rates/10", "str"), std::nullopt);
    expectDeepEqual (JSONUtils::setPointer (obj, "/lfoSpeed", 10), JSON::parse (R"({ "name":           "PIANO 4"
                                                                                , "lfoSpeed":       10
                                                                                , "lfoWaveform":    "triangle"
                                                                                , "pitchEnvelope":  { "rates": [94,67,95,60], "levels": [50,50,50,50] }
                                                                                })"));
    expectDeepEqual (JSONUtils::setPointer (JSON::parse (R"([0,1,2])"), "/0", "bang"), JSON::parse (R"(["bang",1,2])"));
    expectDeepEqual (JSONUtils::setPointer (JSON::parse (R"({"/":"fizz"})"), "/~1", "buzz"), JSON::parse (R"({"/":"buzz"})"));
    expectDeepEqual (JSONUtils::setPointer (JSON::parse (R"({"~":"fizz"})"), "/~0", "buzz"), JSON::parse (R"({"~":"buzz"})"));
    expectDeepEqual (JSONUtils::setPointer (obj, "/pitchEnvelope/rates/0", 80), JSON::parse (R"({ "name":           "PIANO 4"
                                                                                              , "lfoSpeed":       30
                                                                                              , "lfoWaveform":    "triangle"
                                                                                              , "pitchEnvelope":  { "rates": [80,67,95,60], "levels": [50,50,50,50] }
                                                                                              })"));
    expectDeepEqual (JSONUtils::setPointer (obj, "/pitchEnvelope/levels/0", 80), JSON::parse (R"({ "name":           "PIANO 4"
                                                                                               , "lfoSpeed":       30
                                                                                               , "lfoWaveform":    "triangle"
                                                                                               , "pitchEnvelope":  { "rates": [94,67,95,60], "levels": [80,50,50,50] }
                                                                                               })"));
    expectDeepEqual (JSONUtils::setPointer (obj, "/pitchEnvelope/levels/-", 100), JSON::parse (R"({ "name":           "PIANO 4"
                                                                                                , "lfoSpeed":       30
                                                                                                , "lfoWaveform":    "triangle"
                                                                                                , "pitchEnvelope":  { "rates": [94,67,95,60], "levels": [50,50,50,50,100] }
                                                                                                })"));
}