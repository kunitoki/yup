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
    Demonstrates per-frame GpuCanvas rendering with ore-direct 3D vertex rendering
    and GpuProgram-based Gaussian blur post-processing.

    The spinning cube is rendered each frame using rive::ore directly:
    - GLSL 450 vertex + fragment shaders compiled at runtime via ore::Context::makeShaderModule()
    - Per-vertex position/color/normal in a GPU vertex buffer (ore::Buffer)
    - MVP-style transform computed per-frame in the vertex shader
    - Backface culling via ore::CullMode::back

    A Gaussian blur post-process is applied via GpuProgram when ore is available.
    The blur intensity is controlled by a slider.

    ShaderTranspiler is used for reflection only: GLSL → SPIR-V → reflect with the
    target backend language → native slot numbers → BindingMap blob. The GLSL source
    itself is passed to ore unchanged (ore cross-compiles internally for Metal/D3D/GL).
*/
class SpinningCubeDemo : public yup::Component
{
public:
    //==============================================================================
    SpinningCubeDemo()
        : yup::Component ("SpinningCubeDemo")
    {
        blurSlider = std::make_unique<yup::Slider> (yup::Slider::LinearHorizontal);
        blurSlider->setRange (0.0, 40.0);
        blurSlider->setValue (0.0);
        blurSlider->onValueChanged = [this] (double v)
        {
            blurSigma = (float) v;
        };
        addAndMakeVisible (blurSlider.get());

        statusLabel = std::make_unique<yup::Label> ("status");
        statusLabel->setText ("Initializing GPU...", yup::dontSendNotification);
        addAndMakeVisible (statusLabel.get());
    }

    ~SpinningCubeDemo() override = default;

    //==============================================================================
    void refreshDisplay (double) override
    {
        angleY += 0.038f;
        angleX += 0.012f;
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
            yup::MessageManager::callAsync ([this]
            {
                initGpu();
            });
        }

        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        bounds.removeFromBottom (60.0f);

        const int w = (int) bounds.getWidth();
        const int h = (int) bounds.getHeight();

        if (w < 2 || h < 2 || oreCtx == nullptr || ! gpuInitialized)
            return;

        // 1. Create an empty GpuCanvas for the scene, commit the empty Rive 2D frame.
        auto sceneCanvas = yup::GpuCanvas::create (*capturedContext, w, h);
        if (sceneCanvas == nullptr)
            return;

        sceneCanvas->commit();

        // 2. Render the 3D cube into sceneCanvas via ore directly.
        if (cubePipeline != nullptr)
        {
            oreCtx->beginFrame ({});
            renderCube (*sceneCanvas, w, h);
            oreCtx->endFrame();
            oreCtx->waitForGPU();
        }

        // 3. Apply Gaussian blur via GpuProgram (when compiled and sigma > 0).
        yup::Texture::Ptr outputTex = sceneCanvas->asTexture();

        if (blurProgram != nullptr && blurSigma > 0.01f)
        {
            auto blurCanvas = yup::GpuCanvas::create (*capturedContext, w, h);
            if (blurCanvas != nullptr)
            {
                blurCanvas->commit();

                struct alignas (16) BlurParams
                {
                    float sigma;  // sigma in pixels
                    float radius; // normally ceil(sigma * 3.0), clamped on CPU
                    float resX;
                    float resY;
                };

                BlurParams params { blurSigma, (float) ceil (blurSigma * 3.0), (float) w, (float) h };

                blurProgram->setTexture (0, 0, outputTex);
                blurProgram->setUniformBuffer (0, 2, &params, sizeof (params));
                blurProgram->beginFrame();
                blurProgram->dispatch (*blurCanvas);
                blurProgram->endFrame();
                blurProgram->waitForGPU();

                outputTex = blurCanvas->asTexture();
            }
        }

        // 4. Composite to main view.
        if (outputTex != nullptr)
            g.drawTexture (outputTex, bounds);
    }

    //==============================================================================
    void resized() override
    {
        auto bounds = getLocalBounds().to<float>().reduced (10.0f);
        statusLabel->setBounds (bounds.removeFromBottom (25.0f));
        bounds.removeFromBottom (5.0f);
        blurSlider->setBounds (bounds.removeFromBottom (30.0f));
    }

private:
    //==============================================================================
    // ---- Cube geometry -------------------------------------------------------

    struct CubeVertex
    {
        float pos[3];
        float color[3];
        float normal[3];
    };

    static constexpr CubeVertex kCubeVerts[24] = {
        // Front (-Z), red
        { { -1, -1, -1 }, { 0.91f, 0.30f, 0.24f }, { 0, 0, -1 } },
        { { 1, -1, -1 }, { 0.91f, 0.30f, 0.24f }, { 0, 0, -1 } },
        { { 1, 1, -1 }, { 0.91f, 0.30f, 0.24f }, { 0, 0, -1 } },
        { { -1, 1, -1 }, { 0.91f, 0.30f, 0.24f }, { 0, 0, -1 } },
        // Back (+Z), blue
        { { 1, -1, 1 }, { 0.20f, 0.60f, 0.86f }, { 0, 0, 1 } },
        { { -1, -1, 1 }, { 0.20f, 0.60f, 0.86f }, { 0, 0, 1 } },
        { { -1, 1, 1 }, { 0.20f, 0.60f, 0.86f }, { 0, 0, 1 } },
        { { 1, 1, 1 }, { 0.20f, 0.60f, 0.86f }, { 0, 0, 1 } },
        // Left (-X), green
        { { -1, -1, 1 }, { 0.18f, 0.80f, 0.44f }, { -1, 0, 0 } },
        { { -1, -1, -1 }, { 0.18f, 0.80f, 0.44f }, { -1, 0, 0 } },
        { { -1, 1, -1 }, { 0.18f, 0.80f, 0.44f }, { -1, 0, 0 } },
        { { -1, 1, 1 }, { 0.18f, 0.80f, 0.44f }, { -1, 0, 0 } },
        // Right (+X), orange
        { { 1, -1, -1 }, { 0.95f, 0.61f, 0.07f }, { 1, 0, 0 } },
        { { 1, -1, 1 }, { 0.95f, 0.61f, 0.07f }, { 1, 0, 0 } },
        { { 1, 1, 1 }, { 0.95f, 0.61f, 0.07f }, { 1, 0, 0 } },
        { { 1, 1, -1 }, { 0.95f, 0.61f, 0.07f }, { 1, 0, 0 } },
        // Top (+Y), purple
        { { -1, 1, -1 }, { 0.61f, 0.35f, 0.71f }, { 0, 1, 0 } },
        { { 1, 1, -1 }, { 0.61f, 0.35f, 0.71f }, { 0, 1, 0 } },
        { { 1, 1, 1 }, { 0.61f, 0.35f, 0.71f }, { 0, 1, 0 } },
        { { -1, 1, 1 }, { 0.61f, 0.35f, 0.71f }, { 0, 1, 0 } },
        // Bottom (-Y), teal
        { { -1, -1, 1 }, { 0.10f, 0.74f, 0.61f }, { 0, -1, 0 } },
        { { 1, -1, 1 }, { 0.10f, 0.74f, 0.61f }, { 0, -1, 0 } },
        { { 1, -1, -1 }, { 0.10f, 0.74f, 0.61f }, { 0, -1, 0 } },
        { { -1, -1, -1 }, { 0.10f, 0.74f, 0.61f }, { 0, -1, 0 } },
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

    // ---- Shader reflection helpers -------------------------------------------

    static yup::ShaderLanguage shaderLanguageForApi (yup::GraphicsContext::Api api)
    {
        switch (api)
        {
            case yup::GraphicsContext::Api::Metal:
                return yup::ShaderLanguage::msl;
            case yup::GraphicsContext::Api::Direct3D:
                return yup::ShaderLanguage::hlsl;
            case yup::GraphicsContext::Api::OpenGLES:
                return yup::ShaderLanguage::essl;
            default:
                return yup::ShaderLanguage::glsl;
        }
    }

    static std::vector<uint8_t> buildBindingMapFromReflection (const yup::ShaderReflection& refl,
                                                               yup::ShaderStage stage)
    {
        using namespace rive::ore;

        const uint8_t stageMask = (stage == yup::ShaderStage::vertex)
                                    ? BindingMap::kStageVertex
                                    : BindingMap::kStageFragment;
        const int slotIndex = (stage == yup::ShaderStage::vertex) ? 0 : 1;

        BindingMap bm;

        for (const auto& ub : refl.uniformBuffers)
        {
            BindingMap::Entry e {};
            e.group = (uint8_t) ub.set;
            e.binding = (uint8_t) ub.binding;
            e.kind = ResourceKind::UniformBuffer;
            e.stageMask = stageMask;
            e.backendSlot[slotIndex] = (uint16_t) ub.backendSlot;
            bm.push (e);
        }

        for (const auto& img : refl.separateImages)
        {
            BindingMap::Entry e {};
            e.group = (uint8_t) img.set;
            e.binding = (uint8_t) img.binding;
            e.kind = ResourceKind::SampledTexture;
            e.stageMask = stageMask;
            e.backendSlot[slotIndex] = (uint16_t) img.backendSlot;
            e.textureViewDim = TextureViewDim::D2;
            e.textureSampleType = TextureSampleType::Float;
            e.textureMultisampled = false;
            bm.push (e);
        }

        for (const auto& samp : refl.separateSamplers)
        {
            BindingMap::Entry e {};
            e.group = (uint8_t) samp.set;
            e.binding = (uint8_t) samp.binding;
            e.kind = ResourceKind::Sampler;
            e.stageMask = stageMask;
            e.backendSlot[slotIndex] = (uint16_t) samp.backendSlot;
            bm.push (e);
        }

        bm.finalize();
        return bm.toBlob();
    }

    struct ShaderData
    {
        yup::String source;     // Native source: MSL for Metal, HLSL for D3D, GLSL otherwise
        yup::String entryPoint; // Entry point name in the native source
        yup::GpuShaderLanguage gpuLanguage = yup::GpuShaderLanguage::glsl;
        std::vector<uint8_t> bindingMap;
    };

    static std::optional<ShaderData> compileGlslShader (const yup::String& glslSource,
                                                        yup::ShaderStage stage,
                                                        yup::ShaderLanguage targetLang,
                                                        yup::ShaderTranspiler& transpiler)
    {
        // Step 1: GLSL → SPIR-V (always needed for reflection).
        auto spirvResult = transpiler.compileToSPIRV (glslSource, stage, yup::ShaderLanguage::glsl);
        if (spirvResult.failed())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: SPIR-V compile error: "
                                            + spirvResult.getErrorMessage());
            return std::nullopt;
        }

        // Step 2: reflect from SPIR-V with target backend language → native slot numbers.
        auto reflResult = transpiler.reflectFromSPIRV (spirvResult.getValue(), targetLang);
        if (reflResult.failed())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: reflection error: "
                                            + reflResult.getErrorMessage());
            return std::nullopt;
        }

        // Step 3: derive the native source that ore will actually compile.
        // Ore's Metal and D3D backends pass the source directly to the platform
        // compiler — they do NOT cross-compile from GLSL internally.
        yup::String nativeSource;
        yup::String entryPoint;
        yup::GpuShaderLanguage gpuLang;

        if (targetLang == yup::ShaderLanguage::msl)
        {
            auto mslResult = transpiler.decompileFromSPIRV (spirvResult.getValue(), yup::ShaderLanguage::msl);
            if (mslResult.failed())
            {
                yup::Logger::outputDebugString ("SpinningCubeDemo: MSL decompile error: "
                                                + mslResult.getErrorMessage());
                return std::nullopt;
            }
            nativeSource = mslResult.getValue();
            entryPoint = "main0"; // SPIRV-Cross always renames GLSL "main" → "main0" in MSL
            gpuLang = yup::GpuShaderLanguage::msl;
        }
        else if (targetLang == yup::ShaderLanguage::hlsl)
        {
            auto hlslResult = transpiler.decompileFromSPIRV (spirvResult.getValue(), yup::ShaderLanguage::hlsl);
            if (hlslResult.failed())
            {
                yup::Logger::outputDebugString ("SpinningCubeDemo: HLSL decompile error: "
                                                + hlslResult.getErrorMessage());
                return std::nullopt;
            }
            nativeSource = hlslResult.getValue();
            entryPoint = "main";
            gpuLang = yup::GpuShaderLanguage::hlsl;
        }
        else if (targetLang == yup::ShaderLanguage::essl)
        {
            auto esslResult = transpiler.decompileFromSPIRV (spirvResult.getValue(), yup::ShaderLanguage::essl);
            if (esslResult.failed())
            {
                yup::Logger::outputDebugString ("SpinningCubeDemo: ESSL decompile error: "
                                                + esslResult.getErrorMessage());
                return std::nullopt;
            }
            nativeSource = esslResult.getValue();
            entryPoint = "main";
            gpuLang = yup::GpuShaderLanguage::glsl;
        }
        else
        {
            nativeSource = glslSource;
            entryPoint = "main";
            gpuLang = yup::GpuShaderLanguage::glsl;
        }

        return ShaderData { std::move (nativeSource),
                            std::move (entryPoint),
                            gpuLang,
                            buildBindingMapFromReflection (reflResult.getValue(), stage) };
    }

    // ---- GPU initialisation --------------------------------------------------

    void initGpu()
    {
        if (capturedContext == nullptr || gpuInitialized)
            return;

        gpuInitialized = true;
        oreCtx = capturedContext->gpuContext();

        if (oreCtx == nullptr)
        {
            statusLabel->setText ("ore context unavailable - rebuild with enableOreContext=true.",
                                  yup::dontSendNotification);
            return;
        }

        transpiler = new yup::ShaderTranspiler();
        targetShaderLang = shaderLanguageForApi (capturedContext->getApi());

        initBlur();
        initCube();

        if (cubePipeline != nullptr && blurProgram != nullptr)
            statusLabel->setText ("GPU cube + blur ready. Drag slider for blur intensity.", yup::dontSendNotification);
        else if (cubePipeline != nullptr)
            statusLabel->setText ("GPU cube ready. Blur compile failed - see debug log.", yup::dontSendNotification);
        else
            statusLabel->setText ("GPU init failed - see debug log.", yup::dontSendNotification);
    }

    void initBlur()
    {
        // GLSL 450 blur vertex shader: fullscreen triangle from vertex index, no vertex buffer.
        static constexpr char kBlurVS[] = R"glsl(
#version 450
void main() {
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)glsl";

        // GLSL 450 blur fragment shader: 2-D Gaussian using separate texture + sampler.
        // Bindings: set=0 binding=0 = scene texture (separate image)
        //           set=0 binding=1 = sampler
        //           set=0 binding=2 = BlurParams UBO
        static constexpr char kBlurFS[] = R"glsl(
#version 450
layout(set = 0, binding = 0) uniform texture2D u_tex;
layout(set = 0, binding = 1) uniform sampler   u_samp;
layout(set = 0, binding = 2) uniform BlurParams {
    float sigma; float radius; float resX; float resY;
} p;
layout(location = 0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    if (p.sigma <= 0.0001) { fragColor = texture(sampler2D(u_tex, u_samp), uv); return; }
    int   radius = int(clamp(p.radius, 1.0, 32.0));
    vec2  ts     = 1.0 / vec2(p.resX, p.resY);
    float inv2s2 = 0.5 / (p.sigma * p.sigma);
    vec4  sum = vec4(0.0); float wsum = 0.0;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            vec2  off = vec2(float(x), float(y));
            float w   = exp(-dot(off, off) * inv2s2);
            sum  += texture(sampler2D(u_tex, u_samp), uv + off * ts) * w;
            wsum += w;
        }
    }
    fragColor = sum / wsum;
}
)glsl";

        auto vsData = compileGlslShader (kBlurVS, yup::ShaderStage::vertex, targetShaderLang, *transpiler);
        auto fsData = compileGlslShader (kBlurFS, yup::ShaderStage::fragment, targetShaderLang, *transpiler);

        if (! vsData.has_value() || ! fsData.has_value())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: blur shader compile failed.");
            return;
        }

        auto vsCode = vsData->source.toRawUTF8();
        auto fsCode = fsData->source.toRawUTF8();

        yup::GpuShaderSource blurVS;
        blurVS.language = vsData->gpuLanguage;
        blurVS.code = vsCode;
        blurVS.codeSize = (uint32_t) strlen (vsCode);
        blurVS.entryPoint = vsData->entryPoint.toRawUTF8();
        blurVS.bindingMap = vsData->bindingMap.data();
        blurVS.bindingMapSize = (uint32_t) vsData->bindingMap.size();

        yup::GpuShaderSource blurFS;
        blurFS.language = fsData->gpuLanguage;
        blurFS.code = fsCode;
        blurFS.codeSize = (uint32_t) strlen (fsCode);
        blurFS.entryPoint = fsData->entryPoint.toRawUTF8();
        blurFS.bindingMap = fsData->bindingMap.data();
        blurFS.bindingMapSize = (uint32_t) fsData->bindingMap.size();

        std::string err;
        blurProgram = yup::GpuProgram::compile (*capturedContext, blurVS, blurFS, &err);

        if (blurProgram == nullptr)
            yup::Logger::outputDebugString ("SpinningCubeDemo: blur compile failed: " + yup::String (err.c_str()));
    }

    void initCube()
    {
        // GLSL 450 cube vertex shader: rotation (Y then X) + inline perspective.
        // Uniform: set=0 binding=0  CubeUniforms { angleY, angleX, aspect, pad }
        // Attributes: location=0 pos, location=1 color, location=2 normal
        static constexpr char kCubeVS[] = R"glsl(
#version 450
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec3 a_normal;
layout(set = 0, binding = 0) uniform CubeUniforms {
    float angleY; float angleX; float aspect; float pad;
} u;
layout(location = 0) out vec3 v_color;
layout(location = 1) out vec3 v_normal;
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
}
)glsl";

        // GLSL 450 cube fragment shader: diffuse shading. No resource bindings.
        static constexpr char kCubeFS[] = R"glsl(
#version 450
layout(location = 0) in vec3 v_color;
layout(location = 1) in vec3 v_normal;
layout(location = 0) out vec4 fragColor;
void main() {
    vec3  light = normalize(vec3(0.503, 0.671, -0.419));
    float ndotl = clamp(dot(normalize(v_normal), light), 0.0, 1.0);
    fragColor   = vec4(v_color * (0.35 + 0.65 * ndotl), 1.0);
}
)glsl";

        auto vsData = compileGlslShader (kCubeVS, yup::ShaderStage::vertex, targetShaderLang, *transpiler);
        auto fsData = compileGlslShader (kCubeFS, yup::ShaderStage::fragment, targetShaderLang, *transpiler);

        if (! vsData.has_value() || ! fsData.has_value())
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: cube shader compile failed.");
            return;
        }

        auto vsCode = vsData->source.toRawUTF8();
        auto fsCode = fsData->source.toRawUTF8();

        // Compile vertex shader module.
        rive::ore::ShaderModuleDesc vsd;
        vsd.language = rive::ore::ShaderLanguage::glsl;
        vsd.code = vsCode;
        vsd.codeSize = (uint32_t) strlen (vsCode);
        vsd.stage = rive::ore::ShaderStage::vertex;
        vsd.label = "Cube VS";
        vsd.bindingMapBytes = vsData->bindingMap.data();
        vsd.bindingMapSize = (uint32_t) vsData->bindingMap.size();
        cubeVertModule = oreCtx->makeShaderModule (vsd);

        // Compile fragment shader module.
        rive::ore::ShaderModuleDesc fsd;
        fsd.language = rive::ore::ShaderLanguage::glsl;
        fsd.code = fsCode;
        fsd.codeSize = (uint32_t) strlen (fsCode);
        fsd.stage = rive::ore::ShaderStage::fragment;
        fsd.label = "Cube FS";
        fsd.bindingMapBytes = fsData->bindingMap.data();
        fsd.bindingMapSize = (uint32_t) fsData->bindingMap.size();
        cubeFragModule = oreCtx->makeShaderModule (fsd);

        if (cubeVertModule == nullptr || cubeFragModule == nullptr)
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: cube shader compile failed: " + yup::String (oreCtx->lastError().c_str()));
            return;
        }

        // Bind-group layout: @group(0) @binding(0) = UBO, VS-only, Metal buffer(0).
        rive::ore::BindGroupLayoutEntry bglEntry;
        bglEntry.binding = 0;
        bglEntry.kind = rive::ore::BindingKind::uniformBuffer;
        bglEntry.visibility.mask = rive::ore::StageVisibility::kVertex;
        bglEntry.nativeSlotVS = 0;
        bglEntry.nativeSlotFS = rive::ore::BindGroupLayoutEntry::kNativeSlotAbsent;
        bglEntry.nativeSlotCS = rive::ore::BindGroupLayoutEntry::kNativeSlotAbsent;

        rive::ore::BindGroupLayoutDesc bglDesc;
        bglDesc.groupIndex = 0;
        bglDesc.entries = &bglEntry;
        bglDesc.entryCount = 1;
        bglDesc.label = "Cube BGL";
        cubeLayout = oreCtx->makeBindGroupLayout (bglDesc);

        if (cubeLayout == nullptr)
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: cube BGL creation failed.");
            return;
        }

        // Vertex attribute layout: pos(float3,0), color(float3,12), normal(float3,24).
        rive::ore::VertexAttribute attrs[3] = {
            { rive::ore::VertexFormat::float3, 0, 0 },
            { rive::ore::VertexFormat::float3, 12, 1 },
            { rive::ore::VertexFormat::float3, 24, 2 },
        };
        rive::ore::VertexBufferLayout vbLayout;
        vbLayout.stride = sizeof (CubeVertex); // 36 bytes
        vbLayout.stepMode = rive::ore::VertexStepMode::vertex;
        vbLayout.attributes = attrs;
        vbLayout.attributeCount = 3;

        // Pipeline.
        rive::ore::BindGroupLayout* layoutPtr = cubeLayout.get();

        rive::ore::PipelineDesc pipeDesc;
        pipeDesc.vertexModule = cubeVertModule.get();
        pipeDesc.vertexEntryPoint = vsData->entryPoint.toRawUTF8();
        pipeDesc.fragmentModule = cubeFragModule.get();
        pipeDesc.fragmentEntryPoint = fsData->entryPoint.toRawUTF8();
        pipeDesc.vertexBuffers = &vbLayout;
        pipeDesc.vertexBufferCount = 1;
        pipeDesc.topology = rive::ore::PrimitiveTopology::triangleList;
        pipeDesc.indexFormat = rive::ore::IndexFormat::uint16;
        pipeDesc.cullMode = rive::ore::CullMode::back;
        pipeDesc.winding = rive::ore::FaceWinding::counterClockwise;
        pipeDesc.colorCount = 1;
        pipeDesc.colorTargets[0].format = rive::ore::TextureFormat::rgba8unorm;
        pipeDesc.colorTargets[0].blendEnabled = false;
        pipeDesc.bindGroupLayouts = &layoutPtr;
        pipeDesc.bindGroupLayoutCount = 1;
        pipeDesc.label = "Cube Pipeline";

        std::string pipeError;
        cubePipeline = oreCtx->makePipeline (pipeDesc, &pipeError);

        if (cubePipeline == nullptr)
        {
            yup::Logger::outputDebugString ("SpinningCubeDemo: cube pipeline failed: "
                                            + yup::String (pipeError.c_str()));
            return;
        }

        // Upload immutable vertex and index buffers.
        rive::ore::BufferDesc vboDesc;
        vboDesc.usage = rive::ore::BufferUsage::vertex;
        vboDesc.size = sizeof (kCubeVerts);
        vboDesc.data = kCubeVerts;
        vboDesc.immutable = true;
        cubeVBO = oreCtx->makeBuffer (vboDesc);

        rive::ore::BufferDesc iboDesc;
        iboDesc.usage = rive::ore::BufferUsage::index;
        iboDesc.size = sizeof (kCubeIdx);
        iboDesc.data = kCubeIdx;
        iboDesc.immutable = true;
        cubeIBO = oreCtx->makeBuffer (iboDesc);

        if (cubeVBO == nullptr || cubeIBO == nullptr)
            yup::Logger::outputDebugString ("SpinningCubeDemo: cube buffer creation failed.");
    }

    // ---- Per-frame cube render pass -----------------------------------------

    void renderCube (yup::GpuCanvas& canvas, int w, int h)
    {
        if (cubePipeline == nullptr || cubeVBO == nullptr || cubeIBO == nullptr || cubeLayout == nullptr)
            return;

        // Upload per-frame uniform buffer (rotation angles + aspect ratio).
        struct CubeUniforms
        {
            float angleY, angleX, aspect, pad;
        };

        CubeUniforms uniforms { angleY, angleX, (float) w / (float) h, 0.0f };

        rive::ore::BufferDesc ubDesc;
        ubDesc.usage = rive::ore::BufferUsage::uniform;
        ubDesc.size = sizeof (uniforms);
        ubDesc.data = &uniforms;
        ubDesc.immutable = true;
        auto ubo = oreCtx->makeBuffer (ubDesc);
        if (ubo == nullptr)
            return;

        // Bind group.
        rive::ore::BindGroupDesc::UBOEntry uboEntry;
        uboEntry.slot = 0; // WGSL @binding(0)
        uboEntry.buffer = ubo.get();
        uboEntry.offset = 0;
        uboEntry.size = sizeof (uniforms);

        rive::ore::BindGroupDesc bgDesc;
        bgDesc.layout = cubeLayout.get();
        bgDesc.ubos = &uboEntry;
        bgDesc.uboCount = 1;
        bgDesc.textures = nullptr;
        bgDesc.textureCount = 0;
        bgDesc.samplers = nullptr;
        bgDesc.samplerCount = 0;
        auto bg = oreCtx->makeBindGroup (bgDesc);
        if (bg == nullptr)
            return;

        // Encode the render pass targeting sceneCanvas's backing texture.
        canvas.withOreAttachment (oreCtx, [&] (rive::ore::TextureView* view)
        {
            rive::ore::RenderPassDesc rpDesc;
            rpDesc.colorCount = 1;
            rpDesc.colorAttachments[0].view = view;
            rpDesc.colorAttachments[0].loadOp = rive::ore::LoadOp::clear;
            rpDesc.colorAttachments[0].storeOp = rive::ore::StoreOp::store;
            rpDesc.colorAttachments[0].clearColor = { 0.1f, 0.1f, 0.18f, 1.0f };

            auto rp = oreCtx->beginRenderPass (rpDesc);
            rp->setPipeline (cubePipeline.get());
            rp->setVertexBuffer (0, cubeVBO.get(), 0);
            rp->setIndexBuffer (cubeIBO.get(), rive::ore::IndexFormat::uint16, 0);
            rp->setBindGroup (0, bg.get());
            rp->setViewport (0.0f, 0.0f, (float) w, (float) h);
            rp->drawIndexed (36);
            rp->finish();
        });
    }

    //==============================================================================
    yup::GraphicsContext* capturedContext = nullptr;
    rive::ore::Context* oreCtx = nullptr;

    yup::ShaderTranspiler::Ptr transpiler;
    yup::ShaderLanguage targetShaderLang = yup::ShaderLanguage::glsl;

    // Blur pass (GpuProgram fullscreen triangle).
    yup::GpuProgram::Ptr blurProgram;

    // Cube pass (ore direct).
    rive::rcp<rive::ore::ShaderModule> cubeVertModule;
    rive::rcp<rive::ore::ShaderModule> cubeFragModule;
    rive::rcp<rive::ore::Pipeline> cubePipeline;
    rive::rcp<rive::ore::BindGroupLayout> cubeLayout;
    rive::rcp<rive::ore::Buffer> cubeVBO;
    rive::rcp<rive::ore::Buffer> cubeIBO;

    bool gpuInitialized = false;
    float angleY = 0.0f;
    float angleX = 0.15f;
    float blurSigma = 0.0f;

    std::unique_ptr<yup::Slider> blurSlider;
    std::unique_ptr<yup::Label> statusLabel;
};
