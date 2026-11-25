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
/** A material written as a GLSL function, which colors the meshes under a MaterialShaderNode
    instead of the lighting of their Material.

    The material is one GLSL file, with no `#version` and no `main()`. It defines a function that
    returns the color of the surface, and may open one uniform block for its parameters:

    @code
    YUP_MATERIAL_PARAMS            // optional: the uniform block of the material
    {
        vec4 rim;
        float time;
    } params;

    vec4 yupMaterial (YupSurface surface)
    {
        float fresnel = pow (1.0 - max (dot (surface.normal, surface.view), 0.0), 3.0);
        return vec4 (params.rim.rgb * fresnel, fresnel);
    }
    @endcode

    The function returns a linear, scene-referred color with a straight (not premultiplied)
    alpha. The renderer then treats it as the color its own shading computes: it is exposed, tone
    mapped and encoded as the frame needs, and on the full tier it carries the motion of the
    surface. Everything around the color stays the one of the Material of the mesh, resolved as
    without the shader (the material of the primitive, or a MaterialNode): its alpha mode decides
    whether the returned alpha is ignored (opaque), compared with the alpha cutoff (mask) or
    blended (blend), its double sided flag the culling, and the shadows the mesh casts are the
    ones of that material.

    The `YupSurface` the function is given describes the fragment, in world space:

    - `vec3 position`: the position of the fragment.
    - `vec3 normal`: the unit shading normal, after the normal texture of the material, facing
      the viewer on the back faces of a double sided material.
    - `vec3 view`: the unit vector from the fragment towards the camera.
    - `vec2 uv`: the texture coordinates of the mesh.
    - `vec4 color`: the vertex color of the mesh.
    - `vec4 baseColor`: the base color of the material, its factor times its texture and the
      vertex color: linear rgb within [0, 1], straight alpha.
    - `vec3 emissive`: the emission of the material, its factor times its texture and its
      strength: linear rgb.
    - `float metallic`, `float roughness`: the factors of the material times its texture, the
      first from 0 to 1 and the second, perceptual, from 0.04 to 1.
    - `float occlusion`: the occlusion of the material, 1 where nothing is occluded.

    So the factors and the five textures of the Material stay usable, as inputs of the function.
    The material declares no texture of its own. It may call the color helpers of the renderer
    (`yupSrgbToLinear`, `yupLinearToSrgb`, `yupLuma`).

    The shader is unlit: no light, shadow, ambient light, environment, reflection probe or
    irradiance volume reaches the function, and the screen-space effects reading the G-buffer of
    the full tier (ambient occlusion, reflections, contact shadows) neither see nor shade its
    meshes. Reflection probes and irradiance volumes capture them with their Material.

    A shader holds the function compiled for each tier of the renderer, as two shader bundles: a
    raster variant and a full variant, which also writes the motion. A shader created with one of
    them colors the meshes on that tier only, and leaves them to their Material on the other. The
    bundles are compiled ahead of time from shaders/yup_MaterialShader.vert and
    shaders/yup_MaterialShader.frag of this module with `yup_shader_bundler`: the fragment stage
    includes the material file under the name `yup_MaterialSource.glsl` when it is compiled with
    `-DYUP_3D_MATERIAL_SOURCE`, and `-DYUP_3D_TIER_FULL` makes the full variant. With the shader
    transpiler, createFromSource() compiles both from the text of the material.

    It is immutable and reference counted: create it once and give it to as many nodes as needed,
    each with its own values of the uniform block. Creating one takes no device and checks the
    bundles, so a tool without a GPU can tell what is wrong with a material.

    A shader holds no GPU object and no device, only the compiled bundles and the layout of the
    uniform block: it can outlive any window, and destroying it touches no device. Each renderer
    compiles the pipelines it draws the shader with the first time it does, keeps them with its
    own, and lets them go once the shader is destroyed.

    @code
    auto created = MaterialShader::createFromData (raster.getData(), raster.getSize(), full.getData(), full.getSize());
    if (created.failed())
        return Result::fail (created.getErrorMessage());

    auto& node = entity.attach<MaterialShaderNode> (created.getValue());
    node.getUniforms().set ("rim", { 0.0f, 1.0f, 1.0f, 1.0f });
    @endcode

    @see MaterialShaderNode, ShaderUniforms, Material, SceneRenderer
*/
class YUP_API MaterialShader : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<MaterialShader>;

    /** The binding, in set 2, of the uniform block a material opens with `YUP_MATERIAL_PARAMS`. */
    static constexpr uint32 uniformBlockBinding = 14;

    //==============================================================================
    /** Creates a material shader from its two compiled variants.

        Each bundle has to be shaders/yup_MaterialShader.vert and shaders/yup_MaterialShader.frag
        of this module compiled around a material, without `YUP_3D_TIER_FULL` for the raster
        variant and with it for the full one. Either can be nullptr: the shader then colors the
        meshes on the tier of the other only.

        @param rasterBundle  The variant the raster tier draws with, or nullptr.
        @param fullBundle    The variant the full tier draws with, or nullptr.

        @returns The shader, or what prevents the bundles from being one: no bundle at all, a
                 missing stage, a bundle that is not the material template of its tier, a material
                 declaring a uniform block that is not the one `YUP_MATERIAL_PARAMS` opens, one
                 larger than ShaderUniforms::maxSizeInBytes, a texture, a storage buffer, a storage
                 image, a subpass input or a push constant block, or two variants declaring
                 different uniform blocks.
    */
    static ResultValue<MaterialShader::Ptr> createFromBundles (ShaderBundle::Ptr rasterBundle, ShaderBundle::Ptr fullBundle);

    /** Creates a material shader from the bytes of its two compiled variants, `.ysl` bundles
        (compressed or not), taken as createFromBundles() takes them.

        @param rasterData  Start of the bytes of the raster variant, or nullptr without it.
        @param rasterSize  Number of bytes of the raster variant, 0 without it.
        @param fullData    Start of the bytes of the full variant, or nullptr without it.
        @param fullSize    Number of bytes of the full variant, 0 without it.

        @returns The shader, or why the bytes could not be loaded or are not a material shader.
    */
    static ResultValue<MaterialShader::Ptr> createFromData (const void* rasterData, size_t rasterSize, const void* fullData, size_t fullSize);

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Creates a material shader from the GLSL text of a material, compiling both variants.

        The material is compiled inside the template of this module, for the languages
        yup_shader_bundler targets by default (GLSL, ESSL, HLSL, MSL, SPIR-V and WGSL): the shader
        then draws with every graphics API, and what compiles here also compiles ahead of time.
        The errors of the compiler number the lines of the material from 1, under the name
        `material`. Only available when the shader transpiler is compiled in
        (YUP_ENABLE_SHADER_TRANSPILER = 1).

        The template and the shader library it includes are read from @p shaderDirectory: they
        are files of this module, not part of the binary. An application that ships without the
        sources of the module creates its shaders from compiled variants instead, or bundles
        those files and says where they are with setSourceDirectory().

        @param source           The text of the material file: its `yupMaterial()` function, and
                                its uniform block if it has one.
        @param shaderDirectory  The `shaders` folder of the yup_renderer module, holding
                                yup_MaterialShader.vert, yup_MaterialShader.frag and `lib`.

        @returns The shader, or the errors of the compiler, or that the template was not found.
    */
    static ResultValue<MaterialShader::Ptr> createFromSource (const String& source, const File& shaderDirectory = getSourceDirectory());

    /** Returns the folder createFromSource() reads by default: the one given to
        setSourceDirectory(), else the `shaders` folder of this module where the binary was built
        from.

        The folder the binary was built from only exists on a machine holding the sources it was
        compiled from, as a development build has them.

        @returns The folder, or a nonexistent file when the path of the sources is not known.
    */
    static File getSourceDirectory();

    /** Changes the folder createFromSource() reads by default, which is also the one a `Scene3D`
        compiles the materials of its document from.

        This is for an application that compiles materials where the sources of the module are
        not, in a browser or on a phone: it bundles yup_MaterialShader.vert,
        yup_MaterialShader.frag and the `lib` folder of the `shaders` folder of this module with
        its resources, and says here where they are, once and before it creates a material.

        @code
        MaterialShader::setSourceDirectory (File::getSpecialLocation (File::bundleDirectory).getChildFile ("yup_renderer/shaders"));
        @endcode

        @param directory  The folder holding the template and `lib`, or a default File to go back
                          to the folder the binary was built from.
    */
    static void setSourceDirectory (const File& directory);
#endif

    /** Creates the stock material shader, built with the module: it shows the base color of the
        Material of the mesh, unlit, on both tiers. Something to draw with when a material could
        not be loaded.

        @returns The shader, never nullptr.
    */
    static MaterialShader::Ptr createDefault();

    //==============================================================================
    /** Returns the variant the raster tier draws with, or nullptr for a shader created without it. */
    const ShaderBundle::Ptr& getRasterBundle() const noexcept { return rasterBundle; }

    /** Returns the variant the full tier draws with, or nullptr for a shader created without it. */
    const ShaderBundle::Ptr& getFullBundle() const noexcept { return fullBundle; }

    /** Returns the uniforms of the shader, zeroed: the bytes of the uniform block of the material
        with the layout of its members, to fill by name.

        They are empty when the material declares no uniform block. A MaterialShaderNode creates
        its own, see MaterialShaderNode::getUniforms().
    */
    ShaderUniforms createUniforms() const { return uniforms; }

    /** @internal Returns a number no other shader has, nor had: what a renderer tells the shader
        by instead of its address, which a later shader can be created at once this one died.
    */
    uint64 getIdentity() const noexcept { return identity; }

    /** @internal Returns a token that expires when the shader is destroyed. A renderer keeps it
        beside the pipelines it compiled for the shader, to let them go once the shader died.
    */
    std::weak_ptr<void> getLifetime() const noexcept { return lifetime; }

private:
    MaterialShader();

    ShaderBundle::Ptr rasterBundle;
    ShaderBundle::Ptr fullBundle;
    ShaderUniforms uniforms;
    const uint64 identity;                                     // See getIdentity()
    std::shared_ptr<void> lifetime = std::make_shared<char>(); // Owned by the shader alone, see getLifetime()

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaterialShader)
};

} // namespace yup
