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
/** A named time marker within an animation composition. */
struct AnimationMarker
{
    String comment;
    float startFrame = 0.0f;
    float duration = 0.0f;
};

//==============================================================================
/** An asset referenced by layers — either a precomp (nested composition) or an image. */
struct AnimationAsset : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<AnimationAsset>;

    enum class AssetType
    {
        Precomp,
        Image
    };

    AnimationAsset() = default;

    AssetType assetType = AssetType::Image;
    String id;

    // Image assets
    String path;
    std::optional<Image> bitmap;
    int width = 0;
    int height = 0;

    // Precomp assets — own a set of layers
    std::vector<AnimationLayer::Ptr> layers;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationAsset)
};

//==============================================================================
/** Root container for a Lottie animation composition.

    Owns the full layer tree, asset table, and global timing metadata.
    The class is reference-counted so it can be safely shared between the
    parser, renderer, and any application-level caches.
*/
class YUP_API AnimationComposition : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<AnimationComposition>;

    //==============================================================================
    /** Creates an empty composition with the given size and frame rate. */
    [[nodiscard]] static AnimationComposition::Ptr create (Size<float> size, float frameRate = 60.0f);

    //==============================================================================
    // Global metadata
    String name;
    String version { "5.5.7" };
    Size<float> size { 500.0f, 500.0f };
    float frameRate = 60.0f;
    float startFrame = 0.0f;
    float endFrame = 60.0f;

    //==============================================================================
    /** Layers in draw order — index 0 is the top-most layer in Lottie's stack
        (rendered last to appear on top). */
    std::vector<AnimationLayer::Ptr> layers;

    /** Named assets keyed by id. */
    HashMap<String, AnimationAsset::Ptr> assets;

    /** Markers (named time ranges). */
    std::vector<AnimationMarker> markers;

    //==============================================================================
    [[nodiscard]] float totalFrames() const noexcept { return endFrame - startFrame; }

    [[nodiscard]] float duration() const noexcept { return frameRate > 0.0f ? totalFrames() / frameRate : 0.0f; }

    /** Maps normalised position [0, 1] to a frame number. */
    [[nodiscard]] float frameAtProgress (float p) const noexcept;

    /** Maps time in seconds to a frame number. */
    [[nodiscard]] float frameAtTime (float timeSeconds) const noexcept;

    /** Returns the first marker with the given name, or nullptr. */
    [[nodiscard]] const AnimationMarker* findMarker (const String& markerName) const noexcept;

    /** Returns the layer with the given id, or nullptr. */
    [[nodiscard]] AnimationLayer* findLayerById (int id) const noexcept;

    //==============================================================================
    /** Appends a new ShapeLayer and returns a raw pointer. */
    ShapeLayer* addShapeLayer (const String& layerName = {});

    /** Appends a new NullLayer and returns a raw pointer. */
    NullLayer* addNullLayer (const String& layerName = {});

    /** Appends a new SolidLayer and returns a raw pointer. */
    SolidLayer* addSolidLayer (const String& layerName, Color color, Size<float> sz);

    /** Appends any already-constructed layer. */
    void addLayer (AnimationLayer::Ptr layer);

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationComposition)

private:
    AnimationComposition() = default;
};

} // namespace yup
