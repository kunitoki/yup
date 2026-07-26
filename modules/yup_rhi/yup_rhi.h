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

    dependencies:       yup_core yup_shading rive_renderer
    appleFrameworks:    Metal

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_RHI_H_INCLUDED

#include <yup_core/yup_core.h>
#include <yup_shading/yup_shading.h>

//==============================================================================
YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
#include <rive/refcnt.hpp>
#include <rive_renderer/rive_renderer.h>
#include <rive/renderer/render_canvas.hpp>
#include <rive/renderer/ore/ore_context.hpp>
#include <rive/renderer/ore/ore_binding_map.hpp>
YUP_END_IGNORE_WARNINGS_GCC_LIKE

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
#include "rhi/yup_GpuFrame.h"
#include "rhi/yup_GpuPipeline.h"
#include "rhi/yup_GpuComputePipeline.h"
#include "rhi/yup_GpuComputePass.h"
#include "rhi/yup_GpuRenderPass.h"
#include "rhi/yup_GpuTarget.h"
#include "rhi/yup_GpuPipelineCache.h"
#include "rhi/yup_ShaderBindingMap.h"
