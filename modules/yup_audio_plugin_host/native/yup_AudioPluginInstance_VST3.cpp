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

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3

#if YUP_MAC
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace yup
{

using namespace Steinberg;

namespace
{

#if YUP_MAC
template <typename CFType>
struct VST3CFObjectDeleter
{
    void operator() (CFType object) const noexcept
    {
        if (object != nullptr)
            CFRelease (object);
    }
};

template <typename CFType>
using VST3CFUniquePtr = std::unique_ptr<std::remove_pointer_t<CFType>, VST3CFObjectDeleter<CFType>>;
#endif

//==============================================================================
// Converts a Steinberg TChar (UTF-16) string to a yup::String.
String toString (const Vst::TChar* src)
{
    return String (CharPointer_UTF16 (reinterpret_cast<const CharPointer_UTF16::CharType*> (src)));
}

String classIDToString (const TUID& cid)
{
    return String::toHexString (cid, static_cast<int> (sizeof (TUID)));
}

void toString128 (const String& source, Vst::String128 destination)
{
    if (destination == nullptr)
        return;

    const auto utf16 = source.toUTF16();
    const auto length = jmin (static_cast<size_t> (127), utf16.length());

    std::memcpy (destination, utf16.getAddress(), length * sizeof (Vst::TChar));
    destination[length] = 0;
}

void addMidiMessageToVST3Events (Vst::EventList& events, const MidiMessageMetadata& metadata)
{
    const auto message = metadata.getMessage();

    Vst::Event event {};
    event.busIndex = 0;
    event.sampleOffset = metadata.samplePosition;

    if (message.isNoteOn())
    {
        event.type = Vst::Event::kNoteOnEvent;
        event.noteOn.channel = static_cast<int16> (message.getChannel() - 1);
        event.noteOn.pitch = static_cast<int16> (message.getNoteNumber());
        event.noteOn.velocity = message.getFloatVelocity();
        event.noteOn.length = 0;
        event.noteOn.noteId = -1;
        events.addEvent (event);
        return;
    }

    if (message.isNoteOff())
    {
        event.type = Vst::Event::kNoteOffEvent;
        event.noteOff.channel = static_cast<int16> (message.getChannel() - 1);
        event.noteOff.pitch = static_cast<int16> (message.getNoteNumber());
        event.noteOff.velocity = message.getFloatVelocity();
        event.noteOff.noteId = -1;
        events.addEvent (event);
        return;
    }

    if (message.isAftertouch())
    {
        event.type = Vst::Event::kPolyPressureEvent;
        event.polyPressure.channel = static_cast<int16> (message.getChannel() - 1);
        event.polyPressure.pitch = static_cast<int16> (message.getNoteNumber());
        event.polyPressure.pressure = static_cast<float> (message.getAfterTouchValue()) / 127.0f;
        event.polyPressure.noteId = -1;
        events.addEvent (event);
        return;
    }

    if (message.isSysEx())
    {
        event.type = Vst::Event::kDataEvent;
        event.data.type = Vst::DataEvent::kMidiSysEx;
        event.data.bytes = message.getSysExData();
        event.data.size = static_cast<uint32> (message.getSysExDataSize());
        events.addEvent (event);
        return;
    }

    event.type = Vst::Event::kLegacyMIDICCOutEvent;
    event.midiCCOut.channel = static_cast<int8> (jlimit (0, 15, message.getChannel() - 1));

    if (message.isController())
    {
        event.midiCCOut.controlNumber = static_cast<uint8> (message.getControllerNumber());
        event.midiCCOut.value = static_cast<int8> (message.getControllerValue());
        events.addEvent (event);
    }
    else if (message.isProgramChange())
    {
        event.midiCCOut.controlNumber = static_cast<uint8> (Vst::kCtrlProgramChange);
        event.midiCCOut.value = static_cast<int8> (message.getProgramChangeNumber());
        events.addEvent (event);
    }
    else if (message.isPitchWheel())
    {
        const auto value = jlimit (0, 0x3fff, message.getPitchWheelValue());
        event.midiCCOut.controlNumber = static_cast<uint8> (Vst::kPitchBend);
        event.midiCCOut.value = static_cast<int8> (value & 0x7f);
        event.midiCCOut.value2 = static_cast<int8> ((value >> 7) & 0x7f);
        events.addEvent (event);
    }
    else if (message.isChannelPressure())
    {
        event.midiCCOut.controlNumber = static_cast<uint8> (Vst::kAfterTouch);
        event.midiCCOut.value = static_cast<int8> (message.getChannelPressureValue());
        events.addEvent (event);
    }
}

void addVST3EventToMidiBuffer (const Vst::Event& event, MidiBuffer& midiBuffer)
{
    switch (event.type)
    {
        case Vst::Event::kNoteOnEvent:
            midiBuffer.addEvent (MidiMessage::noteOn (event.noteOn.channel + 1,
                                                      event.noteOn.pitch,
                                                      event.noteOn.velocity),
                                 event.sampleOffset);
            break;

        case Vst::Event::kNoteOffEvent:
            midiBuffer.addEvent (MidiMessage::noteOff (event.noteOff.channel + 1,
                                                       event.noteOff.pitch,
                                                       event.noteOff.velocity),
                                 event.sampleOffset);
            break;

        case Vst::Event::kPolyPressureEvent:
            midiBuffer.addEvent (MidiMessage::aftertouchChange (event.polyPressure.channel + 1,
                                                                event.polyPressure.pitch,
                                                                jlimit (0, 127, static_cast<int> (event.polyPressure.pressure * 127.0f))),
                                 event.sampleOffset);
            break;

        case Vst::Event::kDataEvent:
            if (event.data.type == Vst::DataEvent::kMidiSysEx)
                midiBuffer.addEvent (MidiMessage::createSysExMessage (event.data.bytes, static_cast<int> (event.data.size)),
                                     event.sampleOffset);
            break;

        case Vst::Event::kLegacyMIDICCOutEvent:
        {
            const int channel = event.midiCCOut.channel + 1;

            if (event.midiCCOut.controlNumber <= 127)
            {
                midiBuffer.addEvent (MidiMessage::controllerEvent (channel,
                                                                   event.midiCCOut.controlNumber,
                                                                   static_cast<uint8> (event.midiCCOut.value)),
                                     event.sampleOffset);
            }
            else if (event.midiCCOut.controlNumber == Vst::kCtrlProgramChange)
            {
                midiBuffer.addEvent (MidiMessage::programChange (channel, static_cast<uint8> (event.midiCCOut.value)),
                                     event.sampleOffset);
            }
            else if (event.midiCCOut.controlNumber == Vst::kPitchBend)
            {
                midiBuffer.addEvent (MidiMessage::pitchWheel (channel,
                                                              (static_cast<uint8> (event.midiCCOut.value2) << 7)
                                                                  | static_cast<uint8> (event.midiCCOut.value)),
                                     event.sampleOffset);
            }
            else if (event.midiCCOut.controlNumber == Vst::kAfterTouch)
            {
                midiBuffer.addEvent (MidiMessage::channelPressureChange (channel, static_cast<uint8> (event.midiCCOut.value)),
                                     event.sampleOffset);
            }
            else if (event.midiCCOut.controlNumber == Vst::kCtrlPolyPressure)
            {
                midiBuffer.addEvent (MidiMessage::aftertouchChange (channel,
                                                                    static_cast<uint8> (event.midiCCOut.value),
                                                                    static_cast<uint8> (event.midiCCOut.value2)),
                                     event.sampleOffset);
            }

            break;
        }

        default:
            break;
    }
}

//==============================================================================
class HostAttributeList final : public Vst::IAttributeList
{
public:
    HostAttributeList()
    {
        FUNKNOWN_CTOR
    }

    ~HostAttributeList() noexcept {
        FUNKNOWN_DTOR
    }

    tresult PLUGIN_API setInt (Vst::IAttributeList::AttrID id, int64 value) override
    {
        if (id == nullptr)
            return kInvalidArgument;

        attributes[std::string (id)] = Attribute (value);
        return kResultTrue;
    }

    tresult PLUGIN_API getInt (Vst::IAttributeList::AttrID id, int64& value) override
    {
        if (const auto* attribute = findAttribute (id, Attribute::Type::integer))
        {
            value = attribute->intValue;
            return kResultTrue;
        }

        return kResultFalse;
    }

    tresult PLUGIN_API setFloat (Vst::IAttributeList::AttrID id, double value) override
    {
        if (id == nullptr)
            return kInvalidArgument;

        attributes[std::string (id)] = Attribute (value);
        return kResultTrue;
    }

    tresult PLUGIN_API getFloat (Vst::IAttributeList::AttrID id, double& value) override
    {
        if (const auto* attribute = findAttribute (id, Attribute::Type::floatingPoint))
        {
            value = attribute->floatValue;
            return kResultTrue;
        }

        return kResultFalse;
    }

    tresult PLUGIN_API setString (Vst::IAttributeList::AttrID id, const Vst::TChar* value) override
    {
        if (id == nullptr || value == nullptr)
            return kInvalidArgument;

        Attribute attribute;
        attribute.type = Attribute::Type::string;

        while (value[attribute.stringValue.size()] != 0)
            attribute.stringValue.push_back (value[attribute.stringValue.size()]);

        attribute.stringValue.push_back (0);
        attributes[std::string (id)] = std::move (attribute);
        return kResultTrue;
    }

    tresult PLUGIN_API getString (Vst::IAttributeList::AttrID id, Vst::TChar* value, uint32 sizeInBytes) override
    {
        if (value == nullptr)
            return kInvalidArgument;

        if (const auto* attribute = findAttribute (id, Attribute::Type::string))
        {
            const auto bytesToCopy = jmin (sizeInBytes,
                                           static_cast<uint32> (attribute->stringValue.size() * sizeof (Vst::TChar)));
            std::memcpy (value, attribute->stringValue.data(), bytesToCopy);
            return kResultTrue;
        }

        return kResultFalse;
    }

    tresult PLUGIN_API setBinary (Vst::IAttributeList::AttrID id, const void* data, uint32 sizeInBytes) override
    {
        if (id == nullptr || (data == nullptr && sizeInBytes > 0))
            return kInvalidArgument;

        Attribute attribute;
        attribute.type = Attribute::Type::binary;
        attribute.binaryValue.replaceAll (data, static_cast<std::size_t> (sizeInBytes));
        attributes[std::string (id)] = std::move (attribute);
        return kResultTrue;
    }

    tresult PLUGIN_API getBinary (Vst::IAttributeList::AttrID id, const void*& data, uint32& sizeInBytes) override
    {
        if (const auto* attribute = findAttribute (id, Attribute::Type::binary))
        {
            data = attribute->binaryValue.getData();
            sizeInBytes = static_cast<uint32> (attribute->binaryValue.getSize());
            return kResultTrue;
        }

        data = nullptr;
        sizeInBytes = 0;
        return kResultFalse;
    }

    DECLARE_FUNKNOWN_METHODS

private:
    struct Attribute
    {
        enum class Type
        {
            integer,
            floatingPoint,
            string,
            binary
        };

        Attribute() = default;

        explicit Attribute (int64 value)
            : type (Type::integer)
            , intValue (value)
        {
        }

        explicit Attribute (double value)
            : type (Type::floatingPoint)
            , floatValue (value)
        {
        }

        Type type = Type::integer;
        int64 intValue = 0;
        double floatValue = 0.0;
        std::vector<Vst::TChar> stringValue;
        MemoryBlock binaryValue;
    };

    const Attribute* findAttribute (Vst::IAttributeList::AttrID id, Attribute::Type type) const
    {
        if (id == nullptr)
            return nullptr;

        const auto iter = attributes.find (std::string (id));
        if (iter == attributes.end() || iter->second.type != type)
            return nullptr;

        return std::addressof (iter->second);
    }

    std::map<std::string, Attribute> attributes;
};

IMPLEMENT_FUNKNOWN_METHODS (HostAttributeList, Vst::IAttributeList, Vst::IAttributeList::iid)

//==============================================================================
class HostMessage final : public Vst::IMessage
{
public:
    HostMessage()
    {
        FUNKNOWN_CTOR
    }

    ~HostMessage() noexcept {
        FUNKNOWN_DTOR
    }

    FIDString PLUGIN_API getMessageID() override
    {
        return messageId.c_str();
    }

    void PLUGIN_API setMessageID (FIDString id) override
    {
        messageId = id != nullptr ? id : "";
    }

    Vst::IAttributeList* PLUGIN_API getAttributes() override
    {
        if (attributes == nullptr)
            attributes = IPtr<Vst::IAttributeList>::adopt (new HostAttributeList());

        return attributes.get();
    }

    DECLARE_FUNKNOWN_METHODS

private:
    std::string messageId;
    IPtr<Vst::IAttributeList> attributes;
};

IMPLEMENT_FUNKNOWN_METHODS (HostMessage, Vst::IMessage, Vst::IMessage::iid)

//==============================================================================
class HostApplication final : public Vst::IHostApplication
{
public:
    explicit HostApplication (const AudioPluginHostContext& context)
        : hostName (context.hostName.isNotEmpty() ? context.hostName : "YUP")
    {
        FUNKNOWN_CTOR
    }

    ~HostApplication() noexcept {
        FUNKNOWN_DTOR
    }

    tresult PLUGIN_API getName (Vst::String128 name) override
    {
        toString128 (hostName, name);
        return kResultTrue;
    }

    tresult PLUGIN_API createInstance (TUID cid, TUID iid, void** object) override
    {
        if (object == nullptr)
            return kInvalidArgument;

        if (FUnknownPrivate::iidEqual (cid, Vst::IMessage::iid)
            && FUnknownPrivate::iidEqual (iid, Vst::IMessage::iid))
        {
            *object = new HostMessage();
            return kResultTrue;
        }

        if (FUnknownPrivate::iidEqual (cid, Vst::IAttributeList::iid)
            && FUnknownPrivate::iidEqual (iid, Vst::IAttributeList::iid))
        {
            *object = new HostAttributeList();
            return kResultTrue;
        }

        *object = nullptr;
        return kResultFalse;
    }

    DECLARE_FUNKNOWN_METHODS

private:
    String hostName;
};

IMPLEMENT_FUNKNOWN_METHODS (HostApplication, Vst::IHostApplication, Vst::IHostApplication::iid)

//==============================================================================
// RAII wrapper around a dynamically loaded VST3 module.
struct VST3Module
{
    DynamicLibrary library;

#if YUP_MAC
    using BundleEntryFn = bool (*) (CFBundleRef);
    using BundleExitFn = bool (*)();

    VST3CFUniquePtr<CFBundleRef> bundle;
    BundleExitFn bundleExit = nullptr;
    bool bundleEntryCalled = false;
#endif

    using FactoryFn = IPluginFactory*(PLUGIN_API*) ();
    FactoryFn getFactory = nullptr;

#if YUP_MAC
    static VST3CFUniquePtr<CFBundleRef> createBundle (const File& file)
    {
        const auto path = file.getFullPathName();
        VST3CFUniquePtr<CFURLRef> url (CFURLCreateFromFileSystemRepresentation (kCFAllocatorDefault,
                                                                                reinterpret_cast<const UInt8*> (path.toRawUTF8()),
                                                                                static_cast<CFIndex> (path.getNumBytesAsUTF8()),
                                                                                true));

        if (url == nullptr)
            return {};

        return VST3CFUniquePtr<CFBundleRef> (CFBundleCreate (kCFAllocatorDefault, url.get()));
    }

    static File getBundleExecutableFile (CFBundleRef bundleToUse, const File& bundleFile)
    {
        const auto macOSFolder = bundleFile.getChildFile ("Contents/MacOS");

        if (bundleToUse != nullptr)
        {
            auto* executableValue = CFBundleGetValueForInfoDictionaryKey (bundleToUse, kCFBundleExecutableKey);

            if (executableValue != nullptr && CFGetTypeID (executableValue) == CFStringGetTypeID())
                return macOSFolder.getChildFile (String::fromCFString (static_cast<CFStringRef> (executableValue)));
        }

        return macOSFolder.getChildFile (bundleFile.getFileNameWithoutExtension());
    }
#endif

    static File findFirstPackageBinary (const File& package, const String& wildcardPattern)
    {
        Array<File> files;
        package.getChildFile ("Contents").findChildFiles (files, File::findFiles, true, wildcardPattern, File::FollowSymlinks::noCycles);

        const auto expectedName = package.getFileNameWithoutExtension();

        for (const auto& file : files)
            if (file.getFileNameWithoutExtension() == expectedName)
                return file;

        return files.isEmpty() ? File() : files.getFirst();
    }

    static File getPackageBinaryFile (const File& file)
    {
        if (! file.isDirectory())
            return file;

#if YUP_MAC
        return {};
#elif YUP_WINDOWS
        return findFirstPackageBinary (file, "*.vst3");
#elif YUP_LINUX
        return findFirstPackageBinary (file, "*.so");
#else
        return file;
#endif
    }

    static std::unique_ptr<VST3Module> load (const File& file)
    {
        auto m = std::make_unique<VST3Module>();
        auto libraryFile = getPackageBinaryFile (file);

#if YUP_MAC
        if (file.isDirectory())
        {
            m->bundle = createBundle (file);

            if (m->bundle == nullptr)
                return nullptr;

            libraryFile = getBundleExecutableFile (m->bundle.get(), file);
        }
#endif

        if (! m->library.open (libraryFile.getFullPathName()))
            return nullptr;

#if YUP_MAC
        if (m->bundle != nullptr)
        {
            auto bundleEntry = reinterpret_cast<BundleEntryFn> (m->library.getFunction ("bundleEntry"));
            m->bundleExit = reinterpret_cast<BundleExitFn> (m->library.getFunction ("bundleExit"));

            if (bundleEntry == nullptr || m->bundleExit == nullptr)
                return nullptr;

            if (! bundleEntry (m->bundle.get()))
                return nullptr;

            m->bundleEntryCalled = true;
        }
#endif

        m->getFactory = reinterpret_cast<FactoryFn> (
            m->library.getFunction ("GetPluginFactory"));

        if (m->getFactory == nullptr)
            return nullptr;

        return m;
    }

    ~VST3Module()
    {
#if YUP_MAC
        if (bundleEntryCalled && bundleExit != nullptr)
            bundleExit();
#endif
    }
};

//==============================================================================
// Minimal IComponentHandler stub - required by IAudioProcessor::initialize().
class HostComponentHandler : public Vst::IComponentHandler
{
public:
    using RestartCallback = std::function<void (int32)>;
    using ParameterGestureCallback = std::function<void (Vst::ParamID)>;
    using ParameterEditCallback = std::function<void (Vst::ParamID, Vst::ParamValue)>;

    HostComponentHandler()
    {
        FUNKNOWN_CTOR
    }

    ~HostComponentHandler() noexcept {
        FUNKNOWN_DTOR
    }

    tresult PLUGIN_API beginEdit (Vst::ParamID tag) override
    {
        if (beginEditCallback != nullptr)
            beginEditCallback (tag);

        return kResultOk;
    }

    tresult PLUGIN_API performEdit (Vst::ParamID tag, Vst::ParamValue value) override
    {
        if (performEditCallback != nullptr)
            performEditCallback (tag, value);

        return kResultOk;
    }

    tresult PLUGIN_API endEdit (Vst::ParamID tag) override
    {
        if (endEditCallback != nullptr)
            endEditCallback (tag);

        return kResultOk;
    }

    tresult PLUGIN_API restartComponent (int32 flags) override
    {
        if (restartCallback != nullptr)
            restartCallback (flags);

        return kResultOk;
    }

    void setRestartCallback (RestartCallback callback)
    {
        restartCallback = std::move (callback);
    }

    void setParameterEditCallbacks (ParameterGestureCallback beginCallback,
                                    ParameterEditCallback performCallback,
                                    ParameterGestureCallback endCallback)
    {
        beginEditCallback = std::move (beginCallback);
        performEditCallback = std::move (performCallback);
        endEditCallback = std::move (endCallback);
    }

    DECLARE_FUNKNOWN_METHODS

private:
    RestartCallback restartCallback;
    ParameterGestureCallback beginEditCallback;
    ParameterEditCallback performEditCallback;
    ParameterGestureCallback endEditCallback;
};

IMPLEMENT_FUNKNOWN_METHODS (HostComponentHandler, Vst::IComponentHandler, Vst::IComponentHandler::iid)

//==============================================================================
FIDString getVST3PlatformType()
{
#if YUP_WINDOWS
    return kPlatformTypeHWND;
#elif YUP_MAC
    return kPlatformTypeNSView;
#elif YUP_LINUX
    return kPlatformTypeX11EmbedWindowID;
#else
    return nullptr;
#endif
}

#if YUP_MAC
void* getVST3ParentViewFromNativeHandle (void* nativeHandle)
{
    if (nativeHandle == nullptr)
        return nullptr;

    id nativeObject = (__bridge id) nativeHandle;
    if ([nativeObject isKindOfClass:[NSWindow class]])
        return (__bridge void*) [(NSWindow*) nativeObject contentView];

    if ([nativeObject isKindOfClass:[NSView class]])
        return nativeHandle;

    return nullptr;
}
#endif

#if YUP_LINUX
/** Linux has no global run loop, so plugin editors register their fds and timers with the host. */
class VST3HostRunLoop final : public Linux::IRunLoop
{
public:
    VST3HostRunLoop()
    {
        FUNKNOWN_CTOR
    }

    ~VST3HostRunLoop() noexcept
    {
        for (const auto& [fd, handlers] : eventHandlers)
            LinuxEventLoop::unregisterFdCallback (fd);

        FUNKNOWN_DTOR
    }

    tresult PLUGIN_API registerEventHandler (Linux::IEventHandler* handler, Linux::FileDescriptor fd) override
    {
        if (handler == nullptr)
            return kInvalidArgument;

        auto& handlers = eventHandlers[fd];
        if (handlers.empty())
            LinuxEventLoop::registerFdCallback (fd, [this] (int readyFd) { dispatchEvent (readyFd); });

        handlers.emplace_back (handler);
        return kResultTrue;
    }

    tresult PLUGIN_API unregisterEventHandler (Linux::IEventHandler* handler) override
    {
        bool found = false;

        for (auto it = eventHandlers.begin(); it != eventHandlers.end();)
        {
            auto& handlers = it->second;
            const auto previousSize = handlers.size();
            handlers.erase (std::remove (handlers.begin(), handlers.end(), handler), handlers.end());
            found = found || handlers.size() != previousSize;

            if (! handlers.empty())
            {
                ++it;
                continue;
            }

            LinuxEventLoop::unregisterFdCallback (it->first);
            it = eventHandlers.erase (it);
        }

        return found ? kResultTrue : kInvalidArgument;
    }

    tresult PLUGIN_API registerTimer (Linux::ITimerHandler* handler, Linux::TimerInterval milliseconds) override
    {
        if (handler == nullptr || milliseconds == 0)
            return kInvalidArgument;

        auto& timer = timers.emplace_back (std::make_unique<HandlerTimer> (handler));
        timer->startTimer (static_cast<int> (jmin (milliseconds, static_cast<Linux::TimerInterval> (std::numeric_limits<int>::max()))));
        return kResultTrue;
    }

    tresult PLUGIN_API unregisterTimer (Linux::ITimerHandler* handler) override
    {
        const auto previousSize = timers.size();
        timers.erase (std::remove_if (timers.begin(), timers.end(), [handler] (const auto& timer)
        {
            return timer->handler == handler;
        }),
                      timers.end());

        return timers.size() != previousSize ? kResultTrue : kInvalidArgument;
    }

    DECLARE_FUNKNOWN_METHODS

private:
    struct HandlerTimer final : public Timer
    {
        explicit HandlerTimer (Linux::ITimerHandler* handlerToUse)
            : handler (handlerToUse)
        {
        }

        void timerCallback() override { handler->onTimer(); }

        IPtr<Linux::ITimerHandler> handler;
    };

    void dispatchEvent (int fd)
    {
        const auto it = eventHandlers.find (fd);
        if (it == eventHandlers.end())
            return;

        const auto handlers = it->second;
        for (const auto& handler : handlers)
            handler->onFDIsSet (fd);
    }

    std::map<int, std::vector<IPtr<Linux::IEventHandler>>> eventHandlers;
    std::vector<std::unique_ptr<HandlerTimer>> timers;
};

IMPLEMENT_FUNKNOWN_METHODS (VST3HostRunLoop, Linux::IRunLoop, Linux::IRunLoop::iid)
#endif

class VST3Editor;

class VST3EditorFrame final : public IPlugFrame
{
public:
    explicit VST3EditorFrame (VST3Editor& ownerToUse)
        : owner (std::addressof (ownerToUse))
    {
        FUNKNOWN_CTOR
    }

    ~VST3EditorFrame() noexcept {
        FUNKNOWN_DTOR
    }

    tresult PLUGIN_API resizeView (IPlugView* view, ViewRect* newSize) override;

    void clearOwner() noexcept { owner = nullptr; }

    DECLARE_FUNKNOWN_METHODS

private:
    VST3Editor* owner = nullptr;

#if YUP_LINUX
    IPtr<VST3HostRunLoop> runLoop = IPtr<VST3HostRunLoop>::adopt (new VST3HostRunLoop());
#endif
};

IMPLEMENT_REFCOUNT (VST3EditorFrame)

tresult PLUGIN_API VST3EditorFrame::queryInterface (const TUID queryIid, void** obj)
{
#if YUP_LINUX
    if (FUnknownPrivate::iidEqual (queryIid, Linux::IRunLoop::iid))
    {
        runLoop->addRef();
        *obj = runLoop.get();
        return kResultOk;
    }
#endif

    QUERY_INTERFACE (queryIid, obj, FUnknown::iid, IPlugFrame)
    QUERY_INTERFACE (queryIid, obj, IPlugFrame::iid, IPlugFrame)

    *obj = nullptr;
    return kNoInterface;
}

class VST3Editor final : public AudioProcessorEditor
{
public:
    static std::unique_ptr<VST3Editor> create (Vst::IEditController* controller)
    {
        if (controller == nullptr)
            return nullptr;

        IPlugView* rawView = controller->createView (Vst::ViewType::kEditor);
        if (rawView == nullptr)
            return nullptr;

        IPtr<IPlugView> view = IPtr<IPlugView>::adopt (rawView);
        const auto* platformType = getVST3PlatformType();

        if (platformType == nullptr || view->isPlatformTypeSupported (platformType) != kResultTrue)
            return nullptr;

        return std::unique_ptr<VST3Editor> (new VST3Editor (std::move (view)));
    }

    ~VST3Editor() override
    {
        detachPlugView();

        if (frame != nullptr)
        {
            static_cast<VST3EditorFrame*> (frame.get())->clearOwner();
            frame = nullptr;
        }

        plugView = nullptr;
    }

    bool isResizable() const override
    {
        return plugView != nullptr && plugView->canResize() == kResultTrue;
    }

    Size<int> getPreferredSize() const override { return preferredSize; }

    bool shouldPreserveAspectRatio() const override { return aspectRatioSize.has_value(); }

    Size<int> getPreferredAspectRatioSize() const override { return aspectRatioSize.value_or (preferredSize); }

    void paint (Graphics& g) override
    {
        updateContentScale();

        g.setFillColor (Color (0xff101417));
        g.fillAll();
    }

    void resized() override
    {
        updateContentScale();
        resizePlugViewToBounds();
    }

    void attachedToNative() override
    {
        attachPlugView();
    }

    void detachedFromNative() override
    {
        detachPlugView();
    }

    tresult handleResizeRequest (IPlugView* view, ViewRect* newSize)
    {
        if (plugView == nullptr || view != plugView.get() || newSize == nullptr)
            return kInvalidArgument;

        preferredSize = {
            jmax (1, static_cast<int> (newSize->getWidth())),
            jmax (1, static_cast<int> (newSize->getHeight()))
        };

        newSize->right = newSize->left + preferredSize.getWidth();
        newSize->bottom = newSize->top + preferredSize.getHeight();

        setHostedEditorSize (*this, preferredSize);

        plugView->onSize (newSize);
        return kResultTrue;
    }

private:
    explicit VST3Editor (IPtr<IPlugView> view)
        : plugView (std::move (view))
        , frame (IPtr<IPlugFrame>::adopt (new VST3EditorFrame (*this)))
    {
        ViewRect size;
        if (plugView->getSize (std::addressof (size)) == kResultTrue
            && size.getWidth() > 0
            && size.getHeight() > 0)
        {
            preferredSize = {
                jmax (320, static_cast<int> (size.getWidth())),
                jmax (240, static_cast<int> (size.getHeight()))
            };

            if (isResizable())
            {
                aspectRatioSize = probeHostedEditorAspectRatio ({ size.getWidth(), size.getHeight() }, [this] (Size<int> sizeToConstrain)
                {
                    ViewRect rect (0, 0, sizeToConstrain.getWidth(), sizeToConstrain.getHeight());

                    if (plugView->checkSizeConstraint (std::addressof (rect)) != kResultTrue)
                        return sizeToConstrain;

                    return Size<int> (rect.getWidth(), rect.getHeight());
                });
            }
        }

        setSize (preferredSize.to<float>());
    }

    void attachPlugView()
    {
        if (plugView == nullptr || attached)
            return;

        auto* nativeComponent = getNativeComponent();
        if (nativeComponent == nullptr)
            return;

        void* parentHandle = nativeComponent->getNativeHandle();

#if YUP_MAC
        parentHandle = getVST3ParentViewFromNativeHandle (parentHandle);
#endif

        const auto* platformType = getVST3PlatformType();
        if (parentHandle == nullptr || platformType == nullptr)
            return;

        if (plugView->setFrame (frame.get()) != kResultTrue)
            return;

        if (plugView->attached (parentHandle, platformType) != kResultTrue)
        {
            plugView->setFrame (nullptr);
            return;
        }

        attached = true;
        appliedContentScale.reset();
        updateContentScale();
        resizePlugViewToBounds();
    }

    void updateContentScale()
    {
#if ! YUP_MAC
        if (plugView == nullptr || ! attached)
            return;

        const auto scale = getScaleDpi();
        if (appliedContentScale == scale)
            return;

        appliedContentScale = scale;

        if (FUnknownPtr<IPlugViewContentScaleSupport> scaleSupport (plugView); scaleSupport != nullptr)
            scaleSupport->setContentScaleFactor (scale);
#endif
    }

    void detachPlugView()
    {
        if (plugView == nullptr)
            return;

        if (attached)
        {
            plugView->removed();
            attached = false;
        }

        plugView->setFrame (nullptr);
    }

    void resizePlugViewToBounds()
    {
        if (plugView == nullptr || ! attached)
            return;

        const auto bounds = getBoundsRelativeToTopLevelComponent();
        ViewRect size;
        size.left = static_cast<int32> (bounds.getX());
        size.top = static_cast<int32> (bounds.getY());
        size.right = size.left + jmax (1, static_cast<int32> (bounds.getWidth()));
        size.bottom = size.top + jmax (1, static_cast<int32> (bounds.getHeight()));

        if (isResizable() && plugView->checkSizeConstraint (std::addressof (size)) == kResultTrue)
            followHostedEditorSize (*this, { size.getWidth(), size.getHeight() });

        plugView->onSize (std::addressof (size));
    }

    IPtr<IPlugView> plugView;
    IPtr<IPlugFrame> frame;
    Size<int> preferredSize { 640, 480 };
    std::optional<Size<int>> aspectRatioSize;
    std::optional<float> appliedContentScale;
    bool attached = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VST3Editor)
};

tresult PLUGIN_API VST3EditorFrame::resizeView (IPlugView* view, ViewRect* newSize)
{
    if (owner == nullptr)
        return kResultFalse;

    return owner->handleResizeRequest (view, newSize);
}

} // namespace

//==============================================================================

template <class Interface>
IPtr<Interface> queryVST3Interface (FUnknown* object)
{
    if (object == nullptr)
        return {};

    Interface* raw = nullptr;
    if (object->queryInterface (Interface::iid, reinterpret_cast<void**> (&raw)) != kResultOk || raw == nullptr)
        return {};

    return IPtr<Interface>::adopt (raw);
}

Vst::SpeakerArrangement getSpeakerArrangementForChannels (int numChannels)
{
    if (numChannels <= 0)
        return Vst::SpeakerArr::kEmpty;

    if (numChannels == 1)
        return Vst::SpeakerArr::kMono;

    if (numChannels == 2)
        return Vst::SpeakerArr::kStereo;

    if (numChannels >= 64)
        return ~Vst::SpeakerArrangement (0);

    return (Vst::SpeakerArrangement (1) << numChannels) - 1;
}

//==============================================================================
// Lock-free latest-value mailbox per parameter, used to hand parameter changes
// between the message thread and the audio thread without losing the newest value.
class VST3ParameterMailbox
{
public:
    void resize (int newSize)
    {
        size = jmax (0, newSize);
        slots = std::make_unique<Slot[]> (static_cast<std::size_t> (size));
        anyPending.store (false);
    }

    void set (int index, Vst::ParamValue value) noexcept
    {
        if (! isPositiveAndBelow (index, size))
            return;

        auto& slot = slots[static_cast<std::size_t> (index)];
        slot.value.store (value, std::memory_order_relaxed);
        slot.pending.store (true, std::memory_order_release);
        anyPending.store (true, std::memory_order_release);
    }

    template <class Callback>
    void flush (Callback&& callback) noexcept
    {
        if (! anyPending.exchange (false, std::memory_order_acq_rel))
            return;

        for (int index = 0; index < size; ++index)
        {
            auto& slot = slots[static_cast<std::size_t> (index)];

            if (slot.pending.exchange (false, std::memory_order_acq_rel))
                callback (index, slot.value.load (std::memory_order_relaxed));
        }
    }

private:
    struct Slot
    {
        std::atomic<Vst::ParamValue> value { 0.0 };
        std::atomic<bool> pending { false };
    };

    std::unique_ptr<Slot[]> slots;
    int size = 0;
    std::atomic<bool> anyPending { false };
};

//==============================================================================
class VST3Instance
    : public AudioPluginInstance
    , private Timer
{
public:
    VST3Instance (const AudioPluginDescription& desc,
                  const AudioPluginHostContext& context,
                  std::unique_ptr<VST3Module> module,
                  const FUID& classId,
                  IPtr<Vst::IHostApplication> hostApplication,
                  IPtr<Vst::IComponentHandler> componentHandler,
                  IPtr<Vst::IComponent> component,
                  IPtr<Vst::IAudioProcessor> processor,
                  IPtr<Vst::IEditController> controller,
                  bool controllerWasInitialized)
        : AudioPluginInstance (desc, buildBusLayout (component.get()))
        , hostContext (context)
        , vst3Module (std::move (module))
        , vst3ClassId (classId)
        , vst3HostApplication (std::move (hostApplication))
        , vst3ComponentHandler (std::move (componentHandler))
        , vst3Component (std::move (component))
        , vst3Processor (std::move (processor))
        , vst3Controller (std::move (controller))
        , vst3ControllerInitialized (controllerWasInitialized)
    {
        connectComponentAndController();
        syncControllerWithComponentState();
        buildParameterList();
        buildProgramList();
        rebuildMidiMapping();

        if (auto* handler = static_cast<HostComponentHandler*> (vst3ComponentHandler.get()))
        {
            handler->setRestartCallback ([this] (int32 flags)
            {
                restartComponent (flags);
            });

            handler->setParameterEditCallbacks (
                [this] (Vst::ParamID id)
            {
                handleParameterGestureBegin (id);
            },
                [this] (Vst::ParamID id, Vst::ParamValue value)
            {
                handleParameterEdit (id, value);
            },
                [this] (Vst::ParamID id)
            {
                handleParameterGestureEnd (id);
            });
        }

        setNonRealtime (context.isNonRealtime);
        startTimerHz (30);
    }

    ~VST3Instance() override
    {
        stopTimer();

        if (auto* handler = static_cast<HostComponentHandler*> (vst3ComponentHandler.get()))
        {
            handler->setRestartCallback (nullptr);
            handler->setParameterEditCallbacks (nullptr, nullptr, nullptr);
        }

        releaseResources();
        disconnectComponentAndController();

        if (vst3Controller != nullptr)
        {
            vst3Controller->setComponentHandler (nullptr);

            if (vst3ControllerInitialized)
                vst3Controller->terminate();
        }

        vst3Processor = nullptr;
        vst3Controller = nullptr;
        vst3Component->terminate();
        vst3Component = nullptr;
        vst3ComponentHandler = nullptr;
        vst3HostApplication = nullptr;
    }

    //==============================================================================

    void prepareToPlay (const AudioSpec& spec) override
    {
        const ScopedLock lock (getProcessLock());

        if (processingPrepared)
            releaseResources();

        lastSpec = spec;

        setProcessingPrecision (hostContext.preferDoublePrecision && supportsDoublePrecisionProcessing()
                                    ? ProcessingPrecision::doublePrecision
                                    : ProcessingPrecision::singlePrecision);

        applyBusArrangements (getBusLayout());

        Vst::ProcessSetup setup;
        setup.processMode = isNonRealtime() ? Vst::kOffline : Vst::kRealtime;
        setup.symbolicSampleSize = isUsingDoublePrecision() ? Vst::kSample64 : Vst::kSample32;
        setup.maxSamplesPerBlock = spec.maxBlockSize;
        setup.sampleRate = spec.sampleRate;
        vst3Processor->setupProcessing (setup);

        prepareAudioBusBuffers (spec.maxBlockSize);

        for (auto direction : { Vst::kInput, Vst::kOutput })
        {
            const auto& buses = direction == Vst::kInput ? vst3InputBuses : vst3OutputBuses;

            for (std::size_t i = 0; i < buses.size(); ++i)
                vst3Component->activateBus (Vst::kAudio, direction, static_cast<int32> (i), buses[i].numChannels > 0);

            const int numEventBuses = vst3Component->getBusCount (Vst::kEvent, direction);
            for (int i = 0; i < numEventBuses; ++i)
                vst3Component->activateBus (Vst::kEvent, direction, i, true);
        }

        if (isUsingDoublePrecision())
        {
            const auto& layout = getBusLayout();
            const int numChannels = jmax (1, layout.getNumAudioInputChannels(), layout.getNumAudioOutputChannels());
            doublePrecisionBuffer.setSize (numChannels, spec.maxBlockSize, false, true, false);
        }

        vst3Component->setActive (true);
        vst3Processor->setProcessing (true);
        processingPrepared = true;
    }

    void releaseResources() override
    {
        const ScopedLock lock (getProcessLock());

        if (! processingPrepared)
            return;

        if (vst3Processor != nullptr)
            vst3Processor->setProcessing (false);

        if (vst3Component != nullptr)
            vst3Component->setActive (false);

        processingPrepared = false;
    }

    void processBlock (AudioProcessContext<float>& context) override
    {
        ScopedNoDenormals noDenormals;

        const ScopedTryLock lock (getProcessLock());
        if (! lock.isLocked() || isSuspended() || ! processingPrepared)
        {
            context.audio.clear();
            context.midi.clear();
            return;
        }

        if (isBypassed() && bypassParameterId == Vst::kNoParamId)
        {
            processBlockBypassed (context);
            return;
        }

        if (! isUsingDoublePrecision())
        {
            processWithPlugin (context, Vst::kSample32);
            return;
        }

        auto& audioBuffer = context.audio;
        doublePrecisionBuffer.makeCopyOf (audioBuffer, true);

        AudioProcessContext<double> doubleContext { doublePrecisionBuffer, context.midi, context.params, context.playHead };
        processWithPlugin (doubleContext, Vst::kSample64);

        const int numChannels = jmin (audioBuffer.getNumChannels(), doublePrecisionBuffer.getNumChannels());
        const int numSamples = jmin (audioBuffer.getNumSamples(), doublePrecisionBuffer.getNumSamples());

        for (int channel = 0; channel < numChannels; ++channel)
            FloatVectorOperations::convertDoubleToFloat (audioBuffer.getWritePointer (channel), doublePrecisionBuffer.getReadPointer (channel), numSamples);
    }

    void processBlock (AudioProcessContext<double>& context) override
    {
        ScopedNoDenormals noDenormals;

        const ScopedTryLock lock (getProcessLock());
        if (! lock.isLocked() || isSuspended() || ! processingPrepared || ! isUsingDoublePrecision())
        {
            jassert (! processingPrepared || isUsingDoublePrecision());
            context.audio.clear();
            context.midi.clear();
            return;
        }

        if (isBypassed() && bypassParameterId == Vst::kNoParamId)
        {
            processBlockBypassed (context);
            return;
        }

        processWithPlugin (context, Vst::kSample64);
    }

    bool supportsDoublePrecisionProcessing() const override
    {
        return vst3Processor != nullptr && vst3Processor->canProcessSampleSize (Vst::kSample64) == kResultTrue;
    }

    //==============================================================================

    bool isBusLayoutSupported (const AudioBusLayout& layout) const override
    {
        if (layout == getBusLayout())
            return true;

        if (processingPrepared || vst3Processor == nullptr || ! layout.hasSameBusStructure (getBusLayout()))
            return false;

        const auto previous = getCurrentArrangements();
        const bool supported = setArrangements (getArrangementsForLayout (layout));
        setArrangements (previous);

        return supported;
    }

    int getParameterIndexForMidiController (int channel, int controller) const override
    {
        if (! isPositiveAndBelow (channel - 1, 16) || ! isPositiveAndBelow (controller, numMidiMappingControllers))
            return -1;

        return findParameterIndexForVST3Id (midiControllerMapping[static_cast<std::size_t> ((channel - 1) * numMidiMappingControllers + controller)].load (std::memory_order_relaxed));
    }

    //==============================================================================

    int getLatencySamples() override
    {
        return vst3Processor != nullptr ? static_cast<int> (vst3Processor->getLatencySamples()) : 0;
    }

    int getTailSamples() override
    {
        if (vst3Processor == nullptr)
            return 0;

        const auto tail = vst3Processor->getTailSamples();
        return tail == Vst::kInfiniteTail ? std::numeric_limits<int>::max() : static_cast<int> (jmin (tail, static_cast<uint32> (std::numeric_limits<int>::max())));
    }

    //==============================================================================

    int getCurrentPreset() const noexcept override { return currentPreset; }

    void setCurrentPreset (int index) noexcept override
    {
        if (! isPositiveAndBelow (index, numPresets) || programChangeParameterId == Vst::kNoParamId || vst3Controller == nullptr)
            return;

        const auto value = vst3Controller->plainParamToNormalized (programChangeParameterId, static_cast<Vst::ParamValue> (index));
        vst3Controller->setParamNormalized (programChangeParameterId, value);
        toProcessor.set (findSlotForVST3Id (programChangeParameterId), value);

        if (currentPreset != index)
        {
            currentPreset = index;
            updateHostDisplay (ChangeDetails().withProgramChanged (true));
        }
    }

    int getNumPresets() const override { return numPresets; }

    String getPresetName (int index) const override
    {
        if (! isPositiveAndBelow (index, numPresets))
            return {};

        if (isPositiveAndBelow (index, presetNames.size()))
            return presetNames[index];

        return "Preset " + String (index);
    }

    void setPresetName (int, StringRef) override {}

    //==============================================================================

    Result loadStateFromMemory (const MemoryBlock& memoryBlock) override
    {
        if (vst3Component == nullptr)
            return Result::fail ("Plugin not loaded");

        auto* data = const_cast<void*> (memoryBlock.getData());
        const auto size = static_cast<TSize> (memoryBlock.getSize());

        if (memoryBlock.getSize() >= 4 && std::memcmp (data, "VST3", 4) == 0)
        {
            auto stream = IPtr<IBStream>::adopt (new MemoryStream (data, size));

            if (! Vst::PresetFile::loadPreset (stream.get(), vst3ClassId, vst3Component.get(), vst3Controller.get()))
                return Result::fail ("VST3 preset could not be loaded");
        }
        else
        {
            // Raw component state, as saved by earlier versions or other hosts
            auto componentStream = IPtr<IBStream>::adopt (new MemoryStream (data, size));
            if (vst3Component->setState (componentStream.get()) != kResultOk)
                return Result::fail ("VST3 setState() failed");

            if (vst3Controller != nullptr)
            {
                auto controllerStream = IPtr<IBStream>::adopt (new MemoryStream (data, size));
                vst3Controller->setComponentState (controllerStream.get());
            }
        }

        refreshParameterValues();
        return Result::ok();
    }

    Result saveStateIntoMemory (MemoryBlock& memoryBlock) override
    {
        if (vst3Component == nullptr)
            return Result::fail ("Plugin not loaded");

        // A single-component plugin shares one state, only separate controllers get their own chunk
        auto* separateController = vst3ControllerInitialized ? vst3Controller.get() : nullptr;

        auto stream = IPtr<MemoryStream>::adopt (new MemoryStream());
        if (! Vst::PresetFile::savePreset (stream.get(), vst3ClassId, vst3Component.get(), separateController))
            return Result::fail ("VST3 getState() failed");

        memoryBlock.replaceAll (stream->getData(), static_cast<std::size_t> (stream->getSize()));
        return Result::ok();
    }

    //==============================================================================

    bool hasEditor() const override
    {
        return VST3Editor::create (vst3Controller.get()) != nullptr;
    }

    AudioProcessorEditor* createEditor() override
    {
        if (auto editor = VST3Editor::create (vst3Controller.get()))
            return editor.release();

        return nullptr;
    }

    //==============================================================================

    static std::unique_ptr<VST3Instance> create (const AudioPluginDescription& desc,
                                                 const AudioPluginHostContext& context)
    {
        auto mod = VST3Module::load (File (desc.fileOrBundlePath));
        if (mod == nullptr)
            return nullptr;

        IPluginFactory* rawFactory = mod->getFactory();
        if (rawFactory == nullptr)
            return nullptr;

        IPtr<IPluginFactory> factory (rawFactory);

        // Find the component class matching identifier
        const int classCount = factory->countClasses();
        for (int i = 0; i < classCount; ++i)
        {
            PClassInfo classInfo;
            if (factory->getClassInfo (i, &classInfo) != kResultOk)
                continue;

            if (String (classInfo.category) != "Audio Module Class")
                continue;

            if (desc.identifier.isNotEmpty() && ! classIDToString (classInfo.cid).equalsIgnoreCase (desc.identifier))
                continue;

            if (desc.identifier.isEmpty() && String (classInfo.name) != desc.name)
                continue;

            Vst::IComponent* rawComponent = nullptr;
            if (factory->createInstance (classInfo.cid, Vst::IComponent::iid, reinterpret_cast<void**> (&rawComponent)) != kResultOk)
                continue;

            IPtr<Vst::IComponent> component = IPtr<Vst::IComponent>::adopt (rawComponent);

            auto host = IPtr<Vst::IHostApplication>::adopt (new HostApplication (context));
            if (component->initialize (host.get()) != kResultOk)
                continue;

            auto processor = queryVST3Interface<Vst::IAudioProcessor> (component.get());
            if (processor == nullptr)
                continue;

            auto controller = queryVST3Interface<Vst::IEditController> (component.get());

            bool controllerInitialized = false;
            if (controller == nullptr)
            {
                TUID controllerClassId {};
                Vst::IEditController* rawController = nullptr;

                if (component->getControllerClassId (controllerClassId) == kResultTrue
                    && factory->createInstance (controllerClassId, Vst::IEditController::iid, reinterpret_cast<void**> (&rawController)) == kResultOk)
                {
                    controller = IPtr<Vst::IEditController>::adopt (rawController);

                    if (controller->initialize (host.get()) != kResultOk)
                        controller = nullptr;
                    else
                        controllerInitialized = true;
                }
            }

            auto componentHandler = IPtr<Vst::IComponentHandler>::adopt (new HostComponentHandler());
            if (controller != nullptr)
                controller->setComponentHandler (componentHandler.get());

            return std::make_unique<VST3Instance> (desc,
                                                   context,
                                                   std::move (mod),
                                                   FUID::fromTUID (classInfo.cid),
                                                   std::move (host),
                                                   std::move (componentHandler),
                                                   std::move (component),
                                                   std::move (processor),
                                                   std::move (controller),
                                                   controllerInitialized);
        }

        return nullptr;
    }

private:
    using Arrangements = std::pair<std::vector<Vst::SpeakerArrangement>, std::vector<Vst::SpeakerArrangement>>;

    static constexpr int numMidiMappingControllers = Vst::kPitchBend + 1;

    //==============================================================================

    static AudioBusLayout buildBusLayout (Vst::IComponent* component)
    {
        std::vector<AudioBus> inputs, outputs;

        auto addBuses = [component] (Vst::MediaType mediaType, Vst::BusDirection direction, std::vector<AudioBus>& buses)
        {
            const auto busType = mediaType == Vst::kAudio ? AudioBus::Type::Audio : AudioBus::Type::Midi;
            const auto busDirection = direction == Vst::kInput ? AudioBus::Direction::Input : AudioBus::Direction::Output;
            const int count = component->getBusCount (mediaType, direction);

            for (int i = 0; i < count; ++i)
            {
                Vst::BusInfo info {};
                component->getBusInfo (mediaType, direction, i, info);

                buses.emplace_back (toString (info.name),
                                    busType,
                                    busDirection,
                                    mediaType == Vst::kAudio ? static_cast<int> (info.channelCount) : 1,
                                    info.busType == Vst::kAux ? AudioBus::Role::Auxiliary : AudioBus::Role::Main,
                                    (info.flags & Vst::BusInfo::kDefaultActive) != 0);
            }
        };

        addBuses (Vst::kAudio, Vst::kInput, inputs);
        addBuses (Vst::kAudio, Vst::kOutput, outputs);
        addBuses (Vst::kEvent, Vst::kInput, inputs);
        addBuses (Vst::kEvent, Vst::kOutput, outputs);

        return AudioBusLayout (std::move (inputs), std::move (outputs));
    }

    //==============================================================================

    Arrangements getCurrentArrangements() const
    {
        Arrangements result;

        for (auto direction : { Vst::kInput, Vst::kOutput })
        {
            auto& arrangements = direction == Vst::kInput ? result.first : result.second;
            const int count = vst3Component->getBusCount (Vst::kAudio, direction);

            for (int i = 0; i < count; ++i)
            {
                Vst::SpeakerArrangement arrangement = Vst::SpeakerArr::kEmpty;
                vst3Processor->getBusArrangement (direction, i, arrangement);
                arrangements.push_back (arrangement);
            }
        }

        return result;
    }

    Arrangements getArrangementsForLayout (const AudioBusLayout& layout) const
    {
        auto result = getCurrentArrangements();

        auto convert = [] (Span<const AudioBus> buses, std::vector<Vst::SpeakerArrangement>& arrangements)
        {
            std::size_t audioBusIndex = 0;

            for (const auto& bus : buses)
            {
                if (bus.getType() != AudioBus::Type::Audio || audioBusIndex >= arrangements.size())
                    continue;

                auto& arrangement = arrangements[audioBusIndex++];

                // Keep the plugin's own arrangement (e.g. 5.1 vs 6 discrete channels) when the count matches
                if (Vst::SpeakerArr::getChannelCount (arrangement) != bus.getNumChannels())
                    arrangement = getSpeakerArrangementForChannels (bus.getNumChannels());
            }
        };

        convert (layout.getInputBuses(), result.first);
        convert (layout.getOutputBuses(), result.second);
        return result;
    }

    bool setArrangements (Arrangements arrangements) const
    {
        return vst3Processor->setBusArrangements (arrangements.first.data(),
                                                  static_cast<int32> (arrangements.first.size()),
                                                  arrangements.second.data(),
                                                  static_cast<int32> (arrangements.second.size()))
            == kResultTrue;
    }

    void applyBusArrangements (const AudioBusLayout& layout)
    {
        if (setArrangements (getArrangementsForLayout (layout)))
            return;

        YUP_MODULE_DBG (PLUGIN_HOST_VST3, "setBusArrangements rejected, keeping the plugin layout: " << getName());
    }

    void prepareAudioBusBuffers (int maxBlockSize)
    {
        auto fillBuses = [this] (Vst::BusDirection direction, std::vector<Vst::AudioBusBuffers>& buses)
        {
            const int count = vst3Component->getBusCount (Vst::kAudio, direction);
            buses.assign (static_cast<std::size_t> (jmax (0, count)), Vst::AudioBusBuffers {});

            int totalChannels = 0;
            for (int i = 0; i < count; ++i)
            {
                Vst::SpeakerArrangement arrangement = Vst::SpeakerArr::kEmpty;
                int numChannels = 0;

                if (vst3Processor->getBusArrangement (direction, i, arrangement) == kResultTrue)
                {
                    numChannels = Vst::SpeakerArr::getChannelCount (arrangement);
                }
                else
                {
                    Vst::BusInfo info {};
                    if (vst3Component->getBusInfo (Vst::kAudio, direction, i, info) == kResultOk)
                        numChannels = static_cast<int> (info.channelCount);
                }

                buses[static_cast<std::size_t> (i)].numChannels = numChannels;
                totalChannels += numChannels;
            }

            return totalChannels;
        };

        const int totalChannels = fillBuses (Vst::kInput, vst3InputBuses) + fillBuses (Vst::kOutput, vst3OutputBuses);

        channelPointers32.assign (static_cast<std::size_t> (totalChannels), nullptr);
        channelPointers64.assign (static_cast<std::size_t> (totalChannels), nullptr);

        if (isUsingDoublePrecision())
        {
            scratchBuffer64.setSize (jmax (1, totalChannels), maxBlockSize, false, true, false);
            scratchBuffer32.setSize (0, 0);
        }
        else
        {
            scratchBuffer32.setSize (jmax (1, totalChannels), maxBlockSize, false, true, false);
            scratchBuffer64.setSize (0, 0);
        }
    }

    template <typename FloatType>
    void processWithPlugin (AudioProcessContext<FloatType>& context, int32 symbolicSampleSize)
    {
        auto& audio = context.audio;
        const int numSamples = audio.getNumSamples();

        auto& scratch = getScratchBuffer<FloatType>();
        auto& channelPointers = getChannelPointers<FloatType>();

        if (numSamples > scratch.getNumSamples())
        {
            jassertfalse;
            audio.clear();
            context.midi.clear();
            return;
        }

        int scratchChannel = 0;
        std::size_t pointerIndex = 0;

        auto mapBuses = [&] (std::vector<Vst::AudioBusBuffers>& buses, auto views, bool isInput)
        {
            int flatChannel = 0;

            for (std::size_t busIndex = 0; busIndex < buses.size(); ++busIndex)
            {
                auto& bus = buses[busIndex];
                bus.silenceFlags = 0;
                auto** firstChannel = channelPointers.data() + pointerIndex;

                for (int channel = 0; channel < bus.numChannels; ++channel)
                {
                    FloatType* pointer = nullptr;

                    if (! views.empty())
                    {
                        if (busIndex < views.size() && channel < views[busIndex].getNumChannels())
                            pointer = const_cast<FloatType*> (views[busIndex].getChannels()[channel]);
                    }
                    else if (flatChannel < audio.getNumChannels())
                    {
                        pointer = audio.getWritePointer (flatChannel++);
                    }

                    if (pointer == nullptr && scratchChannel < scratch.getNumChannels())
                    {
                        pointer = scratch.getWritePointer (scratchChannel++);

                        if (isInput)
                            FloatVectorOperations::clear (pointer, numSamples);
                    }

                    channelPointers[pointerIndex++] = pointer;
                }

                if constexpr (std::is_same_v<FloatType, float>)
                    bus.channelBuffers32 = firstChannel;
                else
                    bus.channelBuffers64 = firstChannel;
            }
        };

        mapBuses (vst3InputBuses, context.inputs, true);
        mapBuses (vst3OutputBuses, context.outputs, false);

        Vst::ProcessData data {};
        prepareProcessData (data, numSamples, symbolicSampleSize, context.params, context.playHead);
        prepareMidiInputEvents (context.midi);

        data.numInputs = static_cast<int32> (vst3InputBuses.size());
        data.inputs = vst3InputBuses.empty() ? nullptr : vst3InputBuses.data();
        data.numOutputs = static_cast<int32> (vst3OutputBuses.size());
        data.outputs = vst3OutputBuses.empty() ? nullptr : vst3OutputBuses.data();

        vst3Processor->process (data);

        collectOutputParameterChanges();
        collectOutputEvents (context.midi);
    }

    template <typename FloatType>
    AudioBuffer<FloatType>& getScratchBuffer() noexcept
    {
        if constexpr (std::is_same_v<FloatType, float>)
            return scratchBuffer32;
        else
            return scratchBuffer64;
    }

    template <typename FloatType>
    std::vector<FloatType*>& getChannelPointers() noexcept
    {
        if constexpr (std::is_same_v<FloatType, float>)
            return channelPointers32;
        else
            return channelPointers64;
    }

    //==============================================================================

    std::map<Vst::UnitID, String> buildUnitPaths() const
    {
        std::map<Vst::UnitID, String> paths;

        auto unitInfo = queryVST3Interface<Vst::IUnitInfo> (vst3Controller.get());
        if (unitInfo == nullptr)
            return paths;

        std::map<Vst::UnitID, Vst::UnitInfo> units;
        for (int32 i = 0; i < unitInfo->getUnitCount(); ++i)
        {
            Vst::UnitInfo info {};
            if (unitInfo->getUnitInfo (i, info) == kResultOk)
                units[info.id] = info;
        }

        for (const auto& [unitId, info] : units)
        {
            StringArray names;
            auto current = unitId;

            // The depth guard protects against malformed (cyclic) unit hierarchies
            while (current != Vst::kRootUnitId && names.size() < 32)
            {
                const auto iter = units.find (current);
                if (iter == units.end())
                    break;

                names.insert (0, toString (iter->second.name).replace ("/", "-"));
                current = iter->second.parentUnitId;
            }

            paths[unitId] = names.joinIntoString ("/");
        }

        return paths;
    }

    void buildParameterList()
    {
        if (vst3Controller == nullptr)
            return;

        const auto unitPaths = buildUnitPaths();
        const int count = vst3Controller->getParameterCount();

        for (int i = 0; i < count; ++i)
        {
            Vst::ParameterInfo info {};
            if (vst3Controller->getParameterInfo (i, info) != kResultOk)
                continue;

            slotsByVST3Id[info.id] = static_cast<int> (allVST3ParameterIds.size());
            allVST3ParameterIds.push_back (info.id);

            if ((info.flags & Vst::ParameterInfo::kIsProgramChange) != 0)
            {
                if (programChangeParameterId == Vst::kNoParamId)
                {
                    programChangeParameterId = info.id;
                    programChangeUnitId = info.unitId;
                    numPresets = static_cast<int> (info.stepCount) + 1;
                }

                continue;
            }

            const auto id = info.id;
            const bool isBypass = (info.flags & Vst::ParameterInfo::kIsBypass) != 0 && bypassParameterId == Vst::kNoParamId;
            if (isBypass)
                bypassParameterId = id;

            const auto range = info.stepCount > 0
                                 ? NormalisableRange<float> (0.0f, 1.0f, 1.0f / static_cast<float> (info.stepCount))
                                 : NormalisableRange<float> (0.0f, 1.0f);

            AudioParameterBuilder builder;
            builder.withID (String (static_cast<int64> (id)))
                .withName (toString (info.title))
                .withRange (range)
                .withDefault (static_cast<float> (info.defaultNormalizedValue))
                .withAutomatable ((info.flags & Vst::ParameterInfo::kCanAutomate) != 0)
                .withReadOnly ((info.flags & Vst::ParameterInfo::kIsReadOnly) != 0)
                .withStepped (info.stepCount > 0)
                .withEnum ((info.flags & Vst::ParameterInfo::kIsList) != 0)
                .withBypass (isBypass)
                .withValueToString ([this, id] (float value)
            {
                return getParameterText (id, value);
            })
                .withStringToValue ([this, id] (const String& text)
            {
                return getParameterValueFromText (id, text);
            });

            if (id <= AudioParameter::maximumHostParameterID)
                builder.withHostID (id);

            if (const auto units = toString (info.units); units.isNotEmpty())
                builder.withUnit (AudioParameter::ParameterUnit::Custom, units);

            if (const auto path = unitPaths.find (info.unitId); path != unitPaths.end() && path->second.isNotEmpty())
                builder.withModulePath (path->second);

            vst3ParameterIds.push_back (id);
            addParameter (builder.build());
        }

        toProcessor.resize (static_cast<int> (allVST3ParameterIds.size()));
        toController.resize (static_cast<int> (allVST3ParameterIds.size()));
        inputParameterChanges.setMaxParameters (static_cast<int32> (allVST3ParameterIds.size()));
        outputParameterChanges.setMaxParameters (static_cast<int32> (allVST3ParameterIds.size()));

        refreshParameterValues();
    }

    void buildProgramList()
    {
        presetNames.clear();

        if (programChangeParameterId == Vst::kNoParamId)
            return;

        auto unitInfo = queryVST3Interface<Vst::IUnitInfo> (vst3Controller.get());
        if (unitInfo == nullptr)
            return;

        Vst::ProgramListID programListId = Vst::kNoProgramListId;
        for (int32 i = 0; i < unitInfo->getUnitCount(); ++i)
        {
            Vst::UnitInfo info {};
            if (unitInfo->getUnitInfo (i, info) == kResultOk && info.id == programChangeUnitId)
                programListId = info.programListId;
        }

        for (int32 i = 0; i < unitInfo->getProgramListCount(); ++i)
        {
            Vst::ProgramListInfo listInfo {};
            if (unitInfo->getProgramListInfo (i, listInfo) != kResultOk)
                continue;

            if (programListId != Vst::kNoProgramListId && listInfo.id != programListId)
                continue;

            for (int32 program = 0; program < listInfo.programCount; ++program)
            {
                Vst::String128 name {};
                unitInfo->getProgramName (listInfo.id, program, name);
                presetNames.add (toString (name));
            }

            break;
        }

        if (! presetNames.isEmpty())
            numPresets = presetNames.size();
    }

    void rebuildMidiMapping()
    {
        auto mapping = queryVST3Interface<Vst::IMidiMapping> (vst3Controller.get());

        for (int channel = 0; channel < 16; ++channel)
        {
            for (int controller = 0; controller < numMidiMappingControllers; ++controller)
            {
                Vst::ParamID id = Vst::kNoParamId;

                if (mapping == nullptr
                    || mapping->getMidiControllerAssignment (0, static_cast<int16> (channel), static_cast<Vst::CtrlNumber> (controller), id) != kResultTrue)
                {
                    id = Vst::kNoParamId;
                }

                midiControllerMapping[static_cast<std::size_t> (channel * numMidiMappingControllers + controller)].store (id, std::memory_order_relaxed);
            }
        }
    }

    String getParameterText (Vst::ParamID id, float value) const
    {
        Vst::String128 text {};

        if (vst3Controller != nullptr && vst3Controller->getParamStringByValue (id, static_cast<Vst::ParamValue> (value), text) == kResultOk)
            return toString (text);

        return String (value, 3);
    }

    float getParameterValueFromText (Vst::ParamID id, const String& text) const
    {
        Vst::String128 vstText {};
        toString128 (text, vstText);

        Vst::ParamValue value = 0.0;
        if (vst3Controller != nullptr && vst3Controller->getParamValueByString (id, vstText, value) == kResultOk)
            return static_cast<float> (value);

        return text.getFloatValue();
    }

    void refreshParameterValues()
    {
        if (vst3Controller == nullptr)
            return;

        const auto params = getParameters();

        for (std::size_t i = 0; i < vst3ParameterIds.size() && i < params.size(); ++i)
            params[i]->setNormalizedValue (static_cast<float> (vst3Controller->getParamNormalized (vst3ParameterIds[i])));

        if (programChangeParameterId != Vst::kNoParamId && numPresets > 1)
            currentPreset = roundToInt (vst3Controller->normalizedParamToPlain (programChangeParameterId, vst3Controller->getParamNormalized (programChangeParameterId)));
    }

    int findParameterIndexForVST3Id (Vst::ParamID id) const
    {
        const auto iter = std::find (vst3ParameterIds.begin(), vst3ParameterIds.end(), id);
        if (iter == vst3ParameterIds.end())
            return -1;

        return static_cast<int> (std::distance (vst3ParameterIds.begin(), iter));
    }

    int findSlotForVST3Id (Vst::ParamID id) const
    {
        const auto iter = slotsByVST3Id.find (id);
        return iter != slotsByVST3Id.end() ? iter->second : -1;
    }

    //==============================================================================

    void handleParameterGestureBegin (Vst::ParamID id)
    {
        if (auto param = getParameterForVST3Id (id))
            param->beginChangeGesture();
    }

    void handleParameterEdit (Vst::ParamID id, Vst::ParamValue value)
    {
        // The controller already holds the new value, the processor only learns it through process()
        toProcessor.set (findSlotForVST3Id (id), value);

        if (id == programChangeParameterId)
        {
            const auto preset = roundToInt (vst3Controller->normalizedParamToPlain (id, value));

            if (preset != currentPreset)
            {
                currentPreset = preset;
                updateHostDisplay (ChangeDetails().withProgramChanged (true));
            }

            return;
        }

        if (auto param = getParameterForVST3Id (id))
            param->setValueNotifyingHost (param->convertToDenormalizedValue (static_cast<float> (value)));

        if (id == bypassParameterId)
            setBypassed (value >= 0.5);
    }

    void handleParameterGestureEnd (Vst::ParamID id)
    {
        if (auto param = getParameterForVST3Id (id))
            param->endChangeGesture();
    }

    AudioParameter::Ptr getParameterForVST3Id (Vst::ParamID id) const
    {
        const auto index = findParameterIndexForVST3Id (id);
        const auto params = getParameters();

        return isPositiveAndBelow (index, static_cast<int> (params.size())) ? params[static_cast<std::size_t> (index)] : nullptr;
    }

    void bypassStateChanged() override
    {
        if (bypassParameterId == Vst::kNoParamId)
            return;

        const auto value = isBypassed() ? 1.0 : 0.0;
        toProcessor.set (findSlotForVST3Id (bypassParameterId), value);

        if (auto param = getParameterForVST3Id (bypassParameterId))
            param->setNormalizedValue (static_cast<float> (value));

        if (vst3Controller != nullptr && MessageManager::existsAndIsCurrentThread())
            vst3Controller->setParamNormalized (bypassParameterId, value);
        else
            bypassControllerSyncPending.store (true);
    }

    //==============================================================================

    void restartComponent (int32 flags)
    {
        if (MessageManager::existsAndIsCurrentThread())
            handleRestartComponent (flags);
        else
            pendingRestartFlags.fetch_or (flags);
    }

    void handleRestartComponent (int32 flags)
    {
        if ((flags & (Vst::kReloadComponent | Vst::kIoChanged)) != 0)
            reconfigureBuses();

        if ((flags & Vst::kLatencyChanged) != 0)
            setLatencySamples (getLatencySamples());

        if ((flags & Vst::kMidiCCAssignmentChanged) != 0)
            rebuildMidiMapping();

        ChangeDetails details;

        if ((flags & Vst::kParamValuesChanged) != 0)
        {
            refreshParameterValues();
            details = details.withParameterValuesChanged (true);
        }

        if ((flags & Vst::kParamTitlesChanged) != 0)
        {
            buildProgramList();
            details = details.withParameterInfoChanged (true);
        }

        if (details.parameterValuesChanged || details.parameterInfoChanged)
            updateHostDisplay (details);
    }

    void reconfigureBuses()
    {
        const ScopedLock lock (getProcessLock());

        const bool wasPrepared = processingPrepared;
        releaseResources();

        const auto newLayout = buildBusLayout (vst3Component.get());

        if (! newLayout.hasSameBusStructure (getBusLayout()))
            YUP_MODULE_DBG (PLUGIN_HOST_VST3, "plugin changed its bus structure, keeping the previous layout: " << getName());
        else if (const auto result = setBusLayout (newLayout); result.failed())
            YUP_MODULE_DBG (PLUGIN_HOST_VST3, "bus layout update failed: " << result.getErrorMessage());

        if (wasPrepared && lastSpec.has_value())
            prepareToPlay (*lastSpec);
    }

    void timerCallback() override
    {
        if (const auto flags = pendingRestartFlags.exchange (0); flags != 0)
            handleRestartComponent (flags);

        toController.flush ([this] (int slot, Vst::ParamValue value)
        {
            const auto id = allVST3ParameterIds[static_cast<std::size_t> (slot)];

            if (vst3Controller != nullptr)
                vst3Controller->setParamNormalized (id, value);

            if (auto param = getParameterForVST3Id (id))
                param->setNormalizedValue (static_cast<float> (value));

            if (id == bypassParameterId)
                setBypassed (value >= 0.5);
        });

        if (bypassControllerSyncPending.exchange (false) && vst3Controller != nullptr && bypassParameterId != Vst::kNoParamId)
            vst3Controller->setParamNormalized (bypassParameterId, isBypassed() ? 1.0 : 0.0);
    }

    //==============================================================================

    bool connectComponentAndController()
    {
        if (vst3Component == nullptr || vst3Controller == nullptr)
            return false;

        auto componentConnection = queryVST3Interface<Vst::IConnectionPoint> (vst3Component.get());
        auto controllerConnection = queryVST3Interface<Vst::IConnectionPoint> (vst3Controller.get());

        if (componentConnection == nullptr || controllerConnection == nullptr)
            return false;

        if (componentConnection->connect (controllerConnection.get()) != kResultTrue)
            return false;

        if (controllerConnection->connect (componentConnection.get()) != kResultTrue)
        {
            componentConnection->disconnect (controllerConnection.get());
            return false;
        }

        vst3ComponentsConnected = true;
        return true;
    }

    // A separate controller starts from its own defaults, so it must mirror the component state
    void syncControllerWithComponentState()
    {
        if (! vst3ControllerInitialized || vst3Controller == nullptr)
            return;

        auto stream = IPtr<MemoryStream>::adopt (new MemoryStream());
        if (vst3Component->getState (stream.get()) != kResultOk)
            return;

        stream->seek (0, IBStream::kIBSeekSet, nullptr);
        vst3Controller->setComponentState (stream.get());
    }

    void disconnectComponentAndController()
    {
        if (! vst3ComponentsConnected || vst3Component == nullptr || vst3Controller == nullptr)
            return;

        auto componentConnection = queryVST3Interface<Vst::IConnectionPoint> (vst3Component.get());
        auto controllerConnection = queryVST3Interface<Vst::IConnectionPoint> (vst3Controller.get());

        if (componentConnection != nullptr && controllerConnection != nullptr)
        {
            componentConnection->disconnect (controllerConnection.get());
            controllerConnection->disconnect (componentConnection.get());
        }

        vst3ComponentsConnected = false;
    }

    //==============================================================================

    void addInputParameterChange (int slot, int32 sampleOffset, Vst::ParamValue value)
    {
        if (! isPositiveAndBelow (slot, static_cast<int> (allVST3ParameterIds.size())))
            return;

        int32 queueIndex = 0;
        if (auto* queue = inputParameterChanges.addParameterData (allVST3ParameterIds[static_cast<std::size_t> (slot)], queueIndex))
        {
            int32 pointIndex = 0;
            queue->addPoint (sampleOffset, value, pointIndex);
        }
    }

    void prepareProcessData (Vst::ProcessData& data,
                             int numSamples,
                             int32 symbolicSampleSize,
                             const ParameterChangeBuffer& parameterChanges,
                             AudioPlayHead* playHead)
    {
        data.processMode = isNonRealtime() ? Vst::kOffline : Vst::kRealtime;
        data.symbolicSampleSize = symbolicSampleSize;
        data.numSamples = numSamples;

        inputParameterChanges.clearQueue();
        outputParameterChanges.clearQueue();

        toProcessor.flush ([this] (int slot, Vst::ParamValue value)
        {
            addInputParameterChange (slot, 0, value);
        });

        for (const auto& change : parameterChanges)
        {
            if (! isPositiveAndBelow (change.parameterIndex, static_cast<int> (vst3ParameterIds.size())))
                continue;

            const auto slot = findSlotForVST3Id (vst3ParameterIds[static_cast<std::size_t> (change.parameterIndex)]);
            const auto value = static_cast<Vst::ParamValue> (change.normalizedValue);

            addInputParameterChange (slot, change.sampleOffset, value);
            toController.set (slot, value);
        }

        data.inputParameterChanges = &inputParameterChanges;
        data.outputParameterChanges = &outputParameterChanges;
        data.inputEvents = &inputEvents;
        data.outputEvents = &outputEvents;

        if (playHead == nullptr)
            return;

        const auto optPos = playHead->getPosition();
        if (! optPos.has_value())
            return;

        const auto& posInfo = optPos.value();
        vst3ProcessContext = {};
        vst3ProcessContext.sampleRate = getSampleRate();

        if (posInfo.getIsPlaying())
            vst3ProcessContext.state |= Vst::ProcessContext::kPlaying;

        if (posInfo.getIsRecording())
            vst3ProcessContext.state |= Vst::ProcessContext::kRecording;

        if (posInfo.getIsLooping())
            vst3ProcessContext.state |= Vst::ProcessContext::kCycleActive;

        if (auto timeSamples = posInfo.getTimeInSamples())
            vst3ProcessContext.projectTimeSamples = *timeSamples;

        if (auto tempo = posInfo.getBpm())
        {
            vst3ProcessContext.state |= Vst::ProcessContext::kTempoValid;
            vst3ProcessContext.tempo = *tempo;
        }

        if (auto ppq = posInfo.getPpqPosition())
        {
            vst3ProcessContext.state |= Vst::ProcessContext::kProjectTimeMusicValid;
            vst3ProcessContext.projectTimeMusic = *ppq;
        }

        if (auto barPosition = posInfo.getPpqPositionOfLastBarStart())
        {
            vst3ProcessContext.state |= Vst::ProcessContext::kBarPositionValid;
            vst3ProcessContext.barPositionMusic = *barPosition;
        }

        if (auto loopPoints = posInfo.getLoopPoints())
        {
            vst3ProcessContext.state |= Vst::ProcessContext::kCycleValid;
            vst3ProcessContext.cycleStartMusic = loopPoints->ppqStart;
            vst3ProcessContext.cycleEndMusic = loopPoints->ppqEnd;
        }

        if (auto continuousTime = posInfo.getContinuousTimeInSamples())
        {
            vst3ProcessContext.state |= Vst::ProcessContext::kContTimeValid;
            vst3ProcessContext.continousTimeSamples = *continuousTime;
        }

        if (auto timeSignature = posInfo.getTimeSignature())
        {
            vst3ProcessContext.state |= Vst::ProcessContext::kTimeSigValid;
            vst3ProcessContext.timeSigNumerator = timeSignature->numerator;
            vst3ProcessContext.timeSigDenominator = timeSignature->denominator;
        }

        data.processContext = &vst3ProcessContext;
    }

    // Returns the slot and normalized value of the parameter a MIDI controller message is mapped to.
    std::optional<std::pair<int, Vst::ParamValue>> getMidiMappedParameterChange (const MidiMessage& message) const noexcept
    {
        int controller = -1;
        Vst::ParamValue value = 0.0;

        if (message.isController())
        {
            controller = message.getControllerNumber();
            value = message.getControllerValue() / 127.0;
        }
        else if (message.isChannelPressure())
        {
            controller = Vst::kAfterTouch;
            value = message.getChannelPressureValue() / 127.0;
        }
        else if (message.isPitchWheel())
        {
            controller = Vst::kPitchBend;
            value = message.getPitchWheelValue() / 16383.0;
        }

        const int channel = message.getChannel() - 1;
        if (! isPositiveAndBelow (controller, numMidiMappingControllers) || ! isPositiveAndBelow (channel, 16))
            return std::nullopt;

        const auto id = midiControllerMapping[static_cast<std::size_t> (channel * numMidiMappingControllers + controller)].load (std::memory_order_relaxed);
        const auto slot = id != Vst::kNoParamId ? findSlotForVST3Id (id) : -1;

        if (slot < 0)
            return std::nullopt;

        return std::make_pair (slot, value);
    }

    void prepareMidiInputEvents (const MidiBuffer& midiBuffer)
    {
        int count = 0;

        for (const auto& metadata : midiBuffer)
        {
            ignoreUnused (metadata);
            ++count;
        }

        inputEvents.setMaxSize (jmax (64, count));
        inputEvents.clear();

        for (const auto& metadata : midiBuffer)
        {
            if (const auto mapped = getMidiMappedParameterChange (metadata.getMessage()))
            {
                addInputParameterChange (mapped->first, metadata.samplePosition, mapped->second);
                toController.set (mapped->first, mapped->second);
                continue;
            }

            addMidiMessageToVST3Events (inputEvents, metadata);
        }

        outputEvents.setMaxSize (jmax (64, count * 2));
        outputEvents.clear();
    }

    void collectOutputParameterChanges()
    {
        for (int32 i = 0; i < outputParameterChanges.getParameterCount(); ++i)
        {
            auto* queue = outputParameterChanges.getParameterData (i);
            if (queue == nullptr || queue->getPointCount() <= 0)
                continue;

            int32 sampleOffset = 0;
            Vst::ParamValue value = 0.0;
            if (queue->getPoint (queue->getPointCount() - 1, sampleOffset, value) == kResultTrue)
                toController.set (findSlotForVST3Id (queue->getParameterId()), value);
        }
    }

    void collectOutputEvents (MidiBuffer& midiBuffer)
    {
        midiBuffer.clear();

        for (int32 i = 0; i < outputEvents.getEventCount(); ++i)
        {
            Vst::Event event {};
            if (outputEvents.getEvent (i, event) == kResultOk)
                addVST3EventToMidiBuffer (event, midiBuffer);
        }
    }

    void nonRealtimeStateChanged() override
    {
        if (! processingPrepared || vst3Processor == nullptr || getSampleRate() <= 0.0f || getSamplesPerBlock() <= 0)
            return;

        Vst::ProcessSetup setup;
        setup.processMode = isNonRealtime() ? Vst::kOffline : Vst::kRealtime;
        setup.symbolicSampleSize = isUsingDoublePrecision() ? Vst::kSample64 : Vst::kSample32;
        setup.maxSamplesPerBlock = getSamplesPerBlock();
        setup.sampleRate = getSampleRate();
        vst3Processor->setupProcessing (setup);
    }

    //==============================================================================

    AudioPluginHostContext hostContext;
    std::unique_ptr<VST3Module> vst3Module;
    FUID vst3ClassId;
    IPtr<Vst::IHostApplication> vst3HostApplication;
    IPtr<Vst::IComponentHandler> vst3ComponentHandler;
    IPtr<Vst::IComponent> vst3Component;
    IPtr<Vst::IAudioProcessor> vst3Processor;
    IPtr<Vst::IEditController> vst3Controller;

    Vst::ProcessContext vst3ProcessContext {};
    Vst::ParameterChanges inputParameterChanges;
    Vst::ParameterChanges outputParameterChanges;
    Vst::EventList inputEvents;
    Vst::EventList outputEvents;

    std::vector<Vst::AudioBusBuffers> vst3InputBuses;
    std::vector<Vst::AudioBusBuffers> vst3OutputBuses;
    std::vector<float*> channelPointers32;
    std::vector<double*> channelPointers64;
    AudioBuffer<float> scratchBuffer32;
    AudioBuffer<double> scratchBuffer64;
    AudioBuffer<double> doublePrecisionBuffer;
    std::optional<AudioSpec> lastSpec;

    std::vector<Vst::ParamID> vst3ParameterIds;
    std::vector<Vst::ParamID> allVST3ParameterIds;
    std::unordered_map<Vst::ParamID, int> slotsByVST3Id;
    VST3ParameterMailbox toProcessor;
    VST3ParameterMailbox toController;
    std::array<std::atomic<Vst::ParamID>, 16 * numMidiMappingControllers> midiControllerMapping;

    Vst::ParamID bypassParameterId = Vst::kNoParamId;
    Vst::ParamID programChangeParameterId = Vst::kNoParamId;
    Vst::UnitID programChangeUnitId = Vst::kRootUnitId;
    StringArray presetNames;
    int currentPreset = 0;
    int numPresets = 0;

    std::atomic<int32> pendingRestartFlags { 0 };
    std::atomic<bool> bypassControllerSyncPending { false };
    bool processingPrepared = false;
    bool vst3ControllerInitialized = false;
    bool vst3ComponentsConnected = false;
};

//==============================================================================

namespace
{

AudioPluginDescription makeVST3Description (const File& file,
                                            const TUID& classId,
                                            const String& name,
                                            const String& vendor,
                                            const String& version,
                                            const String& subCategories)
{
    AudioPluginDescription desc;
    desc.formatType = AudioPluginFormatType::vst3;
    desc.fileOrBundlePath = file.getFullPathName();
    desc.identifier = classIDToString (classId);
    desc.name = name;
    desc.vendor = vendor;
    desc.version = version;
    desc.category = subCategories;
    desc.isInstrument = subCategories.containsIgnoreCase ("Instrument");
    desc.isEffect = ! desc.isInstrument;

    if (desc.isInstrument)
    {
        desc.numOutputChannels = 2;
        desc.numMidiInputPorts = 1;
    }
    else
    {
        desc.numInputChannels = 2;
        desc.numOutputChannels = 2;
    }

    return desc;
}

// Reads the descriptions from the bundle's moduleinfo.json without loading the module.
std::optional<std::vector<AudioPluginDescription>> scanVST3ModuleInfo (const File& bundle)
{
    if (! bundle.isDirectory())
        return std::nullopt;

    File moduleInfoFile;
    for (const auto* relativePath : { "Contents/Resources/moduleinfo.json", "Contents/moduleinfo.json" })
    {
        if (const auto candidate = bundle.getChildFile (relativePath); candidate.existsAsFile())
        {
            moduleInfoFile = candidate;
            break;
        }
    }

    if (moduleInfoFile == File())
        return std::nullopt;

    const auto json = moduleInfoFile.loadFileAsString().toStdString();
    const auto moduleInfo = ModuleInfoLib::parseJson (json, nullptr);
    if (! moduleInfo.has_value())
        return std::nullopt;

    std::vector<AudioPluginDescription> results;

    for (const auto& classInfo : moduleInfo->classes)
    {
        if (classInfo.category != kVstAudioEffectClass)
            continue;

        FUID classId;
        if (! classId.fromString (classInfo.cid.c_str()))
            continue;

        StringArray subCategories;
        for (const auto& subCategory : classInfo.subCategories)
            subCategories.add (String (subCategory));

        const auto& vendor = classInfo.vendor.empty() ? moduleInfo->factoryInfo.vendor : classInfo.vendor;

        results.push_back (makeVST3Description (bundle,
                                                classId.toTUID(),
                                                String (classInfo.name),
                                                String (vendor),
                                                String (classInfo.version),
                                                subCategories.joinIntoString ("|")));
    }

    return results;
}

// Reads the descriptions from the factory class info. Loads the module but creates no instance.
ResultValue<std::vector<AudioPluginDescription>> scanVST3Factory (const File& file)
{
    auto mod = VST3Module::load (file);
    if (mod == nullptr)
        return makeResultValueFail ("Failed to load VST3 module: " + file.getFullPathName());

    IPluginFactory* rawFactory = mod->getFactory();
    if (rawFactory == nullptr)
        return makeResultValueFail ("No factory in " + file.getFullPathName());

    IPtr<IPluginFactory> factory (rawFactory);
    auto* factory2 = FUnknownPtr<IPluginFactory2> (factory).getInterface();

    std::vector<AudioPluginDescription> results;

    for (int i = 0; i < factory->countClasses(); ++i)
    {
        PClassInfo2 info2 {};
        if (factory2 != nullptr)
        {
            if (factory2->getClassInfo2 (i, &info2) != kResultOk)
                continue;
        }
        else
        {
            PClassInfo info;
            if (factory->getClassInfo (i, &info) != kResultOk)
                continue;

            std::memcpy (info2.cid, info.cid, sizeof (TUID));
            std::memcpy (info2.name, info.name, PClassInfo::kNameSize);
            std::memcpy (info2.category, info.category, PClassInfo::kCategorySize);
        }

        if (String (info2.category) != kVstAudioEffectClass)
            continue;

        results.push_back (makeVST3Description (file,
                                                info2.cid,
                                                String (info2.name),
                                                String (info2.vendor),
                                                String (info2.version),
                                                String (info2.subCategories)));
    }

    return makeResultValueOk (std::move (results));
}

} // namespace

//==============================================================================

VST3Format::VST3Format() = default;
VST3Format::~VST3Format() = default;

AudioPluginFormatType VST3Format::getFormatType() const
{
    return AudioPluginFormatType::vst3;
}

String VST3Format::getFormatName() const
{
    return "VST3";
}

StringArray VST3Format::getFileExtensions() const
{
    return { ".vst3" };
}

FileSearchPath VST3Format::getDefaultSearchPaths() const
{
    FileSearchPath paths;

#if YUP_MAC
    paths.add (File ("/Library/Audio/Plug-Ins/VST3"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory)
                   .getChildFile ("Library/Audio/Plug-Ins/VST3"));
#elif YUP_WINDOWS
    // %CommonProgramFiles%\VST3
    if (const char* pf = getenv ("CommonProgramFiles"))
        paths.add (File (String (pf) + "\\VST3"));
    // %APPDATA%\VST3
    if (const char* appdata = getenv ("APPDATA"))
        paths.add (File (String (appdata) + "\\VST3"));
#elif YUP_LINUX
    paths.add (File ("/usr/lib/vst3"));
    paths.add (File ("/usr/local/lib/vst3"));
    paths.add (File::getSpecialLocation (File::userHomeDirectory).getChildFile (".vst3"));
#endif

    return paths;
}

ResultValue<std::vector<AudioPluginDescription>> VST3Format::scanFile (const File& file)
{
    if (file.getFileExtension().toLowerCase() != ".vst3")
        return makeResultValueFail ("Not a VST3 file");

    YUP_MODULE_DBG (PLUGIN_HOST_VST3, "scanning: " << file.getFullPathName());

    std::vector<AudioPluginDescription> results;

    if (auto fromModuleInfo = scanVST3ModuleInfo (file))
    {
        YUP_MODULE_DBG (PLUGIN_HOST_VST3, "using moduleinfo.json: " << file.getFileName());
        results = std::move (*fromModuleInfo);
    }
    else
    {
        auto fromFactory = scanVST3Factory (file);
        if (fromFactory.failed())
        {
            YUP_MODULE_DBG (PLUGIN_HOST_VST3, fromFactory.getErrorMessage());
            return makeResultValueFail (fromFactory.getErrorMessage());
        }

        results = std::move (fromFactory).getValue();
    }

    YUP_MODULE_DBG (PLUGIN_HOST_VST3, "scan complete: " << results.size() << " plugins in " << file.getFileName());

    if (results.empty())
        return makeResultValueFail ("No Audio Module Class entries in " + file.getFullPathName());

    return makeResultValueOk (std::move (results));
}

ResultValue<std::unique_ptr<AudioPluginInstance>> VST3Format::loadPlugin (
    const AudioPluginDescription& description,
    const AudioPluginHostContext& context)
{
    YUP_MODULE_DBG (PLUGIN_HOST_VST3, "loading: " << description.name << " [" << description.identifier << "]");

    auto instance = VST3Instance::create (description, context);

    if (instance == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_HOST_VST3, "load failed: " << description.name);
        return makeResultValueFail ("Failed to load VST3 plugin: " + description.name);
    }

    YUP_MODULE_DBG (PLUGIN_HOST_VST3, "loaded: " << description.name);
    return makeResultValueOk (std::move (instance));
}

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
