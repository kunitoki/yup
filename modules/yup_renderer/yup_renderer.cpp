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

#ifdef YUP_RENDERER_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#include "yup_renderer.h"

//==============================================================================

#include <cmath>
#include <cstring>

//==============================================================================

#include "core/yup_RenderCapabilities.cpp"
#include "scene/yup_SceneDrawList.cpp"
#include "scene/yup_GpuSceneData.cpp"
#include "scene/yup_MaterialTable.cpp"
#include "scene/yup_GeometryBindings.cpp"
#include "core/yup_LtcTables.cpp"
#include "core/yup_LookupTables.cpp"
#include "scene/yup_ViewData.cpp"
#include "shadows/yup_ShadowAtlas.cpp"
#include "probes/yup_ProbeAtlas.cpp"
#include "shadows/yup_ShadowCascades.cpp"
#include "core/yup_FrameResources.cpp"
#include "passes/yup_LightClusters.cpp"
#include "core/yup_FullscreenPass.cpp"
#include "probes/yup_ProbeRenderer.cpp"
#include "shadows/yup_ShadowRenderer.cpp"
#include "passes/yup_LightingInputs.cpp"
#include "passes/yup_ForwardPass.cpp"
#include "passes/yup_GBufferPass.cpp"
#include "passes/yup_DepthNormalPrepass.cpp"
#include "screenspace/yup_AmbientOcclusionPass.cpp"
#include "screenspace/yup_HiZPyramid.cpp"
#include "screenspace/yup_ReflectionPass.cpp"
#include "passes/yup_LightingPass.cpp"
#include "post/yup_TemporalAA.cpp"
#include "post/yup_ToneMapping.cpp"
#include "post/yup_Bloom.cpp"
#include "post/yup_DepthOfField.cpp"
#include "post/yup_MotionBlur.cpp"
#include "post/yup_FogApply.cpp"
#include "post/yup_PostChain.cpp"
#include "core/yup_FrameSignature.h"
#include "probes/yup_ProbeBaker.cpp"
#include "core/yup_SceneRenderer.cpp"
#include "core/yup_SceneComponent.cpp"
