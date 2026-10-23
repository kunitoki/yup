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

namespace yup::ump
{

//==============================================================================
/**
    Receives a notification when the device behind an open MidiInput or MidiOutput
    goes away, for example because it was unplugged.

    @see MidiInput::addDisconnectionListener, MidiOutput::addDisconnectionListener
*/
struct YUP_API DisconnectionListener
{
    virtual ~DisconnectionListener() = default;

    /** Called once, on the message thread, after the device has disappeared.

        The device object that reported it never delivers or sends again: open a new
        one when the device comes back. Don't destroy that device object from inside
        this callback.
    */
    virtual void disconnected() = 0;
};

} // namespace yup::ump
