#version 450
// Texture calls on arrayed, cube array, 1D, multisampled and shadow textures
layout(location = 0) in vec4 uv;
layout(location = 0) out vec4 color;
layout(set = 0, binding = 0) uniform texture2DArray arrayTex;
layout(set = 0, binding = 1) uniform sampler linearSampler;
layout(set = 0, binding = 2) uniform texture2D depthTex;
layout(set = 0, binding = 3) uniform samplerShadow shadowSampler;
layout(set = 0, binding = 4) uniform texture1D lineTex;
layout(set = 0, binding = 5) uniform texture2DMS msTex;
layout(set = 0, binding = 6) uniform textureCubeArray cubeArray;
layout(set = 0, binding = 7) uniform texture2DArray depthArray;
layout(set = 0, binding = 8) uniform textureCubeArray depthCubeArray;
layout(set = 0, binding = 9) uniform texture2D plainTex;
void main() {
    vec4 a = texture(sampler2DArray(arrayTex, linearSampler), uv.xyz);
    a += texelFetch(sampler2DArray(arrayTex, linearSampler), ivec3(1, 2, 0), 0);
    a += textureGather(sampler2DArray(arrayTex, linearSampler), uv.xyz, 1);
    a += textureGatherOffset(sampler2D(plainTex, linearSampler), uv.xy, ivec2(1, 0));
    a += texelFetchOffset(sampler2D(plainTex, linearSampler), ivec2(3), 0, ivec2(1, 1));
    a += texelFetch(sampler1D(lineTex, linearSampler), 2, 0);
    a += texelFetch(sampler2DMS(msTex, linearSampler), ivec2(0), 1);
    a.x += float(textureSamples(sampler2DMS(msTex, linearSampler)));
    a += texture(samplerCubeArray(cubeArray, linearSampler), uv);
    float s = textureLod(sampler2DShadow(depthTex, shadowSampler), uv.xyz, 0.0);
    s += texture(sampler2DArrayShadow(depthArray, shadowSampler), uv);
    s += texture(samplerCubeArrayShadow(depthCubeArray, shadowSampler), uv, 0.5);
    vec4 g = textureGather(sampler2DArrayShadow(depthArray, shadowSampler), uv.xyz, 0.5);
    ivec3 size = textureSize(sampler2DArray(arrayTex, linearSampler), 0);
    color = a + g + vec4(s) + vec4(size, 1);
}
