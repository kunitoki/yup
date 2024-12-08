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

namespace yup
{

std::unique_ptr<GraphicsContext> GraphicsContext::createContext (Api graphicsApi, Options options)
{
    switch (graphicsApi)
    {
#if YUP_RIVE_USE_METAL && (JUCE_MAC || JUCE_IOS)
    case Api::Metal:
        return juce_constructMetalGraphicsContext (options);
#endif

#if YUP_RIVE_USE_D3D && JUCE_WINDOWS
    case Api::Direct3D:
        return juce_constructDirect3DGraphicsContext (options);
#endif

#if YUP_RIVE_USE_OPENGL || JUCE_LINUX || JUCE_WASM || JUCE_ANDROID
    case Api::OpenGL:
        return juce_constructOpenGLGraphicsContext (options);
#endif

#if YUP_RIVE_USE_VULKAN
    case Api::Vulkan:
        return juce_constructVulkanGraphicsContext (options);
#endif

#if YUP_RIVE_USE_DAWN
    case Api::Dawn:
        return juce_constructDawnGraphicsContext (options);
#endif

    default:
        Logger::outputDebugString ("Invalid API requested for current platform");
        return nullptr;
    }

    Logger::outputDebugString ("Failed to create the graphics context");
    return nullptr;
}

} // namespace yup
