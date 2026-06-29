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

    All methods are static — there is no per-instance state.

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

        The composition is scaled and positioned inside @p bounds. If
        @p keepAspectRatio is true the composition is letterboxed/pillarboxed.
    */
    static void renderComposition (Graphics& g,
                                   const AnimationComposition& comp,
                                   float frameNo,
                                   Rectangle<float> bounds,
                                   bool keepAspectRatio = true);

private:
    //==============================================================================
    // Per-render context built once per frame
    struct RenderContext
    {
        const AnimationComposition& comp;
        float frameNo;
        AffineTransform viewTransform; ///< composition-space → screen-space

        /** Maps layer id → accumulated world-space transform (parents resolved). */
        HashMap<int, AffineTransform> parentTransforms;

        void buildParentTransforms();
        AffineTransform resolveLayerTransform (const AnimationLayer& layer) const;
    };

    //==============================================================================
    static void renderLayer (Graphics& g, const AnimationLayer& layer, const RenderContext& ctx);
    static void renderShapeLayer (Graphics& g, const ShapeLayer& layer, const RenderContext& ctx, float opacity);
    static void renderSolidLayer (Graphics& g, const SolidLayer& layer, const RenderContext& ctx, float opacity);
    static void renderImageLayer (Graphics& g, const ImageLayer& layer, const RenderContext& ctx, float opacity);
    static void renderPrecompLayer (Graphics& g, const PrecompLayer& layer, const RenderContext& ctx, float opacity);

    static void applyMasks (Graphics& g, const AnimationLayer& layer, float frameNo);

    static void renderGroup (Graphics& g,
                             const AnimationGroup& group,
                             float frameNo,
                             float opacity);

    static void applyTrim (Path& path, const AnimationTrim& trim, float frameNo);

    static void applyFill (Graphics& g, const Path& path, const FillPaint& fill, float frameNo, float opacity);
    static void applyStroke (Graphics& g, const Path& path, const StrokePaint& stroke, float frameNo, float opacity);
};

} // namespace yup
