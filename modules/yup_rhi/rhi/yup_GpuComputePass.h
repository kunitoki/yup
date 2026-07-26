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

class GpuComputePipeline;
class GpuBuffer;
class GpuTexture;
class GpuDevice;

//==============================================================================
/** A transient encoder that dispatches compute work to the GPU.

    Obtain a GpuComputePass from GpuComputePass::begin(). Bind a compute
    pipeline and resources (storage buffers, uniform buffers, textures), then
    call dispatch() to run the compute shader and finish() to submit the work.

    The pass is move-only and follows RAII: the destructor calls finish() if
    you haven't already, so the GPU work is never silently dropped.

    @code
        auto pass = GpuComputePass::begin (device);
        pass.setPipeline (pipeline);
        pass.setStorageBuffer (0, 0, inputBuf);
        pass.setStorageBuffer (0, 1, outputBuf);
        pass.setUniformBuffer (0, 2, &params, sizeof (params));
        pass.dispatch (workgroupCount, 1, 1);
        pass.finish();
    @endcode

    @see GpuComputePipeline, GpuBuffer, GpuTexture, GpuDevice
*/
class YUP_API GpuComputePass
{
public:
    //==============================================================================
    /** Begins a compute pass on the given device.

        @param ctx  A GpuDevice with compute shader support.
        @returns    A GpuComputePass ready for binding and dispatch, or an
                    invalid pass if compute is unavailable or the device is null.
    */
    static GpuComputePass begin (GpuDevice::Ptr ctx);

    //==============================================================================
    /** Move constructor. */
    GpuComputePass (GpuComputePass&&) noexcept;

    /** Move assignment operator. */
    GpuComputePass& operator= (GpuComputePass&&) noexcept;

    /** Destructor. Finishes the pass if not already finished. */
    ~GpuComputePass();

    //==============================================================================
    /** Returns true if the pass is valid and has not been finished. */
    bool isValid() const noexcept;

    //==============================================================================
    /** Sets the compute pipeline used by subsequent dispatch() calls.

        @param pipeline  The compiled GpuComputePipeline to use.
    */
    void setPipeline (GpuComputePipeline::Ptr pipeline);

    /** Binds a read-write storage buffer to the given slot.

        The buffer must have been created with GpuBufferType::storage. The
        (group, binding) indices must match the shader's layout declarations.

        @param group    Binding group index declared in the shader (set).
        @param binding  Binding index within the group.
        @param buffer   The storage buffer to bind.
    */
    void setStorageBuffer (int group, int binding, GpuBuffer::Ptr buffer);

    /** Uploads uniform data to the given slot.

        The data is copied immediately and does not need to outlive this call.
        The (group, binding) indices must match the shader's layout declarations.

        @param group     Binding group index declared in the shader (set).
        @param binding   Binding index within the group.
        @param data      Pointer to the uniform data.
        @param byteSize  Size of the data in bytes.
    */
    void setUniformBuffer (int group, int binding, const void* data, size_t byteSize);

    /** Binds a read-only texture to the given slot.

        @param group    Binding group index declared in the shader (set).
        @param binding  Binding index within the group.
        @param texture  The texture to bind for sampling.
    */
    void setTexture (int group, int binding, GpuTexture::Ptr texture);

    //==============================================================================
    /** Dispatches compute workgroups.

        The total number of GPU threads launched is
        @code groupsX * groupsY * groupsZ * workgroupSize @endcode
        where `workgroupSize` is the pipeline's local workgroup size.

        @param groupsX  Number of workgroups in X.
        @param groupsY  Number of workgroups in Y.
        @param groupsZ  Number of workgroups in Z.

        @returns true on success, false if the pass is invalid.
    */
    bool dispatch (uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);

    //==============================================================================
    /** Submits all recorded dispatches to the GPU.

        Idempotent — calling finish() more than once is a no-op returning false.

        @returns true if work was submitted, false if already finished or invalid.
    */
    bool finish();

    //==============================================================================
    struct Impl;

private:
    GpuComputePass() = default;

    std::unique_ptr<Impl> impl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuComputePass)
};

} // namespace yup
