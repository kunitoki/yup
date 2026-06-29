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

CubicBezier::CubicBezier (Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3)
{
    x1 = p0.getX();
    y1 = p0.getY();
    x2 = p1.getX();
    y2 = p1.getY();
    x3 = p2.getX();
    y3 = p2.getY();
    x4 = p3.getX();
    y4 = p3.getY();
}

CubicBezier CubicBezier::fromPoints (Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3)
{
    return CubicBezier (p0, p1, p2, p3);
}

Point<float> CubicBezier::pointAt (float t) const noexcept
{
    const float m_t = 1.0f - t;
    const float a = x1 * m_t + x2 * t;
    const float b = x2 * m_t + x3 * t;
    const float c = x3 * m_t + x4 * t;
    const float d = a * m_t + b * t;
    const float e = b * m_t + c * t;
    const float x = d * m_t + e * t;

    const float ay = y1 * m_t + y2 * t;
    const float by = y2 * m_t + y3 * t;
    const float cy = y3 * m_t + y4 * t;
    const float dy = ay * m_t + by * t;
    const float ey = by * m_t + cy * t;
    const float y = dy * m_t + ey * t;

    return { x, y };
}

Point<float> CubicBezier::derivative (float t) const noexcept
{
    // p'(t) = 3 * (-(1-2t+t^2)*P0   + (1-4t+3t^2)*P1   + (2t-3t^2)*P2   + t^2*P3)

    const float m_t = 1.0f - t;
    const float d = t * t;
    const float a = -m_t * m_t;
    const float b = 1.0f - 4.0f * t + 3.0f * d;
    const float c = 2.0f * t - 3.0f * d;

    return { 3.0f * (a * x1 + b * x2 + c * x3 + d * x4),
             3.0f * (a * y1 + b * y2 + c * y3 + d * y4) };
}

float CubicBezier::angleAt (float t) const noexcept
{
    if (t < 0.0f || t > 1.0f)
        return 0.0f;

    const auto d = derivative (t);
    return std::atan2 (d.getY(), d.getX());
}

float CubicBezier::length() const noexcept
{
    const float polyLen = std::sqrt ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1))
                        + std::sqrt ((x3 - x2) * (x3 - x2) + (y3 - y2) * (y3 - y2))
                        + std::sqrt ((x4 - x3) * (x4 - x3) + (y4 - y3) * (y4 - y3));

    const float chord = std::sqrt ((x4 - x1) * (x4 - x1) + (y4 - y1) * (y4 - y1));

    if ((polyLen - chord) > 0.01f)
    {
        CubicBezier left, right;
        split (left, right);
        return left.length() + right.length();
    }

    return polyLen;
}

float CubicBezier::tAtLength (float len, float totalLen) const noexcept
{
    float t = 1.0f;
    const float error = 0.01f;
    if (len > totalLen || std::abs (len - totalLen) < 1e-5f)
        return t;

    t *= 0.5f;
    float lastBigger = 1.0f;

    for (int num = 0; num < 100500; ++num)
    {
        CubicBezier right = *this;
        CubicBezier left;
        right.parameterSplitLeft (t, left);
        const float lLen = left.length();

        if (std::abs (lLen - len) < error)
            return t;

        if (lLen < len)
            t += (lastBigger - t) * 0.5f;
        else
        {
            lastBigger = t;
            t -= t * 0.5f;
        }
    }

    return t;
}

float CubicBezier::tAtLength (float len) const noexcept
{
    return tAtLength (len, length());
}

void CubicBezier::splitAtLength (float len, CubicBezier& left, CubicBezier& right) const noexcept
{
    right = *this;
    const float t = right.tAtLength (len);
    right.parameterSplitLeft (t, left);
}

void CubicBezier::split (CubicBezier& firstHalf, CubicBezier& secondHalf) const noexcept
{
    const float cx = (x2 + x3) * 0.5f;
    firstHalf.x2 = (x1 + x2) * 0.5f;
    secondHalf.x3 = (x3 + x4) * 0.5f;
    firstHalf.x1 = x1;
    secondHalf.x4 = x4;
    firstHalf.x3 = (firstHalf.x2 + cx) * 0.5f;
    secondHalf.x2 = (secondHalf.x3 + cx) * 0.5f;
    firstHalf.x4 = secondHalf.x1 = (firstHalf.x3 + secondHalf.x2) * 0.5f;

    const float cy = (y2 + y3) * 0.5f;
    firstHalf.y2 = (y1 + y2) * 0.5f;
    secondHalf.y3 = (y3 + y4) * 0.5f;
    firstHalf.y1 = y1;
    secondHalf.y4 = y4;
    firstHalf.y3 = (firstHalf.y2 + cy) * 0.5f;
    secondHalf.y2 = (secondHalf.y3 + cy) * 0.5f;
    firstHalf.y4 = secondHalf.y1 = (firstHalf.y3 + secondHalf.y2) * 0.5f;
}

void CubicBezier::parameterSplitLeft (float t, CubicBezier& left) noexcept
{
    left.x1 = x1;
    left.y1 = y1;

    left.x2 = x1 + t * (x2 - x1);
    left.y2 = y1 + t * (y2 - y1);

    left.x3 = x2 + t * (x3 - x2);
    left.y3 = y2 + t * (y3 - y2);

    const float newX3 = x3 + t * (x4 - x3);
    const float newY3 = y3 + t * (y4 - y3);

    const float newX2 = left.x3 + t * (newX3 - left.x3);
    const float newY2 = left.y3 + t * (newY3 - left.y3);

    left.x3 = left.x2 + t * (left.x3 - left.x2);
    left.y3 = left.y2 + t * (left.y3 - left.y2);

    left.x4 = x1 = left.x3 + t * (newX2 - left.x3);
    left.y4 = y1 = left.y3 + t * (newY2 - left.y3);

    x2 = newX2;
    y2 = newY2;
    x3 = newX3;
    y3 = newY3;
}

CubicBezier CubicBezier::onInterval (float t0, float t1) const noexcept
{
    if (t0 == 0.0f && t1 == 1.0f)
        return *this;

    CubicBezier bezier = *this;
    CubicBezier result;

    bezier.parameterSplitLeft (t0, result);
    const float trueT = (t1 - t0) / (1.0f - t0);
    bezier.parameterSplitLeft (trueT, result);

    return result;
}

} // namespace yup
