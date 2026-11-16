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
    built-in C++ / GLSL / Python / XML / YDSP / YUI definitions via `getBuiltIn`.

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
        "operators": ["+", "-", "->", "::"],
        "wordSets": { "support.function": ["printf", "malloc"] },
        "states": {
            "root": { "rules": [
                { "match": "(class)(\\s+)(\\w+)", "captures": { "1": "keyword", "3": "type.class" } },
                { "match": "[A-Za-z_]\\w*(?=\\s*\\()", "scope": "identifier.function", "skipWords": true },
                { "match": "<<(\\w+)", "scope": "string.heredoc", "push": "heredoc" }
            ] },
            "heredoc": { "scope": "string.heredoc", "rules": [
                { "match": "^$1$", "scope": "string.heredoc", "pop": true }
            ] },
            "stringContent": { "rules": [
                { "match": "\\\\.", "scope": "string.escape" }
            ] }
        },
        "injections": { "string": "stringContent" }
    }
    @endcode

    Only `name` is mandatory; every other section is optional and falls back to
    sensible defaults. Colors are not part of a syntax definition: they belong
    to a CodeEditorScheme (see CodeEditor::setScheme).

    Tokens carry a dotted scope name such as `keyword` or `entity.name.function`.
    The built-in lexer produces the scopes named after TokenType (see
    tokenTypeToString), `wordSets` assign a scope to identifiers, and `states` add
    regular expression rules (see Rule and State) for anything the built-in lexer
    cannot express. The rules of the `root` state are tried at every position before
    the built-in lexer; a rule may push another state, whose rules alone are then used
    (across lines) until a rule pops it.

    `injections` map a token type name (`comment`, `string`, `preprocessor`, ...) to a
    state whose rules are searched inside every token of that type that still carries
    the plain type scope, splitting it into sub-scopes such as `string.escape` or
    `comment.todo`. Injected rules cannot push or pop states.

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
    /** A regular expression rule of a State.

        The pattern is an ECMAScript regular expression (see CodeRegex) matched at the
        current position. In the rules of a pushed state, `$1` to `$9` are replaced by
        the literal text of the capture groups of the rule that pushed the state, which
        lets a closing pattern repeat a delimiter found by the opening one (heredocs,
        raw strings).
    */
    struct Rule
    {
        String match;                                   /**< The regular expression to match. */
        Identifier scope;                               /**< The scope of the matched text, if not empty. */
        std::vector<std::pair<int, Identifier>> captures; /**< The scopes of capture groups, overriding scope where they matched. */
        String push;                                    /**< The state to enter after the match, if not empty. */
        bool pop = false;                               /**< True to leave the current state after the match. */
        bool skipWords = false;                         /**< True to ignore a match whose whole text is a keyword, type or word set word (see isKnownWord). */
    };

    /** A named set of rules, active while it is on top of the tokenizer state stack. */
    struct State
    {
        String name;             /**< The state name; `root` is the state active outside any pushed state. */
        Identifier scope;        /**< The scope of text no rule matches, used by pushed states. */
        std::vector<Rule> rules; /**< The rules, tried in order; the first match wins. */
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
    /** Returns true if the given word is a keyword, or belongs to a word set with a `keyword` scope (e.g. `keyword.control`). */
    bool isKeyword (StringRef word) const;

    /** Returns true if the given word is a type name, or belongs to a word set with a `type` scope (e.g. `type.std`). */
    bool isType (StringRef word) const;

    /** Returns true if the given word is an operator or punctuation. */
    bool isOperator (StringRef word) const;

    /** Returns the scope of a word listed in a word set, or a null Identifier. */
    Identifier getWordScope (StringRef word) const;

    /** Adds words to the word set of a scope.

        Identifiers found in a word set are given its scope, which takes precedence over the keyword and type lists.

        @param scope The scope of the words.
        @param words The words to add.
    */
    void addWords (const Identifier& scope, const StringArray& words);

    /** Returns true if the word is a keyword, a type or part of a word set. */
    bool isKnownWord (StringRef word) const;

    //==============================================================================
    /** Adds a state, or replaces the state with the same name, compiling its rules.

        @param state The state to add.
        @return The result of the operation, failing if a rule pattern is invalid.
    */
    Result addState (State state);

    /** Returns the number of states. */
    int getNumStates() const noexcept;

    /** Returns a state by index. */
    const State& getState (int stateIndex) const;

    /** Returns the index of a state by name, or -1. */
    int findState (StringRef stateName) const;

    /** Returns the compiled pattern of a rule.

        @param stateIndex The state index.
        @param ruleIndex  The rule index within the state.
        @returns The compiled regex, or nullptr if the pattern references the captures
                 of the pushing rule and must be expanded first.
    */
    const CodeRegex* getCompiledRule (int stateIndex, int ruleIndex) const;

    /** Injects the rules of a state into the tokens of a type.

        Every token of the type whose scope is the plain type scope (such as `string`, not
        `string.raw`) is searched with the rules of the state; the earliest match wins, rule
        order breaking ties, and the matches take the rule scope while the rest of the
        token keeps its own. Pushing and popping is ignored in injected rules.

        @param type      The token type to inject into.
        @param stateName The state providing the rules, or an empty string to remove the injection.
    */
    void setInjection (TokenType type, const String& stateName);

    /** Returns the index of the state injected into tokens of a type, or -1. */
    int getInjectionState (TokenType type) const;

    //==============================================================================
    /** An opening and a closing character, such as a bracket or a quote pair. */
    using CharacterPair = std::pair<yup_wchar, yup_wchar>;

    /** Returns the bracket pairs used for bracket matching (default `()`, `[]` and `{}`). */
    const std::vector<CharacterPair>& getBracketPairs() const noexcept { return bracketPairs; }

    /** Returns the pairs the editor closes automatically when the opening character is typed.

        Defaults to the bracket pairs plus every single character string delimiter.
    */
    const std::vector<CharacterPair>& getAutoClosingPairs() const noexcept { return autoClosingPairs; }

    /** Returns true if a character separates words, for word navigation, selection and search.

        Whitespace always separates words; the other separators come from the
        `wordSeparators` JSON property.
    */
    bool isWordSeparator (yup_wchar character) const;

    /** Returns the pattern a line (up to the caret) must match to indent the next line further.

        A line ending with an opening bracket always indents the next line; this pattern adds
        language specific cases, such as a trailing `:` in Python.

        @returns The compiled pattern, or nullptr if the definition has none.
    */
    const CodeRegex* getIncreaseIndentPattern() const noexcept;

    //==============================================================================
    /** Returns true if the given character can start an identifier. */
    bool isIdentifierStart (yup_wchar character) const;

    /** Returns true if the given character can be part of an identifier. */
    bool isIdentifierPart (yup_wchar character) const;

    //==============================================================================
    /** Returns a built-in definition by language name ("cpp", "glsl", "python", "xml", "ydsp" or "yui").

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

    /** Returns the token type of a scope: the type named by its first component, or TokenType::other. */
    static TokenType scopeToTokenType (const Identifier& scope);

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
    std::unordered_map<String, Identifier> wordScopes;

    struct CompiledState
    {
        State state;
        std::vector<CodeRegex> regexes;
    };

    std::vector<CompiledState> states;
    std::array<String, static_cast<size_t> (TokenType::other) + 1> injections;

    std::vector<CharacterPair> bracketPairs { { '(', ')' }, { '[', ']' }, { '{', '}' } };
    std::vector<CharacterPair> autoClosingPairs { bracketPairs };
    String wordSeparators { "()[]{}.,;:<>\"'\\/+-*%=!&|^~" };
    CodeRegex increaseIndentPattern;
};

} // namespace yup
