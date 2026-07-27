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

#include <yup_audio_basics/yup_audio_basics.h>
#include <gtest/gtest.h>

using namespace yup;

namespace
{
template <class F1, class E1, class F2, class E2>
void testRoundTripConversion (Random& r)
{
    auto testWithInPlace = [&] (bool inPlace)
    {
        const int numSamples = 2048;
        int32 original[numSamples], converted[numSamples], reversed[numSamples];

        {
            AudioData::Pointer<F1, E1, AudioData::NonInterleaved, AudioData::NonConst> d (original);
            bool clippingFailed = false;

            for (int i = 0; i < numSamples / 2; ++i)
            {
                d.setAsFloat (r.nextFloat() * 2.2f - 1.1f);

                if (! d.isFloatingPoint())
                    clippingFailed = d.getAsFloat() > 1.0f || d.getAsFloat() < -1.0f || clippingFailed;

                ++d;
                d.setAsInt32 (r.nextInt());
                ++d;
            }

            EXPECT_FALSE (clippingFailed);
        }

        // convert data from the source to dest format..
        std::unique_ptr<AudioData::Converter> conv (new AudioData::ConverterInstance<AudioData::Pointer<F1, E1, AudioData::NonInterleaved, AudioData::Const>,
                                                                                     AudioData::Pointer<F2, E2, AudioData::NonInterleaved, AudioData::NonConst>>());
        conv->convertSamples (inPlace ? reversed : converted, original, numSamples);

        // ..and back again..
        conv.reset (new AudioData::ConverterInstance<AudioData::Pointer<F2, E2, AudioData::NonInterleaved, AudioData::Const>,
                                                     AudioData::Pointer<F1, E1, AudioData::NonInterleaved, AudioData::NonConst>>());
        if (! inPlace)
            zeromem (reversed, sizeof (reversed));

        conv->convertSamples (reversed, inPlace ? reversed : converted, numSamples);

        {
            int biggestDiff = 0;
            AudioData::Pointer<F1, E1, AudioData::NonInterleaved, AudioData::Const> d1 (original);
            AudioData::Pointer<F1, E1, AudioData::NonInterleaved, AudioData::Const> d2 (reversed);

            const int errorMargin = 2 * AudioData::Pointer<F1, E1, AudioData::NonInterleaved, AudioData::Const>::get32BitResolution()
                                  + AudioData::Pointer<F2, E2, AudioData::NonInterleaved, AudioData::Const>::get32BitResolution();

            for (int i = 0; i < numSamples; ++i)
            {
                biggestDiff = jmax (biggestDiff, std::abs (d1.getAsInt32() - d2.getAsInt32()));
                ++d1;
                ++d2;
            }

            EXPECT_LE (biggestDiff, errorMargin);
        }
    };

    testWithInPlace (false);
    testWithInPlace (true);
}

template <class F1, class E1, class FormatType>
void testAllEndianness (Random& r)
{
    testRoundTripConversion<F1, E1, FormatType, AudioData::BigEndian> (r);
    testRoundTripConversion<F1, E1, FormatType, AudioData::LittleEndian> (r);
}

template <class FormatType, class Endianness>
void testAllFormats (Random& r)
{
    testAllEndianness<FormatType, Endianness, AudioData::Int8> (r);
    testAllEndianness<FormatType, Endianness, AudioData::UInt8> (r);
    testAllEndianness<FormatType, Endianness, AudioData::Int16> (r);
    testAllEndianness<FormatType, Endianness, AudioData::Int24> (r);
    testAllEndianness<FormatType, Endianness, AudioData::Int32> (r);
    testAllEndianness<FormatType, Endianness, AudioData::Float32> (r);
}

template <class FormatType>
void testFormatWithAllEndianness (Random& r)
{
    testAllFormats<FormatType, AudioData::BigEndian> (r);
    testAllFormats<FormatType, AudioData::LittleEndian> (r);
}
} // namespace

//==============================================================================
TEST (AudioDataConvertersTest, Int8Conversions)
{
    Random& r = Random::getSystemRandom();
    testFormatWithAllEndianness<AudioData::Int8> (r);
}

TEST (AudioDataConvertersTest, UInt8Conversions)
{
    Random& r = Random::getSystemRandom();
    testFormatWithAllEndianness<AudioData::UInt8> (r);
}

TEST (AudioDataConvertersTest, Int16Conversions)
{
    Random& r = Random::getSystemRandom();
    testFormatWithAllEndianness<AudioData::Int16> (r);
}

TEST (AudioDataConvertersTest, Int24Conversions)
{
    Random& r = Random::getSystemRandom();
    testFormatWithAllEndianness<AudioData::Int24> (r);
}

TEST (AudioDataConvertersTest, Int32Conversions)
{
    Random& r = Random::getSystemRandom();
    testFormatWithAllEndianness<AudioData::Int32> (r);
}

TEST (AudioDataConvertersTest, Float32Conversions)
{
    Random& r = Random::getSystemRandom();
    testFormatWithAllEndianness<AudioData::Float32> (r);
}

TEST (AudioDataConvertersTest, Float64Conversions)
{
    //Random& r = Random::getSystemRandom();
    //testFormatWithAllEndianness<AudioData::Float64> (r);
}

//==============================================================================
TEST (AudioDataConvertersTest, PointerAdvance)
{
    float data[10] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f };
    AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const> ptr (data);

    EXPECT_FLOAT_EQ (ptr.getAsFloat(), 0.0f);
    ++ptr;
    EXPECT_FLOAT_EQ (ptr.getAsFloat(), 0.1f);
    ++ptr;
    EXPECT_FLOAT_EQ (ptr.getAsFloat(), 0.2f);
}

TEST (AudioDataConvertersTest, PointerDecrement)
{
    float data[10] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f };
    AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const> ptr (data + 5);

    EXPECT_FLOAT_EQ (ptr.getAsFloat(), 0.5f);
    --ptr;
    EXPECT_FLOAT_EQ (ptr.getAsFloat(), 0.4f);
}

TEST (AudioDataConvertersTest, PointerJump)
{
    float data[10] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f };
    AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const> ptr (data);

    ptr += 5;
    EXPECT_FLOAT_EQ (ptr.getAsFloat(), 0.5f);

    auto ptr2 = ptr + 2;
    EXPECT_FLOAT_EQ (ptr2.getAsFloat(), 0.7f);
}

TEST (AudioDataConvertersTest, InterleavedPointer)
{
    float data[8] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f };
    AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::Interleaved, AudioData::Const> ptr (data, 2);

    EXPECT_FLOAT_EQ (ptr.getAsFloat(), 0.1f);
    ++ptr;
    EXPECT_FLOAT_EQ (ptr.getAsFloat(), 0.3f);
    ++ptr;
    EXPECT_FLOAT_EQ (ptr.getAsFloat(), 0.5f);
}

TEST (AudioDataConvertersTest, ClearSamples)
{
    float data[10] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
    AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::NonConst> ptr (data);

    ptr.clearSamples (5);

    for (int i = 0; i < 5; ++i)
        EXPECT_FLOAT_EQ (data[i], 0.0f);

    EXPECT_FLOAT_EQ (data[5], 6.0f);
}

TEST (AudioDataConvertersTest, FindMinAndMax)
{
    float data[10] = { 0.1f, -0.5f, 0.8f, -0.2f, 0.4f, 0.9f, -0.7f, 0.3f, -0.1f, 0.6f };
    AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const> ptr (data);

    auto range = ptr.findMinAndMax (10);

    EXPECT_FLOAT_EQ (range.getStart(), -0.7f);
    EXPECT_FLOAT_EQ (range.getEnd(), 0.9f);
}

TEST (AudioDataConvertersTest, FindMinAndMaxEmpty)
{
    float data[1] = { 0.0f };
    AudioData::Pointer<AudioData::Float32, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const> ptr (data);

    auto range = ptr.findMinAndMax (0);

    EXPECT_TRUE (range.isEmpty());
}

TEST (AudioDataConvertersTest, FindMinAndMaxInteger)
{
    int16_t data[10];
    for (int i = 0; i < 10; ++i)
        data[i] = static_cast<int16_t> ((i - 5) * 1000);

    AudioData::Pointer<AudioData::Int16, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const> ptr (data);

    float minVal, maxVal;
    ptr.findMinAndMax (10, minVal, maxVal);

    EXPECT_LT (minVal, 0.0f);
    EXPECT_GT (maxVal, 0.0f);
}

TEST (AudioDataConvertersTest, InterleaveSamples)
{
    float sourceData1[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float sourceData2[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
    const float* sourcePtrs[2] = { sourceData1, sourceData2 };

    float dest[8];

    using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
    using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { sourcePtrs, 2 },
                                  AudioData::InterleavedDest<DestFormat> { dest, 2 },
                                  4);

    EXPECT_FLOAT_EQ (dest[0], 1.0f);
    EXPECT_FLOAT_EQ (dest[1], 5.0f);
    EXPECT_FLOAT_EQ (dest[2], 2.0f);
    EXPECT_FLOAT_EQ (dest[3], 6.0f);
}

TEST (AudioDataConvertersTest, DeinterleaveSamples)
{
    float source[8] = { 1.0f, 5.0f, 2.0f, 6.0f, 3.0f, 7.0f, 4.0f, 8.0f };

    float dest1[4];
    float dest2[4];
    float* destPtrs[2] = { dest1, dest2 };

    using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
    using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { source, 2 },
                                    AudioData::NonInterleavedDest<DestFormat> { destPtrs, 2 },
                                    4);

    EXPECT_FLOAT_EQ (dest1[0], 1.0f);
    EXPECT_FLOAT_EQ (dest1[1], 2.0f);
    EXPECT_FLOAT_EQ (dest2[0], 5.0f);
    EXPECT_FLOAT_EQ (dest2[1], 6.0f);
}
