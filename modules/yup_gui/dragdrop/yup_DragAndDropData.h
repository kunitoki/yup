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

    A DragAndDropData carries the items being dragged as a set of MIME-typed binary
    blobs, reusing the ClipboardData vocabulary of the system clipboard. Plain text,
    file lists, URIs and images are stored over well-known MIME types and exposed
    through convenience accessors, so there is a single source of truth for the
    payload rather than one field per kind of data.

    In addition to the MIME store, a drag carries an optional `var` "native object".
    This is a same-process, zero-copy escape hatch that lets the source of a drag
    hand a live C++ object (a Model, a track, ...) to an interested target without
    serialising it. It is only meaningful while the drag stays inside this process
    and is empty for anything that crosses a process or application boundary.

    Instances are immutable value types built through the fluent `withX` methods:
    each builder copies the object, applies the change and returns the copy.

    @note There is no support for lazy or promised data providers: every MIME blob is
          an eagerly-owned MemoryBlock. To drag a rendered asset out, write it to a
          temporary file and offer its path.

    @see ClipboardData, SystemClipboard
*/
class YUP_API DragAndDropData
{
public:
    //==============================================================================
    /** The MIME type used to store plain UTF-8 text, matching the system clipboard. */
    static const String mimeTypeText;

    /** The MIME type used to store a list of URIs, one per line (RFC 2483). */
    static const String mimeTypeUriList;

    /** The MIME type used to store a PNG-encoded image. */
    static const String mimeTypePng;

    //==============================================================================
    /** Creates an empty DragAndDropData object. */
    DragAndDropData() = default;

    //==============================================================================
    /** Returns a copy of this object with the given files set.

        Each file is offered as a `file://` URI under the `text/uri-list` MIME type,
        so a file list and a URI list share one representation.

        @param files The files to associate with the drag.
    */
    DragAndDropData withFiles (const Array<File>& files) const;

    /** Returns a copy of this object with the given plain text set.

        The text is stored under the `text/plain;charset=utf-8` MIME type. Setting an
        empty string removes any previously stored text.

        @param text The text to associate with the drag.
    */
    DragAndDropData withText (const String& text) const;

    /** Returns a copy of this object with the given URIs set.

        The URIs are stored under the `text/uri-list` MIME type, one per line.

        @param uris The URIs to associate with the drag.
    */
    DragAndDropData withUris (const StringArray& uris) const;

    /** Returns a copy of this object with the given image set.

        The image is encoded as PNG and stored under the `image/png` MIME type. When
        PNG support is not compiled in, or the image is invalid, the payload is
        returned unchanged.

        @param image The image to associate with the drag.
    */
    DragAndDropData withImage (const Image& image) const;

    /** Returns a copy of this object with the given MIME data set.

        Replaces any existing data stored under the same MIME type. Passing an empty
        block removes the entry.

        @param mimeType The MIME type identifying this data.
        @param data     The binary data to associate with the drag.
    */
    DragAndDropData withMimeData (const String& mimeType, MemoryBlock data) const;

    /** Returns a copy of this object with the given MIME data set.

        @param data The MIME-typed data to associate with the drag.
    */
    DragAndDropData withMimeData (const ClipboardData& data) const;

    /** Returns a copy of this object with the given native object set.

        The native object is a same-process-only payload: it is never transported
        across a process or application boundary, and a receiving target must be
        ready to find it empty for external drags.

        @param object The native object to associate with the drag.
    */
    DragAndDropData withNativeObject (const var& object) const;

    //==============================================================================
    /** Returns the plain text stored under `text/plain;charset=utf-8`, or an empty string. */
    String getText() const;

    /** Returns the files parsed from the `text/uri-list` MIME type. */
    Array<File> getFiles() const;

    /** Returns the URIs parsed from the `text/uri-list` MIME type. */
    StringArray getUris() const;

    /** Returns the image decoded from the `image/png` MIME type, or an invalid image. */
    Image getImage() const;

    /** Returns the data stored for the given MIME type, or an empty block. */
    MemoryBlock getMimeData (const String& mimeType) const;

    /** Returns the MIME types currently present in the payload. */
    StringArray getMimeTypes() const;

    /** Returns the same-process native object, or a void var if none was set. */
    const var& getNativeObject() const noexcept;

    /** Returns a view over every MIME-typed entry held by this payload. */
    Span<const ClipboardData> getAllMimeData() const noexcept;

    //==============================================================================
    /** Returns true if the payload holds non-empty plain text. */
    bool hasText() const;

    /** Returns true if the payload holds one or more files. */
    bool hasFiles() const;

    /** Returns true if the payload holds one or more URIs. */
    bool hasUris() const;

    /** Returns true if the payload holds an image. */
    bool hasImage() const;

    /** Returns true if the payload holds data for the given MIME type. */
    bool hasMimeData (const String& mimeType) const;

    /** Returns true if the payload holds a native object set with withNativeObject(). */
    bool hasNativeObject() const noexcept;

    /** Returns true if the payload holds no MIME data and no native object. */
    bool isEmpty() const noexcept;

private:
    //==============================================================================
    const ClipboardData* findEntry (const String& mimeType) const noexcept;
    void setMimeData (const String& mimeType, MemoryBlock data);

    //==============================================================================
    Array<ClipboardData> mimeData;
    var nativeObject;
};

} // namespace yup
