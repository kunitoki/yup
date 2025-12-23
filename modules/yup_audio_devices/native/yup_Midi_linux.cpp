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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <array>
#include <cstring>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__has_include)
#if __has_include(<alsa/ump.h>)
#include <alsa/ump.h>
#define YUP_HAS_ALSA_UMP 1
#endif
#endif

#ifndef YUP_HAS_ALSA_UMP
#define YUP_HAS_ALSA_UMP 0
#endif

namespace yup
{

#if YUP_ALSA

namespace
{
constexpr const char* umpRawmidiPrefix = "ump-rawmidi:";
constexpr const char* umpSequencerPrefix = "ump-seq:";

bool isUmpRawmidiIdentifier (const String& identifier)
{
    return identifier.startsWith (umpRawmidiPrefix);
}

bool isUmpSequencerIdentifier (const String& identifier)
{
    return identifier.startsWith (umpSequencerPrefix);
}

String getUmpRawmidiPath (const String& identifier)
{
    return identifier.fromFirstOccurrenceOf (umpRawmidiPrefix, false, false);
}

String getUmpRawmidiIdentifier (const String& path)
{
    return String (umpRawmidiPrefix) + path;
}

String getUmpSequencerIdentifier (const String& identifier)
{
    return String (umpSequencerPrefix) + identifier;
}

Array<File> findUmpRawmidiNodes()
{
    Array<File> results;
    File ("/dev/snd").findChildFiles (results, File::findFiles, false, "umpC*D*");
    return results;
}

bool parseUmpRawmidiPath (const String& path, int& card, int& device)
{
    const auto name = File (path).getFileName();
    if (! name.startsWith ("umpC"))
        return false;

    const auto dIndex = name.indexOfChar ('D');
    if (dIndex <= 4)
        return false;

    const auto cardString = name.substring (4, dIndex);
    const auto deviceString = name.substring (dIndex + 1);

    if (! cardString.containsOnly ("0123456789") || ! deviceString.containsOnly ("0123456789"))
        return false;

    card = cardString.getIntValue();
    device = deviceString.getIntValue();
    return true;
}

String getUmpAlsaDeviceName (const String& path)
{
    int card = 0;
    int device = 0;
    if (! parseUmpRawmidiPath (path, card, device))
        return {};

    return "hw:" + String (card) + "," + String (device);
}
} // namespace

//==============================================================================
class AlsaClient
{
    auto lowerBound (int portId) const
    {
        const auto comparator = [] (const auto& port, const auto& id)
        {
            return port->getPortId() < id;
        };
        return std::lower_bound (ports.begin(), ports.end(), portId, comparator);
    }

    auto findPortIterator (int portId) const
    {
        const auto iter = lowerBound (portId);
        return (iter == ports.end() || (*iter)->getPortId() != portId) ? ports.end() : iter;
    }

public:
    ~AlsaClient()
    {
        inputThread.reset();

        jassert (activeCallbacks.get() == 0);

        if (handle != nullptr)
        {
            snd_seq_delete_simple_port (handle, announcementsIn);
            snd_seq_close (handle);
        }
    }

    static String getAlsaMidiName()
    {
#ifdef YUP_ALSA_MIDI_NAME
        return YUP_ALSA_MIDI_NAME;
#else
        if (auto* app = YUPApplicationBase::getInstance())
            return app->getApplicationName();

        return "YUP";
#endif
    }

    //==============================================================================
    // represents an input or output port of the supplied AlsaClient
    struct Port
    {
        explicit Port (bool forInput) noexcept
            : isInput (forInput)
        {
        }

        ~Port()
        {
            if (isValid())
            {
                if (isInput)
                    enableCallback (false);
                else
                    snd_midi_event_free (midiParser);

                snd_seq_delete_simple_port (client->get(), portId);
            }
        }

        void connectWith (int sourceClient, int sourcePort) const noexcept
        {
            if (isInput)
                snd_seq_connect_from (client->get(), portId, sourceClient, sourcePort);
            else
                snd_seq_connect_to (client->get(), portId, sourceClient, sourcePort);
        }

        bool isValid() const noexcept
        {
            return client->get() != nullptr && portId >= 0;
        }

        void setupInput (MidiInput* input, MidiInputCallback* cb)
        {
            jassert (cb != nullptr && input != nullptr);
            callback = cb;
            midiInput = input;
            umpReceiver = nullptr;
            umpConverter.reset();
            umpToBytestream = std::make_unique<ump::ToBytestreamConverter> (2048);
        }

        void setupInputUMP (MidiInput* input, ump::Receiver* receiverIn, ump::PacketProtocol protocolIn)
        {
            jassert (receiverIn != nullptr && input != nullptr);
            callback = nullptr;
            midiInput = input;
            umpReceiver = receiverIn;
            umpProtocol = protocolIn;
            umpConverter = std::make_unique<ump::GenericUMPConverter> (protocolIn);
        }

        void setupOutput()
        {
            jassert (! isInput);
            snd_midi_event_new ((size_t) maxEventSize, &midiParser);
        }

        void enableCallback (bool enable)
        {
            callbackEnabled = enable;
        }

        bool sendMessageNow (const MidiMessage& message)
        {
            if (message.getRawDataSize() > maxEventSize)
            {
                maxEventSize = message.getRawDataSize();
                snd_midi_event_free (midiParser);
                snd_midi_event_new ((size_t) maxEventSize, &midiParser);
            }

            snd_seq_event_t event;
            snd_seq_ev_clear (&event);

            auto numBytes = (long) message.getRawDataSize();
            auto* data = message.getRawData();

            auto seqHandle = client->get();
            bool success = true;

            while (numBytes > 0)
            {
                auto numSent = snd_midi_event_encode (midiParser, data, numBytes, &event);

                if (numSent <= 0)
                {
                    success = numSent == 0;
                    break;
                }

                numBytes -= numSent;
                data += numSent;

                snd_seq_ev_set_source (&event, (unsigned char) portId);
                snd_seq_ev_set_subs (&event);
                snd_seq_ev_set_direct (&event);

                if (snd_seq_event_output_direct (seqHandle, &event) < 0)
                {
                    success = false;
                    break;
                }
            }

            snd_midi_event_reset_encode (midiParser);
            return success;
        }

        bool sendUmpMessageNow (const uint32_t* words, uint32_t numWords)
        {
#if defined(SND_SEQ_EVENT_UMP)
            if (words == nullptr || numWords == 0 || numWords > 4)
                return false;

            snd_seq_event_t event;
            snd_seq_ev_clear (&event);
            event.type = 0;
            event.flags |= SND_SEQ_EVENT_UMP;

            std::array<uint32_t, 4> payload {};
            std::memcpy (payload.data(), words, sizeof (uint32_t) * numWords);
            snd_seq_ev_set_variable (&event, (unsigned int) (numWords * sizeof (uint32_t)), payload.data());

            snd_seq_ev_set_source (&event, (unsigned char) portId);
            snd_seq_ev_set_subs (&event);
            snd_seq_ev_set_direct (&event);

            return snd_seq_event_output_direct (client->get(), &event) >= 0;
#else
            (void) words;
            (void) numWords;
            return false;
#endif
        }

        bool operator== (const Port& lhs) const noexcept
        {
            return portId != -1 && portId == lhs.portId;
        }

        void createPort (const String& name, bool enableSubscription)
        {
            if (auto seqHandle = client->get())
            {
                const unsigned int caps =
                    isInput ? (SND_SEQ_PORT_CAP_WRITE | (enableSubscription ? SND_SEQ_PORT_CAP_SUBS_WRITE : 0))
                            : (SND_SEQ_PORT_CAP_READ | (enableSubscription ? SND_SEQ_PORT_CAP_SUBS_READ : 0));

                portName = name;
                portId = snd_seq_create_simple_port (seqHandle, portName.toUTF8(), caps, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
            }
        }

        void handleIncomingMidiMessage (const MidiMessage& message) const
        {
            if (! callbackEnabled)
                return;

            if (callback != nullptr)
            {
                callback->handleIncomingMidiMessage (midiInput, message);
                return;
            }

            if (umpReceiver != nullptr && umpConverter != nullptr)
            {
                const auto timestamp = message.getTimeStamp();
                umpConverter->convert (ump::BytestreamMidiView (&message), [&] (const ump::View& view)
                {
                    umpReceiver->packetReceived (view, timestamp);
                });
            }
        }

        void handlePartialSysexMessage (const uint8* messageData, int numBytesSoFar, double timeStamp)
        {
            if (callbackEnabled)
                callback->handlePartialSysexMessage (midiInput, messageData, numBytesSoFar, timeStamp);
        }

        void handleIncomingUmpPacket (const uint32_t* words, uint32_t numWords, double timeStamp) const
        {
            if (! callbackEnabled || words == nullptr || numWords == 0)
                return;

            const ump::View view (words);

            if (umpReceiver != nullptr)
            {
                umpReceiver->packetReceived (view, timeStamp);
                return;
            }

            if (callback != nullptr && umpToBytestream != nullptr)
            {
                umpToBytestream->convert (view, timeStamp, [&] (const MidiMessage& message)
                {
                    callback->handleIncomingMidiMessage (midiInput, message);
                });
            }
        }

        int getPortId() const { return portId; }

        const String& getPortName() const { return portName; }

    private:
        const std::shared_ptr<AlsaClient> client = AlsaClient::getInstance();

        MidiInputCallback* callback = nullptr;
        snd_midi_event_t* midiParser = nullptr;
        MidiInput* midiInput = nullptr;
        ump::Receiver* umpReceiver = nullptr;
        ump::PacketProtocol umpProtocol = ump::PacketProtocol::MIDI_1_0;
        std::unique_ptr<ump::GenericUMPConverter> umpConverter;
        std::unique_ptr<ump::ToBytestreamConverter> umpToBytestream;

        String portName;

        int maxEventSize = 4096, portId = -1;
        std::atomic<bool> callbackEnabled { false };
        bool isInput = false;
    };

    static std::shared_ptr<AlsaClient> getInstance()
    {
        static std::weak_ptr<AlsaClient> ptr;

        if (auto locked = ptr.lock())
            return locked;

        std::shared_ptr<AlsaClient> result (new AlsaClient());
        ptr = result;
        return result;
    }

    void handleIncomingMidiMessage (snd_seq_event* event, const MidiMessage& message)
    {
        const ScopedLock sl (callbackLock);

        if (auto* port = findPort (event->dest.port))
            port->handleIncomingMidiMessage (message);
    }

    void handlePartialSysexMessage (snd_seq_event* event, const uint8* messageData, int numBytesSoFar, double timeStamp)
    {
        const ScopedLock sl (callbackLock);

        if (auto* port = findPort (event->dest.port))
            port->handlePartialSysexMessage (messageData, numBytesSoFar, timeStamp);
    }

    snd_seq_t* get() const noexcept { return handle; }

    int getId() const noexcept { return clientId; }

    Port* createPort (const String& name, bool forInput, bool enableSubscription)
    {
        const ScopedLock sl (callbackLock);

        auto port = new Port (forInput);
        port->createPort (name, enableSubscription);

        const auto iter = lowerBound (port->getPortId());
        jassert (iter == ports.end() || port->getPortId() < (*iter)->getPortId());
        ports.insert (iter, rawToUniquePtr (port));

        return port;
    }

    void deletePort (Port* port)
    {
        const ScopedLock sl (callbackLock);

        if (const auto iter = findPortIterator (port->getPortId()); iter != ports.end())
            ports.erase (iter);
    }

private:
    AlsaClient()
    {
        snd_seq_open (&handle, "default", SND_SEQ_OPEN_DUPLEX, 0);

        if (handle != nullptr)
        {
            snd_seq_nonblock (handle, SND_SEQ_NONBLOCK);
            snd_seq_set_client_name (handle, getAlsaMidiName().toRawUTF8());
            clientId = snd_seq_client_id (handle);

            // It's good idea to pre-allocate a good number of elements
            ports.reserve (32);

            announcementsIn = snd_seq_create_simple_port (handle,
                                                          TRANS ("announcements").toRawUTF8(),
                                                          SND_SEQ_PORT_CAP_WRITE,
                                                          SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
            snd_seq_connect_from (handle, announcementsIn, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);

            inputThread.emplace (*this);
        }
    }

    Port* findPort (int portId)
    {
        if (const auto iter = findPortIterator (portId); iter != ports.end())
            return iter->get();

        return nullptr;
    }

    snd_seq_t* handle = nullptr;
    int clientId = 0;
    int announcementsIn = 0;
    bool umpConfigured = false;
    std::vector<std::unique_ptr<Port>> ports;
    Atomic<int> activeCallbacks;
    CriticalSection callbackLock;

    //==============================================================================
    class SequencerThread
    {
    public:
        explicit SequencerThread (AlsaClient& c)
            : client (c)
        {
        }

        ~SequencerThread() noexcept
        {
            shouldStop = true;
            thread.join();
        }

    private:
        // If we directly call MidiDeviceListConnectionBroadcaster::get() from the background thread,
        // there's a possibility that we'll deadlock in the following scenario:
        // - The main thread calls MidiDeviceListConnectionBroadcaster::get() for the first time
        //   (e.g. to register a listener). The static MidiDeviceListConnectionBroadcaster singleton
        //   begins construction. During the constructor, an AlsaClient is created to iterate midi
        //   ins/outs.
        // - The AlsaClient starts a new SequencerThread. If connections are updated, the
        //   SequencerThread may call MidiDeviceListConnectionBroadcaster::get().notify()
        //   while the MidiDeviceListConnectionBroadcaster singleton is still being created.
        // - The SequencerThread blocks until the MidiDeviceListConnectionBroadcaster has been
        //   created on the main thread, but the MidiDeviceListConnectionBroadcaster's constructor
        //   can't complete until the AlsaClient's destructor has run, which in turn requires the
        //   SequencerThread to join.
        class UpdateNotifier final : private AsyncUpdater
        {
        public:
            ~UpdateNotifier() override { cancelPendingUpdate(); }

            using AsyncUpdater::triggerAsyncUpdate;

        private:
            void handleAsyncUpdate() override { MidiDeviceListConnectionBroadcaster::get().notify(); }
        };

        AlsaClient& client;
        MidiDataConcatenator concatenator { 2048 };
        std::atomic<bool> shouldStop { false };
        UpdateNotifier notifier;
        std::thread thread { [this]
        {
            Thread::setCurrentThreadName ("YUP MIDI Input");

            auto seqHandle = client.get();

            const int maxEventSize = 16 * 1024;
            snd_midi_event_t* midiParser;

            if (snd_midi_event_new (maxEventSize, &midiParser) >= 0)
            {
                const ScopeGuard freeMidiEvent { [&]
                {
                    snd_midi_event_free (midiParser);
                } };

                const auto numPfds = snd_seq_poll_descriptors_count (seqHandle, POLLIN);
                std::vector<pollfd> pfd (static_cast<size_t> (numPfds));
                snd_seq_poll_descriptors (seqHandle, pfd.data(), (unsigned int) numPfds, POLLIN);

                std::vector<uint8> buffer (maxEventSize);

                while (! shouldStop)
                {
                    // This timeout shouldn't be too long, so that the program can exit in a timely manner
                    if (poll (pfd.data(), (nfds_t) numPfds, 100) > 0)
                    {
                        if (shouldStop)
                            break;

                        do
                        {
                            snd_seq_event_t* inputEvent = nullptr;

                            if (snd_seq_event_input (seqHandle, &inputEvent) >= 0)
                            {
                                const ScopeGuard freeInputEvent { [&]
                                {
                                    snd_seq_free_event (inputEvent);
                                } };

                                constexpr int systemEvents[] {
                                    SND_SEQ_EVENT_CLIENT_CHANGE,
                                    SND_SEQ_EVENT_CLIENT_START,
                                    SND_SEQ_EVENT_CLIENT_EXIT,
                                    SND_SEQ_EVENT_PORT_CHANGE,
                                    SND_SEQ_EVENT_PORT_START,
                                    SND_SEQ_EVENT_PORT_EXIT,
                                    SND_SEQ_EVENT_PORT_SUBSCRIBED,
                                    SND_SEQ_EVENT_PORT_UNSUBSCRIBED,
                                };

                                const auto foundEvent = std::find (std::begin (systemEvents),
                                                                   std::end (systemEvents),
                                                                   inputEvent->type);

                                if (foundEvent != std::end (systemEvents))
                                {
                                    notifier.triggerAsyncUpdate();
                                    continue;
                                }

#if defined(SND_SEQ_EVENT_UMP)
                                if ((inputEvent->flags & SND_SEQ_EVENT_UMP) != 0)
                                {
                                    const auto timeStamp = Time::getMillisecondCounter() * 0.001;
                                    const auto* dataPtr = static_cast<const uint8_t*> (inputEvent->data.ext.ptr);
                                    const auto dataLen = inputEvent->data.ext.len;

                                    if (dataPtr != nullptr && dataLen >= 4)
                                    {
                                        std::array<uint32_t, 4> words {};
                                        const auto bytesToCopy = jmin ((size_t) dataLen, sizeof (words));
                                        std::memcpy (words.data(), dataPtr, bytesToCopy);

                                        if (auto* port = client.findPort (inputEvent->dest.port))
                                            port->handleIncomingUmpPacket (words.data(),
                                                                           static_cast<uint32_t> (bytesToCopy / sizeof (uint32_t)),
                                                                           timeStamp);
                                    }

                                    continue;
                                }
#endif

                                // xxx what about SYSEXes that are too big for the buffer?
                                const auto numBytes = snd_midi_event_decode (midiParser,
                                                                             buffer.data(),
                                                                             maxEventSize,
                                                                             inputEvent);

                                snd_midi_event_reset_decode (midiParser);

                                concatenator.pushMidiData (buffer.data(), (int) numBytes, Time::getMillisecondCounter() * 0.001, inputEvent, client);
                            }
                        } while (snd_seq_event_input_pending (seqHandle, 0) > 0);
                    }
                }
            }
        } };
    };

    std::optional<SequencerThread> inputThread;
};

//==============================================================================
static String getFormattedPortIdentifier (int clientId, int portId)
{
    return String (clientId) + "-" + String (portId);
}

static AlsaClient::Port* iterateMidiClient (AlsaClient& client,
                                            snd_seq_client_info_t* clientInfo,
                                            bool forInput,
                                            Array<MidiDeviceInfo>& devices,
                                            const String& deviceIdentifierToOpen)
{
    AlsaClient::Port* port = nullptr;

    auto seqHandle = client.get();
    snd_seq_port_info_t* portInfo = nullptr;

    snd_seq_port_info_alloca (&portInfo);
    jassert (portInfo != nullptr);
    auto numPorts = snd_seq_client_info_get_num_ports (clientInfo);
    auto sourceClient = snd_seq_client_info_get_client (clientInfo);

    snd_seq_port_info_set_client (portInfo, sourceClient);
    snd_seq_port_info_set_port (portInfo, -1);

    while (--numPorts >= 0)
    {
        if (snd_seq_query_next_port (seqHandle, portInfo) == 0
            && (snd_seq_port_info_get_capability (portInfo)
                & (forInput ? SND_SEQ_PORT_CAP_SUBS_READ : SND_SEQ_PORT_CAP_SUBS_WRITE))
                   != 0)
        {
            String portName (snd_seq_port_info_get_name (portInfo));
            auto portID = snd_seq_port_info_get_port (portInfo);

            const auto baseIdentifier = getFormattedPortIdentifier (sourceClient, portID);
            MidiDeviceInfo device (portName, baseIdentifier);
            devices.add (device);

#if defined(SND_SEQ_PORT_TYPE_MIDI_UMP)
            if ((snd_seq_port_info_get_type (portInfo) & SND_SEQ_PORT_TYPE_MIDI_UMP) != 0)
            {
                devices.add (MidiDeviceInfo { portName,
                                              getUmpSequencerIdentifier (baseIdentifier),
                                              ump::PacketProtocol::MIDI_2_0,
                                              true });
            }
#endif

            if (deviceIdentifierToOpen.isNotEmpty() && deviceIdentifierToOpen == device.identifier)
            {
                if (portID != -1)
                {
                    port = client.createPort (portName, forInput, false);
                    jassert (port->isValid());
                    port->connectWith (sourceClient, portID);
                    break;
                }
            }
        }
    }

    return port;
}

static AlsaClient::Port* iterateMidiDevices (bool forInput,
                                             Array<MidiDeviceInfo>& devices,
                                             const String& deviceIdentifierToOpen)
{
    AlsaClient::Port* port = nullptr;
    auto client = AlsaClient::getInstance();

    if (auto seqHandle = client->get())
    {
        snd_seq_system_info_t* systemInfo = nullptr;
        snd_seq_client_info_t* clientInfo = nullptr;

        snd_seq_system_info_alloca (&systemInfo);
        jassert (systemInfo != nullptr);

        if (snd_seq_system_info (seqHandle, systemInfo) == 0)
        {
            snd_seq_client_info_alloca (&clientInfo);
            jassert (clientInfo != nullptr);

            auto numClients = snd_seq_system_info_get_cur_clients (systemInfo);

            while (--numClients >= 0)
            {
                if (snd_seq_query_next_client (seqHandle, clientInfo) == 0)
                {
                    port = iterateMidiClient (*client,
                                              clientInfo,
                                              forInput,
                                              devices,
                                              deviceIdentifierToOpen);

                    if (port != nullptr)
                        break;
                }
            }
        }
    }

    return port;
}

struct AlsaPortPtr
{
    explicit AlsaPortPtr (AlsaClient::Port* p)
        : ptr (p)
    {
    }

    virtual ~AlsaPortPtr() noexcept { AlsaClient::getInstance()->deletePort (ptr); }

    AlsaClient::Port* ptr = nullptr;
};

//==============================================================================
class MidiInput::Pimpl
{
public:
    void configureForUmp()
    {
#if defined(SND_SEQ_EVENT_UMP)
        if (! umpConfigured && handle != nullptr)
        {
            snd_seq_set_client_midi_version (handle, 2);
#if defined(snd_seq_set_client_ump_conversion)
            snd_seq_set_client_ump_conversion (handle, 1);
#endif
            umpConfigured = true;
        }
#endif
    }

    virtual ~Pimpl() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
};

class SequencerInputPimpl final : public MidiInput::Pimpl
    , public AlsaPortPtr
{
public:
    explicit SequencerInputPimpl (AlsaClient::Port* p, std::unique_ptr<MidiInputCallback> cb)
        : AlsaPortPtr (p)
        , callback (std::move (cb))
    {
    }

    void start() override
    {
        ptr->enableCallback (true);
    }

    void stop() override
    {
        ptr->enableCallback (false);
    }

private:
    std::unique_ptr<MidiInputCallback> callback;
};

class UmpRawmidiInputPimpl final : public MidiInput::Pimpl
{
public:
    UmpRawmidiInputPimpl (int fdIn, std::unique_ptr<ump::U32InputHandler> handlerIn)
        : fd (fdIn)
        , handler (std::move (handlerIn))
    {
        startThread();
    }

#if YUP_HAS_ALSA_UMP
    UmpRawmidiInputPimpl (snd_ump_t* handleIn, std::unique_ptr<ump::U32InputHandler> handlerIn)
        : umpHandle (handleIn)
        , handler (std::move (handlerIn))
    {
        startThread();
    }
#endif

    ~UmpRawmidiInputPimpl() override
    {
        shouldStop = true;
        if (thread.joinable())
            thread.join();

        if (fd >= 0)
            ::close (fd);
#if YUP_HAS_ALSA_UMP
        if (umpHandle != nullptr)
            snd_ump_close (umpHandle);
#endif
    }

    void start() override
    {
        callbackEnabled = true;
        handler->reset();
    }

    void stop() override
    {
        callbackEnabled = false;
    }

private:
    void startThread()
    {
        thread = std::thread ([this]
        {
            Thread::setCurrentThreadName ("YUP UMP RawMIDI Input");

            std::array<uint32_t, 256> buffer {};

#if YUP_HAS_ALSA_UMP
            if (umpHandle != nullptr)
            {
                const auto numPfds = snd_ump_poll_descriptors_count (umpHandle);
                std::vector<pollfd> pfds ((size_t) numPfds);
                snd_ump_poll_descriptors (umpHandle, pfds.data(), (unsigned int) numPfds);

                while (! shouldStop)
                {
                    if (poll (pfds.data(), (nfds_t) pfds.size(), 100) <= 0)
                        continue;

                    unsigned short revents = 0;
                    snd_ump_poll_descriptors_revents (umpHandle, pfds.data(), (unsigned int) pfds.size(), &revents);
                    if ((revents & POLLIN) == 0)
                        continue;

                    const auto bytesRead = snd_ump_read (umpHandle, buffer.data(), buffer.size() * sizeof (uint32_t));
                    if (bytesRead <= 0)
                        continue;

                    const auto words = static_cast<size_t> (bytesRead / sizeof (uint32_t));
                    if (words == 0 || ! callbackEnabled)
                        continue;

                    const auto now = Time::getMillisecondCounterHiRes() * 0.001;
                    handler->pushMidiData (buffer.data(), buffer.data() + words, now);
                }

                return;
            }
#endif

            pollfd pfd { fd, POLLIN, 0 };

            while (! shouldStop)
            {
                if (poll (&pfd, 1, 100) <= 0)
                    continue;

                if ((pfd.revents & POLLIN) == 0)
                    continue;

                const auto bytesRead = ::read (fd, buffer.data(), buffer.size() * sizeof (uint32_t));
                if (bytesRead <= 0)
                    continue;

                const auto words = static_cast<size_t> (bytesRead / sizeof (uint32_t));
                if (words == 0 || ! callbackEnabled)
                    continue;

                const auto now = Time::getMillisecondCounterHiRes() * 0.001;
                handler->pushMidiData (buffer.data(), buffer.data() + words, now);
            }
        });
    }

    int fd = -1;
#if YUP_HAS_ALSA_UMP
    snd_ump_t* umpHandle = nullptr;
#endif
    std::unique_ptr<ump::U32InputHandler> handler;
    std::atomic<bool> shouldStop { false };
    std::atomic<bool> callbackEnabled { false };
    std::thread thread;
};

class UmpReceiverCallback final : public MidiInputCallback
{
public:
    UmpReceiverCallback (ump::PacketProtocol protocolIn, ump::Receiver& receiverIn)
        : receiver (receiverIn)
        , converter (protocolIn)
    {
    }

    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message) override
    {
        const auto timestamp = message.getTimeStamp();
        converter.convert (ump::BytestreamMidiView (&message), [&] (const ump::View& view)
        {
            receiver.packetReceived (view, timestamp);
        });
    }

    void handlePartialSysexMessage (MidiInput*, const uint8*, int, double) override {}

private:
    ump::Receiver& receiver;
    ump::GenericUMPConverter converter;
};

class UmpReceiverCallback final : public MidiInputCallback
{
public:
    UmpReceiverCallback (ump::PacketProtocol protocolIn, ump::Receiver& receiverIn)
        : receiver (receiverIn)
        , converter (protocolIn)
    {
    }

    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message) override
    {
        const auto timestamp = message.getTimeStamp();
        converter.convert (ump::BytestreamMidiView (&message), [&] (const ump::View& view)
        {
            receiver.packetReceived (view, timestamp);
        });
    }

    void handlePartialSysexMessage (MidiInput*, const uint8*, int, double) override {}

private:
    ump::Receiver& receiver;
    ump::GenericUMPConverter converter;
};

Array<MidiDeviceInfo> MidiInput::getAvailableDevices()
{
    Array<MidiDeviceInfo> devices;
    for (const auto& node : findUmpRawmidiNodes())
    {
        const auto path = node.getFullPathName();
        devices.add (MidiDeviceInfo { "UMP " + node.getFileName(),
                                      getUmpRawmidiIdentifier (path),
                                      ump::PacketProtocol::MIDI_2_0,
                                      true });
    }
    iterateMidiDevices (true, devices, {});

    return devices;
}

MidiDeviceInfo MidiInput::getDefaultDevice()
{
    return getAvailableDevices().getFirst();
}

std::unique_ptr<MidiInput> MidiInput::openDevice (const String& deviceIdentifier, MidiInputCallback* callback)
{
    if (deviceIdentifier.isEmpty())
        return {};

    String identifierToOpen = deviceIdentifier;
    if (isUmpSequencerIdentifier (identifierToOpen))
        identifierToOpen = identifierToOpen.fromFirstOccurrenceOf (umpSequencerPrefix, false, false);

    if (isUmpRawmidiIdentifier (identifierToOpen))
    {
        if (callback == nullptr)
            return {};

        const auto path = getUmpRawmidiPath (identifierToOpen);
        int fd = -1;
#if YUP_HAS_ALSA_UMP
        snd_ump_t* umpHandle = nullptr;
        const auto umpName = getUmpAlsaDeviceName (path);
        if (umpName.isNotEmpty()
            && snd_ump_open (&umpHandle, nullptr, umpName.toRawUTF8(), SND_RAWMIDI_NONBLOCK) >= 0)
        {
            snd_ump_nonblock (umpHandle, 1);
        }
        else
        {
            umpHandle = nullptr;
        }
#endif

#if YUP_HAS_ALSA_UMP
        if (umpHandle == nullptr)
            fd = ::open (path.toRawUTF8(), O_RDONLY | O_NONBLOCK);
#else
        fd = ::open (path.toRawUTF8(), O_RDONLY | O_NONBLOCK);
#endif

        if (fd < 0
#if YUP_HAS_ALSA_UMP
            && umpHandle == nullptr
#endif
        )
            return {};

        std::unique_ptr<MidiInput> midiInput (new MidiInput (File (path).getFileName(),
                                                             deviceIdentifier,
                                                             ump::PacketProtocol::MIDI_1_0));

        auto handler = std::make_unique<ump::U32ToBytestreamHandler> (*midiInput, *callback);
#if YUP_HAS_ALSA_UMP
        if (umpHandle != nullptr)
            midiInput->internal.reset (new UmpRawmidiInputPimpl (umpHandle, std::move (handler)));
        else
#endif
            midiInput->internal.reset (new UmpRawmidiInputPimpl (fd, std::move (handler)));
        return midiInput;
    }

    Array<MidiDeviceInfo> devices;
    auto* port = iterateMidiDevices (true, devices, identifierToOpen);

    if (port == nullptr || ! port->isValid())
        return {};

    jassert (port->isValid());

    std::unique_ptr<MidiInput> midiInput (new MidiInput (port->getPortName(),
                                                         deviceIdentifier,
                                                         ump::PacketProtocol::MIDI_1_0));

    port->setupInput (midiInput.get(), callback);
    midiInput->internal.reset (new SequencerInputPimpl (port, nullptr));

    return midiInput;
}

std::unique_ptr<MidiInput> MidiInput::openDevice (const String& deviceIdentifier,
                                                  ump::PacketProtocol protocol,
                                                  ump::Receiver* receiver)
{
    if (deviceIdentifier.isEmpty() || receiver == nullptr)
        return {};

    String identifierToOpen = deviceIdentifier;
    if (isUmpSequencerIdentifier (identifierToOpen))
        identifierToOpen = identifierToOpen.fromFirstOccurrenceOf (umpSequencerPrefix, false, false);

    if (isUmpRawmidiIdentifier (identifierToOpen))
    {
        const auto path = getUmpRawmidiPath (identifierToOpen);
        int fd = -1;
#if YUP_HAS_ALSA_UMP
        snd_ump_t* umpHandle = nullptr;
        const auto umpName = getUmpAlsaDeviceName (path);
        if (umpName.isNotEmpty()
            && snd_ump_open (&umpHandle, nullptr, umpName.toRawUTF8(), SND_RAWMIDI_NONBLOCK) >= 0)
        {
            snd_ump_nonblock (umpHandle, 1);
        }
        else
        {
            umpHandle = nullptr;
        }
#endif

#if YUP_HAS_ALSA_UMP
        if (umpHandle == nullptr)
            fd = ::open (path.toRawUTF8(), O_RDONLY | O_NONBLOCK);
#else
        fd = ::open (path.toRawUTF8(), O_RDONLY | O_NONBLOCK);
#endif

        if (fd < 0
#if YUP_HAS_ALSA_UMP
            && umpHandle == nullptr
#endif
        )
            return {};

        std::unique_ptr<MidiInput> midiInput (new MidiInput (File (path).getFileName(),
                                                             deviceIdentifier,
                                                             protocol));

        auto handler = std::make_unique<ump::U32ToUMPHandler> (protocol, *receiver);
#if YUP_HAS_ALSA_UMP
        if (umpHandle != nullptr)
            midiInput->internal.reset (new UmpRawmidiInputPimpl (umpHandle, std::move (handler)));
        else
#endif
            midiInput->internal.reset (new UmpRawmidiInputPimpl (fd, std::move (handler)));
        return midiInput;
    }

    Array<MidiDeviceInfo> devices;
    auto* port = iterateMidiDevices (true, devices, identifierToOpen);

    if (port == nullptr || ! port->isValid())
        return {};

    if (deviceIdentifier.startsWith (umpSequencerPrefix))
        AlsaClient::getInstance()->configureForUmp();

    std::unique_ptr<MidiInput> midiInput (new MidiInput (port->getPortName(),
                                                         deviceIdentifier,
                                                         protocol));

    if (deviceIdentifier.startsWith (umpSequencerPrefix))
    {
        port->setupInputUMP (midiInput.get(), receiver, protocol);
        midiInput->internal.reset (new SequencerInputPimpl (port, nullptr));
    }
    else
    {
        auto callback = std::make_unique<UmpReceiverCallback> (protocol, *receiver);
        port->setupInput (midiInput.get(), callback.get());
        midiInput->internal.reset (new SequencerInputPimpl (port, std::move (callback)));
    }
    return midiInput;
}

std::unique_ptr<MidiInput> MidiInput::createNewDevice (const String& deviceName, MidiInputCallback* callback)
{
    auto client = AlsaClient::getInstance();
    auto* port = client->createPort (deviceName, true, true);

    if (port == nullptr || ! port->isValid())
        return {};

    std::unique_ptr<MidiInput> midiInput (new MidiInput (deviceName,
                                                         getFormattedPortIdentifier (client->getId(), port->getPortId()),
                                                         ump::PacketProtocol::MIDI_1_0));

    port->setupInput (midiInput.get(), callback);
    midiInput->internal.reset (new SequencerInputPimpl (port, nullptr));

    return midiInput;
}

std::unique_ptr<MidiInput> MidiInput::createNewDevice (const String&,
                                                       ump::PacketProtocol,
                                                       ump::Receiver*)
{
    jassertfalse;
    return {};
}

MidiInput::MidiInput (const String& deviceName,
                      const String& deviceIdentifier,
                      ump::PacketProtocol protocol)
    : deviceInfo (deviceName, deviceIdentifier, protocol)
{
}

MidiInput::~MidiInput()
{
    stop();
}

void MidiInput::start()
{
    internal->start();
}

void MidiInput::stop()
{
    internal->stop();
}

//==============================================================================
class MidiOutput::Pimpl
{
public:
    virtual ~Pimpl() = default;
    virtual void sendMessageNow (const MidiMessage& message) = 0;
    virtual void sendMessageNow (const ump::View& message) = 0;
};

class SequencerOutputPimpl final : public AlsaPortPtr
    , public MidiOutput::Pimpl
{
public:
    explicit SequencerOutputPimpl (AlsaClient::Port* p)
        : AlsaPortPtr (p)
    {
    }

    void sendMessageNow (const MidiMessage& message) override
    {
        ptr->sendMessageNow (message);
    }

    void sendMessageNow (const ump::View& message) override
    {
        ump::ToBytestreamConverter converter { 2048 };
        converter.convert (message, 0.0, [&] (const MidiMessage& midiMessage)
        {
            ptr->sendMessageNow (midiMessage);
        });
    }
};

class UmpSequencerOutputPimpl final : public AlsaPortPtr
    , public MidiOutput::Pimpl
{
public:
    explicit UmpSequencerOutputPimpl (AlsaClient::Port* p, ump::PacketProtocol protocolIn)
        : AlsaPortPtr (p)
        , converter (protocolIn)
    {
    }

    void sendMessageNow (const MidiMessage& message) override
    {
        converter.convert (ump::BytestreamMidiView (&message), [&] (const ump::View& view)
        {
            ptr->sendUmpMessageNow (view.data(), view.size());
        });
    }

    void sendMessageNow (const ump::View& message) override
    {
        ptr->sendUmpMessageNow (message.data(), message.size());
    }

private:
    ump::GenericUMPConverter converter;
};

class UmpRawmidiOutputPimpl final : public MidiOutput::Pimpl
{
public:
    UmpRawmidiOutputPimpl (int fdIn, ump::PacketProtocol protocolIn)
        : fd (fdIn)
        , converter (protocolIn)
    {
    }

#if YUP_HAS_ALSA_UMP
    UmpRawmidiOutputPimpl (snd_ump_t* handleIn, ump::PacketProtocol protocolIn)
        : umpHandle (handleIn)
        , converter (protocolIn)
    {
    }
#endif

    ~UmpRawmidiOutputPimpl() override
    {
        if (fd >= 0)
            ::close (fd);
#if YUP_HAS_ALSA_UMP
        if (umpHandle != nullptr)
            snd_ump_close (umpHandle);
#endif
    }

    void sendMessageNow (const MidiMessage& message) override
    {
        converter.convert (ump::BytestreamMidiView (&message), [&] (const ump::View& view)
        {
            writeWords (view.data(), view.size());
        });
    }

    void sendMessageNow (const ump::View& message) override
    {
        writeWords (message.data(), message.size());
    }

private:
    void writeWords (const uint32_t* data, uint32_t numWords) const
    {
        if (fd < 0 || data == nullptr || numWords == 0)
        {
#if YUP_HAS_ALSA_UMP
            if (umpHandle == nullptr)
                return;
#else
            return;
#endif
        }

#if YUP_HAS_ALSA_UMP
        if (umpHandle != nullptr)
        {
            const auto totalBytes = static_cast<size_t> (numWords) * sizeof (uint32_t);
            snd_ump_write (umpHandle, data, totalBytes);
            return;
        }
#endif

        const auto totalBytes = static_cast<size_t> (numWords) * sizeof (uint32_t);
        const auto* bytes = reinterpret_cast<const uint8_t*> (data);
        size_t offset = 0;

        while (offset < totalBytes)
        {
            const auto written = ::write (fd, bytes + offset, totalBytes - offset);
            if (written <= 0)
                break;

            offset += static_cast<size_t> (written);
        }
    }

    int fd = -1;
#if YUP_HAS_ALSA_UMP
    snd_ump_t* umpHandle = nullptr;
#endif
    ump::GenericUMPConverter converter;
};

Array<MidiDeviceInfo> MidiOutput::getAvailableDevices()
{
    Array<MidiDeviceInfo> devices;
    for (const auto& node : findUmpRawmidiNodes())
    {
        const auto path = node.getFullPathName();
        devices.add (MidiDeviceInfo { "UMP " + node.getFileName(),
                                      getUmpRawmidiIdentifier (path),
                                      ump::PacketProtocol::MIDI_2_0,
                                      true });
    }
    iterateMidiDevices (false, devices, {});

    return devices;
}

MidiDeviceInfo MidiOutput::getDefaultDevice()
{
    return getAvailableDevices().getFirst();
}

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String& deviceIdentifier)
{
    if (deviceIdentifier.isEmpty())
        return {};

    if (isUmpRawmidiIdentifier (deviceIdentifier))
        return openDevice (deviceIdentifier, ump::PacketProtocol::MIDI_2_0);
    if (isUmpSequencerIdentifier (deviceIdentifier))
        return openDevice (deviceIdentifier, ump::PacketProtocol::MIDI_2_0);

    Array<MidiDeviceInfo> devices;
    auto* port = iterateMidiDevices (false, devices, deviceIdentifier);

    if (port == nullptr || ! port->isValid())
        return {};

    std::unique_ptr<MidiOutput> midiOutput (new MidiOutput (port->getPortName(),
                                                            deviceIdentifier,
                                                            ump::PacketProtocol::MIDI_1_0));

    port->setupOutput();
    midiOutput->internal.reset (new SequencerOutputPimpl (port));

    return midiOutput;
}

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String& deviceIdentifier,
                                                    ump::PacketProtocol protocol)
{
    if (deviceIdentifier.isEmpty())
        return {};

    String identifierToOpen = deviceIdentifier;
    if (isUmpSequencerIdentifier (identifierToOpen))
        identifierToOpen = identifierToOpen.fromFirstOccurrenceOf (umpSequencerPrefix, false, false);

    if (isUmpRawmidiIdentifier (identifierToOpen))
    {
        const auto path = getUmpRawmidiPath (identifierToOpen);
        int fd = -1;
#if YUP_HAS_ALSA_UMP
        snd_ump_t* umpHandle = nullptr;
        const auto umpName = getUmpAlsaDeviceName (path);
        if (umpName.isNotEmpty()
            && snd_ump_open (nullptr, &umpHandle, umpName.toRawUTF8(), SND_RAWMIDI_NONBLOCK) >= 0)
        {
            snd_ump_nonblock (umpHandle, 1);
        }
        else
        {
            umpHandle = nullptr;
        }
#endif

#if YUP_HAS_ALSA_UMP
        if (umpHandle == nullptr)
            fd = ::open (path.toRawUTF8(), O_WRONLY | O_NONBLOCK);
#else
        fd = ::open (path.toRawUTF8(), O_WRONLY | O_NONBLOCK);
#endif

        if (fd < 0
#if YUP_HAS_ALSA_UMP
            && umpHandle == nullptr
#endif
        )
            return {};

        std::unique_ptr<MidiOutput> midiOutput (new MidiOutput (File (path).getFileName(),
                                                                deviceIdentifier,
                                                                protocol));

#if YUP_HAS_ALSA_UMP
        if (umpHandle != nullptr)
            midiOutput->internal.reset (new UmpRawmidiOutputPimpl (umpHandle, protocol));
        else
#endif
            midiOutput->internal.reset (new UmpRawmidiOutputPimpl (fd, protocol));
        return midiOutput;
    }

    if (deviceIdentifier.startsWith (umpSequencerPrefix))
    {
#if defined(SND_SEQ_EVENT_UMP)
        Array<MidiDeviceInfo> devices;
        auto* port = iterateMidiDevices (false, devices, identifierToOpen);

        if (port == nullptr || ! port->isValid())
            return {};

        AlsaClient::getInstance()->configureForUmp();

        std::unique_ptr<MidiOutput> midiOutput (new MidiOutput (port->getPortName(),
                                                                deviceIdentifier,
                                                                protocol));

        port->setupOutput();
        midiOutput->internal.reset (new UmpSequencerOutputPimpl (port, protocol));
        return midiOutput;
#else
        return {};
#endif
    }

    if (protocol != ump::PacketProtocol::MIDI_1_0)
        return {};

    return openDevice (deviceIdentifier);
}

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice (const String& deviceName)
{
    auto client = AlsaClient::getInstance();
    auto* port = client->createPort (deviceName, false, true);

    if (port == nullptr || ! port->isValid())
        return {};

    std::unique_ptr<MidiOutput> midiOutput (new MidiOutput (deviceName,
                                                            getFormattedPortIdentifier (client->getId(), port->getPortId()),
                                                            ump::PacketProtocol::MIDI_1_0));

    port->setupOutput();
    midiOutput->internal.reset (new SequencerOutputPimpl (port));

    return midiOutput;
}

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice (const String& deviceName,
                                                         ump::PacketProtocol protocol)
{
    if (protocol != ump::PacketProtocol::MIDI_1_0)
        return {};

    return createNewDevice (deviceName);
}

MidiOutput::~MidiOutput()
{
    stopBackgroundThread();
}

void MidiOutput::sendMessageNow (const MidiMessage& message)
{
    internal->sendMessageNow (message);
}

void MidiOutput::sendMessageNow (const ump::View& message)
{
    internal->sendMessageNow (message);
}

void MidiOutput::sendMessageNow (const ump::Packets& packets)
{
    for (auto it = packets.cbegin(); it != packets.cend(); ++it)
        sendMessageNow (*it);
}

MidiDeviceListConnection MidiDeviceListConnection::make (std::function<void()> cb)
{
    auto& broadcaster = MidiDeviceListConnectionBroadcaster::get();
    // We capture the AlsaClient instance here to ensure that it remains alive for at least as long
    // as the MidiDeviceListConnection. This is necessary because system change messages will only
    // be processed when the AlsaClient's SequencerThread is running.
    return { &broadcaster, broadcaster.add ([fn = std::move (cb), client = AlsaClient::getInstance()]
    {
        NullCheckedInvocation::invoke (fn);
    }) };
}

//==============================================================================
#else

class MidiInput::Pimpl
{
};

// (These are just stub functions if ALSA is unavailable...)
MidiInput::MidiInput (const String& deviceName,
                      const String& deviceID,
                      ump::PacketProtocol protocol)
    : deviceInfo (deviceName, deviceID, protocol)
{
}

MidiInput::~MidiInput() {}

void MidiInput::start() {}

void MidiInput::stop() {}

Array<MidiDeviceInfo> MidiInput::getAvailableDevices() { return {}; }

MidiDeviceInfo MidiInput::getDefaultDevice() { return {}; }

std::unique_ptr<MidiInput> MidiInput::openDevice (const String&, MidiInputCallback*) { return {}; }

std::unique_ptr<MidiInput> MidiInput::openDevice (const String&,
                                                  ump::PacketProtocol,
                                                  ump::Receiver*)
{
    return {};
}

std::unique_ptr<MidiInput> MidiInput::createNewDevice (const String&, MidiInputCallback*) { return {}; }

std::unique_ptr<MidiInput> MidiInput::createNewDevice (const String&,
                                                       ump::PacketProtocol,
                                                       ump::Receiver*)
{
    return {};
}

class MidiOutput::Pimpl
{
};

MidiOutput::~MidiOutput() {}

void MidiOutput::sendMessageNow (const MidiMessage&) {}

void MidiOutput::sendMessageNow (const ump::View&) {}

void MidiOutput::sendMessageNow (const ump::Packets&) {}

Array<MidiDeviceInfo> MidiOutput::getAvailableDevices() { return {}; }

MidiDeviceInfo MidiOutput::getDefaultDevice() { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String&) { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String&, ump::PacketProtocol) { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice (const String&) { return {}; }

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice (const String&, ump::PacketProtocol) { return {}; }

MidiDeviceListConnection MidiDeviceListConnection::make (std::function<void()> cb)
{
    auto& broadcaster = MidiDeviceListConnectionBroadcaster::get();
    return { &broadcaster, broadcaster.add (std::move (cb)) };
}

#endif

} // namespace yup
