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

class GpuComputePassImplGL final : public GpuComputePass::Impl
{
public:
    explicit GpuComputePassImplGL (GpuDevice& deviceToUse)
        : device (deviceToUse)
    {
    }

    bool isValid() const override { return true; }

    //==========================================================================

    bool dispatch (uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override
    {
        if (pipelineRef == nullptr)
            return false;

        auto* pipe = dynamic_cast<GpuComputePipelineGL*> (pipelineRef.get());
        if (pipe == nullptr || pipe->getProgram() == 0)
            return false;

        device.runOnComputeContext ([&]
        {
            GLint previousProgram = 0;
            GLint previousUniformBuffer = 0;
            glGetIntegerv (GL_CURRENT_PROGRAM, &previousProgram);
            glGetIntegerv (GL_UNIFORM_BUFFER_BINDING, &previousUniformBuffer);

            glUseProgram (pipe->getProgram());

            for (auto& sb : storageBindings)
            {
                if (sb.buffer == nullptr)
                    continue;

                auto* bufImpl = sb.buffer->getImpl();
                if (bufImpl == nullptr || bufImpl->glStorageBuffer.id == 0)
                    continue;

                GLuint index = static_cast<GLuint> (sb.group * 16 + sb.binding);
                glBindBufferBase (GL_SHADER_STORAGE_BUFFER, index, bufImpl->glStorageBuffer.id);
            }

            for (auto& ub : uboBindings)
            {
                if (ub.data.empty())
                    continue;

                GLuint ubo = 0;
                glGenBuffers (1, &ubo);
                if (ubo == 0)
                    continue;

                glBindBuffer (GL_UNIFORM_BUFFER, ubo);
                glBufferData (GL_UNIFORM_BUFFER,
                              static_cast<GLsizeiptr> (ub.data.size()),
                              ub.data.data(),
                              GL_DYNAMIC_DRAW);

                GLuint index = static_cast<GLuint> (ub.group * 16 + ub.binding);
                glBindBufferBase (GL_UNIFORM_BUFFER, index, ubo);

                tempBuffers.push_back (ubo);
            }

            glDispatchCompute (groupsX, groupsY, groupsZ);

            glUseProgram (static_cast<GLuint> (previousProgram));
            glBindBuffer (GL_UNIFORM_BUFFER, static_cast<GLuint> (previousUniformBuffer));
        });

        return true;
    }

    //==========================================================================

    void finish() override
    {
        device.runOnComputeContext ([&]
        {
            if (! tempBuffers.empty())
            {
                glDeleteBuffers (static_cast<GLsizei> (tempBuffers.size()), tempBuffers.data());
                tempBuffers.clear();
            }

            glMemoryBarrier (GL_SHADER_STORAGE_BARRIER_BIT
                             | GL_UNIFORM_BARRIER_BIT
                             | GL_BUFFER_UPDATE_BARRIER_BIT);
        });
    }

private:
    GpuDevice& device;
    std::vector<GLuint> tempBuffers;
};

//==============================================================================

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplGL (GpuDevice& device)
{
    return std::make_unique<GpuComputePassImplGL> (device);
}

} // namespace yup

#endif // OpenGL
