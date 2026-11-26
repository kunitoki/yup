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
/** A glTF 2.0 asset converted into YUP meshes, materials, textures, skins, animation clips and
    entity templates.

    Load a .gltf or .glb file once, then instantiate it as many times as needed with
    instantiate(): every call builds a fresh entity tree in a Scene, and all the trees share
    the same meshes, materials, textures, skins and animation clips, also across scenes. Copies
    of a GltfModel are cheap and share them too. Each glTF scene is turned into a Prefab once,
    when loading, and getPrefab() returns it.

    @code
    auto model = yup::GltfModel::loadFromFile (file);
    if (model.wasOk())
    {
        yup::Entity first = model.getReference().instantiate (*scene);
        yup::Entity second = model.getReference().instantiate (*scene, first);
    }
    @endcode

    What is converted:
    - triangle primitives with POSITION, NORMAL, TANGENT, TEXCOORD_0 and COLOR_0, dense or
      sparse, any component type, normalized integers included;
    - metallic-roughness materials with their textures, alpha modes and double-sided flag, plus
      the factors of KHR_materials_emissive_strength, KHR_materials_ior, KHR_materials_specular,
      KHR_materials_clearcoat and KHR_materials_sheen (the textures of these extensions are
      ignored, with a warning);
    - images from buffer views, data URIs and external files, decoded with the image formats
      the application links (EXT_texture_webp sources need libwebp);
    - nodes with their transforms, meshes, cameras and KHR_lights_punctual lights;
    - skins, each as a Skin, with the JOINTS_0 and WEIGHTS_0 of the primitives they move, the
      weights of a vertex being made to add up to 1;
    - morph targets, as the POSITION, NORMAL and TANGENT deltas of the primitives, with the
      weights the mesh gives them and the ones a node replaces those with, and the names
      exporters write to the targetNames of the extras of a mesh, see Mesh::findMorphTarget();
    - animations, each as an AnimationClip whose tracks animate the translation, the rotation,
      the scale or the morph target weights of a node, with step, linear or cubic spline
      interpolation. An asset that has animations gets an AnimationNode on the root entity of
      each of its instances, which plays the clips on the entities of that instance once asked.

    A node whose mesh is skinned, or has morph targets that move it, gets a DeformNode in every
    instance. It draws a copy of the mesh of its own, which it deforms on the CPU: the morph
    targets by their weights, then the skin, following the entities its joints became in that
    instance. So instances deform apart, and the meshes getMeshes() returns stay in their bind
    pose. An instance shows the pose its nodes have from the start, and follows them whenever
    its scene is updated, see DeformNode.

    What is not: a vertex follows the four joints of JOINTS_0 at most, so JOINTS_1 and the
    further ones are ignored, and so are the morph targets of texture coordinates and colors,
    each with a warning. A skin that can't move its mesh, by a joint the skin doesn't have for
    example, leaves the mesh as it is, with a warning too. A file that requires an extension
    that is not supported fails to load, and one that only uses such an extension loads without
    what it changes, with a warning naming it.

    Files are not trusted. External files are only read from the base directory and its
    subdirectories, and what a model keeps is bounded by its size: 2^26 values of what its
    accessors hold, about 256 MB, or 8 for each byte of its buffers when that is more, a vertex
    counting for the 16 it takes. The copy of a mesh that each node deforming it takes in an
    instance counts too, and so do the scenes a file has besides the one a model shows when
    none is asked for, the more so when they hold the nodes of another scene. The primitives,
    skins, morph targets, scenes, deformed nodes and animation channels that would take a model
    over are left out, each with a warning: such a node draws its mesh as it is, and such a
    scene has nothing to instantiate. The scene a model shows when none is asked for is never
    one of them. Problems that don't prevent loading, like those, a primitive that isn't made
    of triangles, an image that can't be decoded or an animation channel that can't be played,
    are collected in getWarnings().

    @see Prefab, Entity, Mesh, Material, Skin, DeformNode, AnimationClip, AnimationNode
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
    /** Builds a new entity tree in a Scene for a scene of the asset.

        The returned entity is named after the glTF scene and holds its root nodes. Every node
        becomes an Entity with a MeshNode, CameraNode or LightNode attached as needed, and with a
        DeformNode drawing a copy of the mesh, already deformed, when a skin or morph targets
        move the mesh. When the asset has animations the returned entity also gets an
        AnimationNode, paused.

        @param scene      The scene to create the entities in.
        @param parent     The entity to add the tree to, or an invalid entity for the root.
        @param sceneIndex The glTF scene to build, or -1 for the default one. An asset
                          without scenes builds every node that has no parent.

        @return The new entity, or an invalid entity if the scene index is out of range, or if
                the scene is one of the others that were left out of a file asking for more than
                a model can keep: see getPrefab().
    */
    Entity instantiate (Scene& scene, Entity parent = {}, int sceneIndex = -1) const;

    /** Returns the prefab instantiate() builds a scene of the asset from.

        @param sceneIndex The glTF scene, or -1 for the default one. An asset without scenes has
                          a single prefab holding every node that has no parent.

        @return The prefab, which lives as long as any copy of this model, or nullptr if the
                scene index is out of range, or for a scene that was left out, with a warning,
                because it would take the model over what it can keep. The default scene, which
                is the first one when the file names none, is never left out.
    */
    const Prefab* getPrefab (int sceneIndex = -1) const noexcept;

    /** Returns the number of scenes in the asset, the ones that were left out included: a scene
        keeps the index it has in the file.
    */
    int getNumScenes() const noexcept;

    //==============================================================================
    /** Returns the meshes, indexed like the glTF meshes, in their bind pose: the instances of a
        mesh that deforms draw copies of it, see DeformNode.
    */
    const std::vector<Mesh::Ptr>& getMeshes() const noexcept;

    /** Returns the materials, indexed like the glTF materials. */
    const std::vector<Material::Ptr>& getMaterials() const noexcept;

    /** Returns the textures that materials use, in the order they were first used. */
    const std::vector<Texture::Ptr>& getTextures() const noexcept;

    /** Returns the animation clips, indexed like the glTF animations. An animation left without
        a channel that can be played has no clip: it keeps its place, as a nullptr.
    */
    const std::vector<AnimationClip::Ptr>& getAnimations() const noexcept;

    /** Returns the skins, indexed like the glTF skins. A skin that can't be used, by its
        inverse bind matrices for example, keeps its place, as a nullptr, and a warning tells
        why.

        The joints of a skin are the indices of glTF nodes: an instance finds the entities
        they became in the DeformNode of each entity drawing a skinned mesh.
    */
    const std::vector<Skin::Ptr>& getSkins() const noexcept;

    /** Returns the problems found while loading that didn't prevent it.

        A model keeps 200 of them at most, the last one then telling that more were left out.
    */
    const StringArray& getWarnings() const noexcept;

private:
    struct Data;
    class Loader;

    std::shared_ptr<const Data> data;
};

} // namespace yup
