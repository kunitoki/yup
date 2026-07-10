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

namespace yup
{

namespace
{

#if YUP_ENABLE_SHADER_TRANSPILER
// Fullscreen matte-composite shader. Samples the offscreen-rendered target and
// matte-source textures (both premultiplied-alpha, UV.y=0=top on every backend)
// and writes target * f(source), where f depends on the matte type:
//   mode 0 (Alpha)    : source.a
//   mode 1 (AlphaInv) : 1 - source.a
//   mode 2 (Luma)     : luma(source.rgb)   [source is premultiplied]
//   mode 3 (LumaInv)  : 1 - luma(source.rgb)
// Because both inputs are premultiplied, multiplying all four channels by the
// coverage factor yields a correctly premultiplied result.
constexpr char kMatteVertSource[] = R"glsl(#version 450
void main() {
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)glsl";

constexpr char kMatteFragSource[] = R"glsl(#version 450
layout(set = 0, binding = 0) uniform texture2D u_target;
layout(set = 0, binding = 1) uniform texture2D u_source;
layout(set = 0, binding = 2) uniform sampler   u_samp;
layout(set = 0, binding = 3) uniform MatteParams {
    float mode; float resX; float resY; float pad;
} p;
layout(location = 0) out vec4 fragColor;
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.resX, p.resY);
    vec4 tgt = texture(sampler2D(u_target, u_samp), uv);
    vec4 src = texture(sampler2D(u_source, u_samp), uv);
    float luma = dot(src.rgb, vec3(0.299, 0.587, 0.114));
    float coverage;
    if      (p.mode < 0.5) coverage = src.a;             // Alpha
    else if (p.mode < 1.5) coverage = 1.0 - src.a;       // AlphaInv
    else if (p.mode < 2.5) coverage = luma;              // Luma
    else                   coverage = 1.0 - luma;        // LumaInv
    fragColor = tgt * coverage;
}
)glsl";
#endif

} // namespace

//==============================================================================

GpuPipeline::Ptr AnimationRenderResources::getMattePipeline (GraphicsContext& context)
{
    if (mattePipelineCompiled_)
        return mattePipeline_;

    mattePipelineCompiled_ = true;

#if YUP_ENABLE_SHADER_TRANSPILER
    // The fragment shader outputs the final premultiplied-alpha pixel
    // (target * coverage). The pass writes into a freshly-cleared target, so it is
    // a pure overwrite: blending must be disabled, otherwise the default
    // src-alpha / one-minus-src-alpha blend would premultiply the RGB a second
    // time and darken the result (white matte -> grey).
    GpuPipelineOptions options;
    options.colorTargetCount = 1;
    options.colorTargets[0].format = GpuTextureFormat::rgba8unorm;
    options.colorTargets[0].blendEnabled = false;

    auto result = GpuPipeline::compileFromGlsl (context,
                                                String::fromUTF8 (kMatteVertSource, (int) sizeof (kMatteVertSource) - 1),
                                                String::fromUTF8 (kMatteFragSource, (int) sizeof (kMatteFragSource) - 1),
                                                options);
    if (result.failed())
    {
        jassertfalse; // Matte-composite shader failed to compile - callers fall back to geometric clip.
        return nullptr;
    }

    mattePipeline_ = result.getValue();
#else
    ignoreUnused (context);
#endif

    return mattePipeline_;
}

void AnimationRenderResources::reset()
{
    mattePipeline_ = nullptr;
    mattePipelineCompiled_ = false;
}

} // namespace yup
