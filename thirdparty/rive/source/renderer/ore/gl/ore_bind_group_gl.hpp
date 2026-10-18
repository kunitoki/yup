#pragma once
#include "rive/renderer/ore/ore_bind_group.hpp"
#include <cstdint>
#include <vector>

namespace rive::ore
{
class ContextGL;
class RenderPassGL;
class TextureGL;

class BindGroupGL : public LITE_RTTI_OVERRIDE(BindGroup, BindGroupGL)
{
public:
    struct GLUBOBinding
    {
        unsigned int buffer = 0; // GLuint
        uint32_t offset = 0;
        uint32_t size = 0;
        uint32_t binding = 0; // WGSL @binding, for sort
        uint32_t slot = 0;    // GL UBO binding point
        bool hasDynamicOffset = false;
    };
    struct GLTexBinding
    {
        unsigned int texture = 0; // GLuint
        unsigned int target = 0;  // GLenum (GL_TEXTURE_2D etc.)
        uint32_t binding = 0;     // WGSL @binding, for pair lookup
        uint32_t slot = 0;        // texture unit
        TextureGL* textureGL = nullptr;
        int baseLevel = 0;
        int maxLevel = 1000; // GL's default TEXTURE_MAX_LEVEL
    };
    struct GLSamplerBinding
    {
        unsigned int sampler = 0; // GLuint
        uint32_t binding = 0;     // WGSL @binding, for pair lookup
        uint32_t slot = 0;        // sampler unit
    };
    struct GLStorageBinding
    {
        unsigned int buffer = 0; // GLuint
        uint64_t offset = 0;
        uint64_t size = 0;
        uint32_t slot = 0; // GL shader storage binding point
    };

    BindGroupGL() = default;
    ~BindGroupGL() override = default;

private:
    friend class ContextGL;
    friend class RenderPassGL;
    std::vector<GLUBOBinding> m_glUBOs;
    std::vector<GLTexBinding> m_glTextures;
    std::vector<GLSamplerBinding> m_glSamplers;
    std::vector<GLStorageBinding> m_glStorageBuffers;
};
} // namespace rive::ore
