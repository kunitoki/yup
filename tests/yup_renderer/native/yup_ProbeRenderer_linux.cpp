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

#include <gtest/gtest.h>

#include <yup_renderer/yup_renderer.h>

#include <SDL3/SDL.h>

#include <cstdlib>

using namespace yup;

#if YUP_ENABLE_SHADER_TRANSPILER

class ProbeRendererGLTests : public ProbeRendererGpuTests
{
protected:
    void SetUp() override
    {
        setenv ("LIBGL_ALWAYS_SOFTWARE", "1", 0);
        setenv ("GALLIUM_DRIVER", "llvmpipe", 0);

        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengl");
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow ("yup_probe_renderer_gl_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (window == nullptr)
            GTEST_SKIP() << "Cannot create an SDL window: " << SDL_GetError();

        glContext = SDL_GL_CreateContext (window);
        if (glContext == nullptr)
            GTEST_SKIP() << "Cannot create an OpenGL context: " << SDL_GetError();

        SDL_GL_MakeCurrent (window, glContext);

        GpuDevice::Options options;
        options.loaderFunction = (GpuDevice::LoaderFunction) SDL_GL_GetProcAddress;
        options.readableFramebuffer = true;

        device = GpuDevice::create (GpuPlatform::OpenGL, options);
        if (device == nullptr)
            GTEST_SKIP() << "No OpenGL device available";

        context = GraphicsContext::createContext (GpuPlatform::OpenGL, options, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No OpenGL graphics context available";

        probes = std::make_unique<ProbeRenderer> (device);
    }

    void TearDown() override
    {
        lookupSampler = nullptr;
        probes = nullptr;
        context = nullptr;
        device = nullptr;

        if (glContext != nullptr)
            SDL_GL_DestroyContext (glContext);

        if (window != nullptr)
            SDL_DestroyWindow (window);
    }

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
};

TEST_F (ProbeRendererGLTests, ConstantRadianceStaysConstantAtEveryMip)
{
    checkConstantRadianceStaysConstantAtEveryMip (2.0e-3f);
}

TEST_F (ProbeRendererGLTests, PrefilterMatchesTheCpuGgxReference)
{
    checkPrefilterMatchesTheCpuGgxReference (0.05f);
}

TEST_F (ProbeRendererGLTests, OctahedralTileMatchesTheCube)
{
    checkOctahedralTileMatchesTheCube (4.0f / 255.0f);
}

TEST_F (ProbeRendererGLTests, PrefilterToAtlasKeepsTheOtherTiles)
{
    checkPrefilterToAtlasKeepsTheOtherTiles();
}

TEST_F (ProbeRendererGLTests, FractionalLodReadsNoNeighborTile)
{
    checkFractionalLodReadsNoNeighborTile();
}

TEST_F (ProbeRendererGLTests, PrefilterMipZeroIsACopyOfTheSource)
{
    checkPrefilterMipZeroIsACopyOfTheSource();
}

TEST_F (ProbeRendererGLTests, PrefilteredCubeHasNoSeams)
{
    checkPrefilteredCubeHasNoSeams();
}

TEST_F (ProbeRendererGLTests, IrradianceMatchesTheCpuProjection)
{
    checkIrradianceMatchesTheCpuProjection (skyGradient, 0.01f);
}

TEST_F (ProbeRendererGLTests, VisibilityHoldsTheMomentsOfTheDistance)
{
    checkVisibilityHoldsTheMomentsOfTheDistance();
}

TEST_F (ProbeRendererGLTests, VisibilityBorderMirrorsTheTileEdges)
{
    checkVisibilityBorderMirrorsTheTileEdges();
}

//==============================================================================
class EnvironmentMapGLTests : public EnvironmentMapGpuTests
{
protected:
    void SetUp() override
    {
        setenv ("LIBGL_ALWAYS_SOFTWARE", "1", 0);
        setenv ("GALLIUM_DRIVER", "llvmpipe", 0);

        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengl");
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow ("yup_environment_map_gl_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (window == nullptr)
            GTEST_SKIP() << "Cannot create an SDL window: " << SDL_GetError();

        glContext = SDL_GL_CreateContext (window);
        if (glContext == nullptr)
            GTEST_SKIP() << "Cannot create an OpenGL context: " << SDL_GetError();

        SDL_GL_MakeCurrent (window, glContext);

        GpuDevice::Options options;
        options.loaderFunction = (GpuDevice::LoaderFunction) SDL_GL_GetProcAddress;
        options.readableFramebuffer = true;

        device = GpuDevice::create (GpuPlatform::OpenGL, options);
        if (device == nullptr)
            GTEST_SKIP() << "No OpenGL device available";

        context = GraphicsContext::createContext (GpuPlatform::OpenGL, options, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No OpenGL graphics context available";

        probes = std::make_unique<ProbeRenderer> (device);
    }

    void TearDown() override
    {
        probes = nullptr;
        context = nullptr;
        device = nullptr;

        if (glContext != nullptr)
            SDL_GL_DestroyContext (glContext);

        if (window != nullptr)
            SDL_DestroyWindow (window);
    }

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
};

TEST_F (EnvironmentMapGLTests, BuildsAPrefilteredCube)
{
    checkBuildsAPrefilteredCube();
}

TEST_F (EnvironmentMapGLTests, ConstantEnvironmentStaysConstantAtEveryLevel)
{
    checkConstantEnvironmentStaysConstantAtEveryLevel (2.0e-3f);
}

TEST_F (EnvironmentMapGLTests, BuildKeepsWhatTheFrameDoesNotRetain)
{
    checkBuildKeepsWhatTheFrameDoesNotRetain();
}

TEST_F (EnvironmentMapGLTests, RgbmEnvironmentMatchesFloat)
{
    checkRgbmEnvironmentMatchesFloat();
}

TEST_F (EnvironmentMapGLTests, FailedBuildReportsItsStatus)
{
    checkFailedBuildReportsItsStatus();
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
