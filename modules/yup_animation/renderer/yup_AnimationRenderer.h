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
/** Stateless renderer that traverses an AnimationComposition and draws it
    using yup::Graphics primitives.

    All methods are static - there is no per-instance state.

    The rendering pipeline:
    1. Build a parent-transform map from layer parentId chains.
    2. Iterate layers in Lottie order (back-to-front render order).
    3. Per layer: check visibility, push Graphics state, apply transform + opacity.
    4. ShapeLayer: iterate groups, accumulate paths from shapes, apply modifiers,
       then apply paints (fill / stroke).
    5. Layer masks applied as clip regions before layer content.
*/
class YUP_API AnimationRenderer
{
public:
    //==============================================================================
    /** Renders all visible layers of @p comp at @p frameNo into @p g.

        The composition is scaled and positioned inside @p bounds according to
        @p fitting and @p justification, mirroring the semantics used by
        Drawable::paint. Content that falls outside the composition viewport is
        clipped to the fitted composition rectangle intersected with @p bounds.
    */
    static void renderComposition (Graphics& g,
                                   const AnimationComposition& comp,
                                   float frameNo,
                                   Rectangle<float> bounds,
                                   Fitting fitting = Fitting::scaleToFit,
                                   Justification justification = Justification::center);

private:
    static void renderComposition (Graphics& g,
                                   const AnimationComposition& comp,
                                   float frameNo,
                                   Rectangle<float> bounds,
                                   Fitting fitting,
                                   Justification justification,
                                   float opacity,
                                   std::optional<Color> paintOverride = std::nullopt);

    static AffineTransform calculateViewTransform (Size<float> compSize,
                                                   Rectangle<float> targetArea,
                                                   Fitting fitting,
                                                   Justification justification);

    //==============================================================================
    // Shared per-frame scene data (avoided per-layer deep copy)
    struct SceneContext
    {
        const AnimationComposition& comp;
        float frameNo;
        Size<float> compSize;

        /** Maps layer id → accumulated world-space transform (parents resolved). */
        HashMap<int, AffineTransform> parentTransforms;

        void buildParentTransforms (const std::vector<AnimationLayer::Ptr>& layers);
    };

    // Precomp texture cache — renders a precomp asset once per frame and reuses
    // the resulting GPU texture for subsequent instances of the same asset.
    struct PrecompCache
    {
        HashMap<String, GpuTexture::Ptr> textures;
    };

    // Per-layer context (cheap to copy — no heap allocations)
    struct RenderContext
    {
        const SceneContext& scene;
        AffineTransform viewTransform; ///< composition-space → screen-space
        float opacity = 1.0f;
        std::optional<Color> paintOverride;
        PrecompCache* precompCache = nullptr; ///< Owned by the outermost renderComposition call.

        AffineTransform resolveLayerTransform (const AnimationLayer& layer) const;
    };

    //==============================================================================
    static void renderLayerList (Graphics& g,
                                 const std::vector<AnimationLayer::Ptr>& layers,
                                 const RenderContext& ctx);
    static void renderLayer (Graphics& g,
                             const AnimationLayer& layer,
                             const RenderContext& ctx,
                             const AnimationLayer* matteSource = nullptr);
    static void renderLayerDirect (Graphics& g,
                                   const AnimationLayer& layer,
                                   const RenderContext& ctx,
                                   const AnimationLayer* matteSource,
                                   float opacity);
    static bool renderLayerIsolated (Graphics& g,
                                     const AnimationLayer& layer,
                                     const RenderContext& ctx,
                                     const AnimationLayer* matteSource,
                                     float opacity);
    static void renderDropShadow (Graphics& g, const AnimationLayer& layer, const RenderContext& ctx, float opacity);
    static void renderLayerContent (Graphics& g, const AnimationLayer& layer, const RenderContext& ctx, float opacity);
    static void renderShapeLayer (Graphics& g, const ShapeLayer& layer, const RenderContext& ctx, float opacity);
    static void renderSolidLayer (Graphics& g, const SolidLayer& layer, const RenderContext& ctx, float opacity);
    static void renderImageLayer (Graphics& g, const ImageLayer& layer, const RenderContext& ctx, float opacity);
    static void renderPrecompLayer (Graphics& g, const PrecompLayer& layer, const RenderContext& ctx, float opacity);

    static bool applyMasks (Graphics& g, const AnimationLayer& layer, float frameNo, Size<float> compSize);
    static void applyMatteSourceClip (Graphics& g,
                                      const AnimationLayer& layer,
                                      const AnimationLayer& matteSource,
                                      const RenderContext& ctx,
                                      bool inverted);

    struct ClipPathResult
    {
        Path path;
        bool active = false;
    };

    static ClipPathResult buildLayerMaskClipPath (const AnimationLayer& layer, float frameNo, Size<float> compSize);

    static void renderGroup (Graphics& g,
                             const AnimationGroup& group,
                             const RenderContext& ctx,
                             float opacity,
                             const AnimationRoundedCorner* parentRoundedCorner = nullptr);

    static void applyTrim (Path& path, const AnimationTrim& trim, float frameNo);
    static void applyTrimIndividually (std::vector<Path>& paths, const AnimationTrim& trim, float frameNo);

    static void applyFill (Graphics& g, const Path& path, const FillPaint& fill, const RenderContext& ctx, float opacity);
    static void applyStroke (Graphics& g, const Path& path, const StrokePaint& stroke, const RenderContext& ctx, float opacity);
};

} // namespace yup
