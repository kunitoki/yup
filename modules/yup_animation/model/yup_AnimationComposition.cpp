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

AnimationComposition::Ptr AnimationComposition::create (Size<float> compSize, float fps)
{
    AnimationComposition::Ptr comp = new AnimationComposition();
    comp->size = compSize;
    comp->frameRate = fps;
    return comp;
}

float AnimationComposition::frameAtProgress (float p) const noexcept
{
    return startFrame + jlimit (0.0f, 1.0f, p) * totalFrames();
}

float AnimationComposition::frameAtTime (float timeSeconds) const noexcept
{
    return startFrame + timeSeconds * frameRate;
}

const AnimationMarker* AnimationComposition::findMarker (const String& markerName) const noexcept
{
    for (const auto& m : markers)
        if (m.comment == markerName)
            return &m;
    return nullptr;
}

AnimationLayer* AnimationComposition::findLayerById (int layerId) const noexcept
{
    for (const auto& l : layers)
        if (l->id == layerId)
            return l.get();
    return nullptr;
}

void AnimationComposition::addLayer (AnimationLayer::Ptr layer)
{
    layers.push_back (std::move (layer));
}

ShapeLayer* AnimationComposition::addShapeLayer (const String& layerName)
{
    auto layer = new ShapeLayer();
    layer->name = layerName;
    layer->inFrame = startFrame;
    layer->outFrame = endFrame;
    layer->id = static_cast<int> (layers.size()) + 1;
    ShapeLayer* raw = layer;
    layers.push_back (layer);
    return raw;
}

NullLayer* AnimationComposition::addNullLayer (const String& layerName)
{
    auto layer = new NullLayer();
    layer->name = layerName;
    layer->inFrame = startFrame;
    layer->outFrame = endFrame;
    layer->id = static_cast<int> (layers.size()) + 1;
    NullLayer* raw = layer;
    layers.push_back (layer);
    return raw;
}

SolidLayer* AnimationComposition::addSolidLayer (const String& layerName,
                                                 Color color,
                                                 Size<float> sz)
{
    auto layer = new SolidLayer();
    layer->name = layerName;
    layer->solidColor = color;
    layer->layerSize = sz;
    layer->inFrame = startFrame;
    layer->outFrame = endFrame;
    layer->id = static_cast<int> (layers.size()) + 1;
    SolidLayer* raw = layer;
    layers.push_back (layer);
    return raw;
}

PropertyOverrideSet* AnimationComposition::getPropertyOverride (const String& keyPath)
{
    if (propertyOverrides.contains (keyPath))
        return &propertyOverrides.getReference (keyPath);
    return nullptr;
}

AnimationComposition::Stats AnimationComposition::computeStats() const noexcept
{
    Stats s;
    for (const auto& layer : layers)
    {
        if (layer == nullptr)
            continue;
        switch (layer->getType())
        {
            case AnimationLayer::Type::Precomp:
                ++s.precompLayerCount;
                break;
            case AnimationLayer::Type::Solid:
                ++s.solidLayerCount;
                break;
            case AnimationLayer::Type::Image:
                ++s.imageLayerCount;
                break;
            case AnimationLayer::Type::Null:
                ++s.nullLayerCount;
                break;
            case AnimationLayer::Type::Shape:
                ++s.shapeLayerCount;
                break;
            case AnimationLayer::Type::Text:
                ++s.textLayerCount;
                break;
        }
    }
    return s;
}

} // namespace yup
