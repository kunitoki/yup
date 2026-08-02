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

#include <yup_audio_processors/yup_audio_processors.h>

using namespace yup;

//==============================================================================

TEST (AudioBusBufferViewTests, DefaultConstructedIsEmpty)
{
    AudioBusBufferView<float> view;
    EXPECT_EQ (0, view.getNumChannels());
    EXPECT_EQ (nullptr, view.getChannels());
    EXPECT_EQ (AudioBus::Role::Main, view.getRole());
}

TEST (AudioBusBufferViewTests, ConstructedWithChannels)
{
    float ch0[16], ch1[16];
    const float* ptrs[] = { ch0, ch1 };

    AudioBusBufferView<const float> view (ptrs, 2, AudioBus::Role::Auxiliary);
    EXPECT_EQ (2, view.getNumChannels());
    EXPECT_EQ (AudioBus::Role::Auxiliary, view.getRole());
    EXPECT_EQ (ch0, view.getReadPointer (0));
    EXPECT_EQ (ch1, view.getReadPointer (1));
}

TEST (AudioBusBufferViewTests, GetReadPointerOutOfRange)
{
    float ch0[16];
    const float* ptrs[] = { ch0 };

    AudioBusBufferView<const float> view (ptrs, 1);
    EXPECT_EQ (ch0, view.getReadPointer (0));
    EXPECT_EQ (nullptr, view.getReadPointer (1));
    EXPECT_EQ (nullptr, view.getReadPointer (-1));
}

TEST (AudioBusBufferViewTests, GetWritePointerOutOfRange)
{
    float ch0[16];
    float* ptrs[] = { ch0 };

    AudioBusBufferView<float> view (ptrs, 1);
    EXPECT_EQ (ch0, view.getWritePointer (0));
    EXPECT_EQ (nullptr, view.getWritePointer (1));
    EXPECT_EQ (nullptr, view.getWritePointer (-1));
}

TEST (AudioBusBufferViewTests, ConstViewConstructedWithMutablePointers)
{
    float ch0[16], ch1[16];
    float* ptrs[] = { ch0, ch1 };

    AudioBusBufferView<const float> view (ptrs, 2);
    EXPECT_EQ (2, view.getNumChannels());
    EXPECT_EQ (ch0, view.getReadPointer (0));
}

TEST (AudioBusBufferViewTests, MutableViewCanWrite)
{
    float ch0[16] = {};
    float* ptrs[] = { ch0 };

    AudioBusBufferView<float> view (ptrs, 1);
    auto* writePtr = view.getWritePointer (0);
    ASSERT_NE (nullptr, writePtr);
    writePtr[0] = 3.14f;
    EXPECT_FLOAT_EQ (3.14f, ch0[0]);
}

TEST (AudioBusBufferViewTests, NullChannelPointers)
{
    AudioBusBufferView<const float> view (nullptr, 2, AudioBus::Role::Auxiliary);
    EXPECT_EQ (2, view.getNumChannels());
    EXPECT_EQ (nullptr, view.getReadPointer (0));
    EXPECT_EQ (nullptr, view.getReadPointer (1));
    EXPECT_EQ (AudioBus::Role::Auxiliary, view.getRole());
}

TEST (AudioBusBufferViewTests, NullChannelPointersMutableView)
{
    AudioBusBufferView<float> view (nullptr, 2, AudioBus::Role::Auxiliary);
    EXPECT_EQ (2, view.getNumChannels());
    EXPECT_EQ (nullptr, view.getWritePointer (0));
    EXPECT_EQ (nullptr, view.getWritePointer (1));
    EXPECT_EQ (nullptr, view.getReadPointer (0));
    EXPECT_EQ (AudioBus::Role::Auxiliary, view.getRole());
}

TEST (AudioBusBufferViewTests, StereoBus)
{
    float left[8], right[8];
    const float* ptrs[] = { left, right };

    AudioBusBufferView<const float> view (ptrs, 2);
    EXPECT_EQ (2, view.getNumChannels());
    EXPECT_EQ (left, view.getReadPointer (0));
    EXPECT_EQ (right, view.getReadPointer (1));
}

TEST (AudioBusBufferViewTests, MonoBus)
{
    float mono[8];
    const float* ptrs[] = { mono };

    AudioBusBufferView<const float> view (ptrs, 1);
    EXPECT_EQ (1, view.getNumChannels());
    EXPECT_EQ (mono, view.getReadPointer (0));
}

TEST (AudioBusBufferViewTests, DefaultRoleIsMain)
{
    float ch0[8];
    const float* ptrs[] = { ch0 };

    AudioBusBufferView<const float> view (ptrs, 1);
    EXPECT_EQ (AudioBus::Role::Main, view.getRole());
}
