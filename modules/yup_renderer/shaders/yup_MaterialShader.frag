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
#extension GL_GOOGLE_include_directive : require

#include "lib/forward.glsl"

// The fragment stage of a MaterialShader: the meshes under a MaterialShaderNode take their color
// from the yupMaterial() function of a material file instead of the lighting of
// yup_ForwardPass.frag. The file has no #version and no main(): it defines
//
//     vec4 yupMaterial(YupSurface surface)
//
// returning the color of the surface, linear and scene-referred, with a straight (not
// premultiplied) alpha. Nothing lights it: no light, shadow, ambient light, environment or probe
// is read here, so the lighting libraries are left out and no other permutation is needed.
// The color is written as yup_ForwardPass.frag writes its own (lib/forward.glsl): exposed and
// encoded per output mode, with the motion of the fragment on the full tier. Its alpha follows the
// alpha mode of the material of the mesh (materialAlpha() in lib/material.glsl): ignored when the
// material is opaque, compared with its cutoff when it is alpha masked, blended otherwise.
//
// Compiled with YUP_3D_MATERIAL_SOURCE, the stage includes the material file from the include path
// under the name yup_MaterialSource.glsl, and MaterialShader::createFromSource() puts the text of
// the material in the place of that #include. Without the define it compiles the stock material
// below, which the module bundles. Two variants: raster, and YUP_3D_TIER_FULL.

// The surface yupMaterial() colors, in world space
struct YupSurface
{
    vec3 position;     // of the fragment
    vec3 normal;       // unit shading normal, after the normal texture of the material, facing the viewer on the back faces of a double sided material
    vec3 view;         // unit vector from the fragment towards the camera
    vec2 uv;           // texture coordinates of the mesh
    vec4 color;        // vertex color of the mesh
    vec4 baseColor;    // base color of the material: its factor times its texture and the vertex color, linear rgb within [0, 1], straight alpha
    vec3 emissive;     // emission of the material: its factor times its texture and its strength, linear rgb
    float metallic;    // of the material: its factor times its texture, from 0 to 1
    float roughness;   // of the material, perceptual: its factor times its texture, from 0.04 to 1
    float occlusion;   // of the material: 1 + strength * (texture - 1), 1 where nothing is occluded
};

// Opens the uniform block of a material, one at most, where MaterialShader binds the values of its
// MaterialShaderNode (set 2, MaterialShader::uniformBlockBinding):
// YUP_MATERIAL_PARAMS { vec4 tint; float time; } params;
#define YUP_MATERIAL_PARAMS layout(set = 2, binding = 14) uniform MaterialParams

#ifdef YUP_3D_MATERIAL_SOURCE
#include "yup_MaterialSource.glsl"
#else
// The stock material: the base color of the material of the mesh, unlit
vec4 yupMaterial(YupSurface surface)
{
    return surface.baseColor;
}
#endif

void main()
{
    MaterialSurface m = materialSample();

    YupSurface surface;
    surface.position = v_worldPosition;
    surface.normal = m.n;
    surface.view = normalize(view.cameraPosition.xyz - v_worldPosition);
    surface.uv = v_uv;
    surface.color = v_color;
    surface.baseColor = m.baseColor;
    surface.emissive = m.emissive;
    surface.metallic = m.metallic;
    surface.roughness = m.roughness;
    surface.occlusion = m.occlusion;

    // Before materialAlpha() discards the fragment an alpha mask cuts out: the material may take
    // screen-space derivatives
    vec4 color = yupMaterial(surface);

    forwardWrite(color.rgb, materialAlpha(color.a));
}
