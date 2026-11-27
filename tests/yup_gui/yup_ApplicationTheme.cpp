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

class ApplicationThemeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();

        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

// =============================================================================

TEST_F (ApplicationThemeTest, FindColorReturnsNulloptWhenColorNotRegistered)
{
    auto c = Component ("testComponent");

    auto result = ApplicationTheme::findComponentColor (c, Identifier ("unknownColor"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, FindColorReturnsRegisteredColor)
{
    auto c = Component ("testComponent");

    const Color expected = Color::fromRGBA (255, 128, 0, 255);
    theme->setColor (Identifier ("testColor"), expected);

    auto result = ApplicationTheme::findComponentColor (c, Identifier ("testColor"));
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), expected);
}

TEST_F (ApplicationThemeTest, SetColorsRegistersMultipleColors)
{
    auto c = Component ("testComponent");

    const Identifier idA ("colorA");
    const Identifier idB ("colorB");
    const Color colorA = Color::fromRGBA (10, 20, 30, 255);
    const Color colorB = Color::fromRGBA (40, 50, 60, 128);

    theme->setColors ({ { idA, colorA }, { idB, colorB } });

    auto resultA = ApplicationTheme::findComponentColor (c, idA);
    auto resultB = ApplicationTheme::findComponentColor (c, idB);

    ASSERT_TRUE (resultA.has_value());
    EXPECT_EQ (resultA.value(), colorA);
    ASSERT_TRUE (resultB.has_value());
    EXPECT_EQ (resultB.value(), colorB);
}

TEST_F (ApplicationThemeTest, SetColorOverwritesExistingColor)
{
    auto c = Component ("testComponent");

    const Identifier id ("myColor");
    const Color first = Color::fromRGBA (1, 2, 3, 255);
    const Color second = Color::fromRGBA (4, 5, 6, 255);

    theme->setColor (id, first);
    theme->setColor (id, second);

    auto result = ApplicationTheme::findComponentColor (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), second);
}

TEST_F (ApplicationThemeTest, ComponentColorOverridesRegisteredColor)
{
    auto c = Component ("testComponent");

    const Identifier id ("accentColor");
    const Color themeColor = Color::fromRGBA (1, 2, 3, 255);
    const Color componentColor = Color::fromRGBA (4, 5, 6, 255);

    theme->setColor (id, themeColor);
    c.setColor (id, componentColor);

    auto result = ApplicationTheme::findComponentColor (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), componentColor);

    c.setColor (id, std::nullopt);

    result = ApplicationTheme::findComponentColor (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), themeColor);
}

TEST_F (ApplicationThemeTest, FindColorUnregisteredIdDoesNotAffectOtherIds)
{
    auto c = Component ("testComponent");

    const Identifier registered ("registered");
    const Identifier unregistered ("unregistered");
    const Color color = Color::fromRGBA (0, 0, 255, 255);

    theme->setColor (registered, color);

    EXPECT_TRUE (ApplicationTheme::findComponentColor (c, registered).has_value());
    EXPECT_FALSE (ApplicationTheme::findComponentColor (c, unregistered).has_value());
}

// =============================================================================

TEST_F (ApplicationThemeTest, FindPropertyReturnsNulloptWhenPropertyNotRegistered)
{
    auto c = Component ("testComponent");

    auto result = ApplicationTheme::findComponentProperty (c, Identifier ("unknownProperty"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, FindPropertyReturnsRegisteredValue)
{
    auto c = Component ("testComponent");

    theme->setProperty (Identifier ("cornerRadius"), 8.0);

    auto result = ApplicationTheme::findComponentProperty (c, Identifier ("cornerRadius"));
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (static_cast<float> (result.value()), 8.0f);
}

TEST_F (ApplicationThemeTest, SetPropertyOverwritesExistingValue)
{
    auto c = Component ("testComponent");

    const Identifier id ("borderWidth");

    theme->setProperty (id, 1.0);
    theme->setProperty (id, 3.5);

    auto result = ApplicationTheme::findComponentProperty (c, id);
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (static_cast<float> (result.value()), 3.5f);
}

TEST_F (ApplicationThemeTest, SetPropertiesRegistersMultipleValues)
{
    auto c = Component ("testComponent");

    const Identifier idA ("smallSpacing");
    const Identifier idB ("largeSpacing");

    theme->setProperties ({ { idA, 4.0 }, { idB, 12.0 } });

    auto resultA = ApplicationTheme::findComponentProperty (c, idA);
    auto resultB = ApplicationTheme::findComponentProperty (c, idB);

    ASSERT_TRUE (resultA.has_value());
    EXPECT_FLOAT_EQ (static_cast<float> (resultA.value()), 4.0f);
    ASSERT_TRUE (resultB.has_value());
    EXPECT_FLOAT_EQ (static_cast<float> (resultB.value()), 12.0f);
}

TEST_F (ApplicationThemeTest, ComponentPropertyOverridesThemeDefault)
{
    auto c = Component ("testComponent");

    const Identifier id ("cornerRadius");

    theme->setProperty (id, 8.0);
    c.setProperty (id, 12.0);

    // The component's own cascade prefers its stored value over the theme default.
    EXPECT_FLOAT_EQ (static_cast<float> (c.findProperty (id)), 12.0f);

    // Clearing the component override falls back to the theme default.
    c.removeProperty (id);
    EXPECT_FLOAT_EQ (static_cast<float> (c.findProperty (id)), 8.0f);
}

TEST_F (ApplicationThemeTest, FindPropertyUnregisteredIdDoesNotAffectOtherIds)
{
    auto c = Component ("testComponent");

    const Identifier registered ("spacing");
    const Identifier unregistered ("padding");

    theme->setProperty (registered, 4.0);

    EXPECT_TRUE (ApplicationTheme::findComponentProperty (c, registered).has_value());
    EXPECT_FALSE (ApplicationTheme::findComponentProperty (c, unregistered).has_value());
}

TEST_F (ApplicationThemeTest, SetPropertyAcceptsZeroAndNegativeValues)
{
    auto c = Component ("testComponent");

    theme->setProperty (Identifier ("zeroProperty"), 0.0);
    theme->setProperty (Identifier ("negativeProperty"), -2.5);

    auto zero = ApplicationTheme::findComponentProperty (c, Identifier ("zeroProperty"));
    auto negative = ApplicationTheme::findComponentProperty (c, Identifier ("negativeProperty"));

    ASSERT_TRUE (zero.has_value());
    EXPECT_FLOAT_EQ (static_cast<float> (zero.value()), 0.0f);

    ASSERT_TRUE (negative.has_value());
    EXPECT_FLOAT_EQ (static_cast<float> (negative.value()), -2.5f);
}

// =============================================================================

TEST_F (ApplicationThemeTest, SetGlobalThemeRoundTrip)
{
    auto themePtr = ApplicationTheme::getGlobalTheme();
    ASSERT_NE (nullptr, themePtr.get());

    auto newTheme = new ApplicationTheme();
    ApplicationTheme::setGlobalTheme (newTheme);

    auto retrieved = ApplicationTheme::getGlobalTheme();
    EXPECT_EQ (newTheme, retrieved.get());

    ApplicationTheme::setGlobalTheme (themePtr.get());
}

TEST_F (ApplicationThemeTest, GetGlobalThemeReturnsNonNull)
{
    auto global = ApplicationTheme::getGlobalTheme();
    EXPECT_NE (nullptr, global.get());
}

TEST_F (ApplicationThemeTest, SetDefaultFont)
{
    auto originalFont = theme->getDefaultFont();

    Font newFont;
    theme->setDefaultFont (newFont);

    EXPECT_EQ (newFont, theme->getDefaultFont());
}

TEST_F (ApplicationThemeTest, GetDefaultFontReturnsValid)
{
    auto font = theme->getDefaultFont();
    EXPECT_TRUE (font.getHeight() >= 0.0f);
}

TEST_F (ApplicationThemeTest, SetDefaultIconFont)
{
    auto originalFont = theme->getDefaultIconFont();

    Font newFont;
    theme->setDefaultIconFont (newFont);

    EXPECT_EQ (newFont, theme->getDefaultIconFont());
}

TEST_F (ApplicationThemeTest, GetDefaultIconFontReturnsValid)
{
    auto font = theme->getDefaultIconFont();
    EXPECT_TRUE (font.getHeight() >= 0.0f);
}

TEST_F (ApplicationThemeTest, FindPropertyInstanceMethodReturnsRegisteredValue)
{
    Component c ("testComponent");

    theme->setProperty (Identifier ("myProperty"), 15.0);

    auto result = theme->findProperty (c, Identifier ("myProperty"));
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (15.0f, static_cast<float> (result.value()));
}

TEST_F (ApplicationThemeTest, FindPropertyInstanceMethodReturnsNulloptForUnknown)
{
    Component c ("testComponent");

    auto result = theme->findProperty (c, Identifier ("unknownProperty"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, FindPropertyInstanceMethodIgnoresComponentOverride)
{
    Component c ("testComponent");

    theme->setProperty (Identifier ("radius"), 10.0);
    c.setProperty (Identifier ("radius"), 25.0);

    // The theme lookup only sees its own defaults; the component store is not consulted here.
    auto result = theme->findProperty (c, Identifier ("radius"));
    ASSERT_TRUE (result.has_value());
    EXPECT_FLOAT_EQ (10.0f, static_cast<float> (result.value()));

    // The component's own cascade, however, prefers its stored override.
    EXPECT_FLOAT_EQ (25.0f, static_cast<float> (c.findProperty (Identifier ("radius"))));
}

TEST_F (ApplicationThemeTest, SetComponentStyleRegistersStyle)
{
    ComponentStyle::Ptr style = new MockComponentStyle();
    theme->setComponentStyle<ProgressBar> (style);

    ProgressBar bar;
    auto found = ApplicationTheme::findComponentStyle (bar);
    EXPECT_EQ (style.get(), found.get());
}

TEST_F (ApplicationThemeTest, FindComponentStyleReturnsNullWhenNotRegistered)
{
    Slider slider (Slider::LinearHorizontal);
    auto found = ApplicationTheme::findComponentStyle (slider);
    EXPECT_EQ (nullptr, found.get());
}

TEST_F (ApplicationThemeTest, FindComponentStylePrefersComponentInstanceStyle)
{
    ComponentStyle::Ptr themeStyle = new MockComponentStyle();
    theme->setComponentStyle<Label> (themeStyle);

    Label label ("test");

    auto found = ApplicationTheme::findComponentStyle (label);
    EXPECT_EQ (themeStyle.get(), found.get());

    ComponentStyle::Ptr instanceStyle = new MockComponentStyle();
    label.setStyle (instanceStyle);

    found = ApplicationTheme::findComponentStyle (label);
    EXPECT_EQ (instanceStyle.get(), found.get());
}

TEST_F (ApplicationThemeTest, AStyleSetAgainForAClassReplacesTheOneBefore)
{
    ComponentStyle::Ptr first = new MockComponentStyle();
    ComponentStyle::Ptr second = new MockComponentStyle();

    theme->setComponentStyle<ProgressBar> (first);
    theme->setComponentStyle<ProgressBar> (second);

    ProgressBar bar;
    EXPECT_EQ (ApplicationTheme::findComponentStyle (bar).get(), second.get());

    // Without one, the class is painted as what it derives from, which here has no style either
    theme->setComponentStyle<ProgressBar> (nullptr);
    EXPECT_EQ (ApplicationTheme::findComponentStyle (bar).get(), nullptr);
}

TEST_F (ApplicationThemeTest, AClassWithNoStyleIsPaintedAsTheClosestOneItsTypeDerivesFrom)
{
    struct Fader : Slider
    {
        Fader()
            : Slider (Slider::LinearVertical)
        {
        }
    };

    ComponentStyle::Ptr sliderStyle = new MockComponentStyle();
    theme->setComponentStyle<Slider> (sliderStyle);

    // Asked as a Component, as Component::paint() asks: the type of the component leads to the class with a style
    Fader fader;
    Component& asComponent = fader;
    EXPECT_EQ (ApplicationTheme::findComponentStyle (asComponent).get(), sliderStyle.get());

    // A type given to one component describes no class of its own, and leads to the one it derives from
    const ComponentTypeInfo knobType (Identifier ("Knob"), std::addressof (Slider::getClassTypeInfo()), {});
    Slider knob (Slider::RotaryVerticalDrag);
    knob.setTypeInfo (std::addressof (knobType));
    EXPECT_EQ (ApplicationTheme::findComponentStyle (static_cast<Component&> (knob)).get(), sliderStyle.get());

    // A style of its own class comes first, whether or not the class tells a type
    ComponentStyle::Ptr faderStyle = new MockComponentStyle();
    theme->setComponentStyle<Fader> (faderStyle);
    EXPECT_EQ (ApplicationTheme::findComponentStyle (asComponent).get(), faderStyle.get());

    // And what does not derive from the class is not painted as it
    Label label ("test");
    EXPECT_EQ (ApplicationTheme::findComponentStyle (label).get(), nullptr);
}

TEST_F (ApplicationThemeTest, AComponentIsPaintedWithTheStyleOfItsClassOrTheOneSetOnIt)
{
    struct Panel : Component
    {
    };

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (32, 32);
    Graphics g (*context, *renderer);

    const Component* painted = nullptr;
    int numPaints = 0;

    theme->setComponentStyle<Slider> (ComponentStyle::createStyle<Slider> ([&] (Graphics&, const ApplicationTheme& paintingTheme, const Slider& slider)
    {
        EXPECT_EQ (std::addressof (paintingTheme), theme.get());

        painted = std::addressof (slider);
        ++numPaints;
    }));

    Slider slider (Slider::LinearHorizontal);
    slider.paint (g);

    EXPECT_EQ (painted, std::addressof (slider));
    EXPECT_EQ (numPaints, 1);

    // A component of a class with no style paints nothing
    Panel panel;
    panel.setOpaque (false);
    panel.paint (g);
    EXPECT_EQ (numPaints, 1);

    // The style set on a component paints it in place of the one of its class
    int numOwnPaints = 0;

    slider.setStyle (ComponentStyle::createStyle<Slider> ([&numOwnPaints] (Graphics&, const ApplicationTheme&, const Slider&)
    {
        ++numOwnPaints;
    }));

    slider.paint (g);
    EXPECT_EQ (numOwnPaints, 1);
    EXPECT_EQ (numPaints, 1);
}

TEST_F (ApplicationThemeTest, NoStyleOfAClassIsFoundWithoutAGlobalTheme)
{
    ComponentStyle::Ptr classStyle = new MockComponentStyle();
    theme->setComponentStyle<Label> (classStyle);

    Label label ("test");
    Label styled ("styled");

    ComponentStyle::Ptr ownStyle = new MockComponentStyle();
    styled.setStyle (ownStyle);

    ApplicationTheme::setGlobalTheme (nullptr);

    EXPECT_EQ (ApplicationTheme::findComponentStyle (label).get(), nullptr);
    EXPECT_EQ (ApplicationTheme::findComponentStyle (styled).get(), ownStyle.get());

    ApplicationTheme::setGlobalTheme (theme);
}

TEST_F (ApplicationThemeTest, SettingAStyleOrThePainterOfAPartChangesTheGeneration)
{
    auto generation = theme->getPaletteGeneration();

    theme->setComponentStyle<Label> (new MockComponentStyle());
    EXPECT_NE (theme->getPaletteGeneration(), generation);

    generation = theme->getPaletteGeneration();

    theme->setComponentPart<Label> (Identifier ("text"), [] (Graphics&, const ApplicationTheme&, const Label&, const Rectangle<float>&) {});
    EXPECT_NE (theme->getPaletteGeneration(), generation);
}

TEST_F (ApplicationThemeTest, PaintsAPartOfAComponentTypeWithThePainterSetForIt)
{
    struct TitleLabel : Label
    {
        TitleLabel()
            : Label ("title")
        {
        }
    };

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (64, 16);
    Graphics g (*context, *renderer);

    Label label ("test");
    const Identifier text ("text");
    const Rectangle<float> area (4.0f, 2.0f, 40.0f, 12.0f);

    // A theme paints nothing of a part it has no painter for
    EXPECT_FALSE (theme->paintPart (g, label, text, area));
    EXPECT_FALSE (ApplicationTheme::paintComponentPart (g, label, text, area));

    const Label* paintedLabel = nullptr;
    Rectangle<float> paintedArea;

    theme->setComponentPart<Label> (text, [&] (Graphics&, const ApplicationTheme& paintingTheme, const Label& l, const Rectangle<float>& a)
    {
        EXPECT_EQ (std::addressof (paintingTheme), theme.get());

        paintedLabel = std::addressof (l);
        paintedArea = a;
    });

    EXPECT_TRUE (ApplicationTheme::paintComponentPart (g, label, text, area));
    EXPECT_EQ (paintedLabel, std::addressof (label));
    EXPECT_EQ (paintedArea, area);

    // The other parts of the type are still unpainted
    EXPECT_FALSE (theme->paintPart (g, label, Identifier ("textOutline"), area));

    // A class deriving from the type is painted as the type is
    TitleLabel title;
    EXPECT_TRUE (theme->paintPart (g, title, text, area));
    EXPECT_EQ (paintedLabel, static_cast<const Label*> (std::addressof (title)));

    // A painter set again replaces the one before
    int numReplaced = 0;
    theme->setComponentPart<Label> (text, [&numReplaced] (Graphics&, const ApplicationTheme&, const Label&, const Rectangle<float>&)
    {
        ++numReplaced;
    });

    paintedLabel = nullptr;
    EXPECT_TRUE (theme->paintPart (g, label, text, area));
    EXPECT_EQ (numReplaced, 1);
    EXPECT_EQ (paintedLabel, nullptr);
}

TEST_F (ApplicationThemeTest, FindColorInstanceMethod)
{
    Component c ("testComponent");

    const Color expected = Color::fromRGBA (10, 20, 30, 255);
    theme->setColor (Identifier ("bg"), expected);

    auto result = theme->findColor (c, Identifier ("bg"));
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), expected);
}

TEST_F (ApplicationThemeTest, FindColorInstanceMethodReturnsNulloptForUnknown)
{
    Component c ("testComponent");

    auto result = theme->findColor (c, Identifier ("unknown"));
    EXPECT_FALSE (result.has_value());
}

TEST_F (ApplicationThemeTest, ComponentPaletteColorsTheSubtreeThroughTheMapping)
{
    const Identifier accentId ("accentId");
    theme->setPaletteMapping ([accentId] (const ThemePalette& palette, ApplicationTheme& t)
    {
        t.setPaletteColor (accentId, palette.getColor (ThemePalette::Role::accent));
    });

    const ThemePalette ownPalette (std::vector<Color> { Color (0xff101010), Color (0xff0080ff) });
    theme->setPalette (ThemePalette (std::vector<Color> { Color (0xff101010), Color (0xffff8000) }));

    Component parent ("parent");
    Component child ("child");
    Component outside ("outside");
    parent.addChildComponent (child);
    parent.setPalette (ownPalette);

    EXPECT_EQ (ownPalette.getColor (ThemePalette::Role::accent), theme->findColor (child, accentId));
    EXPECT_EQ (theme->getPalette().getColor (ThemePalette::Role::accent), theme->findColor (outside, accentId));

    EXPECT_EQ (ownPalette, theme->getPalette (child));
    EXPECT_EQ (theme->getPalette(), theme->getPalette (outside));
}

TEST_F (ApplicationThemeTest, ExplicitThemeColorWinsOverComponentPalette)
{
    const Identifier accentId ("accentId");
    theme->setPaletteMapping ([accentId] (const ThemePalette& palette, ApplicationTheme& t)
    {
        t.setPaletteColor (accentId, palette.getColor (ThemePalette::Role::accent));
    });

    const Color explicitColor (0xff123456);
    theme->setColor (accentId, explicitColor);

    Component c ("component");
    c.setPalette (ThemePalette (std::vector<Color> { Color (0xff101010), Color (0xff0080ff) }));

    EXPECT_EQ (explicitColor, theme->findColor (c, accentId));
}

TEST_F (ApplicationThemeTest, ChangingPaletteMappingRemapsComponentPalettes)
{
    const Identifier colorId ("colorId");
    const auto mapRole = [colorId] (ThemePalette::Role role)
    {
        return [colorId, role] (const ThemePalette& palette, ApplicationTheme& t)
        {
            t.setPaletteColor (colorId, palette.getColor (role));
        };
    };

    const ThemePalette ownPalette (std::vector<Color> { Color (0xff101010), Color (0xff0080ff) });
    Component c ("component");
    c.setPalette (ownPalette);

    theme->setPaletteMapping (mapRole (ThemePalette::Role::accent));
    EXPECT_EQ (ownPalette.getColor (ThemePalette::Role::accent), theme->findColor (c, colorId));

    theme->setPaletteMapping (mapRole (ThemePalette::Role::background));
    EXPECT_EQ (ownPalette.getColor (ThemePalette::Role::background), theme->findColor (c, colorId));
}

TEST_F (ApplicationThemeTest, FindColorInstanceMethodRespectsComponentOverride)
{
    Component c ("testComponent");

    const Color themeColor = Color::fromRGBA (1, 2, 3, 255);
    const Color compColor = Color::fromRGBA (4, 5, 6, 255);
    theme->setColor (Identifier ("fg"), themeColor);
    c.setColor (Identifier ("fg"), compColor);

    auto result = theme->findColor (c, Identifier ("fg"));
    ASSERT_TRUE (result.has_value());
    EXPECT_EQ (result.value(), compColor);
}

TEST_F (ApplicationThemeTest, SetDefaultMonospaceFont)
{
    auto originalFont = theme->getDefaultMonospaceFont();

    Font newFont;
    theme->setDefaultMonospaceFont (newFont);

    EXPECT_EQ (newFont, theme->getDefaultMonospaceFont());
}

TEST_F (ApplicationThemeTest, GetDefaultMonospaceFontReturnsValid)
{
    auto font = theme->getDefaultMonospaceFont();
    EXPECT_TRUE (font.getHeight() >= 0.0f);
}

// =============================================================================

TEST_F (ApplicationThemeTest, DefaultPaletteIsAvailableWithoutMapping)
{
    EXPECT_EQ (ThemePalette(), theme->getPalette());
}

TEST_F (ApplicationThemeTest, PaletteMappingFeedsFindColor)
{
    auto c = Component ("testComponent");
    const Identifier id ("paletteBackedColor");

    theme->setPaletteMapping ([id] (const ThemePalette& palette, ApplicationTheme& t)
    {
        t.setPaletteColor (id, palette.getColor (ThemePalette::Role::accent));
    });

    auto palette = ThemePalette();
    palette.setColor (ThemePalette::Role::accent, Colors::red);
    theme->setPalette (palette);

    EXPECT_EQ (palette, theme->getPalette());
    ASSERT_TRUE (ApplicationTheme::findComponentColor (c, id).has_value());
    EXPECT_EQ (Colors::red, *ApplicationTheme::findComponentColor (c, id));

    palette.setColor (ThemePalette::Role::accent, Colors::green);
    theme->setPalette (palette);
    EXPECT_EQ (Colors::green, *ApplicationTheme::findComponentColor (c, id));
}

TEST_F (ApplicationThemeTest, ColorLookupOrderIsComponentThenThemeThenPalette)
{
    auto c = Component ("testComponent");
    const Identifier id ("layeredColor");

    theme->setPaletteMapping ([id] (const ThemePalette&, ApplicationTheme& t)
    {
        t.setPaletteColor (id, Colors::blue);
    });

    EXPECT_EQ (Colors::blue, *ApplicationTheme::findComponentColor (c, id));

    theme->setColor (id, Colors::green);
    EXPECT_EQ (Colors::green, *ApplicationTheme::findComponentColor (c, id));

    c.setColor (id, Colors::red);
    EXPECT_EQ (Colors::red, *ApplicationTheme::findComponentColor (c, id));
}

TEST_F (ApplicationThemeTest, SetPaletteReplacesPreviousPaletteColors)
{
    auto c = Component ("testComponent");
    const Identifier accentOnly ("accentOnly");

    theme->setPaletteMapping ([accentOnly] (const ThemePalette& palette, ApplicationTheme& t)
    {
        if (palette.getMode() == ThemePalette::Mode::dark)
            t.setPaletteColor (accentOnly, palette.getColor (ThemePalette::Role::accent));
    });

    EXPECT_TRUE (ApplicationTheme::findComponentColor (c, accentOnly).has_value());

    theme->setPalette (ThemePalette (std::vector<Color> { Colors::white, Colors::black }, ThemePalette::Mode::light));
    EXPECT_FALSE (ApplicationTheme::findComponentColor (c, accentOnly).has_value());
}

TEST_F (ApplicationThemeTest, FindDefaultColorSkipsComponentOverridesAndUsesScopedPalette)
{
    Component parent ("parent");
    Component child ("child");
    parent.addChildComponent (child);

    const Identifier id ("scopedColor");
    theme->setPaletteMapping ([id] (const ThemePalette& palette, ApplicationTheme& t)
    {
        t.setPaletteColor (id, palette.getColor (ThemePalette::Role::accent));
    });

    ThemePalette scoped;
    scoped.setColor (ThemePalette::Role::accent, Colors::cyan);
    parent.setPalette (scoped);
    child.setColor (id, Colors::red);

    EXPECT_EQ (theme->findColor (child, id), Colors::red);
    EXPECT_EQ (theme->findDefaultColor (child, id), Colors::cyan);

    theme->setColor (id, Colors::yellow);
    EXPECT_EQ (theme->findDefaultColor (child, id), Colors::yellow);
}

TEST_F (ApplicationThemeTest, ComponentPaletteCacheIsBoundedAcrossDistinctPalettes)
{
    const Identifier id ("accentId");
    theme->setPaletteMapping ([id] (const ThemePalette& palette, ApplicationTheme& t)
    {
        t.setPaletteColor (id, palette.getColor (ThemePalette::Role::accent));
    });

    Component component ("target");
    const size_t totalPalettes = ApplicationTheme::maxCachedComponentPalettes * 3;

    for (size_t i = 0; i < totalPalettes; ++i)
    {
        const Color accent = Color::fromRGBA (static_cast<uint8> (i & 0xff),
                                              static_cast<uint8> ((i * 7) & 0xff),
                                              static_cast<uint8> ((i * 13) & 0xff),
                                              255);

        ThemePalette palette;
        palette.setColor (ThemePalette::Role::accent, accent);
        component.setPalette (palette);

        EXPECT_EQ (theme->findColor (component, id), accent);
        EXPECT_LE (theme->getNumCachedComponentPalettes(), ApplicationTheme::maxCachedComponentPalettes);
    }

    EXPECT_EQ (theme->getNumCachedComponentPalettes(), ApplicationTheme::maxCachedComponentPalettes);
}


