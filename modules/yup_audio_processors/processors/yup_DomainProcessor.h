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
    Base class for all audio processors.

    The AudioProcessor class is the base class for all audio processing modules in the framework.
    It provides a common interface for processing audio and MIDI data, managing parameters, and
    communicating with hosts.

    @see AudioProcessorEditor
*/
template <
    template <typename FloatType> class DomainProcessContext,
    class DomainSpecification>
class YUP_API DomainProcessor : public AudioProcessorBase
{
public:
    //==============================================================================
    /** The floating-point precision used for processBlock() calls. */
    enum class ProcessingPrecision
    {
        singlePrecision,
        doublePrecision
    };

    //==============================================================================
    /** Constructs a DomainProcessor. */
    DomainProcessor (StringRef name)
        : AudioProcessorBase (name)
    {
    }

    /** Destructs a DomainProcessor. */
    ~DomainProcessor() override = default;

    //==============================================================================
    /** Prepares the processor for playback.

        The DomainSpecification provides all necessary information for initialization.
        Subclasses do not need to call the base class.
    */
    virtual void prepareToPlay (const DomainSpecification& spec) = 0;

    /** Releases resources. */
    virtual void releaseResources() = 0;

    /**
        Primary single-precision processing entry point.

        Override this to process a block of audio and MIDI. The context provides
        sample-accurate parameter automation via @c context.params and the transport
        state via @c context.playHead when available.

        The base-class implementation asserts false so unoverridden processors are
        caught at runtime in debug builds.

        @param context  All per-block inputs: audio, MIDI, parameter changes, and position.
    */
    virtual void processBlock (DomainProcessContext<float>& context) = 0;

    /**
        Double-precision processing entry point.

        Override this and return true from supportsDoublePrecisionProcessing() to
        support 64-bit audio. The default implementation does nothing.

        @param context  All per-block inputs with double-precision audio.
    */
    virtual void processBlock (DomainProcessContext<double>& context) { ignoreUnused (context); }

    /**
        Called by plugin wrappers when the processor is bypassed (single-precision).

        The default implementation routes inputs to outputs, or clears extra outputs.

        @param context  All per-block inputs.
    */
    virtual void processBlockBypassed (DomainProcessContext<float>& context) { ignoreUnused (context); }

    /**
        Called by plugin wrappers when the processor is bypassed (double-precision).

        The default implementation routes inputs to outputs, or clears extra outputs.

        @param context  All per-block inputs.
    */
    virtual void processBlockBypassed (DomainProcessContext<double>& context) { ignoreUnused (context); }

    /** Flushes the processor. */
    virtual void flush() {}

    //==============================================================================
    /** Returns true if this processor implements the double-precision processBlock(). */
    virtual bool supportsDoublePrecisionProcessing() const { return false; }

    /** Sets the preferred processing precision for future processBlock() calls. */
    void setProcessingPrecision (ProcessingPrecision precision)
    {
        if (precision == ProcessingPrecision::doublePrecision && ! supportsDoublePrecisionProcessing())
        {
            jassertfalse;
            processingPrecision = ProcessingPrecision::singlePrecision;
            return;
        }

        processingPrecision = precision;
    }

    /** Returns the current processing precision. */
    ProcessingPrecision getProcessingPrecision() const noexcept { return processingPrecision; }

    /** Returns true when the current processing precision is double precision. */
    bool isUsingDoublePrecision() const noexcept { return processingPrecision == ProcessingPrecision::doublePrecision; }

private:
    ProcessingPrecision processingPrecision = ProcessingPrecision::singlePrecision;
};

} // namespace yup
