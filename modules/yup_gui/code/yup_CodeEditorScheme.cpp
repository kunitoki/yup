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

CodeEditorScheme::CodeEditorScheme()
{
    // Seed the classic dark editor palette. Individual colors can be overridden
    // afterwards with setColor to build custom schemes.
    setColor (ColorId::background, Color (0xff1e1e1e));
    setColor (ColorId::text, Color (0xffd4d4d4));
    setColor (ColorId::caret, Color (0xffffffff));
    setColor (ColorId::currentLine, Color (0x11222222));
    setColor (ColorId::gutterBackground, Color (0xff252526));
    setColor (ColorId::gutterText, Color (0xff858585));
    setColor (ColorId::selection, Color (0x33264692));
    setColor (ColorId::searchHighlight, Color (0x3348c0ff));
    setColor (ColorId::breakpoint, Color (0xffe51400));
    setColor (ColorId::minimapBackground, Color (0xff202020));
    setColor (ColorId::minimapForeground, Color (0x66ffffff));
    setColor (ColorId::minimapViewport, Color (0x3300a0ff));

    setColor (SyntaxDefinition::TokenType::comment, Color (0xff6a9955));
    setColor (SyntaxDefinition::TokenType::string, Color (0xffce9178));
    setColor (SyntaxDefinition::TokenType::number, Color (0xffb5cea8));
    setColor (SyntaxDefinition::TokenType::keyword, Color (0xff569cd6));
    setColor (SyntaxDefinition::TokenType::type, Color (0xff4ec9b0));
    setColor (SyntaxDefinition::TokenType::operator_, Color (0xffd4d4d4));
    setColor (SyntaxDefinition::TokenType::preprocessor, Color (0xffc586c0));
    setColor (SyntaxDefinition::TokenType::identifier, Color (0xffd4d4d4));
    setColor (SyntaxDefinition::TokenType::whitespace, Color (0xffd4d4d4));
    setColor (SyntaxDefinition::TokenType::other, Color (0xffd4d4d4));
}

//==============================================================================

const String& CodeEditorScheme::getName() const noexcept
{
    return name;
}

void CodeEditorScheme::setName (StringRef newName)
{
    name = newName;
}

//==============================================================================

void CodeEditorScheme::setColor (const Identifier& colorId, Color color)
{
    colors.insert_or_assign (colorId, color);
}

std::optional<Color> CodeEditorScheme::getColor (const Identifier& colorId) const
{
    if (const auto it = colors.find (colorId); it != colors.end())
        return it->second;

    return std::nullopt;
}

bool CodeEditorScheme::hasColor (const Identifier& colorId) const
{
    return colors.find (colorId) != colors.end();
}

//==============================================================================

void CodeEditorScheme::setColor (SyntaxDefinition::TokenType tokenType, Color color)
{
    setColor (tokenTypeToColorId (tokenType), color);
}

std::optional<Color> CodeEditorScheme::getColor (SyntaxDefinition::TokenType tokenType) const
{
    return getColor (tokenTypeToColorId (tokenType));
}

Identifier CodeEditorScheme::tokenTypeToColorId (SyntaxDefinition::TokenType tokenType)
{
    static const std::array<Identifier, 10> colorIds = []()
    {
        std::array<Identifier, 10> ids;
        for (size_t i = 0; i < ids.size(); ++i)
            ids[i] = Identifier (SyntaxDefinition::tokenTypeToString (static_cast<SyntaxDefinition::TokenType> (i)));
        return ids;
    }();

    return colorIds[static_cast<size_t> (tokenType)];
}

//==============================================================================

namespace
{

String normalizeSchemeName (StringRef schemeName)
{
    return String (schemeName).removeCharacters (" -_").toLowerCase();
}

CodeEditorScheme makeMonokai()
{
    CodeEditorScheme scheme;
    scheme.setName ("Monokai");

    scheme.setColor (CodeEditorScheme::ColorId::background, Color (0xff272822));
    scheme.setColor (CodeEditorScheme::ColorId::text, Color (0xfff8f8f2));
    scheme.setColor (CodeEditorScheme::ColorId::caret, Color (0xfff8f8f0));
    scheme.setColor (CodeEditorScheme::ColorId::currentLine, Color (0xff3e3d32));
    scheme.setColor (CodeEditorScheme::ColorId::gutterBackground, Color (0xff272822));
    scheme.setColor (CodeEditorScheme::ColorId::gutterText, Color (0xff90908a));
    scheme.setColor (CodeEditorScheme::ColorId::selection, Color (0xff49483e));
    scheme.setColor (CodeEditorScheme::ColorId::searchHighlight, Color (0x665f5a41));
    scheme.setColor (CodeEditorScheme::ColorId::breakpoint, Color (0xfff92672));
    scheme.setColor (CodeEditorScheme::ColorId::minimapBackground, Color (0xff1f1f1c));
    scheme.setColor (CodeEditorScheme::ColorId::minimapForeground, Color (0x66f8f8f2));
    scheme.setColor (CodeEditorScheme::ColorId::minimapViewport, Color (0x3366d9ef));

    scheme.setColor (SyntaxDefinition::TokenType::comment, Color (0xff75715e));
    scheme.setColor (SyntaxDefinition::TokenType::string, Color (0xffe6db74));
    scheme.setColor (SyntaxDefinition::TokenType::number, Color (0xffae81ff));
    scheme.setColor (SyntaxDefinition::TokenType::keyword, Color (0xfff92672));
    scheme.setColor (SyntaxDefinition::TokenType::type, Color (0xff66d9ef));
    scheme.setColor (SyntaxDefinition::TokenType::operator_, Color (0xfff8f8f2));
    scheme.setColor (SyntaxDefinition::TokenType::preprocessor, Color (0xffa6e22e));
    scheme.setColor (SyntaxDefinition::TokenType::identifier, Color (0xfff8f8f2));
    scheme.setColor (SyntaxDefinition::TokenType::whitespace, Color (0xfff8f8f2));
    scheme.setColor (SyntaxDefinition::TokenType::other, Color (0xfff8f8f2));

    return scheme;
}

CodeEditorScheme makeAlabaster()
{
    CodeEditorScheme scheme;
    scheme.setName ("Alabaster");

    scheme.setColor (CodeEditorScheme::ColorId::background, Color (0xfff7f7f7));
    scheme.setColor (CodeEditorScheme::ColorId::text, Color (0xff000000));
    scheme.setColor (CodeEditorScheme::ColorId::caret, Color (0xff007acc)); // "active"
    scheme.setColor (CodeEditorScheme::ColorId::currentLine, Color (0xfff0f0f0));
    scheme.setColor (CodeEditorScheme::ColorId::gutterBackground, Color (0xfff2f2f2));
    scheme.setColor (CodeEditorScheme::ColorId::gutterText, Color (0xff999999));
    scheme.setColor (CodeEditorScheme::ColorId::selection, Color (0xffbfdbfe));       // "selection"
    scheme.setColor (CodeEditorScheme::ColorId::searchHighlight, Color (0x66ffbc5d)); // "find_highlight" (orange)
    scheme.setColor (CodeEditorScheme::ColorId::breakpoint, Color (0xffaa3731));      // "red"
    scheme.setColor (CodeEditorScheme::ColorId::minimapBackground, Color (0xffe8e8e8));
    scheme.setColor (CodeEditorScheme::ColorId::minimapForeground, Color (0x66000000));
    scheme.setColor (CodeEditorScheme::ColorId::minimapViewport, Color (0x33007acc));

    scheme.setColor (SyntaxDefinition::TokenType::comment, Color (0xffaa3731));      // red
    scheme.setColor (SyntaxDefinition::TokenType::string, Color (0xff448c27));       // green
    scheme.setColor (SyntaxDefinition::TokenType::number, Color (0xff7a3e9d));       // magenta (constants)
    scheme.setColor (SyntaxDefinition::TokenType::keyword, Color (0xff000000));      // black + bold in the original
    scheme.setColor (SyntaxDefinition::TokenType::type, Color (0xff325cc0));         // blue (definitions)
    scheme.setColor (SyntaxDefinition::TokenType::operator_, Color (0xff777777));    // grey (punctuation)
    scheme.setColor (SyntaxDefinition::TokenType::preprocessor, Color (0xff000000)); // not defined; foreground
    scheme.setColor (SyntaxDefinition::TokenType::identifier, Color (0xff000000));
    scheme.setColor (SyntaxDefinition::TokenType::whitespace, Color (0xff000000));
    scheme.setColor (SyntaxDefinition::TokenType::other, Color (0xff000000));

    return scheme;
}

CodeEditorScheme makeOneDark()
{
    CodeEditorScheme scheme;
    scheme.setName ("One Dark");

    scheme.setColor (CodeEditorScheme::ColorId::background, Color (0xff282c34));
    scheme.setColor (CodeEditorScheme::ColorId::text, Color (0xffabb2bf));
    scheme.setColor (CodeEditorScheme::ColorId::caret, Color (0xff528bff));
    scheme.setColor (CodeEditorScheme::ColorId::currentLine, Color (0xff2c313a));
    scheme.setColor (CodeEditorScheme::ColorId::gutterBackground, Color (0xff21252b));
    scheme.setColor (CodeEditorScheme::ColorId::gutterText, Color (0xff636d83));
    scheme.setColor (CodeEditorScheme::ColorId::selection, Color (0xff3e4451));
    scheme.setColor (CodeEditorScheme::ColorId::searchHighlight, Color (0x556c7180));
    scheme.setColor (CodeEditorScheme::ColorId::breakpoint, Color (0xffe06c75));
    scheme.setColor (CodeEditorScheme::ColorId::minimapBackground, Color (0xff21252b));
    scheme.setColor (CodeEditorScheme::ColorId::minimapForeground, Color (0x66abb2bf));
    scheme.setColor (CodeEditorScheme::ColorId::minimapViewport, Color (0x33528bff));

    scheme.setColor (SyntaxDefinition::TokenType::comment, Color (0xff5c6370));
    scheme.setColor (SyntaxDefinition::TokenType::string, Color (0xff98c379));
    scheme.setColor (SyntaxDefinition::TokenType::number, Color (0xffd19a66));
    scheme.setColor (SyntaxDefinition::TokenType::keyword, Color (0xffc678dd));
    scheme.setColor (SyntaxDefinition::TokenType::type, Color (0xffe5c07b));
    scheme.setColor (SyntaxDefinition::TokenType::operator_, Color (0xff56b6c2));
    scheme.setColor (SyntaxDefinition::TokenType::preprocessor, Color (0xffc678dd));
    scheme.setColor (SyntaxDefinition::TokenType::identifier, Color (0xffabb2bf));
    scheme.setColor (SyntaxDefinition::TokenType::whitespace, Color (0xffabb2bf));
    scheme.setColor (SyntaxDefinition::TokenType::other, Color (0xffabb2bf));

    return scheme;
}

CodeEditorScheme makeSolarizedDark()
{
    CodeEditorScheme scheme;
    scheme.setName ("Solarized Dark");

    scheme.setColor (CodeEditorScheme::ColorId::background, Color (0xff002b36));
    scheme.setColor (CodeEditorScheme::ColorId::text, Color (0xff839496));
    scheme.setColor (CodeEditorScheme::ColorId::caret, Color (0xff839496));
    scheme.setColor (CodeEditorScheme::ColorId::currentLine, Color (0xff073642));
    scheme.setColor (CodeEditorScheme::ColorId::gutterBackground, Color (0xff073642));
    scheme.setColor (CodeEditorScheme::ColorId::gutterText, Color (0xff586e75));
    scheme.setColor (CodeEditorScheme::ColorId::selection, Color (0xff174956));
    scheme.setColor (CodeEditorScheme::ColorId::searchHighlight, Color (0x80586e75));
    scheme.setColor (CodeEditorScheme::ColorId::breakpoint, Color (0xffdc322f));
    scheme.setColor (CodeEditorScheme::ColorId::minimapBackground, Color (0xff073642));
    scheme.setColor (CodeEditorScheme::ColorId::minimapForeground, Color (0x66839496));
    scheme.setColor (CodeEditorScheme::ColorId::minimapViewport, Color (0x33268bd2));

    scheme.setColor (SyntaxDefinition::TokenType::comment, Color (0xff586e75));
    scheme.setColor (SyntaxDefinition::TokenType::string, Color (0xff2aa198));
    scheme.setColor (SyntaxDefinition::TokenType::number, Color (0xffd33682));
    scheme.setColor (SyntaxDefinition::TokenType::keyword, Color (0xff859900));
    scheme.setColor (SyntaxDefinition::TokenType::type, Color (0xffb58900));
    scheme.setColor (SyntaxDefinition::TokenType::operator_, Color (0xff839496));
    scheme.setColor (SyntaxDefinition::TokenType::preprocessor, Color (0xffcb4b16));
    scheme.setColor (SyntaxDefinition::TokenType::identifier, Color (0xff839496));
    scheme.setColor (SyntaxDefinition::TokenType::whitespace, Color (0xff839496));
    scheme.setColor (SyntaxDefinition::TokenType::other, Color (0xff839496));

    return scheme;
}

CodeEditorScheme makeSolarizedLight()
{
    CodeEditorScheme scheme;
    scheme.setName ("Solarized Light");

    scheme.setColor (CodeEditorScheme::ColorId::background, Color (0xfffdf6e3));
    scheme.setColor (CodeEditorScheme::ColorId::text, Color (0xff657b83));
    scheme.setColor (CodeEditorScheme::ColorId::caret, Color (0xff657b83));
    scheme.setColor (CodeEditorScheme::ColorId::currentLine, Color (0xffeee8d5));
    scheme.setColor (CodeEditorScheme::ColorId::gutterBackground, Color (0xffeee8d5));
    scheme.setColor (CodeEditorScheme::ColorId::gutterText, Color (0xff93a1a1));
    scheme.setColor (CodeEditorScheme::ColorId::selection, Color (0xffd5d8c4));
    scheme.setColor (CodeEditorScheme::ColorId::searchHighlight, Color (0x8093a1a1));
    scheme.setColor (CodeEditorScheme::ColorId::breakpoint, Color (0xffdc322f));
    scheme.setColor (CodeEditorScheme::ColorId::minimapBackground, Color (0xffeee8d5));
    scheme.setColor (CodeEditorScheme::ColorId::minimapForeground, Color (0x66657b83));
    scheme.setColor (CodeEditorScheme::ColorId::minimapViewport, Color (0x33268bd2));

    scheme.setColor (SyntaxDefinition::TokenType::comment, Color (0xff93a1a1));
    scheme.setColor (SyntaxDefinition::TokenType::string, Color (0xff2aa198));
    scheme.setColor (SyntaxDefinition::TokenType::number, Color (0xffd33682));
    scheme.setColor (SyntaxDefinition::TokenType::keyword, Color (0xff859900));
    scheme.setColor (SyntaxDefinition::TokenType::type, Color (0xffb58900));
    scheme.setColor (SyntaxDefinition::TokenType::operator_, Color (0xff657b83));
    scheme.setColor (SyntaxDefinition::TokenType::preprocessor, Color (0xffcb4b16));
    scheme.setColor (SyntaxDefinition::TokenType::identifier, Color (0xff657b83));
    scheme.setColor (SyntaxDefinition::TokenType::whitespace, Color (0xff657b83));
    scheme.setColor (SyntaxDefinition::TokenType::other, Color (0xff657b83));

    return scheme;
}

} // namespace

//==============================================================================

const CodeEditorScheme& CodeEditorScheme::getDefault()
{
    static const CodeEditorScheme defaultScheme;
    return defaultScheme;
}

const CodeEditorScheme& CodeEditorScheme::getBuiltIn (StringRef schemeName)
{
    static const CodeEditorScheme monokai = makeMonokai();
    static const CodeEditorScheme alabaster = makeAlabaster();
    static const CodeEditorScheme oneDark = makeOneDark();
    static const CodeEditorScheme solarizedDark = makeSolarizedDark();
    static const CodeEditorScheme solarizedLight = makeSolarizedLight();
    static const CodeEditorScheme inert;

    const String normalized = normalizeSchemeName (schemeName);

    if (normalized == "monokai")
        return monokai;

    if (normalized == "alabaster")
        return alabaster;

    if (normalized == "onedark")
        return oneDark;

    if (normalized == "solarizeddark")
        return solarizedDark;

    if (normalized == "solarizedlight")
        return solarizedLight;

    return inert;
}

const CodeEditorScheme* CodeEditorScheme::getBuiltInForName (StringRef schemeName)
{
    const String normalized = normalizeSchemeName (schemeName);
    if (normalized.isEmpty())
        return nullptr;

    for (const auto& candidate : getAvailableSchemeNames())
    {
        if (normalizeSchemeName (candidate) == normalized)
            return &getBuiltIn (candidate);
    }

    return nullptr;
}

std::vector<String> CodeEditorScheme::getAvailableSchemeNames()
{
    return { "Monokai", "Alabaster", "One Dark", "Solarized Dark", "Solarized Light" };
}

} // namespace yup
