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
    Exposes the parameters of an audio processor to a UI.

    Each parameter becomes a `number` value named after its ID, holding the real (not normalized)
    value, so a `.yui` document binds controls to it by name once the provider is registered with
    YuiBuilder::setModel or YuiDocument::setModel. Give a Slider the range of its parameter:

    @code
    Slider { minimum: 20; maximum: 20000; skew: 0.25; value <=> params.cutoff; }
    @endcode

    Changes made through the UI are written with AudioParameter::setValueNotifyingHost inside a
    change gesture of the parameter, a whole knob drag or a single key or wheel step, so the host
    records automation. Writes and gestures on read-only parameters are ignored. Changes made by
    anything else, such as the host or the audio thread, reach the UI through an
    AudioParameterAttachment per parameter, so the editor must call
    AudioProcessorEditor::startParameterListening.

    The IDs must be valid identifiers to be read in expressions (`cutoff`, not `filter.cutoff`).
    The editor must outlive the provider; create and destroy it on the message thread.

    @code
    MyEditor (MyProcessor& processor)
        : params (*this, processor.getParameters())
    {
        startParameterListening (processor);
        document.setModel ("params", params);
    }
    @endcode

    @see BindingProvider, AudioParameterAttachment, YuiDocument

    @tags{UI}
*/
class YUP_API AudioParameterBindingProvider final : public BindingProvider
{
public:
    //==============================================================================
    /** Creates a provider over a set of parameters.

        @param editor       The editor whose parameter dispatch reports the changes made elsewhere.
        @param parameters   The parameters to expose, usually AudioProcessorBase::getParameters().
    */
    AudioParameterBindingProvider (AudioProcessorEditor& editor, Span<const AudioParameter::Ptr> parameters);

    /** Destructor. */
    ~AudioParameterBindingProvider() override;

    //==============================================================================
    /** Returns the parameter exposed with a name, or nullptr if there is none. */
    AudioParameter::Ptr getParameter (const Identifier& name) const;

    //==============================================================================
    /** @internal */
    std::vector<Property> getProperties() const override;
    /** @internal */
    var getValue (const Identifier& name) const override;
    /** @internal */
    void setValue (const Identifier& name, const var& value) override;
    /** @internal */
    void beginGesture (const Identifier& name) override;
    /** @internal */
    void endGesture (const Identifier& name) override;

private:
    //==============================================================================
    struct Entry
    {
        Identifier name;
        std::unique_ptr<AudioParameterAttachment> attachment;
    };

    AudioParameter* findParameter (const Identifier& name) const;

    std::vector<Entry> entries;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioParameterBindingProvider)
};

} // namespace yup
