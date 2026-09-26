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

//==============================================================================

/**
    Demonstrates GPU compute-shader particle simulation rendered via GpuPipeline.

    A compute shader simulates 8192 particles on the GPU: particles explode
    outward from the center, fall under gravity, bounce off the ground and
    walls, and respawn when their lifetime expires. Each frame the particle
    positions and colors are read back from the compute SSBO, assembled into
    quad geometry on the CPU, and drawn with a soft-circle fragment shader
    using additive blending.

    Requirements:
    - A GpuDevice with compute shader support (Metal, D3D11, WebGPU, GL 4.3+)
    - The particles_update and particles_draw shader bundles, precompiled from data/shaders

    @see GpuComputePipeline, GpuComputePass, GpuPipeline, GpuRenderPass
*/
class ComputeParticlesDemo : public yup::Component
{
public:
    //==============================================================================
    ComputeParticlesDemo()
        : yup::Component ("ComputeParticlesDemo")
    {
        statusLabel = std::make_unique<yup::Label> ("statusLabel");
        statusLabel->setText ("Initializing GPU compute...", yup::dontSendNotification);
        addAndMakeVisible (statusLabel.get());

        gravitySlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        gravitySlider->setRange (0.5, 8.0);
        gravitySlider->setValue (3.5);
        gravitySlider->onValueChanged = [this] (double v)
        {
            simGravity = (float) v;
            gravityLabel->setText ("Gravity: " + yup::String (simGravity, 2), yup::dontSendNotification);
        };
        addAndMakeVisible (gravitySlider.get());

        gravityLabel = std::make_unique<yup::Label> ("gravityLabel");
        gravityLabel->setText ("Gravity: 3.50", yup::dontSendNotification);
        addAndMakeVisible (gravityLabel.get());
    }

    ~ComputeParticlesDemo() override
    {
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

        if (! gpuReady)
            return;

        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        auto particleBounds = bounds;
        particleBounds.removeFromBottom (60.0f);

        const int w = yup::roundToInt (particleBounds.getWidth());
        const int h = yup::roundToInt (particleBounds.getHeight());

        if (w < 2 || h < 2)
            return;

        yup::GpuTexture::Ptr outputTex = simulateAndRender (w, h);

        if (outputTex != nullptr)
            g.drawTexture (outputTex, particleBounds);
    }

    //==============================================================================
    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);

        statusLabel->setBounds (bounds.removeFromBottom (25.0f));

        auto sliderBounds = bounds.removeFromBottom (30.0f);
        gravityLabel->setBounds (sliderBounds.removeFromLeft (80.0f));
        gravitySlider->setBounds (sliderBounds);
    }

    //==============================================================================
    void visibilityChanged() override
    {
        if (isVisible())
        {
            capturedContext = nullptr;
            computePipeline = nullptr;
            renderPipeline = nullptr;
            renderTarget = nullptr;
            particleSSBO = nullptr;
            gpuReady = false;
        }
    }

    void refreshDisplay (double /*lastFrameTimeSeconds*/) override
    {
        if (gpuReady)
            repaint();
    }

private:
    //==============================================================================
    /** One particle as laid out in the GPU SSBO (std430).

        Layout (48 bytes total, 16-byte aligned):
          offset  0: vec2 position  (8 bytes)
          offset  8: vec2 velocity  (8 bytes)
          offset 16: vec4 color     (16 bytes)
          offset 32: float lifetime (4 bytes)
          offset 36: float age      (4 bytes)
          -- 8 bytes implicit tail padding to align struct size to 16 --
    */
    static constexpr int kParticleGpuStrideFloats = 12; // 48 bytes / 4

    /** One vertex for the render pipeline (quad corner).

        Layout (40 bytes):
          offset  0: vec2 center   (location 0, float2)
          offset  8: vec2 offset   (location 1, float2)
          offset 16: vec4 color    (location 2, float4)
          offset 32: vec2 size     (location 3, float2)
    */
    static constexpr int kVertexStrideFloats = 10; // 40 bytes / 4
    static constexpr int kVerticesPerParticle = 6; // 2 triangles

    static constexpr int kParticleCount = 8192;
    static constexpr int kWorkgroupSize = 256;
    static constexpr int kWorkgroupCount = kParticleCount / kWorkgroupSize; // 32

    // Offsets for extracting particle data from the raw GPU readback array.
    static constexpr int kGpuPosX = 0;
    static constexpr int kGpuPosY = 1;
    static constexpr int kGpuColR = 4;
    static constexpr int kGpuColG = 5;
    static constexpr int kGpuColB = 6;
    static constexpr int kGpuColA = 7;

    // Quad corner offsets centred at the origin (in particle-local space).
    static constexpr float kQuadOffsets[kVerticesPerParticle * 2] = {
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        -0.5f,
        0.5f,
        0.5f,
        -0.5f,
        0.5f
    };

    //==============================================================================
    void initGpu()
    {
        if (capturedContext == nullptr)
            return;

        auto device = capturedContext->getGpuDevice();

        if (! device->isComputeAvailable())
        {
            statusLabel->setText ("Compute shaders not available on this GPU backend.", yup::dontSendNotification);
            YUP_DBG ("Compute shaders not available on this GPU backend.");
            return;
        }

        auto computeBundle = loadShaderBundle ("particles_update");
        if (computeBundle.failed())
        {
            statusLabel->setText ("Compute shader load failed: " + computeBundle.getErrorMessage().substring (0, 60),
                                  yup::dontSendNotification);
            YUP_DBG ("Compute shader load failed: " << computeBundle.getErrorMessage());
            return;
        }

        yup::GpuWorkgroupSize wgs { (uint32_t) kWorkgroupSize, 1, 1 };
        auto computeResult = yup::GpuComputePipeline::compileFromBundle (device, computeBundle.getReference(), wgs);

        if (computeResult.failed())
        {
            statusLabel->setText ("Compute shader compile failed: " + computeResult.getErrorMessage().substring (0, 60),
                                  yup::dontSendNotification);
            YUP_DBG ("Compute shader compile failed: " << computeResult.getErrorMessage());
            return;
        }

        computePipeline = computeResult.getValue();

        // Compile the render pipeline.
        yup::GpuPipelineOptions pipelineOpts;

        // Vertex buffer layout: 4 attributes, 40-byte stride.
        pipelineOpts.vertexBuffers.emplace_back (
            kVertexStrideFloats * (uint32_t) sizeof (float),
            yup::GpuVertexStepMode::vertex,
            std::vector<yup::GpuVertexAttribute> {
                { yup::GpuVertexFormat::float2, 0, 0 },  // center
                { yup::GpuVertexFormat::float2, 8, 1 },  // offset
                { yup::GpuVertexFormat::float4, 16, 2 }, // color
                { yup::GpuVertexFormat::float2, 32, 3 }, // size (x,y)
            });

        pipelineOpts.topology = yup::GpuPrimitiveTopology::triangleList;
        pipelineOpts.cullMode = yup::GpuCullMode::none;
        pipelineOpts.colorTargets.emplace_back().blendEnabled = false;

        auto renderResult = compilePipelineFromBundle (device, "particles_draw", pipelineOpts);
        if (renderResult.failed())
        {
            statusLabel->setText ("Render shader compile failed: " + renderResult.getErrorMessage().substring (0, 60),
                                  yup::dontSendNotification);
            YUP_DBG ("Render shader compile failed: " << renderResult.getErrorMessage());
            return;
        }

        renderPipeline = renderResult.getValue();

        // Allocate CPU-side readback and vertex buffers.
        const size_t readbackSize = (size_t) kParticleCount * (size_t) kParticleGpuStrideFloats;
        cpuParticleData.resize (readbackSize, 0.0f);

        // Pre-seed particle data.
        yup::Random rng;
        for (int i = 0; i < kParticleCount; ++i)
        {
            const size_t base = (size_t) i * (size_t) kParticleGpuStrideFloats;
            // Position: random within sim space, biased toward centre.
            cpuParticleData[base + kGpuPosX] = rng.nextFloat() * 2.0f - 1.0f;
            cpuParticleData[base + kGpuPosY] = rng.nextFloat() * 1.5f - 0.5f;
            // Velocity: zero (compute shader will set this on respawn).
            cpuParticleData[base + 2] = 0.0f;
            cpuParticleData[base + 3] = 0.0f;
            // Color: bright, fully opaque.
            cpuParticleData[base + kGpuColR] = rng.nextFloat();
            cpuParticleData[base + kGpuColG] = rng.nextFloat();
            cpuParticleData[base + kGpuColB] = rng.nextFloat();
            cpuParticleData[base + kGpuColA] = 1.0f;
            // Lifetime = 0, age = 0 → triggers immediate respawn in compute.
            cpuParticleData[base + 8] = 0.0f;
            cpuParticleData[base + 9] = 0.0f;
        }

        // Create a persistent SSBO with initial particle data.
        const size_t readbackBytes = cpuParticleData.size() * sizeof (float);
        particleSSBO = device->createBuffer (yup::GpuBufferType::storage,
                                             cpuParticleData.data(),
                                             readbackBytes);

        // Pre-allocate vertex buffer at max capacity.
        const size_t vertexDataSize = (size_t) kParticleCount * (size_t) kVerticesPerParticle * (size_t) kVertexStrideFloats;
        cpuVertexData.resize (vertexDataSize, 0.0f);

        const size_t vertexBytes = vertexDataSize * sizeof (float);
        particleVBO = yup::GpuBuffer::create (device, yup::GpuBufferType::vertex, cpuVertexData.data(), vertexBytes);

        statusLabel->setText (yup::String::formatted ("GPU compute particles | %d particles | %d workgroups",
                                                      kParticleCount,
                                                      kWorkgroupCount),
                              yup::dontSendNotification);

        lastFrameStamp = yup::Time::getHighResolutionTicks();
        frameCount = 0;
        snapshotCount = 0;
        fpsUpdateAccum = 0.0;
        gpuReady = true;
    }

    //==============================================================================
    /** Runs one frame of compute + render and returns the rendered texture. */
    yup::GpuTexture::Ptr simulateAndRender (int viewW, int viewH)
    {
        if (! gpuReady || computePipeline == nullptr || renderPipeline == nullptr)
            return nullptr;

        auto device = capturedContext->getGpuDevice();

        // ---- Compute pass: simulate particles ---------------------------------
        const auto now = yup::Time::getHighResolutionTicks();
        const float deltaTime = yup::Time::highResolutionTicksToSeconds (now - lastFrameStamp);
        lastFrameStamp = now;

        const float clampedDt = yup::jmin (deltaTime, 0.1f);

        if (particleSSBO == nullptr)
            return nullptr;

        const size_t readbackBytes = cpuParticleData.size() * sizeof (float);

        struct alignas (16) ComputeParams
        {
            float deltaTime;
            float gravity;
            float restitution;
            float particleCountF;
            float simLeft;
            float simRight;
            float simBottom;
            float simTop;
        };

        // Compute sim boundaries from the viewport aspect ratio so the
        // simulation fills the full area with uniform scale (circular particles).
        const float viewAspect = (float) viewW / (float) yup::jmax ((float) viewH, 1.0f);
        constexpr float baseSimHeight = 2.5f; // fixed vertical range
        const float simWidth = baseSimHeight * viewAspect;
        const float simLeft = -simWidth * 0.5f;
        const float simRight = simWidth * 0.5f;
        constexpr float simBottom = -1.0f;
        constexpr float simTop = 1.5f;

        ComputeParams cparams { clampedDt, simGravity, 0.45f, (float) kParticleCount, simLeft, simRight, simBottom, simTop };

        {
            auto pass = yup::GpuComputePass::begin (device);
            if (pass.isValid())
            {
                pass.setPipeline (computePipeline);
                pass.setStorageBuffer (0, 1, particleSSBO);
                pass.setUniformBuffer (0, 0, &cparams, sizeof (cparams));
                pass.dispatch ((uint32_t) kWorkgroupCount, 1, 1);
                pass.finish();
            }
        }

        // Pull the latest particle snapshot. On backends that cannot map a buffer
        // synchronously (WebGPU) this is pipelined, so it trails the GPU by a frame
        // or two and returns false on frames where nothing new landed.
        // cpuParticleData then still holds the previous snapshot, so keep drawing
        // it instead of dropping the frame.
        if (device->readBuffer (particleSSBO, cpuParticleData.data(), readbackBytes))
            ++snapshotCount;

        // ---- Build vertex buffer from particle data ---------------------------
        const float sizeY = 20.0f / (float) yup::jmax (viewH, 1);
        const float sizeX = sizeY * (float) viewH / (float) yup::jmax (viewW, 1);

        // Map sim space to clip space.
        const float simMidX = (simLeft + simRight) * 0.5f;
        const float simMidY = (simBottom + simTop) * 0.5f;
        const float scaleX = 2.0f / (simRight - simLeft);
        const float scaleY = 2.0f / (simTop - simBottom);

        const int totalVertices = kParticleCount * kVerticesPerParticle;
        float* vtx = cpuVertexData.data();

        for (int p = 0; p < kParticleCount; ++p)
        {
            const int base = p * kParticleGpuStrideFloats;
            const float px = cpuParticleData[(size_t) base + kGpuPosX];
            const float py = cpuParticleData[(size_t) base + kGpuPosY];

            const float cx = (px - simMidX) * scaleX;
            const float cy = (py - simMidY) * scaleY;

            const float cr = cpuParticleData[(size_t) base + kGpuColR];
            const float cg = cpuParticleData[(size_t) base + kGpuColG];
            const float cb = cpuParticleData[(size_t) base + kGpuColB];
            const float ca = cpuParticleData[(size_t) base + kGpuColA];

            for (int v = 0; v < kVerticesPerParticle; ++v)
            {
                *vtx++ = cx;
                *vtx++ = cy;
                *vtx++ = kQuadOffsets[v * 2 + 0];
                *vtx++ = kQuadOffsets[v * 2 + 1];
                *vtx++ = cr;
                *vtx++ = cg;
                *vtx++ = cb;
                *vtx++ = ca;
                *vtx++ = sizeX;
                *vtx++ = sizeY;
            }
        }

        const size_t vertexBytes = (size_t) totalVertices * kVertexStrideFloats * sizeof (float);

        // ---- Render pass ------------------------------------------------------
        if (! device->updateBuffer (particleVBO, cpuVertexData.data(), vertexBytes))
        {
            statusLabel->setText ("VBO update failed!", yup::dontSendNotification);
            YUP_DBG ("VBO update failed!");
            return nullptr;
        }

        // Create or resize render target.
        if (renderTarget == nullptr || renderTarget->getWidth() != viewW || renderTarget->getHeight() != viewH)
            renderTarget = yup::GpuTarget::create (device, viewW, viewH);

        if (renderTarget == nullptr)
        {
            statusLabel->setText ("Render target creation failed!", yup::dontSendNotification);
            YUP_DBG ("Render target creation failed!");
            return nullptr;
        }

        {
            auto frame = yup::GpuFrame::begin (device);

            auto pass = renderTarget->beginRenderPass (frame, { true, yup::GpuColor::transparentBlack() });
            pass.setPipeline (renderPipeline);
            pass.setVertexBuffer (0, particleVBO);
            pass.draw ((uint32_t) totalVertices);
            pass.finish();

            frame.submit();
        }

        frameCount++;
        fpsUpdateAccum += (double) clampedDt;

        if (fpsUpdateAccum >= 0.25)
        {
            statusLabel->setText (yup::String::formatted ("GPU compute | %d particles | f=%d | s=%d | p0=(%.2f,%.2f) | g=%.2f",
                                                          kParticleCount,
                                                          frameCount,
                                                          snapshotCount,
                                                          (double) cpuParticleData[(size_t) kGpuPosX],
                                                          (double) cpuParticleData[(size_t) kGpuPosY],
                                                          (double) simGravity),
                                  yup::dontSendNotification);
            fpsUpdateAccum = 0.0;
        }

        return renderTarget->asTexture();
    }

    //==============================================================================
    yup::GraphicsContext* capturedContext = nullptr;

    // GPU resources.
    yup::GpuComputePipeline::Ptr computePipeline;
    yup::GpuPipeline::Ptr renderPipeline;
    yup::GpuBuffer::Ptr particleSSBO;
    yup::GpuBuffer::Ptr particleVBO;
    yup::GpuTarget::Ptr renderTarget;

    // CPU-side data.
    std::vector<float> cpuParticleData;
    std::vector<float> cpuVertexData;

    // Simulation state.
    float simGravity = 3.5f;
    yup::int64 lastFrameStamp = 0;
    bool gpuReady = false;

    // FPS counter. snapshotCount tracks how many readbacks actually landed, which
    // on async-readback backends is lower than frameCount.
    int frameCount = 0;
    int snapshotCount = 0;
    double fpsUpdateAccum = 0.0;

    // UI.
    std::unique_ptr<yup::Slider> gravitySlider;
    std::unique_ptr<yup::Label> gravityLabel;
    std::unique_ptr<yup::Label> statusLabel;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComputeParticlesDemo)
};
