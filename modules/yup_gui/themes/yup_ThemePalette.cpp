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

namespace
{

constexpr std::array<Color, 5> defaultPaletteSources {
    Color (0xff14171c), // background
    Color (0xff232830), // surface
    Color (0xffe6eaf0), // text
    Color (0xff5ab8ff), // accent
    Color (0xff8b95a3)  // muted
};

constexpr double minimumTextContrast = 4.5;
constexpr double minimumAccentContrast = 3.0;
constexpr double minimumAccentChroma = 20.0;
constexpr float darkBackgroundMaxLightness = 0.12f;
constexpr float lightBackgroundMinLightness = 0.95f;
constexpr float surfaceLightnessStep = 0.06f;
constexpr float surfaceMinDelta = 0.04f;
constexpr float surfaceMaxDelta = 0.25f;
constexpr float surfaceIdealDelta = 0.10f;

double linearizeChannel (float value)
{
    return value <= 0.04045f ? value / 12.92 : std::pow ((value + 0.055) / 1.055, 2.4);
}

// WCAG relative luminance, Color::getLuminance() is HSL lightness instead
double relativeLuminance (Color color)
{
    return 0.2126 * linearizeChannel (color.getRedFloat())
         + 0.7152 * linearizeChannel (color.getGreenFloat())
         + 0.0722 * linearizeChannel (color.getBlueFloat());
}

double contrastRatio (Color a, Color b)
{
    const auto la = relativeLuminance (a);
    const auto lb = relativeLuminance (b);
    return (jmax (la, lb) + 0.05) / (jmin (la, lb) + 0.05);
}

float lightnessOf (Color color)
{
    return std::get<2> (color.toHSLuv());
}

// CIELUV chroma, HSLuv saturation is relative to the gamut and overrates pale colors
double chromaOf (Color color)
{
    const auto r = linearizeChannel (color.getRedFloat());
    const auto g = linearizeChannel (color.getGreenFloat());
    const auto b = linearizeChannel (color.getBlueFloat());

    const auto x = 0.4124 * r + 0.3576 * g + 0.1805 * b;
    const auto y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    const auto z = 0.0193 * r + 0.1192 * g + 0.9505 * b;

    const auto denominator = x + 15.0 * y + 3.0 * z;
    if (denominator <= 0.0)
        return 0.0;

    const auto lightness = lightnessOf (color) * 100.0;
    const auto u = 13.0 * lightness * (4.0 * x / denominator - 0.19783000664283);
    const auto v = 13.0 * lightness * (9.0 * y / denominator - 0.46831999493879);
    return std::hypot (u, v);
}

Color withLightness (Color color, float lightness)
{
    const auto hsl = color.toHSLuv();
    return Color::fromHSLuv (std::get<0> (hsl), std::get<1> (hsl), jlimit (0.0f, 1.0f, lightness));
}

Color withContrast (Color color, Color background, double targetContrast, float direction)
{
    auto [hue, saturation, lightness] = color.toHSLuv();

    const float limit = direction > 0.0f ? 1.0f : 0.0f;

    while (contrastRatio (color, background) < targetContrast && lightness != limit)
    {
        lightness = jlimit (0.0f, 1.0f, lightness + direction * 0.01f);
        color = Color::fromHSLuv (hue, saturation, lightness);
    }

    return color;
}

// Removes and returns the color with the highest score, ties keep the earliest. A score of
// -infinity marks a color as not eligible.
template <class Score>
std::optional<Color> takeBest (std::vector<Color>& colors, Score&& score)
{
    auto best = colors.end();
    auto bestScore = -std::numeric_limits<double>::infinity();

    for (auto it = colors.begin(); it != colors.end(); ++it)
    {
        if (const auto s = static_cast<double> (score (*it)); s > bestScore)
        {
            bestScore = s;
            best = it;
        }
    }

    if (best == colors.end())
        return std::nullopt;

    const auto color = *best;
    colors.erase (best);
    return color;
}

std::optional<Color> parseHexToken (const String& token)
{
    if ((token.length() != 6 && token.length() != 8) || ! token.containsOnly ("0123456789abcdefABCDEF"))
        return std::nullopt;

    return Color::fromString ("#" + token);
}

} // namespace

//==============================================================================

ThemePalette::ThemePalette()
    : ThemePalette (Span<const Color> (defaultPaletteSources.data(), defaultPaletteSources.size()))
{
}

ThemePalette::ThemePalette (Span<const Color> colors, Mode paletteMode)
    : sourceColors (colors.begin(), colors.end())
    , mode (paletteMode)
{
    if (sourceColors.empty())
        sourceColors.assign (defaultPaletteSources.begin(), defaultPaletteSources.end());

    derive();
}

//==============================================================================

void ThemePalette::derive()
{
    constexpr auto notEligible = -std::numeric_limits<double>::infinity();

    const float direction = mode == Mode::dark ? 1.0f : -1.0f;
    const auto set = [this] (Role role, Color color)
    {
        roles[static_cast<std::size_t> (role)] = color;
    };

    std::vector<Color> remaining;
    for (const auto& color : sourceColors)
        remaining.push_back (color.withAlpha (1.0f));

    std::sort (remaining.begin(), remaining.end(), [] (Color a, Color b)
    {
        return a.getARGB() < b.getARGB();
    });
    remaining.erase (std::unique (remaining.begin(), remaining.end()), remaining.end());

    // Background: the darkest (or lightest) source, pushed to the extreme of the range
    auto background = *takeBest (remaining, [&] (Color c)
    {
        return -direction * lightnessOf (c);
    });

    if (mode == Mode::dark && lightnessOf (background) > darkBackgroundMaxLightness)
        background = withLightness (background, darkBackgroundMaxLightness);
    else if (mode == Mode::light && lightnessOf (background) < lightBackgroundMinLightness)
        background = withLightness (background, lightBackgroundMinLightness);

    const auto backgroundLightness = lightnessOf (background);

    // Text: the most contrasting source, made readable if needed
    auto text = takeBest (remaining, [&] (Color c)
                {
                    return contrastRatio (c, background);
                })
                    .value_or (Color::fromHSLuv (std::get<0> (background.toHSLuv()),
                                                 std::get<1> (background.toHSLuv()) * 0.2f,
                                                 mode == Mode::dark ? 0.93f : 0.12f));

    text = withContrast (text, background, minimumTextContrast, direction);

    // Accent: the most chromatic source, visible against the background
    auto accent = takeBest (remaining, [&] (Color c)
                  {
                      const auto chroma = chromaOf (c);
                      return chroma >= minimumAccentChroma ? chroma : notEligible;
                  })
                      .value_or (Color::fromHSLuv (std::get<0> (text.toHSLuv()), 0.85f, mode == Mode::dark ? 0.68f : 0.45f));

    accent = withContrast (accent, background, minimumAccentContrast, direction);

    // Surfaces: a source slightly away from the background, otherwise one step from it
    const auto surface = takeBest (remaining, [&] (Color c)
                         {
                             const auto delta = direction * (lightnessOf (c) - backgroundLightness);
                             if (delta < surfaceMinDelta || delta > surfaceMaxDelta)
                                 return notEligible;

                             return -static_cast<double> (std::abs (delta - surfaceIdealDelta));
                         })
                             .value_or (withLightness (background, backgroundLightness + direction * surfaceLightnessStep));

    const auto surfaceRaised = withLightness (surface, lightnessOf (surface) + direction * surfaceLightnessStep);

    // Muted text: the least chromatic source that reads, but less than the text does
    const auto textContrast = contrastRatio (text, background);
    const auto textMuted = takeBest (remaining, [&] (Color c)
                           {
                               const auto contrast = contrastRatio (c, background);
                               if (contrast < minimumAccentContrast || contrast > textContrast * 0.7)
                                   return notEligible;

                               return -chromaOf (c);
                           })
                               .value_or (text.mixedWith (background, 0.4f, ColorSpace::SRGB));

    set (Role::background, background);
    set (Role::surface, surface);
    set (Role::surfaceRaised, surfaceRaised);
    set (Role::outline, text.mixedWith (background, 0.8f, ColorSpace::SRGB));
    set (Role::text, text);
    set (Role::textMuted, textMuted);
    set (Role::accent, accent);
    set (Role::onAccent, contrastRatio (background, accent) >= contrastRatio (text, accent) ? background : text);
}

//==============================================================================

ResultValue<ThemePalette> ThemePalette::fromString (const String& text, Mode mode)
{
    std::vector<Color> colors;

    if (auto xml = parseXML (text))
    {
        for (auto* element : xml->getChildIterator())
        {
            if (! element->hasTagName ("color"))
                continue;

            if (element->hasAttribute ("hex"))
            {
                if (auto color = parseHexToken (element->getStringAttribute ("hex").trim().trimCharactersAtStart ("#")))
                    colors.push_back (*color);
            }
            else if (element->hasAttribute ("r") && element->hasAttribute ("g") && element->hasAttribute ("b"))
            {
                const auto component = [&] (StringRef name)
                {
                    return static_cast<uint8> (jlimit (0, 255, element->getIntAttribute (name)));
                };

                colors.emplace_back (component ("r"), component ("g"), component ("b"));
            }
        }
    }

    if (colors.empty())
    {
        String token;
        const auto flushToken = [&]
        {
            if (auto color = parseHexToken (token))
                colors.push_back (*color);

            token.clear();
        };

        for (auto t = text.getCharPointer(); ! t.isEmpty();)
        {
            const auto character = t.getAndAdvance();

            if (CharacterFunctions::isLetterOrDigit (character))
                token += character;
            else
                flushToken();
        }

        flushToken();
    }

    if (colors.empty())
        return makeResultValueFail ("No valid colors found");

    return makeResultValueOk (ThemePalette (colors, mode));
}

//==============================================================================

Color ThemePalette::getColor (Role role) const
{
    jassert (role != Role::numRoles);
    if (role == Role::numRoles)
        return {};

    return roles[static_cast<std::size_t> (role)];
}

void ThemePalette::setColor (Role role, Color color)
{
    jassert (role != Role::numRoles);
    if (role == Role::numRoles)
        return;

    roles[static_cast<std::size_t> (role)] = color;
}

ThemePalette::Mode ThemePalette::getMode() const
{
    return mode;
}

const std::vector<Color>& ThemePalette::getSourceColors() const
{
    return sourceColors;
}

//==============================================================================

bool ThemePalette::operator== (const ThemePalette& other) const
{
    return mode == other.mode && roles == other.roles;
}

bool ThemePalette::operator!= (const ThemePalette& other) const
{
    return ! (*this == other);
}

} // namespace yup
