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

#include <gtest/gtest.h>

#include <set>

#include <yup_graphics/yup_graphics.h>

#if YUP_ENABLE_SHADER_COMPILER

using namespace yup;

//==============================================================================
namespace
{

// Minimal valid shaders for testing

constexpr const char* kMinimalVertexGLSL = R"glsl(
#version 450
void main()
{
    gl_Position = vec4(0.0);
}
)glsl";

constexpr const char* kMinimalFragmentGLSL = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(1.0);
}
)glsl";

constexpr const char* kMinimalComputeGLSL = R"glsl(
#version 450
layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
void main()
{
}
)glsl";

constexpr const char* kFragmentWithUniforms = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(std140, binding = 0) uniform UBO {
    layout(offset = 0) vec4 tint;
    layout(offset = 16) float scale;
} ubo;
layout(binding = 1) uniform sampler2D tex;
layout(location = 0) in vec2 vUV;
void main()
{
    outColor = texture(tex, vUV) * ubo.tint * ubo.scale;
}
)glsl";

constexpr const char* kVertexWithDefines = R"glsl(
#version 450
void main()
{
#if defined(FOO) && FOO == 42
    gl_Position = vec4(1.0);
#else
    gl_Position = vec4(0.0);
#endif
}
)glsl";

constexpr const char* kESSLFragment = R"glsl(
#version 310 es
precision mediump float;
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)glsl";

constexpr const char* kMinimalHLSL = R"hlsl(
float4 main() : SV_POSITION
{
    return float4(0, 0, 0, 1);
}
)hlsl";

constexpr const char* kInvalidGLSL = R"glsl(
#version 450
void main()
{
    gl_Position = unknown_variable;
}
)glsl";

constexpr const char* kGeometryGLSL = R"glsl(
#version 450
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
void main()
{
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();
}
)glsl";

constexpr const char* kTessControlGLSL = R"glsl(
#version 450
layout(vertices = 3) out;
void main()
{
    gl_TessLevelOuter[0] = 1.0;
    gl_TessLevelOuter[1] = 1.0;
    gl_TessLevelOuter[2] = 1.0;
    gl_TessLevelInner[0] = 1.0;
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)glsl";

constexpr const char* kTessEvalGLSL = R"glsl(
#version 450
layout(triangles, equal_spacing, cw) in;
void main()
{
    gl_Position = gl_TessCoord.x * gl_in[0].gl_Position
                + gl_TessCoord.y * gl_in[1].gl_Position
                + gl_TessCoord.z * gl_in[2].gl_Position;
}
)glsl";

constexpr const char* kFragmentWithIntUintInputs = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(location = 0) flat in int intVal;
layout(location = 1) flat in uint uintVal;
void main()
{
    outColor = vec4(float(intVal), float(uintVal), 0.0, 1.0);
}
)glsl";

constexpr const char* kFragmentWithStorageBuffer = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(std430, binding = 0) buffer StorageBuf {
    float values[];
} sb;
void main()
{
    outColor = vec4(sb.values[0], 0.0, 0.0, 1.0);
}
)glsl";

constexpr const char* kFragmentWithArrayUBO = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(std140, binding = 0) uniform ArrayUBO {
    vec4 colors[4];
} ubo;
void main()
{
    outColor = ubo.colors[0] + ubo.colors[1] + ubo.colors[2] + ubo.colors[3];
}
)glsl";

constexpr const char* kFragmentWithMultiTextures = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D tex2D;
layout(binding = 1) uniform sampler3D tex3D;
layout(binding = 2) uniform samplerCube texCube;
layout(binding = 3) uniform sampler2DShadow texShadow;
void main()
{
    float s = texture(tex2D, vec2(0.5)).r;
    s += texture(tex3D, vec3(0.5)).r;
    s += texture(texCube, vec3(0.5)).r;
    s += texture(texShadow, vec3(0.5, 0.5, 0.5));
    outColor = vec4(vec3(s), 1.0);
}
)glsl";

constexpr const char* kFragmentWithTextureArray = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D textures[3];
layout(location = 0) in vec2 vUV;
void main()
{
    outColor = texture(textures[0], vUV) + texture(textures[1], vUV);
}
)glsl";

constexpr const char* kFragmentWithSpecConst = R"glsl(
#version 450
layout(constant_id = 0) const float specConst = 1.0;
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(specConst);
}
)glsl";

constexpr const char* kFragmentWithUBOArray = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(std140, binding = 0) uniform MyUBO {
    vec4 color;
} ubos[3];
void main()
{
    outColor = ubos[0].color + ubos[1].color + ubos[2].color;
}
)glsl";

constexpr const char* kVertexWithIndices = R"glsl(
#version 450
void main()
{
    gl_Position = vec4(float(gl_VertexIndex), float(gl_InstanceIndex), 0.0, 1.0);
}
)glsl";

constexpr const char* kFragmentWithFragCoord = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(gl_FragCoord.xy / 100.0, float(gl_FrontFacing), 1.0);
}
)glsl";

constexpr const char* kComputeWithBuiltins = R"glsl(
#version 450
layout(local_size_x = 8, local_size_y = 1, local_size_z = 1) in;
layout(binding = 0, rgba8) uniform writeonly image2D img;
void main()
{
    vec4 val = vec4(float(gl_NumWorkGroups.x), float(gl_WorkGroupID.x),
                    float(gl_LocalInvocationID.x), float(gl_LocalInvocationIndex));
    imageStore(img, ivec2(gl_GlobalInvocationID.xy), val);
}
)glsl";

constexpr const char* kFragmentWithSeparateSampler = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform texture2D tex;
layout(binding = 1) uniform sampler samp;
void main()
{
    outColor = texture(sampler2D(tex, samp), vec2(0.5));
}
)glsl";

constexpr const char* kVertexWithClipCullDistance = R"glsl(
#version 450
out float gl_ClipDistance[2];
out float gl_CullDistance[2];
void main()
{
    gl_Position = vec4(0.0);
    gl_ClipDistance[0] = 1.0;
    gl_CullDistance[0] = 1.0;
}
)glsl";

constexpr const char* kGeometryWithPrimitiveId = R"glsl(
#version 450
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
void main()
{
    gl_Position = gl_in[gl_PrimitiveIDIn].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();
}
)glsl";

constexpr const char* kVertexWithPointSize = R"glsl(
#version 450
void main()
{
    gl_Position = vec4(0.0);
    gl_PointSize = 1.0;
}
)glsl";

constexpr const char* kFragmentWithFragDepth = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(1.0);
    gl_FragDepth = 0.5;
}
)glsl";

constexpr const char* kGeometryWithLayer = R"glsl(
#version 450
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
void main()
{
    gl_Layer = 0;
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();
}
)glsl";

constexpr const char* kFragmentWithPointCoord = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(gl_PointCoord, 0.0, 1.0);
}
)glsl";

constexpr const char* kTessControlWithPatchVertices = R"glsl(
#version 450
layout(vertices = 3) out;
void main()
{
    gl_TessLevelOuter[0] = float(gl_PatchVerticesIn);
    gl_TessLevelOuter[1] = 1.0;
    gl_TessLevelOuter[2] = 1.0;
    gl_TessLevelInner[0] = 1.0;
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)glsl";

constexpr const char* kFragmentWithHelperInvocation = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    if (gl_HelperInvocation)
        outColor = vec4(0.0);
    else
        outColor = vec4(1.0);
}
)glsl";

constexpr const char* kFragmentWithSampleMask = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(1.0);
    gl_SampleMask[0] = 1;
}
)glsl";

constexpr const char* kFragmentWithBoolUBO = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(std140, binding = 0) uniform BoolUBO {
    layout(offset = 0) bool flag;
    layout(offset = 4) float value;
} ubo;
void main()
{
    outColor = ubo.flag ? vec4(ubo.value) : vec4(0.0);
}
)glsl";

constexpr const char* kFragmentWithSampler1D = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler1D tex1D;
void main()
{
    outColor = vec4(texture(tex1D, 0.5).r);
}
)glsl";

constexpr const char* kFragmentWithExtensions = R"glsl(
#version 450
#extension GL_EXT_shader_16bit_storage : enable
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(1.0);
}
)glsl";

constexpr const char* kFragmentWithSampleIdAndPos = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(float(gl_SampleID), gl_SamplePosition.x, 0.0, 1.0);
}
)glsl";

constexpr const char* kVertexWithDrawParams = R"glsl(
#version 450
void main()
{
    gl_Position = vec4(float(gl_BaseVertex), float(gl_BaseInstance), float(gl_DrawID), 1.0);
}
)glsl";

constexpr const char* kFragmentWithImageBuffer = R"glsl(
#version 450
#extension GL_EXT_texture_buffer : enable
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform samplerBuffer buf;
void main()
{
    outColor = vec4(texelFetch(buf, 0).r);
}
)glsl";

constexpr const char* kFragmentWithSubpassInput = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
layout(input_attachment_index = 0, binding = 0) uniform subpassInput subpass;
void main()
{
    outColor = subpassLoad(subpass);
}
)glsl";

constexpr const char* kFragmentWith8BitStorage = R"glsl(
#version 450
#extension GL_EXT_shader_8bit_storage : enable
layout(location = 0) out vec4 outColor;
layout(std430, binding = 0) buffer StorageBuf8 {
    layout(offset = 0) int8_t i8;
    layout(offset = 1) uint8_t u8;
} sb;
void main()
{
    outColor = vec4(float(sb.i8), float(sb.u8), 0.0, 1.0);
}
)glsl";

constexpr const char* kFragmentWith16BitStorage = R"glsl(
#version 450
#extension GL_EXT_shader_16bit_storage : enable
layout(location = 0) out vec4 outColor;
layout(std430, binding = 0) buffer StorageBuf16 {
    layout(offset = 0) int16_t i16;
    layout(offset = 2) uint16_t u16;
    layout(offset = 4) float16_t f16;
} sb;
void main()
{
    outColor = vec4(float(sb.i16), float(sb.u16), float(sb.f16), 1.0);
}
)glsl";

constexpr const char* kFragmentWithInt64 = R"glsl(
#version 450
#extension GL_ARB_gpu_shader_int64 : enable
layout(location = 0) out vec4 outColor;
layout(std140, binding = 0) uniform Int64UBO {
    layout(offset = 0) int64_t i64;
    layout(offset = 8) uint64_t u64;
} ubo;
void main()
{
    outColor = vec4(float(ubo.i64) * 1e-9, float(ubo.u64) * 1e-9, 0.0, 1.0);
}
)glsl";

constexpr const char* kFragmentWithDouble = R"glsl(
#version 450
#extension GL_ARB_gpu_shader_fp64 : enable
layout(location = 0) out vec4 outColor;
layout(std140, binding = 0) uniform DoubleUBO {
    layout(offset = 0) double dVal;
} ubo;
void main()
{
    outColor = vec4(float(ubo.dVal), 0.0, 0.0, 1.0);
}
)glsl";

} // namespace

//==============================================================================
// ShaderTranspiler tests
//==============================================================================

class ShaderTranspilerTests : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        transpiler = new ShaderTranspiler();
        ASSERT_NE (transpiler, nullptr);
    }

    static void TearDownTestSuite()
    {
        transpiler = nullptr;
    }

    static ShaderTranspiler::Ptr transpiler;
};

ShaderTranspiler::Ptr ShaderTranspilerTests::transpiler {};

//==============================================================================
// compileToSPIRV
//==============================================================================

TEST_F (ShaderTranspilerTests, CompileToSPIRV_ValidVertexShader)
{
    auto result = transpiler->compileToSPIRV (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_ValidFragmentShader)
{
    auto result = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_ValidComputeShader)
{
    auto result = transpiler->compileToSPIRV (
        kMinimalComputeGLSL, ShaderStage::compute, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_GeometryShader)
{
    auto result = transpiler->compileToSPIRV (
        kGeometryGLSL, ShaderStage::geometry, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_WithDefines)
{
    TranspileOptions opts;
    HashMap<String, String> defines;
    defines.set ("FOO", "42");
    opts.defines = std::move (defines);

    auto result = transpiler->compileToSPIRV (
        kVertexWithDefines, ShaderStage::vertex, ShaderLanguage::glsl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_EmptyDefine)
{
    TranspileOptions opts;
    HashMap<String, String> defines;
    defines.set ("SOME_MACRO", "");
    opts.defines = std::move (defines);

    auto result = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_ESSLSource)
{
    // ESSL → SPIR-V compilation is broken in the bundled glslang version
    // (built-in parser fails due to array size 0 in ES builtins).
    // ESSL transpilation (via GLSL → SPIR-V → ESSL decompilation) still works
    // through the transpile() method.
    GTEST_SKIP() << "ESSL → SPIR-V not supported by bundled glslang version";
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_HLSLSource)
{
    auto result = transpiler->compileToSPIRV (
        kMinimalHLSL, ShaderStage::vertex, ShaderLanguage::hlsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_WithEntryPoint)
{
    TranspileOptions opts;
    opts.entryPoint = "main";

    auto result = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, opts);

    ASSERT_TRUE (result.wasOk());
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_InvalidSourceFails)
{
    auto result = transpiler->compileToSPIRV (
        kInvalidGLSL, ShaderStage::vertex, ShaderLanguage::glsl);

    EXPECT_TRUE (result.failed());
    EXPECT_TRUE (result.getErrorMessage().isNotEmpty());
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_EmptySourceFails)
{
    auto result = transpiler->compileToSPIRV (
        "", ShaderStage::vertex, ShaderLanguage::glsl);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_UnsupportedLanguageFails)
{
    auto result = transpiler->compileToSPIRV (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::msl);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_TessControlShader)
{
    auto result = transpiler->compileToSPIRV (
        kTessControlGLSL, ShaderStage::tessControl, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

TEST_F (ShaderTranspilerTests, CompileToSPIRV_TessEvalShader)
{
    auto result = transpiler->compileToSPIRV (
        kTessEvalGLSL, ShaderStage::tessEval, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_GT (result.getValue().getSize(), sizeof (uint32_t) * 5u);
}

//==============================================================================
// decompileFromSPIRV
//==============================================================================

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToGLSL)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("main"));
    EXPECT_TRUE (output.contains ("#version"));
    EXPECT_FALSE (output.contains ("metal"));
    EXPECT_FALSE (output.contains ("float4"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToESSL)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::essl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("main"));
    EXPECT_TRUE (output.contains ("#version"));
    EXPECT_TRUE (output.contains ("es"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToHLSL)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::hlsl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("main"));
    EXPECT_FALSE (output.contains ("#version"));
    EXPECT_FALSE (output.contains ("gl_Position"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("main0"));
    EXPECT_FALSE (output.contains ("#version"));
    EXPECT_TRUE (output.contains ("metal"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_InvalidBinaryFails)
{
    const uint8_t garbage[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
    MemoryBlock invalidSpirv (garbage, sizeof (garbage));

    auto result = transpiler->decompileFromSPIRV (
        invalidSpirv, ShaderLanguage::glsl);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_EmptyBinaryFails)
{
    MemoryBlock empty;

    auto result = transpiler->decompileFromSPIRV (
        empty, ShaderLanguage::glsl);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_TooSmallBinaryFails)
{
    const uint32_t tooSmall[] = { 0x07230203, 0x00010000 };
    MemoryBlock smallSpirv (tooSmall, sizeof (tooSmall));

    auto result = transpiler->decompileFromSPIRV (
        smallSpirv, ShaderLanguage::glsl);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_FlipVertY)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    TranspileOptions opts;
    opts.flipVertY = true;

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::glsl, opts);

    ASSERT_TRUE (result.wasOk());
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_GLSLVersion)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    TranspileOptions opts;
    opts.glslVersion = 330;

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::glsl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (result.getValue().contains ("330"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_UnsupportedTargetFails)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::spirv);

    EXPECT_TRUE (result.failed());
    EXPECT_TRUE (result.getErrorMessage().isNotEmpty());
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToGLSL_TessControl)
{
    auto spirv = transpiler->compileToSPIRV (
        kTessControlGLSL, ShaderStage::tessControl, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("main"));
    EXPECT_TRUE (output.contains ("#version"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL_TessControl)
{
    auto spirv = transpiler->compileToSPIRV (
        kTessControlGLSL, ShaderStage::tessControl, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_FALSE (output.isEmpty());
    EXPECT_TRUE (output.contains ("metal"));
    EXPECT_TRUE (output.contains ("kernel"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToGLSL_TessEval)
{
    auto spirv = transpiler->compileToSPIRV (
        kTessEvalGLSL, ShaderStage::tessEval, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("main"));
    EXPECT_TRUE (output.contains ("#version"));
}

//==============================================================================
// MSL-specific decompilation
//==============================================================================

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL_FlipVertY)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    TranspileOptions opts;
    opts.flipVertY = true;

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
    EXPECT_TRUE (result.getValue().contains ("metal"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL_VertexShader)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("#include <metal_stdlib>"));
    EXPECT_TRUE (output.contains ("using namespace metal;"));
    EXPECT_TRUE (output.contains ("[[position]]"));
    EXPECT_TRUE (output.contains ("vertex"));
    EXPECT_TRUE (output.contains ("metal"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL_FragmentShader)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("#include <metal_stdlib>"));
    EXPECT_TRUE (output.contains ("using namespace metal;"));
    EXPECT_TRUE (output.contains ("[[color(0)]]"));
    EXPECT_TRUE (output.contains ("fragment"));
    EXPECT_TRUE (output.contains ("metal"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL_FramebufferFetch)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    TranspileOptions opts;
    opts.mslUsesFramebufferFetch = true;

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL_ComputeShader)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalComputeGLSL, ShaderStage::compute, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("kernel"));
    EXPECT_TRUE (output.contains ("metal"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL_WithEntryPoint)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    TranspileOptions opts;
    opts.entryPoint = "main";

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

//==============================================================================
// transpile
//==============================================================================

TEST_F (ShaderTranspilerTests, Transpile_GLSLToGLSL)
{
    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("main"));
    EXPECT_TRUE (output.contains ("#version"));
    EXPECT_FALSE (output.contains ("metal"));
}

TEST_F (ShaderTranspilerTests, Transpile_GLSLToMSL)
{
    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_FALSE (output.isEmpty());
    EXPECT_TRUE (output.contains ("metal"));
    EXPECT_FALSE (output.contains ("#version"));
}

TEST_F (ShaderTranspilerTests, Transpile_GLSLToHLSL)
{
    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::hlsl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_FALSE (output.isEmpty());
    EXPECT_FALSE (output.contains ("#version"));
    EXPECT_FALSE (output.contains ("metal"));
}

TEST_F (ShaderTranspilerTests, Transpile_GLSLToESSL)
{
    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::essl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_FALSE (output.isEmpty());
    EXPECT_TRUE (output.contains ("#version"));
    EXPECT_TRUE (output.contains ("es"));
}

TEST_F (ShaderTranspilerTests, Transpile_InvalidSourceFails)
{
    auto result = transpiler->transpile (
        kInvalidGLSL, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::glsl);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, Transpile_WithOptions)
{
    TranspileOptions opts;
    opts.glslVersion = 330;
    opts.flipVertY = true;

    auto result = transpiler->transpile (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::glsl, opts);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("330"));
    EXPECT_TRUE (output.contains ("#version"));
}

TEST_F (ShaderTranspilerTests, Transpile_TessControlToGLSL)
{
    auto result = transpiler->transpile (
        kTessControlGLSL, ShaderStage::tessControl, ShaderLanguage::glsl, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("main"));
    EXPECT_TRUE (output.contains ("#version"));
}

TEST_F (ShaderTranspilerTests, Transpile_TessControlToMSL)
{
    auto result = transpiler->transpile (
        kTessControlGLSL, ShaderStage::tessControl, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_FALSE (output.isEmpty());
    EXPECT_TRUE (output.contains ("metal"));
    EXPECT_TRUE (output.contains ("kernel"));
}

TEST_F (ShaderTranspilerTests, Transpile_TessEvalToGLSL)
{
    auto result = transpiler->transpile (
        kTessEvalGLSL, ShaderStage::tessEval, ShaderLanguage::glsl, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("main"));
    EXPECT_TRUE (output.contains ("#version"));
}

TEST_F (ShaderTranspilerTests, Transpile_TessEvalToMSL)
{
    auto result = transpiler->transpile (
        kTessEvalGLSL, ShaderStage::tessEval, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_FALSE (output.isEmpty());
    EXPECT_TRUE (output.contains ("#include <metal_stdlib>"));
    EXPECT_TRUE (output.contains ("using namespace metal;"));
    EXPECT_TRUE (output.contains ("[[position]]"));
    EXPECT_TRUE (output.contains ("[[attribute(0)]]"));
    EXPECT_TRUE (output.contains ("[[ patch(triangle, 0) ]]"));
    EXPECT_TRUE (output.contains ("[[stage_in]]"));
    EXPECT_TRUE (output.contains ("[[position_in_patch]]"));
    EXPECT_TRUE (output.contains ("vertex"));
}

//==============================================================================
// MSL-specific transpilation
//==============================================================================

TEST_F (ShaderTranspilerTests, Transpile_HLSLToMSL)
{
    auto result = transpiler->transpile (
        kMinimalHLSL, ShaderStage::vertex, ShaderLanguage::hlsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_FALSE (output.isEmpty());
    EXPECT_TRUE (output.contains ("#include <metal_stdlib>"));
    EXPECT_TRUE (output.contains ("using namespace metal;"));
    EXPECT_TRUE (output.contains ("[[position]]"));
    EXPECT_TRUE (output.contains ("vertex"));
}

TEST_F (ShaderTranspilerTests, Transpile_VertexToMSL)
{
    auto result = transpiler->transpile (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("#include <metal_stdlib>"));
    EXPECT_TRUE (output.contains ("using namespace metal;"));
    EXPECT_TRUE (output.contains ("[[position]]"));
    EXPECT_TRUE (output.contains ("vertex"));
}

TEST_F (ShaderTranspilerTests, Transpile_FragmentToMSL)
{
    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("fragment"));
    EXPECT_TRUE (output.contains ("metal"));
    EXPECT_FALSE (output.contains ("gl_FragCoord"));
}

TEST_F (ShaderTranspilerTests, Transpile_ComputeToMSL)
{
    auto result = transpiler->transpile (
        kMinimalComputeGLSL, ShaderStage::compute, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_TRUE (output.contains ("kernel"));
    EXPECT_TRUE (output.contains ("metal"));
}

TEST_F (ShaderTranspilerTests, Transpile_MSLWithFlipVertY)
{
    TranspileOptions opts;
    opts.flipVertY = true;

    auto result = transpiler->transpile (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    const auto& output = result.getValue();
    EXPECT_FALSE (output.isEmpty());
    EXPECT_TRUE (output.contains ("metal"));
    EXPECT_TRUE (output.contains ("vertex"));
}

TEST_F (ShaderTranspilerTests, Transpile_MSLWithFramebufferFetch)
{
    TranspileOptions opts;
    opts.mslUsesFramebufferFetch = true;

    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

TEST_F (ShaderTranspilerTests, Transpile_MSLRespectsEntryPoint)
{
    TranspileOptions opts;
    opts.entryPoint = "main";

    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

//==============================================================================
// reflect
//==============================================================================

TEST_F (ShaderTranspilerTests, Reflect_FragmentShader)
{
    auto result = transpiler->reflect (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.entryPoints.empty());
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::fragment);
}

TEST_F (ShaderTranspilerTests, Reflect_VertexShader)
{
    auto result = transpiler->reflect (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.entryPoints.empty());
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::vertex);
}

TEST_F (ShaderTranspilerTests, Reflect_ComputeShader_WorkgroupSize)
{
    auto result = transpiler->reflect (
        kMinimalComputeGLSL, ShaderStage::compute, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.entryPoints.empty());
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::compute);
    EXPECT_EQ (ref.workgroupSize.x, 16u);
    EXPECT_EQ (ref.workgroupSize.y, 1u);
    EXPECT_EQ (ref.workgroupSize.z, 1u);
}

TEST_F (ShaderTranspilerTests, Reflect_ShaderWithUniforms)
{
    auto result = transpiler->reflect (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();

    EXPECT_FALSE (ref.uniformBuffers.empty());
    EXPECT_FALSE (ref.sampledImages.empty());

    // At least one uniform buffer should be a struct type
    bool foundStructUB = false;
    for (const auto& ub : ref.uniformBuffers)
    {
        if (ub.baseType == ShaderReflection::BaseType::structType)
        {
            foundStructUB = true;
            EXPECT_EQ (ub.type, ShaderReflection::ResourceType::uniformBuffer);
            break;
        }
    }
    EXPECT_TRUE (foundStructUB);
}

TEST_F (ShaderTranspilerTests, Reflect_InvalidSourceFails)
{
    auto result = transpiler->reflect (
        kInvalidGLSL, ShaderStage::vertex, ShaderLanguage::glsl);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, Reflect_TessControlShader)
{
    auto result = transpiler->reflect (
        kTessControlGLSL, ShaderStage::tessControl, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.entryPoints.empty());
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::tessControl);

    // Tessellation control should have tessLevelOuter and tessLevelInner builtins
    bool hasTessLevelOuter = false;
    bool hasTessLevelInner = false;
    for (const auto& bo : ref.builtinOutputs)
    {
        if (bo.builtin == ShaderReflection::BuiltInType::tessLevelOuter)
            hasTessLevelOuter = true;
        if (bo.builtin == ShaderReflection::BuiltInType::tessLevelInner)
            hasTessLevelInner = true;
    }
    EXPECT_TRUE (hasTessLevelOuter);
    EXPECT_TRUE (hasTessLevelInner);
}

TEST_F (ShaderTranspilerTests, Reflect_TessEvalShader)
{
    auto result = transpiler->reflect (
        kTessEvalGLSL, ShaderStage::tessEval, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.entryPoints.empty());
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::tessEval);

    // Tessellation evaluation should have tessCoord builtin input
    bool hasTessCoord = false;
    for (const auto& bi : ref.builtinInputs)
    {
        if (bi.builtin == ShaderReflection::BuiltInType::tessCoord)
        {
            hasTessCoord = true;
            break;
        }
    }
    EXPECT_TRUE (hasTessCoord);
}

TEST_F (ShaderTranspilerTests, Reflect_IntUintTypes)
{
    auto result = transpiler->reflect (
        kFragmentWithIntUintInputs, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.stageInputs.empty());

    bool foundInt = false;
    bool foundUint = false;
    for (const auto& si : ref.stageInputs)
    {
        if (si.baseType == ShaderReflection::BaseType::int32)
            foundInt = true;
        if (si.baseType == ShaderReflection::BaseType::uint32)
            foundUint = true;
    }
    EXPECT_TRUE (foundInt);
    EXPECT_TRUE (foundUint);
}

TEST_F (ShaderTranspilerTests, Reflect_StorageBuffer)
{
    auto result = transpiler->reflect (
        kFragmentWithStorageBuffer, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.storageBuffers.empty());

    bool foundStorageBuf = false;
    for (const auto& sb : ref.storageBuffers)
    {
        if (sb.type == ShaderReflection::ResourceType::storageBuffer)
        {
            foundStorageBuf = true;
            break;
        }
    }
    EXPECT_TRUE (foundStorageBuf);
}

TEST_F (ShaderTranspilerTests, Reflect_ArrayUBO)
{
    auto result = transpiler->reflect (
        kFragmentWithArrayUBO, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.uniformBuffers.empty());
    EXPECT_FALSE (ref.entryPoints.empty());
}

TEST_F (ShaderTranspilerTests, Reflect_StructMemberDetails)
{
    auto result = transpiler->reflect (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.uniformBuffers.empty());
    EXPECT_FALSE (ref.sampledImages.empty());
    EXPECT_FALSE (ref.stageInputs.empty());

    // Verify that uniform buffer and sampled image metadata is populated
    for (const auto& ub : ref.uniformBuffers)
    {
        EXPECT_EQ (ub.type, ShaderReflection::ResourceType::uniformBuffer);
        EXPECT_EQ (ub.baseType, ShaderReflection::BaseType::structType);
    }

    for (const auto& si : ref.sampledImages)
    {
        EXPECT_EQ (si.type, ShaderReflection::ResourceType::sampledImage);
        EXPECT_EQ (si.imageDim, ShaderReflection::ImageDimension::dim2D);
    }
}

TEST_F (ShaderTranspilerTests, Reflect_PointSize)
{
    auto result = transpiler->reflect (
        kVertexWithPointSize, ShaderStage::vertex, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    bool hasPointSize = false;
    for (const auto& bo : ref.builtinOutputs)
    {
        if (bo.builtin == ShaderReflection::BuiltInType::pointSize)
        {
            hasPointSize = true;
            break;
        }
    }
    EXPECT_TRUE (hasPointSize);
}

TEST_F (ShaderTranspilerTests, Reflect_FragDepth)
{
    auto result = transpiler->reflect (
        kFragmentWithFragDepth, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    bool hasFragDepth = false;
    for (const auto& bo : ref.builtinOutputs)
    {
        if (bo.builtin == ShaderReflection::BuiltInType::fragDepth)
        {
            hasFragDepth = true;
            break;
        }
    }
    EXPECT_TRUE (hasFragDepth);
}

TEST_F (ShaderTranspilerTests, Reflect_Layer)
{
    auto result = transpiler->reflect (
        kGeometryWithLayer, ShaderStage::geometry, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    bool hasLayer = false;
    for (const auto& bo : ref.builtinOutputs)
    {
        if (bo.builtin == ShaderReflection::BuiltInType::layer)
        {
            hasLayer = true;
            break;
        }
    }
    EXPECT_TRUE (hasLayer);
}

TEST_F (ShaderTranspilerTests, Reflect_PointCoord)
{
    auto result = transpiler->reflect (
        kFragmentWithPointCoord, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    bool hasPointCoord = false;
    for (const auto& bi : ref.builtinInputs)
    {
        if (bi.builtin == ShaderReflection::BuiltInType::pointCoord)
        {
            hasPointCoord = true;
            break;
        }
    }
    EXPECT_TRUE (hasPointCoord);
}

TEST_F (ShaderTranspilerTests, Reflect_PatchVertices)
{
    auto result = transpiler->reflect (
        kTessControlWithPatchVertices, ShaderStage::tessControl, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    bool hasPatchVertices = false;
    for (const auto& bi : ref.builtinInputs)
    {
        if (bi.builtin == ShaderReflection::BuiltInType::patchVertices)
        {
            hasPatchVertices = true;
            break;
        }
    }
    EXPECT_TRUE (hasPatchVertices);
}

TEST_F (ShaderTranspilerTests, Reflect_HelperInvocation)
{
    auto result = transpiler->reflect (
        kFragmentWithHelperInvocation, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    bool hasHelper = false;
    for (const auto& bi : ref.builtinInputs)
    {
        if (bi.builtin == ShaderReflection::BuiltInType::helperInvocation)
        {
            hasHelper = true;
            break;
        }
    }
    EXPECT_TRUE (hasHelper);
}

TEST_F (ShaderTranspilerTests, Reflect_SampleMask)
{
    auto result = transpiler->reflect (
        kFragmentWithSampleMask, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    bool hasSampleMask = false;
    for (const auto& bo : ref.builtinOutputs)
    {
        if (bo.builtin == ShaderReflection::BuiltInType::sampleMask)
        {
            hasSampleMask = true;
            break;
        }
    }
    EXPECT_TRUE (hasSampleMask);
}

TEST_F (ShaderTranspilerTests, Reflect_Sampler1D)
{
    auto result = transpiler->reflect (
        kFragmentWithSampler1D, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.sampledImages.empty());

    bool foundDim1D = false;
    for (const auto& si : ref.sampledImages)
    {
        if (si.imageDim == ShaderReflection::ImageDimension::dim1D)
        {
            foundDim1D = true;
            break;
        }
    }
    EXPECT_TRUE (foundDim1D);
}

TEST_F (ShaderTranspilerTests, Reflect_SampleIdAndPosition)
{
    auto result = transpiler->reflect (
        kFragmentWithSampleIdAndPos, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    bool hasSampleId = false;
    bool hasSamplePosition = false;
    for (const auto& bi : ref.builtinInputs)
    {
        if (bi.builtin == ShaderReflection::BuiltInType::sampleId)
            hasSampleId = true;
        if (bi.builtin == ShaderReflection::BuiltInType::samplePosition)
            hasSamplePosition = true;
    }
    EXPECT_TRUE (hasSampleId);
    EXPECT_TRUE (hasSamplePosition);
}

TEST_F (ShaderTranspilerTests, Reflect_ImageBuffer)
{
    auto result = transpiler->reflect (
        kFragmentWithImageBuffer, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.sampledImages.empty());

    bool foundDimBuffer = false;
    for (const auto& si : ref.sampledImages)
    {
        if (si.imageDim == ShaderReflection::ImageDimension::dimBuffer)
        {
            foundDimBuffer = true;
            break;
        }
    }
    EXPECT_TRUE (foundDimBuffer);
}

TEST_F (ShaderTranspilerTests, Reflect_SubpassInput)
{
    auto result = transpiler->reflect (
        kFragmentWithSubpassInput, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.subpassInputs.empty());

    bool foundSubpass = false;
    for (const auto& sp : ref.subpassInputs)
    {
        if (sp.type == ShaderReflection::ResourceType::subpassInput)
        {
            foundSubpass = true;
            EXPECT_EQ (sp.imageDim, ShaderReflection::ImageDimension::dimSubpass);
            break;
        }
    }
    EXPECT_TRUE (foundSubpass);
}

//==============================================================================
// Multi-texture dimensions
//==============================================================================

TEST_F (ShaderTranspilerTests, Reflect_MultiTextureDimensions)
{
    auto result = transpiler->reflect (
        kFragmentWithMultiTextures, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.sampledImages.empty());

    bool foundDim2D = false;
    bool foundDim3D = false;
    bool foundCube = false;
    for (const auto& si : ref.sampledImages)
    {
        if (si.imageDim == ShaderReflection::ImageDimension::dim2D)
            foundDim2D = true;
        if (si.imageDim == ShaderReflection::ImageDimension::dim3D)
            foundDim3D = true;
        if (si.imageDim == ShaderReflection::ImageDimension::cube)
            foundCube = true;
    }
    EXPECT_TRUE (foundDim2D);
    EXPECT_TRUE (foundDim3D);
    EXPECT_TRUE (foundCube);
}

TEST_F (ShaderTranspilerTests, Reflect_TextureArray)
{
    auto result = transpiler->reflect (
        kFragmentWithTextureArray, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.sampledImages.empty());
    EXPECT_FALSE (ref.entryPoints.empty());
}

TEST_F (ShaderTranspilerTests, Reflect_TextureArrayLayout)
{
    auto result = transpiler->reflect (
        kFragmentWithTextureArray, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();

    // Texture arrays should be reflected with correct resource type and image dim
    for (const auto& si : ref.sampledImages)
    {
        EXPECT_EQ (si.type, ShaderReflection::ResourceType::sampledImage);
        EXPECT_EQ (si.imageDim, ShaderReflection::ImageDimension::dim2D);
    }
}

TEST_F (ShaderTranspilerTests, Reflect_SpecializationConstant)
{
    auto result = transpiler->reflect (
        kFragmentWithSpecConst, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.specConstants.empty());

    bool foundSpecConst = false;
    for (const auto& sc : ref.specConstants)
    {
        if (sc.constantId == 0)
        {
            foundSpecConst = true;
            EXPECT_EQ (sc.baseType, ShaderReflection::BaseType::float32);
            break;
        }
    }
    EXPECT_TRUE (foundSpecConst);
}

TEST_F (ShaderTranspilerTests, Reflect_UBOArray)
{
    auto result = transpiler->reflect (
        kFragmentWithUBOArray, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.uniformBuffers.empty());
    EXPECT_FALSE (ref.entryPoints.empty());
}

TEST_F (ShaderTranspilerTests, Reflect_VertexIndices)
{
    auto result = transpiler->reflect (
        kVertexWithIndices, ShaderStage::vertex, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.entryPoints.empty());
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::vertex);

    bool hasVertexIndex = false;
    bool hasInstanceIndex = false;
    for (const auto& bi : ref.builtinInputs)
    {
        if (bi.builtin == ShaderReflection::BuiltInType::vertexIndex)
            hasVertexIndex = true;
        if (bi.builtin == ShaderReflection::BuiltInType::instanceIndex)
            hasInstanceIndex = true;
    }
    EXPECT_TRUE (hasVertexIndex);
    EXPECT_TRUE (hasInstanceIndex);
}

TEST_F (ShaderTranspilerTests, Reflect_FragCoordAndFrontFacing)
{
    auto result = transpiler->reflect (
        kFragmentWithFragCoord, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();

    bool hasFragCoord = false;
    bool hasFrontFacing = false;
    for (const auto& bi : ref.builtinInputs)
    {
        if (bi.builtin == ShaderReflection::BuiltInType::fragCoord)
            hasFragCoord = true;
        if (bi.builtin == ShaderReflection::BuiltInType::frontFacing)
            hasFrontFacing = true;
    }
    EXPECT_TRUE (hasFragCoord);
    EXPECT_TRUE (hasFrontFacing);
}

TEST_F (ShaderTranspilerTests, Reflect_ComputeBuiltins)
{
    auto result = transpiler->reflect (
        kComputeWithBuiltins, ShaderStage::compute, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::compute);

    bool hasNumWorkgroups = false;
    bool hasWorkgroupId = false;
    bool hasLocalInvocationId = false;
    bool hasGlobalInvocationId = false;
    bool hasLocalInvocationIndex = false;
    for (const auto& bi : ref.builtinInputs)
    {
        switch (bi.builtin)
        {
            case ShaderReflection::BuiltInType::numWorkgroups:
                hasNumWorkgroups = true;
                break;
            case ShaderReflection::BuiltInType::workgroupId:
                hasWorkgroupId = true;
                break;
            case ShaderReflection::BuiltInType::localInvocationId:
                hasLocalInvocationId = true;
                break;
            case ShaderReflection::BuiltInType::globalInvocationId:
                hasGlobalInvocationId = true;
                break;
            case ShaderReflection::BuiltInType::localInvocationIndex:
                hasLocalInvocationIndex = true;
                break;
            default:
                break;
        }
    }
    EXPECT_TRUE (hasNumWorkgroups);
    EXPECT_TRUE (hasWorkgroupId);
    EXPECT_TRUE (hasLocalInvocationId);
    EXPECT_TRUE (hasGlobalInvocationId);
    EXPECT_TRUE (hasLocalInvocationIndex);
}

TEST_F (ShaderTranspilerTests, Reflect_WorkgroupSizeDetails)
{
    auto result = transpiler->reflect (
        kComputeWithBuiltins, ShaderStage::compute, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_EQ (ref.workgroupSize.x, 8u);
    EXPECT_EQ (ref.workgroupSize.y, 1u);
    EXPECT_EQ (ref.workgroupSize.z, 1u);
}

TEST_F (ShaderTranspilerTests, Reflect_SeparateSampler)
{
    auto result = transpiler->reflect (
        kFragmentWithSeparateSampler, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();

    bool foundImage = false;
    bool foundSampler = false;
    for (const auto& si : ref.separateImages)
    {
        if (si.baseType == ShaderReflection::BaseType::image)
            foundImage = true;
    }
    for (const auto& ss : ref.separateSamplers)
    {
        if (ss.baseType == ShaderReflection::BaseType::sampler)
            foundSampler = true;
    }
    EXPECT_TRUE (foundImage);
    EXPECT_TRUE (foundSampler);
}

TEST_F (ShaderTranspilerTests, Reflect_StorageImage)
{
    auto result = transpiler->reflect (
        kComputeWithBuiltins, ShaderStage::compute, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.storageImages.empty());

    bool foundStorageImage = false;
    for (const auto& si : ref.storageImages)
    {
        if (si.type == ShaderReflection::ResourceType::storageImage)
        {
            foundStorageImage = true;
            EXPECT_EQ (si.imageDim, ShaderReflection::ImageDimension::dim2D);
            break;
        }
    }
    EXPECT_TRUE (foundStorageImage);
}

TEST_F (ShaderTranspilerTests, Reflect_ClipAndCullDistance)
{
    auto result = transpiler->reflect (
        kVertexWithClipCullDistance, ShaderStage::vertex, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();

    bool hasClipDistance = false;
    bool hasCullDistance = false;
    for (const auto& bo : ref.builtinOutputs)
    {
        if (bo.builtin == ShaderReflection::BuiltInType::clipDistance)
            hasClipDistance = true;
        if (bo.builtin == ShaderReflection::BuiltInType::cullDistance)
            hasCullDistance = true;
    }
    EXPECT_TRUE (hasClipDistance);
    EXPECT_TRUE (hasCullDistance);
}

TEST_F (ShaderTranspilerTests, Reflect_GeometryPrimitiveId)
{
    auto result = transpiler->reflect (
        kGeometryWithPrimitiveId, ShaderStage::geometry, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::geometry);

    bool hasPrimitiveId = false;
    for (const auto& bi : ref.builtinInputs)
    {
        if (bi.builtin == ShaderReflection::BuiltInType::primitiveId)
        {
            hasPrimitiveId = true;
            break;
        }
    }
    EXPECT_TRUE (hasPrimitiveId);
}

//==============================================================================
// reflectFromSPIRV
//==============================================================================

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_ValidBinary)
{
    auto spirv = transpiler->compileToSPIRV (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->reflectFromSPIRV (spirv.getValue());

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.entryPoints.empty());
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::fragment);
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_EmptyFails)
{
    MemoryBlock empty;

    auto result = transpiler->reflectFromSPIRV (empty);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_InvalidFails)
{
    const uint8_t garbage[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    MemoryBlock invalid (garbage, sizeof (garbage));

    auto result = transpiler->reflectFromSPIRV (invalid);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_ComputeWorkgroupReflected)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalComputeGLSL, ShaderStage::compute, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->reflectFromSPIRV (spirv.getValue());

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (result.getValue().workgroupSize.x, 16u);
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_TooSmallFails)
{
    const uint32_t tooSmall[] = { 0x07230203, 0x00010000 };
    MemoryBlock smallSpirv (tooSmall, sizeof (tooSmall));

    auto result = transpiler->reflectFromSPIRV (smallSpirv);

    EXPECT_TRUE (result.failed());
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_CorruptHeaderDataFails)
{
    // Valid magic and version, but garbage thereafter — should throw in spirv_cross
    const uint32_t corruptSpirv[] = {
        0x07230203, 0x00010000, 0x00000000, 0x00000000, 0x00000000, 0xDEADBEEF, 0xDEADBEEF, 0xDEADBEEF
    };
    MemoryBlock corrupt (corruptSpirv, sizeof (corruptSpirv));

    auto result = transpiler->reflectFromSPIRV (corrupt);

    EXPECT_TRUE (result.failed());
}

//==============================================================================
// MSL-specific reflection
//==============================================================================

TEST_F (ShaderTranspilerTests, Reflect_VertexShaderForMSL)
{
    auto result = transpiler->reflect (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.entryPoints.empty());
    EXPECT_EQ (ref.entryPoints[0].stage, ShaderStage::vertex);

    // Vertex shaders should have built-in outputs (gl_Position)
    bool hasBuiltinPos = false;
    for (const auto& bo : ref.builtinOutputs)
    {
        if (bo.builtin == ShaderReflection::BuiltInType::position)
        {
            hasBuiltinPos = true;
            break;
        }
    }
    EXPECT_TRUE (hasBuiltinPos);
}

TEST_F (ShaderTranspilerTests, Reflect_FragmentShaderForMSL)
{
    auto result = transpiler->reflect (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();
    EXPECT_FALSE (ref.stageOutputs.empty());
}

TEST_F (ShaderTranspilerTests, Reflect_ShaderWithUniformsForMSL)
{
    auto result = transpiler->reflect (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (result.wasOk());

    const auto& ref = result.getValue();

    EXPECT_FALSE (ref.uniformBuffers.empty());
    EXPECT_FALSE (ref.sampledImages.empty());
    EXPECT_FALSE (ref.stageInputs.empty());

    // Verify bindings are present
    for (const auto& ub : ref.uniformBuffers)
    {
        EXPECT_EQ (ub.type, ShaderReflection::ResourceType::uniformBuffer);
        // UBO should have binding 0
        if (ub.name.contains ("UBO"))
            EXPECT_EQ (ub.binding, 0u);
    }

    for (const auto& si : ref.sampledImages)
    {
        EXPECT_EQ (si.type, ShaderReflection::ResourceType::sampledImage);
        // Texture should have binding 1
        if (si.name.contains ("tex"))
            EXPECT_EQ (si.binding, 1u);
    }
}

//==============================================================================
// Lifecycle
//==============================================================================

TEST_F (ShaderTranspilerTests, MultipleInstancesCanCoexist)
{
    auto t2 = new ShaderTranspiler();
    auto t3 = new ShaderTranspiler();

    auto r1 = t2->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    auto r2 = t3->compileToSPIRV (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl);

    EXPECT_TRUE (r1.wasOk());
    EXPECT_TRUE (r2.wasOk());
}

//==============================================================================
// reflectFromSPIRV with backend-aware slot reflection
//==============================================================================

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_MSL_FragmentShaderWithUniforms)
{
    auto spirvResult = transpiler->compileToSPIRV (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (spirvResult.wasOk());

    auto reflectResult = transpiler->reflectFromSPIRV (
        spirvResult.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (reflectResult.wasOk());

    const auto& ref = reflectResult.getValue();

    ASSERT_FALSE (ref.uniformBuffers.empty());
    ASSERT_FALSE (ref.sampledImages.empty());

    // UBO should have a valid backend slot assigned
    for (const auto& ub : ref.uniformBuffers)
    {
        EXPECT_NE (ub.backendSlot, ~0u);
    }

    // Sampled image (combined sampler) should have both primary and secondary slots
    for (const auto& si : ref.sampledImages)
    {
        EXPECT_NE (si.backendSlot, ~0u);
        EXPECT_NE (si.backendSlotSecondary, ~0u);
    }
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_MSL_BackendSlotsDoNotCollide)
{
    auto spirvResult = transpiler->compileToSPIRV (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (spirvResult.wasOk());

    auto reflectResult = transpiler->reflectFromSPIRV (
        spirvResult.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (reflectResult.wasOk());

    const auto& ref = reflectResult.getValue();

    // MSL has independent slot namespaces per resource type:
    // [[buffer(N)]], [[texture(N)]], [[sampler(N)]] don't collide.
    // Uniqueness is checked per-category.

    auto checkUniq = [] (const std::vector<ShaderReflection::ResourceBinding>& bindings)
    {
        std::set<uint32_t> seen;

        for (const auto& b : bindings)
        {
            if (b.backendSlot != ~0u)
            {
                EXPECT_FALSE (seen.contains (b.backendSlot))
                    << "Duplicate backend slot " << b.backendSlot << " for " << b.name.toRawUTF8();
                seen.insert (b.backendSlot);
            }
        }
    };

    checkUniq (ref.uniformBuffers);
    checkUniq (ref.sampledImages);
    checkUniq (ref.separateImages);
    checkUniq (ref.separateSamplers);
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_MSL_ComputeShader)
{
    auto spirvResult = transpiler->compileToSPIRV (
        kMinimalComputeGLSL, ShaderStage::compute, ShaderLanguage::glsl);

    ASSERT_TRUE (spirvResult.wasOk());

    auto reflectResult = transpiler->reflectFromSPIRV (
        spirvResult.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (reflectResult.wasOk());

    const auto& ref = reflectResult.getValue();
    EXPECT_EQ (ref.workgroupSize.x, 16u);
    EXPECT_EQ (ref.workgroupSize.y, 1u);
    EXPECT_EQ (ref.workgroupSize.z, 1u);
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_GLSL_SlotsMatchSPIRVBinding)
{
    auto spirvResult = transpiler->compileToSPIRV (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (spirvResult.wasOk());

    auto reflectResult = transpiler->reflectFromSPIRV (
        spirvResult.getValue(), ShaderLanguage::glsl);

    ASSERT_TRUE (reflectResult.wasOk());

    const auto& ref = reflectResult.getValue();

    // GLSL backend slots should match the original SPIR-V binding numbers
    for (const auto& ub : ref.uniformBuffers)
    {
        EXPECT_EQ (ub.backendSlot, ub.binding);
    }

    for (const auto& si : ref.sampledImages)
    {
        EXPECT_EQ (si.backendSlot, si.binding);
    }
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_ESSL_SlotsMatchSPIRVBinding)
{
    auto spirvResult = transpiler->compileToSPIRV (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (spirvResult.wasOk());

    auto reflectResult = transpiler->reflectFromSPIRV (
        spirvResult.getValue(), ShaderLanguage::essl);

    ASSERT_TRUE (reflectResult.wasOk());

    const auto& ref = reflectResult.getValue();

    for (const auto& ub : ref.uniformBuffers)
    {
        EXPECT_EQ (ub.backendSlot, ub.binding);
    }
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_HLSL_NotSupported)
{
    auto spirvResult = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (spirvResult.wasOk());

    auto reflectResult = transpiler->reflectFromSPIRV (
        spirvResult.getValue(), ShaderLanguage::hlsl);

    EXPECT_TRUE (reflectResult.failed());
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_NoTargetLang_BackendSlotsAreAbsent)
{
    auto spirvResult = transpiler->compileToSPIRV (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (spirvResult.wasOk());

    // Original overload (no target language) — backend slots should remain ~0u
    auto reflectResult = transpiler->reflectFromSPIRV (spirvResult.getValue());

    ASSERT_TRUE (reflectResult.wasOk());

    const auto& ref = reflectResult.getValue();

    for (const auto& ub : ref.uniformBuffers)
    {
        EXPECT_EQ (ub.backendSlot, ~0u);
        EXPECT_EQ (ub.backendSlotSecondary, ~0u);
    }

    for (const auto& si : ref.sampledImages)
    {
        EXPECT_EQ (si.backendSlot, ~0u);
        EXPECT_EQ (si.backendSlotSecondary, ~0u);
    }
}

TEST_F (ShaderTranspilerTests, ReflectFromSPIRV_MSL_StageInputAndOutputSlots)
{
    auto spirvResult = transpiler->compileToSPIRV (
        kFragmentWithUniforms, ShaderStage::fragment, ShaderLanguage::glsl);

    ASSERT_TRUE (spirvResult.wasOk());

    auto reflectResult = transpiler->reflectFromSPIRV (
        spirvResult.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (reflectResult.wasOk());

    const auto& ref = reflectResult.getValue();

    // Stage inputs/outputs don't get backend buffer/texture/sampler slots
    for (const auto& si : ref.stageInputs)
    {
        EXPECT_EQ (si.backendSlot, ~0u);
    }

    for (const auto& so : ref.stageOutputs)
    {
        EXPECT_EQ (so.backendSlot, ~0u);
    }
}

#endif // YUP_ENABLE_SHADER_COMPILER
