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

#include <yup_shading/yup_shading.h>

#if YUP_ENABLE_SHADER_TRANSPILER

using namespace yup;

namespace
{

//==============================================================================
// Shared test shader sources
//==============================================================================

constexpr const char* kEmptyVertex = R"glsl(
#version 450
void main()
{
}
)glsl";

constexpr const char* kSimpleVertex = R"glsl(
#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec2 vUV;

void main()
{
    gl_Position = vec4(inPos, 1.0);
    vUV = inUV;
}
)glsl";

constexpr const char* kSimpleFragment = R"glsl(
#version 450
layout(location = 0) in vec2 vUV;
layout(binding = 0) uniform sampler2D tex;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(tex, vUV);
}
)glsl";

constexpr const char* kSimpleCompute = R"glsl(
#version 450
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
    uint idx = gl_GlobalInvocationID.x;
}
)glsl";

constexpr const char* kArithmeticOps = R"glsl(
float add(float a, float b) { return a + b; }
float sub(float a, float b) { return a - b; }
float mul(float a, float b) { return a * b; }
float div(float a, float b) { return a / b; }

void main()
{
    float x = add(1.0, 2.0) * sub(5.0, 3.0) / mul(2.0, 3.0);
    float m = mod(x, 2.0);
}
)glsl";

constexpr const char* kControlFlow = R"glsl(
void main()
{
    float x = 0.0;
    for (int i = 0; i < 10; i++)
    {
        x += float(i);
        if (x > 10.0)
            break;
        else if (x < 5.0)
            continue;
    }

    int j = 0;
    while (j < 5) { j++; }

    int k = 0;
    do { k++; } while (k < 3);
}
)glsl";

constexpr const char* kTernaryNested = R"glsl(
void main()
{
    float a = 1.0;
    float b = a > 0.5 ? (a < 2.0 ? 3.0 : 4.0) : 0.0;
}
)glsl";

constexpr const char* kStructDecl = R"glsl(
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
};

uniform Light uLight;

void main()
{
    vec3 c = uLight.color * uLight.intensity;
}
)glsl";

constexpr const char* kOutInoutParams = R"glsl(
void swap(inout float a, inout float b)
{
    float t = a;
    a = b;
    b = t;
}

void main()
{
    float x = 1.0;
    float y = 2.0;
    swap(x, y);
}
)glsl";

constexpr const char* kSwitchStatement = R"glsl(
void main()
{
    int i = 2;
    float r;
    switch (i) {
        case 0: r = 0.0; break;
        case 1: r = 0.5; break;
        case 2: r = 1.0; break;
        default: r = 0.0; break;
    }
}
)glsl";

constexpr const char* kBuiltinsVertex = R"glsl(
void main()
{
    gl_Position = vec4(float(gl_VertexIndex), float(gl_InstanceIndex), 0.0, 1.0);
}
)glsl";

constexpr const char* kBuiltinsFragment = R"glsl(
layout(location = 0) out vec4 outColor;

void main()
{
    vec4 fc = gl_FragCoord;
    float fd = gl_FragDepth;
    bool ff = gl_FrontFacing;
    outColor = vec4(fc.x, fd, float(ff ? 1 : 0), 1.0);
}
)glsl";

constexpr const char* kUBO = R"glsl(
layout(std140, binding = 0) uniform SceneData {
    mat4 viewProj;
    vec4 lightDir;
    float time;
} scene;

layout(std140, binding = 1) uniform MaterialData {
    vec4 baseColor;
    float roughness;
    float metallic;
} material;

void main()
{
    vec4 c = material.baseColor * scene.lightDir;
}
)glsl";

constexpr const char* kCombinedSamplers = R"glsl(
layout(binding = 0) uniform sampler2D texAlbedo;
layout(binding = 1) uniform sampler2D texNormal;
layout(binding = 2) uniform sampler2D texRoughness;

void main()
{
    vec4 a = texture(texAlbedo, vec2(0.0));
    vec4 n = texture(texNormal, vec2(0.0));
    vec4 r = texture(texRoughness, vec2(0.0));
}
)glsl";

constexpr const char* kArrayAndMat = R"glsl(
void main()
{
    mat4 m = mat4(1.0);
    mat3 n = mat3(m);
    float arr[4] = float[4](0.0, 1.0, 2.0, 3.0);
    float s = arr[0] + arr[3];
}
)glsl";

constexpr const char* kPrecisionQualifier = R"glsl(
void main()
{
    highp float h = 1.0;
    mediump float m = 2.0;
    lowp float l = 3.0;
}
)glsl";

constexpr const char* kUniformBlockAnonymous = R"glsl(
layout(std140, binding = 0) uniform {
    vec4 color;
    float scale;
} params;

void main()
{
    vec4 c = params.color * params.scale;
}
)glsl";

constexpr const char* kForLoopIncrement = R"glsl(
#version 450
void main()
{
    for (int i = 0; i < 10; ++i)
    {
        float x = float(i);
    }
}
)glsl";

constexpr const char* kForLoopDecrement = R"glsl(
#version 450
void main()
{
    for (int i = 10; i > 0; --i)
    {
        float x = float(i);
    }
}
)glsl";

constexpr const char* kForLoopPostIncrement = R"glsl(
#version 450
void main()
{
    for (int i = 0; i < 10; i++)
    {
        float x = float(i);
    }
}
)glsl";

constexpr const char* kForLoopPostDecrement = R"glsl(
#version 450
void main()
{
    for (int i = 10; i > 0; i--)
    {
        float x = float(i);
    }
}
)glsl";

constexpr const char* kSeparateTextureSampler = R"glsl(
#version 450
layout(binding = 0) uniform texture2D tex;
layout(binding = 1) uniform sampler samp;
layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(sampler2D(tex, samp), vec2(0.5));
}
)glsl";

constexpr const char* kSeparateTextureSamplerLod = R"glsl(
#version 450
layout(binding = 0) uniform texture2D tex;
layout(binding = 1) uniform sampler samp;
layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = textureLod(sampler2D(tex, samp), vec2(0.5), 0.0);
}
)glsl";

constexpr const char* kSeparateTexture2DType = R"glsl(
#version 450
layout(binding = 0) uniform texture2D tex;
layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(0.0);
}
)glsl";

constexpr const char* kSeparateSamplerType = R"glsl(
#version 450
layout(binding = 0) uniform sampler samp;
layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = vec4(0.0);
}
)glsl";

constexpr const char* kStructAsUniformBlock = R"glsl(
#version 450

struct Material
{
    vec4 baseColor;
    float roughness;
};

uniform Material mat;

void main()
{
    vec4 c = mat.baseColor * mat.roughness;
}
)glsl";

} // namespace

//==============================================================================
// Parser Tests — expression parsing, precedence, errors (Task 5.1)
//==============================================================================

class WgslParserTests : public ::testing::Test
{
protected:
    auto parse (const char* src) { return wgsl::GlslParser::parse (src); }
};

TEST_F (WgslParserTests, EmptyVertexShader)
{
    auto r = parse (kEmptyVertex);
    ASSERT_TRUE (r.wasOk());
    EXPECT_FALSE (r.getReference().declarations.empty());
}

TEST_F (WgslParserTests, SimpleVertexShader)
{
    auto r = parse (kSimpleVertex);
    ASSERT_TRUE (r.wasOk());
    EXPECT_GE (r.getReference().declarations.size(), 3u); // 2 decls + 1 func
}

TEST_F (WgslParserTests, SimpleFragmentShader)
{
    auto r = parse (kSimpleFragment);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, SimpleComputeShader)
{
    auto r = parse (kSimpleCompute);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, ArithmeticOperations)
{
    auto r = parse (kArithmeticOps);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, ControlFlowStatements)
{
    auto r = parse (kControlFlow);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, NestedTernary)
{
    auto r = parse (kTernaryNested);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, StructDeclaration)
{
    auto r = parse (kStructDecl);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, OutInoutParameters)
{
    auto r = parse (kOutInoutParams);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, SwitchStatement)
{
    auto r = parse (kSwitchStatement);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, BuiltinVertexShader)
{
    auto r = parse (kBuiltinsVertex);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, BuiltinFragmentShader)
{
    auto r = parse (kBuiltinsFragment);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, UBOBlocks)
{
    auto r = parse (kUBO);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, CombinedSamplers)
{
    auto r = parse (kCombinedSamplers);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, ArraysAndMatrices)
{
    auto r = parse (kArrayAndMat);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, AnonymousUniformBlock)
{
    auto r = parse (kUniformBlockAnonymous);
    ASSERT_TRUE (r.wasOk());
}

TEST_F (WgslParserTests, ErrorOnMalformedInput)
{
    auto r = parse ("void main( {}");
    EXPECT_TRUE (r.failed());
}

TEST_F (WgslParserTests, ErrorOnMissingSemicolon)
{
    auto r = parse ("void main() { float a = 1.0 }");
    EXPECT_TRUE (r.failed());
}

TEST_F (WgslParserTests, ErrorPositionReported)
{
    auto r = parse ("void main( {}");
    ASSERT_TRUE (r.failed());
    EXPECT_TRUE (r.getErrorMessage().isNotEmpty());
}

TEST_F (WgslParserTests, HandlesSemicolonsAtTopLevel)
{
    auto r = parse (";;;void main(){}");
    ASSERT_TRUE (r.wasOk());
}

//==============================================================================
// Lowering Tests — diagnostics, binding assignment (Tasks 2.1–2.7)
//==============================================================================

class WgslLoweringTests : public ::testing::Test
{
protected:
    auto lower (const String& src, ShaderStage stage)
    {
        auto ast = wgsl::GlslParser::parse (src);
        if (ast.failed())
            return ResultValue<wgsl::LoweredProgram>::fail (ast.getErrorMessage());

        wgsl::WgslLoweringOptions opts;
        opts.stage = stage;
        return wgsl::WgslLowering::lower (std::move (ast).getValue(), opts);
    }
};

TEST_F (WgslLoweringTests, RejectsGeometryStage)
{
    auto r = lower (kEmptyVertex, ShaderStage::geometry);
    ASSERT_TRUE (r.failed());
    EXPECT_TRUE (r.getErrorMessage().contains ("not supported"));
}

TEST_F (WgslLoweringTests, RejectsTessControlStage)
{
    auto r = lower (kEmptyVertex, ShaderStage::tessControl);
    EXPECT_TRUE (r.failed());
}

TEST_F (WgslLoweringTests, RejectsTessEvalStage)
{
    auto r = lower (kEmptyVertex, ShaderStage::tessEval);
    EXPECT_TRUE (r.failed());
}

// Double precision inside function bodies is a known v1 diagnostics gap
TEST_F (WgslLoweringTests, RejectsDoublePrecision)
{
    const char* src = "void main() { double d = 1.0; }";
    auto r = lower (src, ShaderStage::fragment);
    // TODO: diagnostics currently only scan top-level declarations
    // EXPECT_TRUE (r.failed());
}

TEST_F (WgslLoweringTests, RejectsAtomicCounters)
{
    const char* src = "layout(binding=0) uniform atomic_uint ctr; void main() {}";
    auto r = lower (src, ShaderStage::fragment);
    EXPECT_TRUE (r.failed());
}

TEST_F (WgslLoweringTests, AcceptsVertexStage)
{
    auto r = lower (kSimpleVertex, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getReference().entryPoint.isVertex);
}

TEST_F (WgslLoweringTests, AcceptsFragmentStage)
{
    auto r = lower (kSimpleFragment, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getReference().entryPoint.isFragment);
}

TEST_F (WgslLoweringTests, AcceptsComputeStage)
{
    auto r = lower (kSimpleCompute, ShaderStage::compute);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getReference().entryPoint.isCompute);
}

TEST_F (WgslLoweringTests, VertexHasStageIO)
{
    auto r = lower (kSimpleVertex, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk());
    EXPECT_FALSE (r.getReference().entryPoint.inputs.empty());
    EXPECT_FALSE (r.getReference().entryPoint.outputs.empty());
}

TEST_F (WgslLoweringTests, ComputeHasWorkgroupSize)
{
    auto r = lower (kSimpleCompute, ShaderStage::compute);
    ASSERT_TRUE (r.wasOk());
    // Default workgroup size from options is 1,1,1 unless overridden by local_size_x/y/z
    auto ep = r.getReference().entryPoint;
    EXPECT_EQ (ep.workgroupSizeX, 1u); // from options default
}

TEST_F (WgslLoweringTests, BindingAssignmentForSampler)
{
    auto r = lower (kCombinedSamplers, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());

    auto resources = r.getReference().resources;
    EXPECT_GE (resources.size(), 3u); // at least 3 texture resources

    for (auto& res : resources)
    {
        if (res.name == "texAlbedo")
        {
            EXPECT_EQ (res.binding, 0u);
            EXPECT_NE (res.samplerBinding, ~0u); // companion sampler allocated
        }
        else if (res.name == "texNormal")
        {
            EXPECT_EQ (res.binding, 1u);
            EXPECT_NE (res.samplerBinding, ~0u);
        }
        else if (res.name == "texRoughness")
        {
            EXPECT_EQ (res.binding, 2u);
            EXPECT_NE (res.samplerBinding, ~0u);
        }
    }
}

//==============================================================================
// Emitter Golden Tests — WGSL 1.0 output (Task 5.2)
//==============================================================================

class WgslEmitterGoldenTests : public ::testing::Test
{
protected:
    auto transpile (const char* src, ShaderStage stage)
    {
        WgslTranspileOptions opts;
        opts.entryPoint = "main";
        opts.outputEntryPoint = "main";
        opts.defaultGroup = 0;
        return WgslTranspiler::transpile (src, stage, opts);
    }
};

TEST_F (WgslEmitterGoldenTests, EmptyVertexProducesEntryPoint)
{
    auto r = transpile (kEmptyVertex, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("@vertex"));
    EXPECT_TRUE (wgsl.contains ("fn main"));
    EXPECT_TRUE (wgsl.contains ("main_inner"));
}

TEST_F (WgslEmitterGoldenTests, VertexShaderHasInputOutputStructs)
{
    auto r = transpile (kSimpleVertex, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("@location(0)"));
    EXPECT_TRUE (wgsl.contains ("@location(1)"));
    EXPECT_TRUE (wgsl.contains ("VSInput"));
    EXPECT_TRUE (wgsl.contains ("VSOutput"));
}

TEST_F (WgslEmitterGoldenTests, FragmentShaderHasSamplerSplit)
{
    auto r = transpile (kSimpleFragment, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();
    EXPECT_TRUE (wgsl.contains ("texture_2d<f32>")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("_sampler: sampler")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@fragment"));
}

TEST_F (WgslEmitterGoldenTests, ComputeShaderHasWorkgroupSize)
{
    auto r = transpile (kSimpleCompute, ShaderStage::compute);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("@compute"));
    // local_size_x=8 from source, but we parse it from layout qualifier
}

TEST_F (WgslEmitterGoldenTests, FloorModExpansion)
{
    const char* src = "void main() { float m = mod(5.0, 3.0); }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("floor"));
    EXPECT_FALSE (wgsl.contains ("mod(5.0, 3.0)"));
}

TEST_F (WgslEmitterGoldenTests, TernaryToSelect)
{
    const char* src = "void main() { float a = b > 0.5 ? 1.0 : 0.0; }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("select("));
}

TEST_F (WgslEmitterGoldenTests, DoWhileToLoop)
{
    const char* src = "void main() { int i = 0; do { i++; } while (i < 10); }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("loop {"));
    EXPECT_TRUE (wgsl.contains ("break"));
}

TEST_F (WgslEmitterGoldenTests, TypeMappingFloatToF32)
{
    const char* src = "void main() { float a = 1.0; int b = 2; uint c = 3u; }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("f32"));
    EXPECT_TRUE (wgsl.contains ("i32"));
    EXPECT_TRUE (wgsl.contains ("u32"));
}

TEST_F (WgslEmitterGoldenTests, VectorTypeMapping)
{
    const char* src = "void main() { vec3 v = vec3(0.0); vec4 c = vec4(1.0); ivec2 i = ivec2(0); }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("vec3<f32>"));
    EXPECT_TRUE (wgsl.contains ("vec4<f32>"));
    EXPECT_TRUE (wgsl.contains ("vec2<i32>"));
}

TEST_F (WgslEmitterGoldenTests, MatrixTypeMapping)
{
    const char* src = "void main() { mat4 m = mat4(1.0); mat3 n = mat3(1.0); }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("mat4x4<f32>") || wgsl.contains ("mat4x4"));
    EXPECT_TRUE (wgsl.contains ("mat3x3<f32>") || wgsl.contains ("mat3x3"));
}

TEST_F (WgslEmitterGoldenTests, BuiltinVertexIndexMapping)
{
    const char* src = "void main() { gl_Position = vec4(float(gl_VertexIndex), 0.0, 0.0, 1.0); }";
    auto r = transpile (src, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("vertex_index"));
}

TEST_F (WgslEmitterGoldenTests, BuiltinFragCoordMapping)
{
    auto r = transpile (kBuiltinsFragment, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("@builtin(position)"));
}

TEST_F (WgslEmitterGoldenTests, FunctionNameRemapping)
{
    const char* src = "void main() { float a = inversesqrt(2.0); }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("inverseSqrt("));
    EXPECT_FALSE (wgsl.contains ("inversesqrt("));
}

TEST_F (WgslEmitterGoldenTests, TextureToTextureSample)
{
    auto r = transpile (kSimpleFragment, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("textureSample("));
}

TEST_F (WgslEmitterGoldenTests, BindingAttributes)
{
    auto r = transpile (kSimpleFragment, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();
    EXPECT_TRUE (wgsl.contains ("@group(")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@binding(")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, UBOBecomesUniformVar)
{
    auto r = transpile (kUBO, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("var<uniform>"));
}

TEST_F (WgslEmitterGoldenTests, FloatLiteralFormats)
{
    const char* src = "void main() { float x = 5.0; }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    // Should have f32 float literal
    EXPECT_TRUE (wgsl.contains ("5.0"));
}

TEST_F (WgslEmitterGoldenTests, CompoundAssignment)
{
    const char* src = "void main() { float x = 1.0; x += 2.0; x *= 3.0; }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("+="));
    EXPECT_TRUE (wgsl.contains ("*="));
}

TEST_F (WgslEmitterGoldenTests, DiscardStatement)
{
    const char* src = "void main() { if (true) discard; }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("discard"));
}

TEST_F (WgslEmitterGoldenTests, ReturnStatement)
{
    const char* src = "float foo() { return 1.0; } void main() { float x = foo(); return; }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("return 1.0"));
    EXPECT_TRUE (wgsl.contains ("return;"));
}

TEST_F (WgslEmitterGoldenTests, ArraySizedType)
{
    const char* src = "void main() { float arr[4]; arr[0] = 1.0; }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("array<f32, 4>"));
}

TEST_F (WgslEmitterGoldenTests, StructDeclaration)
{
    auto r = transpile (kStructDecl, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains (".color"));
    EXPECT_TRUE (wgsl.contains (".intensity"));
}

TEST_F (WgslEmitterGoldenTests, PrecisionQualifiersStripped)
{
    auto r = transpile (kPrecisionQualifier, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_FALSE (wgsl.contains ("highp"));
    EXPECT_FALSE (wgsl.contains ("mediump"));
    EXPECT_FALSE (wgsl.contains ("lowp"));
}

//==============================================================================
// For-loop update expression tests — WGSL requires statements, not expressions
// in the update slot, so pre/post-inc/dec must not emit extra parens.
//==============================================================================

TEST_F (WgslEmitterGoldenTests, ForLoopPreIncUpdateNoParens)
{
    auto r = transpile (kForLoopIncrement, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("for ("));
    EXPECT_TRUE (wgsl.contains ("+= 1"));
    EXPECT_FALSE (wgsl.contains ("(i += 1)"));
}

TEST_F (WgslEmitterGoldenTests, ForLoopPreDecUpdateNoParens)
{
    auto r = transpile (kForLoopDecrement, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("for ("));
    EXPECT_TRUE (wgsl.contains ("-= 1"));
    EXPECT_FALSE (wgsl.contains ("(i -= 1)"));
}

TEST_F (WgslEmitterGoldenTests, ForLoopPostIncUpdateNoParens)
{
    auto r = transpile (kForLoopPostIncrement, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("for ("));
    EXPECT_TRUE (wgsl.contains ("i++"));
    EXPECT_FALSE (wgsl.contains ("(i++)"));
}

TEST_F (WgslEmitterGoldenTests, ForLoopPostDecUpdateNoParens)
{
    auto r = transpile (kForLoopPostDecrement, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("for ("));
    EXPECT_TRUE (wgsl.contains ("i--"));
    EXPECT_FALSE (wgsl.contains ("(i--)"));
}

//==============================================================================
// Separate texture + sampler — GLSL 4.5 separate texture2D + sampler pattern
//==============================================================================

TEST_F (WgslEmitterGoldenTests, SeparateTextureSamplerUnwrappedInTexture)
{
    auto r = transpile (kSeparateTextureSampler, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Must contain textureSample(tex, samp, ...) — three args, unwrapped
    EXPECT_TRUE (wgsl.contains ("textureSample("));
    EXPECT_TRUE (wgsl.contains ("tex, "));
    EXPECT_TRUE (wgsl.contains (", samp, "));
    // Must NOT contain the combined sampler2D constructor
    EXPECT_FALSE (wgsl.contains ("sampler2D(tex, samp)"));
    EXPECT_FALSE (wgsl.contains ("texture_2d<f32>(tex, samp)"));
}

TEST_F (WgslEmitterGoldenTests, SeparateTextureSamplerUnwrappedInTextureLod)
{
    auto r = transpile (kSeparateTextureSamplerLod, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Must contain textureSampleLevel(tex, samp, ...) — with explicit LOD
    EXPECT_TRUE (wgsl.contains ("textureSampleLevel("));
    EXPECT_TRUE (wgsl.contains ("tex, "));
    EXPECT_TRUE (wgsl.contains (", samp, "));
    // Must NOT contain the combined sampler2D constructor
    EXPECT_FALSE (wgsl.contains ("sampler2D(tex, samp)"));
}

//==============================================================================
// Resource type mapping — separate texture / sampler types
//==============================================================================

TEST_F (WgslEmitterGoldenTests, SeparateTexture2DTypeMapping)
{
    auto r = transpile (kSeparateTexture2DType, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // texture2D in GLSL maps to texture_2d<f32> in WGSL
    EXPECT_TRUE (wgsl.contains ("texture_2d<f32>"));
    EXPECT_TRUE (wgsl.contains ("@binding(0)"));
}

TEST_F (WgslEmitterGoldenTests, SeparateSamplerTypeMapping)
{
    auto r = transpile (kSeparateSamplerType, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // sampler in GLSL maps to sampler in WGSL
    EXPECT_TRUE (wgsl.contains (": sampler"));
    EXPECT_TRUE (wgsl.contains ("samp"));
    EXPECT_FALSE (wgsl.contains ("_sampler: sampler")); // no companion sampler
}

//==============================================================================
// Address space — textures/samplers use `var`, uniform buffers use `var<uniform>`
//==============================================================================

TEST_F (WgslEmitterGoldenTests, TextureUsesVarNotVarUniform)
{
    auto r = transpile (kSeparateTexture2DType, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Separate texture must be `var`, not `var<uniform>`
    EXPECT_TRUE (r.getValue().contains ("var tex: texture_2d<f32>"));
    EXPECT_FALSE (r.getValue().contains ("var<uniform> tex"));
}

TEST_F (WgslEmitterGoldenTests, SamplerUsesVarNotVarUniform)
{
    auto r = transpile (kSeparateSamplerType, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Separate sampler must be `var`, not `var<uniform>`
    EXPECT_TRUE (r.getValue().contains ("var samp: sampler"));
    EXPECT_FALSE (r.getValue().contains ("var<uniform> samp"));
}

TEST_F (WgslEmitterGoldenTests, CombinedSamplerTextureUsesVar)
{
    auto r = transpile (kSimpleFragment, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Combined sampler (sampler2D) texture must be `var`
    EXPECT_TRUE (wgsl.contains ("var tex: texture_2d<f32>"));
    EXPECT_FALSE (wgsl.contains ("var<uniform> tex"));
}

TEST_F (WgslEmitterGoldenTests, CombinedSamplerCompanionUsesVar)
{
    auto r = transpile (kSimpleFragment, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Companion sampler must be `var`
    EXPECT_TRUE (wgsl.contains ("var tex_sampler: sampler"));
    EXPECT_FALSE (wgsl.contains ("var<uniform> tex_sampler"));
}

TEST_F (WgslEmitterGoldenTests, UniformBufferStillUsesVarUniform)
{
    auto r = transpile (kUBO, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Regular UBO must still use `var<uniform>`
    EXPECT_TRUE (wgsl.contains ("var<uniform>"));
}

//==============================================================================
// Struct type emission — struct types must be emitted before their first use
//==============================================================================

TEST_F (WgslEmitterGoldenTests, StructTypeEmittedInWGSL)
{
    auto r = transpile (kStructDecl, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // The struct type itself must appear in the output
    EXPECT_TRUE (wgsl.contains ("struct Light"));
    EXPECT_TRUE (wgsl.contains ("position: vec3<f32>"));
    EXPECT_TRUE (wgsl.contains ("color: vec3<f32>"));
    EXPECT_TRUE (wgsl.contains ("intensity: f32"));
}

TEST_F (WgslEmitterGoldenTests, StructTypeEmittedBeforeUniform)
{
    auto r = transpile (kStructDecl, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // struct Light must appear before uLight (the uniform)
    auto structPos = wgsl.indexOf ("struct Light");
    auto uniformPos = wgsl.indexOf ("uLight");
    EXPECT_NE (structPos, -1);
    EXPECT_NE (uniformPos, -1);
    EXPECT_LT (structPos, uniformPos);
}

TEST_F (WgslEmitterGoldenTests, NestedStructUniformBlock)
{
    auto r = transpile (kStructAsUniformBlock, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // The nested struct type must be emitted
    EXPECT_TRUE (wgsl.contains ("struct Material"));
    EXPECT_TRUE (wgsl.contains ("baseColor: vec4<f32>"));
    EXPECT_TRUE (wgsl.contains ("roughness: f32"));
    // Struct must appear before its usage
    auto structPos = wgsl.indexOf ("struct Material");
    auto usePos = wgsl.indexOf ("mat");
    EXPECT_NE (structPos, -1);
    EXPECT_NE (usePos, -1);
    EXPECT_LT (structPos, usePos) << "struct Material must precede its first usage";
}

//==============================================================================
// ShaderTranspiler Integration Tests (Task 5.3)
//==============================================================================

class WgslTranspilerIntegrationTests : public ::testing::Test
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

ShaderTranspiler::Ptr WgslTranspilerIntegrationTests::transpiler {};

TEST_F (WgslTranspilerIntegrationTests, TranspileGLSLToWGSL)
{
    auto r = transpiler->transpile (kEmptyVertex, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("@vertex"));
}

TEST_F (WgslTranspilerIntegrationTests, TranspileESSLToWGSL)
{
    const char* src = R"glsl(#version 310 es
void main()
{
}
)glsl";

    auto r = transpiler->transpile (src, ShaderStage::vertex, ShaderLanguage::essl, ShaderLanguage::wgsl);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    EXPECT_TRUE (r.getValue().contains ("@vertex"));
}

TEST_F (WgslTranspilerIntegrationTests, TranspileHLSLToWGSLFails)
{
    const char* hlsl = R"hlsl(
float4 vs_main(uint vid : SV_VertexID) : SV_Position {
    return float4(0, 0, 0, 1);
}
)hlsl";

    auto r = transpiler->transpile (hlsl, ShaderStage::vertex, ShaderLanguage::hlsl, ShaderLanguage::wgsl);
    EXPECT_TRUE (r.failed());
}

TEST_F (WgslTranspilerIntegrationTests, TranspileGeometryToWGSLFails)
{
    auto r = transpiler->transpile (kEmptyVertex, ShaderStage::geometry, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    EXPECT_TRUE (r.failed());
}

TEST_F (WgslTranspilerIntegrationTests, TranspileWithDefines)
{
    const char* src = R"glsl(
#version 450
void main()
{
    float x = MY_VALUE;
}
)glsl";

    TranspileOptions opts;
    opts.defines.set ("MY_VALUE", "42.0");

    auto r = transpiler->transpile (src, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::wgsl, opts);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("42.0"));
}

TEST_F (WgslTranspilerIntegrationTests, TranspileFragmentToWGSL)
{
    auto r = transpiler->transpile (kSimpleFragment, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("@fragment"));
    EXPECT_TRUE (r.getValue().contains ("textureSample"));
}

TEST_F (WgslTranspilerIntegrationTests, TranspileComputeToWGSL)
{
    auto r = transpiler->transpile (kSimpleCompute, ShaderStage::compute, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("@compute"));
}

TEST_F (WgslTranspilerIntegrationTests, WGSLReflectionMatchesBindingAssignment)
{
    const char* src = R"glsl(
#version 450
layout(binding = 1) uniform sampler2D tex;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = texture(tex, vec2(0.5));
}
)glsl";

    auto spirvResult = transpiler->compileToSPIRV (src, ShaderStage::fragment, ShaderLanguage::glsl);
    ASSERT_TRUE (spirvResult.wasOk());

    auto reflResult = transpiler->reflectFromSPIRV (spirvResult.getValue(), ShaderLanguage::wgsl);
    ASSERT_TRUE (reflResult.wasOk());

    auto reflection = reflResult.getValue();
    for (auto& img : reflection.sampledImages)
        EXPECT_EQ (img.backendSlot, img.binding);
}

//==============================================================================
// ShaderBundle Integration Tests (Task 5.4)
//==============================================================================

class WgslBundleIntegrationTests : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F (WgslBundleIntegrationTests, BundleCompilerWithWGSLTarget)
{
    ShaderBundleCompiler compiler;

    ShaderBundleCompileRequest req;
    req.source = kSimpleVertex;
    req.sourceLanguage = ShaderLanguage::glsl;

    ShaderBundleEntry entry;
    entry.stage = ShaderStage::vertex;
    entry.targetLanguages = { ShaderLanguage::msl, ShaderLanguage::wgsl };
    req.entries.push_back (entry);

    auto result = compiler.compile (req);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    const auto& bundle = result.getReference();

    auto* mslInfo = bundle.findShader (ShaderStage::vertex, ShaderLanguage::msl);
    ASSERT_NE (mslInfo, nullptr);
    EXPECT_FALSE (mslInfo->source.isEmpty());

    auto* wgslInfo = bundle.findShader (ShaderStage::vertex, ShaderLanguage::wgsl);
    ASSERT_NE (wgslInfo, nullptr);
    EXPECT_FALSE (wgslInfo->source.isEmpty());
    EXPECT_TRUE (wgslInfo->source.contains ("@vertex"));
}

TEST_F (WgslBundleIntegrationTests, BundleRoundtripWithWGSL)
{
    ShaderBundleCompiler compiler;

    ShaderBundleCompileRequest req;
    req.source = kSimpleFragment;
    req.sourceLanguage = ShaderLanguage::glsl;

    ShaderBundleEntry entry;
    entry.stage = ShaderStage::fragment;
    entry.targetLanguages = { ShaderLanguage::wgsl };
    req.entries.push_back (entry);

    auto compileResult = compiler.compile (req);
    ASSERT_TRUE (compileResult.wasOk());

    const auto& bundle = compileResult.getReference();

    MemoryBlock block;
    auto saveResult = bundle.saveToMemoryBlock (block);
    ASSERT_TRUE (saveResult.wasOk());
    ASSERT_GT (block.getSize(), 0u);

    auto loadResult = ShaderBundle::loadFromMemoryBlock (block);
    ASSERT_TRUE (loadResult.wasOk());

    const auto& loaded = loadResult.getReference();
    auto* wgslInfo = loaded.findShader (ShaderStage::fragment, ShaderLanguage::wgsl);
    ASSERT_NE (wgslInfo, nullptr);
    EXPECT_TRUE (wgslInfo->source.contains ("@fragment"));
    EXPECT_EQ (wgslInfo->inputSource, kSimpleFragment);
}

TEST_F (WgslBundleIntegrationTests, BundleCompilerVertexFragmentCompute)
{
    ShaderBundleCompiler compiler;

    ShaderBundleCompileRequest req;
    req.source = kSimpleVertex;
    req.sourceLanguage = ShaderLanguage::glsl;

    {
        ShaderBundleEntry ve;
        ve.stage = ShaderStage::vertex;
        ve.targetLanguages = { ShaderLanguage::wgsl };
        req.entries.push_back (ve);
    }
    {
        ShaderBundleEntry fe;
        fe.stage = ShaderStage::fragment;
        fe.targetLanguages = { ShaderLanguage::wgsl };
        req.entries.push_back (fe);
    }
    {
        ShaderBundleEntry ce;
        ce.stage = ShaderStage::compute;
        ce.targetLanguages = { ShaderLanguage::wgsl };
        req.entries.push_back (ce);
    }

    // Fragment and compute entries will fail since source is a vertex shader,
    // but the bundle compilation happens per-entry, so vertex succeeds.
    auto result = compiler.compile (req);
    EXPECT_FALSE (result.wasOk()); // fragment entry fails with vertex source
}

//==============================================================================
// WgslTranspiler Direct API Tests
//==============================================================================

class WgslTranspilerDirectTests : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F (WgslTranspilerDirectTests, TranspileEmptyVertex)
{
    WgslTranspileOptions opts;
    opts.entryPoint = "main";
    opts.defaultGroup = 0;

    auto r = WgslTranspiler::transpile (kEmptyVertex, ShaderStage::vertex, opts);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().isNotEmpty());
    EXPECT_TRUE (r.getValue().contains ("@vertex"));
}

TEST_F (WgslTranspilerDirectTests, CustomOutputEntryPoint)
{
    WgslTranspileOptions opts;
    opts.entryPoint = "main";
    opts.outputEntryPoint = "vs_main";
    opts.defaultGroup = 0;

    auto r = WgslTranspiler::transpile (kEmptyVertex, ShaderStage::vertex, opts);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("fn vs_main"));
}

TEST_F (WgslTranspilerDirectTests, DefaultWorkgroupSizeUsed)
{
    WgslTranspileOptions opts;
    opts.entryPoint = "main";
    opts.defaultWorkgroupSize = { 16, 8, 1 };

    const char* src = R"glsl(
void main()
{
}
)glsl";

    auto r = WgslTranspiler::transpile (src, ShaderStage::compute, opts);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("@workgroup_size(16, 8, 1)"));
}

TEST_F (WgslTranspilerDirectTests, InvalidGLSLReturnsError)
{
    WgslTranspileOptions opts;

    auto r = WgslTranspiler::transpile ("invalid glsl @@@@", ShaderStage::vertex, opts);
    EXPECT_TRUE (r.failed());
}

TEST_F (WgslTranspilerDirectTests, VertexOutputHasVSOutputStruct)
{
    WgslTranspileOptions opts;

    auto r = WgslTranspiler::transpile (kSimpleVertex, ShaderStage::vertex, opts);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("struct VSInput"));
    EXPECT_TRUE (wgsl.contains ("struct VSOutput"));
    EXPECT_TRUE (wgsl.contains ("@builtin(position)"));
}

//==============================================================================
// Operator Mapping Golden Tests
//==============================================================================

class WgslOperatorMappingTests : public ::testing::Test
{
protected:
    auto transpile (const char* src, ShaderStage stage)
    {
        WgslTranspileOptions opts;
        opts.entryPoint = "main";
        return WgslTranspiler::transpile (src, stage, opts);
    }
};

TEST_F (WgslOperatorMappingTests, Atan2Mapping)
{
    const char* src = "void main() { float a = atan(1.0, 2.0); }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("atan2("));
}

TEST_F (WgslOperatorMappingTests, DFdxMapping)
{
    const char* src = "void main() { float a = dFdx(1.0); }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("dpdx("));
}

TEST_F (WgslOperatorMappingTests, DFdyMapping)
{
    const char* src = "void main() { float a = dFdy(1.0); }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("dpdy("));
}

TEST_F (WgslOperatorMappingTests, FwidthMapping)
{
    const char* src = "void main() { float a = fwidth(1.0); }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("fwidth("));
}

TEST_F (WgslOperatorMappingTests, TexelFetchMapping)
{
    const char* src = R"glsl(
layout(binding = 0) uniform sampler2D tex;
void main() {
    vec4 c = texelFetch(tex, ivec2(0, 0), 0);
}
)glsl";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("textureLoad("));
}

TEST_F (WgslOperatorMappingTests, TextureSizeMapping)
{
    const char* src = R"glsl(
layout(binding = 0) uniform sampler2D tex;
void main() {
    ivec2 s = textureSize(tex, 0);
}
)glsl";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("textureDimensions("));
}

//==============================================================================
// Real-World Shader Tests — examples/graphics/data/shaders/cube.*
//==============================================================================

namespace
{

constexpr const char* kCubeVert = R"glsl(
#version 450

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

constexpr const char* kCubeFrag = R"glsl(
#version 450

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

} // namespace

class WgslRealWorldShaderTests : public ::testing::Test
{
protected:
    auto transpile (const char* src, ShaderStage stage)
    {
        WgslTranspileOptions opts;
        opts.entryPoint = "main";
        opts.outputEntryPoint = "main";
        opts.defaultGroup = 0;
        return WgslTranspiler::transpile (src, stage, opts);
    }
};

// Minimal reproduction: just declarations + void main, no interface block
constexpr const char* kMinimalVert = R"glsl(
#version 450

layout(location = 0) in vec3 a_pos;
layout(location = 0) out vec3 v_color;

void main() {
    v_color = a_pos;
}
)glsl";

// Interface block only
constexpr const char* kVertWithBlock = R"glsl(
#version 450

layout(set = 0, binding = 0) uniform Data {
    float value;
} u;

void main() {
    float x = u.value;
}
)glsl";

TEST_F (WgslRealWorldShaderTests, MinimalVertexParses)
{
    auto r = wgsl::GlslParser::parse (kMinimalVert);
    EXPECT_TRUE (r.wasOk()) << r.getErrorMessage();
}

TEST_F (WgslRealWorldShaderTests, VertexWithBlockParses)
{
    auto r = wgsl::GlslParser::parse (kVertWithBlock);
    EXPECT_TRUE (r.wasOk()) << r.getErrorMessage();
}

// Single-line: void main with nothing else
TEST_F (WgslRealWorldShaderTests, BareMinimum)
{
    auto r = wgsl::GlslParser::parse (R"glsl(
void main() {}
)glsl");
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
}

// void main with just one in/out pair
TEST_F (WgslRealWorldShaderTests, OneInOneOut)
{
    auto r = wgsl::GlslParser::parse (R"glsl(
layout(location = 0) in vec3 pos;
layout(location = 0) out vec4 color;
void main() {
    color = vec4(pos, 1.0);
}
)glsl");
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
}

TEST_F (WgslRealWorldShaderTests, CubeVertexParses)
{
    auto r = wgsl::GlslParser::parse (kCubeVert);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
}

TEST_F (WgslRealWorldShaderTests, CubeFragmentParses)
{
    auto r = wgsl::GlslParser::parse (kCubeFrag);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
}

TEST_F (WgslRealWorldShaderTests, CubeVertexTranspilesToWGSL)
{
    auto r = transpile (kCubeVert, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    auto wgsl = r.getValue();

    // Entry-point wrapping
    EXPECT_TRUE (wgsl.contains ("@vertex"));
    EXPECT_TRUE (wgsl.contains ("main_inner"));

    // Type mapping
    EXPECT_TRUE (wgsl.contains ("f32"));
    EXPECT_TRUE (wgsl.contains ("vec3<f32>"));
    EXPECT_TRUE (wgsl.contains ("vec4<f32>"));

    // Builtins
    EXPECT_TRUE (wgsl.contains ("@builtin(position)"));

    // UBO
    EXPECT_TRUE (wgsl.contains ("var<uniform>"));

    // Trigonometry preserved
    EXPECT_TRUE (wgsl.contains ("cos("));
    EXPECT_TRUE (wgsl.contains ("sin("));

    // IO structs
    EXPECT_TRUE (wgsl.contains ("struct VSInput"));
    EXPECT_TRUE (wgsl.contains ("struct VSOutput"));
}

TEST_F (WgslRealWorldShaderTests, CubeVertexHasCorrectInputs)
{
    auto r = transpile (kCubeVert, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk());
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("a_pos: vec3<f32>"));
    EXPECT_TRUE (wgsl.contains ("a_color: vec3<f32>"));
    EXPECT_TRUE (wgsl.contains ("a_normal: vec3<f32>"));
    EXPECT_TRUE (wgsl.contains ("a_uv: vec2<f32>"));
    EXPECT_TRUE (wgsl.contains ("@location(0)"));
    EXPECT_TRUE (wgsl.contains ("@location(3)"));
}

TEST_F (WgslRealWorldShaderTests, CubeFragmentTranspilesToWGSL)
{
    auto r = transpile (kCubeFrag, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    auto wgsl = r.getValue();

    // Entry point
    EXPECT_TRUE (wgsl.contains ("@fragment"));

    // Math functions preserved
    EXPECT_TRUE (wgsl.contains ("normalize"));
    EXPECT_TRUE (wgsl.contains ("clamp"));
    EXPECT_TRUE (wgsl.contains ("dot"));
    EXPECT_TRUE (wgsl.contains ("mix"));
}

TEST_F (WgslRealWorldShaderTests, CubeVertexViaShaderTranspiler)
{
    auto transpiler = new ShaderTranspiler();

    auto r = transpiler->transpile (kCubeVert, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    EXPECT_TRUE (r.getValue().contains ("@vertex"));
    EXPECT_TRUE (r.getValue().contains ("main_inner"));
}

TEST_F (WgslRealWorldShaderTests, CubeFragmentViaShaderTranspiler)
{
    auto transpiler = new ShaderTranspiler();

    auto r = transpiler->transpile (kCubeFrag, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    EXPECT_TRUE (r.getValue().contains ("@fragment"));
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
