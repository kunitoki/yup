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
/** Morphable bezier path data in Lottie wire format.

    Stores anchor vertices with in/out tangents stored as relative offsets.
    Supports linear interpolation between two shapes for path morphing.
    Converts to yup::Path via toPath().
*/
struct AnimationPathData
{
    std::vector<Point<float>> vertices;    ///< Anchor points
    std::vector<Point<float>> inTangents;  ///< In tangents relative to vertex
    std::vector<Point<float>> outTangents; ///< Out tangents relative to vertex
    bool closed = false;

    //==============================================================================
    /** Returns a yup::Path built from this data. */
    [[nodiscard]] Path toPath() const;

    /** Linearly interpolates between two AnimationPathData values vertex-by-vertex. */
    [[nodiscard]] static AnimationPathData lerp (const AnimationPathData& a,
                                                 const AnimationPathData& b,
                                                 float t);

    bool operator== (const AnimationPathData&) const noexcept;
    bool operator!= (const AnimationPathData&) const noexcept;

    // Required by AnimationLerp<AnimationPathData>
    AnimationPathData operator+ (const AnimationPathData& o) const;
    AnimationPathData operator- (const AnimationPathData& o) const;
    AnimationPathData operator* (float scalar) const;
};

using PathDataProperty = AnimationProperty<AnimationPathData>;

//==============================================================================
// AnimationLerp specialization for path morphing

template <>
struct AnimationLerp<AnimationPathData>
{
    [[nodiscard]] static AnimationPathData lerp (const AnimationPathData& a,
                                                 const AnimationPathData& b,
                                                 float t)
    {
        return AnimationPathData::lerp (a, b, t);
    }
};

} // namespace yup
