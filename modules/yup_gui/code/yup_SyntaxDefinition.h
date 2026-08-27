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
/** A declarative, data-driven syntax definition used by the CodeTokeniser.

    A SyntaxDefinition describes how to highlight one programming language:
    comment delimiters, string rules, number syntax, keyword/type/operator sets,
    preprocessor prefix, and per-token colors. It is loaded from a JSON file
    (see `loadFromData` / `loadFromFile` for the format) or obtained from the
    built-in C++ / GLSL / Python / XML / YDSP definitions via `getBuiltIn`.

    The JSON format is:
    @code
    {
        "name": "C++",
        "extensions": ["cpp", "h", "hpp"],
        "lineComment": "//",
        "blockComment": { "start": "&#92*", "end": "*&#47;" },
        "strings": { "delimiters": ["\\"", "'"], "escape": "\\\\", "multiLine": false },
        "preprocessor": "#",
        "numbers": { "hex": true, "binary": true, "float": true, "exponent": true, "suffix": true },
        "keywords": ["if", "for", "return"],
        "types": ["int", "float", "void"],
        "operators": ["+", "-", "->", "::"]
    }
    @endcode

    Only `name` is mandatory; every other section is optional and falls back to
    sensible defaults. Colors are not part of a syntax definition: they belong
    to a CodeEditorScheme (see CodeEditor::setScheme).

    @see CodeTokeniser
*/
class YUP_API SyntaxDefinition
{
public:
    //==============================================================================
    /** The kind of syntax token produced by the tokenizer. */
    enum class TokenType : uint8_t
    {
        comment,      /**< Comments (line and block). */
        string,       /**< String literals. */
        number,       /**< Numeric literals. */
        keyword,      /**< Reserved keywords (e.g. if, for, class). */
        type,         /**< Type names (e.g. int, float, vec3). */
        operator_,    /**< Operators and punctuation. */
        preprocessor, /**< Preprocessor directives (e.g. #include). */
        identifier,   /**< Identifiers. */
        whitespace,   /**< Whitespace. */
        other         /**< Anything else. */
    };

    //==============================================================================
    /** Start and end delimiters of a multi-character construct (block comment or multi-line string). */
    struct Delimiters
    {
        String start; /**< The opening delimiter. */
        String end;   /**< The closing delimiter. */
    };

    //==============================================================================
    /** Creates an empty, inert definition. */
    SyntaxDefinition() = default;

    /** Loads a definition from a JSON file.

        @param file The JSON file to load.
        @return The result of the operation.
    */
    Result loadFromFile (const File& file);

    /** Loads a definition from JSON text.

        @param jsonText The JSON text to parse.
        @return The result of the operation.
    */
    Result loadFromData (StringRef jsonText);

    //==============================================================================
    /** Returns the display name of the language (e.g. "C++"). */
    const String& getName() const;

    /** Returns the file extensions associated with the language, without leading dots. */
    const std::vector<String>& getFileExtensions() const;

    //==============================================================================
    /** Returns the prefix introducing a line comment (e.g. "//"), or an empty string. */
    const String& getLineCommentPrefix() const;

    /** Returns the block comment delimiters, if the language has block comments. */
    const std::optional<Delimiters>& getBlockComment() const;

    /** Returns the string delimiters (e.g. `"` and `'`). */
    const std::vector<String>& getStringDelimiters() const;

    /** Returns the multi-line string delimiters (e.g. `"""` and `'''`). */
    const std::vector<String>& getMultiLineStringDelimiters() const;

    /** Returns the escape character used inside strings, or 0 if none. */
    yup_wchar getEscapeCharacter() const;

    /** Returns true if the definition declares any multi-line string delimiters. */
    bool areStringsMultiLine() const;

    /** Returns true if C++-style raw string literals (`R"delim(...)delim"`) are supported. */
    bool supportsRawStrings() const;

    /** Returns true if the given word is a valid raw string literal prefix (e.g. "R", "u8R"). */
    bool isRawStringPrefix (StringRef word) const;

    /** Returns the configured raw string literal prefixes. */
    const std::vector<String>& getRawStringPrefixes() const;

    /** Returns true if the given word is a valid string/char literal encoding or format prefix (e.g. "u8", "L", "f"). */
    bool isStringPrefix (StringRef word) const;

    /** Returns the configured string/char literal prefixes. */
    const std::vector<String>& getStringPrefixes() const;

    /** Returns the prefix of preprocessor directives (e.g. "#"), or an empty string. */
    const String& getPreprocessorPrefix() const;

    //==============================================================================
    /** Returns true if numbers may use a 0x.. hexadecimal prefix. */
    bool numbersAllowHex() const;

    /** Returns true if numbers may use a 0b.. binary prefix. */
    bool numbersAllowBinary() const;

    /** Returns true if numbers may contain a fractional part. */
    bool numbersAllowFloat() const;

    /** Returns true if numbers may contain an exponent (e.g. 1e5). */
    bool numbersAllowExponent() const;

    /** Returns true if numbers may carry a suffix (e.g. 100u, 1.5f). */
    bool numbersAllowSuffix() const;

    //==============================================================================
    /** Returns true if the given word is a keyword. */
    bool isKeyword (StringRef word) const;

    /** Returns true if the given word is a type name. */
    bool isType (StringRef word) const;

    /** Returns true if the given word is an operator or punctuation. */
    bool isOperator (StringRef word) const;

    /** Returns true if the given character can start an identifier. */
    bool isIdentifierStart (yup_wchar character) const;

    /** Returns true if the given character can be part of an identifier. */
    bool isIdentifierPart (yup_wchar character) const;

    //==============================================================================
    /** Returns a built-in definition by language name ("cpp", "glsl", "python", "xml" or "ydsp").

        @param languageName The language name.
        @returns The definition (a default inert one if the name is unknown).
    */
    static const SyntaxDefinition& getBuiltIn (StringRef languageName);

    /** Returns a built-in definition matching a file extension, or nullptr.

        @param fileExtension The file extension without a leading dot.
        @returns The matching definition, or nullptr.
    */
    static const SyntaxDefinition* getBuiltInForExtension (StringRef fileExtension);

    /** Returns the string name of a token type, used as the key in the JSON "colors" section. */
    static String tokenTypeToString (TokenType type);

private:
    String name;
    std::vector<String> fileExtensions;

    String lineCommentPrefix;
    std::optional<Delimiters> blockComment;
    std::vector<String> stringDelimiters;
    std::vector<String> multiLineStringDelimiters;
    yup_wchar escapeCharacter = 0;
    bool multiLineStrings = false;
    bool rawStrings = false;
    std::vector<String> rawStringPrefixes;
    std::vector<String> stringPrefixes;
    String preprocessorPrefix;

    bool allowHex = true;
    bool allowBinary = true;
    bool allowFloat = true;
    bool allowExponent = true;
    bool allowSuffix = true;

    std::unordered_set<String> keywords;
    std::unordered_set<String> types;
    std::unordered_set<String> operators;
};

} // namespace yup
