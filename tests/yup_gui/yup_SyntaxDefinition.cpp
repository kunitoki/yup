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
// Built-in C++
// ==============================================================================

TEST (SyntaxDefinitionTests, BuiltInCppLoads)
{
    const auto& definition = SyntaxDefinition::getBuiltIn ("cpp");

    EXPECT_EQ (String ("C++"), definition.getName());
    EXPECT_TRUE (definition.isKeyword ("return"));
    EXPECT_TRUE (definition.isKeyword ("class"));
    EXPECT_FALSE (definition.isKeyword ("notAKeyword"));
    EXPECT_TRUE (definition.isType ("int"));
    EXPECT_TRUE (definition.isType ("float"));
    EXPECT_FALSE (definition.isType ("main"));

    EXPECT_EQ (String ("//"), definition.getLineCommentPrefix());
    ASSERT_TRUE (definition.getBlockComment().has_value());
    EXPECT_EQ (String ("/*"), definition.getBlockComment()->start);
    EXPECT_EQ (String ("*/"), definition.getBlockComment()->end);
    EXPECT_EQ (String ("#"), definition.getPreprocessorPrefix());
}

TEST (SyntaxDefinitionTests, BuiltInCppHasExtensionsAndOperators)
{
    const auto& definition = SyntaxDefinition::getBuiltIn ("cpp");

    const auto& extensions = definition.getFileExtensions();
    ASSERT_FALSE (extensions.empty());
    EXPECT_TRUE (std::find (extensions.begin(), extensions.end(), String ("cpp")) != extensions.end());

    EXPECT_TRUE (definition.isOperator ("->"));
    EXPECT_TRUE (definition.isOperator ("+"));
    EXPECT_TRUE (definition.isOperator ("("));
    EXPECT_FALSE (definition.isOperator ("word"));
}

// ==============================================================================
// Built-in GLSL
// ==============================================================================

TEST (SyntaxDefinitionTests, BuiltInGlslLoads)
{
    const auto& definition = SyntaxDefinition::getBuiltIn ("glsl");

    EXPECT_EQ (String ("GLSL"), definition.getName());
    EXPECT_TRUE (definition.isType ("vec3"));
    EXPECT_TRUE (definition.isType ("mat4"));
    EXPECT_TRUE (definition.isType ("sampler2D"));
    EXPECT_TRUE (definition.isKeyword ("uniform"));
    EXPECT_TRUE (definition.isKeyword ("discard"));
    EXPECT_EQ (String ("//"), definition.getLineCommentPrefix());
    EXPECT_EQ (String ("#"), definition.getPreprocessorPrefix());
}

// ==============================================================================
// Built-in Python
// ==============================================================================

TEST (SyntaxDefinitionTests, BuiltInPythonLoads)
{
    const auto& definition = SyntaxDefinition::getBuiltIn ("python");

    EXPECT_EQ (String ("Python"), definition.getName());
    EXPECT_TRUE (definition.isKeyword ("def"));
    EXPECT_TRUE (definition.isKeyword ("lambda"));
    EXPECT_TRUE (definition.isType ("int"));
    EXPECT_EQ (String ("#"), definition.getLineCommentPrefix());
    EXPECT_TRUE (definition.getPreprocessorPrefix().isEmpty());
    EXPECT_TRUE (definition.getBlockComment().has_value() == false);

    EXPECT_TRUE (definition.areStringsMultiLine());
    ASSERT_EQ (2u, definition.getMultiLineStringDelimiters().size());
    EXPECT_EQ (String ("\"\"\""), definition.getMultiLineStringDelimiters()[0]);
    EXPECT_EQ (String ("'''"), definition.getMultiLineStringDelimiters()[1]);
}

TEST (SyntaxDefinitionTests, BuiltInForExtension)
{
    EXPECT_EQ (String ("C++"), SyntaxDefinition::getBuiltInForExtension ("cpp")->getName());
    EXPECT_EQ (String ("C++"), SyntaxDefinition::getBuiltInForExtension ("h")->getName());
    EXPECT_EQ (String ("GLSL"), SyntaxDefinition::getBuiltInForExtension ("frag")->getName());
    EXPECT_EQ (String ("Python"), SyntaxDefinition::getBuiltInForExtension ("py")->getName());
    EXPECT_EQ (String ("XML"), SyntaxDefinition::getBuiltInForExtension ("xml")->getName());
    EXPECT_EQ (String ("XML"), SyntaxDefinition::getBuiltInForExtension ("svg")->getName());
    EXPECT_EQ (nullptr, SyntaxDefinition::getBuiltInForExtension ("xyz"));
}

// ==============================================================================
// Built-in XML
// ==============================================================================

TEST (SyntaxDefinitionTests, BuiltInXmlLoads)
{
    const auto& definition = SyntaxDefinition::getBuiltIn ("xml");

    EXPECT_EQ (String ("XML"), definition.getName());
    EXPECT_TRUE (definition.isKeyword ("DOCTYPE"));
    EXPECT_TRUE (definition.isKeyword ("CDATA"));
    EXPECT_TRUE (definition.isKeyword ("SYSTEM"));
    EXPECT_FALSE (definition.isKeyword ("return"));
    EXPECT_FALSE (definition.isType ("int"));

    EXPECT_TRUE (definition.getLineCommentPrefix().isEmpty());
    ASSERT_TRUE (definition.getBlockComment().has_value());
    EXPECT_EQ (String ("<!--"), definition.getBlockComment()->start);
    EXPECT_EQ (String ("-->"), definition.getBlockComment()->end);
    EXPECT_TRUE (definition.getPreprocessorPrefix().isEmpty());

    EXPECT_TRUE (definition.isOperator ("<"));
    EXPECT_TRUE (definition.isOperator (">"));
    EXPECT_TRUE (definition.isOperator ("</"));
    EXPECT_TRUE (definition.isOperator ("/>"));
    EXPECT_TRUE (definition.isOperator ("<?"));
    EXPECT_TRUE (definition.isOperator ("?>"));
    EXPECT_TRUE (definition.isOperator ("<!"));
    EXPECT_TRUE (definition.isOperator ("]]>"));
    EXPECT_TRUE (definition.isOperator ("="));
    EXPECT_FALSE (definition.isOperator ("word"));
}

TEST (SyntaxDefinitionTests, UnknownBuiltInIsInert)
{
    const auto& definition = SyntaxDefinition::getBuiltIn ("unknown");

    EXPECT_TRUE (definition.getName().isEmpty());
    EXPECT_FALSE (definition.isKeyword ("return"));
}

// ==============================================================================
// Loading from JSON
// ==============================================================================

TEST (SyntaxDefinitionTests, LoadFromDataParsesCustomDefinition)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData (R"({
        "name": "TestLang",
        "extensions": ["t"],
        "lineComment": ";",
        "keywords": ["foo", "bar"],
        "types": ["widget"]
    })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (String ("TestLang"), definition.getName());
    EXPECT_TRUE (definition.isKeyword ("foo"));
    EXPECT_TRUE (definition.isKeyword ("bar"));
    EXPECT_FALSE (definition.isKeyword ("baz"));
    EXPECT_TRUE (definition.isType ("widget"));
    EXPECT_EQ (String (";"), definition.getLineCommentPrefix());
}

TEST (SyntaxDefinitionTests, LoadFromDataRejectsInvalidJson)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData ("this is not json");

    EXPECT_TRUE (result.failed());
}

TEST (SyntaxDefinitionTests, LoadFromDataRequiresName)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData (R"({
        "lineComment": ";"
    })");

    EXPECT_TRUE (result.failed());
}

// ==============================================================================
// Token type names
// ==============================================================================

TEST (SyntaxDefinitionTests, TokenTypeToStringRoundTrips)
{
    EXPECT_EQ (String ("comment"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::comment));
    EXPECT_EQ (String ("string"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::string));
    EXPECT_EQ (String ("number"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::number));
    EXPECT_EQ (String ("keyword"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::keyword));
    EXPECT_EQ (String ("type"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::type));
    EXPECT_EQ (String ("operator"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::operator_));
    EXPECT_EQ (String ("preprocessor"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::preprocessor));
    EXPECT_EQ (String ("identifier"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::identifier));
    EXPECT_EQ (String ("whitespace"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::whitespace));
    EXPECT_EQ (String ("other"), SyntaxDefinition::tokenTypeToString (SyntaxDefinition::TokenType::other));
}

TEST (SyntaxDefinitionTests, RawStringsFlag)
{
    EXPECT_TRUE (SyntaxDefinition::getBuiltIn ("cpp").supportsRawStrings());
    EXPECT_FALSE (SyntaxDefinition::getBuiltIn ("python").supportsRawStrings());

    SyntaxDefinition custom;
    auto result = custom.loadFromData (R"({
        "name": "TestLang",
        "strings": { "delimiters": ["\""], "rawStrings": true }
    })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (custom.supportsRawStrings());
}

TEST (SyntaxDefinitionTests, RawStringPrefixes)
{
    const auto& cpp = SyntaxDefinition::getBuiltIn ("cpp");

    EXPECT_TRUE (cpp.supportsRawStrings());
    EXPECT_TRUE (cpp.isRawStringPrefix ("R"));
    EXPECT_TRUE (cpp.isRawStringPrefix ("u8R"));
    EXPECT_TRUE (cpp.isRawStringPrefix ("uR"));
    EXPECT_TRUE (cpp.isRawStringPrefix ("UR"));
    EXPECT_TRUE (cpp.isRawStringPrefix ("LR"));
    EXPECT_FALSE (cpp.isRawStringPrefix ("my"));
    EXPECT_FALSE (cpp.isRawStringPrefix ("return"));

    // Custom prefixes override the C++-style defaults.
    SyntaxDefinition custom;
    auto result = custom.loadFromData (R"({
        "name": "TestLang",
        "strings": { "delimiters": ["\""], "rawStrings": true, "rawStringPrefixes": ["my", "foo"] }
    })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (custom.isRawStringPrefix ("my"));
    EXPECT_TRUE (custom.isRawStringPrefix ("foo"));
    EXPECT_FALSE (custom.isRawStringPrefix ("R"));
}

TEST (SyntaxDefinitionTests, RawStringsFlagDefaultsPrefixesToNothing)
{
    SyntaxDefinition custom;
    auto result = custom.loadFromData (R"({
        "name": "TestLang",
        "strings": { "delimiters": ["\""], "rawStrings": true }
    })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_FALSE (custom.isRawStringPrefix ("R"));
    EXPECT_FALSE (custom.isRawStringPrefix ("LR"));
}

TEST (SyntaxDefinitionTests, StringPrefixes)
{
    const auto& cpp = SyntaxDefinition::getBuiltIn ("cpp");
    const auto& python = SyntaxDefinition::getBuiltIn ("python");

    EXPECT_TRUE (cpp.isStringPrefix ("u8"));
    EXPECT_TRUE (cpp.isStringPrefix ("L"));
    EXPECT_TRUE (cpp.isStringPrefix ("u"));
    EXPECT_TRUE (cpp.isStringPrefix ("U"));
    EXPECT_FALSE (cpp.isStringPrefix ("f"));
    EXPECT_FALSE (cpp.isStringPrefix ("my"));

    EXPECT_TRUE (python.isStringPrefix ("f"));
    EXPECT_TRUE (python.isStringPrefix ("r"));
    EXPECT_TRUE (python.isStringPrefix ("b"));
    EXPECT_TRUE (python.isStringPrefix ("fr"));
    EXPECT_TRUE (python.isStringPrefix ("rf"));
    EXPECT_TRUE (python.isStringPrefix ("br"));
    EXPECT_FALSE (python.isStringPrefix ("L"));

    // Custom prefixes override the defaults.
    SyntaxDefinition custom;
    auto result = custom.loadFromData (R"({
        "name": "TestLang",
        "strings": { "delimiters": ["\""], "stringPrefixes": ["zz"] }
    })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (custom.isStringPrefix ("zz"));
    EXPECT_FALSE (custom.isStringPrefix ("u8"));
}

// ==============================================================================
// Degenerate JSON inputs
// ==============================================================================

TEST (SyntaxDefinitionTests, LoadFromDataRejectsNonObjectJson)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData ("[1, 2, 3]");

    EXPECT_TRUE (result.failed());
}

TEST (SyntaxDefinitionTests, LoadFromDataRejectsEmptyObject)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData ("{}");

    EXPECT_TRUE (result.failed());
}

TEST (SyntaxDefinitionTests, LoadFromDataRejectsNonStringName)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData (R"({ "name": 42 })");

    EXPECT_TRUE (result.failed());
}

TEST (SyntaxDefinitionTests, LoadFromDataWithNameOnlyAppliesDefaults)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData (R"({ "name": "Tiny" })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (String ("Tiny"), definition.getName());
    EXPECT_FALSE (definition.isKeyword ("anything"));
    EXPECT_TRUE (definition.getLineCommentPrefix().isEmpty());
    EXPECT_FALSE (definition.getBlockComment().has_value());
    EXPECT_FALSE (definition.areStringsMultiLine());
    EXPECT_TRUE (definition.getPreprocessorPrefix().isEmpty());
}

// ==============================================================================
// Numbers and escape configuration
// ==============================================================================

TEST (SyntaxDefinitionTests, LoadFromDataParsesNumbersSection)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData (R"({
        "name": "NumLang",
        "numbers": { "hex": false, "binary": false, "float": false, "exponent": false, "suffix": false }
    })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_FALSE (definition.numbersAllowHex());
    EXPECT_FALSE (definition.numbersAllowBinary());
    EXPECT_FALSE (definition.numbersAllowFloat());
    EXPECT_FALSE (definition.numbersAllowExponent());
    EXPECT_FALSE (definition.numbersAllowSuffix());
}

TEST (SyntaxDefinitionTests, LoadFromDataDefaultsNumbersToTrue)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData (R"({ "name": "NumLang" })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (definition.numbersAllowHex());
    EXPECT_TRUE (definition.numbersAllowBinary());
    EXPECT_TRUE (definition.numbersAllowFloat());
    EXPECT_TRUE (definition.numbersAllowExponent());
    EXPECT_TRUE (definition.numbersAllowSuffix());
}

TEST (SyntaxDefinitionTests, LoadFromDataAcceptsScalarKeywords)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData (R"({ "name": "ScalarLang", "keywords": "solo" })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_TRUE (definition.isKeyword ("solo"));
    EXPECT_FALSE (definition.isKeyword ("other"));
}

TEST (SyntaxDefinitionTests, LoadFromDataUsesFirstEscapeCharacter)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromData (R"({ "name": "EscLang", "strings": { "delimiters": ["\""], "escape": "\"" } })");

    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (static_cast<yup_wchar> ('"'), definition.getEscapeCharacter());
}

TEST (SyntaxDefinitionTests, LoadFromFileFailsForMissingFile)
{
    SyntaxDefinition definition;
    auto result = definition.loadFromFile (File::getCurrentWorkingDirectory().getChildFile ("yup_definitely_missing_syntax_file.json"));

    EXPECT_TRUE (result.failed());
}

// ==============================================================================
// Built-in lookup edges
// ==============================================================================

TEST (SyntaxDefinitionTests, BuiltInCppCaseVariants)
{
    EXPECT_EQ (String ("C++"), SyntaxDefinition::getBuiltIn ("cpp").getName());
    EXPECT_EQ (String ("C++"), SyntaxDefinition::getBuiltIn ("c++").getName());
    EXPECT_EQ (String ("C++"), SyntaxDefinition::getBuiltIn ("c").getName());
    EXPECT_TRUE (SyntaxDefinition::getBuiltIn ("CPP").getName().isEmpty());
}

TEST (SyntaxDefinitionTests, BuiltInForExtensionIsCaseSensitiveAndDotSensitive)
{
    EXPECT_EQ (nullptr, SyntaxDefinition::getBuiltInForExtension ("CPP"));
    EXPECT_EQ (nullptr, SyntaxDefinition::getBuiltInForExtension (".cpp"));
    EXPECT_EQ (nullptr, SyntaxDefinition::getBuiltInForExtension (""));
}

TEST (SyntaxDefinitionTests, IdentifierStartAndPartRules)
{
    const auto& definition = SyntaxDefinition::getBuiltIn ("cpp");

    EXPECT_TRUE (definition.isIdentifierStart ('a'));
    EXPECT_TRUE (definition.isIdentifierStart ('Z'));
    EXPECT_TRUE (definition.isIdentifierStart ('_'));
    EXPECT_FALSE (definition.isIdentifierStart ('0'));
    EXPECT_FALSE (definition.isIdentifierStart (' '));

    EXPECT_TRUE (definition.isIdentifierPart ('a'));
    EXPECT_TRUE (definition.isIdentifierPart ('_'));
    EXPECT_TRUE (definition.isIdentifierPart ('9'));
    EXPECT_FALSE (definition.isIdentifierPart ('-'));
}

TEST (SyntaxDefinitionTests, GetBuiltInGlslNumbersDifferFromCpp)
{
    const auto& glsl = SyntaxDefinition::getBuiltIn ("glsl");
    EXPECT_FALSE (glsl.numbersAllowBinary());
    EXPECT_FALSE (glsl.numbersAllowSuffix());

    const auto& cpp = SyntaxDefinition::getBuiltIn ("cpp");
    EXPECT_TRUE (cpp.numbersAllowBinary());
    EXPECT_TRUE (cpp.numbersAllowSuffix());
}
