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

AudioProcessorEditor::AudioProcessorEditor()
{
}

AudioProcessorEditor::~AudioProcessorEditor()
{
    stopParameterListening();
}

//==============================================================================

void AudioProcessorEditor::startParameterListening (AudioProcessorBase& processor, int refreshRateHz)
{
    stopParameterListening();
    listenedProcessor = &processor;
    processor.parameterEditors.add (this);

    if (refreshRateHz <= 0)
        return;

    parameterDispatchTimer = std::make_unique<TimedCallback> ([this]
    {
        dispatchPendingParameterChanges();
    });

    parameterDispatchTimer->startTimerHz (refreshRateHz);
}

void AudioProcessorEditor::stopParameterListening()
{
    parameterDispatchTimer.reset();

    if (listenedProcessor != nullptr)
        listenedProcessor->parameterEditors.remove (this);

    listenedProcessor = nullptr;
}

void AudioProcessorEditor::dispatchPendingParameterChanges()
{
    if (listenedProcessor != nullptr)
        listenedProcessor->dispatchParameterChanges();
}

void AudioProcessorBase::dispatchParameterChanges()
{
    if (dispatchingParameterChanges)
        return;

    const ScopedValueSetter<bool> dispatching (dispatchingParameterChanges, true);

    parameterChangeQueue.readChanges ([this] (int index)
    {
        const auto parameter = parameters[static_cast<std::size_t> (index)];
        parameterEditors.call (&AudioProcessorEditor::parameterChanged, parameter.get());
    });
}

void AudioProcessorEditor::parameterChanged (AudioParameter* parameter)
{
    if (auto it = parameterAttachments.find (parameter); it != parameterAttachments.end())
        it->second.call (&AudioParameterAttachment::parameterChanged);
}

void AudioProcessorEditor::registerParameterAttachment (AudioParameter* parameter, AudioParameterAttachment* attachment)
{
    jassert (parameter != nullptr && attachment != nullptr);
    if (parameter != nullptr && attachment != nullptr)
        parameterAttachments[parameter].add (attachment);
}

void AudioProcessorEditor::unregisterParameterAttachment (AudioParameter* parameter, AudioParameterAttachment* attachment)
{
    if (auto it = parameterAttachments.find (parameter); it != parameterAttachments.end())
    {
        it->second.remove (attachment);

        if (it->second.isEmpty())
            parameterAttachments.erase (it);
    }
}

AudioParameter::Ptr AudioProcessorEditor::getParameterAt (Point<float> position) const
{
    ignoreUnused (position);
    return nullptr;
}

//==============================================================================

Size<int> AudioProcessorEditor::getConstrainedSize (Size<int> requestedSize) const
{
    return constrainSize (requestedSize, false);
}

Size<int> AudioProcessorEditor::getConstrainedSizeToFit (Size<int> availableSize) const
{
    return constrainSize (availableSize, true);
}

Size<int> AudioProcessorEditor::constrainSize (Size<int> size, bool fitInside) const
{
    if (! isResizable())
        return getPreferredSize();

    const auto width = jmax (1, size.getWidth());
    const auto height = jmax (1, size.getHeight());

    const auto ratioSize = getPreferredAspectRatioSize();
    if (! shouldPreserveAspectRatio() || ratioSize.getWidth() <= 0 || ratioSize.getHeight() <= 0)
        return { width, height };

    const auto ratio = static_cast<double> (ratioSize.getWidth()) / static_cast<double> (ratioSize.getHeight());

    const auto heightFromWidth = jmax (1, roundToInt (width / ratio));
    const auto widthFromHeight = jmax (1, roundToInt (height * ratio));
    if (height == heightFromWidth || width == widthFromHeight)
        return { width, height };

    // Independent of the current size, so hosts repeating unconstrained sizes while dragging always get the same answer
    const auto isWider = static_cast<double> (width) / static_cast<double> (height) >= ratio;
    if (isWider != fitInside)
        return { width, heightFromWidth };

    return { widthFromHeight, height };
}

} // namespace yup
