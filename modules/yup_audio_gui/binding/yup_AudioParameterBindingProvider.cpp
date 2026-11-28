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

AudioParameterBindingProvider::AudioParameterBindingProvider (AudioProcessorEditor& editor, Span<const AudioParameter::Ptr> parameters)
{
    entries.reserve (parameters.size());

    for (const auto& parameter : parameters)
    {
        jassert (parameter != nullptr);

        const Identifier name (parameter->getID());
        auto onChanged = [this, name]
        {
            notifyValueChanged (name);
        };

        entries.push_back ({ name, std::make_unique<AudioParameterAttachment> (editor, parameter, std::move (onChanged), false) });
    }
}

AudioParameterBindingProvider::~AudioParameterBindingProvider() = default;

//==============================================================================

AudioParameter::Ptr AudioParameterBindingProvider::getParameter (const Identifier& name) const
{
    return findParameter (name);
}

AudioParameter* AudioParameterBindingProvider::findParameter (const Identifier& name) const
{
    for (const auto& entry : entries)
        if (entry.name == name)
            return entry.attachment->getParameter().get();

    return nullptr;
}

//==============================================================================

std::vector<BindingProvider::Property> AudioParameterBindingProvider::getProperties() const
{
    std::vector<Property> properties;
    properties.reserve (entries.size());

    for (const auto& entry : entries)
        properties.push_back ({ entry.name, ComponentProperty::Type::number });

    return properties;
}

var AudioParameterBindingProvider::getValue (const Identifier& name) const
{
    if (const auto* parameter = findParameter (name))
        return static_cast<double> (parameter->getValue());

    return {};
}

void AudioParameterBindingProvider::setValue (const Identifier& name, const var& value)
{
    auto* parameter = findParameter (name);
    if (parameter == nullptr || parameter->isReadOnly())
        return;

    const auto newValue = static_cast<float> (value);
    if (exactlyEqual (newValue, parameter->getValue()))
        return;

    parameter->setValueNotifyingHost (newValue);

    // Reported now rather than on the next dispatch, so the bound controls show the snapped value
    notifyValueChanged (name);
}

void AudioParameterBindingProvider::beginGesture (const Identifier& name)
{
    if (auto* parameter = findParameter (name); parameter != nullptr && ! parameter->isReadOnly())
        parameter->beginChangeGesture();
}

void AudioParameterBindingProvider::endGesture (const Identifier& name)
{
    if (auto* parameter = findParameter (name); parameter != nullptr && ! parameter->isReadOnly())
        parameter->endChangeGesture();
}

std::optional<String> AudioParameterBindingProvider::getTextForValue (const Identifier& name, const var& value) const
{
    if (const auto* parameter = findParameter (name))
        return parameter->convertToString (static_cast<float> (value));

    return std::nullopt;
}

std::optional<var> AudioParameterBindingProvider::getValueForText (const Identifier& name, const String& text) const
{
    if (const auto* parameter = findParameter (name))
        return var (static_cast<double> (parameter->convertFromString (text)));

    return std::nullopt;
}

} // namespace yup
