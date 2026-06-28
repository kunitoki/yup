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

#include "yup_graphics/yup_AffineTransform.cpp"
#include "yup_graphics/yup_Color.cpp"
#include "yup_graphics/yup_ColorGradient.cpp"
#include "yup_graphics/yup_Drawable.cpp"
#include "yup_graphics/yup_Font.cpp"
#include "yup_graphics/yup_Graphics.cpp"
#include "yup_graphics/yup_Image.cpp"
#include "yup_graphics/yup_ImageFormatManager.cpp"
#include "yup_graphics/yup_ImageFormatReader.cpp"
#include "yup_graphics/yup_ImageFormatWriter.cpp"
#include "yup_graphics/yup_Line.cpp"
#include "yup_graphics/yup_Path.cpp"
#include "yup_graphics/yup_Point.cpp"
#include "yup_graphics/yup_Rectangle.cpp"
#include "yup_graphics/yup_RectangleList.cpp"
#include "yup_graphics/yup_Size.cpp"
#include "yup_graphics/yup_StrokeType.cpp"
#include "yup_graphics/yup_StyledText.cpp"
#include "yup_graphics/yup_SVGDocument.cpp"
#include "yup_graphics/yup_SVGParser.cpp"

#include "yup_graphics/yup_BmpImageFormat.cpp"
#include "yup_graphics/yup_PpmImageFormat.cpp"
#if YUP_MODULE_AVAILABLE_libpng && YUP_IMAGE_FORMAT_PNG
#include "yup_graphics/yup_PngImageFormat.cpp"
#endif
#if YUP_MODULE_AVAILABLE_libwebp && YUP_IMAGE_FORMAT_WEBP
#include "yup_graphics/yup_WebPImageFormat.cpp"
#endif
