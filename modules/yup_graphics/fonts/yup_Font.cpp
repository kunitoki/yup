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

namespace
{

uint32_t axisTagFromString (StringRef tagName)
{
    uint32_t tag = 0;
    if (tagName.length() > 0)
        tag += static_cast<uint8_t> (tagName[0]) << 24;
    if (tagName.length() > 1)
        tag += static_cast<uint8_t> (tagName[1]) << 16;
    if (tagName.length() > 2)
        tag += static_cast<uint8_t> (tagName[2]) << 8;
    if (tagName.length() > 3)
        tag += static_cast<uint8_t> (tagName[3]) << 0;
    return tag;
}

String axisTagToString (uint32_t tag)
{
    String tagName;
    tagName
        << static_cast<char> (tag >> 24)
        << static_cast<char> (tag >> 16)
        << static_cast<char> (tag >> 8)
        << static_cast<char> (tag >> 0);
    return tagName;
}

} // namespace

//==============================================================================

Font::Font (rive::rcp<rive::Font> font)
    : font (std::move (font))
{
}

Font::Font (rive::rcp<rive::Font> font, float height)
    : font (std::move (font))
    , height (height)
{
}

//==============================================================================

Result Font::loadFromData (const MemoryBlock& fontBytes)
{
    if (fontBytes.isEmpty())
        return Result::fail ("Unable to instantiate font from empty data");

    font = HBFont::Decode (rive::make_span (static_cast<const uint8_t*> (fontBytes.getData()), fontBytes.getSize()));
    return font ? Result::ok() : Result::fail ("Unable to load font");
}

Result Font::loadFromFile (const File& fontFile)
{
    if (! fontFile.existsAsFile())
        return Result::fail ("Unable to load font from non existing file");

    if (auto is = fontFile.createInputStream(); is != nullptr && is->openedOk())
    {
        yup::MemoryBlock mb;
        is->readIntoMemoryBlock (mb);
        return loadFromData (mb);
    }

    return Result::ok();
}

//==============================================================================

float Font::getAscent() const
{
    if (font != nullptr)
        return font->lineMetrics().ascent;

    return 0.0f;
}

float Font::getDescent() const
{
    if (font != nullptr)
        return font->lineMetrics().descent;

    return 0.0f;
}

int Font::getWeight() const
{
    if (font != nullptr)
        return font->getWeight();

    return 0;
}

bool Font::isItalic() const
{
    if (font != nullptr)
        return font->isItalic();

    return 0;
}

//==============================================================================

float Font::getHeight() const noexcept
{
    return height;
}

void Font::setHeight (float newHeight)
{
    height = newHeight;
}

Font Font::withHeight (float height) const
{
    Font result (*this);
    result.setHeight (height);
    return result;
}

//==============================================================================

int Font::getNumAxis() const
{
    return font != nullptr ? static_cast<int> (font->getAxisCount()) : 0;
}

std::optional<Font::Axis> Font::getAxisDescription (int index) const
{
    if (font == nullptr || ! isPositiveAndBelow (index, getNumAxis()))
        return std::nullopt;

    const auto axis = font->getAxis (static_cast<uint16_t> (index));

    Axis result;
    result.tagName = axisTagToString (axis.tag);
    result.minimumValue = axis.min;
    result.maximumValue = axis.max;
    result.defaultValue = axis.def;
    return result;
}

std::optional<Font::Axis> Font::getAxisDescription (StringRef tagName) const
{
    if (font == nullptr)
        return std::nullopt;

    const auto tag = axisTagFromString (tagName);

    for (int16_t index = 0; index < font->getAxisCount(); ++index)
    {
        const auto axis = font->getAxis (index);
        if (axis.tag == tag)
        {
            Axis result;
            result.tagName = tagName;
            result.minimumValue = axis.min;
            result.maximumValue = axis.max;
            result.defaultValue = axis.def;
            return result;
        }
    }

    return std::nullopt;
}

float Font::getAxisValue (int index) const
{
    if (font == nullptr || ! isPositiveAndBelow (index, getNumAxis()))
        return 0.0f;

    const auto axis = font->getAxis (static_cast<int16_t> (index));

    return font->getAxisValue (axis.tag);
}

float Font::getAxisValue (StringRef tagName) const
{
    jassert (tagName.length() == 4);

    if (font == nullptr)
        return 0.0f;

    return font->getAxisValue (axisTagFromString (tagName));
}

void Font::setAxisValue (int index, float value)
{
    if (font == nullptr || ! isPositiveAndBelow (index, getNumAxis()))
        return;

    const auto axis = font->getAxis (static_cast<int16_t> (index));

    auto newFont = font->makeAtCoord ({ axis.tag,
                                        jlimit (axis.min, axis.max, value) });

    if (newFont != nullptr)
        std::swap (newFont, font);
}

void Font::setAxisValue (StringRef tagName, float value)
{
    if (font == nullptr)
        return;

    auto axis = getAxisDescription (tagName);
    if (! axis.has_value())
        return;

    auto newFont = font->makeAtCoord ({ axisTagFromString (tagName),
                                        jlimit (axis->minimumValue, axis->maximumValue, value) });

    if (newFont != nullptr)
        std::swap (newFont, font);
}

Font Font::withAxisValue (int index, float value) const
{
    if (font == nullptr || ! isPositiveAndBelow (index, getNumAxis()))
        return {};

    auto axis = getAxisDescription (index);
    if (! axis.has_value())
        return {};

    return Font (font->makeAtCoord ({ axisTagFromString (axis->tagName),
                                      jlimit (axis->minimumValue, axis->maximumValue, value) }));
}

Font Font::withAxisValue (StringRef tagName, float value) const
{
    if (font == nullptr)
        return {};

    auto axis = getAxisDescription (tagName);
    if (! axis.has_value())
        return {};

    return Font (font->makeAtCoord ({ axisTagFromString (tagName),
                                      jlimit (axis->minimumValue, axis->maximumValue, value) }));
}

void Font::setAxisValues (std::initializer_list<AxisOption> axisOptions)
{
    if (font == nullptr || axisOptions.size() == 0)
        return;

    std::vector<rive::Font::Coord> coords;
    coords.reserve (axisOptions.size());

    for (const auto& option : axisOptions)
    {
        auto axis = getAxisDescription (StringRef (option.tagName));
        if (! axis.has_value())
            continue;

        coords.push_back ({ axisTagFromString (option.tagName),
                            jlimit (axis->minimumValue, axis->maximumValue, option.value) });
    }

    if (coords.empty())
        return;

    auto newFont = font->makeAtCoords (coords);
    if (newFont != nullptr)
        std::swap (newFont, font);
}

Font Font::withAxisValues (std::initializer_list<AxisOption> axisOptions) const
{
    if (font == nullptr || axisOptions.size() == 0)
        return {};

    std::vector<rive::Font::Coord> coords;
    coords.reserve (axisOptions.size());

    for (const auto& option : axisOptions)
    {
        auto axis = getAxisDescription (StringRef (option.tagName));
        if (! axis.has_value())
            continue;

        coords.push_back ({ axisTagFromString (option.tagName),
                            jlimit (axis->minimumValue, axis->maximumValue, option.value) });
    }

    if (coords.empty())
        return {};

    return Font (font->makeAtCoords (coords));
}

void Font::resetAxisValue (int index)
{
    if (font == nullptr || ! isPositiveAndBelow (index, getNumAxis()))
        return;

    const auto axis = font->getAxis (static_cast<int16_t> (index));

    setAxisValue (index, axis.def);
}

void Font::resetAxisValue (StringRef tagName)
{
    if (font == nullptr)
        return;

    const auto tag = axisTagFromString (tagName);

    for (int16_t index = 0; index < font->getAxisCount(); ++index)
    {
        auto axis = font->getAxis (index);
        if (axis.tag == tag)
        {
            setAxisValue (static_cast<int> (index), axis.def);
            return;
        }
    }
}

void Font::resetAllAxisValues()
{
    if (font == nullptr)
        return;

    std::vector<rive::Font::Coord> coords;
    coords.reserve (getNumAxis());

    for (int16_t index = 0; index < font->getAxisCount(); ++index)
    {
        auto axis = font->getAxis (index);
        coords.push_back ({ axis.tag, axis.def });
    }

    auto newFont = font->makeAtCoords (coords);
    if (newFont != nullptr)
        std::swap (newFont, font);
}

//==============================================================================

Font Font::withFeature (Feature feature) const
{
    if (font == nullptr)
        return {};

    std::vector<rive::Font::Feature> realFeatures;
    realFeatures.push_back (rive::Font::Feature { feature.tag, feature.value });

    return Font (font->withOptions ({}, realFeatures));
}

Font Font::withFeatures (std::initializer_list<Feature> features) const
{
    if (font == nullptr)
        return {};

    std::vector<rive::Font::Feature> realFeatures;
    realFeatures.reserve (features.size());

    for (const auto& feature : features)
        realFeatures.push_back (rive::Font::Feature { feature.tag, feature.value });

    return Font (font->withOptions ({}, realFeatures));
}

//==============================================================================

bool Font::operator== (const Font& other) const
{
    return font == other.font;
}

bool Font::operator!= (const Font& other) const
{
    return font != other.font;
}

//==============================================================================

rive::rcp<rive::Font> Font::getFont() const
{
    return font;
}

} // namespace yup
