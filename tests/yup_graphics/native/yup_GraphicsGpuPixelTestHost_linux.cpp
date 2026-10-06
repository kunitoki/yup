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

#include <SDL3/SDL.h>

#define YUP_GRAPHICS_TESTS_GPU_PIXEL_HOST 1

namespace
{

/** The GPU context the graphics pixel tests draw with: OpenGL, on the context of a hidden SDL window.

    The GL context stays current for as long as the host lives, and is destroyed after the
    GraphicsContext and its device.
*/
class GraphicsGpuPixelTestHost
{
public:
    GraphicsGpuPixelTestHost()
    {
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow ("yup_graphics_gpu_pixel_tests", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (window == nullptr)
            return;

        glContext = SDL_GL_CreateContext (window);
        if (glContext == nullptr)
            return;

        makeCurrent();

        yup::GpuDevice::Options options;
        options.loaderFunction = (yup::GpuDevice::LoaderFunction) SDL_GL_GetProcAddress;
        options.readableFramebuffer = true;

        auto device = yup::GpuDevice::create (yup::GpuPlatform::OpenGL, options);
        if (device == nullptr)
            return;

        context = yup::GraphicsContext::createContext (yup::GpuPlatform::OpenGL, options, std::move (device));
    }

    ~GraphicsGpuPixelTestHost()
    {
        makeCurrent();
        context.reset();

        if (glContext != nullptr)
            SDL_GL_DestroyContext (glContext);

        if (window != nullptr)
            SDL_DestroyWindow (window);
    }

    /** Returns the context to draw with, or nullptr when no GL context could be made. */
    yup::GraphicsContext* getContext() const noexcept { return context.get(); }

    /** Makes the GL context current on the calling thread. */
    void makeCurrent()
    {
        if (window != nullptr && glContext != nullptr)
            SDL_GL_MakeCurrent (window, glContext);
    }

private:
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    std::unique_ptr<yup::GraphicsContext> context;
};

} // namespace
