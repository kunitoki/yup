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
/** Top-level handle for a Lottie animation.

    `Animation` wraps an `AnimationComposition` and provides a convenient,
    high-level API for loading, rendering, and serialising animations.

    Loading:
    @code
    auto anim = Animation::loadFromFile (File ("/path/to/animation.json"));
    if (anim.isValid())
        anim.renderAtProgress (g, 0.5f, bounds);
    @endcode

    Building programmatically:
    @code
    auto comp = AnimationComposition::create ({800.0f, 600.0f}, 25.0f);
    auto* layer = comp->addShapeLayer ("bg");
    auto* group = layer->addGroup ("shapes");
    auto* ellipse = group->addShape<EllipseShape>();
    ellipse->size = Vec2Property::staticValue ({200.0f, 200.0f});
    auto* fill = group->addFill();
    fill->color = ColorProperty::staticValue (Colours::red);

    auto anim = Animation::fromComposition (comp);
    @endcode
*/
class YUP_API Animation
{
public:
    //==============================================================================
    /** Options passed to loading functions. */
    struct LoadOptions
    {
        /** Directory used to resolve relative image asset paths. */
        File resourceDirectory;
    };

    //==============================================================================
    Animation() = default;
    explicit Animation (AnimationComposition::Ptr comp);

    //==============================================================================
    /** Returns true when this object holds a valid composition. */
    [[nodiscard]] bool isValid() const noexcept;

    /** Returns the underlying composition (may be nullptr). */
    [[nodiscard]] AnimationComposition* getComposition() const noexcept;

    //==============================================================================
    /** Loads a Lottie `.json` or `.lottie` (ZIP) file. */
    [[nodiscard]] static Animation loadFromFile (const File& file, const LoadOptions& opts = {});

    /** Loads a Lottie animation from a JSON string. */
    [[nodiscard]] static Animation loadFromData (const String& jsonText, const LoadOptions& opts = {});

    /** Loads a Lottie animation from an InputStream.
        The stream is fully consumed. Both plain JSON and .lottie ZIP streams are supported.
        Returns an invalid Animation on failure. */
    [[nodiscard]] static Animation loadFromStream (InputStream& stream, const LoadOptions& opts = {});

    /** Wraps an already-constructed AnimationComposition. */
    [[nodiscard]] static Animation fromComposition (AnimationComposition::Ptr comp);

    //==============================================================================
    /** Returns the total number of frames. */
    [[nodiscard]] float totalFrames() const noexcept;

    /** Returns the frame rate in frames per second. */
    [[nodiscard]] float frameRate() const noexcept;

    /** Returns the total duration in seconds. */
    [[nodiscard]] float duration() const noexcept;

    /** Returns the composition size (logical pixels). */
    [[nodiscard]] Size<float> size() const noexcept;

    //==============================================================================
    /** Renders the given frame into @p g, fitted inside @p bounds.
        @param keepAspectRatio  When true, pillar/letter-boxes to preserve the aspect ratio.
    */
    void renderFrame (Graphics& g,
                      float frameNo,
                      Rectangle<float> bounds,
                      bool keepAspectRatio = true) const;

    /** Renders the frame corresponding to @p timeInSeconds. */
    void renderAtTime (Graphics& g,
                       float timeInSeconds,
                       Rectangle<float> bounds,
                       bool keepAspectRatio = true) const;

    /** Renders the frame at a normalised progress in [0, 1]. */
    void renderAtProgress (Graphics& g,
                           float progress,
                           Rectangle<float> bounds,
                           bool keepAspectRatio = true) const;

    //==============================================================================
    /** Serialises the composition to a Lottie JSON string. */
    [[nodiscard]] String toJson (bool prettyPrint = true) const;

    /** Saves the composition to a Lottie JSON file. */
    Result saveToFile (const File& destination, bool prettyPrint = true) const;

private:
    AnimationComposition::Ptr composition_;
};

} // namespace yup
