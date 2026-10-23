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

#include <functional>

//==============================================================================
/** The on-screen keyboard, which keeps the keyboard focus so the computer keys play it.

    Holding the focus, it is also the one component that hears the interface's shortcuts,
    so it hands every key pressed with command or control to onShortcut.

    @see SynthInterface
*/
class SynthKeyboard : public yup::MidiKeyboardComponent
{
public:
    using yup::MidiKeyboardComponent::MidiKeyboardComponent;

    /** Called with every key pressed with command or control held. */
    std::function<void (const yup::KeyPress&)> onShortcut;

    void keyDown (const yup::KeyPress& key, const yup::Point<float>& position) override;
};
