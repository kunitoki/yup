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

#include <yup_graphics/yup_graphics.h>

#define YUP_GRAPHICS_TESTS_GPU_PIXEL_HOST 1

namespace
{

/** The GPU context the graphics pixel tests draw with: Metal, on the default device. */
class GraphicsGpuPixelTestHost
{
public:
    /** Returns the context to draw with, or nullptr when no Metal device is available. */
    yup::GraphicsContext* getContext() const noexcept { return context.get(); }

    /** Metal has no current context to make. */
    void makeCurrent() {}

private:
    std::unique_ptr<yup::GraphicsContext> context = yup::GraphicsContext::createContext (yup::GpuPlatform::Metal, {});
};

} // namespace
