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

#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplMetal (GpuDevice&);
#endif
#if YUP_RIVE_USE_D3D && YUP_WINDOWS
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplD3D11 (GpuDevice&);
#endif
#if (YUP_EMSCRIPTEN && RIVE_WEBGPU) || YUP_RIVE_USE_DAWN
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplWebGPU (GpuDevice&);
#endif
#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID
std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplGL (GpuDevice&);
#endif

//==============================================================================

struct GpuComputePass::Impl
{
    GpuComputePipeline::Ptr pipelineRef;
    bool finished = false;

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

//==============================================================================

GpuComputePass GpuComputePass::begin (GpuDevice::Ptr ctx)
{
    GpuComputePass pass;
    if (ctx == nullptr || ! ctx->isComputeAvailable())
        return pass;

    switch (ctx->getPlatform())
    {
#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
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
#elif YUP_RIVE_USE_DAWN
        case GpuPlatform::WebGPU:
            pass.impl = yup_createComputePassImplWebGPU (*ctx);
            break;
#endif

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID
        case GpuPlatform::OpenGL:
        case GpuPlatform::OpenGLES:
            pass.impl = yup_createComputePassImplGL (*ctx);
            break;
#endif

        default:
            break;
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

    impl->finished = true;
    impl->finish();
    impl.reset();
    return true;
}

} // namespace yup
