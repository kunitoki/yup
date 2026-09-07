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

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID

namespace yup
{

//==============================================================================

class GpuComputePipelineGL final : public GpuComputePipeline
{
public:
    GpuComputePipelineGL (GpuDevice::Ptr deviceToUse, GLuint program, GpuWorkgroupSize wgs)
        : device (std::move (deviceToUse))
        , glProgram (program)
        , workgroupSize (wgs)
    {
    }

    ~GpuComputePipelineGL() override
    {
        if (glProgram == 0)
            return;

        if (device != nullptr)
        {
            device->runOnComputeContext ([program = glProgram]
            {
                glDeleteProgram (program);
            });
        }
        else
        {
            glDeleteProgram (glProgram);
        }
    }

    GpuWorkgroupSize getWorkgroupSize() const noexcept override { return workgroupSize; }

    GLuint getProgram() const noexcept { return glProgram; }

private:
    GpuDevice::Ptr device;
    GLuint glProgram;
    GpuWorkgroupSize workgroupSize;
};

//==============================================================================

ResultValue<GpuComputePipeline::Ptr> yup_constructComputePipelineGL (GpuDevice::Ptr device,
                                                                     const GpuShaderSource& source,
                                                                     const GpuWorkgroupSize& workgroupSize)
{
    if (source.code == nullptr || source.codeSize == 0)
        return makeResultValueFail ("Compute shader source is empty");

    if (source.language != GpuShaderLanguage::glsl)
        return makeResultValueFail ("OpenGL compute shaders must be GLSL");

    const auto* glslSource = static_cast<const char*> (source.code);
    auto glslLength = static_cast<GLint> (source.codeSize);

    GLuint shader = glCreateShader (GL_COMPUTE_SHADER);
    if (shader == 0)
        return makeResultValueFail ("Failed to create GL compute shader object");

    glShaderSource (shader, 1, &glslSource, &glslLength);
    glCompileShader (shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE)
    {
        GLint logLen = 0;
        glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &logLen);
        String errMsg = "GL compute shader compilation failed";
        if (logLen > 1)
        {
            std::vector<char> log (static_cast<size_t> (logLen));
            glGetShaderInfoLog (shader, logLen, nullptr, log.data());
            errMsg += ": ";
            errMsg += log.data();
        }
        glDeleteShader (shader);
        return makeResultValueFail (errMsg);
    }

    GLuint program = glCreateProgram();
    glAttachShader (program, shader);
    glLinkProgram (program);

    GLint linked = GL_FALSE;
    glGetProgramiv (program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE)
    {
        GLint logLen = 0;
        glGetProgramiv (program, GL_INFO_LOG_LENGTH, &logLen);
        String errMsg = "GL compute program linking failed";
        if (logLen > 1)
        {
            std::vector<char> log (static_cast<size_t> (logLen));
            glGetProgramInfoLog (program, logLen, nullptr, log.data());
            errMsg += ": ";
            errMsg += log.data();
        }
        glDeleteShader (shader);
        glDeleteProgram (program);
        return makeResultValueFail (errMsg);
    }

    glDeleteShader (shader);

    return makeResultValueOk (GpuComputePipeline::Ptr (new GpuComputePipelineGL (std::move (device), program, workgroupSize)));
}

} // namespace yup

#endif // OpenGL
