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

#pragma once

#include <deque>
#include <set>
#include <vector>

/** The render passes on a mocked ore context: they compile the GLSL of their bundles and record on
    it without a GPU, on every platform, and the fixture keeps what each backend render pass was asked.
*/
class RendererMockTests : public ::testing::Test
{
protected:
    /** A mocked device reading fragment storage buffers, as the full tier passes do. */
    class StorageDevice : public OreInjectedGpuDevice
    {
    public:
        using OreInjectedGpuDevice::OreInjectedGpuDevice;

        bool isFragmentStorageBufferAvailable() const noexcept override { return true; }
    };

    /** What one backend render pass was asked. */
    struct RecordedPass
    {
        uint32_t colorCount = 0;
        bool hasDepth = false;
        std::vector<rive::ore::Pipeline*> pipelines; // Every setPipeline()
        int draws = 0;
        std::vector<float> viewportDepths; // The minimum depth of every setViewport()
        bool finished = false;

        size_t countDistinctPipelines() const { return std::set<rive::ore::Pipeline*> (pipelines.begin(), pipelines.end()).size(); }
    };

    class RecordingOreRenderPass : public ::testing::NiceMock<MockOreRenderPass>
    {
    public:
        explicit RecordingOreRenderPass (RecordedPass& recorded)
        {
            using ::testing::_;
            using ::testing::Invoke;

            ON_CALL (*this, finish()).WillByDefault (Invoke ([this, &recorded]
            {
                m_finished = true;
                recorded.finished = true;
            }));
            ON_CALL (*this, setPipeline (_)).WillByDefault (Invoke ([&recorded] (rive::ore::Pipeline* pipeline) { recorded.pipelines.push_back (pipeline); }));
            ON_CALL (*this, draw (_, _, _, _)).WillByDefault (Invoke ([&recorded] (uint32_t, uint32_t, uint32_t, uint32_t) { ++recorded.draws; }));
            ON_CALL (*this, drawIndexed (_, _, _, _, _)).WillByDefault (Invoke ([&recorded] (uint32_t, uint32_t, uint32_t, int32_t, uint32_t) { ++recorded.draws; }));
            ON_CALL (*this, setViewport (_, _, _, _, _, _)).WillByDefault (Invoke ([&recorded] (float, float, float, float, float minDepth, float)
            {
                recorded.viewportDepths.push_back (minDepth);
            }));
        }
    };

    void SetUp() override
    {
        using ::testing::_;
        using ::testing::Invoke;

        oreCtx = std::make_unique<::testing::NiceMock<MockOreContext>>();
        device = new StorageDevice (oreCtx.get());

        ON_CALL (*oreCtx, makeBuffer (_)).WillByDefault (Invoke ([] (const rive::ore::BufferDesc& desc) -> rive::rcp<rive::ore::Buffer>
        {
            return rive::make_rcp<::testing::NiceMock<MockOreBuffer>> (desc.size);
        }));
        ON_CALL (*oreCtx, makeTexture (_)).WillByDefault (Invoke ([this] (const rive::ore::TextureDesc& desc) -> rive::rcp<rive::ore::Texture>
        {
            if (failTextures)
                return nullptr;

            return rive::make_rcp<::testing::NiceMock<MockOreTexture>> (desc);
        }));
        ON_CALL (*oreCtx, makeTextureViewImpl (_)).WillByDefault (Invoke ([] (const rive::ore::TextureViewDesc&) -> rive::rcp<rive::ore::TextureView>
        {
            return rive::make_rcp<TestOreTextureView>();
        }));
        ON_CALL (*oreCtx, makeSampler (_)).WillByDefault (Invoke ([] (const rive::ore::SamplerDesc&) -> rive::rcp<rive::ore::Sampler>
        {
            return rive::make_rcp<TestOreSampler>();
        }));
        ON_CALL (*oreCtx, makeShaderModule (_)).WillByDefault (Invoke ([] (const rive::ore::ShaderModuleDesc& desc) -> rive::rcp<rive::ore::ShaderModule>
        {
            // As a backend does: GpuPipeline lays its bind groups out from the parsed binding map
            auto module = rive::make_rcp<TestOreShaderModule>();
            if (! rive::ore::BindingMap::fromBlob (desc.bindingMapBytes, desc.bindingMapSize, &module->m_bindingMap))
                return nullptr;

            return module;
        }));
        ON_CALL (*oreCtx, makeBindGroupLayout (_)).WillByDefault (Invoke ([] (const rive::ore::BindGroupLayoutDesc& desc) -> rive::rcp<rive::ore::BindGroupLayout>
        {
            return rive::make_rcp<TestOreBindGroupLayout> (desc);
        }));
        ON_CALL (*oreCtx, makePipeline (_, _)).WillByDefault (Invoke ([this] (const rive::ore::PipelineDesc&, std::string* error) -> rive::rcp<rive::ore::Pipeline>
        {
            if (failPipelines)
            {
                if (error != nullptr)
                    *error = "failed for testing";

                return nullptr;
            }

            ++pipelinesMade;
            return rive::make_rcp<TestOrePipeline>();
        }));
        ON_CALL (*oreCtx, makeBindGroup (_)).WillByDefault (Invoke ([] (const rive::ore::BindGroupDesc& desc) -> rive::rcp<rive::ore::BindGroup>
        {
            return rive::make_rcp<TestOreBindGroup> (desc);
        }));
        ON_CALL (*oreCtx, beginRenderPass (_, _)).WillByDefault (Invoke ([this] (const rive::ore::RenderPassDesc& desc, std::string*) -> std::unique_ptr<rive::ore::RenderPass>
        {
            auto& recorded = passes.emplace_back();
            recorded.colorCount = desc.colorCount;
            recorded.hasDepth = desc.depthStencil.view != nullptr;
            return std::make_unique<RecordingOreRenderPass> (recorded);
        }));
    }

    /** A render target texture of @p width x @p height. */
    GpuTexture::Ptr makeTarget (int width, int height, GpuTextureFormat format = GpuTextureFormat::rgba16float) const
    {
        return GpuTexture::create (device, GpuTextureDesc (static_cast<uint32_t> (width), static_cast<uint32_t> (height), format, true));
    }

    /** A zeroed storage buffer of @p byteSize bytes. */
    GpuBuffer::Ptr makeStorage (size_t byteSize = 64) const
    {
        const std::vector<uint8> zeros (byteSize);
        return GpuBuffer::createStorageForTesting (device, zeros.data(), zeros.size());
    }

    /** Every draw the backend render passes recorded. */
    int countDraws() const
    {
        int draws = 0;
        for (const auto& pass : passes)
            draws += pass.draws;

        return draws;
    }

    std::unique_ptr<::testing::NiceMock<MockOreContext>> oreCtx;
    GpuDevice::Ptr device;
    std::deque<RecordedPass> passes; // Every backend render pass begun, in order
    int pipelinesMade = 0;
    bool failTextures = false;
    bool failPipelines = false;
};
