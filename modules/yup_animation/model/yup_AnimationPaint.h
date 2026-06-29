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
/** Animated gradient with color stops.

    Supports both linear and radial gradient types. Call toColorGradient() to
    evaluate all animated properties and produce a yup::ColorGradient for rendering.
*/
class YUP_API AnimationGradient : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<AnimationGradient>;

    enum class GradientType
    {
        Linear = 1,
        Radial = 2
    };

    struct ColorStop
    {
        FloatProperty position { FloatProperty::staticValue (0.0f) };
        ColorProperty color { ColorProperty::staticValue (Color()) };
    };

    //==============================================================================
    AnimationGradient() = default;

    //==============================================================================
    GradientType gradientType = GradientType::Linear;
    Vec2Property startPoint { Vec2Property::staticValue ({ 0.0f, 0.0f }) };
    Vec2Property endPoint { Vec2Property::staticValue ({ 0.0f, 0.0f }) };
    FloatProperty highlightLen { FloatProperty::staticValue (0.0f) };
    FloatProperty highlightAngle { FloatProperty::staticValue (0.0f) };

    std::vector<ColorStop> colorStops;

    //==============================================================================
    void addColorStop (float pos, Color color);

    /** Evaluates to a yup::ColorGradient at the given frame. */
    [[nodiscard]] ColorGradient toColorGradient (float frameNo) const;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationGradient)
};

//==============================================================================
/** Solid-color fill paint. Lottie type "fl". */
class YUP_API FillPaint : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<FillPaint>;

    enum class FillRule
    {
        NonZero = 1,
        EvenOdd = 2
    };

    //==============================================================================
    FillPaint() = default;

    //==============================================================================
    String name;
    bool hidden = false;
    FillRule fillRule = FillRule::NonZero;
    ColorProperty color { ColorProperty::staticValue (Color (0xFF000000)) };
    FloatProperty opacity { FloatProperty::staticValue (100.0f) };

    /** Optional gradient — overrides solid color when set. */
    AnimationGradient::Ptr gradient;

    //==============================================================================
    [[nodiscard]] Color colorAt (float frameNo) const;
    [[nodiscard]] float opacityAt (float frameNo) const; ///< Returns [0, 1]

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FillPaint)
};

//==============================================================================
/** Dash pattern entry used by StrokePaint. */
struct StrokeDash
{
    enum class Kind
    {
        Dash,
        Gap,
        Offset
    };
    Kind kind = Kind::Dash;
    FloatProperty value { FloatProperty::staticValue (0.0f) };
};

//==============================================================================
/** Solid-color stroke paint. Lottie type "st". */
class YUP_API StrokePaint : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<StrokePaint>;

    //==============================================================================
    StrokePaint() = default;

    //==============================================================================
    String name;
    bool hidden = false;
    ColorProperty color { ColorProperty::staticValue (Color (0xFF000000)) };
    FloatProperty opacity { FloatProperty::staticValue (100.0f) };
    FloatProperty width { FloatProperty::staticValue (2.0f) };
    StrokeCap cap = StrokeCap::Butt;
    StrokeJoin join = StrokeJoin::Miter;
    float miterLimit = 4.0f;

    /** Optional gradient — overrides solid color when set. */
    AnimationGradient::Ptr gradient;

    std::vector<StrokeDash> dashArray;

    //==============================================================================
    [[nodiscard]] Color colorAt (float frameNo) const;
    [[nodiscard]] float opacityAt (float frameNo) const; ///< Returns [0, 1]
    [[nodiscard]] float widthAt (float frameNo) const;
    [[nodiscard]] StrokeType strokeTypeAt (float frameNo) const;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StrokePaint)
};

} // namespace yup
