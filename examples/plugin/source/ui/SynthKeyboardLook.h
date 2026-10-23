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

#include <yup_audio_gui/yup_audio_gui.h>

//==============================================================================
/** The physical look of the playing surface: plastic keys and rubber wheels sunk in
    their slots. The keys keep their white and black on every palette, pressed keys take
    the accent.
*/
namespace SynthKeyboardLook
{
/** Draws a horizontal keyboard with plastic keys. */
void apply (yup::MidiKeyboardComponent& keyboard);

/** Draws a pitch wheel as a rubber wheel in a slot, centered at rest. */
void apply (yup::PitchWheelComponent& wheel);

/** Draws a modulation wheel as a rubber wheel in a slot. */
void apply (yup::ModWheelComponent& wheel);
} // namespace SynthKeyboardLook
