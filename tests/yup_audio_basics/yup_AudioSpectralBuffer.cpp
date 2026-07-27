/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include <yup_audio_basics/yup_audio_basics.h>

#include <cmath>
#include <algorithm>
#include <vector>
#include <cstring>

using namespace yup;

template <class T>
class AudioSpectralBufferTests : public ::testing::Test
{
public:
    using BufferType = SpectralBuffer<T>;

protected:
    void fillChannel (BufferType& buffer, int channel, T startReal, T startImag)
    {
        auto* ptr = buffer.getWritePointer (channel);
        const auto numBins = buffer.getNumBins();

        for (int i = 0; i < numBins; ++i)
        {
            ptr[i * 2] = startReal + static_cast<T> (i);
            ptr[i * 2 + 1] = startImag + static_cast<T> (i);
        }
    }

    void fillRamp (BufferType& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            fillChannel (buffer, ch, static_cast<T> (ch * 100), static_cast<T> (ch * 100 + 50));
    }

    bool pointersAreEqual (const T* a, const T* b, int numFloats)
    {
        for (int i = 0; i < numFloats; ++i)
        {
            if (! approximatelyEqual (a[i], b[i]))
                return false;
        }

        return true;
    }

    T magnitude (T real, T imag) const
    {
        return std::sqrt (real * real + imag * imag);
    }
};

using FloatTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE (AudioSpectralBufferTests, FloatTypes);

//==============================================================================
// Default constructor
TYPED_TEST (AudioSpectralBufferTests, DefaultConstructor)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    EXPECT_EQ (buffer.getNumChannels(), 0);
    EXPECT_EQ (buffer.getNumBins(), 0);
    EXPECT_TRUE (buffer.hasBeenCleared());
}

//==============================================================================
// Constructor with channels and bins
TYPED_TEST (AudioSpectralBufferTests, ConstructorWithChannelsAndBins)
{
    using BufferType = typename TestFixture::BufferType;

    constexpr int channels = 2;
    constexpr int bins = 128;
    BufferType buffer (channels, bins);
    EXPECT_EQ (buffer.getNumChannels(), channels);
    EXPECT_EQ (buffer.getNumBins(), bins);
    EXPECT_FALSE (buffer.hasBeenCleared());

    for (int ch = 0; ch < channels; ++ch)
    {
        const TypeParam* readPtr = buffer.getReadPointer (ch);
        EXPECT_NE (readPtr, nullptr);
    }
}

//==============================================================================
// clear and hasBeenCleared
TYPED_TEST (AudioSpectralBufferTests, ClearSetsAllBinsToZero)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 64);
    this->fillRamp (buffer);
    EXPECT_FALSE (buffer.hasBeenCleared());

    buffer.clear();
    EXPECT_TRUE (buffer.hasBeenCleared());

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* ptr = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumBins() * 2; ++i)
            EXPECT_EQ (ptr[i], TypeParam (0));
    }
}

TYPED_TEST (AudioSpectralBufferTests, ClearIsNoopWhenAlreadyCleared)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 64);
    buffer.clear();
    EXPECT_TRUE (buffer.hasBeenCleared());
    buffer.clear(); // should be a no-op
    EXPECT_TRUE (buffer.hasBeenCleared());
}

TYPED_TEST (AudioSpectralBufferTests, SetNotClearResetsFlag)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 64);
    buffer.clear();
    EXPECT_TRUE (buffer.hasBeenCleared());

    buffer.setNotClear();
    EXPECT_FALSE (buffer.hasBeenCleared());
}

//==============================================================================
// getWritePointer / getReadPointer
TYPED_TEST (AudioSpectralBufferTests, WritePointerMarksBufferAsNotClear)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 64);
    buffer.clear();
    EXPECT_TRUE (buffer.hasBeenCleared());

    [[maybe_unused]] auto* ptr = buffer.getWritePointer (0);
    EXPECT_FALSE (buffer.hasBeenCleared());
}

TYPED_TEST (AudioSpectralBufferTests, WriteAndReadThroughPointers)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 32);

    auto* wPtr = buffer.getWritePointer (0);
    for (int i = 0; i < buffer.getNumBins() * 2; ++i)
        wPtr[i] = static_cast<TypeParam> (i + 1);

    const auto* rPtr = buffer.getReadPointer (0);
    for (int i = 0; i < buffer.getNumBins() * 2; ++i)
        EXPECT_EQ (rPtr[i], static_cast<TypeParam> (i + 1));
}

TYPED_TEST (AudioSpectralBufferTests, WritePointerReturnInterleavedLayout)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (1, 4);

    auto* ptr = buffer.getWritePointer (0);
    ptr[0] = TypeParam (10); // bin 0 real
    ptr[1] = TypeParam (20); // bin 0 imag
    ptr[2] = TypeParam (30); // bin 1 real
    ptr[3] = TypeParam (40); // bin 1 imag

    EXPECT_EQ (ptr[0], TypeParam (10));
    EXPECT_EQ (ptr[1], TypeParam (20));
    EXPECT_EQ (ptr[2], TypeParam (30));
    EXPECT_EQ (ptr[3], TypeParam (40));
}

//==============================================================================
// BinRef access
TYPED_TEST (AudioSpectralBufferTests, BinRefReadWrite)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (1, 4);

    auto bin0 = buffer.getBinRef (0, 0);
    bin0.real() = TypeParam (3);
    bin0.imag() = TypeParam (4);

    EXPECT_EQ (bin0.real(), TypeParam (3));
    EXPECT_EQ (bin0.imag(), TypeParam (4));
}

TYPED_TEST (AudioSpectralBufferTests, BinRefSet)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (1, 4);
    auto bin2 = buffer.getBinRef (0, 2);
    bin2.set (TypeParam (7), TypeParam (8));

    EXPECT_EQ (bin2.real(), TypeParam (7));
    EXPECT_EQ (bin2.imag(), TypeParam (8));
}

TYPED_TEST (AudioSpectralBufferTests, BinRefMagnitude)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (1, 4);
    auto bin = buffer.getBinRef (0, 0);
    bin.set (TypeParam (3), TypeParam (4));

    const auto expectedMag = this->magnitude (TypeParam (3), TypeParam (4));
    EXPECT_NEAR (bin.magnitude(), expectedMag, static_cast<TypeParam> (1e-6));
}

TYPED_TEST (AudioSpectralBufferTests, BinRefPhase)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (1, 4);
    auto bin = buffer.getBinRef (0, 0);
    bin.set (TypeParam (1), TypeParam (1));

    const auto expectedPhase = std::atan2 (TypeParam (1), TypeParam (1));
    EXPECT_NEAR (bin.phase(), expectedPhase, static_cast<TypeParam> (1e-6));
}

TYPED_TEST (AudioSpectralBufferTests, BinRefPointsToCorrectData)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 8);
    auto* raw = buffer.getWritePointer (1);

    raw[6] = TypeParam (42); // bin 3 real (channel1, bin3)
    raw[7] = TypeParam (99); // bin 3 imag

    auto bin = buffer.getBinRef (1, 3);
    EXPECT_EQ (bin.real(), TypeParam (42));
    EXPECT_EQ (bin.imag(), TypeParam (99));
}

//==============================================================================
// copyFrom (simple)
TYPED_TEST (AudioSpectralBufferTests, CopyFromSimple)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType source (2, 32);
    this->fillChannel (source, 0, TypeParam (10), TypeParam (20));
    this->fillChannel (source, 1, TypeParam (30), TypeParam (40));

    BufferType dest (2, 32);
    dest.clear();

    dest.copyFrom (0, source, 0);
    dest.copyFrom (1, source, 1);

    const auto* srcPtr0 = source.getReadPointer (0);
    const auto* dstPtr0 = dest.getReadPointer (0);
    const auto* srcPtr1 = source.getReadPointer (1);
    const auto* dstPtr1 = dest.getReadPointer (1);

    EXPECT_TRUE (this->pointersAreEqual (srcPtr0, dstPtr0, 64));
    EXPECT_TRUE (this->pointersAreEqual (srcPtr1, dstPtr1, 64));
}

TYPED_TEST (AudioSpectralBufferTests, CopyFromSimpleDifferentChannels)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType source (2, 16);
    this->fillChannel (source, 0, TypeParam (1), TypeParam (2));
    this->fillChannel (source, 1, TypeParam (3), TypeParam (4));

    BufferType dest (2, 16);
    dest.copyFrom (1, source, 0);

    const auto* srcPtr = source.getReadPointer (0);
    const auto* dstPtr = dest.getReadPointer (1);

    EXPECT_TRUE (this->pointersAreEqual (srcPtr, dstPtr, 32));
}

//==============================================================================
// copyFrom (ranged)
TYPED_TEST (AudioSpectralBufferTests, CopyFromRanged)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType source (1, 16);
    this->fillChannel (source, 0, TypeParam (10), TypeParam (20));

    BufferType dest (1, 16);
    dest.clear();

    constexpr int startBin = 4;
    constexpr int numBins = 4;
    dest.copyFrom (0, startBin, source, 0, 0, numBins);

    const auto* srcPtr = source.getReadPointer (0);
    const auto* dstPtr = dest.getReadPointer (0);

    // first 4 bins should be zero
    for (int i = 0; i < startBin * 2; ++i)
        EXPECT_EQ (dstPtr[i], TypeParam (0));

    // copied range
    for (int i = 0; i < numBins * 2; ++i)
        EXPECT_EQ (dstPtr[startBin * 2 + i], srcPtr[i]);

    // remainder should be zero
    for (int i = (startBin + numBins) * 2; i < 16 * 2; ++i)
        EXPECT_EQ (dstPtr[i], TypeParam (0));
}

TYPED_TEST (AudioSpectralBufferTests, CopyFromRangedWithNonZeroSourceStart)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType source (1, 16);
    this->fillChannel (source, 0, TypeParam (10), TypeParam (20));

    BufferType dest (1, 16);
    dest.clear();

    constexpr int srcStartBin = 8;
    constexpr int numBins = 4;
    dest.copyFrom (0, 0, source, 0, srcStartBin, numBins);

    const auto* srcPtr = source.getReadPointer (0);
    const auto* dstPtr = dest.getReadPointer (0);

    for (int i = 0; i < numBins; ++i)
    {
        const auto srcIdx = (srcStartBin + i) * 2;
        const auto dstIdx = i * 2;

        EXPECT_EQ (dstPtr[dstIdx], srcPtr[srcIdx]);
        EXPECT_EQ (dstPtr[dstIdx + 1], srcPtr[srcIdx + 1]);
    }
}

//==============================================================================
// fill
TYPED_TEST (AudioSpectralBufferTests, FillSingleChannel)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 8);
    buffer.fill (0, TypeParam (5), TypeParam (6));

    const auto* ptr = buffer.getReadPointer (0);
    for (int i = 0; i < 8; ++i)
    {
        EXPECT_EQ (ptr[i * 2], TypeParam (5));
        EXPECT_EQ (ptr[i * 2 + 1], TypeParam (6));
    }
}

TYPED_TEST (AudioSpectralBufferTests, FillAllChannels)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (3, 4);
    buffer.fill (TypeParam (7), TypeParam (8));

    for (int ch = 0; ch < 3; ++ch)
    {
        const auto* ptr = buffer.getReadPointer (ch);
        for (int i = 0; i < 4; ++i)
        {
            EXPECT_EQ (ptr[i * 2], TypeParam (7));
            EXPECT_EQ (ptr[i * 2 + 1], TypeParam (8));
        }
    }
}

//==============================================================================
// setSize
TYPED_TEST (AudioSpectralBufferTests, SetSizeGrow)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (1, 8);
    this->fillChannel (buffer, 0, TypeParam (1), TypeParam (2));

    buffer.setSize (2, 16, false, true, false);

    EXPECT_EQ (buffer.getNumChannels(), 2);
    EXPECT_EQ (buffer.getNumBins(), 16);
}

TYPED_TEST (AudioSpectralBufferTests, SetSizeKeepExistingContent)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (1, 8);
    this->fillChannel (buffer, 0, TypeParam (10), TypeParam (20));

    buffer.setSize (1, 16, true, true, false);

    EXPECT_EQ (buffer.getNumChannels(), 1);
    EXPECT_EQ (buffer.getNumBins(), 16);

    const auto* ptr = buffer.getReadPointer (0);
    for (int i = 0; i < 8; ++i)
    {
        EXPECT_EQ (ptr[i * 2], TypeParam (10) + static_cast<TypeParam> (i));
        EXPECT_EQ (ptr[i * 2 + 1], TypeParam (20) + static_cast<TypeParam> (i));
    }
}

TYPED_TEST (AudioSpectralBufferTests, SetSizeShrink)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (3, 64);
    buffer.setSize (1, 32, false, false, false);

    EXPECT_EQ (buffer.getNumChannels(), 1);
    EXPECT_EQ (buffer.getNumBins(), 32);
}

TYPED_TEST (AudioSpectralBufferTests, SetSizeNoChangeDoesNothing)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 32);
    this->fillRamp (buffer);

    buffer.setSize (2, 32); // same size

    EXPECT_EQ (buffer.getNumChannels(), 2);
    EXPECT_EQ (buffer.getNumBins(), 32);
}

//==============================================================================
// operator== / operator!=
TYPED_TEST (AudioSpectralBufferTests, EqualityOperator)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType a (1, 4);
    BufferType b (1, 4);

    this->fillChannel (a, 0, TypeParam (1), TypeParam (2));
    this->fillChannel (b, 0, TypeParam (1), TypeParam (2));

    EXPECT_TRUE (a == b);
    EXPECT_FALSE (a != b);
}

TYPED_TEST (AudioSpectralBufferTests, InequalityOperator)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType a (1, 4);
    BufferType b (1, 4);

    this->fillChannel (a, 0, TypeParam (1), TypeParam (2));
    this->fillChannel (b, 0, TypeParam (3), TypeParam (4));

    EXPECT_FALSE (a == b);
    EXPECT_TRUE (a != b);
}

TYPED_TEST (AudioSpectralBufferTests, EqualityDifferentChannels)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType a (2, 4);
    BufferType b (1, 4);

    EXPECT_FALSE (a == b);
    EXPECT_TRUE (a != b);
}

TYPED_TEST (AudioSpectralBufferTests, EqualityDifferentBins)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType a (1, 4);
    BufferType b (1, 8);

    EXPECT_FALSE (a == b);
    EXPECT_TRUE (a != b);
}

//==============================================================================
// makeCopyOf
TYPED_TEST (AudioSpectralBufferTests, MakeCopyOfSameType)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType source (2, 16);
    this->fillRamp (source);

    BufferType dest;
    dest.makeCopyOf (source);

    EXPECT_EQ (dest.getNumChannels(), source.getNumChannels());
    EXPECT_EQ (dest.getNumBins(), source.getNumBins());

    for (int ch = 0; ch < source.getNumChannels(); ++ch)
    {
        EXPECT_TRUE (this->pointersAreEqual (
            source.getReadPointer (ch),
            dest.getReadPointer (ch),
            source.getNumBins() * 2));
    }
}

TYPED_TEST (AudioSpectralBufferTests, MakeCopyOfClearedBuffer)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType source (1, 8);
    source.clear();

    BufferType dest;
    dest.makeCopyOf (source);

    EXPECT_TRUE (dest.hasBeenCleared());
    EXPECT_EQ (dest.getNumBins(), 8);
}

//==============================================================================
// getNumChannels / getNumBins on default constructed
TYPED_TEST (AudioSpectralBufferTests, DefaultBufferHasZeroChannelsAndBins)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer;
    EXPECT_EQ (buffer.getNumChannels(), 0);
    EXPECT_EQ (buffer.getNumBins(), 0);
}

//==============================================================================
// interleaved layout integrity
TYPED_TEST (AudioSpectralBufferTests, InterleavedLayoutPreserved)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 4);
    this->fillChannel (buffer, 0, TypeParam (1), TypeParam (2));
    this->fillChannel (buffer, 1, TypeParam (3), TypeParam (4));

    auto bin0_ch1 = buffer.getBinRef (1, 0);
    auto bin3_ch0 = buffer.getBinRef (0, 3);

    EXPECT_EQ (bin0_ch1.real(), TypeParam (3));
    EXPECT_EQ (bin0_ch1.imag(), TypeParam (4));
    EXPECT_EQ (bin3_ch0.real(), TypeParam (1) + TypeParam (3));
    EXPECT_EQ (bin3_ch0.imag(), TypeParam (2) + TypeParam (3));
}

//==============================================================================
// copyFrom self-copy within same buffer
TYPED_TEST (AudioSpectralBufferTests, CopyFromSameBufferNonOverlapping)
{
    using BufferType = typename TestFixture::BufferType;

    BufferType buffer (2, 16);
    this->fillChannel (buffer, 0, TypeParam (10), TypeParam (20));

    buffer.copyFrom (1, 0, buffer, 0, 0, 8);

    const auto* ch0 = buffer.getReadPointer (0);
    const auto* ch1 = buffer.getReadPointer (1);

    for (int i = 0; i < 16; ++i)
        EXPECT_EQ (ch1[i], ch0[i]);
}
