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

            fillRandomly (random, int1, num);
            doConversionTest (data1, data2, int1, num);

            FloatVectorOperations::fill (data1, (ValueType) 2, num);
            FloatVectorOperations::fill (data2, (ValueType) 3, num);
            FloatVectorOperations::addWithMultiply (data1, data1, data2, num);
            EXPECT_TRUE (areAllValuesEqual (data1, num, (ValueType) 8));
        }

        static void doConversionTest (float* data1, float* data2, int* const int1, int num)
        {
            // Test convertFixedToFloat
            FloatVectorOperations::convertFixedToFloat (data1, int1, 2.0f, num);
            convertFixed (data2, int1, 2.0f, num);
            EXPECT_TRUE (buffersMatch (data1, data2, num));

            // Test convertFloatToFixed with the same data
            HeapBlock<int> int2 (num + 16);
            int* const intResult = addBytesToPointer (int2.get(), 0);

            FloatVectorOperations::convertFloatToFixed (intResult, data1, 0.5f, num);
            convertFloatToFixed (int1, data2, 0.5f, num);

            // Compare the integer results
            for (int i = 0; i < num; ++i)
                EXPECT_EQ (intResult[i], int1[i]);
        }

        static void doConversionTest (double* data1, double* data2, int* const int1, int num)
        {
            // Test convertFixedToFloat for double
            FloatVectorOperations::convertFixedToFloat (data1, int1, 2.0, num);
            convertFixedToDouble (data2, int1, 2.0, num);
            EXPECT_TRUE (buffersMatch (data1, data2, num));

            // Test convertFloatToFixed for double
            HeapBlock<int> int2 (num + 16);
            int* const intResult = addBytesToPointer (int2.get(), 0);

            FloatVectorOperations::convertFloatToFixed (intResult, data1, 0.5, num);
            convertDoubleToFixed (int1, data2, 0.5, num);

            // Compare the integer results
            for (int i = 0; i < num; ++i)
                EXPECT_EQ (intResult[i], int1[i]);
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

        static void convertFixed (float* d, const int* s, ValueType multiplier, int num)
        {
            while (--num >= 0)
                *d++ = (float) *s++ * multiplier;
        }

        static void convertFixedToDouble (double* d, const int* s, double multiplier, int num)
        {
            while (--num >= 0)
                *d++ = (double) *s++ * multiplier;
        }

        static void convertFloatToFixed (int* d, const float* s, float multiplier, int num)
        {
            while (--num >= 0)
                *d++ = (int) (*s++ * multiplier);
        }

        static void convertDoubleToFixed (int* d, const double* s, double multiplier, int num)
        {
            while (--num >= 0)
                *d++ = (int) (*s++ * multiplier);
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
};

TEST_F (FloatVectorOperationsTests, BasicOperations)
{
    for (int i = 1000; --i >= 0;)
    {
        TestRunner<float>::runTest (Random::getSystemRandom());
        TestRunner<double>::runTest (Random::getSystemRandom());
    }
}

TEST_F (FloatVectorOperationsTests, ConversionEdgeCases)
{
    constexpr int testSize = 256; // Use a size that will test SIMD paths

    // Test float conversion edge cases
    {
        HeapBlock<float> floatBuffer (testSize);
        HeapBlock<int> intBuffer (testSize);
        HeapBlock<int> expectedIntBuffer (testSize);
        HeapBlock<float> expectedFloatBuffer (testSize);

        // Test with zero values
        FloatVectorOperations::fill (floatBuffer.get(), 0.0f, testSize);
        FloatVectorOperations::convertFloatToFixed (intBuffer.get(), floatBuffer.get(), 32768.0f, testSize);
        for (int i = 0; i < testSize; ++i)
            EXPECT_EQ (intBuffer[i], 0);

        // Test with +/- 1.0 values (typical audio range)
        FloatVectorOperations::fill (floatBuffer.get(), 1.0f, testSize / 2);
        FloatVectorOperations::fill (floatBuffer.get() + testSize / 2, -1.0f, testSize / 2);
        FloatVectorOperations::convertFloatToFixed (intBuffer.get(), floatBuffer.get(), 32768.0f, testSize);

        for (int i = 0; i < testSize / 2; ++i)
            EXPECT_EQ (intBuffer[i], 32768);
        for (int i = testSize / 2; i < testSize; ++i)
            EXPECT_EQ (intBuffer[i], -32768);

        // Test round-trip conversion
        FloatVectorOperations::convertFixedToFloat (floatBuffer.get(), intBuffer.get(), 1.0f / 32768.0f, testSize);
        for (int i = 0; i < testSize / 2; ++i)
            EXPECT_FLOAT_EQ (floatBuffer[i], 1.0f);
        for (int i = testSize / 2; i < testSize; ++i)
            EXPECT_FLOAT_EQ (floatBuffer[i], -1.0f);
    }

    // Test double conversion edge cases
    {
        HeapBlock<double> doubleBuffer (testSize);
        HeapBlock<int> intBuffer (testSize);

        // Test with zero values
        FloatVectorOperations::fill (doubleBuffer.get(), 0.0, testSize);
        FloatVectorOperations::convertFloatToFixed (intBuffer.get(), doubleBuffer.get(), 32768.0, testSize);
        for (int i = 0; i < testSize; ++i)
            EXPECT_EQ (intBuffer[i], 0);

        // Test with +/- 1.0 values
        FloatVectorOperations::fill (doubleBuffer.get(), 1.0, testSize / 2);
        FloatVectorOperations::fill (doubleBuffer.get() + testSize / 2, -1.0, testSize / 2);
        FloatVectorOperations::convertFloatToFixed (intBuffer.get(), doubleBuffer.get(), 32768.0, testSize);

        for (int i = 0; i < testSize / 2; ++i)
            EXPECT_EQ (intBuffer[i], 32768);
        for (int i = testSize / 2; i < testSize; ++i)
            EXPECT_EQ (intBuffer[i], -32768);

        // Test round-trip conversion
        FloatVectorOperations::convertFixedToFloat (doubleBuffer.get(), intBuffer.get(), 1.0 / 32768.0, testSize);
        for (int i = 0; i < testSize / 2; ++i)
            EXPECT_DOUBLE_EQ (doubleBuffer[i], 1.0);
        for (int i = testSize / 2; i < testSize; ++i)
            EXPECT_DOUBLE_EQ (doubleBuffer[i], -1.0);
    }
}

TEST_F (FloatVectorOperationsTests, ConversionSizeVariations)
{
    // Test various buffer sizes to ensure SIMD optimizations work correctly
    const std::vector<int> testSizes = { 1, 3, 4, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128, 255, 256, 511, 512 };

    for (int size : testSizes)
    {
        HeapBlock<float> floatBuffer (size);
        HeapBlock<double> doubleBuffer (size);
        HeapBlock<int> intBuffer (size);
        HeapBlock<int> expectedIntBuffer (size);

        // Fill with known pattern
        for (int i = 0; i < size; ++i)
        {
            floatBuffer[i] = (float) i / (float) size;
            doubleBuffer[i] = (double) i / (double) size;
            intBuffer[i] = i * 1000;
        }

        // Test float conversions
        FloatVectorOperations::convertFloatToFixed (expectedIntBuffer.get(), floatBuffer.get(), 1000.0f, size);
        for (int i = 0; i < size; ++i)
        {
            int expected = (int) (floatBuffer[i] * 1000.0f);
            EXPECT_EQ (expectedIntBuffer[i], expected) << "Failed at index " << i << " for size " << size;
        }

        // Test double conversions
        FloatVectorOperations::convertFloatToFixed (expectedIntBuffer.get(), doubleBuffer.get(), 1000.0, size);
        for (int i = 0; i < size; ++i)
        {
            int expected = (int) (doubleBuffer[i] * 1000.0);
            EXPECT_EQ (expectedIntBuffer[i], expected) << "Failed at index " << i << " for size " << size;
        }

        // Test convertFixedToFloat
        FloatVectorOperations::convertFixedToFloat (floatBuffer.get(), intBuffer.get(), 0.001f, size);
        for (int i = 0; i < size; ++i)
        {
            float expected = (float) intBuffer[i] * 0.001f;
            EXPECT_FLOAT_EQ (floatBuffer[i], expected) << "Failed at index " << i << " for size " << size;
        }

        FloatVectorOperations::convertFixedToFloat (doubleBuffer.get(), intBuffer.get(), 0.001, size);
        for (int i = 0; i < size; ++i)
        {
            double expected = (double) intBuffer[i] * 0.001;
            EXPECT_DOUBLE_EQ (doubleBuffer[i], expected) << "Failed at index " << i << " for size " << size;
        }
    }
}
