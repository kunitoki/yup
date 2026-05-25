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
/** Engine interface for the TimeStretchProcessor.

    This abstract class defines the interface that all time-stretching backends
    must implement to be used within the TimeStretchProcessor. It provides methods
    for preparing the engine with processing specifications, resetting state,
    setting input position and parameters, providing input audio data, and
    processing audio blocks.

    Each backend implementation will
    inherit from this interface and provide concrete implementations of these
    methods according to their specific algorithms and requirements.

    @see TimeStretchProcessor
*/
class TimeStretchProcessor::Engine
{
public:
    /** Destructor. */
    virtual ~Engine() = default;

    /** Prepares the engine with the given processing specifications. */
    virtual Result prepare (const TimeStretchProcessor::ProcessSpec& spec) = 0;

    /** Resets the engine to its initial state. */
    virtual void reset() = 0;

    /** Sets the input position for the engine. */
    virtual void setInputPosition (int64 newInputPosition) = 0;

    /** Sets the parameters for the engine. */
    virtual void setParameters (const TimeStretchProcessor::Parameters& parameters) = 0;

    /** Sets the input provider for the engine. */
    virtual void setInputProvider (TimeStretchProcessor::InputProvider provider) = 0;

    /** Returns the maximum number of input frames the engine can process at once. */
    virtual int getMaxInputFrameCount() const = 0;

    /** Processes the input audio and produces the output audio. */
    virtual int process (const float* const* inputChannels,
                         int inputFrameCount,
                         float* const* outputChannels,
                         int outputFrameCount) = 0;

    /** Returns the name of the backend. */
    virtual String getBackendName() const = 0;

    /** Returns the latency of the engine in frames. */
    virtual double getLatencyInFrames() const = 0;
};

} // namespace yup
