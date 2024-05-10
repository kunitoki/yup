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
class JUCE_API Color
{
public:
    //==============================================================================
    constexpr Color() noexcept = default;

    constexpr Color (uint32 color) noexcept
        : data (color)
    {
    }

    constexpr Color (uint8 r, uint8 g, uint8 b) noexcept
        : b (b)
        , g (g)
        , r (r)
        , a (255)
    {
    }

    constexpr Color (uint8 a, uint8 r, uint8 g, uint8 b) noexcept
        : b (b)
        , g (g)
        , r (r)
        , a (a)
    {
    }

    //==============================================================================
    constexpr uint32 getARGB() const noexcept
    {
        return data;
    }

    constexpr operator uint32() const noexcept
    {
        return data;
    }

    //==============================================================================
    constexpr uint8 getAlpha() const noexcept
    {
        return a;
    }

    constexpr float getAlphaFloat() const noexcept
    {
        return componentToNormalized (a);
    }

    constexpr Color& setAlpha (uint8 alpha) noexcept
    {
        a = alpha;
        return *this;
    }

    constexpr Color& setAlpha (float alpha) noexcept
    {
        a = normalizedToComponent (alpha);
        return *this;
    }

    constexpr Color withAlpha (uint8 alpha) const noexcept
    {
        return { alpha, r, g, b };
    }

    //==============================================================================
    constexpr uint8 getRed() const noexcept
    {
        return r;
    }

    constexpr float getRedFloat() const noexcept
    {
        return componentToNormalized (r);
    }

    constexpr Color& setRed (uint8 red) noexcept
    {
        r = red;
        return *this;
    }

    constexpr Color& setRed (float red) noexcept
    {
        r = normalizedToComponent (red);
        return *this;
    }

    constexpr Color withRed (uint8 red) const noexcept
    {
        return { a, red, g, b };
    }

    //==============================================================================
    constexpr uint8 getGreen() const noexcept
    {
        return g;
    }

    constexpr float getGreenFloat() const noexcept
    {
        return componentToNormalized (g);
    }

    constexpr Color& setGreen (uint8 green) noexcept
    {
        g = green;
        return *this;
    }

    constexpr Color& setGreen (float green) noexcept
    {
        g = normalizedToComponent (green);
        return *this;
    }

    constexpr Color withGreen (uint8 green) const noexcept
    {
        return { a, r, green, b };
    }

    //==============================================================================
    constexpr uint8 getBlue() const noexcept
    {
        return b;
    }

    constexpr float getBlueFloat() const noexcept
    {
        return componentToNormalized (b);
    }

    constexpr Color& setBlue (uint8 blue) noexcept
    {
        b = blue;
        return *this;
    }

    constexpr Color& setBlue (float blue) noexcept
    {
        b = normalizedToComponent (blue);
        return *this;
    }

    constexpr Color withBlue (uint8 blue) const noexcept
    {
        return { a, r, g, blue };
    }

    //==============================================================================
    constexpr float getHue() const
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

    constexpr float getSaturation() const
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

    constexpr float getLuminance() const
    {
        const float rf = getRedFloat();
        const float gf = getGreenFloat();
        const float bf = getBlueFloat();
        const float max = jmax (rf, gf, bf);
        const float min = jmin (rf, gf, bf);

        return (max + min) / 2.0f;
    }

    //==============================================================================
    constexpr std::tuple<float, float, float> toHSL () const noexcept
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

    constexpr Color darker (float amount) noexcept
    {
        return brighter (-amount);
    }

    //==============================================================================
    constexpr Color contrasting() const noexcept
    {
        return contrasting (0.5f);
    }

    constexpr Color contrasting (float amount) const noexcept
    {
        const auto [h, s, l] = inverted().toHSL();

        return fromHSL (modulo (h + jlimit (0.0f, 1.0f, amount), 1.0f), s, l).withAlpha (a);
    }

    //==============================================================================
    constexpr Color& invert() noexcept
    {
        r = 255 - r;
        g = 255 - g;
        b = 255 - b;
        return *this;
    }

    constexpr Color inverted() const noexcept
    {
        Color result (*this);
        result.invert();
        return result;
    }

    //==============================================================================
    constexpr Color& invertAlpha() noexcept
    {
        a = 255 - a;
        return *this;
    }

    constexpr Color invertedAlpha() const noexcept
    {
        Color result (*this);
        result.invertAlpha();
        return result;
    }

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

} // namespace juce
