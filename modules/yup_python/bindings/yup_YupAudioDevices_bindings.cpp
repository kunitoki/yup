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

#include "yup_YupAudioDevices_bindings.h"

#include "../utilities/yup_PythonInterop.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#include "../utilities/yup_PyBind11Includes.h"

//==============================================================================

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

void registerYupAudioDevicesBindings (py::module_& m)
{
    // clang-format off

    // ============================================================================================ yup::WASAPIDeviceMode

    py::enum_<WASAPIDeviceMode> (m, "WASAPIDeviceMode")
        .value ("shared", WASAPIDeviceMode::shared)
        .value ("exclusive", WASAPIDeviceMode::exclusive)
        .value ("sharedLowLatency", WASAPIDeviceMode::sharedLowLatency)
        .export_values();

    // ============================================================================================ yup::AudioIODeviceCallbackContext

    py::class_<AudioIODeviceCallbackContext> (m, "AudioIODeviceCallbackContext")
        .def (py::init<>())
        .def_readwrite ("hostTimeNs", &AudioIODeviceCallbackContext::hostTimeNs);

    // ============================================================================================ yup::AudioIODeviceCallback

    py::class_<AudioIODeviceCallback, PyAudioIODeviceCallback> (m, "AudioIODeviceCallback")
        .def (py::init<>())
        .def ("audioDeviceAboutToStart", &AudioIODeviceCallback::audioDeviceAboutToStart)
        .def ("audioDeviceStopped", &AudioIODeviceCallback::audioDeviceStopped)
        .def ("audioDeviceError", &AudioIODeviceCallback::audioDeviceError);

    // ============================================================================================ yup::AudioIODevice

    py::class_<AudioIODevice> (m, "AudioIODevice")
        .def ("getName", &AudioIODevice::getName)
        .def ("getTypeName", &AudioIODevice::getTypeName)
        .def ("getOutputChannelNames", &AudioIODevice::getOutputChannelNames)
        .def ("getInputChannelNames", &AudioIODevice::getInputChannelNames)
        .def ("getDefaultOutputChannels", &AudioIODevice::getDefaultOutputChannels)
        .def ("getDefaultInputChannels", &AudioIODevice::getDefaultInputChannels)
        .def ("getAvailableSampleRates", &AudioIODevice::getAvailableSampleRates)
        .def ("getAvailableBufferSizes", &AudioIODevice::getAvailableBufferSizes)
        .def ("getDefaultBufferSize", &AudioIODevice::getDefaultBufferSize)
        .def ("open", &AudioIODevice::open)
        .def ("close", &AudioIODevice::close)
        .def ("isOpen", &AudioIODevice::isOpen)
        .def ("start", &AudioIODevice::start)
        .def ("stop", &AudioIODevice::stop)
        .def ("isPlaying", &AudioIODevice::isPlaying)
        .def ("getLastError", &AudioIODevice::getLastError)
        .def ("getCurrentBufferSizeSamples", &AudioIODevice::getCurrentBufferSizeSamples)
        .def ("getCurrentSampleRate", &AudioIODevice::getCurrentSampleRate)
        .def ("getCurrentBitDepth", &AudioIODevice::getCurrentBitDepth)
        .def ("getActiveOutputChannels", &AudioIODevice::getActiveOutputChannels)
        .def ("getActiveInputChannels", &AudioIODevice::getActiveInputChannels)
        .def ("getOutputLatencyInSamples", &AudioIODevice::getOutputLatencyInSamples)
        .def ("getInputLatencyInSamples", &AudioIODevice::getInputLatencyInSamples)
        .def ("hasControlPanel", &AudioIODevice::hasControlPanel)
        .def ("showControlPanel", &AudioIODevice::showControlPanel)
        .def ("setAudioPreprocessingEnabled", &AudioIODevice::setAudioPreprocessingEnabled)
        .def ("getXRunCount", &AudioIODevice::getXRunCount)
        .def ("__repr__", [] (const AudioIODevice& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " name=\"" << self.getName() << "\""
                << " type=\"" << self.getTypeName() << "\">";
            return result;
        });

    // ============================================================================================ yup::AudioDeviceManager::AudioDeviceSetup

    py::class_<AudioDeviceManager::AudioDeviceSetup> (m, "AudioDeviceSetup")
        .def (py::init<>())
        .def_readwrite ("outputDeviceName", &AudioDeviceManager::AudioDeviceSetup::outputDeviceName)
        .def_readwrite ("inputDeviceName", &AudioDeviceManager::AudioDeviceSetup::inputDeviceName)
        .def_readwrite ("sampleRate", &AudioDeviceManager::AudioDeviceSetup::sampleRate)
        .def_readwrite ("bufferSize", &AudioDeviceManager::AudioDeviceSetup::bufferSize)
        .def_readwrite ("inputChannels", &AudioDeviceManager::AudioDeviceSetup::inputChannels)
        .def_readwrite ("useDefaultInputChannels", &AudioDeviceManager::AudioDeviceSetup::useDefaultInputChannels)
        .def_readwrite ("outputChannels", &AudioDeviceManager::AudioDeviceSetup::outputChannels)
        .def_readwrite ("useDefaultOutputChannels", &AudioDeviceManager::AudioDeviceSetup::useDefaultOutputChannels)
        .def ("__eq__", &AudioDeviceManager::AudioDeviceSetup::operator==)
        .def ("__ne__", &AudioDeviceManager::AudioDeviceSetup::operator!=);

    // ============================================================================================ yup::AudioDeviceManager::LevelMeter

    py::class_<AudioDeviceManager::LevelMeter, ReferenceCountedObjectPtr<AudioDeviceManager::LevelMeter>> (m, "LevelMeter")
        .def ("getCurrentLevel", &AudioDeviceManager::LevelMeter::getCurrentLevel);

    // ============================================================================================ yup::AudioDeviceManager

    py::class_<AudioDeviceManager, ChangeBroadcaster> (m, "AudioDeviceManager")
        .def (py::init<>())
        .def ("initialise", [] (AudioDeviceManager& self,
                                int numInputChannelsNeeded,
                                int numOutputChannelsNeeded,
                                const XmlElement* savedState,
                                bool selectDefaultDeviceOnFailure,
                                const String& preferredDefaultDeviceName,
                                const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
        {
            return self.initialise (numInputChannelsNeeded,
                                    numOutputChannelsNeeded,
                                    savedState,
                                    selectDefaultDeviceOnFailure,
                                    preferredDefaultDeviceName,
                                    preferredSetupOptions);
        },
            "numInputChannelsNeeded"_a,
            "numOutputChannelsNeeded"_a,
            "savedState"_a = nullptr,
            "selectDefaultDeviceOnFailure"_a = true,
            "preferredDefaultDeviceName"_a = String(),
            "preferredSetupOptions"_a = nullptr)
        .def ("initialiseWithDefaultDevices", &AudioDeviceManager::initialiseWithDefaultDevices,
              "numInputChannelsNeeded"_a, "numOutputChannelsNeeded"_a)
        .def ("createStateXml", &AudioDeviceManager::createStateXml)
        .def ("getAudioDeviceSetup", [] (AudioDeviceManager& self)
        {
            return self.getAudioDeviceSetup();
        })
        .def ("setAudioDeviceSetup", &AudioDeviceManager::setAudioDeviceSetup,
              "newSetup"_a, "treatAsChosenDevice"_a)
        .def ("getCurrentAudioDevice", &AudioDeviceManager::getCurrentAudioDevice,
              py::return_value_policy::reference)
        .def ("getCurrentAudioDeviceType", &AudioDeviceManager::getCurrentAudioDeviceType)
        .def ("getCurrentDeviceTypeObject", &AudioDeviceManager::getCurrentDeviceTypeObject,
              py::return_value_policy::reference)
        .def ("setCurrentAudioDeviceType", &AudioDeviceManager::setCurrentAudioDeviceType,
              "type"_a, "treatAsChosenDevice"_a)
        .def ("closeAudioDevice", &AudioDeviceManager::closeAudioDevice)
        .def ("restartLastAudioDevice", &AudioDeviceManager::restartLastAudioDevice)
        .def ("addAudioCallback", &AudioDeviceManager::addAudioCallback)
        .def ("removeAudioCallback", &AudioDeviceManager::removeAudioCallback)
        .def ("getCpuUsage", &AudioDeviceManager::getCpuUsage)
        .def ("playTestSound", &AudioDeviceManager::playTestSound)
        .def ("getInputLevelGetter", &AudioDeviceManager::getInputLevelGetter)
        .def ("getOutputLevelGetter", &AudioDeviceManager::getOutputLevelGetter)
        .def ("__repr__", [] (const AudioDeviceManager& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " type=\"" << self.getCurrentAudioDeviceType() << "\">";
            return result;
        });

    // ============================================================================================ yup::AudioSourcePlayer

    py::class_<AudioSourcePlayer, AudioIODeviceCallback> (m, "AudioSourcePlayer")
        .def (py::init<>())
        .def ("setSource", [] (AudioSourcePlayer& self, AudioSource* source)
        {
            self.setSource (source);
        }, "newSource"_a)
        .def ("getCurrentSource", [] (AudioSourcePlayer& self) -> py::object
        {
            auto* src = self.getCurrentSource();
            if (src == nullptr)
                return py::none();
            return py::cast (src, py::return_value_policy::reference);
        })
        .def ("setGain", &AudioSourcePlayer::setGain)
        .def ("getGain", &AudioSourcePlayer::getGain)
        .def ("prepareToPlay", &AudioSourcePlayer::prepareToPlay);

    // ============================================================================================ yup::AudioTransportSource

    py::class_<AudioTransportSource, PositionableAudioSource, ChangeBroadcaster> (m, "AudioTransportSource")
        .def (py::init<>())
        .def ("setSource", &AudioTransportSource::setSource,
              "newSource"_a,
              "readAheadBufferSize"_a = 0,
              "readAheadThread"_a = nullptr,
              "sourceSampleRateToCorrectFor"_a = 0.0,
              "maxNumChannels"_a = 2)
        .def ("setPosition", &AudioTransportSource::setPosition)
        .def ("getCurrentPosition", &AudioTransportSource::getCurrentPosition)
        .def ("getLengthInSeconds", &AudioTransportSource::getLengthInSeconds)
        .def ("hasStreamFinished", &AudioTransportSource::hasStreamFinished)
        .def ("start", &AudioTransportSource::start)
        .def ("stop", &AudioTransportSource::stop)
        .def ("isPlaying", &AudioTransportSource::isPlaying)
        .def ("setGain", &AudioTransportSource::setGain)
        .def ("getGain", &AudioTransportSource::getGain);

    // clang-format on
}

} // namespace yup::Bindings
