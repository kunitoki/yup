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

namespace yup
{

YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wnonnull")

constexpr auto yupAudioObjectPropertyElementMain =
#if defined(MAC_OS_VERSION_12_0)
    kAudioObjectPropertyElementMain;
#else
    kAudioObjectPropertyElementMaster;
#endif

//==============================================================================
enum class PlaybackDirection : std::size_t
{
    input = 0,
    output = 1
};

static constexpr std::size_t toDirectionIndex (PlaybackDirection d) noexcept
{
    return static_cast<std::size_t> (d);
}

static constexpr std::array<PlaybackDirection, 2> getAllPlaybackDirections() noexcept
{
    return { PlaybackDirection::input, PlaybackDirection::output };
}

static constexpr AudioObjectPropertyScope toAudioPropertyScope (PlaybackDirection direction) noexcept
{
    return direction == PlaybackDirection::input ? kAudioDevicePropertyScopeInput
                                                 : kAudioDevicePropertyScopeOutput;
}

//==============================================================================
class PropertyAddress
{
public:
    explicit PropertyAddress (AudioObjectPropertySelector selector) noexcept
        : PropertyAddress (selector, kAudioObjectPropertyScopeGlobal, yupAudioObjectPropertyElementMain)
    {
    }

    PropertyAddress (AudioObjectPropertySelector selector, AudioObjectPropertyScope scope) noexcept
        : PropertyAddress (selector, scope, yupAudioObjectPropertyElementMain)
    {
    }

    PropertyAddress (AudioObjectPropertySelector selector, AudioObjectPropertyScope scope, AudioObjectPropertyElement element) noexcept
        : address { selector, scope, element }
    {
    }

    PropertyAddress (AudioObjectPropertySelector selector, PlaybackDirection direction) noexcept
        : PropertyAddress (selector, toAudioPropertyScope (direction), yupAudioObjectPropertyElementMain)
    {
    }

    PropertyAddress (AudioObjectPropertySelector selector, PlaybackDirection direction, AudioObjectPropertyElement element) noexcept
        : PropertyAddress (selector, toAudioPropertyScope (direction), element)
    {
    }

    const AudioObjectPropertyAddress* get() const noexcept { return &address; }

    operator AudioObjectPropertyAddress() const noexcept { return address; }

private:
    AudioObjectPropertyAddress address;
};

//==============================================================================
class PropertyListener
{
public:
    using Callback = std::function<void (UInt32, const AudioObjectPropertyAddress*)>;

    PropertyListener (AudioObjectID objectIdIn,
                      AudioObjectPropertySelector selector,
                      AudioObjectPropertyScope scope,
                      Callback callbackIn)
        : objectId (objectIdIn)
        , address (selector, scope, kAudioObjectPropertyElementWildcard)
        , callback (std::move (callbackIn))
    {
        if (objectId == kAudioObjectUnknown)
            return;

        AudioObjectAddPropertyListener (objectId, address.get(), listenerCallback, this);
    }

    ~PropertyListener()
    {
        if (objectId == kAudioObjectUnknown)
            return;

        AudioObjectRemovePropertyListener (objectId, address.get(), listenerCallback, this);
    }

private:
    static OSStatus listenerCallback (AudioObjectID,
                                      UInt32 numAddresses,
                                      const AudioObjectPropertyAddress* addrs,
                                      void* clientData)
    {
        static_cast<PropertyListener*> (clientData)->callback (numAddresses, addrs);
        return noErr;
    }

    AudioObjectID objectId = kAudioObjectUnknown;
    PropertyAddress address;
    Callback callback;

    YUP_DECLARE_NON_COPYABLE (PropertyListener)
    YUP_DECLARE_NON_MOVEABLE (PropertyListener)
};

//==============================================================================
class AudioObject
{
public:
    explicit AudioObject (AudioObjectID objectIdIn) noexcept
        : objectId (objectIdIn)
    {
    }

    AudioObject() noexcept = default;
    AudioObject (const AudioObject&) noexcept = default;
    AudioObject (AudioObject&&) noexcept = default;
    AudioObject& operator= (const AudioObject&) noexcept = default;
    AudioObject& operator= (AudioObject&&) noexcept = default;

    AudioObjectID getId() const noexcept { return objectId; }

    bool isValid() const noexcept { return objectId != kAudioObjectUnknown; }

    bool operator== (const AudioObject& other) const noexcept { return objectId == other.objectId; }

    bool operator!= (const AudioObject& other) const noexcept { return ! (*this == other); }

    template <typename PropertyType>
    std::optional<PropertyType> getProperty (PropertyAddress address) const
    {
        if (! hasProperty (address))
            return {};

        UInt32 size = sizeof (PropertyType);
        PropertyType value {};

        if (AudioObjectGetPropertyData (objectId, address.get(), 0, nullptr, &size, &value) != noErr)
            return {};

        return value;
    }

    template <typename PropertyType>
    PropertyType getPropertyOrDefault (PropertyAddress address, PropertyType defaultValue = {}) const
    {
        return getProperty<PropertyType> (address).value_or (defaultValue);
    }

    template <typename PropertyType>
    std::vector<PropertyType> getPropertyArray (PropertyAddress address) const
    {
        if (! hasProperty (address))
            return {};

        UInt32 size = 0;

        if (AudioObjectGetPropertyDataSize (objectId, address.get(), 0, nullptr, &size) != noErr)
            return {};

        jassert ((size % sizeof (PropertyType)) == 0);
        std::vector<PropertyType> result (size / sizeof (PropertyType));

        if (AudioObjectGetPropertyData (objectId, address.get(), 0, nullptr, &size, result.data()) != noErr)
            return {};

        return result;
    }

    template <typename PropertyType>
    bool setProperty (PropertyAddress address, const PropertyType& value)
    {
        if (! hasProperty (address))
            return false;

        Boolean isSettable = NO;

        if (AudioObjectIsPropertySettable (objectId, address.get(), &isSettable) != noErr || ! isSettable)
            return false;

        return AudioObjectSetPropertyData (objectId, address.get(), 0, nullptr, sizeof (PropertyType), &value) == noErr;
    }

    std::unique_ptr<PropertyListener> createPropertyListener (AudioObjectPropertySelector selector,
                                                              AudioObjectPropertyScope scope,
                                                              PropertyListener::Callback callback)
    {
        return std::make_unique<PropertyListener> (objectId, selector, scope, std::move (callback));
    }

private:
    bool hasProperty (PropertyAddress address) const noexcept
    {
        return isValid() && AudioObjectHasProperty (objectId, address.get());
    }

    AudioObjectID objectId = kAudioObjectUnknown;
};

static_assert (sizeof (AudioObject) == sizeof (AudioObjectID));

//==============================================================================
class AudioStream : public AudioObject
{
public:
    using AudioObject::AudioObject;

    int getLatency() const
    {
        return static_cast<int> (getPropertyOrDefault<UInt32> (PropertyAddress (kAudioStreamPropertyLatency)));
    }

    int getBitDepth() const
    {
        return static_cast<int> (getPropertyOrDefault<AudioStreamBasicDescription> (PropertyAddress (kAudioStreamPropertyPhysicalFormat)).mBitsPerChannel);
    }
};

static_assert (sizeof (AudioStream) == sizeof (AudioObject));

//==============================================================================
class AudioDevice : public AudioObject
{
    static constexpr AudioObjectPropertySelector mainVolumeSelector =
#if defined(MAC_OS_VERSION_12_0)
        kAudioHardwareServiceDeviceProperty_VirtualMainVolume;
#else
        kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
#endif

public:
    using AudioObject::AudioObject;

    String getName() const
    {
        const auto cf = getProperty<CFStringRef> (PropertyAddress (kAudioDevicePropertyDeviceNameCFString));
        if (! cf || ! *cf)
            return {};
        const CFUniquePtr<CFStringRef> holder { *cf };
        return String::fromCFString (holder.get());
    }

    String getUid() const
    {
        const auto cf = getProperty<CFStringRef> (PropertyAddress (kAudioDevicePropertyDeviceUID));
        if (! cf || ! *cf)
            return {};
        const CFUniquePtr<CFStringRef> holder { *cf };
        return String::fromCFString (holder.get());
    }

    double getSampleRate() const
    {
        return static_cast<double> (getPropertyOrDefault<Float64> (PropertyAddress (kAudioDevicePropertyNominalSampleRate)));
    }

    bool requestSampleRate (double newRate)
    {
        return setProperty (PropertyAddress (kAudioDevicePropertyNominalSampleRate), static_cast<Float64> (newRate));
    }

    int getBufferSize() const
    {
        return static_cast<int> (getPropertyOrDefault<UInt32> (PropertyAddress (kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeWildcard)));
    }

    bool requestBufferSize (int newSize)
    {
        return setProperty (PropertyAddress (kAudioDevicePropertyBufferFrameSize), static_cast<UInt32> (newSize));
    }

    int getNumChannels (PlaybackDirection direction) const
    {
        if (! isValid())
            return 0;

        const PropertyAddress address { kAudioDevicePropertyStreamConfiguration, direction };

        UInt32 size = 0;

        if (! AudioObjectHasProperty (getId(), address.get()))
            return 0;

        if (AudioObjectGetPropertyDataSize (getId(), address.get(), 0, nullptr, &size) != noErr || size == 0)
            return 0;

        std::vector<std::byte> storage (size);
        const auto* bufList = reinterpret_cast<const AudioBufferList*> (storage.data());

        if (AudioObjectGetPropertyData (getId(), address.get(), 0, nullptr, &size, storage.data()) != noErr)
            return 0;

        int total = 0;
        for (UInt32 i = 0; i < bufList->mNumberBuffers; ++i)
            total += static_cast<int> (bufList->mBuffers[i].mNumberChannels);
        return total;
    }

    std::vector<AudioStream> getStreams (PlaybackDirection direction) const
    {
        const auto ids = getPropertyArray<AudioStreamID> (PropertyAddress (kAudioDevicePropertyStreams, direction));
        std::vector<AudioStream> streams;
        streams.reserve (ids.size());
        for (const auto id : ids)
            streams.emplace_back (id);
        return streams;
    }

    int getLatency (PlaybackDirection direction) const
    {
        return static_cast<int> (getPropertyOrDefault<UInt32> (PropertyAddress (kAudioDevicePropertyLatency, direction)));
    }

    int getSafetyOffset (PlaybackDirection direction) const
    {
        return static_cast<int> (getPropertyOrDefault<UInt32> (PropertyAddress (kAudioDevicePropertySafetyOffset, direction)));
    }

    int getStreamLatency (PlaybackDirection direction) const
    {
        const auto streams = getStreams (direction);
        return streams.empty() ? 0 : streams.front().getLatency();
    }

    int getBitDepth() const
    {
        for (auto direction : getAllPlaybackDirections())
            for (auto stream : getStreams (direction))
                if (const auto depth = stream.getBitDepth(); depth > 0)
                    return depth;

        return 24;
    }

    String getChannelName (PlaybackDirection direction, int index) const
    {
        const auto element = static_cast<AudioObjectPropertyElement> (index + 1);
        const auto cf = getProperty<CFStringRef> (PropertyAddress (kAudioObjectPropertyElementName, direction, element));
        if (! cf || ! *cf)
            return {};
        const CFUniquePtr<CFStringRef> holder { *cf };
        return String::fromCFString (holder.get());
    }

    float getMainVolume() const
    {
        return getPropertyOrDefault<Float32> (PropertyAddress (mainVolumeSelector));
    }

    bool setMainVolume (float newVolume)
    {
        return setProperty (PropertyAddress (mainVolumeSelector), static_cast<Float32> (newVolume));
    }

    bool isMuted() const
    {
        return getPropertyOrDefault<UInt32> (PropertyAddress (kAudioDevicePropertyMute, kAudioDevicePropertyScopeOutput)) != 0;
    }

    bool setMute (bool mute)
    {
        return setProperty (PropertyAddress (kAudioDevicePropertyMute, kAudioDevicePropertyScopeOutput), static_cast<UInt32> (mute ? 1 : 0));
    }

    bool isAlive() const
    {
        return getPropertyOrDefault<UInt32> (PropertyAddress (kAudioDevicePropertyDeviceIsAlive, kAudioObjectPropertyScopeWildcard)) != 0;
    }

    bool isAggregateDevice() const
    {
        return getPropertyOrDefault<AudioClassID> (PropertyAddress (kAudioObjectPropertyClass)) == kAudioAggregateDeviceClassID;
    }

    AudioWorkgroup getAudioWorkgroup() const
    {
#if YUP_AUDIOWORKGROUP_TYPES_AVAILABLE
        if (auto handle = getProperty<void*> (PropertyAddress (kAudioDevicePropertyIOThreadOSWorkgroup, kAudioObjectPropertyScopeWildcard)))
        {
            os_workgroup_t workgroup = (__bridge_transfer os_workgroup_t) * handle;
            return makeRealAudioWorkgroup (workgroup);
        }
#endif
        return {};
    }

    std::unique_ptr<PropertyListener> createPropertyListener (AudioObjectPropertySelector selector,
                                                              PropertyListener::Callback callback)
    {
        return AudioObject::createPropertyListener (selector, kAudioObjectPropertyScopeWildcard, std::move (callback));
    }
};

static_assert (sizeof (AudioDevice) == sizeof (AudioObject));

//==============================================================================
class SystemObject final : public AudioObject
{
public:
    SystemObject() noexcept
        : AudioObject (kAudioObjectSystemObject)
    {
    }

    AudioDevice getDefaultDevice (PlaybackDirection direction) const
    {
        static constexpr AudioObjectPropertySelector selectors[] {
            kAudioHardwarePropertyDefaultInputDevice,
            kAudioHardwarePropertyDefaultOutputDevice
        };
        return AudioDevice (getPropertyOrDefault<AudioDeviceID> (PropertyAddress (selectors[toDirectionIndex (direction)])));
    }

    std::vector<AudioDevice> getAudioDevices() const
    {
        const auto ids = getPropertyArray<AudioDeviceID> (PropertyAddress (kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeWildcard));
        std::vector<AudioDevice> devices;
        devices.reserve (ids.size());
        for (const auto id : ids)
            devices.emplace_back (id);
        return devices;
    }

    AudioDevice translateUidToDevice (const String& uid) const
    {
        if (uid.isEmpty())
            return AudioDevice (kAudioObjectUnknown);

        const CFUniquePtr<CFStringRef> uidString { uid.toCFString() };
        auto* uidRef = uidString.get();
        AudioDeviceID result = kAudioObjectUnknown;
        UInt32 resultSize = sizeof (result);
        const PropertyAddress address { kAudioHardwarePropertyTranslateUIDToDevice };

        AudioObjectGetPropertyData (kAudioObjectSystemObject, address.get(), sizeof (uidRef), &uidRef, &resultSize, &result);
        return AudioDevice (result);
    }
};

//==============================================================================
class AggregateAudioDevice : public AudioDevice
{
public:
    using AudioDevice::AudioDevice;

    std::vector<AudioDevice> getSubDevices() const
    {
        const auto ids = getPropertyArray<AudioDeviceID> (PropertyAddress (kAudioAggregateDevicePropertyActiveSubDeviceList));
        std::vector<AudioDevice> devices;
        devices.reserve (ids.size());
        for (const auto id : ids)
            devices.emplace_back (id);
        return devices;
    }

    String getClockingDeviceUid() const
    {
        constexpr auto mainSubDeviceSelector =
#if defined(MAC_OS_VERSION_12_0)
            kAudioAggregateDevicePropertyMainSubDevice;
#else
            kAudioAggregateDevicePropertyMasterSubDevice;
#endif

        for (const auto selector : { (AudioObjectPropertySelector) kAudioAggregateDevicePropertyClockDevice, (AudioObjectPropertySelector) mainSubDeviceSelector })
        {
            const auto cf = getProperty<CFStringRef> (PropertyAddress (selector));
            if (! cf || ! *cf)
                continue;
            const CFUniquePtr<CFStringRef> holder { *cf };
            const auto uid = String::fromCFString (holder.get());
            if (uid.isNotEmpty())
                return uid;
        }

        const auto subs = getSubDevices();
        return subs.empty() ? String {} : subs.front().getUid();
    }
};

static_assert (sizeof (AggregateAudioDevice) == sizeof (AudioObject));

//==============================================================================
class ManagedAudioBufferList final : public AudioBufferList
{
public:
    struct Deleter
    {
        void operator() (ManagedAudioBufferList* p) const
        {
            if (p != nullptr)
                p->~ManagedAudioBufferList();

            delete[] reinterpret_cast<std::byte*> (p);
        }
    };

    using Ref = std::unique_ptr<ManagedAudioBufferList, Deleter>;

    //==============================================================================
    static Ref create (std::size_t numBuffers)
    {
        static_assert (alignof (ManagedAudioBufferList) <= alignof (std::max_align_t));

        if (std::unique_ptr<std::byte[]> storage { new std::byte[storageSizeForNumBuffers (numBuffers)] })
            return Ref { new (storage.release()) ManagedAudioBufferList (numBuffers) };

        return nullptr;
    }

    //==============================================================================
    static std::size_t storageSizeForNumBuffers (std::size_t numBuffers) noexcept
    {
        return audioBufferListHeaderSize + (numBuffers * sizeof (::AudioBuffer));
    }

    static std::size_t numBuffersForStorageSize (std::size_t bytes) noexcept
    {
        bytes -= audioBufferListHeaderSize;

        // storage size ends between to buffers in AudioBufferList
        jassert ((bytes % sizeof (::AudioBuffer)) == 0);

        return bytes / sizeof (::AudioBuffer);
    }

private:
    // Do not call the base constructor here as this will zero-initialize the first buffer,
    // for which no storage may be available though (when numBuffers == 0).
    explicit ManagedAudioBufferList (std::size_t numBuffers)
    {
        mNumberBuffers = static_cast<UInt32> (numBuffers);
    }

    static constexpr auto audioBufferListHeaderSize = sizeof (AudioBufferList) - sizeof (::AudioBuffer);

    YUP_DECLARE_NON_COPYABLE (ManagedAudioBufferList)
    YUP_DECLARE_NON_MOVEABLE (ManagedAudioBufferList)
};

//==============================================================================
struct IgnoreUnused
{
    template <typename... Ts>
    void operator() (Ts&&...) const
    {
    }
};

template <typename T>
static auto getDataPtrAndSize (T& t)
{
    static_assert (std::is_pod_v<T>);
    return std::make_tuple (&t, (UInt32) sizeof (T));
}

static auto getDataPtrAndSize (ManagedAudioBufferList::Ref& t)
{
    const auto size = t.get() != nullptr
                        ? ManagedAudioBufferList::storageSizeForNumBuffers (t->mNumberBuffers)
                        : 0;
    return std::make_tuple (t.get(), (UInt32) size);
}

//==============================================================================
[[nodiscard]] static bool audioObjectHasProperty (AudioObjectID objectID, const AudioObjectPropertyAddress address)
{
    return objectID != kAudioObjectUnknown && AudioObjectHasProperty (objectID, &address);
}

template <typename T, typename OnError = IgnoreUnused>
[[nodiscard]] static auto audioObjectGetProperty (AudioObjectID objectID,
                                                  const AudioObjectPropertyAddress address,
                                                  OnError&& onError = {})
{
    using Result = std::conditional_t<std::is_same_v<T, AudioBufferList>, ManagedAudioBufferList::Ref, std::optional<T>>;

    if (! audioObjectHasProperty (objectID, address))
        return Result {};

    auto result = [&]
    {
        if constexpr (std::is_same_v<T, AudioBufferList>)
        {
            UInt32 size {};

            if (auto status = AudioObjectGetPropertyDataSize (objectID, &address, 0, nullptr, &size); status != noErr)
            {
                onError (status);
                return Result {};
            }

            return ManagedAudioBufferList::create (ManagedAudioBufferList::numBuffersForStorageSize (size));
        }
        else
        {
            return T {};
        }
    }();

    auto [ptr, size] = getDataPtrAndSize (result);

    if (size == 0)
        return Result {};

    if (auto status = AudioObjectGetPropertyData (objectID, &address, 0, nullptr, &size, ptr); status != noErr)
    {
        onError (status);
        return Result {};
    }

    return Result { std::move (result) };
}

template <typename T, typename OnError = IgnoreUnused>
static bool audioObjectSetProperty (AudioObjectID objectID,
                                    const AudioObjectPropertyAddress address,
                                    const T value,
                                    OnError&& onError = {})
{
    if (! audioObjectHasProperty (objectID, address))
        return false;

    Boolean isSettable = NO;
    if (auto status = AudioObjectIsPropertySettable (objectID, &address, &isSettable); status != noErr)
    {
        onError (status);
        return false;
    }

    if (! isSettable)
        return false;

    if (auto status = AudioObjectSetPropertyData (objectID, &address, 0, nullptr, static_cast<UInt32> (sizeof (T)), &value); status != noErr)
    {
        onError (status);
        return false;
    }

    return true;
}

template <typename T, typename OnError = IgnoreUnused>
[[nodiscard]] static std::vector<T> audioObjectGetProperties (AudioObjectID objectID,
                                                              const AudioObjectPropertyAddress address,
                                                              OnError&& onError = {})
{
    if (! audioObjectHasProperty (objectID, address))
        return {};

    UInt32 size {};

    if (auto status = AudioObjectGetPropertyDataSize (objectID, &address, 0, nullptr, &size); status != noErr)
    {
        onError (status);
        return {};
    }

    // If this is hit, the number of results is not integral, and the following
    // AudioObjectGetPropertyData will probably write past the end of the result buffer.
    jassert ((size % sizeof (T)) == 0);
    std::vector<T> result (size / sizeof (T));

    if (auto status = AudioObjectGetPropertyData (objectID, &address, 0, nullptr, &size, result.data()); status != noErr)
    {
        onError (status);
        return {};
    }

    return result;
}

static AudioObjectPropertyScope getAudioDevicePropertyScope (bool input) noexcept
{
    return toAudioPropertyScope (input ? PlaybackDirection::input : PlaybackDirection::output);
}

constexpr auto yupPrivateAggregateDeviceNamePrefix = "YUP Aggregate ";
constexpr auto yupPrivateAggregateDevicePIDMarker = "pid=";

static String createPrivateAggregateDeviceName (const String& deviceName)
{
    return String (yupPrivateAggregateDeviceNamePrefix) + yupPrivateAggregateDevicePIDMarker + String ((int) ::getpid()) + " " + deviceName;
}

static pid_t getPrivateAggregateDeviceProcessID (const String& name)
{
    if (! name.startsWith (yupPrivateAggregateDeviceNamePrefix))
        return 0;

    const auto suffix = name.fromFirstOccurrenceOf (yupPrivateAggregateDeviceNamePrefix, false, false).trimStart();
    if (! suffix.startsWith (yupPrivateAggregateDevicePIDMarker))
        return 0;

    const auto pidAndName = suffix.fromFirstOccurrenceOf (yupPrivateAggregateDevicePIDMarker, false, false);
    const auto pidString = pidAndName.initialSectionContainingOnly ("0123456789");

    if (pidString.isEmpty())
        return 0;

    const auto numDigits = pidString.length();
    if (pidAndName.length() <= numDigits || ! CharacterFunctions::isWhitespace (pidAndName[numDigits]))
        return 0;

    return (pid_t) pidString.getIntValue();
}

static int getDirectionIndex (bool input) noexcept
{
    return static_cast<int> (toDirectionIndex (input ? PlaybackDirection::input : PlaybackDirection::output));
}

static String audioObjectGetStringProperty (AudioObjectID objectID, AudioObjectPropertySelector selector)
{
    if (auto retainedString = audioObjectGetProperty<CFStringRef> (objectID, PropertyAddress (selector)))
    {
        if (*retainedString != nullptr)
        {
            const CFUniquePtr<CFStringRef> stringHolder { *retainedString };
            return String::fromCFString (stringHolder.get());
        }
    }

    return {};
}

static String getAudioDeviceUID (AudioDeviceID deviceID)
{
    return audioObjectGetStringProperty (deviceID, kAudioDevicePropertyDeviceUID);
}

static String getAudioDeviceName (AudioDeviceID deviceID)
{
    return audioObjectGetStringProperty (deviceID, kAudioDevicePropertyDeviceNameCFString);
}

static AudioDeviceID getAudioDeviceIDForUID (const String& uid)
{
    if (uid.isEmpty())
        return kAudioObjectUnknown;

    const CFUniquePtr<CFStringRef> uidString { uid.toCFString() };
    auto* uidRef = uidString.get();
    AudioDeviceID result = kAudioObjectUnknown;
    UInt32 resultSize = sizeof (result);
    const PropertyAddress address { kAudioHardwarePropertyTranslateUIDToDevice };

    if (AudioObjectGetPropertyData (kAudioObjectSystemObject, address.get(), sizeof (uidRef), &uidRef, &resultSize, &result) != noErr)
        return kAudioObjectUnknown;

    return result;
}

static bool isAggregateAudioDevice (AudioDeviceID deviceID)
{
    return audioObjectGetProperty<AudioClassID> (deviceID, PropertyAddress (kAudioObjectPropertyClass)).value_or (0) == kAudioAggregateDeviceClassID;
}

static int getDirectNumChannelsForDevice (AudioDeviceID deviceID, bool input)
{
    int total = 0;

    if (auto bufList = audioObjectGetProperty<AudioBufferList> (deviceID, PropertyAddress (kAudioDevicePropertyStreamConfiguration, getAudioDevicePropertyScope (input))))
    {
        const auto numStreams = static_cast<int> (bufList->mNumberBuffers);

        for (int i = 0; i < numStreams; ++i)
            total += static_cast<int> (bufList->mBuffers[i].mNumberChannels);
    }

    return total;
}

static std::vector<AudioDeviceID> getAggregateSubDeviceIDs (AudioDeviceID deviceID)
{
    return audioObjectGetProperties<AudioDeviceID> (deviceID, PropertyAddress (kAudioAggregateDevicePropertyActiveSubDeviceList));
}

static void addAudioDeviceIDIfMissing (std::vector<AudioDeviceID>& deviceIDs, AudioDeviceID deviceID)
{
    if (deviceID == kAudioObjectUnknown || deviceID == 0)
        return;

    if (std::find (deviceIDs.begin(), deviceIDs.end(), deviceID) == deviceIDs.end())
        deviceIDs.push_back (deviceID);
}

static void appendFlattenedAudioDeviceIDs (std::vector<AudioDeviceID>& result, AudioDeviceID deviceID, int depth = 0)
{
    if (deviceID == kAudioObjectUnknown || deviceID == 0)
        return;

    if (depth > 8 || ! isAggregateAudioDevice (deviceID))
    {
        addAudioDeviceIDIfMissing (result, deviceID);
        return;
    }

    auto subDeviceIDs = getAggregateSubDeviceIDs (deviceID);

    if (subDeviceIDs.empty())
    {
        addAudioDeviceIDIfMissing (result, deviceID);
        return;
    }

    for (const auto subDeviceID : subDeviceIDs)
        appendFlattenedAudioDeviceIDs (result, subDeviceID, depth + 1);
}

static std::vector<AudioDeviceID> getFlattenedAudioDeviceIDs (AudioDeviceID deviceID)
{
    std::vector<AudioDeviceID> result;
    appendFlattenedAudioDeviceIDs (result, deviceID);
    return result;
}

static int getNumChannelsForAudioDevice (AudioDeviceID deviceID, bool input)
{
    if (isAggregateAudioDevice (deviceID))
    {
        auto total = 0;

        for (const auto subDeviceID : getFlattenedAudioDeviceIDs (deviceID))
            total += getDirectNumChannelsForDevice (subDeviceID, input);

        if (total > 0)
            return total;
    }

    return getDirectNumChannelsForDevice (deviceID, input);
}

struct AggregateSubDeviceChannelGroup
{
    String name;
    int remainingChannels = 0;
};

static std::vector<AggregateSubDeviceChannelGroup> getAggregateSubDeviceChannelGroups (AudioDeviceID deviceID, bool input)
{
    std::vector<AggregateSubDeviceChannelGroup> result;

    if (! isAggregateAudioDevice (deviceID))
        return result;

    for (const auto subDeviceID : getFlattenedAudioDeviceIDs (deviceID))
    {
        const auto numChannels = getDirectNumChannelsForDevice (subDeviceID, input);

        if (numChannels > 0)
            result.push_back ({ getAudioDeviceName (subDeviceID), numChannels });
    }

    return result;
}

static String describeChannelMap (const std::vector<int>& channelMap)
{
    String result ("[");

    for (std::size_t i = 0; i < channelMap.size(); ++i)
    {
        if (i != 0)
            result << ", ";

        result << String (channelMap[i]);
    }

    result << "]";
    return result;
}

static String describeChannelBits (const BigInteger& channels)
{
    return "set=" + String (channels.countNumberOfSetBits())
         + ", highest=" + String (channels.getHighestBit())
         + ", bits=" + channels.toString (2);
}

static String describeAudioDeviceID (AudioDeviceID deviceID)
{
    if (deviceID == kAudioObjectUnknown || deviceID == 0)
        return "none";

    auto result = getAudioDeviceName (deviceID);

    if (result.isEmpty())
        result = "<unnamed>";

    result << " [" << String (deviceID) << "]";

    if (const auto uid = getAudioDeviceUID (deviceID); uid.isNotEmpty())
        result << ", uid=" << uid;

    result << ", inputs=" << String (getNumChannelsForAudioDevice (deviceID, true))
           << ", outputs=" << String (getNumChannelsForAudioDevice (deviceID, false));

    if (isAggregateAudioDevice (deviceID))
        result << ", aggregate";

    return result;
}

YUP_END_IGNORE_WARNINGS_GCC_LIKE

#define YUP_SYSTEMAUDIOVOL_IMPLEMENTED 1

float YUP_CALLTYPE SystemAudioVolume::getGain()
{
    return SystemObject {}.getDefaultDevice (PlaybackDirection::output).getMainVolume();
}

bool YUP_CALLTYPE SystemAudioVolume::setGain (float gain)
{
    return SystemObject {}.getDefaultDevice (PlaybackDirection::output).setMainVolume (gain);
}

bool YUP_CALLTYPE SystemAudioVolume::isMuted()
{
    return SystemObject {}.getDefaultDevice (PlaybackDirection::output).isMuted();
}

bool YUP_CALLTYPE SystemAudioVolume::setMuted (bool mute)
{
    return SystemObject {}.getDefaultDevice (PlaybackDirection::output).setMute (mute);
}

//==============================================================================
template <typename T>
static T findNearestValue (const Array<T>& values, T target)
{
    if (values.isEmpty())
        return target;

    const auto it = std::lower_bound (values.begin(), values.end(), target);

    if (it == values.begin())
        return *it;

    if (it == values.end())
        return *(it - 1);

    const T upper = *it;
    const T lower = *(it - 1);
    return std::abs (target - lower) < std::abs (target - upper) ? lower : upper;
}

template <typename Fn>
static bool tryMultiple (Fn predicate, int maxTries)
{
    if (predicate())
        return true;

    for (int i = 1; i < maxTries; ++i)
    {
        Thread::yield();
        if (predicate())
            return true;
    }

    return false;
}

//==============================================================================
class ScopedCFArray;

class ScopedCFDictionary
{
public:
    void setString (const String& key, const String& value)
    {
        const CFUniquePtr<CFStringRef> cfValue { value.toCFString() };
        setRawValue (key, cfValue.get());
    }

    void setInt (const String& key, UInt32 value)
    {
        const CFUniquePtr<CFNumberRef> cfValue { CFNumberCreate (nullptr, kCFNumberIntType, &value) };
        setRawValue (key, cfValue.get());
    }

    void setArray (const String& key, const ScopedCFArray& array);

    CFDictionaryRef get() const noexcept { return dict.get(); }

private:
    void setRawValue (const String& key, const void* value)
    {
        const CFUniquePtr<CFStringRef> cfKey { key.toCFString() };
        CFDictionarySetValue (dict.get(), cfKey.get(), value);
    }

    CFUniquePtr<CFMutableDictionaryRef> dict { CFDictionaryCreateMutable (nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks) };
};

class ScopedCFArray
{
public:
    void appendDictionary (const ScopedCFDictionary& dictionary)
    {
        CFArrayAppendValue (array.get(), dictionary.get());
    }

    CFArrayRef get() const noexcept { return array.get(); }

private:
    CFUniquePtr<CFMutableArrayRef> array { CFArrayCreateMutable (nullptr, 0, &kCFTypeArrayCallBacks) };
};

inline void ScopedCFDictionary::setArray (const String& key, const ScopedCFArray& arr)
{
    setRawValue (key, arr.get());
}

//==============================================================================
struct AggregateDeviceDescription
{
    struct SubDevice
    {
        AudioDeviceID deviceID = kAudioObjectUnknown;
        String uid;
    };

    String name;
    AudioDeviceID clockDeviceID = kAudioObjectUnknown;
    std::vector<SubDevice> subDevices;
    std::array<std::vector<int>, 2> channelMaps;

    AggregateDeviceDescription (const String& nameIn, AudioDeviceID inputDeviceID, AudioDeviceID outputDeviceID)
        : name (nameIn)
    {
        clockDeviceID = findClockingDeviceID (outputDeviceID);

        if (clockDeviceID == kAudioObjectUnknown)
            clockDeviceID = findClockingDeviceID (inputDeviceID);

        addDevice (outputDeviceID, PlaybackDirection::output);
        addDevice (inputDeviceID, PlaybackDirection::input);
    }

    bool isEmpty() const noexcept { return subDevices.empty(); }

    AudioDeviceID createAggregateDevice() const
    {
        AudioDeviceID result = kAudioObjectUnknown;
        const auto dict = toDictionary();
        const OSStatus status = AudioHardwareCreateAggregateDevice (dict.get(), &result);
        return status == noErr ? result : kAudioObjectUnknown;
    }

private:
    static AudioDeviceID findClockingDeviceID (AudioDeviceID deviceID)
    {
        if (deviceID == kAudioObjectUnknown || deviceID == 0)
            return kAudioObjectUnknown;

        if (! isAggregateAudioDevice (deviceID))
            return deviceID;

        const AggregateAudioDevice aggregate (deviceID);
        const auto uid = aggregate.getClockingDeviceUid();

        if (uid.isNotEmpty())
        {
            if (const auto translated = SystemObject {}.translateUidToDevice (uid); translated.isValid())
                return translated.getId();
        }

        const auto subs = getFlattenedAudioDeviceIDs (deviceID);
        return subs.empty() ? kAudioObjectUnknown : subs.front();
    }

    int getFirstChannelIndex (AudioDeviceID deviceID, PlaybackDirection direction) const
    {
        int index = 0;

        for (const auto& sub : subDevices)
        {
            if (sub.deviceID == deviceID)
                break;

            index += getDirectNumChannelsForDevice (sub.deviceID, direction == PlaybackDirection::input);
        }

        return index;
    }

    void addDevice (AudioDeviceID deviceID, PlaybackDirection direction)
    {
        if (deviceID == kAudioObjectUnknown || deviceID == 0)
            return;

        for (const auto subDeviceID : getFlattenedAudioDeviceIDs (deviceID))
        {
            const auto uid = getAudioDeviceUID (subDeviceID);

            if (std::none_of (subDevices.begin(), subDevices.end(), [subDeviceID] (const auto& s)
            {
                return s.deviceID == subDeviceID;
            }))
                subDevices.push_back ({ subDeviceID, uid });

            const auto numChannels = getDirectNumChannelsForDevice (subDeviceID, direction == PlaybackDirection::input);
            const auto baseChannel = getFirstChannelIndex (subDeviceID, direction);
            auto& map = channelMaps[toDirectionIndex (direction)];

            for (int ch = 0; ch < numChannels; ++ch)
                map.push_back (baseChannel + ch);
        }
    }

    ScopedCFDictionary toDictionary() const
    {
        const auto clockUid = getAudioDeviceUID (clockDeviceID);

        static constexpr UInt32 kDriftCompensationMaxQuality = 0x7F;

        ScopedCFArray subDeviceArray;

        for (const auto& sub : subDevices)
        {
            ScopedCFDictionary subDict;
            subDict.setString (kAudioSubDeviceUIDKey, sub.uid);
            subDict.setInt (kAudioSubDeviceDriftCompensationKey, sub.deviceID != clockDeviceID ? 1 : 0);
            subDict.setInt (kAudioSubDeviceDriftCompensationQualityKey, kDriftCompensationMaxQuality);
            subDeviceArray.appendDictionary (subDict);
        }

        constexpr auto mainSubDeviceKey =
#if defined(kAudioAggregateDeviceMainSubDeviceKey)
            kAudioAggregateDeviceMainSubDeviceKey;
#else
            kAudioAggregateDeviceMasterSubDeviceKey;
#endif

        ScopedCFDictionary description;
        description.setString (kAudioAggregateDeviceUIDKey, Uuid().toString());
        description.setString (kAudioAggregateDeviceNameKey, name);
        description.setInt (kAudioAggregateDeviceIsPrivateKey, 1);
        description.setArray (kAudioAggregateDeviceSubDeviceListKey, subDeviceArray);

        if (clockUid.isNotEmpty())
        {
            description.setString (kAudioAggregateDeviceClockDeviceKey, clockUid);
            description.setString (mainSubDeviceKey, clockUid);
        }

        return description;
    }
};

//==============================================================================
struct CoreAudioClasses
{
    class CoreAudioIODeviceType;
    class CoreAudioIODevice;

    //==============================================================================
    class CoreAudioInternal final : private Timer
        , private AsyncUpdater
    {
    private:
        // members with deduced return types need to be defined before they
        // are used, so define it here. decltype doesn't help as you can't
        // capture anything in lambdas inside a decltype context.
        auto err2log() const
        {
            return [this] (OSStatus err)
            {
                OK (err);
            };
        }

    public:
        CoreAudioInternal (CoreAudioIODevice& d,
                           AudioDeviceID id,
                           bool hasInput,
                           bool hasOutput,
                           AudioDeviceID inputChannelDeviceIDIn = kAudioObjectUnknown,
                           AudioDeviceID outputChannelDeviceIDIn = kAudioObjectUnknown,
                           std::array<std::vector<int>, 2> channelMapsIn = {})
            : owner (d)
            , deviceID (id)
            , channelDeviceIDs { inputChannelDeviceIDIn != kAudioObjectUnknown ? inputChannelDeviceIDIn : id,
                                 outputChannelDeviceIDIn != kAudioObjectUnknown ? outputChannelDeviceIDIn : id }
            , channelMaps (std::move (channelMapsIn))
            , inStream (hasInput ? new Stream (true, *this, {}) : nullptr)
            , outStream (hasOutput ? new Stream (false, *this, {}) : nullptr)
        {
            jassert (deviceID != 0);

            updateDetailsFromDevice();
            YUP_MODULE_DBG (CORE_AUDIO, "Creating CoreAudioInternal\n"
                                            << (inStream != nullptr ? ("    inputDeviceId " + String (deviceID) + "\n") : "") << (outStream != nullptr ? ("    outputDeviceId " + String (deviceID) + "\n") : "") << getDeviceDetails().joinIntoString ("\n    "));

            const PropertyAddress wildcardAddress { kAudioObjectPropertySelectorWildcard, kAudioObjectPropertyScopeWildcard, kAudioObjectPropertyElementWildcard };
            AudioObjectAddPropertyListener (deviceID, wildcardAddress.get(), deviceListenerProc, this);
        }

        ~CoreAudioInternal() override
        {
            stopTimer();
            cancelPendingUpdate();

            const PropertyAddress wildcardAddress { kAudioObjectPropertySelectorWildcard, kAudioObjectPropertyScopeWildcard, kAudioObjectPropertyElementWildcard };
            AudioObjectRemovePropertyListener (deviceID, wildcardAddress.get(), deviceListenerProc, this);

            stop (false);
        }

        auto getStreams() const { return std::array<Stream*, 2> { { inStream.get(), outStream.get() } }; }

        void allocateTempBuffers()
        {
            auto tempBufSize = bufferSize + 4;

            auto streams = getStreams();
            const auto total = std::accumulate (streams.begin(), streams.end(), 0, [] (int n, const auto& s)
            {
                return n + (s != nullptr ? s->channels : 0);
            });
            audioBuffer.calloc (total * tempBufSize);

            auto channels = 0;
            for (auto* stream : streams)
                channels += stream != nullptr ? stream->allocateTempBuffers (tempBufSize, channels, audioBuffer) : 0;
        }

        struct CallbackDetailsForChannel
        {
            int streamNum;
            int dataOffsetSamples;
            int dataStrideSamples;
        };

        Array<double> getSampleRatesFromDevice() const
        {
            Array<double> newSampleRates;

            if (auto ranges = audioObjectGetProperties<AudioValueRange> (deviceID,
                                                                         PropertyAddress (kAudioDevicePropertyAvailableNominalSampleRates, kAudioObjectPropertyScopeWildcard),
                                                                         err2log());
                ! ranges.empty())
            {
                for (const auto rate : SampleRateHelpers::getAllSampleRates())
                {
                    for (auto range = ranges.rbegin(); range != ranges.rend(); ++range)
                    {
                        if (range->mMinimum - 2 <= rate && rate <= range->mMaximum + 2)
                        {
                            newSampleRates.add (rate);
                            break;
                        }
                    }
                }
            }

            if (newSampleRates.isEmpty() && sampleRate > 0)
                newSampleRates.add (sampleRate);

            auto nominalRate = getNominalSampleRate();

            if ((nominalRate > 0) && ! newSampleRates.contains (nominalRate))
                newSampleRates.addUsingDefaultSort (nominalRate);

            return newSampleRates;
        }

        Array<int> getBufferSizesFromDevice() const
        {
            Array<int> newBufferSizes;

            if (auto ranges = audioObjectGetProperties<AudioValueRange> (deviceID, PropertyAddress (kAudioDevicePropertyBufferFrameSizeRange, kAudioObjectPropertyScopeWildcard), err2log()); ! ranges.empty())
            {
                newBufferSizes.add ((int) (ranges[0].mMinimum + 15) & ~15);

                for (int i = 32; i <= 2048; i += 32)
                {
                    for (auto range = ranges.rbegin(); range != ranges.rend(); ++range)
                    {
                        if (i >= range->mMinimum && i <= range->mMaximum)
                        {
                            newBufferSizes.addIfNotAlreadyThere (i);
                            break;
                        }
                    }
                }

                if (bufferSize > 0)
                    newBufferSizes.addIfNotAlreadyThere (bufferSize);
            }

            if (newBufferSizes.isEmpty() && bufferSize > 0)
                newBufferSizes.add (bufferSize);

            return newBufferSizes;
        }

        int getFrameSizeFromDevice() const
        {
            return static_cast<int> (audioObjectGetProperty<UInt32> (deviceID, PropertyAddress (kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeWildcard)).value_or (0));
        }

        bool isDeviceAlive() const
        {
            return deviceID != 0
                && audioObjectGetProperty<UInt32> (deviceID, PropertyAddress (kAudioDevicePropertyDeviceIsAlive, kAudioObjectPropertyScopeWildcard), err2log()).value_or (0) != 0;
        }

        bool updateDetailsFromDevice (const BigInteger& activeIns, const BigInteger& activeOuts)
        {
            stopTimer();

            if (! isDeviceAlive())
                return false;

            auto newSampleRate = getNominalSampleRate();
            auto newBufferSize = getFrameSizeFromDevice();

            auto newBufferSizes = getBufferSizesFromDevice();
            auto newSampleRates = getSampleRatesFromDevice();

            auto newInput = rawToUniquePtr (inStream != nullptr ? new Stream (true, *this, activeIns) : nullptr);
            auto newOutput = rawToUniquePtr (outStream != nullptr ? new Stream (false, *this, activeOuts) : nullptr);

            auto newBitDepth = jmax (getBitDepth (newInput), getBitDepth (newOutput));

#if YUP_AUDIOWORKGROUP_TYPES_AVAILABLE
            audioWorkgroup = [this]() -> AudioWorkgroup
            {
                if (auto* workgroupHandle = audioObjectGetProperty<void*> (deviceID, PropertyAddress (kAudioDevicePropertyIOThreadOSWorkgroup, kAudioObjectPropertyScopeWildcard)).value_or (nullptr))
                {
                    os_workgroup_t workgroup = (__bridge_transfer os_workgroup_t) workgroupHandle;
                    return makeRealAudioWorkgroup (workgroup);
                }

                return {};
            }();
#endif

            {
                const ScopedLock sl (callbackLock);

                bitDepth = newBitDepth > 0 ? newBitDepth : 32;

                if (newSampleRate > 0)
                    sampleRate = newSampleRate;

                bufferSize = newBufferSize;

                sampleRates.swapWith (newSampleRates);
                bufferSizes.swapWith (newBufferSizes);

                std::swap (inStream, newInput);
                std::swap (outStream, newOutput);

                allocateTempBuffers();
            }

            YUP_MODULE_DBG (CORE_AUDIO, "Updated device details: deviceID=" << String (deviceID) << "\n    " << getDeviceDetails().joinIntoString ("\n    "));

            return true;
        }

        bool updateDetailsFromDevice()
        {
            return updateDetailsFromDevice (getActiveChannels (inStream), getActiveChannels (outStream));
        }

        StringArray getDeviceDetails()
        {
            StringArray result;

            String availableSampleRates ("Available sample rates:");

            for (auto& s : sampleRates)
                availableSampleRates << " " << s;

            result.add (availableSampleRates);
            result.add ("Sample rate: " + String (sampleRate));
            String availableBufferSizes ("Available buffer sizes:");

            for (auto& b : bufferSizes)
                availableBufferSizes << " " << b;

            result.add (availableBufferSizes);
            result.add ("Buffer size: " + String (bufferSize));
            result.add ("Bit depth: " + String (bitDepth));
            result.add ("Input latency: " + String (getLatency (inStream)));
            result.add ("Output latency: " + String (getLatency (outStream)));
            result.add ("Input channel names: " + getChannelNames (inStream));
            result.add ("Output channel names: " + getChannelNames (outStream));

            return result;
        }

        static auto getScope (bool input)
        {
            return getAudioDevicePropertyScope (input);
        }

        AudioDeviceID getChannelDeviceID (bool input) const
        {
            return channelDeviceIDs[static_cast<std::size_t> (getDirectionIndex (input))];
        }

        const std::vector<int>& getChannelMap (bool input) const
        {
            return channelMaps[static_cast<std::size_t> (getDirectionIndex (input))];
        }

        //==============================================================================
        StringArray getSources (bool input)
        {
            StringArray s;
            auto types = audioObjectGetProperties<OSType> (deviceID, PropertyAddress (kAudioDevicePropertyDataSources, kAudioObjectPropertyScopeWildcard));

            for (auto type : types)
            {
                AudioValueTranslation avt;
                char buffer[256];

                avt.mInputData = &type;
                avt.mInputDataSize = sizeof (UInt32);
                avt.mOutputData = buffer;
                avt.mOutputDataSize = 256;

                UInt32 transSize = sizeof (avt);
                const PropertyAddress pa { kAudioDevicePropertyDataSourceNameForID, getScope (input) };

                if (OK (AudioObjectGetPropertyData (deviceID, pa.get(), 0, nullptr, &transSize, &avt)))
                    s.add (buffer);
            }

            return s;
        }

        int getCurrentSourceIndex (bool input) const
        {
            if (deviceID != 0)
            {
                if (auto currentSourceID = audioObjectGetProperty<OSType> (deviceID, PropertyAddress (kAudioDevicePropertyDataSource, getScope (input)), err2log()))
                {
                    auto types = audioObjectGetProperties<OSType> (deviceID, PropertyAddress (kAudioDevicePropertyDataSources, kAudioObjectPropertyScopeWildcard));

                    if (auto it = std::find (types.begin(), types.end(), *currentSourceID); it != types.end())
                        return static_cast<int> (std::distance (types.begin(), it));
                }
            }

            return -1;
        }

        void setCurrentSourceIndex (int index, bool input)
        {
            if (deviceID != 0)
            {
                auto types = audioObjectGetProperties<OSType> (deviceID, PropertyAddress (kAudioDevicePropertyDataSources, kAudioObjectPropertyScopeWildcard));

                if (isPositiveAndBelow (index, static_cast<int> (types.size())))
                {
                    audioObjectSetProperty<OSType> (deviceID, PropertyAddress (kAudioDevicePropertyDataSource, getScope (input)), types[static_cast<std::size_t> (index)], err2log());
                }
            }
        }

        double getNominalSampleRate() const
        {
            return static_cast<double> (audioObjectGetProperty<Float64> (deviceID, PropertyAddress (kAudioDevicePropertyNominalSampleRate), err2log()).value_or (0.0));
        }

        bool setNominalSampleRate (double newSampleRate) const
        {
            const auto currentSampleRate = getNominalSampleRate();

            if (std::abs (currentSampleRate - newSampleRate) < 1.0)
            {
                YUP_MODULE_DBG (CORE_AUDIO, "Sample-rate already set: deviceID=" << String (deviceID) << ", sampleRate=" << String (currentSampleRate));
                return true;
            }

            const auto result = audioObjectSetProperty (deviceID, PropertyAddress (kAudioDevicePropertyNominalSampleRate), static_cast<Float64> (newSampleRate), err2log());

            YUP_MODULE_DBG (CORE_AUDIO, "Set sample-rate " << (result ? "succeeded" : "failed") << ": deviceID=" << String (deviceID) << ", current=" << String (currentSampleRate) << ", requested=" << String (newSampleRate) << ", reported=" << String (getNominalSampleRate()));

            return result;
        }

        //==============================================================================
        String reopen (const BigInteger& ins, const BigInteger& outs, double newSampleRate, int bufferSizeSamples)
        {
            YUP_MODULE_DBG (CORE_AUDIO, "Reopen requested: deviceID=" << String (deviceID) << ", sampleRate=" << String (newSampleRate) << ", bufferSize=" << String (bufferSizeSamples) << ", inputs={" << describeChannelBits (ins) << "}"
                                                                      << ", outputs={" << describeChannelBits (outs) << "}");

            callbacksAllowed = false;
            const ScopeGuard scope { [&]
            {
                callbacksAllowed = true;
            } };

            stopTimer();

            stop (false);

            if (! setNominalSampleRate (newSampleRate))
            {
                updateDetailsFromDevice (ins, outs);
                YUP_MODULE_DBG (CORE_AUDIO, "Reopen failed: couldn't change sample-rate, deviceID=" << String (deviceID));
                return "Couldn't change sample rate";
            }

            if (! audioObjectSetProperty (deviceID, PropertyAddress (kAudioDevicePropertyBufferFrameSize), static_cast<UInt32> (bufferSizeSamples), err2log()))
            {
                updateDetailsFromDevice (ins, outs);
                YUP_MODULE_DBG (CORE_AUDIO, "Reopen failed: couldn't change buffer size, deviceID=" << String (deviceID));
                return "Couldn't change buffer size";
            }

            // Annoyingly, after changing the rate and buffer size, some devices fail to
            // correctly report their new settings until some random time in the future, so
            // after calling updateDetailsFromDevice, we need to manually bodge these values
            // to make sure we're using the correct numbers..
            updateDetailsFromDevice (ins, outs);
            sampleRate = newSampleRate;
            bufferSize = bufferSizeSamples;

            if (sampleRates.size() == 0)
            {
                YUP_MODULE_DBG (CORE_AUDIO, "Reopen failed: no sample-rates, deviceID=" << String (deviceID));
                return "Device has no available sample-rates";
            }

            if (bufferSizes.size() == 0)
            {
                YUP_MODULE_DBG (CORE_AUDIO, "Reopen failed: no buffer-sizes, deviceID=" << String (deviceID));
                return "Device has no available buffer-sizes";
            }

            YUP_MODULE_DBG (CORE_AUDIO, "Reopen completed: deviceID=" << String (deviceID) << ", sampleRate=" << String (sampleRate) << ", bufferSize=" << String (bufferSize) << ", inputChannels=" << String (getChannels (inStream)) << ", outputChannels=" << String (getChannels (outStream)) << ", bitDepth=" << String (bitDepth) << ", inputLatency=" << String (getLatency (inStream)) << ", outputLatency=" << String (getLatency (outStream)));

            return {};
        }

        bool start (AudioIODeviceCallback* callbackToNotify)
        {
            const ScopedLock sl (callbackLock);

            YUP_MODULE_DBG (CORE_AUDIO, "Start requested: deviceID=" << String (deviceID) << ", callback=" << (callbackToNotify != nullptr ? "set" : "null") << ", hasProc=" << (scopedProcID.get() != nullptr ? "true" : "false") << ", sampleRate=" << String (sampleRate) << ", bufferSize=" << String (bufferSize) << ", inputChannels=" << String (getChannels (inStream)) << ", outputChannels=" << String (getChannels (outStream)));

            if (callback == nullptr && callbackToNotify != nullptr)
            {
                callback = callbackToNotify;
                callback->audioDeviceAboutToStart (&owner);
            }

            for (auto* stream : getStreams())
                if (stream != nullptr)
                    stream->previousSampleTime = invalidSampleTime;

            owner.hadDiscontinuity = false;

            if (scopedProcID.get() == nullptr && deviceID != 0)
            {
                scopedProcID = [&self = *this,
                                &lock = callbackLock,
                                nextProcID = ScopedAudioDeviceIOProcID { *this, deviceID, audioIOProc },
                                dID = deviceID]() mutable -> ScopedAudioDeviceIOProcID
                {
                    // It *looks* like AudioDeviceStart may start the audio callback running, and then
                    // immediately lock an internal mutex.
                    // The same mutex is locked before calling the audioIOProc.
                    // If we get very unlucky, then we can end up with thread A taking the callbackLock
                    // and calling AudioDeviceStart, followed by thread B taking the CoreAudio lock
                    // and calling into audioIOProc, which waits on the callbackLock. When thread A
                    // continues it attempts to take the CoreAudio lock, and the program deadlocks.

                    if (auto* procID = nextProcID.get())
                    {
                        const ScopedUnlock su (lock);

                        if (self.OK (AudioDeviceStart (dID, procID)))
                            return std::move (nextProcID);
                    }

                    return {};
                }();
            }

            playing = scopedProcID.get() != nullptr && callback != nullptr;

            YUP_MODULE_DBG (CORE_AUDIO, "Start " << (playing.load() ? "succeeded" : "failed") << ": deviceID=" << String (deviceID) << ", hasProc=" << (scopedProcID.get() != nullptr ? "true" : "false"));

            return scopedProcID.get() != nullptr;
        }

        AudioIODeviceCallback* stop (bool leaveInterruptRunning)
        {
            const ScopedLock sl (callbackLock);

            YUP_MODULE_DBG (CORE_AUDIO, "Stop requested: deviceID=" << String (deviceID) << ", leaveInterruptRunning=" << (leaveInterruptRunning ? "true" : "false") << ", hasCallback=" << (callback != nullptr ? "true" : "false") << ", hasProc=" << (scopedProcID.get() != nullptr ? "true" : "false") << ", playing=" << (playing.load() ? "true" : "false"));

            auto result = std::exchange (callback, nullptr);

            if (scopedProcID.get() != nullptr && (deviceID != 0) && ! leaveInterruptRunning)
            {
                audioDeviceStopPending = true;

                // wait until AudioDeviceStop() has been called on the IO thread
                for (int i = 40; --i >= 0;)
                {
                    if (audioDeviceStopPending == false)
                        break;

                    const ScopedUnlock ul (callbackLock);
                    Thread::sleep (50);
                }

                scopedProcID = {};
                playing = false;
            }

            YUP_MODULE_DBG (CORE_AUDIO, "Stop completed: deviceID=" << String (deviceID) << ", callbackReturned=" << (result != nullptr ? "true" : "false") << ", hasProc=" << (scopedProcID.get() != nullptr ? "true" : "false") << ", playing=" << (playing.load() ? "true" : "false") << ", stopPending=" << (audioDeviceStopPending ? "true" : "false"));

            return result;
        }

        double getSampleRate() const { return sampleRate; }

        int getBufferSize() const { return bufferSize; }

        void audioCallback (const AudioTimeStamp* inputTimestamp,
                            const AudioTimeStamp* outputTimestamp,
                            const AudioBufferList* inInputData,
                            AudioBufferList* outOutputData)
        {
            const ScopedLock sl (callbackLock);

            if (audioDeviceStopPending)
            {
                if (OK (AudioDeviceStop (deviceID, scopedProcID.get()), false))
                    audioDeviceStopPending = false;

                return;
            }

            const auto numInputChans = getChannels (inStream);
            const auto numOutputChans = getChannels (outStream);

            if (callback != nullptr)
            {
                for (int i = numInputChans; --i >= 0;)
                {
                    auto& info = inStream->channelInfo.getReference (i);
                    auto dest = inStream->tempBuffers[i];
                    auto src = ((const float*) inInputData->mBuffers[info.streamNum].mData) + info.dataOffsetSamples;
                    auto stride = info.dataStrideSamples;

                    if (stride != 0) // if this is zero, info is invalid
                    {
                        for (int j = bufferSize; --j >= 0;)
                        {
                            *dest++ = *src;
                            src += stride;
                        }
                    }
                }

                for (auto* stream : getStreams())
                    if (stream != nullptr)
                        owner.hadDiscontinuity |= stream->checkTimestampsForDiscontinuity (stream == inStream.get() ? inputTimestamp
                                                                                                                    : outputTimestamp);

                const auto* timeStamp = numOutputChans > 0 ? outputTimestamp : inputTimestamp;
                const auto nanos = timeStamp != nullptr ? timeConversions.hostTimeToNanos (timeStamp->mHostTime) : 0;
                const AudioIODeviceCallbackContext context {
                    timeStamp != nullptr ? &nanos : nullptr,
                };

                callback->audioDeviceIOCallbackWithContext (getTempBuffers (inStream), numInputChans, getTempBuffers (outStream), numOutputChans, bufferSize, context);

                if (! getChannelMap (false).empty())
                {
                    for (UInt32 i = 0; i < outOutputData->mNumberBuffers; ++i)
                        zeromem (outOutputData->mBuffers[i].mData,
                                 outOutputData->mBuffers[i].mDataByteSize);
                }

                for (int i = numOutputChans; --i >= 0;)
                {
                    auto& info = outStream->channelInfo.getReference (i);
                    auto src = outStream->tempBuffers[i];
                    auto dest = ((float*) outOutputData->mBuffers[info.streamNum].mData) + info.dataOffsetSamples;
                    auto stride = info.dataStrideSamples;

                    if (stride != 0) // if this is zero, info is invalid
                    {
                        for (int j = bufferSize; --j >= 0;)
                        {
                            *dest = *src++;
                            dest += stride;
                        }
                    }
                }
            }
            else
            {
                for (UInt32 i = 0; i < outOutputData->mNumberBuffers; ++i)
                    zeromem (outOutputData->mBuffers[i].mData,
                             outOutputData->mBuffers[i].mDataByteSize);
            }

            for (auto* stream : getStreams())
                if (stream != nullptr)
                    stream->previousSampleTime += static_cast<Float64> (bufferSize);
        }

        // called by callbacks (possibly off the main thread)
        void deviceDetailsChanged()
        {
            if (callbacksAllowed.get() == 1)
                startTimer (100);
        }

        // called by callbacks (possibly off the main thread)
        void deviceRequestedRestart()
        {
            owner.restart();
            triggerAsyncUpdate();
        }

        bool isPlaying() const { return playing.load(); }

        //==============================================================================
        struct Stream
        {
            Stream (bool isInput, CoreAudioInternal& parent, const BigInteger& activeRequested)
                : input (isInput)
                , latency (getLatencyFromDevice (isInput, parent))
                , bitDepth (getBitDepthFromDevice (isInput, parent))
                , chanNames (getChannelNames (isInput, parent))
                , activeChans ([&activeRequested, clearFrom = chanNames.size()]
            {
                auto result = activeRequested;
                result.setRange (clearFrom, result.getHighestBit() + 1 - clearFrom, false);
                return result;
            }())
                , channelInfo (getChannelInfos (isInput, parent, activeChans))
                , channels (static_cast<int> (channelInfo.size()))
            {
            }

            int allocateTempBuffers (int tempBufSize, int channelCount, HeapBlock<float>& buffer)
            {
                tempBuffers.calloc (channels + 2);

                for (int i = 0; i < channels; ++i)
                    tempBuffers[i] = buffer + channelCount++ * tempBufSize;

                return channels;
            }

            template <typename Visitor>
            static auto visitChannels (bool isInput, CoreAudioInternal& parent, Visitor&& visitor)
            {
                struct Args
                {
                    int stream, channelIdx, chanNum, streamChannels;
                };

                using VisitorResultType = typename std::invoke_result_t<Visitor, const Args&>::value_type;
                Array<VisitorResultType> result;
                std::vector<Args> physicalChannels;

                if (auto bufList = audioObjectGetProperty<AudioBufferList> (parent.deviceID, PropertyAddress (kAudioDevicePropertyStreamConfiguration, getScope (isInput)), parent.err2log()))
                {
                    const int numStreams = static_cast<int> (bufList->mNumberBuffers);
                    int chanNum = 0;

                    for (int i = 0; i < numStreams; ++i)
                    {
                        auto& b = bufList->mBuffers[i];

                        for (unsigned int j = 0; j < b.mNumberChannels; ++j)
                            physicalChannels.push_back (Args { i, static_cast<int> (j), chanNum++, static_cast<int> (b.mNumberChannels) });
                    }
                }

                const auto addResult = [&] (const Args& args)
                {
                    // Passing an anonymous struct ensures that callback can't confuse the argument order
                    if (auto opt = visitor (args))
                        result.add (std::move (*opt));
                };

                const auto& channelMap = parent.getChannelMap (isInput);

                if (channelMap.empty())
                {
                    for (const auto& args : physicalChannels)
                        addResult (args);
                }
                else
                {
                    auto logicalChannel = 0;

                    for (const auto physicalChannel : channelMap)
                    {
                        if (physicalChannel >= 0 && physicalChannel < static_cast<int> (physicalChannels.size()))
                        {
                            auto args = physicalChannels[static_cast<std::size_t> (physicalChannel)];
                            args.chanNum = logicalChannel;
                            addResult (args);
                        }

                        ++logicalChannel;
                    }
                }

                return result;
            }

            static Array<CallbackDetailsForChannel> getChannelInfos (bool isInput, CoreAudioInternal& parent, const BigInteger& active)
            {
                return visitChannels (isInput, parent, [&] (const auto& args) -> std::optional<CallbackDetailsForChannel>
                {
                    if (! active[args.chanNum])
                        return {};

                    return CallbackDetailsForChannel { args.stream, args.channelIdx, args.streamChannels };
                });
            }

            static StringArray getChannelNames (bool isInput, CoreAudioInternal& parent)
            {
                const auto channelDeviceID = parent.getChannelDeviceID (isInput);
                auto subDeviceGroups = getAggregateSubDeviceChannelGroups (channelDeviceID, isInput);
                std::size_t subDeviceGroupIndex = 0;

                const auto getSubDeviceName = [&]() -> String
                {
                    while (subDeviceGroupIndex < subDeviceGroups.size() && subDeviceGroups[subDeviceGroupIndex].remainingChannels == 0)
                        ++subDeviceGroupIndex;

                    if (subDeviceGroupIndex >= subDeviceGroups.size())
                        return {};

                    --subDeviceGroups[subDeviceGroupIndex].remainingChannels;
                    return subDeviceGroups[subDeviceGroupIndex].name;
                };

                auto names = visitChannels (isInput, parent, [&] (const auto& args) -> std::optional<String>
                {
                    String name;
                    const auto element = static_cast<AudioObjectPropertyElement> (args.chanNum + 1);

                    if (auto retainedName = audioObjectGetProperty<CFStringRef> (channelDeviceID, PropertyAddress (kAudioObjectPropertyElementName, getScope (isInput), element)).value_or (nullptr))
                    {
                        const CFUniquePtr<CFStringRef> nameString { retainedName };
                        name = String::fromCFString (nameString.get());
                    }

                    if (name.isEmpty())
                        name << (isInput ? "Input " : "Output ") << (args.chanNum + 1);

                    if (auto subDeviceName = getSubDeviceName(); subDeviceName.isNotEmpty())
                        name << " (" << subDeviceName << ")";

                    return name;
                });

                return { names };
            }

            static int getBitDepthFromDevice (bool isInput, CoreAudioInternal& parent)
            {
                return static_cast<int> (audioObjectGetProperty<AudioStreamBasicDescription> (parent.deviceID, PropertyAddress (kAudioStreamPropertyPhysicalFormat, getScope (isInput)), parent.err2log())
                                             .value_or (AudioStreamBasicDescription {})
                                             .mBitsPerChannel);
            }

            static int getLatencyFromDevice (bool isInput, CoreAudioInternal& parent)
            {
                const auto scope = getScope (isInput);

                const auto deviceLatency = audioObjectGetProperty<UInt32> (parent.deviceID, PropertyAddress (kAudioDevicePropertyLatency, scope)).value_or (0);

                const auto safetyOffset = audioObjectGetProperty<UInt32> (parent.deviceID, PropertyAddress (kAudioDevicePropertySafetyOffset, scope)).value_or (0);

                const auto framesInBuffer = audioObjectGetProperty<UInt32> (parent.deviceID, PropertyAddress (kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeWildcard)).value_or (0);

                UInt32 streamLatency = 0;

                if (auto streams = audioObjectGetProperties<AudioStreamID> (parent.deviceID, PropertyAddress (kAudioDevicePropertyStreams, scope)); ! streams.empty())
                    streamLatency = audioObjectGetProperty<UInt32> (streams.front(), PropertyAddress (kAudioStreamPropertyLatency, scope)).value_or (0);

                return static_cast<int> (deviceLatency + safetyOffset + framesInBuffer + streamLatency);
            }

            bool checkTimestampsForDiscontinuity (const AudioTimeStamp* timestamp) noexcept
            {
                if (channels > 0)
                {
                    jassert (timestamp == nullptr || (((timestamp->mFlags & kAudioTimeStampSampleTimeValid) != 0) && ((timestamp->mFlags & kAudioTimeStampHostTimeValid) != 0)));

                    if (exactlyEqual (previousSampleTime, invalidSampleTime))
                        previousSampleTime = timestamp != nullptr ? timestamp->mSampleTime : 0.0;

                    if (timestamp != nullptr && std::fabs (previousSampleTime - timestamp->mSampleTime) >= 1.0)
                    {
                        previousSampleTime = timestamp->mSampleTime;
                        return true;
                    }
                }

                return false;
            }

            //==============================================================================
            const bool input;
            const int latency;
            const int bitDepth;
            const StringArray chanNames;
            const BigInteger activeChans;
            const Array<CallbackDetailsForChannel> channelInfo;
            const int channels = 0;
            Float64 previousSampleTime;

            HeapBlock<float*> tempBuffers;

            YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Stream)
        };

        template <typename Callback>
        static auto getWithDefault (const std::unique_ptr<Stream>& ptr, Callback&& callback)
        {
            return ptr != nullptr ? callback (*ptr) : decltype (callback (*ptr)) {};
        }

        template <typename Value>
        static auto getWithDefault (const std::unique_ptr<Stream>& ptr, Value (Stream::* member))
        {
            return getWithDefault (ptr, [&] (Stream& s)
            {
                return s.*member;
            });
        }

        static int getLatency (const std::unique_ptr<Stream>& ptr) { return getWithDefault (ptr, &Stream::latency); }

        static int getBitDepth (const std::unique_ptr<Stream>& ptr) { return getWithDefault (ptr, &Stream::bitDepth); }

        static int getChannels (const std::unique_ptr<Stream>& ptr) { return getWithDefault (ptr, &Stream::channels); }

        static int getNumChannelNames (const std::unique_ptr<Stream>& ptr) { return getWithDefault (ptr, &Stream::chanNames).size(); }

        static String getChannelNames (const std::unique_ptr<Stream>& ptr) { return getWithDefault (ptr, &Stream::chanNames).joinIntoString (" "); }

        static BigInteger getActiveChannels (const std::unique_ptr<Stream>& ptr) { return getWithDefault (ptr, &Stream::activeChans); }

        static float** getTempBuffers (const std::unique_ptr<Stream>& ptr)
        {
            return getWithDefault (ptr, [] (auto& s)
            {
                return s.tempBuffers.get();
            });
        }

        //==============================================================================
        static constexpr Float64 invalidSampleTime = std::numeric_limits<Float64>::max();

        CoreAudioIODevice& owner;
        int bitDepth = 32;
        std::atomic<int> xruns { 0 };
        Array<double> sampleRates;
        Array<int> bufferSizes;
        AudioDeviceID deviceID;
        std::array<AudioDeviceID, 2> channelDeviceIDs;
        std::array<std::vector<int>, 2> channelMaps;
        std::unique_ptr<Stream> inStream, outStream;

        AudioWorkgroup audioWorkgroup;

    private:
        class ScopedAudioDeviceIOProcID
        {
        public:
            ScopedAudioDeviceIOProcID() = default;

            ScopedAudioDeviceIOProcID (CoreAudioInternal& coreAudio, AudioDeviceID d, AudioDeviceIOProc audioIOProc)
                : deviceID (d)
            {
                if (! coreAudio.OK (AudioDeviceCreateIOProcID (deviceID, audioIOProc, &coreAudio, &proc)))
                    proc = {};
            }

            ~ScopedAudioDeviceIOProcID() noexcept
            {
                if (proc != AudioDeviceIOProcID {})
                    AudioDeviceDestroyIOProcID (deviceID, proc);
            }

            ScopedAudioDeviceIOProcID (ScopedAudioDeviceIOProcID&& other) noexcept
            {
                swap (other);
            }

            ScopedAudioDeviceIOProcID& operator= (ScopedAudioDeviceIOProcID&& other) noexcept
            {
                ScopedAudioDeviceIOProcID { std::move (other) }.swap (*this);
                return *this;
            }

            AudioDeviceIOProcID get() const { return proc; }

        private:
            void swap (ScopedAudioDeviceIOProcID& other) noexcept
            {
                std::swap (other.deviceID, deviceID);
                std::swap (other.proc, proc);
            }

            AudioDeviceID deviceID = {};
            AudioDeviceIOProcID proc = {};
        };

        //==============================================================================
        ScopedAudioDeviceIOProcID scopedProcID;
        CoreAudioTimeConversions timeConversions;
        AudioIODeviceCallback* callback = nullptr;
        CriticalSection callbackLock;
        bool audioDeviceStopPending = false;
        std::atomic<bool> playing { false };
        double sampleRate = 0;
        int bufferSize = 0;
        HeapBlock<float> audioBuffer;
        Atomic<int> callbacksAllowed { 1 };

        //==============================================================================
        void timerCallback() override
        {
            stopTimer();
            auto oldSampleRate = sampleRate;
            auto oldBufferSize = bufferSize;

            YUP_MODULE_DBG (CORE_AUDIO, "Device change timer: deviceID=" << String (deviceID) << ", oldSampleRate=" << String (oldSampleRate) << ", oldBufferSize=" << String (oldBufferSize));

            if (! updateDetailsFromDevice())
            {
                YUP_MODULE_DBG (CORE_AUDIO, "Device change update failed: deviceID=" << String (deviceID));
                owner.stopInternal();
            }
            else if ((oldBufferSize != bufferSize || ! approximatelyEqual (oldSampleRate, sampleRate)) && owner.shouldRestartDevice())
            {
                YUP_MODULE_DBG (CORE_AUDIO, "Device change requires restart: deviceID=" << String (deviceID) << ", newSampleRate=" << String (sampleRate) << ", newBufferSize=" << String (bufferSize));
                owner.restart();
            }
            else
            {
                YUP_MODULE_DBG (CORE_AUDIO, "Device change applied without restart: deviceID=" << String (deviceID) << ", newSampleRate=" << String (sampleRate) << ", newBufferSize=" << String (bufferSize));
            }
        }

        void handleAsyncUpdate() override
        {
            if (owner.deviceType != nullptr)
                owner.deviceType->audioDeviceListChanged();
        }

        static OSStatus audioIOProc (AudioDeviceID /*inDevice*/,
                                     [[maybe_unused]] const AudioTimeStamp* inNow,
                                     const AudioBufferList* inInputData,
                                     const AudioTimeStamp* inInputTime,
                                     AudioBufferList* outOutputData,
                                     const AudioTimeStamp* inOutputTime,
                                     void* device)
        {
            static_cast<CoreAudioInternal*> (device)->audioCallback (inInputTime, inOutputTime, inInputData, outOutputData);
            return noErr;
        }

        static OSStatus deviceListenerProc (AudioDeviceID /*inDevice*/,
                                            UInt32 numAddresses,
                                            const AudioObjectPropertyAddress* pa,
                                            void* inClientData)
        {
            auto& intern = *static_cast<CoreAudioInternal*> (inClientData);

            const auto xruns = (int) std::count_if (pa, pa + numAddresses, [] (const AudioObjectPropertyAddress& x)
            {
                return x.mSelector == kAudioDeviceProcessorOverload;
            });

            intern.xruns.fetch_add (xruns);

            const auto detailsChanged = std::any_of (pa, pa + numAddresses, [] (const AudioObjectPropertyAddress& x)
            {
                constexpr UInt32 selectors[] {
                    kAudioDevicePropertyBufferSize,
                    kAudioDevicePropertyBufferFrameSize,
                    kAudioDevicePropertyNominalSampleRate,
                    kAudioDevicePropertyStreamFormat,
                    kAudioDevicePropertyDeviceIsAlive,
                    kAudioStreamPropertyPhysicalFormat,
                };

                return std::find (std::begin (selectors), std::end (selectors), x.mSelector) != std::end (selectors);
            });

            const auto requestedRestart = std::any_of (pa, pa + numAddresses, [] (const AudioObjectPropertyAddress& x)
            {
                constexpr UInt32 selectors[] {
                    kAudioDevicePropertyDeviceHasChanged,
                    kAudioObjectPropertyOwnedObjects,
                };

                return std::find (std::begin (selectors), std::end (selectors), x.mSelector) != std::end (selectors);
            });

            if (detailsChanged)
                intern.deviceDetailsChanged();

            if (requestedRestart)
                intern.deviceRequestedRestart();

            return noErr;
        }

        //==============================================================================
        bool OK (const OSStatus errorCode, bool logError = true) const
        {
            if (errorCode == noErr)
                return true;

            const String errorMessage ("CoreAudio error: " + String::toHexString ((int) errorCode));

            if (logError)
                YUP_MODULE_DBG (CORE_AUDIO, errorMessage);

            if (callback != nullptr)
                callback->audioDeviceError (errorMessage);

            return false;
        }

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CoreAudioInternal)
    };

    //==============================================================================
    class CoreAudioIODevice final : public AudioIODevice
        , private Timer
    {
    public:
        CoreAudioIODevice (CoreAudioIODeviceType* dt,
                           const String& deviceName,
                           AudioDeviceID inputDeviceId,
                           AudioDeviceID outputDeviceId,
                           int inputDeviceIndexIn,
                           int outputDeviceIndexIn)
            : AudioIODevice (deviceName, "CoreAudio")
            , deviceType (dt)
            , inputDeviceIndex (inputDeviceIndexIn)
            , outputDeviceIndex (outputDeviceIndexIn)
        {
            internal = [this, &deviceName, &inputDeviceId, &outputDeviceId]() -> std::unique_ptr<CoreAudioInternal>
            {
                if (outputDeviceId == 0 || outputDeviceId == inputDeviceId)
                {
                    jassert (inputDeviceId != 0);
                    return std::make_unique<CoreAudioInternal> (*this,
                                                                inputDeviceId,
                                                                true,
                                                                outputDeviceId != 0,
                                                                inputDeviceId,
                                                                outputDeviceId != 0 ? outputDeviceId : kAudioObjectUnknown);
                }

                if (inputDeviceId == 0)
                {
                    return std::make_unique<CoreAudioInternal> (*this,
                                                                outputDeviceId,
                                                                false,
                                                                true,
                                                                kAudioObjectUnknown,
                                                                outputDeviceId);
                }

                const AggregateDeviceDescription aggregateDesc (createPrivateAggregateDeviceName (deviceName),
                                                                inputDeviceId,
                                                                outputDeviceId);

                if (aggregateDesc.isEmpty())
                    return nullptr;

                YUP_MODULE_DBG (CORE_AUDIO, "Creating private aggregate for input=" << getAudioDeviceName (inputDeviceId) << " [" << String (inputDeviceId) << "]"
                                                                                    << ", output=" << getAudioDeviceName (outputDeviceId) << " [" << String (outputDeviceId) << "]"
                                                                                    << ", subdevices=" << String (static_cast<int> (aggregateDesc.subDevices.size())) << "\n    inputMap=" << describeChannelMap (aggregateDesc.channelMaps[0]) << "\n    outputMap=" << describeChannelMap (aggregateDesc.channelMaps[1]) << ", inputChannels=" << String (static_cast<int> (aggregateDesc.channelMaps[0].size())) << ", outputChannels=" << String (static_cast<int> (aggregateDesc.channelMaps[1].size())) << ", clockDevice=" << String (aggregateDesc.clockDeviceID));

                aggregateDeviceID = aggregateDesc.createAggregateDevice();

                YUP_MODULE_DBG (CORE_AUDIO, "Private aggregate creation " << (aggregateDeviceID != kAudioObjectUnknown ? "succeeded" : "failed") << ", deviceID=" << String (aggregateDeviceID));

                if (aggregateDeviceID == kAudioObjectUnknown)
                    return nullptr;

                auto channelMaps = aggregateDesc.channelMaps;
                return std::make_unique<CoreAudioInternal> (*this,
                                                            aggregateDeviceID,
                                                            true,
                                                            true,
                                                            inputDeviceId,
                                                            outputDeviceId,
                                                            std::move (channelMaps));
            }();

            if (internal == nullptr)
                return;

            const PropertyAddress wildcardAddress { kAudioObjectPropertySelectorWildcard, kAudioObjectPropertyScopeWildcard, kAudioObjectPropertyElementWildcard };
            AudioObjectAddPropertyListener (kAudioObjectSystemObject, wildcardAddress.get(), hardwareListenerProc, internal.get());
        }

        ~CoreAudioIODevice() override
        {
            if (internal != nullptr)
            {
                close();

                const PropertyAddress wildcardAddress { kAudioObjectPropertySelectorWildcard, kAudioObjectPropertyScopeWildcard, kAudioObjectPropertyElementWildcard };
                AudioObjectRemovePropertyListener (kAudioObjectSystemObject, wildcardAddress.get(), hardwareListenerProc, internal.get());
            }

            if (aggregateDeviceID != 0)
            {
                YUP_MODULE_DBG (CORE_AUDIO, "Destroying private aggregate: deviceID=" << String (aggregateDeviceID));
                AudioHardwareDestroyAggregateDevice (aggregateDeviceID);
            }
        }

        StringArray getOutputChannelNames() override { return internal->outStream != nullptr ? internal->outStream->chanNames : StringArray(); }

        StringArray getInputChannelNames() override { return internal->inStream != nullptr ? internal->inStream->chanNames : StringArray(); }

        bool isOpen() override { return isOpen_; }

        Array<double> getAvailableSampleRates() override { return internal->sampleRates; }

        Array<int> getAvailableBufferSizes() override { return internal->bufferSizes; }

        double getCurrentSampleRate() override { return internal->getSampleRate(); }

        int getCurrentBitDepth() override { return internal->bitDepth; }

        int getCurrentBufferSizeSamples() override { return internal->getBufferSize(); }

        int getXRunCount() const noexcept override { return internal->xruns.load (std::memory_order_relaxed); }

        bool isValid() const noexcept { return internal != nullptr; }

        int getIndexOfDevice (bool asInput) const
        {
            return asInput ? inputDeviceIndex
                           : outputDeviceIndex;
        }

        int getDefaultBufferSize() override
        {
            int best = 0;

            for (int i = 0; best < 512 && i < internal->bufferSizes.size(); ++i)
                best = internal->bufferSizes.getUnchecked (i);

            if (best == 0)
                best = 512;

            return best;
        }

        String open (const BigInteger& inputChannels,
                     const BigInteger& outputChannels,
                     double sampleRate,
                     int bufferSizeSamples) override
        {
            YUP_MODULE_DBG (CORE_AUDIO, "Open requested: " << getName() << ", inputChannels={" << describeChannelBits (inputChannels) << "}"
                                                           << ", outputChannels={" << describeChannelBits (outputChannels) << "}"
                                                           << ", requestedSampleRate=" << String (sampleRate) << ", requestedBufferSize=" << String (bufferSizeSamples));

            isOpen_ = true;
            internal->xruns.store (0);

            inputChannelsRequested = inputChannels;
            outputChannelsRequested = outputChannels;

            if (bufferSizeSamples <= 0)
                bufferSizeSamples = getDefaultBufferSize();

            if (sampleRate <= 0)
                sampleRate = internal->getNominalSampleRate();

            lastError = internal->reopen (inputChannels, outputChannels, sampleRate, bufferSizeSamples);

            isOpen_ = lastError.isEmpty();

            YUP_MODULE_DBG (CORE_AUDIO, "Open " << (isOpen_ ? "succeeded" : "failed") << ": " << getName() << ", sampleRate=" << String (getCurrentSampleRate()) << ", bufferSize=" << String (getCurrentBufferSizeSamples()) << ", activeInputs={" << describeChannelBits (getActiveInputChannels()) << "}"
                                                << ", activeOutputs={" << describeChannelBits (getActiveOutputChannels()) << "}" << (lastError.isNotEmpty() ? ", error=" + lastError : String()));

            return lastError;
        }

        void close() override
        {
            if (internal == nullptr)
                return;

            YUP_MODULE_DBG (CORE_AUDIO, "Close requested: " << getName() << ", isOpen=" << (isOpen_ ? "true" : "false") << ", isPlaying=" << (internal->isPlaying() ? "true" : "false"));

            isOpen_ = false;
            internal->stop (false);
        }

        BigInteger getActiveOutputChannels() const override { return CoreAudioInternal::getActiveChannels (internal->outStream); }

        BigInteger getActiveInputChannels() const override { return CoreAudioInternal::getActiveChannels (internal->inStream); }

        int getOutputLatencyInSamples() override { return CoreAudioInternal::getLatency (internal->outStream); }

        int getInputLatencyInSamples() override { return CoreAudioInternal::getLatency (internal->inStream); }

        void start (AudioIODeviceCallback* callback) override
        {
            YUP_MODULE_DBG (CORE_AUDIO, "AudioIODevice start requested: " << getName() << ", callback=" << (callback != nullptr ? "set" : "null"));

            if (internal->start (callback))
                previousCallback = callback;

            YUP_MODULE_DBG (CORE_AUDIO, "AudioIODevice start completed: " << getName() << ", isPlaying=" << (internal->isPlaying() ? "true" : "false"));
        }

        void stop() override
        {
            YUP_MODULE_DBG (CORE_AUDIO, "AudioIODevice stop requested: " << getName());

            restartDevice = false;
            stopAndGetLastCallback();
        }

        AudioIODeviceCallback* stopAndGetLastCallback() const
        {
            auto* lastCallback = internal->stop (true);

            if (lastCallback != nullptr)
                lastCallback->audioDeviceStopped();

            return lastCallback;
        }

        AudioIODeviceCallback* stopInternal()
        {
            restartDevice = true;
            return stopAndGetLastCallback();
        }

        AudioWorkgroup getWorkgroup() const override
        {
            return internal->audioWorkgroup;
        }

        bool isPlaying() override
        {
            return internal->isPlaying();
        }

        String getLastError() override
        {
            return lastError;
        }

        void audioDeviceListChanged()
        {
            if (deviceType != nullptr)
                deviceType->audioDeviceListChanged();
        }

        // called by callbacks (possibly off the main thread)
        void restart()
        {
            YUP_MODULE_DBG (CORE_AUDIO, "Restart requested: " << getName() << ", previousCallback=" << (previousCallback != nullptr ? "set" : "null"));

            {
                const ScopedLock sl (closeLock);
                previousCallback = stopInternal();
            }

            startTimer (100);
        }

        bool setCurrentSampleRate (double newSampleRate)
        {
            const auto result = internal->setNominalSampleRate (newSampleRate);

            YUP_MODULE_DBG (CORE_AUDIO, "setCurrentSampleRate " << (result ? "succeeded" : "failed") << ": " << getName() << ", requested=" << String (newSampleRate) << ", current=" << String (getCurrentSampleRate()));

            return result;
        }

        bool shouldRestartDevice() const noexcept { return restartDevice; }

        WeakReference<CoreAudioIODeviceType> deviceType;
        bool hadDiscontinuity;

    private:
        std::unique_ptr<CoreAudioInternal> internal;
        AudioDeviceID aggregateDeviceID = 0;
        int inputDeviceIndex = -1, outputDeviceIndex = -1;
        bool isOpen_ = false, restartDevice = true;
        String lastError;
        AudioIODeviceCallback* previousCallback = nullptr;
        BigInteger inputChannelsRequested, outputChannelsRequested;
        CriticalSection closeLock;

        void timerCallback() override
        {
            stopTimer();

            YUP_MODULE_DBG (CORE_AUDIO, "Restart timer fired: " << getName() << ", sampleRate=" << String (getCurrentSampleRate()) << ", bufferSize=" << String (getCurrentBufferSizeSamples()) << ", previousCallback=" << (previousCallback != nullptr ? "set" : "null"));

            stopInternal();

            internal->updateDetailsFromDevice();

            open (inputChannelsRequested, outputChannelsRequested, getCurrentSampleRate(), getCurrentBufferSizeSamples());
            start (previousCallback);
        }

        static OSStatus hardwareListenerProc (AudioDeviceID /*inDevice*/,
                                              UInt32 numAddresses,
                                              const AudioObjectPropertyAddress* pa,
                                              void* inClientData)
        {
            const auto detailsChanged = std::any_of (pa, pa + numAddresses, [] (const AudioObjectPropertyAddress& x)
            {
                return x.mSelector == kAudioHardwarePropertyDevices;
            });

            if (detailsChanged)
                static_cast<CoreAudioInternal*> (inClientData)->deviceDetailsChanged();

            return noErr;
        }

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CoreAudioIODevice)
    };

    //==============================================================================
    class CoreAudioIODeviceType final : public AudioIODeviceType
        , private AsyncUpdater
    {
    public:
        CoreAudioIODeviceType()
            : AudioIODeviceType ("CoreAudio")
        {
            // Remove stale private aggregate devices left behind by previous crashes.
            // The private aggregate name encodes the PID of the creating process.
            for (const auto& device : SystemObject {}.getAudioDevices())
            {
                if (! device.isAggregateDevice())
                    continue;

                const auto name = device.getName();

                if (! name.startsWith (yupPrivateAggregateDeviceNamePrefix))
                    continue;

                const auto pid = getPrivateAggregateDeviceProcessID (name);
                if (pid <= 0)
                    continue;

                const auto processExists = pid > 0 && (::kill (pid, 0) == 0 || errno != ESRCH);

                if (! processExists)
                {
                    YUP_MODULE_DBG (CORE_AUDIO, "Destroying stale private aggregate: " << name);
                    AudioHardwareDestroyAggregateDevice (device.getId());
                }
            }

            const PropertyAddress devicesAddress { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeWildcard, kAudioObjectPropertyElementWildcard };
            AudioObjectAddPropertyListener (kAudioObjectSystemObject, devicesAddress.get(), hardwareListenerProc, this);
        }

        ~CoreAudioIODeviceType() override
        {
            cancelPendingUpdate();

            const PropertyAddress devicesAddress { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeWildcard, kAudioObjectPropertyElementWildcard };
            AudioObjectRemovePropertyListener (kAudioObjectSystemObject, devicesAddress.get(), hardwareListenerProc, this);
        }

        //==============================================================================
        void scanForDevices() override
        {
            hasScanned = true;

            inputDeviceNames.clear();
            outputDeviceNames.clear();
            inputIds.clear();
            outputIds.clear();

            struct DeviceEntry
            {
                String name;
                String uid;
                AudioDeviceID deviceID = kAudioObjectUnknown;
            };

            std::vector<DeviceEntry> inputDevices, outputDevices;

            auto audioDevices = audioObjectGetProperties<AudioDeviceID> (kAudioObjectSystemObject, PropertyAddress (kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeWildcard));

            for (const auto audioDevice : audioDevices)
            {
                if (const auto optionalName = audioObjectGetProperty<CFStringRef> (audioDevice, PropertyAddress (kAudioDevicePropertyDeviceNameCFString, kAudioObjectPropertyScopeWildcard)))
                {
                    if (const CFUniquePtr<CFStringRef> name { *optionalName })
                    {
                        const auto nameString = String::fromCFString (name.get());

                        if (isAggregateAudioDevice (audioDevice) && nameString.startsWith (yupPrivateAggregateDeviceNamePrefix))
                        {
                            YUP_MODULE_DBG (CORE_AUDIO, "Skipping private aggregate during scan: " << nameString << " [" << String (audioDevice) << "]");
                            continue;
                        }

                        const auto uidString = getAudioDeviceUID (audioDevice);
                        const auto numIns = getNumChannels (audioDevice, true);
                        const auto numOuts = getNumChannels (audioDevice, false);

                        YUP_MODULE_DBG (CORE_AUDIO, "Found device: " << nameString << " [" << String (audioDevice) << "]"
                                                                     << ", uid=" << uidString << ", inputs=" << String (numIns) << ", outputs=" << String (numOuts));

                        if (numIns > 0)
                            inputDevices.push_back ({ nameString, uidString, audioDevice });

                        if (numOuts > 0)
                            outputDevices.push_back ({ nameString, uidString, audioDevice });
                    }
                }
            }

            const auto sortDevices = [] (std::vector<DeviceEntry>& devices)
            {
                std::sort (devices.begin(), devices.end(), [] (const auto& a, const auto& b)
                {
                    if (const auto byName = a.name.compareNatural (b.name); byName != 0)
                        return byName < 0;

                    if (const auto byUid = a.uid.compare (b.uid); byUid != 0)
                        return byUid < 0;

                    return a.deviceID < b.deviceID;
                });
            };

            const auto populateDeviceList = [] (const std::vector<DeviceEntry>& devices, StringArray& names, Array<AudioDeviceID>& ids)
            {
                for (const auto& device : devices)
                {
                    names.add (device.name);
                    ids.add (device.deviceID);
                }
            };

            sortDevices (inputDevices);
            sortDevices (outputDevices);

            populateDeviceList (inputDevices, inputDeviceNames, inputIds);
            populateDeviceList (outputDevices, outputDeviceNames, outputIds);

            YUP_MODULE_DBG (CORE_AUDIO, "Scan complete: inputs=" << String (inputDeviceNames.size()) << ", outputs=" << String (outputDeviceNames.size()));

            inputDeviceNames.appendNumbersToDuplicates (false, true);
            outputDeviceNames.appendNumbersToDuplicates (false, true);
        }

        StringArray getDeviceNames (bool wantInputNames) const override
        {
            jassert (hasScanned); // need to call scanForDevices() before doing this

            return wantInputNames ? inputDeviceNames
                                  : outputDeviceNames;
        }

        int getDefaultDeviceIndex (bool forInput) const override
        {
            jassert (hasScanned); // need to call scanForDevices() before doing this

            // if they're asking for any input channels at all, use the default input, so we
            // get the built-in mic rather than the built-in output with no inputs..

            const auto selector = forInput ? kAudioHardwarePropertyDefaultInputDevice
                                           : kAudioHardwarePropertyDefaultOutputDevice;

            if (auto deviceID = audioObjectGetProperty<AudioDeviceID> (kAudioObjectSystemObject, PropertyAddress (selector)))
            {
                auto& ids = forInput ? inputIds : outputIds;

                if (auto it = std::find (ids.begin(), ids.end(), deviceID); it != ids.end())
                {
                    const auto index = static_cast<int> (std::distance (ids.begin(), it));
                    YUP_MODULE_DBG (CORE_AUDIO, "Default " << (forInput ? "input" : "output") << " device resolved: index=" << String (index) << ", " << describeAudioDeviceID (*deviceID));
                    return index;
                }

                YUP_MODULE_DBG (CORE_AUDIO, "Default " << (forInput ? "input" : "output") << " device not found in scan: " << describeAudioDeviceID (*deviceID));
            }

            YUP_MODULE_DBG (CORE_AUDIO, "Default " << (forInput ? "input" : "output") << " device falling back to index 0");
            return 0;
        }

        int getIndexOfDevice (AudioIODevice* device, bool asInput) const override
        {
            jassert (hasScanned); // need to call scanForDevices() before doing this

            if (auto* d = dynamic_cast<CoreAudioIODevice*> (device))
                return d->getIndexOfDevice (asInput);

            return -1;
        }

        bool hasSeparateInputsAndOutputs() const override { return true; }

        AudioIODevice* createDevice (const String& outputDeviceName,
                                     const String& inputDeviceName) override
        {
            jassert (hasScanned); // need to call scanForDevices() before doing this

            auto inputIndex = inputDeviceNames.indexOf (inputDeviceName);
            auto outputIndex = outputDeviceNames.indexOf (outputDeviceName);

            auto inputDeviceID = inputIds[inputIndex];
            auto outputDeviceID = outputIds[outputIndex];

            YUP_MODULE_DBG (CORE_AUDIO, "createDevice requested: outputName=" << outputDeviceName << ", outputIndex=" << String (outputIndex) << ", output=" << describeAudioDeviceID (outputDeviceID) << ", inputName=" << inputDeviceName << ", inputIndex=" << String (inputIndex) << ", input=" << describeAudioDeviceID (inputDeviceID));

            if (inputDeviceID == 0 && outputDeviceID == 0)
            {
                YUP_MODULE_DBG (CORE_AUDIO, "createDevice failed: no input or output device selected");
                return nullptr;
            }

            const auto combinedName = outputDeviceName.isEmpty() ? inputDeviceName
                                                                 : outputDeviceName;

            YUP_MODULE_DBG (CORE_AUDIO, "createDevice using CoreAudioIODevice: name=" << combinedName);
            auto device = std::make_unique<CoreAudioIODevice> (this, combinedName, inputDeviceID, outputDeviceID, inputIndex, outputIndex);

            if (! device->isValid())
            {
                YUP_MODULE_DBG (CORE_AUDIO, "createDevice failed: couldn't initialise CoreAudioIODevice");
                return nullptr;
            }

            return device.release();
        }

        void audioDeviceListChanged()
        {
            scanForDevices();
            callDeviceChangeListeners();
        }

        //==============================================================================
    private:
        StringArray inputDeviceNames, outputDeviceNames;
        Array<AudioDeviceID> inputIds, outputIds;

        bool hasScanned = false;

        void handleAsyncUpdate() override
        {
            audioDeviceListChanged();
        }

        static int getNumChannels (AudioDeviceID deviceID, bool input)
        {
            return getNumChannelsForAudioDevice (deviceID, input);
        }

        static OSStatus hardwareListenerProc (AudioDeviceID, UInt32, const AudioObjectPropertyAddress*, void* clientData)
        {
            static_cast<CoreAudioIODeviceType*> (clientData)->triggerAsyncUpdate();
            return noErr;
        }

        YUP_DECLARE_WEAK_REFERENCEABLE (CoreAudioIODeviceType)
        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CoreAudioIODeviceType)
    };
};

} // namespace yup
