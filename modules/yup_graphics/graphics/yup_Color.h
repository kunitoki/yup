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
/** Represents an RGBA color for graphical use.

    This class encapsulates color information in RGBA format, where each component (red, green, blue, alpha)
    is represented as an 8-bit value. It provides methods for adjusting color components individually,
    converting to and from HSL (Hue, Saturation, Luminance), and for performing operations like brightening,
    darkening, and contrasting.
*/
class JUCE_API Color
{
public:
    //==============================================================================
    /** Default constructor, initializes the color to transparent black.

        Creates a color with all components set to zero, which is fully transparent black.
    */
    constexpr Color() noexcept = default;

    /** Constructs a color from a 32-bit integer.

        This constructor initializes the color from a 32-bit integer, assuming a format
        where the highest byte is alpha, followed by red, green, and blue.

        @param color The 32-bit integer representing the color.
    */
    constexpr Color (uint32 color) noexcept
        : data (color)
    {
    }

    /** Constructs a color with specified red, green, and blue components, defaulting alpha to opaque.

        @param r The red component, from 0 to 255.
        @param g The green component, from 0 to 255.
        @param b The blue component, from 0 to 255.
    */
    constexpr Color (uint8 r, uint8 g, uint8 b) noexcept
        : b (b)
        , g (g)
        , r (r)
        , a (255)
    {
    }

    /** Constructs a color with specified alpha, red, green, and blue components.

        @param a The alpha component, from 0 (transparent) to 255 (opaque).
        @param r The red component, from 0 to 255.
        @param g The green component, from 0 to 255.
        @param b The blue component, from 0 to 255.
    */
    constexpr Color (uint8 a, uint8 r, uint8 g, uint8 b) noexcept
        : b (b)
        , g (g)
        , r (r)
        , a (a)
    {
    }

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    constexpr Color (const Color& other) noexcept = default;
    constexpr Color (Color&& other) noexcept = default;
    constexpr Color& operator=(const Color& other) noexcept = default;
    constexpr Color& operator=(Color&& other) noexcept = default;

    //==============================================================================
    /** Returns the color as a 32-bit integer in ARGB format.

        @return The color as a 32-bit integer.
    */
    constexpr uint32 getARGB() const noexcept
    {
        return data;
    }

    /** Implicit conversion to a 32-bit integer.

        Allows the color to be used wherever a 32-bit integer color is expected.

        @return The color as a 32-bit integer.
    */
    constexpr operator uint32() const noexcept
    {
        return data;
    }

    //==============================================================================
    /** Returns the alpha component of the color.

        @return The alpha component as an 8-bit integer.
    */
    constexpr uint8 getAlpha() const noexcept
    {
        return a;
    }

    /** Returns the alpha component of the color as a floating point value.

        @return The alpha component, normalized to the range [0, 1].
    */
    constexpr float getAlphaFloat() const noexcept
    {
        return componentToNormalized (a);
    }

    /** Sets the alpha component of the color.

        @param alpha The new alpha value as an 8-bit integer.

        @return A reference to this color object to allow for method chaining.
    */
    constexpr Color& setAlpha (uint8 alpha) noexcept
    {
        a = alpha;
        return *this;
    }

    /** Sets the alpha component of the color using a floating point value.

        @param alpha The new alpha value, normalized to the range [0, 1].

        @return A reference to this color object to allow for method chaining.
    */
    constexpr Color& setAlpha (float alpha) noexcept
    {
        a = normalizedToComponent (alpha);
        return *this;
    }

    /** Returns a new color with the specified alpha value.

        This method creates a new color with the same red, green, and blue values but a different alpha.

        @param alpha The new alpha value as an 8-bit integer.

        @return A new color with the specified alpha value.
    */
    constexpr Color withAlpha (uint8 alpha) const noexcept
    {
        return { alpha, r, g, b };
    }

    // TODO - doxygen
    constexpr Color withAlpha (float alpha) const noexcept
    {
        return { normalizedToComponent (alpha), r, g, b };
    }

    // TODO - doxygen
    constexpr Color withMultipliedAlpha (uint8 alpha) const noexcept
    {
        return { normalizedToComponent (componentToNormalized (a) * componentToNormalized (alpha)), r, g, b };
    }

    // TODO - doxygen
    constexpr Color withMultipliedAlpha (float alpha) const noexcept
    {
        return { normalizedToComponent (componentToNormalized (a) * alpha), r, g, b };
    }

    //==============================================================================
    /** Returns the red component of the color.

        @return The red component as an 8-bit integer.
    */
    constexpr uint8 getRed() const noexcept
    {
        return r;
    }

    /** Returns the red component of the color as a floating point value.

        @return The red component, normalized to the range [0, 1].
    */
    constexpr float getRedFloat() const noexcept
    {
        return componentToNormalized (r);
    }

    /** Sets the red component of the color.

        @param red The new red value as an 8-bit integer.

        @return A reference to this color object to allow for method chaining.
    */
    constexpr Color& setRed (uint8 red) noexcept
    {
        r = red;
        return *this;
    }

    /** Sets the red component of the color using a floating point value.

        @param red The new red value, normalized to the range [0, 1].

        @return A reference to this color object to allow for method chaining.
    */
    constexpr Color& setRed (float red) noexcept
    {
        r = normalizedToComponent (red);
        return *this;
    }

    /** Returns a new color with the specified red value.

        This method creates a new color with the same green, blue, and alpha values but a different red.

        @param red The new red value as an 8-bit integer.

        @return A new color with the specified red value.
    */
    constexpr Color withRed (uint8 red) const noexcept
    {
        return { a, red, g, b };
    }

    // TODO - doxygen
    constexpr Color withRed (float red) const noexcept
    {
        return { a, normalizedToComponent (red), g, b };
    }

    //==============================================================================
    /** Returns the green component of the color.

        @return The green component as an 8-bit integer.
    */
    constexpr uint8 getGreen() const noexcept
    {
        return g;
    }

    /** Returns the green component of the color as a floating point value.

        @return The green component, normalized to the range [0, 1].
    */
    constexpr float getGreenFloat() const noexcept
    {
        return componentToNormalized (g);
    }

    /** Sets the green component of the color.

        @param green The new green value as an 8-bit integer.

        @return A reference to this color object to allow for method chaining.
    */
    constexpr Color& setGreen (uint8 green) noexcept
    {
        g = green;
        return *this;
    }

    /** Sets the green component of the color using a floating point value.

        @param green The new green value, normalized to the range [0, 1].

        @return A reference to this color object to allow for method chaining.
    */
    constexpr Color& setGreen (float green) noexcept
    {
        g = normalizedToComponent (green);
        return *this;
    }

    /** Returns a new color with the specified green value.

        This method creates a new color with the same red, blue, and alpha values but a different green.

        @param green The new green value as an 8-bit integer.

        @return A new color with the specified green value.
    */
    constexpr Color withGreen (uint8 green) const noexcept
    {
        return { a, r, green, b };
    }

    // TODO - doxygen
    constexpr Color withGreen (float green) const noexcept
    {
        return { a, r, normalizedToComponent (green), b };
    }

    //==============================================================================
    /** Returns the blue component of the color.

        @return The blue component as an 8-bit integer.
    */
    constexpr uint8 getBlue() const noexcept
    {
        return b;
    }

    /** Returns the blue component of the color as a floating point value.

        @return The blue component, normalized to the range [0, 1].
    */
    constexpr float getBlueFloat() const noexcept
    {
        return componentToNormalized (b);
    }

    /** Sets the blue component of the color.

        @param blue The new blue value as an 8-bit integer.

        @return A reference to this color object to allow for method chaining.
    */
    constexpr Color& setBlue (uint8 blue) noexcept
    {
        b = blue;
        return *this;
    }

    /** Sets the blue component of the color using a floating point value.

        @param blue The new blue value, normalized to the range [0, 1].

        @return A reference to this color object to allow for method chaining.
    */
    constexpr Color& setBlue (float blue) noexcept
    {
        b = normalizedToComponent (blue);
        return *this;
    }

    /** Returns a new color with the specified blue value.

        This method creates a new color with the same red, green, and alpha values but a different blue.

        @param blue The new blue value as an 8-bit integer.

        @return A new color with the specified blue value.
    */
    constexpr Color withBlue (uint8 blue) const noexcept
    {
        return { a, r, g, blue };
    }

    // TODO - doxygen
    constexpr Color withBlue (float blue) const noexcept
    {
        return { a, r, g, normalizedToComponent (blue) };
    }

    //==============================================================================
    /** Calculates the hue component of the color in HSL representation.

        Hue is measured as a location on the standard color wheel, expressed as a fraction between 0 and 1.0.
        This function calculates hue by converting RGB values to HSL and then extracting the hue component.

        @return The hue component of the color, normalized to the range [0, 1].
    */
    constexpr float getHue() const noexcept
    {
        const float rf = getRedFloat();
        const float gf = getGreenFloat();
        const float bf = getBlueFloat();
        const float max = jmax (rf, gf, bf);
        const float min = jmin (rf, gf, bf);

        if (max == min)
            return 0.0f;

        const float d = max - min;
        float h = 0.0f;

        if (max == rf)
            h = (gf - bf) / d + (gf < bf ? 6.0f : 0.0f);

        else if (max == gf)
            h = (bf - rf) / d + 2.0f;

        else if (max == bf)
            h = (rf - gf) / d + 4.0f;

        h /= 6.0f;
        return h;
    }

    /** Calculates the saturation component of the color in HSL representation.

        Saturation measures the intensity of color. A saturation value of 0 corresponds to a shade of grey,
        while a value of 1 indicates the full intensity of the color. This method calculates saturation by converting
        RGB values to HSL and then extracting the saturation component.

        @return The saturation component of the color, normalized to the range [0, 1].
    */
    constexpr float getSaturation() const noexcept
    {
        const float rf = getRedFloat();
        const float gf = getGreenFloat();
        const float bf = getBlueFloat();
        const float max = jmax (rf, gf, bf);
        const float min = jmin (rf, gf, bf);

        if (max == min)
            return 0.0f;

        const float l = (max + min) / 2.0f;
        const float d = max - min;

        return l > 0.5f ? d / (2.0f - max - min) : d / (max + min);
    }

    /** Calculates the luminance of the color.

        Luminance is a measure of the brightness of a color, normalized to a range from 0 (black) to 1 (white).
        This method calculates luminance by averaging the maximum and minimum RGB components.

        @return The luminance component of the color, normalized to the range [0, 1].
    */
    constexpr float getLuminance() const noexcept
    {
        const float rf = getRedFloat();
        const float gf = getGreenFloat();
        const float bf = getBlueFloat();
        const float max = jmax (rf, gf, bf);
        const float min = jmin (rf, gf, bf);

        return (max + min) / 2.0f;
    }

    //==============================================================================
    /** Converts the color to its HSL (Hue, Saturation, Luminance) components.

        This method provides a way to obtain the HSL representation of the color, which can be useful for color manipulation
        and effects. The returned tuple contains the hue, saturation, and luminance components, respectively.

        @return A tuple consisting of hue, saturation, and luminance.
    */
    constexpr std::tuple<float, float, float> toHSL() const noexcept
    {
        const float rf = getRedFloat();
        const float gf = getGreenFloat();
        const float bf = getBlueFloat();
        const float max = jmax (rf, gf, bf);
        const float min = jmin (rf, gf, bf);

        const float l = (max + min) / 2.0f;
        if (max == min)
            return std::make_tuple (0.0f, 0.0f, l); // achromatic

        const float d = max - min;

        float h = 0.0f;
        float s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);

        if (max == rf)
            h = (gf - bf) / d + (gf < bf ? 6.0f : 0.0f);

        else if (max == gf)
            h = (bf - rf) / d + 2.0f;

        else if (max == bf)
            h = (rf - gf) / d + 4.0f;

        h /= 6.0f;

        return std::make_tuple (h, s, l);
    }

    /** Constructs a color from HSL values.

        This static method allows for the creation of a color from its HSL representation.
        It is useful for generating colors based on more perceptual components rather than direct color component manipulation.

        @param h The hue component, normalized to [0, 1].
        @param s The saturation component, normalized to [0, 1].
        @param l The luminance component, normalized to [0, 1].

        @return A Color object corresponding to the given HSL values.
    */
    constexpr static Color fromHSL (float h, float s, float l) noexcept
    {
        auto hue2rgb = [](float p, float q, float t)
        {
            if (t < 0.0f) t += 1.0f;
            if (t > 1.0f) t -= 1.0f;
            if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
            if (t < 1.0f / 2.0f) return q;
            if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
            return p;
        };

        float r = l, g = l, b = l;

        if (s != 0.0f)
        {
            const float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
            const float p = 2.0f * l - q;

            r = hue2rgb(p, q, h + 1.0f / 3.0f);
            g = hue2rgb(p, q, h);
            b = hue2rgb(p, q, h - 1.0f / 3.0f);
        }

        return { static_cast<uint8> (r * 255), static_cast<uint8> (g * 255), static_cast<uint8> (b * 255) };
    }

    //==============================================================================
    /** Makes the color brighter by a specified amount.

        This method increases the RGB components of the color by the given amount, capped at 1.0 to maintain valid color values.
        It is useful for creating lighter variations of the color without altering the hue and saturation significantly.

        @param amount The amount by which to increase the RGB components, normalized to the range [0, 1].

        @return A new Color object that is brighter than the original.
    */
    constexpr Color brighter (float amount) noexcept
    {
        return
        {
            a,
            normalizedToComponent (getRedFloat() + amount),
            normalizedToComponent (getGreenFloat() + amount),
            normalizedToComponent (getBlueFloat() + amount)
        };
    }

    /** Makes the color darker by a specified amount.

        This method decreases the RGB components of the color by the given amount, capped at 0 to maintain valid color values.
        It is useful for creating darker variations of the color without altering the hue and saturation significantly.

        @param amount The amount by which to decrease the RGB components, normalized to the range [0, 1].

        @return A new Color object that is darker than the original.
    */
    constexpr Color darker (float amount) noexcept
    {
        return brighter (-amount);
    }

    //==============================================================================
    /** Returns a contrasting color.

        This method calculates a color that contrasts with the current color based on its luminance.
        It is particularly useful for ensuring text or UI elements are readable when placed on backgrounds of varying colors.

        @return A new Color object that contrasts with the current color.
    */
    constexpr Color contrasting() const noexcept
    {
        return contrasting (0.5f);
    }

    /** Returns a contrasting color adjusted by a specified amount.

        This method provides finer control over the contrast calculation by allowing adjustment of the hue shift used in
        determining the contrasting color. The luminance and saturation of the original color are maintained.

        @param amount The amount to adjust the hue by, normalized to the range [0, 1].

        @return A new Color object that contrasts with the current color.
    */
    constexpr Color contrasting (float amount) const noexcept
    {
        const auto [h, s, l] = inverted().toHSL();

        return fromHSL (modulo (h + jlimit (0.0f, 1.0f, amount), 1.0f), s, l).withAlpha (a);
    }

    //==============================================================================
    /** Inverts the color components (RGB) of the current color.

        This method changes each RGB component to its complementary value, which is useful for creating negative effects or for visual highlights.

        @return A reference to this Color object, now with inverted RGB values.
    */
    constexpr Color& invert() noexcept
    {
        r = 255 - r;
        g = 255 - g;
        b = 255 - b;
        return *this;
    }

    /** Returns a new color that is the inverse of the current color.

        This method creates a new Color object with each RGB component set to its complementary value, effectively providing the negative of the color.

        @return A new Color object with inverted RGB values.
    */
    constexpr Color inverted() const noexcept
    {
        Color result (*this);
        result.invert();
        return result;
    }

    //==============================================================================
    /** Inverts the alpha component of the current color.

        This method changes the alpha component to its complementary value, which is useful for reversing transparency effects.

        @return A reference to this Color object, now with an inverted alpha value.
    */
    constexpr Color& invertAlpha() noexcept
    {
        a = 255 - a;
        return *this;
    }

    /** Returns a new color that is the inverse of the current color in terms of alpha transparency.

        This method creates a new Color object with the alpha component set to its complementary value, effectively reversing the transparency of the color.

        @return A new Color object with an inverted alpha value.
    */
    constexpr Color invertedAlpha() const noexcept
    {
        Color result (*this);
        result.invertAlpha();
        return result;
    }

    //==============================================================================
    // TODO - doxygen
    static Color opaqueRandom() noexcept
    {
        auto random = Random();
        random.setSeedRandomly();

        return
        {
            255,
            static_cast<uint8> (random.nextInt (255)),
            static_cast<uint8> (random.nextInt (255)),
            static_cast<uint8> (random.nextInt (255))
        };
    }

    //==============================================================================
    static Color fromString (const String& colourString);

private:
    constexpr static float componentToNormalized (uint8 component) noexcept
    {
        return static_cast<float> (component) / 255.0f;
    }

    constexpr static uint8 normalizedToComponent (float normalized) noexcept
    {
        return static_cast<uint8> (jlimit (0.0f, 1.0f, normalized) * 255.0f);
    }

    union
    {
        struct
        {
            uint8 b, g, r, a;
        };

        uint32 data = 0xff000000;
    };
};

} // namespace yup
