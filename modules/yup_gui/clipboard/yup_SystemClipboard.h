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
/** Represents a piece of clipboard data with its MIME type.

    Bundles together a MIME type string and a binary data payload.

    @see SystemClipboard

    @tags{GUI}
*/
struct YUP_API ClipboardData
{
    /** Creates an empty clipboard data item. */
    ClipboardData() = default;

    /** Creates a clipboard data item.

        @param mimeType  The MIME type identifying this data (e.g. "text/plain", "image/png").
        @param data      The binary data to place on the clipboard.
    */
    ClipboardData (String mimeType, MemoryBlock data);

    /** The MIME type identifying this data. */
    String mimeType;

    /** The binary data payload. */
    MemoryBlock data;
};

//==============================================================================
/** System clipboard.

    This class provides methods to copy and retrieve data from the system clipboard.
    Supports plain text, arbitrary MIME-typed binary data, and primary selection
    (X11 / Wayland).

    @note This class is not thread-safe. All methods should only be called from the main thread.

    @tags{GUI}
*/
class YUP_API SystemClipboard
{
public:
    //==============================================================================
    /** @name Text Clipboard */
    ///@{

    /** Copies the given text to the system clipboard.

        @param text The text to copy to the clipboard.
    */
    static void copyTextToClipboard (const String& text);

    /** Retrieves the text from the system clipboard.

        @return The text from the clipboard, or an empty string if no text is available.
    */
    static String getTextFromClipboard();

    /** Checks whether the clipboard contains non-empty text.

        @return True if the clipboard exists and has text content.
    */
    static bool hasClipboardText();

    ///@}

    //==============================================================================
    /** @name MIME-Typed Clipboard Data */
    ///@{

    /** Places a single data item on the clipboard.

        The data is eagerly copied — the MemoryBlock owns the payload for as long as
        the clipboard holds it.

        @param data       The clipboard data item to offer.
        @param onRelease  An optional callback invoked when the clipboard replaces or
                          clears this data. Use this to release external resources.
        @return           True on success, false on failure.
    */
    static bool copyToClipboard (const ClipboardData& data,
                                 std::function<void()> onRelease = {});

    /** Places multiple data items on the clipboard, each with a different MIME type.

        This allows offering the same logical content in multiple formats (e.g. offering
        an image as both "image/png" and "image/jpeg").

        @param data       The clipboard data items to offer. Must not be empty.
        @param onRelease  An optional callback invoked when the clipboard replaces or
                          clears this batch. Use this to release external resources.
        @return           True on success, false on failure.
    */
    static bool copyToClipboard (const Array<ClipboardData>& data,
                                 std::function<void()> onRelease = {});

    /** Retrieves clipboard data for the specified MIME type.

        @param mimeType  The MIME type to read from the clipboard.
        @return          A ClipboardData containing the MIME type and payload, or an
                         empty ClipboardData if the data is not available.
    */
    static ClipboardData getFromClipboard (const String& mimeType);

    /** Checks whether the clipboard contains data for the given MIME type.

        @param mimeType  The MIME type to check for.
        @return          True if data exists in the clipboard for that MIME type.
    */
    static bool hasClipboardData (const String& mimeType);

    /** Retrieves the list of MIME types currently available in the clipboard.

        @return  The available MIME types.
    */
    static StringArray getClipboardMimeTypes();

    /** Clears all data from the system clipboard.

        @return  True on success, false on failure.
    */
    static bool clearClipboardData();

    ///@}

    //==============================================================================
    /** @name Primary Selection (X11 / Wayland) */
    ///@{

    /** Copies the given text to the primary selection.

        On X11 and Wayland, the primary selection is distinct from the clipboard
        and typically holds currently highlighted text. On other platforms, this
        stores the text internally for later retrieval.

        @param text  The text to copy to the primary selection.
    */
    static void copyTextToPrimarySelection (const String& text);

    /** Retrieves the text from the primary selection.

        @return  The text from the primary selection, or an empty string if
                 no text is available.
    */
    static String getTextFromPrimarySelection();

    /** Checks whether the primary selection contains non-empty text.

        @return  True if the primary selection has text content.
    */
    static bool hasPrimarySelectionText();

    ///@}

private:
    struct ClipboardDataState : public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<ClipboardDataState>;

        ClipboardDataState (Array<ClipboardData> items, std::function<void()> onRelease)
            : items (std::move (items))
            , onRelease (std::move (onRelease))
        {
        }

        Array<ClipboardData> items;
        std::function<void()> onRelease;

    private:
        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipboardDataState)
    };

    static const void* clipboardDataCallback (void* userdata, const char* mimeType, size_t* size);
    static void clipboardCleanupCallback (void* userdata);

    static ReferenceCountedObjectPtr<ClipboardDataState> activeClipboardState;
};

} // namespace yup
