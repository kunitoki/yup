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
