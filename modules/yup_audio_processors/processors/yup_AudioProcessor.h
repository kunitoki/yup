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

class AudioProcessorEditor;

//==============================================================================
/**
    Identifies the plugin format wrapper that is hosting the processors of the current binary.

    @see AudioProcessor::getWrapperType
*/
enum class AudioPluginWrapperType
{
    unknown,    /**< Not loaded through a plugin wrapper (for example, a host or a test). */
    clap,       /**< CLAP plugin. */
    vst3,       /**< VST3 plugin. */
    au,         /**< Audio Unit v2 plugin. */
    auv3,       /**< Audio Unit v3 app extension. */
    aax,        /**< AAX plugin. */
    lv2,        /**< LV2 plugin. */
    standalone  /**< Standalone plugin application. */
};

//==============================================================================
/**
    Base class for all audio processors.

    The AudioProcessor class is the base class for all audio processing modules in the framework.
    It provides a common interface for processing audio and MIDI data, managing parameters, and
    communicating with hosts.

    @see AudioProcessorEditor
*/
class YUP_API AudioProcessor : public DomainProcessor<AudioProcessContext, AudioSpec>
{
    using BaseDomainProcessor = DomainProcessor<AudioProcessContext, AudioSpec>;

public:
    //==============================================================================
    /** Constructs an AudioProcessor. */
    AudioProcessor (StringRef name, AudioBusLayout busLayout);

    /** Destructs an AudioProcessor. */
    ~AudioProcessor() override;

    //==============================================================================
    /** Returns the bus layout. */
    const AudioBusLayout& getBusLayout() const noexcept { return busLayout; }

    /** Returns the number of audio outputs. */
    int getNumAudioOutputs() const;

    /** Returns the number of audio inputs. */
    int getNumAudioInputs() const;

    //==============================================================================
    /** Returns true if the processor accepts MIDI input. */
    virtual bool acceptsMidi() const noexcept;

    /** Returns true if the processor produces MIDI output. */
    virtual bool producesMidi() const noexcept;

    //==============================================================================
    /** Returns the number of simultaneous voices this processor can produce.
        Returns 0 for effects and MIDI-only processors. Override in instruments. */
    virtual int getNumVoices() const { return 0; }

    //==============================================================================
    /** Returns true if the processor has an editor. */
    virtual bool hasEditor() const = 0;

    /** Creates an editor for the processor. */
    virtual AudioProcessorEditor* createEditor();

    //==============================================================================
    /** Returns the plugin format wrapper this binary was built and loaded as.

        Plugin code is compiled once and shared by all formats, so use this instead of
        format-specific preprocessor macros. The value is set by the wrapper before the
        processor is created, so it can already be queried from processor constructors.
    */
    static AudioPluginWrapperType getWrapperType() noexcept;

    /** Returns a lowercase name for a wrapper type (for example "vst3" or "standalone"). */
    static StringRef getWrapperTypeName (AudioPluginWrapperType wrapperType) noexcept;

    /** @internal Used by plugin wrappers to declare the format of the current binary. */
    static void setWrapperType (AudioPluginWrapperType wrapperType) noexcept;

    //==============================================================================
    /** @internal Used by plugin wrappers. */
    void setPlaybackConfiguration (float sampleRate, int samplesPerBlock) override;

private:
    AudioBusLayout busLayout;
};

} // namespace yup
