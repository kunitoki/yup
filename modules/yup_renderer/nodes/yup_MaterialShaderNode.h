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

#pragma once

namespace yup
{

//==============================================================================
/** A part that colors every mesh in the subtree of its entity with a MaterialShader, instead of
    the lighting of their materials.

    The shader applies to the entity's own meshes and to all of its descendants, and the nearest
    MaterialShaderNode up the tree wins, as with MaterialNode. The materials of the meshes are
    resolved as without it: the shader reads their base color and textures, and their alpha mode,
    alpha cutoff and double sided flag still decide how the meshes are drawn.

    The node holds its own values of the uniform block of the shader, so the meshes of two nodes
    sharing a shader can differ:

    @code
    auto& hologram = entity.attach<yup::MaterialShaderNode> (shader);
    hologram.getUniforms().set ("rim", { 0.0f, 1.0f, 1.0f, 1.0f });
    hologram.getUniforms().set ("time", seconds);
    @endcode

    SceneRenderer draws the meshes with the shader on the tiers it has a variant for, where its
    pipelines compile; elsewhere, and in the captures of reflection probes and irradiance volumes,
    they are drawn with their materials. Changing the shader or a value restarts the convergence
    of the full tier by itself, see SceneRenderer::isConverged().

    @see MaterialShader, MaterialNode, Entity
*/
class YUP_API MaterialShaderNode : public Node
{
public:
    /** Creates a part without a shader, which changes nothing. */
    MaterialShaderNode() = default;

    /** Creates a part coloring with a shader, its uniforms zeroed. */
    explicit MaterialShaderNode (MaterialShader::Ptr shaderToUse)
    {
        setShader (std::move (shaderToUse));
    }

    /** Changes the shader, and resets the uniforms to the zeroed ones of the new shader.

        @param newShader  The shader, or nullptr to leave the meshes to their materials, or to a
                          MaterialShaderNode further up the tree.
    */
    void setShader (MaterialShader::Ptr newShader)
    {
        shader = std::move (newShader);
        uniforms = shader != nullptr ? shader->createUniforms() : ShaderUniforms();
    }

    /** Returns the shader, or nullptr without one. */
    const MaterialShader::Ptr& getShader() const noexcept { return shader; }

    /** Returns the values of the uniform block of the shader for the meshes under this node, to
        set by member name. They are empty without a shader, or when its material declares no block.

        Set their members: uniforms taken from another shader would not fit this one, and the
        meshes would then be drawn with zeroed values.
    */
    ShaderUniforms& getUniforms() noexcept { return uniforms; }

    /** Returns the values of the uniform block of the shader for the meshes under this node. */
    const ShaderUniforms& getUniforms() const noexcept { return uniforms; }

private:
    MaterialShader::Ptr shader;
    ShaderUniforms uniforms;
};

} // namespace yup
