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
/** A flat, sorted list of everything to draw in an entity tree, built on the CPU.

    build() walks the tree once, depth-first, accumulating world matrices and the material
    override of the nearest MaterialNode, and skipping hidden subtrees. Every primitive of
    every MeshNode slot becomes a DrawItem, and every LightNode slot becomes a Light.

    Opaque and masked items are sorted by pipeline state (double-sided, then mirrored) and then
    by material, to limit state changes. Blended items are sorted back to front, so they are drawn over what is behind them.

    The items point into the meshes and materials of the tree: they stay valid until the tree
    changes or the list is built again.

    @see SceneRenderer
*/
class YUP_API SceneDrawList
{
public:
    //==============================================================================
    /** One primitive to draw. */
    struct DrawItem
    {
        Matrix4 world;                               ///< Model to world transform.
        Mesh* mesh = nullptr;                        ///< The mesh owning the primitive.
        int primitiveIndex = 0;                      ///< The primitive within the mesh.
        const Mesh::Primitive* primitive = nullptr;  ///< The primitive.
        const Material* material = nullptr;          ///< The resolved material, never nullptr.
        float viewDistance = 0.0f;                   ///< Distance of the bounds center along the view direction.
        bool mirrored = false;                       ///< True if the world transform mirrors, which reverses the winding.
    };

    /** One light, in world space. */
    struct Light
    {
        const LightNode* node = nullptr;             ///< The light settings.
        Vector3<float> position;                     ///< World position of the entity.
        Vector3<float> direction;                    ///< Unit direction the light shines towards, the entity -Z.
    };

    //==============================================================================
    /** Creates an empty list. */
    SceneDrawList();

    /** Destructor. */
    ~SceneDrawList();

    //==============================================================================
    /** Rebuilds the list from an entity tree.

        @param root  The root of the tree. Its own world matrix is used as the starting point.
        @param view  The view matrix, used to sort blended items by distance.
    */
    void build (const EntityNode& root, const Matrix4& view);

    /** Empties the list. */
    void clear();

    //==============================================================================
    /** Returns the opaque and masked items, sorted by pipeline state and material. */
    Span<const DrawItem> getOpaqueItems() const noexcept { return opaqueItems; }

    /** Returns the blended items, sorted from the farthest to the nearest. */
    Span<const DrawItem> getBlendItems() const noexcept { return blendItems; }

    /** Returns the lights. */
    Span<const Light> getLights() const noexcept { return lights; }

    /** Returns the material used by primitives without a material and without an override. */
    const Material& getDefaultMaterial() const noexcept { return *defaultMaterial; }

private:
    void collect (const EntityNode& entity, const Matrix4& world, const Material* overrideMaterial, const Matrix4& view);

    std::vector<DrawItem> opaqueItems;
    std::vector<DrawItem> blendItems;
    std::vector<Light> lights;
    Material::Ptr defaultMaterial;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneDrawList)
};

} // namespace yup
