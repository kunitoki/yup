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

    ID:                 yup_rhi
    vendor:             yup
    version:            2.0.0
    name:               YUP RHI Classes
    description:        Low-level GPU abstraction layer (RHI) with compute and render-pipeline support.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_core yup_shading yup_simd rive
    appleFrameworks:    Metal

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_RHI_H_INCLUDED

#include <yup_core/yup_core.h>
#include <yup_shading/yup_shading.h>
#include <yup_simd/yup_simd.h>

//==============================================================================
YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
#include <rive/rive.h>
#include <rive/refcnt.hpp>
#include <rive/renderer/rive_renderer.hpp>
#include <rive/renderer/render_canvas.hpp>
#include <rive/renderer/ore/ore_context.hpp>
#include <rive/renderer/ore/ore_binding_map.hpp>
YUP_END_IGNORE_WARNINGS_GCC_LIKE

//==============================================================================
/** Config: YUP_RHI_USE_GL_COMPUTE

    Enables the OpenGL compute backend, which needs desktop GL 4.3+ or GLES 3.1+ entry points.
    WebGL (and WebGPU on the web) only reach GLES 3.0, so they build without it.
*/
#ifndef YUP_RHI_USE_GL_COMPUTE
#if (YUP_RIVE_USE_OPENGL && ! YUP_WASM)
#define YUP_RHI_USE_GL_COMPUTE 1
#else
#define YUP_RHI_USE_GL_COMPUTE 0
#endif
#endif

//==============================================================================
#include <memory>
#include <optional>
#include <vector>

//==============================================================================
#include "rhi/yup_GpuTypes.h"
#include "context/yup_OffscreenTarget.h"
#include "context/yup_RenderableTarget.h"
#include "context/yup_GpuDevice.h"
#include "rhi/yup_GpuBuffer.h"
#include "rhi/yup_GpuTexture.h"
#include "rhi/yup_GpuSampler.h"
#include "rhi/yup_GpuFrame.h"
#include "rhi/yup_GpuPipeline.h"
#include "rhi/yup_GpuComputePipeline.h"
#include "rhi/yup_GpuComputePass.h"
#include "rhi/yup_GpuRenderPass.h"
#include "rhi/yup_GpuTarget.h"
#include "rhi/yup_GpuPipelineCache.h"
#include "rhi/yup_ShaderBindingMap.h"
