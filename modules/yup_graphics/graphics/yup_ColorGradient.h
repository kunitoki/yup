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
/** Represents a gradient for graphical use, defined by multiple colors and their positions.

    This class encapsulates a gradient, which can be either linear or radial, specified by color stops.
    Each stop has a color and a position, and in the case of radial gradients, a radius is calculated.
    Supports multiple color stops for complex gradients while maintaining backward compatibility.
*/
class YUP_API ColorGradient
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
    /** Represents a single color stop in a gradient. */
    struct ColorStop
    {
        constexpr ColorStop() = default;

        constexpr ColorStop (Color color, float x, float y, float delta)
            : color (color)
            , x (x)
            , y (y)
            , delta (delta)
        {
        }

        constexpr ColorStop (const ColorStop& other) noexcept = default;
        constexpr ColorStop (ColorStop&& other) noexcept = default;
        constexpr ColorStop& operator= (const ColorStop& other) noexcept = default;
        constexpr ColorStop& operator= (ColorStop&& other) noexcept = default;

        Color color;
        float x = 0.0f;
        float y = 0.0f;
        float delta = 0.0f;
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
    {
        stops.emplace_back (color1, x1, y1, 0.0f);
        stops.emplace_back (color2, x2, y2, 1.0f);
        
        if (type == Radial)
            radius = std::sqrt (square (x2 - x1) + square (y2 - y1));
    }

    /** Constructs a gradient with multiple color stops.

        @param type The type of gradient (Linear or Radial).
        @param colorStops Vector of ColorStop objects defining the gradient.
    */
    ColorGradient (Type type, const std::vector<ColorStop>& colorStops) noexcept
        : type (type)
        , stops (colorStops)
    {
        if (type == Radial && stops.size() >= 2)
        {
            const auto& first = stops.front();
            const auto& last = stops.back();
            radius = std::sqrt (square (last.x - first.x) + square (last.y - first.y));
        }
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
    /** Gets the starting color of the gradient (first color stop).

        @return The starting color.
    */
    Color getStartColor() const
    {
        return stops.empty() ? Color() : stops.front().color;
    }

    /** Gets the x-coordinate of the starting color position.

        @return The x-coordinate of the start position.
    */
    float getStartX() const
    {
        return stops.empty() ? 0.0f : stops.front().x;
    }

    /** Gets the y-coordinate of the starting color position.

        @return The y-coordinate of the start position.
    */
    float getStartY() const
    {
        return stops.empty() ? 0.0f : stops.front().y;
    }

    /** Gets the relative position of the starting color in the gradient.

        @return The relative position (delta value) of the start position, typically 0.0f for the start.
    */
    float getStartDelta() const
    {
        return stops.empty() ? 0.0f : stops.front().delta;
    }

    //==============================================================================
    /** Gets the ending color of the gradient (last color stop).

        @return The ending color.
    */
    Color getFinishColor() const
    {
        return stops.empty() ? Color() : stops.back().color;
    }

    /** Gets the x-coordinate of the ending color position.

        @return The x-coordinate of the finish position.
    */
    float getFinishX() const
    {
        return stops.empty() ? 0.0f : stops.back().x;
    }

    /** Gets the y-coordinate of the ending color position.

        @return The y-coordinate of the finish position.
    */
    float getFinishY() const
    {
        return stops.empty() ? 0.0f : stops.back().y;
    }

    /** Gets the relative position of the ending color in the gradient.

        @return The relative position (delta value) of the finish position, typically 1.0f for the end.
    */
    float getFinishDelta() const
    {
        return stops.empty() ? 1.0f : stops.back().delta;
    }

    //==============================================================================
    /** Gets the number of color stops in the gradient.

        @return The number of color stops.
    */
    size_t getNumStops() const
    {
        return stops.size();
    }

    /** Gets a color stop by index.

        @param index The index of the color stop to retrieve.
        @return The color stop at the specified index.
    */
    const ColorStop& getStop (size_t index) const
    {
        jassert (index < stops.size());
        return stops[index];
    }

    /** Gets all color stops.

        @return A const reference to the vector of color stops.
    */
    const std::vector<ColorStop>& getStops() const
    {
        return stops;
    }

    /** Adds a color stop to the gradient.

        @param color The color of the new stop.
        @param x The x-coordinate of the stop.
        @param y The y-coordinate of the stop.
        @param delta The relative position of the stop (0.0-1.0).
    */
    void addColorStop (Color color, float x, float y, float delta)
    {
        stops.emplace_back (color, x, y, delta);
        std::sort (stops.begin(), stops.end(), [](const ColorStop& a, const ColorStop& b) {
            return a.delta < b.delta;
        });
    }

    /** Clears all color stops. */
    void clearStops()
    {
        stops.clear();
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
    /** Sets the alpha value for all color stops in the gradient.

        @param alpha The alpha value to set, affecting the transparency of the entire gradient.
    */
    void setAlpha (uint8 alpha)
    {
        for (auto& stop : stops)
            stop.color.setAlpha (alpha);
    }

    /** Sets the alpha value for all color stops in the gradient.

        @param alpha The alpha value to set, affecting the transparency of the entire gradient.
    */
    void setAlpha (float alpha)
    {
        for (auto& stop : stops)
            stop.color.setAlpha (alpha);
    }

    /** Creates a new gradient with a specified alpha value for all color stops.

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

    /** Creates a new gradient with a specified alpha value for all color stops.

        @param alpha The alpha value for the new gradient.

        @return A new ColorGradient object with the specified alpha value.
    */
    ColorGradient withAlpha (float alpha) const
    {
        ColorGradient result (*this);
        result.setAlpha (alpha);
        return result;
    }

    /** Creates a new gradient with multiplied alpha values for all color stops.

        @param alpha The alpha multiplier for the new gradient.

        @return A new ColorGradient object with multiplied alpha values.
    */
    ColorGradient withMultipliedAlpha (uint8 alpha) const
    {
        ColorGradient result (*this);
        for (auto& stop : result.stops)
            stop.color = stop.color.withMultipliedAlpha (alpha);
        return result;
    }

    /** Creates a new gradient with multiplied alpha values for all color stops.

        @param alpha The alpha multiplier for the new gradient.

        @return A new ColorGradient object with multiplied alpha values.
    */
    ColorGradient withMultipliedAlpha (float alpha) const
    {
        ColorGradient result (*this);
        for (auto& stop : result.stops)
            stop.color = stop.color.withMultipliedAlpha (alpha);
        return result;
    }

private:
    Type type = Type::Linear;
    std::vector<ColorStop> stops;
    float radius = 0.0f;
};

} // namespace yup
