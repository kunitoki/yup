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

#include <yup_core/yup_core.h>

#include <map>

using namespace yup;

class JSONTests : public ::testing::Test
{
protected:
    Random random;

    String createRandomWideCharString()
    {
        yup_wchar buffer[40] = { 0 };

        for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
        {
            if (random.nextBool())
            {
                do
                {
                    buffer[i] = static_cast<yup_wchar> (1 + random.nextInt (0x10ffff - 1));
                } while (! CharPointer_UTF16::canRepresent (buffer[i]));
            }
            else
            {
                buffer[i] = static_cast<yup_wchar> (1 + random.nextInt (0xff));
            }
        }

        return CharPointer_UTF32 (buffer);
    }

    String createRandomIdentifier()
    {
        char buffer[30] = { 0 };

        for (int i = 0; i < numElementsInArray (buffer) - 1; ++i)
        {
            static const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-:";
            buffer[i] = chars[random.nextInt (sizeof (chars) - 1)];
        }

        return CharPointer_ASCII (buffer);
    }

    var createRandomDouble()
    {
        return var ((random.nextDouble() * 1000.0) + 0.1);
    }

    var createRandomVar (int depth)
    {
        switch (random.nextInt (depth > 3 ? 6 : 8))
        {
            case 0:
                return {};
            case 1:
                return random.nextInt();
            case 2:
                return random.nextInt64();
            case 3:
                return random.nextBool();
            case 4:
                return createRandomDouble();
            case 5:
                return createRandomWideCharString();

            case 6:
            {
                var v (createRandomVar (depth + 1));

                for (int i = 1 + random.nextInt (30); --i >= 0;)
                    v.append (createRandomVar (depth + 1));

                return v;
            }

            case 7:
            {
                auto o = new DynamicObject();

                for (int i = random.nextInt (30); --i >= 0;)
                    o->setProperty (createRandomIdentifier(), createRandomVar (depth + 1));

                return o;
            }

            default:
                return {};
        }
    }
};

TEST_F (JSONTests, ParseAndGenerate)
{
    EXPECT_EQ (JSON::parse (String()), var());
    EXPECT_TRUE (JSON::parse ("{}").isObject());
    EXPECT_TRUE (JSON::parse ("[]").isArray());
    EXPECT_TRUE (JSON::parse ("[ 1234 ]")[0].isInt());
    EXPECT_TRUE (JSON::parse ("[ 12345678901234 ]")[0].isInt64());
    EXPECT_TRUE (JSON::parse ("[ 1.123e3 ]")[0].isDouble());
    EXPECT_TRUE (JSON::parse ("[ -1234]")[0].isInt());
    EXPECT_TRUE (JSON::parse ("[-12345678901234]")[0].isInt64());
    EXPECT_TRUE (JSON::parse ("[-1.123e3]")[0].isDouble());

    for (int i = 100; --i >= 0;)
    {
        var v = i > 0 ? createRandomVar (0) : var();
        bool oneLine = random.nextBool();

        String asString = JSON::toString (v, oneLine);
        var parsed = JSON::parse ("[" + asString + "]")[0];
        String parsedString = JSON::toString (parsed, oneLine);

        EXPECT_FALSE (asString.isEmpty());
        EXPECT_EQ (parsedString, asString);
    }
}

TEST_F (JSONTests, FloatFormatting)
{
    std::map<double, String> tests {
        { 1, "1.0" },
        { 1.1, "1.1" },
        { 1.01, "1.01" },
        { 0.76378, "0.76378" },
        { -10, "-10.0" },
        { 10.01, "10.01" },
        { 0.0123, "0.0123" },
        { -3.7e-27, "-3.7e-27" },
        { 1e+40, "1.0e40" },
        { -12345678901234567.0, "-1.234567890123457e16" },
        { 192000, "192000.0" },
        { 1234567, "1.234567e6" },
        { 0.00006, "0.00006" },
        { 0.000006, "6.0e-6" }
    };

    for (const auto& [value, expected] : tests)
    {
        EXPECT_EQ (JSON::toString (value), expected);
    }
}
