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


#ifndef YUP_3D_LIB_SHADOWS
#define YUP_3D_LIB_SHADOWS

#include "lib/light_types.glsl"
#include "lib/lighting.glsl"
#include "lib/sampling.glsl"
#include "lib/view.glsl"

// The shadows of the lights, declared once for the passes that shade (included by lib/lights.glsl):
// the set 3 shadow maps, compared through the set 0 shadowCompare sampler, and the ShadowBlock
// describing them. Then shadowsVisibility(), the one shadow lookup of the light loop.
// The passes including it must enable GL_EXT_texture_shadow_lod: the shadow maps are compared at an
// explicit level of 0, which is valid in the non-uniform control flow of the light loop on every
// backend (a constant level 0 compiles to textureGrad on GL and GLES).

// Mirrors GpuShadowRecord in yup_GpuSceneData.h: one shadow map view, a cascade, a spot light or
// one face of a point light
struct GpuShadowRecord
{
    mat4 viewProjection; // world to shadow clip space, depth 0 to 1
    vec4 atlasRect;      // u, v, size in UV units, 1 / atlas size (cascades: layer, 0, 1, 1 / resolution); size 0 = no map
    vec4 params;         // depth bias, normal offset (world units; point and spot lights: per unit of depth from the light), near, far
    vec4 contact;        // (contact shadow march distance (0 off), contact shadow thickness, number of views of the light (point and spot lights: 1 or 6), 0): contact writers set x and y only
};

// Mirrors GpuShadowData in yup_GpuSceneData.h
struct GpuShadowData
{
    vec4 cascadeSplits;          // view-space far distance of each cascade
    vec4 cascadeTexel;           // world size of one texel of each cascade
    vec4 sunInfo;                // cascade count, unused (0), blend band fraction, 1 when receivers lie beyond the last cascade
    vec4 filterInfo;             // soft shadows (0/1), PCF taps, blocker taps, flags (1 contact shadows, 2 temporal noise)
    GpuShadowRecord records[64]; // cascades first, then 6 per point light (and per spot light too wide for one view) and 1 per spot light
};

layout(set = 0, binding = 4) uniform samplerShadow shadowCompare;          // lessEqual, linear: bilinear comparisons
layout(set = 3, binding = 0) uniform texture2DArray shadowCascadesCompare; // the sun cascades, one layer each
layout(set = 3, binding = 2) uniform texture2D shadowAtlasCompare;         // the views of the point and spot lights, one tile each

// Soft shadows compile into the bundles of the shadow filter variant only (YUP_3D_SOFT_SHADOWS), on
// both tiers, so the default frame neither runs nor binds them
#ifdef YUP_3D_SOFT_SHADOWS
// The same maps read raw by the soft shadow blocker search, with texelFetch through pointClamp
// (lib/lighting.glsl): WGSL loads them without a sampler, as unfilterable float textures
layout(set = 3, binding = 1) uniform texture2DArray shadowCascadesDepth;
layout(set = 3, binding = 3) uniform texture2D shadowAtlasDepth;
#endif

// The shadow views of the frame, next to the shadow maps of set 3
layout(set = 3, binding = 4) uniform ShadowBlock
{
    GpuShadowData data;
} shadows;

// True when temporal anti-aliasing averages the frames (filterInfo.w flag 2): the shadow filters
// may then change every frame
bool shadowsTemporal()
{
    return mod(floor(shadows.data.filterInfo.w * 0.5), 2.0) > 0.5;
}

// Two values in [0, 1) per pixel for the shadow filters, interleaved gradient noise (Jimenez 2014).
// They change every frame, over a cycle of 64 frames, only when temporal anti-aliasing averages them
// (shadowsTemporal()); otherwise the pattern stays still
vec2 shadowsNoise()
{
    float k = shadowsTemporal() ? mod(view.clipPlanes.z, 64.0) : 0.0;
    return vec2(yupInterleavedGradientNoise(gl_FragCoord.xy + 5.588238 * k),
                yupInterleavedGradientNoise(gl_FragCoord.yx + 5.588238 * k));
}

// One bilinear comparison at uv against depth: of cascade layer `layer`, or of the atlas with the
// uv clamped to `bounds` (min uv, max uv), so no lookup reads the tile next to it
float shadowsCompare(bool atlas, vec2 uv, float layer, vec4 bounds, float depth)
{
    if (atlas)
        return textureLod(sampler2DShadow(shadowAtlasCompare, shadowCompare), vec3(clamp(uv, bounds.xy, bounds.zw), depth), 0.0);

    return textureLod(sampler2DArrayShadow(shadowCascadesCompare, shadowCompare), vec4(uv, layer, depth), 0.0);
}

// Tap i of count on a Vogel spiral over the unit disk, rotated by angle: taps spaced by the golden
// angle at radii growing with the square root of their index cover the disk evenly for any count
// (H. Vogel, "A better way to construct the sunflower head", Mathematical Biosciences 44, 1979)
vec2 shadowsVogel(int i, int count, float angle)
{
    float theta = float(i) * 2.39996323 + angle;
    return vec2(cos(theta), sin(theta)) * sqrt((float(i) + 0.5) / float(count));
}

// The filtered visibility of a shadow map around uv (texel: 1 / its size), see shadowsCompare().
// Raster: the 4x4 tent of the Phase 0 map (bilinear comparisons over a 3x3 texel footprint, after
// Reeves et al., "Rendering Antialiased Shadows with Depth Maps", SIGGRAPH 1987), in four bilinear
// lookups whose offsets and weights follow the sub-texel position (I. Castano, "Shadow Mapping
// Summary - Part 1", 2013): per axis the texels base - 1 to base + 2 weigh 1 - f, 1, 1, f.
// Full: 8 lookups on a Vogel spiral of 1.5 texels, rotated per pixel by the noise, which temporal
// anti-aliasing averages.
float shadowsFilter(bool atlas, vec2 uv, float layer, vec4 bounds, float depth, float texel, vec2 noise)
{
#ifdef YUP_3D_TIER_FULL
    float angle = noise.x * 6.28318531;
    float lit = 0.0;

    for (int i = 0; i < 8; ++i)
        lit += shadowsCompare(atlas, uv + shadowsVogel(i, 8, angle) * (1.5 * texel), layer, bounds, depth);

    return lit / 8.0;
#else
    // Per axis two lookups, between texels base - 1 and base (weights 1 - f and 1), and between
    // base + 1 and base + 2 (weights 1 and f), in texel units where texel i is centered on i + 0.5
    vec2 grid = uv / texel - 0.5;
    vec2 base = floor(grid);
    vec2 f = grid - base;

    vec2 w0 = 2.0 - f;
    vec2 w1 = 1.0 + f;
    vec2 p0 = (base - 0.5 + 1.0 / w0) * texel;
    vec2 p1 = (base + 1.5 + f / w1) * texel;

    float lit = w0.x * w0.y * shadowsCompare(atlas, p0, layer, bounds, depth)
              + w1.x * w0.y * shadowsCompare(atlas, vec2(p1.x, p0.y), layer, bounds, depth)
              + w0.x * w1.y * shadowsCompare(atlas, vec2(p0.x, p1.y), layer, bounds, depth)
              + w1.x * w1.y * shadowsCompare(atlas, p1, layer, bounds, depth);

    return lit / 9.0;
#endif
}

#ifdef YUP_3D_SOFT_SHADOWS

// The stored depth of the texel under uv: of cascade layer `layer`, or of the atlas with the uv
// clamped to `bounds` as in shadowsCompare()
float shadowsStoredDepth(bool atlas, vec2 uv, float layer, vec4 bounds, float texel)
{
    if (atlas)
        return texelFetch(sampler2D(shadowAtlasDepth, pointClamp), ivec2(clamp(uv, bounds.xy, bounds.zw) / texel), 0).r;

    ivec2 last = ivec2(round(1.0 / texel)) - 1;
    return texelFetch(sampler2DArray(shadowCascadesDepth, pointClamp), ivec3(clamp(ivec2(uv / texel), ivec2(0), last), int(layer)), 0).r;
}

// The stored depth of the receiver's plane per uv unit across the map, around the receiver at
// `position` (ndc: its normalized device coordinates in the view): shadow views, orthographic or
// perspective, map planes to planes, so the plane's stored depth is affine in the map's uv, and two
// more of its points `size` apart give it exactly (receiver plane depth bias, after J. Isidoro,
// "Shadow Mapping: GPU-based Tips and Techniques", GDC 2006). uvScale is the map's share of the uv
// (1 for a cascade, the tile size in the atlas). A plane seen edge-on gets no gradient. The plane
// follows the normal it is given, which on the full tier is the shading normal (preflight R17 / R25):
// a normal-mapped receiver tilts it with its bumps, which wide kernels can turn into self-shadowing
// or leaks, a known limit (flat geometry is exact)
vec2 shadowsReceiverGradient(mat4 viewProjection, vec3 position, vec3 normal, vec3 ndc, float size, float uvScale)
{
    vec3 tangent = normalize(cross(normal, abs(normal.y) < 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));
    vec4 b = viewProjection * vec4(position + tangent * size, 1.0);
    vec4 c = viewProjection * vec4(position + cross(normal, tangent) * size, 1.0);
    if (b.w <= 0.0 || c.w <= 0.0)
        return vec2(0.0);

    vec3 k = cross(b.xyz / b.w - ndc, c.xyz / c.w - ndc);
    if (abs(k.z) <= 1.0e-4 * length(k))
        return vec2(0.0);

    // ndc x = 2 u, y = -2 v over the map's share of the uv
    return vec2(-k.x, k.y) / k.z * (2.0 / uvScale);
}

// The stored depth of a perspective view (nearFar its near and far planes) as a view depth: blockers
// average in view depth. A cascade's stored depth is linear already (nearFar 0)
float shadowsLinearDepth(float stored, vec2 nearFar)
{
    return nearFar.y > 0.0 ? nearFar.x * nearFar.y / (nearFar.y - stored * (nearFar.y - nearFar.x)) : stored;
}

// The blocker search of percentage-closer soft shadows: filterInfo.z taps over a disk of `radius`
// (uv units) around uv. A tap blocks when its stored depth lies nearer the light than the receiver's
// plane there (stored `depth` at uv, `gradient` per uv unit), less the plane's change over the texel
// read (texel: 1 / the map's size), so the receiver never blocks itself; and nearer than the
// receiver, so where the plane recedes, the surfaces of a concave corner do not block it. Taps off
// the map (`bounds`: min uv, max uv) read nothing: a clamped lookup would repeat its edge. Returns the summed depth of
// the blockers, in view depth for a perspective view (see shadowsLinearDepth), and how many there are
vec2 shadowsFindBlockers(bool atlas, vec2 uv, float layer, vec4 bounds, float depth, vec2 gradient, float texel, float radius, float angle, vec2 nearFar)
{
    int taps = int(shadows.data.filterInfo.z);
    float margin = length(gradient) * texel;
    vec2 found = vec2(0.0);

    for (int i = 0; i < taps; ++i)
    {
        vec2 offset = shadowsVogel(i, taps, angle) * radius;
        if (any(lessThan(uv + offset, bounds.xy)) || any(greaterThan(uv + offset, bounds.zw)))
            continue;

        float stored = shadowsStoredDepth(atlas, uv + offset, layer, bounds, texel);
        if (stored < min(depth + dot(gradient, offset), depth) - margin)
            found += vec2(shadowsLinearDepth(stored, nearFar), 1.0);
    }

    return found;
}

// The filter of percentage-closer soft shadows: filterInfo.y comparisons over a disk of `radius`
// (uv units) around uv, each against the receiver's plane there (see shadowsFindBlockers), less its
// change over the bilinear footprint of the comparison. Taps off the map (`bounds`: min uv, max uv)
// are left out, as the blocker search leaves them out
float shadowsSoftFilter(bool atlas, vec2 uv, float layer, vec4 bounds, float depth, vec2 gradient, float texel, float radius, float angle)
{
    int taps = int(shadows.data.filterInfo.y);
    float margin = length(gradient) * texel;
    vec2 lit = vec2(0.0);

    for (int i = 0; i < taps; ++i)
    {
        vec2 offset = shadowsVogel(i, taps, angle) * radius;
        if (any(lessThan(uv + offset, bounds.xy)) || any(greaterThan(uv + offset, bounds.zw)))
            continue;

        lit += vec2(shadowsCompare(atlas, uv + offset, layer, bounds, min(depth + dot(gradient, offset), depth) - margin), 1.0);
    }

    return lit.y > 0.0 ? lit.x / lit.y : 1.0;
}

// The widest blocker search and filter of soft shadows, in texels: wider when temporal anti-aliasing
// averages the rotated taps over many frames. A few taps over a wider disk would miss small casters
float shadowsSoftLimit()
{
    return shadowsTemporal() ? 64.0 : 32.0;
}

// Soft visibility of cascade c at uv for a sun of angular radius atan (tanRadius): percentage-closer
// soft shadows (R. Fernando, "Percentage-Closer Soft Shadows", SIGGRAPH 2005 sketches). The
// cascade's depth is linear over its depth range (params.w), so a depth difference times that range
// is a world distance along the light. The search covers the blockers that can hide part of the sun
// from the receiver, between the near end of the range and the receiver; their average depth sets
// the penumbra, (receiver - blocker) * tan (radius) in world units, filtered over as many texels.
// No blocker leaves the receiver lit. The taps turn by the noise and follow the receiver's plane
// (shadowsReceiverGradient: position, normal and ndc of the receiver)
float sunSoftShadow(int c, vec2 uv, float depth, vec3 position, vec3 normal, vec3 ndc, vec2 noise, float tanRadius)
{
    float layer = shadows.data.records[c].atlasRect.x;
    float texel = shadows.data.records[c].atlasRect.w;
    float depthRange = shadows.data.records[c].params.w;
    float texelWorld = shadows.data.cascadeTexel[c];
    float limit = shadowsSoftLimit();
    float angle = noise.x * 6.28318531;
    vec2 gradient = shadowsReceiverGradient(shadows.data.records[c].viewProjection, position, normal, ndc, 16.0 * texelWorld, 1.0);

    float search = min(tanRadius * depthRange * depth / texelWorld, limit);
    vec4 bounds = vec4(0.0, 0.0, 1.0, 1.0);
    vec2 blockers = shadowsFindBlockers(false, uv, layer, bounds, depth, gradient, texel, search * texel, angle, vec2(0.0));
    if (blockers.y < 0.5)
        return 1.0;

    float penumbra = clamp((depth - blockers.x / blockers.y) * depthRange * tanRadius / texelWorld, 1.0, limit);
    return shadowsSoftFilter(false, uv, layer, bounds, depth, gradient, texel, penumbra * texel, angle);
}

// Soft visibility in a point or spot light view of the atlas at uv, for a light of radius
// lightRadius: percentage-closer soft shadows (Fernando 2005, see sunSoftShadow()) in perspective.
// The receiver's stored depth and the blockers' linearize to view depths with the record's near and
// far planes (params.z, w). The search covers the cone from the receiver to the light where it
// crosses the near plane, the penumbra is (receiver - blocker) * radius / blocker in world units at
// the receiver, and both turn into uv through the view's projection, at most shadowsSoftLimit()
// texels, a quarter of the tile, and the distance to the tile's edge: the guard band of a cube face
// spans a couple of texels, so a soft shadow narrows to them where it crosses a face edge, a known
// limit that keeps it continuous there.
// No blocker leaves the receiver lit. The taps follow the receiver's plane, as the sun's do
float localSoftShadow(vec2 uv, vec4 rect, vec4 bounds, vec3 position, vec3 normal, vec3 ndc, mat4 viewProjection, vec4 params, vec2 noise, float lightRadius)
{
    // The uv of one world unit across the view at a depth of 1: the projection's x scale, over the tile
    float scale = 0.5 * rect.z * length(vec3(viewProjection[0][0], viewProjection[1][0], viewProjection[2][0]));
    // The kernels stay inside the tile, the face and its guard band: they narrow towards a face edge
    // rather than lose the taps beyond it
    vec2 edge = min(uv - bounds.xy, bounds.zw - uv);
    float limit = max(min(min(0.25 * rect.z, shadowsSoftLimit() * rect.w), min(edge.x, edge.y)), rect.w);
    float angle = noise.x * 6.28318531;
    float near = params.z;
    float receiver = shadowsLinearDepth(ndc.z, params.zw);
    vec2 gradient = shadowsReceiverGradient(viewProjection, position, normal, ndc, 16.0 * rect.w * receiver / scale, rect.z);

    float search = min(lightRadius * (receiver - near) / receiver * scale / near, limit);
    vec2 blockers = shadowsFindBlockers(true, uv, 0.0, bounds, ndc.z, gradient, rect.w, search, angle, params.zw);
    if (blockers.y < 0.5)
        return 1.0;

    float blocker = blockers.x / blockers.y;
    float penumbra = clamp((receiver - blocker) * lightRadius / blocker * scale / receiver, rect.w, limit);
    return shadowsSoftFilter(true, uv, 0.0, bounds, ndc.z, gradient, rect.w, penumbra, angle);
}

// True when the frame filters with soft shadows and the light has a size to soften them with
bool shadowsAreSoft(float lightSize)
{
    return shadows.data.filterInfo.x > 0.5 && lightSize > 0.0;
}
#endif

// The filtered visibility of cascade c at a world position, from its record: offset along the
// geometric normal, more where the light grazes the surface, then compared with its depth bias.
// A sun of tan (angular radius) lightSize > 0 casts soft shadows when the frame asks for them
// (YUP_3D_SOFT_SHADOWS bundles only)
float shadowsCascadeVisibility(int c, vec3 worldPosition, vec3 geometricNormal, float nDotL, vec2 noise, float lightSize)
{
    vec4 params = shadows.data.records[c].params;
    vec3 position = worldPosition + geometricNormal * (params.y * (1.0 - nDotL));

    vec4 clip = shadows.data.records[c].viewProjection * vec4(position, 1.0);
    vec2 uv = vec2(clip.x * 0.5 + 0.5, 0.5 - clip.y * 0.5);
    float depth = clip.z - params.x;

    if (any(lessThanEqual(uv, vec2(0.0))) || any(greaterThanEqual(uv, vec2(1.0))) || depth >= 1.0)
        return 1.0;

    float layer = shadows.data.records[c].atlasRect.x;
    float texel = shadows.data.records[c].atlasRect.w;

#ifdef YUP_3D_SOFT_SHADOWS
    if (shadowsAreSoft(lightSize))
        return sunSoftShadow(c, uv, depth, position, geometricNormal, clip.xyz, noise, lightSize);
#endif

    return shadowsFilter(false, uv, layer, vec4(0.0), depth, texel, noise);
}

// Visibility of the sun at a world position: 1 lit, 0 shadowed. Cascaded shadow maps (W. Engel,
// "Cascaded Shadow Maps", ShaderX5, 2006): the first cascade whose far split lies beyond the view
// depth, filtered with percentage closer filtering (Reeves et al. 1987, see shadowsCascadeVisibility).
// Over the last sunInfo.z of its depth range a cascade hands over to the next one, dithered by the
// noise (one lookup) where temporal anti-aliasing averages it, blended linearly otherwise. The last cascade reaches every receiver,
// unless the far plane or the shadow distance cut it short (sunInfo.w): it then fades out to lit, so
// the end of the shadows never pops. lightSize is the tangent of the sun's angular radius
float sunShadow(vec3 worldPosition, vec3 geometricNormal, vec3 toLight, float viewDepth, vec2 noise, float lightSize)
{
    int count = int(shadows.data.sunInfo.x);
    bool fadesOut = shadows.data.sunInfo.w > 0.5;

    int c = 0;
    for (int i = 0; i < 3; ++i)
        c += i + 1 < count && viewDepth >= shadows.data.cascadeSplits[i] ? 1 : 0;

    bool last = c + 1 >= count;
    float splitNear = c > 0 ? shadows.data.cascadeSplits[max(c - 1, 0)] : 0.0;
    float splitFar = shadows.data.cascadeSplits[c];

    if (last && fadesOut && viewDepth >= splitFar)
        return 1.0;

    float bandStart = splitFar - shadows.data.sunInfo.z * (splitFar - splitNear);
    float t = last && ! fadesOut ? 0.0 : clamp((viewDepth - bandStart) / max(splitFar - bandStart, 1.0e-6), 0.0, 1.0);

    float nDotL = clamp(dot(geometricNormal, toLight), 0.0, 1.0);

#ifdef YUP_3D_TIER_FULL
    if (shadowsTemporal())
    {
        if (! last && noise.y < t)
            return shadowsCascadeVisibility(c + 1, worldPosition, geometricNormal, nDotL, noise, lightSize);

        float dithered = shadowsCascadeVisibility(c, worldPosition, geometricNormal, nDotL, noise, lightSize);
        return last ? mix(dithered, 1.0, t) : dithered;
    }
#endif

    float visibility = shadowsCascadeVisibility(c, worldPosition, geometricNormal, nDotL, noise, lightSize);
    if (t <= 0.0)
        return visibility;

    float next = last ? 1.0 : shadowsCascadeVisibility(c + 1, worldPosition, geometricNormal, nDotL, noise, lightSize);
    return mix(visibility, next, t);
}

// Visibility of a point or spot light: firstRecord from GpuLight.spotRadiusShadow.w. 1 lit, 0 shadowed.
// Perspective shadow maps (L. Williams, "Casting Curved Shadows on Curved Surfaces", SIGGRAPH 1978)
// in tiles of the atlas: one view for a spot light, or six for a point light (and a spot light too
// wide for one), +X, -X, +Y, -Y, +Z, -Z, of which the major axis from the light picks one. Each view
// spans a guard band beyond its 90 degrees, so the filter of a receiver near the edge of a face stays
// inside the face, and the lookups are clamped to the tile. The receiver moves along its geometric
// normal by the normal offset, more where the light grazes it, and towards the light by the depth
// bias, both scaled by its depth in the view as a texel is; the filter is the sun's (shadowsFilter),
// or soft shadows (localSoftShadow) for a light of radius lightRadius > 0 when the frame asks for them.
// A record without a map (atlasRect.z == 0) leaves the light unshadowed.
float localShadow(int firstRecord, vec3 worldPosition, vec3 geometricNormal, vec3 lightPosition, vec2 noise, float lightRadius)
{
    if (shadows.data.records[firstRecord].atlasRect.z <= 0.0)
        return 1.0;

    vec3 fromLight = worldPosition - lightPosition;
    int r = firstRecord;

    if (shadows.data.records[firstRecord].contact.z > 1.5)
    {
        vec3 a = abs(fromLight);
        int axis = a.x >= a.y && a.x >= a.z ? 0 : (a.y >= a.z ? 1 : 2);
        r += axis * 2 + (fromLight[axis] < 0.0 ? 1 : 0);
    }

    mat4 viewProjection = shadows.data.records[r].viewProjection;
    vec4 params = shadows.data.records[r].params;
    vec4 rect = shadows.data.records[r].atlasRect;

    // Behind the view of a spot light: outside its cone, which lights nothing there
    float viewDepth = (viewProjection * vec4(worldPosition, 1.0)).w;
    if (viewDepth <= 0.0)
        return 1.0;

    vec3 toLight = -fromLight / max(length(fromLight), 1.0e-6);
    float nDotL = clamp(dot(geometricNormal, toLight), 0.0, 1.0);
    vec3 position = worldPosition + geometricNormal * (params.y * viewDepth * (1.0 - nDotL)) + toLight * (params.x * viewDepth);

    vec4 clip = viewProjection * vec4(position, 1.0);
    vec3 ndc = clip.xyz / clip.w;
    if (ndc.z >= 1.0)
        return 1.0;

    vec2 uv = rect.xy + vec2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5) * rect.z;
    vec4 bounds = vec4(rect.xy + 0.5 * rect.w, rect.xy + rect.z - 0.5 * rect.w);

#ifdef YUP_3D_SOFT_SHADOWS
    if (shadowsAreSoft(lightRadius))
        return localSoftShadow(clamp(uv, bounds.xy, bounds.zw), rect, bounds, position, geometricNormal, ndc, viewProjection, params, noise, lightRadius);
#endif

    return shadowsFilter(true, clamp(uv, bounds.xy, bounds.zw), 0.0, bounds, ndc.z, rect.w, noise);
}

// The shadow of one light at a world position: 1 lit, 0 shadowed, from the first shadow record of
// the light (spotRadiusShadow.w, -1 for none), dispatched by type: a directional light whose first
// record is a cascade is the sun; any other directional record (contact shadows only) has no map;
// point and spot lights read their views in the atlas (localShadow), from the light's center.
// The light's radius softens the shadows: the angular radius of a directional light, clamped at
// 1.5 rad as its area lighting does, the world radius of point and spot lights
float shadowsVisibility(GpuLight light, vec3 worldPosition, vec3 geometricNormal)
{
    float record = light.spotRadiusShadow.w;
    if (record < 0.0)
        return 1.0;

    if (light.positionType.w < 0.5)
    {
        if (record >= shadows.data.sunInfo.x)
            return 1.0;

        float viewDepth = -(view.view * vec4(worldPosition, 1.0)).z;
        return sunShadow(worldPosition, geometricNormal, -light.directionRange.xyz, viewDepth, shadowsNoise(), tan(min(light.spotRadiusShadow.z, 1.5)));
    }

    return localShadow(int(record), worldPosition, geometricNormal, light.positionType.xyz, shadowsNoise(), light.spotRadiusShadow.z);
}

#ifdef YUP_3D_CONTACT_SHADOWS
// Contact shadows (full tier, the passes reading the scene depth): the pass defining
// YUP_3D_CONTACT_SHADOWS declares the scene depth u_depth before including this file.

// Screen-space visibility toward a light over a short distance, 1 lit, 0 occluded (full tier).
// A view-space ray march through the scene depth (P. Karabelas, "Screen space shadows", Spartan
// engine): 8 steps over marchDistance, the start jittered by the noise, which temporal anti-aliasing
// averages. A step lies in a shadow when the depth buffer holds a surface in front of it by less
// than thickness, the depth assumed for what the buffer shows. Hits fade out near the screen edges,
// beyond which the march sees nothing.
float contactShadow(vec3 viewPosition, vec3 viewToLight, float marchDistance, float thickness, float noise)
{
    vec3 stepVector = viewToLight * (marchDistance / 8.0);
    vec3 position = viewPosition + stepVector * noise;

    for (int i = 0; i < 8; ++i)
    {
        vec4 clip = view.projection * vec4(position, 1.0);
        if (clip.w <= 0.0)
            return 1.0;

        vec2 uv = vec2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
        if (any(lessThan(uv, vec2(0.0))) || any(greaterThanEqual(uv, vec2(1.0))))
            return 1.0;

        // Texels at depth 1 show no surface
        float depth = texelFetch(sampler2D(u_depth, pointClamp), ivec2(uv * view.viewport.xy), 0).r;
        float rayDepth = -position.z;

        if (depth < 1.0)
        {
            float sceneDepth = viewLinearDepth(depth);
            if (sceneDepth < rayDepth && rayDepth - sceneDepth < thickness)
            {
                vec2 edge = min(uv, 1.0 - uv);
                return 1.0 - clamp(min(edge.x, edge.y) * 20.0, 0.0, 1.0);
            }
        }

        position += stepVector;
    }

    return 1.0;
}

// The contact shadow of one light at a world position: 1 lit, 0 shadowed. Its settings live in the
// light's first shadow record, contact.x the march distance (0 off) and contact.y the thickness,
// marched when the tier enables contact shadows (filterInfo.w flag 1). The ray heads for the light's
// center, no farther than the light, and starts a pixel's footprint above the surface along its
// normal, so flat surfaces do not shadow themselves. A surface facing away from that center is not
// marched: the ray would run into the surface itself, while an area light below its horizon may
// still light it
float shadowsContact(GpuLight light, vec3 worldPosition, vec3 normal)
{
    float record = light.spotRadiusShadow.w;
    if (record < 0.0 || mod(shadows.data.filterInfo.w, 2.0) < 0.5)
        return 1.0;

    vec4 contact = shadows.data.records[int(record)].contact;
    if (contact.x <= 0.0)
        return 1.0;

    vec3 toLight = -light.directionRange.xyz;
    float marchDistance = contact.x;

    if (light.positionType.w > 0.5)
    {
        vec3 offset = light.positionType.xyz - worldPosition;
        float lightDistance = length(offset);
        toLight = offset / max(lightDistance, 1.0e-6);
        marchDistance = min(marchDistance, lightDistance);
    }

    if (dot(normal, toLight) <= 0.0)
        return 1.0;

    // Clip w is the view depth, or 1 for orthographic projections
    vec3 viewPosition = (view.view * vec4(worldPosition, 1.0)).xyz;
    float pixel = 2.0 * (view.projection * vec4(viewPosition, 1.0)).w * view.viewport.w / view.projection[1][1];
    viewPosition += mat3(view.view) * normal * pixel;

    return contactShadow(viewPosition, mat3(view.view) * toLight, marchDistance, contact.y, shadowsNoise().y);
}
#endif

#endif
