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

#include "RetraceAudioRender.h"

#include <algorithm>
#include <cmath>

namespace RetraceAudioRender
{
namespace
{
constexpr int blockSize = 512;

struct MidiEvent
{
    yup::int64 sample = 0;
    yup::MidiMessage message;
};

struct ParameterEvent
{
    yup::int64 sample = 0;
    Automation automation;
};

yup::int64 toSamples (double seconds, double sampleRate)
{
    return static_cast<yup::int64> (std::llround (yup::jmax (0.0, seconds) * sampleRate));
}
} // namespace

double getLength (const Request& request)
{
    auto length = request.length;

    if (length <= 0.0)
    {
        double lastEnd = 0.0;

        for (const auto& note : request.notes)
            lastEnd = yup::jmax (lastEnd, note.start + note.duration);

        for (const auto& point : request.automation)
            lastEnd = yup::jmax (lastEnd, point.time);

        length = lastEnd + yup::jmax (0.0, request.tail);
    }

    return yup::jlimit (0.01, maxSeconds, length);
}

yup::ResultValue<yup::AudioBuffer<float>> render (yup::AudioProcessor& processor,
                                                  const Request& request,
                                                  const std::function<bool()>& shouldStop)
{
    const auto sampleRate = request.sampleRate;

    if (sampleRate < 8000.0 || sampleRate > 384000.0)
        return yup::makeResultValueFail ("The sample rate must be between 8000 and 384000");

    std::vector<MidiEvent> midiEvents;

    for (const auto& note : request.notes)
    {
        const auto channel = yup::jlimit (1, 16, note.channel);
        const auto number = yup::jlimit (0, 127, note.note);
        const auto start = toSamples (note.start, sampleRate);

        midiEvents.push_back ({ start, yup::MidiMessage::noteOn (channel, number, yup::jlimit (0.0f, 1.0f, note.velocity)) });
        midiEvents.push_back ({ yup::jmax (start + 1, toSamples (note.start + note.duration, sampleRate)), yup::MidiMessage::noteOff (channel, number) });
    }

    // A note off sorts before a note on at the same sample, so a repeated note retriggers.
    std::stable_sort (midiEvents.begin(), midiEvents.end(), [] (const auto& a, const auto& b)
    {
        return a.sample != b.sample ? a.sample < b.sample : (a.message.isNoteOff() && ! b.message.isNoteOff());
    });

    std::vector<ParameterEvent> parameterEvents;

    for (const auto& point : request.automation)
        parameterEvents.push_back ({ toSamples (point.time, sampleRate), point });

    std::stable_sort (parameterEvents.begin(), parameterEvents.end(), [] (const auto& a, const auto& b) { return a.sample < b.sample; });

    processor.setOfflineProcessing (true);
    processor.setPlaybackConfiguration (static_cast<float> (sampleRate), blockSize);

    const auto latency = static_cast<yup::int64> (yup::jmax (0, processor.getLatencySamples()));
    const auto totalSamples = toSamples (getLength (request), sampleRate);

    yup::AudioBuffer<float> output (2, static_cast<int> (totalSamples));
    output.clear();
    yup::AudioBuffer<float> block (2, blockSize);
    yup::MidiBuffer midi;
    yup::ParameterChangeBuffer parameters;
    parameters.reserve (static_cast<int> (parameterEvents.size()) + 32);

    auto nextMidi = midiEvents.begin();
    auto nextParameter = parameterEvents.begin();

    for (yup::int64 position = 0; position < totalSamples + latency; position += blockSize)
    {
        if (shouldStop != nullptr && shouldStop())
        {
            processor.releaseResources();
            return yup::makeResultValueFail ("The render was cancelled");
        }

        const auto count = static_cast<int> (yup::jmin (static_cast<yup::int64> (blockSize), totalSamples + latency - position));
        block.setSize (2, count, false, false, true);
        midi.clear();
        parameters.clear();

        for (; nextMidi != midiEvents.end() && nextMidi->sample < position + count; ++nextMidi)
            midi.addEvent (nextMidi->message, static_cast<int> (yup::jmax (static_cast<yup::int64> (0), nextMidi->sample - position)));

        for (; nextParameter != parameterEvents.end() && nextParameter->sample < position + count; ++nextParameter)
        {
            const auto& automation = nextParameter->automation;
            parameters.addChange (automation.parameterIndex, automation.normalizedValue, static_cast<int> (yup::jmax (static_cast<yup::int64> (0), nextParameter->sample - position)));
        }

        yup::AudioProcessContext<float> context { block, midi, parameters };
        processor.processBlock (context);

        // The first latency samples are the processor's delay, not sound.
        for (int index = 0; index < count; ++index)
        {
            const auto target = position + index - latency;

            if (target < 0 || target >= totalSamples)
                continue;

            for (int channel = 0; channel < 2; ++channel)
                output.setSample (channel, static_cast<int> (target), block.getSample (yup::jmin (channel, block.getNumChannels() - 1), index));
        }
    }

    processor.releaseResources();
    return yup::makeResultValueOk (std::move (output));
}
} // namespace RetraceAudioRender
