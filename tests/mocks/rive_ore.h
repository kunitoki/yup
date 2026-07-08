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

#include <gmock/gmock.h>

#include <rive/renderer/ore/ore_context.hpp>
#include <rive/renderer/ore/ore_render_pass.hpp>
#include <rive/renderer/ore/ore_buffer.hpp>
#include <rive/renderer/ore/ore_texture.hpp>
#include <rive/renderer/ore/ore_sampler.hpp>
#include <rive/renderer/ore/ore_shader_module.hpp>
#include <rive/renderer/ore/ore_pipeline.hpp>
#include <rive/renderer/ore/ore_bind_group.hpp>
#include <rive/renderer/ore/ore_bind_group_layout.hpp>

// ==============================================================================
// Mock rive::ore::Context — the root GPU device abstraction.
// ==============================================================================

class MockOreContext : public rive::ore::Context
{
public:
    explicit MockOreContext()
        : rive::ore::Context (nullptr)
    {
    }

    MOCK_METHOD (rive::rcp<rive::ore::Buffer>, makeBuffer, (const rive::ore::BufferDesc&), (override));
    MOCK_METHOD (rive::rcp<rive::ore::Texture>, makeTexture, (const rive::ore::TextureDesc&), (override));
    MOCK_METHOD (rive::rcp<rive::ore::TextureView>, makeTextureView, (const rive::ore::TextureViewDesc&), (override));
    MOCK_METHOD (rive::rcp<rive::ore::Sampler>, makeSampler, (const rive::ore::SamplerDesc&), (override));
    MOCK_METHOD (rive::rcp<rive::ore::ShaderModule>, makeShaderModule, (const rive::ore::ShaderModuleDesc&), (override));
    MOCK_METHOD (rive::rcp<rive::ore::BindGroupLayout>, makeBindGroupLayout, (const rive::ore::BindGroupLayoutDesc&), (override));
    MOCK_METHOD (rive::rcp<rive::ore::Pipeline>, makePipeline, (const rive::ore::PipelineDesc&, std::string*), (override));
    MOCK_METHOD (rive::rcp<rive::ore::BindGroup>, makeBindGroup, (const rive::ore::BindGroupDesc&), (override));
    MOCK_METHOD (std::unique_ptr<rive::ore::RenderPass>, beginRenderPass, (const rive::ore::RenderPassDesc&, std::string*), (override));
    MOCK_METHOD (void, beginFrame, (const rive::ore::Context::FrameDescriptor&), (override));
    MOCK_METHOD (void, endFrame, (), (override));
    MOCK_METHOD (void, waitForGPU, (), (override));
    MOCK_METHOD (rive::rcp<rive::ore::TextureView>, wrapCanvasTexture, (rive::gpu::RenderCanvas*), (override));
    MOCK_METHOD (rive::rcp<rive::ore::TextureView>, wrapRiveTexture, (rive::gpu::Texture*, uint32_t, uint32_t), (override));
    MOCK_METHOD (rive::ore::ShaderTarget, shaderTarget, (), (const, override));
};

// ==============================================================================
// Mock rive::ore::RenderPass — draw-command encoder.
// ==============================================================================

class MockOreRenderPass : public rive::ore::RenderPass
{
public:
    MockOreRenderPass() = default;

    MOCK_METHOD (void, setPipeline, (rive::ore::Pipeline*), (override));
    MOCK_METHOD (void, setVertexBuffer, (uint32_t, rive::ore::Buffer*, uint32_t), (override));
    MOCK_METHOD (void, setIndexBuffer, (rive::ore::Buffer*, rive::ore::IndexFormat, uint32_t), (override));
    MOCK_METHOD (void, setBindGroup, (uint32_t, rive::ore::BindGroup*, const uint32_t*, uint32_t), (override));
    MOCK_METHOD (void, setViewport, (float, float, float, float, float, float), (override));
    MOCK_METHOD (void, setScissorRect, (uint32_t, uint32_t, uint32_t, uint32_t), (override));
    MOCK_METHOD (void, setStencilReference, (uint32_t), (override));
    MOCK_METHOD (void, setBlendColor, (float, float, float, float), (override));
    MOCK_METHOD (void, draw, (uint32_t, uint32_t, uint32_t, uint32_t), (override));
    MOCK_METHOD (void, drawIndexed, (uint32_t, uint32_t, uint32_t, int32_t, uint32_t), (override));
    MOCK_METHOD (void, finish, (), (override));
};

// ==============================================================================
// Mock rive::ore::Buffer — GPU buffer resource.
// ==============================================================================

class MockOreBuffer : public rive::ore::Buffer
{
public:
    MockOreBuffer()
        : rive::ore::Buffer (0, rive::ore::BufferUsage::uniform)
    {
    }

    MOCK_METHOD (void, update, (const void*, uint32_t, uint32_t), (override));
};

// ==============================================================================
// Mock rive::ore::Texture — GPU texture resource.
// ==============================================================================

struct MockOreTexture : public rive::ore::Texture
{
    MockOreTexture()
        : rive::ore::Texture ({})
    {
    }

    MOCK_METHOD (void, upload, (const rive::ore::TextureDataDesc&), (override));
};

// ==============================================================================
// Test-friendly subclasses for types with only virtual destructors.
// These expose protected constructors so tests can instantiate them.
// ==============================================================================

struct TestOreTextureView : public rive::ore::TextureView
{
    TestOreTextureView()
        : rive::ore::TextureView (static_cast<rive::rcp<rive::ore::Texture>> (nullptr), {})
    {
    }
};

struct TestOrePipeline : public rive::ore::Pipeline
{
    TestOrePipeline()
        : rive::ore::Pipeline (rive::ore::PipelineDesc())
    {
    }
};

struct TestOreBindGroup : public rive::ore::BindGroup
{
    TestOreBindGroup() = default;
};

struct TestOreBindGroupLayout : public rive::ore::BindGroupLayout
{
    TestOreBindGroupLayout() = default;
};

struct TestOreSampler : public rive::ore::Sampler
{
    TestOreSampler() = default;
};

struct TestOreShaderModule : public rive::ore::ShaderModule
{
    TestOreShaderModule() = default;
};
