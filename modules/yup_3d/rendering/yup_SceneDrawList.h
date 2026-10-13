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
    every MeshNode slot becomes a DrawItem, every LightNode slot becomes a Light, and every
    ReflectionProbeNode slot becomes a ReflectionProbe, unless its influence is empty: a radius
    or an extent of 0 or less, or an entity scaled to 0. Every IrradianceVolumeNode slot becomes
    an IrradianceVolume, under the same conditions.

    Opaque and masked items are sorted by pipeline state (double-sided, then mirrored) and then
    by material, to limit state changes. Blended items are sorted back to front, so they are drawn over what is behind them.

    The items point into the meshes and materials of the tree: they stay valid until the tree
    changes or the list is built again.

    getLights() can hold a light that is not in the tree: when BuildOptions::addDefaultLight is
    set and the tree has no LightNode, it holds the default light owned by the list.

    With BuildOptions::selectLightsPerDraw, every item also lists the lights shading it: the
    maxLightsPerDraw most influential ones, faded out near the cutoff, see selectLights(), and
    the two reflection probes influencing it most, see selectReflectionProbes(), and the irradiance
    volume lighting it, see selectIrradianceVolume().

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
        std::array<float, maxLightsPerDraw> lightWeights = [] { std::array<float, maxLightsPerDraw> ones; ones.fill (1.0f); return ones; }(); ///< What scales the intensity of each of lightIndices, see selectLights().
        int numLights = 0;                           ///< How many lightIndices are valid.
        std::array<int, 2> reflectionProbes { -1, -1 }; ///< Indices into getReflectionProbes(), the most influential first, -1 when unused, see BuildOptions::selectLightsPerDraw.
        int irradianceVolume = -1;                   ///< Index into getIrradianceVolumes(), -1 for none, see BuildOptions::selectLightsPerDraw.
    };

    /** One light, in world space. */
    struct Light
    {
        const LightNode* node = nullptr;             ///< The light settings.
        Vector3<float> position;                     ///< World position of the entity.
        Vector3<float> direction;                    ///< Unit direction the light shines towards, the entity -Z.
    };

    /** One reflection probe, in world space. */
    struct ReflectionProbe
    {
        ReflectionProbeNode* node = nullptr; ///< The probe settings.
        Matrix4 world;                       ///< Entity to world.
        Vector3<float> position;             ///< Capture position, the entity origin in world space.
        BoundingBox bounds;                  ///< World-space bounds of the influence volume, exact for a sphere under any rotation and scale.
    };

    /** One irradiance volume, in world space. */
    struct IrradianceVolume
    {
        IrradianceVolumeNode* node = nullptr; ///< The volume settings.
        Matrix4 world;                        ///< Entity to world.
        BoundingBox bounds;                   ///< World-space bounds of the box.
    };

    /** Options for build(). */
    struct BuildOptions
    {
        /** Adds a light owned by the list when the tree has no LightNode, see Scene::isUsingDefaultLight():
            a white directional light of intensity 3, shining towards (-0.35, -0.75, -0.55), that casts no shadows.
        */
        bool addDefaultLight = false;

        /** Fills DrawItem::lightIndices, DrawItem::lightWeights and DrawItem::numLights of every item with selectLights(),
            DrawItem::reflectionProbes with selectReflectionProbes() and DrawItem::irradianceVolume with selectIrradianceVolume().
        */
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

    /** Returns the reflection probes. */
    Span<const ReflectionProbe> getReflectionProbes() const noexcept { return reflectionProbes; }

    /** Returns the irradiance volumes. */
    Span<const IrradianceVolume> getIrradianceVolumes() const noexcept { return irradianceVolumes; }

    /** Returns the material used by primitives without a material and without an override. */
    const Material& getDefaultMaterial() const noexcept { return *defaultMaterial; }

    //==============================================================================
    /** Picks up to maxLightsPerDraw lights for an object with the given world bounds, by influence.

        Directional lights come first, by intensity times color luminance. Then come the point and
        spot lights whose effective range (GpuSceneData::getEffectiveRange()) reaches the bounds, by
        intensity * luminance / max (squared distance to the bounds, radius^2 + 1e-4). A light with
        a range is also scaled by the window of the shaders, 1 - (distance / range)^4, so it ranks
        0 at the edge of its range and enters or leaves the selection there without a jump. Dark lights
        of every type (GpuSceneData::isDark()) are skipped, and so are spot lights whose cone,
        widened to a sphere bound, misses the bounds. Ties keep the lower index.

        When more lights qualify than fit, build() also gives each picked light a weight that
        scales its intensity (DrawItem::lightWeights), so a light crossing the cutoff fades instead of switching on or off
        within a frame: smoothstep (c, 2 c, rank), where c is the rank of the first light left
        out. A light at least twice as influential as that one keeps its full intensity, and so
        does every light when at most maxLightsPerDraw qualify, or a directional light when the
        light left out is not directional.

        @param worldBounds  The bounds of the object, in world space.
        @param lights       The lights to pick from.
        @param indices      Receives the indices into @p lights, most influential first.

        @returns How many indices were written.
    */
    static int selectLights (const BoundingBox& worldBounds, Span<const Light> lights, std::array<uint16, maxLightsPerDraw>& indices);

    /** Picks the two probes that take priority over @p itemBounds, the first one first.

        A probe is a candidate when its world bounds intersect @p itemBounds; an empty box
        intersects nothing. Candidates are ranked by the volume of their world bounds, smallest
        first, so a probe nested inside a larger one takes priority where both reach. Equal
        volumes are ranked by the distance from the item center to the probe position (the center
        of its influence), then by bake id, oldest first, and full ties keep the lower index.

        The shaders blend in that order: the first probe by its weight w0, the second by
        (1 - w0) * w1, the environment takes the rest. Only two probes are kept, so a draw moving
        until a third probe outranks the second switches between them abruptly.

        @param probes      The probes to pick from.
        @param itemBounds  The bounds of the object, in world space.

        @returns The indices into @p probes, the nearest first. Unused entries are -1.
    */
    static std::array<int, 2> selectReflectionProbes (Span<const ReflectionProbe> probes, const BoundingBox& itemBounds) noexcept;

    /** Picks the irradiance volume lighting an object: the smallest one whose world bounds contain
        the center of @p itemBounds, by the volume of those bounds. Ties keep the lower index.

        The raster tier lights each draw with the volume of its center, so an object spanning two
        volumes takes its light from one of them; the full tier picks per pixel.

        @param volumes     The volumes to pick from.
        @param itemBounds  The bounds of the object, in world space.

        @returns The index into @p volumes, or -1 when none contains the center or the bounds are empty.
    */
    static int selectIrradianceVolume (Span<const IrradianceVolume> volumes, const BoundingBox& itemBounds) noexcept;

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
    static int selectLights (const BoundingBox& worldBounds, Span<const Light> lights, Span<const LightRanking> rankings,
                             std::array<uint16, maxLightsPerDraw>& indices, std::array<float, maxLightsPerDraw>& weights);

    void collect (const EntityNode& entity, const Matrix4& world, const Material* overrideMaterial, const Matrix4& view);

    std::vector<DrawItem> opaqueItems;
    std::vector<DrawItem> blendItems;
    std::vector<Light> lights;
    std::vector<LightRanking> lightRankings;
    std::vector<ReflectionProbe> reflectionProbes;
    std::vector<IrradianceVolume> irradianceVolumes;
    Material::Ptr defaultMaterial;
    std::unique_ptr<LightNode> defaultLight;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneDrawList)
};

} // namespace yup
