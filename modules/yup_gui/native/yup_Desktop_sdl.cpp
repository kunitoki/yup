/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

void Desktop::updateScreens()
{
    int numScreens = 0;
    SDL_DisplayID* displays = SDL_GetDisplays (&numScreens);
    if (displays == nullptr)
        return;

    screens.clear();
    const SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();

    for (int i = 0; i < numScreens; ++i)
    {
        const SDL_DisplayID displayID = displays[i];

        SDL_Rect bounds;
        if (! SDL_GetDisplayBounds (displayID, &bounds))
            continue;

        SDL_Rect usableBounds = bounds;
        SDL_GetDisplayUsableBounds (displayID, &usableBounds);

        auto screen = std::make_unique<Screen>();
        screen->name = String::fromUTF8 (SDL_GetDisplayName (displayID));
        screen->isPrimary = (displayID == primaryDisplay);
        screen->virtualPosition = Point<int> (bounds.x, bounds.y);
        screen->workArea = Rectangle<int> (usableBounds.x, usableBounds.y, usableBounds.w, usableBounds.h);

        const float contentScale = SDL_GetDisplayContentScale (displayID);
        screen->contentScaleX = contentScale > 0.0f ? contentScale : 1.0f;
        screen->contentScaleY = screen->contentScaleX;

        screens.add (screen.release());
    }

    SDL_free (displays);
}

//==============================================================================

void Desktop::setMouseCursor (const MouseCursor& cursorToSet)
{
    static const auto cursors = []
    {
        return std::unordered_map<MouseCursor::Type, SDL_Cursor*> {
            { MouseCursor::Default, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_DEFAULT) },
            { MouseCursor::Text, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_TEXT) },
            { MouseCursor::Wait, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_WAIT) },
            { MouseCursor::WaitArrow, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_PROGRESS) },
            { MouseCursor::Hand, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_POINTER) },
            { MouseCursor::Crosshair, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_CROSSHAIR) },
            { MouseCursor::Crossbones, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_NOT_ALLOWED) },
            { MouseCursor::ResizeLeftRight, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_EW_RESIZE) },
            { MouseCursor::ResizeUpDown, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_NS_RESIZE) },
            { MouseCursor::ResizeTopLeftRightBottom, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_NWSE_RESIZE) },
            { MouseCursor::ResizeBottomLeftRightTop, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_NESW_RESIZE) },
            { MouseCursor::ResizeAll, SDL_CreateSystemCursor (SDL_SYSTEM_CURSOR_MOVE) }
        };
    }();

    currentMouseCursor = cursorToSet;

    if (cursorToSet.getType() == MouseCursor::None)
    {
        SDL_HideCursor();
    }
    else
    {
        auto it = cursors.find (cursorToSet.getType());
        if (it != cursors.end())
            SDL_SetCursor (it->second);

        SDL_ShowCursor();
    }
}

//==============================================================================

Point<float> Desktop::getCurrentMouseLocation() const
{
    float x = 0.0f, y = 0.0f;

    SDL_GetGlobalMouseState (&x, &y);

    return { x, y };
}

void Desktop::setCurrentMouseLocation (const Point<float>& location)
{
    SDL_WarpMouseGlobal (location.getX(), location.getY());
}

} // namespace yup
