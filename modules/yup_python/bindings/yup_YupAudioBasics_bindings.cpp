/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#include "yup_YupAudioBasics_bindings.h"

#include "../utilities/yup_PythonInterop.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#include "../utilities/yup_PyBind11Includes.h"

//==============================================================================

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

void registerYupAudioBasicsBindings (py::module_& m)
{
    // clang-format off

    // ============================================================================================ yup::AudioBuffer

    auto registerAudioBuffer = []<typename Type> (py::module_& m, const char* name)
    {
        py::class_<AudioBuffer<Type>> classAudioBuffer (m, name);

        classAudioBuffer
            .def (py::init<>())
            .def (py::init<int, int>(), "numChannels"_a, "numSamples"_a)
            .def (py::init<const AudioBuffer<Type>&>())
            .def ("getNumChannels", &AudioBuffer<Type>::getNumChannels)
            .def ("getNumSamples", &AudioBuffer<Type>::getNumSamples)
            .def ("getReadPointer", py::overload_cast<int> (&AudioBuffer<Type>::getReadPointer, py::const_), py::return_value_policy::reference_internal)
            .def ("getReadPointer", py::overload_cast<int, int> (&AudioBuffer<Type>::getReadPointer, py::const_), py::return_value_policy::reference_internal)
            .def ("getWritePointer", py::overload_cast<int> (&AudioBuffer<Type>::getWritePointer), py::return_value_policy::reference_internal)
            .def ("getWritePointer", py::overload_cast<int, int> (&AudioBuffer<Type>::getWritePointer), py::return_value_policy::reference_internal)
            //.def ("getArrayOfReadPointers", &AudioBuffer<Type>::getArrayOfReadPointers, py::return_value_policy::reference_internal)
            //.def ("getArrayOfWritePointers", &AudioBuffer<Type>::getArrayOfWritePointers, py::return_value_policy::reference_internal)
            .def ("setSize", &AudioBuffer<Type>::setSize, "numChannels"_a, "numSamples"_a, "keepExistingContent"_a = false, "clearExtraSpace"_a = false, "avoidReallocating"_a = false)
            .def ("setDataToReferTo", [] (AudioBuffer<Type>& self, py::list dataToReferTo, int numChannels, int numSamples)
            {
                py::pybind11_fail ("setDataToReferTo is not yet supported in Python bindings");
            })
            .def ("clear", py::overload_cast<> (&AudioBuffer<Type>::clear))
            .def ("clear", py::overload_cast<int, int> (&AudioBuffer<Type>::clear))
            .def ("clear", py::overload_cast<int, int, int> (&AudioBuffer<Type>::clear))
            .def ("hasBeenCleared", &AudioBuffer<Type>::hasBeenCleared)
            .def ("getSample", &AudioBuffer<Type>::getSample)
            .def ("setSample", &AudioBuffer<Type>::setSample)
            .def ("addSample", &AudioBuffer<Type>::addSample)
            .def ("applyGain", py::overload_cast<int, int, int, Type> (&AudioBuffer<Type>::applyGain))
            .def ("applyGain", py::overload_cast<int, int, Type> (&AudioBuffer<Type>::applyGain))
            .def ("applyGain", py::overload_cast<Type> (&AudioBuffer<Type>::applyGain))
            .def ("applyGainRamp", py::overload_cast<int, int, int, Type, Type> (&AudioBuffer<Type>::applyGainRamp))
            .def ("addFrom", py::overload_cast<int, int, const AudioBuffer<Type>&, int, int, int, Type> (&AudioBuffer<Type>::addFrom))
            .def ("addFrom", py::overload_cast<int, int, const Type*, int, Type> (&AudioBuffer<Type>::addFrom))
            .def ("addFromWithRamp", &AudioBuffer<Type>::addFromWithRamp)
            .def ("copyFrom", py::overload_cast<int, int, const AudioBuffer<Type>&, int, int, int> (&AudioBuffer<Type>::copyFrom))
            .def ("copyFrom", py::overload_cast<int, int, const Type*, int> (&AudioBuffer<Type>::copyFrom))
            .def ("copyFrom", py::overload_cast<int, int, const Type*, int, Type> (&AudioBuffer<Type>::copyFrom))
            .def ("copyFromWithRamp", &AudioBuffer<Type>::copyFromWithRamp)
            .def ("findMinMax", &AudioBuffer<Type>::findMinMax)
            .def ("getMagnitude", py::overload_cast<int, int, int> (&AudioBuffer<Type>::getMagnitude, py::const_))
            .def ("getMagnitude", py::overload_cast<int, int> (&AudioBuffer<Type>::getMagnitude, py::const_))
            .def ("getRMSLevel", &AudioBuffer<Type>::getRMSLevel)
            //.def ("reverse", py::overload_cast<int, int> (&AudioBuffer<Type>::reverse))
            //.def ("reverse", py::overload_cast<int, int, int> (&AudioBuffer<Type>::reverse))
            .def ("__repr__", [name] (const AudioBuffer<Type>& self)
            {
                String result;
                result
                    << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, name, 1)
                    << " object at " << String::formatted ("%p", std::addressof (self))
                    << " channels=" << self.getNumChannels()
                    << " samples=" << self.getNumSamples() << ">";
                return result;
            });
    };

    registerAudioBuffer.operator()<float> (m, "AudioBufferFloat");
    registerAudioBuffer.operator()<double> (m, "AudioBufferDouble");

    // Alias for the most common type
    m.attr ("AudioBuffer") = m.attr ("AudioBufferFloat");

    // ============================================================================================ yup::AudioChannelSet (forward declare for Array)

    py::class_<AudioChannelSet> classAudioChannelSet (m, "AudioChannelSet");

    // ============================================================================================ yup::Array<AudioChannelSet::ChannelType> and Array<AudioChannelSet>

    py::enum_<AudioChannelSet::ChannelType> (classAudioChannelSet, "ChannelType")
        .value ("Unknown", AudioChannelSet::ChannelType::unknown)
        .value ("Left", AudioChannelSet::ChannelType::left)
        .value ("Right", AudioChannelSet::ChannelType::right)
        .value ("Center", AudioChannelSet::ChannelType::centre)
        .value ("LFE", AudioChannelSet::ChannelType::LFE)
        .value ("LeftSurround", AudioChannelSet::ChannelType::leftSurround)
        .value ("RightSurround", AudioChannelSet::ChannelType::rightSurround)
        .value ("LeftCenter", AudioChannelSet::ChannelType::leftCentre)
        .value ("RightCenter", AudioChannelSet::ChannelType::rightCentre)
        .value ("CenterSurround", AudioChannelSet::ChannelType::centreSurround)
        .value ("LeftSurroundSide", AudioChannelSet::ChannelType::leftSurroundSide)
        .value ("RightSurroundSide", AudioChannelSet::ChannelType::rightSurroundSide)
        .value ("TopMiddle", AudioChannelSet::ChannelType::topMiddle)
        .value ("TopFrontLeft", AudioChannelSet::ChannelType::topFrontLeft)
        .value ("TopFrontCenter", AudioChannelSet::ChannelType::topFrontCentre)
        .value ("TopFrontRight", AudioChannelSet::ChannelType::topFrontRight)
        .value ("TopRearLeft", AudioChannelSet::ChannelType::topRearLeft)
        .value ("TopRearCenter", AudioChannelSet::ChannelType::topRearCentre)
        .value ("TopRearRight", AudioChannelSet::ChannelType::topRearRight)
        .value ("WideLeft", AudioChannelSet::ChannelType::wideLeft)
        .value ("WideRight", AudioChannelSet::ChannelType::wideRight)
        .value ("LFE2", AudioChannelSet::ChannelType::LFE2)
        .value ("LeftSurroundRear", AudioChannelSet::ChannelType::leftSurroundRear)
        .value ("RightSurroundRear", AudioChannelSet::ChannelType::rightSurroundRear)
        .value ("Ambisonics0", AudioChannelSet::ChannelType::ambisonicACN0)
        .value ("Ambisonics1", AudioChannelSet::ChannelType::ambisonicACN1)
        .value ("Ambisonics2", AudioChannelSet::ChannelType::ambisonicACN2)
        .value ("Ambisonics3", AudioChannelSet::ChannelType::ambisonicACN3)
        .value ("Ambisonics4", AudioChannelSet::ChannelType::ambisonicACN4)
        .value ("Ambisonics5", AudioChannelSet::ChannelType::ambisonicACN5)
        .value ("Ambisonics6", AudioChannelSet::ChannelType::ambisonicACN6)
        .value ("Ambisonics7", AudioChannelSet::ChannelType::ambisonicACN7)
        .value ("Ambisonics8", AudioChannelSet::ChannelType::ambisonicACN8)
        .value ("Ambisonics9", AudioChannelSet::ChannelType::ambisonicACN9)
        .value ("Ambisonics10", AudioChannelSet::ChannelType::ambisonicACN10)
        .value ("Ambisonics11", AudioChannelSet::ChannelType::ambisonicACN11)
        .value ("Ambisonics12", AudioChannelSet::ChannelType::ambisonicACN12)
        .value ("Ambisonics13", AudioChannelSet::ChannelType::ambisonicACN13)
        .value ("Ambisonics14", AudioChannelSet::ChannelType::ambisonicACN14)
        .value ("Ambisonics15", AudioChannelSet::ChannelType::ambisonicACN15)
        .value ("Ambisonics16", AudioChannelSet::ChannelType::ambisonicACN16)
        .value ("Ambisonics17", AudioChannelSet::ChannelType::ambisonicACN17)
        .value ("Ambisonics18", AudioChannelSet::ChannelType::ambisonicACN18)
        .value ("Ambisonics19", AudioChannelSet::ChannelType::ambisonicACN19)
        .value ("Ambisonics20", AudioChannelSet::ChannelType::ambisonicACN20)
        .value ("Ambisonics21", AudioChannelSet::ChannelType::ambisonicACN21)
        .value ("Ambisonics22", AudioChannelSet::ChannelType::ambisonicACN22)
        .value ("Ambisonics23", AudioChannelSet::ChannelType::ambisonicACN23)
        .value ("Ambisonics24", AudioChannelSet::ChannelType::ambisonicACN24)
        .value ("Ambisonics25", AudioChannelSet::ChannelType::ambisonicACN25)
        .value ("Ambisonics26", AudioChannelSet::ChannelType::ambisonicACN26)
        .value ("Ambisonics27", AudioChannelSet::ChannelType::ambisonicACN27)
        .value ("Ambisonics28", AudioChannelSet::ChannelType::ambisonicACN28)
        .value ("Ambisonics29", AudioChannelSet::ChannelType::ambisonicACN29)
        .value ("Ambisonics30", AudioChannelSet::ChannelType::ambisonicACN30)
        .value ("Ambisonics31", AudioChannelSet::ChannelType::ambisonicACN31)
        .value ("Ambisonics32", AudioChannelSet::ChannelType::ambisonicACN32)
        .value ("Ambisonics33", AudioChannelSet::ChannelType::ambisonicACN33)
        .value ("Ambisonics34", AudioChannelSet::ChannelType::ambisonicACN34)
        .value ("Ambisonics35", AudioChannelSet::ChannelType::ambisonicACN35)
        .value ("DiscreteChannel0", AudioChannelSet::ChannelType::discreteChannel0)
        .export_values();

    registerArray<Array, AudioChannelSet::ChannelType> (m);
    registerArray<Array, AudioChannelSet> (m);

    classAudioChannelSet
        .def (py::init<>())
        .def (py::init<const AudioChannelSet&>())
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def ("size", &AudioChannelSet::size)
        .def ("isDiscreteLayout", &AudioChannelSet::isDiscreteLayout)
        .def ("getTypeOfChannel", &AudioChannelSet::getTypeOfChannel)
        .def ("getChannelIndexForType", &AudioChannelSet::getChannelIndexForType)
        .def ("getChannelTypes", &AudioChannelSet::getChannelTypes)
        .def ("addChannel", &AudioChannelSet::addChannel)
        .def ("removeChannel", &AudioChannelSet::removeChannel)
        .def ("getSpeakerArrangementAsString", &AudioChannelSet::getSpeakerArrangementAsString)
        .def ("getDescription", &AudioChannelSet::getDescription)
        .def ("getAbbreviatedChannelTypeName", &AudioChannelSet::getAbbreviatedChannelTypeName)
        .def ("getChannelTypeName", &AudioChannelSet::getChannelTypeName)
        .def_static ("getChannelTypeFromAbbreviation", &AudioChannelSet::getChannelTypeFromAbbreviation)
        .def_static ("fromAbbreviatedString", &AudioChannelSet::fromAbbreviatedString)
        .def_static ("fromWaveChannelMask", &AudioChannelSet::fromWaveChannelMask)
        .def ("getWaveChannelMask", &AudioChannelSet::getWaveChannelMask)
        .def_static ("namedChannelSet", &AudioChannelSet::namedChannelSet)
        .def_static ("disabled", &AudioChannelSet::disabled)
        .def_static ("mono", &AudioChannelSet::mono)
        .def_static ("stereo", &AudioChannelSet::stereo)
        .def_static ("createLCR", &AudioChannelSet::createLCR)
        .def_static ("createLRS", &AudioChannelSet::createLRS)
        .def_static ("createLCRS", &AudioChannelSet::createLCRS)
        .def_static ("create5point0", &AudioChannelSet::create5point0)
        .def_static ("create5point1", &AudioChannelSet::create5point1)
        .def_static ("create6point0", &AudioChannelSet::create6point0)
        .def_static ("create6point1", &AudioChannelSet::create6point1)
        .def_static ("create6point0Music", &AudioChannelSet::create6point0Music)
        .def_static ("create6point1Music", &AudioChannelSet::create6point1Music)
        .def_static ("create7point0", &AudioChannelSet::create7point0)
        .def_static ("create7point1", &AudioChannelSet::create7point1)
        .def_static ("create7point0SDDS", &AudioChannelSet::create7point0SDDS)
        .def_static ("create7point1SDDS", &AudioChannelSet::create7point1SDDS)
        .def_static ("create7point0point2", &AudioChannelSet::create7point0point2)
        .def_static ("create7point1point2", &AudioChannelSet::create7point1point2)
        .def_static ("create9point0point4", &AudioChannelSet::create9point0point4)
        .def_static ("create9point1point4", &AudioChannelSet::create9point1point4)
        .def_static ("create9point0point6", &AudioChannelSet::create9point0point6)
        .def_static ("create9point1point6", &AudioChannelSet::create9point1point6)
        .def_static ("ambisonic", py::overload_cast<int> (&AudioChannelSet::ambisonic), "order"_a = 1)
        .def_static ("discreteChannels", &AudioChannelSet::discreteChannels)
        .def_static ("canonicalChannelSet", &AudioChannelSet::canonicalChannelSet)
        .def_static ("channelSetsWithNumberOfChannels", &AudioChannelSet::channelSetsWithNumberOfChannels)
        .def ("__repr__", [] (const AudioChannelSet& self)
        {
            String result;
            result
                << "<" << Helpers::pythonizeModuleClassName (PythonModuleName, typeid (AudioChannelSet).name(), 1)
                << " object at " << String::formatted ("%p", std::addressof (self))
                << " description=\"" << self.getDescription() << "\">";
            return result;
        });

    // ============================================================================================ yup::Decibels

    py::class_<Decibels> classDecibels (m, "Decibels");

    classDecibels
        .def_static ("decibelsToGain", [] (float decibels, float minusInfinityDb = -100.0f)
        {
            return Decibels::decibelsToGain (decibels, minusInfinityDb);
        }, "decibels"_a, "minusInfinityDb"_a = -100.0f)
        .def_static ("gainToDecibels", [] (float gain, float minusInfinityDb = -100.0f)
        {
            return Decibels::gainToDecibels (gain, minusInfinityDb);
        }, "gain"_a, "minusInfinityDb"_a = -100.0f)
        .def_static ("gainWithLowerBound", [] (float gain, float lowerBoundDb)
        {
            return Decibels::gainWithLowerBound (gain, lowerBoundDb);
        }, "gain"_a, "lowerBoundDb"_a)
        .def_static ("toString", [] (float decibels, int decimalPlaces = 2, float minusInfinityDb = -100.0f, bool shouldIncludeSuffix = true, StringRef customMinusInfinityString = {})
        {
            return Decibels::toString (decibels, decimalPlaces, minusInfinityDb, shouldIncludeSuffix, customMinusInfinityString);
        }, "decibels"_a, "decimalPlaces"_a = 2, "minusInfinityDb"_a = -100.0f, "shouldIncludeSuffix"_a = true, "customMinusInfinityString"_a = StringRef());

    // ============================================================================================ yup::ADSR

    py::class_<ADSR> classADSR (m, "ADSR");

    py::class_<ADSR::Parameters> classADSRParameters (classADSR, "Parameters");

    classADSRParameters
        .def (py::init<>())
        .def (py::init<float, float, float, float>(), "attack"_a, "decay"_a, "sustain"_a, "release"_a)
        .def_readwrite ("attack", &ADSR::Parameters::attack)
        .def_readwrite ("decay", &ADSR::Parameters::decay)
        .def_readwrite ("sustain", &ADSR::Parameters::sustain)
        .def_readwrite ("release", &ADSR::Parameters::release);

    classADSR
        .def (py::init<>())
        .def ("setParameters", &ADSR::setParameters)
        .def ("getParameters", &ADSR::getParameters)
        .def ("isActive", &ADSR::isActive)
        .def ("setSampleRate", &ADSR::setSampleRate)
        .def ("noteOn", &ADSR::noteOn)
        .def ("noteOff", &ADSR::noteOff)
        .def ("reset", &ADSR::reset)
        .def ("getNextSample", &ADSR::getNextSample);

    // ============================================================================================ yup::Reverb

    py::class_<Reverb> classReverb (m, "Reverb");

    py::class_<Reverb::Parameters> classReverbParameters (classReverb, "Parameters");

    classReverbParameters
        .def (py::init<>())
        .def_readwrite ("roomSize", &Reverb::Parameters::roomSize)
        .def_readwrite ("damping", &Reverb::Parameters::damping)
        .def_readwrite ("wetLevel", &Reverb::Parameters::wetLevel)
        .def_readwrite ("dryLevel", &Reverb::Parameters::dryLevel)
        .def_readwrite ("width", &Reverb::Parameters::width)
        .def_readwrite ("freezeMode", &Reverb::Parameters::freezeMode);

    classReverb
        .def (py::init<>())
        .def ("setParameters", &Reverb::setParameters)
        .def ("getParameters", &Reverb::getParameters)
        .def ("reset", &Reverb::reset)
        .def ("processStereo", &Reverb::processStereo)
        .def ("processMono", &Reverb::processMono);

    // ============================================================================================ yup::SmoothedValue

    auto registerSmoothedValue = []<typename Type> (py::module_& m, const char* name)
    {
        py::class_<SmoothedValue<Type>> classSmoothedValue (m, name);

        classSmoothedValue
            .def (py::init<>())
            .def (py::init<Type>(), "initialValue"_a)
            .def ("reset", [] (SmoothedValue<Type>& self, int numSteps) { self.reset (numSteps); })
            .def ("reset", [] (SmoothedValue<Type>& self, double sampleRate, double rampLengthInSeconds) { self.reset (sampleRate, rampLengthInSeconds); })
            .def ("setCurrentAndTargetValue", [] (SmoothedValue<Type>& self, Type newValue) { self.setCurrentAndTargetValue (newValue); })
            .def ("setTargetValue", [] (SmoothedValue<Type>& self, Type newValue) { self.setTargetValue (newValue); })
            .def ("getCurrentValue", [] (SmoothedValue<Type>& self) { return self.getCurrentValue(); })
            .def ("getTargetValue", [] (SmoothedValue<Type>& self) { return self.getTargetValue(); })
            .def ("getNextValue", [] (SmoothedValue<Type>& self) { return self.getNextValue(); })
            .def ("skip", [] (SmoothedValue<Type>& self, int numSamples) { self.skip (numSamples); })
            .def ("isSmoothing", [] (SmoothedValue<Type>& self) { return self.isSmoothing(); });
    };

    registerSmoothedValue.operator()<float> (m, "SmoothedValueFloat");
    registerSmoothedValue.operator()<double> (m, "SmoothedValueDouble");

    m.attr ("SmoothedValue") = m.attr ("SmoothedValueFloat");

    // ============================================================================================ yup::IIRCoefficients

    py::class_<IIRCoefficients> classIIRCoefficients (m, "IIRCoefficients");

    classIIRCoefficients
        .def (py::init<>())
        .def_static ("makeLowPass", static_cast<IIRCoefficients (*)(double, double)> (&IIRCoefficients::makeLowPass), "sampleRate"_a, "frequency"_a)
        .def_static ("makeHighPass", static_cast<IIRCoefficients (*)(double, double)> (&IIRCoefficients::makeHighPass), "sampleRate"_a, "frequency"_a)
        .def_static ("makeBandPass", static_cast<IIRCoefficients (*)(double, double)> (&IIRCoefficients::makeBandPass), "sampleRate"_a, "frequency"_a)
        .def_static ("makeLowShelf", &IIRCoefficients::makeLowShelf, "sampleRate"_a, "cutOffFrequency"_a, "Q"_a, "gainFactor"_a)
        .def_static ("makeHighShelf", &IIRCoefficients::makeHighShelf, "sampleRate"_a, "cutOffFrequency"_a, "Q"_a, "gainFactor"_a)
        .def_static ("makePeakFilter", &IIRCoefficients::makePeakFilter, "sampleRate"_a, "centerFrequency"_a, "Q"_a, "gainFactor"_a)
        .def_static ("makeNotchFilter", static_cast<IIRCoefficients (*)(double, double, double)> (&IIRCoefficients::makeNotchFilter), "sampleRate"_a, "frequency"_a, "Q"_a)
        .def_static ("makeAllPass", static_cast<IIRCoefficients (*)(double, double, double)> (&IIRCoefficients::makeAllPass), "sampleRate"_a, "frequency"_a, "Q"_a);

    // ============================================================================================ yup::IIRFilter

    py::class_<IIRFilter> classIIRFilter (m, "IIRFilter");

    classIIRFilter
        .def (py::init<>())
        .def ("reset", [] (IIRFilter& self) { self.reset(); })
        .def ("setCoefficients", [] (IIRFilter& self, const IIRCoefficients& coeffs) { self.setCoefficients (coeffs); })
        .def ("processSamples", [] (IIRFilter& self, float* samples, int numSamples) { self.processSamples (samples, numSamples); })
        .def ("processSingleSampleRaw", [] (IIRFilter& self, float sample) { return self.processSingleSampleRaw (sample); });

    // ============================================================================================ yup::AudioSourceChannelInfo

    py::class_<AudioSourceChannelInfo> classAudioSourceChannelInfo (m, "AudioSourceChannelInfo");

    classAudioSourceChannelInfo
        .def (py::init<>())
        .def (py::init ([](AudioBuffer<float>& bufferToUse, int startSampleOffset, int numSamplesToUse)
        {
            return AudioSourceChannelInfo (&bufferToUse, startSampleOffset, numSamplesToUse);
        }), "bufferToUse"_a, "startSampleOffset"_a, "numSamplesToRead"_a)
        .def_readwrite ("buffer", &AudioSourceChannelInfo::buffer)
        .def_readwrite ("startSample", &AudioSourceChannelInfo::startSample)
        .def_readwrite ("numSamples", &AudioSourceChannelInfo::numSamples)
        .def ("clearActiveBufferRegion", &AudioSourceChannelInfo::clearActiveBufferRegion);

    // ============================================================================================ yup::AudioSource

    py::class_<AudioSource, PyAudioSource<>> classAudioSource (m, "AudioSource");

    classAudioSource
        .def (py::init<>())
        .def ("prepareToPlay", &AudioSource::prepareToPlay)
        .def ("releaseResources", &AudioSource::releaseResources)
        .def ("getNextAudioBlock", &AudioSource::getNextAudioBlock);

    // ============================================================================================ yup::PositionableAudioSource

    py::class_<PositionableAudioSource, PyPositionableAudioSource<>, AudioSource> classPositionableAudioSource (m, "PositionableAudioSource");

    classPositionableAudioSource
        .def (py::init<>())
        .def ("setNextReadPosition", &PositionableAudioSource::setNextReadPosition)
        .def ("getNextReadPosition", &PositionableAudioSource::getNextReadPosition)
        .def ("getTotalLength", &PositionableAudioSource::getTotalLength)
        .def ("isLooping", &PositionableAudioSource::isLooping)
        .def ("setLooping", &PositionableAudioSource::setLooping);

    // ============================================================================================ yup::ToneGeneratorAudioSource

    py::class_<ToneGeneratorAudioSource, AudioSource> classToneGeneratorAudioSource (m, "ToneGeneratorAudioSource");

    classToneGeneratorAudioSource
        .def (py::init<>())
        .def ("setAmplitude", &ToneGeneratorAudioSource::setAmplitude)
        .def ("setFrequency", &ToneGeneratorAudioSource::setFrequency);

    // ============================================================================================ yup::MixerAudioSource

    py::class_<MixerAudioSource, AudioSource> classMixerAudioSource (m, "MixerAudioSource");

    classMixerAudioSource
        .def (py::init<>())
        .def ("addInputSource", &MixerAudioSource::addInputSource, "newInput"_a, "deleteWhenRemoved"_a)
        .def ("removeInputSource", &MixerAudioSource::removeInputSource)
        .def ("removeAllInputs", &MixerAudioSource::removeAllInputs);

    // ============================================================================================ yup::SynthesiserSound

    py::class_<SynthesiserSound, PySynthesiserSound, ReferenceCountedObjectPtr<SynthesiserSound>> classSynthesiserSound (m, "SynthesiserSound");

    classSynthesiserSound
        .def (py::init<>())
        .def ("appliesToNote", &SynthesiserSound::appliesToNote)
        .def ("appliesToChannel", &SynthesiserSound::appliesToChannel);

    // ============================================================================================ yup::SynthesiserVoice

    py::class_<SynthesiserVoice, PySynthesiserVoice> classSynthesiserVoice (m, "SynthesiserVoice");

    classSynthesiserVoice
        .def (py::init<>())
        .def ("canPlaySound", &SynthesiserVoice::canPlaySound)
        .def ("startNote", &SynthesiserVoice::startNote)
        .def ("stopNote", &SynthesiserVoice::stopNote)
        .def ("pitchWheelMoved", &SynthesiserVoice::pitchWheelMoved)
        .def ("controllerMoved", &SynthesiserVoice::controllerMoved)
        .def ("renderNextBlock", py::overload_cast<AudioBuffer<float>&, int, int> (&SynthesiserVoice::renderNextBlock))
        .def ("setCurrentPlaybackSampleRate", &SynthesiserVoice::setCurrentPlaybackSampleRate)
        .def ("isVoiceActive", &SynthesiserVoice::isVoiceActive)
        .def ("isKeyDown", &SynthesiserVoice::isKeyDown)
        .def ("isSostenutoPedalDown", &SynthesiserVoice::isSostenutoPedalDown)
        .def ("isSustainPedalDown", &SynthesiserVoice::isSustainPedalDown)
        .def ("getCurrentlyPlayingNote", &SynthesiserVoice::getCurrentlyPlayingNote)
        .def ("getCurrentlyPlayingSound", &SynthesiserVoice::getCurrentlyPlayingSound, py::return_value_policy::reference)
        .def ("getSampleRate", &SynthesiserVoice::getSampleRate);

    // ============================================================================================ yup::Synthesiser

    py::class_<Synthesiser> classSynthesiser (m, "Synthesiser");

    classSynthesiser
        .def (py::init<>())
        .def ("clearVoices", &Synthesiser::clearVoices)
        .def ("getVoice", &Synthesiser::getVoice, py::return_value_policy::reference)
        .def ("addVoice", &Synthesiser::addVoice)
        .def ("removeVoice", &Synthesiser::removeVoice)
        .def ("clearSounds", &Synthesiser::clearSounds)
        .def ("getNumSounds", &Synthesiser::getNumSounds)
        .def ("getSound", &Synthesiser::getSound, py::return_value_policy::reference)
        .def ("addSound", &Synthesiser::addSound)
        .def ("removeSound", &Synthesiser::removeSound)
        .def ("setNoteStealingEnabled", &Synthesiser::setNoteStealingEnabled)
        .def ("isNoteStealingEnabled", &Synthesiser::isNoteStealingEnabled)
        .def ("setMinimumRenderingSubdivisionSize", &Synthesiser::setMinimumRenderingSubdivisionSize)
        .def ("setCurrentPlaybackSampleRate", &Synthesiser::setCurrentPlaybackSampleRate)
        .def ("renderNextBlock", py::overload_cast<AudioBuffer<float>&, const MidiBuffer&, int, int> (&Synthesiser::renderNextBlock), "outputAudio"_a, "inputMidi"_a, "startSample"_a, "numSamples"_a)
        .def ("allNotesOff", &Synthesiser::allNotesOff);

    // ============================================================================================ yup::AudioPlayHead

    py::class_<AudioPlayHead> classAudioPlayHead (m, "AudioPlayHead");

    py::enum_<AudioPlayHead::FrameRateType> (classAudioPlayHead, "FrameRateType")
        .value ("fps23976", AudioPlayHead::FrameRateType::fps23976)
        .value ("fps24", AudioPlayHead::FrameRateType::fps24)
        .value ("fps25", AudioPlayHead::FrameRateType::fps25)
        .value ("fps2997", AudioPlayHead::FrameRateType::fps2997)
        .value ("fps2997drop", AudioPlayHead::FrameRateType::fps2997drop)
        .value ("fps30", AudioPlayHead::FrameRateType::fps30)
        .value ("fps30drop", AudioPlayHead::FrameRateType::fps30drop)
        .value ("fps60", AudioPlayHead::FrameRateType::fps60)
        .value ("fps60drop", AudioPlayHead::FrameRateType::fps60drop)
        .value ("fpsUnknown", AudioPlayHead::FrameRateType::fpsUnknown)
        .export_values();

    py::class_<AudioPlayHead::FrameRate> classAudioPlayHeadFrameRate (classAudioPlayHead, "FrameRate");

    classAudioPlayHeadFrameRate
        .def (py::init<>())
        .def (py::init<AudioPlayHead::FrameRateType>())
        .def (py::self == py::self)
        .def (py::self != py::self)
        .def ("getType", &AudioPlayHead::FrameRate::getType)
        .def ("getBaseRate", &AudioPlayHead::FrameRate::getBaseRate)
        .def ("isDrop", &AudioPlayHead::FrameRate::isDrop)
        .def ("isPullDown", &AudioPlayHead::FrameRate::isPullDown)
        .def ("getEffectiveRate", &AudioPlayHead::FrameRate::getEffectiveRate)
        .def ("withBaseRate", &AudioPlayHead::FrameRate::withBaseRate)
        .def ("withDrop", &AudioPlayHead::FrameRate::withDrop, "drop"_a = true)
        .def ("withPullDown", &AudioPlayHead::FrameRate::withPullDown, "pulldown"_a = true);

    py::class_<AudioPlayHead::TimeSignature> classAudioPlayHeadTimeSignature (classAudioPlayHead, "TimeSignature");

    classAudioPlayHeadTimeSignature
        .def (py::init<>())
        .def_readwrite ("numerator", &AudioPlayHead::TimeSignature::numerator)
        .def_readwrite ("denominator", &AudioPlayHead::TimeSignature::denominator)
        .def (py::self == py::self)
        .def (py::self != py::self);

    py::class_<AudioPlayHead::LoopPoints> classAudioPlayHeadLoopPoints (classAudioPlayHead, "LoopPoints");

    classAudioPlayHeadLoopPoints
        .def (py::init<>())
        .def_readwrite ("ppqStart", &AudioPlayHead::LoopPoints::ppqStart)
        .def_readwrite ("ppqEnd", &AudioPlayHead::LoopPoints::ppqEnd)
        .def (py::self == py::self)
        .def (py::self != py::self);

    py::class_<AudioPlayHead::PositionInfo> classAudioPlayHeadPositionInfo (classAudioPlayHead, "PositionInfo");

    classAudioPlayHeadPositionInfo
        .def (py::init<>())
        .def ("getTimeInSamples", &AudioPlayHead::PositionInfo::getTimeInSamples)
        .def ("setTimeInSamples", &AudioPlayHead::PositionInfo::setTimeInSamples)
        .def ("getTimeInSeconds", &AudioPlayHead::PositionInfo::getTimeInSeconds)
        .def ("setTimeInSeconds", &AudioPlayHead::PositionInfo::setTimeInSeconds)
        .def ("getBpm", &AudioPlayHead::PositionInfo::getBpm)
        .def ("setBpm", &AudioPlayHead::PositionInfo::setBpm)
        .def ("getTimeSignature", &AudioPlayHead::PositionInfo::getTimeSignature)
        .def ("setTimeSignature", &AudioPlayHead::PositionInfo::setTimeSignature)
        .def ("getLoopPoints", &AudioPlayHead::PositionInfo::getLoopPoints)
        .def ("setLoopPoints", &AudioPlayHead::PositionInfo::setLoopPoints)
        .def ("getBarCount", &AudioPlayHead::PositionInfo::getBarCount)
        .def ("setBarCount", &AudioPlayHead::PositionInfo::setBarCount)
        .def ("getPpqPositionOfLastBarStart", &AudioPlayHead::PositionInfo::getPpqPositionOfLastBarStart)
        .def ("setPpqPositionOfLastBarStart", &AudioPlayHead::PositionInfo::setPpqPositionOfLastBarStart)
        .def ("getFrameRate", &AudioPlayHead::PositionInfo::getFrameRate)
        .def ("setFrameRate", &AudioPlayHead::PositionInfo::setFrameRate)
        .def ("getPpqPosition", &AudioPlayHead::PositionInfo::getPpqPosition)
        .def ("setPpqPosition", &AudioPlayHead::PositionInfo::setPpqPosition)
        .def ("getEditOriginTime", &AudioPlayHead::PositionInfo::getEditOriginTime)
        .def ("setEditOriginTime", &AudioPlayHead::PositionInfo::setEditOriginTime)
        .def ("getHostTimeNs", &AudioPlayHead::PositionInfo::getHostTimeNs)
        .def ("setHostTimeNs", &AudioPlayHead::PositionInfo::setHostTimeNs)
        .def ("getContinuousTimeInSamples", &AudioPlayHead::PositionInfo::getContinuousTimeInSamples)
        .def ("setContinuousTimeInSamples", &AudioPlayHead::PositionInfo::setContinuousTimeInSamples)
        .def ("getIsPlaying", &AudioPlayHead::PositionInfo::getIsPlaying)
        .def ("setIsPlaying", &AudioPlayHead::PositionInfo::setIsPlaying)
        .def ("getIsRecording", &AudioPlayHead::PositionInfo::getIsRecording)
        .def ("setIsRecording", &AudioPlayHead::PositionInfo::setIsRecording)
        .def ("getIsLooping", &AudioPlayHead::PositionInfo::getIsLooping)
        .def ("setIsLooping", &AudioPlayHead::PositionInfo::setIsLooping)
        .def (py::self == py::self)
        .def (py::self != py::self);

    classAudioPlayHead
        .def ("getPosition", &AudioPlayHead::getPosition)
        .def ("canControlTransport", &AudioPlayHead::canControlTransport)
        .def ("transportPlay", &AudioPlayHead::transportPlay)
        .def ("transportRecord", &AudioPlayHead::transportRecord)
        .def ("transportRewind", &AudioPlayHead::transportRewind);

    // clang-format on
}

} // namespace yup::Bindings
