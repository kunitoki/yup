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
// AnimationPathData

Path AnimationPathData::toPath() const
{
    Path p;

    const int n = static_cast<int> (vertices.size());
    if (n == 0)
        return p;

    p.moveTo (vertices[0]);

    for (int i = 0; i < n - 1; ++i)
    {
        const Point<float> cp1 = vertices[i] + outTangents[i];
        const Point<float> cp2 = vertices[i + 1] + inTangents[i + 1];
        p.cubicTo (cp1, cp2.getX(), cp2.getY(), vertices[i + 1].getX(), vertices[i + 1].getY());
    }

    if (closed && n > 1)
    {
        const Point<float> cp1 = vertices[n - 1] + outTangents[n - 1];
        const Point<float> cp2 = vertices[0] + inTangents[0];
        p.cubicTo (cp1, cp2.getX(), cp2.getY(), vertices[0].getX(), vertices[0].getY());
        p.close();
    }

    return p;
}

AnimationPathData AnimationPathData::lerp (const AnimationPathData& a,
                                           const AnimationPathData& b,
                                           float t)
{
    AnimationPathData result;
    result.closed = a.closed;

    const int n = static_cast<int> (a.vertices.size());
    if (n != static_cast<int> (b.vertices.size()))
        return a;

    result.vertices.resize (static_cast<size_t> (n));
    result.inTangents.resize (static_cast<size_t> (n));
    result.outTangents.resize (static_cast<size_t> (n));

    for (int i = 0; i < n; ++i)
    {
        result.vertices[i] = AnimationLerp<Point<float>>::lerp (a.vertices[i], b.vertices[i], t);
        result.inTangents[i] = AnimationLerp<Point<float>>::lerp (a.inTangents[i], b.inTangents[i], t);
        result.outTangents[i] = AnimationLerp<Point<float>>::lerp (a.outTangents[i], b.outTangents[i], t);
    }

    return result;
}

bool AnimationPathData::operator== (const AnimationPathData& o) const noexcept
{
    return closed == o.closed && vertices == o.vertices;
}

bool AnimationPathData::operator!= (const AnimationPathData& o) const noexcept
{
    return ! (*this == o);
}

AnimationPathData AnimationPathData::operator+ (const AnimationPathData& o) const
{
    return lerp (*this, o, 1.0f);
}

AnimationPathData AnimationPathData::operator- (const AnimationPathData& o) const
{
    return lerp (o, *this, -1.0f);
}

AnimationPathData AnimationPathData::operator* (float scalar) const
{
    AnimationPathData result = *this;
    for (auto& v : result.vertices)
        v = v * scalar;
    for (auto& v : result.inTangents)
        v = v * scalar;
    for (auto& v : result.outTangents)
        v = v * scalar;
    return result;
}

} // namespace yup
