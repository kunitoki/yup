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

#include <yup_data_model/yup_data_model.h>
#include <yup_gui/yup_gui.h>

#include <functional>
#include <type_traits>
#include <utility>

//==============================================================================
/** The component keeping the undo history that every edit made inside it records into.

    @see SynthUndoHistory
*/
class UndoHistoryOwner
{
public:
    virtual ~UndoHistoryOwner() = default;

    /** Returns the history the edits record into. */
    virtual yup::UndoManager& getUndoHistory() = 0;
};

//==============================================================================
/** Records the edits made in the interface into the history of its UndoHistoryOwner.

    Every edit is kept as the value before and after it with a function that puts a
    value back. That function must work on state that outlives the interface, such as
    the processor's parameters, partials, curves and routes, since the history does.
*/
namespace SynthUndoHistory
{
/** Returns the history of the first owner up the tree from a component, or nullptr. */
yup::UndoManager* find (yup::Component& component);

/** Starts a new step, for edits recorded with joinsLastStep that must not join an older one. */
void beginStep (yup::Component& component);

/** One edit, already made when it is recorded, so its first redo does nothing. */
template <typename Value>
class Edit : public yup::UndoableAction
{
public:
    Edit (Value valueBefore, Value valueAfter, std::function<void (const Value&)> applyValue)
        : before (std::move (valueBefore))
        , after (std::move (valueAfter))
        , apply (std::move (applyValue))
    {
    }

    bool isValid() const override { return true; }

    bool perform (yup::UndoableActionState state) override
    {
        const auto isRecording = std::exchange (recording, false);

        if (state == yup::UndoableActionState::Redo && isRecording)
            return true;

        apply (state == yup::UndoableActionState::Undo ? before : after);
        return true;
    }

private:
    Value before;
    Value after;
    std::function<void (const Value&)> apply;
    bool recording = true;
};

/** Records an edit already made, as a step of its own or as part of the last step.

    @param component     A component inside the interface, to find the history from
    @param before        The value before the edit
    @param after         The value after it
    @param apply         Puts a value back
    @param joinsLastStep True to undo it together with the step recorded just before
*/
template <typename Value>
void record (yup::Component& component,
             Value before,
             Value after,
             std::type_identity_t<std::function<void (const Value&)>> apply,
             bool joinsLastStep = false)
{
    auto* history = find (component);

    if (history == nullptr)
        return;

    if (! joinsLastStep)
        history->beginNewTransaction();

    history->perform (new Edit<Value> (std::move (before), std::move (after), std::move (apply)));
}
} // namespace SynthUndoHistory
