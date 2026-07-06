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
layout(binding = 0) uniform UBO {
    vec4 tint;
    float scale;
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
    EXPECT_TRUE (result.getValue().contains ("main"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToESSL)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::essl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (result.getValue().contains ("main"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToHLSL)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::hlsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (result.getValue().contains ("main"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (result.getValue().contains ("main0"));
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
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL_VertexShader)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    // vertex shader in MSL uses 'vertex' qualifier on return type
    EXPECT_TRUE (result.getValue().contains ("vertex"));
}

TEST_F (ShaderTranspilerTests, DecompileFromSPIRV_ToMSL_FragmentShader)
{
    auto spirv = transpiler->compileToSPIRV (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirv.wasOk());

    auto result = transpiler->decompileFromSPIRV (
        spirv.getValue(), ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    // fragment shader in MSL uses 'fragment' qualifier on return type
    EXPECT_TRUE (result.getValue().contains ("fragment"));
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
    // compute shader in MSL uses 'kernel' qualifier
    EXPECT_TRUE (result.getValue().contains ("kernel"));
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
    EXPECT_TRUE (result.getValue().contains ("main"));
}

TEST_F (ShaderTranspilerTests, Transpile_GLSLToMSL)
{
    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

TEST_F (ShaderTranspilerTests, Transpile_GLSLToHLSL)
{
    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::hlsl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

TEST_F (ShaderTranspilerTests, Transpile_GLSLToESSL)
{
    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::essl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
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
    EXPECT_TRUE (result.getValue().contains ("330"));
}

//==============================================================================
// MSL-specific transpilation
//==============================================================================

TEST_F (ShaderTranspilerTests, Transpile_HLSLToMSL)
{
    auto result = transpiler->transpile (
        kMinimalHLSL, ShaderStage::vertex, ShaderLanguage::hlsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
}

TEST_F (ShaderTranspilerTests, Transpile_VertexToMSL)
{
    auto result = transpiler->transpile (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (result.getValue().contains ("vertex"));
}

TEST_F (ShaderTranspilerTests, Transpile_FragmentToMSL)
{
    auto result = transpiler->transpile (
        kMinimalFragmentGLSL, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (result.getValue().contains ("fragment"));
}

TEST_F (ShaderTranspilerTests, Transpile_ComputeToMSL)
{
    auto result = transpiler->transpile (
        kMinimalComputeGLSL, ShaderStage::compute, ShaderLanguage::glsl, ShaderLanguage::msl);

    ASSERT_TRUE (result.wasOk());
    EXPECT_TRUE (result.getValue().contains ("kernel"));
}

TEST_F (ShaderTranspilerTests, Transpile_MSLWithFlipVertY)
{
    TranspileOptions opts;
    opts.flipVertY = true;

    auto result = transpiler->transpile (
        kMinimalVertexGLSL, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::msl, opts);

    ASSERT_TRUE (result.wasOk());
    EXPECT_FALSE (result.getValue().isEmpty());
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

#endif // YUP_ENABLE_SHADER_COMPILER
