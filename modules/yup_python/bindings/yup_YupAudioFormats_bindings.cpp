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

#include "yup_YupAudioFormats_bindings.h"

#include "../utilities/yup_PythonInterop.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#include "../utilities/yup_PyBind11Includes.h"

//==============================================================================

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

void registerYupAudioFormatsBindings (py::module_& m)
{
    // clang-format off

    // ============================================================================================ yup::AudioFormatType

    py::enum_<AudioFormatType> (m, "AudioFormatType")
        .value ("wav", AudioFormatType::wav)
        .value ("mp3", AudioFormatType::mp3)
        .value ("flac", AudioFormatType::flac)
        .value ("ogg", AudioFormatType::ogg)
        .value ("opus", AudioFormatType::opus)
        .value ("coreAudio", AudioFormatType::coreAudio)
        .value ("windowsMedia", AudioFormatType::windowsMedia)
        .value ("all", AudioFormatType::all)
        .export_values();

    // ============================================================================================ yup::AudioFormatManager

    py::class_<AudioFormatManager> (m, "AudioFormatManager")
        .def (py::init<>())
        .def ("registerDefaultFormats", &AudioFormatManager::registerDefaultFormats,
              "types"_a = AudioFormatType::all)
        .def ("registerFormat", &AudioFormatManager::registerFormat)
        .def ("createReaderFor", &AudioFormatManager::createReaderFor)
        .def ("__repr__", [] (const AudioFormatManager& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " object at " << String::formatted ("%p", std::addressof (self)) << ">";
            return result;
        });

    // ============================================================================================ yup::AudioFormatReader

    py::class_<AudioFormatReader> (m, "AudioFormatReader")
        .def ("getFormatName", &AudioFormatReader::getFormatName)
        .def ("read", [](AudioFormatReader& self,
                         AudioBuffer<float>* buffer,
                         int startSampleInDestBuffer,
                         int numSamples,
                         int64 readerStartSample,
                         bool useReaderLeftChan,
                         bool useReaderRightChan)
        {
            return self.read (buffer, startSampleInDestBuffer, numSamples,
                              readerStartSample, useReaderLeftChan, useReaderRightChan);
        },
            "buffer"_a,
            "startSampleInDestBuffer"_a,
            "numSamples"_a,
            "readerStartSample"_a,
            "useReaderLeftChan"_a,
            "useReaderRightChan"_a)
        .def_readonly ("sampleRate", &AudioFormatReader::sampleRate)
        .def_readonly ("bitsPerSample", &AudioFormatReader::bitsPerSample)
        .def_readonly ("lengthInSamples", &AudioFormatReader::lengthInSamples)
        .def_readonly ("numChannels", &AudioFormatReader::numChannels)
        .def_readonly ("usesFloatingPointData", &AudioFormatReader::usesFloatingPointData)
        .def_readonly ("metadataValues", &AudioFormatReader::metadataValues)
        .def ("__repr__", [] (const AudioFormatReader& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " format=\"" << self.getFormatName() << "\""
                << " sampleRate=" << self.sampleRate
                << " numChannels=" << self.numChannels
                << " lengthInSamples=" << self.lengthInSamples << ">";
            return result;
        });

    // ============================================================================================ yup::AudioFormatReaderSource

    py::class_<AudioFormatReaderSource, PositionableAudioSource> (m, "AudioFormatReaderSource")
        .def (py::init ([] (AudioFormatReader* reader, bool deleteReaderWhenThisIsDeleted)
        {
            // Transfer ownership: release the Python-owned reader to C++
            return std::make_unique<AudioFormatReaderSource> (reader, deleteReaderWhenThisIsDeleted);
        }),
              "sourceReader"_a, "deleteReaderWhenThisIsDeleted"_a = true)
        .def ("getAudioFormatReader", [] (AudioFormatReaderSource& self) -> AudioFormatReader*
        {
            if (auto* reader = self.getAudioFormatReader())
                return reader;
            return nullptr;
        }, py::return_value_policy::reference)
        .def ("__repr__", [] (const AudioFormatReaderSource& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (self).name(), 1)
                << " totalLength=" << self.getTotalLength()
                << " looping=" << (self.isLooping() ? "true" : "false") << ">";
            return result;
        });

    // clang-format on
}

} // namespace yup::Bindings
