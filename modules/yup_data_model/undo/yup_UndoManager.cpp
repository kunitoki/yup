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

void UndoManager::CoalescedItem::add (UndoableAction::Ptr action)
{
    if (action != nullptr)
        childItems.add (action);
}

bool UndoManager::CoalescedItem::perform (UndoableActionState stateToPerform)
{
    if (stateToPerform == UndoableActionState::Undo)
    {
        for (int i = childItems.size() - 1; i >= 0; i--)
        {
            if (! childItems[i]->perform (stateToPerform))
                childItems.remove (i);
        }
    }
    else
    {
        for (int i = 0; i < childItems.size(); i++)
        {
            if (! childItems[i]->perform (stateToPerform))
                childItems.remove (i--);
        }
    }

    return ! childItems.isEmpty();
}

bool UndoManager::CoalescedItem::isValid() const
{
    return childItems.isEmpty();
}

//==============================================================================

UndoManager::ScopedTransaction::ScopedTransaction (UndoManager& undoManager)
    : undoManager (undoManager)
{
    undoManager.flushCurrentAction();
}

UndoManager::ScopedTransaction::~ScopedTransaction()
{
    undoManager.flushCurrentAction();
}

//==============================================================================

UndoManager::UndoManager (int maxHistorySize)
    : maxHistorySize (maxHistorySize)
{
    setEnabled (true);
}

//==============================================================================

bool UndoManager::perform (UndoableAction::Ptr action)
{
    jassert (action != nullptr);

    if (action->perform (UndoableActionState::Redo))
    {
        currentlyBuiltAction->add (action);
        return true;
    }

    return false;
}

//==============================================================================

bool UndoManager::undo()
{
    return internalPerform (UndoableActionState::Undo);
}

bool UndoManager::redo()
{
    return internalPerform (UndoableActionState::Redo);
}

//==============================================================================

void UndoManager::setEnabled (bool shouldBeEnabled)
{
    if (isEnabled() != shouldBeEnabled)
    {
        if (shouldBeEnabled)
        {
            startTimer (500);

            currentlyBuiltAction = new CoalescedItem;
        }
        else
        {
            stopTimer();

            currentlyBuiltAction = nullptr;

            undoHistory.clear();
        }
    }
}

bool UndoManager::isEnabled() const
{
    return isTimerRunning();
}

//==============================================================================

void UndoManager::timerCallback()
{
    flushCurrentAction();
}

//==============================================================================

bool UndoManager::internalPerform (UndoableActionState stateToPerform)
{
    flushCurrentAction();

    auto& actionIndex = (stateToPerform == UndoableActionState::Undo) ? nextUndoAction : nextRedoAction;

    if (auto current = undoHistory[actionIndex])
    {
        current->perform (stateToPerform);

        const auto delta = (stateToPerform == UndoableActionState::Undo) ? -1 : 1;
        nextUndoAction += delta;
        nextRedoAction += delta;

        return true;
    }

    return false;
}

//==============================================================================

bool UndoManager::flushCurrentAction()
{
    if (! currentlyBuiltAction->isValid())
        return false;

    // Remove all future actions
    if (nextRedoAction < undoHistory.size())
        undoHistory.removeRange (nextRedoAction, undoHistory.size() - nextRedoAction);

    undoHistory.add (currentlyBuiltAction.get());
    currentlyBuiltAction = new CoalescedItem;

    // Clean up to keep the undo history in check
    const int numToRemove = jmax (0, undoHistory.size() - maxHistorySize);
    if (numToRemove > 0)
        undoHistory.removeRange (0, numToRemove);

    nextUndoAction = undoHistory.size() - 1;
    nextRedoAction = undoHistory.size();

    return true;
}

} // namespace yup
