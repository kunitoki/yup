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

namespace
{

template <typename FloatType>
void processPluginBypassedBlock (const AudioPluginDescription& description,
                                 AudioBuffer<FloatType>& audioBuffer) noexcept
{
    const int numSamples = audioBuffer.getNumSamples();
    const int numBufferChannels = audioBuffer.getNumChannels();
    const int numInputs = jmin (description.numInputChannels, numBufferChannels);
    const int numOutputs = jmin (description.numOutputChannels, numBufferChannels);
    const int numChannelsToCopy = jmin (numInputs, numOutputs);

    for (int channel = 0; channel < numChannelsToCopy; ++channel)
    {
        const auto* source = audioBuffer.getReadPointer (channel);
        auto* destination = audioBuffer.getWritePointer (channel);

        if (source != destination)
            FloatVectorOperations::copy (destination, source, numSamples);
    }

    for (int channel = numChannelsToCopy; channel < numOutputs; ++channel)
        audioBuffer.clear (channel, 0, numSamples);
}

} // namespace

AudioPluginInstance::AudioPluginInstance (const AudioPluginDescription& description,
                                          AudioBusLayout busLayout)
    : AudioProcessor (description.name, std::move (busLayout))
    , pluginDescription (description)
{
}

AudioPluginInstance::~AudioPluginInstance() = default;

const AudioPluginDescription& AudioPluginInstance::getDescription() const noexcept
{
    return pluginDescription;
}

AudioPluginFormatType AudioPluginInstance::getFormatType() const noexcept
{
    return pluginDescription.formatType;
}

void AudioPluginInstance::setNonRealtime (bool shouldBeNonRealtime) noexcept
{
    if (nonRealtime == shouldBeNonRealtime)
        return;

    nonRealtime = shouldBeNonRealtime;
    nonRealtimeStateChanged();
}

bool AudioPluginInstance::isNonRealtime() const noexcept
{
    return nonRealtime;
}

void AudioPluginInstance::setBypassed (bool shouldBeBypassed) noexcept
{
    if (bypassed.exchange (shouldBeBypassed) == shouldBeBypassed)
        return;

    bypassStateChanged();
}

bool AudioPluginInstance::isBypassed() const noexcept
{
    return bypassed.load();
}

void AudioPluginInstance::processBlockBypassed (AudioProcessContext<float>& context)
{
    processPluginBypassedBlock (pluginDescription, context.audio);
}

void AudioPluginInstance::processBlockBypassed (AudioProcessContext<double>& context)
{
    processPluginBypassedBlock (pluginDescription, context.audio);
}

//==============================================================================

namespace
{

/** Resizes the window holding a hosted editor so that the editor ends up with the given size. */
void setHostedEditorSize (Component& editor, Size<int> editorSize)
{
    auto* topLevel = editor.getTopLevelComponent();

    topLevel->setSize ({ topLevel->getWidth() - editor.getWidth() + static_cast<float> (editorSize.getWidth()),
                         topLevel->getHeight() - editor.getHeight() + static_cast<float> (editorSize.getHeight()) });
}

/** Detects whether a hosted editor locks its aspect ratio, for plugin formats which can't report it.

    Asks the plugin to constrain a size twice as wide as its current one: an editor which locks its aspect ratio
    grows the height as well. Returns the size to derive the aspect ratio from, or nothing for free resizing.
*/
std::optional<Size<int>> probeHostedEditorAspectRatio (Size<int> currentSize, const std::function<Size<int> (Size<int>)>& constrainSize)
{
    if (currentSize.getWidth() <= 0 || currentSize.getHeight() <= 0)
        return std::nullopt;

    const auto probed = constrainSize ({ currentSize.getWidth() * 2, currentSize.getHeight() });

    if (probed.getWidth() <= 0 || probed.getHeight() <= currentSize.getHeight())
        return std::nullopt;

    const auto ratio = currentSize.getWidth() / static_cast<double> (currentSize.getHeight());
    const auto probedRatio = probed.getWidth() / static_cast<double> (probed.getHeight());

    if (std::abs (probedRatio - ratio) > ratio * 0.02)
        return std::nullopt;

    return currentSize;
}

/** Makes the window holding a hosted editor follow the size the plugin settled on, when it differs from the editor's. */
void followHostedEditorSize (Component& editor, Size<int> editorSize)
{
    if (std::abs (editor.getWidth() - static_cast<float> (editorSize.getWidth())) <= 1.0f
        && std::abs (editor.getHeight() - static_cast<float> (editorSize.getHeight())) <= 1.0f)
    {
        return;
    }

    MessageManager::callAsync ([weakEditor = WeakReference<Component> (&editor), editorSize]
    {
        if (auto* editorToResize = weakEditor.get())
            setHostedEditorSize (*editorToResize, editorSize);
    });
}

} // namespace

} // namespace yup
