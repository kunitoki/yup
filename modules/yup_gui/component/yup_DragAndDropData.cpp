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

DragAndDropData DragAndDropData::withFiles (const Array<File>& newFiles) const
{
    DragAndDropData result (*this);
    result.files = newFiles;
    return result;
}

DragAndDropData DragAndDropData::withText (const String& newText) const
{
    DragAndDropData result (*this);
    result.text = newText;
    return result;
}

DragAndDropData DragAndDropData::withUris (const StringArray& newUris) const
{
    DragAndDropData result (*this);
    result.uris = newUris;
    return result;
}

//==============================================================================

const Array<File>& DragAndDropData::getFiles() const noexcept
{
    return files;
}

const String& DragAndDropData::getText() const noexcept
{
    return text;
}

const StringArray& DragAndDropData::getUris() const noexcept
{
    return uris;
}

//==============================================================================

bool DragAndDropData::hasFiles() const noexcept
{
    return ! files.isEmpty();
}

bool DragAndDropData::hasText() const noexcept
{
    return text.isNotEmpty();
}

bool DragAndDropData::hasUris() const noexcept
{
    return ! uris.isEmpty();
}

bool DragAndDropData::isEmpty() const noexcept
{
    return ! hasFiles() && ! hasText() && ! hasUris();
}

} // namespace yup
