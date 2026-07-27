/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#ifdef YUP_GRAPHICS_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#include "yup_graphics.h"

//==============================================================================

YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
#include <rive/renderer/rive_renderer.hpp>
#include <rive/renderer/rive_render_image.hpp>
#include <rive/text/font_hb.hpp>
#include <rive/renderer/ore/ore_context.hpp>
#include <rive/renderer/ore/ore_binding_map.hpp>
#include <rive/renderer/ore/ore_bind_group_layout.hpp>
#include <rive/renderer/ore/ore_pipeline.hpp>
#include <rive/renderer/ore/ore_bind_group.hpp>
YUP_END_IGNORE_WARNINGS_GCC_LIKE

//==============================================================================

#include <libclipper2/libclipper2.h>

//==============================================================================

#if YUP_ENABLE_SHADER_COMPILER
#include <glslang/glslang.h>
#include <spirv_cross/spirv_cross.h>
#endif

//==============================================================================

#if YUP_WINDOWS

#if YUP_RIVE_USE_D3D
#include <array>
#include <dxgi1_2.h>

#include "native/yup_GraphicsContext_d3d.cpp"
#endif

#if YUP_RIVE_USE_OPENGL
#include "native/yup_GraphicsContext_opengl.cpp"
#endif

//==============================================================================

#elif YUP_MAC || YUP_IOS

#if YUP_RIVE_USE_METAL
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#if YUP_MAC
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#endif

#include "native/yup_GraphicsContext_metal.cpp"
#endif

#if YUP_RIVE_USE_OPENGL
#include "native/yup_GraphicsContext_opengl.cpp"
#endif

//==============================================================================

#elif YUP_LINUX || YUP_WASM || YUP_ANDROID

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include "native/yup_GraphicsContext_webgpu.cpp"
#else

#if YUP_EMSCRIPTEN && RIVE_WEBGL
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#include "native/yup_GraphicsContext_opengl.cpp"
#endif

#endif

//==============================================================================

#if YUP_RIVE_USE_DAWN
#include "native/yup_GraphicsContext_dawn.cpp"
#include "native/yup_GraphicsContext_dawn_helper.cpp"
#endif

//==============================================================================

#ifndef YUP_DRAWABLE_LOGGING
#define YUP_DRAWABLE_LOGGING 0
#endif

#if YUP_DRAWABLE_LOGGING
#define YUP_DRAWABLE_LOG(x) YUP_DBG (x)
#else
#define YUP_DRAWABLE_LOG(x)
#endif

//==============================================================================

#include "native/yup_GraphicsContext_headless.cpp"

//==============================================================================
#include "context/yup_GraphicsContext.cpp"
#include "rhi/yup_GpuTexture.cpp"
#include "primitives/yup_Path.cpp"
#include "primitives/yup_CubicBezier.cpp"
#include "fonts/yup_Font.cpp"
#include "fonts/yup_StyledText.cpp"
#include "imaging/yup_ImagePixelData.cpp"
#include "imaging/yup_Image.cpp"
#include "imaging/yup_ImageMetadata.cpp"
#include "imaging/yup_ImageFormat.cpp"
#include "imaging/yup_ImageFormatReader.cpp"
#include "imaging/yup_ImageFormatWriter.cpp"
#include "imaging/yup_ImageFormatManager.cpp"
#include "graphics/yup_Color.cpp"
#include "graphics/yup_Colors.cpp"
#include "graphics/yup_Graphics.cpp"
#include "svg/yup_SVGDocument.cpp"
#include "svg/yup_SVGCssParser.cpp"
#include "svg/yup_SVGParser.cpp"
#include "drawables/yup_Drawable.cpp"
#include "rhi/yup_ShaderBindingMap.cpp"
#include "rhi/yup_GpuBuffer.cpp"
#include "rhi/yup_GpuPipeline.cpp"
#include "rhi/yup_GpuFrame.cpp"
#include "rhi/yup_GpuRenderPass.cpp"
#include "rhi/yup_GpuTarget.cpp"
#include "rhi/yup_GpuCanvas.cpp"
#include "rhi/yup_GpuPipelineCache.cpp"

//==============================================================================
#if YUP_IMAGE_FORMAT_BMP
#include "formats/yup_BmpImageFormat.cpp"
#endif

#if YUP_IMAGE_FORMAT_PPM
#include "formats/yup_PpmImageFormat.cpp"
#endif

#if YUP_IMAGE_FORMAT_TGA
#include "formats/yup_TgaImageFormat.cpp"
#endif

#if YUP_IMAGE_FORMAT_PNG
#include <libpng/libpng.h>
#include "formats/yup_PngImageFormat.cpp"
#endif

#if YUP_IMAGE_FORMAT_JPEG
#include <libjpeg/libjpeg.h>
#include "formats/yup_JpegImageFormat.cpp"
#endif

#if YUP_IMAGE_FORMAT_WEBP
#include <libwebp/libwebp.h>
#include "formats/yup_WebPImageFormat.cpp"
#endif

#if YUP_IMAGE_FORMAT_GIF
#include "formats/yup_GifImageFormat.cpp"
#endif

#if YUP_IMAGE_FORMAT_TIFF
#include <libtiff/libtiff.h>
#include "formats/yup_TiffImageFormat.cpp"
#endif
