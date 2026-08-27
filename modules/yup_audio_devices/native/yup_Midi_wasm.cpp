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

#if YUP_EMSCRIPTEN

//==============================================================================
// JavaScript bridge for the Web MIDI API (https://www.w3.org/TR/webmidi/).
// clang-format off

EM_JS (void, yupWebMidiRequestAccess, (), {
    if (typeof navigator === 'undefined' || typeof navigator.requestMIDIAccess !== 'function')
    {
        Module._yupWebMidiAccessRejected();
        return;
    }

    navigator.requestMIDIAccess ({ sysex: true }).then (function (access)
    {
        Module.yupMidiAccess = access;
        Module.yupMidiInputs = new Map();
        Module.yupMidiOutputs = new Map();

        var onStateChange = function (event)
        {
            var port = event.port;
            var isInput = (port.type === 'input');
            var map = isInput ? Module.yupMidiInputs : Module.yupMidiOutputs;

            if (port.state === 'connected')
            {
                map.set (port.id, port);
                Module.ccall ('yupWebMidiPortAdded', null, ['string', 'string', 'number'],
                              [port.id, port.name || port.id, isInput ? 1 : 0]);
            }
            else
            {
                map.delete (port.id);
                Module.ccall ('yupWebMidiPortRemoved', null, ['string', 'number'],
                              [port.id, isInput ? 1 : 0]);
            }
        };

        access.onstatechange = onStateChange;

        access.inputs.forEach (function (port) { onStateChange ({ port: port }); });
        access.outputs.forEach (function (port) { onStateChange ({ port: port }); });

        Module._yupWebMidiAccessResolved (access.sysexEnabled ? 1 : 0);
    }).catch (function (error)
    {
        console.error ('Web MIDI access denied or unavailable:', error);
        Module._yupWebMidiAccessRejected();
    });
});

EM_JS (void, yupWebMidiSetInputHandler, (const char* portId), {
    var id = UTF8ToString (portId);
    var port = Module.yupMidiInputs && Module.yupMidiInputs.get (id);

    if (! port)
        return;

    port.onmidimessage = function (event)
    {
        if (! event.data || event.data.length === 0)
            return;

        var len = event.data.length;
        var ptr = Module._yupWebMidiGetScratch (len);

        if (! ptr)
            return;

        HEAPU8.set (event.data, ptr);
        Module.ccall ('yupWebMidiMessageReceived', null, ['string', 'number', 'number'], [id, ptr, len]);
    };
});

EM_JS (void, yupWebMidiOpenPort, (const char* portId), {
    var id = UTF8ToString (portId);
    var port = (Module.yupMidiInputs && Module.yupMidiInputs.get (id)) || (Module.yupMidiOutputs && Module.yupMidiOutputs.get (id));

    if (port)
        port.open().catch (function (error) { console.warn ('Could not open MIDI port:', error); });
});

EM_JS (void, yupWebMidiClosePort, (const char* portId), {
    var id = UTF8ToString (portId);
    var port = (Module.yupMidiInputs && Module.yupMidiInputs.get (id)) || (Module.yupMidiOutputs && Module.yupMidiOutputs.get (id));

    if (port)
        port.close().catch (function (error) { console.warn ('Could not close MIDI port:', error); });
});

EM_JS (void, yupWebMidiSend, (const char* portId, const uint8_t* data, int len), {
    var id = UTF8ToString (portId);
    var port = Module.yupMidiOutputs && Module.yupMidiOutputs.get (id);

    if (! port || len <= 0)
        return;

    try
    {
        port.send (new Uint8Array (HEAPU8.buffer, data, len));
    }
    catch (error)
    {
        console.warn ('Could not send MIDI message:', error);
    }
});

// clang-format on

//==============================================================================
namespace
{

void runOnMainThread (void (*fn) (void*), void* arg)
{
#if defined(__EMSCRIPTEN_PTHREADS__)
    if (emscripten_is_main_runtime_thread())
    {
        fn (arg);
    }
    else
    {
        emscripten_sync_run_in_main_runtime_thread (EM_FUNC_SIG_VI, fn, arg);
    }
#else
    fn (arg);
#endif
}

void webMidiRequestAccess()
{
    runOnMainThread ([] (void*)
    {
        yupWebMidiRequestAccess();
    },
                     nullptr);
}

void webMidiSetInputHandler (const String& portId)
{
    struct Args
    {
        const char* text;
    } args { portId.toRawUTF8() };

    runOnMainThread ([] (void* context)
    {
        yupWebMidiSetInputHandler (static_cast<Args*> (context)->text);
    },
                     &args);
}

void webMidiOpenPort (const String& portId)
{
    struct Args
    {
        const char* text;
    } args { portId.toRawUTF8() };

    runOnMainThread ([] (void* context)
    {
        yupWebMidiOpenPort (static_cast<Args*> (context)->text);
    },
                     &args);
}

void webMidiClosePort (const String& portId)
{
    struct Args
    {
        const char* text;
    } args { portId.toRawUTF8() };

    runOnMainThread ([] (void* context)
    {
        yupWebMidiClosePort (static_cast<Args*> (context)->text);
    },
                     &args);
}

void webMidiSend (const String& portId, const void* data, size_t len)
{
    struct Args
    {
        const char* text;
        const uint8_t* data;
        size_t len;
    } args { portId.toRawUTF8(), static_cast<const uint8_t*> (data), len };

    runOnMainThread ([] (void* context)
    {
        const auto& a = *static_cast<Args*> (context);
        yupWebMidiSend (a.text, a.data, static_cast<int> (a.len));
    },
                     &args);
}

//==============================================================================
CriticalSection midiCallbackLock;
std::map<String, MidiInput::Pimpl*> activeInputs;

void registerInput (const String& portId, MidiInput::Pimpl* pimpl)
{
    const ScopedLock sl (midiCallbackLock);

    if (auto [it, inserted] = activeInputs.emplace (portId, pimpl); ! inserted)
    {
        if (it->second != pimpl)
        {
            jassertfalse; // A second input device for the same port is already open
            it->second = pimpl;
        }
    }
}

void unregisterInput (const String& portId, MidiInput::Pimpl* pimpl)
{
    const ScopedLock sl (midiCallbackLock);

    if (auto it = activeInputs.find (portId); it != activeInputs.end() && it->second == pimpl)
        activeInputs.erase (it);
}

//==============================================================================
class WebMidiAccess
{
public:
    static WebMidiAccess& get()
    {
        static WebMidiAccess instance;
        return instance;
    }

    void ensureRequested()
    {
        auto expected = state.load();

        if (expected != State::unrequested && expected != State::failed)
            return;

        if (state.compare_exchange_strong (expected, State::requested))
            webMidiRequestAccess();
    }

    Array<MidiDeviceInfo> getDevices (bool isInput)
    {
        ensureRequested();

        const ScopedLock sl (lock);
        return isInput ? inputDevices : outputDevices;
    }

    const MidiDeviceInfo* findDevice (bool isInput, const String& identifier)
    {
        const ScopedLock sl (lock);
        return findDeviceUnlocked (isInput ? inputDevices : outputDevices, identifier);
    }

    void portAdded (const String& id, const String& name, bool isInput)
    {
        {
            const ScopedLock sl (lock);
            auto& devices = isInput ? inputDevices : outputDevices;

            if (findDeviceUnlocked (devices, id) == nullptr)
                devices.add (MidiDeviceInfo (name, id));
        }

        MidiDeviceListConnectionBroadcaster::get().notify();
    }

    void portRemoved (const String& id, bool isInput)
    {
        {
            const ScopedLock sl (lock);
            auto& devices = isInput ? inputDevices : outputDevices;
            devices.removeIf ([&] (const MidiDeviceInfo& device)
            {
                return device.identifier == id;
            });
        }

        MidiDeviceListConnectionBroadcaster::get().notify();
    }

    void accessResolved()
    {
        state = State::resolved;
        MidiDeviceListConnectionBroadcaster::get().notify();
    }

    void accessRejected()
    {
        state = State::failed;
    }

    uint8_t* getScratch (size_t len)
    {
        if (scratch.size() < len)
            scratch.resize (len);

        return scratch.data();
    }

private:
    WebMidiAccess() = default;

    static const MidiDeviceInfo* findDeviceUnlocked (const Array<MidiDeviceInfo>& devices, const String& identifier)
    {
        for (const auto& device : devices)
            if (device.identifier == identifier)
                return &device;

        return nullptr;
    }

    enum class State
    {
        unrequested,
        requested,
        resolved,
        failed
    };

    std::atomic<State> state { State::unrequested };
    CriticalSection lock;
    Array<MidiDeviceInfo> inputDevices, outputDevices;
    std::vector<uint8_t> scratch;
};

} // namespace

//==============================================================================
class MidiInput::Pimpl
{
public:
    Pimpl (MidiInput& input, MidiInputCallback& callback)
        : portId (input.getIdentifier())
        , handler (std::make_unique<ump::BytestreamToBytestreamHandler> (input, callback))
    {
    }

    Pimpl (MidiInput& input, ump::PacketProtocol protocol, ump::Receiver& receiver)
        : portId (input.getIdentifier())
        , handler (std::make_unique<ump::BytestreamToUMPHandler> (protocol, receiver))
    {
    }

    ~Pimpl()
    {
        stop();
        unregisterInput (portId, this);
    }

    void start()
    {
        if (! active.exchange (true))
        {
            registerInput (portId, this);
            webMidiSetInputHandler (portId);
        }

        webMidiOpenPort (portId);
    }

    void stop()
    {
        if (active.exchange (false))
        {
            webMidiClosePort (portId);
            handler->reset();
        }
    }

    void handleMessage (const uint8_t* data, int len, double time) const
    {
        if (active)
            handler->pushMidiData (data, len, time);
    }

    String portId;
    std::unique_ptr<ump::BytestreamInputHandler> handler;
    std::atomic<bool> active { false };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

//==============================================================================
class MidiOutput::Pimpl
{
public:
    explicit Pimpl (String portIdIn)
        : portId (std::move (portIdIn))
    {
    }

    void send (const uint8_t* data, size_t len)
    {
        webMidiSend (portId, data, len);
    }

    String portId;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
};

//==============================================================================
extern "C"
{
    void EMSCRIPTEN_KEEPALIVE yupWebMidiAccessResolved (int)
    {
        WebMidiAccess::get().accessResolved();
    }

    void EMSCRIPTEN_KEEPALIVE yupWebMidiAccessRejected()
    {
        WebMidiAccess::get().accessRejected();
    }

    void EMSCRIPTEN_KEEPALIVE yupWebMidiPortAdded (const char* id, const char* name, int isInput)
    {
        WebMidiAccess::get().portAdded (String::fromUTF8 (id), String::fromUTF8 (name), isInput != 0);
    }

    void EMSCRIPTEN_KEEPALIVE yupWebMidiPortRemoved (const char* id, int isInput)
    {
        WebMidiAccess::get().portRemoved (String::fromUTF8 (id), isInput != 0);
    }

    void EMSCRIPTEN_KEEPALIVE yupWebMidiMessageReceived (const char* portId, const uint8_t* data, int len)
    {
        const String id (String::fromUTF8 (portId));
        const double time = Time::getMillisecondCounterHiRes() * 0.001;

        const ScopedLock sl (midiCallbackLock);

        if (auto it = activeInputs.find (id); it != activeInputs.end())
            it->second->handleMessage (data, len, time);
    }

    uint8_t* EMSCRIPTEN_KEEPALIVE yupWebMidiGetScratch (int len)
    {
        return WebMidiAccess::get().getScratch (static_cast<size_t> (len));
    }

} // extern "C"

//==============================================================================
Array<MidiDeviceInfo> MidiInput::getAvailableDevices()
{
    return WebMidiAccess::get().getDevices (true);
}

MidiDeviceInfo MidiInput::getDefaultDevice()
{
    return getAvailableDevices().getFirst();
}

std::unique_ptr<MidiInput> MidiInput::openDevice (const String& deviceIdentifier, MidiInputCallback* callback)
{
    auto& access = WebMidiAccess::get();
    access.ensureRequested();

    if (deviceIdentifier.isEmpty() || callback == nullptr)
        return {};

    const auto* info = access.findDevice (true, deviceIdentifier);

    if (info == nullptr)
        return {};

    std::unique_ptr<MidiInput> midiInput (new MidiInput (info->name, deviceIdentifier, ump::PacketProtocol::MIDI_1_0));
    midiInput->internal = std::make_unique<MidiInput::Pimpl> (*midiInput, *callback);

    return midiInput;
}

std::unique_ptr<MidiInput> MidiInput::openDevice (const String& deviceIdentifier,
                                                  ump::PacketProtocol protocol,
                                                  ump::Receiver* receiver)
{
    auto& access = WebMidiAccess::get();
    access.ensureRequested();

    if (deviceIdentifier.isEmpty() || receiver == nullptr)
        return {};

    const auto* info = access.findDevice (true, deviceIdentifier);

    if (info == nullptr)
        return {};

    std::unique_ptr<MidiInput> midiInput (new MidiInput (info->name, deviceIdentifier, protocol));
    midiInput->internal = std::make_unique<MidiInput::Pimpl> (*midiInput, protocol, *receiver);

    return midiInput;
}

std::unique_ptr<MidiInput> MidiInput::createNewDevice (const String&, MidiInputCallback*)
{
    // The Web MIDI API does not allow creating virtual devices
    return {};
}

std::unique_ptr<MidiInput> MidiInput::createNewDevice (const String&, ump::PacketProtocol, ump::Receiver*)
{
    // The Web MIDI API does not allow creating virtual devices
    return {};
}

//==============================================================================
Array<MidiDeviceInfo> MidiOutput::getAvailableDevices()
{
    return WebMidiAccess::get().getDevices (false);
}

MidiDeviceInfo MidiOutput::getDefaultDevice()
{
    return getAvailableDevices().getFirst();
}

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String& deviceIdentifier)
{
    auto& access = WebMidiAccess::get();
    access.ensureRequested();

    if (deviceIdentifier.isEmpty())
        return {};

    const auto* info = access.findDevice (false, deviceIdentifier);

    if (info == nullptr)
        return {};

    std::unique_ptr<MidiOutput> midiOutput (new MidiOutput (info->name, deviceIdentifier, ump::PacketProtocol::MIDI_1_0));
    midiOutput->internal = std::make_unique<MidiOutput::Pimpl> (deviceIdentifier);

    return midiOutput;
}

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String& deviceIdentifier, ump::PacketProtocol protocol)
{
    if (protocol != ump::PacketProtocol::MIDI_1_0)
        return {};

    return openDevice (deviceIdentifier);
}

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice (const String&)
{
    // The Web MIDI API does not allow creating virtual devices
    return {};
}

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice (const String&, ump::PacketProtocol)
{
    // The Web MIDI API does not allow creating virtual devices
    return {};
}

MidiDeviceListConnection MidiDeviceListConnection::make (std::function<void()> callback)
{
    // AudioDeviceManager constructs this member from any thread (e.g. in tests),
    // so only connect to the broadcaster when a message thread exists.
    if (MessageManager::existsAndIsCurrentThread())
    {
        auto& broadcaster = MidiDeviceListConnectionBroadcaster::get();
        return { &broadcaster, broadcaster.add (std::move (callback)) };
    }

    return { nullptr, 0 };
}

#else // !YUP_EMSCRIPTEN

class MidiInput::Pimpl
{
public:
    void start() {}

    void stop() {}
};

class MidiOutput::Pimpl
{
public:
    void send (const uint8_t*, size_t) {}
};

Array<MidiDeviceInfo> MidiInput::getAvailableDevices() { return {}; }

MidiDeviceInfo MidiInput::getDefaultDevice() { return {}; }

std::unique_ptr<MidiInput> MidiInput::openDevice (const String&, MidiInputCallback*) { return {}; }

std::unique_ptr<MidiInput> MidiInput::openDevice (const String&, ump::PacketProtocol, ump::Receiver*) { return {}; }

std::unique_ptr<MidiInput> MidiInput::createNewDevice (const String&, MidiInputCallback*) { return {}; }

std::unique_ptr<MidiInput> MidiInput::createNewDevice (const String&, ump::PacketProtocol, ump::Receiver*) { return {}; }

Array<MidiDeviceInfo> MidiOutput::getAvailableDevices() { return {}; }

MidiDeviceInfo MidiOutput::getDefaultDevice() { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String&) { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String&, ump::PacketProtocol) { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice (const String&) { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice (const String&, ump::PacketProtocol) { return {}; }

MidiDeviceListConnection MidiDeviceListConnection::make (std::function<void()>)
{
    // MIDI is not implemented for non-Emscripten WASM targets
    return { nullptr, 0 };
}

#endif // YUP_EMSCRIPTEN

//==============================================================================
// Shared implementations (identical on every WASM target)

MidiInput::MidiInput (const String& deviceName,
                      const String& deviceIdentifier,
                      ump::PacketProtocol protocol)
    : deviceInfo (deviceName, deviceIdentifier, protocol)
{
}

MidiInput::~MidiInput() = default;

void MidiInput::start()
{
    if (auto* mi = internal.get())
        mi->start();
}

void MidiInput::stop()
{
    if (auto* mi = internal.get())
        mi->stop();
}

//==============================================================================
MidiOutput::~MidiOutput()
{
    stopBackgroundThread();
}

void MidiOutput::sendMessageNow (const MidiMessage& message)
{
    if (auto* pimpl = internal.get())
        pimpl->send (static_cast<const uint8_t*> (message.getRawData()), static_cast<size_t> (message.getRawDataSize()));
}

void MidiOutput::sendMessageNow (const ump::View& message)
{
    ump::ToBytestreamConverter converter { 2048 };

    converter.convert (message, 0.0, [this] (const MidiMessage& midiMessage)
    {
        sendMessageNow (midiMessage);
    });
}

void MidiOutput::sendMessageNow (const ump::Packets& packets)
{
    for (auto it = packets.cbegin(); it != packets.cend(); ++it)
        sendMessageNow (*it);
}

} // namespace yup
