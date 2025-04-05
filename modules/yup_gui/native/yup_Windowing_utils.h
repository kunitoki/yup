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

MouseEvent::Buttons toMouseButton (Uint8 sdlButton) noexcept;

KeyModifiers toKeyModifiers (Uint16 sdlMod) noexcept;

KeyPress toKeyPress (SDL_Keycode key, SDL_Scancode scancode, KeyModifiers modifiers) noexcept;

//==============================================================================

bool isMouseOutsideWindow (SDL_Window* window);

//==============================================================================

void* getNativeWindowHandle (SDL_Window* window);

Rectangle<int> getNativeWindowPosition (void* nativeDisplay, void* nativeWindow);

void setNativeParent (void* nativeDisplay, void* nativeWindow, SDL_Window* window);

//==============================================================================

GraphicsContext::Api getGraphicsContextApi (const std::optional<GraphicsContext::Api>& forceContextApi);

Uint32 setContextWindowHints (GraphicsContext::Api desiredApi);

} // namespace yup
