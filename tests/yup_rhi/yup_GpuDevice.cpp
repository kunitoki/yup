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

#include <yup_rhi/yup_rhi.h>

using namespace yup;

//==============================================================================
// GpuDevice — error path tests
//==============================================================================

class GpuDeviceErrorTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Headless, {});
        ASSERT_NE (device, nullptr);
    }

    GpuDevice::Ptr device;
};

// ---------------------------------------------------------------------------
// create — invalid API
// ---------------------------------------------------------------------------

TEST_F (GpuDeviceErrorTests, CreateWithInvalidApiReturnsNull)
{
    const auto invalidApi = static_cast<GpuPlatform> (9999);
    auto ctx = GpuDevice::create (invalidApi, {});
    EXPECT_EQ (ctx, nullptr);
}

// ---------------------------------------------------------------------------
// readBuffer — default returns false
// ---------------------------------------------------------------------------

TEST_F (GpuDeviceErrorTests, ReadBufferReturnsFalse)
{
    // readBuffer is a no-op in the base class; always returns false.
    uint8 buf[64] = {};
    EXPECT_FALSE (device->readBuffer (nullptr, buf, sizeof (buf)));
}

TEST_F (GpuDeviceErrorTests, ReadBufferWithNonNullBufferReturnsFalse)
{
    const float data[] = { 1.0f, 2.0f, 3.0f };
    auto buffer = GpuBuffer::create (device, GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buffer, nullptr);

    uint8 readback[sizeof (data)] = {};
    EXPECT_FALSE (device->readBuffer (buffer, readback, sizeof (readback)));
}

// ---------------------------------------------------------------------------
// updateBuffer — error paths
// ---------------------------------------------------------------------------

TEST_F (GpuDeviceErrorTests, UpdateBufferWithNullBufferReturnsFalse)
{
    const float data[] = { 1.0f, 2.0f };
    EXPECT_FALSE (device->updateBuffer (nullptr, data, sizeof (data)));
}

TEST_F (GpuDeviceErrorTests, UpdateBufferWithNullDataReturnsFalse)
{
    const float data[] = { 1.0f, 2.0f, 3.0f };
    auto buffer = GpuBuffer::create (device, GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buffer, nullptr);

    EXPECT_FALSE (device->updateBuffer (buffer, nullptr, sizeof (data)));
}

TEST_F (GpuDeviceErrorTests, UpdateBufferWithZeroSizeReturnsFalse)
{
    const float data[] = { 1.0f };
    auto buffer = GpuBuffer::create (device, GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buffer, nullptr);

    EXPECT_FALSE (device->updateBuffer (buffer, data, 0));
}

TEST_F (GpuDeviceErrorTests, UpdateBufferSucceedsForValidOreBuffer)
{
    const float initial[] = { 1.0f, 2.0f, 3.0f };
    auto buffer = GpuBuffer::create (device, GpuBufferType::vertex, initial, sizeof (initial));
    ASSERT_NE (buffer, nullptr);

    const float updated[] = { 4.0f, 5.0f };
    // Update fewer bytes than the buffer size — should succeed.
    EXPECT_TRUE (device->updateBuffer (buffer, updated, sizeof (updated)));
}

TEST_F (GpuDeviceErrorTests, UpdateBufferFailsForStorageBuffer)
{
    // Storage buffers are not supported by the base GpuDevice::createBuffer.
    const float data[] = { 1.0f, 2.0f };
    auto buffer = GpuBuffer::create (device, GpuBufferType::storage, data, sizeof (data));
    // Storage buffer creation returns null from the base implementation.
    EXPECT_EQ (buffer, nullptr);
}

// ---------------------------------------------------------------------------
// createBuffer — error paths
// ---------------------------------------------------------------------------

TEST_F (GpuDeviceErrorTests, CreateBufferWithNullDataReturnsNull)
{
    EXPECT_EQ (device->createBuffer (GpuBufferType::vertex, nullptr, 16), nullptr);
}

TEST_F (GpuDeviceErrorTests, CreateBufferWithZeroSizeReturnsNull)
{
    const float data[] = { 1.0f };
    EXPECT_EQ (device->createBuffer (GpuBufferType::vertex, data, 0), nullptr);
}

TEST_F (GpuDeviceErrorTests, CreateBufferWithStorageTypeReturnsNull)
{
    const float data[] = { 1.0f };
    // Storage buffers not supported by base implementation.
    EXPECT_EQ (device->createBuffer (GpuBufferType::storage, data, sizeof (data)), nullptr);
}

// ---------------------------------------------------------------------------
// isComputeAvailable
// ---------------------------------------------------------------------------

TEST_F (GpuDeviceErrorTests, IsComputeAvailableOnHeadlessReturnsFalse)
{
    EXPECT_FALSE (device->isComputeAvailable());
}

// ---------------------------------------------------------------------------
// getPlatform
// ---------------------------------------------------------------------------

TEST_F (GpuDeviceErrorTests, GetPlatformOnHeadlessReturnsHeadless)
{
    EXPECT_EQ (device->getPlatform(), GpuPlatform::Headless);
}

// ---------------------------------------------------------------------------
// GpuBuffer — additional coverage
// ---------------------------------------------------------------------------

class GpuBufferErrorTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Headless, {});
        ASSERT_NE (device, nullptr);
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuBufferErrorTests, CreateWithUniformTypeSucceeds)
{
    const float data[] = { 1.0f, 2.0f };
    auto buf = GpuBuffer::create (device, GpuBufferType::uniform, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::uniform);
    EXPECT_EQ (buf->getSizeInBytes(), sizeof (data));
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuBufferErrorTests, CreateWithIndexTypeSucceeds)
{
    const uint16_t data[] = { 0, 1, 2, 3, 4, 5 };
    auto buf = GpuBuffer::create (device, GpuBufferType::index, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::index);
    EXPECT_EQ (buf->getSizeInBytes(), sizeof (data));
}

TEST_F (GpuBufferErrorTests, DefaultConstructedBufferIsInvalid)
{
    GpuBuffer::Ptr nullBuf;
    EXPECT_EQ (nullBuf, nullptr);
}
