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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                   yup_renderer
    vendor:               yup
    version:              2.0.0
    name:                 YUP Renderer
    description:          PBR rendering of yup_3d scenes through the RHI, on a raster and a full (deferred, temporal) tier.
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_3d yup_gui
    testDeps:             libpng

    shaderDirectory:      shaders
    shaderIncludes:       shaders/lib
    shaderDefinePrefix:   YUP_3D_
    shaderBundles:        ForwardPass AREA_LIGHTS? SOFT_SHADOWS?
                          ForwardPass AREA_LIGHTS? SOFT_SHADOWS? LOCAL_INDIRECT LAYERS?
                          ForwardPass TIER_FULL AREA_LIGHTS? SOFT_SHADOWS?
                          LightingPass TIER_FULL AREA_LIGHTS? SOFT_SHADOWS?
                          LightingPass TIER_FULL AREA_LIGHTS? SOFT_SHADOWS? LOCAL_INDIRECT LAYERS?
                          LightingClassify TIER_FULL LOCAL_INDIRECT?
                          GBufferPass TIER_FULL
                          ShadowPass MASKED?
                          DebugOverlay BOXES?
                          TemporalAA UPSCALE?
                          ToneMapping RADIANCE_COMPOSITE?
                          GtaoTrace SCREEN_SPACE_GI?
                          GtaoDenoise SCREEN_SPACE_GI?
                          Background, GBufferBackground, DepthNormalPrepass, LightClusters, DebugView
                          BloomDownsample, BloomUpsample, SsgiComposite, HiZCopy, HiZDownsample, SsrTrace, SsrResolve, SsrDenoise
                          DofSetup, DofTiles, DofGather, DofComposite, FogApply
                          MotionTileMax, MotionNeighborMax, MotionReconstruct
                          ProbeEquirectToCube, ProbeCubeDownsample, ProbePrefilter, ProbeCaptureResolve
                          ProbeCaptureDistance, ProbeShProjection, ProbeVisibility

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_RENDERER_H_INCLUDED

#include <yup_3d/yup_3d.h>
#include <yup_gui/yup_gui.h>

//==============================================================================

#include "resources/yup_SphericalHarmonics.h"
#include "resources/yup_EnvironmentMap.h"
#include "nodes/yup_ReflectionProbeNode.h"
#include "nodes/yup_IrradianceVolumeNode.h"
#include "nodes/yup_WorldFogNode.h"
#include "nodes/yup_BackgroundNode.h"
#include "nodes/yup_AmbientLightNode.h"
#include "nodes/yup_ExposureNode.h"
#include "nodes/yup_ToneMappingNode.h"
#include "nodes/yup_DefaultLightNode.h"
#include "nodes/yup_EnvironmentNode.h"
#include "scene/yup_SceneSettings.h"
#include "core/yup_RenderSettings.h"
#include "core/yup_RenderCapabilities.h"
#include "scene/yup_SceneDrawList.h"
#include "scene/yup_GpuSceneData.h"
#include "scene/yup_MaterialTable.h"
#include "scene/yup_GeometryBindings.h"
#include "core/yup_LtcTables.h"
#include "core/yup_LookupTables.h"
#include "scene/yup_ViewData.h"
#include "shadows/yup_ShadowAtlas.h"
#include "probes/yup_ProbeAtlas.h"
#include "shadows/yup_ShadowCascades.h"
#include "core/yup_FrameResources.h"
#include "passes/yup_LightClusters.h"
#include "core/yup_FullscreenPass.h"
#include "probes/yup_ProbeRenderer.h"
#include "shadows/yup_ShadowRenderer.h"
#include "probes/yup_ProbeBaker.h"
#include "passes/yup_LightingInputs.h"
#include "passes/yup_ForwardPass.h"
#include "passes/yup_GBufferPass.h"
#include "passes/yup_LightingPass.h"
#include "passes/yup_DepthNormalPrepass.h"
#include "passes/yup_DebugOverlayPass.h"
#include "screenspace/yup_AmbientOcclusionPass.h"
#include "screenspace/yup_HiZPyramid.h"
#include "screenspace/yup_ReflectionPass.h"
#include "post/yup_TemporalAA.h"
#include "post/yup_ToneMapping.h"
#include "post/yup_Bloom.h"
#include "post/yup_DepthOfField.h"
#include "post/yup_MotionBlur.h"
#include "post/yup_FogApply.h"
#include "post/yup_PostChain.h"
#include "core/yup_SceneRenderer.h"
#include "core/yup_SceneComponent.h"
