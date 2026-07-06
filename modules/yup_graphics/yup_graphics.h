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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                 yup_graphics
    vendor:             yup
    version:            2.0.0
    name:               YUP Graphics Classes
    description:        The essential set of basic YUP graphics classes.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_core yup_simd rive rive_renderer libclipper2 glslang spirv_cross
    appleFrameworks:    Metal
    searchpaths:        native

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_GRAPHICS_H_INCLUDED

#include <yup_core/yup_core.h>
#include <yup_simd/yup_simd.h>

#include <rive_renderer/rive_renderer.h>

//==============================================================================

YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wattributes", "-Wdeprecated-declarations")
#include <rive/rive.h>
#include <rive/factory.hpp>
#include <rive/text/raw_text.hpp>
#include <rive/text/utf.hpp>
YUP_END_IGNORE_WARNINGS_GCC_LIKE

//==============================================================================

#include <compare>
#include <tuple>

//==============================================================================
/** Config: YUP_IMAGE_FORMAT_BMP

    Enable BMP image format support.
*/
#ifndef YUP_IMAGE_FORMAT_BMP
#define YUP_IMAGE_FORMAT_BMP 1
#endif

/** Config: YUP_IMAGE_FORMAT_PPM

    Enable PPM/PGM/PBM image format support.
*/
#ifndef YUP_IMAGE_FORMAT_PPM
#define YUP_IMAGE_FORMAT_PPM 1
#endif

/** Config: YUP_IMAGE_FORMAT_PNG

    Enable PNG image format support.
*/
#ifndef YUP_IMAGE_FORMAT_PNG
#if YUP_MODULE_AVAILABLE_libpng
#define YUP_IMAGE_FORMAT_PNG 1
#endif
#endif

/** Config: YUP_IMAGE_FORMAT_JPEG

    Enable JPEG image format support.
*/
#ifndef YUP_IMAGE_FORMAT_JPEG
#if YUP_MODULE_AVAILABLE_libjpeg
#define YUP_IMAGE_FORMAT_JPEG 1
#endif
#endif

/** Config: YUP_IMAGE_FORMAT_WEBP

    Enable WebP image format support.
*/
#ifndef YUP_IMAGE_FORMAT_WEBP
#if YUP_MODULE_AVAILABLE_libwebp
#define YUP_IMAGE_FORMAT_WEBP 1
#endif
#endif

/** Config: YUP_IMAGE_FORMAT_GIF

    Enable GIF image format support (read and write, including animation).
    Requires libgif (YUP_MODULE_AVAILABLE_libgif).
*/
#ifndef YUP_IMAGE_FORMAT_GIF
#if YUP_MODULE_AVAILABLE_libgif
#define YUP_IMAGE_FORMAT_GIF 1
#endif
#endif

/** Config: YUP_ENABLE_SHADER_COMPILER

    Enable shader compiler support.
*/
#ifndef YUP_ENABLE_SHADER_COMPILER
#if YUP_MODULE_AVAILABLE_glslang && YUP_MODULE_AVAILABLE_spirv_cross
#define YUP_ENABLE_SHADER_COMPILER 1
#endif
#endif

//==============================================================================

#if YUP_IMAGE_FORMAT_PNG && ! YUP_MODULE_AVAILABLE_libpng
#undef YUP_IMAGE_FORMAT_PNG
#define YUP_IMAGE_FORMAT_PNG 0
#endif

#if YUP_IMAGE_FORMAT_JPEG && ! YUP_MODULE_AVAILABLE_libjpeg
#undef YUP_IMAGE_FORMAT_JPEG
#define YUP_IMAGE_FORMAT_JPEG 0
#endif

#if YUP_IMAGE_FORMAT_WEBP && ! YUP_MODULE_AVAILABLE_libwebp
#undef YUP_IMAGE_FORMAT_WEBP
#define YUP_IMAGE_FORMAT_WEBP 0
#endif

#if YUP_IMAGE_FORMAT_GIF && ! YUP_MODULE_AVAILABLE_libgif
#undef YUP_IMAGE_FORMAT_GIF
#define YUP_IMAGE_FORMAT_GIF 0
#endif

//==============================================================================

#include "layout/yup_Justification.h"
#include "layout/yup_Fitting.h"
#include "primitives/yup_AffineTransform.h"
#include "primitives/yup_Size.h"
#include "primitives/yup_Point.h"
#include "primitives/yup_Line.h"
#include "primitives/yup_Rectangle.h"
#include "primitives/yup_RectangleList.h"
#include "primitives/yup_Path.h"
#include "primitives/yup_CubicBezier.h"
#include "fonts/yup_Font.h"
#include "fonts/yup_StyledText.h"
#include "imaging/yup_Image.h"
#include "imaging/yup_ImageFormat.h"
#include "imaging/yup_ImageFormatReader.h"
#include "imaging/yup_ImageFormatWriter.h"
#include "imaging/yup_ImageFormatManager.h"
#include "graphics/yup_BlendMode.h"
#include "graphics/yup_Color.h"
#include "graphics/yup_ColorGradient.h"
#include "graphics/yup_Colors.h"
#include "graphics/yup_StrokeJoin.h"
#include "graphics/yup_StrokeCap.h"
#include "graphics/yup_StrokeType.h"
#include "graphics/yup_FillType.h"
#include "context/yup_GraphicsContext.h"
#include "graphics/yup_Graphics.h"
#include "svg/yup_SVGElement.h"
#include "svg/yup_SVGGradient.h"
#include "svg/yup_SVGClipPath.h"
#include "svg/yup_SVGMask.h"
#include "svg/yup_SVGMarker.h"
#include "svg/yup_SVGPattern.h"
#include "svg/yup_SVGFilter.h"
#include "svg/yup_SVGCssRule.h"
#include "svg/yup_SVGDocument.h"
#include "svg/yup_SVGCssParser.h"
#include "svg/yup_SVGParser.h"
#include "drawables/yup_Drawable.h"

//==============================================================================
#if YUP_IMAGE_FORMAT_BMP
#include "formats/yup_BmpImageFormat.h"
#endif

#if YUP_IMAGE_FORMAT_PPM
#include "formats/yup_PpmImageFormat.h"
#endif

#if YUP_IMAGE_FORMAT_PNG
#include "formats/yup_PngImageFormat.h"
#endif

#if YUP_IMAGE_FORMAT_JPEG
#include "formats/yup_JpegImageFormat.h"
#endif

#if YUP_IMAGE_FORMAT_WEBP
#include "formats/yup_WebPImageFormat.h"
#endif

#if YUP_IMAGE_FORMAT_GIF
#include <libgif/libgif.h>
#include "formats/yup_GifImageFormat.h"
#endif
