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

#if YUP_WINDOWS

#include <shlobj.h>

namespace yup
{

//==============================================================================
namespace
{

FORMATETC makeFormatEtc (CLIPFORMAT format)
{
    FORMATETC result {};

    result.cfFormat = format;
    result.ptd = nullptr;
    result.dwAspect = DVASPECT_CONTENT;
    result.lindex = -1;
    result.tymed = TYMED_HGLOBAL;

    return result;
}

/** The clipboard format for a PNG, registered once.

    Windows has no standard PNG clipboard format, so applications that exchange images register one
    under this name - including the ones that would receive a YUP drag.
*/
CLIPFORMAT pngFormat()
{
    static const auto format = static_cast<CLIPFORMAT> (RegisterClipboardFormatW (L"PNG"));

    return format;
}

//==============================================================================

/** Allocates a movable global holding a copy of @a bytes. */
HGLOBAL makeGlobalFromBytes (const void* bytes, SIZE_T size)
{
    if (bytes == nullptr || size == 0)
        return nullptr;

    auto* global = GlobalAlloc (GMEM_MOVEABLE, size);

    if (global == nullptr)
        return nullptr;

    if (auto* destination = static_cast<char*> (GlobalLock (global)))
    {
        memcpy (destination, bytes, size);
        GlobalUnlock (global);
    }

    return global;
}

/** Allocates the CF_UNICODETEXT form of @a text, which is the wide text plus a terminator. */
HGLOBAL makeGlobalFromText (const String& text)
{
    const auto* source = text.toWideCharPointer();
    int characters = 0;

    while (source[characters] != 0)
        ++characters;

    return makeGlobalFromBytes (source, static_cast<SIZE_T> (characters + 1) * sizeof (wchar_t));
}

/** Allocates the CF_HDROP form of a file list: a DROPFILES header followed by the paths, each
    null-terminated, with one more terminator after the last. */
HGLOBAL makeGlobalFromFiles (const Array<File>& files)
{
    StringArray paths;

    for (const auto& file : files)
        paths.add (file.getFullPathName());

    const auto headerSize = sizeof (DROPFILES);
    const auto bytesPerCharacter = static_cast<int> (sizeof (wchar_t));

    int payloadBytes = bytesPerCharacter;

    for (const auto& path : paths)
        payloadBytes += (path.length() + 1) * bytesPerCharacter;

    auto* global = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, headerSize + static_cast<SIZE_T> (payloadBytes));

    if (global == nullptr)
        return nullptr;

    auto* header = static_cast<DROPFILES*> (GlobalLock (global));

    if (header == nullptr)
    {
        GlobalFree (global);
        return nullptr;
    }

    header->pFiles = static_cast<DWORD> (headerSize);
    header->fWide = TRUE; // the shell reads the paths as wide characters

    auto* write = reinterpret_cast<wchar_t*> (reinterpret_cast<char*> (header) + headerSize);

    for (const auto& path : paths)
    {
        for (const auto* character = path.toWideCharPointer(); *character != 0; ++character)
            *write++ = *character;

        *write++ = 0;
    }

    *write = 0;

    GlobalUnlock (global);

    return global;
}

//==============================================================================

/** The OLE data object a drag carries.

    Only GetData and EnumFormatEtc do any work: a destination asks which formats are offered and then
    asks for the one it wants, so the rest of the interface can decline. Formats are allocated per
    request, which is what OLE expects - the medium it hands out owns the global. */
class PayloadDataObject final : public IDataObject
{
public:
    explicit PayloadDataObject (const DragAndDropData& newPayload)
        : payload (newPayload)
    {
    }

    //==============================================================================
    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID iid, void** object) override
    {
        if (object == nullptr)
            return E_INVALIDARG;

        if (iid == IID_IUnknown || iid == IID_IDataObject)
        {
            *object = static_cast<IDataObject*> (this);
            AddRef();
            return S_OK;
        }

        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return static_cast<ULONG> (InterlockedIncrement (&refCount));
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const auto remaining = static_cast<ULONG> (InterlockedDecrement (&refCount));

        if (remaining == 0)
            delete this;

        return remaining;
    }

    //==============================================================================
    HRESULT STDMETHODCALLTYPE GetData (FORMATETC* format, STGMEDIUM* medium) override
    {
        if (format == nullptr || medium == nullptr)
            return E_INVALIDARG;

        auto* global = makeGlobalFor (format->cfFormat);

        if (global == nullptr)
            return DV_E_FORMATETC;

        medium->tymed = TYMED_HGLOBAL;
        medium->hGlobal = global;
        medium->pUnkForRelease = nullptr;

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryGetData (FORMATETC* format) override
    {
        if (format == nullptr)
            return E_INVALIDARG;

        return offersFormat (format->cfFormat) ? S_OK : DV_E_FORMATETC;
    }

    HRESULT STDMETHODCALLTYPE GetDataHere (FORMATETC*, STGMEDIUM*) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc (FORMATETC*, FORMATETC* out) override
    {
        if (out != nullptr)
            out->ptd = nullptr;

        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE SetData (FORMATETC*, STGMEDIUM*, BOOL) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE EnumFormatEtc (DWORD direction, IEnumFORMATETC** enumerator) override
    {
        if (direction != DATADIR_GET || enumerator == nullptr)
            return E_NOTIMPL;

        const auto formats = makeFormats();

        if (formats.isEmpty())
            return S_FALSE;

        return SHCreateStdEnumFmtEtc (static_cast<UINT> (formats.size()), formats.getData(), enumerator);
    }

private:
    //==============================================================================
    bool offersFormat (CLIPFORMAT format) const
    {
        if (format == CF_HDROP)
            return ! payload.getFiles().isEmpty();

        if (format == CF_UNICODETEXT)
            return payload.hasText() || payload.hasUris();

        if (format == pngFormat())
            return payload.getMimeData (DragAndDropData::mimeTypePng).getSize() > 0;

        return false;
    }

    Array<FORMATETC> makeFormats() const
    {
        Array<FORMATETC> formats;

        if (! payload.getFiles().isEmpty())
            formats.add (makeFormatEtc (CF_HDROP));

        if (payload.hasText() || payload.hasUris())
            formats.add (makeFormatEtc (CF_UNICODETEXT));

        if (payload.getMimeData (DragAndDropData::mimeTypePng).getSize() > 0)
            formats.add (makeFormatEtc (pngFormat()));

        return formats;
    }

    HGLOBAL makeGlobalFor (CLIPFORMAT format) const
    {
        if (format == CF_HDROP && ! payload.getFiles().isEmpty())
            return makeGlobalFromFiles (payload.getFiles());

        if (format == CF_UNICODETEXT)
        {
            const auto text = payload.hasText() ? payload.getText()
                                                : payload.getUris().joinIntoString ("\n");

            return makeGlobalFromText (text);
        }

        if (format == pngFormat())
        {
            const auto png = payload.getMimeData (DragAndDropData::mimeTypePng);

            return makeGlobalFromBytes (png.getData(), static_cast<SIZE_T> (png.getSize()));
        }

        return nullptr;
    }

    //==============================================================================
    DragAndDropData payload;
    volatile LONG refCount = 1;
};

//==============================================================================

/** The OLE drop source, which OLE asks whether to keep the drag going as the pointer and the buttons
    change. */
class PayloadDropSource final : public IDropSource
{
public:
    //==============================================================================
    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID iid, void** object) override
    {
        if (object == nullptr)
            return E_INVALIDARG;

        if (iid == IID_IUnknown || iid == IID_IDropSource)
        {
            *object = static_cast<IDropSource*> (this);
            AddRef();
            return S_OK;
        }

        *object = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return static_cast<ULONG> (InterlockedIncrement (&refCount));
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const auto remaining = static_cast<ULONG> (InterlockedDecrement (&refCount));

        if (remaining == 0)
            delete this;

        return remaining;
    }

    //==============================================================================
    HRESULT STDMETHODCALLTYPE QueryContinueDrag (BOOL escapePressed, DWORD keyState) override
    {
        if (escapePressed)
            return DRAGDROP_S_CANCEL;

        // OLE reports the pointer's button by its absence from the key state: the drag is over as
        // soon as it comes up.
        if ((keyState & MK_LBUTTON) == 0)
            return DRAGDROP_S_DROP;

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GiveFeedback (DWORD) override
    {
        return DRAGDROP_S_USEDEFAULTCURSORS;
    }

private:
    volatile LONG refCount = 1;
};

} // namespace

//==============================================================================

std::optional<DragAndDropAction> performNativeDrag (Component& sourceComponent, const DragAndDropData& data)
{
    auto* native = sourceComponent.getNativeComponent();

    if (native == nullptr)
        return std::nullopt;

    auto* window = static_cast<HWND> (native->getNativeHandle());

    if (window == nullptr)
        return std::nullopt;

    auto* dataObject = new PayloadDataObject (data);
    auto* dropSource = new PayloadDropSource();

    DWORD effect = DROPEFFECT_NONE;

    // Runs the OLE drag loop, so this does not return until the gesture is over and the effect comes
    // back with it - the same shape as the AppKit implementation.
    const auto result = DoDragDrop (window,
                                    dataObject,
                                    DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK,
                                    &effect);

    dataObject->Release();
    dropSource->Release();

    // A cancelled drag was still exported, so it reports nothing performed rather than falling back to
    // an in-app drag; only a failure to start at all does that.
    if (result != DRAGDROP_S_DROP && result != DRAGDROP_S_CANCEL)
        return std::nullopt;

    if ((effect & DROPEFFECT_MOVE) != 0)
        return DragAndDropAction::move;

    if ((effect & DROPEFFECT_LINK) != 0)
        return DragAndDropAction::link;

    if ((effect & DROPEFFECT_COPY) != 0)
        return DragAndDropAction::copy;

    return DragAndDropAction::none;
}

} // namespace yup

#endif // YUP_WINDOWS
