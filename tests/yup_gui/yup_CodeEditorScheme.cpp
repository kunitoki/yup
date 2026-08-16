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

// ==============================================================================
// Default scheme
// ==============================================================================

TEST (CodeEditorSchemeTests, DefaultSchemeProvidesEveryColor)
{
    const CodeEditorScheme scheme;

    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::background).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::text).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::caret).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::currentLine).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::gutterBackground).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::gutterText).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::selection).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::searchHighlight).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::breakpoint).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::minimapBackground).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::minimapForeground).has_value());
    EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::minimapViewport).has_value());

    for (int i = 0; i < 10; ++i)
        EXPECT_TRUE (scheme.getColor (static_cast<SyntaxDefinition::TokenType> (i)).has_value());
}

TEST (CodeEditorSchemeTests, UnknownColorIdentifierReturnsNullopt)
{
    const CodeEditorScheme scheme;

    EXPECT_FALSE (scheme.getColor (Identifier ("doesNotExist")).has_value());
    EXPECT_FALSE (scheme.hasColor (Identifier ("doesNotExist")));
}

TEST (CodeEditorSchemeTests, DefaultSchemeBackgroundIsDark)
{
    const CodeEditorScheme scheme;

    EXPECT_EQ (Color (0xff1e1e1e), scheme.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0)));
}

// ==============================================================================
// Set / get round trips
// ==============================================================================

TEST (CodeEditorSchemeTests, SetAndGetColorRoundTrips)
{
    CodeEditorScheme scheme;

    const Color color (0xff123456);
    scheme.setColor (CodeEditorScheme::ColorId::background, color);

    EXPECT_TRUE (scheme.hasColor (CodeEditorScheme::ColorId::background));
    EXPECT_EQ (color, scheme.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0)));
}

TEST (CodeEditorSchemeTests, SetAndGetTokenColorRoundTrips)
{
    CodeEditorScheme scheme;

    const Color color (0xffabcdef);
    scheme.setColor (SyntaxDefinition::TokenType::keyword, color);

    EXPECT_EQ (color, scheme.getColor (SyntaxDefinition::TokenType::keyword).value_or (Color (0)));
}

TEST (CodeEditorSchemeTests, OverridingAColorReplacesIt)
{
    CodeEditorScheme scheme;

    scheme.setColor (CodeEditorScheme::ColorId::text, Color (0xff000000));
    scheme.setColor (CodeEditorScheme::ColorId::text, Color (0xffffffff));

    EXPECT_EQ (Color (0xffffffff), scheme.getColor (CodeEditorScheme::ColorId::text).value_or (Color (0)));
}

TEST (CodeEditorSchemeTests, NameRoundTrips)
{
    CodeEditorScheme scheme;

    EXPECT_TRUE (scheme.getName().isEmpty());

    scheme.setName ("My Scheme");
    EXPECT_EQ (String ("My Scheme"), scheme.getName());
}

// ==============================================================================
// Token color identifiers
// ==============================================================================

TEST (CodeEditorSchemeTests, TokenTypeToColorIdMatchesSyntaxNames)
{
    EXPECT_EQ (Identifier ("keyword"), CodeEditorScheme::tokenTypeToColorId (SyntaxDefinition::TokenType::keyword));
    EXPECT_EQ (Identifier ("string"), CodeEditorScheme::tokenTypeToColorId (SyntaxDefinition::TokenType::string));
    EXPECT_EQ (Identifier ("comment"), CodeEditorScheme::tokenTypeToColorId (SyntaxDefinition::TokenType::comment));
    EXPECT_EQ (Identifier ("other"), CodeEditorScheme::tokenTypeToColorId (SyntaxDefinition::TokenType::other));
}

// ==============================================================================
// Built-in schemes
// ==============================================================================

TEST (CodeEditorSchemeTests, AvailableSchemeNamesResolveToBuiltIns)
{
    const auto names = CodeEditorScheme::getAvailableSchemeNames();

    ASSERT_EQ (5u, names.size());

    for (const auto& name : names)
    {
        EXPECT_NE (nullptr, CodeEditorScheme::getBuiltInForName (name));
        EXPECT_FALSE (CodeEditorScheme::getBuiltIn (name).getName().isEmpty());
    }
}

TEST (CodeEditorSchemeTests, GetBuiltInForUnknownNameReturnsNullptr)
{
    EXPECT_EQ (nullptr, CodeEditorScheme::getBuiltInForName ("notAScheme"));
    EXPECT_EQ (nullptr, CodeEditorScheme::getBuiltInForName (""));
}

TEST (CodeEditorSchemeTests, GetBuiltInIsCaseInsensitiveAndIgnoresSpaces)
{
    const auto& canonical = CodeEditorScheme::getBuiltIn ("One Dark");
    const auto& spaced = CodeEditorScheme::getBuiltIn ("oneDark");
    const auto& dashed = CodeEditorScheme::getBuiltIn ("one-dark");

    const auto background = canonical.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0));

    EXPECT_EQ (background, spaced.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0)));
    EXPECT_EQ (background, dashed.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0)));
}

TEST (CodeEditorSchemeTests, BuiltInSchemesAreComplete)
{
    for (const auto& name : CodeEditorScheme::getAvailableSchemeNames())
    {
        const auto& scheme = CodeEditorScheme::getBuiltIn (name);

        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::background).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::text).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::caret).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::currentLine).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::gutterBackground).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::gutterText).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::selection).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::searchHighlight).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::breakpoint).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::minimapBackground).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::minimapForeground).has_value());
        EXPECT_TRUE (scheme.getColor (CodeEditorScheme::ColorId::minimapViewport).has_value());

        for (int i = 0; i < 10; ++i)
            EXPECT_TRUE (scheme.getColor (static_cast<SyntaxDefinition::TokenType> (i)).has_value());
    }
}

TEST (CodeEditorSchemeTests, BuiltInSchemesHaveDistinctBackgrounds)
{
    const auto& monokai = CodeEditorScheme::getBuiltIn ("monokai");
    const auto& alabaster = CodeEditorScheme::getBuiltIn ("alabaster");
    const auto& oneDark = CodeEditorScheme::getBuiltIn ("oneDark");
    const auto& solarizedDark = CodeEditorScheme::getBuiltIn ("solarizedDark");
    const auto& solarizedLight = CodeEditorScheme::getBuiltIn ("solarizedLight");

    const auto monokaiBg = monokai.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0));
    const auto alabasterBg = alabaster.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0));
    const auto oneDarkBg = oneDark.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0));
    const auto solarizedDarkBg = solarizedDark.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0));
    const auto solarizedLightBg = solarizedLight.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0));

    EXPECT_NE (monokaiBg, alabasterBg);
    EXPECT_NE (monokaiBg, oneDarkBg);
    EXPECT_NE (oneDarkBg, solarizedDarkBg);
    EXPECT_NE (solarizedDarkBg, solarizedLightBg);
    EXPECT_NE (alabasterBg, solarizedLightBg);
}

TEST (CodeEditorSchemeTests, GetDefaultReturnsDarkScheme)
{
    const auto& defaultScheme = CodeEditorScheme::getDefault();

    EXPECT_TRUE (defaultScheme.getColor (CodeEditorScheme::ColorId::background).has_value());
    EXPECT_EQ (Color (0xff1e1e1e), defaultScheme.getColor (CodeEditorScheme::ColorId::background).value_or (Color (0)));
}
