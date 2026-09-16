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

const String DragAndDropData::mimeTypeText { "text/plain;charset=utf-8" };
const String DragAndDropData::mimeTypeUriList { "text/uri-list" };
const String DragAndDropData::mimeTypePng { "image/png" };

//==============================================================================

namespace
{
    MemoryBlock stringToBlock (const String& text)
    {
        return MemoryBlock (text.toRawUTF8(), text.getNumBytesAsUTF8());
    }

    String blockToString (const MemoryBlock& block)
    {
        return String::fromUTF8 (static_cast<const char*> (block.getData()), static_cast<int> (block.getSize()));
    }

    // text/uri-list payloads follow RFC 2483: one URI per line, with blank lines
    // and comment lines (starting with '#') ignored.
    StringArray parseUriList (const String& list)
    {
        StringArray result;

        for (auto line : StringArray::fromLines (list))
        {
            line = line.trim();

            if (line.isEmpty() || line.startsWithChar ('#'))
                continue;

            result.add (line);
        }

        return result;
    }

    String buildUriList (const StringArray& uris)
    {
        return uris.joinIntoString ("\r\n");
    }
} // namespace

//==============================================================================

DragAndDropData DragAndDropData::withFiles (const Array<File>& files) const
{
    StringArray uris;

    for (const auto& file : files)
        uris.add (URL (file).toString (true));

    return withUris (uris);
}

DragAndDropData DragAndDropData::withText (const String& text) const
{
    DragAndDropData result (*this);
    result.setMimeData (mimeTypeText, stringToBlock (text));
    return result;
}

DragAndDropData DragAndDropData::withUris (const StringArray& uris) const
{
    DragAndDropData result (*this);
    result.setMimeData (mimeTypeUriList, stringToBlock (buildUriList (uris)));
    return result;
}

DragAndDropData DragAndDropData::withImage (const Image& image) const
{
#if YUP_IMAGE_FORMAT_PNG
    if (image.isValid())
    {
        // The writer takes ownership of the destination stream, so allocate it on the
        // heap and read the encoded bytes back before the writer's scope ends.
        auto* stream = new MemoryOutputStream();
        PngImageFormatWriter writer (stream, PixelFormat::RGBA);

        if (! writer.writeImage (image))
            return *this;

        return withMimeData (mimeTypePng, stream->getMemoryBlock());
    }
#else
    ignoreUnused (image);
#endif

    return *this;
}

DragAndDropData DragAndDropData::withMimeData (const String& mimeType, MemoryBlock data) const
{
    DragAndDropData result (*this);
    result.setMimeData (mimeType, std::move (data));
    return result;
}

DragAndDropData DragAndDropData::withMimeData (const ClipboardData& data) const
{
    return withMimeData (data.mimeType, data.data);
}

DragAndDropData DragAndDropData::withNativeObject (const var& object) const
{
    DragAndDropData result (*this);
    result.nativeObject = object;
    return result;
}

//==============================================================================

String DragAndDropData::getText() const
{
    if (auto* entry = findEntry (mimeTypeText))
        return blockToString (entry->data);

    return {};
}

Array<File> DragAndDropData::getFiles() const
{
    Array<File> files;

    for (const auto& uri : getUris())
    {
        URL url (uri);

        if (url.isLocalFile())
            files.add (url.getLocalFile());
    }

    return files;
}

StringArray DragAndDropData::getUris() const
{
    if (auto* entry = findEntry (mimeTypeUriList))
        return parseUriList (blockToString (entry->data));

    return {};
}

Image DragAndDropData::getImage() const
{
#if YUP_IMAGE_FORMAT_PNG
    if (auto* entry = findEntry (mimeTypePng))
    {
        auto result = Image::loadFromData (entry->data.asBytes());

        if (result.wasOk())
            return result.getValue();
    }
#endif

    return {};
}

MemoryBlock DragAndDropData::getMimeData (const String& mimeType) const
{
    if (auto* entry = findEntry (mimeType))
        return entry->data;

    return {};
}

StringArray DragAndDropData::getMimeTypes() const
{
    StringArray result;

    for (const auto& entry : mimeData)
        result.add (entry.mimeType);

    return result;
}

const var& DragAndDropData::getNativeObject() const noexcept
{
    return nativeObject;
}

Span<const ClipboardData> DragAndDropData::getAllMimeData() const noexcept
{
    return { mimeData.getRawDataPointer(), static_cast<size_t> (mimeData.size()) };
}

//==============================================================================

bool DragAndDropData::hasText() const
{
    return findEntry (mimeTypeText) != nullptr;
}

bool DragAndDropData::hasFiles() const
{
    return ! getFiles().isEmpty();
}

bool DragAndDropData::hasUris() const
{
    return findEntry (mimeTypeUriList) != nullptr;
}

bool DragAndDropData::hasImage() const
{
    return findEntry (mimeTypePng) != nullptr;
}

bool DragAndDropData::hasMimeData (const String& mimeType) const
{
    return findEntry (mimeType) != nullptr;
}

bool DragAndDropData::hasNativeObject() const noexcept
{
    return ! nativeObject.isVoid();
}

bool DragAndDropData::isEmpty() const noexcept
{
    return mimeData.isEmpty() && ! hasNativeObject();
}

//==============================================================================

const ClipboardData* DragAndDropData::findEntry (const String& mimeType) const noexcept
{
    for (const auto& entry : mimeData)
        if (entry.mimeType.equalsIgnoreCase (mimeType))
            return &entry;

    return nullptr;
}

void DragAndDropData::setMimeData (const String& mimeType, MemoryBlock data)
{
    for (int i = mimeData.size(); --i >= 0;)
        if (mimeData.getReference (i).mimeType.equalsIgnoreCase (mimeType))
            mimeData.remove (i);

    if (data.getSize() > 0)
        mimeData.add (ClipboardData (mimeType, std::move (data)));
}

} // namespace yup
