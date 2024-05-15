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
/** Represents a gradient for graphical use, defined by two colors and their positions.

    This class encapsulates a gradient, which can be either linear or radial, specified by two color stops.
    Each stop has a color and a position, and in the case of radial gradients, a radius is calculated.
*/
class JUCE_API ColorGradient
{
public:
    //==============================================================================
    /** Enumeration for gradient types. */
    enum Type : unsigned int
    {
        Linear, ///< A linear gradient transitions smoothly between colors along a straight line.
        Radial  ///< A radial gradient transitions smoothly between colors in a circular pattern.
    };

    //==============================================================================
    /** Default constructor, initializes an empty gradient.

        Constructs a gradient with default values, typically used as a placeholder before setting specific gradient parameters.
    */
    ColorGradient() noexcept = default;

    /** Constructs a gradient with specified attributes.

        @param color1 The starting color of the gradient.
        @param x1 The x-coordinate of the starting color.
        @param y1 The y-coordinate of the starting color.
        @param color2 The ending color of the gradient.
        @param x2 The x-coordinate of the ending color.
        @param y2 The y-coordinate of the ending color.
        @param type The type of gradient (Linear or Radial).
    */
    ColorGradient (Color color1, float x1, float y1, Color color2, float x2, float y2, Type type) noexcept
        : type (type)
        , start (color1, x1, y1, 0.0f)
        , finish (color2, x2, y2, 1.0f)
    {
        if (type == Radial)
            radius = std::sqrt (square (x2 - x1) + square (y2 - y1));
    }

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    ColorGradient (const ColorGradient& other) = default;
    ColorGradient (ColorGradient&& other) = default;
    ColorGradient& operator= (const ColorGradient& other) = default;
    ColorGradient& operator= (ColorGradient&& other) = default;

    //==============================================================================
    /** Gets the type of the gradient.

        @return The type of the gradient, either Linear or Radial.
    */
    Type getType() const noexcept
    {
        return type;
    }

    //==============================================================================
    /** Gets the starting color of the gradient.

        @return The starting color.
    */
    Color getStartColor() const
    {
        return start.color;
    }

    /** Gets the x-coordinate of the starting color position.

        @return The x-coordinate of the start position.
    */
    float getStartX() const
    {
        return start.x;
    }

    /** Gets the y-coordinate of the starting color position.

        @return The y-coordinate of the start position.
    */
    float getStartY() const
    {
        return start.y;
    }

    /** Gets the relative position of the starting color in the gradient.

        @return The relative position (delta value) of the start position, typically 0.0f for the start.
    */
    float getStartDelta() const
    {
        return start.delta;
    }

    //==============================================================================
    /** Gets the ending color of the gradient.

        @return The ending color.
    */
    Color getFinishColor() const
    {
        return finish.color;
    }

    /** Gets the x-coordinate of the ending color position.

        @return The x-coordinate of the finish position.
    */
    float getFinishX() const
    {
        return finish.x;
    }

    /** Gets the y-coordinate of the ending color position.

        @return The y-coordinate of the finish position.
    */
    float getFinishY() const
    {
        return finish.y;
    }

    /** Gets the relative position of the ending color in the gradient.

        @return The relative position (delta value) of the finish position, typically 1.0f for the end.
    */
    float getFinishDelta() const
    {
        return finish.delta;
    }

    //==============================================================================
    /** Gets the radius of the radial gradient.

        This value is only relevant for radial gradients and represents the distance from the start to the finish position.

        @return The radius of the radial gradient.
    */
    float getRadius() const
    {
        return radius;
    }

    //==============================================================================
    /** Sets the alpha value for both color stops in the gradient.

        @param alpha The alpha value to set, affecting the transparency of the entire gradient.
    */
    void setAlpha (uint8 alpha)
    {
        start.color.setAlpha (alpha);
        finish.color.setAlpha (alpha);
    }

    /** Creates a new gradient with a specified alpha value for both color stops.

        This method allows you to create a new gradient identical to the current one but with a different transparency level.

        @param alpha The alpha value for the new gradient.

        @return A new ColorGradient object with the specified alpha value.
    */
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
