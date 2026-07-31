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
    - YUP_ENABLE_SHADER_TRANSPILER for online GLSL → native compilation

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
    /** GLSL 450 compute shader: particle physics simulation.

        Particle data is stored as a flat float array in an SSBO to avoid
        struct-based layouts that can confuse the Metal transpiler's
        binding reflection. Each particle occupies 12 floats (48 bytes):
          offset  0: posX, posY
          offset  2: velX, velY
          offset  4: colR, colG, colB, colA
          offset  8: lifetime
          offset  9: age
          offset 10-11: padding (unused)
    */
    /** GLSL 450 compute shader: particle physics simulation.

        Each particle occupies 12 floats (48 bytes in std430):
          offset  0: posX, posY
          offset  2: velX, velY
          offset  4: colR, colG, colB, colA
          offset  8: lifetime
          offset  9: age
          offset 10-11: padding (unused)

        Bindings: UBO at (0,0), SSBO at (0,1) — matching Metal's
        declaration-order buffer indexing.
    */
    static constexpr const char kComputeSource[] = R"glsl(#version 450
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std140, set = 0, binding = 0) uniform Params {
    float deltaTime;
    float gravity;
    float restitution;
    float particleCountF;
    float simLeft;
    float simRight;
    float simBottom;
    float simTop;
} params;

layout(std430, set = 0, binding = 1) buffer ParticleBuffer {
    float data[];
};

// Simple hash function for pseudo-random numbers per particle.
uint wangHash(uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return seed;
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    uint particleCount = uint(params.particleCountF);
    if (idx >= particleCount)
        return;

    uint base = idx * 12u;

    // Read particle state from the flat array.
    vec2 pos       = vec2(data[base + 0u], data[base + 1u]);
    vec2 vel       = vec2(data[base + 2u], data[base + 3u]);
    vec4 col       = vec4(data[base + 4u], data[base + 5u], data[base + 6u], data[base + 7u]);
    float lifetime = data[base + 8u];
    float age      = data[base + 9u];

    // Age the particle.
    age += params.deltaTime;

    // Respawn when lifetime expires — explode outward from the centre.
    if (age >= lifetime) {
        float angle = float(wangHash(idx * 2u + uint(age * 1000.0))) / float(0xFFFFFFFFu) * 6.283185307;
        float speed = float(wangHash(idx * 3u)) / float(0xFFFFFFFFu) * 1.8 + 0.2;

        pos = vec2(0.0, params.simBottom + 1.4);
        vel = vec2(cos(angle), sin(angle)) * speed;
        age = 0.0;
        lifetime = float(wangHash(idx * 5u)) / float(0xFFFFFFFFu) * 1.5 + 0.3;

        // Random saturated color.
        uint cr = wangHash(idx * 7u);
        uint cg = wangHash(idx * 11u);
        uint cb = wangHash(idx * 13u);
        col = vec4(
            float(cr & 0xFFu) / 255.0,
            float(cg & 0xFFu) / 255.0,
            float(cb & 0xFFu) / 255.0,
            1.0
        );
    }

    // Apply gravity.
    vel.y -= params.gravity * params.deltaTime;

    // Integrate position.
    pos += vel * params.deltaTime;

    // Bounce off ground.
    if (pos.y < params.simBottom) {
        pos.y = params.simBottom;
        vel.y = abs(vel.y) * params.restitution;
        vel.x *= 0.92;
    }

    // Bounce off side walls (viewport edges).
    if (pos.x < params.simLeft) {
        vel.x = abs(vel.x) * 0.7;
        pos.x = params.simLeft;
    }
    if (pos.x > params.simRight) {
        vel.x = -abs(vel.x) * 0.7;
        pos.x = params.simRight;
    }

    // Write back to the flat array.
    data[base + 0u] = pos.x;
    data[base + 1u] = pos.y;
    data[base + 2u] = vel.x;
    data[base + 3u] = vel.y;
    data[base + 4u] = col.r;
    data[base + 5u] = col.g;
    data[base + 6u] = col.b;
    data[base + 7u] = col.a;
    data[base + 8u] = lifetime;
    data[base + 9u] = age;
}
)glsl";

    //==============================================================================
    /** GLSL 450 vertex shader: expands each vertex into a clip-space quad corner. */
    static constexpr const char kRenderVertSource[] = R"glsl(#version 450
layout(location = 0) in vec2 aCenter;
layout(location = 1) in vec2 aOffset;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aSize;

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vOffset;

void main() {
    gl_Position = vec4(aCenter + aOffset * aSize, 0.0, 1.0);
    vColor = aColor;
    vOffset = aOffset;
}
)glsl";

    //==============================================================================
    /** GLSL 450 fragment shader: hard opaque circle, no blending.

        vOffset is the raw quad corner offset in [-0.5, 0.5].
        Multiply by 2 to normalise to [-1, 1] for a correct
        unit-circle distance test that works with any viewport aspect. */
    static constexpr const char kRenderFragSource[] = R"glsl(#version 450
layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vOffset;

layout(location = 0) out vec4 outColor;

void main() {
    // Raw offset is always [-0.5, 0.5] on both axes regardless of
    // viewport aspect compensation. Normalise to [-1, 1] for a true circle.
    float dist = length(vOffset * 2.0);
    if (dist > 1.0)
        discard;
    outColor = vColor;
}
)glsl";

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

        // Compile the compute pipeline from GLSL.
        yup::String glslSource = yup::String::fromUTF8 (kComputeSource, sizeof (kComputeSource) - 1);

#if YUP_ENABLE_SHADER_TRANSPILER
        yup::GpuWorkgroupSize wgs { (uint32_t) kWorkgroupSize, 1, 1 };
        auto computeResult = yup::GpuComputePipeline::compileFromGlsl (device, glslSource, wgs);

        if (computeResult.failed())
        {
            statusLabel->setText ("Compute shader compile failed: " + computeResult.getErrorMessage().substring (0, 60),
                                  yup::dontSendNotification);
            YUP_DBG ("Compute shader compile failed: " << computeResult.getErrorMessage());
            return;
        }

        computePipeline = computeResult.getValue();
#else
        statusLabel->setText ("Shader transpiler not available (YUP_ENABLE_SHADER_TRANSPILER).", yup::dontSendNotification);
        YUP_DBG ("Shader transpiler not available (YUP_ENABLE_SHADER_TRANSPILER).");
        return;
#endif

        // Compile the render pipeline.
        yup::String vertSource = yup::String::fromUTF8 (kRenderVertSource, sizeof (kRenderVertSource) - 1);
        yup::String fragSource = yup::String::fromUTF8 (kRenderFragSource, sizeof (kRenderFragSource) - 1);

        // Vertex buffer layout: 4 attributes, 40-byte stride.
        static const yup::GpuVertexAttribute kVertexAttrs[] = {
            { yup::GpuVertexFormat::float2, 0, 0 },  // center
            { yup::GpuVertexFormat::float2, 8, 1 },  // offset
            { yup::GpuVertexFormat::float4, 16, 2 }, // color
            { yup::GpuVertexFormat::float2, 32, 3 }, // size (x,y)
        };

        static const yup::GpuVertexBufferLayout kVertexLayout = {
            kVertexStrideFloats * (uint32_t) sizeof (float),
            yup::GpuVertexStepMode::vertex,
            kVertexAttrs,
            (uint32_t) yup::numElementsInArray (kVertexAttrs)
        };

        yup::GpuPipelineOptions pipelineOpts;
        pipelineOpts.vertexBuffers = &kVertexLayout;
        pipelineOpts.vertexBufferCount = 1;
        pipelineOpts.topology = yup::GpuPrimitiveTopology::triangleList;
        pipelineOpts.cullMode = yup::GpuCullMode::none;
        pipelineOpts.colorTargets[0].blendEnabled = false;
        pipelineOpts.colorTargetCount = 1;

        auto renderResult = yup::GpuPipeline::compileFromGlsl (device, vertSource, fragSource, pipelineOpts);
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
