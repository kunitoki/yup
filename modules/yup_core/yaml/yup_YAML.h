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
/**
    Contains static methods for converting YAML-formatted text to and from var objects.

    The var class is structurally compatible with YAML-formatted data, so these
    functions allow you to parse YAML into a var object, and to convert a var
    object to YAML-formatted text.

    The parser accepts the JSON-compatible subset of YAML 1.2 plus the common
    configuration-file constructs, which includes:

    - Block and flow-style mappings and sequences
    - Plain, single-quoted, and double-quoted scalars
    - Comments (anything following a # outside of a quoted scalar)
    - Block scalars using the | (literal) and > (folded) indicators, including
      the +/- chomping indicators and explicit indentation indicators
    - Core-schema type resolution: null (~, null, Null, NULL and empty scalars),
      booleans (true/True/TRUE/false/False/FALSE), integers (decimal, 0x and 0o
      prefixes, and _ digit separators), and floats (decimal, exponents, .inf
      and .nan variants); everything else is treated as a string
    - Anchors (&name), aliases (*name), and merge keys (<<:), which are resolved
      by copying data into the resulting var tree

    The following YAML features are NOT supported, and will either be treated as
    plain data or cause a parse error:

    - Custom tags (!tag and !!tag)
    - Multi-document streams (--- and ... document markers)
    - YAML 1.1 boolean spellings such as yes/no/on/off (these stay strings)

    Note that the writer never emits anchors, aliases or merge keys - data is
    always written out expanded and inline.

    @see var

    @tags{Core}
*/
class YUP_API YAML
{
public:
    //==============================================================================
    /** Parses a string of YAML-formatted text, and returns a result code containing
        any parse errors.

        This will return the parsed structure in the parsedResult parameter, and will
        return a Result object to indicate whether parsing was successful, and if not,
        it will contain an error message including the line and column at which the
        error was encountered.

        If you're not interested in the error message, you can use one of the other
        shortcut parse methods, which simply return a var() if the parsing fails.

        Note that this will only parse a document which starts with a mapping or a
        sequence definition. If you want to also be able to parse any kind of
        primitive YAML scalar, use the fromString() method.
    */
    static Result parse (const String& text, var& parsedResult);

    /** Attempts to parse some YAML-formatted text, and returns the result as a var object.

        If the parsing fails, this simply returns var() - if you need to find out more
        detail about the parse error, use the alternative parse() method which returns a Result.

        Note that this will only parse a document which starts with a mapping or a
        sequence definition. If you want to also be able to parse any kind of
        primitive YAML scalar, use the fromString() method.
    */
    static var parse (const String& text);

    /** Attempts to parse some YAML-formatted text from a file, and returns the result
        as a var object.

        Note that this is just a short-cut for reading the entire file into a string and
        parsing the result.

        If the parsing fails, this simply returns var() - if you need to find out more
        detail about the parse error, use the alternative parse() method which returns a Result.
    */
    static var parse (const File& file);

    /** Attempts to parse some YAML-formatted text from a stream, and returns the result
        as a var object.

        Note that this is just a short-cut for reading the entire stream into a string and
        parsing the result.

        If the parsing fails, this simply returns var() - if you need to find out more
        detail about the parse error, use the alternative parse() method which returns a Result.
    */
    static var parse (InputStream& input);

    enum class Spacing
    {
        none,       ///< All optional whitespace should be omitted, and output should use flow style
        singleLine, ///< All output should be on a single line using flow style, but with some additional spacing, e.g. after commas and colons
        multiLine,  ///< Output will use block style with newlines and indentation, in order to make it easy to read for humans
    };

    /**
        Allows formatting var objects as YAML with various configurable options.
    */
    class [[nodiscard]] FormatOptions
    {
    public:
        /** Returns a copy of this Formatter with the specified spacing. */
        FormatOptions withSpacing (Spacing x) const { return withMember (*this, &FormatOptions::spacing, x); }

        /** Returns a copy of this Formatter with the specified maximum number of decimal places.
            This option determines the precision of floating point numbers in scientific notation.
        */
        FormatOptions withMaxDecimalPlaces (int x) const { return withMember (*this, &FormatOptions::maxDecimalPlaces, x); }

        /** Returns a copy of this Formatter with the specified indent level.
            This should only be necessary when serialising multiline nested types.
        */
        FormatOptions withIndentLevel (int x) const { return withMember (*this, &FormatOptions::indent, x); }

        /** Returns the spacing used by this Formatter. */
        Spacing getSpacing() const { return spacing; }

        /** Returns the maximum number of decimal places used by this Formatter. */
        int getMaxDecimalPlaces() const { return maxDecimalPlaces; }

        /** Returns the indent level of this Formatter. */
        int getIndentLevel() const { return indent; }

    private:
        Spacing spacing = Spacing::multiLine;
        int maxDecimalPlaces = 15;
        int indent = 0;
    };

    //==============================================================================
    /** Returns a string which contains a YAML-formatted representation of the var object.
        If allOnOneLine is true, the result will be compacted into a single line of text
        using flow style. If false, it will be laid-out in a more human-readable block style.
        The maximumDecimalPlaces parameter determines the precision of floating point numbers
        in scientific notation.
        @see writeToStream
    */
    static String toString (const var& objectToFormat,
                            bool allOnOneLine = false,
                            int maximumDecimalPlaces = 15);

    /** Returns a string which contains a YAML-formatted representation of the var object, using
        formatting described by the FormatOptions parameter.
        @see writeToStream
    */
    static String toString (const var& objectToFormat,
                            const FormatOptions& formatOptions);

    /** Parses a string that was created with the toString() method.
        This is slightly different to the parse() methods because they will reject primitive
        values and only accept array or object definitions, whereas this method will handle
        either.
    */
    static var fromString (StringRef);

    /** Writes a YAML-formatted representation of the var object to the given stream.
        If allOnOneLine is true, the result will be compacted into a single line of text
        using flow style. If false, it will be laid-out in a more human-readable block style.
        The maximumDecimalPlaces parameter determines the precision of floating point numbers
        in scientific notation.
        @see toString
    */
    static void writeToStream (OutputStream& output,
                               const var& objectToFormat,
                               bool allOnOneLine = false,
                               int maximumDecimalPlaces = 15);

    /** Writes a YAML-formatted representation of the var object to the given stream, using
        formatting described by the FormatOptions parameter.
        @see toString
    */
    static void writeToStream (OutputStream& output,
                               const var& objectToFormat,
                               const FormatOptions& formatOptions);

    /** Returns a version of a string with any extended characters escaped for use inside
        a double-quoted YAML scalar. */
    static String escapeString (StringRef);

    /** Parses a quoted string-literal in YAML format, returning the un-escaped result in the
        result parameter, and an error message in case the content was illegal.
        This advances the text parameter, leaving it positioned after the closing quote.
    */
    static Result parseQuotedString (String::CharPointerType& text, var& result);

private:
    //==============================================================================
    YAML() = delete; // This class can't be instantiated - just use its static methods.
};

} // namespace yup
