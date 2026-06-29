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
/** Path trim modifier. Lottie type "tm".

    Trims the rendered portion of paths. Start/end/offset are 0-100 percentages.
*/
class YUP_API AnimationTrim : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<AnimationTrim>;

    enum class TrimMode
    {
        Simultaneously,
        Individually
    };

    //==============================================================================
    AnimationTrim() = default;

    //==============================================================================
    String name;
    bool hidden = false;
    TrimMode mode = TrimMode::Simultaneously;
    FloatProperty start { FloatProperty::staticValue (0.0f) };  ///< 0-100
    FloatProperty end { FloatProperty::staticValue (100.0f) };  ///< 0-100
    FloatProperty offset { FloatProperty::staticValue (0.0f) }; ///< degrees

    //==============================================================================
    struct TrimSegment
    {
        float start;
        float end;
    }; ///< Both in [0, 1]

    /** Returns the trim segment [start, end] in [0, 1] range at the given frame. */
    [[nodiscard]] TrimSegment getSegment (float frameNo) const;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationTrim)
};

//==============================================================================
/** Repeater modifier. Lottie type "rp".

    Produces multiple copies of shape content with an incrementally applied transform.
*/
class YUP_API AnimationRepeater : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<AnimationRepeater>;

    //==============================================================================
    AnimationRepeater() = default;

    //==============================================================================
    String name;
    bool hidden = false;
    FloatProperty copies { FloatProperty::staticValue (1.0f) };
    FloatProperty offset { FloatProperty::staticValue (0.0f) };

    /** Pre-computed maximum copy count across all keyframes (for buffer sizing). */
    float maxCopies = 1.0f;

    /** Per-copy transform increment applied additively to each copy. */
    AnimationTransform copyTransform;

    /** Opacity of the first copy (0-100). */
    FloatProperty startOpacity { FloatProperty::staticValue (100.0f) };
    /** Opacity of the last copy (0-100). */
    FloatProperty endOpacity { FloatProperty::staticValue (100.0f) };

    //==============================================================================
    [[nodiscard]] int copiesAt (float frameNo) const;
    [[nodiscard]] float offsetAt (float frameNo) const;
    [[nodiscard]] float startOpacityAt (float frameNo) const;
    [[nodiscard]] float endOpacityAt (float frameNo) const;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationRepeater)
};

//==============================================================================
/** Layer mask. Part of "masksProperties" in Lottie JSON. */
class YUP_API AnimationMask : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<AnimationMask>;

    enum class Mode
    {
        None,
        Add,
        Subtract,
        Intersect,
        Difference
    };

    //==============================================================================
    AnimationMask() = default;

    //==============================================================================
    String name;
    bool inverted = false;
    Mode mode = Mode::Add;
    PathDataProperty shape {};
    FloatProperty opacity { FloatProperty::staticValue (100.0f) };

    //==============================================================================
    [[nodiscard]] Path shapeAt (float frameNo) const;
    [[nodiscard]] float opacityAt (float frameNo) const; ///< Returns [0, 1]

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationMask)
};

//==============================================================================
/** Rounded corner modifier. Lottie type "rd".

    Applies rounded corners to the adjacent rect and polystar shapes.
    The radius is a 0-1 value that scales with the shape dimensions.
*/
class YUP_API AnimationRoundedCorner : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<AnimationRoundedCorner>;

    //==============================================================================
    AnimationRoundedCorner() = default;

    //==============================================================================
    String name;
    bool hidden = false;
    FloatProperty radius { FloatProperty::staticValue (0.0f) }; ///< 0-1 ratio

    //==============================================================================
    [[nodiscard]] float radiusAt (float frameNo) const;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationRoundedCorner)
};

} // namespace yup
