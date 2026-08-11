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

namespace
{

const CodeTokeniser::Token* findTokenOfType (Span<const CodeTokeniser::Token> tokens, SyntaxDefinition::TokenType type)
{
    for (const auto& token : tokens)
        if (token.type == type)
            return &token;

    return nullptr;
}

String tokenText (const CodeDocument& document, int line, const CodeTokeniser::Token& token)
{
    return document.getLine (line).substring (token.start, token.end);
}

} // namespace

// ==============================================================================
// C++ tokenization
// ==============================================================================

TEST (CodeTokeniserTests, TokenizesBasicCppLine)
{
    CodeDocument document;
    document.setText ("int main() { return 0; }");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    const auto* typeToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::type);
    ASSERT_NE (nullptr, typeToken);
    EXPECT_EQ (String ("int"), tokenText (document, 0, *typeToken));

    const auto* keywordToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::keyword);
    ASSERT_NE (nullptr, keywordToken);
    EXPECT_EQ (String ("return"), tokenText (document, 0, *keywordToken));

    const auto* numberToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::number);
    ASSERT_NE (nullptr, numberToken);
    EXPECT_EQ (String ("0"), tokenText (document, 0, *numberToken));

    EXPECT_NE (nullptr, findTokenOfType (tokens, SyntaxDefinition::TokenType::identifier));
    EXPECT_NE (nullptr, findTokenOfType (tokens, SyntaxDefinition::TokenType::operator_));
}

TEST (CodeTokeniserTests, LineCommentConsumesRestOfLine)
{
    CodeDocument document;
    document.setText ("int x; // comment here");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    const auto* commentToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::comment);
    ASSERT_NE (nullptr, commentToken);
    EXPECT_EQ (String ("// comment here"), tokenText (document, 0, *commentToken));
}

TEST (CodeTokeniserTests, PreprocessorDirectiveConsumesLine)
{
    CodeDocument document;
    document.setText ("#include <vector>");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    ASSERT_EQ (1u, tokens.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::preprocessor, tokens[0].type);
    EXPECT_EQ (String ("#include <vector>"), tokenText (document, 0, tokens[0]));
}

TEST (CodeTokeniserTests, StringTokensHandleEscapes)
{
    CodeDocument document;
    document.setText ("auto s = \"say \\\"hi\\\"\";");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("\"say \\\"hi\\\"\""), tokenText (document, 0, *stringToken));
}

TEST (CodeTokeniserTests, NumberVariants)
{
    CodeDocument document;
    document.setText ("0x1F 1.5 1e5 100u 42");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    String numberTexts;
    for (const auto& token : tokens)
        if (token.type == SyntaxDefinition::TokenType::number)
            numberTexts << tokenText (document, 0, token) << " ";

    EXPECT_EQ (String ("0x1F 1.5 1e5 100u 42 "), numberTexts);
}

TEST (CodeTokeniserTests, LongestOperatorMatch)
{
    CodeDocument document;
    document.setText ("a->b::c");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    EXPECT_EQ (5u, tokens.size());
    EXPECT_EQ (String ("->"), tokenText (document, 0, tokens[1]));
    EXPECT_EQ (String ("::"), tokenText (document, 0, tokens[3]));
}

// ==============================================================================
// Multi-line constructs
// ==============================================================================

TEST (CodeTokeniserTests, BlockCommentSpansLines)
{
    CodeDocument document;
    document.setText ("/* start\nmiddle */ int x;\nend");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto line0 = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, line0.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::comment, line0[0].type);
    EXPECT_EQ (String ("/* start"), tokenText (document, 0, line0[0]));

    const auto line1 = tokeniser.getTokens (document, 1);
    const auto* commentToken = findTokenOfType (line1, SyntaxDefinition::TokenType::comment);
    ASSERT_NE (nullptr, commentToken);
    EXPECT_EQ (String ("middle */"), tokenText (document, 1, *commentToken));
    EXPECT_NE (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::type));

    const auto line2 = tokeniser.getTokens (document, 2);
    EXPECT_NE (nullptr, findTokenOfType (line2, SyntaxDefinition::TokenType::identifier));
    EXPECT_EQ (nullptr, findTokenOfType (line2, SyntaxDefinition::TokenType::comment));
}

TEST (CodeTokeniserTests, EditPropagatesStateChangeToFollowingLines)
{
    CodeDocument document;
    document.setText ("/*\nhello");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // Line 1 is a comment because line 0 opened a block comment.
    EXPECT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 1), SyntaxDefinition::TokenType::comment));

    // Turning line 0 into a line comment closes the block comment, so line 1
    // must be re-tokenized as plain code (state ripple propagation).
    document.replaceRange (CodeDocument::Position (document, 0, 0),
                           CodeDocument::Position (document, 0, 2),
                           "//");

    const auto line1 = tokeniser.getTokens (document, 1);
    EXPECT_EQ (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::comment));
    EXPECT_NE (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::identifier));
}

TEST (CodeTokeniserTests, EditReflectsInChangedLine)
{
    CodeDocument document;
    document.setText ("int x;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    EXPECT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::type));

    document.replaceRange (CodeDocument::Position (document, 0, 0),
                           CodeDocument::Position (document, 0, 3),
                           "float");

    const auto tokens = tokeniser.getTokens (document, 0);
    const auto* typeToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::type);
    ASSERT_NE (nullptr, typeToken);
    EXPECT_EQ (String ("float"), tokenText (document, 0, *typeToken));
}

// ==============================================================================
// Python tokenization
// ==============================================================================

TEST (CodeTokeniserTests, TokenizesPython)
{
    CodeDocument document;
    document.setText ("def foo(a, b):  # adds two numbers\n    return a + b");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto line0 = tokeniser.getTokens (document, 0);
    const auto* defToken = findTokenOfType (line0, SyntaxDefinition::TokenType::keyword);
    ASSERT_NE (nullptr, defToken);
    EXPECT_EQ (String ("def"), tokenText (document, 0, *defToken));
    EXPECT_NE (nullptr, findTokenOfType (line0, SyntaxDefinition::TokenType::comment));

    const auto line1 = tokeniser.getTokens (document, 1);
    EXPECT_NE (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::keyword));
}

TEST (CodeTokeniserTests, PythonTripleQuotedStringSpansLines)
{
    CodeDocument document;
    document.setText ("\"\"\"hello\nworld\"\"\"\nvalue = 3");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto line0 = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, line0.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line0[0].type);

    const auto line1 = tokeniser.getTokens (document, 1);
    ASSERT_EQ (1u, line1.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line1[0].type);
    EXPECT_EQ (String ("world\"\"\""), tokenText (document, 1, line1[0]));

    // The string closed on line 1, so line 2 is normal code again.
    const auto line2 = tokeniser.getTokens (document, 2);
    EXPECT_EQ (nullptr, findTokenOfType (line2, SyntaxDefinition::TokenType::string));
    EXPECT_NE (nullptr, findTokenOfType (line2, SyntaxDefinition::TokenType::number));
}

// ==============================================================================
// Tokenizer state
// ==============================================================================

TEST (CodeTokeniserTests, NoTokensWithoutDefinition)
{
    CodeDocument document;
    document.setText ("anything");

    CodeTokeniser tokeniser;

    EXPECT_FALSE (tokeniser.hasSyntaxDefinition());
    EXPECT_TRUE (tokeniser.getTokens (document, 0).empty());
}

TEST (CodeTokeniserTests, ClearDiscardsCache)
{
    CodeDocument document;
    document.setText ("int x;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    EXPECT_FALSE (tokeniser.getTokens (document, 0).empty());

    tokeniser.clear();
    EXPECT_FALSE (tokeniser.getTokens (document, 0).empty());
}

// ==============================================================================
// Cache coherency after line deletions
// ==============================================================================

TEST (CodeTokeniserTests, DeleteLinesDoesNotReturnStaleTokensForShiftedLines)
{
    // Reproduce: cut removes lines 1..3 from a 5-line document.  Without the fix,
    // the token cache at index 2 keeps "ghi"'s token list for the "mno" line.
    CodeDocument document;
    document.setText ("int abc;\nint def;\nint ghi;\nint jkl;\nint mno;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // Prime the cache for all five lines.
    for (int i = 0; i < 5; ++i)
        ASSERT_FALSE (tokeniser.getTokens (document, i).empty());

    // Remove lines 1-3 (def, ghi, jkl); document becomes [abc, mno] — 2 lines.
    document.removeRange (CodeDocument::Position (document, 1, 0),
                          CodeDocument::Position (document, 4, 0));

    ASSERT_EQ (2, document.getNumLines());

    // Line 1 in the updated document is "int mno;" — it must tokenize as "mno",
    // not as the old line that occupied cache slot 1 before the cut.
    const auto tokens = tokeniser.getTokens (document, 1);
    ASSERT_FALSE (tokens.empty());

    String allText;
    for (const auto& token : tokens)
        allText << document.getLine (1).substring (token.start, token.end);

    EXPECT_EQ (String ("int mno;"), allText);
    EXPECT_EQ (String ("mno"), tokenText (document, 1, *findTokenOfType (tokens, SyntaxDefinition::TokenType::identifier)));
}

TEST (CodeTokeniserTests, InsertLineDoesNotReturnStaleTokensForShiftedLines)
{
    // Pressing Enter in the middle of line 1 splits it in two, shifting every line
    // below it down by one. Without a growth branch in codeDocumentChanged, cache
    // slot 3 keeps the tokens computed for the (longer) line that used to sit
    // there, and the forward-propagation stability check can decide the
    // shifted-in line's state is "unchanged" and never mark it dirty, so it keeps
    // returning tokens sized for the wrong (longer) line — caught here by checking
    // that the tokens actually tile the new, shorter line's length, exactly what
    // CodeEditor::rebuildStyledText relies on to decide whether to trust them.
    CodeDocument document;
    document.setText ("int abc;\nint de;\nint ghi;\nint jklmnop;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    for (int i = 0; i < 4; ++i)
        ASSERT_FALSE (tokeniser.getTokens (document, i).empty());

    // Split line 1 ("int de;") after "int " into two lines.
    document.insertText (CodeDocument::Position (document, 1, 4), "\n");

    ASSERT_EQ (5, document.getNumLines());
    ASSERT_EQ (String ("int "), document.getLine (1));
    ASSERT_EQ (String ("de;"), document.getLine (2));
    ASSERT_EQ (String ("int ghi;"), document.getLine (3));
    ASSERT_EQ (String ("int jklmnop;"), document.getLine (4));

    // Line 3 is now "int ghi;" (shifted down from index 2), not the longer
    // "int jklmnop;" that used to occupy cache slot 3 before the insert.
    const auto tokens = tokeniser.getTokens (document, 3);
    ASSERT_FALSE (tokens.empty());
    EXPECT_EQ (document.getLine (3).length(), tokens.back().end);

    ASSERT_NE (nullptr, findTokenOfType (tokens, SyntaxDefinition::TokenType::identifier));
    EXPECT_EQ (String ("ghi"), tokenText (document, 3, *findTokenOfType (tokens, SyntaxDefinition::TokenType::identifier)));
}

TEST (CodeTokeniserTests, BackspaceJoiningLinesDoesNotReturnStaleTokensForShiftedLines)
{
    // Backspace at the start of line 1 joins lines 0 and 1, shifting line 2 down.
    // Without the fix, line 1 (previously line 2) returns stale tokens.
    CodeDocument document;
    document.setText ("int a;\nint b;\nint c;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    for (int i = 0; i < 3; ++i)
        ASSERT_FALSE (tokeniser.getTokens (document, i).empty());

    // Delete the newline at the end of line 0, joining it with line 1.
    document.removeRange (CodeDocument::Position (document, 0, 6),
                          CodeDocument::Position (document, 1, 0));

    ASSERT_EQ (2, document.getNumLines());

    // Line 1 is now "int c;" — must not return tokens for "int b;".
    const auto tokens = tokeniser.getTokens (document, 1);
    ASSERT_NE (nullptr, findTokenOfType (tokens, SyntaxDefinition::TokenType::identifier));
    EXPECT_EQ (String ("c"), tokenText (document, 1, *findTokenOfType (tokens, SyntaxDefinition::TokenType::identifier)));
}

// ==============================================================================
// C++ raw string literals
// ==============================================================================

TEST (CodeTokeniserTests, CppRawStringTokenizesSingleLine)
{
    CodeDocument document;
    document.setText ("auto s = R\"(hello world)\";");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("R\"(hello world)\""), tokenText (document, 0, *stringToken));
}

TEST (CodeTokeniserTests, CppRawStringWithDelimiterAndInnerQuotes)
{
    CodeDocument document;
    document.setText ("auto s = R\"foo(a \"quoted\" (b))foo\";");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    // The whole raw literal, including quotes and parens inside, is one string token.
    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("R\"foo(a \"quoted\" (b))foo\""), tokenText (document, 0, *stringToken));
}

TEST (CodeTokeniserTests, CppRawStringSpansLines)
{
    CodeDocument document;
    document.setText ("R\"(line one\nline two)\"\nafter = true;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // Line 0 opens the raw string and does not close it.
    const auto line0 = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, line0.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line0[0].type);
    EXPECT_EQ (String ("R\"(line one"), tokenText (document, 0, line0[0]));

    // Line 1 is a continuation; the closing )" ends the string token.
    const auto line1 = tokeniser.getTokens (document, 1);
    ASSERT_EQ (1u, line1.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line1[0].type);
    EXPECT_EQ (String ("line two)\""), tokenText (document, 1, line1[0]));

    // Line 2 is normal code again (the string closed on line 1).
    const auto line2 = tokeniser.getTokens (document, 2);
    EXPECT_NE (nullptr, findTokenOfType (line2, SyntaxDefinition::TokenType::identifier));
    EXPECT_EQ (nullptr, findTokenOfType (line2, SyntaxDefinition::TokenType::string));
}

TEST (CodeTokeniserTests, CppRawStringWithEmptyDelimiter)
{
    CodeDocument document;
    document.setText ("auto s = R\"()\";");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("R\"()\""), tokenText (document, 0, *stringToken));
}

TEST (CodeTokeniserTests, CppPrefixedStringLiterals)
{
    CodeDocument document;
    document.setText ("auto s1 = L\"wide\"; auto s2 = u8\"utf8\"; auto s3 = u\"u16\"; auto s4 = U\"u32\";");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    std::vector<String> stringTexts;
    for (const auto& token : tokens)
        if (token.type == SyntaxDefinition::TokenType::string)
            stringTexts.push_back (tokenText (document, 0, token));

    ASSERT_EQ (4u, stringTexts.size());
    EXPECT_EQ (String ("L\"wide\""), stringTexts[0]);
    EXPECT_EQ (String ("u8\"utf8\""), stringTexts[1]);
    EXPECT_EQ (String ("u\"u16\""), stringTexts[2]);
    EXPECT_EQ (String ("U\"u32\""), stringTexts[3]);
}

TEST (CodeTokeniserTests, CppPrefixedCharacterLiterals)
{
    CodeDocument document;
    document.setText ("auto c1 = L'a'; auto c2 = u8'b'; auto c3 = U'c';");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    std::vector<String> stringTexts;
    for (const auto& token : tokens)
        if (token.type == SyntaxDefinition::TokenType::string)
            stringTexts.push_back (tokenText (document, 0, token));

    ASSERT_EQ (3u, stringTexts.size());
    EXPECT_EQ (String ("L'a'"), stringTexts[0]);
    EXPECT_EQ (String ("u8'b'"), stringTexts[1]);
    EXPECT_EQ (String ("U'c'"), stringTexts[2]);
}

TEST (CodeTokeniserTests, CppRawStringWithContentContainingCloseSequence)
{
    // Content containing )" requires a non-empty delimiter: R"xyz()")xyz"
    CodeDocument document;
    document.setText ("auto s = R\"xyz()\")xyz\";");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("R\"xyz()\")xyz\""), tokenText (document, 0, *stringToken));
}

TEST (CodeTokeniserTests, CustomRawStringPrefix)
{
    SyntaxDefinition definition;
    definition.loadFromData (R"({
        "name": "TestLang",
        "strings": { "delimiters": ["\""], "rawStrings": true, "rawStringPrefixes": ["my"] }
    })");

    CodeDocument document;
    document.setText ("auto s = my\"(hello)\";");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (definition);

    const auto tokens = tokeniser.getTokens (document, 0);

    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("my\"(hello)\""), tokenText (document, 0, *stringToken));
}

// ==============================================================================
// Prefixed string literals (definition-driven)
// ==============================================================================

TEST (CodeTokeniserTests, PythonFStringTokenizes)
{
    CodeDocument document;
    document.setText ("name = f\"hello {value}\"; other = f'world';");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto tokens = tokeniser.getTokens (document, 0);

    std::vector<String> stringTexts;
    for (const auto& token : tokens)
        if (token.type == SyntaxDefinition::TokenType::string)
            stringTexts.push_back (tokenText (document, 0, token));

    ASSERT_EQ (2u, stringTexts.size());
    EXPECT_EQ (String ("f\"hello {value}\""), stringTexts[0]);
    EXPECT_EQ (String ("f'world'"), stringTexts[1]);
}

TEST (CodeTokeniserTests, PythonPrefixedMultiLineStringSpansLines)
{
    CodeDocument document;
    document.setText ("doc = f\"\"\"first line\nsecond line\"\"\"\nvalue = 3");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto line0 = tokeniser.getTokens (document, 0);
    const auto* line0String = findTokenOfType (line0, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, line0String);
    EXPECT_EQ (String ("f\"\"\"first line"), tokenText (document, 0, *line0String));

    const auto line1 = tokeniser.getTokens (document, 1);
    const auto* line1String = findTokenOfType (line1, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, line1String);
    EXPECT_EQ (String ("second line\"\"\""), tokenText (document, 1, *line1String));

    // The string closed on line 1, so line 2 is normal code.
    const auto line2 = tokeniser.getTokens (document, 2);
    EXPECT_EQ (nullptr, findTokenOfType (line2, SyntaxDefinition::TokenType::string));
    EXPECT_NE (nullptr, findTokenOfType (line2, SyntaxDefinition::TokenType::number));
}

// ==============================================================================
// Degenerate line indexes and content
// ==============================================================================

TEST (CodeTokeniserTests, GetTokensClampsOutOfRangeLineIndex)
{
    CodeDocument document;
    document.setText ("int a;\nint b;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto negative = tokeniser.getTokens (document, -7);
    ASSERT_NE (nullptr, findTokenOfType (negative, SyntaxDefinition::TokenType::identifier));
    EXPECT_EQ (String ("a"), tokenText (document, 0, *findTokenOfType (negative, SyntaxDefinition::TokenType::identifier)));

    const auto oversized = tokeniser.getTokens (document, 99);
    ASSERT_NE (nullptr, findTokenOfType (oversized, SyntaxDefinition::TokenType::identifier));
    EXPECT_EQ (String ("b"), tokenText (document, 1, *findTokenOfType (oversized, SyntaxDefinition::TokenType::identifier)));
}

TEST (CodeTokeniserTests, EmptyLinesProduceNoTokens)
{
    CodeDocument document;
    document.setText ("\n\n");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    EXPECT_TRUE (tokeniser.getTokens (document, 0).empty());
    EXPECT_TRUE (tokeniser.getTokens (document, 1).empty());
    EXPECT_TRUE (tokeniser.getTokens (document, 2).empty());
}

TEST (CodeTokeniserTests, WhitespaceOnlyLineProducesWhitespaceToken)
{
    CodeDocument document;
    document.setText ("   \nint a;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, tokens.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::whitespace, tokens[0].type);
    EXPECT_EQ (0, tokens[0].start);
    EXPECT_EQ (3, tokens[0].end);
}

TEST (CodeTokeniserTests, TabCharactersAreWhitespace)
{
    CodeDocument document;
    document.setText ("\tint\ta;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);
    EXPECT_NE (nullptr, findTokenOfType (tokens, SyntaxDefinition::TokenType::whitespace));
    EXPECT_NE (nullptr, findTokenOfType (tokens, SyntaxDefinition::TokenType::type));
}

// ==============================================================================
// Unterminated constructs
// ==============================================================================

TEST (CodeTokeniserTests, UnterminatedStringConsumesRestOfLine)
{
    CodeDocument document;
    document.setText ("auto s = \"abc");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);
    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("\"abc"), tokenText (document, 0, *stringToken));
    EXPECT_EQ (13, stringToken->end);
}

TEST (CodeTokeniserTests, UnterminatedStringDoesNotLeakToNextLine)
{
    CodeDocument document;
    document.setText ("auto s = \"abc\ndef = 2");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto line0 = tokeniser.getTokens (document, 0);
    EXPECT_NE (nullptr, findTokenOfType (line0, SyntaxDefinition::TokenType::string));

    const auto line1 = tokeniser.getTokens (document, 1);
    EXPECT_EQ (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::string));
    EXPECT_NE (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::identifier));
}

TEST (CodeTokeniserTests, CharLiteralWithEscapedQuote)
{
    CodeDocument document;
    document.setText ("auto c = '\\'';");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);
    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("'\\''"), tokenText (document, 0, *stringToken));
}

TEST (CodeTokeniserTests, BlockCommentDoesNotNest)
{
    CodeDocument document;
    document.setText ("/* a /* b */");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // The first "*/" closes the comment; the inner "/*" does not nest.
    const auto tokens = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, tokens.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::comment, tokens[0].type);
    EXPECT_EQ (String ("/* a /* b */"), tokenText (document, 0, tokens[0]));
}

TEST (CodeTokeniserTests, UnclosedBlockCommentConsumesFollowingLineComment)
{
    CodeDocument document;
    document.setText ("/* open\n// not a line comment");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto line0 = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, line0.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::comment, line0[0].type);

    const auto line1 = tokeniser.getTokens (document, 1);
    ASSERT_EQ (1u, line1.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::comment, line1[0].type);
    EXPECT_EQ (String ("// not a line comment"), tokenText (document, 1, line1[0]));
}

// ==============================================================================
// Preprocessor edges
// ==============================================================================

TEST (CodeTokeniserTests, PreprocessorOnlyAtLineStart)
{
    CodeDocument document;
    document.setText ("  #include <x>\n#include <y>");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // Leading whitespace disables the whole-line preprocessor directive.
    const auto line0 = tokeniser.getTokens (document, 0);
    EXPECT_EQ (nullptr, findTokenOfType (line0, SyntaxDefinition::TokenType::preprocessor));
    EXPECT_NE (nullptr, findTokenOfType (line0, SyntaxDefinition::TokenType::operator_));

    const auto line1 = tokeniser.getTokens (document, 1);
    ASSERT_EQ (1u, line1.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::preprocessor, line1[0].type);
}

TEST (CodeTokeniserTests, HashInsideStringIsNotPreprocessor)
{
    CodeDocument document;
    document.setText ("s = \"#include\"");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);
    EXPECT_EQ (nullptr, findTokenOfType (tokens, SyntaxDefinition::TokenType::preprocessor));
    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("\"#include\""), tokenText (document, 0, *stringToken));
}

TEST (CodeTokeniserTests, HashMidLineIsOperator)
{
    CodeDocument document;
    document.setText ("a # b");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);
    const auto hashToken = std::find_if (tokens.begin(), tokens.end(), [] (const CodeTokeniser::Token& token)
    {
        return token.type == SyntaxDefinition::TokenType::operator_;
    });

    ASSERT_NE (tokens.end(), hashToken);
    EXPECT_EQ (String ("#"), tokenText (document, 0, *hashToken));
}

// ==============================================================================
// Degenerate number forms
// ==============================================================================

TEST (CodeTokeniserTests, DegenerateNumberFormsDoNotCrash)
{
    CodeDocument document;
    document.setText ("0x 0b 1. .5 1e 1e+ 0xG 0b102");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    String numberTexts;
    for (const auto& token : tokeniser.getTokens (document, 0))
        if (token.type == SyntaxDefinition::TokenType::number)
            numberTexts << tokenText (document, 0, token) << " ";

    // Note: "1." yields number "1" + operator ".", ".5" yields operator "." + number "5",
    // and "0x" / "0b" with no digits are still whole number tokens.
    EXPECT_EQ (String ("0x 0b 1 5 1 1 0x 0b10 2 "), numberTexts);
}

TEST (CodeTokeniserTests, ExponentVariants)
{
    CodeDocument document;
    document.setText ("1e5 1E+5 1e-3 1e5f");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    String numberTexts;
    for (const auto& token : tokeniser.getTokens (document, 0))
        if (token.type == SyntaxDefinition::TokenType::number)
            numberTexts << tokenText (document, 0, token) << " ";

    EXPECT_EQ (String ("1e5 1E+5 1e-3 1e5f "), numberTexts);
}

TEST (CodeTokeniserTests, HexAndBinaryWithSuffix)
{
    CodeDocument document;
    document.setText ("0x1FUL 0b1010u");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    String numberTexts;
    for (const auto& token : tokeniser.getTokens (document, 0))
        if (token.type == SyntaxDefinition::TokenType::number)
            numberTexts << tokenText (document, 0, token) << " ";

    EXPECT_EQ (String ("0x1FUL 0b1010u "), numberTexts);
}

TEST (CodeTokeniserTests, KeywordBoundariesAreExact)
{
    CodeDocument document;
    document.setText ("return returnx if( x1 1x");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    String keywordTexts;
    String identifierTexts;
    for (const auto& token : tokeniser.getTokens (document, 0))
    {
        if (token.type == SyntaxDefinition::TokenType::keyword)
            keywordTexts << tokenText (document, 0, token) << " ";
        else if (token.type == SyntaxDefinition::TokenType::identifier)
            identifierTexts << tokenText (document, 0, token) << " ";
    }

    // Substring matches ("returnx") are not keywords; "1x" splits into number + identifier.
    EXPECT_EQ (String ("return if "), keywordTexts);
    EXPECT_EQ (String ("returnx x1 x "), identifierTexts);
}

TEST (CodeTokeniserTests, NonAsciiCharactersAreOtherTokens)
{
    CodeDocument document;
    document.setText (String::fromUTF8 ("caf\xc3\xa9"));

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // The identifier rules only cover ASCII letters, digits and '_'.
    const auto tokens = tokeniser.getTokens (document, 0);
    ASSERT_EQ (2u, tokens.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::identifier, tokens[0].type);
    EXPECT_EQ (SyntaxDefinition::TokenType::other, tokens[1].type);
}

// ==============================================================================
// Cache invalidation edge cases
// ==============================================================================

TEST (CodeTokeniserTests, InvalidateLinesForcesRetokenization)
{
    CodeDocument document;
    document.setText ("int a;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    ASSERT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::type));

    // setText with dontSendNotification bypasses the listener, leaving the cache stale.
    document.setText ("foo bar", dontSendNotification);
    EXPECT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::type));

    tokeniser.invalidateLines (0, 0);
    EXPECT_EQ (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::type));
    EXPECT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::identifier));
}

TEST (CodeTokeniserTests, InvalidateLinesWithInvertedRangeIsHarmless)
{
    CodeDocument document;
    document.setText ("int a;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    ASSERT_FALSE (tokeniser.getTokens (document, 0).empty());

    tokeniser.invalidateLines (5, 1); // clamped and inverted: no-op, no crash
    ASSERT_FALSE (tokeniser.getTokens (document, 0).empty());
}

// ==============================================================================
// Document and definition lifecycle
// ==============================================================================

TEST (CodeTokeniserTests, SwitchingDocumentsDetachesFromPrevious)
{
    CodeDocument first;
    first.setText ("int a;");
    CodeDocument second;
    second.setText ("int b;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    ASSERT_FALSE (tokeniser.getTokens (first, 0).empty());
    ASSERT_FALSE (tokeniser.getTokens (second, 0).empty());

    // Editing the first document must not crash (the tokeniser no longer listens to it)
    // and must not disturb the second document's cache.
    first.setText ("float a;");

    const auto tokens = tokeniser.getTokens (second, 0);
    ASSERT_NE (nullptr, findTokenOfType (tokens, SyntaxDefinition::TokenType::identifier));
    EXPECT_EQ (String ("b"), tokenText (second, 0, *findTokenOfType (tokens, SyntaxDefinition::TokenType::identifier)));
}

TEST (CodeTokeniserTests, TokeniserDestructionRemovesListener)
{
    CodeDocument document;
    document.setText ("int a;");

    {
        CodeTokeniser tokeniser;
        tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));
        ASSERT_FALSE (tokeniser.getTokens (document, 0).empty());
    }

    // The destructor removed the tokeniser's listener, so editing afterwards must not
    // notify a dangling object.
    document.replaceRange (CodeDocument::Position (document, 0, 0),
                           CodeDocument::Position (document, 0, 3),
                           "float");
    EXPECT_EQ (String ("float a;"), document.getText());
}

TEST (CodeTokeniserTests, SwitchingDefinitionInvalidatesCache)
{
    CodeDocument document;
    document.setText ("def foo():\n    return 1");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // "def" is not a C++ keyword.
    EXPECT_EQ (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::keyword));

    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto tokens = tokeniser.getTokens (document, 0);
    ASSERT_NE (nullptr, findTokenOfType (tokens, SyntaxDefinition::TokenType::keyword));
    EXPECT_EQ (String ("def"), tokenText (document, 0, *findTokenOfType (tokens, SyntaxDefinition::TokenType::keyword)));
}

TEST (CodeTokeniserTests, SetTextRetokenizesFromScratch)
{
    CodeDocument document;
    document.setText ("int a;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));
    ASSERT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::type));

    document.setText ("#define X\nint a;");

    const auto line0 = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, line0.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::preprocessor, line0[0].type);
}

// ==============================================================================
// Raw string / multi-line string edges
// ==============================================================================

TEST (CodeTokeniserTests, EditOpeningRawStringRipplesToFollowingLines)
{
    CodeDocument document;
    document.setText ("auto s = \"abc\";\nint x;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    EXPECT_EQ (nullptr, findTokenOfType (tokeniser.getTokens (document, 1), SyntaxDefinition::TokenType::string));

    // Turn line 0 into an unclosed raw string literal, so line 1 becomes a continuation.
    document.replaceRange (CodeDocument::Position (document, 0, 9),
                           CodeDocument::Position (document, 0, 15),
                           "R\"(abc");

    const auto line1 = tokeniser.getTokens (document, 1);
    ASSERT_EQ (1u, line1.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line1[0].type);
    EXPECT_EQ (String ("int x;"), tokenText (document, 1, line1[0]));
}

TEST (CodeTokeniserTests, RawStringWithMismatchedCloseDelimiterStaysOpen)
{
    CodeDocument document;
    document.setText ("R\"foo(abc)bar\"");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // The closing ")" must be followed by the exact delimiter "foo", so the literal
    // never closes on this line and the whole line becomes one string token.
    const auto tokens = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, tokens.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, tokens[0].type);
    EXPECT_EQ (String ("R\"foo(abc)bar\""), tokenText (document, 0, tokens[0]));
}

TEST (CodeTokeniserTests, RawStringWithOversizedDelimiterFallsBackGracefully)
{
    CodeDocument document;
    document.setText ("R\"aaaaaaaaaaaaaaaaa(abc)aaaaaaaaaaaaaaaaa\"");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // The delimiter exceeds the 16-character limit, so the raw-string branch is not
    // taken: 'R' is an identifier and the rest scans as an ordinary string.
    const auto tokens = tokeniser.getTokens (document, 0);
    ASSERT_EQ (2u, tokens.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::identifier, tokens[0].type);
    EXPECT_EQ (String ("R"), tokenText (document, 0, tokens[0]));
    EXPECT_EQ (SyntaxDefinition::TokenType::string, tokens[1].type);
}

TEST (CodeTokeniserTests, EscapedTripleQuoteDoesNotClosePythonString)
{
    CodeDocument document;
    document.setText ("\"\"\"a\\\"\"\"\nstill string");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto line0 = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, line0.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line0[0].type);

    // The escaped quote on line 0 does not close the triple-quoted string.
    const auto line1 = tokeniser.getTokens (document, 1);
    ASSERT_EQ (1u, line1.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line1[0].type);
}

TEST (CodeTokeniserTests, PythonSingleQuotedTripleStringSpansLines)
{
    CodeDocument document;
    document.setText ("'''one\ntwo'''");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto line0 = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, line0.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line0[0].type);
    EXPECT_EQ (String ("'''one"), tokenText (document, 0, line0[0]));

    const auto line1 = tokeniser.getTokens (document, 1);
    ASSERT_EQ (1u, line1.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line1[0].type);
    EXPECT_EQ (String ("two'''"), tokenText (document, 1, line1[0]));
}
