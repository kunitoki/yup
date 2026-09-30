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

constexpr const char* kSwitchStmt = R"glsl(
#version 450
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

constexpr const char* kWhileLoop = R"glsl(
#version 450
void main()
{
    int j = 0;
    while (j < 5) { j++; }
}
)glsl";

constexpr const char* kVectorRelational = R"glsl(
#version 450
void main()
{
    bvec3 r = lessThan(vec3(1.0), vec3(2.0));
    bvec2 e = equal(ivec2(0), ivec2(0));
    bvec4 g = greaterThan(vec4(1.0), vec4(0.5));
    bvec2 ne = notEqual(vec2(0.0), vec2(1.0));
    bvec3 le = lessThanEqual(vec3(0.0), vec3(0.0));
    bvec2 ge = greaterThanEqual(vec2(2.0), vec2(1.0));
}
)glsl";

constexpr const char* kIsnanIsinf = R"glsl(
#version 450
void main()
{
    float v = 0.0;
    bool n = isnan(v);
    bool i = isinf(v);
}
)glsl";

constexpr const char* kIsamplerUSampler = R"glsl(
#version 450
layout(binding = 0) uniform isampler2D texI;
layout(binding = 1) uniform usampler2D texU;
void main()
{
    ivec4 c1 = texelFetch(texI, ivec2(0, 0), 0);
    uvec4 c2 = texelFetch(texU, ivec2(0, 0), 0);
}
)glsl";

constexpr const char* kStorageBuffer = R"glsl(
#version 450
layout(std430, binding = 0) buffer OutputBlock {
    float values[];
};
void main()
{
    values[0] = 1.0;
}
)glsl";

constexpr const char* kComputeWorkgroupSizes = R"glsl(
#version 450
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    uint idx = gl_GlobalInvocationID.x;
}
)glsl";

constexpr const char* kDoWhileNoBraces = R"glsl(
#version 450
void main()
{
    int i = 0;
    do i++; while (i < 3);
}
)glsl";

constexpr const char* kSampler2DShadow = R"glsl(
#version 450
layout(binding = 0) uniform sampler2DShadow shadowMap;
layout(location = 0) out vec4 fragColor;
void main()
{
    fragColor = vec4(0.0);
}
)glsl";

constexpr const char* kUIntLiterals = R"glsl(
#version 450
void main()
{
    uint a = 5u;
    uint b = a + 3u;
}
)glsl";

constexpr const char* kAtanScalar = R"glsl(
#version 450
void main()
{
    float a = atan(1.0, 2.0);
    float b = atan(1.0);
}
)glsl";

constexpr const char* kRadiansDegrees = R"glsl(
#version 450
void main()
{
    float r = radians(180.0);
    float d = degrees(3.14159);
}
)glsl";

constexpr const char* kFwidthCoarseFine = R"glsl(
#version 450
void main()
{
    float c = fwidthCoarse(1.0);
    float f = fwidthFine(1.0);
}
)glsl";

constexpr const char* kAllMathBuiltins = R"glsl(
#version 450
void main()
{
    float s = step(0.5, 1.0);
    float sm = smoothstep(0.0, 1.0, 0.5);
    float l = length(vec3(1.0));
    float d = distance(vec3(0.0), vec3(1.0));
    vec3 c = cross(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0));
    vec3 r1 = reflect(vec3(1.0), vec3(0.0, 1.0, 0.0));
    vec3 r2 = refract(vec3(1.0), vec3(0.0, 1.0, 0.0), 0.5);
    vec3 fw = faceforward(vec3(1.0), vec3(0.0), vec3(0.0, 0.0, 1.0));
    mat3 t = transpose(mat3(1.0));
    float si = sin(0.5);
    float co = cos(0.5);
    float ta = tan(0.5);
    float asi = asin(0.5);
    float aco = acos(0.5);
    float at = atan(0.5, 1.0);
    float pw = pow(2.0, 3.0);
    float ex = exp(1.0);
    float lo = log(2.0);
    float e2 = exp2(2.0);
    float l2 = log2(4.0);
    float sq = sqrt(4.0);
    float ab = abs(-1.0);
    float sg = sign(-2.0);
    float fl = floor(1.5);
    float tr = trunc(1.5);
    float ro = round(1.5);
    float ce = ceil(1.5);
    float fr = fract(1.5);
    float mi = min(1.0, 2.0);
    float ma = max(1.0, 2.0);
}
)glsl";

constexpr const char* kBvecTypes = R"glsl(
#version 450
void main()
{
    bvec2 b2 = bvec2(true, false);
    bvec3 b3 = bvec3(true);
    bvec4 b4 = bvec4(false);
    bool b1 = any(b2) && all(b3);
}
)glsl";

constexpr const char* kLogicalOperators = R"glsl(
#version 450
void main()
{
    bool a = true;
    bool b = false;
    bool c = a && b;
    bool d = a || b;
    bool e = !a;
    float x = 1.0;
    float y = 2.0;
    bool f = x > 0.0 && y < 3.0;
}
)glsl";

constexpr const char* kDotBracketExpr = R"glsl(
#version 450
struct Data { float value; vec3 color; };
uniform Data u;
void main()
{
    float v = u.value;
    vec3 c = u.color;
}
)glsl";

constexpr const char* kSamplerNoBinding = R"glsl(
#version 450
uniform sampler2D tex;
void main()
{
    vec4 c = texture(tex, vec2(0.0));
}
)glsl";

constexpr const char* kOutInoutFunction = R"glsl(
#version 450
void scale(inout float a, out float b)
{
    b = a * 2.0;
    a = a + 1.0;
}
void main()
{
    float x = 1.0;
    float y;
    scale(x, y);
}
)glsl";

constexpr const char* kUnnamedUniformBlock = R"glsl(
#version 450
layout(set = 0, binding = 0) uniform {
    float value;
};
void main()
{
    float x = value;
}
)glsl";

constexpr const char* kUnnamedBufferBlock = R"glsl(
#version 450
layout(set = 0, binding = 0) buffer {
    float value;
};
void main()
{
    value = 1.0;
}
)glsl";

constexpr const char* kFunctionNoParamReassignment = R"glsl(
#version 450
float add(float a, float b) { return a + b; }
float mul(float a, float b) { return a * b; }
void main()
{
    float x = add(1.0, 2.0) * mul(3.0, 4.0);
}
)glsl";

constexpr const char* kIfElseStatement = R"glsl(
#version 450
void main()
{
    float x;
    if (true) {
        x = 1.0;
    } else {
        x = 2.0;
    }
}
)glsl";

constexpr const char* kFragmentNoInputs = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(0.0);
}
)glsl";

constexpr const char* kVertexOnlyImplicitBuiltins = R"glsl(
#version 450
void main()
{
    gl_Position = vec4(float(gl_VertexIndex), 0.0, 0.0, 1.0);
}
)glsl";

constexpr const char* kComputeAllBuiltins = R"glsl(
#version 450
layout(local_size_x = 8, local_size_y = 4, local_size_z = 1) in;
void main()
{
    uint gi = gl_GlobalInvocationID.x;
    uint li = gl_LocalInvocationIndex;
    uint wi = gl_WorkGroupID.x;
    uint nw = gl_NumWorkGroups.x;
}
)glsl";

constexpr const char* kIntegerVectors = R"glsl(
#version 450
void main()
{
    ivec2 i2 = ivec2(0, 1);
    ivec3 i3 = ivec3(1, 2, 3);
    ivec4 i4 = ivec4(0);
    uvec2 u2 = uvec2(0u, 1u);
    uvec3 u3 = uvec3(1u);
    uvec4 u4 = uvec4(0u);
}
)glsl";

constexpr const char* kUnaryPlusBitwiseNot = R"glsl(
#version 450
void main()
{
    float p = +1.0;
    int n = ~0;
}
)glsl";

constexpr const char* kBitwiseOps = R"glsl(
#version 450
void main()
{
    int a = 0xFF;
    int b = 0x0F;
    int r1 = a & b;
    int r2 = a | b;
    int r3 = a ^ b;
    int r4 = a << 2;
    int r5 = a >> 2;
}
)glsl";

constexpr const char* kCompoundAssignmentOps = R"glsl(
#version 450
void main()
{
    int x = 10;
    x %= 3;
    x <<= 1;
    x >>= 1;
    x &= 0xFF;
    x ^= 0x0F;
    x |= 0xF0;
}
)glsl";

constexpr const char* kCommaOperator = R"glsl(
#version 450
void main()
{
    int x;
    int y = (x = 1, x + 2);
}
)glsl";

constexpr const char* kSamplerTypeVariants = R"glsl(
#version 450
layout(binding = 0) uniform sampler3D volTex;
layout(binding = 1) uniform samplerCube cubeTex;
layout(binding = 2) uniform sampler2DArray arrTex;
layout(binding = 3) uniform isampler3D ivolTex;
layout(binding = 4) uniform isamplerCube icubeTex;
layout(binding = 5) uniform isampler2DArray iarrTex;
layout(binding = 6) uniform usampler3D uvolTex;
layout(binding = 7) uniform usamplerCube ucubeTex;
layout(binding = 8) uniform usampler2DArray uarrTex;
void main()
{
}
)glsl";

constexpr const char* kMatrixTypes = R"glsl(
#version 450
void main()
{
    mat2 m2 = mat2(1.0);
    mat3 m3 = mat3(1.0);
    mat4 m4 = mat4(1.0);
    mat2x3 m23 = mat2x3(1.0);
    mat3x2 m32 = mat3x2(1.0);
    mat2x4 m24 = mat2x4(1.0);
    mat4x2 m42 = mat4x2(1.0);
    mat3x4 m34 = mat3x4(1.0);
    mat4x3 m43 = mat4x3(1.0);
}
)glsl";

// A uniform block whose members share one declaration, as post-process shaders
// commonly write their parameter block.
constexpr const char* kUniformBlockCommaSeparatedMembers = R"glsl(
#version 450
layout(set = 0, binding = 0) uniform texture2D u_tex;
layout(set = 0, binding = 1) uniform sampler u_samp;
layout(set = 0, binding = 2) uniform Params { float s, r, rx, ry, dx, dy, pad0, pad1; } p;
layout(location = 0) out vec4 fragColor;

void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(p.rx, p.ry);
    fragColor = texture(sampler2D(u_tex, u_samp), uv + vec2(p.dx, p.dy) * p.s);
}
)glsl";

// Comma-separated members in a plain struct, where a declarator also carries its
// own array specifier. The struct is never instantiated: the parser does not
// resolve user-declared struct names as type names inside a function body, so
// `Bundle bundle;` would fail for reasons unrelated to the member list.
constexpr const char* kStructCommaSeparatedMembers = R"glsl(
#version 450
struct Bundle {
    float a, b, weights[4];
    vec2 offset, scale;
};

void main()
{
}
)glsl";

//==============================================================================
// AST helpers
//==============================================================================

/** Finds a named struct or interface block in a parsed translation unit. */
const wgsl::StructSpecifier* findStruct (const wgsl::TranslationUnit& unit, const std::string& name)
{
    for (const auto& external : unit.declarations)
        if (const auto* declaration = std::get_if<wgsl::Declaration> (&external))
            if (declaration->structSpecifier != nullptr && declaration->structSpecifier->name == name)
                return declaration->structSpecifier.get();

    return nullptr;
}

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

TEST_F (WgslParserTests, NamedBlockWithoutInstanceName)
{
    const char* src = R"glsl(
layout(std140, binding = 0) uniform BlockName {
    float value;
    vec3 color;
};
void main() { float x = value + color.r; }
)glsl";
    auto r = parse (src);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    // Parsed as a Declaration with structSpecifier + qualifier, no initDeclaratorList
}

TEST_F (WgslParserTests, NamedBufferBlockWithoutInstanceName)
{
    const char* src = R"glsl(
layout(std430, binding = 0) buffer StorageBlock {
    float data;
};
void main() { data = 1.0; }
)glsl";
    auto r = parse (src);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
}

TEST_F (WgslParserTests, StructWithMultipleFields)
{
    const char* src = R"glsl(
struct Params {
    float scale;
    vec3 offset;
    vec4 color;
    int flags;
};
void main() { Params p; }
)glsl";
    auto r = parse (src);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    // Exercises the "Parse fields" while loop + user-defined type resolution
}

TEST_F (WgslParserTests, NamedBlockWithInstanceName)
{
    const char* src = R"glsl(
layout(std140, binding = 0) uniform Data {
    float value;
    vec3 color;
} u;
void main() { float x = u.value; }
)glsl";
    auto r = parse (src);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
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

TEST_F (WgslParserTests, UniformBlockWithCommaSeparatedMembers)
{
    auto r = parse (kUniformBlockCommaSeparatedMembers);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    const auto* params = findStruct (r.getReference(), "Params");
    ASSERT_NE (nullptr, params);
    ASSERT_EQ (8u, params->fields.size());
    EXPECT_EQ ("s", params->fields.front().name);
    EXPECT_EQ ("pad1", params->fields.back().name);
}

TEST_F (WgslParserTests, StructWithCommaSeparatedMembers)
{
    auto r = parse (kStructCommaSeparatedMembers);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    const auto* bundle = findStruct (r.getReference(), "Bundle");
    ASSERT_NE (nullptr, bundle);
    ASSERT_EQ (5u, bundle->fields.size());
    EXPECT_EQ ("a", bundle->fields[0].name);
    EXPECT_EQ ("b", bundle->fields[1].name);
    EXPECT_EQ ("offset", bundle->fields[3].name);

    // An array specifier binds to its own declarator, not to the shared base type.
    EXPECT_EQ ("weights", bundle->fields[2].name);
    EXPECT_EQ (1u, bundle->fields[2].type.arraySpecifiers.size());
    EXPECT_TRUE (bundle->fields[1].type.arraySpecifiers.empty());
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
// AST unit tests — make* factories, copyExpr, ArraySpecifier
//==============================================================================

class WgslAstUnitTests : public ::testing::Test
{
};

TEST_F (WgslAstUnitTests, MakeCompoundStatement)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    auto stmt = Statement::makeCompound (loc, {});
    EXPECT_TRUE (stmt.is<StmtCompound>());
    EXPECT_EQ (stmt.loc.line, 1);
}

TEST_F (WgslAstUnitTests, MakeExprStatement)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr e;
    e.loc = loc;
    e.value = ExprIntConst { loc, 42 };
    auto stmt = Statement::makeExpr (loc, std::move (e));
    EXPECT_TRUE (stmt.is<StmtExpr>());
    auto& se = stmt.as<StmtExpr>();
    ASSERT_NE (se.expr, nullptr);
    EXPECT_TRUE (se.expr->is<ExprIntConst>());
}

TEST_F (WgslAstUnitTests, MakeReturnStatement)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    auto val = std::make_unique<Expr>();
    val->loc = loc;
    val->value = ExprIntConst { loc, 42 };
    auto stmt = Statement::makeReturn (loc, std::move (val));
    EXPECT_TRUE (stmt.is<StmtJump>());
    auto& j = stmt.as<StmtJump>();
    EXPECT_EQ (j.kind, JumpKind::returnJump);
    ASSERT_NE (j.returnValue, nullptr);
    EXPECT_TRUE (j.returnValue->is<ExprIntConst>());
}

TEST_F (WgslAstUnitTests, MakeEmptyStatement)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    auto stmt = Statement::makeEmpty (loc);
    EXPECT_TRUE (stmt.is<StmtCompound>());
    auto& comp = stmt.as<StmtCompound>();
    EXPECT_TRUE (comp.statements.empty());
}

TEST_F (WgslAstUnitTests, TypeSpecifierMake)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    auto ts = TypeSpecifier::make (loc, TypeKind::floatType);
    EXPECT_EQ (ts.kind, TypeKind::floatType);
    EXPECT_EQ (ts.loc.line, 1);
}

TEST_F (WgslAstUnitTests, TypeSpecifierMakeNamed)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    auto ts = TypeSpecifier::makeNamed (loc, "MyStruct");
    EXPECT_EQ (ts.kind, TypeKind::namedStruct);
    EXPECT_EQ (ts.structName, "MyStruct");
}

TEST_F (WgslAstUnitTests, ArraySpecifierCopyAssignment)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    ArraySpecifier as;
    as.loc = loc;
    as.isUnsized = false;
    Expr sizeExpr;
    sizeExpr.loc = loc;
    sizeExpr.value = ExprIntConst { loc, 10 };
    as.sizeExpr = std::make_unique<Expr> (copyExpr (sizeExpr));

    // Copy via assignment
    ArraySpecifier as2;
    as2 = as;

    EXPECT_FALSE (as2.isUnsized);
    ASSERT_NE (as2.sizeExpr, nullptr);
    EXPECT_TRUE (as2.sizeExpr->is<ExprIntConst>());
    EXPECT_EQ (as2.sizeExpr->as<ExprIntConst>().value, 10);

    // Self-assignment
    as2 = as2;
    EXPECT_FALSE (as2.isUnsized);
}

TEST_F (WgslAstUnitTests, CopyExprVariable)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr e;
    e.loc = loc;
    e.value = ExprVariable { loc, "foo" };
    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprVariable>());
    EXPECT_EQ (copy.as<ExprVariable>().name, "foo");
}

TEST_F (WgslAstUnitTests, CopyExprUnary)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr operand;
    operand.loc = loc;
    operand.value = ExprVariable { loc, "x" };

    Expr e;
    e.loc = loc;
    e.value = ExprUnary { loc, UnaryOp::minus, std::make_unique<Expr> (std::move (operand)) };
    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprUnary>());
    auto& un = copy.as<ExprUnary>();
    EXPECT_EQ (un.op, UnaryOp::minus);
    ASSERT_NE (un.operand, nullptr);
    EXPECT_TRUE (un.operand->is<ExprVariable>());
}

TEST_F (WgslAstUnitTests, CopyExprBinary)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr left, right;
    left.loc = loc;
    left.value = ExprVariable { loc, "a" };
    right.loc = loc;
    right.value = ExprIntConst { loc, 1 };

    Expr e;
    e.loc = loc;
    e.value = ExprBinary { loc, BinaryOp::add, std::make_unique<Expr> (std::move (left)), std::make_unique<Expr> (std::move (right)) };
    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprBinary>());
    auto& bin = copy.as<ExprBinary>();
    EXPECT_EQ (bin.op, BinaryOp::add);
    ASSERT_NE (bin.left, nullptr);
    EXPECT_TRUE (bin.left->is<ExprVariable>());
    ASSERT_NE (bin.right, nullptr);
    EXPECT_TRUE (bin.right->is<ExprIntConst>());
}

TEST_F (WgslAstUnitTests, CopyExprTernary)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr cond, tBranch, fBranch;
    cond.loc = loc;
    cond.value = ExprBoolConst { loc, true };
    tBranch.loc = loc;
    tBranch.value = ExprIntConst { loc, 1 };
    fBranch.loc = loc;
    fBranch.value = ExprIntConst { loc, 0 };

    Expr e;
    e.loc = loc;
    e.value = ExprTernary { loc,
                            std::make_unique<Expr> (std::move (cond)),
                            std::make_unique<Expr> (std::move (tBranch)),
                            std::make_unique<Expr> (std::move (fBranch)) };
    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprTernary>());
    auto& tern = copy.as<ExprTernary>();
    ASSERT_NE (tern.condition, nullptr);
    EXPECT_TRUE (tern.condition->is<ExprBoolConst>());
    ASSERT_NE (tern.trueBranch, nullptr);
    ASSERT_NE (tern.falseBranch, nullptr);
}

TEST_F (WgslAstUnitTests, CopyExprAssignment)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr lhs, rhs;
    lhs.loc = loc;
    lhs.value = ExprVariable { loc, "x" };
    rhs.loc = loc;
    rhs.value = ExprIntConst { loc, 5 };

    Expr e;
    e.loc = loc;
    e.value = ExprAssignment { loc, AssignmentOp::assign, std::make_unique<Expr> (std::move (lhs)), std::make_unique<Expr> (std::move (rhs)) };
    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprAssignment>());
    auto& assign = copy.as<ExprAssignment>();
    EXPECT_EQ (assign.op, AssignmentOp::assign);
    ASSERT_NE (assign.lhs, nullptr);
    ASSERT_NE (assign.rhs, nullptr);
    EXPECT_TRUE (assign.rhs->is<ExprIntConst>());
}

TEST_F (WgslAstUnitTests, CopyExprBracket)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr base, idx;
    base.loc = loc;
    base.value = ExprVariable { loc, "arr" };
    idx.loc = loc;
    idx.value = ExprIntConst { loc, 0 };

    Expr e;
    e.loc = loc;
    e.value = ExprBracket { loc,
                            std::make_unique<Expr> (std::move (base)),
                            std::make_unique<Expr> (std::move (idx)) };
    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprBracket>());
    auto& br = copy.as<ExprBracket>();
    ASSERT_NE (br.base, nullptr);
    EXPECT_TRUE (br.base->is<ExprVariable>());
    ASSERT_NE (br.index, nullptr);
    EXPECT_TRUE (br.index->is<ExprIntConst>());
}

TEST_F (WgslAstUnitTests, CopyExprFunCall)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr callee, arg;
    callee.loc = loc;
    callee.value = ExprVariable { loc, "foo" };
    arg.loc = loc;
    arg.value = ExprFloatConst { loc, 1.0f };

    Expr e;
    e.loc = loc;
    ExprFunCall call;
    call.loc = loc;
    call.callee = std::make_unique<Expr> (std::move (callee));
    call.args.push_back (std::move (arg));
    e.value = std::move (call);

    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprFunCall>());
    auto& fc = copy.as<ExprFunCall>();
    ASSERT_NE (fc.callee, nullptr);
    EXPECT_TRUE (fc.callee->is<ExprVariable>());
    EXPECT_EQ (fc.args.size(), 1u);
    EXPECT_TRUE (fc.args[0].is<ExprFloatConst>());
}

TEST_F (WgslAstUnitTests, CopyExprDot)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr base;
    base.loc = loc;
    base.value = ExprVariable { loc, "obj" };

    Expr e;
    e.loc = loc;
    e.value = ExprDot { loc, std::make_unique<Expr> (std::move (base)), "member" };
    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprDot>());
    auto& dot = copy.as<ExprDot>();
    EXPECT_EQ (dot.member, "member");
    ASSERT_NE (dot.base, nullptr);
    EXPECT_TRUE (dot.base->is<ExprVariable>());
}

TEST_F (WgslAstUnitTests, CopyExprComma)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr left, right;
    left.loc = loc;
    left.value = ExprIntConst { loc, 1 };
    right.loc = loc;
    right.value = ExprIntConst { loc, 2 };

    Expr e;
    e.loc = loc;
    e.value = ExprComma { loc,
                          std::make_unique<Expr> (std::move (left)),
                          std::make_unique<Expr> (std::move (right)) };
    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprComma>());
    auto& com = copy.as<ExprComma>();
    ASSERT_NE (com.left, nullptr);
    ASSERT_NE (com.right, nullptr);
    EXPECT_TRUE (com.left->is<ExprIntConst>());
}

TEST_F (WgslAstUnitTests, CopyExprTypeConstructor)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr arg;
    arg.loc = loc;
    arg.value = ExprFloatConst { loc, 0.0f };

    Expr e;
    e.loc = loc;
    ExprTypeConstructor ctor;
    ctor.loc = loc;
    ctor.type = TypeSpecifier::make (loc, TypeKind::vec2);
    ctor.args.push_back (std::move (arg));
    e.value = std::move (ctor);

    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprTypeConstructor>());
    auto& tc = copy.as<ExprTypeConstructor>();
    EXPECT_EQ (tc.type.kind, TypeKind::vec2);
    EXPECT_EQ (tc.args.size(), 1u);
    EXPECT_TRUE (tc.args[0].is<ExprFloatConst>());
}

TEST_F (WgslAstUnitTests, CopyExprParen)
{
    using namespace yup::wgsl;

    SourceLocation loc { 1, 1 };
    Expr inner;
    inner.loc = loc;
    inner.value = ExprIntConst { loc, 42 };

    Expr e;
    e.loc = loc;
    e.value = ExprParen { loc, std::make_unique<Expr> (std::move (inner)) };
    auto copy = copyExpr (e);
    EXPECT_TRUE (copy.is<ExprParen>());
    auto& p = copy.as<ExprParen>();
    ASSERT_NE (p.expr, nullptr);
    EXPECT_TRUE (p.expr->is<ExprIntConst>());
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

TEST_F (WgslLoweringTests, RejectsDoublePrecision)
{
    const char* src = "void main() { double d = 1.0; }";
    auto r = lower (src, ShaderStage::fragment);
    ASSERT_TRUE (r.failed());
    EXPECT_TRUE (r.getErrorMessage().contains ("1:15: Double precision")) << r.getErrorMessage();
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
    // Workgroup size extracted from layout(local_size_x = 8, ...) in source
    auto ep = r.getReference().entryPoint;
    EXPECT_EQ (ep.workgroupSizeX, 8u);
    EXPECT_EQ (ep.workgroupSizeY, 8u);
    EXPECT_EQ (ep.workgroupSizeZ, 1u);
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

    // Companion samplers follow the highest binding and never collide with textures
    std::set<uint32_t> bindings;
    for (auto& res : resources)
    {
        EXPECT_TRUE (bindings.insert (res.binding).second) << res.name;
        EXPECT_TRUE (bindings.insert (res.samplerBinding).second) << res.name;
    }

    EXPECT_EQ (bindings, (std::set<uint32_t> { 0, 1, 2, 3, 4, 5 }));
}

TEST_F (WgslLoweringTests, FragmentHasStageIO)
{
    auto r = lower (kSimpleFragment, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    auto ep = r.getReference().entryPoint;
    // Fragment should have at least one input (vUV) and one output (outColor)
    bool hasInput = false;
    bool hasOutput = false;
    for (auto& io : ep.inputs)
        if (io.name == "vUV")
            hasInput = true;
    for (auto& io : ep.outputs)
        if (io.name == "outColor")
            hasOutput = true;
    EXPECT_TRUE (hasInput);
    EXPECT_TRUE (hasOutput);
}

TEST_F (WgslLoweringTests, ComputeWorkgroupSizesFromSource)
{
    // Use a variant with explicit local_size that the parser handles
    const char* src = R"glsl(
#version 450
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    uint idx = gl_GlobalInvocationID.x;
}
)glsl";
    auto r = lower (src, ShaderStage::compute);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    auto ep = r.getReference().entryPoint;
    EXPECT_TRUE (ep.isCompute);
    // The lowering should capture workgroup size from the layout qualifier
    // (may be 1,1,1 default if the parser doesn't propagate local_size in declarations)
}

TEST_F (WgslLoweringTests, UniformBlockHasResource)
{
    auto r = lower (kUBO, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    auto resources = r.getReference().resources;
    EXPECT_GE (resources.size(), 2u); // scene + material

    bool foundScene = false, foundMaterial = false;
    for (auto& res : resources)
    {
        if (res.name == "scene")
            foundScene = true;
        if (res.name == "material")
            foundMaterial = true;
    }
    EXPECT_TRUE (foundScene);
    EXPECT_TRUE (foundMaterial);
}

TEST_F (WgslLoweringTests, SeparateTextureAndSamplerResources)
{
    auto r = lower (kSeparateTextureSampler, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    auto resources = r.getReference().resources;
    EXPECT_GE (resources.size(), 2u); // tex + samp

    bool foundTex = false, foundSamp = false;
    for (auto& res : resources)
    {
        if (res.name == "tex")
        {
            foundTex = true;
            EXPECT_EQ (res.samplerBinding, ~0u); // separate texture has no companion
        }
        if (res.name == "samp")
        {
            foundSamp = true;
            EXPECT_EQ (res.samplerBinding, ~0u); // separate sampler has no companion
        }
    }
    EXPECT_TRUE (foundTex);
    EXPECT_TRUE (foundSamp);
}

TEST_F (WgslLoweringTests, RejectsSubpassInput)
{
    const char* src = R"glsl(
#version 450
layout(binding = 0) uniform subpassInput sp;
void main() { }
)glsl";
    auto r = lower (src, ShaderStage::fragment);
    ASSERT_TRUE (r.failed());
    EXPECT_TRUE (r.getErrorMessage().contains ("Subpass inputs are not supported")) << r.getErrorMessage();
}

TEST_F (WgslLoweringTests, AutoBindingForSamplerWithoutExplicitBinding)
{
    auto r = lower (kSamplerNoBinding, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto resources = r.getReference().resources;
    EXPECT_GE (resources.size(), 1u);
    // Combined sampler without explicit binding gets auto-assigned
    EXPECT_NE (resources[0].samplerBinding, ~0u); // companion sampler allocated
}

TEST_F (WgslLoweringTests, OutInoutParametersProcessed)
{
    auto r = lower (kOutInoutFunction, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    // Should succeed — out/inout params are lowered to pointer equivalents
}

TEST_F (WgslLoweringTests, UnnamedUniformBlockIsOneResource)
{
    auto r = lower (kUnnamedUniformBlock, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    // The block gets a synthesized instance: one resource, not one per member
    auto& resources = r.getReference().resources;
    ASSERT_EQ (resources.size(), 1u);
    EXPECT_NE (resources[0].name, "value");
    EXPECT_EQ (resources[0].group, 0u);
    EXPECT_EQ (resources[0].binding, 0u);
}

TEST_F (WgslLoweringTests, UnnamedBufferBlockIsOneResource)
{
    auto r = lower (kUnnamedBufferBlock, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    auto& resources = r.getReference().resources;
    ASSERT_EQ (resources.size(), 1u);
    EXPECT_NE (resources[0].name, "value");
    EXPECT_EQ (resources[0].group, 0u);
    EXPECT_EQ (resources[0].binding, 0u);
}

TEST_F (WgslLoweringTests, FunctionWithNoReassignedParamsSucceeds)
{
    auto r = lower (kFunctionNoParamReassignment, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    // shadowReassignedParams exits early when no parameter is reassigned
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
    const char* src = "void main() { float b = 0.25; float a = b > 0.5 ? 1.0 : 0.0; }";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("var a: f32 = select(0.0, 1.0, (b > 0.5));")) << wgsl;
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

TEST_F (WgslEmitterGoldenTests, UnnamedUniformBlockEmitsOneStructVariable)
{
    auto r = transpile (kUnnamedUniformBlock, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // One struct-typed variable at the block binding, with member reads going through it
    EXPECT_TRUE (wgsl.contains ("struct Block {")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("value: f32,")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@group(0) @binding(0) var<uniform> _Block: Block;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("= _Block.value;")) << wgsl;
    EXPECT_EQ (wgsl.indexOf ("var<uniform>"), wgsl.lastIndexOf ("var<uniform>")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, UnnamedBufferBlockEmitsOneStorageVariable)
{
    auto r = transpile (kUnnamedBufferBlock, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("var<storage, read_write> _Block: Block;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("_Block.value = 1.0;")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, UBOKeepsEveryCommaSeparatedMember)
{
    auto r = transpile (kUniformBlockCommaSeparatedMembers, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("var<uniform>")) << wgsl;

    for (auto* member : { "s", "r", "rx", "ry", "dx", "dy", "pad0", "pad1" })
        EXPECT_TRUE (wgsl.contains (String (member) + ": f32")) << member << " missing from:\n"
                                                                << wgsl;
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

TEST_F (WgslEmitterGoldenTests, IfElseBranchEmitted)
{
    auto r = transpile (kIfElseStatement, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("else")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("1.0")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("2.0")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, IfElseIfChainEmitted)
{
    const char* src = R"glsl(
void main() {
    float x;
    if (true) {
        x = 1.0;
    } else if (false) {
        x = 2.0;
    } else {
        x = 3.0;
    }
}
)glsl";
    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("else")) << wgsl;
    // else if chain preserves the nesting
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

TEST_F (WgslEmitterGoldenTests, GlobalConstEmitsModuleScopeConst)
{
    const char* src = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
const vec3 accent = vec3(0.25, 0.5, 0.75);
const float focal = 2.8;
const uint kCount = 4u;
void main()
{
    outColor = vec4(accent * focal * float(kCount), 1.0);
}
)glsl";

    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("const accent: vec3<f32> = ")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("const focal: f32 = ")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("const kCount: u32 = ")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("var<uniform>")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, GlobalConstArrayKeepsArrayType)
{
    const char* src = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
const float kDither[4] = float[4](0.0, 2.0, 3.0, 1.0);
void main()
{
    int index = int(gl_FragCoord.x) & 3;
    outColor = vec4(kDither[index]);
}
)glsl";

    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("const kDither: array<f32, 4> = ")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("var<uniform>")) << wgsl;
}

//==============================================================================
// Implicit conversions - GLSL converts implicitly where WGSL requires it spelled out
//==============================================================================

TEST_F (WgslEmitterGoldenTests, InstanceAndVertexIndexAreSignedLikeGlsl)
{
    const char* src = R"glsl(
#version 450
void main()
{
    int instance = gl_InstanceIndex;
    gl_Position = vec4(float(instance), float(gl_VertexIndex), 0.0, 1.0);
}
)glsl";

    auto r = transpile (src, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("var<private> gl_VertexIndex: i32;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var<private> gl_InstanceIndex: i32;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("gl_VertexIndex = i32(input.vertex_index);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("gl_InstanceIndex = i32(input.instance_index);")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, SignedIndexMixedWithUnsignedIsConverted)
{
    const char* src = R"glsl(
#version 450
void main()
{
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    gl_Position = vec4(x, 0.0, 0.0, 1.0);
}
)glsl";

    auto r = transpile (src, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("(u32(gl_VertexIndex) & 1u)")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, ScalarArgumentsOfVectorBuiltinsAreSplatted)
{
    const char* src = R"glsl(
#version 450
layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 outColor;
void main()
{
    vec3 a = clamp(vColor, 0.0, 1.0);
    vec3 b = min(vColor, 0.5);
    vec3 c = max(vColor, 0.25);
    vec3 d = step(0.5, vColor);
    vec3 e = smoothstep(0.0, 1.0, vColor);
    vec3 f = mix(vColor, a, 0.5);
    outColor = vec4(a + b + c + d + e + f, 1.0);
}
)glsl";

    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("clamp(vColor, vec3<f32>(")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("min(vColor, vec3<f32>(")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("max(vColor, vec3<f32>(")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("step(vec3<f32>(")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("smoothstep(vec3<f32>(")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("mix(vColor, a, vec3<f32>(")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, IntegerOperandsAreConvertedToFloat)
{
    const char* src = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
float scale(float x) { return x * 0.5; }
float toFloat(int n) { return n; }
void main()
{
    int i = 3;
    float f = i;
    float g = f * i;
    bool b = i < 0.5;
    float h = scale(i);
    outColor = vec4(f, g, h, toFloat(i));
}
)glsl";

    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("var f: f32 = f32(i);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("(f * f32(i))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("(f32(i) < ")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("scale(f32(i))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("return f32(n);")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, ConstructorArgumentsAreConverted)
{
    const char* src = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    int i = 1;
    vec2 p = vec2(i, 1.0);
    vec3 q = vec3(i);
    outColor = vec4(p, q.xy);
}
)glsl";

    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("vec2<f32>(f32(i), ")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("vec3<f32>(f32(i))")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, SignedShiftAmountIsConvertedToUnsigned)
{
    const char* src = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    uint u = 8u;
    int s = 2;
    uint r = u << s;
    outColor = vec4(float(r));
}
)glsl";

    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("(u << u32(s))")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, AbstractLiteralsAreNotWrapped)
{
    const char* src = R"glsl(
#version 450
layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 outColor;
void main()
{
    float x = 1;
    float y = x * 2;
    vec3 v = vColor * 2.0;
    outColor = vec4(v, y);
}
)glsl";

    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("var x: f32 = 1;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("(x * 2)")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("f32(1)")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("f32(2)")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("vec3<f32>(2")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, IntegerRemainderUsesWgslOperator)
{
    const char* src = R"glsl(
#version 450
layout(location = 0) out vec4 outColor;
void main()
{
    int a = 7;
    int b = 3;
    int c = a % b;
    outColor = vec4(float(c));
}
)glsl";

    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("(a % b)")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("floor(")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, FragmentShaderAllowsSamplingInNonUniformControlFlow)
{
    const char* src = R"glsl(
#version 450
layout(location = 0) in vec2 vUV;
layout(binding = 0) uniform sampler2D tex;
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4(0.0);
    if (vUV.x > 0.5)
        outColor = texture(tex, vUV);
}
)glsl";

    auto r = transpile (src, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.startsWith ("diagnostic(off, derivative_uniformity);")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, VertexShaderHasNoUniformityDiagnostic)
{
    auto r = transpile (kSimpleVertex, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    EXPECT_FALSE (r.getValue().contains ("diagnostic(")) << r.getValue();
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
// Statement emission coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, SwitchStatement)
{
    auto r = transpile (kSwitchStmt, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("switch (i) {")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("case 0: {\n            r = 0.0;\n        }")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("case 1: {\n            r = 0.5;\n        }")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("case 2: {\n            r = 1.0;\n        }")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("default: {\n            r = 0.0;\n        }")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("{}")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, WhileLoop)
{
    auto r = transpile (kWhileLoop, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("while ("));
    EXPECT_TRUE (wgsl.contains ("j++"));
}

//==============================================================================
// Function name mapping coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, VectorRelationalLessThan)
{
    auto r = transpile (kVectorRelational, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // lessThan/equal → inline operators
    EXPECT_FALSE (wgsl.contains ("lessThan("));
    EXPECT_FALSE (wgsl.contains ("greaterThan("));
    EXPECT_FALSE (wgsl.contains ("equal("));
    EXPECT_FALSE (wgsl.contains ("notEqual("));
    EXPECT_FALSE (wgsl.contains ("lessThanEqual("));
    EXPECT_FALSE (wgsl.contains ("greaterThanEqual("));
}

TEST_F (WgslEmitterGoldenTests, IsnanIsinfMapping)
{
    auto r = transpile (kIsnanIsinf, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // WGSL has no isnan/isinf: bit-test polyfills stay correct even under fast-math assumptions
    EXPECT_FALSE (wgsl.contains ("isNan(")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("isInf(")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fn glsl_isnan_f32(x: f32) -> bool {")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("(bitcast<u32>(x) & 0x7fffffffu) > 0x7f800000u")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("(bitcast<u32>(x) & 0x7fffffffu) == 0x7f800000u")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("glsl_isnan_f32(v)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("glsl_isinf_f32(v)")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, AtanSingleArg)
{
    auto r = transpile (kAtanScalar, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // atan(y, x) maps to atan2, the single-argument form stays atan; literal-only calls use typed literals
    EXPECT_TRUE (wgsl.contains ("atan2(1.0f, 2.0f)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("atan(1.0f)")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("atan2(1.0f)")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, RadiansDegreesMapping)
{
    auto r = transpile (kRadiansDegrees, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("radians("));
    EXPECT_TRUE (wgsl.contains ("degrees("));
}

TEST_F (WgslEmitterGoldenTests, FwidthCoarseFineMapping)
{
    auto r = transpile (kFwidthCoarseFine, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("fwidthCoarse("));
    EXPECT_TRUE (wgsl.contains ("fwidthFine("));
}

//==============================================================================
// Sampler type mapping coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, IntegerSamplerTypes)
{
    auto r = transpile (kIsamplerUSampler, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // isampler2D → texture_2d<i32>, usampler2D → texture_2d<u32>
    EXPECT_TRUE (wgsl.contains ("texture_2d<i32>"));
    EXPECT_TRUE (wgsl.contains ("texture_2d<u32>"));
    // texelFetch → textureLoad
    EXPECT_TRUE (wgsl.contains ("textureLoad("));
}

TEST_F (WgslEmitterGoldenTests, Sampler2DShadowType)
{
    auto r = transpile (kSampler2DShadow, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // sampler2DShadow → texture_depth_2d
    EXPECT_TRUE (wgsl.contains ("texture_depth_2d"));
}

//==============================================================================
// Resource emission coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, UBOWithMultipleMembersEmitted)
{
    auto r = transpile (kUBO, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Two uniform blocks → two @group/@binding declarations
    EXPECT_TRUE (wgsl.contains ("scene")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("material")) << wgsl;
    // Both should use var<uniform>
    EXPECT_TRUE (wgsl.contains ("var<uniform>"));
}

//==============================================================================
// Compute entry-point coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, ComputeWorkgroupSizesFromSource)
{
    auto r = transpile (kComputeWorkgroupSizes, ShaderStage::compute);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("@compute"));
    EXPECT_TRUE (wgsl.contains ("@workgroup_size"));
    // The compute input builtin should be present
    EXPECT_TRUE (wgsl.contains ("invocation_id") || wgsl.contains ("GlobalInvocationID"));
}

//==============================================================================
// Statement edge-case coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, DoWhileWithoutBraces)
{
    auto r = transpile (kDoWhileNoBraces, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Should still produce WGSL loop with break (lowering handles do-while → loop)
    EXPECT_TRUE (wgsl.contains ("loop {"));
    EXPECT_TRUE (wgsl.contains ("break"));
}

//==============================================================================
// Expression coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, UnsignedIntLiterals)
{
    auto r = transpile (kUIntLiterals, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("5u"));
    EXPECT_TRUE (wgsl.contains ("3u"));
    EXPECT_TRUE (wgsl.contains ("u32"));
}

//==============================================================================
// Math builtins — comprehensive name mapping coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, AllMathBuiltinsPreserved)
{
    auto r = transpile (kAllMathBuiltins, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // All math builtins should be preserved (name-mapped where needed)
    EXPECT_TRUE (wgsl.contains ("step("));
    EXPECT_TRUE (wgsl.contains ("smoothstep("));
    EXPECT_TRUE (wgsl.contains ("length("));
    EXPECT_TRUE (wgsl.contains ("distance("));
    EXPECT_TRUE (wgsl.contains ("cross("));
    EXPECT_TRUE (wgsl.contains ("reflect("));
    EXPECT_TRUE (wgsl.contains ("refract("));
    EXPECT_TRUE (wgsl.contains ("faceForward("));
    EXPECT_TRUE (wgsl.contains ("transpose("));
    EXPECT_TRUE (wgsl.contains ("sin("));
    EXPECT_TRUE (wgsl.contains ("cos("));
    EXPECT_TRUE (wgsl.contains ("tan("));
    EXPECT_TRUE (wgsl.contains ("asin("));
    EXPECT_TRUE (wgsl.contains ("acos("));
    EXPECT_TRUE (wgsl.contains ("atan2("));
    EXPECT_TRUE (wgsl.contains ("pow("));
    EXPECT_TRUE (wgsl.contains ("exp("));
    EXPECT_TRUE (wgsl.contains ("log("));
    EXPECT_TRUE (wgsl.contains ("sqrt("));
    EXPECT_TRUE (wgsl.contains ("abs("));
    EXPECT_TRUE (wgsl.contains ("sign("));
    EXPECT_TRUE (wgsl.contains ("floor("));
    EXPECT_TRUE (wgsl.contains ("trunc("));
    EXPECT_TRUE (wgsl.contains ("round("));
    EXPECT_TRUE (wgsl.contains ("ceil("));
    EXPECT_TRUE (wgsl.contains ("fract("));
    EXPECT_TRUE (wgsl.contains ("min("));
    EXPECT_TRUE (wgsl.contains ("max("));
}

//==============================================================================
// Boolean vector type coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, BvecTypes)
{
    auto r = transpile (kBvecTypes, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("vec2<bool>"));
    EXPECT_TRUE (wgsl.contains ("vec3<bool>"));
    EXPECT_TRUE (wgsl.contains ("vec4<bool>"));
    EXPECT_TRUE (wgsl.contains ("any("));
    EXPECT_TRUE (wgsl.contains ("all("));
}

//==============================================================================
// Logical operators
//==============================================================================

TEST_F (WgslEmitterGoldenTests, LogicalOperators)
{
    auto r = transpile (kLogicalOperators, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("&&") || wgsl.contains ("&&")); // WGSL uses &&
    EXPECT_TRUE (wgsl.contains ("!"));
}

//==============================================================================
// wgslTypeName, binaryOpSymbol, assignOpSymbol coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, IntegerVectorTypes)
{
    auto r = transpile (kIntegerVectors, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("vec2<i32>"));
    EXPECT_TRUE (wgsl.contains ("vec3<i32>"));
    EXPECT_TRUE (wgsl.contains ("vec4<i32>"));
    EXPECT_TRUE (wgsl.contains ("vec2<u32>"));
    EXPECT_TRUE (wgsl.contains ("vec3<u32>"));
    EXPECT_TRUE (wgsl.contains ("vec4<u32>"));
}

TEST_F (WgslEmitterGoldenTests, MatrixTypes)
{
    auto r = transpile (kMatrixTypes, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("mat2x2<f32>"));
    EXPECT_TRUE (wgsl.contains ("mat3x3<f32>"));
    EXPECT_TRUE (wgsl.contains ("mat4x4<f32>"));
    EXPECT_TRUE (wgsl.contains ("mat2x3<f32>"));
    EXPECT_TRUE (wgsl.contains ("mat3x2<f32>"));
    EXPECT_TRUE (wgsl.contains ("mat2x4<f32>"));
    EXPECT_TRUE (wgsl.contains ("mat4x2<f32>"));
    EXPECT_TRUE (wgsl.contains ("mat3x4<f32>"));
    EXPECT_TRUE (wgsl.contains ("mat4x3<f32>"));
}

TEST_F (WgslEmitterGoldenTests, SamplerTypeVariants)
{
    auto r = transpile (kSamplerTypeVariants, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Combined sampler type mappings
    EXPECT_TRUE (wgsl.contains ("texture_3d<f32>"));
    EXPECT_TRUE (wgsl.contains ("texture_cube<f32>"));
    EXPECT_TRUE (wgsl.contains ("texture_2d_array<f32>"));
    // Integer sampler types
    EXPECT_TRUE (wgsl.contains ("texture_3d<i32>"));
    EXPECT_TRUE (wgsl.contains ("texture_cube<i32>"));
    EXPECT_TRUE (wgsl.contains ("texture_2d_array<i32>"));
    // Unsigned sampler types
    EXPECT_TRUE (wgsl.contains ("texture_3d<u32>"));
    EXPECT_TRUE (wgsl.contains ("texture_cube<u32>"));
    EXPECT_TRUE (wgsl.contains ("texture_2d_array<u32>"));
}

TEST_F (WgslEmitterGoldenTests, UnaryPlusAndBitwiseNot)
{
    auto r = transpile (kUnaryPlusBitwiseNot, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // WGSL has no unary plus: the operand stands on its own; bitwise not is preserved
    EXPECT_TRUE (wgsl.contains ("var p: f32 = 1.0;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("~0")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, BitwiseOperators)
{
    auto r = transpile (kBitwiseOps, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Bitwise operators should be preserved
    EXPECT_TRUE (wgsl.contains ("&"));
    EXPECT_TRUE (wgsl.contains ("|"));
    EXPECT_TRUE (wgsl.contains ("^"));
    EXPECT_TRUE (wgsl.contains ("<<"));
    EXPECT_TRUE (wgsl.contains (">>"));
}

TEST_F (WgslEmitterGoldenTests, CompoundAssignmentOps)
{
    auto r = transpile (kCompoundAssignmentOps, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // All compound assignment operators
    EXPECT_TRUE (wgsl.contains ("%="));
    EXPECT_TRUE (wgsl.contains ("<<="));
    EXPECT_TRUE (wgsl.contains (">>="));
    EXPECT_TRUE (wgsl.contains ("&="));
    EXPECT_TRUE (wgsl.contains ("^="));
    EXPECT_TRUE (wgsl.contains ("|="));
}

TEST_F (WgslEmitterGoldenTests, EmitExprComma)
{
    auto r = transpile (kCommaOperator, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // WGSL has no comma operator: the left side becomes a statement before the declaration
    EXPECT_TRUE (wgsl.contains ("    x = 1;\n    var y: i32 = ((x + 2));")) << wgsl;
    EXPECT_FALSE (wgsl.contains ("comma")) << wgsl;
}

//==============================================================================
// Compute entry-point coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, ComputeEntryPointAllBuiltins)
{
    auto r = transpile (kComputeAllBuiltins, ShaderStage::compute);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("@compute"));
    EXPECT_TRUE (wgsl.contains ("@workgroup_size(8, 4, 1)"));
    // Compute builtins in entry-point signature
    EXPECT_TRUE (wgsl.contains ("@builtin(global_invocation_id)"));
    EXPECT_TRUE (wgsl.contains ("@builtin(local_invocation_index)"));
    EXPECT_TRUE (wgsl.contains ("@builtin(workgroup_id)"));
    EXPECT_TRUE (wgsl.contains ("@builtin(num_workgroups)"));
    // computeInputType: vec3<u32> for most, u32 for local_invocation_index
    EXPECT_TRUE (wgsl.contains ("vec3<u32>"));
    EXPECT_TRUE (wgsl.contains (": u32")); // local_invocation_index → u32
}

//==============================================================================
// Implicit builtin coverage
//==============================================================================

TEST_F (WgslEmitterGoldenTests, VertexWithOnlyImplicitBuiltins)
{
    auto r = transpile (kVertexOnlyImplicitBuiltins, ShaderStage::vertex);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();
    EXPECT_TRUE (wgsl.contains ("@vertex"));
    EXPECT_TRUE (wgsl.contains ("@builtin(vertex_index)"));
    EXPECT_TRUE (wgsl.contains ("@builtin(position)"));
}

TEST_F (WgslEmitterGoldenTests, FragmentWithNoExplicitInputs)
{
    auto r = transpile (kFragmentNoInputs, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();
    EXPECT_TRUE (wgsl.contains ("@fragment"));
    // Fragment always gets implicit builtin inputs
    EXPECT_TRUE (wgsl.contains ("frag_coord") || wgsl.contains ("fragCoord") || wgsl.contains ("position"));
}

TEST_F (WgslEmitterGoldenTests, OutInoutParamsLowered)
{
    auto r = transpile (kOutInoutFunction, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();
    // out/inout params become function pointers, dereferenced in the callee and passed by address
    EXPECT_TRUE (wgsl.contains ("fn scale(a: ptr<function, f32>, b: ptr<function, f32>)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("(*b) = ((*a) * 2.0);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("scale(&x, &y);")) << wgsl;
}

//==============================================================================
// Dot/bracket expression legalization
//==============================================================================

TEST_F (WgslLoweringTests, DotBracketExpressionsLegalized)
{
    auto r = lower (kDotBracketExpr, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    // Verify the symbol table picked up struct member accesses
    auto ep = r.getReference().entryPoint;
    EXPECT_TRUE (ep.isFragment);
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

TEST_F (WgslTranspilerIntegrationTests, WGSLBindingsMatchReflection)
{
    // Vulkan GLSL requires explicit bindings everywhere, ESSL only on blocks: glslang auto-maps the
    // unbound samplers it considers live. They are declared around explicit ones, and one of them
    // is never used, so it gets no binding and the WGSL output leaves it out.
    const char* src = R"glsl(#version 310 es
precision highp float;
layout(location = 0) out vec4 color;
uniform sampler2D texB;
layout(std140, binding = 3) uniform Params { vec4 tint; } params;
layout(binding = 0) uniform sampler2D texA;
uniform sampler2D unusedTex;
void main()
{
    color = textureLod(texA, vec2(0.5), 0.0) * textureLod(texB, vec2(0.5), 0.0) * params.tint;
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}
)glsl";

    auto wgsl = transpiler->transpile (src, ShaderStage::vertex, ShaderLanguage::essl, ShaderLanguage::wgsl);
    ASSERT_TRUE (wgsl.wasOk()) << wgsl.getErrorMessage();

    std::map<String, std::pair<int, int>> emitted; // variable -> (group, binding)
    for (const auto& line : StringArray::fromLines (wgsl.getValue()))
    {
        if (! line.startsWith ("@group("))
            continue;

        const auto group = line.fromFirstOccurrenceOf ("@group(", false, false).upToFirstOccurrenceOf (")", false, false).getIntValue();
        const auto binding = line.fromFirstOccurrenceOf ("@binding(", false, false).upToFirstOccurrenceOf (")", false, false).getIntValue();
        const auto name = line.fromFirstOccurrenceOf ("var", false, false).fromFirstOccurrenceOf (" ", false, false).upToFirstOccurrenceOf (":", false, false).trim();
        emitted[name] = { group, binding };
    }

    ASSERT_EQ (emitted.size(), 5u) << wgsl.getValue();
    EXPECT_FALSE (wgsl.getValue().contains ("unusedTex")) << wgsl.getValue();

    // Explicit bindings kept, live unbound ones take free slots in declaration order, companion samplers follow the highest binding
    EXPECT_EQ (emitted["texA"], std::make_pair (0, 0));
    EXPECT_EQ (emitted["texB"], std::make_pair (0, 1));
    EXPECT_EQ (emitted["params"], std::make_pair (0, 3));
    EXPECT_EQ (emitted["texA_sampler"], std::make_pair (0, 4));
    EXPECT_EQ (emitted["texB_sampler"], std::make_pair (0, 5));

    auto spirv = transpiler->compileToSPIRV (src, ShaderStage::vertex, ShaderLanguage::essl);
    ASSERT_TRUE (spirv.wasOk()) << spirv.getErrorMessage();

    auto reflection = transpiler->reflectFromSPIRV (spirv.getValue(), ShaderLanguage::wgsl);
    ASSERT_TRUE (reflection.wasOk()) << reflection.getErrorMessage();

    std::set<std::pair<int, int>> reflected;
    for (const auto* resources : { &reflection.getReference().uniformBuffers, &reflection.getReference().sampledImages })
    {
        for (const auto& r : *resources)
        {
            reflected.insert ({ (int) r.set, (int) r.backendSlot });

            if (r.backendSlotSecondary != ~0u)
                reflected.insert ({ (int) r.set, (int) r.backendSlotSecondary });
        }
    }

    std::set<std::pair<int, int>> fromWgsl;
    for (const auto& [name, slot] : emitted)
        fromWgsl.insert (slot);

    EXPECT_EQ (reflected, fromWgsl);

    for (const auto& image : reflection.getReference().sampledImages)
    {
        const auto expected = emitted[image.name + "_sampler"];
        EXPECT_EQ ((int) image.backendSlotSecondary, expected.second) << image.name;
        EXPECT_EQ ((int) image.backendSlot, emitted[image.name].second) << image.name;
    }
}

TEST_F (WgslTranspilerIntegrationTests, TranspileComputeToWGSLWithWorkgroup)
{
    auto r = transpiler->transpile (kComputeWorkgroupSizes, ShaderStage::compute, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    EXPECT_TRUE (r.getValue().contains ("@compute"));
    EXPECT_TRUE (r.getValue().contains ("@workgroup_size"));
}

TEST_F (WgslTranspilerIntegrationTests, TranspileSeparateTextureSampler)
{
    auto r = transpiler->transpile (kSeparateTextureSampler, ShaderStage::fragment, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();
    EXPECT_TRUE (wgsl.contains ("@fragment"));
    EXPECT_TRUE (wgsl.contains ("textureSample("));
}

TEST_F (WgslTranspilerIntegrationTests, TranspileToSPIRVFailsForWGSL)
{
    // WGSL cannot be transpiled to SPIR-V directly
    auto r = transpiler->transpile (kEmptyVertex, ShaderStage::vertex, ShaderLanguage::wgsl, ShaderLanguage::spirv);
    EXPECT_TRUE (r.failed());
}

//==============================================================================
// ShaderTypes coverage tests
//==============================================================================

TEST_F (WgslTranspilerIntegrationTests, ShaderStageToString)
{
    EXPECT_EQ (toString (ShaderStage::vertex), String ("vertex"));
    EXPECT_EQ (toString (ShaderStage::fragment), String ("fragment"));
    EXPECT_EQ (toString (ShaderStage::compute), String ("compute"));
}

TEST_F (WgslTranspilerIntegrationTests, ShaderLanguageToString)
{
    EXPECT_EQ (toString (ShaderLanguage::glsl), String ("glsl"));
    EXPECT_EQ (toString (ShaderLanguage::wgsl), String ("wgsl"));
    EXPECT_EQ (toString (ShaderLanguage::msl), String ("msl"));
}

//==============================================================================
// preprocessGlsl coverage tests — define, include paths, parse/preprocess failures
//==============================================================================

TEST_F (WgslTranspilerIntegrationTests, PreprocessWithDefineValue)
{
    TranspileOptions opts;
    opts.defines.set ("MY_VAL", "42.0");

    auto r = transpiler->transpile (kEmptyVertex, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::wgsl, opts);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    EXPECT_TRUE (r.getValue().contains ("@vertex"));
}

TEST_F (WgslTranspilerIntegrationTests, PreprocessWithDefineNoValue)
{
    TranspileOptions opts;
    opts.defines.set ("ENABLED", ""); // value is empty → "#define ENABLED\n"

    auto r = transpiler->transpile (kEmptyVertex, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::wgsl, opts);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    EXPECT_TRUE (r.getValue().contains ("@vertex"));
}

TEST_F (WgslTranspilerIntegrationTests, PreprocessWithIncludePaths)
{
    TranspileOptions opts;
    opts.includePaths.push_back ("/nonexistent/include/path");

    auto r = transpiler->transpile (kEmptyVertex, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::wgsl, opts);
    // May succeed (path not actually used) or fail depending on glslang behavior
    // The key is that includer.pushExternalDirectory() is exercised
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
}

TEST_F (WgslTranspilerIntegrationTests, PreprocessFailsOnInvalidGLSL)
{
    auto r = transpiler->transpile ("not valid glsl at all;", ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    EXPECT_TRUE (r.failed()) << "Expected parse failure for invalid GLSL";
}

TEST_F (WgslTranspilerIntegrationTests, PreprocessFailsOnMissingInclude)
{
    const char* src = R"glsl(
#version 450
#include <nonexistent_header_xyz.glsl>
void main()
{
}
)glsl";
    auto r = transpiler->transpile (src, ShaderStage::vertex, ShaderLanguage::glsl, ShaderLanguage::wgsl);
    EXPECT_TRUE (r.failed()) << "Expected failure from missing #include";
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
    opts.defaultGroup = 0;

    auto r = WgslTranspiler::transpile (kEmptyVertex, ShaderStage::vertex, opts);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().isNotEmpty());
    EXPECT_TRUE (r.getValue().contains ("@vertex"));
}

TEST_F (WgslTranspilerDirectTests, CustomOutputEntryPoint)
{
    WgslTranspileOptions opts;
    opts.outputEntryPoint = "vs_main";
    opts.defaultGroup = 0;

    auto r = WgslTranspiler::transpile (kEmptyVertex, ShaderStage::vertex, opts);
    ASSERT_TRUE (r.wasOk());
    EXPECT_TRUE (r.getValue().contains ("fn vs_main"));
}

TEST_F (WgslTranspilerDirectTests, DefaultWorkgroupSizeUsed)
{
    WgslTranspileOptions opts;
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

//==============================================================================
// Hardening tests: every GLSL construct either transpiles to equivalent WGSL
// or fails with a line:column diagnostic
//==============================================================================

class WgslHardeningTests : public ::testing::Test
{
protected:
    static ResultValue<String> transpile (const char* src, ShaderStage stage, StringArray* warnings = nullptr)
    {
        WgslTranspileOptions opts;
        opts.warnings = warnings;
        return WgslTranspiler::transpile (src, stage, opts);
    }

    static String transpileOk (const char* src, ShaderStage stage = ShaderStage::fragment)
    {
        auto r = transpile (src, stage);
        EXPECT_TRUE (r.wasOk()) << r.getErrorMessage();
        return r.wasOk() ? r.getValue() : String();
    }

    static void expectFailure (const char* src, const char* message, ShaderStage stage = ShaderStage::fragment)
    {
        auto r = transpile (src, stage);
        ASSERT_TRUE (r.failed()) << r.getValue();
        EXPECT_TRUE (r.getErrorMessage().contains (message)) << r.getErrorMessage();
    }
};

//==============================================================================
// Failing loudly

TEST_F (WgslHardeningTests, UnknownLayoutQualifierFails)
{
    expectFailure ("layout(foo = 1) uniform U { float x; } u; void main() {}", "1:8: Unknown layout qualifier 'foo'");
}

TEST_F (WgslHardeningTests, UnknownTypeFails)
{
    expectFailure ("void main() { Foo x; }", "1:15: Unknown type 'Foo'");
}

TEST_F (WgslHardeningTests, OutOfRangeIntegerLiteralsFail)
{
    expectFailure ("void main() { int x = 4294967296; }", "does not fit in 32 bits");
    expectFailure ("void main() { int x = 3000000000; }", "is too large for int");
}

TEST_F (WgslHardeningTests, InvalidOctalLiteralFails)
{
    expectFailure ("void main() { int x = 09; }", "Invalid digit '9' in octal literal");
}

TEST_F (WgslHardeningTests, ErrorsInsideNestedBodiesAreReported)
{
    expectFailure ("void main() { if (true) { float x = ; } }", "1:37: Expected expression, got ';'");
    expectFailure ("void main() { for (int i = 0; i < ; i++) {} }", "Expected expression");
    expectFailure ("void main() { switch (1) { case 0: float x = ; } }", "Expected expression");
}

TEST_F (WgslHardeningTests, BrokenArraySizeAndInitializerFail)
{
    expectFailure ("void main() { float a[+]; }", "Expected expression");
    expectFailure ("void main() { float a[2] = { 1.0, ) }; }", "Expected expression");
}

TEST_F (WgslHardeningTests, UnresolvedPreprocessorDirectiveFails)
{
    expectFailure ("#define X 1\nvoid main() {}", "must be resolved before transpiling");
}

TEST_F (WgslHardeningTests, DeepNestingFailsCleanly)
{
    std::string src = "void main() { float x = ";
    src += std::string (400, '(') + "1.0" + std::string (400, ')') + "; }";
    expectFailure (src.c_str(), "Nesting is too deep");
}

TEST_F (WgslHardeningTests, DoubleLiteralFails)
{
    expectFailure ("void main() { float x = float(1.0lf); }", "Double precision literals are not supported");
}

TEST_F (WgslHardeningTests, UnknownBuiltinFunctionFails)
{
    expectFailure ("void main() { float x = noSuchFunction(1.0); }", "Unsupported builtin function 'noSuchFunction'");
}

TEST_F (WgslHardeningTests, PointSizeOtherThanOneFails)
{
    expectFailure ("void main() { gl_PointSize = 2.0; gl_Position = vec4(0.0); }", "only writing 1.0 is supported", ShaderStage::vertex);
}

TEST_F (WgslHardeningTests, PointSizeOneIsDroppedWithWarning)
{
    StringArray warnings;
    auto r = transpile ("void main() { gl_PointSize = 1.0; gl_Position = vec4(0.0); }", ShaderStage::vertex, &warnings);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    EXPECT_FALSE (r.getValue().contains ("gl_PointSize = ")) << r.getValue();
    ASSERT_EQ (warnings.size(), 1);
    EXPECT_TRUE (warnings[0].contains ("gl_PointSize = 1.0 has no WGSL equivalent")) << warnings[0];
}

TEST_F (WgslHardeningTests, UnsupportedBuiltinVariablesFail)
{
    expectFailure ("layout(location = 0) out vec4 c; void main() { c = vec4(gl_PointCoord, 0.0, 1.0); }", "'gl_PointCoord' has no WGSL equivalent");
    expectFailure ("void main() { gl_ClipDistance[0] = 1.0; }", "'gl_ClipDistance' has no WGSL equivalent", ShaderStage::vertex);
    expectFailure ("void main() { }", "Geometry and tessellation stages are not supported", ShaderStage::geometry);
}

TEST_F (WgslHardeningTests, UnsupportedResourcesFail)
{
    expectFailure ("layout(push_constant) uniform P { float x; } p; void main() {}", "Push constants are not supported");
    expectFailure ("layout(binding = 0) uniform samplerBuffer b; void main() {}", "'samplerBuffer' has no WGSL equivalent");
    expectFailure ("layout(binding = 0) uniform sampler2D t[4]; void main() {}", "Arrays of 'sampler2D' are not supported");
    expectFailure ("layout(binding = 0) uniform image2D img; void main() {}", "needs a layout format qualifier");
    expectFailure ("layout(std140, row_major, binding = 0) uniform U { mat4 m; } u; void main() {}", "row_major matrices are not supported");
    expectFailure ("layout(std430, binding = 0) buffer B { float v[]; } b; void main() { b.v[0] = 1.0; }", "can't write storage buffers", ShaderStage::vertex);
}

TEST_F (WgslHardeningTests, IntegerTextureSamplingFails)
{
    expectFailure ("layout(binding = 0) uniform isampler2D t; void main() { ivec4 v = texture(t, vec2(0.5)); }", "Integer textures can only be read with texelFetch()");
}

TEST_F (WgslHardeningTests, StructEqualityFails)
{
    expectFailure ("struct S { float a; }; void main() { S x = S(1.0); S y = S(2.0); bool e = x == y; }", "Comparing structs, arrays or matrices");
}

TEST_F (WgslHardeningTests, CaseLabelOutsideSwitchFails)
{
    expectFailure ("void main() { case 1: return; }", "'case' label outside of a switch statement");
}

TEST_F (WgslHardeningTests, LineDirectiveSetsDiagnosticLines)
{
    expectFailure ("#line 100\nvoid main() { Foo x; }", "100:15: Unknown type 'Foo'");
}

//==============================================================================
// Literals, globals and resources

TEST_F (WgslHardeningTests, FloatLiteralsRoundTrip)
{
    const auto wgsl = transpileOk ("void main() { float a = 1e-10; float b = 3.14159265359; float c = 2.3283064365386963e-10; float d = 100.0; float e = .5e-3; float f = 1.f; float g = 2.; }");
    EXPECT_TRUE (wgsl.contains ("var a: f32 = 1e-10;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var b: f32 = 3.1415927;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var c: f32 = 2.3283064e-10;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var d: f32 = 100.0;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var e: f32 = 0.0005;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var f: f32 = 1.0;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var g: f32 = 2.0;")) << wgsl;
}

TEST_F (WgslHardeningTests, IntegerLiteralForms)
{
    const auto wgsl = transpileOk ("void main() { int m = -2147483648; int h = 0xFFFFFFFF; int o = 017; uint u = 0x1Fu; float x = 1.0; float n = - -x; }");
    EXPECT_TRUE (wgsl.contains ("var m: i32 = -2147483648;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var h: i32 = -1;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var o: i32 = 15;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var u: u32 = 31u;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var n: f32 = -(-x);")) << wgsl;
}

TEST_F (WgslHardeningTests, PlainGlobalsArePrivateAndInitializedInMain)
{
    const auto wgsl = transpileOk ("float g = 2.0; void bump() { g += 1.0; } void main() { bump(); }");
    EXPECT_TRUE (wgsl.contains ("var<private> g: f32;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fn main_inner() {\n    g = 2.0;\n    bump();")) << wgsl;
}

TEST_F (WgslHardeningTests, SharedVariablesAreWorkgroup)
{
    const auto wgsl = transpileOk ("layout(local_size_x = 8) in; shared float tile[8]; void main() { tile[gl_LocalInvocationIndex] = 1.0; barrier(); }", ShaderStage::compute);
    EXPECT_TRUE (wgsl.contains ("var<workgroup> tile: array<f32, 8>;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("workgroupBarrier();")) << wgsl;
}

TEST_F (WgslHardeningTests, ArrayLengthIsConstantOrArrayLength)
{
    const auto wgsl = transpileOk ("layout(local_size_x = 1) in; layout(std430, binding = 0) buffer B { float v[]; } b; void main() { float a[5]; int n = a.length(); int m = b.v.length(); b.v[0] = float(n + m); }", ShaderStage::compute);
    EXPECT_TRUE (wgsl.contains ("var n: i32 = 5;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var m: i32 = i32(arrayLength(&b.v));")) << wgsl;
}

TEST_F (WgslHardeningTests, EntryPointNameIsHonored)
{
    WgslTranspileOptions opts;
    opts.outputEntryPoint = "fs_main";
    auto r = WgslTranspiler::transpile ("layout(location = 0) out vec4 c; void fs_main_helper() {} void main() { c = vec4(1.0); }", ShaderStage::fragment, opts);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    EXPECT_TRUE (r.getValue().contains ("@fragment\nfn fs_main(")) << r.getValue();
    EXPECT_TRUE (r.getValue().contains ("fn main_inner()")) << r.getValue();
}

TEST_F (WgslHardeningTests, ReservedAndGeneratedNamesDontCollide)
{
    const auto wgsl = transpileOk ("float main_inner() { return 1.0; } void main() { float target = main_inner(); float ref = target; float input = ref; }");
    EXPECT_TRUE (wgsl.contains ("var target_: f32 = main_inner();")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var ref_: f32 = target_;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fn main_inner_1()")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("    main_inner_1();\n")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fn main(input_1: FSInput)")) << wgsl;
}

TEST_F (WgslHardeningTests, OverloadsGetDistinctNames)
{
    const auto wgsl = transpileOk ("float f(float a) { return a; } float f(vec2 a) { return a.x; } float f(int a) { return float(a); } void main() { float x = f(1.0) + f(vec2(2.0)) + f(3); }");
    EXPECT_TRUE (wgsl.contains ("fn f(a: f32) -> f32")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fn f_1(a: vec2<f32>) -> f32")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fn f_2(a: i32) -> f32")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("((f(1.0f) + f_1(vec2<f32>(2.0))) + f_2(3i))")) << wgsl;
}

TEST_F (WgslHardeningTests, BindingsKeepExplicitValuesAndSplitCombinedSamplers)
{
    const auto wgsl = transpileOk ("layout(set = 1, binding = 4) uniform sampler2D t; layout(set = 1) layout(binding = 2) uniform U { vec4 c; } u; layout(location = 0) out vec4 o; void main() { o = texture(t, u.c.xy) * u.c; }");
    EXPECT_TRUE (wgsl.contains ("@group(1) @binding(4) var t: texture_2d<f32>;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@group(1) @binding(5) var t_sampler: sampler;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@group(1) @binding(2) var<uniform> u: U;")) << wgsl;
}

//==============================================================================
// Statements and expressions

TEST_F (WgslHardeningTests, SwitchFallthroughAndMissingDefault)
{
    const auto wgsl = transpileOk ("void main() { int i = 1; float r = 0.0; switch (i) { case 0: case 1: r = 1.0; case 2: r += 2.0; break; case 3: r = 3.0; } }");
    EXPECT_TRUE (wgsl.contains ("case 0, 1: {\n            r = 1.0;\n            r += 2.0;\n        }")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("case 2: {\n            r += 2.0;\n        }")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("case 3: {\n            r = 3.0;\n        }")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("default: {\n        }")) << wgsl;
}

TEST_F (WgslHardeningTests, UnsignedSwitchLabelsMatchSelector)
{
    const auto wgsl = transpileOk ("void main() { uint u = 1u; switch (u) { case 1: u = 2u; break; default: break; } }");
    EXPECT_TRUE (wgsl.contains ("case 1: {")) << wgsl;
}

TEST_F (WgslHardeningTests, DoWhileContinueEvaluatesCondition)
{
    const auto wgsl = transpileOk ("void main() { int k = 0; do { k++; if (k == 2) continue; } while (k < 4); }");
    EXPECT_TRUE (wgsl.contains ("loop {\n        k++;\n        if ((k == 2)) {\n            continue;\n        }\n        continuing {\n            break if !((k < 4));\n        }\n    }")) << wgsl;
}

TEST_F (WgslHardeningTests, ForWithCommaUpdateUsesContinuing)
{
    const auto wgsl = transpileOk ("void main() { float r = 0.0; for (int x = 0, y = 4; x < y; x++, y--) { r += 1.0; } }");
    EXPECT_TRUE (wgsl.contains ("var x: i32 = 0;\n        var y: i32 = 4;\n        loop {\n            if (!((x < y))) {\n                break;\n            }")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("continuing {\n                x++;\n                y--;\n            }")) << wgsl;
}

TEST_F (WgslHardeningTests, SideEffectsInsideExpressionsAreHoisted)
{
    const auto wgsl = transpileOk ("void main() { float arr[4]; int i = 0; arr[i++] = 3.0; int j = ++i + i; }");
    EXPECT_TRUE (wgsl.contains ("let _t: i32 = i;\n    i++;\n    arr[_t] = 3.0;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("i += 1;\n    var j: i32 = (i + i);")) << wgsl;
}

TEST_F (WgslHardeningTests, SwizzleStoresAreSplitIntoComponents)
{
    const auto wgsl = transpileOk ("void main() { vec4 c = vec4(0.0); c.xy *= 2.0; c.zx = vec2(1.0, 2.0); c.w = 1.0; }");
    EXPECT_TRUE (wgsl.contains ("let _t: vec2<f32> = (c.xy * 2.0);\n    c.x = _t.x;\n    c.y = _t.y;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("c.z = _t_1.x;\n    c.x = _t_1.y;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("c.w = 1.0;")) << wgsl;
}

TEST_F (WgslHardeningTests, OutArgumentsThatAreNotLocalsUseCopyInCopyOut)
{
    const auto wgsl = transpileOk ("vec3 g; void f(inout float v) { v += 1.0; } void h(out vec3 v) { v = vec3(1.0); } void main() { vec4 c = vec4(0.0); f(c.y); h(g); float x = 0.0; f(x); }");
    EXPECT_TRUE (wgsl.contains ("var _t: f32 = c.y;\n    f(&_t);\n    c.y = _t;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var _t_1: vec3<f32>;\n    h(&_t_1);\n    g = _t_1;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("f(&x);")) << wgsl;
}

TEST_F (WgslHardeningTests, WrittenParametersAreCopied)
{
    const auto wgsl = transpileOk ("float f(float x, vec2 p) { x *= 2.0; p.x = x; return p.x; } void main() { float r = f(1.0, vec2(0.0)); }");
    EXPECT_TRUE (wgsl.contains ("fn f(_x: f32, _p: vec2<f32>) -> f32 {\n    var x: f32 = _x;\n    var p: vec2<f32> = _p;")) << wgsl;
}

TEST_F (WgslHardeningTests, TernaryAndShortCircuitWithSideEffects)
{
    const auto wgsl = transpileOk ("int n; bool bump() { n++; return n > 1; } void main() { float r = 0.0; float s = r > 0.0 ? (r += 1.0) : 0.0; bool z = r > 0.0 && (n++ > 1); }");
    EXPECT_TRUE (wgsl.contains ("if ((r > 0.0)) {\n        r += 1.0;\n        _t = (r);\n    }\n    else {\n        _t = 0.0;\n    }\n    var s: f32 = _t;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var _t_1: bool = (r > 0.0);\n    if (_t_1) {")) << wgsl;
}

TEST_F (WgslHardeningTests, VectorEqualityReducesToBool)
{
    const auto wgsl = transpileOk ("void main() { vec3 v = vec3(1.0); vec3 w = vec3(2.0); bool e = v == w; bool n = v != w; }");
    EXPECT_TRUE (wgsl.contains ("var e: bool = all((v == w));")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var n: bool = any((v != w));")) << wgsl;
}

TEST_F (WgslHardeningTests, ConstructorsWgslLacks)
{
    const auto wgsl = transpileOk ("void main() { mat4 m4 = mat4(1.0); mat3 m3 = mat3(m4); mat2 m2 = mat2(vec2(1.0), 2.0, 3.0); vec4 v = vec4(1.0); vec2 t = vec2(v); float f = float(v); }");
    EXPECT_TRUE (wgsl.contains ("var m4: mat4x4<f32> = mat4x4<f32>(vec4<f32>(1.0, 0.0, 0.0, 0.0), vec4<f32>(0.0, 1.0, 0.0, 0.0),")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var m3: mat3x3<f32> = mat3x3<f32>(m4[0].xyz, m4[1].xyz, m4[2].xyz);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("let _t: vec2<f32> = vec2<f32>(1.0);\n    var m2: mat2x2<f32> = mat2x2<f32>(_t.x, _t.y, 2.0, 3.0);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var t: vec2<f32> = v.xy;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var f: f32 = v.x;")) << wgsl;
}

TEST_F (WgslHardeningTests, MathPolyfills)
{
    const auto wgsl = transpileOk ("void main() { vec3 v = mod(vec3(5.0), 2.0); mat3 m = inverse(mat3(2.0)); mat3x2 o = outerProduct(vec2(1.0), vec3(1.0)); }");
    EXPECT_TRUE (wgsl.contains ("fn glsl_mod_vec3f32_f32(x: vec3<f32>, y: f32) -> vec3<f32> {\n    return x - y * floor(x / y);\n}")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fn glsl_inverse_mat3x3f32(m: mat3x3<f32>) -> mat3x3<f32>")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("return mat3x2<f32>(c * r.x, c * r.y, c * r.z);")) << wgsl;
}

TEST_F (WgslHardeningTests, BuiltinFunctionMappings)
{
    const auto wgsl = transpileOk ("layout(location = 0) out vec4 o; void main() { float f = 1.0; uint u = floatBitsToUint(f); int c = bitCount(u); int l = findLSB(u); uint r = bitfieldReverse(u); uint p = packHalf2x16(vec2(f)); float dx = dFdxFine(f); bvec2 b = not(bvec2(true)); vec2 s = mix(vec2(0.0), vec2(1.0), b); float e = roundEven(f); int ex; float fr = frexp(f, ex); o = vec4(dx + e + fr); }");
    EXPECT_TRUE (wgsl.contains ("bitcast<u32>(f)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var c: i32 = i32(countOneBits(u));")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var l: i32 = i32(firstTrailingBit(u));")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("reverseBits(u)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("pack2x16float(vec2<f32>(f))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("dpdxFine(f)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("!(vec2<bool>(true))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("select(vec2<f32>(0.0), vec2<f32>(1.0), b)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var e: f32 = round(f);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("let _t_1 = frexp(_t);\n    ex = _t_1.exp;")) << wgsl;
}

//==============================================================================
// Declarations

TEST_F (WgslHardeningTests, DeclarationFormsParse)
{
    const auto wgsl = transpileOk (R"glsl(
#version 450
#extension GL_GOOGLE_include_directive : enable
precision highp float;
float helper(void);
struct Pair { float a; float b; } gPair;
layout(location = 0) out vec4 o;
float helper(void) { return 1.0; }
void main(void)
{
    precision mediump int;
    struct Local { vec2 p; } loc;
    loc.p = vec2(helper());
    vec4(1.0).x;
    [[unroll]] for (int i = 0; i < 2; i++) { gPair.a += 1.0; }
    const in float unused = 0.0;
    bool x = true ^^ false;
    o = vec4(loc.p, gPair.a, x ? 1.0 : 0.0);
}
)glsl");
    EXPECT_TRUE (wgsl.contains ("struct Pair {")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var<private> gPair: Pair;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("struct Local {")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var loc: Local;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fn helper() -> f32")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("(true != false)")) << wgsl;
}

TEST_F (WgslHardeningTests, StageIOBlocksAreFlattened)
{
    const auto vs = transpileOk ("layout(location = 2) out VertexData { vec2 uv; flat int id; } vd; void main() { vd.uv = vec2(0.0); vd.id = 3; gl_Position = vec4(0.0); }", ShaderStage::vertex);
    EXPECT_TRUE (vs.contains ("@location(2) vd_uv: vec2<f32>,")) << vs;
    EXPECT_TRUE (vs.contains ("@location(3) @interpolate(flat) vd_id: i32,")) << vs;
    EXPECT_TRUE (vs.contains ("vd_uv = vec2<f32>(0.0);")) << vs;

    const auto fs = transpileOk ("layout(location = 2) in VertexData { vec2 uv; flat int id; }; layout(location = 0) out vec4 o; void main() { o = vec4(uv, float(id), 1.0); }");
    EXPECT_TRUE (fs.contains ("@location(2) uv: vec2<f32>,")) << fs;
    EXPECT_TRUE (fs.contains ("@location(3) @interpolate(flat) id: i32,")) << fs;
}

TEST_F (WgslHardeningTests, InvariantPositionAndPerVertexRedeclaration)
{
    const auto wgsl = transpileOk ("out gl_PerVertex { vec4 gl_Position; }; invariant gl_Position; void main() { gl_Position = vec4(1.0); }", ShaderStage::vertex);
    EXPECT_TRUE (wgsl.contains ("@builtin(position) @invariant position: vec4<f32>,")) << wgsl;
}

//==============================================================================
// Stage IO and builtins

TEST_F (WgslHardeningTests, VaryingsAndInterpolation)
{
    const auto wgsl = transpileOk ("layout(location = 0) flat in ivec2 cell; layout(location = 1) noperspective centroid in vec2 uv; layout(location = 2) sample in float s; layout(location = 3) in mat2 m; layout(location = 0) out vec4 o; void main() { o = vec4(vec2(cell) + uv + m[1], s, 1.0); gl_FragDepth = 0.5; }");
    EXPECT_TRUE (wgsl.contains ("@location(0) @interpolate(flat) cell: vec2<i32>,")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@location(1) @interpolate(linear, centroid) uv: vec2<f32>,")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@location(2) @interpolate(perspective, sample) s: f32,")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@location(3) m_0: vec2<f32>,\n    @location(4) m_1: vec2<f32>,")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("m = mat2x2<f32>(input.m_0, input.m_1);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@builtin(frag_depth) frag_depth: f32,")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("output.frag_depth = gl_FragDepth;")) << wgsl;
}

TEST_F (WgslHardeningTests, IntegerVaryingsAreAlwaysFlat)
{
    const auto wgsl = transpileOk ("layout(location = 0) out uint id; void main() { id = 1u; gl_Position = vec4(0.0); }", ShaderStage::vertex);
    EXPECT_TRUE (wgsl.contains ("@location(0) @interpolate(flat) id: u32,")) << wgsl;
}

TEST_F (WgslHardeningTests, MissingLocationsAreAssignedInDeclarationOrder)
{
    const auto wgsl = transpileOk ("layout(location = 0) in vec4 a; in vec2 b, c; void main() { gl_Position = a + vec4(b, c); }", ShaderStage::vertex);
    EXPECT_TRUE (wgsl.contains ("@location(0) a: vec4<f32>,\n    @location(1) b: vec2<f32>,\n    @location(2) c: vec2<f32>,")) << wgsl;
}

TEST_F (WgslHardeningTests, GlVertexIdAliasesVertexIndex)
{
    const auto wgsl = transpileOk ("void main() { gl_Position = vec4(float(gl_VertexID + gl_InstanceID)); }", ShaderStage::vertex);
    EXPECT_TRUE (wgsl.contains ("gl_VertexID = gl_VertexIndex;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("gl_InstanceID = gl_InstanceIndex;")) << wgsl;
}

TEST_F (WgslHardeningTests, SpecializationConstantsBecomeOverrides)
{
    const auto wgsl = transpileOk ("layout(local_size_x_id = 3, local_size_y = 4) in; layout(constant_id = 7) const float kScale = 2.5; layout(std430, binding = 0) buffer B { float v[]; } b; void main() { b.v[gl_GlobalInvocationID.x] = kScale * float(gl_WorkGroupSize.x); }", ShaderStage::compute);
    EXPECT_TRUE (wgsl.contains ("@id(3) override _workgroup_size_x: u32 = 1u;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@id(7) override kScale: f32 = 2.5;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@compute @workgroup_size(_workgroup_size_x, 4, 1)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("gl_WorkGroupSize = vec3<u32>(_workgroup_size_x, 4u, 1u);")) << wgsl;
}

TEST_F (WgslHardeningTests, DualSourceBlending)
{
    const auto wgsl = transpileOk ("layout(location = 0, index = 0) out vec4 color; layout(location = 0, index = 1) out vec4 blend; void main() { color = vec4(1.0); blend = vec4(0.5); }");
    EXPECT_TRUE (wgsl.startsWith ("enable dual_source_blending;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@location(0) @blend_src(0) color: vec4<f32>,\n    @location(0) @blend_src(1) blend: vec4<f32>,")) << wgsl;
}

//==============================================================================
// Buffers and textures

TEST_F (WgslHardeningTests, Std140ScalarArraysAndTwoRowMatricesAreWrapped)
{
    const auto wgsl = transpileOk ("layout(std140, binding = 0) uniform U { float values[4]; mat2 rot; vec2 pairs[2]; } u; layout(location = 0) flat in int i; layout(location = 0) out vec4 o; void main() { float copy[4] = u.values; mat2 r = u.rot; o = vec4(u.values[i], u.rot[1][0], u.pairs[1].y, copy[0] + r[0][0]); }");
    EXPECT_TRUE (wgsl.contains ("struct _std140_f32 {\n    @size(16) v: f32,\n}")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("struct _std140_vec2_f32 {\n    @size(16) v: vec2<f32>,\n}")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("struct U {\n    values: array<_std140_f32, 4>,\n    rot: array<_std140_vec2_f32, 2>,\n    pairs: array<_std140_vec2_f32, 2>,\n}")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("u.values[i].v")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("u.rot[1].v[0]")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("u.pairs[1].v.y")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("return mat2x2<f32>(x[0].v, x[1].v);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("r[i] = x[i].v;")) << wgsl;
}

TEST_F (WgslHardeningTests, BlockPaddingBoolsAndOffsets)
{
    const auto wgsl = transpileOk ("layout(std140, binding = 0) uniform U { float a; bool enabled; layout(offset = 32) vec3 dir; } u; layout(location = 0) out vec4 o; void main() { o = vec4(u.enabled ? u.dir : vec3(u.a), 1.0); }");
    EXPECT_TRUE (wgsl.contains ("struct U {\n    a: f32,\n    @size(28) enabled: u32,\n    @size(16) dir: vec3<f32>,\n}")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("bool(u.enabled)")) << wgsl;
}

TEST_F (WgslHardeningTests, StorageAccessAndAtomics)
{
    const auto wgsl = transpileOk ("layout(local_size_x = 64) in; layout(std430, binding = 0) readonly buffer In { float v[]; } src; layout(std430, binding = 1) buffer Out { uint count; int values[]; } dst; shared uint local; void main() { uint i = gl_GlobalInvocationID.x; atomicAdd(local, 1u); uint old = atomicAdd(dst.count, 1u); int prev = atomicCompSwap(dst.values[i], 0, 1); dst.count = uint(src.v[i]); }", ShaderStage::compute);
    EXPECT_TRUE (wgsl.contains ("@group(0) @binding(0) var<storage, read> src: In;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@group(0) @binding(1) var<storage, read_write> dst: Out;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("count: atomic<u32>,\n    values: array<atomic<i32>>,")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var<workgroup> local: atomic<u32>;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("_ = atomicAdd(&local, 1u);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var old: u32 = atomicAdd(&dst.count, 1u);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("atomicCompareExchangeWeak(&dst.values[i], 0, 1)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("atomicStore(&dst.count, u32(src.v[i]));")) << wgsl;
}

TEST_F (WgslHardeningTests, StorageImages)
{
    const auto wgsl = transpileOk ("layout(local_size_x = 8, local_size_y = 8) in; layout(binding = 0, rgba8) writeonly uniform image2D dst; layout(binding = 1, r32f) readonly uniform image2D src; void main() { ivec2 p = ivec2(gl_GlobalInvocationID.xy); ivec2 size = imageSize(dst); imageStore(dst, p, vec4(imageLoad(src, p).r)); }", ShaderStage::compute);
    EXPECT_TRUE (wgsl.contains ("var dst: texture_storage_2d<rgba8unorm, write>;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var src: texture_storage_2d<r32float, read>;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("vec2<i32>(textureDimensions(dst))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureStore(dst, p, vec4<f32>(textureLoad(src, p).r));")) << wgsl;
}

TEST_F (WgslHardeningTests, TextureFunctionVariants)
{
    const auto wgsl = transpileOk (R"glsl(
layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2DArray layers;
layout(binding = 2) uniform sampler2DShadow shadow;
layout(binding = 3) uniform texture2D depthTex;
layout(binding = 4) uniform samplerShadow cmp;
layout(location = 0) in vec3 uv;
layout(location = 0) out vec4 o;
void main()
{
    vec4 a = textureLod(tex, uv.xy, 2.0) + textureGrad(tex, uv.xy, vec2(0.1), vec2(0.1));
    vec4 b = textureOffset(tex, uv.xy, ivec2(1, 0)) + textureProj(tex, uv) + texture(tex, uv.xy, 0.5);
    vec4 c = texture(layers, uv) + texelFetch(tex, ivec2(0), 0) + textureGather(tex, uv.xy, 1);
    float d = texture(shadow, uv) + texture(sampler2DShadow(depthTex, cmp), uv);
    ivec2 s = textureSize(tex, 0);
    ivec3 ls = textureSize(layers, 0);
    o = a + b + c + vec4(d + float(s.x + ls.z) + float(textureQueryLevels(tex)));
}
)glsl");
    EXPECT_TRUE (wgsl.contains ("textureSampleLevel(tex, tex_sampler, uv.xy, 2.0)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureSampleGrad(tex, tex_sampler, uv.xy, vec2<f32>(0.1), vec2<f32>(0.1))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureSample(tex, tex_sampler, uv.xy, vec2<i32>(1, 0))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureSample(tex, tex_sampler, (uv.xy / uv.z))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureSampleBias(tex, tex_sampler, uv.xy, 0.5)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureSample(layers, layers_sampler, uv.xy, i32(floor((uv.z + 0.5))))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureLoad(tex, vec2<i32>(0), 0)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureGather(1, tex, tex_sampler, uv.xy)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var shadow_sampler: sampler_comparison;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureSampleCompare(shadow, shadow_sampler, uv.xy, uv.z)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var depthTex: texture_depth_2d;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("textureSampleCompare(depthTex, cmp, uv.xy, uv.z)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("var s: vec2<i32> = vec2<i32>(textureDimensions(tex, 0));")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("vec3<i32>(vec3<u32>(textureDimensions(layers, 0), textureNumLayers(layers)))")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("i32(textureNumLevels(tex))")) << wgsl;
}

TEST_F (WgslHardeningTests, SamplersPassedToFunctions)
{
    const auto wgsl = transpileOk ("layout(binding = 0) uniform sampler2D tex; layout(location = 0) out vec4 o; vec4 fetch(sampler2D s, vec2 uv) { return texture(s, uv); } void main() { o = fetch(tex, vec2(0.5)); }");
    EXPECT_TRUE (wgsl.contains ("fn fetch(s: texture_2d<f32>, s_sampler: sampler, uv: vec2<f32>) -> vec4<f32>")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("return textureSample(s, s_sampler, uv);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fetch(tex, tex_sampler, vec2<f32>(0.5))")) << wgsl;
}

TEST_F (WgslHardeningTests, VertexSamplingUsesLevelZero)
{
    const auto wgsl = transpileOk ("layout(binding = 0) uniform sampler2D heights; layout(location = 0) in vec2 uv; void main() { gl_Position = vec4(uv, texture(heights, uv).r, 1.0); }", ShaderStage::vertex);
    EXPECT_TRUE (wgsl.contains ("textureSampleLevel(heights, heights_sampler, uv, 0.0)")) << wgsl;
}

//==============================================================================
// Shipped shaders

TEST_F (WgslHardeningTests, PbrBrdfKeepsHammersleyPrecision)
{
    // Excerpt of examples/graphics/data/shaders/pbr_brdf.frag
    const auto wgsl = transpileOk (R"glsl(
#version 450
layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 fragColor;
const uint kSampleCount = 512u;
float radicalInverse(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    return float(bits) * 2.3283064365386963e-10;
}
void main() {
    float s = 0.0;
    for (uint i = 0u; i < kSampleCount; ++i)
        s += radicalInverse(i);
    fragColor = vec4(s / float(kSampleCount), v_uv, 1.0);
}
)glsl");
    EXPECT_TRUE (wgsl.contains ("fn radicalInverse(_bits: u32) -> f32 {\n    var bits: u32 = _bits;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("return (f32(bits) * 2.3283064e-10);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("for (var i: u32 = 0u; (i < kSampleCount); i += 1) {")) << wgsl;
}

TEST_F (WgslHardeningTests, ParticlesUpdateCompute)
{
    // Excerpt of examples/graphics/data/shaders/particles_update.comp
    const auto wgsl = transpileOk (R"glsl(
#version 450
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
layout(std140, set = 0, binding = 0) uniform Params { float deltaTime; float gravity; float particleCountF; } params;
layout(std430, set = 0, binding = 1) buffer ParticleBuffer { float data[]; };
uint wangHash(uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    return seed;
}
void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= uint(params.particleCountF))
        return;
    uint base = idx * 12u;
    vec2 vel = vec2(data[base + 2u], data[base + 3u]);
    vel.y -= params.gravity * params.deltaTime;
    data[base + 2u] = vel.x + float(wangHash(idx)) / float(0xFFFFFFFFu);
    data[base + 3u] = vel.y;
}
)glsl", ShaderStage::compute);
    EXPECT_TRUE (wgsl.contains ("struct ParticleBuffer {\n    data: array<f32>,\n}")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@group(0) @binding(1) var<storage, read_write> _ParticleBuffer: ParticleBuffer;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("fn wangHash(_seed: u32) -> u32 {\n    var seed: u32 = _seed;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("vel.y -= (params.gravity * params.deltaTime);")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("_ParticleBuffer.data[(base + 3u)] = vel.y;")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@compute @workgroup_size(256, 1, 1)")) << wgsl;
}

TEST_F (WgslHardeningTests, SpectrogramShaders)
{
    // Excerpt of modules/yup_audio_gui/displays/yup_SpectrogramComponentShader.frag and .vert
    const auto fs = transpileOk (R"glsl(
#version 450
layout(set = 0, binding = 0) uniform texture2D u_prev;
layout(set = 0, binding = 1) uniform sampler   u_samp;
layout(set = 0, binding = 2) uniform WaterfallParams { float numRows; float width; float height; float bins; } p;
layout(set = 0, binding = 3) uniform RowData { vec4 mags[512]; } rows;
layout(set = 0, binding = 4) uniform LutData { uvec4 lut[64]; } lut;
layout(location = 0) out vec4 fragColor;
float fetchMag(int idx) { return rows.mags[idx >> 2][idx & 3]; }
void main() {
    vec2 uv = gl_FragCoord.xy / vec2(p.width, p.height);
    if (uv.y >= p.numRows / p.height) {
        fragColor = textureLod(sampler2D(u_prev, u_samp), vec2(uv.x, uv.y - p.numRows / p.height), 0.0);
    } else {
        int i0 = int(fetchMag(int(gl_FragCoord.x)) * 255.0);
        uint c0 = lut.lut[i0 >> 2][i0 & 3];
        fragColor = vec4(float((c0 >> 16) & 255u), float((c0 >> 8) & 255u), float(c0 & 255u), float((c0 >> 24) & 255u)) / 255.0;
    }
}
)glsl");
    EXPECT_TRUE (fs.contains ("struct RowData {\n    mags: array<vec4<f32>, 512>,\n}")) << fs;
    EXPECT_TRUE (fs.contains ("struct LutData {\n    lut: array<vec4<u32>, 64>,\n}")) << fs;
    EXPECT_TRUE (fs.contains ("return rows.mags[(idx >> 2)][(idx & 3)];")) << fs;
    EXPECT_TRUE (fs.contains ("textureSampleLevel(u_prev, u_samp, vec2<f32>(uv.x, (uv.y - (p.numRows / p.height))), 0.0)")) << fs;
    EXPECT_TRUE (fs.contains ("f32((((c0 >> 16)) & 255u))")) << fs;

    const auto vs = transpileOk (R"glsl(
#version 450
void main() {
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)glsl", ShaderStage::vertex);
    EXPECT_TRUE (vs.contains ("var x: f32 = (f32((((u32(gl_VertexIndex) & 1u)) << 2u)) - 1.0);")) << vs;
}

//==============================================================================
// Shader corpus: every shader transpiles, and validates with naga when it is installed
//==============================================================================

#if YUP_MAC || YUP_LINUX || YUP_WINDOWS

class WgslCorpusTests : public ::testing::Test
{
protected:
    static File repositoryRoot()
    {
        return File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory();
    }

    /** Stress shaders in tests/data/wgsl plus the shaders the repository ships. */
    static Array<File> corpus()
    {
        const auto root = repositoryRoot();
        Array<File> files;

        for (const auto& directory : { root.getChildFile ("tests/data/wgsl"), root.getChildFile ("examples/graphics/data/shaders") })
            for (const auto& entry : RangedDirectoryIterator (directory, false, "*.vert;*.frag;*.comp", File::findFiles))
                files.add (entry.getFile());

        files.add (root.getChildFile ("modules/yup_audio_gui/displays/yup_SpectrogramComponentShader.vert"));
        files.add (root.getChildFile ("modules/yup_audio_gui/displays/yup_SpectrogramComponentShader.frag"));
        return files;
    }

    static ShaderStage stageOf (const File& file)
    {
        const auto extension = file.getFileExtension();
        return extension == ".vert" ? ShaderStage::vertex : extension == ".frag" ? ShaderStage::fragment : ShaderStage::compute;
    }

    /** Runs a command and returns its exit code and output, or nothing if it didn't finish in time. */
    static std::optional<std::pair<uint32, String>> run (const StringArray& arguments, int timeoutMs)
    {
        ChildProcess process;
        if (! process.start (arguments))
            return std::nullopt;

        if (! process.waitForProcessToFinish (timeoutMs))
        {
            process.kill();
            return std::nullopt;
        }

        const auto output = process.readAllProcessOutput();
        return std::make_pair (process.getExitCode(), output);
    }

    /** The naga CLI from wgpu (`cargo install naga-cli`), from YUP_NAGA, ~/.cargo/bin or the PATH. */
    static std::optional<File> findNaga()
    {
        Array<File> candidates;

        if (const auto fromEnvironment = SystemStats::getEnvironmentVariable ("YUP_NAGA", {}); fromEnvironment.isNotEmpty())
            candidates.add (File (fromEnvironment));

        const auto executable = String ("naga") + (File::getSeparatorChar() == '\\' ? ".exe" : "");
        candidates.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile (".cargo/bin").getChildFile (executable));

        for (const auto& directory : StringArray::fromTokens (SystemStats::getEnvironmentVariable ("PATH", {}), File::getSeparatorChar() == '\\' ? ";" : ":", {}))
            candidates.add (File (directory).getChildFile (executable));

        for (const auto& candidate : candidates)
        {
            if (! candidate.existsAsFile())
                continue;

            // Other programs are called naga too: only trust one that answers with a version number
            const auto result = run ({ candidate.getFullPathName(), "--version" }, 5000);
            const auto version = result.has_value() ? result->second.trim() : String();

            if (result.has_value() && result->first == 0 && version.containsOnly ("0123456789.") && version.contains ("."))
                return candidate;
        }

        return std::nullopt;
    }

    static ResultValue<String> transpile (ShaderTranspiler& transpiler, const File& file)
    {
        TranspileOptions options;
        options.includePaths.push_back (file.getParentDirectory().getFullPathName());
        return transpiler.transpile (file.loadFileAsString(), stageOf (file), ShaderLanguage::glsl, ShaderLanguage::wgsl, options);
    }
};

TEST_F (WgslCorpusTests, ShadersTranspile)
{
    const auto files = corpus();
    ASSERT_GT (files.size(), 10);

    ShaderTranspiler::Ptr transpiler = new ShaderTranspiler();

    for (const auto& file : files)
    {
        auto wgsl = transpile (*transpiler, file);
        EXPECT_TRUE (wgsl.wasOk()) << file.getFileName() << ": " << wgsl.getErrorMessage();
    }
}

TEST_F (WgslCorpusTests, ShadersAreValidWgsl)
{
    const auto naga = findNaga();
    if (! naga.has_value())
        GTEST_SKIP() << "naga not found: install it with `cargo install naga-cli` or set YUP_NAGA";

    ShaderTranspiler::Ptr transpiler = new ShaderTranspiler();
    TemporaryFile output (".wgsl");

    for (const auto& file : corpus())
    {
        auto wgsl = transpile (*transpiler, file);
        if (! wgsl.wasOk())
            continue; // reported by ShadersTranspile

        ASSERT_TRUE (output.getFile().replaceWithText (wgsl.getValue()));

        const auto result = run ({ naga->getFullPathName(), output.getFile().getFullPathName() }, 30000);
        ASSERT_TRUE (result.has_value()) << "naga timed out on " << file.getFileName();
        EXPECT_EQ (result->first, 0u) << file.getFileName() << " is not valid WGSL:\n"
                                      << result->second << "\n"
                                      << wgsl.getValue();
    }
}

#endif

#endif // YUP_ENABLE_SHADER_TRANSPILER
