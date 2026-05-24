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

#pragma once

namespace yup
{

//==============================================================================
/**
    Flexible time-stretching and pitch-shifting processor.

    This class provides a unified, backend-agnostic interface for time-stretching
    and pitch-shifting. Internally it can use different algorithms, allowing the
    backend to be swapped without changing user code. The API is block-based and
    operates on non-interleaved float audio channels.

    The caller controls the output length by specifying the desired output frame
    count per process call. The time ratio can be expressed explicitly via that
    output size, or derived from the stored timeRatio using getExpectedOutputFrameCount().

    Example usage:
    @code
    TimeStretchProcessor processor;
    TimeStretchProcessor::ProcessSpec spec;
    spec.inputSampleRate = 48000.0;
    spec.outputSampleRate = 48000.0;
    spec.maximumBlockSize = 512;
    spec.numChannels = 2;

    processor.prepare (spec);
    processor.setTimeRatio (1.5);
    processor.setPitchRatio (1.0);

    const int outputFrames = processor.getExpectedOutputFrameCount (512);
    processor.process (inputPointers, 512, outputPointers, outputFrames);
    @endcode
*/
class TimeStretchProcessor
{
public:
    //==============================================================================
    /** Input provider for granular processing (begin frame, num frames). */
    using InputProvider = std::function<void (int64 beginFrame,
                                              int numFrames,
                                              float* const* destChannels,
                                              int channelStride,
                                              int& muteHead,
                                              int& muteTail)>;

    //==============================================================================
    /** Backends supported by the time-stretch processor. */
    enum class Backend
    {
        automatic,  /**< Automatically select the best available backend. */
        timeDomain, /**< Use the built-in time-domain backend for tempo changes without independent pitch shifting. */
        bungee      /**< Use the Bungee backend if available. */
    };

    //==============================================================================
    /**
        Processing configuration for preparing the processor.

        Both sample rates are expressed in Hz. If outputSampleRate is 0, the
        inputSampleRate value is used for both directions.
    */
    struct ProcessSpec
    {
        double inputSampleRate = 0.0;
        double outputSampleRate = 0.0;
        int maximumBlockSize = 0;
        int numChannels = 0;
    };

    //==============================================================================
    /**
        High-level parameters for time-stretching and pitch-shifting.

        timeRatio is the output length divided by the input length, where 1.0
        means no time stretch, values > 1.0 slow down, and values < 1.0 speed up.
        pitchRatio is a frequency multiplier, where 1.0 means no pitch shift.
    */
    struct Parameters
    {
        double timeRatio = 1.0;
        double pitchRatio = 1.0;
    };

    //==============================================================================
    /** Constructs a new processor with default parameters. */
    TimeStretchProcessor();

    /** Destructor. */
    ~TimeStretchProcessor();

    /** Move constructor. */
    TimeStretchProcessor (TimeStretchProcessor&& other) noexcept;

    /** Move assignment operator. */
    TimeStretchProcessor& operator= (TimeStretchProcessor&& other) noexcept;

    //==============================================================================
    /**
        Prepare the processor for the given configuration.

        This allocates internal buffers and selects a backend. The method is not
        real-time safe and should be called during initialization.

        @param spec            Processing specification (rates, channels, block size).
        @param preferredBackend Backend to use, or automatic to pick the best available.
        @returns Result::ok() on success, or a failure with a descriptive message.
    */
    Result prepare (const ProcessSpec& spec, Backend preferredBackend = Backend::automatic);

    /**
        Reset the internal state of the processor.

        This clears the algorithm state and latency. The method is not guaranteed
        to be real-time safe because some backends may reallocate internal buffers.
    */
    void reset();

    /** Returns true if the processor has been prepared successfully. */
    bool isPrepared() const noexcept { return prepared; }

    //==============================================================================
    /** Set a custom input provider for granular backends. */
    void setInputProvider (InputProvider provider);

    //==============================================================================
    /** Set both time and pitch parameters at once. */
    void setParameters (const Parameters& newParameters);

    /** Retrieve the current time and pitch parameters. */
    Parameters getParameters() const noexcept { return parameters; }

    /** Set the time stretch ratio (output length / input length). */
    void setTimeRatio (double newTimeRatio);

    /** Set the pitch ratio (frequency multiplier). */
    void setPitchRatio (double newPitchRatio);

    /** Returns the current time ratio. */
    double getTimeRatio() const noexcept { return parameters.timeRatio; }

    /** Returns the current pitch ratio. */
    double getPitchRatio() const noexcept { return parameters.pitchRatio; }

    //==============================================================================
    /** Seek to a new input position. */
    void setInputPosition (int64 newInputPosition);

    /** Returns the maximum number of input frames that might be requested by the InputProvider. */
    int getMaxInputFrameCount() const;

    /**
        Estimate the output frame count for a given input frame count.

        @param inputFrameCount Number of input frames for the next process call.
        @returns The expected output frame count based on timeRatio.
    */
    int getExpectedOutputFrameCount (int inputFrameCount) const noexcept;

    /**
        Returns the current processing latency in input frames.

        @returns Latency in frames, or 0 if not available.
    */
    double getLatencyInFrames() const;

    /**
        Process a block of audio using an explicit output frame count.

        The output buffer must contain at least outputFrameCount frames for each
        channel. The method returns the number of frames written, which may differ
        by +/-1 depending on the backend's rounding behavior.

        @param inputChannels     Array of input channel pointers (non-interleaved).
        @param inputFrameCount   Number of input frames.
        @param outputChannels    Array of output channel pointers (non-interleaved).
        @param outputFrameCount  Desired output frame count for this block.
        @returns ResultValue<int> containing the number of frames rendered.
    */
    ResultValue<int> process (const float* const* inputChannels,
                              int inputFrameCount,
                              float* const* outputChannels,
                              int outputFrameCount);

    /**
        Process a block of audio using AudioBuffer wrappers.

        This variant processes all samples in the input buffer and writes to the
        beginning of the output buffer.

        @param input            Input buffer (non-interleaved).
        @param output           Output buffer (non-interleaved).
        @param outputFrameCount Desired output frame count for this block.
        @returns ResultValue<int> containing the number of frames rendered.
    */
    ResultValue<int> process (const AudioBuffer<float>& input,
                              AudioBuffer<float>& output,
                              int outputFrameCount);

    /**
        Process a block using the stored timeRatio to compute output frames.

        @param inputChannels      Array of input channel pointers.
        @param inputFrameCount    Number of input frames.
        @param outputChannels     Array of output channel pointers.
        @param outputFrameCapacity Maximum available output frames.
        @returns ResultValue<int> containing the number of frames rendered.
    */
    ResultValue<int> processUsingTimeRatio (const float* const* inputChannels,
                                            int inputFrameCount,
                                            float* const* outputChannels,
                                            int outputFrameCapacity);

    /**
        Process a buffer using the stored timeRatio.

        @param input   Input buffer.
        @param output  Output buffer.
        @returns ResultValue<int> containing the number of frames rendered.
    */
    ResultValue<int> processUsingTimeRatio (const AudioBuffer<float>& input,
                                            AudioBuffer<float>& output);

    //==============================================================================
    /** Returns the backend currently selected for processing. */
    Backend getBackend() const noexcept { return backend; }

    /** Returns a human-readable name of the active backend. */
    String getBackendName() const;

    /** Returns true if the requested backend is available at compile time. */
    static bool isBackendAvailable (Backend backend) noexcept;

    /** Returns the list of backends that are available at compile time. */
    static std::vector<Backend> getAvailableBackends();

    /**
        Switch to a different backend.

        If the processor is already prepared, this will reinitialize the backend
        using the existing ProcessSpec. Any internal state is discarded.

        @param newBackend The backend to switch to.
        @returns Result::ok() on success, or a failure if the backend is unavailable.
    */
    Result setBackend (Backend newBackend);

    //==============================================================================
#ifndef DOXYGEN
    /** @internal */
    class Engine;
#endif

private:
    //==============================================================================
    static Backend resolveBackend (Backend preferredBackend) noexcept;
    static Result validateSpec (const ProcessSpec& spec, ProcessSpec& validatedSpec);

    Result rebuildEngine (Backend preferredBackend);

    //==============================================================================
    ProcessSpec spec;
    Parameters parameters;
    Backend backend = Backend::automatic;
    bool prepared = false;
    InputProvider inputProvider;

    std::unique_ptr<Engine> engine;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TimeStretchProcessor)
};

} // namespace yup
