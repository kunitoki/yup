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

namespace juce
{

class MidiInput::Pimpl
{
};

MidiInput::MidiInput (const String& deviceName, const String& deviceID)
    : deviceInfo (deviceName, deviceID)
{
}

MidiInput::~MidiInput() {}

void MidiInput::start() {}

void MidiInput::stop() {}

Array<MidiDeviceInfo> MidiInput::getAvailableDevices() { return {}; }

MidiDeviceInfo MidiInput::getDefaultDevice() { return {}; }

std::unique_ptr<MidiInput> MidiInput::openDevice (const String&, MidiInputCallback*) { return {}; }

std::unique_ptr<MidiInput> MidiInput::createNewDevice (const String&, MidiInputCallback*) { return {}; }

class MidiOutput::Pimpl
{
};

MidiOutput::~MidiOutput() {}

void MidiOutput::sendMessageNow (const MidiMessage&) {}

Array<MidiDeviceInfo> MidiOutput::getAvailableDevices() { return {}; }

MidiDeviceInfo MidiOutput::getDefaultDevice() { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String&) { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice (const String&) { return {}; }

MidiDeviceListConnection MidiDeviceListConnection::make (std::function<void()> cb)
{
    auto& broadcaster = MidiDeviceListConnectionBroadcaster::get();
    return { &broadcaster, broadcaster.add (std::move (cb)) };
}

} // namespace juce
