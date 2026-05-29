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

namespace yup
{

//==============================================================================

/**
    A builder class for constructing AudioProcessorParameter instances using a fluent-style API.

    This class simplifies the creation of AudioProcessorParameter objects by allowing you
    to configure each aspect step-by-step, including type, range, mapping functions,
    string conversion, and optional smoothing.

    Example:

    @code
    auto gainParam = AudioParameterBuilder{}
        .withID ("gain")
        .withName ("Gain")
        .withRange (0.0f, 1.0f)
        .withDefault (0.5f)
        .withSmoothing (100.0f)
        .build();
    @endcode

    @see AudioProcessorParameter
*/
class AudioParameterBuilder
{
public:
    /** Constructs a new AudioParameterBuilder. */
    AudioParameterBuilder();

    /** Sets the parameter ID (used in the state tree and automation). */
    AudioParameterBuilder& withID (const String& paramID);

    /** Sets the parameter display name. */
    AudioParameterBuilder& withName (const String& paramName);

    /**
        Sets the stable host-facing automation ID.

        Use this for plugins that need automation compatibility when parameters are
        reordered or when reserved parameter slots are kept for future expansion.
        Once released, an ID should never be reused for a different parameter.
        Values must be less than or equal to AudioParameter::maximumHostParameterID.
    */
    AudioParameterBuilder& withHostID (uint32 hostParameterID);

    /**
        Sets the parameter's value range.

        @param min The minimum allowed value.
        @param max The maximum allowed value.
    */
    AudioParameterBuilder& withRange (float minValue, float maxValue);

    /**
        Sets the parameter's value range.

        @param valueRange The allowed value range.
    */
    AudioParameterBuilder& withRange (NormalisableRange<float> valueRange);

    /**
        Sets the parameter's default value.

        @param defaultVal The default value.
    */
    AudioParameterBuilder& withDefault (float defaultValue);

    /** Sets the value-to-string display conversion function. */
    AudioParameterBuilder& withValueToString (AudioParameter::ValueToString fn);

    /** Sets the string-to-value parsing function. */
    AudioParameterBuilder& withStringToValue (AudioParameter::StringToValue fn);

    /** Sets the smoothing time for the parameter. */
    AudioParameterBuilder& withSmoothing (float smoothingTimeMs);

    /** Sets whether hosts may automate this parameter. */
    AudioParameterBuilder& withAutomatable (bool shouldBeAutomatable);

    /** Sets whether hosts may display but not change this parameter. */
    AudioParameterBuilder& withReadOnly (bool shouldBeReadOnly);

    /** Sets whether this parameter accepts only discrete step values. */
    AudioParameterBuilder& withStepped (bool shouldBeStepped);

    /** Sets whether this stepped parameter represents an enumerated value. */
    AudioParameterBuilder& withEnum (bool shouldBeEnum);

    /** Sets whether this parameter supports CLAP modulation events. */
    AudioParameterBuilder& withModulatable (bool shouldBeModulatable);

    /** Sets whether this parameter supports CLAP per-note modulation events. */
    AudioParameterBuilder& withPerNoteModulatable (bool shouldBePerNoteModulatable);

    /** Sets the optional host-facing module path, using "/" as a separator. */
    AudioParameterBuilder& withModulePath (const String& modulePath);

    /**
        Finalizes the builder and returns a fully constructed AudioProcessorParameter instance.

        @returns A shared_ptr to the constructed AudioProcessorParameter.
    */
    AudioParameter::Ptr build() const;

private:
    String id;
    AudioParameter::Metadata metadata;
    AudioParameter::ValueToString valueToString = nullptr;
    AudioParameter::StringToValue stringToValue = nullptr;
};

} // namespace yup
