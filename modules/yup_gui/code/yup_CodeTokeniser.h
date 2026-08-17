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

#pragma once

namespace yup
{

//==============================================================================
/** Incremental, line-based syntax tokenizer driven by a SyntaxDefinition.

    The tokenizer caches the tokens of every line it has been asked about and
    re-tokenizes a line only when it changed. Because multi-line constructs
    (block comments, multi-line strings) make a line's tokenization depend on the
    previous line's state, each cached line also records the state it entered and
    left; when a line's exit state changes, the next line is marked dirty and
    re-tokenized lazily on demand. This keeps the cost of an edit proportional to
    the lines actually affected and displayed, rather than the whole document —
    except when the edit changes the document's line count (e.g. pressing Enter,
    or a multi-line paste), in which case every line from the edit point onward
    is marked dirty, since the cache is indexed by line number and there is no
    cheap way to tell which of the surviving entries are still aligned with their
    line after the shift.

    The tokenizer subscribes itself as a `CodeDocument::Listener` the first time
    it is asked to tokenize a document, so edits automatically invalidate the
    affected lines.

    @code
    CodeDocument document;
    document.setText ("int main() { return 0; }");

    CodeTokeniser tokeniser;
    tokeniser.setSyntaxDefinition (SyntaxDefinition::getBuiltIn ("cpp"));

    for (auto& token : tokeniser.getTokens (document, 0))
        auto tokenType = token.type; // color it with CodeEditorScheme::getColor (tokenType)
    @endcode

    @see SyntaxDefinition, CodeDocument
*/
class YUP_API CodeTokeniser : public CodeDocument::Listener
{
public:
    //==============================================================================
    /** A token covering a character range of a single line. */
    struct Token
    {
        int start = 0;                                                         /**< The character offset within the line. */
        int end = 0;                                                           /**< The character offset one past the token. */
        SyntaxDefinition::TokenType type = SyntaxDefinition::TokenType::other; /**< The token type. */
    };

    //==============================================================================
    /** Line states used by the incremental tokenizer to track multi-line constructs. */
    enum LineState : uint8_t
    {
        normal = 0,        /**< Not inside any multi-line construct. */
        inBlockComment,    /**< The line started inside a block comment. */
        inMultiLineString, /**< The line started inside a multi-line string; + index selects the delimiter. */
        inRawString        /**< The line started inside a C++ raw string literal; the delimiter is in TokenCache::rawStringDelimiter. */
    };

    //==============================================================================
    /** Creates an empty tokenizer. */
    CodeTokeniser() = default;

    /** Destructor. */
    ~CodeTokeniser() override;

    //==============================================================================
    /** Sets the syntax definition used for tokenization, invalidating all cached tokens.

        @param definition The definition to use (must outlive this tokenizer).
    */
    void setSyntaxDefinition (const SyntaxDefinition& definition);

    /** Returns the currently active syntax definition. */
    const SyntaxDefinition& getSyntaxDefinition() const;

    /** Returns true if a syntax definition has been set. */
    bool hasSyntaxDefinition() const;

    //==============================================================================
    /** Returns the tokens of a line, re-tokenizing it if it changed.

        @param document The document to tokenize.
        @param lineIndex The 0-based line index.
        @returns The tokens of the line, in order.
    */
    Span<const Token> getTokens (const CodeDocument& document, int lineIndex);

    /** Marks a range of lines as changed so they are re-tokenized lazily on demand.

        @param firstLine The first line (inclusive).
        @param lastLine  The last line (inclusive).
    */
    void invalidateLines (int firstLine, int lastLine);

    /** Discards all cached tokens. */
    void clear();

    //==============================================================================
    /** @internal Called by the document when its content changes. */
    void codeDocumentChanged (CodeDocument& document, int firstChangedLine, int lastChangedLine) override;

private:
    struct TokenCache
    {
        std::vector<Token> tokens;
        uint8_t stateBefore = LineState::normal;
        uint8_t stateAfter = LineState::normal;
        String rawStringDelimiter; /**< The delimiter of a raw string literal in effect on this line. */
    };

    const TokenCache& tokenize (const CodeDocument& document, int lineIndex);
    void attachToDocument (const CodeDocument& document);

    const SyntaxDefinition* definition = nullptr;
    CodeDocument* attachedDocument = nullptr;

    std::vector<std::optional<TokenCache>> cache;
    std::vector<bool> dirty;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CodeTokeniser)
};

} // namespace yup
