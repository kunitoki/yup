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


#ifndef YUP_3D_LIB_MATERIAL_TYPES
#define YUP_3D_LIB_MATERIAL_TYPES

// Mirrors GpuMaterial in yup_GpuSceneData.h
struct GpuMaterial
{
    vec4 specular;         // rgb specular color, w specular factor (KHR_materials_specular)
    vec4 extended;         // x ior (KHR_materials_ior), y emissive strength (KHR_materials_emissive_strength), zw reserved
};

#endif
