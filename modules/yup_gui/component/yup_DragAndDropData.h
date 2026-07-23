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

#pragma once

namespace yup
{

//==============================================================================
/** Represents the payload of a drag-and-drop operation delivered to a component.

    A DragAndDropData object carries the items dropped onto a component. It is an
    immutable value type built through the fluent `withX` methods, mirroring the
    style of MouseEvent. Today it can hold files (local filesystem paths) and text,
    with URIs reserved as an extension point for future URI-capable backends.

    @see Component::isInterestedInDrag, Component::itemsDropped
*/
class YUP_API DragAndDropData
{
public:
    //==============================================================================
    /** Creates an empty DragAndDropData object. */
    DragAndDropData() = default;

    //==============================================================================
    /** Returns a copy of this object with the given files set.

        @param files The files to associate with the drop.
    */
    DragAndDropData withFiles (const Array<File>& files) const;

    /** Returns a copy of this object with the given text set.

        @param text The text to associate with the drop.
    */
    DragAndDropData withText (const String& text) const;

    /** Returns a copy of this object with the given URIs set.

        Reserved for future URI-capable backends; SDL leaves this empty.

        @param uris The URIs to associate with the drop.
    */
    DragAndDropData withUris (const StringArray& uris) const;

    //==============================================================================
    /** Returns the dropped files. */
    const Array<File>& getFiles() const noexcept;

    /** Returns the dropped text. */
    const String& getText() const noexcept;

    /** Returns the dropped URIs. */
    const StringArray& getUris() const noexcept;

    //==============================================================================
    /** Returns true if the payload contains one or more files. */
    bool hasFiles() const noexcept;

    /** Returns true if the payload contains text. */
    bool hasText() const noexcept;

    /** Returns true if the payload contains one or more URIs. */
    bool hasUris() const noexcept;

    /** Returns true if the payload contains no files, text, or URIs. */
    bool isEmpty() const noexcept;

private:
    Array<File> files;
    String text;
    StringArray uris;
};

} // namespace yup
