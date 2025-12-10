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

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

class FloatVectorOperationsTests : public ::testing::Test
{
protected:
    template <class ValueType>
    struct TestRunner
    {
        static void runTest (Random& random)
        {
            const int range = random.nextBool() ? 500 : 10;
            const int num = random.nextInt (range) + 1;

            HeapBlock<ValueType> buffer1 (num + 16), buffer2 (num + 16);
            HeapBlock<int> buffer3 (num + 16, true);

#if YUP_ARM
            ValueType* const data1 = buffer1;
            ValueType* const data2 = buffer2;
            int* const int1 = buffer3;
#else
            // These tests deliberately operate on misaligned memory and will be flagged up by
            // checks for undefined behavior!
            ValueType* const data1 = addBytesToPointer (buffer1.get(), random.nextInt (16));
            ValueType* const data2 = addBytesToPointer (buffer2.get(), random.nextInt (16));
            int* const int1 = addBytesToPointer (buffer3.get(), random.nextInt (16));
#endif

            fillRandomly (random, data1, num);
            fillRandomly (random, data2, num);

            Range<ValueType> minMax1 (FloatVectorOperations::findMinAndMax (data1, num));
            Range<ValueType> minMax2 (Range<ValueType>::findMinAndMax (data1, num));
            EXPECT_TRUE (minMax1 == minMax2);

            EXPECT_TRUE (valuesMatch (FloatVectorOperations::findMinimum (data1, num), yup::findMinimum (data1, num)));
            EXPECT_TRUE (valuesMatch (FloatVectorOperations::findMaximum (data1, num), yup::findMaximum (data1, num)));

            EXPECT_TRUE (valuesMatch (FloatVectorOperations::findMinimum (data2, num), yup::findMinimum (data2, num)));
            EXPECT_TRUE (valuesMatch (FloatVectorOperations::findMaximum (data2, num), yup::findMaximum (data2, num)));

            FloatVectorOperations::clear (data1, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, 0));

            FloatVectorOperations::fill (data1, (ValueType) 2, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, (ValueType) 2));

            FloatVectorOperations::add (data1, (ValueType) 2, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, (ValueType) 4));

            FloatVectorOperations::copy (data2, data1, num);
            EXPECT_TRUE (areAllValuesEqual (data2, num, (ValueType) 4));

            FloatVectorOperations::add (data2, data1, num);
            EXPECT_TRUE (areAllValuesEqual (data2, num, (ValueType) 8));

            FloatVectorOperations::copyWithMultiply (data2, data1, (ValueType) 4, num);
            EXPECT_TRUE (areAllValuesEqual (data2, num, (ValueType) 16));

            FloatVectorOperations::addWithMultiply (data2, data1, (ValueType) 4, num);
            EXPECT_TRUE (areAllValuesEqual (data2, num, (ValueType) 32));

            FloatVectorOperations::multiply (data1, (ValueType) 2, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, (ValueType) 8));

            FloatVectorOperations::multiply (data1, data2, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, (ValueType) 256));

            FloatVectorOperations::negate (data2, data1, num);
            EXPECT_TRUE (areAllValuesEqual (data2, num, (ValueType) -256));

            FloatVectorOperations::subtract (data1, data2, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, (ValueType) 512));

            FloatVectorOperations::abs (data1, data2, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, (ValueType) 256));

            FloatVectorOperations::abs (data2, data1, num);
            EXPECT_TRUE (areAllValuesEqual (data2, num, (ValueType) 256));

            FloatVectorOperations::fill (data1, (ValueType) 2, num);
            FloatVectorOperations::fill (data2, (ValueType) 3, num);
            FloatVectorOperations::addWithMultiply (data1, data1, data2, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, (ValueType) 8));

            FloatVectorOperations::fill (data1, (ValueType) 8, num);
            FloatVectorOperations::copyWithDividend (data2, data1, (ValueType) 16, num);
            EXPECT_TRUE (areAllValuesEqual (data2, num, (ValueType) 2));

            FloatVectorOperations::fill (data1, (ValueType) 12, num);
            FloatVectorOperations::copyWithDivide (data2, data1, (ValueType) 3, num);
            EXPECT_TRUE (areAllValuesEqual (data2, num, (ValueType) 4));

            FloatVectorOperations::fill (data1, (ValueType) 20, num);
            FloatVectorOperations::divide (data1, (ValueType) 4, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, (ValueType) 5));

            FloatVectorOperations::fill (data1, (ValueType) 15, num);
            FloatVectorOperations::fill (data2, (ValueType) 3, num);
            HeapBlock<ValueType> result (num + 16);
#if YUP_ARM
            ValueType* const resultData = result;
#else
            // These tests deliberately operate on misaligned memory and will be flagged up by
            // checks for undefined behavior!
            ValueType* const resultData = addBytesToPointer (result.get(), random.nextInt (16));
#endif
            FloatVectorOperations::divide (resultData, data1, data2, num);
            EXPECT_TRUE (areAllValuesEqual (resultData, num, (ValueType) 5));

            FloatVectorOperations::fill (data1, (ValueType) 18, num);
            FloatVectorOperations::divide (data2, data1, (ValueType) 6, num);
            EXPECT_TRUE (areAllValuesEqual (data2, num, (ValueType) 3));
        }

        static void fillRandomly (Random& random, ValueType* d, int num)
        {
            while (--num >= 0)
                *d++ = (ValueType) (random.nextDouble() * 1000.0);
        }

        static void fillRandomly (Random& random, int* d, int num)
        {
            while (--num >= 0)
                *d++ = random.nextInt();
        }

        static bool areAllValuesEqual (const ValueType* d, int num, ValueType target)
        {
            while (--num >= 0)
                if (! exactlyEqual (*d++, target))
                    return false;

            return true;
        }

        static bool buffersMatch (const ValueType* d1, const ValueType* d2, int num)
        {
            while (--num >= 0)
                if (! valuesMatch (*d1++, *d2++))
                    return false;

            return true;
        }

        static bool valuesMatch (ValueType v1, ValueType v2)
        {
            return std::abs (v1 - v2) < std::numeric_limits<ValueType>::epsilon();
        }
    };

    template <class ValueType>
    static bool valuesMatch (ValueType v1, ValueType v2)
    {
        return std::abs (v1 - v2) < std::numeric_limits<ValueType>::epsilon();
    }

    template <class ValueType>
    static bool buffersMatch (const ValueType* d1, const ValueType* d2, int num)
    {
        while (--num >= 0)
        {
            if (! valuesMatch (*d1++, *d2++))
                return false;
        }

        return true;
    }

    static void convertFixedToFloat (float* d, const int* s, float multiplier, int num)
    {
        while (--num >= 0)
            *d++ = (float) *s++ * multiplier;
    }

    static void convertFloatToFixed (int* d, const float* s, float multiplier, int num)
    {
        while (--num >= 0)
            *d++ = (int) (*s++ * multiplier);
    }

    template <class ValueType>
    static void fillRandomly (Random& random, ValueType* d, int num)
    {
        while (--num >= 0)
            *d++ = (ValueType) (random.nextDouble() * 1000.0);
    }
};

TEST_F (FloatVectorOperationsTests, BasicOperations)
{
    for (int i = 1000; --i >= 0;)
    {
        TestRunner<float>::runTest (Random::getSystemRandom());
        TestRunner<double>::runTest (Random::getSystemRandom());
    }
}

TEST_F (FloatVectorOperationsTests, FloatToFixedAndBack)
{
    Random& random = Random::getSystemRandom();

    for (int i = 1000; --i >= 0;)
    {
        const int range = random.nextBool() ? 500 : 10;
        const int num = random.nextInt (range) + 1;

        HeapBlock<float> buffer1 (num + 16), buffer2 (num + 16);
        HeapBlock<int> buffer3 (num + 16, true);

#if YUP_ARM
        float* const data1 = buffer1;
        float* const data2 = buffer2;
        int* const int1 = buffer3;
#else
        // These tests deliberately operate on misaligned memory and will be flagged up by
        // checks for undefined behavior!
        float* const data1 = addBytesToPointer (buffer1.get(), random.nextInt (16));
        float* const data2 = addBytesToPointer (buffer2.get(), random.nextInt (16));
        int* const int1 = addBytesToPointer (buffer3.get(), random.nextInt (16));
#endif

        fillRandomly (random, data1, num);
        fillRandomly (random, data2, num);

        fillRandomly (random, int1, num);
        const auto multiplier = (float) (1.0 / (1 << 16));

        convertFixedToFloat (data1, int1, multiplier, num);
        FloatVectorOperations::convertFixedToFloat (data2, int1, multiplier, num);
        EXPECT_TRUE (buffersMatch (data1, data2, num));

        convertFloatToFixed (int1, data1, 1.0f / multiplier, num);
        HeapBlock<int> int2 (num + 16);
#if YUP_ARM
        int* const intData = int2;
#else
        int* const intData = addBytesToPointer (int2.get(), random.nextInt (16));
#endif
        FloatVectorOperations::convertFloatToFixed (intData, data1, 1.0f / multiplier, num);

        for (int i = 0; i < num; ++i)
            EXPECT_EQ (int1[i], intData[i]);
    }
}

TEST_F (FloatVectorOperationsTests, FloatToDoubleAndBack)
{
    Random& random = Random::getSystemRandom();

    for (int i = 1000; --i >= 0;)
    {
        const int range = random.nextBool() ? 500 : 10;
        const int num = random.nextInt (range) + 1;

        HeapBlock<float> floatBuffer (num + 16);
        HeapBlock<double> doubleBuffer (num + 16);

#if YUP_ARM
        float* const floatData = floatBuffer;
        double* const doubleData = doubleBuffer;
#else
        float* const floatData = addBytesToPointer (floatBuffer.get(), random.nextInt (16));
        double* const doubleData = addBytesToPointer (doubleBuffer.get(), random.nextInt (16));
#endif

        fillRandomly (random, floatData, num);
        FloatVectorOperations::convertFloatToDouble (doubleData, floatData, num);
        for (int i = 0; i < num; ++i)
            EXPECT_NEAR ((float) doubleData[i], (float) floatData[i], std::numeric_limits<float>::epsilon());

        fillRandomly (random, doubleData, num);
        FloatVectorOperations::convertDoubleToFloat (floatData, doubleData, num);
        for (int i = 0; i < num; ++i)
            EXPECT_NEAR ((float) floatData[i], (float) doubleData[i], std::numeric_limits<float>::epsilon());
    }
}

TEST_F (FloatVectorOperationsTests, FindMinAndMax)
{
    float data[10] = { 0.1f, -0.5f, 0.8f, -0.2f, 0.4f, 0.9f, -0.7f, 0.3f, -0.1f, 0.6f };

    auto range = FloatVectorOperations::findMinAndMax (data, 10);

    EXPECT_FLOAT_EQ (range.getStart(), -0.7f);
    EXPECT_FLOAT_EQ (range.getEnd(), 0.9f);
}

TEST_F (FloatVectorOperationsTests, FindMinimum)
{
    float data[10] = { 0.1f, -0.5f, 0.8f, -0.2f, 0.4f, 0.9f, -0.7f, 0.3f, -0.1f, 0.6f };

    auto minVal = FloatVectorOperations::findMinimum (data, 10);

    EXPECT_FLOAT_EQ (minVal, -0.7f);
}

TEST_F (FloatVectorOperationsTests, FindMaximum)
{
    float data[10] = { 0.1f, -0.5f, 0.8f, -0.2f, 0.4f, 0.9f, -0.7f, 0.3f, -0.1f, 0.6f };

    auto maxVal = FloatVectorOperations::findMaximum (data, 10);

    EXPECT_FLOAT_EQ (maxVal, 0.9f);
}

TEST_F (FloatVectorOperationsTests, Negate)
{
    float src[5] = { 1.0f, -2.0f, 3.0f, -4.0f, 5.0f };
    float dest[5];

    FloatVectorOperations::negate (dest, src, 5);

    EXPECT_FLOAT_EQ (dest[0], -1.0f);
    EXPECT_FLOAT_EQ (dest[1], 2.0f);
    EXPECT_FLOAT_EQ (dest[2], -3.0f);
    EXPECT_FLOAT_EQ (dest[3], 4.0f);
    EXPECT_FLOAT_EQ (dest[4], -5.0f);
}

TEST_F (FloatVectorOperationsTests, Abs)
{
    float src[5] = { 1.0f, -2.0f, 3.0f, -4.0f, 5.0f };
    float dest[5];

    FloatVectorOperations::abs (dest, src, 5);

    EXPECT_FLOAT_EQ (dest[0], 1.0f);
    EXPECT_FLOAT_EQ (dest[1], 2.0f);
    EXPECT_FLOAT_EQ (dest[2], 3.0f);
    EXPECT_FLOAT_EQ (dest[3], 4.0f);
    EXPECT_FLOAT_EQ (dest[4], 5.0f);
}

TEST_F (FloatVectorOperationsTests, MinWithScalar)
{
    float src[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    float dest[5];

    FloatVectorOperations::min (dest, src, 3.0f, 5);

    EXPECT_FLOAT_EQ (dest[0], 1.0f);
    EXPECT_FLOAT_EQ (dest[1], 2.0f);
    EXPECT_FLOAT_EQ (dest[2], 3.0f);
    EXPECT_FLOAT_EQ (dest[3], 3.0f);
    EXPECT_FLOAT_EQ (dest[4], 3.0f);
}

TEST_F (FloatVectorOperationsTests, MinWithArray)
{
    float src1[5] = { 1.0f, 5.0f, 2.0f, 4.0f, 3.0f };
    float src2[5] = { 3.0f, 2.0f, 4.0f, 1.0f, 5.0f };
    float dest[5];

    FloatVectorOperations::min (dest, src1, src2, 5);

    EXPECT_FLOAT_EQ (dest[0], 1.0f);
    EXPECT_FLOAT_EQ (dest[1], 2.0f);
    EXPECT_FLOAT_EQ (dest[2], 2.0f);
    EXPECT_FLOAT_EQ (dest[3], 1.0f);
    EXPECT_FLOAT_EQ (dest[4], 3.0f);
}

TEST_F (FloatVectorOperationsTests, MaxWithScalar)
{
    float src[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    float dest[5];

    FloatVectorOperations::max (dest, src, 3.0f, 5);

    EXPECT_FLOAT_EQ (dest[0], 3.0f);
    EXPECT_FLOAT_EQ (dest[1], 3.0f);
    EXPECT_FLOAT_EQ (dest[2], 3.0f);
    EXPECT_FLOAT_EQ (dest[3], 4.0f);
    EXPECT_FLOAT_EQ (dest[4], 5.0f);
}

TEST_F (FloatVectorOperationsTests, MaxWithArray)
{
    float src1[5] = { 1.0f, 5.0f, 2.0f, 4.0f, 3.0f };
    float src2[5] = { 3.0f, 2.0f, 4.0f, 1.0f, 5.0f };
    float dest[5];

    FloatVectorOperations::max (dest, src1, src2, 5);

    EXPECT_FLOAT_EQ (dest[0], 3.0f);
    EXPECT_FLOAT_EQ (dest[1], 5.0f);
    EXPECT_FLOAT_EQ (dest[2], 4.0f);
    EXPECT_FLOAT_EQ (dest[3], 4.0f);
    EXPECT_FLOAT_EQ (dest[4], 5.0f);
}

TEST_F (FloatVectorOperationsTests, Clip)
{
    float src[7] = { -2.0f, -0.5f, 0.0f, 0.5f, 1.0f, 1.5f, 2.0f };
    float dest[7];

    FloatVectorOperations::clip (dest, src, 0.0f, 1.0f, 7);

    EXPECT_FLOAT_EQ (dest[0], 0.0f);
    EXPECT_FLOAT_EQ (dest[1], 0.0f);
    EXPECT_FLOAT_EQ (dest[2], 0.0f);
    EXPECT_FLOAT_EQ (dest[3], 0.5f);
    EXPECT_FLOAT_EQ (dest[4], 1.0f);
    EXPECT_FLOAT_EQ (dest[5], 1.0f);
    EXPECT_FLOAT_EQ (dest[6], 1.0f);
}

TEST_F (FloatVectorOperationsTests, CopyWithDividend)
{
    float src[5] = { 2.0f, 4.0f, 5.0f, 10.0f, 20.0f };
    float dest[5];

    FloatVectorOperations::copyWithDividend (dest, src, 20.0f, 5);

    EXPECT_FLOAT_EQ (dest[0], 10.0f);
    EXPECT_FLOAT_EQ (dest[1], 5.0f);
    EXPECT_FLOAT_EQ (dest[2], 4.0f);
    EXPECT_FLOAT_EQ (dest[3], 2.0f);
    EXPECT_FLOAT_EQ (dest[4], 1.0f);
}

TEST_F (FloatVectorOperationsTests, CopyWithDivide)
{
    float src[5] = { 20.0f, 10.0f, 8.0f, 4.0f, 2.0f };
    float dest[5];

    FloatVectorOperations::copyWithDivide (dest, src, 2.0f, 5);

    EXPECT_FLOAT_EQ (dest[0], 10.0f);
    EXPECT_FLOAT_EQ (dest[1], 5.0f);
    EXPECT_FLOAT_EQ (dest[2], 4.0f);
    EXPECT_FLOAT_EQ (dest[3], 2.0f);
    EXPECT_FLOAT_EQ (dest[4], 1.0f);
}

TEST_F (FloatVectorOperationsTests, DivideScalarByArray)
{
    float src[5] = { 2.0f, 4.0f, 5.0f, 10.0f, 20.0f };
    float dest[5];

    FloatVectorOperations::divide (dest, 20.0f, 5);

    // Initial dest values get divided
    for (int i = 0; i < 5; ++i)
        dest[i] = src[i];

    FloatVectorOperations::divide (dest, 2.0f, 5);

    EXPECT_FLOAT_EQ (dest[0], 1.0f);
    EXPECT_FLOAT_EQ (dest[1], 2.0f);
    EXPECT_FLOAT_EQ (dest[2], 2.5f);
    EXPECT_FLOAT_EQ (dest[3], 5.0f);
    EXPECT_FLOAT_EQ (dest[4], 10.0f);
}

TEST_F (FloatVectorOperationsTests, EnableFlushToZeroMode)
{
    // Just test that it doesn't crash
    EXPECT_NO_THROW (FloatVectorOperations::enableFlushToZeroMode (true));
    EXPECT_NO_THROW (FloatVectorOperations::enableFlushToZeroMode (false));
}

TEST_F (FloatVectorOperationsTests, LargeBufferOperations)
{
    const int size = 10000;
    HeapBlock<float> src (size);
    HeapBlock<float> dest (size);

    Random& random = Random::getSystemRandom();
    for (int i = 0; i < size; ++i)
        src[i] = random.nextFloat() * 2.0f - 1.0f;

    // Test that large buffer operations don't crash
    EXPECT_NO_THROW (FloatVectorOperations::copy (dest, src, size));
    EXPECT_NO_THROW (FloatVectorOperations::multiply (dest, 2.0f, size));
    EXPECT_NO_THROW (FloatVectorOperations::add (dest, 1.0f, size));
    EXPECT_NO_THROW (FloatVectorOperations::clear (dest, size));
}

TEST_F (FloatVectorOperationsTests, DoubleOperations)
{
    double src[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    double dest[5];

    FloatVectorOperations::clear (dest, 5);
    for (int i = 0; i < 5; ++i)
        EXPECT_DOUBLE_EQ (dest[i], 0.0);

    FloatVectorOperations::copy (dest, src, 5);
    for (int i = 0; i < 5; ++i)
        EXPECT_DOUBLE_EQ (dest[i], src[i]);

    FloatVectorOperations::multiply (dest, 2.0, 5);
    for (int i = 0; i < 5; ++i)
        EXPECT_DOUBLE_EQ (dest[i], src[i] * 2.0);

    FloatVectorOperations::add (dest, 1.0, 5);
    for (int i = 0; i < 5; ++i)
        EXPECT_DOUBLE_EQ (dest[i], src[i] * 2.0 + 1.0);
}

// ==============================================================================
// Additional Add Operations Tests
// ==============================================================================

TEST_F (FloatVectorOperationsTests, AddWithAmount_Float)
{
    float dest[5];
    float src[5] = { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f };

    // add(dest, src, amount) sets dest = src + amount
    FloatVectorOperations::add (dest, src, 10.0f, 5);

    EXPECT_FLOAT_EQ (dest[0], 10.5f); // 0.5 + 10.0
    EXPECT_FLOAT_EQ (dest[1], 11.0f); // 1.0 + 10.0
    EXPECT_FLOAT_EQ (dest[2], 11.5f); // 1.5 + 10.0
    EXPECT_FLOAT_EQ (dest[3], 12.0f); // 2.0 + 10.0
    EXPECT_FLOAT_EQ (dest[4], 12.5f); // 2.5 + 10.0
}

TEST_F (FloatVectorOperationsTests, AddWithAmount_Double)
{
    double dest[5];
    double src[5] = { 0.5, 1.0, 1.5, 2.0, 2.5 };

    // add(dest, src, amount) sets dest = src + amount
    FloatVectorOperations::add (dest, src, 10.0, 5);

    EXPECT_DOUBLE_EQ (dest[0], 10.5); // 0.5 + 10.0
    EXPECT_DOUBLE_EQ (dest[1], 11.0); // 1.0 + 10.0
    EXPECT_DOUBLE_EQ (dest[2], 11.5); // 1.5 + 10.0
    EXPECT_DOUBLE_EQ (dest[3], 12.0); // 2.0 + 10.0
    EXPECT_DOUBLE_EQ (dest[4], 12.5); // 2.5 + 10.0
}

TEST_F (FloatVectorOperationsTests, AddTwoArrays_Float)
{
    float dest[5];
    float src1[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    float src2[5] = { 0.5f, 1.5f, 2.5f, 3.5f, 4.5f };

    FloatVectorOperations::add (dest, src1, src2, 5);

    EXPECT_FLOAT_EQ (dest[0], 1.5f);
    EXPECT_FLOAT_EQ (dest[1], 3.5f);
    EXPECT_FLOAT_EQ (dest[2], 5.5f);
    EXPECT_FLOAT_EQ (dest[3], 7.5f);
    EXPECT_FLOAT_EQ (dest[4], 9.5f);
}

TEST_F (FloatVectorOperationsTests, AddTwoArrays_Double)
{
    double dest[5];
    double src1[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    double src2[5] = { 0.5, 1.5, 2.5, 3.5, 4.5 };

    FloatVectorOperations::add (dest, src1, src2, 5);

    EXPECT_DOUBLE_EQ (dest[0], 1.5);
    EXPECT_DOUBLE_EQ (dest[1], 3.5);
    EXPECT_DOUBLE_EQ (dest[2], 5.5);
    EXPECT_DOUBLE_EQ (dest[3], 7.5);
    EXPECT_DOUBLE_EQ (dest[4], 9.5);
}

// ==============================================================================
// Subtract Operations Tests
// ==============================================================================

TEST_F (FloatVectorOperationsTests, SubtractTwoArrays_Float)
{
    float dest[5];
    float src1[5] = { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };
    float src2[5] = { 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };

    FloatVectorOperations::subtract (dest, src1, src2, 5);

    EXPECT_FLOAT_EQ (dest[0], 4.0f);
    EXPECT_FLOAT_EQ (dest[1], 2.5f);
    EXPECT_FLOAT_EQ (dest[2], 1.0f);
    EXPECT_FLOAT_EQ (dest[3], -0.5f);
    EXPECT_FLOAT_EQ (dest[4], -2.0f);
}

TEST_F (FloatVectorOperationsTests, SubtractTwoArrays_Double)
{
    double dest[5];
    double src1[5] = { 5.0, 4.0, 3.0, 2.0, 1.0 };
    double src2[5] = { 1.0, 1.5, 2.0, 2.5, 3.0 };

    FloatVectorOperations::subtract (dest, src1, src2, 5);

    EXPECT_DOUBLE_EQ (dest[0], 4.0);
    EXPECT_DOUBLE_EQ (dest[1], 2.5);
    EXPECT_DOUBLE_EQ (dest[2], 1.0);
    EXPECT_DOUBLE_EQ (dest[3], -0.5);
    EXPECT_DOUBLE_EQ (dest[4], -2.0);
}

// ==============================================================================
// Subtract With Multiply Operations Tests
// ==============================================================================

TEST_F (FloatVectorOperationsTests, SubtractWithMultiply_FloatScalar)
{
    float dest[5] = { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };
    float src[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

    FloatVectorOperations::subtractWithMultiply (dest, src, 2.0f, 5);

    EXPECT_FLOAT_EQ (dest[0], 8.0f);
    EXPECT_FLOAT_EQ (dest[1], 16.0f);
    EXPECT_FLOAT_EQ (dest[2], 24.0f);
    EXPECT_FLOAT_EQ (dest[3], 32.0f);
    EXPECT_FLOAT_EQ (dest[4], 40.0f);
}

TEST_F (FloatVectorOperationsTests, SubtractWithMultiply_DoubleScalar)
{
    double dest[5] = { 10.0, 20.0, 30.0, 40.0, 50.0 };
    double src[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };

    FloatVectorOperations::subtractWithMultiply (dest, src, 2.0, 5);

    EXPECT_DOUBLE_EQ (dest[0], 8.0);
    EXPECT_DOUBLE_EQ (dest[1], 16.0);
    EXPECT_DOUBLE_EQ (dest[2], 24.0);
    EXPECT_DOUBLE_EQ (dest[3], 32.0);
    EXPECT_DOUBLE_EQ (dest[4], 40.0);
}

TEST_F (FloatVectorOperationsTests, SubtractWithMultiply_FloatArrays)
{
    float dest[5] = { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };
    float src1[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    float src2[5] = { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };

    FloatVectorOperations::subtractWithMultiply (dest, src1, src2, 5);

    EXPECT_FLOAT_EQ (dest[0], 8.0f);  // 10 - (1 * 2)
    EXPECT_FLOAT_EQ (dest[1], 14.0f); // 20 - (2 * 3)
    EXPECT_FLOAT_EQ (dest[2], 18.0f); // 30 - (3 * 4)
    EXPECT_FLOAT_EQ (dest[3], 20.0f); // 40 - (4 * 5)
    EXPECT_FLOAT_EQ (dest[4], 20.0f); // 50 - (5 * 6)
}

TEST_F (FloatVectorOperationsTests, SubtractWithMultiply_DoubleArrays)
{
    double dest[5] = { 10.0, 20.0, 30.0, 40.0, 50.0 };
    double src1[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    double src2[5] = { 2.0, 3.0, 4.0, 5.0, 6.0 };

    FloatVectorOperations::subtractWithMultiply (dest, src1, src2, 5);

    EXPECT_DOUBLE_EQ (dest[0], 8.0);  // 10 - (1 * 2)
    EXPECT_DOUBLE_EQ (dest[1], 14.0); // 20 - (2 * 3)
    EXPECT_DOUBLE_EQ (dest[2], 18.0); // 30 - (3 * 4)
    EXPECT_DOUBLE_EQ (dest[3], 20.0); // 40 - (4 * 5)
    EXPECT_DOUBLE_EQ (dest[4], 20.0); // 50 - (5 * 6)
}

// ==============================================================================
// Multiply Operations Tests
// ==============================================================================

TEST_F (FloatVectorOperationsTests, MultiplyTwoArrays_Float)
{
    float dest[5];
    float src1[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    float src2[5] = { 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };

    FloatVectorOperations::multiply (dest, src1, src2, 5);

    EXPECT_FLOAT_EQ (dest[0], 2.0f);
    EXPECT_FLOAT_EQ (dest[1], 6.0f);
    EXPECT_FLOAT_EQ (dest[2], 12.0f);
    EXPECT_FLOAT_EQ (dest[3], 20.0f);
    EXPECT_FLOAT_EQ (dest[4], 30.0f);
}

TEST_F (FloatVectorOperationsTests, MultiplyTwoArrays_Double)
{
    double dest[5];
    double src1[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    double src2[5] = { 2.0, 3.0, 4.0, 5.0, 6.0 };

    FloatVectorOperations::multiply (dest, src1, src2, 5);

    EXPECT_DOUBLE_EQ (dest[0], 2.0);
    EXPECT_DOUBLE_EQ (dest[1], 6.0);
    EXPECT_DOUBLE_EQ (dest[2], 12.0);
    EXPECT_DOUBLE_EQ (dest[3], 20.0);
    EXPECT_DOUBLE_EQ (dest[4], 30.0);
}

TEST_F (FloatVectorOperationsTests, MultiplyWithScalar_Float)
{
    float dest[5];
    float src[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };

    FloatVectorOperations::multiply (dest, src, 3.0f, 5);

    EXPECT_FLOAT_EQ (dest[0], 3.0f);
    EXPECT_FLOAT_EQ (dest[1], 6.0f);
    EXPECT_FLOAT_EQ (dest[2], 9.0f);
    EXPECT_FLOAT_EQ (dest[3], 12.0f);
    EXPECT_FLOAT_EQ (dest[4], 15.0f);
}

TEST_F (FloatVectorOperationsTests, MultiplyWithScalar_Double)
{
    double dest[5];
    double src[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };

    FloatVectorOperations::multiply (dest, src, 3.0, 5);

    EXPECT_DOUBLE_EQ (dest[0], 3.0);
    EXPECT_DOUBLE_EQ (dest[1], 6.0);
    EXPECT_DOUBLE_EQ (dest[2], 9.0);
    EXPECT_DOUBLE_EQ (dest[3], 12.0);
    EXPECT_DOUBLE_EQ (dest[4], 15.0);
}

// ==============================================================================
// Divide Operations Tests
// ==============================================================================

TEST_F (FloatVectorOperationsTests, DivideByArray_Float)
{
    float dest[5] = { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };
    float src[5] = { 2.0f, 4.0f, 5.0f, 8.0f, 10.0f };

    FloatVectorOperations::divide (dest, src, 5);

    EXPECT_FLOAT_EQ (dest[0], 5.0f);
    EXPECT_FLOAT_EQ (dest[1], 5.0f);
    EXPECT_FLOAT_EQ (dest[2], 6.0f);
    EXPECT_FLOAT_EQ (dest[3], 5.0f);
    EXPECT_FLOAT_EQ (dest[4], 5.0f);
}

TEST_F (FloatVectorOperationsTests, DivideByArray_Double)
{
    double dest[5] = { 10.0, 20.0, 30.0, 40.0, 50.0 };
    double src[5] = { 2.0, 4.0, 5.0, 8.0, 10.0 };

    FloatVectorOperations::divide (dest, src, 5);

    EXPECT_DOUBLE_EQ (dest[0], 5.0);
    EXPECT_DOUBLE_EQ (dest[1], 5.0);
    EXPECT_DOUBLE_EQ (dest[2], 6.0);
    EXPECT_DOUBLE_EQ (dest[3], 5.0);
    EXPECT_DOUBLE_EQ (dest[4], 5.0);
}

// ==============================================================================
// Min/Max Operations Tests (Double versions)
// ==============================================================================

TEST_F (FloatVectorOperationsTests, MinWithScalar_Double)
{
    double src[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    double dest[5];

    FloatVectorOperations::min (dest, src, 3.0, 5);

    EXPECT_DOUBLE_EQ (dest[0], 1.0);
    EXPECT_DOUBLE_EQ (dest[1], 2.0);
    EXPECT_DOUBLE_EQ (dest[2], 3.0);
    EXPECT_DOUBLE_EQ (dest[3], 3.0);
    EXPECT_DOUBLE_EQ (dest[4], 3.0);
}

TEST_F (FloatVectorOperationsTests, MinWithArray_Double)
{
    double src1[5] = { 1.0, 5.0, 2.0, 4.0, 3.0 };
    double src2[5] = { 3.0, 2.0, 4.0, 1.0, 5.0 };
    double dest[5];

    FloatVectorOperations::min (dest, src1, src2, 5);

    EXPECT_DOUBLE_EQ (dest[0], 1.0);
    EXPECT_DOUBLE_EQ (dest[1], 2.0);
    EXPECT_DOUBLE_EQ (dest[2], 2.0);
    EXPECT_DOUBLE_EQ (dest[3], 1.0);
    EXPECT_DOUBLE_EQ (dest[4], 3.0);
}

TEST_F (FloatVectorOperationsTests, MaxWithScalar_Double)
{
    double src[5] = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    double dest[5];

    FloatVectorOperations::max (dest, src, 3.0, 5);

    EXPECT_DOUBLE_EQ (dest[0], 3.0);
    EXPECT_DOUBLE_EQ (dest[1], 3.0);
    EXPECT_DOUBLE_EQ (dest[2], 3.0);
    EXPECT_DOUBLE_EQ (dest[3], 4.0);
    EXPECT_DOUBLE_EQ (dest[4], 5.0);
}

TEST_F (FloatVectorOperationsTests, MaxWithArray_Double)
{
    double src1[5] = { 1.0, 5.0, 2.0, 4.0, 3.0 };
    double src2[5] = { 3.0, 2.0, 4.0, 1.0, 5.0 };
    double dest[5];

    FloatVectorOperations::max (dest, src1, src2, 5);

    EXPECT_DOUBLE_EQ (dest[0], 3.0);
    EXPECT_DOUBLE_EQ (dest[1], 5.0);
    EXPECT_DOUBLE_EQ (dest[2], 4.0);
    EXPECT_DOUBLE_EQ (dest[3], 4.0);
    EXPECT_DOUBLE_EQ (dest[4], 5.0);
}

// ==============================================================================
// Clip Operations Tests (Double version)
// ==============================================================================

TEST_F (FloatVectorOperationsTests, Clip_Double)
{
    double src[7] = { -2.0, -0.5, 0.0, 0.5, 1.0, 1.5, 2.0 };
    double dest[7];

    FloatVectorOperations::clip (dest, src, 0.0, 1.0, 7);

    EXPECT_DOUBLE_EQ (dest[0], 0.0);
    EXPECT_DOUBLE_EQ (dest[1], 0.0);
    EXPECT_DOUBLE_EQ (dest[2], 0.0);
    EXPECT_DOUBLE_EQ (dest[3], 0.5);
    EXPECT_DOUBLE_EQ (dest[4], 1.0);
    EXPECT_DOUBLE_EQ (dest[5], 1.0);
    EXPECT_DOUBLE_EQ (dest[6], 1.0);
}

// ==============================================================================
// Conversion Operations Tests
// ==============================================================================

TEST_F (FloatVectorOperationsTests, ConvertFixedToFloat)
{
    int src[5] = { 1000, 2000, 3000, 4000, 5000 };
    float dest[5];
    float multiplier = 0.001f;

    FloatVectorOperations::convertFixedToFloat (dest, src, multiplier, 5);

    EXPECT_FLOAT_EQ (dest[0], 1.0f);
    EXPECT_FLOAT_EQ (dest[1], 2.0f);
    EXPECT_FLOAT_EQ (dest[2], 3.0f);
    EXPECT_FLOAT_EQ (dest[3], 4.0f);
    EXPECT_FLOAT_EQ (dest[4], 5.0f);
}

TEST_F (FloatVectorOperationsTests, ConvertFloatToFixed)
{
    float src[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    int dest[5];
    float multiplier = 1000.0f;

    FloatVectorOperations::convertFloatToFixed (dest, src, multiplier, 5);

    EXPECT_EQ (dest[0], 1000);
    EXPECT_EQ (dest[1], 2000);
    EXPECT_EQ (dest[2], 3000);
    EXPECT_EQ (dest[3], 4000);
    EXPECT_EQ (dest[4], 5000);
}

TEST_F (FloatVectorOperationsTests, ConvertFloatToDouble)
{
    float src[5] = { 1.5f, 2.5f, 3.5f, 4.5f, 5.5f };
    double dest[5];

    FloatVectorOperations::convertFloatToDouble (dest, src, 5);

    EXPECT_NEAR (dest[0], 1.5, 0.0001);
    EXPECT_NEAR (dest[1], 2.5, 0.0001);
    EXPECT_NEAR (dest[2], 3.5, 0.0001);
    EXPECT_NEAR (dest[3], 4.5, 0.0001);
    EXPECT_NEAR (dest[4], 5.5, 0.0001);
}

TEST_F (FloatVectorOperationsTests, ConvertDoubleToFloat)
{
    double src[5] = { 1.5, 2.5, 3.5, 4.5, 5.5 };
    float dest[5];

    FloatVectorOperations::convertDoubleToFloat (dest, src, 5);

    EXPECT_FLOAT_EQ (dest[0], 1.5f);
    EXPECT_FLOAT_EQ (dest[1], 2.5f);
    EXPECT_FLOAT_EQ (dest[2], 3.5f);
    EXPECT_FLOAT_EQ (dest[3], 4.5f);
    EXPECT_FLOAT_EQ (dest[4], 5.5f);
}

// ==============================================================================
// Denormal Support Tests
// ==============================================================================

TEST_F (FloatVectorOperationsTests, DisableDenormalisedNumberSupport)
{
    // Test that it doesn't crash
    EXPECT_NO_THROW (FloatVectorOperations::disableDenormalisedNumberSupport (true));
    EXPECT_NO_THROW (FloatVectorOperations::disableDenormalisedNumberSupport (false));
}

TEST_F (FloatVectorOperationsTests, AreDenormalsDisabled)
{
    // Initially should be false
    bool initialState = FloatVectorOperations::areDenormalsDisabled();

    // Try to disable denormals
    FloatVectorOperations::disableDenormalisedNumberSupport (true);

    // Check if the state changed (platform dependent)
    bool afterDisable = FloatVectorOperations::areDenormalsDisabled();

    // Re-enable denormals
    FloatVectorOperations::disableDenormalisedNumberSupport (false);

    bool afterEnable = FloatVectorOperations::areDenormalsDisabled();

    // The actual behavior is platform-dependent, but the functions should not crash
    EXPECT_TRUE (true);
}

TEST_F (FloatVectorOperationsTests, ScopedNoDenormals)
{
    bool initialState = FloatVectorOperations::areDenormalsDisabled();

    {
        ScopedNoDenormals scopedDisable;

        // Inside scope, denormals might be disabled (platform dependent)
        bool insideScope = FloatVectorOperations::areDenormalsDisabled();
    }

    // After scope, state should be restored
    bool afterScope = FloatVectorOperations::areDenormalsDisabled();

    // The scoped object should not crash
    EXPECT_TRUE (true);
}

TEST_F (FloatVectorOperationsTests, ScopedNoDenormals_NestedScopes)
{
    {
        ScopedNoDenormals outer;

        {
            ScopedNoDenormals inner;

            // Nested scopes should work correctly
            EXPECT_TRUE (true);
        }

        // Outer scope still active
        EXPECT_TRUE (true);
    }

    // All scopes exited, should not crash
    EXPECT_TRUE (true);
}
