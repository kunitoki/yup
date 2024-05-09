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
    constexpr Color brighter (float amount) noexcept
    {
        amount = amount * 0.1f;

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
        constexpr float middleLuminance = 128.0;

        if (luminance() > middleLuminance)
            return Color (255, 0, 0, 0);
        else
            return Color (255, 255, 255, 255);
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

    constexpr float luminance() const
    {
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
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
