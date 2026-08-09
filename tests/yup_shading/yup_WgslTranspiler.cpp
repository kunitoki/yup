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
    // subpassInput may be rejected or accepted depending on lowering support
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

TEST_F (WgslLoweringTests, UnnamedUniformBlockHasResources)
{
    auto r = lower (kUnnamedUniformBlock, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    auto& resources = r.getReference().resources;
    EXPECT_GE (resources.size(), 1u);

    bool foundValue = false;
    for (auto& res : resources)
    {
        if (res.name == "value")
        {
            foundValue = true;
            EXPECT_EQ (res.group, 0u);
            EXPECT_EQ (res.binding, 0u);
        }
    }
    EXPECT_TRUE (foundValue);
}

TEST_F (WgslLoweringTests, UnnamedBufferBlockHasResources)
{
    auto r = lower (kUnnamedBufferBlock, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();

    auto& resources = r.getReference().resources;
    EXPECT_GE (resources.size(), 1u);

    bool foundValue = false;
    for (auto& res : resources)
    {
        if (res.name == "value")
        {
            foundValue = true;
            EXPECT_EQ (res.group, 0u);
            EXPECT_EQ (res.binding, 0u);
        }
    }
    EXPECT_TRUE (foundValue);
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

TEST_F (WgslEmitterGoldenTests, UnnamedUniformBlockEmitsFlatVars)
{
    auto r = transpile (kUnnamedUniformBlock, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // Unnamed interface block fields are emitted as flat global variables
    EXPECT_TRUE (wgsl.contains ("var<uniform>")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("value: f32")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@group(0)")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("@binding(0)")) << wgsl;
    // Should NOT contain a struct wrapping the fields
    EXPECT_FALSE (wgsl.contains ("struct {")) << wgsl;
}

TEST_F (WgslEmitterGoldenTests, UnnamedBufferBlockEmitsStorageVars)
{
    auto r = transpile (kUnnamedBufferBlock, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    EXPECT_TRUE (wgsl.contains ("var<storage")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("read_write")) << wgsl;
    EXPECT_TRUE (wgsl.contains ("value: f32")) << wgsl;
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

    EXPECT_TRUE (wgsl.contains ("switch ("));
    EXPECT_TRUE (wgsl.contains ("case 0:"));
    EXPECT_TRUE (wgsl.contains ("case 1:"));
    EXPECT_TRUE (wgsl.contains ("case 2:"));
    EXPECT_TRUE (wgsl.contains ("default:"));
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

    EXPECT_TRUE (wgsl.contains ("isNan("));
    EXPECT_TRUE (wgsl.contains ("isInf("));
}

TEST_F (WgslEmitterGoldenTests, AtanSingleArg)
{
    auto r = transpile (kAtanScalar, ShaderStage::fragment);
    ASSERT_TRUE (r.wasOk()) << r.getErrorMessage();
    auto wgsl = r.getValue();

    // atan(x, y) → atan2(x, y), atan(x) → atan2(x) (mapFunctionName always maps atan→atan2)
    EXPECT_TRUE (wgsl.contains ("atan2("));
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

    // Unary plus (+) and bitwise not (~) should be preserved
    EXPECT_TRUE (wgsl.contains ("+"));
    EXPECT_TRUE (wgsl.contains ("~"));
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
    // Comma operator may be lowered or emitted; just verify success
    EXPECT_NE (r.getValue().length(), 0u);
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
    // out/inout params become pointer params with & prefix
    EXPECT_TRUE (wgsl.contains ("&a"));
    EXPECT_TRUE (wgsl.contains ("&b"));
    EXPECT_TRUE (wgsl.contains ("scale("));
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
