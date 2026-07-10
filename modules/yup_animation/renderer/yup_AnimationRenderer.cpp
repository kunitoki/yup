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
// SceneContext

void AnimationRenderer::SceneContext::buildParentTransforms (const std::vector<AnimationLayer::Ptr>& layers)
{
    parentTransforms.clear();

    std::vector<bool> resolved (layers.size(), false);
    bool anyResolved = true;

    while (anyResolved)
    {
        anyResolved = false;
        for (size_t i = 0; i < layers.size(); ++i)
        {
            if (resolved[i])
                continue;

            const AnimationLayer* layer = layers[i].get();
            if (layer == nullptr)
            {
                resolved[i] = true;
                continue;
            }

            if (layer->parentId < 0)
            {
                parentTransforms.set (layer->id, layer->transform.toAffineTransform (frameNo));
                resolved[i] = true;
                anyResolved = true;
            }
            else if (auto* parentXf = parentTransforms.getPointer (layer->parentId))
            {
                parentTransforms.set (layer->id, layer->transform.toAffineTransform (frameNo).followedBy (*parentXf));
                resolved[i] = true;
                anyResolved = true;
            }
        }
    }

    for (size_t i = 0; i < layers.size(); ++i)
    {
        if (! resolved[i] && layers[i] != nullptr)
        {
            const AnimationLayer* layer = layers[i].get();
            parentTransforms.set (layer->id, layer->transform.toAffineTransform (frameNo));
        }
    }
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
                                           Justification justification)
{
    renderComposition (g, comp, frameNo, bounds, fitting, justification, 1.0f, std::nullopt);
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
                                           std::optional<Color> paintOverride)
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
    Path viewportClip;
    viewportClip.addRectangle (clipRect);

    const auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());

    auto transformedViewportClip = viewportClip.transformed (clipTransform);
    const auto currentClipPath = g.getClipPath();
    if (! transformedViewportClip.isEmpty() && ! currentClipPath.isEmpty())
        transformedViewportClip = currentClipPath.combinedWith (transformedViewportClip, Path::BooleanOperation::Intersect);

    const auto savedTransform = g.getTransform();

    g.setTransform (AffineTransform::identity());
    g.setClipPath (transformedViewportClip);
    g.setTransform (savedTransform);

    SceneContext sceneCtx { comp, frameNo, compSize };
    sceneCtx.buildParentTransforms (comp.layers);

    PrecompCache precompCache;
    RenderContext ctx { sceneCtx, viewXf, opacity, std::move (paintOverride), &precompCache };

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
    const float opacity = ctx.opacity * layer.transform.opacityAt (ctx.scene.frameNo);

    if (opacity <= 0.0f)
        return;

    if (opacity < 1.0f && renderLayerIsolated (g, layer, ctx, matteSource, opacity))
        return;

    renderLayerDirect (g, layer, ctx, matteSource, opacity);
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
        && (layer.matteType == AnimationLayer::MatteType::Alpha
            || layer.matteType == AnimationLayer::MatteType::AlphaInv))
    {
        applyMatteSourceClip (g, layer, *matteSource, ctx, layer.matteType == AnimationLayer::MatteType::AlphaInv);
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

    auto transparencyLayer = g.beginTransparencyLayer (fittedRect, opacity);
    if (! transparencyLayer.isValid())
        return false;

    // Inside the layer, render with the fitted scale but no translation: the
    // layer-local origin is already the fitted rectangle's top-left corner.
    RenderContext layerCtx = ctx;
    layerCtx.viewTransform = AffineTransform::scaling (ctx.viewTransform.getScaleX(), ctx.viewTransform.getScaleY());
    layerCtx.opacity = 1.0f;

    renderLayerDirect (transparencyLayer.getGraphics(), layer, layerCtx, matteSource, 1.0f);

    // The target area is already in screen space, so it composites back using the
    // parent's current transform without re-applying the view transform.
    return transparencyLayer.commit();
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

    // Apply viewport clip matching renderComposition's clip behavior
    auto clipState = g.saveState();
    Path viewportClip;
    viewportClip.addRectangle (precompBounds);

    const auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());
    auto transformedViewportClip = viewportClip.transformed (clipTransform);
    const auto currentClipPath = g.getClipPath();
    if (! transformedViewportClip.isEmpty() && ! currentClipPath.isEmpty())
        transformedViewportClip = currentClipPath.combinedWith (transformedViewportClip, Path::BooleanOperation::Intersect);

    const auto savedTransform = g.getTransform();
    g.setTransform (AffineTransform::identity());
    g.setClipPath (transformedViewportClip);
    g.setTransform (savedTransform);

    // Check cache first — if this precomp asset was already rendered this frame,
    // just draw the cached texture. Skip caching when already rendering to an
    // offscreen target (nested precomps or inside GpuCanvas).
    if (ctx.precompCache != nullptr)
    {
        if (auto* cached = ctx.precompCache->textures.getPointer (layer.precompRefId))
        {
            g.setOpacity (g.getOpacity() * opacity);
            g.drawTexture (*cached, precompBounds);
            return;
        }
    }

    // Build a scene context for the asset's layers (reuses the parent comp's assets map)
    SceneContext precompScene { ctx.scene.comp, localFrame, layer.layerSize };

    const float scaleX = precompBounds.getWidth() / layer.layerSize.getWidth();
    const float scaleY = precompBounds.getHeight() / layer.layerSize.getHeight();
    const AffineTransform precompViewXf = AffineTransform::scaling (scaleX, scaleY)
                                              .followedBy (AffineTransform::translation (precompBounds.getX(), precompBounds.getY()));

    if (ctx.precompCache != nullptr)
    {
        // Render precomp to an offscreen canvas once, then cache for reuse.
        // Size the offscreen target to the on-screen device resolution so the
        // cached texture is not upscaled (which would lose quality). The current
        // graphics transform maps layer space to device pixels, so its scale
        // factor tells us how many device pixels each layer unit occupies.
        const float deviceScale = jlimit (1.0f, 8.0f, g.getTransform().getScaleFactor());

        const int w = static_cast<int> (std::ceil (layer.layerSize.getWidth() * deviceScale));
        const int h = static_cast<int> (std::ceil (layer.layerSize.getHeight() * deviceScale));

        if (w > 0 && h > 0)
        {
            auto canvas = GpuCanvas::create (g.getGraphicsContext(), w, h);
            if (canvas != nullptr)
            {
                {
                    auto& offscreenG = canvas->beginDraw();

                    SceneContext offscreenScene { ctx.scene.comp, localFrame, layer.layerSize };
                    offscreenScene.buildParentTransforms (asset->layers);

                    RenderContext offscreenCtx { offscreenScene, AffineTransform::scaling (deviceScale), 1.0f, ctx.paintOverride, ctx.precompCache };
                    renderLayerList (offscreenG, asset->layers, offscreenCtx);
                }

                auto tex = canvas->asTexture();
                if (tex != nullptr)
                {
                    ctx.precompCache->textures.set (layer.precompRefId, tex);

                    g.setOpacity (g.getOpacity() * opacity);
                    g.drawTexture (tex, precompBounds);
                    return;
                }
            }
        }
    }

    precompScene.buildParentTransforms (asset->layers);

    RenderContext precompCtx { precompScene, precompViewXf, opacity, ctx.paintOverride };

    renderLayerList (g, asset->layers, precompCtx);
}

//==============================================================================

namespace
{

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

Path createRectanglePath (Rectangle<float> bounds)
{
    Path path;
    path.addRectangle (bounds);
    return path;
}

void applyClipPathInCurrentTransform (Graphics& g, const Path& clipPath, bool allowEmpty = false)
{
    if (clipPath.isEmpty() && ! allowEmpty)
        return;

    const auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());
    auto transformedClipPath = clipPath.transformed (clipTransform);
    const auto currentClipPath = g.getClipPath();
    if (! transformedClipPath.isEmpty() && ! currentClipPath.isEmpty())
        transformedClipPath = currentClipPath.combinedWith (transformedClipPath, Path::BooleanOperation::Intersect);

    const auto savedTransform = g.getTransform();

    g.setTransform (AffineTransform::identity());
    g.setClipPath (transformedClipPath);
    g.setTransform (savedTransform);
}

} // namespace

AnimationRenderer::ClipPathResult AnimationRenderer::buildLayerMaskClipPath (const AnimationLayer& layer, float frameNo, Size<float> compSize)
{
    if (layer.areAllMasksStatic()
        && layer.cachedMaskClipPath.has_value()
        && layer.cachedMaskFrameNo == frameNo)
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

    if (layer.areAllMasksStatic() && hasAnyMask)
    {
        layer.cachedMaskClipPath = clipPath;
        layer.cachedMaskFrameNo = frameNo;
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

    applyClipPathInCurrentTransform (g, clipPath.path);
    return true;
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
                                     const AnimationRoundedCorner* parentRoundedCorner)
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
    const bool hasModifiers = hasRounded || hasTrim || hasRepeater || hasMergePaths;

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
            const float radiusRatio = activeRoundedCorner->radiusAt (frameNo);
            if (radiusRatio > 1e-5f)
            {
                for (auto& path : preparedCache)
                {
                    auto bounds = path.getBounds();
                    const float minDim = jmin (bounds.getWidth(), bounds.getHeight());
                    const float cornerRadius = minDim * radiusRatio * 0.5f;
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
            renderGroup (g, *child.group, ctx, opacity, activeRoundedCorner);

            // A nested group without its own paint acts as a geometry container
            // for the parent's Merge Paths. Without an active Merge Paths modifier
            // the group is self-contained — do NOT feed its geometry into the
            // parent's paints (avoids filling construction-guide shapes).
            if (! mergesNestedGeometry)
                continue;

            const bool hasOwnPaint = std::any_of (child.group->children.begin(),
                                                  child.group->children.end(),
                                                  [] (const AnimationGroup::ChildItem& c)
            {
                return c.kind == AnimationGroup::ChildKind::Fill
                    || c.kind == AnimationGroup::ChildKind::Stroke;
            });

            if (! hasOwnPaint)
            {
                Path nestedGeometry = buildMatteClipPathForGroup (*child.group, frameNo, AffineTransform::identity());
                if (! nestedGeometry.isEmpty())
                {
                    currentPaths.push_back (std::move (nestedGeometry));
                    preparedValid = false;
                }
            }
        }
    }
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
