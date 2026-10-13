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

#include <yup_3d/yup_3d.h>

#include <gtest/gtest.h>

using namespace yup;

class GeometryBindingsTests : public ::testing::Test
{
protected:
    static SceneDrawList::DrawItem makeItem (const Material& material, bool mirrored = false)
    {
        SceneDrawList::DrawItem item;
        item.material = &material;
        item.mirrored = mirrored;
        item.world = Matrix4::translation ({ 1.0f, 2.0f, 3.0f });
        return item;
    }
};

TEST_F (GeometryBindingsTests, RasterStateCombinesDoubleSidedAndMirrored)
{
    Material singleSided, doubleSided;
    doubleSided.doubleSided = true;

    EXPECT_EQ (GeometryBindings::getRasterState (makeItem (singleSided)), 0u);
    EXPECT_EQ (GeometryBindings::getRasterState (makeItem (singleSided, true)), 1u);
    EXPECT_EQ (GeometryBindings::getRasterState (makeItem (doubleSided)), 2u);
    EXPECT_EQ (GeometryBindings::getRasterState (makeItem (doubleSided, true)), 3u);
    EXPECT_EQ (GeometryBindings::numRasterStates, 4u);
}

TEST_F (GeometryBindingsTests, PipelineGeometryCullsAndWindsPerState)
{
    for (size_t state = 0; state < GeometryBindings::numRasterStates; ++state)
    {
        GpuPipelineOptions options;
        GeometryBindings::setPipelineGeometry (options, state);

        ASSERT_EQ (options.vertexBuffers.size(), 1u);
        EXPECT_EQ (options.vertexBuffers[0].stride, static_cast<uint32_t> (sizeof (Mesh::Vertex)));
        EXPECT_EQ (options.vertexBuffers[0].attributes.size(), 5u);
        EXPECT_EQ (options.indexFormat, GpuIndexFormat::uint32);
        EXPECT_EQ (options.cullMode, (state & 2) != 0 ? GpuCullMode::none : GpuCullMode::back);
        EXPECT_EQ (options.winding, (state & 1) != 0 ? GpuFaceWinding::clockwise : GpuFaceWinding::counterClockwise);

        // The draw, material, raster light list and raster probe list blocks move from draw to draw
        const std::vector<GpuUniformSlot> dynamic { { 2, 0 }, { 2, 1 }, { 2, 12 }, { 2, 13 } };
        EXPECT_EQ (options.dynamicUniformBuffers, dynamic);
    }
}

TEST_F (GeometryBindingsTests, DrawBlockCarriesTheMaterialIdAndThePreviousTransform)
{
    Material material;
    auto item = makeItem (material);

    // Without a node there is no previous transform: no motion
    auto block = GeometryBindings::makeDrawBlock (item, 7);
    EXPECT_FLOAT_EQ (block.ids[0], 7.0f);
    EXPECT_FLOAT_EQ (block.model[12], 1.0f);
    EXPECT_FLOAT_EQ (block.previousModel[12], 1.0f);

    MeshNode node;
    node.previousWorld = Matrix4::translation ({ 4.0f, 5.0f, 6.0f });
    item.node = &node;

    block = GeometryBindings::makeDrawBlock (item, 0);
    EXPECT_FLOAT_EQ (block.ids[0], 0.0f);
    EXPECT_FLOAT_EQ (block.model[12], 1.0f);
    EXPECT_FLOAT_EQ (block.previousModel[12], 4.0f);
    EXPECT_FLOAT_EQ (block.previousModel[13], 5.0f);
    EXPECT_FLOAT_EQ (block.previousModel[14], 6.0f);
}

TEST_F (GeometryBindingsTests, MaterialBlockPacksTheMaterial)
{
    Material material;
    material.alphaMode = Material::AlphaMode::mask;
    material.alphaCutoff = 0.25f;
    material.roughnessFactor = 0.3f;
    material.specularFactor = 0.5f;
    material.ior = 1.33f;

    const auto block = GeometryBindings::makeMaterialBlock (material, true);
    EXPECT_FLOAT_EQ (block.flags[0], 1.0f);
    EXPECT_FLOAT_EQ (block.flags[3], 1.0f);
    EXPECT_FLOAT_EQ (block.emissiveFactor[3], 0.25f);
    EXPECT_FLOAT_EQ (block.params[1], 0.3f);

    const auto expected = GpuSceneData::packMaterial (material);
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ (block.extension.specular[i], expected.specular[i]);
        EXPECT_FLOAT_EQ (block.extension.extended[i], expected.extended[i]);
    }

    EXPECT_FLOAT_EQ (GeometryBindings::makeMaterialBlock (material, false).flags[3], 0.0f);
}

/** GeometryBindings::draw() on a mocked device that binds uniform buffers at dynamic offsets,
    counting the set 2 bind groups the draws of a frame make. */
class GeometryBindingsMockTests : public ::testing::Test
{
protected:
    using RK = rive::ore::ResourceKind;

    class DynamicUniformDevice : public OreAndTargetGpuDevice
    {
    public:
        using OreAndTargetGpuDevice::OreAndTargetGpuDevice;

        bool isDynamicUniformOffsetAvailable() const noexcept override { return true; }
    };

    /** A backend pass that finish() finishes, as the real ones do. */
    class FinishingOreRenderPass : public ::testing::NiceMock<MockOreRenderPass>
    {
    public:
        FinishingOreRenderPass()
        {
            ON_CALL (*this, finish()).WillByDefault (::testing::Invoke ([this] { m_finished = true; }));
        }
    };

    void SetUp() override
    {
        using ::testing::_;
        using ::testing::Invoke;

        oreCtx = std::make_unique<::testing::NiceMock<MockOreContext>>();
        device = new DynamicUniformDevice (oreCtx.get(), MockOffscreenTarget::withGpuTexture (64, 64));

        ON_CALL (*oreCtx, makeBuffer (_)).WillByDefault (Invoke ([] (const rive::ore::BufferDesc& desc) -> rive::rcp<rive::ore::Buffer>
        {
            return rive::make_rcp<::testing::NiceMock<MockOreBuffer>> (desc.size);
        }));
        ON_CALL (*oreCtx, makeTexture (_)).WillByDefault (Invoke ([] (const rive::ore::TextureDesc& desc) -> rive::rcp<rive::ore::Texture>
        {
            return rive::make_rcp<::testing::NiceMock<MockOreTexture>> (desc);
        }));
        ON_CALL (*oreCtx, makeTextureViewImpl (_)).WillByDefault (Invoke ([] (const rive::ore::TextureViewDesc&) -> rive::rcp<rive::ore::TextureView>
        {
            return rive::make_rcp<TestOreTextureView>();
        }));
        ON_CALL (*oreCtx, wrapRiveTexture (_, _, _)).WillByDefault (Invoke ([] (rive::gpu::Texture*, uint32_t, uint32_t) -> rive::rcp<rive::ore::TextureView>
        {
            return rive::make_rcp<TestOreTextureView>();
        }));
        ON_CALL (*oreCtx, makeSampler (_)).WillByDefault (Invoke ([] (const rive::ore::SamplerDesc&) -> rive::rcp<rive::ore::Sampler>
        {
            return rive::make_rcp<TestOreSampler>();
        }));
        ON_CALL (*oreCtx, makeBindGroupLayout (_)).WillByDefault (Invoke ([] (const rive::ore::BindGroupLayoutDesc& desc) -> rive::rcp<rive::ore::BindGroupLayout>
        {
            return rive::make_rcp<TestOreBindGroupLayout> (desc);
        }));
        ON_CALL (*oreCtx, makePipeline (_, _)).WillByDefault (Invoke ([] (const rive::ore::PipelineDesc&, std::string*) -> rive::rcp<rive::ore::Pipeline>
        {
            return rive::make_rcp<TestOrePipeline>();
        }));
        ON_CALL (*oreCtx, makeBindGroup (_)).WillByDefault (Invoke ([this] (const rive::ore::BindGroupDesc& desc) -> rive::rcp<rive::ore::BindGroup>
        {
            if (desc.layout != nullptr && desc.layout->groupIndex() == 2)
                ++setTwoGroupsMade;

            return rive::make_rcp<TestOreBindGroup> (desc);
        }));
        ON_CALL (*oreCtx, beginRenderPass (_, _)).WillByDefault (Invoke ([this] (const rive::ore::RenderPassDesc&, std::string*)
        {
            auto* renderPass = new FinishingOreRenderPass();
            ON_CALL (*renderPass, setBindGroup (_, _, _, _)).WillByDefault (Invoke ([this] (uint32_t group, rive::ore::BindGroup*, const uint32_t*, uint32_t)
            {
                if (group == 2)
                    ++setTwoGroupsBound;
            }));
            return std::unique_ptr<rive::ore::RenderPass> (renderPass);
        }));

        mesh->addPrimitive ({ { { 0.0f, 1.0f, 0.0f } }, { { -1.0f, -1.0f, 0.0f } }, { { 1.0f, -1.0f, 0.0f } } }, { 0, 1, 2 });
        ASSERT_TRUE (geometry.prepare (device).wasOk());

        target = GpuTarget::create (device, 64, 64);
        ASSERT_NE (target, nullptr);
    }

    /** Compiles a pipeline from setPipelineGeometry() whose shaders declare a view block at set 0
        binding 0, and @p setTwo at set 2. */
    GpuPipeline::Ptr compileGeometryPipeline (const std::vector<std::pair<uint32_t, RK>>& setTwo)
    {
        rive::ore::BindingMap map;
        const auto declare = [&map] (uint32_t group, uint32_t binding, RK kind)
        {
            rive::ore::BindingMap::Entry e;
            e.group = group;
            e.binding = binding;
            e.kind = kind;
            e.stageMask = rive::ore::BindingMap::kStageVertex | rive::ore::BindingMap::kStageFragment;
            e.backendSlot[0] = (uint16_t) (group * 16 + binding);
            e.backendSlot[1] = (uint16_t) (group * 16 + binding);
            map.push (e);
        };

        declare (0, 0, RK::UniformBuffer);
        for (const auto& [binding, kind] : setTwo)
            declare (2, binding, kind);

        map.finalize();

        auto module = rive::make_rcp<TestOreShaderModule>();
        module->m_bindingMap = map;
        ON_CALL (*oreCtx, makeShaderModule (::testing::_)).WillByDefault (::testing::Return (module));

        GpuShaderSource source;
        source.language = GpuShaderLanguage::glsl;
        source.code = gpuShaderSourceBytes ("void main() {}");
        source.bindingMap = map.toBlob();

        GpuPipelineOptions options;
        GeometryBindings::setPipelineGeometry (options, 0);

        auto compiled = GpuPipeline::compile (device, source, source, options);
        EXPECT_TRUE (compiled.wasOk());
        return compiled.wasOk() ? compiled.getValue() : nullptr;
    }

    /** The set 2 of the raster ForwardPass: the draw and material blocks, five textures, their
        samplers and the light list. */
    static std::vector<std::pair<uint32_t, RK>> forwardSetTwo()
    {
        std::vector<std::pair<uint32_t, RK>> bindings { { 0, RK::UniformBuffer }, { 1, RK::UniformBuffer }, { 12, RK::UniformBuffer } };
        for (uint32_t binding = 2; binding <= 6; ++binding)
            bindings.push_back ({ binding, RK::SampledTexture });
        for (uint32_t binding = 7; binding <= 11; ++binding)
            bindings.push_back ({ binding, RK::Sampler });

        return bindings;
    }

    /** The set 2 of the raster ForwardPass local indirect variants: forwardSetTwo() and the probe list. */
    static std::vector<std::pair<uint32_t, RK>> localIndirectSetTwo()
    {
        auto bindings = forwardSetTwo();
        bindings.push_back ({ 13, RK::UniformBuffer });
        return bindings;
    }

    /** Draws @p count items, each moved and each with the next of @p materials, in one frame, setting
        a light list that changes every draw first when @p lightList is set, and a probe list that
        changes every draw when @p probeList is set. Returns the set 2 bind groups the frame made. */
    int drawFrame (const GpuPipeline::Ptr& pipeline, const std::vector<Material::Ptr>& materials, int count, bool lightList, bool probeList = false)
    {
        const auto madeBefore = setTwoGroupsMade;

        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame);
        pass.setPipeline (pipeline);

        const float view[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
        pass.setUniformBuffer (0, 0, view, sizeof (view));

        std::array<float, 132> lights {};
        std::array<float, 96> probes {}; // ProbeListBlock: two GpuReflectionProbe, a GpuIrradianceVolume and a count, 384 bytes

        for (int i = 0; i < count; ++i)
        {
            SceneDrawList::DrawItem item;
            item.mesh = mesh.get();
            item.primitive = &mesh->getPrimitive (0);
            item.material = materials[(size_t) i % materials.size()].get();
            item.world = Matrix4::translation ({ static_cast<float> (i), 0.0f, 0.0f });

            if (lightList)
            {
                lights[0] = static_cast<float> (i);
                pass.setUniformBuffer (2, 12, lights.data(), sizeof (lights));
            }

            if (probeList)
            {
                probes[0] = static_cast<float> (i);
                pass.setUniformBuffer (2, 13, probes.data(), sizeof (probes));
            }

            EXPECT_TRUE (geometry.draw (pass, device, item, 0));
        }

        EXPECT_TRUE (pass.finish());
        frame.submit();
        return setTwoGroupsMade - madeBefore;
    }

    std::unique_ptr<::testing::NiceMock<MockOreContext>> oreCtx;
    GpuDevice::Ptr device;
    GpuTarget::Ptr target;
    Mesh::Ptr mesh { new Mesh ("triangle") };
    GeometryBindings geometry;
    int setTwoGroupsMade = 0;
    int setTwoGroupsBound = 0;
};

TEST_F (GeometryBindingsMockTests, DrawsSharingAMaterialBuildSetTwoOncePerFrame)
{
    auto pipeline = compileGeometryPipeline (forwardSetTwo());
    ASSERT_NE (pipeline, nullptr);

    const std::vector<Material::Ptr> shared { new Material() };

    // Every draw moves the draw block and the light list, yet only binds set 2 at new offsets. The
    // blocks of 48 draws, about 1 KB each, fit in one 64 KB chunk of the frame's uniform buffer
    EXPECT_EQ (drawFrame (pipeline, shared, 48, true), 1);
    EXPECT_EQ (setTwoGroupsBound, 48);

    // The next frame writes into a chunk of its own, so builds the set once again
    EXPECT_EQ (drawFrame (pipeline, shared, 48, true), 1);
}

TEST_F (GeometryBindingsMockTests, MaterialsDifferingInFactorsShareSetTwo)
{
    auto pipeline = compileGeometryPipeline (forwardSetTwo());
    ASSERT_NE (pipeline, nullptr);

    // Without textures, materials share the stand-in textures and sampler: only their block moves
    Material::Ptr red { new Material() };
    Material::Ptr rough { new Material() };
    red->baseColorFactor = { 1.0f, 0.0f, 0.0f, 1.0f };
    rough->roughnessFactor = 0.9f;

    EXPECT_EQ (drawFrame (pipeline, { red, rough }, 48, true), 1);
}

TEST_F (GeometryBindingsMockTests, ShadowCastersSharingAMaterialBuildSetTwoOncePerFrame)
{
    // The opaque shadow pipeline declares only the draw block in set 2
    auto pipeline = compileGeometryPipeline ({ { 0, RK::UniformBuffer } });
    ASSERT_NE (pipeline, nullptr);

    const std::vector<Material::Ptr> shared { new Material() };
    EXPECT_EQ (drawFrame (pipeline, shared, 64, false), 1);
}

TEST_F (GeometryBindingsMockTests, ShadowCastersAlternatingMaterialsBuildSetTwoOncePerFrame)
{
    auto pipeline = compileGeometryPipeline ({ { 0, RK::UniformBuffer } });
    ASSERT_NE (pipeline, nullptr);

    // The opaque shadow set 2 lacks the material block, textures and samplers every draw sets
    Material::Ptr red { new Material() };
    Material::Ptr rough { new Material() };
    red->baseColorFactor = { 1.0f, 0.0f, 0.0f, 1.0f };
    rough->roughnessFactor = 0.9f;

    EXPECT_EQ (drawFrame (pipeline, { red, rough }, 64, false), 1);
}

TEST_F (GeometryBindingsMockTests, ProbeListsMovingPerDrawBuildSetTwoOncePerFrame)
{
    auto pipeline = compileGeometryPipeline (localIndirectSetTwo());
    ASSERT_NE (pipeline, nullptr);

    // The raster probe list is a dynamic block too: a new one per draw only moves its offset. Its 384
    // bytes take a 512 byte slot of the ring at a 256 byte alignment, as its 272 did before the volume
    const std::vector<Material::Ptr> shared { new Material() };
    EXPECT_EQ (drawFrame (pipeline, shared, 24, true, true), 1);
    EXPECT_EQ (setTwoGroupsBound, 24);
}

TEST_F (GeometryBindingsMockTests, ProbeListsOutsideThePipelineLeaveSetTwoAlone)
{
    // The default raster bundles declare no probe list: setting one dirties nothing
    auto pipeline = compileGeometryPipeline (forwardSetTwo());
    ASSERT_NE (pipeline, nullptr);

    const std::vector<Material::Ptr> shared { new Material() };
    EXPECT_EQ (drawFrame (pipeline, shared, 24, true, true), 1);
}
