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
    /** How the gradient is repeated or reflected beyond its endpoints. */
    enum class Spread : uint8_t
    {
        Pad,    ///< Colors beyond the endpoints are clamped to the nearest endpoint color.
        Repeat, ///< The gradient pattern is repeated (tiled).
        Reflect ///< The gradient pattern is repeated with alternating mirroring.
    };

    //==============================================================================
    /** Represents a single color stop in a gradient. */
    struct ColorStop
    {
        /** Constructs a default color stop with zero values. */
        constexpr ColorStop() = default;

        /** Constructs a color stop with the given color, x, y and delta. */
        constexpr ColorStop (Color color, float x, float y, float delta)
            : color (color)
            , x (x)
            , y (y)
            , delta (delta)
        {
        }

        /** Constructs a color stop with the given color, point and delta. */
        constexpr ColorStop (Color color, const Point<float>& p, float delta)
            : color (color)
            , x (p.getX())
            , y (p.getY())
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
    ColorGradient (Color color1, float x1, float y1, Color color2, float x2, float y2, Type type = Type::Linear) noexcept
        : type (type)
    {
        stops.emplace_back (color1, x1, y1, 0.0f);
        stops.emplace_back (color2, x2, y2, 1.0f);

        if (type == Radial)
            radius = std::sqrt (square (x2 - x1) + square (y2 - y1));
    }

    /** Constructs a gradient with specified attributes.

        @param color1 The starting color of the gradient.
        @param p1 The pointof the starting color.
        @param color2 The ending color of the gradient.
        @param p2 The point of the ending color.
        @param type The type of gradient (Linear or Radial).
    */
    ColorGradient (Color color1, const Point<float>& p1, Color color2, const Point<float>& p2, Type type = Type::Linear) noexcept
        : ColorGradient (color1, p1.getX(), p1.getY(), color2, p2.getX(), p2.getY(), type)
    {
    }

    /** Constructs a gradient with multiple color stops.

        @param type The type of gradient (Linear or Radial).
        @param colorStops Vector of ColorStop objects defining the gradient.
    */
    ColorGradient (Type type, std::vector<ColorStop> colorStops) noexcept
        : type (type)
        , stops (std::move (colorStops))
    {
        if (type == Radial && stops.size() >= 2)
        {
            const auto& first = stops.front();
            const auto& last = stops.back();
            radius = std::sqrt (square (last.x - first.x) + square (last.y - first.y));
        }
    }

    /** Constructs a gradient with multiple color stops.

        @param type The type of gradient (Linear or Radial).
        @param colorStops Initializer list of ColorStop objects defining the gradient.
    */
    ColorGradient (Type type, std::initializer_list<ColorStop> colorStops) noexcept
        : type (type)
        , stops (std::move (colorStops))
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

    /** Gets the spread mode of the gradient. */
    Spread getSpread() const noexcept
    {
        return spread;
    }

    /** Returns a copy of this gradient with the given spread mode. */
    ColorGradient withSpread (Spread newSpread) const noexcept
    {
        auto copy = *this;
        copy.spread = newSpread;
        return copy;
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
    Span<const ColorStop> getStops() const
    {
        return stops;
    }

    /** Returns the interpolated color at the given gradient position (0.0-1.0). */
    Color getColorAt (float t) const noexcept
    {
        if (stops.empty())
            return {};

        if (stops.size() == 1)
            return stops.front().color;

        if (t <= stops.front().delta)
            return stops.front().color;

        if (t >= stops.back().delta)
            return stops.back().color;

        for (size_t i = 1; i < stops.size(); ++i)
        {
            if (t <= stops[i].delta)
            {
                const auto& a = stops[i - 1];
                const auto& b = stops[i];
                const float denom = b.delta - a.delta;
                if (denom <= 0.0f)
                    return b.color;

                const float localT = (t - a.delta) / denom;
                return a.color.mixedWith (b.color, localT, ColorSpace::SRGB);
            }
        }

        return stops.back().color;
    }

    /** Returns the interpolated color at a given point in gradient space. */
    Color getColorAt (float x, float y) const noexcept
    {
        if (stops.empty())
            return {};

        const auto start = Point<float> (stops.front().x, stops.front().y);
        const auto end = Point<float> (stops.back().x, stops.back().y);

        if (type == Type::Radial)
        {
            float radiusValue = radius;
            if (radiusValue <= 0.0f)
                radiusValue = start.distanceTo (end);

            if (radiusValue <= 0.0f)
                return stops.front().color;

            float t = start.distanceTo ({ x, y }) / radiusValue;
            t = applySpread (t);
            return getColorAt (t);
        }

        const float dx = end.getX() - start.getX();
        const float dy = end.getY() - start.getY();
        const float lenSquared = dx * dx + dy * dy;
        if (lenSquared <= 0.0f)
            return stops.front().color;

        float t = ((x - start.getX()) * dx + (y - start.getY()) * dy) / lenSquared;
        t = applySpread (t);
        return getColorAt (t);
    }

    /** Returns the interpolated color at a given point in gradient space. */
    Color getColorAt (const Point<float>& p) const noexcept
    {
        return getColorAt (p.getX(), p.getY());
    }

    /** Fills a lookup table with colors sampled evenly along this gradient.

        The output values are packed as `0xAARRGGBB`, matching Color, Rive
        ColorInt, and image pixel APIs such as ImagePixelData::setPixel().

        @param colors    the destination lookup table to fill
    */
    void fillGradient (Span<uint32> colors) const noexcept
    {
        if (colors.empty())
            return;

        if (stops.empty())
        {
            std::fill (colors.begin(), colors.end(), Color().getARGB());
            return;
        }

        if (stops.size() == 1 || colors.size() == 1)
        {
            std::fill (colors.begin(), colors.end(), stops.front().color.getARGB());
            return;
        }

        size_t stopIndex = 0;

        for (size_t i = 0; i < colors.size(); ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (colors.size() - 1);

            if (t <= stops.front().delta)
            {
                colors[i] = stops.front().color.getARGB();
                continue;
            }

            if (t >= stops.back().delta)
            {
                colors[i] = stops.back().color.getARGB();
                continue;
            }

            while (stopIndex + 1 < stops.size() && stops[stopIndex + 1].delta <= t)
                ++stopIndex;

            const auto& start = stops[stopIndex];
            const auto& end = stops[stopIndex + 1];
            const float deltaRange = end.delta - start.delta;

            if (deltaRange <= 0.0f)
            {
                colors[i] = end.color.getARGB();
                continue;
            }

            const float localT = (t - start.delta) / deltaRange;
            colors[i] = start.color.mixedWith (end.color, localT, ColorSpace::SRGB).getARGB();
        }
    }

    /** Adds a color stop to the gradient.

        @param color The color of the new stop.
        @param x The x-coordinate of the stop.
        @param y The y-coordinate of the stop.
        @param delta The relative position of the stop (0.0-1.0).
    */
    void addColorStop (Color color, float x, float y, float delta)
    {
        stops.emplace_back (color, x, y, jlimit (0.0f, 1.0f, delta));

        std::sort (stops.begin(), stops.end(), [] (const ColorStop& a, const ColorStop& b)
        {
            return a.delta < b.delta;
        });
    }

    /** Adds a color stop to the gradient.

        @param color The color of the new stop.
        @param p The point of the stop.
        @param delta The relative position of the stop (0.0-1.0).
    */
    void addColorStop (Color color, const Point<float>& p, float delta)
    {
        addColorStop (color, p.getX(), p.getY(), delta);
    }

    /** Adds a color stop to the gradient.

        @param color The color of the new stop.
        @param delta The relative position of the stop (0.0-1.0).
    */
    void addColorStop (Color color, float delta)
    {
        if (stops.size() <= 1)
            return;

        auto start = stops.front();
        auto end = stops.back();
        auto line = Line<float> (start.x, start.y, end.x, end.y);

        addColorStop (color, line.pointAlong (delta), delta);
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

    //==============================================================================
    /** Constructs a gradient with evenly spaced color stops generated from two colors.

        This helper creates a gradient containing a specified number of stops in linear position space.
        Colors are generated using Color::mixedWith for sRGB and spectral spaces, while RGB produces only
        the start/end stops. The stops are placed along the line between the start and end positions.

        @param color1 The starting color of the gradient.
        @param x1 The x-coordinate of the starting color.
        @param y1 The y-coordinate of the starting color.
        @param color2 The ending color of the gradient.
        @param x2 The x-coordinate of the ending color.
        @param y2 The y-coordinate of the ending color.
        @param numColors The number of color stops to generate (including start and end).
        @param type The type of gradient (Linear or Radial).
        @param colorSpace The color space used for mixing.

        @return A ColorGradient with evenly spaced stops between the given colors.
    */
    static ColorGradient fromLinearColors (Color color1,
                                           float x1,
                                           float y1,
                                           Color color2,
                                           float x2,
                                           float y2,
                                           size_t numColors,
                                           Type type = Type::Linear,
                                           ColorSpace colorSpace = ColorSpace::Spectral) noexcept
    {
        if (numColors == 0)
            return ColorGradient (type, {});

        if (numColors == 1)
            return ColorGradient (type, { ColorStop (color1, x1, y1, 0.0f) });

        if (colorSpace == ColorSpace::RGB)
        {
            return ColorGradient (type, { ColorStop (color1, x1, y1, 0.0f), ColorStop (color2, x2, y2, 1.0f) });
        }

        std::vector<ColorStop> colorStops;
        colorStops.reserve (numColors);

        const Line<float> line (x1, y1, x2, y2);
        const float step = 1.0f / static_cast<float> (numColors - 1);

        for (size_t i = 0; i < numColors; ++i)
        {
            const float t = static_cast<float> (i) * step;
            colorStops.emplace_back (color1.mixedWith (color2, t, colorSpace), line.pointAlong (t), t);
        }

        return ColorGradient (type, std::move (colorStops));
    }

    /** Constructs a gradient with evenly spaced color stops generated from two colors.

        This helper creates a gradient containing a specified number of stops in linear position space.
        Colors are generated using Color::mixedWith for sRGB and spectral spaces, while RGB produces only
        the start/end stops. The stops are placed along the line between the start and end positions.

        @param color1 The starting color of the gradient.
        @param p1 The point of the starting color.
        @param color2 The ending color of the gradient.
        @param p2 The point of the ending color.
        @param numColors The number of color stops to generate (including start and end).
        @param type The type of gradient (Linear or Radial).
        @param colorSpace The color space used for mixing.

        @return A ColorGradient with evenly spaced stops between the given colors.
    */
    static ColorGradient fromLinearColors (Color color1,
                                           const Point<float>& p1,
                                           Color color2,
                                           const Point<float>& p2,
                                           size_t numColors,
                                           Type type = Type::Linear,
                                           ColorSpace colorSpace = ColorSpace::Spectral) noexcept
    {
        return fromLinearColors (color1, p1.getX(), p1.getY(), color2, p2.getX(), p2.getY(), numColors, type, colorSpace);
    }

private:
    Type type = Type::Linear;
    Spread spread = Spread::Pad;
    std::vector<ColorStop> stops;
    float radius = 0.0f;

    float applySpread (float t) const noexcept
    {
        switch (spread)
        {
            case Spread::Pad:
                return jlimit (0.0f, 1.0f, t);
            case Spread::Repeat:
            {
                const float f = t - std::floor (t);
                return f < 0.0f ? f + 1.0f : f;
            }
            case Spread::Reflect:
            {
                const float twoT = t * 0.5f;
                const float rounded = std::round (twoT);
                const float result = std::abs (t - 2.0f * rounded);
                return jlimit (0.0f, 1.0f, result);
            }
        }
        return jlimit (0.0f, 1.0f, t);
    }
};

} // namespace yup
