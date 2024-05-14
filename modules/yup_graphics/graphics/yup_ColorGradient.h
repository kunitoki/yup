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

namespace yup
{

//==============================================================================
class JUCE_API ColorGradient
{
public:
    //==============================================================================
    enum Type : unsigned int
    {
        Linear,
        Radial
    };

    //==============================================================================
    ColorGradient() noexcept = default;

    ColorGradient (Color color1, float x1, float y1, Color color2, float x2, float y2, Type type) noexcept
        : type (type)
        , start (color1, x1, y1, 0.0f)
        , finish (color2, x2, y2, 1.0f)
    {
        if (type == Radial)
            radius = std::sqrt (square (x2 - x1) + square (y2 - y1));
    }

    ColorGradient (const ColorGradient& other) = default;
    ColorGradient (ColorGradient&& other) = default;
    ColorGradient& operator= (const ColorGradient& other) = default;
    ColorGradient& operator= (ColorGradient&& other) = default;

    //==============================================================================
    Type getType() const noexcept
    {
        return type;
    }

    //==============================================================================
    Color getStartColor() const
    {
        return start.color;
    }

    float getStartX() const
    {
        return start.x;
    }

    float getStartY() const
    {
        return start.y;
    }

    float getStartDelta() const
    {
        return start.delta;
    }

    //==============================================================================
    Color getFinishColor() const
    {
        return finish.color;
    }

    float getFinishX() const
    {
        return finish.x;
    }

    float getFinishY() const
    {
        return finish.y;
    }

    float getFinishDelta() const
    {
        return finish.delta;
    }

    //==============================================================================
    float getRadius() const
    {
        return radius;
    }

    //==============================================================================
    void setAlpha (uint8 alpha)
    {
        start.color = start.color.withAlpha (alpha);
        finish.color = finish.color.withAlpha (alpha);
    }

    ColorGradient withAlpha (uint8 alpha) const
    {
        ColorGradient result (*this);
        result.setAlpha (alpha);
        return result;
    }

private:
    struct ColorStop
    {
        ColorStop() = default;

        ColorStop (Color color, float x, float y, float delta)
            : color (color)
            , x (x)
            , y (y)
            , delta (delta)
        {
        }

        Color color;
        float x = 0.0f;
        float y = 0.0f;
        float delta = 0.0f;
    };

    Type type = Type::Linear;
    ColorStop start;
    ColorStop finish;
    float radius = 0.0f;
};

} // namespace yup
