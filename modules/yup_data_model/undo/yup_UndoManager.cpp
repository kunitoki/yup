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

UndoManager::Transaction::Transaction (StringRef name)
    : transactionName (name)
{
}

void UndoManager::Transaction::add (UndoableAction::Ptr action)
{
    if (action != nullptr)
        childItems.add (action);
}

int UndoManager::Transaction::size() const
{
    return childItems.size();
}

String UndoManager::Transaction::getTransactionName() const
{
    return transactionName;
}

void UndoManager::Transaction::setTransactionName (StringRef newName)
{
    transactionName = newName;
}

bool UndoManager::Transaction::perform (UndoableActionState stateToPerform)
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

    return isValid();
}

bool UndoManager::Transaction::isValid() const
{
    return ! childItems.isEmpty();
}

//==============================================================================

UndoManager::ScopedTransaction::ScopedTransaction (UndoManager& undoManager)
    : undoManager (undoManager)
{
    undoManager.beginNewTransaction();
}

UndoManager::ScopedTransaction::ScopedTransaction (UndoManager& undoManager, StringRef transactionName)
    : undoManager (undoManager)
{
    undoManager.beginNewTransaction (transactionName);
}

UndoManager::ScopedTransaction::~ScopedTransaction()
{
    undoManager.flushCurrentTransaction();
}

//==============================================================================

UndoManager::UndoManager()
    : maxHistorySize (100)
    , actionGroupThreshold (RelativeTime::milliseconds (500))
{
    setEnabled (true);
}

UndoManager::UndoManager (int maxHistorySize)
    : maxHistorySize (maxHistorySize)
    , actionGroupThreshold (RelativeTime::milliseconds (500))
{
    setEnabled (true);
}

UndoManager::UndoManager (RelativeTime actionGroupThreshold)
    : maxHistorySize (100)
    , actionGroupThreshold (actionGroupThreshold)
{
    setEnabled (true);
}

UndoManager::UndoManager (int maxHistorySize, RelativeTime actionGroupThreshold)
    : maxHistorySize (maxHistorySize)
    , actionGroupThreshold (actionGroupThreshold)
{
    setEnabled (true);
}

//==============================================================================

bool UndoManager::perform (UndoableAction::Ptr action)
{
    jassert (action != nullptr);

    if (! isEnabled())
        return false;

    if (action->perform (UndoableActionState::Redo))
    {
        if (currentTransaction == nullptr)
            beginNewTransaction();

        currentTransaction->add (action);

        return true;
    }

    return false;
}

//==============================================================================

void UndoManager::beginNewTransaction()
{
    beginNewTransaction ({});
}

void UndoManager::beginNewTransaction (StringRef transactionName)
{
    flushCurrentTransaction();

    if (currentTransaction == nullptr)
        currentTransaction = new Transaction (transactionName);

    else if (currentTransaction->isValid())
        currentTransaction->setTransactionName (transactionName);
}

//==============================================================================

int UndoManager::getNumTransactions() const
{
    return undoHistory.size() + (currentTransaction != nullptr ? 1 : 0);
}

String UndoManager::getTransactionName (int index) const
{
    if (isPositiveAndBelow (index, getNumTransactions ()))
    {
        if (isPositiveAndBelow (index, undoHistory.size()))
            return undoHistory.getUnchecked (index)->getTransactionName();

        else if (currentTransaction != nullptr)
            return currentTransaction->getTransactionName();
    }

    return {};
}

String UndoManager::getCurrentTransactionName() const
{
    if (currentTransaction != nullptr)
        return currentTransaction->getTransactionName();

    return {};
}

void UndoManager::setCurrentTransactionName (StringRef newName)
{
    if (currentTransaction != nullptr)
        currentTransaction->setTransactionName (newName);
}

//==============================================================================

bool UndoManager::canUndo() const
{
    return (currentTransaction != nullptr && currentTransaction->isValid())
        || isPositiveAndBelow (nextUndoAction, undoHistory.size());
}

bool UndoManager::undo()
{
    return internalPerform (UndoableActionState::Undo);
}

bool UndoManager::canRedo() const
{
    return (currentTransaction != nullptr && currentTransaction->isValid())
        || isPositiveAndBelow (nextRedoAction, undoHistory.size());
}

bool UndoManager::redo()
{
    return internalPerform (UndoableActionState::Redo);
}

//==============================================================================

void UndoManager::clear()
{
    nextUndoAction = -1;
    nextRedoAction = -1;

    undoHistory.clearQuick();
    currentTransaction.reset();
}

//==============================================================================

void UndoManager::setEnabled (bool shouldBeEnabled)
{
    if (isEnabled() != shouldBeEnabled)
    {
        isUndoEnabled = shouldBeEnabled;

        if (shouldBeEnabled)
        {
            if (actionGroupThreshold > RelativeTime())
                startTimer (static_cast<int> (actionGroupThreshold.inMilliseconds()));
        }
        else
        {
            if (actionGroupThreshold > RelativeTime())
                stopTimer();

            flushCurrentTransaction();

            undoHistory.clear();
        }
    }
}

bool UndoManager::isEnabled() const
{
    return isUndoEnabled;
}

//==============================================================================

void UndoManager::timerCallback()
{
    beginNewTransaction();
}

//==============================================================================

bool UndoManager::internalPerform (UndoableActionState stateToPerform)
{
    flushCurrentTransaction();

    auto actionIndex = (stateToPerform == UndoableActionState::Undo) ? nextUndoAction : nextRedoAction;
    if (! isPositiveAndBelow (actionIndex, undoHistory.size()))
        return false;

    auto current = undoHistory[actionIndex];
    if (current == nullptr)
        return false;

    auto result = current->perform (stateToPerform);
    if (result)
    {
        const auto delta = (stateToPerform == UndoableActionState::Undo) ? -1 : 1;
        nextUndoAction += delta;
        nextRedoAction += delta;
    }

    return result;
}

//==============================================================================

bool UndoManager::flushCurrentTransaction()
{
    if (currentTransaction == nullptr || ! currentTransaction->isValid())
        return false;

    // Remove all future actions
    if (nextRedoAction < undoHistory.size())
        undoHistory.removeRange (nextRedoAction, undoHistory.size() - nextRedoAction);

    undoHistory.add (currentTransaction.get());
    currentTransaction = nullptr;

    // Clean up to keep the undo history in check
    const int numToRemove = jmax (0, undoHistory.size() - maxHistorySize);
    if (numToRemove > 0)
        undoHistory.removeRange (0, numToRemove);

    nextUndoAction = undoHistory.size() - 1;
    nextRedoAction = undoHistory.size();

    return true;
}

} // namespace yup
