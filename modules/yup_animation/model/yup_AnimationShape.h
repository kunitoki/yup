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
/** Abstract base class for all Lottie shape geometry types.

    Subclasses override buildPath(frameNo) to materialise a yup::Path at a
    given composition frame, evaluating any animated properties internally.
*/
class YUP_API AnimationShape : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<AnimationShape>;

    enum class Kind
    {
        Ellipse,
        Rect,
        BezierPath,
        Polystar
    };

    virtual ~AnimationShape() = default;

    [[nodiscard]] Kind getKind() const noexcept { return kind; }

    [[nodiscard]] String getName() const { return name; }

    [[nodiscard]] bool isHidden() const noexcept { return hidden; }

    [[nodiscard]] int getDirection() const noexcept { return direction; }

    void setName (const String& name) { this->name = name; }

    void setHidden (bool hidden) { this->hidden = hidden; }

    /** Direction: 1 = clockwise (default), 3 = counter-clockwise (Lottie "d"). */
    void setDirection (int dir) { direction = dir; }

    /** Evaluates the shape geometry at the given frame and returns a yup::Path. */
    [[nodiscard]] virtual Path buildPath (float frameNo) const = 0;

protected:
    explicit AnimationShape (Kind kind)
        : kind (kind)
    {
    }

    Kind kind;
    String name;
    bool hidden = false;
    int direction = 1;

    YUP_DECLARE_NON_COPYABLE (AnimationShape)
};

//==============================================================================
/** Animated ellipse shape. Lottie type "el". */
class YUP_API EllipseShape : public AnimationShape
{
public:
    using Ptr = ReferenceCountedObjectPtr<EllipseShape>;

    EllipseShape()
        : AnimationShape (Kind::Ellipse)
    {
    }

    Vec2Property center { Vec2Property::staticValue ({ 0.0f, 0.0f }) };
    SizeProperty size { SizeProperty::staticValue ({ 100.0f, 100.0f }) };

    [[nodiscard]] Path buildPath (float frameNo) const override;
};

//==============================================================================
/** Animated rounded rectangle shape. Lottie type "rc". */
class YUP_API RectShape : public AnimationShape
{
public:
    using Ptr = ReferenceCountedObjectPtr<RectShape>;

    RectShape()
        : AnimationShape (Kind::Rect)
    {
    }

    Vec2Property position { Vec2Property::staticValue ({ 0.0f, 0.0f }) };
    SizeProperty size { SizeProperty::staticValue ({ 100.0f, 100.0f }) };
    FloatProperty roundness { FloatProperty::staticValue (0.0f) };

    [[nodiscard]] Path buildPath (float frameNo) const override;
};

//==============================================================================
/** Animated bezier path shape. Lottie type "sh". Supports path morphing. */
class YUP_API BezierPathShape : public AnimationShape
{
public:
    using Ptr = ReferenceCountedObjectPtr<BezierPathShape>;

    BezierPathShape()
        : AnimationShape (Kind::BezierPath)
    {
    }

    PathDataProperty pathData {};

    [[nodiscard]] Path buildPath (float frameNo) const override;
};

//==============================================================================
/** Animated polystar or polygon. Lottie type "sr". */
class YUP_API PolystarShape : public AnimationShape
{
public:
    using Ptr = ReferenceCountedObjectPtr<PolystarShape>;

    enum class StarType
    {
        Star = 1,
        Polygon = 2
    };

    PolystarShape()
        : AnimationShape (Kind::Polystar)
    {
    }

    StarType starType = StarType::Polygon;
    Vec2Property position { Vec2Property::staticValue ({ 0.0f, 0.0f }) };
    FloatProperty points { FloatProperty::staticValue (5.0f) };
    FloatProperty outerRadius { FloatProperty::staticValue (100.0f) };
    FloatProperty innerRadius { FloatProperty::staticValue (50.0f) };
    FloatProperty outerRoundness { FloatProperty::staticValue (0.0f) };
    FloatProperty innerRoundness { FloatProperty::staticValue (0.0f) };
    FloatProperty rotation { FloatProperty::staticValue (0.0f) };

    [[nodiscard]] Path buildPath (float frameNo) const override;
};

} // namespace yup
