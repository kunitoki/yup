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

// Injections split strings, comments and directives into finer scopes: this joins adjacent tokens of a type back.
std::vector<String> joinedRunsOfType (const CodeDocument& document, int line, Span<const CodeTokeniser::Token> tokens, SyntaxDefinition::TokenType type)
{
    std::vector<String> runs;
    int lastEnd = -1;

    for (const auto& token : tokens)
    {
        if (token.type != type)
        {
            lastEnd = -1;
            continue;
        }

        if (token.start == lastEnd)
            runs.back() += tokenText (document, line, token);
        else
            runs.push_back (tokenText (document, line, token));

        lastEnd = token.end;
    }

    return runs;
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

    ASSERT_FALSE (tokens.empty());
    EXPECT_EQ (SyntaxDefinition::TokenType::preprocessor, tokens[0].type);
    EXPECT_EQ (0, tokens.front().start);
    EXPECT_EQ (17, tokens.back().end);

    for (const auto& token : tokens)
    {
        EXPECT_NE (SyntaxDefinition::TokenType::identifier, token.type);
        EXPECT_NE (SyntaxDefinition::TokenType::operator_, token.type);

        if (tokenText (document, 0, token) == "<vector>")
            EXPECT_EQ (Identifier ("string.include"), token.scope);
    }
}

TEST (CodeTokeniserTests, StringTokensHandleEscapes)
{
    CodeDocument document;
    document.setText ("auto s = \"say \\\"hi\\\"\";");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    const auto strings = joinedRunsOfType (document, 0, tokens, SyntaxDefinition::TokenType::string);
    ASSERT_EQ (1u, strings.size());
    EXPECT_EQ (String ("\"say \\\"hi\\\"\""), strings[0]);
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

    const auto stringTexts = joinedRunsOfType (document, 0, tokens, SyntaxDefinition::TokenType::string);

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

    const auto stringTexts = joinedRunsOfType (document, 0, tokens, SyntaxDefinition::TokenType::string);

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

    const auto stringTexts = joinedRunsOfType (document, 0, tokens, SyntaxDefinition::TokenType::string);

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
    const auto strings = joinedRunsOfType (document, 0, tokens, SyntaxDefinition::TokenType::string);
    ASSERT_EQ (1u, strings.size());
    EXPECT_EQ (String ("'\\''"), strings[0]);
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
    ASSERT_FALSE (line1.empty());
    EXPECT_EQ (SyntaxDefinition::TokenType::preprocessor, line1[0].type);
    EXPECT_EQ (12, line1.back().end);

    for (const auto& token : line1)
        EXPECT_NE (SyntaxDefinition::TokenType::operator_, token.type);
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

TEST (CodeTokeniserTests, InsertedLinesShiftCacheAndPropagateState)
{
    CodeDocument document;
    document.setText ("int a;\nint b;\nint c;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    for (int line = 0; line < document.getNumLines(); ++line)
        tokeniser.getTokens (document, line);

    document.insertText (CodeDocument::Position (document, 1, 0), "x;\n/*\n");

    ASSERT_EQ (5, document.getNumLines());
    EXPECT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::type));
    EXPECT_EQ (nullptr, findTokenOfType (tokeniser.getTokens (document, 3), SyntaxDefinition::TokenType::type));
    EXPECT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 4), SyntaxDefinition::TokenType::comment));

    document.removeRange (CodeDocument::Position (document, 1, 0), CodeDocument::Position (document, 3, 0));

    ASSERT_EQ (3, document.getNumLines());
    EXPECT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 2), SyntaxDefinition::TokenType::type));
    EXPECT_EQ (nullptr, findTokenOfType (tokeniser.getTokens (document, 2), SyntaxDefinition::TokenType::comment));
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
    ASSERT_FALSE (line0.empty());
    EXPECT_EQ (SyntaxDefinition::TokenType::preprocessor, line0[0].type);
    EXPECT_EQ (Identifier ("identifier.macro"), line0.back().scope);
    EXPECT_EQ (9, line0.back().end);
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
    ASSERT_EQ (1u, joinedRunsOfType (document, 0, line0, SyntaxDefinition::TokenType::string).size());

    for (const auto& token : line0)
        EXPECT_EQ (SyntaxDefinition::TokenType::string, token.type);

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

// ==============================================================================
// Block comment and raw string delimiter edges
// ==============================================================================

TEST (CodeTokeniserTests, BlockCommentWithEmptyEndNeverCloses)
{
    SyntaxDefinition definition;
    definition.loadFromData (R"({
        "name": "TestLang",
        "blockComment": { "start": "<<" }
    })");

    CodeDocument document;
    document.setText ("<< comment text");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (definition);

    // An empty "end" delimiter can never be found in the text, so the block comment
    // consumes the rest of the line (covers findPattern's empty-pattern guard).
    const auto tokens = tokeniser.getTokens (document, 0);
    ASSERT_EQ (1u, tokens.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::comment, tokens[0].type);
    EXPECT_EQ (String ("<< comment text"), tokenText (document, 0, tokens[0]));
}

TEST (CodeTokeniserTests, RawStringPrefixFollowedBySpaceIsNotRawString)
{
    CodeDocument document;
    document.setText ("R\" (bad)\";");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    const auto tokens = tokeniser.getTokens (document, 0);

    // Whitespace immediately after the opening quote is not a valid raw-string
    // delimiter character, so "R" falls back to being an ordinary identifier and
    // the rest tokenizes as an ordinary double-quoted string.
    ASSERT_GE (tokens.size(), 2u);
    EXPECT_EQ (SyntaxDefinition::TokenType::identifier, tokens[0].type);
    EXPECT_EQ (String ("R"), tokenText (document, 0, tokens[0]));

    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("\" (bad)\""), tokenText (document, 0, *stringToken));
}

// ==============================================================================
// Multi-line string delimiter edges
// ==============================================================================

TEST (CodeTokeniserTests, MultiLineDelimiterWithoutMultiLineFlagDoesNotSpanLines)
{
    SyntaxDefinition definition;
    definition.loadFromData (R"({
        "name": "TestLang",
        "strings": { "multiLineDelimiters": ["\"\"\""], "multiLine": false }
    })");

    CodeDocument document;
    document.setText ("x = \"\"\"unterminated");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (definition);

    // The delimiter is recognized, but multi-line strings are disabled, so the
    // unterminated match simply consumes to the end of the line without carrying
    // any state to the next line.
    const auto tokens = tokeniser.getTokens (document, 0);
    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("\"\"\"unterminated"), tokenText (document, 0, *stringToken));
}

TEST (CodeTokeniserTests, TripleQuotedStringClosesOnSameLine)
{
    CodeDocument document;
    document.setText ("x = \"\"\"hello\"\"\"\ny = 1");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto line0 = tokeniser.getTokens (document, 0);
    const auto* stringToken = findTokenOfType (line0, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("\"\"\"hello\"\"\""), tokenText (document, 0, *stringToken));

    // The string closed on line 0, so line 1 is normal code, not a continuation.
    const auto line1 = tokeniser.getTokens (document, 1);
    EXPECT_EQ (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::string));
    EXPECT_NE (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::number));
}

TEST (CodeTokeniserTests, PythonPrefixedTripleQuotedStringClosesOnSameLine)
{
    CodeDocument document;
    document.setText ("doc = f\"\"\"hello\"\"\"\nvalue = 3");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto line0 = tokeniser.getTokens (document, 0);
    const auto* stringToken = findTokenOfType (line0, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("f\"\"\"hello\"\"\""), tokenText (document, 0, *stringToken));

    // The string closed on line 0, so line 1 is normal code, not a continuation.
    const auto line1 = tokeniser.getTokens (document, 1);
    EXPECT_EQ (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::string));
    EXPECT_NE (nullptr, findTokenOfType (line1, SyntaxDefinition::TokenType::number));
}

TEST (CodeTokeniserTests, PrefixedMultiLineDelimiterWithoutMultiLineFlagDoesNotSpanLines)
{
    SyntaxDefinition definition;
    definition.loadFromData (R"({
        "name": "TestLang",
        "strings": { "delimiters": ["\""], "multiLineDelimiters": ["\"\"\""], "multiLine": false, "stringPrefixes": ["f"] }
    })");

    CodeDocument document;
    document.setText ("x = f\"\"\"unterminated");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (definition);

    // The prefixed delimiter is recognized, but multi-line strings are disabled, so
    // the unterminated match consumes to the end of the line without returning a
    // continuation state.
    const auto tokens = tokeniser.getTokens (document, 0);
    const auto* stringToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::string);
    ASSERT_NE (nullptr, stringToken);
    EXPECT_EQ (String ("f\"\"\"unterminated"), tokenText (document, 0, *stringToken));
}

TEST (CodeTokeniserTests, PythonPrefixedStringWithEscapedQuote)
{
    CodeDocument document;
    document.setText ("s = f\"a\\\"b\"");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));

    const auto tokens = tokeniser.getTokens (document, 0);

    // The escaped quote inside the prefixed single-line string must not close it early.
    const auto strings = joinedRunsOfType (document, 0, tokens, SyntaxDefinition::TokenType::string);
    ASSERT_EQ (1u, strings.size());
    EXPECT_EQ (String ("f\"a\\\"b\""), strings[0]);
}

// ==============================================================================
// Cache and lifecycle edge cases
// ==============================================================================

TEST (CodeTokeniserTests, InvalidateLinesOnEmptyCacheIsANoOp)
{
    CodeDocument document;
    document.setText ("int a;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // No getTokens call yet, so the cache is empty; invalidateLines must be a
    // harmless no-op rather than indexing into an empty vector.
    tokeniser.invalidateLines (0, 5);

    EXPECT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::type));
}

TEST (CodeTokeniserTests, DocumentEditAfterClearIsHarmless)
{
    CodeDocument document;
    document.setText ("int a;");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    // Attach to the document and populate the cache.
    ASSERT_NE (nullptr, findTokenOfType (tokeniser.getTokens (document, 0), SyntaxDefinition::TokenType::type));

    // clear() empties the cache but does not detach the listener, so the next edit
    // notifies codeDocumentChanged with an empty cache — it must be a no-op rather
    // than dereferencing a non-existent cache entry.
    tokeniser.clear();

    document.replaceRange (CodeDocument::Position (document, 0, 0),
                           CodeDocument::Position (document, 0, 3),
                           "float");

    const auto tokens = tokeniser.getTokens (document, 0);
    const auto* typeToken = findTokenOfType (tokens, SyntaxDefinition::TokenType::type);
    ASSERT_NE (nullptr, typeToken);
    EXPECT_EQ (String ("float"), tokenText (document, 0, *typeToken));
}

TEST (CodeTokeniserTests, ReloadingSyntaxDefinitionInPlacePropagatesStateChange)
{
    SyntaxDefinition definition;
    definition.loadFromData (R"({
        "name": "TestLang",
        "strings": { "delimiters": ["\""] }
    })");

    CodeDocument document;
    document.setText ("a\n~~unterminated\nb");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (definition);

    // Prime the cache for all three lines under the original definition (no
    // multi-line delimiters yet, so "~~" tokenizes as ordinary characters).
    for (int i = 0; i < 3; ++i)
        ASSERT_FALSE (tokeniser.getTokens (document, i).empty());

    EXPECT_EQ (nullptr, findTokenOfType (tokeniser.getTokens (document, 1), SyntaxDefinition::TokenType::string));

    // Reload the same definition object in place, now declaring "~~" as a
    // multi-line string delimiter. loadFromData is additive, so this changes what
    // line 1 means without the tokeniser being told — it only reacts to
    // CodeDocument edits, so the caller must invalidate the affected lines itself.
    definition.loadFromData (R"({
        "name": "TestLang",
        "strings": { "multiLineDelimiters": ["~~"], "multiLine": true }
    })");

    tokeniser.invalidateLines (1, 1);

    // Line 1 now opens an unterminated multi-line string, which changes its exit
    // state and must ripple forward to mark line 2 dirty as well.
    const auto line1 = tokeniser.getTokens (document, 1);
    ASSERT_EQ (1u, line1.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line1[0].type);

    const auto line2 = tokeniser.getTokens (document, 2);
    ASSERT_EQ (1u, line2.size());
    EXPECT_EQ (SyntaxDefinition::TokenType::string, line2[0].type);
}

// ==============================================================================
// Rule states
// ==============================================================================

class CodeTokeniserRuleTests : public ::testing::Test
{
protected:
    void load (const char* json)
    {
        const auto result = definition.loadFromData (json);
        ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
        tokeniser.setSyntaxDefinition (definition);
    }

    String scopeOf (int line, StringRef text)
    {
        const auto lineText = document.getLine (line);
        for (const auto& token : tokeniser.getTokens (document, line))
        {
            if (lineText.substring (token.start, token.end) == text)
                return token.scope.toString();
        }

        return "<missing>";
    }

    CodeDocument document;
    SyntaxDefinition definition;
    CodeTokeniser tokeniser;
};

TEST_F (CodeTokeniserRuleTests, RootRulesRunBeforeBuiltInLexer)
{
    load (R"json({
        "name": "Test",
        "keywords": ["def"],
        "states": { "root": { "rules": [
            { "match": "@[A-Za-z_.]+", "scope": "preprocessor.decorator" },
            { "match": "(def)(\\s+)([A-Za-z_]\\w*)", "captures": { "1": "keyword", "3": "entity.name.function" } }
        ] } }
    })json");

    document.setText ("@app.route\ndef handler(x): return 1");

    EXPECT_EQ (String ("preprocessor.decorator"), scopeOf (0, "@app.route"));
    EXPECT_EQ (String ("keyword"), scopeOf (1, "def"));
    EXPECT_EQ (String ("entity.name.function"), scopeOf (1, "handler"));
    EXPECT_EQ (String ("number"), scopeOf (1, "1"));

    const auto tokens = tokeniser.getTokens (document, 1);
    ASSERT_FALSE (tokens.empty());
    EXPECT_EQ (SyntaxDefinition::TokenType::keyword, tokens[0].type);
    EXPECT_EQ (SyntaxDefinition::TokenType::other, tokens[2].type);
}

TEST_F (CodeTokeniserRuleTests, BuiltInTokensCarryTheirTypeScope)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));
    document.setText ("int x = 1; // done");

    for (const auto& token : tokeniser.getTokens (document, 0))
        EXPECT_EQ (Identifier (SyntaxDefinition::tokenTypeToString (token.type)), token.scope);
}

TEST_F (CodeTokeniserRuleTests, WordSetsScopeIdentifiers)
{
    load (R"json({ "name": "Test", "keywords": ["len"], "wordSets": { "support.function": ["print", "len"] } })json");
    document.setText ("print len value");

    EXPECT_EQ (String ("support.function"), scopeOf (0, "print"));
    EXPECT_EQ (String ("support.function"), scopeOf (0, "len"));
    EXPECT_EQ (String ("identifier"), scopeOf (0, "value"));
}

TEST_F (CodeTokeniserRuleTests, PushedStateSpansLinesUntilPoppedByCapturedDelimiter)
{
    load (R"json({
        "name": "Test",
        "states": {
            "root": { "rules": [ { "match": "<<([A-Z]+)", "scope": "string.heredoc", "push": "heredoc" } ] },
            "heredoc": { "scope": "string.heredoc", "rules": [ { "match": "^$1$", "scope": "string.heredoc", "pop": true } ] }
        }
    })json");

    document.setText ("cat <<END\nEND x\nEND\nafter");

    EXPECT_EQ (String ("string.heredoc"), scopeOf (0, "<<END"));

    const auto body = tokeniser.getTokens (document, 1);
    ASSERT_EQ (1u, body.size());
    EXPECT_EQ (String ("string.heredoc"), body[0].scope.toString());

    EXPECT_EQ (String ("string.heredoc"), scopeOf (2, "END"));
    EXPECT_EQ (String ("identifier"), scopeOf (3, "after"));
}

TEST_F (CodeTokeniserRuleTests, ZeroWidthRuleCanPopAtEndOfLine)
{
    load (R"json({
        "name": "Test",
        "states": {
            "root": { "rules": [ { "match": "#", "scope": "preprocessor", "push": "directive" } ] },
            "directive": { "scope": "preprocessor", "rules": [
                { "match": "//.*", "scope": "comment" },
                { "match": "$", "pop": true }
            ] }
        }
    })json");

    document.setText ("#define X // note\nnext");

    EXPECT_EQ (String ("comment"), scopeOf (0, "// note"));
    EXPECT_EQ (String ("identifier"), scopeOf (1, "next"));
}

TEST_F (CodeTokeniserRuleTests, EditingPushLineRetokenizesFollowingLines)
{
    load (R"json({
        "name": "Test",
        "states": {
            "root": { "rules": [ { "match": "<<<", "scope": "string", "push": "block" } ] },
            "block": { "scope": "string", "rules": [ { "match": ">>>", "scope": "string", "pop": true } ] }
        }
    })json");

    document.setText ("a\nword\n>>>");
    EXPECT_EQ (String ("identifier"), scopeOf (1, "word"));

    document.insertText (CodeDocument::Position (document, 0, 0), "<<<");
    EXPECT_EQ (String ("string"), tokeniser.getTokens (document, 1)[0].scope.toString());

    document.removeRange (CodeDocument::Position (document, 0, 0), CodeDocument::Position (document, 0, 3));
    EXPECT_EQ (String ("identifier"), scopeOf (1, "word"));
}

TEST_F (CodeTokeniserRuleTests, BuiltInPythonScopesDecoratorsAndDefinitionNames)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));
    document.setText ("    @functools.cache\n    def compute(x):\nclass Widget(Base):");

    EXPECT_EQ (String ("preprocessor.decorator"), scopeOf (0, "@functools.cache"));
    EXPECT_EQ (String ("keyword"), scopeOf (1, "def"));
    EXPECT_EQ (String ("identifier.function"), scopeOf (1, "compute"));
    EXPECT_EQ (String ("keyword"), scopeOf (2, "class"));
    EXPECT_EQ (String ("type.class"), scopeOf (2, "Widget"));
}

TEST_F (CodeTokeniserRuleTests, BuiltInXmlScopesTagsAndAttributes)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("xml"));
    document.setText ("<svg:rect width=\"10\" data-id='a'/></svg:rect>");

    EXPECT_EQ (String ("keyword.tag"), scopeOf (0, "svg:rect"));
    EXPECT_EQ (String ("type.attribute"), scopeOf (0, "width"));
    EXPECT_EQ (String ("type.attribute"), scopeOf (0, "data-id"));
    EXPECT_EQ (String ("string"), scopeOf (0, "\"10\""));
    EXPECT_EQ (String ("operator"), scopeOf (0, "</"));
}

TEST_F (CodeTokeniserRuleTests, InjectionsSplitStringAndCommentTokens)
{
    load (R"json({
        "name": "Test",
        "lineComment": "//",
        "strings": { "delimiters": ["\""], "escape": "\\" },
        "states": {
            "escapes": { "rules": [ { "match": "\\\\.", "scope": "string.escape" } ] },
            "notes": { "rules": [ { "match": "TODO", "scope": "comment.todo" } ] }
        },
        "injections": { "string": "escapes", "comment": "notes" }
    })json");

    document.setText ("\"a\\nb\" // TODO later");

    EXPECT_EQ (String ("string"), scopeOf (0, "\"a"));
    EXPECT_EQ (String ("string.escape"), scopeOf (0, "\\n"));
    EXPECT_EQ (String ("string"), scopeOf (0, "b\""));
    EXPECT_EQ (String ("comment"), scopeOf (0, "// "));
    EXPECT_EQ (String ("comment.todo"), scopeOf (0, "TODO"));
    EXPECT_EQ (String ("comment"), scopeOf (0, " later"));
}

TEST_F (CodeTokeniserRuleTests, InjectionsRejectUnknownTokenTypesAndStates)
{
    SyntaxDefinition bad;
    EXPECT_TRUE (bad.loadFromData (R"json({ "name": "Test", "states": { "s": { "rules": [] } }, "injections": { "bogus": "s" } })json").failed());

    SyntaxDefinition missing;
    EXPECT_TRUE (missing.loadFromData (R"json({ "name": "Test", "injections": { "string": "nowhere" } })json").failed());
}

TEST_F (CodeTokeniserRuleTests, SkipWordsLeavesKnownWordsToTheLexer)
{
    load (R"json({
        "name": "Test",
        "keywords": ["if"],
        "states": { "root": { "rules": [
            { "match": "[A-Za-z_]\\w*(?=\\s*\\()", "scope": "identifier.function", "skipWords": true }
        ] } }
    })json");

    document.setText ("if (x) call (y)");

    EXPECT_EQ (String ("keyword"), scopeOf (0, "if"));
    EXPECT_EQ (String ("identifier.function"), scopeOf (0, "call"));
}

TEST_F (CodeTokeniserRuleTests, BuiltInCppRawStringsAreNotInjected)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));
    document.setText ("R\"(a\\n%d)\"");

    EXPECT_EQ (String ("string.raw"), scopeOf (0, "R\"(a\\n%d)\""));
}

TEST_F (CodeTokeniserRuleTests, BuiltInCppScopes)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));
    document.setText ("#include <vector>\n"
                      "namespace yup { class YUP_API Widget; }\n"
                      "if (ptr->size() > MAX_SIZE) return std::max (a, b); // TODO: fix\n"
                      "auto s = \"x=%d\\n\"; obj.member = nullptr;\n"
                      "[[nodiscard]] Widget make();\n"
                      "/// @param x the value");

    EXPECT_EQ (String ("string.include"), scopeOf (0, "<vector>"));
    EXPECT_EQ (String ("type.namespace"), scopeOf (1, "yup"));
    EXPECT_EQ (String ("identifier.macro"), scopeOf (1, "YUP_API"));
    EXPECT_EQ (String ("type.class"), scopeOf (1, "Widget"));
    EXPECT_EQ (String ("keyword.control"), scopeOf (2, "if"));
    EXPECT_EQ (String ("identifier.function"), scopeOf (2, "size"));
    EXPECT_EQ (String ("identifier.constant"), scopeOf (2, "MAX_SIZE"));
    EXPECT_EQ (String ("keyword.control"), scopeOf (2, "return"));
    EXPECT_EQ (String ("type.namespace"), scopeOf (2, "std"));
    EXPECT_EQ (String ("identifier.function"), scopeOf (2, "max"));
    EXPECT_EQ (String ("comment.todo"), scopeOf (2, "TODO:"));
    EXPECT_EQ (String ("string.format"), scopeOf (3, "%d"));
    EXPECT_EQ (String ("string.escape"), scopeOf (3, "\\n"));
    EXPECT_EQ (String ("identifier.member"), scopeOf (3, "member"));
    EXPECT_EQ (String ("keyword.constant"), scopeOf (3, "nullptr"));
    EXPECT_EQ (String ("keyword.attribute"), scopeOf (4, "[[nodiscard]]"));
    EXPECT_EQ (String ("type.class"), scopeOf (4, "Widget"));
    EXPECT_EQ (String ("identifier.function"), scopeOf (4, "make"));
    EXPECT_EQ (String ("comment.doc.tag"), scopeOf (5, "@param"));
}

TEST_F (CodeTokeniserRuleTests, BuiltInGlslScopes)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("glsl"));
    document.setText ("uniform sampler2D tex; gl_FragColor = texture (tex, uv).rgba;");

    EXPECT_EQ (String ("keyword"), scopeOf (0, "uniform"));
    EXPECT_EQ (String ("type"), scopeOf (0, "sampler2D"));
    EXPECT_EQ (String ("identifier.builtin"), scopeOf (0, "gl_FragColor"));
    EXPECT_EQ (String ("identifier.builtin"), scopeOf (0, "texture"));
    EXPECT_EQ (String ("identifier.member"), scopeOf (0, "rgba"));
}

TEST_F (CodeTokeniserRuleTests, BuiltInPythonScopes)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("python"));
    document.setText ("async def run(self, items):\n"
                      "    print(f\"{name!r} {{x}} \\t\", None, ValueError)\n"
                      "MAX_COUNT = 1_000  # TODO\n"
                      "self.value.strip()");

    EXPECT_EQ (String ("keyword.control"), scopeOf (0, "async"));
    EXPECT_EQ (String ("identifier.function"), scopeOf (0, "run"));
    EXPECT_EQ (String ("identifier.language"), scopeOf (0, "self"));
    EXPECT_EQ (String ("identifier.builtin"), scopeOf (1, "print"));
    EXPECT_EQ (String ("string.interpolation"), scopeOf (1, "{name!r}"));
    EXPECT_EQ (String ("string.escape"), scopeOf (1, "{{"));
    EXPECT_EQ (String ("string.escape"), scopeOf (1, "\\t"));
    EXPECT_EQ (String ("keyword.constant"), scopeOf (1, "None"));
    EXPECT_EQ (String ("type.exception"), scopeOf (1, "ValueError"));
    EXPECT_EQ (String ("identifier.constant"), scopeOf (2, "MAX_COUNT"));
    EXPECT_EQ (String ("number"), scopeOf (2, "1_000"));
    EXPECT_EQ (String ("comment.todo"), scopeOf (2, "TODO"));
    EXPECT_EQ (String ("identifier.member"), scopeOf (3, "value"));
    EXPECT_EQ (String ("identifier.function"), scopeOf (3, "strip"));
}

TEST_F (CodeTokeniserRuleTests, BuiltInYuiScopes)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("yui"));
    document.setText ("component LabeledKnob : Column {\n"
                      "    property label: string = \"VALUE\";\n"
                      "    width: 150ms; fill: #ff8800;\n"
                      "    .knob:hover { color: rgba(0, 0, 0, 0.5); }\n"
                      "    ComboBox:hover:enabled,\n"
                      "    .open { cursor: hand; }\n"
                      "    tint: palette.accent.darker (0.1),\n");

    EXPECT_EQ (String ("keyword"), scopeOf (0, "component"));
    EXPECT_EQ (String ("type.class"), scopeOf (0, "LabeledKnob"));
    EXPECT_EQ (String ("type.class"), scopeOf (0, "Column"));
    EXPECT_EQ (String ("keyword"), scopeOf (1, "property"));
    EXPECT_EQ (String ("identifier.property"), scopeOf (1, "label"));
    EXPECT_EQ (String ("type"), scopeOf (1, "string"));
    EXPECT_EQ (String ("identifier.property"), scopeOf (2, "width"));
    EXPECT_EQ (String ("number"), scopeOf (2, "150"));
    EXPECT_EQ (String ("number.unit"), scopeOf (2, "ms"));
    EXPECT_EQ (String ("identifier.property"), scopeOf (2, "fill"));
    EXPECT_EQ (String ("number.color"), scopeOf (2, "#ff8800"));
    EXPECT_EQ (String ("type.selector"), scopeOf (3, ".knob"));
    EXPECT_EQ (String ("type.selector"), scopeOf (3, ":hover"));
    EXPECT_EQ (String ("identifier.property"), scopeOf (3, "color"));
    EXPECT_EQ (String ("identifier.builtin"), scopeOf (3, "rgba"));
    EXPECT_EQ (String ("type.class"), scopeOf (4, "ComboBox"));
    EXPECT_EQ (String ("type.selector"), scopeOf (4, ":hover"));
    EXPECT_EQ (String ("type.selector"), scopeOf (4, ":enabled"));
    EXPECT_EQ (String ("type.selector"), scopeOf (5, ".open"));
    EXPECT_EQ (String ("identifier.property"), scopeOf (6, "tint"));
    EXPECT_EQ (String ("identifier.function"), scopeOf (6, "darker"));
}

TEST_F (CodeTokeniserRuleTests, BuiltInYuiScopesTheNameOfAShader)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("yui"));
    document.setText ("shader Crt {\n"
                      "    fragment: \"fx/crt.frag\";\n"
                      "}\n");

    EXPECT_EQ (String ("keyword"), scopeOf (0, "shader"));
    EXPECT_EQ (String ("type.class"), scopeOf (0, "Crt"));
}

TEST_F (CodeTokeniserRuleTests, BuiltInYdspScopes)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("ydsp"));
    document.setText ("import fx.Delay as fx;\n"
                      "input parameter float gain = 1.0 [[ name: \"Gain\",\n"
                      "    min: 0.0 ]];\n"
                      "processor Voice { out = sin (in * 2.0); }");

    EXPECT_EQ (String ("keyword"), scopeOf (0, "import"));
    EXPECT_EQ (String ("type.namespace"), scopeOf (0, "fx.Delay"));
    EXPECT_EQ (String ("keyword"), scopeOf (1, "parameter"));
    EXPECT_EQ (String ("type"), scopeOf (1, "float"));
    EXPECT_EQ (String ("operator"), scopeOf (1, "[["));
    EXPECT_EQ (String ("identifier.property"), scopeOf (1, "name"));
    EXPECT_EQ (String ("string"), scopeOf (1, "\"Gain\""));
    EXPECT_EQ (String ("identifier.property"), scopeOf (2, "min"));
    EXPECT_EQ (String ("operator"), scopeOf (2, "]]"));
    EXPECT_EQ (String ("type.class"), scopeOf (3, "Voice"));
    EXPECT_EQ (String ("identifier.language"), scopeOf (3, "out"));
    EXPECT_EQ (String ("identifier.builtin"), scopeOf (3, "sin"));
    EXPECT_EQ (String ("identifier.language"), scopeOf (3, "in"));
}

TEST_F (CodeTokeniserRuleTests, BuiltInXmlScopesDeclarationsEntitiesAndCdata)
{
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("xml"));
    document.setText ("<!DOCTYPE note SYSTEM \"n.dtd\">\n"
                      "<a>Tom &amp; Jerry<![CDATA[x < y]]></a>");

    EXPECT_EQ (String ("keyword"), scopeOf (0, "DOCTYPE"));
    EXPECT_EQ (String ("keyword"), scopeOf (0, "SYSTEM"));
    EXPECT_EQ (String ("string"), scopeOf (0, "\"n.dtd\""));
    EXPECT_EQ (String ("identifier.text"), scopeOf (1, "Tom"));
    EXPECT_EQ (String ("string.escape"), scopeOf (1, "&amp;"));
    EXPECT_EQ (String ("keyword"), scopeOf (1, "<![CDATA["));
    EXPECT_EQ (String ("keyword"), scopeOf (1, "]]>"));
    EXPECT_EQ (String ("keyword.tag"), scopeOf (1, "a"));
}
