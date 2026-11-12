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
/** Binds a control to an audio parameter so it refreshes when the parameter changes.

    Create one per parameter-driven control, passing the editor, the parameter and a callback that
    updates the control from the parameter's current value, typically the control's existing
    syncFromParameter(). The attachment registers with the editor's parameter dispatcher by
    parameter identity; when AudioProcessorEditor::dispatchPendingParameterChanges() sees a change
    in the processor's ParameterChangeQueue, it invokes the callback on the message thread. This
    replaces polling every parameter every frame with refreshing only the controls that moved.

    The attachment unregisters itself on destruction, so the editor it refers to must outlive it,
    which holds for controls owned by the editor. Rebind to another parameter with setParameter().
    Create, rebind and destroy attachments on the message thread. Callbacks may remove or rebind
    attachments, including themselves.

    @see AudioProcessorEditor::startParameterListening, ParameterChangeQueue, AudioParameter
*/
class YUP_API AudioParameterAttachment
{
public:
    //==============================================================================
    /** Creates an attachment binding a parameter to a callback.

        @param editor        The editor whose dispatcher drives this attachment.
        @param parameter     The parameter to follow, or nullptr to bind later with setParameter().
        @param onChanged     Called on the message thread when the parameter changes.
        @param callOnCreate  When true, invokes onChanged once immediately for the initial sync.
    */
    AudioParameterAttachment (AudioProcessorEditor& editor,
                              AudioParameter::Ptr parameter,
                              std::function<void()> onChanged,
                              bool callOnCreate = true);

    /** Destructor. Unregisters from the editor's dispatcher. */
    ~AudioParameterAttachment();

    //==============================================================================
    /** Rebinds the attachment to another parameter, re-registering with the dispatcher.

        @param newParameter  The new parameter to follow, or nullptr to unbind.
        @param callNow       When true, invokes the callback once after rebinding.
    */
    void setParameter (AudioParameter::Ptr newParameter, bool callNow = true);

    /** Returns the parameter the attachment currently follows, or nullptr when unbound. */
    const AudioParameter::Ptr& getParameter() const noexcept { return parameter; }

    //==============================================================================
    /** @internal Invokes the callback. Called by the editor dispatcher on the message thread. */
    void parameterChanged();

private:
    void registerSelf();
    void unregisterSelf();

    AudioProcessorEditor& editor;
    AudioParameter::Ptr parameter;
    std::shared_ptr<std::function<void()>> onChanged;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioParameterAttachment)
};

} // namespace yup
