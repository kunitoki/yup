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
/** A component that gives a material to the meshes of the 3D component it is in: the
    `Material3D` of a `.yui` document.

    It is not a node of the scene and has no entity. It owns a Material, described by its
    properties, and attaches it as a MaterialNode to the entity of its parent: a node, or the
    `Scene3D` itself for the whole scene. The material then replaces the one of every mesh of
    that entity and of the entities under it, the nearest `Material3D` up the tree winning, and
    stops doing so when the component goes.

    @code
    Mesh3D {
        vertices: [-1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, 1, 0];
        uvs: [0, 1,  1, 1,  1, 0,  0, 0];
        indices: [0, 1, 2,  0, 2, 3];

        Material3D { baseColorTexture: "textures/wood.png"; roughness: 0.6; metallic: 0; }
    }
    @endcode

    The properties are those of Material, all of them usable in style rules, transitions and
    keyframes, which restyle the meshes: `baseColor`, `baseColorTexture`, `metallic`,
    `roughness`, `metallicRoughnessTexture`, `normalTexture`, `normalScale`, `occlusionTexture`,
    `occlusionStrength`, `emissive`, `emissiveTexture`, `emissiveStrength`, `alphaMode`,
    `alphaCutoff`, `doubleSided`, `clearcoat`, `clearcoatRoughness`, `sheenColor` and
    `sheenRoughness`.

    The colors are colors of the interface, sRGB encoded: the material gets them as the linear
    factors it shades with (the alpha of `baseColor` as it is), and they read back as they were
    set. The textures are paths of image files, read through the document and shared by the
    materials of the same `Scene3D` (see SceneComponent::loadTexture). An image that can't be
    read or decoded is reported in the diagnostics of the document and leaves the material
    without that texture.

    A parent takes one material: its first `Material3D`. Another one is reported and not used,
    until the first one goes.

    With `shader`, the meshes are colored by a material written as a GLSL function instead of
    the lighting of the material (see MaterialShader): it names a `shader` block of the
    stylesheet styling the component, which gives the file of the material and declares its
    inputs, the members of its uniform block by name. The name is written as the other names
    of a document are, without quotes, and `none` is no shader. The component attaches a
    MaterialShaderNode beside its MaterialNode, so the material still decides the alpha mode,
    the culling, the shadows and the textures the function reads.

    @code
    shader Hologram {
        material: "fx/hologram.glsl";
        property rim: color = #00ffff;
        property strength: number = 1;
    }

    style {
        Material3D          { strength: 1; transition: strength 200ms; }
        .armed > Material3D { strength: 3; rim: palette.accent; }
    }

    Scene3D {
        Mesh3D {
            vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0];
            Material3D { shader: Hologram; baseColor: #204060; }
        }
    }
    @endcode

    Each input is a property of the `Material3D`, read as Component::findProperty() finds it:
    set on the component, animated, styled, inherited, or else the default the block gives it.
    So bindings, style rules and transitions drive the shader. A number, an integer and a
    boolean fill a scalar, a point two, a rectangle four, an array of numbers as many as it
    holds, and a color four: linear red, green and blue, as the function returns them, and the
    alpha as it is. A `time` member the block declares no property for is filled with the
    seconds the material has been shown, which repaints the scene on every frame.

    What is inherited is a value given under the name of the input to a component the material
    is in, its `Mesh3D` or its `Scene3D`, whose type has no property of that name. The `scale`
    of a node or the `time` of a model is theirs, also when a style rule gives it, and is not
    given to the shaders inside them. An input named like a property of `Material3D` itself,
    `roughness` or `opacity`, reads that property and never the default of the block: it is
    how a shader reads the factors of its material.

    The file is loaded once for the `Scene3D`, whatever the stylesheet does (see
    SceneComponent::loadMaterialShader), when the document has loaded or on the display frame
    that follows the naming of the shader, and its values are read again on every display
    frame: they are only given to the renderer when one changed.

    What prevents a shader from being used is reported in the diagnostics of the document, and
    leaves the meshes to the material. A name the stylesheet has no block for, and a block
    declaring a `fragment` instead of a `material`, are problems of the `shader` of the
    component. What prevents the material itself from loading is a problem of its block,
    reported where the block is written and once whatever the number of components naming it,
    as the styles report what prevents an image shader: at its `material` for files that can't
    be read and for what the loader or the compiler says of them, and at a `property` that
    does not fit the uniform block. A file that stops loading while it is edited leaves the
    shader as it was. All of them are reported with the load of the document, or by the display
    frame that finds them later, and again after each load for as long as they stand.

    @see SceneNodeComponent, SceneMeshComponent, Material, MaterialNode, MaterialShader, MaterialShaderNode
*/
class YUP_API SceneMaterialComponent : public Component
{
public:
    //==============================================================================
    /** Creates a white, fully rough, fully metallic material, as a default Material is.

        @param componentID The ID of the component.
    */
    explicit SceneMaterialComponent (StringRef componentID = {});

    /** Destructor. The parent stops using the material, and uses its next Material3D if it has one. */
    ~SceneMaterialComponent() override;

    //==============================================================================
    /** Returns the material the properties describe, which the component owns.

        It is the same object for the life of the component, and holds the linear factors the
        colors were converted to and the textures that could be loaded. Set the fields that have
        no property on it, then repaint the scene.
    */
    Material::Ptr getMaterial() const noexcept { return material; }

    //==============================================================================
    /** Chooses the shader coloring the meshes instead of the lighting of the material, the
        `shader` of a `Material3D`.

        The shader is looked for, and loaded, when the document has loaded, or by the next
        display frame of the SceneComponent.

        @param newName The name of a `shader` block of the stylesheet styling the component,
                       one declaring a `material`, or an empty string for no shader. A name
                       the stylesheet has no such block for is reported by that frame.
    */
    void setShaderName (const String& newName);

    /** Returns the name of the shader coloring the meshes, or an empty string. */
    const String& getShaderName() const noexcept { return shaderName; }

    //==============================================================================
    /** Returns the static type info describing the Material3D type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal */
    void parentHierarchyChanged() override;
    /** @internal Gives the entity of a 3D component the material of its first Material3D: what a parent whose entity changed calls. */
    static void updateHost (Component& hostToUpdate);
    /** @internal Loads again the textures made from the file that changed, and has the shader looked for again. */
    void assetChanged (const String& path);
    /** @internal Looks for the shaders the Material3D components shown by a scene name and reads their values again, on
        every display frame. Returns true when a value given to the renderer changed. */
    static bool updateShaders (Scene& shownScene, double elapsedSeconds);

private:
    //==============================================================================
    struct ShaderLink;
    struct ShaderProblem;

    /* An input of the shader: a property of the component, with the default its `shader` block gives it. */
    struct ShaderInput
    {
        ShaderEffect::Property property;
        var fallback;                                   // The default, when it reads nothing that changes
        std::unique_ptr<PropertyExpression> expression; // The default, when it does
    };

    template <class Value>
    void setValue (Value& value, const Value& newValue, const Identifier& property);

    void setColorValue (Color& color, Color newColor, const Identifier& property);
    void setTexturePath (size_t slot, const String& newPath);
    void updateTexture (size_t slot);
    void attachTo (Entity newTarget, bool isUsed);
    void detach();
    void repaintScene();
    void updateShaderLink();
    bool refreshShader (double elapsedSeconds, std::vector<ShaderProblem>& problems);
    void updateShader (StyleSheet::Ptr sheet, std::vector<ShaderProblem>& problems);
    bool readShaderInputs (const DataTree& block, const ShaderUniforms& uniforms, std::vector<ShaderInput>& inputs, std::vector<ShaderProblem>& problems);
    void addShaderProblem (std::vector<ShaderProblem>& problems, const DataTree& element, const Identifier& property, const String& message);
    var findShaderInput (const Identifier& name) const;
    bool updateUniforms (MaterialShaderNode& node, double elapsedSeconds);

    Material::Ptr material { new Material() };
    Color baseColor { 0xffffffff };
    Color emissive { 0xff000000 };
    Color sheenColor { 0xff000000 };
    std::array<String, 5> texturePaths;
    WeakReference<Component> host;
    Scene::Ptr scene; // Kept with the entity the material is on, which can then be asked at any time whether it is still there
    Entity target;
    bool isLeaving = false;
    String shaderName;
    String shaderPath;           // The file of the shader the meshes are colored with
    StyleSheet::Ptr shaderSheet; // The stylesheet the shader was looked for in, nullptr to look for it again
    std::weak_ptr<const DataTreeSourceMap> shaderLoad; // The source map of the load of the document it was last looked for after
    std::vector<ShaderInput> shaderInputs;
    double shaderTime = 0.0;
    bool fillsShaderTime = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneMaterialComponent)
};

} // namespace yup
