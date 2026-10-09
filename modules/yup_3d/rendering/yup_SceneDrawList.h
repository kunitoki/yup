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

    getLights() can hold a light that is not in the tree: when BuildOptions::addDefaultLight is
    set and the tree has no LightNode, it holds the default light owned by the list.

    With BuildOptions::selectLightsPerDraw, every item also lists the lights shading it: the
    maxLightsPerDraw most influential ones, see selectLights().

    @see SceneRenderer
*/
class YUP_API SceneDrawList
{
public:
    //==============================================================================
    /** The most lights shading one draw item. */
    static constexpr int maxLightsPerDraw = 8;

    //==============================================================================
    /** One primitive to draw. */
    struct DrawItem
    {
        Matrix4 world;                               ///< Model to world transform.
        Mesh* mesh = nullptr;                        ///< The mesh owning the primitive.
        MeshNode* node = nullptr;                    ///< The part drawing the mesh, which keeps its previous transform.
        int primitiveIndex = 0;                      ///< The primitive within the mesh.
        const Mesh::Primitive* primitive = nullptr;  ///< The primitive.
        const Material* material = nullptr;          ///< The resolved material, never nullptr.
        float viewDistance = 0.0f;                   ///< Distance of the bounds center along the view direction.
        bool mirrored = false;                       ///< True if the world transform mirrors, which reverses the winding.
        std::array<uint16, maxLightsPerDraw> lightIndices {}; ///< Indices into getLights(), most influential first, see BuildOptions::selectLightsPerDraw.
        int numLights = 0;                           ///< How many lightIndices are valid.
    };

    /** One light, in world space. */
    struct Light
    {
        const LightNode* node = nullptr;             ///< The light settings.
        Vector3<float> position;                     ///< World position of the entity.
        Vector3<float> direction;                    ///< Unit direction the light shines towards, the entity -Z.
    };

    /** Options for build(). */
    struct BuildOptions
    {
        /** Adds a light owned by the list when the tree has no LightNode, see Scene::isUsingDefaultLight():
            a white directional light of intensity 3, shining towards (-0.35, -0.75, -0.55), that casts no shadows.
        */
        bool addDefaultLight = false;

        /** Fills DrawItem::lightIndices and DrawItem::numLights of every item with selectLights(). */
        bool selectLightsPerDraw = false;
    };

    //==============================================================================
    /** Creates an empty list. */
    SceneDrawList();

    /** Destructor. */
    ~SceneDrawList();

    //==============================================================================
    /** Rebuilds the list from an entity tree.

        @param root     The root of the tree. Its own world matrix is used as the starting point.
        @param view     The view matrix, used to sort blended items by distance.
        @param options  What to add beyond the tree.
    */
    void build (const EntityNode& root, const Matrix4& view, const BuildOptions& options);

    /** Rebuilds the list from an entity tree with the default BuildOptions. */
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

    //==============================================================================
    /** Picks up to maxLightsPerDraw lights for an object with the given world bounds, by influence.

        Directional lights come first, by intensity times color luminance. Then come the point and
        spot lights whose effective range (GpuSceneData::getEffectiveRange()) reaches the bounds, by
        intensity * luminance / max (squared distance to the bounds, radius^2 + 1e-4). Dark lights
        of every type (GpuSceneData::isDark()) are skipped, and so are spot lights whose cone,
        widened to a sphere bound, misses the bounds. Ties keep the lower index.

        @param worldBounds  The bounds of the object, in world space.
        @param lights       The lights to pick from.
        @param indices      Receives the indices into @p lights, most influential first.

        @returns How many indices were written.
    */
    static int selectLights (const BoundingBox& worldBounds, Span<const Light> lights, std::array<uint16, maxLightsPerDraw>& indices);

private:
    /** What selectLights() ranks a light by, computed once per light rather than per draw. */
    struct LightRanking
    {
        float power = 0.0f;    // intensity * color luminance
        float range = 0.0f;    // GpuSceneData::getEffectiveRange()
        float cosOuter = 0.0f; // the cosine of the outer cone angle of a spot light
        bool dark = true;      // GpuSceneData::isDark()
    };

    static void rankLights (Span<const Light> lights, std::vector<LightRanking>& rankings);
    static int selectLights (const BoundingBox& worldBounds, Span<const Light> lights, Span<const LightRanking> rankings, std::array<uint16, maxLightsPerDraw>& indices);

    void collect (const EntityNode& entity, const Matrix4& world, const Material* overrideMaterial, const Matrix4& view);

    std::vector<DrawItem> opaqueItems;
    std::vector<DrawItem> blendItems;
    std::vector<Light> lights;
    std::vector<LightRanking> lightRankings;
    Material::Ptr defaultMaterial;
    std::unique_ptr<LightNode> defaultLight;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneDrawList)
};

} // namespace yup
