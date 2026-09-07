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

#include <yup_rhi/yup_rhi.h>

//==============================================================================
#if YUP_WINDOWS
#if YUP_RIVE_USE_D3D
#include <algorithm>
#include <array>
#include <string>

#include <d3dcompiler.h>
#include <dxgi1_2.h>

#include <rive/renderer/d3d11/d3d11.hpp>
#endif

#if YUP_RIVE_USE_OPENGL
#include <rive/renderer/gl/gles3.hpp>
#endif

#elif YUP_MAC || YUP_IOS
#if YUP_RIVE_USE_METAL
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#endif

#elif YUP_LINUX || YUP_WASM || YUP_ANDROID
#if YUP_EMSCRIPTEN && RIVE_WEBGPU
#include <emscripten/emscripten.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>
#elif YUP_EMSCRIPTEN && RIVE_WEBGL
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include <rive/renderer/gl/gles3.hpp>

#endif

#if YUP_RIVE_USE_DAWN
#include "dawn/native/DawnNative.h"
#include "dawn/dawn_proc.h"
#endif

//==============================================================================
#include "rhi/yup_GpuBuffer.cpp"

#include "native/yup_GpuDevice_headless.cpp"

#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
#include "native/yup_GpuDevice_metal.cpp"
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
#include "native/yup_GpuDevice_d3d.cpp"
#endif

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
#include "native/yup_GpuDevice_opengl.cpp"
#endif

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
#include "native/yup_GpuDevice_webgpu.cpp"
#elif YUP_RIVE_USE_DAWN
#include "native/yup_GpuDevice_dawn.cpp"
#endif

//==============================================================================
#include "rhi/yup_GpuFrame.cpp"
#include "rhi/yup_GpuPipeline.cpp"
#include "rhi/yup_GpuPipelineCache.cpp"
#include "rhi/yup_GpuRenderPass.cpp"

//==============================================================================
#include "rhi/yup_GpuComputePass.cpp"

#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
#include "native/yup_GpuComputePipeline_metal.cpp"
#include "native/yup_GpuComputePass_metal.cpp"
#endif
#if YUP_RIVE_USE_D3D && YUP_WINDOWS
#include "native/yup_GpuComputePipeline_d3d.cpp"
#include "native/yup_GpuComputePass_d3d.cpp"
#endif
#if (YUP_EMSCRIPTEN && RIVE_WEBGPU) || YUP_RIVE_USE_DAWN
#include "native/yup_GpuComputePipeline_webgpu.cpp"
#include "native/yup_GpuComputePass_webgpu.cpp"
#endif
#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID
#include "native/yup_GpuComputePipeline_opengl.cpp"
#include "native/yup_GpuComputePass_opengl.cpp"
#endif

#include "rhi/yup_GpuComputePipeline.cpp"

//==============================================================================
#include "rhi/yup_GpuTarget.cpp"
#include "rhi/yup_GpuTexture.cpp"
#include "rhi/yup_ShaderBindingMap.cpp"
#include "context/yup_GpuDevice.cpp"
