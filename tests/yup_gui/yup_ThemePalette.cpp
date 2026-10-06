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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

// =============================================================================

class ThemePaletteTests : public ::testing::Test
{
protected:
    using Role = ThemePalette::Role;
    using Mode = ThemePalette::Mode;

    static std::vector<Color> frostedColors()
    {
        return { Color (0xff100b00), Color (0xff85cb33), Color (0xffefffc8), Color (0xffa5cbc3), Color (0xff3b341f) };
    }

    static String frostedXml()
    {
        return R"(<palette>
  <color name="Pitch Black" hex="100b00" r="16" g="11" b="0" />
  <color name="Yellow Green" hex="85cb33" r="133" g="203" b="51" />
  <color name="Frosted Mint" hex="efffc8" r="239" g="255" b="200" />
  <color name="Ash Grey" hex="a5cbc3" r="165" g="203" b="195" />
  <color name="Dark Khaki" hex="3b341f" r="59" g="52" b="31" />
</palette>)";
    }

    static double relativeLuminance (Color c)
    {
        const auto linearize = [] (float v)
        {
            return v <= 0.04045f ? v / 12.92 : std::pow ((v + 0.055) / 1.055, 2.4);
        };

        return 0.2126 * linearize (c.getRedFloat()) + 0.7152 * linearize (c.getGreenFloat()) + 0.0722 * linearize (c.getBlueFloat());
    }

    static double contrastRatio (Color a, Color b)
    {
        const auto la = relativeLuminance (a);
        const auto lb = relativeLuminance (b);
        return (jmax (la, lb) + 0.05) / (jmin (la, lb) + 0.05);
    }

    static void expectAllRolesEqual (const ThemePalette& a, const ThemePalette& b)
    {
        for (int i = 0; i < static_cast<int> (Role::numRoles); ++i)
            EXPECT_EQ (a.getColor (static_cast<Role> (i)), b.getColor (static_cast<Role> (i))) << "role " << i;
    }
};

// =============================================================================

TEST_F (ThemePaletteTests, FrostedPaletteDerivesExpectedRoles)
{
    const ThemePalette palette (frostedColors());

    EXPECT_EQ (Mode::dark, palette.getMode());
    EXPECT_EQ (Color (0xff100b00), palette.getColor (Role::background));
    EXPECT_EQ (Color (0xff3b341f), palette.getColor (Role::surface));
    EXPECT_EQ (Color (0xffefffc8), palette.getColor (Role::text));
    EXPECT_EQ (Color (0xff85cb33), palette.getColor (Role::accent));
    EXPECT_EQ (Color (0xffa5cbc3), palette.getColor (Role::textMuted));
}

TEST_F (ThemePaletteTests, SourceOrderDoesNotAffectRoles)
{
    const ThemePalette reference (frostedColors());

    auto shuffled = frostedColors();
    std::reverse (shuffled.begin(), shuffled.end());
    expectAllRolesEqual (reference, ThemePalette (shuffled));

    std::rotate (shuffled.begin(), shuffled.begin() + 2, shuffled.end());
    expectAllRolesEqual (reference, ThemePalette (shuffled));
}

TEST_F (ThemePaletteTests, SurfaceRaisedSitsFurtherFromBackgroundThanSurface)
{
    const ThemePalette palette (frostedColors());

    const auto backgroundL = std::get<2> (palette.getColor (Role::background).toHSLuv());
    const auto surfaceL = std::get<2> (palette.getColor (Role::surface).toHSLuv());
    const auto raisedL = std::get<2> (palette.getColor (Role::surfaceRaised).toHSLuv());

    EXPECT_GT (surfaceL, backgroundL);
    EXPECT_GT (raisedL, surfaceL);
}

TEST_F (ThemePaletteTests, EmptySourceGivesDefaultPalette)
{
    const ThemePalette defaultPalette;
    const ThemePalette empty (std::vector<Color> {});

    expectAllRolesEqual (defaultPalette, empty);
    EXPECT_FALSE (defaultPalette.getSourceColors().empty());
}

TEST_F (ThemePaletteTests, SingleColorIsCompletedByDerivation)
{
    const ThemePalette palette (std::vector<Color> { Color (0xff101820) });

    EXPECT_EQ (Color (0xff101820), palette.getColor (Role::background));
    EXPECT_GE (contrastRatio (palette.getColor (Role::text), palette.getColor (Role::background)), 4.5);
    EXPECT_GE (contrastRatio (palette.getColor (Role::accent), palette.getColor (Role::background)), 3.0);
    EXPECT_NE (palette.getColor (Role::surface), palette.getColor (Role::background));
}

TEST_F (ThemePaletteTests, TwoColorsAreCompletedByDerivation)
{
    const ThemePalette palette (std::vector<Color> { Color (0xfff5f5f5), Color (0xff101418) });

    EXPECT_EQ (Color (0xff101418), palette.getColor (Role::background));
    EXPECT_EQ (Color (0xfff5f5f5), palette.getColor (Role::text));
    EXPECT_GE (contrastRatio (palette.getColor (Role::accent), palette.getColor (Role::background)), 3.0);
}

TEST_F (ThemePaletteTests, TextContrastIsAlwaysReadable)
{
    Random random (1234);

    for (int iteration = 0; iteration < 32; ++iteration)
    {
        std::vector<Color> colors;
        const int numColors = 1 + random.nextInt (6);

        for (int i = 0; i < numColors; ++i)
            colors.push_back (Color (static_cast<uint32> (random.nextInt()) | 0xff000000u));

        for (const auto mode : { Mode::dark, Mode::light })
        {
            const ThemePalette palette (colors, mode);
            EXPECT_GE (contrastRatio (palette.getColor (Role::text), palette.getColor (Role::background)), 4.5)
                << "iteration " << iteration;
        }
    }
}

TEST_F (ThemePaletteTests, BackgroundIsClampedToModeExtreme)
{
    const ThemePalette palette (std::vector<Color> { Color (0xff808080), Color (0xffc0c0c0) });

    EXPECT_LE (std::get<2> (palette.getColor (Role::background).toHSLuv()), 0.125f);
}

TEST_F (ThemePaletteTests, LightModePicksLightBackground)
{
    const ThemePalette palette (frostedColors(), Mode::light);

    EXPECT_EQ (Mode::light, palette.getMode());
    EXPECT_EQ (Color (0xffefffc8), palette.getColor (Role::background));
    EXPECT_EQ (Color (0xff100b00), palette.getColor (Role::text));
    EXPECT_GE (contrastRatio (palette.getColor (Role::accent), palette.getColor (Role::background)), 3.0);
}

TEST_F (ThemePaletteTests, FromStringParsesXmlHexAttributes)
{
    auto result = ThemePalette::fromString (frostedXml());
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    EXPECT_EQ (frostedColors().size(), result.getReference().getSourceColors().size());
    expectAllRolesEqual (ThemePalette (frostedColors()), result.getReference());
}

TEST_F (ThemePaletteTests, FromStringParsesXmlRgbAttributes)
{
    auto result = ThemePalette::fromString (R"(<palette>
  <color r="16" g="11" b="0" />
  <color r="133" g="203" b="51" />
  <color r="239" g="255" b="200" />
  <color r="165" g="203" b="195" />
  <color r="59" g="52" b="31" />
</palette>)");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    expectAllRolesEqual (ThemePalette (frostedColors()), result.getReference());
}

TEST_F (ThemePaletteTests, FromStringParsesHexListsWithAndWithoutHash)
{
    auto withHash = ThemePalette::fromString ("#100b00, #85cb33, #efffc8, #a5cbc3, #3b341f");
    auto withoutHash = ThemePalette::fromString ("100b00 85cb33 efffc8 a5cbc3 3b341f");

    ASSERT_TRUE (withHash.wasOk());
    ASSERT_TRUE (withoutHash.wasOk());
    expectAllRolesEqual (ThemePalette (frostedColors()), withHash.getReference());
    expectAllRolesEqual (ThemePalette (frostedColors()), withoutHash.getReference());
}

TEST_F (ThemePaletteTests, FromStringParsesCoolorsUrl)
{
    auto result = ThemePalette::fromString ("https://coolors.co/100b00-85cb33-efffc8-a5cbc3-3b341f");

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (5u, result.getReference().getSourceColors().size());
    expectAllRolesEqual (ThemePalette (frostedColors()), result.getReference());
}

TEST_F (ThemePaletteTests, FromStringParsesEightDigitHexAsRgba)
{
    auto result = ThemePalette::fromString ("#102030ff");

    ASSERT_TRUE (result.wasOk());
    ASSERT_EQ (1u, result.getReference().getSourceColors().size());
    EXPECT_EQ (Color (0xff102030), result.getReference().getSourceColors().front());
}

TEST_F (ThemePaletteTests, FromStringFailsOnGarbage)
{
    EXPECT_TRUE (ThemePalette::fromString ("this is not a palette").failed());
    EXPECT_TRUE (ThemePalette::fromString ("").failed());
    EXPECT_TRUE (ThemePalette::fromString ("<palette><color hex=\"zzzzzz\" /></palette>").failed());
}

TEST_F (ThemePaletteTests, FromStringPassesModeThrough)
{
    auto result = ThemePalette::fromString (frostedXml(), Mode::light);

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (Mode::light, result.getReference().getMode());
    EXPECT_EQ (Color (0xffefffc8), result.getReference().getColor (Role::background));
}

TEST_F (ThemePaletteTests, SetColorOverridesDerivedRole)
{
    ThemePalette palette (frostedColors());
    const ThemePalette original (frostedColors());

    palette.setColor (Role::accent, Colors::red);

    EXPECT_EQ (Colors::red, palette.getColor (Role::accent));
    EXPECT_EQ (original.getColor (Role::background), palette.getColor (Role::background));
    EXPECT_NE (original, palette);
}

TEST_F (ThemePaletteTests, EqualityComparesRolesAndMode)
{
    EXPECT_EQ (ThemePalette (frostedColors()), ThemePalette (frostedColors()));
    EXPECT_NE (ThemePalette (frostedColors()), ThemePalette (frostedColors(), Mode::light));
    EXPECT_NE (ThemePalette(), ThemePalette (frostedColors()));
}
