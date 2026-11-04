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

#include <yup_audio_processors/yup_audio_processors.h>

#include <functional>
#include <vector>

//==============================================================================
/** Renders notes and parameter automation through a processor faster than real time. */
namespace RetraceAudioRender
{
/** The longest render, in seconds. */
constexpr double maxSeconds = 120.0;

/** A note to play, timed in seconds from the start of the render. */
struct Note
{
    int note = 60;
    float velocity = 0.8f; /**< 0 to 1. */
    double start = 0.0;
    double duration = 1.0;
    int channel = 1;       /**< 1 to 16. */
};

/** A parameter moving to a value at a time, in seconds from the start of the render. */
struct Automation
{
    int parameterIndex = 0;     /**< The index in AudioProcessor::getParameters(). */
    double time = 0.0;
    float normalizedValue = 0.0f;
};

/** What to render. */
struct Request
{
    std::vector<Note> notes;
    std::vector<Automation> automation;
    double sampleRate = 48000.0;
    double length = 0.0; /**< The length in seconds, or 0 to stop @c tail seconds after the last note ends. */
    double tail = 2.0;
};

/** Returns how long a request renders, in seconds, within maxSeconds. */
double getLength (const Request& request);

/** Prepares a processor that nothing else plays, renders the request through it and
    returns the stereo output, aligned for the processor's latency.

    Runs on any thread. @p shouldStop is polled between blocks and cancels the render
    when it returns true.
*/
yup::ResultValue<yup::AudioBuffer<float>> render (yup::AudioProcessor& processor,
                                                  const Request& request,
                                                  const std::function<bool()>& shouldStop);
} // namespace RetraceAudioRender
