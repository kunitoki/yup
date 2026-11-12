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

AudioParameterAttachment::AudioParameterAttachment (AudioProcessorEditor& editorToUse,
                                                    AudioParameter::Ptr parameterToUse,
                                                    std::function<void()> onChangedToUse,
                                                    bool callOnCreate)
    : editor (editorToUse)
    , parameter (std::move (parameterToUse))
    , onChanged (onChangedToUse != nullptr ? std::make_shared<std::function<void()>> (std::move (onChangedToUse)) : nullptr)
{
    registerSelf();

    if (callOnCreate)
        parameterChanged();
}

AudioParameterAttachment::~AudioParameterAttachment()
{
    unregisterSelf();
}

void AudioParameterAttachment::setParameter (AudioParameter::Ptr newParameter, bool callNow)
{
    unregisterSelf();

    parameter = std::move (newParameter);

    registerSelf();

    if (callNow)
        parameterChanged();
}

void AudioParameterAttachment::parameterChanged()
{
    // Keep the callable and its state alive if it destroys the attachment.
    if (auto callback = onChanged)
        (*callback)();
}

void AudioParameterAttachment::registerSelf()
{
    if (parameter != nullptr)
        editor.registerParameterAttachment (parameter.get(), this);
}

void AudioParameterAttachment::unregisterSelf()
{
    if (parameter != nullptr)
        editor.unregisterParameterAttachment (parameter.get(), this);
}

} // namespace yup
