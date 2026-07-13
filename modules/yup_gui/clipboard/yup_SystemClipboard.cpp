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

ClipboardData::ClipboardData (String mime, MemoryBlock d)
    : mimeType (std::move (mime))
    , data (std::move (d))
{
}

//==============================================================================

ReferenceCountedObjectPtr<SystemClipboard::ClipboardDataState> SystemClipboard::activeClipboardState;

//==============================================================================

const void* SystemClipboard::clipboardDataCallback (void* userdata, const char* mimeType, size_t* size)
{
    auto* state = static_cast<ClipboardDataState*> (userdata);

    if (state == nullptr || mimeType == nullptr)
    {
        *size = 0;
        return nullptr;
    }

    const String requestedMime = String::fromUTF8 (mimeType);

    for (const auto& item : state->items)
    {
        if (item.mimeType == requestedMime)
        {
            *size = item.data.getSize();
            return item.data.getData();
        }
    }

    *size = 0;
    return nullptr;
}

void SystemClipboard::clipboardCleanupCallback (void* userdata)
{
    auto* state = static_cast<ClipboardDataState*> (userdata);

    if (state != nullptr && state->onRelease)
        state->onRelease();

    if (activeClipboardState.get() == state)
        activeClipboardState = nullptr;
}

//==============================================================================

void SystemClipboard::copyTextToClipboard (const String& text)
{
    SDL_SetClipboardText (text.toRawUTF8());
}

String SystemClipboard::getTextFromClipboard()
{
    if (char* clipboardText = SDL_GetClipboardText())
    {
        String textInClipboard = String::fromUTF8 (clipboardText);
        SDL_free (clipboardText);
        return textInClipboard;
    }

    return {};
}

bool SystemClipboard::hasClipboardText()
{
    return SDL_HasClipboardText();
}

//==============================================================================

bool SystemClipboard::copyToClipboard (const ClipboardData& data, std::function<void()> onRelease)
{
    return copyToClipboard (Array<ClipboardData> { data }, std::move (onRelease));
}

bool SystemClipboard::copyToClipboard (const Array<ClipboardData>& data, std::function<void()> onRelease)
{
    jassert (! data.isEmpty());

    if (data.isEmpty())
        return false;

    // Clean up any previous state held by this process
    if (activeClipboardState != nullptr)
    {
        if (activeClipboardState->onRelease)
            activeClipboardState->onRelease();

        activeClipboardState = nullptr;
    }

    ClipboardDataState::Ptr state = new ClipboardDataState (data, std::move (onRelease));
    activeClipboardState = state;

    std::vector<std::string> utf8MimeTypes;
    utf8MimeTypes.reserve (data.size());

    std::vector<const char*> mimeTypePtrs;
    mimeTypePtrs.reserve (data.size());

    for (const auto& item : state->items)
    {
        utf8MimeTypes.push_back (item.mimeType.toRawUTF8());
        mimeTypePtrs.push_back (utf8MimeTypes.back().c_str());
    }

    return SDL_SetClipboardData (clipboardDataCallback,
                                 clipboardCleanupCallback,
                                 state.get(),
                                 mimeTypePtrs.data(),
                                 mimeTypePtrs.size());
}

ClipboardData SystemClipboard::getFromClipboard (const String& mimeType)
{
    size_t dataSize = 0;
    void* data = SDL_GetClipboardData (mimeType.toRawUTF8(), &dataSize);

    if (data == nullptr || dataSize == 0)
        return {};

    ClipboardData result (mimeType, MemoryBlock (data, dataSize));
    SDL_free (data);
    return result;
}

bool SystemClipboard::hasClipboardData (const String& mimeType)
{
    return SDL_HasClipboardData (mimeType.toRawUTF8());
}

StringArray SystemClipboard::getClipboardMimeTypes()
{
    size_t numMimeTypes = 0;
    char** mimeTypes = SDL_GetClipboardMimeTypes (&numMimeTypes);

    if (mimeTypes == nullptr)
        return {};

    StringArray result;
    for (size_t i = 0; i < numMimeTypes; ++i)
    {
        if (mimeTypes[i] != nullptr)
            result.add (String::fromUTF8 (mimeTypes[i]));
    }

    SDL_free (mimeTypes);
    return result;
}

bool SystemClipboard::clearClipboardData()
{
    return SDL_ClearClipboardData();
}

//==============================================================================

void SystemClipboard::copyTextToPrimarySelection (const String& text)
{
    SDL_SetPrimarySelectionText (text.toRawUTF8());
}

String SystemClipboard::getTextFromPrimarySelection()
{
    if (char* selectionText = SDL_GetPrimarySelectionText())
    {
        String text = String::fromUTF8 (selectionText);
        SDL_free (selectionText);
        return text;
    }

    return {};
}

bool SystemClipboard::hasPrimarySelectionText()
{
    return SDL_HasPrimarySelectionText();
}

} // namespace yup
