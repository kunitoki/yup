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

namespace yup
{

class GpuDevice;

//==============================================================================
/** A reference-counted GPU buffer handle.

    Wraps a backend-native ore buffer holding vertex, index, or uniform data.
    Create one via GpuBuffer::create() and bind it to a GpuRenderPass for indexed
    or non-indexed geometry rendering. The underlying GPU resource lives for as
    long as at least one GpuBuffer::Ptr exists.

    Buffers are created with the data provided at creation time and may be
    rewritten in place afterwards via GpuDevice::updateBuffer().

    @see GpuRenderPass, GraphicsContext::Options
*/
class YUP_API GpuBuffer : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuBuffer>;

    //==============================================================================
    /** Creates a GPU buffer and uploads the given data.

        @param ctx        A GpuDevice where GPU context is available.
        @param type       The intended usage of the buffer.
        @param data       Pointer to the source data to upload (must be non-null).
        @param byteSize   Number of bytes to upload (must be greater than zero).

        @returns A valid GpuBuffer, or nullptr on failure (ore unavailable or allocation failed).

        @warning Requires ctx.isGpuAvailable() (GPU context available on this backend).
    */
    static GpuBuffer::Ptr create (GpuDevice::Ptr ctx,
                                  GpuBufferType type,
                                  const void* data,
                                  size_t byteSize);

    //==============================================================================
    /** Destructor. */
    ~GpuBuffer();

    //==============================================================================
    /** Returns the buffer usage type. */
    GpuBufferType getType() const noexcept;

    /** Returns the size of the buffer in bytes. */
    size_t getSizeInBytes() const noexcept;

    /** Returns true if this buffer holds a valid GPU resource. */
    bool isValid() const noexcept;

    //==============================================================================
    /** @internal Test hook, not for production use: a storage buffer backed by an ore buffer of the
        ore context of @p ctx, which render passes bind in place of a native one. It lets passes
        reading storage buffers record on a mocked ore context, which has no native buffers.

        @returns nullptr without an ore context, data or size.
    */
    static GpuBuffer::Ptr createStorageForTesting (GpuDevice::Ptr ctx, const void* data, size_t byteSize);

private:
    //==============================================================================
    friend class GpuDevice;
    friend class GpuDeviceMetal;
    friend class GpuDeviceD3D;
    friend class GpuDeviceGL;
    friend class GpuDeviceWebGPU;
    friend class GpuDeviceVulkan;
    friend class GpuRenderPass;
    friend class GpuComputePass;
    friend class GpuComputePassImplMetal;
    friend class GpuComputePassImplD3D11;
    friend class GpuComputePassImplGL;
    friend class GpuComputePassImplWebGPU;
    friend class GpuComputePassImplVulkan;

    GpuBuffer() = default;

    struct Impl;

    Impl* getImpl() noexcept;
    const Impl* getImpl() const noexcept;

    static Ptr createWithImpl (Impl&& impl);

    static constexpr size_t ImplSizeBytes = 128;
    TypeErasedObject<ImplSizeBytes> impl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuBuffer)
};

} // namespace yup
