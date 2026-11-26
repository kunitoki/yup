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

namespace
{

//==============================================================================

ResultValue<Font> loadCoreTextFont (CTFontRef ctFont, bool useSystemShaper, float weight, float width)
{
    auto font = HBFont::FromSystem (const_cast<void*> (static_cast<const void*> (ctFont)),
                                    useSystemShaper,
                                    static_cast<uint16_t> (jlimit (1.0f, 1000.0f, weight)),
                                    static_cast<uint8_t> (jlimit (1.0f, 255.0f, width)));

    if (font == nullptr)
        return yup::makeResultValueFail ("Unable to load the CoreText font");

    return yup::makeResultValueOk (Font (std::move (font)));
}

ResultValue<Font> loadSystemUIFont (CTFontUIFontType fontType)
{
    const CFUniquePtr<CTFontRef> systemFont (CTFontCreateUIFontForLanguage (fontType, 0.0, nullptr));
    if (systemFont == nullptr)
        return yup::makeResultValueFail ("Unable to load the system UI font");

    return loadCoreTextFont (systemFont.get(), true, 400.0f, 100.0f);
}

float getCoreTextAxisValue (CTFontRef font, uint32 tag, float fallback)
{
    const CFUniquePtr<CFNumberRef> axisId (CFNumberCreate (nullptr, kCFNumberSInt32Type, &tag));

    if (const CFUniquePtr<CFDictionaryRef> variation (CTFontCopyVariation (font)); variation != nullptr)
    {
        if (auto value = static_cast<CFNumberRef> (CFDictionaryGetValue (variation.get(), axisId.get())))
        {
            float result = fallback;
            CFNumberGetValue (value, kCFNumberFloatType, &result);
            return result;
        }
    }

    if (const CFUniquePtr<CFArrayRef> axes (CTFontCopyVariationAxes (font)); axes != nullptr)
    {
        for (CFIndex i = 0; i < CFArrayGetCount (axes.get()); ++i)
        {
            auto axis = static_cast<CFDictionaryRef> (CFArrayGetValueAtIndex (axes.get(), i));
            auto identifier = static_cast<CFNumberRef> (CFDictionaryGetValue (axis, kCTFontVariationAxisIdentifierKey));
            auto defaultValue = static_cast<CFNumberRef> (CFDictionaryGetValue (axis, kCTFontVariationAxisDefaultValueKey));

            uint32 identifierTag = 0;
            if (identifier == nullptr || defaultValue == nullptr || ! CFNumberGetValue (identifier, kCFNumberSInt32Type, &identifierTag) || identifierTag != tag)
                continue;

            float result = fallback;
            CFNumberGetValue (defaultValue, kCFNumberFloatType, &result);
            return result;
        }
    }

    return fallback;
}

String copyDescriptorString (CTFontDescriptorRef descriptor, CFStringRef attribute)
{
    if (const CFUniquePtr<CFStringRef> value (static_cast<CFStringRef> (CTFontDescriptorCopyAttribute (descriptor, attribute))); value != nullptr)
        return String::fromCFString (value.get());

    return {};
}

SystemFontFace makeSystemFontFace (CTFontDescriptorRef descriptor)
{
    SystemFontFace face;
    face.familyName = copyDescriptorString (descriptor, kCTFontFamilyNameAttribute);
    face.styleName = copyDescriptorString (descriptor, kCTFontStyleNameAttribute);
    face.location = copyDescriptorString (descriptor, kCTFontNameAttribute);

    const CFUniquePtr<CFDictionaryRef> traits (static_cast<CFDictionaryRef> (CTFontDescriptorCopyAttribute (descriptor, kCTFontTraitsAttribute)));
    if (traits == nullptr)
        return face;

    if (auto weight = static_cast<CFNumberRef> (CFDictionaryGetValue (traits.get(), kCTFontWeightTrait)))
    {
        // CoreText weights go from -1 (thin) to 1 (black), 0 being regular
        double normalized = 0.0;
        CFNumberGetValue (weight, kCFNumberDoubleType, &normalized);
        face.weight = static_cast<float> (normalized <= 0.0 ? 400.0 + normalized * 300.0 : 400.0 + normalized * 500.0);
    }

    if (auto symbolic = static_cast<CFNumberRef> (CFDictionaryGetValue (traits.get(), kCTFontSymbolicTrait)))
    {
        uint32 symbolicTraits = 0;
        CFNumberGetValue (symbolic, kCFNumberSInt32Type, &symbolicTraits);
        face.italic = (symbolicTraits & kCTFontItalicTrait) != 0;
    }

    return face;
}

//==============================================================================

ResultValue<Font> loadPlatformSerifFont()
{
    return loadSystemUIFont (kCTFontUIFontSystem);
}

ResultValue<Font> loadPlatformMonospaceFont()
{
    return loadSystemUIFont (kCTFontUIFontUserFixedPitch);
}

ResultValue<Font> loadPlatformColorEmojiFont()
{
    const CFUniquePtr<CTFontRef> emojiFont (CTFontCreateWithName (CFSTR ("AppleColorEmoji"), 0.0, nullptr));
    if (emojiFont == nullptr)
        return yup::makeResultValueFail ("Unable to load the Apple Color Emoji font");

    // Shaped by HarfBuzz: CoreText shaping reports fallback runs in UTF-16 offsets, which
    // overrun the text for emoji outside the basic multilingual plane
    return loadCoreTextFont (emojiFont.get(), false, 400.0f, 100.0f);
}

//==============================================================================

StringArray getSystemFontFamilyNames()
{
    StringArray result;

    if (const CFUniquePtr<CFArrayRef> names (CTFontManagerCopyAvailableFontFamilyNames()); names != nullptr)
    {
        for (CFIndex i = 0; i < CFArrayGetCount (names.get()); ++i)
            result.add (String::fromCFString (static_cast<CFStringRef> (CFArrayGetValueAtIndex (names.get(), i))));
    }

    return result;
}

std::vector<SystemFontFace> findSystemFontFaces (const String& familyName)
{
    std::vector<SystemFontFace> faces;

    const auto families = getSystemFontFamilyNames();
    const auto familyIndex = families.indexOf (familyName, true);
    if (familyIndex < 0 || families[familyIndex].startsWithChar ('.'))
        return faces;

    const CFUniquePtr<CFStringRef> cfFamilyName (families[familyIndex].toCFString());
    const void* keys[] = { kCTFontFamilyNameAttribute };
    const void* values[] = { cfFamilyName.get() };
    const CFUniquePtr<CFDictionaryRef> attributes (CFDictionaryCreate (nullptr, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    const CFUniquePtr<CTFontDescriptorRef> request (CTFontDescriptorCreateWithAttributes (attributes.get()));
    const CFUniquePtr<CFSetRef> mandatory (CFSetCreate (nullptr, keys, 1, &kCFTypeSetCallBacks));
    const CFUniquePtr<CFArrayRef> matches (CTFontDescriptorCreateMatchingFontDescriptors (request.get(), mandatory.get()));
    if (matches == nullptr)
        return faces;

    for (CFIndex i = 0; i < CFArrayGetCount (matches.get()); ++i)
    {
        auto face = makeSystemFontFace (static_cast<CTFontDescriptorRef> (CFArrayGetValueAtIndex (matches.get(), i)));

        // CoreText substitutes fonts it cannot find, so keep only the faces of the requested family
        if (face.familyName.equalsIgnoreCase (families[familyIndex]) && face.styleName.isNotEmpty() && face.location.isNotEmpty())
            faces.push_back (std::move (face));
    }

    return faces;
}

rive::rcp<rive::Font> loadSystemFontFace (const SystemFontFace& face)
{
    const CFUniquePtr<CFStringRef> postScriptName (face.location.toCFString());
    const CFUniquePtr<CTFontRef> ctFont (CTFontCreateWithName (postScriptName.get(), 0.0, nullptr));
    if (ctFont == nullptr)
        return nullptr;

    // CoreText substitutes fonts it cannot find
    if (const CFUniquePtr<CFStringRef> loadedName (CTFontCopyPostScriptName (ctFont.get())); loadedName == nullptr || String::fromCFString (loadedName.get()) != face.location)
        return nullptr;

    // HBFont::FromSystem sets these axes, so pass the ones of the face to keep named instances of variable fonts
    const auto weight = getCoreTextAxisValue (ctFont.get(), HB_TAG ('w', 'g', 'h', 't'), 400.0f);
    const auto width = getCoreTextAxisValue (ctFont.get(), HB_TAG ('w', 'd', 't', 'h'), 100.0f);

    auto font = loadCoreTextFont (ctFont.get(), false, weight, width);
    return font.wasOk() ? font.getValue().getFont() : nullptr;
}

} // namespace

} // namespace yup
