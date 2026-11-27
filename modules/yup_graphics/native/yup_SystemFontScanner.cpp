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

// Implemented by the native/yup_Font_<platform> file of the platforms scanning font folders
Array<File> getSystemFontFolders();

void addFacesOfFile (const File& file, std::vector<SystemFontFace>& faces)
{
    const MemoryMappedFile mapped (file, MemoryMappedFile::readOnly);
    if (mapped.getData() == nullptr || mapped.getSize() == 0 || mapped.getSize() >= (size_t (1) << 31))
        return;

    auto* blob = hb_blob_create_or_fail (static_cast<const char*> (mapped.getData()),
                                         static_cast<unsigned> (mapped.getSize()),
                                         HB_MEMORY_MODE_READONLY,
                                         nullptr,
                                         nullptr);
    if (blob == nullptr)
        return;

    const auto numFaces = hb_face_count (blob);
    for (unsigned index = 0; index < numFaces; ++index)
    {
        auto* face = hb_face_create_or_fail (blob, index);
        if (face == nullptr)
            continue;

        SystemFontFace systemFace;
        systemFace.familyName = readFontName (face, HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY, HB_OT_NAME_ID_FONT_FAMILY);
        systemFace.styleName = readFontName (face, HB_OT_NAME_ID_TYPOGRAPHIC_SUBFAMILY, HB_OT_NAME_ID_FONT_SUBFAMILY);
        systemFace.location = file.getFullPathName();
        systemFace.faceIndex = index;

        if (auto* font = hb_font_create (face))
        {
            systemFace.weight = hb_style_get_value (font, HB_STYLE_TAG_WEIGHT);
            systemFace.italic = hb_style_get_value (font, HB_STYLE_TAG_ITALIC) != 0.0f;
            hb_font_destroy (font);
        }

        hb_face_destroy (face);

        if (systemFace.familyName.isNotEmpty() && systemFace.styleName.isNotEmpty() && ! systemFace.familyName.startsWithChar ('.'))
            faces.push_back (std::move (systemFace));
    }

    hb_blob_destroy (blob);
}

const std::vector<SystemFontFace>& getAllSystemFontFaces()
{
    static const auto allFaces = []
    {
        std::vector<SystemFontFace> faces;

        for (const auto& folder : getSystemFontFolders())
        {
            if (! folder.isDirectory())
                continue;

            for (const auto& file : folder.findChildFiles (File::findFiles, true, "*.ttf;*.otf;*.ttc;*.otc;*.TTF;*.OTF;*.TTC;*.OTC"))
                addFacesOfFile (file, faces);
        }

        return faces;
    }();

    return allFaces;
}

//==============================================================================

StringArray getSystemFontFamilyNames()
{
    StringArray result;

    for (const auto& face : getAllSystemFontFaces())
        result.add (face.familyName);

    return result;
}

std::vector<SystemFontFace> findSystemFontFaces (const String& familyName)
{
    std::vector<SystemFontFace> faces;

    for (const auto& face : getAllSystemFontFaces())
    {
        if (face.familyName.equalsIgnoreCase (familyName))
            faces.push_back (face);
    }

    return faces;
}

rive::rcp<rive::Font> loadSystemFontFace (const SystemFontFace& face)
{
#ifdef RIVE_HB_FILE_MAPPING
    return HBFont::DecodeFile (face.location.toRawUTF8(), face.faceIndex);

#else
    MemoryBlock data;
    if (! File (face.location).loadFileAsData (data) || data.isEmpty())
        return nullptr;

    // HBFont::Decode always takes the first face, so build the face of a collection here
    auto* bytes = new MemoryBlock (std::move (data));
    auto* blob = hb_blob_create_or_fail (static_cast<const char*> (bytes->getData()),
                                         static_cast<unsigned> (bytes->getSize()),
                                         HB_MEMORY_MODE_READONLY,
                                         bytes,
                                         [] (void* context)
    {
        delete static_cast<MemoryBlock*> (context);
    });

    if (blob == nullptr)
        return nullptr;

    auto* hbFace = hb_face_create_or_fail (blob, face.faceIndex);
    hb_blob_destroy (blob);
    if (hbFace == nullptr)
        return nullptr;

    auto* hbFont = hb_font_create (hbFace);
    hb_face_destroy (hbFace);

    return hbFont != nullptr ? rive::rcp<rive::Font> (new HBFont (hbFont)) : nullptr;

#endif
}

} // namespace

} // namespace yup
