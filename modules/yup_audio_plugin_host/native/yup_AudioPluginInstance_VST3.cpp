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

    static std::unique_ptr<VST3Module> load (const File& file)
    {
        auto m = std::make_unique<VST3Module>();
        auto libraryFile = file;

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
// Minimal IComponentHandler stub — required by IAudioProcessor::initialize().
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
};

IMPLEMENT_FUNKNOWN_METHODS (VST3EditorFrame, IPlugFrame, IPlugFrame::iid)

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

    void paint (Graphics& g) override
    {
        g.setFillColor (Color (0xff101417));
        g.fillAll();
    }

    void resized() override
    {
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

        if (auto* topLevel = getTopLevelComponent())
            topLevel->setSize (preferredSize.to<float>());
        else
            setSize (preferredSize.to<float>());

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
        resizePlugViewToBounds();
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

        plugView->onSize (std::addressof (size));
    }

    IPtr<IPlugView> plugView;
    IPtr<IPlugFrame> frame;
    Size<int> preferredSize { 640, 480 };
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

class VST3Instance : public AudioPluginInstance
{
public:
    VST3Instance (const AudioPluginDescription& desc,
                  const AudioPluginHostContext& context,
                  std::unique_ptr<VST3Module> module,
                  IPtr<Vst::IHostApplication> hostApplication,
                  IPtr<Vst::IComponentHandler> componentHandler,
                  IPtr<Vst::IComponent> component,
                  IPtr<Vst::IAudioProcessor> processor,
                  IPtr<Vst::IEditController> controller,
                  bool controllerWasInitialized)
        : AudioPluginInstance (desc, buildBusLayout (component.get()))
        , hostContext (context)
        , vst3Module (std::move (module))
        , vst3HostApplication (std::move (hostApplication))
        , vst3ComponentHandler (std::move (componentHandler))
        , vst3Component (std::move (component))
        , vst3Processor (std::move (processor))
        , vst3Controller (std::move (controller))
        , vst3ControllerInitialized (controllerWasInitialized)
    {
        if (auto* handler = static_cast<HostComponentHandler*> (vst3ComponentHandler.get()))
        {
            handler->setRestartCallback ([this] (int32 flags)
            {
                handleRestartComponent (flags);
            });
        }

        connectComponentAndController();
        buildParameterList();

        if (auto* handler = static_cast<HostComponentHandler*> (vst3ComponentHandler.get()))
        {
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
    }

    ~VST3Instance() override
    {
        disconnectComponentAndController();
        releaseResources();

        if (auto* handler = static_cast<HostComponentHandler*> (vst3ComponentHandler.get()))
        {
            handler->setRestartCallback (nullptr);
            handler->setParameterEditCallbacks (nullptr, nullptr, nullptr);
        }

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

    void prepareToPlay (float sampleRate, int maxBlockSize) override
    {
        Vst::ProcessSetup setup;
        setup.processMode = isNonRealtime() ? Vst::kOffline : Vst::kRealtime;
        setProcessingPrecision (hostContext.preferDoublePrecision && supportsDoublePrecisionProcessing()
                                    ? ProcessingPrecision::doublePrecision
                                    : ProcessingPrecision::singlePrecision);

        setup.symbolicSampleSize = isUsingDoublePrecision() ? Vst::kSample64 : Vst::kSample32;
        setup.maxSamplesPerBlock = maxBlockSize;
        setup.sampleRate = sampleRate;

        const int numInputs = vst3Component->getBusCount (Vst::kAudio, Vst::kInput);
        const int numOutputs = vst3Component->getBusCount (Vst::kAudio, Vst::kOutput);

        if (isUsingDoublePrecision())
        {
            const int numChannels = jmax (1, numInputs, numOutputs);
            doublePrecisionBuffer.setSize (numChannels, maxBlockSize, false, true, false);
        }

        vst3Processor->setupProcessing (setup);
        processingPrepared = true;

        for (int i = 0; i < numInputs; ++i)
            vst3Component->activateBus (Vst::kAudio, Vst::kInput, i, true);

        for (int i = 0; i < numOutputs; ++i)
            vst3Component->activateBus (Vst::kAudio, Vst::kOutput, i, true);

        const int numEventInputs = vst3Component->getBusCount (Vst::kEvent, Vst::kInput);
        for (int i = 0; i < numEventInputs; ++i)
            vst3Component->activateBus (Vst::kEvent, Vst::kInput, i, true);

        const int numEventOutputs = vst3Component->getBusCount (Vst::kEvent, Vst::kOutput);
        for (int i = 0; i < numEventOutputs; ++i)
            vst3Component->activateBus (Vst::kEvent, Vst::kOutput, i, true);

        vst3Component->setActive (true);
        vst3Processor->setProcessing (true);
    }

    void releaseResources() override
    {
        if (vst3Processor != nullptr)
            vst3Processor->setProcessing (false);

        if (vst3Component != nullptr)
            vst3Component->setActive (false);

        processingPrepared = false;
    }

    void processBlock (AudioProcessContext<float>& context) override
    {
        ScopedNoDenormals noDenormals;

        if (isBypassed())
        {
            processBlockBypassed (context);
            return;
        }

        auto& audioBuffer = context.audio;
        auto& midiBuffer = context.midi;

        if (isUsingDoublePrecision())
        {
            doublePrecisionBuffer.makeCopyOf (audioBuffer, true);
            AudioProcessContext<double> doubleCtx { doublePrecisionBuffer, midiBuffer, context.params, context.samplePosition };
            processBlock (doubleCtx);

            const int numChannels = jmin (audioBuffer.getNumChannels(), doublePrecisionBuffer.getNumChannels());
            const int numSamples = jmin (audioBuffer.getNumSamples(), doublePrecisionBuffer.getNumSamples());

            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* destination = audioBuffer.getWritePointer (channel);
                const auto* source = doublePrecisionBuffer.getReadPointer (channel);

                FloatVectorOperations::convertDoubleToFloat (destination, source, numSamples);
            }

            return;
        }

        Vst::ProcessData data {};
        prepareProcessData (data, audioBuffer.getNumSamples(), Vst::kSample32, context.params);
        prepareMidiInputEvents (midiBuffer);

        // Input busses
        Vst::AudioBusBuffers inputBus {};
        if (vst3Component->getBusCount (Vst::kAudio, Vst::kInput) > 0 && audioBuffer.getNumChannels() > 0)
        {
            inputBus.numChannels = audioBuffer.getNumChannels();
            inputBus.channelBuffers32 = const_cast<float**> (audioBuffer.getArrayOfReadPointers());
            data.inputs = &inputBus;
            data.numInputs = 1;
        }

        // Output busses
        Vst::AudioBusBuffers outputBus {};
        if (vst3Component->getBusCount (Vst::kAudio, Vst::kOutput) > 0 && audioBuffer.getNumChannels() > 0)
        {
            outputBus.numChannels = audioBuffer.getNumChannels();
            outputBus.channelBuffers32 = const_cast<float**> (audioBuffer.getArrayOfWritePointers());
            data.outputs = &outputBus;
            data.numOutputs = 1;
        }

        vst3Processor->process (data);

        collectOutputEvents (midiBuffer);
    }

    void processBlock (AudioProcessContext<double>& context) override
    {
        ScopedNoDenormals noDenormals;

        if (isBypassed())
        {
            processBlockBypassed (context);
            return;
        }

        auto& audioBuffer = context.audio;
        auto& midiBuffer = context.midi;

        if (! isUsingDoublePrecision())
        {
            jassertfalse;
            audioBuffer.clear();
            midiBuffer.clear();
            return;
        }

        Vst::ProcessData data {};
        prepareProcessData (data, audioBuffer.getNumSamples(), Vst::kSample64, context.params);
        prepareMidiInputEvents (midiBuffer);

        Vst::AudioBusBuffers inputBus {};
        if (vst3Component->getBusCount (Vst::kAudio, Vst::kInput) > 0 && audioBuffer.getNumChannels() > 0)
        {
            inputBus.numChannels = audioBuffer.getNumChannels();
            inputBus.channelBuffers64 = const_cast<double**> (audioBuffer.getArrayOfReadPointers());
            data.inputs = &inputBus;
            data.numInputs = 1;
        }

        Vst::AudioBusBuffers outputBus {};
        if (vst3Component->getBusCount (Vst::kAudio, Vst::kOutput) > 0 && audioBuffer.getNumChannels() > 0)
        {
            outputBus.numChannels = audioBuffer.getNumChannels();
            outputBus.channelBuffers64 = const_cast<double**> (audioBuffer.getArrayOfWritePointers());
            data.outputs = &outputBus;
            data.numOutputs = 1;
        }

        vst3Processor->process (data);

        collectOutputEvents (midiBuffer);
    }

    bool supportsDoublePrecisionProcessing() const override
    {
        return vst3Processor != nullptr && vst3Processor->canProcessSampleSize (Vst::kSample64) == kResultTrue;
    }

    //==============================================================================

    int getLatencySamples() override
    {
        return vst3Processor != nullptr ? static_cast<int> (vst3Processor->getLatencySamples()) : 0;
    }

    //==============================================================================

    int getCurrentPreset() const noexcept override { return currentPreset; }

    void setCurrentPreset (int index) noexcept override
    {
        if (vst3Controller != nullptr)
        {
            // VST3 presets are loaded externally; track index only
            currentPreset = index;
        }
    }

    int getNumPresets() const override { return numPresets; }

    String getPresetName (int index) const override
    {
        if (index < 0 || index >= numPresets)
            return {};

        return "Preset " + String (index);
    }

    void setPresetName (int, StringRef) override {}

    //==============================================================================

    Result loadStateFromMemory (const MemoryBlock& memoryBlock) override
    {
        if (vst3Component == nullptr)
            return Result::fail ("Plugin not loaded");

        MemoryBlock mutable_copy = memoryBlock;
        IBStream* stream = new MemoryStream (mutable_copy.getData(),
                                             static_cast<TSize> (mutable_copy.getSize()));

        const auto res = vst3Component->setState (stream);
        stream->release();

        if (res != kResultOk)
            return Result::fail ("VST3 setState() failed");

        return Result::ok();
    }

    Result saveStateIntoMemory (MemoryBlock& memoryBlock) override
    {
        if (vst3Component == nullptr)
            return Result::fail ("Plugin not loaded");

        MemoryStream* stream = new MemoryStream();
        const auto res = vst3Component->getState (stream);

        if (res != kResultOk)
        {
            stream->release();
            return Result::fail ("VST3 getState() failed");
        }

        memoryBlock.replaceAll (stream->getData(), static_cast<std::size_t> (stream->getSize()));
        stream->release();
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

    void handleRestartComponent (int32 flags)
    {
        if ((flags & Vst::kLatencyChanged) != 0)
            setLatencySamples (getLatencySamples());
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

            Vst::IAudioProcessor* rawProcessor = nullptr;
            if (component->queryInterface (Vst::IAudioProcessor::iid,
                                           reinterpret_cast<void**> (&rawProcessor))
                != kResultOk)
                continue;
            IPtr<Vst::IAudioProcessor> processor = IPtr<Vst::IAudioProcessor>::adopt (rawProcessor);

            Vst::IEditController* rawController = nullptr;
            component->queryInterface (Vst::IEditController::iid,
                                       reinterpret_cast<void**> (&rawController));
            IPtr<Vst::IEditController> controller = IPtr<Vst::IEditController>::adopt (rawController);

            bool controllerInitialized = false;
            if (controller == nullptr)
            {
                TUID controllerClassId {};
                if (component->getControllerClassId (controllerClassId) == kResultTrue)
                {
                    if (factory->createInstance (controllerClassId,
                                                 Vst::IEditController::iid,
                                                 reinterpret_cast<void**> (&rawController))
                        == kResultOk)
                    {
                        controller = IPtr<Vst::IEditController>::adopt (rawController);

                        if (controller->initialize (host.get()) != kResultOk)
                            controller = nullptr;
                        else
                            controllerInitialized = true;
                    }
                }
            }

            auto componentHandler = IPtr<Vst::IComponentHandler>::adopt (new HostComponentHandler());
            if (controller != nullptr)
                controller->setComponentHandler (componentHandler.get());

            return std::make_unique<VST3Instance> (desc,
                                                   context,
                                                   std::move (mod),
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
    static AudioBusLayout buildBusLayout (Vst::IComponent* component)
    {
        std::vector<AudioBus> inputs, outputs;

        const int numInputs = component->getBusCount (Vst::kAudio, Vst::kInput);
        for (int i = 0; i < numInputs; ++i)
        {
            Vst::BusInfo info;
            component->getBusInfo (Vst::kAudio, Vst::kInput, i, info);
            inputs.emplace_back (toString (info.name), AudioBus::Type::Audio, AudioBus::Direction::Input, static_cast<int> (info.channelCount));
        }

        const int numOutputs = component->getBusCount (Vst::kAudio, Vst::kOutput);
        for (int i = 0; i < numOutputs; ++i)
        {
            Vst::BusInfo info;
            component->getBusInfo (Vst::kAudio, Vst::kOutput, i, info);
            outputs.emplace_back (toString (info.name), AudioBus::Type::Audio, AudioBus::Direction::Output, static_cast<int> (info.channelCount));
        }

        const int numMidiInputs = component->getBusCount (Vst::kEvent, Vst::kInput);
        for (int i = 0; i < numMidiInputs; ++i)
        {
            Vst::BusInfo info;
            component->getBusInfo (Vst::kEvent, Vst::kInput, i, info);
            inputs.emplace_back (toString (info.name), AudioBus::Type::MIDI, AudioBus::Direction::Input, 1);
        }

        const int numMidiOutputs = component->getBusCount (Vst::kEvent, Vst::kOutput);
        for (int i = 0; i < numMidiOutputs; ++i)
        {
            Vst::BusInfo info;
            component->getBusInfo (Vst::kEvent, Vst::kOutput, i, info);
            outputs.emplace_back (toString (info.name), AudioBus::Type::MIDI, AudioBus::Direction::Output, 1);
        }

        return AudioBusLayout (std::move (inputs), std::move (outputs));
    }

    void buildParameterList()
    {
        if (vst3Controller == nullptr)
            return;

        const int count = vst3Controller->getParameterCount();
        numPresets = 0;

        for (int i = 0; i < count; ++i)
        {
            Vst::ParameterInfo info;
            vst3Controller->getParameterInfo (i, info);

            if (info.flags & Vst::ParameterInfo::kIsProgramChange)
            {
                numPresets = static_cast<int> (info.stepCount) + 1;
                continue;
            }

            auto param = AudioParameterBuilder()
                             .withID (String (static_cast<int64> (info.id)))
                             .withName (toString (info.title))
                             .withRange (0.0f, 1.0f)
                             .withDefault (static_cast<float> (info.defaultNormalizedValue))
                             .build();

            vst3ParameterIds.push_back (info.id);
            addParameter (std::move (param));
        }

        inputParameterChanges.setMaxParameters (static_cast<int32> (vst3ParameterIds.size()));
    }

    int findParameterIndexForVST3Id (Vst::ParamID id) const
    {
        const auto iter = std::find (vst3ParameterIds.begin(), vst3ParameterIds.end(), id);
        if (iter == vst3ParameterIds.end())
            return -1;

        return static_cast<int> (std::distance (vst3ParameterIds.begin(), iter));
    }

    void handleParameterGestureBegin (Vst::ParamID id)
    {
        const auto index = findParameterIndexForVST3Id (id);
        const auto params = getParameters();

        if (isPositiveAndBelow (index, static_cast<int> (params.size())))
            params[static_cast<std::size_t> (index)]->beginChangeGesture();
    }

    void handleParameterEdit (Vst::ParamID id, Vst::ParamValue value)
    {
        const auto index = findParameterIndexForVST3Id (id);
        const auto params = getParameters();

        if (isPositiveAndBelow (index, static_cast<int> (params.size())))
            params[static_cast<std::size_t> (index)]->setNormalizedValue (static_cast<float> (value));
    }

    void handleParameterGestureEnd (Vst::ParamID id)
    {
        const auto index = findParameterIndexForVST3Id (id);
        const auto params = getParameters();

        if (isPositiveAndBelow (index, static_cast<int> (params.size())))
            params[static_cast<std::size_t> (index)]->endChangeGesture();
    }

    bool connectComponentAndController()
    {
        if (vst3Component == nullptr || vst3Controller == nullptr)
            return false;

        Vst::IConnectionPoint* rawComponentConnection = nullptr;
        if (vst3Component->queryInterface (Vst::IConnectionPoint::iid,
                                           reinterpret_cast<void**> (&rawComponentConnection))
            != kResultOk)
        {
            return false;
        }

        Vst::IConnectionPoint* rawControllerConnection = nullptr;
        if (vst3Controller->queryInterface (Vst::IConnectionPoint::iid,
                                            reinterpret_cast<void**> (&rawControllerConnection))
            != kResultOk)
        {
            rawComponentConnection->release();
            return false;
        }

        auto componentConnection = IPtr<Vst::IConnectionPoint>::adopt (rawComponentConnection);
        auto controllerConnection = IPtr<Vst::IConnectionPoint>::adopt (rawControllerConnection);

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

    void disconnectComponentAndController()
    {
        if (! vst3ComponentsConnected || vst3Component == nullptr || vst3Controller == nullptr)
            return;

        Vst::IConnectionPoint* rawComponentConnection = nullptr;
        Vst::IConnectionPoint* rawControllerConnection = nullptr;

        if (vst3Component->queryInterface (Vst::IConnectionPoint::iid,
                                           reinterpret_cast<void**> (&rawComponentConnection))
            != kResultOk)
        {
            return;
        }

        if (vst3Controller->queryInterface (Vst::IConnectionPoint::iid,
                                            reinterpret_cast<void**> (&rawControllerConnection))
            != kResultOk)
        {
            rawComponentConnection->release();
            return;
        }

        auto componentConnection = IPtr<Vst::IConnectionPoint>::adopt (rawComponentConnection);
        auto controllerConnection = IPtr<Vst::IConnectionPoint>::adopt (rawControllerConnection);

        componentConnection->disconnect (controllerConnection.get());
        controllerConnection->disconnect (componentConnection.get());
        vst3ComponentsConnected = false;
    }

    void prepareProcessData (Vst::ProcessData& data,
                             int numSamples,
                             int32 symbolicSampleSize,
                             const ParameterChangeBuffer& parameterChanges)
    {
        data.processMode = isNonRealtime() ? Vst::kOffline : Vst::kRealtime;
        data.symbolicSampleSize = symbolicSampleSize;
        data.numSamples = numSamples;

        inputParameterChanges.clearQueue();

        for (const auto& change : parameterChanges)
        {
            if (! isPositiveAndBelow (change.parameterIndex, static_cast<int> (vst3ParameterIds.size())))
                continue;

            int32 queueIndex = 0;
            if (auto* queue = inputParameterChanges.addParameterData (vst3ParameterIds[static_cast<std::size_t> (change.parameterIndex)], queueIndex))
            {
                int32 pointIndex = 0;
                queue->addPoint (change.sampleOffset,
                                 static_cast<Vst::ParamValue> (change.normalizedValue),
                                 pointIndex);
            }
        }

        data.inputParameterChanges = &inputParameterChanges;
        data.inputEvents = &inputEvents;
        data.outputEvents = &outputEvents;

        inputEvents.clear();
        outputEvents.clear();

        if (hostContext.playHead == nullptr)
            return;

        const auto optPos = hostContext.playHead->getPosition();
        if (! optPos.has_value())
            return;

        const auto& posInfo = optPos.value();
        vst3ProcessContext = {};
        vst3ProcessContext.state = Vst::ProcessContext::kPlaying;
        vst3ProcessContext.sampleRate = getSampleRate();

        if (auto timeSamples = posInfo.getTimeInSamples())
            vst3ProcessContext.projectTimeSamples = *timeSamples;

        if (auto tempo = posInfo.getBpm())
            vst3ProcessContext.tempo = *tempo;

        data.processContext = &vst3ProcessContext;
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
            addMidiMessageToVST3Events (inputEvents, metadata);

        outputEvents.setMaxSize (jmax (64, count * 2));
        outputEvents.clear();
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

    AudioPluginHostContext hostContext;
    std::unique_ptr<VST3Module> vst3Module;
    IPtr<Vst::IHostApplication> vst3HostApplication;
    IPtr<Vst::IComponentHandler> vst3ComponentHandler;
    IPtr<Vst::IComponent> vst3Component;
    IPtr<Vst::IAudioProcessor> vst3Processor;
    IPtr<Vst::IEditController> vst3Controller;
    Vst::ProcessContext vst3ProcessContext {};
    Vst::ParameterChanges inputParameterChanges;
    Vst::EventList inputEvents;
    Vst::EventList outputEvents;
    AudioBuffer<double> doublePrecisionBuffer;
    std::vector<Vst::ParamID> vst3ParameterIds;
    int currentPreset = 0;
    int numPresets = 0;
    bool processingPrepared = false;
    bool vst3ControllerInitialized = false;
    bool vst3ComponentsConnected = false;
};

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
    if (file.getFileExtension().toLowerCase() != ".vst3"
        && ! file.isDirectory())
        return makeResultValueFail ("Not a VST3 file");

    auto mod = VST3Module::load (file);
    if (mod == nullptr)
        return makeResultValueFail ("Failed to load VST3 module: " + file.getFullPathName());

    IPluginFactory* rawFactory = mod->getFactory();
    if (rawFactory == nullptr)
        return makeResultValueFail ("No factory in " + file.getFullPathName());

    IPtr<IPluginFactory> factory (rawFactory);

    std::vector<AudioPluginDescription> results;
    const int classCount = factory->countClasses();

    for (int i = 0; i < classCount; ++i)
    {
        PClassInfo2 info2 {};
        if (auto* factory2 = FUnknownPtr<IPluginFactory2> (factory).getInterface())
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
            info2.vendor[0] = '\0';
            info2.version[0] = '\0';
            info2.subCategories[0] = '\0';
        }

        if (String (info2.category) != "Audio Module Class")
            continue;

        AudioPluginDescription desc;
        desc.formatType = AudioPluginFormatType::vst3;
        desc.fileOrBundlePath = file.getFullPathName();
        desc.name = String (info2.name);
        desc.vendor = String (info2.vendor);
        desc.version = String (info2.version);
        desc.category = String (info2.subCategories);
        desc.isInstrument = String (info2.subCategories).containsIgnoreCase ("Instrument");
        desc.isEffect = ! desc.isInstrument;

        desc.identifier = classIDToString (info2.cid);

        if (desc.isInstrument)
        {
            desc.numInputChannels = 0;
            desc.numOutputChannels = 2;
            desc.numMidiInputPorts = 1;
        }
        else
        {
            desc.numInputChannels = 2;
            desc.numOutputChannels = 2;
        }

        results.push_back (std::move (desc));
    }

    if (results.empty())
        return makeResultValueFail ("No Audio Module Class entries in " + file.getFullPathName());

    return makeResultValueOk (std::move (results));
}

ResultValue<std::unique_ptr<AudioPluginInstance>> VST3Format::loadPlugin (
    const AudioPluginDescription& description,
    const AudioPluginHostContext& context)
{
    auto instance = VST3Instance::create (description, context);

    if (instance == nullptr)
        return makeResultValueFail ("Failed to load VST3 plugin: " + description.name);

    return makeResultValueOk (std::move (instance));
}

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
