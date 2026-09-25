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

#include <yup_shading/yup_shading.h>

//==============================================================================

/**
    Image-based-lighting PBR demo on the RHI texture / sampler surface.

    Everything here is baked on the GPU over the first few painted frames - one
    stage per frame, so the bake never hitches the render thread - and is then
    reused unchanged:

    - A **procedural sky** is rendered into all six faces of an rgba16float
      cube map, one render pass per face, targeting the face through a
      layer-narrowed attachment view (GpuTarget::createFromTexture). Its sun
      disk and halo track the sun-intensity slider, so dragging the slider
      re-bakes the whole environment coherently.
    - The sky cube is convolved into a small **irradiance cube** for the diffuse
      IBL term.
    - A **prefiltered specular cube** is built as a mip chain: one render pass per
      (mip, face) pair, each importance-sampling the sky at its own roughness.
      Because ore has no mipmap generation, every level is rendered rather than
      downsampled - which is what a roughness-varying prefilter wants anyway.
    - The split-sum **BRDF lookup table** is integrated into a two-channel
      rg16float 2D target.
    - CPU-generated albedo, normal and RMA (roughness-metallic-ao) textures are
      uploaded with GpuTexture::upload() to exercise the CPU-to-GPU path.

    Every frame then draws a sky backdrop and a 5x3 grid of spheres in one
    render pass sharing one depth attachment. Each sphere carries its own
    material through a per-instance vertex buffer: row 0 bare metals with
    physically-based f0 tints (gold, copper, steel, titanium, iron), row 1
    glossy coatings and ceramics with a clearcoat lobe, row 2 matte textured
    materials modulated by the RMA map. The grid itself is instanced in a single
    indexed draw.

    Float render targets are extension-gated on OpenGL ES / WebGL2, so the demo
    probes GpuDevice::isFormatRenderable() and falls back to 8-bit targets
    instead of rendering black. Light is exposed and tonemapped with the ACES
    fit before the final gamma curve.

    @see GpuTexture, GpuSampler, GpuTarget::createFromTexture, GpuRenderPass
*/
class PbrDemo : public yup::Component
{
public:
    //==============================================================================
    PbrDemo()
        : yup::Component ("PbrDemo")
    {
        exposureSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        exposureSlider->setRange (0.1, 4.0);
        exposureSlider->setValue (1.1);
        exposureSlider->onValueChanged = [this] (double v)
        {
            exposure = (float) v;
        };
        addAndMakeVisible (exposureSlider.get());

        sunSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        sunSlider->setRange (0.0, 12.0);
        sunSlider->setValue (4.0);
        sunSlider->onValueChanged = [this] (double v)
        {
            sunIntensity = (float) v;

            if (pipelinesReady && bakeStagesDone >= kBakeStageCount)
                bakeStagesDone = 0;
        };
        addAndMakeVisible (sunSlider.get());

        statusLabel = std::make_unique<yup::Label> ("status");
        statusLabel->setText ("Initializing GPU...", yup::dontSendNotification);
        addAndMakeVisible (statusLabel.get());
    }

    ~PbrDemo() override = default;

    //==============================================================================
    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        if (! isDragging)
            cameraYaw += (float) (lastFrameTimeSeconds * 0.25);

        repaint (getSceneArea());
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::darkslategray));
        g.fillAll();

        if (capturedContext == nullptr)
        {
            capturedContext = &g.getGraphicsContext();
            initGpu();
        }

        const auto sceneBounds = getSceneArea();
        const int w = (int) sceneBounds.getWidth();
        const int h = (int) sceneBounds.getHeight();

        if (w < 2 || h < 2 || ! pipelinesReady)
            return;

        auto device = capturedContext->getGpuDevice();

        if (sceneTarget == nullptr || sceneTarget->getWidth() != w || sceneTarget->getHeight() != h)
        {
            sceneTarget = yup::GpuTarget::create (device, w, h);
            depthTexture = yup::GpuTexture::create (device, makeDepthDesc (w, h));

            if (sceneTarget == nullptr || depthTexture == nullptr)
                return;
        }

        if (bakeStagesDone < kBakeStageCount)
            bakeStage (device, bakeStagesDone++);

        auto frame = yup::GpuFrame::begin (device);
        if (! frame.isValid())
            return;

        renderScene (frame, w, h);

        frame.submit();

        if (auto tex = sceneTarget->asTexture())
            g.drawTexture (tex, sceneBounds);
    }

    //==============================================================================
    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        statusLabel->setBounds (bounds.removeFromBottom (25.0f));
        bounds.removeFromBottom (4.0f);
        sunSlider->setBounds (bounds.removeFromBottom (26.0f));
        bounds.removeFromBottom (4.0f);
        exposureSlider->setBounds (bounds.removeFromBottom (26.0f));
    }

    //==============================================================================
    void mouseDown (const yup::MouseEvent& event) override
    {
        isDragging = true;
        lastDragPosition = event.getPosition();
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        const auto position = event.getPosition();
        const auto delta = position - lastDragPosition;
        lastDragPosition = position;

        cameraYaw -= delta.getX() * 0.01f;
        cameraPitch = yup::jlimit (-1.3f, 1.3f, cameraPitch + delta.getY() * 0.01f);
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        isDragging = false;
    }

private:
    //==============================================================================
    static constexpr int kEnvSize = 256;       // Sky cube face size.
    static constexpr int kIrradianceSize = 32; // Diffuse irradiance cube face size.
    static constexpr int kPrefilterSize = 256; // Specular prefilter base face size.
    static constexpr int kPrefilterMips = 6;   // Roughness levels in the prefilter chain.
    static constexpr int kBrdfSize = 256;      // Split-sum BRDF lookup table size.
    static constexpr int kMaterialTextureSize = 256;

    static constexpr int kSphereRings = 32;
    static constexpr int kSphereSectors = 48;

    static constexpr int kGridColumns = 5; // Column axis: roughness sweep (see kMaterialPresets).
    static constexpr int kGridRows = 3;    // Row axis: material class (metals / coated / matte).

    static constexpr int kBakeStageCount = 2 + kPrefilterMips * 6 + 1;

    static constexpr float kBackdropRadius = 60.0f;

    //==============================================================================
    struct PbrVertex
    {
        float position[3];
        float normal[3];
        float uv[2];
    };

    struct alignas (16) InstanceMaterial
    {
        float metallic;         // a_material.x
        float roughness;        // a_material.y
        float uvScale;          // a_material.z
        float normalStrength;   // a_material.w

        float tint[4];          // a_tint: linear albedo / metal f0 rgb + roughness-map amount (a)

        float clearcoat;        // a_extra.x
        float clearcoatRoughness; // a_extra.y
        float aoScale;          // a_extra.z
        float unused;           // a_extra.w
    };

    struct alignas (16) BakeUniforms
    {
        int32_t face;
        float roughness;
        float flipY;
        float envSize;
        float sunIntensity; // scales the baked sun disk / halo in the sky
    };

    struct alignas (16) SceneUniforms
    {
        float camera[4];   // yaw, pitch, distance, aspect
        float material[4]; // exposure, sunIntensity, prefilterMaxLod, backdropRadius (0 = sphere grid)
        float light[4];    // sun direction xyz, unused
    };

    //==============================================================================
    yup::Rectangle<float> getSceneArea() const
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        bounds.removeFromBottom (25.0f + 4.0f + 26.0f + 4.0f + 26.0f);
        return bounds;
    }

    static yup::GpuTextureDesc makeDepthDesc (int width, int height)
    {
        yup::GpuTextureDesc desc;
        desc.width = (uint32_t) width;
        desc.height = (uint32_t) height;
        desc.format = yup::GpuTextureFormat::depth24plusStencil8;
        desc.renderTarget = true;
        desc.label = "PbrDemo depth";
        return desc;
    }

    // ---- GPU initialisation --------------------------------------------------

    void initGpu()
    {
        if (capturedContext == nullptr || ! capturedContext->isGpuAvailable())
        {
            statusLabel->setText ("GPU context unavailable", yup::dontSendNotification);
            return;
        }

#if ! YUP_ENABLE_SHADER_TRANSPILER
        statusLabel->setText ("Requires YUP_ENABLE_SHADER_TRANSPILER", yup::dontSendNotification);
#else
        auto device = capturedContext->getGpuDevice();

        hdrFormat = device->isFormatRenderable (yup::GpuTextureFormat::rgba16float)
                      ? yup::GpuTextureFormat::rgba16float
                      : yup::GpuTextureFormat::rgba8unorm;

        brdfFormat = device->isFormatRenderable (yup::GpuTextureFormat::rg16float)
                       ? yup::GpuTextureFormat::rg16float
                       : yup::GpuTextureFormat::rgba8unorm;

        if (! createGeometry (device) || ! createMaterialTextures (device) || ! createIblResources (device))
        {
            statusLabel->setText ("Failed to allocate GPU resources", yup::dontSendNotification);
            return;
        }

        if (! compilePipelines (device))
            return;

        pipelinesReady = true;

        statusLabel->setText (yup::String ("PBR IBL ready - environment ")
                                  + (hdrFormat == yup::GpuTextureFormat::rgba16float ? "256px rgba16float" : "256px rgba8unorm (no float targets)")
                                  + ", prefilter "
                                  + (hdrFormat == yup::GpuTextureFormat::rgba16float ? "rgba16float" : "rgba8unorm")
                                  + ", 15 materials, clearcoat, ACES",
                              yup::dontSendNotification);
#endif
    }

    bool createGeometry (yup::GpuDevice::Ptr device)
    {
        std::vector<PbrVertex> sphereVertices;
        std::vector<uint16_t> sphereIndices;
        buildSphere (sphereVertices, sphereIndices);

        sphereIndexCount = (uint32_t) sphereIndices.size();

        sphereVBO = device->createBuffer (yup::GpuBufferType::vertex, sphereVertices.data(), sphereVertices.size() * sizeof (PbrVertex));
        sphereIBO = device->createBuffer (yup::GpuBufferType::index, sphereIndices.data(), sphereIndices.size() * sizeof (uint16_t));

        return sphereVBO != nullptr
            && sphereIBO != nullptr
            && createInstanceData (device);
    }

    /** Builds a UV sphere of unit radius with normals and spherical UVs. */
    static void buildSphere (std::vector<PbrVertex>& vertices, std::vector<uint16_t>& indices)
    {
        constexpr float pi = yup::MathConstants<float>::pi;
        constexpr float twoPi = yup::MathConstants<float>::twoPi;

        for (int ring = 0; ring <= kSphereRings; ++ring)
        {
            const float v = (float) ring / (float) kSphereRings;
            const float theta = v * pi;
            const float sinTheta = std::sin (theta);
            const float cosTheta = std::cos (theta);

            for (int sector = 0; sector <= kSphereSectors; ++sector)
            {
                const float u = (float) sector / (float) kSphereSectors;
                const float phi = u * twoPi;

                const float x = sinTheta * std::cos (phi);
                const float y = cosTheta;
                const float z = sinTheta * std::sin (phi);

                vertices.push_back ({ { x, y, z }, { x, y, z }, { u, v } });
            }
        }

        const int stride = kSphereSectors + 1;

        for (int ring = 0; ring < kSphereRings; ++ring)
        {
            for (int sector = 0; sector < kSphereSectors; ++sector)
            {
                const auto a = (uint16_t) (ring * stride + sector);
                const auto b = (uint16_t) (a + stride);

                indices.push_back (a);
                indices.push_back (b);
                indices.push_back ((uint16_t) (a + 1));

                indices.push_back ((uint16_t) (a + 1));
                indices.push_back (b);
                indices.push_back ((uint16_t) (b + 1));
            }
        }
    }

    /** One cell of the 5x3 material chart, ordered row-major so the i-th
        instance matches the shader's row = i / kColumns, column = i % kColumns.
        Row 0 = bare metals (metallic 1, the albedo doubles as the f0 tint),
        row 1 = glossy coatings / ceramics with a clearcoat lobe,
        row 2 = matte textured materials modulated by the RMA map. */
    struct MaterialPreset
    {
        float albedo[3]; // linear-space base color / metal f0
        float metallic;
        float roughness;
        float uvScale;
        float normalStrength;
        float mapAmount; // how strongly the albedo / normal / RMA maps modulate the base
        float clearcoat;
        float clearcoatRoughness;
        float aoScale;
    };

    static constexpr MaterialPreset kMaterialPresets[kGridRows][kGridColumns] = {
        { // Bare metals: roughness sweeps left to right, the tint is the metal's f0.
          { { 1.000f, 0.766f, 0.336f }, 1.0f, 0.06f, 1.4f, 0.12f, 0.15f, 0.0f, 0.0f, 0.35f }, // gold
          { { 0.955f, 0.637f, 0.538f }, 1.0f, 0.12f, 1.4f, 0.12f, 0.15f, 0.0f, 0.0f, 0.35f }, // copper
          { { 0.800f, 0.830f, 0.860f }, 1.0f, 0.22f, 1.4f, 0.12f, 0.15f, 0.0f, 0.0f, 0.35f }, // steel
          { { 0.620f, 0.570f, 0.500f }, 1.0f, 0.38f, 1.4f, 0.12f, 0.15f, 0.0f, 0.0f, 0.35f }, // titanium
          { { 0.560f, 0.570f, 0.580f }, 1.0f, 0.62f, 1.4f, 0.12f, 0.15f, 0.0f, 0.0f, 0.35f }  // iron
        },
        { // Glossy dielectrics: painted and glazed surfaces, clearcoat on the left.
          { { 0.620f, 0.050f, 0.060f }, 0.0f, 0.10f, 2.2f, 0.30f, 0.35f, 0.90f, 0.05f, 0.50f }, // carmine lacquer
          { { 0.900f, 0.890f, 0.850f }, 0.0f, 0.17f, 2.2f, 0.30f, 0.35f, 0.65f, 0.05f, 0.50f }, // pearl ceramic
          { { 0.100f, 0.160f, 0.550f }, 0.0f, 0.28f, 2.2f, 0.30f, 0.35f, 0.40f, 0.06f, 0.50f }, // cobalt enamel
          { { 0.240f, 0.420f, 0.180f }, 0.0f, 0.42f, 2.2f, 0.30f, 0.35f, 0.0f, 0.0f, 0.50f },  // moss plastic
          { { 0.260f, 0.100f, 0.300f }, 0.0f, 0.62f, 2.2f, 0.30f, 0.35f, 0.0f, 0.0f, 0.50f }   // aubergine plastic
        },
        { // Matte textured materials: the RMA map does the talking.
          { { 0.180f, 0.100f, 0.060f }, 0.0f, 0.38f, 5.0f, 0.50f, 1.00f, 0.0f, 0.0f, 0.85f }, // leather
          { { 0.120f, 0.120f, 0.140f }, 0.0f, 0.52f, 5.0f, 0.50f, 1.00f, 0.0f, 0.0f, 0.85f }, // charcoal
          { { 0.420f, 0.190f, 0.080f }, 0.0f, 0.66f, 5.0f, 0.50f, 1.00f, 0.0f, 0.0f, 0.85f }, // terracotta
          { { 0.240f, 0.270f, 0.310f }, 0.0f, 0.80f, 5.0f, 0.50f, 1.00f, 0.0f, 0.0f, 0.85f }, // slate
          { { 0.550f, 0.450f, 0.280f }, 0.0f, 0.95f, 5.0f, 0.50f, 1.00f, 0.0f, 0.0f, 0.85f }  // sand
        }
    };

    bool createInstanceData (yup::GpuDevice::Ptr device)
    {
        std::vector<InstanceMaterial> data;
        data.reserve (kGridRows * kGridColumns);

        for (int row = 0; row < kGridRows; ++row)
        {
            for (int column = 0; column < kGridColumns; ++column)
            {
                const auto& p = kMaterialPresets[row][column];
                data.push_back ({
                    p.metallic,
                    p.roughness,
                    p.uvScale,
                    p.normalStrength,
                    { p.albedo[0], p.albedo[1], p.albedo[2], p.mapAmount },
                    p.clearcoat,
                    p.clearcoatRoughness,
                    p.aoScale,
                    0.0f
                });
            }
        }

        sphereInstanceVBO = device->createBuffer (yup::GpuBufferType::vertex, data.data(), data.size() * sizeof (InstanceMaterial));

        return sphereInstanceVBO != nullptr;
    }

    bool createMaterialTextures (yup::GpuDevice::Ptr device)
    {
        constexpr int size = kMaterialTextureSize;

        yup::GpuTextureDesc desc;
        desc.width = desc.height = (uint32_t) size;
        desc.format = yup::GpuTextureFormat::rgba8unorm;

        desc.label = "PbrDemo albedo";
        albedoTexture = yup::GpuTexture::create (device, desc);

        desc.label = "PbrDemo normal map";
        normalTexture = yup::GpuTexture::create (device, desc);

        desc.label = "PbrDemo RMA";
        rmaTexture = yup::GpuTexture::create (device, desc);

        if (albedoTexture == nullptr || normalTexture == nullptr || rmaTexture == nullptr)
            return false;

        auto valueNoise = [] (float u, float v, float fu, float fv, float phase)
        {
            constexpr float twoPi = yup::MathConstants<float>::twoPi;
            return std::sin ((u * fu + phase) * twoPi) * std::sin ((v * fv + phase * 1.3f) * twoPi);
        };

        auto height = [&valueNoise] (float u, float v)
        {
            const float lo  = valueNoise (u, v, 2.0f, 3.0f, 0.30f);
            const float mid = valueNoise (u, v, 6.0f, 5.0f, 0.85f);
            const float hi  = valueNoise (u, v, 17.0f, 13.0f, 1.45f);
            return 0.5f + 0.5f * (0.45f * lo + 0.30f * mid + 0.25f * hi);
        };

        std::vector<uint8_t> albedo ((size_t) size * size * 4);
        std::vector<uint8_t> normals ((size_t) size * size * 4);
        std::vector<uint8_t> rma ((size_t) size * size * 4);

        const float step = 1.0f / (float) size;

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const size_t index = ((size_t) y * size + x) * 4;
                const float u = ((float) x + 0.5f) * step;
                const float v = ((float) y + 0.5f) * step;

                const float blotch = 0.5f + 0.5f * valueNoise (u, v, 4.0f, 3.0f, 2.10f);
                const float warm = 0.5f + 0.5f * valueNoise (u, v, 9.0f, 7.0f, 0.40f);
                albedo[index + 0] = (uint8_t) yup::jlimit (0, 255, (int) ((0.82f + 0.14f * blotch) * 255.0f));
                albedo[index + 1] = (uint8_t) yup::jlimit (0, 255, (int) ((0.80f + 0.13f * blotch + 0.03f * warm) * 255.0f));
                albedo[index + 2] = (uint8_t) yup::jlimit (0, 255, (int) ((0.75f + 0.12f * blotch + 0.05f * warm) * 255.0f));
                albedo[index + 3] = 255;

                const float dx = height (u + step, v) - height (u - step, v);
                const float dy = height (u, v + step) - height (u, v - step);
                const float mag = std::sqrt (dx * dx + dy * dy);

                float nx = -dx * 5.0f;
                float ny = -dy * 5.0f;
                float nz = 1.0f;

                const float length = std::sqrt (nx * nx + ny * ny + nz * nz);
                nx /= length;
                ny /= length;
                nz /= length;

                normals[index + 0] = (uint8_t) yup::jlimit (0, 255, (int) ((nx * 0.5f + 0.5f) * 255.0f));
                normals[index + 1] = (uint8_t) yup::jlimit (0, 255, (int) ((ny * 0.5f + 0.5f) * 255.0f));
                normals[index + 2] = (uint8_t) yup::jlimit (0, 255, (int) ((nz * 0.5f + 0.5f) * 255.0f));
                normals[index + 3] = 255;

                const float fine = valueNoise (u, v, 23.0f, 19.0f, 0.55f);
                const float cavity = yup::jlimit (0.0f, 1.0f, mag / 0.16f);
                rma[index + 0] = 255;
                rma[index + 1] = (uint8_t) yup::jlimit (0, 255, (int) (yup::jlimit (0.15f, 0.85f, 0.5f + 0.35f * fine) * 255.0f));
                rma[index + 2] = (uint8_t) yup::jlimit (0, 255, (int) ((1.0f - 0.6f * cavity) * 255.0f));
                rma[index + 3] = 255;
            }
        }

        yup::GpuTextureDataDesc upload;
        upload.data = albedo.data();
        if (! albedoTexture->upload (upload))
            return false;

        upload.data = normals.data();
        if (! normalTexture->upload (upload))
            return false;

        upload.data = rma.data();
        return rmaTexture->upload (upload);
    }

    bool createIblResources (yup::GpuDevice::Ptr device)
    {
        auto makeCube = [&] (int size, int mips, const char* label)
        {
            yup::GpuTextureDesc desc;
            desc.width = desc.height = (uint32_t) size;
            desc.depthOrArrayLayers = 6;
            desc.type = yup::GpuTextureType::cube;
            desc.format = hdrFormat;
            desc.renderTarget = true;
            desc.mipLevels = (uint32_t) mips;
            desc.label = label;
            return yup::GpuTexture::create (device, desc);
        };

        environmentCube = makeCube (kEnvSize, 1, "PbrDemo environment");
        irradianceCube = makeCube (kIrradianceSize, 1, "PbrDemo irradiance");
        prefilterCube = makeCube (kPrefilterSize, kPrefilterMips, "PbrDemo prefilter");

        yup::GpuTextureDesc brdfDesc;
        brdfDesc.width = brdfDesc.height = (uint32_t) kBrdfSize;
        brdfDesc.format = brdfFormat;
        brdfDesc.renderTarget = true;
        brdfDesc.label = "PbrDemo BRDF LUT";
        brdfLut = yup::GpuTexture::create (device, brdfDesc);

        if (environmentCube == nullptr || irradianceCube == nullptr || prefilterCube == nullptr || brdfLut == nullptr)
            return false;

        yup::GpuSamplerDesc samplerDesc;
        samplerDesc.minFilter = yup::GpuFilter::linear;
        samplerDesc.magFilter = yup::GpuFilter::linear;
        samplerDesc.mipmapFilter = yup::GpuFilter::linear;
        samplerDesc.maxLod = (float) kPrefilterMips;
        samplerDesc.label = "PbrDemo trilinear";
        trilinearSampler = yup::GpuSampler::create (device, samplerDesc);

        samplerDesc.wrapU = yup::GpuWrapMode::repeat;
        samplerDesc.wrapV = yup::GpuWrapMode::repeat;
        samplerDesc.label = "PbrDemo repeat";
        materialSampler = yup::GpuSampler::create (device, samplerDesc);

        return trilinearSampler != nullptr && materialSampler != nullptr;
    }

    // ---- Pipelines -----------------------------------------------------------

#if YUP_ENABLE_SHADER_TRANSPILER
    bool compilePipelines (yup::GpuDevice::Ptr device)
    {
        auto compile = [&] (const char* name,
                            const yup::String& vertexSource,
                            const yup::String& fragmentSource,
                            const yup::GpuPipelineOptions& options) -> yup::GpuPipeline::Ptr
        {
            auto result = yup::GpuPipeline::compileFromGlsl (device, vertexSource, fragmentSource, options);

            if (result.failed())
            {
                statusLabel->setText (yup::String (name) + " shader failed: " + result.getErrorMessage(),
                                      yup::dontSendNotification);
                return nullptr;
            }

            return result.getReference();
        };

        const auto fullscreenVert = yup::String::fromUTF8 (kFullscreenVert, sizeof (kFullscreenVert) - 1);
        const auto faceHelpers = yup::String::fromUTF8 (kFaceHelpers, sizeof (kFaceHelpers) - 1);
        const auto sceneVert = yup::String::fromUTF8 (kSceneVert, sizeof (kSceneVert) - 1);

        skyPipeline = compile ("Sky bake", fullscreenVert, faceHelpers + kSkyFrag, bakePipelineOptions (hdrFormat));
        irradiancePipeline = compile ("Irradiance", fullscreenVert, faceHelpers + kIrradianceFrag, bakePipelineOptions (hdrFormat));
        prefilterPipeline = compile ("Prefilter", fullscreenVert, faceHelpers + kPrefilterFrag, bakePipelineOptions (hdrFormat));
        brdfPipeline = compile ("BRDF LUT", fullscreenVert, yup::String::fromUTF8 (kBrdfFrag, sizeof (kBrdfFrag) - 1), bakePipelineOptions (brdfFormat));

        backgroundPipeline = compile ("Background", sceneVert, yup::String::fromUTF8 (kBackgroundFrag, sizeof (kBackgroundFrag) - 1), backgroundPipelineOptions());
        scenePipeline = compile ("Scene", sceneVert, yup::String::fromUTF8 (kSceneFrag, sizeof (kSceneFrag) - 1), scenePipelineOptions());

        return skyPipeline != nullptr
            && irradiancePipeline != nullptr
            && prefilterPipeline != nullptr
            && brdfPipeline != nullptr
            && backgroundPipeline != nullptr
            && scenePipeline != nullptr;
    }
#else
    bool compilePipelines (yup::GpuDevice::Ptr) { return false; }
#endif

    static yup::GpuPipelineOptions bakePipelineOptions (yup::GpuTextureFormat format)
    {
        yup::GpuPipelineOptions options;
        auto& colorTarget = options.colorTargets.emplace_back();
        colorTarget.format = format;
        colorTarget.blendEnabled = false;
        return options;
    }

    static std::vector<yup::GpuVertexBufferLayout> sceneVertexLayouts()
    {
        return {
            { (uint32_t) sizeof (PbrVertex), yup::GpuVertexStepMode::vertex, {
                  { yup::GpuVertexFormat::float3, 0, 0 },
                  { yup::GpuVertexFormat::float3, 12, 1 },
                  { yup::GpuVertexFormat::float2, 24, 2 },
              } },
            { (uint32_t) sizeof (InstanceMaterial), yup::GpuVertexStepMode::instance, {
                  { yup::GpuVertexFormat::float4, 0, 3 },  // a_material
                  { yup::GpuVertexFormat::float4, 16, 4 }, // a_tint
                  { yup::GpuVertexFormat::float4, 32, 5 }, // a_extra
              } },
        };
    }

    static yup::GpuPipelineOptions scenePipelineOptions()
    {
        yup::GpuPipelineOptions options;
        options.vertexBuffers = sceneVertexLayouts();
        options.indexFormat = yup::GpuIndexFormat::uint16;
        options.cullMode = yup::GpuCullMode::none;

        auto& colorTarget = options.colorTargets.emplace_back();
        colorTarget.format = yup::GpuTextureFormat::rgba8unorm;
        colorTarget.blendEnabled = false;

        options.depthStencil.enabled = true;
        options.depthStencil.format = yup::GpuTextureFormat::depth24plusStencil8;
        options.depthStencil.depthCompare = yup::GpuCompareFunction::less;
        options.depthStencil.depthWriteEnabled = true;
        return options;
    }

    static yup::GpuPipelineOptions backgroundPipelineOptions()
    {
        auto options = scenePipelineOptions();
        options.depthStencil.depthCompare = yup::GpuCompareFunction::always;
        options.depthStencil.depthWriteEnabled = false;
        return options;
    }

    // ---- Baking --------------------------------------------------------------

    void bakeStage (yup::GpuDevice::Ptr device, int stage)
    {
        if (stage == 0)
        {
            bakeEnvironment (device);
            return;
        }

        if (stage == 1)
        {
            bakeIrradiance (device);
            return;
        }

        const int prefilterStages = kPrefilterMips * 6;
        if (stage < 2 + prefilterStages)
        {
            const int job = stage - 2;
            bakePrefilterFace (device, job / 6, job % 6);
            return;
        }

        bakeBrdfLut (device);
    }

    void bakeEnvironment (yup::GpuDevice::Ptr device)
    {
        auto frame = yup::GpuFrame::begin (device);

        for (int face = 0; face < 6; ++face)
        {
            auto target = yup::GpuTarget::createFromTexture (device, environmentCube, { 0, (uint32_t) face });
            if (target == nullptr)
                continue;

            BakeUniforms uniforms { face, 0.0f, 1.0f, (float) kEnvSize, sunIntensity };

            auto pass = target->beginRenderPass (frame, { true, yup::Colors::black });
            pass.setPipeline (skyPipeline);
            pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
            pass.draw (3);
            pass.finish();
        }
    }

    void bakeIrradiance (yup::GpuDevice::Ptr device)
    {
        auto frame = yup::GpuFrame::begin (device);

        for (int face = 0; face < 6; ++face)
        {
            auto target = yup::GpuTarget::createFromTexture (device, irradianceCube, { 0, (uint32_t) face });
            if (target == nullptr)
                continue;

            BakeUniforms uniforms { face, 0.0f, 1.0f, (float) kEnvSize, sunIntensity };

            auto pass = target->beginRenderPass (frame, { true, yup::Colors::black });
            pass.setPipeline (irradiancePipeline);
            pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
            pass.setTexture (0, 1, environmentCube);
            pass.setSampler (0, 2, trilinearSampler);
            pass.draw (3);
            pass.finish();
        }
    }

    void bakePrefilterFace (yup::GpuDevice::Ptr device, int mip, int face)
    {
        auto frame = yup::GpuFrame::begin (device);

        auto target = yup::GpuTarget::createFromTexture (device, prefilterCube, { (uint32_t) mip, (uint32_t) face });
        if (target == nullptr)
            return;

        const float roughness = (float) mip / (float) (kPrefilterMips - 1);
        BakeUniforms uniforms { face, roughness, 1.0f, (float) kEnvSize, sunIntensity };

        auto pass = target->beginRenderPass (frame, { true, yup::Colors::black });
        pass.setPipeline (prefilterPipeline);
        pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
        pass.setTexture (0, 1, environmentCube);
        pass.setSampler (0, 2, trilinearSampler);
        pass.draw (3);
        pass.finish();
    }

    void bakeBrdfLut (yup::GpuDevice::Ptr device)
    {
        auto frame = yup::GpuFrame::begin (device);

        if (auto target = yup::GpuTarget::createFromTexture (device, brdfLut))
        {
            BakeUniforms uniforms { 0, 0.0f, 1.0f, (float) kBrdfSize, sunIntensity };

            auto pass = target->beginRenderPass (frame, { true, yup::Colors::black });
            pass.setPipeline (brdfPipeline);
            pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
            pass.draw (3);
            pass.finish();
        }
    }

    // ---- Per-frame scene -----------------------------------------------------

    void renderScene (yup::GpuFrame& frame, int width, int height)
    {
        SceneUniforms uniforms {};
        uniforms.camera[0] = cameraYaw;
        uniforms.camera[1] = cameraPitch;
        uniforms.camera[2] = cameraDistance;
        uniforms.camera[3] = (float) width / (float) height;
        uniforms.material[0] = exposure;
        uniforms.material[1] = sunIntensity;
        uniforms.material[2] = (float) (kPrefilterMips - 1);
        uniforms.material[3] = 0.0f; // backdrop radius 0 => sphere grid
        uniforms.light[0] = 0.35f;   // must match the sky's sun direction
        uniforms.light[1] = 0.28f;
        uniforms.light[2] = -0.90f;
        uniforms.light[3] = 0.0f;

        auto pass = sceneTarget->beginRenderPass (frame, { true, yup::Colors::black });
        pass.setDepthStencilAttachment (depthTexture);

        auto backdropUniforms = uniforms;
        backdropUniforms.material[3] = kBackdropRadius;
        pass.setPipeline (backgroundPipeline);
        pass.setUniformBuffer (0, 0, &backdropUniforms, sizeof (backdropUniforms));
        pass.setTexture (0, 1, environmentCube);
        pass.setSampler (0, 2, trilinearSampler);
        pass.setVertexBuffer (0, sphereVBO);
        pass.setVertexBuffer (1, sphereInstanceVBO);
        pass.setIndexBuffer (yup::GpuIndexFormat::uint16, sphereIBO);
        pass.drawIndexed (sphereIndexCount);

        pass.setPipeline (scenePipeline);
        pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
        pass.setTexture (0, 1, irradianceCube);
        pass.setTexture (0, 2, prefilterCube);
        pass.setTexture (0, 3, brdfLut);
        pass.setTexture (0, 4, albedoTexture);
        pass.setTexture (0, 5, normalTexture);
        pass.setTexture (0, 7, rmaTexture);
        pass.setSampler (0, 6, trilinearSampler);
        pass.setSampler (0, 8, materialSampler);
        pass.setVertexBuffer (0, sphereVBO);
        pass.setVertexBuffer (1, sphereInstanceVBO);
        pass.setIndexBuffer (yup::GpuIndexFormat::uint16, sphereIBO);
        pass.drawIndexed (sphereIndexCount, kGridColumns * kGridRows);

        pass.finish();
    }

    //==============================================================================
    // ---- Shaders -------------------------------------------------------------

    /** Fullscreen triangle from the vertex index, with a UV for the bake passes. */
    static constexpr char kFullscreenVert[] = R"glsl(#version 450
layout(location = 0) out vec2 v_uv;
void main() {
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    v_uv = vec2(x, y) * 0.5 + 0.5;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)glsl";

    /** Shared prelude for the cube bakes: the Bake block, the face parameterisation
        and the analytic sky. Concatenated in front of each bake fragment body. */
    static constexpr char kFaceHelpers[] = R"glsl(#version 450
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;
layout(set = 0, binding = 0) uniform Bake {
    int   face;
    float roughness;
    float flipY;
    float envSize;
    float sunIntensity;
} u;

const float PI = 3.14159265359;

// Maps a face index plus a [0,1] texel coordinate onto the direction the cube
// map hardware associates with that texel, matching the GL/Metal/D3D layout.
vec3 faceDirection(int face, vec2 uv) {
    vec2 c = uv * 2.0 - 1.0;
    if (face == 0) return normalize(vec3( 1.0, -c.y, -c.x));
    if (face == 1) return normalize(vec3(-1.0, -c.y,  c.x));
    if (face == 2) return normalize(vec3( c.x,  1.0,  c.y));
    if (face == 3) return normalize(vec3( c.x, -1.0, -c.y));
    if (face == 4) return normalize(vec3( c.x, -c.y,  1.0));
    return normalize(vec3(-c.x, -c.y, -1.0));
}

vec2 faceUV() {
    return vec2(v_uv.x, u.flipY > 0.5 ? 1.0 - v_uv.y : v_uv.y);
}

vec3 proceduralSky(vec3 d) {
    vec3 sunDir = normalize(vec3(0.35, 0.28, -0.90));
    float up = clamp(d.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 sky = mix(vec3(0.58, 0.68, 0.84), vec3(0.10, 0.22, 0.55), pow(up, 0.6));
    float horizon = 1.0 - smoothstep(-0.15, 0.45, d.y); // warm haze band near the horizon
    sky += vec3(0.24, 0.13, 0.05) * horizon * 0.55;
    vec3 ground = vec3(0.16, 0.14, 0.12);
    vec3 col = mix(ground, sky, smoothstep(-0.06, 0.10, d.y));
    float cosSun = clamp(dot(d, sunDir), 0.0, 1.0);
    // The sun disk and its halo scale with u.sunIntensity so the slider
    // re-bakes the whole environment (and its IBL derivatives) coherently.
    col += vec3(1.7, 1.45, 1.15) * u.sunIntensity * pow(cosSun, 600.0);
    col += vec3(0.16, 0.12, 0.07) * u.sunIntensity * pow(cosSun, 10.0);
    return col;
}

// Hammersley low-discrepancy sequence, used by the GGX importance sampler.
float radicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint n) {
    return vec2(float(i) / float(n), radicalInverse(i));
}

vec3 importanceSampleGGX(vec2 xi, vec3 n, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 h = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);
    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}
)glsl";

    static constexpr char kSkyFrag[] = R"glsl(
void main() {
    fragColor = vec4(proceduralSky(faceDirection(u.face, faceUV())), 1.0);
}
)glsl";

    static constexpr char kIrradianceFrag[] = R"glsl(
layout(set = 0, binding = 1) uniform textureCube u_env;
layout(set = 0, binding = 2) uniform sampler     u_samp;

void main() {
    vec3 n = faceDirection(u.face, faceUV());

    vec3 up = abs(n.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, n));
    up = normalize(cross(n, right));

    vec3 irradiance = vec3(0.0);
    float samples = 0.0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += 0.15) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += 0.05) {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 direction = tangentSample.x * right + tangentSample.y * up + tangentSample.z * n;
            irradiance += textureLod(samplerCube(u_env, u_samp), direction, 0.0).rgb * cos(theta) * sin(theta);
            samples += 1.0;
        }
    }

    fragColor = vec4(PI * irradiance / max(samples, 1.0), 1.0);
}
)glsl";

    static constexpr char kPrefilterFrag[] = R"glsl(
layout(set = 0, binding = 1) uniform textureCube u_env;
layout(set = 0, binding = 2) uniform sampler     u_samp;

const uint kSampleCount = 256u;

void main() {
    vec3 n = faceDirection(u.face, faceUV());
    vec3 v = n;

    vec3 prefiltered = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < kSampleCount; ++i) {
        vec3 h = importanceSampleGGX(hammersley(i, kSampleCount), n, u.roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);

        float nDotL = dot(n, l);
        if (nDotL > 0.0) {
            prefiltered += textureLod(samplerCube(u_env, u_samp), l, 0.0).rgb * nDotL;
            totalWeight += nDotL;
        }
    }

    fragColor = vec4(prefiltered / max(totalWeight, 0.001), 1.0);
}
)glsl";

    static constexpr char kBrdfFrag[] = R"glsl(#version 450
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;
layout(set = 0, binding = 0) uniform Bake {
    int   face;
    float roughness;
    float flipY;
    float envSize;
    float sunIntensity;
} u;

const float PI = 3.14159265359;
const uint kSampleCount = 512u;

float radicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec3 importanceSampleGGX(vec2 xi, vec3 n, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 h = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, n));
    vec3 bitangent = cross(n, tangent);
    return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

// Smith geometry term with the IBL (rather than direct-lighting) k.
float geometrySmith(float nDotV, float nDotL, float roughness) {
    float k = (roughness * roughness) * 0.5;
    float ggxV = nDotV / (nDotV * (1.0 - k) + k);
    float ggxL = nDotL / (nDotL * (1.0 - k) + k);
    return ggxV * ggxL;
}

void main() {
    vec2 uv = vec2(v_uv.x, u.flipY > 0.5 ? 1.0 - v_uv.y : v_uv.y);

    float nDotV = max(uv.x, 0.001);
    float roughness = max(uv.y, 0.001);

    vec3 v = vec3(sqrt(1.0 - nDotV * nDotV), 0.0, nDotV);
    vec3 n = vec3(0.0, 0.0, 1.0);

    float scale = 0.0;
    float bias = 0.0;

    for (uint i = 0u; i < kSampleCount; ++i) {
        vec2 xi = vec2(float(i) / float(kSampleCount), radicalInverse(i));
        vec3 h = importanceSampleGGX(xi, n, roughness);
        vec3 l = normalize(2.0 * dot(v, h) * h - v);

        float nDotL = max(l.z, 0.0);
        float nDotH = max(h.z, 0.0);
        float vDotH = max(dot(v, h), 0.0);

        if (nDotL > 0.0) {
            float g = geometrySmith(nDotV, nDotL, roughness);
            float gVis = (g * vDotH) / max(nDotH * nDotV, 0.001);
            float fc = pow(1.0 - vDotH, 5.0);

            scale += (1.0 - fc) * gVis;
            bias += fc * gVis;
        }
    }

    fragColor = vec4(scale / float(kSampleCount), bias / float(kSampleCount), 0.0, 1.0);
}
)glsl";

    /** Shared vertex stage for the sky backdrop and the sphere grid. A non-zero
        backdropRadius in the Scene block wraps the same unit-sphere mesh around
        the camera; zero positions the instanced sphere grid.

        The projection maps z/w into [0, 1], which every backend treats as a
        monotonically increasing depth, so a single shader drives the depth test
        correctly on all of them. */
    static constexpr char kSceneVert[] = R"glsl(#version 450
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

// Per-instance material, fed from a per-instance vertex buffer (locations 3-5).
layout(location = 3) in vec4 a_material; // metallic, roughness, uvScale, normalStrength
layout(location = 4) in vec4 a_tint;     // linear albedo / metal f0 rgb, roughness-map amount
layout(location = 5) in vec4 a_extra;    // clearcoat, clearcoat roughness, ao scale, unused

layout(set = 0, binding = 0) uniform Scene {
    vec4 camera;   // yaw, pitch, distance, aspect
    vec4 material; // exposure, sunIntensity, prefilterMaxLod, backdropRadius (0 = sphere grid)
    vec4 light;    // sun direction xyz
} u;

layout(location = 0) out vec3 v_worldPosition;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec3 v_cameraPosition;
layout(location = 4) out vec4 v_material;
layout(location = 5) out vec4 v_tint;
layout(location = 6) out vec4 v_extra;

const int kColumns = 5;
const int kRows = 3;
const float kSpacing = 2.6;

void main() {
    float yaw = u.camera.x;
    float pitch = u.camera.y;
    float orbitRadius = u.camera.z;
    float aspect = u.camera.w;

    vec3 cameraPosition = vec3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch)) * orbitRadius;

    // A non-zero backdropRadius wraps the same unit-sphere mesh around the
    // camera as the sky backdrop; zero positions the instanced sphere grid.
    // gl_InstanceIndex is always 0 for the backdrop, which is drawn with a
    // single instance.
    float backdropRadius = u.material.w;
    float isBackdrop = step(0.001, backdropRadius);

    int instance = gl_InstanceIndex;
    int column = instance % kColumns;
    int row = instance / kColumns;

    vec2 offset = (vec2(float(column), float(row)) - vec2(float(kColumns - 1), float(kRows - 1)) * 0.5) * kSpacing;
    vec3 gridPosition = a_position + vec3(offset, 0.0);
    vec3 backdropPosition = cameraPosition + a_position * backdropRadius;
    vec3 worldPosition = mix(gridPosition, backdropPosition, isBackdrop);

    vec3 forward = normalize(-cameraPosition);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);

    vec3 toVertex = worldPosition - cameraPosition;
    vec3 viewSpace = vec3(dot(toVertex, right), dot(toVertex, up), dot(toVertex, forward));

    // z/w lands in [0, 1], which every backend treats as monotonically increasing
    // depth. Keep the near plane as far out as the scene allows: a near plane of
    // 0.1 against a far plane of 120 would squeeze the whole grid into a fraction
    // of a percent of the depth range and z-fight.
    float fov = 1.7320508;  // cot(30 degrees) => 60 degree vertical field of view
    float zNear = 1.0;
    float zFar = 120.0;

    gl_Position = vec4(viewSpace.x * fov / aspect,
                       viewSpace.y * fov,
                       zFar * (viewSpace.z - zNear) / (zFar - zNear),
                       viewSpace.z);

    v_worldPosition = worldPosition;
    v_normal = a_normal;
    v_uv = a_uv;
    v_cameraPosition = cameraPosition;

    v_material = a_material;
    v_tint = a_tint;
    v_extra = a_extra;
}
)glsl";

    static constexpr char kBackgroundFrag[] = R"glsl(#version 450
layout(location = 0) in vec3 v_worldPosition;
layout(location = 3) in vec3 v_cameraPosition;

layout(set = 0, binding = 0) uniform Scene {
    vec4 camera;
    vec4 material;
    vec4 light;
} u;

layout(set = 0, binding = 1) uniform textureCube u_environment;
layout(set = 0, binding = 2) uniform sampler     u_samp;

layout(location = 0) out vec4 fragColor;

vec3 acesTonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec3 direction = normalize(v_worldPosition - v_cameraPosition);
    vec3 color = textureLod(samplerCube(u_environment, u_samp), direction, 0.0).rgb;

    color = acesTonemap(color * u.material.x);
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
)glsl";

    static constexpr char kSceneFrag[] = R"glsl(#version 450
layout(location = 0) in vec3 v_worldPosition;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec3 v_cameraPosition;
layout(location = 4) in vec4 v_material;
layout(location = 5) in vec4 v_tint;
layout(location = 6) in vec4 v_extra;

layout(set = 0, binding = 0) uniform Scene {
    vec4 camera;   // yaw, pitch, distance, aspect
    vec4 material; // exposure, sunIntensity, prefilterMaxLod, backdropRadius (0 = sphere grid)
    vec4 light;    // sun direction xyz
} u;

layout(set = 0, binding = 1) uniform textureCube u_irradiance;
layout(set = 0, binding = 2) uniform textureCube u_prefilter;
layout(set = 0, binding = 3) uniform texture2D   u_brdf;
layout(set = 0, binding = 4) uniform texture2D   u_albedo;
layout(set = 0, binding = 5) uniform texture2D   u_normalMap;
layout(set = 0, binding = 6) uniform sampler     u_samp;
layout(set = 0, binding = 7) uniform texture2D   u_rma;
layout(set = 0, binding = 8) uniform sampler     u_sampRepeat;

layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

float distributionGGX(float nDotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float d = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 0.0001);
}

float geometrySmithDirect(float nDotV, float nDotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float ggxV = nDotV / (nDotV * (1.0 - k) + k);
    float ggxL = nDotL / (nDotL * (1.0 - k) + k);
    return ggxV * ggxL;
}

vec3 acesTonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    float metallic = clamp(v_material.x, 0.0, 1.0);
    float baseRoughness = clamp(v_material.y, 0.06, 1.0);
    float mapAmount = v_tint.a;
    float aoScale = v_extra.z;

    // Analytic tangent frame for a unit sphere, perturbed by the uploaded
    // normal map.
    vec3 geometricNormal = normalize(v_normal);

    // cross(Y, N) collapses to zero at the poles, and normalizing that yields NaN
    // shading normals in a band around them - pick a different reference axis there.
    vec3 reference = abs(geometricNormal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(reference, geometricNormal));
    vec3 bitangent = cross(geometricNormal, tangent);

    vec2 uv = v_uv * max(v_material.z, 0.25);
    vec3 tangentNormal = texture(sampler2D(u_normalMap, u_sampRepeat), uv).xyz * 2.0 - 1.0;
    tangentNormal.xy *= v_material.w;

    vec3 n = normalize(mat3(tangent, bitangent, geometricNormal) * tangentNormal);
    vec3 v = normalize(v_cameraPosition - v_worldPosition);
    vec3 r = reflect(-v, n);

    vec3 albedoTex = pow(texture(sampler2D(u_albedo, u_sampRepeat), uv).rgb, vec3(2.2));
    vec3 albedo = albedoTex * v_tint.rgb;

    vec3 rma = texture(sampler2D(u_rma, u_sampRepeat), uv).rgb;
    float roughness = clamp(mix(baseRoughness, baseRoughness * (rma.g / 0.5), mapAmount), 0.06, 1.0);
    float ao = mix(1.0, rma.b, aoScale);

    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    float nDotV = max(dot(n, v), 0.0001);

    // Direct sun contribution.
    vec3 l = normalize(u.light.xyz);
    vec3 h = normalize(v + l);
    float nDotL = max(dot(n, l), 0.0);
    float nDotH = max(dot(n, h), 0.0);
    float vDotH = max(dot(v, h), 0.0);

    vec3 fresnel = f0 + (vec3(1.0) - f0) * pow(1.0 - vDotH, 5.0);
    float ndf = distributionGGX(nDotH, roughness);
    float geometry = geometrySmithDirect(nDotV, nDotL, roughness);

    vec3 specularDirect = (ndf * geometry * fresnel) / max(4.0 * nDotV * nDotL, 0.0001);
    vec3 diffuseDirect = (vec3(1.0) - fresnel) * (1.0 - metallic) * albedo / PI;
    vec3 direct = (diffuseDirect + specularDirect) * nDotL * u.material.y;

    // Image-based ambient: diffuse irradiance plus the split-sum specular term.
    vec3 fresnelIbl = f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - nDotV, 5.0);
    vec3 kD = (vec3(1.0) - fresnelIbl) * (1.0 - metallic);

    vec3 irradiance = textureLod(samplerCube(u_irradiance, u_samp), n, 0.0).rgb;
    vec3 diffuseIbl = irradiance * albedo;

    // The prefilter chain is read by explicit LOD: a mip-narrowed texture view
    // would silently degrade to mip 0 on OpenGL.
    vec3 prefiltered = textureLod(samplerCube(u_prefilter, u_samp), r, roughness * u.material.z).rgb;
    vec2 brdf = texture(sampler2D(u_brdf, u_samp), vec2(nDotV, roughness)).rg;
    vec3 specularIbl = prefiltered * (fresnelIbl * brdf.x + brdf.y);

    // Clearcoat: a glossy dielectric layer whose own, much smoother roughness
    // lobe replaces the base specular reflection as the coat builds up.
    float clearcoat = v_extra.x;
    if (clearcoat > 0.001)
    {
        float ccRoughness = clamp(v_extra.y, 0.03, 0.5);
        float fcc = 0.04 + 0.96 * pow(1.0 - nDotV, 5.0);
        vec2 ccBrdf = texture(sampler2D(u_brdf, u_samp), vec2(nDotV, ccRoughness)).rg;
        vec3 ccPrefiltered = textureLod(samplerCube(u_prefilter, u_samp), r, ccRoughness * u.material.z).rgb;
        vec3 ccSpecular = ccPrefiltered * (fcc * ccBrdf.x + ccBrdf.y);
        specularIbl = mix(specularIbl, ccSpecular, clearcoat);
    }

    vec3 color = ao * (kD * diffuseIbl + specularIbl) + direct;

    color = acesTonemap(color * u.material.x);
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
)glsl";

    //==============================================================================
    yup::GraphicsContext* capturedContext = nullptr;
    bool pipelinesReady = false;
    int bakeStagesDone = 0;

    yup::GpuTextureFormat hdrFormat = yup::GpuTextureFormat::rgba16float;
    yup::GpuTextureFormat brdfFormat = yup::GpuTextureFormat::rg16float;

    yup::GpuPipeline::Ptr skyPipeline;
    yup::GpuPipeline::Ptr irradiancePipeline;
    yup::GpuPipeline::Ptr prefilterPipeline;
    yup::GpuPipeline::Ptr brdfPipeline;
    yup::GpuPipeline::Ptr backgroundPipeline;
    yup::GpuPipeline::Ptr scenePipeline;

    yup::GpuTexture::Ptr environmentCube;
    yup::GpuTexture::Ptr irradianceCube;
    yup::GpuTexture::Ptr prefilterCube;
    yup::GpuTexture::Ptr brdfLut;
    yup::GpuTexture::Ptr albedoTexture;
    yup::GpuTexture::Ptr normalTexture;
    yup::GpuTexture::Ptr rmaTexture;
    yup::GpuTexture::Ptr depthTexture;
    yup::GpuSampler::Ptr trilinearSampler;
    yup::GpuSampler::Ptr materialSampler;

    yup::GpuTarget::Ptr sceneTarget;

    yup::GpuBuffer::Ptr sphereVBO;
    yup::GpuBuffer::Ptr sphereIBO;
    uint32_t sphereIndexCount = 0;

    yup::GpuBuffer::Ptr sphereInstanceVBO;

    float cameraYaw = 0.6f;
    float cameraPitch = 0.15f;
    float cameraDistance = 13.0f;
    float exposure = 1.1f;
    float sunIntensity = 4.0f;

    bool isDragging = false;
    yup::Point<float> lastDragPosition;

    std::unique_ptr<yup::Slider> exposureSlider;
    std::unique_ptr<yup::Slider> sunSlider;
    std::unique_ptr<yup::Label> statusLabel;
};
