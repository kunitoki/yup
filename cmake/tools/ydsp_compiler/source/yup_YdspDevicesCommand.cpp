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

#include "yup_YdspCommands.h"
#include <yup_audio_devices/yup_audio_devices.h>

#include <iostream>

using namespace yup;

static int listDevices (bool json)
{
    AudioDeviceManager manager;
    Array<var> audio;
    for (auto* type : manager.getAvailableDeviceTypes())
    {
        type->scanForDevices();
        for (bool input : { true, false })
            for (const auto& name : type->getDeviceNames (input))
            {
                auto* record = new DynamicObject();
                record->setProperty ("type", type->getTypeName());
                record->setProperty ("direction", input ? "input" : "output");
                record->setProperty ("name", name);
                audio.add (var (record));
                if (! json)
                    std::cout << "audio " << (input ? "input " : "output ") << type->getTypeName() << ": " << name << "\n";
            }
    }
    Array<var> midi;
    for (bool input : { true, false })
        for (const auto& device : input ? MidiInput::getAvailableDevices() : MidiOutput::getAvailableDevices())
        {
            auto* record = new DynamicObject();
            record->setProperty ("direction", input ? "input" : "output");
            record->setProperty ("name", device.name);
            record->setProperty ("id", device.identifier);
            midi.add (var (record));
            if (! json)
                std::cout << "midi " << (input ? "input " : "output ") << device.name << " [" << device.identifier << "]\n";
        }
    if (json)
    {
        auto* result = new DynamicObject();
        result->setProperty ("audio", var (audio));
        result->setProperty ("midi", var (midi));
        std::cout << JSON::toString (var (result)).toStdString() << "\n";
    }
    return 0;
}

int runYdspDevicesCommand (int argc, char** argv)
{
    if (argc > 3 || (argc == 3 && String (argv[2]) != "--json"))
    {
        std::cerr << "Usage: yup_dsp_compiler devices [--json]\n";
        return 2;
    }
    ScopedYupInitialiser_GUI init;
    return listDevices (argc == 3);
}
