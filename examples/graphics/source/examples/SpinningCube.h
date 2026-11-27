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
#include <yup_animation/yup_animation.h>

#include <ShaderBundle.h>

//==============================================================================

/**
    Demonstrates per-frame GPU rendering through the RHI layer: a textured cube drawn with a
    GpuPipeline, followed by a separable Gaussian blur post-process on a second pipeline.

    - The cube pipeline comes from a shader bundle embedded in the app, the blur pipeline from
      the precompiled effect_blur bundle shared with the Component Effects demo.
    - Per-vertex position/color/normal/uv in a GpuBuffer, drawn indexed with back face culling.
    - A Lottie animation is played into an offscreen GpuCanvas every frame and sampled by the
      cube faces. Drop a .lottie or .json file onto the cube to replace it.
    - The cube pass and both blur passes share one GpuFrame.

    Drag to spin the cube (it keeps the momentum of a flick), scroll or pinch to blur it,
    double-click to reset.
*/
class SpinningCubeDemo : public yup::Component
    , public yup::DragAndDropTarget
    , public LoadingDemo
{
public:
    //==============================================================================
    SpinningCubeDemo()
        : yup::Component ("SpinningCubeDemo")
    {
        setWantedGestures (yup::GestureType::pinch);

        addLoadingStep ([this]
        {
            if (capturedContext->isGpuAvailable())
            {
                initBlur();
                return;
            }

            statusText = "GPU context unavailable";
            finishLoading();
        }, StepThread::render);

        addLoadingStep ([this]
        {
            initCube();
        }, StepThread::render);

        addLoadingStep ([this]
        {
            initLottie();

            if (cubePipeline == nullptr)
                statusText = "GPU init failed - see debug log";

            gpuInitialized = cubePipeline != nullptr;
        }, StepThread::render);
    }

    ~SpinningCubeDemo() override = default;

    //==============================================================================
    void refreshDisplay (double lastFrameTimeSeconds)
    {
        const auto deltaSeconds = static_cast<float> (yup::jlimit (0.0, 0.1, lastFrameTimeSeconds));

        if (isDragging)
        {
            // The spin follows the pointer: remember how fast it went, for the flick on release
            if (deltaSeconds > 0.0f)
            {
                velocity = velocity + (dragSinceLastFrame / deltaSeconds - velocity) * 0.5f;
                dragSinceLastFrame = {};
            }
        }
        else
        {
            // After a flick, ease back to the idle spin
            velocity = velocity + (idleVelocity - velocity) * (1.0f - std::exp (-deltaSeconds * 1.2f));
            angles = angles + velocity * deltaSeconds;
        }

        if (hasInteracted)
            hintAlpha = yup::jmax (0.0f, hintAlpha - deltaSeconds * 0.8f);

        lottiePlayer.advanceTime (lastFrameTimeSeconds);
        repaint();
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

        g.setFillColor (palette.getColor (yup::ThemePalette::Role::background));
        g.fillAll();

        if (capturedContext == nullptr)
            capturedContext = &g.getGraphicsContext();

        if (pumpLoading())
            return;

        const auto bounds = getLocalBounds();
        const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (14.0f);

        if (! gpuInitialized)
        {
            g.setFillColor (palette.getColor (yup::ThemePalette::Role::textMuted));
            g.fillFittedText (statusText, font, bounds, yup::Justification::center);
            return;
        }

        if (auto texture = renderScene ((int) bounds.getWidth(), (int) bounds.getHeight()))
            g.drawTexture (texture, bounds);

        if (hintAlpha > 0.0f)
        {
            g.setFillColor (yup::Colors::white.withAlpha (0.75f * hintAlpha));
            g.fillFittedText ("Drag to spin - scroll or pinch to blur - double-click to reset - drop a Lottie file",
                              font,
                              bounds.reduced (16.0f).removeFromBottom (24.0f),
                              yup::Justification::center);
        }
    }

    //==============================================================================
    void mouseDown (const yup::MouseEvent& event) override
    {
        if (! event.isLeftButtonDown())
            return;

        isDragging = true;
        hasInteracted = true;
        lastDragPosition = event.getPosition();
        dragSinceLastFrame = {};
        velocity = {};
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        if (! isDragging)
            return;

        constexpr float sensitivity = 0.006f;

        const auto delta = event.getPosition() - lastDragPosition;
        lastDragPosition = event.getPosition();

        const auto rotation = yup::Point<float> (-delta.getX(), -delta.getY()) * sensitivity;
        angles = angles + rotation;
        dragSinceLastFrame = dragSinceLastFrame + rotation;
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        isDragging = false;
    }

    void mouseDoubleClick (const yup::MouseEvent&) override
    {
        angles = initialAngles;
        velocity = idleVelocity;
        blurSigma = 0.0f;
    }

    bool mouseWheel (const yup::MouseEvent&, const yup::MouseWheelData& wheelData) override
    {
        setBlur (blurSigma + wheelData.getDeltaY() * 20.0f);
        return true;
    }

    void gesturePinch (const yup::GestureEvent& event) override
    {
        // Pinching in blurs, spreading the fingers sharpens
        if (event.getScaleDelta() > 0.0f)
            setBlur (blurSigma - std::log (event.getScaleDelta()) * 20.0f);
    }

    //==============================================================================
    bool isInterestedInDragSource (const yup::DragAndDropSourceDetails& details) override
    {
        for (const auto& file : details.data.getFiles())
        {
            if (isSupportedLottieFile (file))
                return true;
        }

        return false;
    }

    bool itemDropped (const yup::DragAndDropSourceDetails& details) override
    {
        for (const auto& file : details.data.getFiles())
        {
            if (isSupportedLottieFile (file))
                return loadLottieFile (file);
        }

        return false;
    }

private:
    //==============================================================================
    void setBlur (float newSigma)
    {
        blurSigma = yup::jlimit (0.0f, maxBlurSigma, newSigma);
        hasInteracted = true;
    }

    // ---- Cube geometry -------------------------------------------------------

    struct CubeVertex
    {
        float pos[3];
        float color[3];
        float normal[3];
        float uv[2];
    };

    static constexpr CubeVertex kCubeVerts[24] = {
        // Front (-Z), red
        { { -1, -1, -1 }, { 0.91f, 0.30f, 0.24f }, { 0, 0, -1 }, { 0, 0 } },
        { { 1, -1, -1 }, { 0.91f, 0.30f, 0.24f }, { 0, 0, -1 }, { 1, 0 } },
        { { 1, 1, -1 }, { 0.91f, 0.30f, 0.24f }, { 0, 0, -1 }, { 1, 1 } },
        { { -1, 1, -1 }, { 0.91f, 0.30f, 0.24f }, { 0, 0, -1 }, { 0, 1 } },
        // Back (+Z), blue
        { { 1, -1, 1 }, { 0.20f, 0.60f, 0.86f }, { 0, 0, 1 }, { 0, 0 } },
        { { -1, -1, 1 }, { 0.20f, 0.60f, 0.86f }, { 0, 0, 1 }, { 1, 0 } },
        { { -1, 1, 1 }, { 0.20f, 0.60f, 0.86f }, { 0, 0, 1 }, { 1, 1 } },
        { { 1, 1, 1 }, { 0.20f, 0.60f, 0.86f }, { 0, 0, 1 }, { 0, 1 } },
        // Left (-X), green
        { { -1, -1, 1 }, { 0.18f, 0.80f, 0.44f }, { -1, 0, 0 }, { 0, 0 } },
        { { -1, -1, -1 }, { 0.18f, 0.80f, 0.44f }, { -1, 0, 0 }, { 1, 0 } },
        { { -1, 1, -1 }, { 0.18f, 0.80f, 0.44f }, { -1, 0, 0 }, { 1, 1 } },
        { { -1, 1, 1 }, { 0.18f, 0.80f, 0.44f }, { -1, 0, 0 }, { 0, 1 } },
        // Right (+X), orange
        { { 1, -1, -1 }, { 0.95f, 0.61f, 0.07f }, { 1, 0, 0 }, { 0, 0 } },
        { { 1, -1, 1 }, { 0.95f, 0.61f, 0.07f }, { 1, 0, 0 }, { 1, 0 } },
        { { 1, 1, 1 }, { 0.95f, 0.61f, 0.07f }, { 1, 0, 0 }, { 1, 1 } },
        { { 1, 1, -1 }, { 0.95f, 0.61f, 0.07f }, { 1, 0, 0 }, { 0, 1 } },
        // Top (+Y), purple
        { { -1, 1, -1 }, { 0.61f, 0.35f, 0.71f }, { 0, 1, 0 }, { 0, 0 } },
        { { 1, 1, -1 }, { 0.61f, 0.35f, 0.71f }, { 0, 1, 0 }, { 1, 0 } },
        { { 1, 1, 1 }, { 0.61f, 0.35f, 0.71f }, { 0, 1, 0 }, { 1, 1 } },
        { { -1, 1, 1 }, { 0.61f, 0.35f, 0.71f }, { 0, 1, 0 }, { 0, 1 } },
        // Bottom (-Y), teal
        { { -1, -1, 1 }, { 0.10f, 0.74f, 0.61f }, { 0, -1, 0 }, { 0, 0 } },
        { { 1, -1, 1 }, { 0.10f, 0.74f, 0.61f }, { 0, -1, 0 }, { 1, 0 } },
        { { 1, -1, -1 }, { 0.10f, 0.74f, 0.61f }, { 0, -1, 0 }, { 1, 1 } },
        { { -1, -1, -1 }, { 0.10f, 0.74f, 0.61f }, { 0, -1, 0 }, { 0, 1 } },
    };

    // clang-format off
    static constexpr uint16_t kCubeIdx[36] = {
        0, 1, 2, 0, 2, 3, // front
        4, 5, 6, 4, 6, 7, // back
        8, 9, 10, 8, 10, 11, // left
        12, 13, 14, 12, 14, 15, // right
        16, 17, 18, 16, 18, 19, // top
        20, 21, 22, 20, 22, 23, // bottom
    };
    // clang-format on

    //==============================================================================
    /** Builds the pipeline options describing the cube's vertex layout and state. */
    static yup::GpuPipelineOptions cubePipelineOptions()
    {
        yup::GpuPipelineOptions options;
        options.vertexBuffers.emplace_back (
            (uint32_t) sizeof (CubeVertex),
            yup::GpuVertexStepMode::vertex,
            std::vector<yup::GpuVertexAttribute> {
                { yup::GpuVertexFormat::float3, 0, 0 },
                { yup::GpuVertexFormat::float3, 12, 1 },
                { yup::GpuVertexFormat::float3, 24, 2 },
                { yup::GpuVertexFormat::float2, 36, 3 },
            });
        options.topology = yup::GpuPrimitiveTopology::triangleList;
        options.indexFormat = yup::GpuIndexFormat::uint16;
        options.cullMode = yup::GpuCullMode::back;
        options.winding = yup::GpuFaceWinding::counterClockwise;
        return options;
    }

    // ---- GPU initialisation --------------------------------------------------

    void initLottie()
    {
        auto anim = yup::Animation::loadFromFile (getAssetPath ("data/lottie/goal.lottie"));

        if (! anim.isValid())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: failed to load Lottie texture animation.");
            return;
        }

        applyLottieAnimation (std::move (anim));
    }

    /** Returns true for file types that can be used as the cube's Lottie texture. */
    static bool isSupportedLottieFile (const yup::File& file)
    {
        return file.hasFileExtension ("json") || file.hasFileExtension ("lottie");
    }

    /** Loads a dropped Lottie file and uses it as the cube's texture animation. */
    bool loadLottieFile (const yup::File& file)
    {
        auto anim = yup::Animation::loadFromFile (file);
        if (! anim.isValid())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: failed to load dropped Lottie file: " + file.getFileName());
            return false;
        }

        applyLottieAnimation (std::move (anim));
        return true;
    }

    void applyLottieAnimation (yup::Animation anim)
    {
        lottiePlayer.setAnimation (std::move (anim));
        lottiePlayer.setLooping (true);
        lottiePlayer.play();
    }

    void initBlur()
    {
        auto result = compilePipelineFromBundle (capturedContext->getGpuDevice(), "effect_blur");

        if (result.failed())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: " + result.getErrorMessage());
            return;
        }

        blurPipeline = result.getValue();
    }

    void initCube()
    {
        auto loaded = yup::ShaderBundle::loadFromData (yup::ShaderBundleFile_data, yup::ShaderBundleFile_size);
        if (loaded.failed())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: failed to load shader bundle: " + loaded.getErrorMessage());
            return;
        }

        auto result = yup::GpuPipeline::compileFromBundle (capturedContext->getGpuDevice(),
                                                           loaded.getReference(),
                                                           cubePipelineOptions());

        if (result.failed())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: cube shader compile failed: " + result.getErrorMessage());
            return;
        }

        cubePipeline = result.getValue();

        // Upload immutable vertex and index buffers.
        cubeVBO = yup::GpuBuffer::create (capturedContext->getGpuDevice(), yup::GpuBufferType::vertex, kCubeVerts, sizeof (kCubeVerts));
        cubeIBO = yup::GpuBuffer::create (capturedContext->getGpuDevice(), yup::GpuBufferType::index, kCubeIdx, sizeof (kCubeIdx));

        if (cubeVBO == nullptr || cubeIBO == nullptr)
            yup::Logger::outputDebugString ("SpinningCubeDemo: cube buffer creation failed.");
    }

    // ---- Per-frame scene render ----------------------------------------------

    /** Renders the cube, then blurs it, returning the texture to composite. */
    yup::GpuTexture::Ptr renderScene (int w, int h)
    {
        if (w < 2 || h < 2)
            return nullptr;

        // An empty GpuTarget for the scene, used purely as a render target
        if (sceneCanvas == nullptr || sceneCanvas->getWidth() != w || sceneCanvas->getHeight() != h)
        {
            sceneCanvas = yup::GpuTarget::create (capturedContext->getGpuDevice(), w, h);
            if (sceneCanvas == nullptr)
                return nullptr;
        }

        // The current Lottie frame, sampled by the cube's fragment shader
        const auto lottieTexture = renderLottieTexture();

        // The cube pass and both blur passes run on the same command buffer
        auto frame = yup::GpuFrame::begin (capturedContext->getGpuDevice());

        renderCube (frame, *sceneCanvas, w, h, lottieTexture);

        auto outputTex = sceneCanvas->asTexture();

        if (blurPipeline != nullptr && blurSigma > 0.01f)
            outputTex = renderBlur (frame, outputTex, w, h);

        // Submit without stalling: all contexts share one command queue, so the main frame
        // sampling outputTex is serialised after this work on the GPU.
        frame.submit();

        return outputTex;
    }

    /** Separable Gaussian blur: two O(radius) passes, horizontal then vertical. */
    yup::GpuTexture::Ptr renderBlur (yup::GpuFrame& frame, yup::GpuTexture::Ptr input, int w, int h)
    {
        struct alignas (16) BlurParams
        {
            float sigma;  // sigma in pixels
            float radius; // ceil(sigma * 3.0), clamped in-shader
            float resX;
            float resY;
            float dirX; // pass direction: (1,0) horizontal, (0,1) vertical
            float dirY;
            float pad0;
            float pad1;
        };

        // Ping-pong render targets reused across frames (recreated on resize).
        for (auto* target : { &blurCanvasA, &blurCanvasB })
        {
            if (*target == nullptr || (*target)->getWidth() != w || (*target)->getHeight() != h)
                *target = yup::GpuTarget::create (capturedContext->getGpuDevice(), w, h);

            if (*target == nullptr)
                return input;
        }

        const float radius = std::ceil (blurSigma * 3.0f);

        auto runPass = [&] (yup::GpuTarget& passCanvas, const yup::GpuTexture::Ptr& passInput, float dirX, float dirY)
        {
            BlurParams params { blurSigma, radius, (float) w, (float) h, dirX, dirY, 0.0f, 0.0f };

            auto pass = passCanvas.beginRenderPass (frame, { true, yup::Colors::transparentBlack });
            pass.setPipeline (blurPipeline);
            pass.setTexture (0, 0, passInput);
            pass.setUniformBuffer (0, 2, &params, sizeof (params));
            pass.draw (3);
            pass.finish();

            return passCanvas.asTexture();
        };

        auto horizontal = runPass (*blurCanvasA, input, 1.0f, 0.0f);
        return runPass (*blurCanvasB, horizontal, 0.0f, 1.0f);
    }

    /** Renders the current Lottie frame into an offscreen GpuCanvas (2D path) and
        returns a sampleable texture for the cube's fragment shader. */
    yup::GpuTexture::Ptr renderLottieTexture()
    {
        if (! lottiePlayer.getAnimation().isValid())
            return nullptr;

        // A single canvas is reused every frame: only its contents change, so
        // the GPU target textures are allocated once instead of per frame.
        if (lottieCanvas == nullptr)
        {
            lottieCanvas = yup::GpuCanvas::create (*capturedContext, kLottieTextureSize, kLottieTextureSize);
            if (lottieCanvas == nullptr)
                return nullptr;
        }

        auto& g = lottieCanvas->beginDraw();
        g.setFillColor (yup::Colors::white);
        g.fillAll();

        lottiePlayer.render (g, yup::Rectangle<float> (0.0f, 0.0f, (float) kLottieTextureSize, (float) kLottieTextureSize));

        return lottieCanvas->asTexture();
    }

    // ---- Per-frame cube render pass -----------------------------------------

    void renderCube (yup::GpuFrame& frame, yup::GpuTarget& canvas, int w, int h, const yup::GpuTexture::Ptr& lottieTexture)
    {
        if (cubePipeline == nullptr || cubeVBO == nullptr || cubeIBO == nullptr)
            return;

        // Per-frame uniform buffer (rotation angles + aspect ratio).
        struct CubeUniforms
        {
            float angleY, angleX, aspect, pad;
        };

        CubeUniforms uniforms { angles.getX(), angles.getY(), (float) w / (float) h, 0.0f };

        auto pass = canvas.beginRenderPass (frame, { true, yup::Color (0xff1a1a2e) });
        pass.setPipeline (cubePipeline);
        pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
        if (lottieTexture != nullptr)
            pass.setTexture (0, 1, lottieTexture);
        pass.setVertexBuffer (0, cubeVBO);
        pass.setIndexBuffer (yup::GpuIndexFormat::uint16, cubeIBO);
        pass.drawIndexed (yup::numElementsInArray (kCubeIdx));
        pass.finish();
    }

    //==============================================================================
    yup::GraphicsContext* capturedContext = nullptr;

    // Cube pass (GpuPipeline indexed geometry).
    yup::GpuPipeline::Ptr cubePipeline;
    yup::GpuBuffer::Ptr cubeVBO;
    yup::GpuBuffer::Ptr cubeIBO;
    yup::GpuTarget::Ptr sceneCanvas;

    // Blur pass (GpuPipeline fullscreen triangle).
    yup::GpuPipeline::Ptr blurPipeline;

    // Ping-pong blur render targets, reused across frames (recreated on resize).
    yup::GpuTarget::Ptr blurCanvasA;
    yup::GpuTarget::Ptr blurCanvasB;

    // Lottie texture source sampled by the cube faces.
    static constexpr int kLottieTextureSize = 512;
    yup::AnimationPlayer lottiePlayer;
    yup::GpuCanvas::Ptr lottieCanvas;

    // Spin, as (angle around Y, angle around X) in radians
    static constexpr yup::Point<float> initialAngles { 0.0f, 0.15f };
    static constexpr yup::Point<float> idleVelocity { 2.28f, 0.72f };
    static constexpr float maxBlurSigma = 40.0f;

    bool gpuInitialized = false;
    yup::String statusText;
    yup::Point<float> angles = initialAngles;
    yup::Point<float> velocity = idleVelocity;
    yup::Point<float> dragSinceLastFrame;
    float blurSigma = 0.0f;
    float hintAlpha = 1.0f;
    bool hasInteracted = false;

    bool isDragging = false;
    yup::Point<float> lastDragPosition;

    yup::FrameAttachment frameAttachment { *this, yup::bindFront (&SpinningCubeDemo::refreshDisplay, this) };
};
