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

//==============================================================================

#if YUP_RIVE_USE_METAL && YUP_APPLE
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplMetal (GpuDevice&);
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplMetal (GpuDevice&, rive::ore::Context& frameContext);
#endif
#if YUP_RIVE_USE_D3D && YUP_WINDOWS
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplD3D11 (GpuDevice&);
#endif
#if YUP_EMSCRIPTEN && RIVE_WEBGPU
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplWebGPU (GpuDevice&);
#endif
#if YUP_RHI_USE_GL_COMPUTE
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplGL (GpuDevice&);
#endif
#if YUP_RIVE_USE_VULKAN
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplVulkan (GpuDevice&);
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplVulkan (GpuDevice&, void* commandBuffer, uint64_t generation);
#endif

//==============================================================================

struct GpuComputePass::Impl
{
    GpuComputePipeline::Ptr pipelineRef;
    bool finished = false;

    /** The frame this pass records into, until either of them finishes it. */
    GpuFrame::Impl* frame = nullptr;

    struct StorageBinding
    {
        int group;
        int binding;
        GpuBuffer::Ptr buffer;
    };

    struct UboBinding
    {
        int group;
        int binding;
        std::vector<uint8_t> data;
    };

    struct TexBinding
    {
        int group;
        int binding;
        GpuTexture::Ptr texture;
    };

    std::vector<StorageBinding> storageBindings;
    std::vector<UboBinding> uboBindings;
    std::vector<TexBinding> texBindings;

    virtual ~Impl() = default;

    virtual bool isValid() const = 0;
    virtual bool dispatch (uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) = 0;
    virtual void finish() = 0;
};

void yup_finishComputePass (GpuComputePass::Impl& pass)
{
    if (pass.finished)
        return;

    pass.finished = true;
    pass.finish();

    if (pass.frame != nullptr && pass.frame->openComputePass == &pass)
        pass.frame->openComputePass = nullptr;

    pass.frame = nullptr;
}

//==============================================================================

GpuComputePass GpuComputePass::begin (GpuDevice::Ptr ctx)
{
    GpuComputePass pass;
    if (ctx == nullptr || ! ctx->isComputeAvailable())
        return pass;

    switch (ctx->getPlatform())
    {
#if YUP_RIVE_USE_METAL && YUP_APPLE
        case GpuPlatform::Metal:
            pass.impl = yup_createComputePassImplMetal (*ctx);
            break;
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
        case GpuPlatform::Direct3D:
            pass.impl = yup_createComputePassImplD3D11 (*ctx);
            break;
#endif

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        case GpuPlatform::WebGPU:
            pass.impl = yup_createComputePassImplWebGPU (*ctx);
            break;
#endif

#if YUP_RHI_USE_GL_COMPUTE
        case GpuPlatform::OpenGL:
        case GpuPlatform::OpenGLES:
            pass.impl = yup_createComputePassImplGL (*ctx);
            break;
#endif

#if YUP_RIVE_USE_VULKAN
        case GpuPlatform::Vulkan:
            pass.impl = yup_createComputePassImplVulkan (*ctx);
            break;
#endif

        default:
            break;
    }

    return pass;
}

GpuComputePass GpuComputePass::begin (GpuFrame& frame)
{
    auto* frameImpl = frame.getImpl();
    if (frameImpl == nullptr || frameImpl->device == nullptr || frameImpl->submitted || frameImpl->released)
        return {};

    auto& device = *frameImpl->device;
    const auto platform = device.getPlatform();

    // Direct3D 11 needs nothing to join: Rive and compute share the immediate context
    const bool joinsFrame = device.isComputeAvailable()
                         && (platform == GpuPlatform::Metal
                             || platform == GpuPlatform::Direct3D
                             || (platform == GpuPlatform::Vulkan && frameImpl->commandBuffer != nullptr));

    if (! joinsFrame)
        return begin (frameImpl->device);

    // Dispatches cannot run inside a render pass, and Metal allows one open encoder at a time
    if (frameImpl->openPass != nullptr)
    {
        frameImpl->openPass->finish();
        frameImpl->openPass = nullptr;
    }

    if (frameImpl->openComputePass != nullptr)
        yup_finishComputePass (*frameImpl->openComputePass);

    GpuComputePass pass;

    switch (platform)
    {
#if YUP_RIVE_USE_METAL && YUP_APPLE
        case GpuPlatform::Metal:
            pass.impl = yup_createComputePassImplMetal (device, *frameImpl->oreCtx);
            break;
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
        case GpuPlatform::Direct3D:
            pass.impl = yup_createComputePassImplD3D11 (device);
            break;
#endif

#if YUP_RIVE_USE_VULKAN
        case GpuPlatform::Vulkan:
            pass.impl = yup_createComputePassImplVulkan (device, frameImpl->commandBuffer, frameImpl->generation);
            break;
#endif

        default:
            break;
    }

    if (pass.impl != nullptr)
    {
        pass.impl->frame = frameImpl;
        frameImpl->openComputePass = pass.impl.get();
    }

    return pass;
}

//==============================================================================

GpuComputePass::GpuComputePass (GpuComputePass&&) noexcept = default;

GpuComputePass& GpuComputePass::operator= (GpuComputePass&& other) noexcept
{
    if (this != &other)
    {
        finish();
        impl = std::move (other.impl);
    }
    return *this;
}

GpuComputePass::~GpuComputePass()
{
    finish();
}

//==============================================================================

bool GpuComputePass::isValid() const noexcept
{
    return impl != nullptr && ! impl->finished && impl->isValid();
}

void GpuComputePass::setPipeline (GpuComputePipeline::Ptr pipeline)
{
    if (impl)
        impl->pipelineRef = std::move (pipeline);
}

void GpuComputePass::setStorageBuffer (int group, int binding, GpuBuffer::Ptr buffer)
{
    if (! impl)
        return;

    for (auto& sb : impl->storageBindings)
    {
        if (sb.group == group && sb.binding == binding)
        {
            sb.buffer = std::move (buffer);
            return;
        }
    }

    impl->storageBindings.push_back ({ group, binding, std::move (buffer) });
}

void GpuComputePass::setUniformBuffer (int group, int binding, const void* data, size_t byteSize)
{
    if (! impl)
        return;

    jassert (data != nullptr && byteSize > 0);
    if (data == nullptr || byteSize == 0)
        return;

    for (auto& ub : impl->uboBindings)
    {
        if (ub.group == group && ub.binding == binding)
        {
            ub.data.assign (static_cast<const uint8_t*> (data),
                            static_cast<const uint8_t*> (data) + byteSize);
            return;
        }
    }

    Impl::UboBinding ub;
    ub.group = group;
    ub.binding = binding;
    ub.data.assign (static_cast<const uint8_t*> (data),
                    static_cast<const uint8_t*> (data) + byteSize);
    impl->uboBindings.push_back (std::move (ub));
}

void GpuComputePass::setTexture (int group, int binding, GpuTexture::Ptr texture)
{
    if (! impl)
        return;

    for (auto& tb : impl->texBindings)
    {
        if (tb.group == group && tb.binding == binding)
        {
            tb.texture = std::move (texture);
            return;
        }
    }

    impl->texBindings.push_back ({ group, binding, std::move (texture) });
}

//==============================================================================

bool GpuComputePass::dispatch (uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
    if (! isValid())
        return false;

    return impl->dispatch (groupsX, groupsY, groupsZ);
}

//==============================================================================

bool GpuComputePass::finish()
{
    if (impl == nullptr || impl->finished)
        return false;

    yup_finishComputePass (*impl);
    impl.reset();
    return true;
}

} // namespace yup
