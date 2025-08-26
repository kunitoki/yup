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

#pragma once

namespace yup
{

//==============================================================================
/**
    Layered partitioned convolution engine optimized for real-time audio processing.

    Combines multiple processing strategies for efficient convolution:
    - Direct FIR computation for early taps (low latency)
    - One or more FFT-based Overlap-Add layers with uniform partitioning per layer

    The engine uses YUP's FFTProcessor for real FFT operations and supports:
    - Arbitrary input/output block sizes with internal buffering
    - Real-time safe processing (no heap allocations during process())
    - Configurable layer hierarchy for optimal CPU/latency trade-off

    Example usage:
    @code
    PartitionedConvolver convolver;

    // Configure layers: 256 direct taps + FFT layers with hops 256, 1024, 4096
    convolver.setTypicalLayout(256, {256, 1024, 4096});

    // Prepare for processing with maximum block size (must be called before process)
    convolver.prepare(512); // Maximum 512 samples per process() call

    // Set impulse response (e.g., reverb IR)
    std::vector<float> impulseResponse = loadImpulseResponse();
    convolver.setImpulseResponse(impulseResponse);

    // In audio callback (accumulates into output):
    convolver.process(inputBuffer, outputBuffer, numSamples); // numSamples <= 512
    @endcode

    @note The process() method accumulates results into the output buffer.
          Clear the output buffer first if overwrite behavior is desired.
*/
class PartitionedConvolver
{
public:
    //==============================================================================
    /** Configuration for a single FFT-based convolution layer */
    struct LayerSpec
    {
        int hopSize; /**< Partition size L (FFT size will be 2*L) */
    };

    //==============================================================================
    /** Default constructor */
    PartitionedConvolver();

    /** Destructor */
    ~PartitionedConvolver();

    // Non-copyable but movable
    PartitionedConvolver (PartitionedConvolver&& other) noexcept;
    PartitionedConvolver& operator= (PartitionedConvolver&& other) noexcept;

    //==============================================================================
    /**
        Configure the convolution layers before setting the impulse response.

        @param directFIRTaps  Number of early taps to process with direct FIR (for low latency)
        @param layers         Vector of layer specifications with increasing hop sizes
                             (e.g., {{256}, {1024}, {4096}} for 256→1024→4096 progression)
    */
    void configureLayers (std::size_t directFIRTaps, const std::vector<LayerSpec>& layers);

    /**
        Convenience method to set a typical late-reverb configuration.

        @param directTaps  Number of direct FIR taps for early reflections
        @param hops        Vector of hop sizes for FFT layers (geometrically increasing recommended)
    */
    void setTypicalLayout (std::size_t directTaps, const std::vector<int>& hops);

    //==============================================================================
    /** Impulse response loading options. */
    struct IRLoadOptions
    {
        IRLoadOptions()
            : normalize (true)
            , headroomDb (-12.0f)
        {
        }

        bool normalize;
        float headroomDb;
    };

    /**
        Set the impulse response for convolution.

        @param impulseResponse  Pointer to impulse response samples
        @param length          Number of samples in the impulse response

        @note This method is not real-time safe and should be called during initialization
              or from a background thread when audio is paused.
    */
    void setImpulseResponse (const float* impulseResponse, std::size_t length, const IRLoadOptions& options = {});

    /**
        Set the impulse response from a vector.

        @param impulseResponse  Vector containing impulse response samples
    */
    void setImpulseResponse (const std::vector<float>& impulseResponse, const IRLoadOptions& options = {});

    //==============================================================================
    /**
        Prepare the convolver for processing with a specific maximum block size.

        @param maxBlockSize  Maximum number of samples that will be passed to process()

        @note This method is not real-time safe and should be called during initialization
              or when audio processing is paused. It pre-allocates all internal buffers
              to handle the specified block size without further allocations.
    */
    void prepare (std::size_t maxBlockSize);

    /**
        Process audio samples through the convolver.

        @param input       Input audio buffer
        @param output      Output audio buffer (results are accumulated)
        @param numSamples  Number of samples to process

        @note Results are accumulated into the output buffer. Clear it first if needed.
        @note This method is real-time safe with no heap allocations.
    */
    void process (const float* input, float* output, std::size_t numSamples);

    /**
        Reset all internal processing state (clears delay lines, overlap buffers).
        Impulse response partitions are preserved.
    */
    void reset();

private:
    //==============================================================================
    class DirectFIR;
    class FFTLayer;
    class CircularBuffer;
    class Impl;

    std::unique_ptr<Impl> pImpl;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PartitionedConvolver)
};

} // namespace yup
