/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

//==============================================================================
/**
    Wraps a pointer to a null-terminated UTF-8 character string, and provides
    various methods to operate on the data.
    @see CharPointer_UTF16, CharPointer_UTF32

    @tags{Core}
*/
class CharPointer_UTF8 final
{
public:
    using CharType = char;

    explicit CharPointer_UTF8 (const CharType* rawPointer) noexcept
        : data (const_cast<CharType*> (rawPointer))
    {
    }

    CharPointer_UTF8 (const CharPointer_UTF8& other) = default;

    CharPointer_UTF8 operator= (CharPointer_UTF8 other) noexcept
    {
        data = other.data;
        return *this;
    }

    CharPointer_UTF8 operator= (const CharType* text) noexcept
    {
        data = const_cast<CharType*> (text);
        return *this;
    }

    /** This is a pointer comparison, it doesn't compare the actual text. */
    bool operator== (CharPointer_UTF8 other) const noexcept { return data == other.data; }

    bool operator!= (CharPointer_UTF8 other) const noexcept { return data != other.data; }

    bool operator<= (CharPointer_UTF8 other) const noexcept { return data <= other.data; }

    bool operator< (CharPointer_UTF8 other) const noexcept { return data < other.data; }

    bool operator>= (CharPointer_UTF8 other) const noexcept { return data >= other.data; }

    bool operator> (CharPointer_UTF8 other) const noexcept { return data > other.data; }

    /** Returns the address that this pointer is pointing to. */
    CharType* getAddress() const noexcept { return data; }

    /** Returns the address that this pointer is pointing to. */
    operator const CharType*() const noexcept { return data; }

    /** Returns true if this pointer is pointing to a null character. */
    bool isEmpty() const noexcept { return *data == 0; }

    /** Returns true if this pointer is not pointing to a null character. */
    bool isNotEmpty() const noexcept { return *data != 0; }

    /** Returns the unicode character that this pointer is pointing to. */
    yup_wchar operator*() const noexcept
    {
        int numBytes;
        return decodeCharacter (data, numBytes);
    }

    /** Moves this pointer along to the next character in the string. */
    CharPointer_UTF8& operator++() noexcept
    {
        jassert (*data != 0); // trying to advance past the end of the string?
        int numBytes;
        decodeCharacter (data, numBytes);
        data += numBytes;
        return *this;
    }

    /** Moves this pointer back to the previous character in the string. */
    CharPointer_UTF8 operator--() noexcept
    {
        auto* lead = data - 1;

        for (int count = 0; count < 3 && (static_cast<uint8> (*lead) & 0xc0) == 0x80; ++count)
            --lead;

        int numBytes;
        decodeCharacter (lead, numBytes);

        data = lead + numBytes == data ? lead : data - 1;
        return *this;
    }

    /** Returns the character that this pointer is currently pointing to, and then
        advances the pointer to point to the next character. */
    yup_wchar getAndAdvance() noexcept
    {
        int numBytes;
        auto character = decodeCharacter (data, numBytes);

        if (character != 0)
            data += numBytes;

        return character;
    }

    /** Returns the character at this position and advances past it, without reading
        more than maxBytes bytes. A sequence cut off by the bound is a replacement
        character. The bound must be at least 1.
    */
    yup_wchar getAndAdvance (int maxBytes) noexcept
    {
        int numBytes;
        auto character = decodeCharacter (data, numBytes, maxBytes);

        if (character != 0)
            data += numBytes;

        return character;
    }

    /** Moves this pointer along to the next character in the string. */
    CharPointer_UTF8 operator++ (int) noexcept
    {
        CharPointer_UTF8 temp (*this);
        ++*this;
        return temp;
    }

    /** Moves this pointer forwards by the specified number of characters. */
    void operator+= (int numToSkip) noexcept
    {
        if (numToSkip < 0)
        {
            while (++numToSkip <= 0)
                --*this;
        }
        else
        {
            while (--numToSkip >= 0)
                ++*this;
        }
    }

    /** Moves this pointer backwards by the specified number of characters. */
    void operator-= (int numToSkip) noexcept
    {
        operator+= (-numToSkip);
    }

    /** Returns the character at a given character index from the start of the string. */
    yup_wchar operator[] (int characterIndex) const noexcept
    {
        auto p (*this);
        p += characterIndex;
        return *p;
    }

    /** Returns a pointer which is moved forwards from this one by the specified number of characters. */
    CharPointer_UTF8 operator+ (int numToSkip) const noexcept
    {
        auto p (*this);
        p += numToSkip;
        return p;
    }

    /** Returns a pointer which is moved backwards from this one by the specified number of characters. */
    CharPointer_UTF8 operator- (int numToSkip) const noexcept
    {
        auto p (*this);
        p += -numToSkip;
        return p;
    }

    /** Returns the number of characters in this string. */
    size_t length() const noexcept
    {
        auto* d = data;
        size_t count = 0;

        while (*d != 0)
        {
            int numBytes;
            decodeCharacter (d, numBytes);
            d += numBytes;
            ++count;
        }

        return count;
    }

    /** Returns the number of characters in this string, or the given value, whichever is lower. */
    size_t lengthUpTo (const size_t maxCharsToCount) const noexcept
    {
        return CharacterFunctions::lengthUpTo (*this, maxCharsToCount);
    }

    /** Returns the number of characters in this string, or up to the given end pointer, whichever is lower. */
    size_t lengthUpTo (const CharPointer_UTF8 end) const noexcept
    {
        return CharacterFunctions::lengthUpTo (*this, end);
    }

    /** Returns the number of bytes that are used to represent this string.
        This includes the terminating null character.
    */
    size_t sizeInBytes() const noexcept
    {
        YUP_BEGIN_IGNORE_WARNINGS_MSVC (6387)
        jassert (data != nullptr);
        return strlen (data) + 1;
        YUP_END_IGNORE_WARNINGS_MSVC
    }

    /** Returns the number of bytes that would be needed to represent the given
        unicode character in this encoding format.
    */
    static size_t getBytesRequiredFor (const yup_wchar charToWrite) noexcept
    {
        size_t num = 1;
        auto c = (uint32) charToWrite;

        if (c >= 0x80)
        {
            ++num;
            if (c >= 0x800)
            {
                ++num;
                if (c >= 0x10000)
                    ++num;
            }
        }

        return num;
    }

    /** Returns the number of bytes that would be needed to represent the given
        string in this encoding format.
        The value returned does NOT include the terminating null character.
    */
    template <class CharPointer>
    static size_t getBytesRequiredFor (CharPointer text) noexcept
    {
        size_t count = 0;

        while (auto n = text.getAndAdvance())
            count += getBytesRequiredFor (n);

        return count;
    }

    /** Returns a pointer to the null character that terminates this string. */
    CharPointer_UTF8 findTerminatingNull() const noexcept
    {
        return CharPointer_UTF8 (data + strlen (data));
    }

    /** Writes a unicode character to this string, and advances this pointer to point to the next position. */
    void write (const yup_wchar charToWrite) noexcept
    {
        auto c = (uint32) charToWrite;

        if (c >= 0x80)
        {
            int numExtraBytes = 1;
            if (c >= 0x800)
            {
                ++numExtraBytes;
                if (c >= 0x10000)
                    ++numExtraBytes;
            }

            *data++ = (CharType) ((uint32) (0xff << (7 - numExtraBytes)) | (c >> (numExtraBytes * 6)));

            while (--numExtraBytes >= 0)
                *data++ = (CharType) (0x80 | (0x3f & (c >> (numExtraBytes * 6))));
        }
        else
        {
            *data++ = (CharType) c;
        }
    }

    /** Writes a null character to this string (leaving the pointer's position unchanged). */
    void writeNull() const noexcept
    {
        *data = 0;
    }

    /** Copies a source string to this pointer, advancing this pointer as it goes. */
    template <typename CharPointer>
    void writeAll (const CharPointer src) noexcept
    {
        CharacterFunctions::copyAll (*this, src);
    }

    /** Copies a source string to this pointer, advancing this pointer as it goes.
        The maxDestBytes parameter specifies the maximum number of bytes that can be written
        to the destination buffer before stopping.
    */
    template <typename CharPointer>
    size_t writeWithDestByteLimit (const CharPointer src, const size_t maxDestBytes) noexcept
    {
        return CharacterFunctions::copyWithDestByteLimit (*this, src, maxDestBytes);
    }

    /** Copies a source string to this pointer, advancing this pointer as it goes.
        The maxChars parameter specifies the maximum number of characters that can be
        written to the destination buffer before stopping (including the terminating null).
    */
    template <typename CharPointer>
    void writeWithCharLimit (const CharPointer src, const int maxChars) noexcept
    {
        CharacterFunctions::copyWithCharLimit (*this, src, maxChars);
    }

    /** Compares this string with another one. */
    template <typename CharPointer>
    int compare (const CharPointer other) const noexcept
    {
        return CharacterFunctions::compare (*this, other);
    }

    /** Compares this string with another one, up to a specified number of characters. */
    template <typename CharPointer>
    int compareUpTo (const CharPointer other, const int maxChars) const noexcept
    {
        return CharacterFunctions::compareUpTo (*this, other, maxChars);
    }

    /** Compares this string with another one. */
    template <typename CharPointer>
    int compareIgnoreCase (const CharPointer other) const noexcept
    {
        return CharacterFunctions::compareIgnoreCase (*this, other);
    }

    /** Compares this string with another one. */
    int compareIgnoreCase (const CharPointer_UTF8 other) const noexcept
    {
        return CharacterFunctions::compareIgnoreCase (*this, other);
    }

    /** Compares this string with another one, up to a specified number of characters. */
    template <typename CharPointer>
    int compareIgnoreCaseUpTo (const CharPointer other, const int maxChars) const noexcept
    {
        return CharacterFunctions::compareIgnoreCaseUpTo (*this, other, maxChars);
    }

    /** Returns the character index of a substring, or -1 if it isn't found. */
    template <typename CharPointer>
    int indexOf (const CharPointer stringToFind) const noexcept
    {
        return CharacterFunctions::indexOf (*this, stringToFind);
    }

    /** Returns the character index of a unicode character, or -1 if it isn't found. */
    int indexOf (const yup_wchar charToFind) const noexcept
    {
        return CharacterFunctions::indexOfChar (*this, charToFind);
    }

    /** Returns the character index of a unicode character, or -1 if it isn't found. */
    int indexOf (const yup_wchar charToFind, const bool ignoreCase) const noexcept
    {
        return ignoreCase ? CharacterFunctions::indexOfCharIgnoreCase (*this, charToFind)
                          : CharacterFunctions::indexOfChar (*this, charToFind);
    }

    /** Returns true if the first character of this string is whitespace. */
    bool isWhitespace() const noexcept { return CharacterFunctions::isWhitespace ((yup_wchar) * (*this)); }

    /** Returns true if the first character of this string is a digit. */
    bool isDigit() const noexcept
    {
        const CharType c = *data;
        return c >= '0' && c <= '9';
    }

    /** Returns true if the first character of this string is a letter. */
    bool isLetter() const noexcept { return CharacterFunctions::isLetter (operator*()) != 0; }

    /** Returns true if the first character of this string is a letter or digit. */
    bool isLetterOrDigit() const noexcept { return CharacterFunctions::isLetterOrDigit (operator*()) != 0; }

    /** Returns true if the first character of this string is upper-case. */
    bool isUpperCase() const noexcept { return CharacterFunctions::isUpperCase (operator*()) != 0; }

    /** Returns true if the first character of this string is lower-case. */
    bool isLowerCase() const noexcept { return CharacterFunctions::isLowerCase (operator*()) != 0; }

    /** Returns an upper-case version of the first character of this string. */
    yup_wchar toUpperCase() const noexcept { return CharacterFunctions::toUpperCase (operator*()); }

    /** Returns a lower-case version of the first character of this string. */
    yup_wchar toLowerCase() const noexcept { return CharacterFunctions::toLowerCase (operator*()); }

    /** Parses this string as a 32-bit integer. */
    int getIntValue32() const noexcept { return atoi (data); }

    /** Parses this string as a 64-bit integer. */
    int64 getIntValue64() const noexcept
    {
#if YUP_WINDOWS
        return _atoi64 (data);
#else
        return atoll (data);
#endif
    }

    /** Parses this string as a floating point double. */
    double getDoubleValue() const noexcept { return CharacterFunctions::getDoubleValue (*this); }

    /** Returns the first non-whitespace character in the string. */
    CharPointer_UTF8 findEndOfWhitespace() const noexcept { return CharacterFunctions::findEndOfWhitespace (*this); }

    /** Move this pointer to the first non-whitespace character in the string. */
    void incrementToEndOfWhitespace() noexcept { CharacterFunctions::incrementToEndOfWhitespace (*this); }

    /** Returns true if the given unicode character can be represented in this encoding. */
    static bool canRepresent (yup_wchar character) noexcept
    {
        return ((uint32) character) < (uint32) 0x10ffff;
    }

    /** Returns true if this data contains a valid string in this encoding. */
    static bool isValidString (const CharType* dataToTest, int maxBytesToRead)
    {
        while (maxBytesToRead > 0 && *dataToTest != 0)
        {
            int numBytes;
            const auto character = decodeCharacter (dataToTest, numBytes, maxBytesToRead);

            const bool isEncodedReplacement = numBytes == 3
                                              && (uint8) dataToTest[0] == 0xef
                                              && (uint8) dataToTest[1] == 0xbf
                                              && (uint8) dataToTest[2] == 0xbd;

            if (character == replacementCharacter && ! isEncodedReplacement)
                return false;

            dataToTest += numBytes;
            maxBytesToRead -= numBytes;
        }

        return true;
    }

    /** Atomically swaps this pointer for a new value, returning the previous value. */
    CharPointer_UTF8 atomicSwap (const CharPointer_UTF8 newValue)
    {
        return CharPointer_UTF8 (reinterpret_cast<Atomic<CharType*>&> (data).exchange (newValue.data));
    }

    /** These values are the byte-order mark (BOM) values for a UTF-8 stream. */
    enum
    {
        byteOrderMark1 = 0xef,
        byteOrderMark2 = 0xbb,
        byteOrderMark3 = 0xbf
    };

    /** Returns true if the first three bytes in this pointer are the UTF8 byte-order mark (BOM).
        The pointer must not be null, and must point to at least 3 valid bytes.
    */
    static bool isByteOrderMark (const void* possibleByteOrder) noexcept
    {
        YUP_BEGIN_IGNORE_WARNINGS_MSVC (28182)
        jassert (possibleByteOrder != nullptr);
        auto c = static_cast<const uint8*> (possibleByteOrder);

        return c[0] == (uint8) byteOrderMark1
            && c[1] == (uint8) byteOrderMark2
            && c[2] == (uint8) byteOrderMark3;
        YUP_END_IGNORE_WARNINGS_MSVC
    }

private:
    static constexpr yup_wchar replacementCharacter = 0xfffd;

    /*  Decodes the character at data, setting numBytes to the bytes it spans. Malformed
        sequences decode to the replacement character spanning only their valid prefix, so
        every byte is consumed once and a null byte is only ever returned as the terminator.
        A sequence cut off by maxBytes is malformed, so nothing past that bound is read.
    */
    static yup_wchar decodeCharacter (const CharType* data, int& numBytes, int maxBytes = std::numeric_limits<int>::max()) noexcept
    {
        const auto lead = (uint8) data[0];
        numBytes = 1;

        if (lead < 0x80)
            return lead;

        int length = 0;
        uint32 value = 0;
        uint8 minNext = 0x80;
        uint8 maxNext = 0xbf;

        if (lead >= 0xc2 && lead <= 0xdf)
        {
            length = 2;
            value = lead & 0x1f;
        }
        else if (lead >= 0xe0 && lead <= 0xef)
        {
            length = 3;
            value = lead & 0x0f;
            minNext = lead == 0xe0 ? 0xa0 : 0x80;
            maxNext = lead == 0xed ? 0x9f : 0xbf;
        }
        else if (lead >= 0xf0 && lead <= 0xf4)
        {
            length = 4;
            value = lead & 0x07;
            minNext = lead == 0xf0 ? 0x90 : 0x80;
            maxNext = lead == 0xf4 ? 0x8f : 0xbf;
        }
        else
        {
            return replacementCharacter;
        }

        for (int i = 1; i < length; ++i)
        {
            if (i >= maxBytes)
                return replacementCharacter;

            const auto next = (uint8) data[i];

            if (next < minNext || next > maxNext)
                return replacementCharacter;

            numBytes = i + 1;
            value = (value << 6) | (next & 0x3f);
            minNext = 0x80;
            maxNext = 0xbf;
        }

        return (yup_wchar) value;
    }

    CharType* data;
};

} // namespace yup
