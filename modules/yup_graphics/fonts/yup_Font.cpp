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

//==============================================================================

struct ColorEmojiFallback
{
    CriticalSection lock;
    rive::rcp<rive::Font> font;
    bool isResolved = false;
};

ColorEmojiFallback& getColorEmojiFallback()
{
    static ColorEmojiFallback fallback;
    return fallback;
}

rive::rcp<rive::Font> getColorEmojiFallbackFontLoadingOnce()
{
    auto& fallback = getColorEmojiFallback();
    const ScopedLock sl (fallback.lock);

    if (! fallback.isResolved)
    {
        fallback.isResolved = true;

        if (auto result = Font::loadColorEmojiSystemFont(); result.wasOk())
            fallback.font = result.getValue().getFont();
    }

    return fallback.font;
}

rive::rcp<rive::Font> findFallbackFont (rive::Unichar missing, uint32_t fallbackIndex, const rive::Font* requestingFont)
{
    if (fallbackIndex > 0)
        return nullptr;

    auto font = getColorEmojiFallbackFontLoadingOnce();
    if (font == nullptr || font.get() == requestingFont || ! font->hasGlyph (missing))
        return nullptr;

    return font;
}

[[maybe_unused]] const bool isFallbackFontInstalled = []
{
    rive::Font::gFallbackProc = findFallbackFont;
    return true;
}();

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

//==============================================================================

String readFontName (hb_face_t* face, hb_ot_name_id_t preferredId, hb_ot_name_id_t fallbackId)
{
    for (const auto nameId : { preferredId, fallbackId })
    {
        const auto length = hb_ot_name_get_utf8 (face, nameId, HB_LANGUAGE_INVALID, nullptr, nullptr);
        if (length == 0)
            continue;

        HeapBlock<char> buffer (length + 1, true);
        auto size = length + 1;
        hb_ot_name_get_utf8 (face, nameId, HB_LANGUAGE_INVALID, &size, buffer.get());

        if (auto name = String::fromUTF8 (buffer.get(), static_cast<int> (size)).trim(); name.isNotEmpty())
            return name;
    }

    return {};
}

hb_face_t* getFontFace (const rive::rcp<rive::Font>& font)
{
    if (auto* hbFont = dynamic_cast<HBFont*> (font.get()); hbFont != nullptr && hbFont->font() != nullptr)
        return hb_font_get_face (hbFont->font());

    return nullptr;
}

//==============================================================================

/** An installed font face, as listed by the native font enumeration. */
struct SystemFontFace
{
    String familyName;
    String styleName;
    float weight = 400.0f;
    bool italic = false;

    String location;        // The file path, or the PostScript name where the system loads fonts by name
    unsigned faceIndex = 0; // The face within a font collection file
};

// Implemented by the native/yup_Font_<platform> files
ResultValue<Font> loadPlatformSerifFont();
ResultValue<Font> loadPlatformMonospaceFont();
ResultValue<Font> loadPlatformColorEmojiFont();
StringArray getSystemFontFamilyNames();
std::vector<SystemFontFace> findSystemFontFaces (const String& familyName);
rive::rcp<rive::Font> loadSystemFontFace (const SystemFontFace& face);

void sortByWeightThenSlant (std::vector<SystemFontFace>& faces)
{
    std::stable_sort (faces.begin(), faces.end(), [] (const auto& a, const auto& b)
    {
        if (! approximatelyEqual (a.weight, b.weight))
            return a.weight < b.weight;

        return ! a.italic && b.italic;
    });
}

const SystemFontFace* findDefaultFace (const std::vector<SystemFontFace>& faces)
{
    for (const auto& face : faces)
    {
        if (face.styleName.equalsIgnoreCase ("Regular"))
            return &face;
    }

    const SystemFontFace* best = nullptr;
    for (const auto& face : faces)
    {
        if (face.italic)
            continue;

        if (best == nullptr || std::abs (face.weight - 400.0f) < std::abs (best->weight - 400.0f))
            best = &face;
    }

    return best != nullptr ? best : (faces.empty() ? nullptr : &faces.front());
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

ResultValue<Font> Font::loadFontFromData (const MemoryBlock& fontBytes)
{
    if (fontBytes.isEmpty())
        return yup::makeResultValueFail ("Unable to instantiate font from empty data");

    auto font = HBFont::Decode (rive::make_span (static_cast<const uint8_t*> (fontBytes.getData()), fontBytes.getSize()));
    if (font == nullptr)
        return yup::makeResultValueFail ("Unable to load font");

    return yup::makeResultValueOk (Font (std::move (font)));
}

ResultValue<Font> Font::loadFontFromData (const Span<const uint8>& fontBytes)
{
    if (fontBytes.empty())
        return yup::makeResultValueFail ("Unable to instantiate font from empty data");

    auto font = HBFont::Decode (rive::make_span (fontBytes.data(), fontBytes.size()));
    if (font == nullptr)
        return yup::makeResultValueFail ("Unable to load font");

    return yup::makeResultValueOk (Font (std::move (font)));
}

//==============================================================================

ResultValue<Font> Font::loadFontFromFile (const File& fontFile)
{
    if (! fontFile.existsAsFile())
        return yup::makeResultValueFail ("Unable to load font from non existing file");

    auto is = fontFile.createInputStream();
    if (is == nullptr || ! is->openedOk())
        return yup::makeResultValueFail ("Unable to open font file");

    yup::MemoryBlock mb;
    is->readIntoMemoryBlock (mb);

    return loadFontFromData (mb);
}

ResultValue<Font> Font::loadFontFromFirstAvailableFile (std::initializer_list<const char*> fontPaths)
{
    for (auto* fontPath : fontPaths)
    {
        auto font = loadFontFromFile (File (fontPath));
        if (font.wasOk())
            return font;
    }

    return yup::makeResultValueFail ("No font found among the provided paths");
}

//==============================================================================

ResultValue<Font> Font::loadSerifSystemTextFont()
{
    return loadPlatformSerifFont();
}

ResultValue<Font> Font::loadColorEmojiSystemFont()
{
    auto result = loadPlatformColorEmojiFont();

    if (result.wasOk() && ! result.getValue().getFont()->hasColorGlyphs())
        return yup::makeResultValueFail ("The system emoji font has no color glyphs");

    return result;
}

void Font::setColorEmojiFallbackFont (const Font& font)
{
    auto& fallback = getColorEmojiFallback();
    const ScopedLock sl (fallback.lock);

    fallback.font = font.getFont();
    fallback.isResolved = true;
}

Font Font::getColorEmojiFallbackFont()
{
    return Font (getColorEmojiFallbackFontLoadingOnce());
}

ResultValue<Font> Font::loadMonospaceSystemTextFont()
{
    return loadPlatformMonospaceFont();
}

//==============================================================================

StringArray Font::findSystemFontFamilies()
{
    StringArray names;

    for (const auto& name : getSystemFontFamilyNames())
    {
        if (name.isNotEmpty() && ! name.startsWithChar ('.'))
            names.add (name);
    }

    names.sortNatural();

    StringArray families;
    for (const auto& name : names)
    {
        if (families.isEmpty() || ! families[families.size() - 1].equalsIgnoreCase (name))
            families.add (name);
    }

    return families;
}

StringArray Font::findSystemFontStyles (const String& familyName)
{
    StringArray styles;

    auto faces = findSystemFontFaces (familyName);
    sortByWeightThenSlant (faces);

    for (const auto& face : faces)
        styles.addIfNotAlreadyThere (face.styleName, true);

    return styles;
}

ResultValue<Font> Font::loadSystemFont (const String& familyName, const String& styleName)
{
    if (familyName.isEmpty())
        return yup::makeResultValueFail ("No font family name given");

    auto faces = findSystemFontFaces (familyName);
    sortByWeightThenSlant (faces);

    if (faces.empty())
        return yup::makeResultValueFail ("The font family \"" + familyName + "\" is not installed");

    const SystemFontFace* face = nullptr;

    if (styleName.isEmpty())
    {
        face = findDefaultFace (faces);
    }
    else
    {
        for (const auto& candidate : faces)
        {
            if (candidate.styleName.equalsIgnoreCase (styleName))
            {
                face = &candidate;
                break;
            }
        }
    }

    if (face == nullptr)
        return yup::makeResultValueFail ("The font family \"" + familyName + "\" has no style \"" + styleName + "\"");

    auto font = loadSystemFontFace (*face);
    if (font == nullptr)
        return yup::makeResultValueFail ("Unable to load the font \"" + familyName + " " + face->styleName + "\"");

    return yup::makeResultValueOk (Font (std::move (font)));
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

String Font::getFamilyName() const
{
    if (auto* face = getFontFace (font))
        return readFontName (face, HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY, HB_OT_NAME_ID_FONT_FAMILY);

    return {};
}

String Font::getStyleName() const
{
    if (auto* face = getFontFace (font))
        return readFontName (face, HB_OT_NAME_ID_TYPOGRAPHIC_SUBFAMILY, HB_OT_NAME_ID_FONT_SUBFAMILY);

    return {};
}

StringArray Font::getFeatureTags() const
{
    StringArray tags;

    if (font == nullptr)
        return tags;

    for (const auto tag : font->features())
        tags.add (axisTagToString (tag));

    tags.removeDuplicates (false);
    tags.sort (false);
    return tags;
}

std::vector<yup_wchar> Font::getCodepoints() const
{
    std::vector<yup_wchar> result;

    if (font == nullptr)
        return result;

    for (const auto codepoint : font->codepoints())
        result.push_back (static_cast<yup_wchar> (codepoint));

    std::sort (result.begin(), result.end());
    result.erase (std::unique (result.begin(), result.end()), result.end());
    return result;
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
    Font result (*this);
    result.setAxisValue (index, value);
    return result;
}

Font Font::withAxisValue (StringRef tagName, float value) const
{
    Font result (*this);
    result.setAxisValue (tagName, value);
    return result;
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
    Font result (*this);
    result.setAxisValues (axisOptions);
    return result;
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
        return *this;

    std::vector<rive::Font::Feature> realFeatures;
    realFeatures.push_back (rive::Font::Feature { feature.tag, feature.value });

    return Font (font->withOptions ({}, realFeatures), height);
}

Font Font::withFeatures (std::initializer_list<Feature> features) const
{
    if (font == nullptr)
        return *this;

    std::vector<rive::Font::Feature> realFeatures;
    realFeatures.reserve (features.size());

    for (const auto& feature : features)
        realFeatures.push_back (rive::Font::Feature { feature.tag, feature.value });

    return Font (font->withOptions ({}, realFeatures), height);
}

//==============================================================================

bool Font::operator== (const Font& other) const
{
    return font == other.font && exactlyEqual (height, other.height);
}

bool Font::operator!= (const Font& other) const
{
    return ! (*this == other);
}

//==============================================================================

rive::rcp<rive::Font> Font::getFont() const
{
    return font;
}

//==============================================================================

namespace
{

struct FontTypefaceHolder;

struct FontHolderRegistry
{
    std::mutex mutex;
    std::unordered_map<const rive::Font*, FontTypefaceHolder*> holders;
};

const std::shared_ptr<FontHolderRegistry>& getFontHolderRegistry()
{
    static const auto registry = std::make_shared<FontHolderRegistry>();
    return registry;
}

struct FontTypefaceHolder final : ReferenceCountedObject
{
    FontTypefaceHolder (rive::rcp<rive::Font> fontToHold, std::shared_ptr<FontHolderRegistry> registryToUse)
        : font (std::move (fontToHold))
        , registry (std::move (registryToUse))
    {
    }

    ~FontTypefaceHolder() override
    {
        if (registry == nullptr || font == nullptr)
            return;

        const std::scoped_lock lock (registry->mutex);
        if (const auto it = registry->holders.find (font.get()); it != registry->holders.end() && it->second == this)
            registry->holders.erase (it);
    }

    rive::rcp<rive::Font> font;
    std::shared_ptr<FontHolderRegistry> registry;
};

} // namespace

Font VariantConverter<Font>::fromVar (const var& v)
{
    const auto* values = v.getArray();
    if (values == nullptr || values->size() != 2)
        return {};

    const auto& heightVar = (*values)[1];
    if (! (heightVar.isDouble() || heightVar.isInt() || heightVar.isInt64()))
        return {};

    const auto parsedHeight = static_cast<float> (heightVar);

    if ((*values)[0].isVoid())
        return Font().withHeight (parsedHeight);

    if (const auto* holder = dynamic_cast<const FontTypefaceHolder*> ((*values)[0].getObject()))
        return Font (holder->font, parsedHeight);

    return {};
}

var VariantConverter<Font>::toVar (const Font& f)
{
    if (f.getFont() == nullptr)
        return Array<var> { var(), static_cast<double> (f.getHeight()) };

    const auto& registry = getFontHolderRegistry();
    ReferenceCountedObjectPtr<FontTypefaceHolder> holder;

    {
        const std::scoped_lock lock (registry->mutex);
        const auto* rawFont = f.getFont().get();

        if (const auto it = registry->holders.find (rawFont); it != registry->holders.end() && it->second->getReferenceCount() > 0)
        {
            holder = it->second;
        }
        else
        {
            holder = new FontTypefaceHolder (f.getFont(), registry);
            registry->holders[rawFont] = holder.get();
        }
    }

    return Array<var> { var (holder.get()), static_cast<double> (f.getHeight()) };
}

} // namespace yup
