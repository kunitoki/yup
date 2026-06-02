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
*/

namespace yup
{

//==============================================================================

/**
    A flexible, thread-safe parameter class with support for custom mapping,
    string conversion, smoothing, and different parameter types (linear, log, dB, enum, etc).

    Use AudioParameterBuilder to construct instances of this class.

    @see AudioParameterBuilder
*/
class AudioParameter : public ReferenceCountedObject
{
public:
    //==============================================================================

    /** A pointer to an AudioParameter. */
    using Ptr = ReferenceCountedObjectPtr<AudioParameter>;

    //==============================================================================

    /** A function that converts a real value to a string. */
    using ValueToString = std::function<String (float)>;

    /** A function that converts a string to a real value. */
    using StringToValue = std::function<float (const String&)>;

    /** Sentinel used when a parameter does not provide an explicit host-facing ID. */
    static constexpr uint32 invalidHostParameterID = 0xffffffffu;

    /**
        Highest host-facing parameter ID that is portable across VST3, AUv2, and CLAP.

        VST3 reserves the upper half of the 32-bit parameter ID range for hosts, so
        explicit IDs used by YUP plugins must stay in the lower half.
    */
    static constexpr uint32 maximumHostParameterID = 0x7fffffffu;

    /** Metadata used by processors and plugin wrappers when describing this parameter. */
    struct Metadata
    {
    private:
        /** Bit flags used to store optional parameter capabilities compactly. */
        enum Flag : uint8
        {
            automatableFlag = 1 << 0,
            readOnlyFlag = 1 << 1,
            steppedFlag = 1 << 2,
            enumeratedFlag = 1 << 3,
            modulatableFlag = 1 << 4,
            perNoteModulatableFlag = 1 << 5,
            smoothingEnabledFlag = 1 << 6
        };

        /** Returns true if the supplied flag is set. */
        bool isFlagSet (Flag flag) const noexcept { return (flags & flag) != 0; }

        /** Enables or disables the supplied flag. */
        void setFlag (Flag flag, bool shouldBeSet) noexcept
        {
            if (shouldBeSet)
                flags = static_cast<uint8> (flags | flag);
            else
                flags = static_cast<uint8> (flags & ~static_cast<uint8> (flag));
        }

    public:
        /** Returns true when hosts may automate this parameter. */
        bool isAutomatable() const noexcept { return isFlagSet (automatableFlag); }

        /** Sets whether hosts may automate this parameter. */
        void setAutomatable (bool shouldBeAutomatable) noexcept { setFlag (automatableFlag, shouldBeAutomatable); }

        /** Returns true when hosts may display but not change this parameter. */
        bool isReadOnly() const noexcept { return isFlagSet (readOnlyFlag); }

        /** Sets whether hosts may display but not change this parameter. */
        void setReadOnly (bool shouldBeReadOnly) noexcept { setFlag (readOnlyFlag, shouldBeReadOnly); }

        /** Returns true when this parameter accepts only discrete step values. */
        bool isStepped() const noexcept { return isFlagSet (steppedFlag); }

        /** Sets whether this parameter accepts only discrete step values. */
        void setStepped (bool shouldBeStepped) noexcept { setFlag (steppedFlag, shouldBeStepped); }

        /** Returns true when this stepped parameter represents an enumerated value. */
        bool isEnum() const noexcept { return isFlagSet (enumeratedFlag); }

        /** Sets whether this stepped parameter represents an enumerated value. */
        void setEnum (bool shouldBeEnum) noexcept { setFlag (enumeratedFlag, shouldBeEnum); }

        /** Returns true when this parameter supports CLAP modulation events. */
        bool isModulatable() const noexcept { return isFlagSet (modulatableFlag); }

        /** Sets whether this parameter supports CLAP modulation events. */
        void setModulatable (bool shouldBeModulatable) noexcept { setFlag (modulatableFlag, shouldBeModulatable); }

        /** Returns true when this parameter supports CLAP per-note modulation events. */
        bool isPerNoteModulatable() const noexcept { return isFlagSet (perNoteModulatableFlag); }

        /** Sets whether this parameter supports CLAP per-note modulation events. */
        void setPerNoteModulatable (bool shouldBePerNoteModulatable) noexcept { setFlag (perNoteModulatableFlag, shouldBePerNoteModulatable); }

        /** Returns true if smoothing is enabled. */
        bool isSmoothingEnabled() const noexcept { return isFlagSet (smoothingEnabledFlag); }

        /** Sets whether smoothing is enabled. */
        void setSmoothingEnabled (bool shouldBeEnabled) noexcept { setFlag (smoothingEnabledFlag, shouldBeEnabled); }

        /** The parameter display name. */
        String name;

        /** Optional stable host-facing automation ID. */
        uint32 hostParameterID = invalidHostParameterID;

        /** The parameter value range. */
        NormalisableRange<float> valueRange = { 0.0f, 1.0f };

        /** The default real value. */
        float defaultValue = 0.0f;

        /** The smoothing time in milliseconds. */
        float smoothingTimeMs = 0.0f;

        /** Optional host-facing module path, using "/" as a separator. */
        String modulePath;

    private:
        uint8 flags = automatableFlag;
    };

    //==============================================================================

    /**
        Constructs an AudioParameter instance.

        @param id               The parameter ID used in processor state.
        @param metadata         The parameter display, range, default, smoothing,
                                and host-facing metadata.
        @param valueToString    Converts real values to display strings.
        @param stringToValue    Parses display strings back to real values.
    */
    AudioParameter (const String& id,
                    Metadata metadata,
                    ValueToString valueToString = nullptr,
                    StringToValue stringToValue = nullptr);

    /** Destructor. */
    ~AudioParameter();

    //==============================================================================

    /** Returns the parameter ID. */
    const String& getID() const { return paramID; }

    /** Returns the parameter name. */
    const String& getName() const { return metadata.name; }

    /**
        Returns true when this parameter has an explicit host-facing automation ID.

        Explicit IDs should be stable forever once a plugin version ships. Do not
        reuse an old ID for a different parameter, even if the original parameter is
        removed from the plugin UI.
    */
    bool hasExplicitHostParameterID() const noexcept
    {
        return metadata.hostParameterID != invalidHostParameterID;
    }

    /**
        Returns the host-facing automation ID for this parameter.

        If no explicit ID was provided, this returns the parameter's index assigned
        by AudioProcessor::addParameter(), preserving the legacy index-based mapping.
    */
    uint32 getHostParameterID() const noexcept
    {
        return hasExplicitHostParameterID()
                 ? metadata.hostParameterID
                 : (paramIndex >= 0 ? static_cast<uint32> (paramIndex) : invalidHostParameterID);
    }

    //==============================================================================

    /** Returns the index of this parameter in its container. */
    int getIndexInContainer() const { return paramIndex; }

    /** Sets the index of this parameter in its container. */
    void setIndexInContainer (int newIndex) { paramIndex = newIndex; }

    //==============================================================================

    /** Returns the minimum value. */
    float getMinimumValue() const { return metadata.valueRange.start; }

    /** Returns the maximum value. */
    float getMaximumValue() const { return metadata.valueRange.end; }

    /** Returns the default value. */
    float getDefaultValue() const { return metadata.defaultValue; }

    /** Returns true when hosts may automate this parameter. */
    bool isAutomatable() const noexcept { return metadata.isAutomatable(); }

    /** Returns true when hosts may display but not change this parameter. */
    bool isReadOnly() const noexcept { return metadata.isReadOnly(); }

    /** Returns true when this parameter accepts only discrete step values. */
    bool isStepped() const noexcept { return metadata.isStepped() || metadata.valueRange.interval > 0.0f; }

    /** Returns the number of discrete steps, or 0 for continuous parameters. */
    int getNumSteps() const noexcept
    {
        if (! isStepped())
            return 0;

        if (metadata.valueRange.interval <= 0.0f || metadata.valueRange.end <= metadata.valueRange.start)
            return 1;

        return jmax (1, static_cast<int> (std::floor (((metadata.valueRange.end - metadata.valueRange.start) / metadata.valueRange.interval) + 0.5f)));
    }

    /** Returns true when this stepped parameter represents an enumerated value. */
    bool isEnum() const noexcept { return metadata.isEnum(); }

    /** Returns true when this parameter supports CLAP modulation events. */
    bool isModulatable() const noexcept { return metadata.isModulatable(); }

    /** Returns true when this parameter supports CLAP per-note modulation events. */
    bool isPerNoteModulatable() const noexcept { return metadata.isPerNoteModulatable(); }

    /** Returns the module path of this parameter. */
    String getModulePath() const { return metadata.modulePath; }

    //==============================================================================

    /** Begins a change gesture for this parameter.
    
        Gestures can be nested, but each beginChangeGesture() call must be balanced with a corresponding endChangeGesture() call.

        Hosts typically use change gestures to group multiple parameter changes into a single undo step and to indicate when to update automation envelopes.
    */
    void beginChangeGesture();

    /** Ends a change gesture for this parameter.
    
        Gestures can be nested, but each endChangeGesture() call must be balanced with a corresponding beginChangeGesture() call.

        Hosts typically use change gestures to group multiple parameter changes into a single undo step and to indicate when to update automation envelopes.
    */
    void endChangeGesture();

    /** Returns true if a change gesture is currently being performed. */
    bool isPerformingChangeGesture() const { return isInsideGesture.load() != 0; }

    //==============================================================================

    /**
        Sets the real (un-normalized) parameter value and notifies the host.

        @param value The new real value.
    */
    void setValueNotifyingHost (float value);

    /**
        Sets the real (un-normalized) parameter value.

        @param newValue The new real value.
    */
    void setValue (float newValue)
    {
        currentValue.store (metadata.valueRange.snapToLegalValue (newValue));
    }

    /** Gets the real (un-normalized) parameter value. */
    float getValue() const { return currentValue.load(); }

    /**
        Sets the normalized [0..1] value.

        @param normalizedValue The new normalized value.
    */
    void setNormalizedValue (float normalizedValue)
    {
        setValue (convertToDenormalizedValue (normalizedValue));
    }

    /** Gets the normalized [0..1] value. */
    float getNormalizedValue() const
    {
        return convertToNormalizedValue (getValue());
    }

    //==============================================================================

    /** Converts a real value to a normalized [0..1] value.
    
        @param denormalizedValue The real value to convert.
    */
    float convertToNormalizedValue (float denormalizedValue) const
    {
        return metadata.valueRange.convertTo0to1 (denormalizedValue);
    }

    /** Converts a normalized [0..1] value to a real value.
    
        @param normalizedValue The normalized value to convert.
    */
    float convertToDenormalizedValue (float normalizedValue) const
    {
        return metadata.valueRange.convertFrom0to1 (normalizedValue);
    }

    //==============================================================================

    /** Converts a real value to its display string. */
    String toString() const { return valueToString (getValue()); }

    /** Parses a string into a real parameter value. */
    void fromString (const String& string) { setValue (stringToValue (string)); }

    //==============================================================================

    /** Converts a real value to its display string. */
    String convertToString (float value) const { return valueToString (value); }

    /** Parses a string into a real parameter value. */
    float convertFromString (const String& string) const { return stringToValue (string); }

    //==============================================================================

    /** Returns true if smoothing is enabled. */
    bool isSmoothingEnabled() const { return metadata.isSmoothingEnabled(); }

    /** Returns the smoothing time in milliseconds. */
    float getSmoothingTimeMs() const { return metadata.smoothingTimeMs; }

    //==============================================================================

    /** A listener for parameter changes. */
    class Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when the parameter value changes. */
        virtual void parameterValueChanged (const AudioParameter::Ptr& parameter, int indexInContainer) = 0;

        /** Called when a gesture begins. */
        virtual void parameterGestureBegin (const AudioParameter::Ptr& parameter, int indexInContainer) = 0;

        /** Called when a gesture ends. */
        virtual void parameterGestureEnd (const AudioParameter::Ptr& parameter, int indexInContainer) = 0;
    };

    /** Adds a listener to the parameter. */
    void addListener (Listener* listener);

    /** Removes a listener from the parameter. */
    void removeListener (Listener* listener);

private:
    using ListenersType = ListenerList<Listener, Array<Listener*, CriticalSection>>;

    String paramID;
    int paramIndex = -1;
    std::atomic<float> currentValue = 0.0f;
    Metadata metadata;
    ListenersType listeners;
    ValueToString valueToString = nullptr;
    StringToValue stringToValue = nullptr;
    std::atomic<int> isInsideGesture = 0;
};

} // namespace yup
