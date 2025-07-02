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

namespace yup
{

//==============================================================================
/** Specifies how to scale and fit an item to a target area.

    This enum is used to specify how to scale and fit an item to a target area.
*/
enum class Fitting
{
    none,             /**< Do not scale or fit at all. */
    scaleToFit,       /**< Scale proportionally to fit within bounds (preserve aspect ratio, no cropping). */
    fitWidth,         /**< Scale to match width, preserve aspect ratio. */
    fitHeight,        /**< Scale to match height, preserve aspect ratio. */
    scaleToFill,      /**< Scale proportionally to completely fill bounds (may crop, preserves aspect ratio). */
    fill,             /**< Stretch to fill bounds completely (aspect ratio not preserved). */
    tile,             /**< Repeat content to fill bounds (used in backgrounds, patterns). */
    centerCrop,       /**< Like scaleToFill, but ensures the center remains visible (common in media). */
    centerInside,     /**< Like scaleToFit, but does not upscale beyond original size. */
    stretchWidth,     /**< Stretch horizontally, preserve vertical size. */
    stretchHeight     /**< Stretch vertically, preserve horizontal size. */
};

} // namespace yup
