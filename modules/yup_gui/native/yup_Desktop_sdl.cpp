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

        // Display geometry is reported in native units, convert it to logical points
        // so it lives in the same coordinate space as the component bounds
        const float unitsPerPoint = getDisplayUnitsPerPoint (displayID);

        auto screen = std::make_unique<Screen>();
        screen->name = String::fromUTF8 (SDL_GetDisplayName (displayID));
        screen->isPrimary = (displayID == primaryDisplay);
        screen->virtualPosition = Point<int> (roundToInt (bounds.x / unitsPerPoint),
                                              roundToInt (bounds.y / unitsPerPoint));
        screen->workArea = Rectangle<int> (roundToInt (usableBounds.x / unitsPerPoint),
                                           roundToInt (usableBounds.y / unitsPerPoint),
                                           roundToInt (usableBounds.w / unitsPerPoint),
                                           roundToInt (usableBounds.h / unitsPerPoint));

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

    const SDL_Point point { static_cast<int> (x), static_cast<int> (y) };
    return Point<float> (x, y) / getDisplayUnitsPerPoint (SDL_GetDisplayForPoint (&point));
}

void Desktop::setCurrentMouseLocation (const Point<float>& location)
{
    // Mixed dpi setups are approximated by the primary display scale here
    const auto scale = getDisplayUnitsPerPoint (SDL_GetPrimaryDisplay());

    SDL_WarpMouseGlobal (location.getX() * scale, location.getY() * scale);
}

} // namespace yup
