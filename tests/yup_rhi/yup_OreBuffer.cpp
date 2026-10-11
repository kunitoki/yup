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

#include <yup_rhi/yup_rhi.h>

#include <gtest/gtest.h>

#include <rive/renderer/ore/ore_buffer.hpp>

using namespace yup;

//==============================================================================
class GpuOreBufferTests : public ::testing::Test
{
protected:
    /** Records the update() calls the base writeUnsynchronized() makes. */
    class UpdateRecordingBuffer : public rive::ore::Buffer
    {
    public:
        UpdateRecordingBuffer()
            : rive::ore::Buffer (512, rive::ore::BufferUsage::uniform)
        {
        }

        void update (const void* data, uint32_t size, uint32_t offset) override
        {
            updates.push_back ({ data, size, offset });
        }

        struct Update
        {
            const void* data;
            uint32_t size;
            uint32_t offset;
        };

        std::vector<Update> updates;
    };
};

TEST_F (GpuOreBufferTests, DefaultUnsynchronizedWriteUpdatesTheRange)
{
    auto buffer = rive::make_rcp<UpdateRecordingBuffer>();
    const uint8 bytes[16] = {};

    buffer->writeUnsynchronized (bytes, sizeof (bytes), 256, false);
    buffer->writeUnsynchronized (bytes, sizeof (bytes), 0, true);

    ASSERT_EQ (buffer->updates.size(), 2u);
    EXPECT_EQ (buffer->updates[0].data, bytes);
    EXPECT_EQ (buffer->updates[0].size, 16u);
    EXPECT_EQ (buffer->updates[0].offset, 256u);
    EXPECT_EQ (buffer->updates[1].offset, 0u);
}
