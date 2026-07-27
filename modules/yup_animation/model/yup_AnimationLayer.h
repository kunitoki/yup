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
/** Abstract base class for all Lottie layer types.

    Carries all timing, transform, blend mode, matte, and mask data common
    to every layer. Subclasses hold type-specific content.
*/
class YUP_API AnimationLayer : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<AnimationLayer>;

    enum class Type : uint8_t
    {
        Precomp = 0,
        Solid = 1,
        Image = 2,
        Null = 3,
        Shape = 4,
        Text = 5
    };

    enum class MatteType : uint8_t
    {
        None = 0,
        Alpha = 1,
        AlphaInv = 2,
        Luma = 3,
        LumaInv = 4
    };

    //==============================================================================
    /** A layer-level drop shadow effect parsed from Lottie's `ADBE Drop Shadow`.

        The effect is rendered as an offset duplicate of the layer's alpha using
        the configured shadow color. Soft blurred shadows are not modelled yet;
        a non-zero softness value is retained so renderers can add blur support
        without changing the parsed data model.
    */
    struct DropShadow
    {
        /** Shadow color. */
        ColorProperty color { ColorProperty::staticValue (Color (0xFF000000)) };

        /** Shadow opacity in Lottie's 0-100 range. */
        FloatProperty opacity { FloatProperty::staticValue (100.0f) };

        /** Shadow direction in degrees. */
        FloatProperty direction { FloatProperty::staticValue (0.0f) };

        /** Shadow distance in layer-space pixels. */
        FloatProperty distance { FloatProperty::staticValue (0.0f) };

        /** Shadow softness in pixels. Currently stored but rendered as a hard shadow. */
        FloatProperty softness { FloatProperty::staticValue (0.0f) };

        /** Whether only the generated shadow should be rendered. */
        bool shadowOnly = false;

        /** Whether the effect is enabled. */
        bool enabled = true;

        /** Returns the shadow opacity in [0, 1] at the given frame. */
        [[nodiscard]] float opacityAt (float frameNo) const;

        /** Returns the layer-space shadow offset at the given frame. */
        [[nodiscard]] Point<float> offsetAt (float frameNo) const;
    };

    /** A layer-level Fill effect parsed from Lottie's `ADBE Fill`.

        The effect recolors the rendered layer content. Partial effect opacity is
        retained as alpha on the override color; exact blending with the original
        layer content is renderer-dependent.
    */
    struct FillEffect
    {
        /** Replacement color. */
        ColorProperty color { ColorProperty::staticValue (Color (0xFF000000)) };

        /** Effect opacity. Some Lottie exporters encode this as 0-1, others as 0-100. */
        FloatProperty opacity { FloatProperty::staticValue (1.0f) };

        /** Whether the effect is enabled. */
        bool enabled = true;

        /** Returns the effect opacity in [0, 1] at the given frame. */
        [[nodiscard]] float opacityAt (float frameNo) const;

        /** Returns the replacement color at the given frame. */
        [[nodiscard]] Color colorAt (float frameNo) const;
    };

    //==============================================================================
    virtual ~AnimationLayer() = default;

    [[nodiscard]] virtual Type getType() const noexcept = 0;

    //==============================================================================
    // Identity and timing
    String name;
    int id = -1;              ///< "ind"
    int parentId = -1;        ///< "parent" - -1 means no parent
    float inFrame = 0.0f;     ///< "ip"
    float outFrame = 0.0f;    ///< "op"
    float startFrame = 0.0f;  ///< "st" - local time offset
    float timeStretch = 1.0f; ///< "sr"

    // Optional time remapping (overrides startFrame when set)
    std::optional<FloatProperty> timeRemap;
    bool timeRemapLoopOutCycle = false;

    // Visual state
    AnimationTransform transform;
    BlendMode blendMode = BlendMode::SrcOver;
    MatteType matteType = MatteType::None; ///< "tt" - this layer is the matte target
    bool isMatteSource = false;            ///< "td" - this layer is the matte source (not rendered directly)
    bool is3D = false;
    bool hidden = false;
    bool autoOrient = false;

    /** Masks applied to this layer. */
    std::vector<AnimationMask::Ptr> masks;

    /** Optional drop shadow effect applied to this layer. */
    std::optional<DropShadow> dropShadow;

    /** Optional Fill effect applied to this layer. */
    std::optional<FillEffect> fillEffect;

    //==============================================================================
    /** Returns true when all masks on this layer are static (not animated). */
    [[nodiscard]] bool areAllMasksStatic() const noexcept;

    //==============================================================================
    /** Maps a composition frame number to the layer's local frame, accounting
        for startFrame, timeStretch, and optional timeRemap. */
    [[nodiscard]] float localFrame (float compFrame) const noexcept;

    /** Maps a composition frame number to the layer's local frame.

        Lottie time-remap (`tm`) values are encoded in seconds, so @p frameRate
        is used to convert remapped time values back into local frame numbers.
        Layers without time remap use the same startFrame/timeStretch mapping
        as localFrame(float).
    */
    [[nodiscard]] float localFrame (float compFrame, float frameRate) const noexcept;

    /** Returns true when this layer is visible at the given composition frame. */
    [[nodiscard]] bool isVisibleAt (float compFrame) const noexcept;

    YUP_DECLARE_NON_COPYABLE (AnimationLayer)

protected:
    AnimationLayer() = default;

private:
    friend class AnimationRenderer;
    mutable std::optional<Path> cachedMaskClipPath;
    mutable float cachedMaskFrameNo = -1.0f;
};

//==============================================================================
/** A null (transform-only) layer. Useful as a parent for layer parenting. */
class YUP_API NullLayer : public AnimationLayer
{
public:
    using Ptr = ReferenceCountedObjectPtr<NullLayer>;

    NullLayer() = default;

    [[nodiscard]] Type getType() const noexcept override { return Type::Null; }

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NullLayer)
};

//==============================================================================
/** A solid colour fill layer. */
class YUP_API SolidLayer : public AnimationLayer
{
public:
    using Ptr = ReferenceCountedObjectPtr<SolidLayer>;

    SolidLayer() = default;

    [[nodiscard]] Type getType() const noexcept override { return Type::Solid; }

    Color solidColor;
    Size<float> layerSize { 100.0f, 100.0f };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SolidLayer)
};

//==============================================================================
/** An image layer. References an image asset by id. */
class YUP_API ImageLayer : public AnimationLayer
{
public:
    using Ptr = ReferenceCountedObjectPtr<ImageLayer>;

    ImageLayer() = default;

    [[nodiscard]] Type getType() const noexcept override { return Type::Image; }

    String assetRefId;
    std::optional<Image> image; ///< Resolved at parse time

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageLayer)
};

//==============================================================================
/** A pre-composition layer embedding another composition by asset reference. */
class YUP_API PrecompLayer : public AnimationLayer
{
public:
    using Ptr = ReferenceCountedObjectPtr<PrecompLayer>;

    PrecompLayer() = default;

    [[nodiscard]] Type getType() const noexcept override { return Type::Precomp; }

    String precompRefId;
    Size<float> layerSize { 0.0f, 0.0f };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrecompLayer)
};

} // namespace yup
