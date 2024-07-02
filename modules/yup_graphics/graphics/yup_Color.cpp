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
*/

namespace yup
{

//==============================================================================

namespace
{
int hexCharToInt (juce_wchar c) noexcept
{
    return CharacterFunctions::getHexDigitValue (c);
}

int parseNextInt (String::CharPointerType& data)
{
    int result = 0;
    bool isNegative = false;

    while (*data != '\0' && (*data == ' ' || *data == ','))
        ++data;

    if (*data == '-')
    {
        isNegative = true;
        ++data;
    }

    while (*data >= '0' && *data <= '9')
    {
        result = result * 10 + (*data - '0');
        ++data;
    }

    while (*data != '\0' && (*data == ' ' || *data == ',' || *data == ')'))
        ++data;

    return isNegative ? -result : result;
}

Color parseHexColor (const String& hexString)
{
    const int length = hexString.length();
    auto data = hexString.getCharPointer();

    if (length == 4) // #RGB
    {
        uint8 red = static_cast<uint8> (hexCharToInt (data[1]) * 16 + hexCharToInt (data[1]));
        uint8 green = static_cast<uint8> (hexCharToInt (data[2]) * 16 + hexCharToInt (data[2]));
        uint8 blue = static_cast<uint8> (hexCharToInt (data[3]) * 16 + hexCharToInt (data[3]));

        return { red, green, blue };
    }
    else if (length == 7) // #RRGGBB
    {
        uint8 red = static_cast<uint8> (hexCharToInt (data[1]) * 16 + hexCharToInt (data[2]));
        uint8 green = static_cast<uint8> (hexCharToInt (data[3]) * 16 + hexCharToInt (data[4]));
        uint8 blue = static_cast<uint8> (hexCharToInt (data[5]) * 16 + hexCharToInt (data[6]));

        return { red, green, blue };
    }
    else if (length == 9) // #RRGGBBAA
    {
        uint8 red = static_cast<uint8> (hexCharToInt (data[1]) * 16 + hexCharToInt (data[2]));
        uint8 green = static_cast<uint8> (hexCharToInt (data[3]) * 16 + hexCharToInt (data[4]));
        uint8 blue = static_cast<uint8> (hexCharToInt (data[5]) * 16 + hexCharToInt (data[6]));
        uint8 alpha = static_cast<uint8> (hexCharToInt (data[7]) * 16 + hexCharToInt (data[8]));

        return { red, green, blue, alpha };
    }
    else
    {
        return {};
    }
}

Color parseRGBColor (const String& rgbString)
{
    int r = 0, g = 0, b = 0, a = 255;

    auto data = rgbString.getCharPointer();
    bool isRGBA = rgbString.startsWithIgnoreCase ("rgba(");
    bool isRGB = rgbString.startsWithIgnoreCase ("rgb(");

    if (! isRGBA && ! isRGB)
        return {};

    while (*data != '(' && *data != '\0')
        ++data;

    if (*data == '(')
        ++data;

    r = parseNextInt (data);
    g = parseNextInt (data);
    b = parseNextInt (data);

    if (isRGBA)
        a = parseNextInt (data);

    return { static_cast<uint8> (r), static_cast<uint8> (g), static_cast<uint8> (b), static_cast<uint8> (a) };
}

Color parseNamedColor (const String& name)
{
    if (auto color = Colors::getNamedColor (name))
        return *color;

    return {};
}
} // namespace

//==============================================================================

String Color::toString() const
{
    String result;
    result << "#" << String::toHexString (r) << String::toHexString (g) << String::toHexString (b) << String::toHexString (a);
    return result;
}

String Color::toStringRGB (bool withAlpha) const
{
    String result;
    result << "rgb(" << String (r) << "," << String (g);

    if (withAlpha)
        result << "," << String (b);

    result << ")";

    return result;
}

//==============================================================================

Color Color::fromString (const String& colorString)
{
    if (colorString.startsWith ("#"))
        return parseHexColor (colorString);

    else if (colorString.startsWithIgnoreCase ("rgb"))
        return parseRGBColor (colorString);

    else
        return parseNamedColor (colorString);
}

} // namespace yup
