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
    renderComposition (g, comp, frameNo, bounds, keepAspectRatio, 1.0f);
}

void AnimationRenderer::renderComposition (Graphics& g,
                                           const AnimationComposition& comp,
                                           float frameNo,
                                           Rectangle<float> bounds,
                                           bool keepAspectRatio,
                                           float opacity)
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
    const auto transformedViewportClip = viewportClip.transformed (clipTransform);
    const auto savedTransform = g.getTransform();

    g.setTransform (AffineTransform::identity());
    g.setClipPath (transformedViewportClip);
    g.setTransform (savedTransform);

    RenderContext ctx { comp, frameNo, viewXf, opacity };
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

    auto saveState = g.saveState();

    // Apply layer transform
    const AffineTransform xf = ctx.resolveLayerTransform (layer);
    g.setTransform (xf.followedBy (g.getTransform()));

    // Apply alpha matte clip (tt=1 only — inverted matte requires offscreen compositing)
    if (matteSource != nullptr && layer.matteType == AnimationLayer::MatteType::Alpha)
        applyMatteSourceClip (g, *matteSource, ctx);

    // Apply shape masks as clip
    if (! layer.masks.empty())
        applyMasks (g, layer, ctx.frameNo);

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

        renderGroup (g, *group, ctx.frameNo, opacity);
    }
}

void AnimationRenderer::renderSolidLayer (Graphics& g, const SolidLayer& layer, const RenderContext& ctx, float opacity)
{
    (void) ctx;

    g.setFillColor (layer.solidColor.withMultipliedAlpha (opacity));
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

    const float localFrame = layer.localFrame (ctx.frameNo);
    const Rectangle<float> precompBounds (0.0f, 0.0f, layer.layerSize.getWidth(), layer.layerSize.getHeight());

    // Build a temporary composition from the precomp asset and render it
    auto tempComp = AnimationComposition::create (layer.layerSize, ctx.comp.frameRate);
    tempComp->layers = asset->layers;
    tempComp->assets = ctx.comp.assets;
    tempComp->startFrame = ctx.comp.startFrame;
    tempComp->endFrame = ctx.comp.endFrame;

    renderComposition (g, *tempComp, localFrame, precompBounds, false, opacity);
}

//==============================================================================

void AnimationRenderer::applyMasks (Graphics& g, const AnimationLayer& layer, float frameNo)
{
    if (layer.masks.empty())
        return;

    // Build a combined clip path from all masks
    Path clipPath;
    bool first = true;

    for (const auto& mask : layer.masks)
    {
        if (mask == nullptr)
            continue;

        Path maskPath = mask->shapeAt (frameNo);
        if (mask->inverted)
        {
            // Inverted mask: use the bounding-box complement
            // (approximate: not implemented for non-rectangular compositors)
        }

        if (first)
        {
            clipPath = maskPath;
            first = false;
        }
        else
        {
            switch (mask->mode)
            {
                case AnimationMask::Mode::Add:
                    clipPath.appendPath (maskPath);
                    break;
                case AnimationMask::Mode::Subtract:
                case AnimationMask::Mode::Intersect:
                case AnimationMask::Mode::Difference:
                case AnimationMask::Mode::None:
                    break;
            }
        }
    }

    if (! first)
    {
        const auto clipTransform = g.getTransform().translated (g.getDrawingArea().getTopLeft());
        const auto transformedClipPath = clipPath.transformed (clipTransform);
        const auto savedTransform = g.getTransform();

        g.setTransform (AffineTransform::identity());
        g.setClipPath (transformedClipPath);
        g.setTransform (savedTransform);
    }
}

//==============================================================================

void AnimationRenderer::applyMatteSourceClip (Graphics& g,
                                              const AnimationLayer& matteSource,
                                              const RenderContext& ctx)
{
    // Only shape layers are supported as matte sources here — other types
    // (precomp, image) require offscreen compositing not yet available.
    if (matteSource.getType() != AnimationLayer::Type::Shape)
        return;

    const auto& shapeLayer = static_cast<const ShapeLayer&> (matteSource);

    Path clipPath;
    for (const auto& group : shapeLayer.groups)
    {
        if (group == nullptr || group->hidden)
            continue;
        for (const auto& child : group->children)
        {
            if (child.kind == AnimationGroup::ChildKind::Shape
                && child.shape != nullptr
                && ! child.shape->isHidden())
            {
                clipPath.appendPath (child.shape->buildPath (ctx.frameNo));
            }
        }
    }

    if (clipPath.isEmpty())
        return;

    // Transform the matte source paths into device space using the source layer's
    // own resolved transform (not the target's current transform in g).
    const AffineTransform matteXf = ctx.resolveLayerTransform (matteSource);
    const auto offset = g.getDrawingArea().getTopLeft();
    const auto clipTransform = matteXf.translated ((float) offset.getX(), (float) offset.getY());
    const auto transformedClipPath = clipPath.transformed (clipTransform);
    const auto savedTransform = g.getTransform();

    g.setTransform (AffineTransform::identity());
    g.setClipPath (transformedClipPath);
    g.setTransform (savedTransform);
}

//==============================================================================

void AnimationRenderer::renderGroup (Graphics& g,
                                     const AnimationGroup& group,
                                     float frameNo,
                                     float opacity,
                                     const AnimationRoundedCorner* parentRoundedCorner)
{
    if (group.hidden)
        return;

    auto saveState = g.saveState();

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

        // Per-copy transform matching rlottie: position and rotation scale linearly
        // with the copy index, while scale scales exponentially (pow(s, copyIndex)).
        const AnimationTransform& xf = activeRepeater->copyTransform;
        const Point<float> anchor = xf.anchor.getValueAt (frameNo);
        const Size<float> sc = xf.scale.getValueAt (frameNo);
        const float rot = xf.rotation.getValueAt (frameNo);
        const Point<float> pos = xf.separatePosition
                                   ? Point<float> { xf.positionX.getValueAt (frameNo), xf.positionY.getValueAt (frameNo) }
                                   : xf.position.getValueAt (frameNo);

        auto buildCopyTransform = [&] (int copyIdx) -> AffineTransform
        {
            const float m = static_cast<float> (copyIdx);
            const float sx = std::pow (sc.getWidth() / 100.0f, m);
            const float sy = std::pow (sc.getHeight() / 100.0f, m);
            AffineTransform t;
            t = t.translated (-anchor.getX(), -anchor.getY());
            t = t.rotated (degreesToRadians (rot * m));
            t = t.scaled (sx, sy);
            t = t.translated (anchor.getX(), anchor.getY());
            t = t.translated (pos.getX() * m, pos.getY() * m);
            return t;
        };

        std::vector<Path> repeatedPaths;
        for (int copy = 0; copy < numCopies; ++copy)
        {
            const float t = (numCopies > 1) ? static_cast<float> (copy) / static_cast<float> (numCopies - 1) : 0.0f;
            const float copyOpacity = activeRepeater->startOpacityAt (frameNo) * (1.0f - t)
                                    + activeRepeater->endOpacityAt (frameNo) * t;
            (void) copyOpacity; // TODO: per-copy opacity via Graphics state

            const AffineTransform copyXfm = buildCopyTransform (copy);
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

                applyStroke (g, combinedPath, *child.stroke, frameNo, opacity);
            }
        }
        else if (child.kind == AnimationGroup::ChildKind::Fill && child.fill != nullptr)
        {
            if (! child.fill->hidden)
            {
                Path combinedPath;
                for (const auto& path : preparePaths (currentPaths))
                    combinedPath.appendPath (path);

                applyFill (g, combinedPath, *child.fill, frameNo, opacity);
            }
        }
        else if (child.kind == AnimationGroup::ChildKind::Group && child.group != nullptr)
        {
            renderGroup (g, *child.group, frameNo, opacity, activeRoundedCorner);
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

void AnimationRenderer::applyFill (Graphics& g, const Path& path, const FillPaint& fill, float frameNo, float opacity)
{
    const float finalOpacity = opacity * fill.opacityAt (frameNo);
    if (finalOpacity <= 0.0f)
        return;

    if (fill.gradient != nullptr)
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

void AnimationRenderer::applyStroke (Graphics& g, const Path& path, const StrokePaint& stroke, float frameNo, float opacity)
{
    const float finalOpacity = opacity * stroke.opacityAt (frameNo);
    if (finalOpacity <= 0.0f)
        return;

    if (stroke.gradient != nullptr)
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
    g.strokePath (path);
}

} // namespace yup
