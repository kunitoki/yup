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

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <string>

namespace yup
{

//==============================================================================
/** Handles converting var objects to YAML-formatted text. */
struct YAMLFormatter
{
    enum
    {
        indentSize = 2
    };

    //==============================================================================
    static void writeSpaces (OutputStream& out, int numSpaces)
    {
        out.writeRepeatedByte (' ', (size_t) numSpaces);
    }

    //==============================================================================
    static bool isYamlReservedWord (const String& s)
    {
        // Core-schema null/boolean spellings, plus the YAML 1.1 boolean spellings which
        // we quote defensively so that other tools never misread them.
        static const char* const reservedWords[] = { "true", "false", "null", "yes", "no", "on", "off", "y", "n", "~" };

        for (auto* word : reservedWords)
            if (s.equalsIgnoreCase (word))
                return true;

        return false;
    }

    static bool looksLikeNumber (const String& s)
    {
        auto t = s;

        if (t.startsWithChar ('+') || t.startsWithChar ('-'))
            t = t.substring (1);

        if (t.isEmpty())
            return true;

        if (t[0] >= '0' && t[0] <= '9')
            return true;

        if (t.startsWithIgnoreCase (".inf") || t.startsWithIgnoreCase (".nan"))
            return true;

        return t[0] == '.' && t.length() > 1 && t[1] >= '0' && t[1] <= '9';
    }

    static bool isRestrictedScalarStart (yup_wchar c)
    {
        switch (c)
        {
            case '!':
            case '&':
            case '*':
            case '|':
            case '>':
            case '\'':
            case '"':
            case '%':
            case '@':
            case '`':
                return true;

            default:
                return false;
        }
    }

    static bool needsQuoting (const String& s)
    {
        if (s.isEmpty())
            return true;

        const auto first = s[0];

        // Plain scalars can't start or end with whitespace, since it gets trimmed on parse
        if (CharacterFunctions::isWhitespace (first)
            || CharacterFunctions::isWhitespace (s.getLastCharacter()))
            return true;

        // A '#' can't start a scalar or be preceded by a space (it would start a comment)
        if (first == '#' || s.contains (" #"))
            return true;

        // ": " can't appear inside a plain scalar, and lone indicator chars are ambiguous.
        // A trailing ':' would be read back as a mapping key separator in a sequence or
        // flow context, so it must be quoted too.
        if (s.contains (": ")
            || s == ":" || s == "-" || s == "?"
            || s.endsWithChar (':')
            || (s.length() > 1 && (first == ':' || first == '-' || first == '?') && s[1] == ' '))
            return true;

        // These characters are flow indicators, so they're never safe in a plain scalar
        if (s.containsAnyOf (",[]{}"))
            return true;

        // Indicator characters that can't start a plain scalar
        if (isRestrictedScalarStart (first))
            return true;

        // Strings that would resolve to a non-string type under the YAML core schema
        if (isYamlReservedWord (s) || looksLikeNumber (s))
            return true;

        // The merge-key indicator would be resolved as a merge key on reparse
        if (s == "<<")
            return true;

        // Document markers would terminate a document if written at the start of a line
        if (s == "---" || s == "..." || s.startsWith ("--- ") || s.startsWith ("... "))
            return true;

        // Any control character requires escaping inside a double-quoted scalar
        for (auto i = s.getCharPointer(); ! i.isEmpty(); ++i)
            if (CharacterFunctions::isControlCharacter (*i))
                return true;

        return false;
    }

    //==============================================================================
    static void writeUnicodeEscape (OutputStream& out, yup_wchar c)
    {
        if (CharPointer_UTF16::getBytesRequiredFor (c) > 2)
        {
            CharPointer_UTF16::CharType chars[2];
            CharPointer_UTF16 utf16 (chars);
            utf16.write (c);

            for (int i = 0; i < 2; ++i)
                out << "\\u" << String::toHexString ((int) chars[i]).paddedLeft ('0', 4);
        }
        else
        {
            out << "\\u" << String::toHexString ((int) c).paddedLeft ('0', 4);
        }
    }

    static void writeEscaped (OutputStream& out, String::CharPointerType t)
    {
        for (;;)
        {
            auto c = t.getAndAdvance();

            switch (c)
            {
                case 0:
                    return;

                case '"':
                    out << "\\\"";
                    break;
                case '\\':
                    out << "\\\\";
                    break;
                case '\a':
                    out << "\\a";
                    break;
                case '\b':
                    out << "\\b";
                    break;
                case '\t':
                    out << "\\t";
                    break;
                case '\n':
                    out << "\\n";
                    break;
                case '\v':
                    out << "\\v";
                    break;
                case '\f':
                    out << "\\f";
                    break;
                case '\r':
                    out << "\\r";
                    break;
                case 0x1b: // ESC
                    out << "\\e";
                    break;

                default:
                    if (CharacterFunctions::isAsciiPrintable (c))
                    {
                        out << (char) c;
                    }
                    else
                    {
                        writeUnicodeEscape (out, c);
                    }

                    break;
            }
        }
    }

    static void writeScalar (OutputStream& out, const String& s)
    {
        if (needsQuoting (s))
        {
            out << '"';
            writeEscaped (out, s.getCharPointer());
            out << '"';
        }
        else
        {
            out << s;
        }
    }

    //==============================================================================
    static bool isEmptyContainer (const var& v)
    {
        if (v.isArray())
            return v.getArray()->isEmpty();

        if (v.isObject())
            return v.getDynamicObject()->getProperties().isEmpty();

        return false;
    }

    // parseFlowKey() (the reader) stops at the first bare ':' with no lookahead, so any
    // embedded colon here must be quoted even when writeScalar()/needsQuoting() would
    // otherwise leave it plain — block-style keys don't share this restriction.
    static void writeFlowMapKey (OutputStream& out, const String& s)
    {
        if (needsQuoting (s) || s.containsChar (':'))
        {
            out << '"';
            writeEscaped (out, s.getCharPointer());
            out << '"';
        }
        else
        {
            out << s;
        }
    }

    static void writeFlowMap (OutputStream& out, DynamicObject* object, const YAML::FormatOptions& format)
    {
        out << '{';

        int index = 0;
        for (const auto& prop : object->getProperties())
        {
            if (index > 0)
            {
                out << ',';
                if (format.getSpacing() == YAML::Spacing::singleLine)
                    out << ' ';
            }

            writeFlowMapKey (out, prop.name.toString());
            out << ':';
            if (format.getSpacing() == YAML::Spacing::singleLine)
                out << ' ';

            writeValue (out, prop.value, format, 0);
            ++index;
        }

        out << '}';
    }

    static void writeFlowSequence (OutputStream& out, const Array<var>& array, const YAML::FormatOptions& format)
    {
        out << '[';

        for (int i = 0; i < array.size(); ++i)
        {
            if (i > 0)
            {
                out << ',';
                if (format.getSpacing() == YAML::Spacing::singleLine)
                    out << ' ';
            }

            writeValue (out, array.getReference (i), format, 0);
        }

        out << ']';
    }

    //==============================================================================
    static void writeMapEntry (OutputStream& out, const String& key, const var& value, const YAML::FormatOptions& format, int keyLevel)
    {
        writeScalar (out, key);
        out << ':';

        if (value.isObject() || value.isArray())
        {
            if (isEmptyContainer (value))
            {
                out << ' ';
                writeValue (out, value, format, keyLevel);
                return;
            }

            if (format.getSpacing() == YAML::Spacing::multiLine)
            {
                out << '\n';
                writeValue (out, value, format, keyLevel + indentSize);
                return;
            }
        }

        out << ' ';
        writeValue (out, value, format, keyLevel);
    }

    static void writeBlockMap (OutputStream& out, DynamicObject* object, const YAML::FormatOptions& format, int level)
    {
        bool first = true;

        for (const auto& prop : object->getProperties())
        {
            if (! first)
                out << '\n';

            writeSpaces (out, level);
            writeMapEntry (out, prop.name.toString(), prop.value, format, level);
            first = false;
        }
    }

    static void writeSequenceItem (OutputStream& out, const var& item, const YAML::FormatOptions& format, int itemLevel)
    {
        // This is called directly after a "- " has been written; subsequent lines of the
        // item (if it's a block container) align at the column specified by itemLevel.
        if (item.isObject() || item.isArray())
        {
            if (isEmptyContainer (item))
            {
                out << (item.isArray() ? "[]" : "{}");
                return;
            }

            if (format.getSpacing() == YAML::Spacing::multiLine)
            {
                if (item.isArray())
                {
                    const auto& array = *item.getArray();
                    for (int i = 0; i < array.size(); ++i)
                    {
                        if (i > 0)
                        {
                            out << '\n';
                            writeSpaces (out, itemLevel);
                        }

                        out << "- ";
                        writeSequenceItem (out, array.getReference (i), format, itemLevel + indentSize);
                    }

                    return;
                }

                int index = 0;
                for (const auto& prop : item.getDynamicObject()->getProperties())
                {
                    if (index > 0)
                    {
                        out << '\n';
                        writeSpaces (out, itemLevel);
                    }

                    writeMapEntry (out, prop.name.toString(), prop.value, format, itemLevel);
                    ++index;
                }

                return;
            }

            if (item.isArray())
                writeFlowSequence (out, *item.getArray(), format);
            else
                writeFlowMap (out, item.getDynamicObject(), format);

            return;
        }

        writeValue (out, item, format, itemLevel);
    }

    static void writeBlockSequence (OutputStream& out, const Array<var>& array, const YAML::FormatOptions& format, int level)
    {
        for (int i = 0; i < array.size(); ++i)
        {
            if (i > 0)
                out << '\n';

            writeSpaces (out, level);
            out << "- ";
            writeSequenceItem (out, array.getReference (i), format, level + indentSize);
        }
    }

    //==============================================================================
    static void writeValue (OutputStream& out, const var& v, const YAML::FormatOptions& format, int level)
    {
        if (v.isString())
        {
            writeScalar (out, v.toString());
        }
        else if (v.isVoid() || v.isUndefined())
        {
            out << "null";
        }
        else if (v.isBool())
        {
            out << (static_cast<bool> (v) ? "true" : "false");
        }
        else if (v.isDouble())
        {
            auto d = static_cast<double> (v);

            if (yup_isfinite (d))
                out << serialiseDouble (d, format.getMaxDecimalPlaces());
            else if (d != d)
                out << ".nan";
            else
                out << (d < 0 ? "-.inf" : ".inf");
        }
        else if (v.isInt() || v.isInt64())
        {
            out << static_cast<int64> (v);
        }
        else if (v.isArray())
        {
            auto* array = v.getArray();

            if (array->isEmpty())
            {
                out << "[]";
            }
            else if (format.getSpacing() == YAML::Spacing::multiLine)
            {
                writeBlockSequence (out, *array, format, level);
            }
            else
            {
                writeFlowSequence (out, *array, format);
            }
        }
        else if (v.isObject())
        {
            if (auto* object = v.getDynamicObject())
            {
                if (object->getProperties().isEmpty())
                {
                    out << "{}";
                }
                else if (format.getSpacing() == YAML::Spacing::multiLine)
                {
                    writeBlockMap (out, object, format, level);
                }
                else
                {
                    writeFlowMap (out, object, format);
                }
            }
            else
            {
                jassertfalse; // Only DynamicObjects can be converted to YAML!
            }
        }
        else
        {
            // Can't convert these other types of object to YAML!
            jassert (! (v.isMethod() || v.isBinaryData()));

            writeScalar (out, v.toString());
        }
    }
};

//==============================================================================
void YAML::writeToStream (OutputStream& out, const var& v, const FormatOptions& opt)
{
    YAMLFormatter::writeValue (out, v, opt, 0);
}

String YAML::toString (const var& v, const FormatOptions& opt)
{
    MemoryOutputStream mo { 1024 };
    writeToStream (mo, v, opt);
    return mo.toUTF8();
}

String YAML::toString (const var& data, const bool allOnOneLine, int maximumDecimalPlaces)
{
    return toString (data, FormatOptions {}.withSpacing (allOnOneLine ? Spacing::singleLine : Spacing::multiLine).withMaxDecimalPlaces (maximumDecimalPlaces));
}

void YAML::writeToStream (OutputStream& output, const var& data, const bool allOnOneLine, int maximumDecimalPlaces)
{
    writeToStream (output, data, FormatOptions {}.withSpacing (allOnOneLine ? Spacing::singleLine : Spacing::multiLine).withMaxDecimalPlaces (maximumDecimalPlaces));
}

String YAML::escapeString (StringRef s)
{
    MemoryOutputStream mo;
    YAMLFormatter::writeEscaped (mo, s.text);
    return mo.toString();
}

//==============================================================================
/** Handles converting YAML-formatted text to var objects. */
struct YAMLParser
{
    YAMLParser (const String& text)
    {
        StringArray rawLines;
        rawLines.addLines (text);

        for (int i = 0; i < rawLines.size(); ++i)
        {
            Line line;
            line.text = rawLines[i].replace ("\r", "");
            line.number = i + 1;
            line.indent = getIndent (line.text, line.number);
            lines.add (line);
        }
    }

    struct Line
    {
        String text;
        int indent = 0;
        int number = 0;

        bool isEmptyOrComment() const
        {
            const auto content = text.trimStart();
            return content.isEmpty() || content.startsWithChar ('#');
        }
    };

    struct ErrorException
    {
        String message;
        int line = 1;

        String getDescription() const { return String (line) + ":1: error: " + message; }

        Result getResult() const { return Result::fail (getDescription()); }
    };

    struct NeedMoreTextException
    {
    };

    [[noreturn]] static void throwErrorStatic (const String& message, int line)
    {
        ErrorException e;
        e.message = message;
        e.line = line;
        throw e;
    }

    [[noreturn]] void throwError (const String& message, int line)
    {
        throwErrorStatic (message, line);
    }

    //==============================================================================
    static constexpr int maxParseDepth = 512;

    Array<Line> lines;
    HashMap<String, var> anchors;
    HashMap<String, int> expandingAliases; // Tracks aliases currently being expanded (cycle detection)
    int currentDepth = 0;
    int flowStartLine = 1;

    void enterDepth (int lineNumber)
    {
        if (++currentDepth > maxParseDepth)
            throwError ("Maximum nesting depth exceeded", lineNumber);
    }

    void leaveDepth()
    {
        --currentDepth;
    }

    //==============================================================================
    static int getIndent (const String& text, int lineNumber)
    {
        int count = 0;

        for (auto i = text.getCharPointer(); ! i.isEmpty(); ++i)
        {
            if (*i == ' ')
            {
                ++count;
                continue;
            }

            if (*i == '\t')
                throwErrorStatic ("Tabs are not allowed for indentation", lineNumber);

            break;
        }

        return count;
    }

    int skipBlankLines (int index) const
    {
        while (index < lines.size() && lines.getReference (index).isEmptyOrComment())
            ++index;

        return index;
    }

    int nextContentIndent (int index) const
    {
        index = skipBlankLines (index);
        return index < lines.size() ? lines.getReference (index).indent : -1;
    }

    static bool isDocumentMarker (const Line& line)
    {
        const auto content = line.text.substring (line.indent);
        return content == "---" || content.startsWith ("--- ")
            || content == "..." || content.startsWith ("... ");
    }

    static bool isSequenceEntry (const String& content)
    {
        return content == "-" || content.startsWith ("- ");
    }

    static String stripInlineComment (const String& text)
    {
        const int index = text.indexOf (" #");
        return index >= 0 ? text.substring (0, index) : text;
    }

    static int findKeySeparator (const String& text, bool& wasQuoted)
    {
        wasQuoted = false;

        int i = 0;
        const int len = text.length();

        while (i < len && text[i] == ' ')
            ++i;

        if (i >= len)
            return -1;

        const auto quote = text[i];

        if (quote == '"' || quote == '\'')
        {
            wasQuoted = true;
            ++i;

            while (i < len)
            {
                if (text[i] == quote)
                {
                    ++i;
                    break;
                }

                if (quote == '"' && text[i] == '\\')
                    ++i;

                ++i;
            }

            while (i < len && text[i] == ' ')
                ++i;

            return (i < len && text[i] == ':') ? i : -1;
        }

        while (i < len)
        {
            if (text[i] == ':' && (i + 1 >= len || text[i + 1] == ' '))
                return i;

            ++i;
        }

        return -1;
    }

    //==============================================================================
    static bool appendSimpleEscape (MemoryOutputStream& buffer, char c)
    {
        switch (c)
        {
            case '0':
                buffer.appendUTF8Char ('\0');
                break;
            case 'a':
                buffer.appendUTF8Char ('\a');
                break;
            case 'b':
                buffer.appendUTF8Char ('\b');
                break;
            case 't':
                buffer.appendUTF8Char ('\t');
                break;
            case 'n':
                buffer.appendUTF8Char ('\n');
                break;
            case 'v':
                buffer.appendUTF8Char ('\v');
                break;
            case 'f':
                buffer.appendUTF8Char ('\f');
                break;
            case 'r':
                buffer.appendUTF8Char ('\r');
                break;
            case 'e':
                buffer.appendUTF8Char (0x1b);
                break;
            case ' ':
                buffer.appendUTF8Char (' ');
                break;
            case '"':
                buffer.appendUTF8Char ('"');
                break;
            case '/':
                buffer.appendUTF8Char ('/');
                break;
            case '\\':
                buffer.appendUTF8Char ('\\');
                break;
            case 'N':
                buffer.appendUTF8Char (0x85);
                break;
            case '_':
                buffer.appendUTF8Char (0xa0);
                break;
            case 'L':
                buffer.appendUTF8Char (0x2028);
                break;
            case 'P':
                buffer.appendUTF8Char (0x2029);
                break;
            default:
                return false;
        }

        return true;
    }

    //==============================================================================
    static int readHexEscape (const String& text, int& index, int numDigits, int lineNumber)
    {
        int value = 0;

        for (int i = 0; i < numDigits; ++i)
        {
            if (index >= text.length())
                throwErrorStatic ("Invalid unicode escape sequence", lineNumber);

            const auto digit = CharacterFunctions::getHexDigitValue (text[index]);

            if (digit < 0)
                throwErrorStatic ("Invalid unicode escape sequence", lineNumber);

            value = (value << 4) + digit;
            ++index;
        }

        return value;
    }

    static void ensureLineEnd (const String& text, int index, int lineNumber)
    {
        while (index < text.length() && text[index] == ' ')
            ++index;

        if (index < text.length() && text[index] != '#')
            throwErrorStatic ("Unexpected content after scalar", lineNumber);
    }

    static var parseQuotedScalar (const String& text, int& index, int lineNumber)
    {
        const auto quote = text[index];
        MemoryOutputStream buffer (256);
        ++index;

        const int len = text.length();

        for (;;)
        {
            if (index >= len)
                throwErrorStatic ("Unterminated quoted string", lineNumber);

            auto c = text[index];

            if (quote == '\'' && c == '\'' && index + 1 < len && text[index + 1] == '\'')
            {
                buffer.appendUTF8Char ('\'');
                index += 2;
                continue;
            }

            if (c == quote)
            {
                ++index;
                break;
            }

            if (quote == '"' && c == '\\')
            {
                ++index;

                if (index >= len)
                    throwErrorStatic ("Unterminated escape sequence", lineNumber);

                c = text[index];
                ++index;

                if (appendSimpleEscape (buffer, (char) c))
                    continue;

                switch (c)
                {
                    case 'x':
                        buffer.appendUTF8Char ((yup_wchar) readHexEscape (text, index, 2, lineNumber));
                        break;

                    case 'u':
                    {
                        auto value = (yup_wchar) readHexEscape (text, index, 4, lineNumber);

                        // Combine a UTF-16 surrogate pair into a single code point
                        if (value >= 0xd800 && value <= 0xdbff
                            && index + 1 < len && text[index] == '\\' && text[index + 1] == 'u')
                        {
                            int lookahead = index + 2;
                            const auto low = (yup_wchar) readHexEscape (text, lookahead, 4, lineNumber);

                            if (low >= 0xdc00 && low <= 0xdfff)
                            {
                                value = 0x10000 + ((((value - 0xd800) << 10) | (low - 0xdc00)));
                                index = lookahead;
                            }
                        }

                        buffer.appendUTF8Char (value);
                        break;
                    }

                    case 'U':
                        buffer.appendUTF8Char ((yup_wchar) readHexEscape (text, index, 8, lineNumber));
                        break;
                    default:
                        throwErrorStatic ("Invalid escape sequence", lineNumber);
                }

                continue;
            }

            buffer.appendUTF8Char (c);
            ++index;
        }

        return var (buffer.toUTF8());
    }

    static String parseKeyText (const String& keyText, int lineNumber)
    {
        if (keyText.startsWithChar ('*') || keyText.startsWithChar ('&'))
            throwErrorStatic ("Aliases and anchors are not supported as map keys", lineNumber);

        if (keyText.startsWithChar ('\'') || keyText.startsWithChar ('"'))
        {
            int index = 0;
            const auto key = parseQuotedScalar (keyText, index, lineNumber);
            ensureLineEnd (keyText, index, lineNumber);
            return key.toString();
        }

        return keyText;
    }

    static String stripAnchor (String& text)
    {
        const int spaceIndex = text.indexOfChar (' ');
        String name;

        if (spaceIndex >= 0)
        {
            name = text.substring (1, spaceIndex);
            text = text.substring (spaceIndex + 1).trimStart();
        }
        else
        {
            name = text.substring (1);
            text = {};
        }

        return name;
    }

    //==============================================================================
    static var deepCopy (const var& v, int depthRemaining = maxParseDepth)
    {
        if (depthRemaining <= 0)
            return v;

        if (v.isArray())
        {
            auto result = var (Array<var>());

            for (const auto& item : *v.getArray())
                result.getArray()->add (deepCopy (item, depthRemaining - 1));

            return result;
        }

        if (v.isObject())
        {
            if (auto* object = v.getDynamicObject())
            {
                auto copy = new DynamicObject();
                var result (copy);

                for (const auto& prop : object->getProperties())
                    copy->setProperty (prop.name, deepCopy (prop.value, depthRemaining - 1));

                return result;
            }
        }

        return v;
    }

    //==============================================================================
    static bool parseIntFromDigits (const String& digits, int base, bool negative, var& result)
    {
        std::string ascii;
        ascii.reserve ((size_t) digits.length());

        for (auto i = digits.getCharPointer(); ! i.isEmpty(); ++i)
            ascii.push_back ((char) *i);

        // std::from_chars with base is C++26; use strtoull for portability.
        const char* start = ascii.data();
        char* end = nullptr;
        errno = 0;

        const auto unsignedMagnitude = std::strtoull (start, &end, base);

        if (errno != 0 || end != start + ascii.size())
            return false;

        const auto maxInt64 = static_cast<uint64> (std::numeric_limits<int64>::max());

        if (negative)
        {
            // INT64_MIN edge case: magnitude = INT64_MAX + 1 fits in uint64
            if (unsignedMagnitude == maxInt64 + 1)
            {
                result = var (std::numeric_limits<int64>::min());
                return true;
            }

            if (unsignedMagnitude > maxInt64)
                return false;

            const auto signedValue = -static_cast<int64> (unsignedMagnitude);

            if (signedValue >= std::numeric_limits<int>::min() && signedValue <= std::numeric_limits<int>::max())
                result = var ((int) signedValue);
            else
                result = var (signedValue);

            return true;
        }
        else
        {
            if (unsignedMagnitude > maxInt64)
                return false;

            const auto signedValue = static_cast<int64> (unsignedMagnitude);

            if (signedValue >= std::numeric_limits<int>::min() && signedValue <= std::numeric_limits<int>::max())
                result = var ((int) signedValue);
            else
                result = var (signedValue);

            return true;
        }
    }

    static bool tryParseNumber (const String& raw, var& result)
    {
        auto s = raw;
        bool negative = false;

        if (s.startsWithChar ('-'))
        {
            negative = true;
            s = s.substring (1);
        }
        else if (s.startsWithChar ('+'))
        {
            s = s.substring (1);
        }

        if (s.isEmpty())
            return false;

        if (s.equalsIgnoreCase (".inf"))
        {
            result = var (negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity());
            return true;
        }

        if (s.equalsIgnoreCase (".nan"))
        {
            result = var (std::numeric_limits<double>::quiet_NaN());
            return true;
        }

        if (s.length() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        {
            const auto digits = s.substring (2).removeCharacters ("_");

            if (digits.isEmpty() || ! digits.containsOnly ("0123456789abcdefABCDEF"))
                return false;

            return parseIntFromDigits (digits, 16, negative, result);
        }

        if (s.length() > 2 && s[0] == '0' && (s[1] == 'o' || s[1] == 'O'))
        {
            const auto digits = s.substring (2).removeCharacters ("_");

            if (digits.isEmpty() || ! digits.containsOnly ("01234567"))
                return false;

            return parseIntFromDigits (digits, 8, negative, result);
        }

        bool seenDigit = false;
        bool seenDot = false;
        bool seenExponent = false;
        const int len = s.length();

        for (int i = 0; i < len; ++i)
        {
            const auto c = s[i];

            if (c == '_')
                continue;

            if (c >= '0' && c <= '9')
            {
                seenDigit = true;
                continue;
            }

            if (c == '.' && ! seenDot && ! seenExponent)
            {
                seenDot = true;
                continue;
            }

            if ((c == 'e' || c == 'E') && seenDigit && ! seenExponent)
            {
                seenExponent = true;

                if (i + 1 < len && (s[i + 1] == '+' || s[i + 1] == '-'))
                    ++i;

                continue;
            }

            return false;
        }

        if (! seenDigit)
            return false;

        if (seenExponent)
        {
            int last = len - 1;

            while (last >= 0 && s[last] == '_')
                --last;

            if (last < 0 || ! (s[last] >= '0' && s[last] <= '9'))
                return false;
        }

        const auto cleaned = s.removeCharacters ("_");

        if (seenDot || seenExponent)
        {
            result = var (negative ? -cleaned.getDoubleValue() : cleaned.getDoubleValue());
            return true;
        }

        return parseIntFromDigits (cleaned, 10, negative, result);
    }

    static var resolveScalar (const String& raw)
    {
        if (raw.isEmpty())
            return {};

        if (raw == "~" || raw.equalsIgnoreCase ("null"))
            return {};

        if (raw.equalsIgnoreCase ("true")
            || raw.equalsIgnoreCase ("yes")
            || raw.equalsIgnoreCase ("on")
            || raw.equalsIgnoreCase ("y"))
            return var (true);

        if (raw.equalsIgnoreCase ("false")
            || raw.equalsIgnoreCase ("no")
            || raw.equalsIgnoreCase ("off")
            || raw.equalsIgnoreCase ("n"))
            return var (false);

        var number;

        if (tryParseNumber (raw, number))
            return number;

        return var (raw);
    }

    //==============================================================================
    var getAlias (const String& name, int lineNumber)
    {
        if (! anchors.contains (name))
            throwError ("Undefined alias '*" + name + "'", lineNumber);

        if (expandingAliases.contains (name))
            throwError ("Cyclic alias reference '*" + name + "'", lineNumber);

        expandingAliases.set (name, 1);
        auto result = deepCopy (anchors[name]);
        expandingAliases.remove (name);

        return result;
    }

    void addAnchor (const String& name, const var& value, int lineNumber)
    {
        if (anchors.contains (name))
            throwError ("Duplicate anchor '&" + name + "'", lineNumber);

        anchors.set (name, value);
    }

    void applyMerge (DynamicObject* target, const var& mergeValue, int lineNumber)
    {
        if (mergeValue.isArray())
        {
            for (const auto& item : *mergeValue.getArray())
                applyMerge (target, item, lineNumber);

            return;
        }

        if (auto* source = mergeValue.getDynamicObject())
        {
            for (const auto& prop : source->getProperties())
                if (! target->hasProperty (prop.name))
                    target->setProperty (prop.name, deepCopy (prop.value));

            return;
        }

        throwError ("Merge key '<<' requires a mapping", lineNumber);
    }

    //==============================================================================
    var parseNestedBlockValue (int entryIndent, int& lineIndex, int lineNumber)
    {
        const int nestedIndent = nextContentIndent (lineIndex + 1);

        if (nestedIndent <= entryIndent)
            return {};

        lineIndex = skipBlankLines (lineIndex + 1);

        if (lineIndex >= lines.size())
            return {};

        const auto& nestedLine = lines.getReference (lineIndex);
        const auto nestedContent = nestedLine.text.substring (nestedIndent);
        bool wasQuoted = false;

        if (nestedContent.startsWithChar ('{') || nestedContent.startsWithChar ('[')
            || isSequenceEntry (nestedContent) || findKeySeparator (nestedContent, wasQuoted) >= 0)
            return parseBlockNode (nestedIndent, lineIndex);

        const int nestedStartLine = lineIndex;
        auto value = parseMapValue (nestedContent, nestedIndent, nestedLine.number, lineIndex);

        if (lineIndex == nestedStartLine)
            ++lineIndex;

        return value;
    }

    var parseBlockNode (int indent, int& lineIndex)
    {
        lineIndex = skipBlankLines (lineIndex);

        if (lineIndex >= lines.size())
            return {};

        const auto& line = lines.getReference (lineIndex);

        if (isDocumentMarker (line))
            throwError ("Document markers are not supported", line.number);

        if (line.indent < indent)
            return {};

        if (line.indent > indent)
            throwError ("Unexpected indentation", line.number);

        enterDepth (line.number);

        const auto content = line.text.substring (indent);
        var result;

        if (content.startsWithChar ('{') || content.startsWithChar ('['))
            result = parseFlowValue (content, lineIndex, line.number);
        else if (isSequenceEntry (content))
            result = parseBlockSequence (indent, lineIndex);
        else
            result = parseBlockMap (indent, lineIndex);

        leaveDepth();
        return result;
    }

    var parseBlockMap (int indent, int& lineIndex)
    {
        auto resultObject = new DynamicObject();
        var result (resultObject);
        parseBlockMapEntries (resultObject, indent, lineIndex);
        return result;
    }

    void parseBlockMapEntries (DynamicObject* map, int indent, int& lineIndex)
    {
        for (;;)
        {
            lineIndex = skipBlankLines (lineIndex);

            if (lineIndex >= lines.size())
                return;

            const auto& line = lines.getReference (lineIndex);

            if (isDocumentMarker (line))
                throwError ("Document markers are not supported", line.number);

            if (line.indent < indent)
                return;

            if (line.indent > indent)
                throwError ("Unexpected indentation in mapping", line.number);

            const auto content = line.text.substring (indent);

            if (isSequenceEntry (content) || content.startsWithChar ('{') || content.startsWithChar ('['))
                return;

            bool wasQuoted = false;

            if (findKeySeparator (content, wasQuoted) < 0)
                return;

            parseMapEntryFromText (map, content, indent, line.number, lineIndex);
        }
    }

    void parseMapEntryFromText (DynamicObject* map, const String& entryText, int entryIndent, int lineNumber, int& lineIndex)
    {
        const int entryLine = lineIndex;
        bool keyWasQuoted = false;
        const int separatorIndex = findKeySeparator (entryText, keyWasQuoted);

        if (separatorIndex < 0)
            throwError ("Expected ':' after map key", lineNumber);

        const auto key = parseKeyText (entryText.substring (0, separatorIndex).trimEnd(), lineNumber);

        if (key.isEmpty())
            throwError ("Invalid map key", lineNumber);

        auto rest = entryText.substring (separatorIndex + 1);
        int valueIndex = 0;

        while (valueIndex < rest.length() && rest[valueIndex] == ' ')
            ++valueIndex;

        const auto valueText = rest.substring (valueIndex).trimEnd();
        const bool isMergeKey = (! keyWasQuoted) && (key == "<<");
        const auto value = parseMapValue (valueText, entryIndent, lineNumber, lineIndex);

        if (isMergeKey)
            applyMerge (map, value, lineNumber);
        else
            map->setProperty (Identifier (key), value);

        if (lineIndex == entryLine)
            ++lineIndex;
    }

    void parseBlockSequenceEntries (Array<var>* destArray, int indent, int& lineIndex)
    {
        for (;;)
        {
            lineIndex = skipBlankLines (lineIndex);

            if (lineIndex >= lines.size())
                return;

            const auto& line = lines.getReference (lineIndex);

            if (isDocumentMarker (line))
                throwError ("Document markers are not supported", line.number);

            if (line.indent < indent)
                return;

            if (line.indent > indent)
                throwError ("Unexpected indentation in sequence", line.number);

            const auto content = line.text.substring (indent);

            if (! isSequenceEntry (content))
                return;

            const auto itemText = content == "-" ? String() : content.substring (2);
            destArray->add (parseSequenceItem (itemText, indent, line.number, lineIndex));
        }
    }

    var parseBlockSequence (int indent, int& lineIndex)
    {
        auto result = var (Array<var>());
        parseBlockSequenceEntries (result.getArray(), indent, lineIndex);
        return result;
    }

    var parseSequenceItem (const String& itemText, int indent, int lineNumber, int& lineIndex)
    {
        const int itemLine = lineIndex;
        auto rest = itemText.trimEnd();
        String anchorName;

        if (rest.startsWithChar ('&'))
        {
            anchorName = stripAnchor (rest);
            rest = rest.trimEnd();
        }

        const auto item = parseSequenceItemContent (rest, indent, lineNumber, lineIndex);

        if (anchorName.isNotEmpty())
            addAnchor (anchorName, deepCopy (item), lineNumber);

        if (lineIndex == itemLine)
            ++lineIndex;

        return item;
    }

    var parseSequenceItemContent (const String& itemText, int indent, int lineNumber, int& lineIndex)
    {
        const auto rest = itemText;

        if (rest.isEmpty())
            return parseNestedBlockValue (indent, lineIndex, lineNumber);

        if (rest.startsWithChar ('#'))
            return {};

        if (rest.startsWithChar ('&'))
        {
            auto anchoredRest = rest;
            const auto anchorName = stripAnchor (anchoredRest);

            var anchored;

            if (anchoredRest.isEmpty())
                anchored = parseNestedBlockValue (indent, lineIndex, lineNumber);
            else
                anchored = parseSequenceItemContent (anchoredRest, indent, lineNumber, lineIndex);

            addAnchor (anchorName, deepCopy (anchored), lineNumber);
            return anchored;
        }

        if (rest.startsWithChar ('*'))
            return getAlias (rest.substring (1).trim(), lineNumber);

        if (rest.startsWithChar ('{') || rest.startsWithChar ('['))
            return parseFlowValue (rest, lineIndex, lineNumber);

        if (rest.startsWithChar ('|') || rest.startsWithChar ('>'))
            return parseBlockScalar (rest, lineNumber, indent, lineIndex);

        if (isSequenceEntry (rest))
        {
            auto result = var (Array<var>());
            auto destArray = result.getArray();
            const auto firstItemText = rest == "-" ? String() : rest.substring (2);
            destArray->add (parseSequenceItem (firstItemText, indent + YAMLFormatter::indentSize, lineNumber, lineIndex));

            const int continuationIndent = nextContentIndent (lineIndex);

            if (continuationIndent > indent)
                parseBlockSequenceEntries (destArray, continuationIndent, lineIndex);

            return result;
        }

        bool wasQuoted = false;

        if (findKeySeparator (rest, wasQuoted) >= 0)
        {
            auto map = new DynamicObject();
            var result (map);

            parseMapEntryFromText (map, rest, indent + YAMLFormatter::indentSize, lineNumber, lineIndex);

            const int continuationIndent = nextContentIndent (lineIndex);

            if (continuationIndent > indent)
                parseBlockMapEntries (map, continuationIndent, lineIndex);

            return result;
        }

        if (rest.startsWithChar ('\'') || rest.startsWithChar ('"'))
        {
            int index = 0;
            auto value = parseQuotedScalar (rest, index, lineNumber);
            ensureLineEnd (rest, index, lineNumber);
            return value;
        }

        if (rest.contains (": "))
            throwError ("Plain scalars cannot contain ': '", lineNumber);

        return resolveScalar (stripInlineComment (rest).trimEnd());
    }

    //==============================================================================
    var parseBlockScalar (const String& headerText, int lineNumber, int entryIndent, int& lineIndex)
    {
        const auto indicator = headerText[0];
        const auto header = stripInlineComment (headerText).trimEnd();

        bool stripChomping = false;
        bool keepChomping = false;
        int explicitIndent = 0;

        for (int i = 1; i < header.length(); ++i)
        {
            const auto c = header[i];

            if (c == '-')
                stripChomping = true;
            else if (c == '+')
                keepChomping = true;
            else if (c >= '1' && c <= '9')
                explicitIndent = explicitIndent * 10 + (c - '0');
            else
                throwError ("Invalid block scalar header", lineNumber);
        }

        StringArray contentLines;
        Array<bool> moreIndentedFlags;
        int detectedIndent = explicitIndent > 0 ? entryIndent + explicitIndent : -1;

        while (lineIndex + 1 < lines.size())
        {
            const auto& nextLine = lines.getReference (lineIndex + 1);

            if (nextLine.text.trim().isEmpty())
            {
                contentLines.add (String());
                moreIndentedFlags.add (false);
                ++lineIndex;
                continue;
            }

            if (detectedIndent < 0)
            {
                detectedIndent = nextLine.indent;

                if (detectedIndent <= entryIndent)
                {
                    detectedIndent = entryIndent + 1;
                    break;
                }
            }

            if (nextLine.indent < detectedIndent)
                break;

            contentLines.add (indicator == '|' ? nextLine.text.substring (detectedIndent)
                                               : nextLine.text.substring (detectedIndent).trimEnd());
            moreIndentedFlags.add (nextLine.indent > detectedIndent);
            ++lineIndex;
        }

        ++lineIndex; // leave lineIndex pointing at the first line after the block scalar

        String result;

        if (indicator == '|')
        {
            result = contentLines.joinIntoString ("\n");
        }
        else
        {
            // Folded style: single line breaks fold to spaces, but empty lines and
            // more-indented lines are preserved as literal line breaks.
            for (int i = 0; i < contentLines.size(); ++i)
            {
                if (i > 0 && ! contentLines[i].isEmpty())
                {
                    int blankCount = 0;

                    for (int j = i - 1; j >= 0 && contentLines[j].isEmpty(); --j)
                        ++blankCount;

                    if (blankCount > 0)
                    {
                        for (int k = 0; k < blankCount; ++k)
                            result << "\n";
                    }
                    else if (moreIndentedFlags[i] || moreIndentedFlags[i - 1])
                    {
                        result << "\n";
                    }
                    else
                    {
                        result << " ";
                    }
                }

                if (! contentLines[i].isEmpty())
                    result << contentLines[i];
            }
        }

        if (stripChomping)
        {
            while (result.endsWithChar ('\n'))
                result = result.dropLastCharacters (1);
        }
        else if (! keepChomping)
        {
            while (result.endsWithChar ('\n'))
                result = result.dropLastCharacters (1);

            if (! result.isEmpty())
                result << "\n";
        }

        return var (result);
    }

    //==============================================================================
    struct FlowCursor
    {
        const String& text;
        int index = 0;

        explicit FlowCursor (const String& t)
            : text (t)
        {
        }

        yup_wchar peek() const { return index < text.length() ? text[index] : (yup_wchar) 0; }

        yup_wchar peekAt (int offset) const
        {
            const auto i = index + offset;
            return (i >= 0 && i < text.length()) ? text[i] : (yup_wchar) 0;
        }

        yup_wchar get() { return index < text.length() ? text[index++] : (yup_wchar) 0; }

        bool eof() const { return index >= text.length(); }

        void skipSpaces()
        {
            while (peek() == ' ' || peek() == '\t' || peek() == '\n')
                ++index;
        }
    };

    [[noreturn]] void throwNeedMore()
    {
        throw NeedMoreTextException();
    }

    static void skipComment (FlowCursor& cursor)
    {
        while (! cursor.eof() && cursor.peek() != '\n')
            cursor.get();
    }

    String scanPlainFlowValue (FlowCursor& cursor)
    {
        MemoryOutputStream buffer;
        bool previousWasWhitespace = true;

        for (;;)
        {
            const auto c = cursor.peek();

            if (c == 0 || c == ',' || c == '}' || c == ']')
                break;

            if (c == '#' && previousWasWhitespace)
                break;

            if (c == ':')
            {
                const auto next = cursor.peekAt (1);

                if (next == 0 || next == ' ' || next == '\t' || next == '\n'
                    || next == ',' || next == '}' || next == ']')
                    break;
            }

            buffer.appendUTF8Char (c);
            cursor.get();
            previousWasWhitespace = (c == ' ' || c == '\t' || c == '\n');
        }

        return buffer.toUTF8().trimEnd();
    }

    String parseFlowKey (FlowCursor& cursor, bool& wasQuoted)
    {
        for (;;)
        {
            cursor.skipSpaces();

            if (cursor.eof())
                throwNeedMore();

            if (cursor.peek() == '#')
            {
                skipComment (cursor);
                continue;
            }

            break;
        }

        const auto c = cursor.peek();

        if (c == '"' || c == '\'')
        {
            wasQuoted = true;
            return parseFlowQuoted (cursor);
        }

        wasQuoted = false;

        MemoryOutputStream buffer;

        for (;;)
        {
            const auto ch = cursor.peek();

            if (ch == 0 || ch == ':' || ch == ',' || ch == '}' || ch == ']')
                break;

            buffer.appendUTF8Char (ch);
            cursor.get();
        }

        return buffer.toUTF8().trimEnd();
    }

    int readFlowHexEscape (FlowCursor& cursor, int numDigits)
    {
        int value = 0;

        for (int i = 0; i < numDigits; ++i)
        {
            if (cursor.eof())
                throwNeedMore();

            const auto digit = CharacterFunctions::getHexDigitValue (cursor.get());

            if (digit < 0)
                throwError ("Invalid unicode escape sequence", flowStartLine);

            value = (value << 4) + digit;
        }

        return value;
    }

    String parseFlowQuoted (FlowCursor& cursor)
    {
        const auto quote = cursor.get();
        MemoryOutputStream buffer (256);

        for (;;)
        {
            if (cursor.eof())
                throwNeedMore();

            auto c = cursor.get();

            if (quote == '\'' && c == '\'' && cursor.peek() == '\'')
            {
                cursor.get();
                buffer.appendUTF8Char ('\'');
                continue;
            }

            if (c == quote)
                break;

            if (quote == '"' && c == '\\')
            {
                if (cursor.eof())
                    throwNeedMore();

                c = cursor.get();

                if (appendSimpleEscape (buffer, (char) c))
                    continue;

                switch (c)
                {
                    case 'x':
                        buffer.appendUTF8Char ((yup_wchar) readFlowHexEscape (cursor, 2));
                        break;

                    case 'u':
                    {
                        auto value = (yup_wchar) readFlowHexEscape (cursor, 4);

                        // Combine a UTF-16 surrogate pair into a single code point
                        if (value >= 0xd800 && value <= 0xdbff
                            && cursor.peek() == '\\' && cursor.peekAt (1) == 'u')
                        {
                            const int savedIndex = cursor.index;
                            cursor.get();
                            cursor.get();
                            const auto low = (yup_wchar) readFlowHexEscape (cursor, 4);

                            if (low >= 0xdc00 && low <= 0xdfff)
                                value = 0x10000 + ((((value - 0xd800) << 10) | (low - 0xdc00)));
                            else
                                cursor.index = savedIndex;
                        }

                        buffer.appendUTF8Char (value);
                        break;
                    }

                    case 'U':
                        buffer.appendUTF8Char ((yup_wchar) readFlowHexEscape (cursor, 8));
                        break;
                    default:
                        throwError ("Invalid escape sequence", flowStartLine);
                }

                continue;
            }

            buffer.appendUTF8Char (c);
        }

        return buffer.toUTF8();
    }

    var parseFlowValueNode (FlowCursor& cursor)
    {
        for (;;)
        {
            if (cursor.eof())
                throwNeedMore();

            if (cursor.peek() == '#')
            {
                skipComment (cursor);
                cursor.skipSpaces();
                continue;
            }

            break;
        }

        const auto c = cursor.peek();

        if (c == '{')
            return parseFlowMap (cursor);

        if (c == '[')
            return parseFlowSequence (cursor);

        if (c == '"' || c == '\'')
            return parseFlowQuoted (cursor);

        if (c == '*')
        {
            cursor.get();
            return getAlias (scanPlainFlowValue (cursor).trim(), flowStartLine);
        }

        if (c == '&')
        {
            cursor.get();
            const auto name = scanPlainFlowValue (cursor).trim();

            if (name.isEmpty())
                throwError ("Missing anchor name", flowStartLine);

            const auto anchored = parseFlowValueNode (cursor);
            addAnchor (name, deepCopy (anchored), flowStartLine);
            return anchored;
        }

        const auto text = scanPlainFlowValue (cursor);

        if (text.isEmpty())
            throwError ("Expected a value in flow collection", flowStartLine);

        return resolveScalar (text);
    }

    var parseFlowMap (FlowCursor& cursor)
    {
        enterDepth (flowStartLine);

        auto resultObject = new DynamicObject();
        var result (resultObject);
        cursor.get(); // '{'

        for (;;)
        {
            cursor.skipSpaces();

            if (cursor.eof())
                throwNeedMore();

            if (cursor.peek() == '}')
            {
                cursor.get();
                leaveDepth();
                return result;
            }

            bool keyWasQuoted = false;
            const auto key = parseFlowKey (cursor, keyWasQuoted);

            if (key.isEmpty())
                throwError ("Invalid map key", flowStartLine);

            cursor.skipSpaces();

            if (cursor.peek() != ':')
                throwError ("Expected ':' in flow mapping", flowStartLine);

            cursor.get();
            cursor.skipSpaces();

            const auto value = parseFlowValueNode (cursor);

            if (! keyWasQuoted && key == "<<")
                applyMerge (resultObject, value, flowStartLine);
            else
                resultObject->setProperty (Identifier (key), value);

            cursor.skipSpaces();

            if (cursor.eof())
                throwNeedMore();

            if (cursor.peek() == '#')
                skipComment (cursor);

            cursor.skipSpaces();

            if (cursor.eof())
                throwNeedMore();

            if (cursor.peek() == ',')
            {
                cursor.get();
                continue;
            }

            if (cursor.peek() == '}')
            {
                cursor.get();
                leaveDepth();
                return result;
            }

            throwError ("Expected ',' or '}' in flow mapping", flowStartLine);
        }
    }

    var parseFlowSequence (FlowCursor& cursor)
    {
        enterDepth (flowStartLine);

        auto result = var (Array<var>());
        auto destArray = result.getArray();
        cursor.get(); // '['

        for (;;)
        {
            cursor.skipSpaces();

            if (cursor.eof())
                throwNeedMore();

            if (cursor.peek() == ']')
            {
                cursor.get();
                leaveDepth();
                return result;
            }

            destArray->add (parseFlowValueNode (cursor));
            cursor.skipSpaces();

            if (cursor.eof())
                throwNeedMore();

            if (cursor.peek() == '#')
                skipComment (cursor);

            cursor.skipSpaces();

            if (cursor.eof())
                throwNeedMore();

            if (cursor.peek() == ',')
            {
                cursor.get();
                continue;
            }

            if (cursor.peek() == ']')
            {
                cursor.get();
                leaveDepth();
                return result;
            }

            throwError ("Expected ',' or ']' in flow sequence", flowStartLine);
        }
    }

    var parseFlowValue (String text, int& lineIndex, int lineNumber)
    {
        flowStartLine = lineNumber;

        for (;;)
        {
            var result;
            int endIndex = 0;

            try
            {
                FlowCursor cursor (text);
                const auto c = cursor.peek();

                if (c == '{')
                    result = parseFlowMap (cursor);
                else if (c == '[')
                    result = parseFlowSequence (cursor);
                else
                    throwError ("Expected a flow mapping or sequence", lineNumber);

                endIndex = cursor.index;
            }
            catch (const NeedMoreTextException&)
            {
                if (lineIndex + 1 >= lines.size())
                    throwError ("Unterminated flow collection", lineNumber);

                ++lineIndex;
                const auto& nextLine = lines.getReference (lineIndex);
                text << "\n"
                     << nextLine.text.substring (nextLine.indent);
                continue;
            }

            const int newlinePos = text.indexOfChar ('\n', endIndex);
            const auto tail = newlinePos >= 0 ? text.substring (endIndex, newlinePos) : text.substring (endIndex);

            if (! isLineEnd (tail))
                throwError ("Unexpected content after flow collection", lineNumber);

            ++lineIndex;
            return result;
        }
    }

    static bool isLineEnd (const String& tail)
    {
        const auto trimmed = tail.trimStart();
        return trimmed.isEmpty() || trimmed.startsWithChar ('#');
    }

    //==============================================================================
    var parseDocument (bool requireContainer)
    {
        const int firstLine = skipBlankLines (0);

        if (firstLine >= lines.size())
        {
            if (requireContainer)
                throwError ("Expected a mapping or sequence at the start of the document", 1);

            return {};
        }

        const auto& first = lines.getReference (firstLine);

        if (isDocumentMarker (first))
            throwError ("Document markers are not supported", first.number);

        if (first.indent != 0)
            throwError ("Unexpected indentation at the start of the document", first.number);

        var result;
        int lineIndex = firstLine;

        const auto content = first.text.substring (first.indent);
        bool wasQuoted = false;

        if (content.startsWithChar ('{') || content.startsWithChar ('['))
        {
            result = parseMapValue (content, 0, first.number, lineIndex);
        }
        else if (isSequenceEntry (content) || findKeySeparator (content, wasQuoted) >= 0)
        {
            result = parseBlockNode (0, lineIndex);
        }
        else if (! requireContainer)
        {
            const int resultLine = lineIndex;
            result = parseMapValue (content, 0, first.number, lineIndex);

            if (lineIndex == resultLine)
                ++lineIndex;
        }
        else
        {
            throwError ("Expected a mapping or sequence at the start of the document", first.number);
        }

        const int rest = skipBlankLines (lineIndex);

        if (rest < lines.size())
            throwError ("Unexpected content after the end of the document", lines.getReference (rest).number);

        return result;
    }

    //==============================================================================
    var parseMapValue (const String& valueText, int entryIndent, int lineNumber, int& lineIndex)
    {
        if (valueText.isEmpty())
            return parseNestedBlockValue (entryIndent, lineIndex, lineNumber);

        if (valueText.startsWithChar ('#'))
            return {};

        if (valueText.startsWithChar ('&'))
        {
            auto rest = valueText;
            const auto anchorName = stripAnchor (rest);

            var anchored;

            if (rest.isEmpty())
                anchored = parseNestedBlockValue (entryIndent, lineIndex, lineNumber);
            else
                anchored = parseMapValue (rest, entryIndent, lineNumber, lineIndex);

            addAnchor (anchorName, deepCopy (anchored), lineNumber);
            return anchored;
        }

        if (valueText.startsWithChar ('*'))
            return getAlias (valueText.substring (1).trim(), lineNumber);

        if (valueText.startsWithChar ('|') || valueText.startsWithChar ('>'))
            return parseBlockScalar (valueText, lineNumber, entryIndent, lineIndex);

        if (valueText.startsWithChar ('{') || valueText.startsWithChar ('['))
            return parseFlowValue (valueText, lineIndex, lineNumber);

        if (valueText.startsWithChar ('\'') || valueText.startsWithChar ('"'))
        {
            int index = 0;
            auto value = parseQuotedScalar (valueText, index, lineNumber);
            ensureLineEnd (valueText, index, lineNumber);
            return value;
        }

        if (valueText.contains (": "))
            throwError ("Plain scalars cannot contain ': '", lineNumber);

        return resolveScalar (stripInlineComment (valueText).trimEnd());
    }
};

//==============================================================================
Result YAML::parse (const String& text, var& result)
{
    try
    {
        result = YAMLParser (text).parseDocument (true);
    }
    catch (const YAMLParser::ErrorException& error)
    {
        return error.getResult();
    }

    return Result::ok();
}

var YAML::parse (const String& text)
{
    var result;

    if (parse (text, result))
        return result;

    return {};
}

var YAML::parse (InputStream& input)
{
    return parse (input.readEntireStreamAsString());
}

var YAML::parse (const File& file)
{
    return parse (file.loadFileAsString());
}

var YAML::fromString (StringRef text)
{
    try
    {
        return YAMLParser (text.text).parseDocument (false);
    }
    catch (const YAMLParser::ErrorException&)
    {
    }

    return {};
}

Result YAML::parseQuotedString (String::CharPointerType& t, var& result)
{
    const auto quote = t.getAndAdvance();

    if (quote != '"' && quote != '\'')
        return Result::fail ("Not a quoted string!");

    MemoryOutputStream buffer;

    for (;;)
    {
        auto c = t.getAndAdvance();

        if (c == 0)
            return Result::fail ("Unexpected EOF in quoted string!");

        if (quote == '\'' && c == '\'' && *t == '\'')
        {
            ++t;
            buffer.appendUTF8Char ('\'');
            continue;
        }

        if (c == quote)
            break;

        if (quote == '"' && c == '\\')
        {
            c = t.getAndAdvance();

            if (YAMLParser::appendSimpleEscape (buffer, (char) c))
                continue;

            switch (c)
            {
                case 'x':
                {
                    yup_wchar value = 0;

                    for (int i = 0; i < 2; ++i)
                    {
                        const auto digit = CharacterFunctions::getHexDigitValue (t.getAndAdvance());
                        if (digit < 0)
                            return Result::fail ("Invalid unicode escape sequence!");

                        value = (yup_wchar) ((value << 4) + digit);
                    }

                    buffer.appendUTF8Char (value);
                    break;
                }

                case 'u':
                {
                    yup_wchar value = 0;

                    for (int i = 0; i < 4; ++i)
                    {
                        const auto digit = CharacterFunctions::getHexDigitValue (t.getAndAdvance());
                        if (digit < 0)
                            return Result::fail ("Invalid unicode escape sequence!");

                        value = (yup_wchar) ((value << 4) + digit);
                    }

                    // Combine a UTF-16 surrogate pair into a single code point
                    if (value >= 0xd800 && value <= 0xdbff && *t == '\\')
                    {
                        auto afterBackslash = t;
                        ++afterBackslash;

                        if (*afterBackslash == 'u')
                        {
                            auto savedT = t;
                            t = afterBackslash;
                            t.getAndAdvance();

                            yup_wchar low = 0;

                            for (int i = 0; i < 4; ++i)
                            {
                                const auto digit = CharacterFunctions::getHexDigitValue (t.getAndAdvance());
                                if (digit < 0)
                                    return Result::fail ("Invalid unicode escape sequence!");

                                low = (yup_wchar) ((low << 4) + digit);
                            }

                            if (low >= 0xdc00 && low <= 0xdfff)
                                value = 0x10000 + ((((value - 0xd800) << 10) | (low - 0xdc00)));
                            else
                                t = savedT;
                        }
                    }

                    buffer.appendUTF8Char (value);
                    break;
                }

                case 'U':
                {
                    yup_wchar value = 0;

                    for (int i = 0; i < 8; ++i)
                    {
                        const auto digit = CharacterFunctions::getHexDigitValue (t.getAndAdvance());
                        if (digit < 0)
                            return Result::fail ("Invalid unicode escape sequence!");

                        value = (yup_wchar) ((value << 4) + digit);
                    }

                    buffer.appendUTF8Char (value);
                    break;
                }

                default:
                    return Result::fail ("Invalid escape sequence!");
            }

            continue;
        }

        buffer.appendUTF8Char (c);
    }

    result = buffer.toUTF8();
    return Result::ok();
}

} // namespace yup
