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
    Demonstrates per-frame GpuCanvas rendering with GpuPipeline-based 3D vertex
    rendering and a GpuPipeline-based Gaussian blur post-process.

    The spinning cube is rendered each frame through the RHI layer:
    - GLSL 450 vertex + fragment shaders compiled at runtime via
      GpuPipeline::compileFromGlsl() (which transpiles to the backend-native
      language and derives the binding-map sidecar via reflection)
    - Per-vertex position/color/normal in a GpuBuffer vertex buffer
    - MVP-style transform computed per-frame in the vertex shader
    - Backface culling via GpuCullMode::back and an indexed draw
    - A GpuFrame scopes the GPU work; GpuCanvas::beginRenderPass() encodes into
      the scene canvas via a GpuRenderPass

    A separable Gaussian blur post-process is applied via a second GpuPipeline.
    The blur intensity is controlled by a slider; the horizontal and vertical
    passes share a single GpuFrame.

    Both the cube and the blur pipelines can be live-edited: use the Cube/Blur
    buttons to pick which pipeline to edit, the Vertex/Fragment buttons to pick
    the stage, and Load/Save to persist the active pipeline's vertex + fragment
    GLSL to a .ysl shader bundle.

    The default Lottie animation is played back each frame into an offscreen
    GpuCanvas (2D path) and sampled by the cube's fragment shader, so the moving
    animation is mapped onto every cube face and blended over the per-face tint.
*/
class SpinningCubeDemo : public yup::Component
{
public:
    //==============================================================================
    SpinningCubeDemo()
        : yup::Component ("SpinningCubeDemo")
    {
        currentVertSource = yup::String::fromUTF8 (kDefaultVertSource, sizeof (kDefaultVertSource) - 1);
        currentFragSource = yup::String::fromUTF8 (kDefaultFragSource, sizeof (kDefaultFragSource) - 1);
        currentBlurVertSource = yup::String::fromUTF8 (kDefaultBlurVertSource, sizeof (kDefaultBlurVertSource) - 1);
        currentBlurFragSource = yup::String::fromUTF8 (kDefaultBlurFragSource, sizeof (kDefaultBlurFragSource) - 1);

        blurSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        blurSlider->setRange (0.0, 128.0);
        blurSlider->setValue (0.0);
        blurSlider->onValueChanged = [this] (double v)
        {
            blurSigma = (float) v;
        };
        addAndMakeVisible (blurSlider.get());

        statusLabel = std::make_unique<yup::Label> ("status");
        statusLabel->setText ("Initializing GPU...", yup::dontSendNotification);
        addAndMakeVisible (statusLabel.get());

        // Shader editor UI.
        shaderEditor = std::make_unique<yup::TextEditor> ("shaderEditor");
        shaderEditor->setMultiLine (true);
        shaderEditor->setReadOnly (false);
        shaderEditor->setText (currentVertSource, yup::dontSendNotification);
        shaderEditor->onTextChange = [this]
        {
            activeStageSource() = shaderEditor->getText();
        };
        addAndMakeVisible (shaderEditor.get());

        compileButton = std::make_unique<yup::TextButton> ("Compile");
        compileButton->onClick = [this]
        {
            recompileActiveShader();
        };
        addAndMakeVisible (compileButton.get());

        resetButton = std::make_unique<yup::TextButton> ("Reset");
        resetButton->onClick = [this]
        {
            activeStageSource() = defaultStageSource();
            shaderEditor->setText (activeStageSource(), yup::dontSendNotification);
            recompileActiveShader();
        };
        addAndMakeVisible (resetButton.get());

        cubeTargetButton = std::make_unique<yup::TextButton> ("Cube");
        cubeTargetButton->onClick = [this]
        {
            switchToTarget (false);
        };
        addAndMakeVisible (cubeTargetButton.get());

        blurTargetButton = std::make_unique<yup::TextButton> ("Blur");
        blurTargetButton->onClick = [this]
        {
            switchToTarget (true);
        };
        addAndMakeVisible (blurTargetButton.get());

        vertToggleButton = std::make_unique<yup::TextButton> ("Vertex");
        vertToggleButton->onClick = [this]
        {
            switchToStage (true);
        };
        addAndMakeVisible (vertToggleButton.get());

        fragToggleButton = std::make_unique<yup::TextButton> ("Fragment");
        fragToggleButton->onClick = [this]
        {
            switchToStage (false);
        };
        addAndMakeVisible (fragToggleButton.get());

        loadButton = std::make_unique<yup::TextButton> ("Load .ysl");
        loadButton->onClick = [this]
        {
            loadShaderBundle();
        };
        addAndMakeVisible (loadButton.get());

        saveButton = std::make_unique<yup::TextButton> ("Save .ysl");
        saveButton->onClick = [this]
        {
            saveShaderBundle();
        };
        addAndMakeVisible (saveButton.get());

        shaderModeLabel = std::make_unique<yup::Label> ("shaderMode");
        shaderModeLabel->setText ("Cube - Vertex Shader", yup::dontSendNotification);
        addAndMakeVisible (shaderModeLabel.get());

        compileStatusLabel = std::make_unique<yup::Label> ("compileStatus");
        compileStatusLabel->setText ("", yup::dontSendNotification);
        addAndMakeVisible (compileStatusLabel.get());

        errorEditor = std::make_unique<yup::TextEditor> ("errorEditor");
        errorEditor->setMultiLine (true);
        errorEditor->setReadOnly (true);
        errorEditor->setColor (yup::TextEditor::Style::textColorId, yup::Colors::red);
        errorEditor->setVisible (false);
        addChildComponent (errorEditor.get());
    }

    ~SpinningCubeDemo() override = default;

    //==============================================================================
    void refreshDisplay (double lastFrameTimeSeconds) override
    {
        if (! isDragging)
        {
            angleY += 0.038f;
            angleX += 0.012f;
        }

        lottiePlayer.advanceTime (lastFrameTimeSeconds);
        repaint();
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

        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        bounds.removeFromBottom (60.0f);

        // Split: left 65% = cube, right 35% = editor panel (managed by child components).
        auto cubeBounds = bounds.removeFromLeft (bounds.getWidth() * 0.65f);

        const int w = (int) cubeBounds.getWidth();
        const int h = (int) cubeBounds.getHeight();

        if (w < 2 || h < 2 || ! gpuInitialized)
            return;

        // 1. Create an empty GpuCanvas for the scene (used purely as a render
        //    target - no 2D commit required).
        if (sceneCanvas == nullptr || sceneCanvas->getWidth() != w || sceneCanvas->getHeight() != h)
        {
            sceneCanvas = yup::GpuCanvas::create (*capturedContext, w, h);
            if (sceneCanvas == nullptr)
                return;
        }

        // 2. Render the current Lottie frame into an offscreen GpuCanvas (2D path)
        //    so it can be sampled as a texture by the cube's fragment shader.
        yup::GpuTexture::Ptr lottieTexture = renderLottieTexture();

        // 3. Render the 3D cube into sceneCanvas via GpuPipeline + GpuRenderPass.
        if (cubePipeline != nullptr)
            renderCube (*sceneCanvas, w, h, lottieTexture);

        // 4. Apply separable Gaussian blur: two O(radius) passes (H then V).
        yup::GpuTexture::Ptr outputTex = sceneCanvas->asTexture();

        if (blurPipeline != nullptr && blurSigma > 0.01f)
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

            const float radius = (float) ceil (blurSigma * 3.0);

            // Ping-pong render targets reused across frames (recreated on resize).
            if (blurCanvasA == nullptr || blurCanvasA->getWidth() != w || blurCanvasA->getHeight() != h)
                blurCanvasA = yup::GpuCanvas::create (*capturedContext, w, h);

            if (blurCanvasB == nullptr || blurCanvasB->getWidth() != w || blurCanvasB->getHeight() != h)
                blurCanvasB = yup::GpuCanvas::create (*capturedContext, w, h);

            if (blurCanvasA != nullptr && blurCanvasB != nullptr)
            {
                // Both blur passes share a single GpuFrame.
                auto frame = yup::GpuFrame::begin (*capturedContext);

                auto runPass = [&] (yup::GpuCanvas& passCanvas, const yup::GpuTexture::Ptr& input, float dirX, float dirY) -> yup::GpuTexture::Ptr
                {
                    BlurParams params { blurSigma, radius, (float) w, (float) h, dirX, dirY, 0.0f, 0.0f };

                    auto pass = passCanvas.beginRenderPass (frame, { true, yup::Colors::transparentBlack });
                    pass.setPipeline (*blurPipeline);
                    pass.setTexture (0, 0, input);
                    pass.setUniformBuffer (0, 2, &params, sizeof (params));
                    pass.draw (3);
                    pass.finish();

                    return passCanvas.asTexture();
                };

                outputTex = runPass (*blurCanvasA, outputTex, 1.0f, 0.0f); // horizontal
                outputTex = runPass (*blurCanvasB, outputTex, 0.0f, 1.0f); // vertical

                // Submit without stalling: all contexts share one command queue,
                // so the main frame that samples outputTex is serialised after
                // this work on the GPU. No CPU wait is required.
                frame.submit();
            }
        }

        // 5. Composite to main view.
        if (outputTex != nullptr)
            g.drawTexture (outputTex, cubeBounds);
    }

    //==============================================================================
    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        statusLabel->setBounds (bounds.removeFromBottom (25.0f));
        bounds.removeFromBottom (5.0f);
        blurSlider->setBounds (bounds.removeFromBottom (30.0f));

        // Split remaining area: cube left 65%, editor panel right 35%.
        auto editorBounds = bounds.removeFromRight (bounds.getWidth() * 0.35f);
        editorBounds.removeFromLeft (4.0f);

        // Editor panel layout.
        shaderModeLabel->setBounds (editorBounds.removeFromTop (22.0f));
        editorBounds.removeFromTop (4.0f);

        auto buttonRow1 = editorBounds.removeFromTop (26.0f);
        auto bw1 = buttonRow1.getWidth();
        cubeTargetButton->setBounds (buttonRow1.removeFromLeft (bw1 * 0.47f));
        buttonRow1.removeFromLeft (bw1 * 0.06f);
        blurTargetButton->setBounds (buttonRow1);

        editorBounds.removeFromTop (4.0f);

        auto buttonRow2 = editorBounds.removeFromTop (26.0f);
        auto bw2 = buttonRow2.getWidth();
        vertToggleButton->setBounds (buttonRow2.removeFromLeft (bw2 * 0.47f));
        buttonRow2.removeFromLeft (bw2 * 0.06f);
        fragToggleButton->setBounds (buttonRow2);

        editorBounds.removeFromTop (4.0f);

        auto buttonRow3 = editorBounds.removeFromTop (26.0f);
        auto bw3 = buttonRow3.getWidth();
        compileButton->setBounds (buttonRow3.removeFromLeft (bw3 * 0.47f));
        buttonRow3.removeFromLeft (bw3 * 0.06f);
        resetButton->setBounds (buttonRow3);

        editorBounds.removeFromTop (4.0f);

        auto buttonRow4 = editorBounds.removeFromTop (26.0f);
        auto bw4 = buttonRow4.getWidth();
        loadButton->setBounds (buttonRow4.removeFromLeft (bw4 * 0.47f));
        buttonRow4.removeFromLeft (bw4 * 0.06f);
        saveButton->setBounds (buttonRow4);

        editorBounds.removeFromTop (4.0f);

        compileStatusLabel->setBounds (editorBounds.removeFromTop (22.0f));
        editorBounds.removeFromTop (4.0f);

        if (errorEditor->isVisible())
        {
            errorEditor->setBounds (editorBounds.removeFromBottom (200.0f));
            editorBounds.removeFromBottom (4.0f);
        }

        shaderEditor->setBounds (editorBounds);
    }

    //==============================================================================
    void mouseDown (const yup::MouseEvent& event) override
    {
        if (event.isLeftButtonDown() && getCubeArea().contains (event.getPosition()))
        {
            isDragging = true;
            lastDragPosition = event.getPosition();
        }
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        if (! isDragging)
            return;

        auto delta = event.getPosition() - lastDragPosition;
        lastDragPosition = event.getPosition();

        constexpr float sensitivity = 0.004f;
        angleY -= delta.getX() * sensitivity;
        angleX -= delta.getY() * sensitivity;
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        isDragging = false;
    }

private:
    //==============================================================================
    /** Returns the area where the cube is rendered, matching the layout used in paint(). */
    yup::Rectangle<float> getCubeArea() const
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        bounds.removeFromBottom (60.0f);
        return bounds.removeFromLeft (bounds.getWidth() * 0.65f);
    }

    //==============================================================================
    // ---- Default cube shader sources -----------------------------------------

    static constexpr char kDefaultVertSource[] = R"glsl(#version 450
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec2 a_uv;
layout(set = 0, binding = 0) uniform CubeUniforms {
    float angleY; float angleX; float aspect; float pad;
} u;
layout(location = 0) out vec3 v_color;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
void main() {
    float cy = cos(u.angleY), sy = sin(u.angleY);
    float cx = cos(u.angleX), sx = sin(u.angleX);
    vec3 p  = a_pos;
    vec3 ry = vec3(p.x*cy + p.z*sy,  p.y,  -p.x*sy + p.z*cy);
    vec3 rx = vec3(ry.x,  ry.y*cx - ry.z*sx,  ry.y*sx + ry.z*cx);
    vec3 n  = a_normal;
    vec3 ryn = vec3(n.x*cy + n.z*sy,  n.y,  -n.x*sy + n.z*cy);
    vec3 rxn = vec3(ryn.x, ryn.y*cx - ryn.z*sx, ryn.y*sx + ryn.z*cx);
    float d = rx.z + 3.5;
    float fov = 1.7320508;
    gl_Position = vec4(rx.x * fov / u.aspect, rx.y * fov, (d - 0.1) / 99.9 * d, d);
    v_color  = a_color;
    v_normal = rxn;
    v_uv     = a_uv;
}
)glsl";

    static constexpr char kDefaultFragSource[] = R"glsl(#version 450
layout(location = 0) in vec3 v_color;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(set = 0, binding = 1) uniform texture2D u_tex;
layout(set = 0, binding = 2) uniform sampler   u_samp;
layout(location = 0) out vec4 fragColor;
void main() {
    vec3  light = normalize(vec3(0.503, 0.671, -0.419));
    float ndotl = clamp(dot(normalize(v_normal), light), 0.0, 1.0);
    vec4  tex   = texture(sampler2D(u_tex, u_samp), vec2(v_uv.x, 1.0 - v_uv.y));
    vec3  base  = mix(v_color, tex.rgb, tex.a);
    fragColor   = vec4(base * (0.35 + 0.65 * ndotl), 1.0);
}
)glsl";

    // ---- Default blur shader sources -----------------------------------------

    // GLSL 450 blur vertex shader: fullscreen triangle from vertex index, no vertex buffer.
    static constexpr char kDefaultBlurVertSource[] = R"glsl(#version 450
void main() {
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)glsl";

    // GLSL 450 blur fragment shader: separable 1-D Gaussian using separate texture + sampler.
    // Runs twice per frame (horizontal then vertical) driven by BlurParams.dir, turning the
    // naive O(radius^2) 2-D blur into two O(radius) passes.
    // Bindings: set=0 binding=0 = scene texture (separate image)
    //           set=0 binding=1 = sampler
    //           set=0 binding=2 = BlurParams UBO
    static constexpr char kDefaultBlurFragSource[] = R"glsl(#version 450
layout(set = 0, binding = 0) uniform texture2D u_tex;
layout(set = 0, binding = 1) uniform sampler   u_samp;
layout(set = 0, binding = 2) uniform BlurParams {
    float sigma; float radius; float resX; float resY;
    float dirX;  float dirY;   float pad0; float pad1;
} p;
layout(location = 0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    if (p.sigma <= 0.0001) { fragColor = texture(sampler2D(u_tex, u_samp), uv); return; }
    int   radius = int(clamp(p.radius, 1.0, 128.0));  // slider max 40 > ceil(120)
    vec2  step   = vec2(p.dirX, p.dirY) / vec2(p.resX, p.resY);
    float inv2s2 = 0.5 / (p.sigma * p.sigma);
    vec4  sum  = texture(sampler2D(u_tex, u_samp), uv);
    float wsum = 1.0;
    for (int i = 1; i <= radius; ++i) {
        float w   = exp(-float(i * i) * inv2s2);
        vec2  off = step * float(i);
        sum  += texture(sampler2D(u_tex, u_samp), uv + off) * w;
        sum  += texture(sampler2D(u_tex, u_samp), uv - off) * w;
        wsum += 2.0 * w;
    }
    fragColor = sum / wsum;
}
)glsl";

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
        static constexpr yup::GpuVertexAttribute attrs[4] = {
            { yup::GpuVertexFormat::float3, 0, 0 },
            { yup::GpuVertexFormat::float3, 12, 1 },
            { yup::GpuVertexFormat::float3, 24, 2 },
            { yup::GpuVertexFormat::float2, 36, 3 },
        };

        static constexpr yup::GpuVertexBufferLayout vbLayout {
            (uint32_t) sizeof (CubeVertex),
            yup::GpuVertexStepMode::vertex,
            attrs,
            4
        };

        yup::GpuPipelineOptions options;
        options.vertexBuffers = &vbLayout;
        options.vertexBufferCount = 1;
        options.topology = yup::GpuPrimitiveTopology::triangleList;
        options.indexFormat = yup::GpuIndexFormat::uint16;
        options.cullMode = yup::GpuCullMode::back;
        options.winding = yup::GpuFaceWinding::counterClockwise;
        return options;
    }

    // ---- GPU initialisation --------------------------------------------------

    void initGpu()
    {
        if (capturedContext == nullptr || gpuInitialized)
            return;

        gpuInitialized = true;

        if (! capturedContext->isGpuAvailable())
        {
            statusLabel->setText ("ore context unavailable - rebuild with enableOreContext=true.",
                                  yup::dontSendNotification);
            return;
        }

        initBlur();
        initCube();
        initLottie();

        if (cubePipeline != nullptr && blurPipeline != nullptr)
            statusLabel->setText ("GPU cube + separable blur ready. Drag slider for blur intensity.", yup::dontSendNotification);
        else if (cubePipeline != nullptr)
            statusLabel->setText ("GPU cube ready. Blur compile failed - see debug log.", yup::dontSendNotification);
        else
            statusLabel->setText ("GPU init failed - see debug log.", yup::dontSendNotification);
    }

    void initLottie()
    {
#if YUP_ANDROID
        yup::MemoryInputStream is (yup::LottieFile_data, yup::LottieFile_size, false);
        auto anim = yup::Animation::loadFromStream (is);
#else
        auto anim = yup::Animation::loadFromFile (getAssetPath (YUP_EXAMPLE_GRAPHICS_LOTTIE_FILE));
#endif

        if (! anim.isValid())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: failed to load Lottie texture animation.");
            return;
        }

        lottiePlayer.setAnimation (std::move (anim));
        lottiePlayer.setLooping (true);
        lottiePlayer.play();
    }

    void initBlur()
    {
        auto result = yup::GpuPipeline::compileFromGlsl (*capturedContext,
                                                         currentBlurVertSource,
                                                         currentBlurFragSource,
                                                         {});

        if (result.failed())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: blur compile failed: " + result.getErrorMessage());
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

        auto result = yup::GpuPipeline::compileFromBundle (*capturedContext,
                                                           loaded.getReference(),
                                                           cubePipelineOptions());

        const auto& bundle = loaded.getReference();

        if (auto* vs = bundle.findShader (yup::ShaderStage::vertex, yup::ShaderLanguage::glsl))
            currentVertSource = vs->source;

        if (auto* fs = bundle.findShader (yup::ShaderStage::fragment, yup::ShaderLanguage::glsl))
            currentFragSource = fs->source;

        if (result.failed())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: cube shader compile failed: " + result.getErrorMessage());
            return;
        }

        cubePipeline = result.getValue();

        // Upload immutable vertex and index buffers.
        cubeVBO = yup::GpuBuffer::create (*capturedContext, yup::GpuBufferType::vertex, kCubeVerts, sizeof (kCubeVerts));
        cubeIBO = yup::GpuBuffer::create (*capturedContext, yup::GpuBufferType::index, kCubeIdx, sizeof (kCubeIdx));

        if (cubeVBO == nullptr || cubeIBO == nullptr)
            yup::Logger::outputDebugString ("SpinningCubeDemo: cube buffer creation failed.");
    }

    // ---- Shader live-editing helpers -----------------------------------------

    /** Returns a reference to the source string for the active target + stage. */
    yup::String& activeStageSource()
    {
        if (editingBlur)
            return showingVertexShader ? currentBlurVertSource : currentBlurFragSource;

        return showingVertexShader ? currentVertSource : currentFragSource;
    }

    /** Returns the default source for the active target + stage. */
    yup::String defaultStageSource() const
    {
        if (editingBlur)
            return showingVertexShader
                     ? yup::String::fromUTF8 (kDefaultBlurVertSource, sizeof (kDefaultBlurVertSource) - 1)
                     : yup::String::fromUTF8 (kDefaultBlurFragSource, sizeof (kDefaultBlurFragSource) - 1);

        return showingVertexShader
                 ? yup::String::fromUTF8 (kDefaultVertSource, sizeof (kDefaultVertSource) - 1)
                 : yup::String::fromUTF8 (kDefaultFragSource, sizeof (kDefaultFragSource) - 1);
    }

    void updateShaderModeLabel()
    {
        shaderModeLabel->setText (yup::String (editingBlur ? "Blur" : "Cube")
                                      + " - "
                                      + (showingVertexShader ? "Vertex Shader" : "Fragment Shader"),
                                  yup::dontSendNotification);
    }

    void switchToTarget (bool blur)
    {
        if (editingBlur == blur)
            return;

        syncEditorSource();
        editingBlur = blur;
        shaderEditor->setText (activeStageSource(), yup::dontSendNotification);
        updateShaderModeLabel();
    }

    void switchToStage (bool vertex)
    {
        if (showingVertexShader == vertex)
            return;

        syncEditorSource();
        showingVertexShader = vertex;
        shaderEditor->setText (activeStageSource(), yup::dontSendNotification);
        updateShaderModeLabel();
    }

    /** Recompiles the active pipeline (cube or blur) from the current sources. */
    void recompileActiveShader()
    {
        if (capturedContext == nullptr || ! gpuInitialized)
            return;

        syncEditorSource();

        if (editingBlur)
        {
            auto result = yup::GpuPipeline::compileFromGlsl (*capturedContext,
                                                             currentBlurVertSource,
                                                             currentBlurFragSource,
                                                             {});

            if (result.failed())
            {
                showError (result.getErrorMessage());
                return;
            }

            blurPipeline = result.getValue();
        }
        else
        {
            auto result = yup::GpuPipeline::compileFromGlsl (*capturedContext,
                                                             currentVertSource,
                                                             currentFragSource,
                                                             cubePipelineOptions());

            if (result.failed())
            {
                showError (result.getErrorMessage());
                return;
            }

            cubePipeline = result.getValue();
        }

        hideError();
    }

    void showError (const yup::String& text)
    {
        errorEditor->setText (text, yup::dontSendNotification);
        errorEditor->setVisible (true);
        compileStatusLabel->setText ("Compile error.", yup::dontSendNotification);
        resized();
    }

    void hideError()
    {
        errorEditor->setVisible (false);
        errorEditor->setText ("", yup::dontSendNotification);
        compileStatusLabel->setText ("Compiled OK", yup::dontSendNotification);
        resized();
    }

    void loadShaderBundle()
    {
        auto chooser = yup::FileChooser::create ("Load Shader Bundle",
                                                 yup::File(),
                                                 "*.ysl");
        chooser->browseForFileToOpen ([this] (bool success, const yup::Array<yup::File>& results)
        {
            if (! success || results.isEmpty())
                return;

            auto loaded = yup::ShaderBundle::loadFromFile (results[0]);
            if (loaded.failed())
            {
                compileStatusLabel->setText ("Failed to load .ysl: " + loaded.getErrorMessage(), yup::dontSendNotification);
                return;
            }

            const auto& bundle = loaded.getReference();

            bool loadedAny = false;

            if (auto* vs = bundle.findShader (yup::ShaderStage::vertex, yup::ShaderLanguage::glsl))
            {
                (editingBlur ? currentBlurVertSource : currentVertSource) = vs->source;
                loadedAny = true;
            }

            if (auto* fs = bundle.findShader (yup::ShaderStage::fragment, yup::ShaderLanguage::glsl))
            {
                (editingBlur ? currentBlurFragSource : currentFragSource) = fs->source;
                loadedAny = true;
            }

            if (! loadedAny)
            {
                // Legacy bundles only stored a single source blob.
                activeStageSource() = bundle.getOriginalSource();
            }

            shaderEditor->setText (activeStageSource(), yup::dontSendNotification);
            recompileActiveShader();
        });
    }

    void saveShaderBundle()
    {
        syncEditorSource();

        yup::String vertSource = editingBlur ? currentBlurVertSource : currentVertSource;
        yup::String fragSource = editingBlur ? currentBlurFragSource : currentFragSource;

        auto chooser = yup::FileChooser::create ("Save Shader Bundle",
                                                 yup::File(),
                                                 "*.ysl");
        chooser->browseForFileToSave ([vertSource, fragSource] (bool success, const yup::Array<yup::File>& results)
        {
            if (! success || results.isEmpty())
                return;

            yup::ShaderBundle bundle;
            bundle.setOriginalSource (vertSource);

            yup::ShaderInfo vertInfo;
            vertInfo.stage = yup::ShaderStage::vertex;
            vertInfo.language = yup::ShaderLanguage::glsl;
            vertInfo.entryPoint = "main";
            vertInfo.source = vertSource;
            bundle.addShader (std::move (vertInfo));

            yup::ShaderInfo fragInfo;
            fragInfo.stage = yup::ShaderStage::fragment;
            fragInfo.language = yup::ShaderLanguage::glsl;
            fragInfo.entryPoint = "main";
            fragInfo.source = fragSource;
            bundle.addShader (std::move (fragInfo));

            auto result = bundle.saveToFile (results[0]);
            if (result.failed())
                yup::Logger::outputDebugString ("SpinningCubeDemo: save YSLB failed: " + result.getErrorMessage());
        },
                                      true);
    }

    void syncEditorSource()
    {
        activeStageSource() = shaderEditor->getText();
    }

    // ---- Per-frame Lottie texture render -------------------------------------

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

        lottiePlayer.render (g, yup::Rectangle<float> (0.0f, 0.0f, (float) kLottieTextureSize, (float) kLottieTextureSize), true);

        return lottieCanvas->asTexture();
    }

    // ---- Per-frame cube render pass -----------------------------------------

    void renderCube (yup::GpuCanvas& canvas, int w, int h, const yup::GpuTexture::Ptr& lottieTexture)
    {
        if (cubePipeline == nullptr || cubeVBO == nullptr || cubeIBO == nullptr)
            return;

        // Per-frame uniform buffer (rotation angles + aspect ratio).
        struct CubeUniforms
        {
            float angleY, angleX, aspect, pad;
        };

        CubeUniforms uniforms { angleY, angleX, (float) w / (float) h, 0.0f };

        auto frame = yup::GpuFrame::begin (*capturedContext);

        auto pass = canvas.beginRenderPass (frame, { true, yup::Color (0xff1a1a2e) });
        pass.setPipeline (*cubePipeline);
        pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
        if (lottieTexture != nullptr)
            pass.setTexture (0, 1, lottieTexture);
        pass.setVertexBuffer (0, cubeVBO);
        pass.setIndexBuffer (yup::GpuIndexFormat::uint16, cubeIBO);
        pass.drawIndexed (yup::numElementsInArray (kCubeIdx));
        pass.finish();

        // Submit without stalling: the shared command queue serialises this
        // work ahead of the main frame that samples the scene texture, so no
        // CPU wait is needed here.
        frame.submit();
    }

    //==============================================================================
    yup::GraphicsContext* capturedContext = nullptr;

    // Cube pass (GpuPipeline indexed geometry).
    yup::GpuPipeline::Ptr cubePipeline;
    yup::GpuBuffer::Ptr cubeVBO;
    yup::GpuBuffer::Ptr cubeIBO;
    yup::GpuCanvas::Ptr sceneCanvas;

    // Blur pass (GpuPipeline fullscreen triangle).
    yup::GpuPipeline::Ptr blurPipeline;

    // Ping-pong blur render targets, reused across frames (recreated on resize).
    yup::GpuCanvas::Ptr blurCanvasA;
    yup::GpuCanvas::Ptr blurCanvasB;

    // Lottie texture source sampled by the cube faces.
    static constexpr int kLottieTextureSize = 512;
    yup::AnimationPlayer lottiePlayer;
    yup::GpuCanvas::Ptr lottieCanvas;

    bool gpuInitialized = false;
    float angleY = 0.0f;
    float angleX = 0.15f;
    float blurSigma = 0.0f;

    bool isDragging = false;
    yup::Point<float> lastDragPosition;

    std::unique_ptr<yup::Slider> blurSlider;
    std::unique_ptr<yup::Label> statusLabel;

    // Shader live editing.
    yup::String currentVertSource;
    yup::String currentFragSource;
    yup::String currentBlurVertSource;
    yup::String currentBlurFragSource;
    bool showingVertexShader = true;
    bool editingBlur = false;

    std::unique_ptr<yup::TextEditor> shaderEditor;
    std::unique_ptr<yup::TextButton> compileButton;
    std::unique_ptr<yup::TextButton> resetButton;
    std::unique_ptr<yup::TextButton> cubeTargetButton;
    std::unique_ptr<yup::TextButton> blurTargetButton;
    std::unique_ptr<yup::TextButton> vertToggleButton;
    std::unique_ptr<yup::TextButton> fragToggleButton;
    std::unique_ptr<yup::TextButton> loadButton;
    std::unique_ptr<yup::TextButton> saveButton;
    std::unique_ptr<yup::Label> shaderModeLabel;
    std::unique_ptr<yup::Label> compileStatusLabel;
    std::unique_ptr<yup::TextEditor> errorEditor;
};
