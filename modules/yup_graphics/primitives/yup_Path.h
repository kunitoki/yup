/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

namespace juce
{

//==============================================================================

class JUCE_API Path
{
public:
    //==============================================================================
    enum SegmentType
    {
        MoveTo,
        LineTo,
        QuadTo,
        CubicTo
    };

    //==============================================================================
    Path() noexcept = default;
    Path (float x, float y) noexcept;
    Path (const Point<float>& p) noexcept;

    //==============================================================================
    Path (const Path& other) noexcept = default;
    Path (Path&& other) noexcept = default;
    Path& operator= (const Path& other) noexcept = default;
    Path& operator= (Path&& other) noexcept = default;

    //==============================================================================
    void reserveSpace (int numSegments);
    int size() const;

    //==============================================================================
    void clear();

    //==============================================================================
    void moveTo (float x, float y);
    void moveTo (const Point<float>& p);

    //==============================================================================
    void lineTo (float x, float y);
    void lineTo (const Point<float>& p);

    //==============================================================================
    void quadTo (float x, float y, float x1, float y1);
    void quadTo (const Point<float>& p, float x1, float y1);

    //==============================================================================
    void cubicTo (float x, float y, float x1, float y1, float x2, float y2);
    void cubicTo (const Point<float>& p, float x1, float y1, float x2, float y2);

    //==============================================================================
    void close();

    //==============================================================================
    void addLine (const Point<float>& p1, const Point<float>& p2);
    void addLine (const Line<float>& line);

    //==============================================================================
    void addRectangle (float x, float y, float width, float height);
    void addRectangle (const Rectangle<float>& rect);

    //==============================================================================
    void addRoundedRectangle (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight);
    void addRoundedRectangle (const Rectangle<float>& rect, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight);

    //==============================================================================
    void addEllipse (float x, float y, float width, float height);
    void addEllipse (const Rectangle<float>& rect);

    //==============================================================================
    void addArc (float x, float y, float width, float height,
                 float fromRadians, float toRadians,
                 bool startAsNewSubPath);

    void addArc (const Rectangle<float>& rect,
                 float fromRadians, float toRadians,
                 bool startAsNewSubPath);

    void addCenteredArc (float centerX, float centerY, float radiusX, float radiusY,
                         float rotationOfEllipse, float fromRadians, float toRadians,
                         bool startAsNewSubPath);

    void addCenteredArc (const Point<float>& center, float radiusX, float radiusY,
                         float rotationOfEllipse, float fromRadians, float toRadians,
                         bool startAsNewSubPath);

    //==============================================================================
    void appendPath (const Path& other);

    //==============================================================================
    struct Segment
    {
        constexpr Segment() noexcept = default;

        constexpr Segment (SegmentType type) noexcept
            : type (type)
        {
        }

        constexpr Segment (SegmentType type, float x, float y) noexcept
            : type (type)
            , x (x)
            , y (y)
        {
        }

        constexpr Segment (SegmentType type, float x, float y, float x1, float y1) noexcept
            : type (type)
            , x (x)
            , y (y)
            , x1 (x1)
            , y1 (y1)
        {
        }

        constexpr Segment (SegmentType type, float x, float y, float x1, float y1, float x2, float y2) noexcept
            : type (type)
            , x (x)
            , y (y)
            , x1 (x1)
            , y1 (y1)
            , x2 (x2)
            , y2 (y2)
        {
        }

        SegmentType type = SegmentType::MoveTo;
        float x = 0.0f, y = 0.0f;
        float x1 = 0.0f, y1 = 0.0f;
        float x2 = 0.0f, y2 = 0.0f;
    };

    //==============================================================================
    auto begin()
    {
        return data.begin();
    }

    auto begin() const
    {
        return data.begin();
    }

    //==============================================================================
    auto end()
    {
        return data.end();
    }

    auto end() const
    {
        return data.end();
    }

private:
    std::vector<Segment> data;
    int lastSubpathIndex = -1;
};

} // namespace juce
