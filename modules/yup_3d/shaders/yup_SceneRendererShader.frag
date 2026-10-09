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

#version 450

// glTF metallic-roughness shading with KHR_lights_punctual lights and a flat ambient term.
// Normal mapping builds the tangent frame from screen-space derivatives, so meshes need no
// tangent attribute, and the same derivatives filter the highlights of smooth surfaces.
// An optional environment adds reflections from a latitude-longitude image whose mip levels are
// blurred per roughness, and diffuse light from nine spherical harmonics coefficients. One
// directional light can be shadowed from a packed depth map, filtered with a 4x4 tent.
// Color textures are decoded from sRGB here, and the result is exposed, tone mapped (Reinhard
// or ACES) and sRGB encoded for an rgba8unorm target.

// Must match FrameData in yup_SceneRenderer.cpp and yup_SceneRendererShader.vert
layout(set = 0, binding = 0) uniform FrameData
{
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
    vec4 ambient;
    vec4 lightInfo;
    vec4 lightPositions[8];
    vec4 lightDirections[8];
    vec4 lightColors[8];
    vec4 lightSpots[8];
    mat4 shadowMatrix;
    vec4 shadowInfo;          // shadowed light index (-1 for none), texel size, normal offset, depth bias
    vec4 environmentInfo;     // intensity (0 for none), last level, tone mapping (0 Reinhard, 1 ACES)
    vec4 irradiance[9];
} frame;

// Must match MaterialData in yup_SceneRenderer.cpp
layout(set = 0, binding = 2) uniform MaterialData
{
    vec4 baseColorFactor;
    vec4 emissiveFactor;      // rgb emissive, w alpha cutoff
    vec4 params;              // metallic, roughness, normal scale, occlusion strength
    vec4 flags;               // alpha mode (0 opaque, 1 mask, 2 blend), base color sRGB, emissive sRGB, has normal texture
} material;

layout(set = 0, binding = 3) uniform texture2D u_baseColor;
layout(set = 0, binding = 4) uniform texture2D u_metallicRoughness;
layout(set = 0, binding = 5) uniform texture2D u_normal;
layout(set = 0, binding = 6) uniform texture2D u_occlusion;
layout(set = 0, binding = 7) uniform texture2D u_emissive;
layout(set = 0, binding = 8) uniform sampler s_baseColor;
layout(set = 0, binding = 9) uniform sampler s_metallicRoughness;
layout(set = 0, binding = 10) uniform sampler s_normal;
layout(set = 0, binding = 11) uniform sampler s_occlusion;
layout(set = 0, binding = 12) uniform sampler s_emissive;
// Metal allows sampler slots up to 15: the shadow map is read with texelFetch, which ignores
// the sampler it is paired with
layout(set = 0, binding = 13) uniform texture2D u_environment;
layout(set = 0, binding = 14) uniform texture2D u_shadow;
layout(set = 0, binding = 15) uniform sampler s_environment;

layout(location = 0) in vec3 v_worldPosition;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec4 v_color;

layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

vec3 srgbToLinear(vec3 c)
{
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 linearToSrgb(vec3 c)
{
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), c));
}

vec3 environmentIrradiance(vec3 n)
{
    vec3 result = frame.irradiance[0].rgb
                + frame.irradiance[1].rgb * n.y
                + frame.irradiance[2].rgb * n.z
                + frame.irradiance[3].rgb * n.x
                + frame.irradiance[4].rgb * (n.x * n.y)
                + frame.irradiance[5].rgb * (n.y * n.z)
                + frame.irradiance[6].rgb * (3.0 * n.z * n.z - 1.0)
                + frame.irradiance[7].rgb * (n.x * n.z)
                + frame.irradiance[8].rgb * (n.x * n.x - n.y * n.y);

    return max(result, vec3(0.0));
}

vec3 environmentRadiance(vec3 d, float roughness)
{
    // Around the vertical axis from +Z, and from the zenith down; the level follows the roughness
    // atan is undefined at the poles, where both arguments are zero
    vec2 uv = vec2(atan(d.x, d.z + 1.0e-6) / (2.0 * PI), acos(clamp(d.y, -1.0, 1.0)) / PI);
    return textureLod(sampler2D(u_environment, s_environment), uv, roughness * frame.environmentInfo.y).rgb;
}

// Split-sum scale and bias of the specular reflectance, fitted analytically (Karis, mobile)
vec2 environmentBrdf(float roughness, float nDotV)
{
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * nDotV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

float unpackDepth(vec4 rgba)
{
    return dot(rgba, vec4(1.0, 1.0 / 255.0, 1.0 / 65025.0, 1.0 / 16581375.0));
}

float shadowVisibility(vec3 worldPosition, vec3 geometricNormal, vec3 l)
{
    // Offset along the normal, more where the light grazes the surface
    float nDotL = clamp(dot(geometricNormal, l), 0.0, 1.0);
    vec3 position = worldPosition + geometricNormal * (frame.shadowInfo.z * (1.0 - nDotL));

    vec4 clip = frame.shadowMatrix * vec4(position, 1.0);
    vec2 coords = vec2(clip.x * 0.5 + 0.5, 0.5 - clip.y * 0.5);
    if (coords.x <= 0.0 || coords.y <= 0.0 || coords.x >= 1.0 || coords.y >= 1.0 || clip.z >= 1.0)
        return 1.0;

    float texel = frame.shadowInfo.y;
    float depth = clip.z - frame.shadowInfo.w;

    // Bilinear comparisons over a 3x3 texel footprint: a 4x4 tent
    vec2 grid = coords / texel - 0.5;
    vec2 base = floor(grid);
    vec2 f = grid - base;

    int size = int(1.0 / texel + 0.5);
    ivec2 origin = ivec2(base) - ivec2(1);

    float lit = 0.0;
    for (int j = 0; j < 4; ++j)
    {
        float wy = j == 0 ? 1.0 - f.y : (j == 3 ? f.y : 1.0);

        for (int i = 0; i < 4; ++i)
        {
            float wx = i == 0 ? 1.0 - f.x : (i == 3 ? f.x : 1.0);
            ivec2 position = clamp(origin + ivec2(i, j), ivec2(0), ivec2(size - 1));
            float stored = unpackDepth(texelFetch(sampler2D(u_shadow, s_environment), position, 0));
            lit += wx * wy * (depth <= stored ? 1.0 : 0.0);
        }
    }

    return lit / 9.0;
}

vec3 acesFilmic(vec3 x)
{
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

vec3 perturbNormal(vec3 n, vec3 tangentNormal, float side)
{
    vec3 dpdx = dFdx(v_worldPosition);
    vec3 dpdy = dFdy(v_worldPosition);
    vec2 duvdx = dFdx(v_uv);
    vec2 duvdy = dFdy(v_uv);

    float det = duvdx.x * duvdy.y - duvdy.x * duvdx.y;
    if (abs(det) < 1.0e-12)
        return n * side;

    vec3 t = (duvdy.y * dpdx - duvdx.y * dpdy) / det;
    t = t - n * dot(n, t);

    float tangentLength = length(t);
    if (tangentLength < 1.0e-12)
        return n * side;

    // Back faces flip the whole tangent frame, not just the normal
    t = t / tangentLength;
    vec3 b = cross(n, t);

    return normalize(mat3(t * side, b * side, n * side) * tangentNormal);
}

void main()
{
    vec4 baseColorSample = texture(sampler2D(u_baseColor, s_baseColor), v_uv);
    if (material.flags.y > 0.5)
        baseColorSample.rgb = srgbToLinear(baseColorSample.rgb);

    vec4 baseColor = material.baseColorFactor * baseColorSample * v_color;

    // The normal uses screen-space derivatives: compute it before any fragment is discarded
    float side = gl_FrontFacing ? 1.0 : -1.0;
    vec3 n = normalize(v_normal);
    vec3 geometricNormal = n * side;

    if (material.flags.w > 0.5)
    {
        vec3 tangentNormal = texture(sampler2D(u_normal, s_normal), v_uv).xyz * 2.0 - 1.0;
        tangentNormal.xy = tangentNormal.xy * material.params.z;
        n = perturbNormal(n, tangentNormal, side);
    }
    else
    {
        n = n * side;
    }

    // Specular antialiasing: widen the highlight where the normal varies within the pixel, so
    // very smooth curved surfaces don't sparkle (Tokuyoshi and Kaplanyan, filtered GGX)
    vec3 dndx = dFdx(n);
    vec3 dndy = dFdy(n);
    float kernelRoughness = min(0.3 * (dot(dndx, dndx) + dot(dndy, dndy)), 0.2);

    if (material.flags.x < 0.5)
    {
        baseColor.a = 1.0;
    }
    else if (material.flags.x < 1.5)
    {
        if (baseColor.a < material.emissiveFactor.w)
            discard;

        baseColor.a = 1.0;
    }

    vec4 metallicRoughness = texture(sampler2D(u_metallicRoughness, s_metallicRoughness), v_uv);
    float metallic = clamp(material.params.x * metallicRoughness.b, 0.0, 1.0);
    float roughness = clamp(material.params.y * metallicRoughness.g, 0.04, 1.0);
    float occlusion = 1.0 + material.params.w * (texture(sampler2D(u_occlusion, s_occlusion), v_uv).r - 1.0);

    vec3 emissive = texture(sampler2D(u_emissive, s_emissive), v_uv).rgb;
    if (material.flags.z > 0.5)
        emissive = srgbToLinear(emissive);

    emissive = emissive * material.emissiveFactor.rgb;

    vec3 v = normalize(frame.cameraPosition.xyz - v_worldPosition);
    vec3 f0 = mix(vec3(0.04), baseColor.rgb, metallic);
    vec3 diffuseColor = baseColor.rgb * (1.0 - metallic);
    float alpha = roughness * roughness;
    float alpha2 = clamp(alpha * alpha + kernelRoughness, 0.0, 1.0);
    float nDotV = max(dot(n, v), 0.0001);

    vec3 color = vec3(0.0);
    int lightCount = int(frame.lightInfo.x);

    for (int i = 0; i < 8; ++i)
    {
        if (i >= lightCount)
            break;

        vec4 positionType = frame.lightPositions[i];
        vec4 directionRange = frame.lightDirections[i];
        vec4 colorIntensity = frame.lightColors[i];

        vec3 l = -directionRange.xyz;
        float attenuation = 1.0;

        if (positionType.w > 0.5)
        {
            vec3 toLight = positionType.xyz - v_worldPosition;
            float distanceSquared = max(dot(toLight, toLight), 0.0001);
            float lightDistance = sqrt(distanceSquared);

            l = toLight / lightDistance;
            attenuation = 1.0 / distanceSquared;

            if (directionRange.w > 0.0)
            {
                float ratio = lightDistance / directionRange.w;
                float ratio2 = ratio * ratio;
                attenuation = attenuation * clamp(1.0 - ratio2 * ratio2, 0.0, 1.0);
            }

            if (positionType.w > 1.5)
            {
                vec4 spot = frame.lightSpots[i];
                float cone = clamp(dot(directionRange.xyz, -l) * spot.x + spot.y, 0.0, 1.0);
                attenuation = attenuation * cone * cone;
            }
        }

        float nDotL = dot(n, l);
        if (nDotL <= 0.0 || attenuation <= 0.0)
            continue;

        if (float(i) == frame.shadowInfo.x)
        {
            attenuation = attenuation * shadowVisibility(v_worldPosition, geometricNormal, l);
            if (attenuation <= 0.0)
                continue;
        }

        vec3 h = normalize(v + l);
        float nDotH = max(dot(n, h), 0.0);
        float vDotH = max(dot(v, h), 0.0);

        // GGX distribution, height-correlated Smith visibility, Schlick Fresnel
        float d = nDotH * nDotH * (alpha2 - 1.0) + 1.0;
        float distribution = alpha2 / (PI * d * d);

        float ggxV = nDotL * sqrt(nDotV * nDotV * (1.0 - alpha2) + alpha2);
        float ggxL = nDotV * sqrt(nDotL * nDotL * (1.0 - alpha2) + alpha2);
        float visibility = 0.5 / max(ggxV + ggxL, 0.0001);

        vec3 fresnel = f0 + (vec3(1.0) - f0) * pow(1.0 - vDotH, 5.0);

        vec3 diffuse = (vec3(1.0) - fresnel) * diffuseColor / PI;
        vec3 specular = fresnel * (distribution * visibility);

        color += (diffuse + specular) * colorIntensity.rgb * (colorIntensity.w * nDotL * attenuation);
    }

    color += frame.ambient.rgb * (diffuseColor + f0) * occlusion;

    if (frame.environmentInfo.x > 0.0)
    {
        // The filtered roughness, so the reflections blur where the highlights do
        float environmentRoughness = sqrt(sqrt(alpha2));
        vec3 r = reflect(-v, n);

        vec2 brdf = environmentBrdf(environmentRoughness, nDotV);
        vec3 specularWeight = f0 * brdf.x + brdf.y;

        // Reflections pointing into the surface are hidden by it
        float horizon = clamp(1.0 + dot(r, geometricNormal), 0.0, 1.0);

        vec3 diffuse = diffuseColor * environmentIrradiance(n) * (vec3(1.0) - specularWeight);
        vec3 specular = environmentRadiance(r, environmentRoughness) * specularWeight * (horizon * horizon);

        color += (diffuse + specular) * (occlusion * frame.environmentInfo.x);
    }

    color += emissive;

    color = color * frame.ambient.w;
    color = frame.environmentInfo.z > 0.5 ? acesFilmic(color) : color / (vec3(1.0) + color);

    fragColor = vec4(linearToSrgb(color), baseColor.a);
}
