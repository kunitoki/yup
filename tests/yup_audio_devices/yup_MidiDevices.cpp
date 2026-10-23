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

#include <gtest/gtest.h>

#include <yup_audio_devices/yup_audio_devices.h>

#include <optional>

using namespace yup;

//==============================================================================
class MidiDevicesTests : public ::testing::Test
{
protected:
    struct CountingListener final : ump::DisconnectionListener
    {
        void disconnected() override { ++count; }

        int count = 0;
    };

    struct IgnoringCallback final : MidiInputCallback
    {
        void handleIncomingMidiMessage (MidiInput*, const MidiMessage&) override {}
    };

    static void pumpMessages (int milliseconds)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        MessageManager::getInstance()->runDispatchLoopUntil (milliseconds);
#else
        Thread::sleep (milliseconds);
#endif
    }

    template <typename Condition>
    static bool waitFor (Condition&& condition)
    {
        for (int attempt = 0; attempt < 100 && ! condition(); ++attempt)
            pumpMessages (20);

        return condition();
    }

    static std::optional<MidiDeviceInfo> findInput (const String& name)
    {
        for (const auto& device : MidiInput::getAvailableDevices())
            if (device.name == name)
                return device;

        return std::nullopt;
    }

    static String makeUniqueDeviceName()
    {
        return "YUP Disconnection Test " + String (Time::currentTimeMillis());
    }
};

#if YUP_MAC || YUP_LINUX
TEST_F (MidiDevicesTests, InputReportsDisconnectionOnceWhenItsSourceGoesAway)
{
    const auto name = makeUniqueDeviceName();

    auto source = MidiOutput::createNewDevice (name);
    if (source == nullptr)
        GTEST_SKIP() << "Virtual MIDI devices are not available";

    std::optional<MidiDeviceInfo> info;
    if (! waitFor ([&] { return (info = findInput (name)).has_value(); }))
        GTEST_SKIP() << "The virtual MIDI source was not listed";

    IgnoringCallback callback;
    auto input = MidiInput::openDevice (info->identifier, &callback);
    ASSERT_NE (nullptr, input);

    CountingListener listener;
    input->addDisconnectionListener (listener);
    EXPECT_FALSE (input->isDisconnected());

    source.reset();

    EXPECT_TRUE (waitFor ([&] { return listener.count > 0; }));
    EXPECT_TRUE (input->isDisconnected());

    pumpMessages (100);
    EXPECT_EQ (1, listener.count);

    input->removeDisconnectionListener (listener);
}
#endif
