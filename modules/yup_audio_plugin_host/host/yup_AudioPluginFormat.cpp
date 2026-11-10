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

namespace yup
{

ResultValue<std::vector<AudioPluginDescription>> AudioPluginFormat::scanFileFully (const File& file,
                                                                                   const AudioPluginHostContext& context)
{
    jassert (MessageManager::getInstanceWithoutCreating() == nullptr || MessageManager::existsAndIsCurrentThread());

    auto scanned = scanFile (file);
    if (scanned.failed())
        return scanned;

    const auto countMidiBuses = [] (Span<const AudioBus> buses)
    {
        return static_cast<int> (std::count_if (buses.begin(), buses.end(), [] (const AudioBus& bus)
        {
            return bus.getType() == AudioBus::Type::Midi;
        }));
    };

    std::vector<AudioPluginDescription> results;
    String lastError;

    for (auto& description : std::move (scanned).getValue())
    {
        auto loaded = loadPlugin (description, context);
        if (loaded.failed())
        {
            lastError = loaded.getErrorMessage();
            continue;
        }

        const auto instance = std::move (loaded).getValue();
        const auto& layout = instance->getBusLayout();

        description.numInputChannels = layout.getNumAudioInputChannels();
        description.numOutputChannels = layout.getNumAudioOutputChannels();
        description.numMidiInputPorts = jmax (countMidiBuses (layout.getInputBuses()), instance->acceptsMidi() ? 1 : 0);
        description.numMidiOutputPorts = jmax (countMidiBuses (layout.getOutputBuses()), instance->producesMidi() ? 1 : 0);

        results.push_back (std::move (description));
    }

    if (results.empty())
        return makeResultValueFail (lastError.isNotEmpty() ? lastError : "No plugins could be opened in " + file.getFullPathName());

    return makeResultValueOk (std::move (results));
}

} // namespace yup
