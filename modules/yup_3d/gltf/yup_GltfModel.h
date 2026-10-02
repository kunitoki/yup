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

namespace yup
{

//==============================================================================
/** A glTF 2.0 asset converted into YUP meshes, materials, textures and entity templates.

    Load a .gltf or .glb file once, then instantiate it as many times as needed with
    createEntity(): every call builds a fresh entity tree, and all the trees share the same
    meshes, materials and textures. Copies of a GltfModel are cheap and share them too.

    @code
    auto model = yup::GltfModel::loadFromFile (file);
    if (model.wasOk())
        someEntity->addChild (model.getReference().createEntity());
    @endcode

    What is converted:
    - triangle primitives with POSITION, NORMAL, TEXCOORD_0 and COLOR_0, dense or sparse,
      any component type, normalized integers included;
    - metallic-roughness materials with their textures, alpha modes and double-sided flag;
    - images from buffer views, data URIs and external files, decoded with the image formats
      the application links (EXT_texture_webp sources need libwebp);
    - nodes with their transforms, meshes, cameras and KHR_lights_punctual lights.

    Skinning, morph targets and animations are ignored. External files are only read from
    the base directory and its subdirectories. Problems that don't prevent loading, like a
    primitive that isn't made of triangles or an image that can't be decoded, are collected
    in getWarnings().

    @see EntityNode, Mesh, Material
*/
class YUP_API GltfModel
{
public:
    //==============================================================================
    /** Creates an empty model. */
    GltfModel();

    //==============================================================================
    /** Loads a .gltf or .glb file.

        External buffers and images are resolved relative to the folder of the file.

        @param file The file to load.
    */
    static ResultValue<GltfModel> loadFromFile (const File& file);

    /** Loads a .gltf or .glb file from memory.

        @param data           The file contents.
        @param baseDirectory  The folder external buffers and images are read from. When it is
                              not set, external files can't be loaded.
    */
    static ResultValue<GltfModel> loadFromData (Span<const uint8> data, const File& baseDirectory = {});

    //==============================================================================
    /** Builds a new entity tree for a scene of the asset.

        The returned root entity is named after the scene and holds its root nodes. Every node
        becomes an EntityNode with a MeshNode, CameraNode or LightNode attached as needed.

        @param sceneIndex The scene to build, or -1 for the default one. An asset without
                          scenes builds every node that has no parent.

        @return The root entity, or nullptr if the scene index is out of range.
    */
    EntityNode::Ptr createEntity (int sceneIndex = -1) const;

    /** Returns the number of scenes in the asset. */
    int getNumScenes() const noexcept;

    //==============================================================================
    /** Returns the meshes, indexed like the glTF meshes. */
    const std::vector<Mesh::Ptr>& getMeshes() const noexcept;

    /** Returns the materials, indexed like the glTF materials. */
    const std::vector<Material::Ptr>& getMaterials() const noexcept;

    /** Returns the textures that materials use, in the order they were first used. */
    const std::vector<Texture::Ptr>& getTextures() const noexcept;

    /** Returns the problems found while loading that didn't prevent it. */
    const StringArray& getWarnings() const noexcept;

private:
    struct Data;
    class Loader;

    std::shared_ptr<const Data> data;
};

} // namespace yup
