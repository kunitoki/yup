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
// RenderContext

void AnimationRenderer::RenderContext::buildParentTransforms()
{
    parentTransforms.clear();

    // Build transforms for each layer, resolving parent chains.
    // We iterate until all transforms are resolved or no progress is made.
    std::vector<bool> resolved (comp.layers.size(), false);
    bool anyResolved = true;

    while (anyResolved)
    {
        anyResolved = false;
        for (size_t i = 0; i < comp.layers.size(); ++i)
        {
            if (resolved[i])
                continue;

            const AnimationLayer* layer = comp.layers[i].get();
            if (layer == nullptr)
            {
                resolved[i] = true;
                continue;
            }

            if (layer->parentId < 0)
            {
                // No parent — local transform only
                parentTransforms.set (layer->id, layer->transform.toAffineTransform (frameNo));
                resolved[i] = true;
                anyResolved = true;
            }
            else if (parentTransforms.contains (layer->parentId))
            {
                const AffineTransform parentXf = parentTransforms[layer->parentId];
                parentTransforms.set (layer->id, layer->transform.toAffineTransform (frameNo).followedBy (parentXf));
                resolved[i] = true;
                anyResolved = true;
            }
        }
    }

    // Any unresolved layers (circular reference) get their own local transform
    for (size_t i = 0; i < comp.layers.size(); ++i)
    {
        if (! resolved[i] && comp.layers[i] != nullptr)
        {
            const AnimationLayer* layer = comp.layers[i].get();
            parentTransforms.set (layer->id, layer->transform.toAffineTransform (frameNo));
        }
    }
}

AffineTransform AnimationRenderer::RenderContext::resolveLayerTransform (const AnimationLayer& layer) const
{
    if (parentTransforms.contains (layer.id))
        return parentTransforms[layer.id].followedBy (viewTransform);
    return layer.transform.toAffineTransform (frameNo).followedBy (viewTransform);
}

//==============================================================================
// AnimationRenderer

void AnimationRenderer::renderComposition (Graphics& g,
                                           const AnimationComposition& comp,
                                           float frameNo,
                                           Rectangle<float> bounds,
                                           bool keepAspectRatio)
{
    renderComposition (g, comp, frameNo, bounds, keepAspectRatio, 1.0f, std::nullopt);
}

void AnimationRenderer::renderComposition (Graphics& g,
                                           const AnimationComposition& comp,
                                           float frameNo,
                                           Rectangle<float> bounds,
                                           bool keepAspectRatio,
                                           float opacity,
                                           std::optional<Color> paintOverride)
{
    const Size<float> compSize = comp.size;
    if (compSize.getWidth() <= 0.0f || compSize.getHeight() <= 0.0f)
        return;

    // Compute view transform: composition-space → screen-space
    AffineTransform viewXf;
    if (keepAspectRatio)
    {
        const float scaleX = bounds.getWidth() / compSize.getWidth();
        const float scaleY = bounds.getHeight() / compSize.getHeight();
        const float scale = jmin (scaleX, scaleY);
        const float tx = bounds.getX() + (bounds.getWidth() - compSize.getWidth() * scale) * 0.5f;
        const float ty = bounds.getY() + (bounds.getHeight() - compSize.getHeight() * scale) * 0.5f;
        viewXf = AffineTransform::scaling (scale, scale).followedBy (AffineTransform::translation (tx, ty));
    }
    else
    {
        const float scaleX = bounds.getWidth() / compSize.getWidth();
        const float scaleY = bounds.getHeight() / compSize.getHeight();
        viewXf = AffineTransform::scaling (scaleX, scaleY).followedBy (AffineTransform::translation (bounds.getX(), bounds.getY()));
    }

    auto clipState = g.saveState();
    Path viewportClip;
    viewportClip.addRectangle (bounds);

    const auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());

    auto transformedViewportClip = viewportClip.transformed (clipTransform);
    const auto currentClipPath = g.getClipPath();
    if (! transformedViewportClip.isEmpty() && ! currentClipPath.isEmpty())
        transformedViewportClip = currentClipPath.combinedWith (transformedViewportClip, Path::BooleanOperation::Intersect);

    const auto savedTransform = g.getTransform();

    g.setTransform (AffineTransform::identity());
    g.setClipPath (transformedViewportClip);
    g.setTransform (savedTransform);

    RenderContext ctx { comp, frameNo, viewXf, opacity, std::move (paintOverride) };
    ctx.buildParentTransforms();

    // Lottie: layer 0 is topmost visually → render bottom-up (last index first).
    // Matte pairs: a layer with isMatteSource=true (td=1) sits just ABOVE the matte
    // target (the layer with matteType != None, tt>0). Array index i-1 is the matte
    // source for layer i when layer i has tt>0. Matte sources must not render normally.
    for (int i = (int) comp.layers.size() - 1; i >= 0; --i)
    {
        const AnimationLayer* layer = comp.layers[(size_t) i].get();
        if (layer == nullptr || layer->hidden)
            continue;

        // Matte source layers (td=1) are consumed by the adjacent matte target; skip.
        if (layer->isMatteSource)
            continue;

        if (! layer->isVisibleAt (frameNo))
            continue;

        // If this layer is a matte target, the matte source is at array index i-1.
        const AnimationLayer* matteSource = nullptr;
        if (layer->matteType != AnimationLayer::MatteType::None && i >= 1)
        {
            const AnimationLayer* candidate = comp.layers[(size_t) (i - 1)].get();
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
    const float opacity = ctx.opacity * layer.transform.opacityAt (ctx.frameNo);

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

    if (! applyMasks (g, layer, ctx.frameNo, ctx.comp.size))
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
        const float fillEffectOpacity = layer.fillEffect->opacityAt (ctx.frameNo);
        if (fillEffectOpacity > 0.0f)
            layerCtx.paintOverride = layer.fillEffect->colorAt (ctx.frameNo).withMultipliedAlpha (fillEffectOpacity);
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
    auto transparencyLayer = g.beginTransparencyLayer ({ 0.0f, 0.0f, ctx.comp.size.getWidth(), ctx.comp.size.getHeight() }, opacity);
    if (! transparencyLayer.isValid())
        return false;

    RenderContext layerCtx = ctx;
    layerCtx.viewTransform = AffineTransform::identity();
    layerCtx.opacity = 1.0f;

    renderLayerDirect (transparencyLayer.getGraphics(), layer, layerCtx, matteSource, 1.0f);

    auto saveState = g.saveState();
    const AffineTransform baseTransform = g.getTransform();
    g.setTransform (ctx.viewTransform.followedBy (baseTransform));

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
    const AnimationAsset* asset = ctx.comp.assets.contains (layer.precompRefId)
                                    ? ctx.comp.assets[layer.precompRefId].get()
                                    : nullptr;
    if (asset == nullptr)
        return;

    const float localFrame = layer.localFrame (ctx.frameNo, ctx.comp.frameRate);
    const Rectangle<float> precompBounds (0.0f, 0.0f, layer.layerSize.getWidth(), layer.layerSize.getHeight());

    // Build a temporary composition from the precomp asset and render it
    auto tempComp = AnimationComposition::create (layer.layerSize, ctx.comp.frameRate);
    tempComp->layers = asset->layers;
    tempComp->assets = ctx.comp.assets;
    tempComp->startFrame = ctx.comp.startFrame;
    tempComp->endFrame = ctx.comp.endFrame;

    renderComposition (g, *tempComp, localFrame, precompBounds, false, opacity, ctx.paintOverride);
}

//==============================================================================

namespace
{

struct ClipPathResult
{
    Path path;
    bool active = false;
};

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

ClipPathResult buildLayerMaskClipPath (const AnimationLayer& layer, float frameNo, Size<float> compSize)
{
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

    return { clipPath, hasAnyMask };
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

bool AnimationRenderer::applyMasks (Graphics& g, const AnimationLayer& layer, float frameNo, Size<float> compSize)
{
    if (layer.masks.empty())
        return true;

    const auto clipPath = buildLayerMaskClipPath (layer, frameNo, compSize);
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
    const float shadowOpacity = opacity * shadow.opacityAt (ctx.frameNo);
    if (shadowOpacity <= 0.0f)
        return;

    Path shadowPath = buildLayerAlphaPath (layer, ctx.comp, ctx.frameNo);
    if (shadowPath.isEmpty())
        return;

    shadowPath = shadowPath.transformed (AffineTransform::translation (shadow.offsetAt (ctx.frameNo)));

    auto shadowState = g.saveState();
    g.setFillColor (shadow.color.getValueAt (ctx.frameNo).withMultipliedAlpha (shadowOpacity));
    g.fillPath (shadowPath);
}

void AnimationRenderer::applyMatteSourceClip (Graphics& g,
                                              const AnimationLayer& layer,
                                              const AnimationLayer& matteSource,
                                              const RenderContext& ctx,
                                              bool inverted)
{
    Path clipPath = buildLayerAlphaPath (matteSource, ctx.comp, ctx.frameNo);

    if (! matteSource.masks.empty())
    {
        const auto matteMaskPath = buildLayerMaskClipPath (matteSource, ctx.frameNo, ctx.comp.size);
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
    const auto layerBoundsPath = createRectanglePath (getLayerContentBounds (layer, ctx.comp.size));

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

    const float frameNo = ctx.frameNo;
    g.setTransform (group.transform.toAffineTransform (frameNo).followedBy (g.getTransform()));
    opacity *= group.transform.opacityAt (frameNo);
    if (opacity <= 0.0f)
        return;

    // Lottie draw-order: collect shapes, then when paint is encountered, apply it
    // Children are ordered: shapes first, then paints — we gather paths then paint.
    // Groups inside iterate recursively.

    // Collect trim and repeater modifiers (they affect all shapes in this scope)
    const AnimationTrim* activeTrim = nullptr;
    const AnimationRepeater* activeRepeater = nullptr;
    const AnimationRoundedCorner* activeRoundedCorner = parentRoundedCorner;

    for (const auto& child : group.children)
    {
        if (child.kind == AnimationGroup::ChildKind::Trim && child.trim != nullptr)
            activeTrim = child.trim.get();
        if (child.kind == AnimationGroup::ChildKind::Repeater && child.repeater != nullptr)
            activeRepeater = child.repeater.get();
        if (child.kind == AnimationGroup::ChildKind::RoundedCorner && child.roundedCorner != nullptr)
            activeRoundedCorner = child.roundedCorner.get(); // local overrides parent
    }

    auto preparePaths = [&] (const std::vector<Path>& sourcePaths)
    {
        std::vector<Path> paths = sourcePaths;

        if (activeRoundedCorner != nullptr && ! activeRoundedCorner->hidden)
        {
            const float radiusRatio = activeRoundedCorner->radiusAt (frameNo);
            if (radiusRatio > 1e-5f)
            {
                for (auto& path : paths)
                {
                    auto bounds = path.getBounds();
                    const float minDim = jmin (bounds.getWidth(), bounds.getHeight());
                    const float cornerRadius = minDim * radiusRatio * 0.5f;
                    if (cornerRadius > 1e-5f)
                        path = path.withRoundedCorners (cornerRadius);
                }
            }
        }

        if (activeTrim != nullptr && ! activeTrim->hidden)
        {
            if (activeTrim->mode == AnimationTrim::TrimMode::Simultaneously)
            {
                for (auto& path : paths)
                    applyTrim (path, *activeTrim, frameNo);
            }
            else
            {
                applyTrimIndividually (paths, *activeTrim, frameNo);
            }
        }

        if (activeRepeater == nullptr || activeRepeater->hidden)
            return paths;

        const int numCopies = activeRepeater->copiesAt (frameNo);

        // The path is still in group-local coordinates here; Graphics applies the
        // group's transform later. Keep repeater copies in that same local space.
        const AnimationTransform& xf = activeRepeater->copyTransform;
        const Point<float> anchor = xf.anchor.getValueAt (frameNo);
        const Size<float> sc = xf.scale.getValueAt (frameNo);
        const float rot = xf.rotation.getValueAt (frameNo);
        const Point<float> pos = xf.separatePosition
                                   ? Point<float> { xf.positionX.getValueAt (frameNo), xf.positionY.getValueAt (frameNo) }
                                   : xf.position.getValueAt (frameNo);

        auto buildCopyTransform = [&] (float multiplier) -> AffineTransform
        {
            const float sx = std::pow (sc.getWidth() / 100.0f, multiplier);
            const float sy = std::pow (sc.getHeight() / 100.0f, multiplier);
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
            for (const auto& p : paths)
                repeatedPaths.push_back (p.transformed (copyXfm));
        }
        return repeatedPaths;
    };

    std::vector<Path> currentPaths;
    for (const auto& child : group.children)
    {
        if (child.kind == AnimationGroup::ChildKind::Shape && child.shape != nullptr)
        {
            if (! child.shape->isHidden())
                currentPaths.push_back (child.shape->buildPath (frameNo));
        }
        else if (child.kind == AnimationGroup::ChildKind::Stroke && child.stroke != nullptr)
        {
            if (! child.stroke->hidden)
            {
                Path combinedPath;
                for (const auto& path : preparePaths (currentPaths))
                    combinedPath.appendPath (path);

                applyStroke (g, combinedPath, *child.stroke, ctx, opacity);
            }
        }
        else if (child.kind == AnimationGroup::ChildKind::Fill && child.fill != nullptr)
        {
            if (! child.fill->hidden)
            {
                Path combinedPath;
                for (const auto& path : preparePaths (currentPaths))
                    combinedPath.appendPath (path);

                applyFill (g, combinedPath, *child.fill, ctx, opacity);
            }
        }
        else if (child.kind == AnimationGroup::ChildKind::Group && child.group != nullptr)
        {
            renderGroup (g, *child.group, ctx, opacity, activeRoundedCorner);
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

    const float frameNo = ctx.frameNo;
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

    const float frameNo = ctx.frameNo;
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
        // Convert Lottie dash format to interleaved dash/gap array.
        // Lottie: [dash, gap, dash, gap, ...] + optional offset.
        // If even length, copy last dash as gap per Lottie winding spec.
        std::vector<float> dashValues;
        for (const auto& d : stroke.dashArray)
            dashValues.push_back (d.value.getValueAt (frameNo));

        float dashOffset = 0.0f;
        if (dashValues.size() > 1)
        {
            if ((dashValues.size() % 2) == 0)
            {
                // Even length: copy last dash value as gap, then move offset
                dashOffset = dashValues.back();
                const float lastDash = dashValues[dashValues.size() - 2];
                dashValues.back() = lastDash;
                dashValues.push_back (dashOffset);
            }
            else
            {
                // Odd length: last value is offset
                dashOffset = dashValues.back();
                dashValues.pop_back();
            }

            // Create dashed path
            Array<float> dashArray;
            for (float v : dashValues)
            {
                if (v > 0.0f)
                    dashArray.add (v);
            }

            if (! dashArray.isEmpty())
            {
                // Ensure even dash count by duplicating if odd
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
                    // Apply dash offset
                    if (dashOffset != 0.0f)
                        dashOffset = std::fmod (std::abs (dashOffset), totalLen);

                    int dashIdx = 0;
                    float patternPos = dashOffset;
                    while (patternPos >= dashArray[dashIdx])
                    {
                        patternPos -= dashArray[dashIdx];
                        dashIdx = (dashIdx + 1) % dashArray.size();
                    }

                    // Walk path and build dashed version
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
