/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include "../mocks/rive_gpu.h"
#include "../mocks/rive_ore.h"
#include "../mocks/yup_graphics.h"

#include "yup_AffineTransform.cpp"
#include "yup_Color.cpp"
#include "yup_ColorGradient.cpp"
#include "yup_CubicBezier.cpp"
#include "yup_Drawable.cpp"
#include "yup_Font.cpp"
#include "yup_Graphics.cpp"
#include "yup_GraphicsContext.cpp"
#include "yup_GraphicsOffscreen.cpp"
#include "yup_GpuCanvas.cpp"
#include "yup_Image.cpp"
#include "yup_ImageFormatManager.cpp"
#include "yup_ImageFormatMetadataExtended.cpp"
#include "yup_ImageFormatReader.cpp"
#include "yup_ImageFormatWriter.cpp"
#include "yup_ImageFileIO.cpp"
#include "yup_ImageDataFiles.cpp"
#include "yup_ImageFormats.cpp"
#include "yup_ImageMetadata.cpp"
#include "yup_ImageFormatMetadata.cpp"
#include "yup_Line.cpp"
#include "yup_Path.cpp"
#include "yup_Point.cpp"
#include "yup_Rectangle.cpp"
#include "yup_RectangleList.cpp"
#include "yup_Size.cpp"
#include "yup_StrokeType.cpp"
#include "yup_StyledText.cpp"
#include "yup_SVGDocument.cpp"
#include "yup_SVGParser.cpp"

#if YUP_IMAGE_FORMAT_BMP
#include "yup_BmpImageFormat.cpp"
#endif
#if YUP_IMAGE_FORMAT_PPM
#include "yup_PpmImageFormat.cpp"
#endif
#if YUP_IMAGE_FORMAT_TGA
#include "yup_TgaImageFormat.cpp"
#endif
#if YUP_MODULE_AVAILABLE_libpng && YUP_IMAGE_FORMAT_PNG
#include "yup_PngImageFormat.cpp"
#endif
#if YUP_MODULE_AVAILABLE_libjpeg && YUP_IMAGE_FORMAT_JPEG
#include "yup_JpegImageFormat.cpp"
#endif
#if YUP_MODULE_AVAILABLE_libwebp && YUP_IMAGE_FORMAT_WEBP
#include "yup_WebPImageFormat.cpp"
#endif
#if YUP_MODULE_AVAILABLE_libgif && YUP_IMAGE_FORMAT_GIF
#include "yup_GifImageFormat.cpp"
#endif
#if YUP_MODULE_AVAILABLE_libtiff && YUP_IMAGE_FORMAT_TIFF
#include "yup_TiffImageFormat.cpp"
#endif
