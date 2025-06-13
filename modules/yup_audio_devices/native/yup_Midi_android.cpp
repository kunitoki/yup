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

//==============================================================================
#include "generated/yup_YupMidiSupport_bytecode.h"
#define javaYupMidiSupport yup::javaYupMidiSupportBytecode

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK)                                            \
    METHOD (getYupAndroidMidiInputDeviceNameAndIDs, "getYupAndroidMidiInputDeviceNameAndIDs", "()[Ljava/lang/String;")   \
    METHOD (getYupAndroidMidiOutputDeviceNameAndIDs, "getYupAndroidMidiOutputDeviceNameAndIDs", "()[Ljava/lang/String;") \
    METHOD (openMidiInputPortWithID, "openMidiInputPortWithID", "(IJ)Lorg/kunitoki/yup/YupMidiSupport$YupMidiPort;")     \
    METHOD (openMidiOutputPortWithID, "openMidiOutputPortWithID", "(I)Lorg/kunitoki/yup/YupMidiSupport$YupMidiPort;")

DECLARE_JNI_CLASS_WITH_MIN_SDK (MidiDeviceManager, "org/kunitoki/yup/YupMidiSupport$MidiDeviceManager", 23)
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (start, "start", "()V")                                            \
    METHOD (stop, "stop", "()V")                                              \
    METHOD (close, "close", "()V")                                            \
    METHOD (sendMidi, "sendMidi", "([BII)V")                                  \
    METHOD (getName, "getName", "()Ljava/lang/String;")

DECLARE_JNI_CLASS_WITH_MIN_SDK (YupMidiPort, "org/kunitoki/yup/YupMidiSupport$YupMidiPort", 23)
#undef JNI_CLASS_MEMBERS

//==============================================================================
class MidiInput::Pimpl
{
public:
    Pimpl (MidiInput* midiInput, int deviceID, yup::MidiInputCallback* midiInputCallback, jobject deviceManager)
        : yupMidiInput (midiInput)
        , callback (midiInputCallback)
        , midiConcatenator (2048)
        , javaMidiDevice (LocalRef<jobject> (getEnv()->CallObjectMethod (deviceManager,
                                                                         MidiDeviceManager.openMidiInputPortWithID,
                                                                         (jint) deviceID,
                                                                         (jlong) this)))
    {
    }

    ~Pimpl()
    {
        if (jobject d = javaMidiDevice.get())
        {
            getEnv()->CallVoidMethod (d, YupMidiPort.close);
            javaMidiDevice.clear();
        }
    }

    bool isOpen() const noexcept
    {
        return javaMidiDevice != nullptr;
    }

    void start()
    {
        if (jobject d = javaMidiDevice.get())
            getEnv()->CallVoidMethod (d, YupMidiPort.start);
    }

    void stop()
    {
        if (jobject d = javaMidiDevice.get())
            getEnv()->CallVoidMethod (d, YupMidiPort.stop);

        callback = nullptr;
    }

    String getName() const noexcept
    {
        if (jobject d = javaMidiDevice.get())
            return yupString (LocalRef<jstring> ((jstring) getEnv()->CallObjectMethod (d, YupMidiPort.getName)));

        return {};
    }

    static void handleReceive (JNIEnv* env, Pimpl& myself, jbyteArray byteArray, jint offset, jint len, jlong timestamp)
    {
        jassert (byteArray != nullptr);
        auto* data = env->GetByteArrayElements (byteArray, nullptr);

        std::vector<uint8> buffer (static_cast<size_t> (len));
        std::memcpy (buffer.data(), data + offset, static_cast<size_t> (len));

        myself.midiConcatenator.pushMidiData (buffer.data(),
                                              len,
                                              static_cast<double> (timestamp) * 1.0e-9,
                                              myself.yupMidiInput,
                                              *myself.callback);

        env->ReleaseByteArrayElements (byteArray, data, 0);
    }

private:
    MidiInput* yupMidiInput;
    MidiInputCallback* callback;
    MidiDataConcatenator midiConcatenator;
    GlobalRef javaMidiDevice;
};

//==============================================================================
class MidiOutput::Pimpl
{
public:
    Pimpl (const LocalRef<jobject>& midiDevice)
        : javaMidiDevice (midiDevice)
    {
    }

    ~Pimpl()
    {
        if (jobject d = javaMidiDevice.get())
        {
            getEnv()->CallVoidMethod (d, YupMidiPort.close);
            javaMidiDevice.clear();
        }
    }

    void send (jbyteArray byteArray, jint offset, jint len)
    {
        if (jobject d = javaMidiDevice.get())
            getEnv()->CallVoidMethod (d,
                                      YupMidiPort.sendMidi,
                                      byteArray,
                                      offset,
                                      len);
    }

    String getName() const noexcept
    {
        if (jobject d = javaMidiDevice.get())
            return yupString (LocalRef<jstring> ((jstring) getEnv()->CallObjectMethod (d, YupMidiPort.getName)));

        return {};
    }

private:
    GlobalRef javaMidiDevice;
};

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    CALLBACK (generatedCallback<&MidiInput::Pimpl::handleReceive>, "handleReceive", "(J[BIIJ)V")

DECLARE_JNI_CLASS_WITH_MIN_SDK (YupMidiInputPort, "org/kunitoki/yup/YupMidiSupport$YupMidiInputPort", 23)
#undef JNI_CLASS_MEMBERS

//==============================================================================
class AndroidMidiDeviceManager
{
public:
    AndroidMidiDeviceManager() = default;

    Array<MidiDeviceInfo> getDevices (bool input)
    {
        if (jobject dm = deviceManager.get())
        {
            jobjectArray jDeviceNameAndIDs = (jobjectArray) getEnv()->CallObjectMethod (dm, input ? MidiDeviceManager.getYupAndroidMidiInputDeviceNameAndIDs : MidiDeviceManager.getYupAndroidMidiOutputDeviceNameAndIDs);

            // Create a local reference as converting this to a JUCE string will call into JNI
            LocalRef<jobjectArray> localDeviceNameAndIDs (jDeviceNameAndIDs);

            auto deviceNameAndIDs = javaStringArrayToYup (localDeviceNameAndIDs);
            deviceNameAndIDs.appendNumbersToDuplicates (false, false, CharPointer_UTF8 ("-"), CharPointer_UTF8 (""));

            Array<MidiDeviceInfo> devices;

            for (int i = 0; i < deviceNameAndIDs.size(); i += 2)
                devices.add ({ deviceNameAndIDs[i], deviceNameAndIDs[i + 1] });

            return devices;
        }

        return {};
    }

    MidiInput::Pimpl* openMidiInputPortWithID (int deviceID, MidiInput* yupMidiInput, yup::MidiInputCallback* callback)
    {
        if (auto dm = deviceManager.get())
        {
            auto androidMidiInput = std::make_unique<MidiInput::Pimpl> (yupMidiInput, deviceID, callback, dm);

            if (androidMidiInput->isOpen())
                return androidMidiInput.release();

            // Perhaps the device is already open
            jassertfalse;
        }

        return nullptr;
    }

    MidiOutput::Pimpl* openMidiOutputPortWithID (int deviceID)
    {
        if (auto dm = deviceManager.get())
        {
            if (auto javaMidiPort = getEnv()->CallObjectMethod (dm, MidiDeviceManager.openMidiOutputPortWithID, (jint) deviceID))
                return new MidiOutput::Pimpl (LocalRef<jobject> (javaMidiPort));

            // Perhaps the port is already open
            jassertfalse;
        }

        return nullptr;
    }

private:
    static void handleDevicesChanged (JNIEnv*, jclass)
    {
        MidiDeviceListConnectionBroadcaster::get().notify();
    }

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK)                                                                                   \
    CALLBACK (handleDevicesChanged, "handleDevicesChanged", "()V")                                                                                              \
    STATICMETHOD (getAndroidMidiDeviceManager, "getAndroidMidiDeviceManager", "(Landroid/content/Context;)Lorg/kunitoki/yup/YupMidiSupport$MidiDeviceManager;") \
    STATICMETHOD (getAndroidBluetoothManager, "getAndroidBluetoothManager", "(Landroid/content/Context;)Lorg/kunitoki/yup/YupMidiSupport$BluetoothMidiManager;")

    DECLARE_JNI_CLASS_WITH_BYTECODE (YupMidiSupport, "org/kunitoki/yup/YupMidiSupport", 23, javaYupMidiSupport)
#undef JNI_CLASS_MEMBERS

    GlobalRef deviceManager { LocalRef<jobject> (getEnv()->CallStaticObjectMethod (YupMidiSupport,
                                                                                   YupMidiSupport.getAndroidMidiDeviceManager,
                                                                                   getAppContext().get())) };
};

//==============================================================================
Array<MidiDeviceInfo> MidiInput::getAvailableDevices()
{
    if (getAndroidSDKVersion() < 23)
        return {};

    AndroidMidiDeviceManager manager;
    return manager.getDevices (true);
}

MidiDeviceInfo MidiInput::getDefaultDevice()
{
    if (getAndroidSDKVersion() < 23)
        return {};

    return getAvailableDevices().getFirst();
}

std::unique_ptr<MidiInput> MidiInput::openDevice (const String& deviceIdentifier, MidiInputCallback* callback)
{
    if (getAndroidSDKVersion() < 23 || deviceIdentifier.isEmpty())
        return {};

    AndroidMidiDeviceManager manager;

    std::unique_ptr<MidiInput> midiInput (new MidiInput ({}, deviceIdentifier));

    if (auto* port = manager.openMidiInputPortWithID (deviceIdentifier.getIntValue(), midiInput.get(), callback))
    {
        midiInput->internal.reset (port);
        midiInput->setName (port->getName());

        return midiInput;
    }

    return {};
}

MidiInput::MidiInput (const String& deviceName, const String& deviceIdentifier)
    : deviceInfo (deviceName, deviceIdentifier)
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
Array<MidiDeviceInfo> MidiOutput::getAvailableDevices()
{
    if (getAndroidSDKVersion() < 23)
        return {};

    AndroidMidiDeviceManager manager;
    return manager.getDevices (false);
}

MidiDeviceInfo MidiOutput::getDefaultDevice()
{
    if (getAndroidSDKVersion() < 23)
        return {};

    return getAvailableDevices().getFirst();
}

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String& deviceIdentifier)
{
    if (getAndroidSDKVersion() < 23 || deviceIdentifier.isEmpty())
        return {};

    AndroidMidiDeviceManager manager;

    if (auto* port = manager.openMidiOutputPortWithID (deviceIdentifier.getIntValue()))
    {
        std::unique_ptr<MidiOutput> midiOutput (new MidiOutput ({}, deviceIdentifier));
        midiOutput->internal.reset (port);
        midiOutput->setName (port->getName());

        return midiOutput;
    }

    return {};
}

MidiOutput::~MidiOutput()
{
    stopBackgroundThread();
}

void MidiOutput::sendMessageNow (const MidiMessage& message)
{
    if (auto* androidMidi = internal.get())
    {
        auto* env = getEnv();
        auto messageSize = message.getRawDataSize();

        LocalRef<jbyteArray> messageContent (env->NewByteArray (messageSize));
        auto content = messageContent.get();

        auto* rawBytes = env->GetByteArrayElements (content, nullptr);
        std::memcpy (rawBytes, message.getRawData(), static_cast<size_t> (messageSize));
        env->ReleaseByteArrayElements (content, rawBytes, 0);

        androidMidi->send (content, (jint) 0, (jint) messageSize);
    }
}

MidiDeviceListConnection MidiDeviceListConnection::make (std::function<void()> callback)
{
    auto& broadcaster = MidiDeviceListConnectionBroadcaster::get();
    return { &broadcaster, broadcaster.add (std::move (callback)) };
}

} // namespace yup
