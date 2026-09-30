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
/* SDL has no browser clipboard backend: its clipboard only lives inside the app.
   Copies are mirrored to navigator.clipboard, and the paste chord is held back from
   SDL (which would cancel it) until the browser paste event has delivered the system
   clipboard text, then it is replayed as a regular SDL key event.
*/
void writeTextToBrowserClipboard (const String& text)
{
    // clang-format off
    EM_ASM ({
        if (typeof navigator !== "undefined" && navigator.clipboard && navigator.clipboard.writeText)
            navigator.clipboard.writeText (UTF8ToString ($0)).catch (function() {});
    }, text.toRawUTF8());
    // clang-format on
}

void handleBrowserPaste (char* text)
{
    if (text != nullptr)
    {
        SDL_SetClipboardText (text);
        std::free (text);
    }

    auto* window = SDL_GetKeyboardFocus();
    if (window == nullptr)
        return;

    SDL_Event event {};
    event.key.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = SDL_GetWindowID (window);
    event.key.scancode = SDL_SCANCODE_V;
    event.key.key = SDLK_V;
    event.key.mod = SDL_GetModState();
    event.key.down = true;

    SDL_PushEvent (&event);
}

void installBrowserPasteHandlers()
{
    // clang-format off
    EM_ASM ({
        var pasteCallback = $0;

        if (typeof window === "undefined" || window.yupPasteHandlersInstalled)
            return;

        if (! (navigator.clipboard && navigator.clipboard.writeText))
            return;

        window.yupPasteHandlersInstalled = true;

        var pendingTimer = null;

        function isPasteChord (event)
        {
            var target = event.target;
            if (target && (target.isContentEditable || target.tagName === "INPUT" || target.tagName === "TEXTAREA"))
                return false;

            return (event.key === "v" || event.key === "V") && ! event.altKey && (event.ctrlKey || event.metaKey);
        }

        function dispatchPaste (text)
        {
            if (pendingTimer === null)
                return;

            clearTimeout (pendingTimer);
            pendingTimer = null;

            var ptr = 0;
            if (text)
            {
                var length = lengthBytesUTF8 (text) + 1;
                ptr = _malloc (length);
                stringToUTF8 (text, ptr, length);
            }

            dynCall ("vp", pasteCallback, [ptr]);
        }

        window.addEventListener ("keydown", function (event)
        {
            if (! isPasteChord (event))
                return;

            event.stopImmediatePropagation();

            if (pendingTimer === null)
                pendingTimer = setTimeout (function() { dispatchPaste (null); }, 150);
        }, true);

        window.addEventListener ("keypress", function (event)
        {
            if (isPasteChord (event))
                event.stopImmediatePropagation();
        }, true);

        window.addEventListener ("paste", function (event)
        {
            if (pendingTimer === null)
                return;

            event.preventDefault();
            dispatchPaste (event.clipboardData ? event.clipboardData.getData ("text/plain") : null);
        }, true);
    }, reinterpret_cast<void*> (&handleBrowserPaste));
    // clang-format on
}

} // namespace

} // namespace yup
