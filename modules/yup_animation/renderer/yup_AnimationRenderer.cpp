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

namespace
{

/** Depth caps for the two recursive walks over layer data. Both guard against
    cycles in malformed files rather than limiting any legitimate animation. */
constexpr int maxParentChainDepth = 32;
constexpr int maxPrecompDepth = 16;

/** Cache key for one precomp render: the asset, plus the position in its own
    timeline it is sampled at.

    Two layers can reference the same asset at different phases - their start
    frames differ - and those renders are not interchangeable, so the phase has to
    be part of the key. Quantised to a thousandth of a frame to keep the key an
    exact integer rather than a formatted float.
*/
String precompCacheKey (const String& precompRefId, float localFrame)
{
    return precompRefId + "|" + String (static_cast<int> (std::round (localFrame * 1000.0f)));
}

/** Counts how many layer instances draw each precomp render of @p comp, at
    @p frameNo for the given level.

    Only layers this frame actually draws are counted. A composition referenced
    by several layers whose visibility windows never overlap - the common
    "sequential slices of one scene" export - is drawn once per frame like any
    single-reference layer, and counting the invisible references too would put a
    full-size offscreen target back in its path for nothing.

    Each render is expanded once: a render asked for more than once is rasterized
    a single time and then blitted, so its contents are also drawn exactly once -
    which is what the count records.
*/
void countPrecompReferences (const AnimationComposition& comp,
                             const std::vector<AnimationLayer::Ptr>& layers,
                             float frameNo,
                             HashMap<String, int>& counts,
                             HashMap<String, int>& expandedRenders,
                             int depth)
{
    if (depth > maxPrecompDepth)
        return;

    for (const auto& layer : layers)
    {
        if (layer == nullptr || layer->hidden || layer->isMatteSource)
            continue;

        if (! layer->isVisibleAt (frameNo))
            continue;

        if (layer->getType() != AnimationLayer::Type::Precomp)
            continue;

        const auto& precompLayer = static_cast<const PrecompLayer&> (*layer);
        const auto localFrame = precompLayer.localFrame (frameNo, comp.frameRate);
        const auto renderKey = precompCacheKey (precompLayer.precompRefId, localFrame);

        const int previousCount = counts.contains (renderKey) ? counts[renderKey] : 0;
        counts.set (renderKey, previousCount + 1);

        if (expandedRenders.contains (renderKey))
            continue;

        expandedRenders.set (renderKey, 1);

        if (const auto* assetPtr = comp.assets.getPointer (precompLayer.precompRefId))
        {
            if (const auto* asset = assetPtr->get())
                countPrecompReferences (comp, asset->layers, localFrame, counts, expandedRenders, depth + 1);
        }
    }
}

/** Pushes @p clipPath as the clip in effect, interpreting it in the current
    transform. Callers must have saved the Graphics state, which owns the clip
    until it is restored. */
void pushClipPath (Graphics& g, const Path& clipPath)
{
    const auto savedTransform = g.getTransform();

    g.setTransform (AffineTransform::identity());
    g.setClipPath (clipPath);
    g.setTransform (savedTransform);
}

/** Culls everything drawn until the caller's saved Graphics state is restored. */
void pushEmptyClip (Graphics& g)
{
    pushClipPath (g, Path());
}

/** Clips @p clipRect, given in composition space, into the current transform and
    intersects it with the clip already in effect.

    Returns false when nothing drawn afterwards can be visible, which lets the
    caller skip its content outright.

    The intersection itself is left to the renderer, which resolves two
    overlapping rectangular clips with a bounds test. Running a path boolean op
    here instead produces the same region as a polygon with a redundant vertex
    per crossing, which the renderer then has to tessellate as a clip path.
*/
bool applyViewportClip (Graphics& g, Rectangle<float> clipRect)
{
    if (clipRect.getWidth() <= 0.0f || clipRect.getHeight() <= 0.0f)
    {
        pushEmptyClip (g);
        return false;
    }

    const auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());

    Path viewportClip;
    viewportClip.addRectangle (clipRect);
    auto transformedViewportClip = viewportClip.transformed (clipTransform);

    const auto viewportBounds = transformedViewportClip.getBounds();
    if (transformedViewportClip.isEmpty() || viewportBounds.getWidth() <= 0.0f || viewportBounds.getHeight() <= 0.0f)
    {
        pushEmptyClip (g);
        return false;
    }

    const auto currentClipPath = g.getClipPath();
    if (! currentClipPath.isEmpty())
    {
        const auto currentBounds = currentClipPath.getBounds();

        if (currentBounds.getWidth() <= 0.0f || currentBounds.getHeight() <= 0.0f
            || ! currentBounds.intersects (viewportBounds))
        {
            pushEmptyClip (g);
            return false;
        }

        if (viewportBounds.contains (currentBounds))
            return true;
    }

    pushClipPath (g, transformedViewportClip);
    return true;
}

/** Intersects the current clip with @p clipPath, given in the current transform,
    and pushes the result.

    Returns false when nothing drawn afterwards can be visible. When
    @p allowEmpty is false an empty @p clipPath means "no clip" and leaves the
    clip in effect untouched.
*/
bool applyClipPathInCurrentTransform (Graphics& g, const Path& clipPath, bool allowEmpty = false)
{
    if (clipPath.isEmpty() && ! allowEmpty)
        return true;

    const auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());
    auto transformedClipPath = clipPath.transformed (clipTransform);

    const auto clipBounds = transformedClipPath.getBounds();
    if (transformedClipPath.isEmpty() || clipBounds.getWidth() <= 0.0f || clipBounds.getHeight() <= 0.0f)
    {
        pushEmptyClip (g);
        return false;
    }

    const auto currentClipPath = g.getClipPath();
    if (! currentClipPath.isEmpty())
    {
        const auto currentBounds = currentClipPath.getBounds();

        if (currentBounds.getWidth() <= 0.0f || currentBounds.getHeight() <= 0.0f
            || ! currentBounds.intersects (clipBounds))
        {
            pushEmptyClip (g);
            return false;
        }

        transformedClipPath = currentClipPath.combinedWith (transformedClipPath, Path::BooleanOperation::Intersect);
    }

    if (transformedClipPath.isEmpty())
        return false;

    pushClipPath (g, transformedClipPath);
    return true;
}

/** Content bounds of @p layer in its own space, used to size clips and
    offscreen targets. */
Rectangle<float> getLayerContentBounds (const AnimationLayer& layer, Size<float> compSize)
{
    Size<float> size = compSize;

    switch (layer.getType())
    {
        case AnimationLayer::Type::Solid:
            size = static_cast<const SolidLayer&> (layer).layerSize;
            break;

        case AnimationLayer::Type::Image:
            if (const auto& image = static_cast<const ImageLayer&> (layer).image)
                size = { static_cast<float> (image->getWidth()), static_cast<float> (image->getHeight()) };
            break;

        case AnimationLayer::Type::Precomp:
            size = static_cast<const PrecompLayer&> (layer).layerSize;
            break;

        case AnimationLayer::Type::Shape:
        case AnimationLayer::Type::Text:
        case AnimationLayer::Type::Null:
            break;
    }

    if (size.getWidth() <= 0.0f || size.getHeight() <= 0.0f)
        size = compSize;

    return { 0.0f, 0.0f, size.getWidth(), size.getHeight() };
}

/** Screen-space bounds of everything @p layer can draw, or an empty rectangle
    when they cannot be derived cheaply.

    Only the layer types whose content is bounded by the layer's own box are
    reported. A shape layer draws wherever its geometry happens to be, and an
    enabled drop shadow adds a second draw offset from the content, so both fall
    back to the caller's conservative bounds.
*/
Rectangle<float> layerContentBoundsOnScreen (const AnimationLayer& layer,
                                             Size<float> compSize,
                                             const AffineTransform& layerToScreen)
{
    if (layer.dropShadow.has_value() && layer.dropShadow->enabled)
        return {};

    switch (layer.getType())
    {
        case AnimationLayer::Type::Solid:
        case AnimationLayer::Type::Image:
        case AnimationLayer::Type::Precomp:
            return getLayerContentBounds (layer, compSize).transformed (layerToScreen);

        case AnimationLayer::Type::Shape:
        case AnimationLayer::Type::Text:
        case AnimationLayer::Type::Null:
            break;
    }

    return {};
}

/** Opacities this close to fully opaque are treated as opaque. Exporters write
    values like 99 or 99.9 for layers meant to be seen at full strength, and
    isolating one behind a full-size offscreen composite to reproduce a sub-1%
    difference in alpha costs far more than the difference is worth. */
constexpr float opaqueOpacityThreshold = 0.999f;

/** Returns true when @p layer's opacity has to be applied by compositing the
    layer offscreen rather than by scaling each of its paints.

    A layer that fills or blits a single primitive already folds its opacity into
    that primitive's alpha, so compositing it offscreen would only reproduce the
    same pixels through a render target. Layers that draw several primitives which
    can overlap need the offscreen composite for correct group opacity, and an
    enabled drop shadow adds a second, overlapping draw to any layer type.
*/
bool needsTransparencyLayer (const AnimationLayer& layer)
{
    if (layer.dropShadow.has_value() && layer.dropShadow->enabled)
        return true;

    switch (layer.getType())
    {
        case AnimationLayer::Type::Shape:
        case AnimationLayer::Type::Precomp:
            return true;

        case AnimationLayer::Type::Solid:
        case AnimationLayer::Type::Image:
        case AnimationLayer::Type::Text:
        case AnimationLayer::Type::Null:
            return false;
    }

    return true;
}

} // namespace

//==============================================================================
// SceneContext

void AnimationRenderer::SceneContext::buildParentTransforms (const std::vector<AnimationLayer::Ptr>& layers)
{
    layersById.clear();

    for (const auto& layer : layers)
    {
        if (layer != nullptr)
            layersById.set (layer->id, layer.get());
    }

    parentTransforms.clear();

    for (const auto& layer : layers)
    {
        if (layer != nullptr)
            resolveWorldTransform (*layer, 0);
    }
}

const AffineTransform& AnimationRenderer::SceneContext::resolveWorldTransform (const AnimationLayer& layer, int depth)
{
    if (auto* resolved = parentTransforms.getPointer (layer.id))
        return *resolved;

    AffineTransform transform = layer.transform.toAffineTransform (frameNo);

    if (depth < maxParentChainDepth && layer.parentId >= 0 && layer.parentId != layer.id)
    {
        if (auto* parent = layersById.getPointer (layer.parentId))
            transform = transform.followedBy (resolveWorldTransform (**parent, depth + 1));
    }

    parentTransforms.set (layer.id, transform);

    return *parentTransforms.getPointer (layer.id);
}

//==============================================================================
// RenderContext

AffineTransform AnimationRenderer::RenderContext::resolveLayerTransform (const AnimationLayer& layer) const
{
    if (auto* entry = scene.parentTransforms.getPointer (layer.id))
        return entry->followedBy (viewTransform);
    return layer.transform.toAffineTransform (scene.frameNo).followedBy (viewTransform);
}

//==============================================================================
// AnimationRenderer

void AnimationRenderer::renderComposition (Graphics& g,
                                           const AnimationComposition& comp,
                                           float frameNo,
                                           Rectangle<float> bounds,
                                           Fitting fitting,
                                           Justification justification,
                                           AnimationRenderResources* renderResources)
{
    renderComposition (g, comp, frameNo, bounds, fitting, justification, 1.0f, std::nullopt, renderResources);
}

AffineTransform AnimationRenderer::calculateViewTransform (Size<float> compSize,
                                                           Rectangle<float> targetArea,
                                                           Fitting fitting,
                                                           Justification justification)
{
    float scaleX = targetArea.getWidth() / compSize.getWidth();
    float scaleY = targetArea.getHeight() / compSize.getHeight();

    switch (fitting)
    {
        case Fitting::none:
            scaleX = scaleY = 1.0f;
            break;

        case Fitting::scaleToFit:
            scaleX = scaleY = jmin (scaleX, scaleY);
            break;

        case Fitting::fitWidth:
            scaleY = scaleX;
            break;

        case Fitting::fitHeight:
            scaleX = scaleY;
            break;

        case Fitting::scaleToFill:
        case Fitting::centerCrop:
            scaleX = scaleY = jmax (scaleX, scaleY);
            break;

        case Fitting::fill:
            break;

        case Fitting::centerInside:
            scaleX = scaleY = jmin (1.0f, jmin (scaleX, scaleY));
            break;

        case Fitting::stretchWidth:
            scaleY = 1.0f;
            break;

        case Fitting::stretchHeight:
            scaleX = 1.0f;
            break;

        case Fitting::tile:
            scaleX = scaleY = 1.0f;
            break;
    }

    const float scaledWidth = compSize.getWidth() * scaleX;
    const float scaledHeight = compSize.getHeight() * scaleY;

    float offsetX = targetArea.getX();
    float offsetY = targetArea.getY();

    if (justification.testFlags (Justification::horizontalCenter))
        offsetX += (targetArea.getWidth() - scaledWidth) * 0.5f;
    else if (justification.testFlags (Justification::right))
        offsetX += targetArea.getWidth() - scaledWidth;

    if (justification.testFlags (Justification::verticalCenter))
        offsetY += (targetArea.getHeight() - scaledHeight) * 0.5f;
    else if (justification.testFlags (Justification::bottom))
        offsetY += targetArea.getHeight() - scaledHeight;

    return AffineTransform::scaling (scaleX, scaleY)
        .followedBy (AffineTransform::translation (offsetX, offsetY));
}

void AnimationRenderer::renderComposition (Graphics& g,
                                           const AnimationComposition& comp,
                                           float frameNo,
                                           Rectangle<float> bounds,
                                           Fitting fitting,
                                           Justification justification,
                                           float opacity,
                                           std::optional<Color> paintOverride,
                                           AnimationRenderResources* renderResources)
{
    const Size<float> compSize = comp.size;
    if (compSize.getWidth() <= 0.0f || compSize.getHeight() <= 0.0f)
        return;

    // Compute view transform: composition-space → screen-space
    const AffineTransform viewXf = calculateViewTransform (compSize, bounds, fitting, justification);

    // The composition viewport rectangle mapped to screen space. Content outside
    // it must be clipped, so shapes extending beyond the composition bounds don't
    // spill into the letterbox / pillarbox area of the target.
    const Rectangle<float> compRect (0.0f, 0.0f, compSize.getWidth(), compSize.getHeight());
    const Rectangle<float> fittedRect = compRect.transformed (viewXf);
    const Rectangle<float> clipRect = fittedRect.intersection (bounds);

    auto clipState = g.saveState();

    if (! applyViewportClip (g, clipRect))
        return;

    SceneContext sceneCtx { comp, frameNo, compSize };
    sceneCtx.buildParentTransforms (comp.layers);

    PrecompCache precompCache;
    {
        HashMap<String, int> expandedAssets;
        countPrecompReferences (comp, comp.layers, frameNo, precompCache.referenceCounts, expandedAssets, 0);
    }

    std::vector<AnimationRenderResources::MatteCanvasLease> matteLeases;
    RenderContext ctx { sceneCtx, viewXf, opacity, std::move (paintOverride), &precompCache, renderResources, &matteLeases };

    renderLayerList (g, comp.layers, ctx);
}

void AnimationRenderer::renderLayerList (Graphics& g,
                                         const std::vector<AnimationLayer::Ptr>& layers,
                                         const RenderContext& ctx)
{
    for (int i = (int) layers.size() - 1; i >= 0; --i)
    {
        const AnimationLayer* layer = layers[(size_t) i].get();
        if (layer == nullptr || layer->hidden)
            continue;

        if (layer->isMatteSource)
            continue;

        if (! layer->isVisibleAt (ctx.scene.frameNo))
            continue;

        const AnimationLayer* matteSource = nullptr;
        if (layer->matteType != AnimationLayer::MatteType::None && i >= 1)
        {
            const AnimationLayer* candidate = layers[(size_t) (i - 1)].get();
            if (candidate != nullptr && candidate->isMatteSource)
                matteSource = candidate;
        }

        renderLayer (g, *layer, ctx, matteSource);
    }
}

//==============================================================================

void AnimationRenderer::renderLayer (Graphics& g,
                                     const AnimationLayer& layer,
                                     const RenderContext& ctx,
                                     const AnimationLayer* matteSource)
{
    const float opacity = jmin (1.0f, ctx.opacity * layer.transform.opacityAt (ctx.scene.frameNo));

    if (opacity <= 0.0f)
        return;

    const float effectiveOpacity = opacity >= opaqueOpacityThreshold ? 1.0f : opacity;

    // Track mattes need the source's *rendered alpha* (including its fill opacity,
    // gradients, and anti-aliased edges), not just its silhouette. Composite the
    // source and target offscreen and multiply their alphas on the GPU. Falls
    // through to the geometric-clip path (applyMatteSourceClip) when the GPU is
    // unavailable (e.g. headless) or an offscreen target cannot be allocated.
    if (matteSource != nullptr
        && layer.matteType != AnimationLayer::MatteType::None
        && renderLayerWithMatte (g, layer, ctx, *matteSource, effectiveOpacity))
        return;

    if (effectiveOpacity < 1.0f
        && needsTransparencyLayer (layer)
        && renderLayerIsolated (g, layer, ctx, matteSource, effectiveOpacity))
        return;

    renderLayerDirect (g, layer, ctx, matteSource, effectiveOpacity);
}

void AnimationRenderer::renderLayerDirect (Graphics& g,
                                           const AnimationLayer& layer,
                                           const RenderContext& ctx,
                                           const AnimationLayer* matteSource,
                                           float opacity)
{
    auto saveState = g.saveState();

    // Apply layer transform
    const AffineTransform baseTransform = g.getTransform();
    const AffineTransform xf = ctx.resolveLayerTransform (layer);
    g.setTransform (xf.followedBy (baseTransform));

    if (! applyMasks (g, layer, ctx.scene.frameNo, ctx.scene.compSize))
        return;

    if (matteSource != nullptr
        && layer.matteType != AnimationLayer::MatteType::None)
    {
        // Geometric fallback (no GPU): clip the target to the matte source's
        // silhouette. This is a hard binary mask, so it cannot reproduce the
        // source's partial alpha / luma gradient - the GPU path in
        // renderLayerWithMatte does that. Inverted modes subtract the silhouette.
        const bool inverted = layer.matteType == AnimationLayer::MatteType::AlphaInv
                           || layer.matteType == AnimationLayer::MatteType::LumaInv;
        applyMatteSourceClip (g, layer, *matteSource, ctx, inverted);
    }

    RenderContext layerCtx = ctx;

    if (! layerCtx.paintOverride.has_value()
        && layer.fillEffect.has_value()
        && layer.fillEffect->enabled)
    {
        const float fillEffectOpacity = layer.fillEffect->opacityAt (ctx.scene.frameNo);
        if (fillEffectOpacity > 0.0f)
            layerCtx.paintOverride = layer.fillEffect->colorAt (ctx.scene.frameNo).withMultipliedAlpha (fillEffectOpacity);
    }

    if (! layerCtx.paintOverride.has_value()
        && layer.dropShadow.has_value()
        && layer.dropShadow->enabled)
    {
        renderDropShadow (g, layer, layerCtx, opacity);

        if (layer.dropShadow->shadowOnly)
            return;
    }

    renderLayerContent (g, layer, layerCtx, opacity);
}

bool AnimationRenderer::renderLayerIsolated (Graphics& g,
                                             const AnimationLayer& layer,
                                             const RenderContext& ctx,
                                             const AnimationLayer* matteSource,
                                             float opacity)
{
    // Allocate the transparency layer at the fitted (screen) resolution rather
    // than the composition resolution. Sizing the offscreen buffer to the
    // composition (e.g. 90x90) and then upscaling it to a larger target rasterizes
    // the layer at the small size and blurs it. Mapping the composition rectangle
    // through the view transform gives the on-screen size, so the layer is
    // rasterized at native resolution and composited back 1:1.
    const Rectangle<float> compRect (0.0f, 0.0f, ctx.scene.compSize.getWidth(), ctx.scene.compSize.getHeight());
    const Rectangle<float> fittedRect = compRect.transformed (ctx.viewTransform);

    auto targetArea = fittedRect;

    if (auto contentBounds = layerContentBoundsOnScreen (layer, ctx.scene.compSize, ctx.resolveLayerTransform (layer));
        ! contentBounds.isEmpty())
    {
        // Anything the layer draws is clipped to the composition viewport, so the
        // target never needs to reach past it.
        contentBounds = contentBounds.intersection (fittedRect);

        if (contentBounds.isEmpty())
            return false;

        targetArea = contentBounds;
    }

    auto transparencyLayer = g.beginTransparencyLayer (targetArea, opacity);
    if (! transparencyLayer.isValid())
        return false;

    const auto targetShift = targetArea.getTopLeft() - fittedRect.getTopLeft();

    RenderContext layerCtx = ctx;
    layerCtx.viewTransform = AffineTransform::scaling (ctx.viewTransform.getScaleX(), ctx.viewTransform.getScaleY())
                                 .followedBy (AffineTransform::translation (-targetShift.getX(), -targetShift.getY()));
    layerCtx.opacity = 1.0f;

    renderLayerDirect (transparencyLayer.getGraphics(), layer, layerCtx, matteSource, 1.0f);

    // The target area is already in screen space, so it composites back using the
    // parent's current transform without re-applying the view transform.
    return transparencyLayer.commit();
}

//==============================================================================

namespace
{

struct MatteParams
{
    float mode;
    float resX;
    float resY;
    float pad;
};

float matteModeValue (AnimationLayer::MatteType type) noexcept
{
    switch (type)
    {
        case AnimationLayer::MatteType::Alpha:
            return 0.0f;
        case AnimationLayer::MatteType::AlphaInv:
            return 1.0f;
        case AnimationLayer::MatteType::Luma:
            return 2.0f;
        case AnimationLayer::MatteType::LumaInv:
            return 3.0f;
        case AnimationLayer::MatteType::None:
            break;
    }

    return 0.0f;
}

} // namespace

bool AnimationRenderer::renderLayerWithMatte (Graphics& g,
                                              const AnimationLayer& layer,
                                              const RenderContext& ctx,
                                              const AnimationLayer& matteSource,
                                              float opacity)
{
    auto& context = g.getGraphicsContext();
    if (! context.isGpuAvailable())
        return false;

    // Rasterize at the fitted on-screen resolution so the composited result is
    // not upscaled (mirrors the transparency-layer / precomp sizing).
    const Rectangle<float> compRect (0.0f, 0.0f, ctx.scene.compSize.getWidth(), ctx.scene.compSize.getHeight());
    const Rectangle<float> fittedRect = compRect.transformed (ctx.viewTransform);

    const int w = static_cast<int> (std::ceil (fittedRect.getWidth()));
    const int h = static_cast<int> (std::ceil (fittedRect.getHeight()));
    if (w <= 0 || h <= 0)
        return false;

    // Reuse a caller-provided persistent pipeline when available (avoids a
    // per-frame shader recompile during playback); otherwise compile a temporary
    // for this call. Either way the pipeline is owned by a scope that ends before
    // the GraphicsContext - never by a static, whose destruction at process exit
    // would outlive the ore context and its leak detector.
    AnimationRenderResources localResources;
    AnimationRenderResources& resources = ctx.renderResources != nullptr ? *ctx.renderResources : localResources;

    auto canvases = resources.acquireMatteCanvases (context, w, h);
    if (! canvases.isValid())
        return false;

    auto pipeline = resources.getMattePipeline (context);
    if (pipeline == nullptr)
        return false;

    // Render offscreen in layer-local screen space: the fitted rectangle's
    // top-left is the canvas origin, so drop the view transform's translation and
    // keep only its scale.
    RenderContext offscreenCtx = ctx;
    offscreenCtx.viewTransform = AffineTransform::scaling (ctx.viewTransform.getScaleX(), ctx.viewTransform.getScaleY());
    offscreenCtx.opacity = 1.0f;

    // Only one offscreen 2D frame may be open at a time, so each canvas is fully
    // drawn and committed (via asTexture()) before the next is opened.

    // 1. Matte target (the layer being masked) into targetCanvas.
    GpuTexture::Ptr targetTex;
    {
        auto& tg = canvases.getTargetCanvas().beginDraw();
        renderLayerDirect (tg, layer, offscreenCtx, nullptr, 1.0f);
        targetTex = canvases.getTargetCanvas().asTexture();
    }

    // 2. Matte source (defines the mask) into sourceCanvas.
    GpuTexture::Ptr sourceTex;
    {
        auto& sg = canvases.getSourceCanvas().beginDraw();
        renderLayerDirect (sg, matteSource, offscreenCtx, nullptr, 1.0f);
        sourceTex = canvases.getSourceCanvas().asTexture();
    }

    if (targetTex == nullptr || sourceTex == nullptr)
        return false;

    // 3. Composite target * coverage(source) into resultCanvas.
    //
    // The result canvas is written *only* by this pass: it never goes through
    // beginDraw(), so nothing else clears it, and its backing texture is allocated
    // uninitialized. Compositing it after a failed encode therefore blits undefined
    // GPU memory over the whole layer, which reads as a flash of an arbitrary color.
    // Every step is checked so a failure falls through to the geometric-clip path
    // instead.
    {
        MatteParams params { matteModeValue (layer.matteType), (float) w, (float) h, 0.0f };

        auto frame = GpuFrame::begin (context.getGpuDevice());
        if (! frame.isValid())
            return false;

        auto pass = canvases.getResultCanvas().beginRenderPass (frame, { true, Colors::transparentBlack });
        if (! pass.isValid())
            return false;

        pass.setPipeline (pipeline);
        pass.setTexture (0, 0, targetTex);
        pass.setTexture (0, 1, sourceTex);
        pass.setUniformBuffer (0, 3, &params, sizeof (params));

        if (! pass.draw (3))
            return false;

        if (! pass.finish())
            return false;

        if (! frame.submit())
            return false;

        // Leaving this scope waits for the GPU before releasing the views, uniform
        // buffer and sampler the pass references by raw pointer, so the result
        // canvas is complete before it is sampled below.
    }

    auto resultTex = canvases.getResultCanvas().asTexture();
    if (resultTex == nullptr)
        return false;

    // 4. Composite the matted result back into the main graphics at layer opacity.
    auto saveState = g.saveState();
    g.setOpacity (g.getOpacity() * opacity);
    g.drawTexture (resultTex, fittedRect);

    // drawTexture only queues a reference to resultTex - the enclosing frame reads
    // it at flush time, after this function has returned. Keep the lease alive for
    // the rest of the composition render so the pool cannot hand these canvases to
    // another matte layer, which would overwrite the pixels just queued. Every
    // matte in a composition is sized to the same fitted rectangle, so without this
    // the pool reuses one canvas triple for all of them and only the last matte
    // survives (e.g. world_locations.json's four matted dots collapse to one).
    if (ctx.matteLeases != nullptr)
        ctx.matteLeases->push_back (std::move (canvases));

    return true;
}

void AnimationRenderer::renderLayerContent (Graphics& g, const AnimationLayer& layer, const RenderContext& ctx, float opacity)
{
    switch (layer.getType())
    {
        case AnimationLayer::Type::Shape:
            renderShapeLayer (g, static_cast<const ShapeLayer&> (layer), ctx, opacity);
            break;

        case AnimationLayer::Type::Solid:
            renderSolidLayer (g, static_cast<const SolidLayer&> (layer), ctx, opacity);
            break;

        case AnimationLayer::Type::Image:
            renderImageLayer (g, static_cast<const ImageLayer&> (layer), ctx, opacity);
            break;

        case AnimationLayer::Type::Precomp:
            renderPrecompLayer (g, static_cast<const PrecompLayer&> (layer), ctx, opacity);
            break;

        case AnimationLayer::Type::Text:
            break; // Text layers are not implemented yet

        case AnimationLayer::Type::Null:
            break; // Null layers have no visual output
    }
}

void AnimationRenderer::renderShapeLayer (Graphics& g, const ShapeLayer& layer, const RenderContext& ctx, float opacity)
{
    // Groups are rendered back-to-front (last group first in Lottie order)
    for (int i = (int) layer.groups.size() - 1; i >= 0; --i)
    {
        const AnimationGroup* group = layer.groups[(size_t) i].get();
        if (group == nullptr || group->hidden)
            continue;

        renderGroup (g, *group, ctx, opacity);
    }
}

void AnimationRenderer::renderSolidLayer (Graphics& g, const SolidLayer& layer, const RenderContext& ctx, float opacity)
{
    const Color fillColor = ctx.paintOverride.value_or (layer.solidColor);
    g.setFillColor (fillColor.withMultipliedAlpha (opacity));
    g.fillRect (Rectangle<float> (0.0f, 0.0f, layer.layerSize.getWidth(), layer.layerSize.getHeight()));
}

void AnimationRenderer::renderImageLayer (Graphics& g, const ImageLayer& layer, const RenderContext& ctx, float opacity)
{
    (void) ctx;

    if (! layer.image.has_value())
        return;

    g.setOpacity (g.getOpacity() * opacity);
    g.drawImage (*layer.image, { 0.0f, 0.0f, (float) layer.image->getWidth(), (float) layer.image->getHeight() });
}

void AnimationRenderer::renderPrecompLayer (Graphics& g, const PrecompLayer& layer, const RenderContext& ctx, float opacity)
{
    const auto* assetPtr = ctx.scene.comp.assets.getPointer (layer.precompRefId);
    const AnimationAsset* asset = assetPtr ? assetPtr->get() : nullptr;
    if (asset == nullptr || asset->layers.empty())
        return;

    const float localFrame = layer.localFrame (ctx.scene.frameNo, ctx.scene.comp.frameRate);
    const Rectangle<float> precompBounds (0.0f, 0.0f, layer.layerSize.getWidth(), layer.layerSize.getHeight());

    auto clipState = g.saveState();
    if (! applyViewportClip (g, precompBounds))
        return;

    // The asset's content depends on the phase it is sampled at, so the cache is
    // keyed by asset *and* phase. A phase asked for only once is drawn straight
    // into the parent: opening a target for it would buy nothing, since no other
    // reference wants that render.
    const auto renderKey = precompCacheKey (layer.precompRefId, localFrame);

    const bool needsOffscreen = ctx.precompCache != nullptr
                             && (opacity < 1.0f || ctx.precompCache->isSharedAsset (renderKey));

    SceneContext precompScene { ctx.scene.comp, localFrame, layer.layerSize };

    if (needsOffscreen)
    {
        if (auto* cached = ctx.precompCache->textures.getPointer (renderKey))
        {
            g.setOpacity (g.getOpacity() * opacity);
            g.drawTexture (*cached, precompBounds);
            return;
        }

        // Size the offscreen target to the on-screen device resolution so the
        // cached texture is not upscaled (which would lose quality). The current
        // graphics transform maps layer space to device pixels, so its scale
        // factor tells us how many device pixels each layer unit occupies.
        const float deviceScale = jlimit (1.0f, 8.0f, g.getTransform().getScaleFactor());

        const int w = static_cast<int> (std::ceil (layer.layerSize.getWidth() * deviceScale));
        const int h = static_cast<int> (std::ceil (layer.layerSize.getHeight() * deviceScale));

        if (w > 0 && h > 0)
        {
            // Keyed by render rather than by asset: two phases of one asset are
            // two different images, and sharing a canvas would clobber the first.
            auto canvas = ctx.renderResources != nullptr
                            ? ctx.renderResources->getPrecompCanvas (g.getGraphicsContext(), renderKey, w, h)
                            : GpuCanvas::create (g.getGraphicsContext(), w, h);
            if (canvas != nullptr)
            {
                {
                    auto& offscreenG = canvas->beginDraw();

                    precompScene.buildParentTransforms (asset->layers);

                    RenderContext offscreenCtx { precompScene, AffineTransform::scaling (deviceScale), 1.0f, ctx.paintOverride, ctx.precompCache, ctx.renderResources, ctx.matteLeases };
                    renderLayerList (offscreenG, asset->layers, offscreenCtx);
                }

                auto tex = canvas->asTexture();
                if (tex != nullptr)
                {
                    ctx.precompCache->textures.set (renderKey, tex);

                    g.setOpacity (g.getOpacity() * opacity);
                    g.drawTexture (tex, precompBounds);
                    return;
                }
            }
        }
    }

    const float scaleX = precompBounds.getWidth() / layer.layerSize.getWidth();
    const float scaleY = precompBounds.getHeight() / layer.layerSize.getHeight();
    const AffineTransform precompViewXf = AffineTransform::scaling (scaleX, scaleY)
                                              .followedBy (AffineTransform::translation (precompBounds.getX(), precompBounds.getY()));

    precompScene.buildParentTransforms (asset->layers);

    RenderContext precompCtx { precompScene, precompViewXf, opacity, ctx.paintOverride, ctx.precompCache, ctx.renderResources, ctx.matteLeases };

    renderLayerList (g, asset->layers, precompCtx);
}

//==============================================================================

namespace
{

Path createRectanglePath (Rectangle<float> bounds)
{
    Path path;
    path.addRectangle (bounds);
    return path;
}

} // namespace

AnimationRenderer::ClipPathResult AnimationRenderer::buildLayerMaskClipPath (const AnimationLayer& layer, float frameNo, Size<float> compSize)
{
    const bool masksAreStatic = layer.areAllMasksStatic();

    const bool cacheHits = masksAreStatic
                             ? layer.cachedMaskIsStatic
                             : (layer.cachedMaskClipPath.has_value() && layer.cachedMaskFrameNo == frameNo);

    if (cacheHits && layer.cachedMaskSize == compSize)
        return { *layer.cachedMaskClipPath, true };

    const auto maskBoundsPath = createRectanglePath (getLayerContentBounds (layer, compSize));

    Path clipPath;
    bool hasAnyMask = false;

    for (const auto& mask : layer.masks)
    {
        if (mask == nullptr)
            continue;

        if (mask->mode == AnimationMask::Mode::None)
            continue;

        hasAnyMask = true;

        Path maskPath = mask->shapeAt (frameNo);
        if (mask->inverted)
            maskPath = maskBoundsPath.combinedWith (maskPath, Path::BooleanOperation::Subtract);

        switch (mask->mode)
        {
            case AnimationMask::Mode::Add:
                clipPath = clipPath.isEmpty() ? maskPath
                                              : clipPath.combinedWith (maskPath, Path::BooleanOperation::Union);
                break;

            case AnimationMask::Mode::Subtract:
                clipPath = clipPath.isEmpty() ? maskBoundsPath.combinedWith (maskPath, Path::BooleanOperation::Subtract)
                                              : clipPath.combinedWith (maskPath, Path::BooleanOperation::Subtract);
                break;

            case AnimationMask::Mode::Intersect:
                clipPath = clipPath.isEmpty() ? maskBoundsPath.combinedWith (maskPath, Path::BooleanOperation::Intersect)
                                              : clipPath.combinedWith (maskPath, Path::BooleanOperation::Intersect);
                break;

            case AnimationMask::Mode::Difference:
                clipPath = clipPath.isEmpty() ? maskPath
                                              : clipPath.combinedWith (maskPath, Path::BooleanOperation::Xor);
                break;

            case AnimationMask::Mode::None:
                break;
        }
    }

    if (hasAnyMask)
    {
        layer.cachedMaskClipPath = clipPath;
        layer.cachedMaskFrameNo = frameNo;
        layer.cachedMaskIsStatic = masksAreStatic;
        layer.cachedMaskSize = compSize;
    }

    return { clipPath, hasAnyMask };
}

bool AnimationRenderer::applyMasks (Graphics& g, const AnimationLayer& layer, float frameNo, Size<float> compSize)
{
    if (layer.masks.empty())
        return true;

    const auto clipPath = AnimationRenderer::buildLayerMaskClipPath (layer, frameNo, compSize);
    if (! clipPath.active)
        return true;

    if (clipPath.path.isEmpty())
        return false;

    return applyClipPathInCurrentTransform (g, clipPath.path);
}

//==============================================================================

namespace
{

Path buildMatteClipPathForGroup (const AnimationGroup& group,
                                 float frameNo,
                                 const AffineTransform& parentTransform)
{
    if (group.hidden || group.transform.opacityAt (frameNo) <= 0.0f)
        return {};

    const AffineTransform groupTransform = group.transform.toAffineTransform (frameNo).followedBy (parentTransform);

    Path clipPath;
    for (const auto& child : group.children)
    {
        if (child.kind == AnimationGroup::ChildKind::Shape
            && child.shape != nullptr
            && ! child.shape->isHidden())
        {
            clipPath.appendPath (child.shape->buildPath (frameNo), groupTransform);
        }
        else if (child.kind == AnimationGroup::ChildKind::Group
                 && child.group != nullptr)
        {
            clipPath.appendPath (buildMatteClipPathForGroup (*child.group, frameNo, groupTransform));
        }
    }

    return clipPath;
}

const AnimationLayer* findLayerById (const std::vector<AnimationLayer::Ptr>& layers, int layerId)
{
    for (const auto& layer : layers)
    {
        if (layer != nullptr && layer->id == layerId)
            return layer.get();
    }

    return nullptr;
}

AffineTransform buildLayerTransformInAsset (const AnimationLayer& layer,
                                            const std::vector<AnimationLayer::Ptr>& layers,
                                            float frameNo,
                                            int depth = 0)
{
    AffineTransform transform = layer.transform.toAffineTransform (frameNo);

    if (layer.parentId < 0 || depth > 32)
        return transform;

    if (const auto* parent = findLayerById (layers, layer.parentId))
        transform = transform.followedBy (buildLayerTransformInAsset (*parent, layers, frameNo, depth + 1));

    return transform;
}

Path buildLayerAlphaPath (const AnimationLayer& layer, const AnimationComposition& comp, float frameNo)
{
    switch (layer.getType())
    {
        case AnimationLayer::Type::Shape:
        {
            const auto& shapeLayer = static_cast<const ShapeLayer&> (layer);

            Path clipPath;
            for (const auto& group : shapeLayer.groups)
            {
                if (group != nullptr)
                    clipPath.appendPath (buildMatteClipPathForGroup (*group, frameNo, AffineTransform::identity()));
            }
            return clipPath;
        }

        case AnimationLayer::Type::Solid:
        case AnimationLayer::Type::Image:
            return createRectanglePath (getLayerContentBounds (layer, comp.size));

        case AnimationLayer::Type::Precomp:
        {
            const auto& precompLayer = static_cast<const PrecompLayer&> (layer);
            const AnimationAsset* asset = comp.assets.contains (precompLayer.precompRefId)
                                            ? comp.assets[precompLayer.precompRefId].get()
                                            : nullptr;
            if (asset == nullptr)
                return createRectanglePath (getLayerContentBounds (layer, comp.size));

            const float localFrame = precompLayer.localFrame (frameNo, comp.frameRate);

            Path clipPath;
            for (const auto& childLayer : asset->layers)
            {
                if (childLayer == nullptr || childLayer->hidden || childLayer->isMatteSource || ! childLayer->isVisibleAt (localFrame))
                    continue;

                Path childPath = buildLayerAlphaPath (*childLayer, comp, localFrame);
                if (childPath.isEmpty())
                    continue;

                clipPath.appendPath (childPath.transformed (buildLayerTransformInAsset (*childLayer, asset->layers, localFrame)));
            }

            return clipPath;
        }

        case AnimationLayer::Type::Text:
        case AnimationLayer::Type::Null:
            break;
    }

    return {};
}

} // namespace

void AnimationRenderer::renderDropShadow (Graphics& g, const AnimationLayer& layer, const RenderContext& ctx, float opacity)
{
    if (! layer.dropShadow.has_value())
        return;

    const auto& shadow = *layer.dropShadow;
    const float shadowOpacity = opacity * shadow.opacityAt (ctx.scene.frameNo);
    if (shadowOpacity <= 0.0f)
        return;

    Path shadowPath = buildLayerAlphaPath (layer, ctx.scene.comp, ctx.scene.frameNo);
    if (shadowPath.isEmpty())
        return;

    shadowPath = shadowPath.transformed (AffineTransform::translation (shadow.offsetAt (ctx.scene.frameNo)));

    auto shadowState = g.saveState();
    g.setFillColor (shadow.color.getValueAt (ctx.scene.frameNo).withMultipliedAlpha (shadowOpacity));
    g.fillPath (shadowPath);
}

void AnimationRenderer::applyMatteSourceClip (Graphics& g,
                                              const AnimationLayer& layer,
                                              const AnimationLayer& matteSource,
                                              const RenderContext& ctx,
                                              bool inverted)
{
    Path clipPath = buildLayerAlphaPath (matteSource, ctx.scene.comp, ctx.scene.frameNo);

    if (! matteSource.masks.empty())
    {
        const auto matteMaskPath = AnimationRenderer::buildLayerMaskClipPath (matteSource, ctx.scene.frameNo, ctx.scene.compSize);
        if (matteMaskPath.active)
            clipPath = clipPath.combinedWith (matteMaskPath.path, Path::BooleanOperation::Intersect);
    }

    if (clipPath.isEmpty())
    {
        if (! inverted)
            applyClipPathInCurrentTransform (g, clipPath, true);

        return;
    }

    const AffineTransform layerXf = ctx.resolveLayerTransform (layer);
    const AffineTransform matteXf = ctx.resolveLayerTransform (matteSource);
    const auto matteToLayerXf = matteXf.followedBy (layerXf.inverted());
    const auto mattePathInLayerSpace = clipPath.transformed (matteToLayerXf);
    const auto layerBoundsPath = createRectanglePath (getLayerContentBounds (layer, ctx.scene.compSize));

    // Alpha mattes mask the target content directly. Intersecting with a derived
    // target alpha path can punch holes when child paths have mixed winding.
    const auto matteClipPath = inverted
                                 ? layerBoundsPath.combinedWith (mattePathInLayerSpace, Path::BooleanOperation::Subtract)
                                 : mattePathInLayerSpace;

    applyClipPathInCurrentTransform (g, matteClipPath, true);
}

//==============================================================================

void AnimationRenderer::renderGroup (Graphics& g,
                                     const AnimationGroup& group,
                                     const RenderContext& ctx,
                                     float opacity,
                                     const AnimationRoundedCorner* parentRoundedCorner,
                                     std::vector<Path>* geometryOut)
{
    if (group.hidden)
        return;

    auto saveState = g.saveState();

    const float frameNo = ctx.scene.frameNo;
    g.setTransform (group.transform.toAffineTransform (frameNo).followedBy (g.getTransform()));
    opacity *= group.transform.opacityAt (frameNo);
    if (opacity <= 0.0f)
        return;

    // Lottie draw-order: collect shapes, then when paint is encountered, apply it
    // Children are ordered: shapes first, then paints - we gather paths then paint.
    // Groups inside iterate recursively.

    // Collect trim and repeater modifiers (they affect all shapes in this scope)
    const AnimationTrim* activeTrim = nullptr;
    const AnimationRepeater* activeRepeater = nullptr;
    const AnimationRoundedCorner* activeRoundedCorner = parentRoundedCorner;
    const AnimationMergePaths* activeMergePaths = nullptr;

    if (group.hasAnyModifier)
    {
        for (const auto& child : group.children)
        {
            if (child.kind == AnimationGroup::ChildKind::Trim && child.trim != nullptr)
                activeTrim = child.trim.get();
            if (child.kind == AnimationGroup::ChildKind::Repeater && child.repeater != nullptr)
                activeRepeater = child.repeater.get();
            if (child.kind == AnimationGroup::ChildKind::RoundedCorner && child.roundedCorner != nullptr)
                activeRoundedCorner = child.roundedCorner.get();
            if (child.kind == AnimationGroup::ChildKind::MergePaths && child.mergePaths != nullptr)
                activeMergePaths = child.mergePaths.get();
        }
    }

    const bool hasRepeater = activeRepeater != nullptr && ! activeRepeater->hidden;
    const bool hasTrim = activeTrim != nullptr && ! activeTrim->hidden;
    const bool hasRounded = activeRoundedCorner != nullptr && ! activeRoundedCorner->hidden;
    // Only boolean merge modes (Add/Subtract/Intersect/Exclude) resolve to a path
    // boolean op. Plain "Merge" (mode 1) keeps the default concatenation so the
    // fill winding rule still carves holes (e.g. the counters in "O" and "A").
    const bool hasMergePaths = activeMergePaths != nullptr
                            && ! activeMergePaths->hidden
                            && activeMergePaths->isBooleanMerge();
    // Any merge-paths modifier (including plain "Merge") means the group's nested
    // geometry should feed the merge/fill. Without it, nested groups are
    // self-contained and must NOT contribute geometry to the parent's paints
    // (otherwise paint-less construction guides get filled - e.g. the stray
    // star shapes in pumped_up.json / mughead.json).
    const bool mergesNestedGeometry = activeMergePaths != nullptr && ! activeMergePaths->hidden;
    // An enclosing group asking for this group's geometry also needs the geometry
    // of any paint-less group nested inside it.
    const bool collectsNestedGeometry = mergesNestedGeometry || geometryOut != nullptr;
    const bool hasModifiers = hasRounded || hasTrim || hasRepeater || hasMergePaths;
    const bool hasDirectPaint = std::any_of (group.children.begin(),
                                             group.children.end(),
                                             [] (const AnimationGroup::ChildItem& child)
    {
        return child.kind == AnimationGroup::ChildKind::Fill
            || child.kind == AnimationGroup::ChildKind::Stroke;
    });

    std::vector<Path> currentPaths;
    std::vector<Path> preparedCache;
    bool preparedValid = false;

    auto computePrepared = [&]
    {
        preparedCache = currentPaths;

        // Merge Paths combines all collected geometry into a single shape using a
        // boolean operation, resolving overlapping/mixed-winding sub-paths correctly.
        if (hasMergePaths && preparedCache.size() > 1)
        {
            const auto op = activeMergePaths->toBooleanOperation();
            Path merged = preparedCache.front();
            for (size_t i = 1; i < preparedCache.size(); ++i)
                merged = merged.combinedWith (preparedCache[i], op);

            preparedCache.clear();
            preparedCache.push_back (std::move (merged));
        }

        if (hasRounded)
        {
            // The Round Corners (rd) radius is scaled relative to the shape size
            // so it reads the same way reference renderers (LottieFiles) present
            // it; withRoundedCorners then clamps per-corner to half the edge.
            const float radiusRatio = activeRoundedCorner->radiusAt (frameNo);
            if (radiusRatio > 1e-5f)
            {
                for (auto& path : preparedCache)
                {
                    const auto bounds = path.getBounds();
                    const float minDim = jmin (bounds.getWidth(), bounds.getHeight());
                    const float cornerRadius = minDim * radiusRatio;
                    if (cornerRadius > 1e-5f)
                        path = path.withRoundedCorners (cornerRadius);
                }
            }
        }

        if (hasTrim)
        {
            if (activeTrim->mode == AnimationTrim::TrimMode::Simultaneously)
            {
                for (auto& path : preparedCache)
                    applyTrim (path, *activeTrim, frameNo);
            }
            else
            {
                applyTrimIndividually (preparedCache, *activeTrim, frameNo);
            }
        }

        if (hasRepeater)
        {
            const int numCopies = activeRepeater->copiesAt (frameNo);

            const AnimationTransform& xf = activeRepeater->copyTransform;
            const Point<float> anchor = xf.anchor.getValueAt (frameNo);
            const Size<float> sc = xf.scale.getValueAt (frameNo);
            const float rot = xf.rotation.getValueAt (frameNo);
            const Point<float> pos = xf.separatePosition
                                       ? Point<float> { xf.positionX.getValueAt (frameNo), xf.positionY.getValueAt (frameNo) }
                                       : xf.position.getValueAt (frameNo);

            auto buildCopyTransform = [&] (float multiplier) -> AffineTransform
            {
                const float sx = (sc.getWidth() < 99.9f || sc.getWidth() > 100.1f)
                                   ? std::pow (sc.getWidth() / 100.0f, multiplier)
                                   : 1.0f;
                const float sy = (sc.getHeight() < 99.9f || sc.getHeight() > 100.1f)
                                   ? std::pow (sc.getHeight() / 100.0f, multiplier)
                                   : 1.0f;
                AffineTransform t;
                t = t.translated (-anchor.getX(), -anchor.getY());
                t = t.rotated (degreesToRadians (rot * multiplier));
                t = t.scaled (sx, sy);
                t = t.translated (anchor.getX(), anchor.getY());
                t = t.translated (pos.getX() * multiplier, pos.getY() * multiplier);
                return t;
            };

            std::vector<Path> repeatedPaths;
            for (int copy = 0; copy < numCopies; ++copy)
            {
                const float t = numCopies > 0 ? static_cast<float> (copy) / static_cast<float> (numCopies) : 0.0f;
                const float copyOpacity = activeRepeater->startOpacityAt (frameNo) * (1.0f - t)
                                        + activeRepeater->endOpacityAt (frameNo) * t;
                (void) copyOpacity; // TODO: per-copy opacity via Graphics state

                const AffineTransform copyXfm = buildCopyTransform (static_cast<float> (copy) + activeRepeater->offsetAt (frameNo));
                for (const auto& p : preparedCache)
                    repeatedPaths.push_back (p.transformed (copyXfm));
            }
            preparedCache = std::move (repeatedPaths);
        }

        preparedValid = true;
    };

    for (const auto& child : group.children)
    {
        if (child.kind == AnimationGroup::ChildKind::Shape && child.shape != nullptr)
        {
            if (! child.shape->isHidden())
            {
                currentPaths.push_back (child.shape->buildPath (frameNo));
                preparedValid = false;
            }
        }
        else if (child.kind == AnimationGroup::ChildKind::Stroke && child.stroke != nullptr)
        {
            if (! child.stroke->hidden && ! currentPaths.empty())
            {
                Path combinedPath;

                if (hasModifiers)
                {
                    if (! preparedValid)
                        computePrepared();

                    for (const auto& path : preparedCache)
                        combinedPath.appendPath (path);
                }
                else
                {
                    for (const auto& path : currentPaths)
                        combinedPath.appendPath (path);
                }

                applyStroke (g, combinedPath, *child.stroke, ctx, opacity);
            }
        }
        else if (child.kind == AnimationGroup::ChildKind::Fill && child.fill != nullptr)
        {
            if (! child.fill->hidden && ! currentPaths.empty())
            {
                Path combinedPath;

                if (hasModifiers)
                {
                    if (! preparedValid)
                        computePrepared();

                    for (const auto& path : preparedCache)
                        combinedPath.appendPath (path);
                }
                else
                {
                    for (const auto& path : currentPaths)
                        combinedPath.appendPath (path);
                }

                applyFill (g, combinedPath, *child.fill, ctx, opacity);
            }
        }
        else if (child.kind == AnimationGroup::ChildKind::Group && child.group != nullptr)
        {
            // A nested group without its own paint can supply geometry to a
            // parent paint or Merge Paths modifier.
            const bool wantsNestedGeometry = collectsNestedGeometry || hasDirectPaint;

            const bool hasOwnPaint = std::any_of (child.group->children.begin(),
                                                  child.group->children.end(),
                                                  [] (const AnimationGroup::ChildItem& c)
            {
                return c.kind == AnimationGroup::ChildKind::Fill
                    || c.kind == AnimationGroup::ChildKind::Stroke;
            });

            // Harvest the geometry from the nested render call rather than
            // rebuilding it from raw shapes: the nested group's own modifiers are
            // what define the outline. A trim reducing a 4-point star to an arc is
            // how RubberHose rigs draw a limb (mughead.json, pumped_up.json) - drop
            // it and the parent's stroke paints the whole star instead.
            std::vector<Path> nestedGeometry;
            const bool harvest = wantsNestedGeometry && ! hasOwnPaint;

            renderGroup (g, *child.group, ctx, opacity, activeRoundedCorner, harvest ? &nestedGeometry : nullptr);

            if (harvest)
            {
                Path combinedGeometry;
                for (const auto& path : nestedGeometry)
                    combinedGeometry.appendPath (path);

                if (! combinedGeometry.isEmpty())
                {
                    currentPaths.push_back (std::move (combinedGeometry));
                    preparedValid = false;
                }
            }
        }
    }

    if (geometryOut == nullptr || currentPaths.empty())
        return;

    // Report the modifier-applied geometry in the enclosing group's space. The
    // group transform is applied here because the caller paints these paths under
    // its own transform, not this group's.
    if (hasModifiers && ! preparedValid)
        computePrepared();

    const auto groupTransform = group.transform.toAffineTransform (frameNo);

    for (const auto& path : (hasModifiers ? preparedCache : currentPaths))
        geometryOut->push_back (path.transformed (groupTransform));
}

void AnimationRenderer::applyTrim (Path& path, const AnimationTrim& trim, float frameNo)
{
    const auto segment = trim.getSegment (frameNo);
    path.trim (segment.start, segment.end);
}

void AnimationRenderer::applyTrimIndividually (std::vector<Path>& paths, const AnimationTrim& trim, float frameNo)
{
    const auto segment = trim.getSegment (frameNo);

    if (std::abs (segment.start - segment.end) <= 1.0e-5f)
    {
        for (auto& path : paths)
            path.clear();

        return;
    }

    if (segment.start <= 0.0f && segment.end >= 1.0f)
        return;

    float totalLength = 0.0f;
    for (const auto& path : paths)
        totalLength += path.getLength();

    if (totalLength <= 1.0e-5f)
    {
        for (auto& path : paths)
            path.clear();

        return;
    }

    auto appendDistributedRange = [] (Path& result,
                                      const Path& source,
                                      float sourceStart,
                                      float sourceEnd,
                                      float rangeStart,
                                      float rangeEnd)
    {
        const float overlapStart = jmax (sourceStart, rangeStart);
        const float overlapEnd = jmin (sourceEnd, rangeEnd);

        if (overlapEnd <= overlapStart)
            return;

        const float sourceLength = sourceEnd - sourceStart;
        if (sourceLength <= 1.0e-5f)
            return;

        if (overlapStart <= sourceStart + 1.0e-5f && overlapEnd >= sourceEnd - 1.0e-5f)
        {
            result.appendPath (source);
            return;
        }

        const float localStart = (overlapStart - sourceStart) / sourceLength;
        const float localEnd = (overlapEnd - sourceStart) / sourceLength;
        result.appendPath (source.getTrimmedPath (localStart, localEnd));
    };

    const float startDistance = segment.start * totalLength;
    const float endDistance = segment.end * totalLength;
    float pathStart = 0.0f;

    for (auto& path : paths)
    {
        const float pathLength = path.getLength();
        const float pathEnd = pathStart + pathLength;
        Path trimmed;

        if (segment.start < segment.end)
        {
            appendDistributedRange (trimmed, path, pathStart, pathEnd, startDistance, endDistance);
        }
        else
        {
            appendDistributedRange (trimmed, path, pathStart, pathEnd, startDistance, totalLength);
            appendDistributedRange (trimmed, path, pathStart, pathEnd, 0.0f, endDistance);
        }

        path = std::move (trimmed);
        pathStart = pathEnd;
    }
}

void AnimationRenderer::applyFill (Graphics& g, const Path& path, const FillPaint& fill, const RenderContext& ctx, float opacity)
{
    if (! fill.enabled)
        return;

    const float frameNo = ctx.scene.frameNo;
    const float finalOpacity = opacity * fill.opacityAt (frameNo);
    if (finalOpacity <= 0.0f)
        return;

    if (ctx.paintOverride.has_value())
    {
        g.setFillColor (ctx.paintOverride->withMultipliedAlpha (finalOpacity));
    }
    else if (fill.gradient != nullptr)
    {
        ColorGradient cg = fill.gradient->toColorGradient (frameNo).withMultipliedAlpha (finalOpacity);
        g.setFillColorGradient (cg);
    }
    else
    {
        Color c = fill.colorAt (frameNo).withMultipliedAlpha (finalOpacity);
        g.setFillColor (c);
    }

    Path pathToFill = path;
    pathToFill.setUsingNonZeroWinding (fill.fillRule == FillPaint::FillRule::NonZero);
    g.fillPath (pathToFill);
}

void AnimationRenderer::applyStroke (Graphics& g, const Path& path, const StrokePaint& stroke, const RenderContext& ctx, float opacity)
{
    if (! stroke.enabled)
        return;

    const float frameNo = ctx.scene.frameNo;
    const float finalOpacity = opacity * stroke.opacityAt (frameNo);
    if (finalOpacity <= 0.0f)
        return;

    if (ctx.paintOverride.has_value())
    {
        g.setStrokeColor (ctx.paintOverride->withMultipliedAlpha (finalOpacity));
    }
    else if (stroke.gradient != nullptr)
    {
        ColorGradient cg = stroke.gradient->toColorGradient (frameNo).withMultipliedAlpha (finalOpacity);
        g.setStrokeColorGradient (cg);
    }
    else
    {
        Color c = stroke.colorAt (frameNo).withMultipliedAlpha (finalOpacity);
        g.setStrokeColor (c);
    }

    g.setStrokeType (stroke.strokeTypeAt (frameNo));

    // Apply dash pattern if present
    if (! stroke.dashArray.empty())
    {
        const auto resolved = stroke.resolveDash (frameNo);
        const auto& dashValues = resolved.dashValues;
        float dashOffset = resolved.offset;

        if (dashValues.size() > 1)
        {
            Array<float> dashArray;
            for (float v : dashValues)
            {
                if (v > 0.0f)
                    dashArray.add (v);
            }

            if (! dashArray.isEmpty())
            {
                if ((dashArray.size() % 2) != 0)
                {
                    const int originalSize = dashArray.size();
                    for (int i = 0; i < originalSize; ++i)
                        dashArray.add (dashArray[i]);
                }

                float totalLen = 0.0f;
                for (auto d : dashArray)
                    totalLen += d;

                if (totalLen > 0.0f)
                {
                    if (dashOffset != 0.0f)
                        dashOffset = std::fmod (std::abs (dashOffset), totalLen);

                    int dashIdx = 0;
                    float patternPos = dashOffset;
                    while (patternPos >= dashArray[dashIdx])
                    {
                        patternPos -= dashArray[dashIdx];
                        dashIdx = (dashIdx + 1) % dashArray.size();
                    }

                    Path dashedPath;
                    Point<float> currentPt;
                    Point<float> subPathStart;
                    bool hasCurrent = false;

                    auto addDashSegment = [&] (Point<float> p1, Point<float> p2)
                    {
                        const float len = p1.distanceTo (p2);
                        if (len <= 0.0f)
                            return;

                        const auto dir = (p2 - p1) / len;
                        float travelled = 0.0f;

                        while (travelled < len)
                        {
                            const float remainingInDash = dashArray[dashIdx] - patternPos;
                            const float step = jmin (remainingInDash, len - travelled);

                            if ((dashIdx % 2) == 0 && step > 0.0f)
                            {
                                const auto segStart = p1 + dir * travelled;
                                const auto segEnd = p1 + dir * (travelled + step);
                                dashedPath.startNewSubPath (segStart);
                                dashedPath.lineTo (segEnd);
                            }

                            travelled += step;
                            patternPos = 0.0f;
                            dashIdx = (dashIdx + 1) % dashArray.size();
                        }
                    };

                    for (const auto& segment : path)
                    {
                        switch (segment.verb)
                        {
                            case Path::Verb::MoveTo:
                                currentPt = segment.point;
                                subPathStart = currentPt;
                                hasCurrent = true;
                                dashIdx = 0;
                                patternPos = dashOffset;
                                while (patternPos >= dashArray[dashIdx])
                                {
                                    patternPos -= dashArray[dashIdx];
                                    dashIdx = (dashIdx + 1) % dashArray.size();
                                }
                                break;

                            case Path::Verb::LineTo:
                                if (hasCurrent)
                                    addDashSegment (currentPt, segment.point);
                                currentPt = segment.point;
                                break;

                            case Path::Verb::Close:
                                if (hasCurrent)
                                    addDashSegment (currentPt, subPathStart);
                                currentPt = subPathStart;
                                break;

                            default:
                                break;
                        }
                    }

                    g.strokePath (dashedPath);
                    return;
                }
            }
        }
    }

    g.strokePath (path);
}

} // namespace yup
