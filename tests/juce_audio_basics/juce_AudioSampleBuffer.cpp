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

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>

using namespace juce;

template <class T>
class AudioBufferTests : public ::testing::Test
{
public:
    using BufferType = AudioBuffer<T>;

protected:
    void initializeBuffer (BufferType& buffer, int channels, int samples)
    {
        buffer.setSize (channels, samples, false, true, false);

        for (int ch = 0; ch < channels; ++ch)
        {
            T* writePtr = buffer.getWritePointer (ch);
            for (int i = 0; i < samples; ++i)
                writePtr[i] = static_cast<T> (i + 1);
        }
    }

    bool buffersAreEqual (const BufferType& a, const BufferType& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
        {
            return false;
        }

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
        {
            const T* aData = a.getReadPointer (ch);
            const T* bData = b.getReadPointer (ch);
            for (int i = 0; i < a.getNumSamples(); ++i)
            {
                if (! approximatelyEqual (aData[i], bData[i]))
                    return false;
            }
        }

        return true;
    }
};

using FloatTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE (AudioBufferTests, FloatTypes);

// Test default constructor
TYPED_TEST (AudioBufferTests, DefaultConstructor)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    EXPECT_EQ (buffer.getNumChannels(), 0);
    EXPECT_EQ (buffer.getNumSamples(), 0);
    EXPECT_TRUE (buffer.hasBeenCleared());
}

// Test constructor with numChannels and numSamples
TYPED_TEST (AudioBufferTests, ConstructorWithChannelsAndSamples)
{
    using BufferType = typename TestFixture::BufferType;

    int channels = 2;
    int samples = 100;
    BufferType buffer (channels, samples);
    EXPECT_EQ (buffer.getNumChannels(), channels);
    EXPECT_EQ (buffer.getNumSamples(), samples);
    EXPECT_FALSE (buffer.hasBeenCleared());

    // Verify that data is uninitialized (not necessarily zero)
    for (int ch = 0; ch < channels; ++ch)
    {
        const TypeParam* readPtr = buffer.getReadPointer (ch);
        for (int i = 0; i < samples; ++i)
        {
            // Since data is undefined, we cannot predict the value
            // Just ensure that pointers are not null
            EXPECT_NE (readPtr, nullptr);
        }
    }
}

// Test copy constructor
TYPED_TEST (AudioBufferTests, CopyConstructor)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType original;
    this->initializeBuffer (original, 3, 50);

    BufferType copy (original);
    EXPECT_TRUE (this->buffersAreEqual (original, copy));
    EXPECT_FALSE (copy.hasBeenCleared());
}

// Test copy assignment operator
TYPED_TEST (AudioBufferTests, CopyAssignment)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType original;
    this->initializeBuffer (original, 4, 75);

    BufferType copy;
    copy = original;
    EXPECT_TRUE (this->buffersAreEqual (original, copy));
    EXPECT_FALSE (copy.hasBeenCleared());
}

// Test move constructor
TYPED_TEST (AudioBufferTests, MoveConstructor)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType original;
    this->initializeBuffer (original, 2, 60);

    BufferType moved (std::move (original));
    EXPECT_EQ (moved.getNumChannels(), 2);
    EXPECT_EQ (moved.getNumSamples(), 60);
    EXPECT_FALSE (moved.hasBeenCleared());

    // Original should be in a valid but unspecified state
    EXPECT_EQ (original.getNumChannels(), 0);
    EXPECT_EQ (original.getNumSamples(), 0);
    EXPECT_TRUE (original.hasBeenCleared());
}

// Test move assignment operator
TYPED_TEST (AudioBufferTests, MoveAssignment)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType original;
    this->initializeBuffer (original, 5, 120);

    BufferType moved;
    moved = std::move (original);
    EXPECT_EQ (moved.getNumChannels(), 5);
    EXPECT_EQ (moved.getNumSamples(), 120);
    EXPECT_FALSE (moved.hasBeenCleared());

    // Original should be in a valid but unspecified state
    EXPECT_EQ (original.getNumChannels(), 0);
    EXPECT_EQ (original.getNumSamples(), 0);
    EXPECT_TRUE (original.hasBeenCleared());
}

// Test setSize method
TYPED_TEST (AudioBufferTests, SetSize)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (3, 150, true, true, false);
    EXPECT_EQ (buffer.getNumChannels(), 3);
    EXPECT_EQ (buffer.getNumSamples(), 150);
    EXPECT_TRUE (buffer.hasBeenCleared());

    // Verify data is zeroed if clearExtraSpace is true
    for (int ch = 0; ch < 3; ++ch)
    {
        const TypeParam* readPtr = buffer.getReadPointer (ch);
        for (int i = 0; i < 150; ++i)
            EXPECT_TRUE (approximatelyEqual (readPtr[i], static_cast<TypeParam> (0)));
    }
}

// Test setDataToReferTo method
TYPED_TEST (AudioBufferTests, SetDataToReferTo)
{
    using BufferType = typename TestFixture::BufferType;

    int channels = 2;
    int samples = 100;
    std::vector<std::vector<TypeParam>> data (channels, std::vector<TypeParam> (samples, static_cast<TypeParam> (1.0)));

    // Create array of pointers to channel data
    TypeParam* channelPointers[2];
    for (int ch = 0; ch < channels; ++ch)
        channelPointers[ch] = data[ch].data();

    BufferType buffer;
    buffer.setDataToReferTo (channelPointers, channels, 0, samples);
    EXPECT_EQ (buffer.getNumChannels(), channels);
    EXPECT_EQ (buffer.getNumSamples(), samples);
    EXPECT_FALSE (buffer.hasBeenCleared());

    // Verify data
    for (int ch = 0; ch < channels; ++ch)
    {
        const TypeParam* readPtr = buffer.getReadPointer (ch);
        for (int i = 0; i < samples; ++i)
            EXPECT_TRUE (approximatelyEqual (readPtr[i], static_cast<TypeParam> (1.0)));
    }
}

// Test clear and hasBeenCleared methods
TYPED_TEST (AudioBufferTests, ClearAndHasBeenCleared)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    this->initializeBuffer (buffer, 2, 50);
    EXPECT_FALSE (buffer.hasBeenCleared());

    buffer.clear();
    EXPECT_TRUE (buffer.hasBeenCleared());

    // Verify all samples are zero
    for (int ch = 0; ch < 2; ++ch)
    {
        const TypeParam* readPtr = buffer.getReadPointer (ch);
        for (int i = 0; i < 50; ++i)
            EXPECT_TRUE (approximatelyEqual (readPtr[i], static_cast<TypeParam> (0)));
    }

    // Clear a region
    buffer.setNotClear();
    buffer.setSample (0, 0, static_cast<TypeParam> (5.0));
    buffer.clear (0, 0, 1);
    EXPECT_FALSE (buffer.hasBeenCleared());
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (0)));
}

// Test getSample and setSample methods
TYPED_TEST (AudioBufferTests, GetAndSetSample)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 10, false, false, false);
    EXPECT_FALSE (buffer.hasBeenCleared());

    buffer.setSample (0, 5, static_cast<TypeParam> (3.14));
    EXPECT_FALSE (buffer.hasBeenCleared());
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 5), static_cast<TypeParam> (3.14)));

    // Overwrite the sample
    buffer.setSample (0, 5, static_cast<TypeParam> (2.71));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 5), static_cast<TypeParam> (2.71)));
}

// Test addSample method
TYPED_TEST (AudioBufferTests, AddSample)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 5, false, true, false);
    buffer.addSample (0, 2, static_cast<TypeParam> (1.5));
    EXPECT_FALSE (buffer.hasBeenCleared());
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (1.5)));

    buffer.addSample (0, 2, static_cast<TypeParam> (2.5));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (4.0)));
}

// Test applyGain method
TYPED_TEST (AudioBufferTests, ApplyGain)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    this->initializeBuffer (buffer, 2, 4);
    EXPECT_FALSE (buffer.hasBeenCleared());

    buffer.applyGain (0, 0, 4, static_cast<TypeParam> (2.0));
    for (int i = 0; i < 4; ++i)
        EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, i), static_cast<TypeParam> ((i + 1) * 2.0)));

    buffer.applyGain (0, 0, 4, static_cast<TypeParam> (0.5));

    buffer.applyGain (static_cast<TypeParam> (0.5));
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 4; ++i)
            EXPECT_TRUE (approximatelyEqual (buffer.getSample (ch, i), static_cast<TypeParam> ((i + 1) * 0.5)));
    }

    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test applyGainRamp method
TYPED_TEST (AudioBufferTests, ApplyGainRamp)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 5, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (3.0));
    buffer.setSample (0, 3, static_cast<TypeParam> (4.0));
    buffer.setSample (0, 4, static_cast<TypeParam> (5.0));

    buffer.applyGainRamp (0, 0, 5, static_cast<TypeParam> (1.0), static_cast<TypeParam> (2.0));

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (2.5)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (4.5)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 3), static_cast<TypeParam> (7.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 4), static_cast<TypeParam> (10.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test addFrom method
TYPED_TEST (AudioBufferTests, AddFromBuffer)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    this->initializeBuffer (dest, 1, 3); // dest: [1,2,3]
    this->initializeBuffer (src, 1, 3);  // src: [1,2,3]

    dest.addFrom (0, 0, src, 0, 0, 3, static_cast<TypeParam> (1.0));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 0), static_cast<TypeParam> (2.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 1), static_cast<TypeParam> (4.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 2), static_cast<TypeParam> (6.0)));
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test addFrom with gain
TYPED_TEST (AudioBufferTests, AddFromWithGain)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    this->initializeBuffer (dest, 1, 2); // dest: [1,2]
    this->initializeBuffer (src, 1, 2);  // src: [1,2]

    dest.addFrom (0, 0, src, 0, 0, 2, static_cast<TypeParam> (3.0));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 0), static_cast<TypeParam> (4.0))); // 1 + 1*3
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 1), static_cast<TypeParam> (8.0))); // 2 + 2*3
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test copyFrom method
TYPED_TEST (AudioBufferTests, CopyFromBuffer)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    this->initializeBuffer (src, 2, 4); // src channel 0: [1,2,3,4], channel 1: [1,2,3,4]

    dest.setSize (2, 4, false, false, false);
    dest.copyFrom (0, 0, src, 0, 0, 4);
    dest.copyFrom (1, 0, src, 1, 0, 4);

    EXPECT_TRUE (this->buffersAreEqual (dest, src));
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test copyFrom with gain
TYPED_TEST (AudioBufferTests, CopyFromWithGain)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    this->initializeBuffer (src, 1, 3); // src: [1,2,3]

    dest.setSize (1, 3, false, false, false);
    dest.copyFrom (0, 0, src.getReadPointer (0), 3, static_cast<TypeParam> (2.0));

    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 0), static_cast<TypeParam> (2.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 1), static_cast<TypeParam> (4.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 2), static_cast<TypeParam> (6.0)));
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test findMinMax method
TYPED_TEST (AudioBufferTests, FindMinMax)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 5, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (-1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (-3.0));
    buffer.setSample (0, 3, static_cast<TypeParam> (4.0));
    buffer.setSample (0, 4, static_cast<TypeParam> (-5.0));

    auto range = buffer.findMinMax (0, 0, 5);
    EXPECT_TRUE (approximatelyEqual (range.getStart(), static_cast<TypeParam> (-5.0)));
    EXPECT_TRUE (approximatelyEqual (range.getEnd(), static_cast<TypeParam> (4.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test getMagnitude method
TYPED_TEST (AudioBufferTests, GetMagnitude)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (2, 4, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (-3.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (4.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (-5.0));
    buffer.setSample (0, 3, static_cast<TypeParam> (6.0));

    buffer.setSample (1, 0, static_cast<TypeParam> (-2.0));
    buffer.setSample (1, 1, static_cast<TypeParam> (3.0));
    buffer.setSample (1, 2, static_cast<TypeParam> (-4.0));
    buffer.setSample (1, 3, static_cast<TypeParam> (5.0));

    TypeParam mag = buffer.getMagnitude (0, 0, 4);
    EXPECT_TRUE (approximatelyEqual (mag, static_cast<TypeParam> (6.0)));

    mag = buffer.getMagnitude (1, 0, 4);
    EXPECT_TRUE (approximatelyEqual (mag, static_cast<TypeParam> (5.0)));

    mag = buffer.getMagnitude (0, 1, 2);
    EXPECT_TRUE (approximatelyEqual (mag, static_cast<TypeParam> (5.0)));

    mag = buffer.getMagnitude (1, 1, 2);
    EXPECT_TRUE (approximatelyEqual (mag, static_cast<TypeParam> (4.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test getRMSLevel method
TYPED_TEST (AudioBufferTests, GetRMSLevel)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 4, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (3.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (4.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (0.0));
    buffer.setSample (0, 3, static_cast<TypeParam> (-5.0));

    double expectedRMS = std::sqrt ((9.0 + 16.0 + 0.0 + 25.0) / 4.0);
    EXPECT_TRUE (approximatelyEqual (buffer.getRMSLevel (0, 0, 4), static_cast<TypeParam> (expectedRMS)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test reverse method
TYPED_TEST (AudioBufferTests, Reverse)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 5, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (3.0));
    buffer.setSample (0, 3, static_cast<TypeParam> (4.0));
    buffer.setSample (0, 4, static_cast<TypeParam> (5.0));

    buffer.reverse (0, 1, 3); // Reverse samples 1,2,3

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (4.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (3.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 3), static_cast<TypeParam> (2.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 4), static_cast<TypeParam> (5.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());

    // Reverse entire buffer
    buffer.reverse (0, 0, 5);
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (5.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (2.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (3.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 3), static_cast<TypeParam> (4.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 4), static_cast<TypeParam> (1.0)));
}

// Test operator== and operator!=
TYPED_TEST (AudioBufferTests, EqualityOperators)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer1, buffer2;
    this->initializeBuffer (buffer1, 2, 3);
    this->initializeBuffer (buffer2, 2, 3);

    EXPECT_TRUE (buffer1 == buffer2);
    EXPECT_FALSE (buffer1 != buffer2);

    buffer2.setSample (1, 2, static_cast<TypeParam> (999.0));
    EXPECT_FALSE (buffer1 == buffer2);
    EXPECT_TRUE (buffer1 != buffer2);
}

// Test makeCopyOf method
TYPED_TEST (AudioBufferTests, MakeCopyOf)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType src;
    this->initializeBuffer (src, 2, 4); // src: [1,2,3,4] in both channels

    // Create a destination buffer with different type
    using OtherType = typename std::conditional<std::is_same<TypeParam, float>::value, double, float>::type;
    AudioBuffer<OtherType> dest;
    dest.makeCopyOf (src, false);

    EXPECT_EQ (dest.getNumChannels(), src.getNumChannels());
    EXPECT_EQ (dest.getNumSamples(), src.getNumSamples());
    EXPECT_FALSE (dest.hasBeenCleared());

    for (int ch = 0; ch < src.getNumChannels(); ++ch)
    {
        for (int i = 0; i < src.getNumSamples(); ++i)
            EXPECT_TRUE (approximatelyEqual (dest.getReadPointer (ch)[i], static_cast<OtherType> (src.getReadPointer (ch)[i])));
    }
}

// Test operator!=
TYPED_TEST (AudioBufferTests, NotEqualOperator)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer1, buffer2;
    buffer1.setSize (1, 2, false, false, false);
    buffer2.setSize (1, 2, false, false, false);

    buffer1.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer2.setSample (0, 0, static_cast<TypeParam> (2.0));

    EXPECT_TRUE (buffer1 != buffer2);
}

// Test getArrayOfReadPointers and getArrayOfWritePointers
TYPED_TEST (AudioBufferTests, ArrayOfPointers)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (2, 3, false, false, false);

    // Initialize data
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (3.0));

    buffer.setSample (1, 0, static_cast<TypeParam> (4.0));
    buffer.setSample (1, 1, static_cast<TypeParam> (5.0));
    buffer.setSample (1, 2, static_cast<TypeParam> (6.0));

    // Get read pointers
    const TypeParam* const* readPtrs = buffer.getArrayOfReadPointers();
    EXPECT_TRUE (approximatelyEqual (readPtrs[0][0], static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (readPtrs[0][1], static_cast<TypeParam> (2.0)));
    EXPECT_TRUE (approximatelyEqual (readPtrs[0][2], static_cast<TypeParam> (3.0)));

    EXPECT_TRUE (approximatelyEqual (readPtrs[1][0], static_cast<TypeParam> (4.0)));
    EXPECT_TRUE (approximatelyEqual (readPtrs[1][1], static_cast<TypeParam> (5.0)));
    EXPECT_TRUE (approximatelyEqual (readPtrs[1][2], static_cast<TypeParam> (6.0)));

    // Get write pointers and modify
    TypeParam* const* writePtrs = buffer.getArrayOfWritePointers();
    writePtrs[0][0] = static_cast<TypeParam> (7.0);
    writePtrs[1][2] = static_cast<TypeParam> (8.0);

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (7.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (1, 2), static_cast<TypeParam> (8.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test copyFromWithRamp method
TYPED_TEST (AudioBufferTests, CopyFromWithRamp)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    src.setSize (1, 4, false, false, false);
    src.setSample (0, 0, static_cast<TypeParam> (1.0));
    src.setSample (0, 1, static_cast<TypeParam> (2.0));
    src.setSample (0, 2, static_cast<TypeParam> (3.0));
    src.setSample (0, 3, static_cast<TypeParam> (4.0));

    dest.setSize (1, 4, false, false, false);
    dest.copyFromWithRamp (0, 0, src.getReadPointer (0), 4, static_cast<TypeParam> (1.0), static_cast<TypeParam> (2.0));

    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 1), static_cast<TypeParam> (2.6666666666666665)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 2), static_cast<TypeParam> (5.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 3), static_cast<TypeParam> (8.0)));
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test applyGain with entire buffer
TYPED_TEST (AudioBufferTests, ApplyGainEntireBuffer)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    this->initializeBuffer (buffer, 2, 3); // [1,2,3], [1,2,3]

    buffer.applyGain (static_cast<TypeParam> (3.0));
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 3; ++i)
            EXPECT_TRUE (approximatelyEqual (buffer.getSample (ch, i), static_cast<TypeParam> ((i + 1) * 3.0)));
    }

    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test addFrom with overlapping regions (self-addition)
TYPED_TEST (AudioBufferTests, DISABLED_AddFromSelf)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 4, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (3.0));
    buffer.setSample (0, 3, static_cast<TypeParam> (4.0));

    // Add channel 0 to itself with gain 1
    buffer.addFrom (0, 0, buffer, 0, 0, 4, static_cast<TypeParam> (1.0));

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (2.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (4.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (6.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 3), static_cast<TypeParam> (8.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test setNotClear method
TYPED_TEST (AudioBufferTests, SetNotClear)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 2, true, true, false); // Clear buffer
    EXPECT_TRUE (buffer.hasBeenCleared());

    buffer.setSample (0, 0, static_cast<TypeParam> (5.0));
    EXPECT_FALSE (buffer.hasBeenCleared());

    buffer.clear();
    EXPECT_TRUE (buffer.hasBeenCleared());

    buffer.setNotClear();
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test makeCopyOf with different types
TYPED_TEST (AudioBufferTests, MakeCopyOfDifferentTypes)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType src;
    this->initializeBuffer (src, 1, 3); // [1,2,3]

    // Create a buffer of the same type and make a copy
    BufferType copy;
    copy.makeCopyOf (src, false);
    EXPECT_TRUE (this->buffersAreEqual (src, copy));
    EXPECT_FALSE (copy.hasBeenCleared());

    // Create a buffer of different type and make a copy
    using OtherType = typename std::conditional<std::is_same<TypeParam, float>::value, double, float>::type;
    AudioBuffer<OtherType> copyOther;
    copyOther.makeCopyOf (src, false);
    EXPECT_EQ (copyOther.getNumChannels(), src.getNumChannels());
    EXPECT_EQ (copyOther.getNumSamples(), src.getNumSamples());
    for (int ch = 0; ch < src.getNumChannels(); ++ch)
    {
        for (int i = 0; i < src.getNumSamples(); ++i)
            EXPECT_TRUE (approximatelyEqual (copyOther.getReadPointer (ch)[i], static_cast<OtherType> (src.getReadPointer (ch)[i])));
    }

    EXPECT_FALSE (copyOther.hasBeenCleared());
}

// Test clear with partial region
TYPED_TEST (AudioBufferTests, ClearPartialRegion)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 5, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (3.0));
    buffer.setSample (0, 3, static_cast<TypeParam> (4.0));
    buffer.setSample (0, 4, static_cast<TypeParam> (5.0));

    buffer.clear (1, 3); // Clear samples 1,2,3

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (0.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (0.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 3), static_cast<TypeParam> (0.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 4), static_cast<TypeParam> (5.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());

    // Check hasBeenCleared when entire buffer is cleared
    buffer.clear (0, 5);
    EXPECT_TRUE (buffer.hasBeenCleared());
}

// Test copy constructor with isClear flag
TYPED_TEST (AudioBufferTests, CopyConstructorWithClearFlag)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType original;
    original.setSize (2, 2, true, true, false); // Clear buffer
    BufferType copy (original);
    EXPECT_EQ (copy.getNumChannels(), original.getNumChannels());
    EXPECT_EQ (copy.getNumSamples(), original.getNumSamples());
    EXPECT_TRUE (copy.hasBeenCleared());

    // Now set some data and copy again
    original.setSample (0, 0, static_cast<TypeParam> (1.0));
    original.setSample (1, 1, static_cast<TypeParam> (2.0));
    BufferType copy2 (original);
    EXPECT_FALSE (copy2.hasBeenCleared());
    EXPECT_TRUE (approximatelyEqual (copy2.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (copy2.getSample (1, 1), static_cast<TypeParam> (2.0)));
}

// Test copy assignment with isClear flag
TYPED_TEST (AudioBufferTests, CopyAssignmentWithClearFlag)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType original;
    original.setSize (1, 1, true, true, false); // Clear buffer
    BufferType copy;
    copy = original;
    EXPECT_EQ (copy.getNumChannels(), original.getNumChannels());
    EXPECT_EQ (copy.getNumSamples(), original.getNumSamples());
    EXPECT_TRUE (copy.hasBeenCleared());

    // Now set data and copy again
    original.setSample (0, 0, static_cast<TypeParam> (5.0));
    copy = original;
    EXPECT_FALSE (copy.hasBeenCleared());
    EXPECT_TRUE (approximatelyEqual (copy.getSample (0, 0), static_cast<TypeParam> (5.0)));
}

// Test copyFrom with self-copy (should handle correctly)
TYPED_TEST (AudioBufferTests, DISABLED_CopyFromSelf)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 3, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (3.0));

    buffer.copyFrom (0, 0, buffer, 0, 0, 3);
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (2.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (3.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test addFromWithRamp method
TYPED_TEST (AudioBufferTests, AddFromWithRamp)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    dest.setSize (1, 3, false, false, false);
    dest.setSample (0, 0, static_cast<TypeParam> (1.0));
    dest.setSample (0, 1, static_cast<TypeParam> (2.0));
    dest.setSample (0, 2, static_cast<TypeParam> (3.0));

    src.setSize (1, 3, false, false, false);
    src.setSample (0, 0, static_cast<TypeParam> (4.0));
    src.setSample (0, 1, static_cast<TypeParam> (5.0));
    src.setSample (0, 2, static_cast<TypeParam> (6.0));

    dest.addFromWithRamp (0, 0, src.getReadPointer (0), 3, static_cast<TypeParam> (1.0), static_cast<TypeParam> (2.0));

    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 0), static_cast<TypeParam> (1.0 + 4.0 * 1.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 1), static_cast<TypeParam> (2.0 + 5.0 * 1.5)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 2), static_cast<TypeParam> (3.0 + 6.0 * 2.0)));
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test getWritePointer marks buffer as not clear
TYPED_TEST (AudioBufferTests, GetWritePointerMarksNotClear)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 2, true, true, false); // Clear buffer
    EXPECT_TRUE (buffer.hasBeenCleared());

    TypeParam* writePtr = buffer.getWritePointer (0);
    writePtr[0] = static_cast<TypeParam> (10.0);
    EXPECT_FALSE (buffer.hasBeenCleared());
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (10.0)));
}

// Test getReadPointer does not modify clear flag
TYPED_TEST (AudioBufferTests, GetReadPointerDoesNotModifyClear)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 3, true, true, false); // Clear buffer
    EXPECT_TRUE (buffer.hasBeenCleared());

    const TypeParam* readPtr = buffer.getReadPointer (0);
    (void) readPtr; // Suppress unused variable warning
    EXPECT_TRUE (buffer.hasBeenCleared());
}

// Test multiple channels
TYPED_TEST (AudioBufferTests, MultipleChannels)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (3, 4, false, false, false);
    for (int ch = 0; ch < 3; ++ch)
    {
        for (int i = 0; i < 4; ++i)
            buffer.setSample (ch, i, static_cast<TypeParam> (ch * 10 + i));
    }

    for (int ch = 0; ch < 3; ++ch)
    {
        for (int i = 0; i < 4; ++i)
            EXPECT_TRUE (approximatelyEqual (buffer.getSample (ch, i), static_cast<TypeParam> (ch * 10 + i)));
    }

    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test makeCopyOf with avoidReallocating
TYPED_TEST (AudioBufferTests, MakeCopyOfAvoidReallocating)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType src;
    src.setSize (2, 2, false, false, false);
    src.setSample (0, 0, static_cast<TypeParam> (1.0));
    src.setSample (0, 1, static_cast<TypeParam> (2.0));
    src.setSample (1, 0, static_cast<TypeParam> (3.0));
    src.setSample (1, 1, static_cast<TypeParam> (4.0));

    BufferType dest;
    dest.setSize (2, 2, true, true, false); // Initially cleared
    dest.makeCopyOf (src, true);
    EXPECT_TRUE (dest == src);
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test handling zero channels and zero samples
TYPED_TEST (AudioBufferTests, ZeroChannelsAndSamples)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (0, 0, false, false, false);
    EXPECT_EQ (buffer.getNumChannels(), 0);
    EXPECT_EQ (buffer.getNumSamples(), 0);
    EXPECT_TRUE (buffer.hasBeenCleared());

    // Attempt to set size with zero channels but some samples
    buffer.setSize (0, 10, false, false, false);
    EXPECT_EQ (buffer.getNumChannels(), 0);
    EXPECT_EQ (buffer.getNumSamples(), 10);
    EXPECT_FALSE (buffer.hasBeenCleared());

    // Attempt to set size with some channels but zero samples
    buffer.setSize (2, 0, false, false, false);
    EXPECT_EQ (buffer.getNumChannels(), 2);
    EXPECT_EQ (buffer.getNumSamples(), 0);
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test clear with entire buffer
TYPED_TEST (AudioBufferTests, ClearEntireBuffer)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (2, 2, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (1, 0, static_cast<TypeParam> (3.0));
    buffer.setSample (1, 1, static_cast<TypeParam> (4.0));

    buffer.clear();
    EXPECT_TRUE (buffer.hasBeenCleared());

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < 2; ++i)
            EXPECT_TRUE (approximatelyEqual (buffer.getSample (ch, i), static_cast<TypeParam> (0.0)));
    }
}

// Test addFrom with zero samples (should do nothing)
TYPED_TEST (AudioBufferTests, AddFromZeroSamples)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    dest.setSize (1, 2, false, false, false);
    src.setSize (1, 2, false, false, false);
    dest.setSample (0, 0, static_cast<TypeParam> (1.0));
    dest.setSample (0, 1, static_cast<TypeParam> (2.0));
    src.setSample (0, 0, static_cast<TypeParam> (3.0));
    src.setSample (0, 1, static_cast<TypeParam> (4.0));

    dest.addFrom (0, 0, src, 0, 0, 0, static_cast<TypeParam> (1.0));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 1), static_cast<TypeParam> (2.0)));
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test copyFrom with zero samples (should do nothing)
TYPED_TEST (AudioBufferTests, CopyFromZeroSamples)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    dest.setSize (1, 2, false, false, false);
    src.setSize (1, 2, false, false, false);
    dest.setSample (0, 0, static_cast<TypeParam> (1.0));
    dest.setSample (0, 1, static_cast<TypeParam> (2.0));
    src.setSample (0, 0, static_cast<TypeParam> (3.0));
    src.setSample (0, 1, static_cast<TypeParam> (4.0));

    dest.copyFrom (0, 0, src, 0, 0, 0);
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 1), static_cast<TypeParam> (2.0)));
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test applyGain with zero gain (should clear buffer)
TYPED_TEST (AudioBufferTests, ApplyZeroGain)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    this->initializeBuffer (buffer, 1, 3);
    EXPECT_FALSE (buffer.hasBeenCleared());

    buffer.applyGain (static_cast<TypeParam> (0.0));
    EXPECT_TRUE (buffer.hasBeenCleared());

    for (int ch = 0; ch < 1; ++ch)
    {
        for (int i = 0; i < 3; ++i)
            EXPECT_TRUE (approximatelyEqual (buffer.getSample (ch, i), static_cast<TypeParam> (0.0)));
    }
}

// Test applyGainRamp with zero gain
TYPED_TEST (AudioBufferTests, ApplyGainRampToZero)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 3, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (3.0));

    buffer.applyGainRamp (0, 3, static_cast<TypeParam> (0.0), static_cast<TypeParam> (0.0));
    EXPECT_TRUE (buffer.hasBeenCleared());

    for (int ch = 0; ch < 1; ++ch)
    {
        for (int i = 0; i < 3; ++i)
            EXPECT_TRUE (approximatelyEqual (buffer.getSample (ch, i), static_cast<TypeParam> (0.0)));
    }
}

// Test reverse on cleared buffer (should do nothing)
TYPED_TEST (AudioBufferTests, ReverseClearedBuffer)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 3, true, true, false); // Cleared buffer

    // Attempt to reverse
    buffer.reverse (0, 0, 3);
    EXPECT_TRUE (buffer.hasBeenCleared());

    // All samples should still be zero
    for (int i = 0; i < 3; ++i)
        EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, i), static_cast<TypeParam> (0.0)));
}

// Test addFrom with gain ramp
TYPED_TEST (AudioBufferTests, AddFromWithGainRamp)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    dest.setSize (1, 3, false, false, false);
    dest.setSample (0, 0, static_cast<TypeParam> (1.0));
    dest.setSample (0, 1, static_cast<TypeParam> (2.0));
    dest.setSample (0, 2, static_cast<TypeParam> (3.0));

    src.setSize (1, 3, false, false, false);
    src.setSample (0, 0, static_cast<TypeParam> (1.0));
    src.setSample (0, 1, static_cast<TypeParam> (1.0));
    src.setSample (0, 2, static_cast<TypeParam> (1.0));

    dest.addFromWithRamp (0, 0, src.getReadPointer (0), 3, static_cast<TypeParam> (1.0), static_cast<TypeParam> (2.0));

    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 0), static_cast<TypeParam> (2.0))); // 1 + 1*1
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 1), static_cast<TypeParam> (3.5))); // 2 + 1*1.5
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 2), static_cast<TypeParam> (5.0))); // 3 + 1*2
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test copyFromWithRamp
TYPED_TEST (AudioBufferTests, CopyFromWithRampEntireBuffer)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType dest, src;
    src.setSize (1, 4, false, false, false);
    src.setSample (0, 0, static_cast<TypeParam> (1.0));
    src.setSample (0, 1, static_cast<TypeParam> (2.0));
    src.setSample (0, 2, static_cast<TypeParam> (3.0));
    src.setSample (0, 3, static_cast<TypeParam> (4.0));

    dest.setSize (1, 4, false, false, false);
    dest.copyFromWithRamp (0, 0, src.getReadPointer (0), 4, static_cast<TypeParam> (1.0), static_cast<TypeParam> (3.0));

    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 1), static_cast<TypeParam> (3.333333333333333)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 2), static_cast<TypeParam> (6.9999999999999991)));
    EXPECT_TRUE (approximatelyEqual (dest.getSample (0, 3), static_cast<TypeParam> (11.999999999999998)));
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test findMinMax on cleared buffer
TYPED_TEST (AudioBufferTests, FindMinMaxClearedBuffer)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 5, true, true, false); // Cleared buffer

    auto range = buffer.findMinMax (0, 0, 5);
    EXPECT_TRUE (approximatelyEqual (range.getStart(), static_cast<TypeParam> (0.0)));
    EXPECT_TRUE (approximatelyEqual (range.getEnd(), static_cast<TypeParam> (0.0)));
}

// Test getRMSLevel on cleared buffer
TYPED_TEST (AudioBufferTests, GetRMSLevelClearedBuffer)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 4, true, true, false); // Cleared buffer

    EXPECT_TRUE (approximatelyEqual (buffer.getRMSLevel (0, 0, 4), static_cast<TypeParam> (0.0)));
}

// Test getSample out of range (should trigger assert, but not tested here)
TYPED_TEST (AudioBufferTests, GetSampleOutOfRange)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 2, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));

    // Accessing out of range is undefined behavior; here we just ensure it doesn't crash
    // In real tests, you'd use EXPECT_DEATH or similar, but it's not portable
    SUCCEED();
}

// Test setSize with keepExistingContent
TYPED_TEST (AudioBufferTests, SetSizeKeepExistingContent)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    this->initializeBuffer (buffer, 2, 3); // [1,2,3], [1,2,3]

    buffer.setSize (2, 5, true, true, false);
    EXPECT_EQ (buffer.getNumChannels(), 2);
    EXPECT_EQ (buffer.getNumSamples(), 5);
    EXPECT_FALSE (buffer.hasBeenCleared());

    // Existing data should be preserved
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (2.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (3.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 3), static_cast<TypeParam> (0.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 4), static_cast<TypeParam> (0.0)));

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (1, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (1, 1), static_cast<TypeParam> (2.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (1, 2), static_cast<TypeParam> (3.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (1, 3), static_cast<TypeParam> (0.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (1, 4), static_cast<TypeParam> (0.0)));
}

// Test setSize with avoidReallocating
TYPED_TEST (AudioBufferTests, SetSizeAvoidReallocating)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (2, 4, false, false, false);
    this->initializeBuffer (buffer, 2, 4); // [1,2,3,4], [1,2,3,4]

    // Resize to smaller size without reallocating
    buffer.setSize (2, 2, true, true, true);
    EXPECT_EQ (buffer.getNumChannels(), 2);
    EXPECT_EQ (buffer.getNumSamples(), 2);
    EXPECT_FALSE (buffer.hasBeenCleared());

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (2.0)));

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (1, 0), static_cast<TypeParam> (1.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (1, 1), static_cast<TypeParam> (2.0)));
}

// Test setSize with avoidReallocating when increasing size
TYPED_TEST (AudioBufferTests, SetSizeAvoidReallocatingIncreasingSize)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (2, 2, false, false, true); // Initially 2 samples

    buffer.setSize (2, 4, false, false, true); // Increase size, should reallocate
    EXPECT_EQ (buffer.getNumChannels(), 2);
    EXPECT_EQ (buffer.getNumSamples(), 4);
}

// Test makeCopyOf with avoidReallocating set to true
TYPED_TEST (AudioBufferTests, MakeCopyOfWithAvoidReallocating)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType src;
    src.setSize (2, 3, false, false, false);
    this->initializeBuffer (src, 2, 3);

    BufferType dest;
    dest.setSize (2, 3, true, true, true); // Preallocate with avoidReallocating=true
    dest.makeCopyOf (src, true);

    EXPECT_TRUE (this->buffersAreEqual (src, dest));
    EXPECT_FALSE (dest.hasBeenCleared());
}

// Test setDataToReferTo with offset
TYPED_TEST (AudioBufferTests, SetDataToReferToWithOffset)
{
    using BufferType = typename TestFixture::BufferType;

    int channels = 1;
    int totalSamples = 5;
    int offset = 2;
    std::vector<TypeParam> data (totalSamples, static_cast<TypeParam> (0.0));
    data[2] = static_cast<TypeParam> (3.0);
    data[3] = static_cast<TypeParam> (4.0);
    data[4] = static_cast<TypeParam> (5.0);

    TypeParam* channelPointers[1];
    channelPointers[0] = data.data();

    BufferType buffer;
    buffer.setDataToReferTo (channelPointers, channels, offset, 3);
    EXPECT_EQ (buffer.getNumChannels(), channels);
    EXPECT_EQ (buffer.getNumSamples(), 3);
    EXPECT_FALSE (buffer.hasBeenCleared());

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (3.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (4.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (5.0)));
}

// Test setSize with avoidReallocating set to false
TYPED_TEST (AudioBufferTests, SetSizeWithoutAvoidReallocating)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (2, 2, false, false, true); // Preallocate with avoidReallocating=true

    buffer.setSize (2, 3, false, false, false); // Increase size, allow reallocation
    EXPECT_EQ (buffer.getNumChannels(), 2);
    EXPECT_EQ (buffer.getNumSamples(), 3);
}

// Test applyGainRamp with increasing gain
TYPED_TEST (AudioBufferTests, ApplyGainRampIncreasing)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 4, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (1.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (3.0));
    buffer.setSample (0, 3, static_cast<TypeParam> (4.0));

    buffer.applyGainRamp (0, 0, 4, static_cast<TypeParam> (1.0), static_cast<TypeParam> (4.0));

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (1.0 * 1.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (2.0 * 2.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (3.0 * 3.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 3), static_cast<TypeParam> (4.0 * 4.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test applyGainRamp with decreasing gain
TYPED_TEST (AudioBufferTests, ApplyGainRampDecreasing)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 4, false, false, false);
    buffer.setSample (0, 0, static_cast<TypeParam> (4.0));
    buffer.setSample (0, 1, static_cast<TypeParam> (3.0));
    buffer.setSample (0, 2, static_cast<TypeParam> (2.0));
    buffer.setSample (0, 3, static_cast<TypeParam> (1.0));

    buffer.applyGainRamp (0, 0, 4, static_cast<TypeParam> (4.0), static_cast<TypeParam> (1.0));

    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (4.0 * 4.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (3.0 * 3.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (2.0 * 2.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 3), static_cast<TypeParam> (1.0 * 1.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test getWritePointer with sample index
TYPED_TEST (AudioBufferTests, GetWritePointerWithSampleIndex)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    buffer.setSize (1, 5, true, true, false); // Cleared buffer

    TypeParam* writePtr = buffer.getWritePointer (0, 2);
    writePtr[0] = static_cast<TypeParam> (7.0);
    writePtr[1] = static_cast<TypeParam> (8.0);
    buffer.setNotClear();

    EXPECT_FALSE (buffer.hasBeenCleared());
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 2), static_cast<TypeParam> (7.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 3), static_cast<TypeParam> (8.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 0), static_cast<TypeParam> (0.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 1), static_cast<TypeParam> (0.0)));
    EXPECT_TRUE (approximatelyEqual (buffer.getSample (0, 4), static_cast<TypeParam> (0.0)));
}

// Test addFrom with external data buffer
TYPED_TEST (AudioBufferTests, AddFromExternalData)
{
    using BufferType = typename TestFixture::BufferType;

    int channels = 1;
    int samples = 3;
    std::vector<TypeParam> externalData = { static_cast<TypeParam> (1.0), static_cast<TypeParam> (2.0), static_cast<TypeParam> (3.0) };

    TypeParam* channelPointers[1] = { externalData.data() };
    BufferType buffer;
    buffer.setDataToReferTo (channelPointers, channels, 0, samples);
    EXPECT_EQ (buffer.getNumChannels(), channels);
    EXPECT_EQ (buffer.getNumSamples(), samples);
    EXPECT_FALSE (buffer.hasBeenCleared());

    buffer.addSample (0, 1, static_cast<TypeParam> (5.0)); // externalData[1] += 5
    EXPECT_TRUE (approximatelyEqual (externalData[1], static_cast<TypeParam> (7.0)));
    EXPECT_FALSE (buffer.hasBeenCleared());
}

// Test addFrom with external data and gain
TYPED_TEST (AudioBufferTests, DISABLED_AddFromExternalDataWithGain)
{
    using BufferType = typename TestFixture::BufferType;

    int channels = 1;
    int samples = 2;
    std::vector<TypeParam> externalData = { static_cast<TypeParam> (2.0), static_cast<TypeParam> (3.0) };

    TypeParam* channelPointers[1] = { externalData.data() };
    BufferType buffer;
    buffer.setDataToReferTo (channelPointers, channels, 0, samples);
    EXPECT_EQ (buffer.getNumChannels(), channels);
    EXPECT_EQ (buffer.getNumSamples(), samples);
    EXPECT_FALSE (buffer.hasBeenCleared());

    buffer.addFrom (0, 0, buffer, 0, 0, 2, static_cast<TypeParam> (2.0));             // externalData += externalData * 2
    EXPECT_TRUE (approximatelyEqual (externalData[0], static_cast<TypeParam> (6.0))); // 2 + 2*2
    EXPECT_TRUE (approximatelyEqual (externalData[1], static_cast<TypeParam> (9.0))); // 3 + 3*2
    EXPECT_FALSE (buffer.hasBeenCleared());
}
