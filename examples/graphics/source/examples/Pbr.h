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

    bool compilePipelines (yup::GpuDevice::Ptr device)
    {
        auto compile = [&] (const char* name, yup::StringRef shaderName, const yup::GpuPipelineOptions& options) -> yup::GpuPipeline::Ptr
        {
            auto result = compilePipelineFromBundle (device, shaderName, options);

            if (result.failed())
            {
                statusLabel->setText (yup::String (name) + " shader failed: " + result.getErrorMessage(),
                                      yup::dontSendNotification);
                return nullptr;
            }

            return result.getReference();
        };

        skyPipeline = compile ("Sky bake", "pbr_sky", bakePipelineOptions (hdrFormat));
        irradiancePipeline = compile ("Irradiance", "pbr_irradiance", bakePipelineOptions (hdrFormat));
        prefilterPipeline = compile ("Prefilter", "pbr_prefilter", bakePipelineOptions (hdrFormat));
        brdfPipeline = compile ("BRDF LUT", "pbr_brdf", bakePipelineOptions (brdfFormat));

        backgroundPipeline = compile ("Background", "pbr_background", backgroundPipelineOptions());
        scenePipeline = compile ("Scene", "pbr_scene", scenePipelineOptions());

        return skyPipeline != nullptr
            && irradiancePipeline != nullptr
            && prefilterPipeline != nullptr
            && brdfPipeline != nullptr
            && backgroundPipeline != nullptr
            && scenePipeline != nullptr;
    }

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
