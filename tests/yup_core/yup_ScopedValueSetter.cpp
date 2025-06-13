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

#include <yup_core/yup_core.h>

#include <gtest/gtest.h>

using namespace yup;

class ScopedValueSetterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        testInt = 10;
        testFloat = 1.5f;
        testBool = false;
        testString = "initial";
    }

    int testInt;
    float testFloat;
    bool testBool;
    String testString;
};

TEST_F (ScopedValueSetterTest, BasicIntegerSetAndRestore)
{
    EXPECT_EQ (testInt, 10);

    {
        ScopedValueSetter<int> setter (testInt, 20);
        EXPECT_EQ (testInt, 20);
    }

    EXPECT_EQ (testInt, 10); // Should be restored to original value
}

TEST_F (ScopedValueSetterTest, BasicFloatSetAndRestore)
{
    EXPECT_FLOAT_EQ (testFloat, 1.5f);

    {
        ScopedValueSetter<float> setter (testFloat, 3.14f);
        EXPECT_FLOAT_EQ (testFloat, 3.14f);
    }

    EXPECT_FLOAT_EQ (testFloat, 1.5f); // Should be restored to original value
}

TEST_F (ScopedValueSetterTest, BasicBoolSetAndRestore)
{
    EXPECT_FALSE (testBool);

    {
        ScopedValueSetter<bool> setter (testBool, true);
        EXPECT_TRUE (testBool);
    }

    EXPECT_FALSE (testBool); // Should be restored to original value
}

TEST_F (ScopedValueSetterTest, BasicStringSetAndRestore)
{
    EXPECT_EQ (testString, "initial");

    {
        ScopedValueSetter<String> setter (testString, String ("temporary"));
        EXPECT_EQ (testString, "temporary");
    }

    EXPECT_EQ (testString, "initial"); // Should be restored to original value
}

TEST_F (ScopedValueSetterTest, ThreeParameterConstructor)
{
    EXPECT_EQ (testInt, 10);

    {
        ScopedValueSetter<int> setter (testInt, 20, 30);
        EXPECT_EQ (testInt, 20); // Should be set to new value
    }

    EXPECT_EQ (testInt, 30); // Should be set to final value, not original
}

TEST_F (ScopedValueSetterTest, ThreeParameterConstructorString)
{
    EXPECT_EQ (testString, "initial");

    {
        ScopedValueSetter<String> setter (testString, String ("temporary"), String ("final"));
        EXPECT_EQ (testString, "temporary");
    }

    EXPECT_EQ (testString, "final"); // Should be set to final value
}

TEST_F (ScopedValueSetterTest, NestedScopedSetters)
{
    EXPECT_EQ (testInt, 10);

    {
        ScopedValueSetter<int> outerSetter (testInt, 20);
        EXPECT_EQ (testInt, 20);

        {
            ScopedValueSetter<int> innerSetter (testInt, 30);
            EXPECT_EQ (testInt, 30);
        }

        EXPECT_EQ (testInt, 20); // Should be restored to outer setter value
    }

    EXPECT_EQ (testInt, 10); // Should be restored to original value
}

TEST_F (ScopedValueSetterTest, NestedWithFinalValues)
{
    EXPECT_EQ (testInt, 10);

    {
        ScopedValueSetter<int> outerSetter (testInt, 20, 25);
        EXPECT_EQ (testInt, 20);

        {
            ScopedValueSetter<int> innerSetter (testInt, 30, 35);
            EXPECT_EQ (testInt, 30);
        }

        EXPECT_EQ (testInt, 35); // Should be restored to outer setter final value
    }

    EXPECT_EQ (testInt, 25); // Should remain at the outer setter final value
}

TEST_F (ScopedValueSetterTest, SameValueSetAndRestore)
{
    EXPECT_EQ (testInt, 10);

    {
        ScopedValueSetter<int> setter (testInt, 10); // Set to same value as original
        EXPECT_EQ (testInt, 10);
    }

    EXPECT_EQ (testInt, 10); // Should remain unchanged
}

TEST_F (ScopedValueSetterTest, ZeroToNonZeroAndBack)
{
    int zeroValue = 0;

    {
        ScopedValueSetter<int> setter (zeroValue, 42);
        EXPECT_EQ (zeroValue, 42);
    }

    EXPECT_EQ (zeroValue, 0);
}

TEST_F (ScopedValueSetterTest, MultipleSequentialSetters)
{
    EXPECT_EQ (testInt, 10);

    {
        ScopedValueSetter<int> setter1 (testInt, 20);
        EXPECT_EQ (testInt, 20);
    }

    EXPECT_EQ (testInt, 10);

    {
        ScopedValueSetter<int> setter2 (testInt, 30);
        EXPECT_EQ (testInt, 30);
    }

    EXPECT_EQ (testInt, 10);
}

TEST_F (ScopedValueSetterTest, CustomTypeTest)
{
    struct CustomType
    {
        int value;

        CustomType (int v = 0)
            : value (v)
        {
        }

        bool operator== (const CustomType& other) const { return value == other.value; }
    };

    CustomType customValue (100);

    EXPECT_EQ (customValue.value, 100);

    {
        ScopedValueSetter<CustomType> setter (customValue, CustomType (200));
        EXPECT_EQ (customValue.value, 200);
    }

    EXPECT_EQ (customValue.value, 100); // Should be restored
}

TEST_F (ScopedValueSetterTest, ExceptionSafety)
{
    EXPECT_EQ (testInt, 10);

    try
    {
        ScopedValueSetter<int> setter (testInt, 20);
        EXPECT_EQ (testInt, 20);
        throw std::runtime_error ("test exception");
    }
    catch (const std::exception&)
    {
        // Exception caught, setter should still restore the value
    }

    EXPECT_EQ (testInt, 10); // Should be restored despite exception
}
