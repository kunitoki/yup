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

#include <gtest/gtest.h>

#include <yup_data_model/yup_data_model.h>

using namespace yup;

// A simple UndoableAction class for testing purposes
class TestAction : public UndoableAction
{
public:
    using Ptr = ReferenceCountedObjectPtr<TestAction>;

    TestAction (bool& flag)
        : flag (flag)
    {
    }

    bool perform (UndoableActionState) override
    {
        flag = ! flag;
        return true;
    }

    bool isValid() const override
    {
        return true;
    }

private:
    bool& flag;
};

// A more complex UndoableAction class for additional testing purposes
class ToggleAction : public UndoableAction
{
public:
    using Ptr = ReferenceCountedObjectPtr<ToggleAction>;

    ToggleAction (int& counter)
        : counter (counter)
    {
    }

    bool perform (UndoableActionState state) override
    {
        if (state == UndoableActionState::Redo)
            ++counter;
        else if (state == UndoableActionState::Undo)
            --counter;

        return true;
    }

    bool isValid() const override
    {
        return true;
    }

private:
    int& counter;
};

// An action that fails when performed in a specific state, used to test failure handling
class FailingAction : public UndoableAction
{
public:
    using Ptr = ReferenceCountedObjectPtr<FailingAction>;

    FailingAction (bool& flag, UndoableActionState failingState)
        : flag (flag)
        , failingState (failingState)
    {
    }

    bool perform (UndoableActionState state) override
    {
        if (state == failingState)
            return false;

        flag = ! flag;
        return true;
    }

    bool isValid() const override
    {
        return true;
    }

private:
    bool& flag;
    UndoableActionState failingState;
};

class UndoManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        actionFlag = false;
        counter = 0;
        undoManager = std::make_unique<UndoManager> (10, RelativeTime::milliseconds (0));
    }

    void TearDown() override
    {
    }

    bool actionFlag;
    int counter;
    std::unique_ptr<UndoManager> undoManager;
};

TEST_F (UndoManagerTest, PerformAction)
{
    TestAction::Ptr action = new TestAction (actionFlag);
    EXPECT_TRUE (undoManager->perform (action));
    EXPECT_TRUE (actionFlag);
}

TEST_F (UndoManagerTest, UndoAction)
{
    TestAction::Ptr action = new TestAction (actionFlag);
    undoManager->perform (action);
    EXPECT_TRUE (actionFlag);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_FALSE (actionFlag);
}

TEST_F (UndoManagerTest, TransactionCount)
{
    actionFlag = false;

    EXPECT_EQ (undoManager->getNumTransactions(), 0);

    {
        undoManager->beginNewTransaction();
        EXPECT_EQ (undoManager->getNumTransactions(), 1);

        undoManager->perform (new TestAction (actionFlag));
        EXPECT_EQ (undoManager->getNumTransactions(), 1);
    }

    {
        undoManager->beginNewTransaction();
        EXPECT_EQ (undoManager->getNumTransactions(), 2);

        undoManager->perform (new TestAction (actionFlag));
        EXPECT_EQ (undoManager->getNumTransactions(), 2);
    }

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (undoManager->getNumTransactions(), 2);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (undoManager->getNumTransactions(), 2);

    EXPECT_FALSE (undoManager->undo());
    EXPECT_EQ (undoManager->getNumTransactions(), 2);

    undoManager->clear();
    EXPECT_EQ (undoManager->getNumTransactions(), 0);
}

TEST_F (UndoManagerTest, TransactionIteration)
{
    actionFlag = false;

    EXPECT_EQ (undoManager->getNumTransactions(), 0);

    {
        undoManager->beginNewTransaction ("1");
        EXPECT_EQ (undoManager->getNumTransactions(), 1);
        undoManager->perform (new TestAction (actionFlag));
        EXPECT_EQ (undoManager->getNumTransactions(), 1);
        undoManager->perform (new TestAction (actionFlag));
        EXPECT_EQ (undoManager->getNumTransactions(), 1);
    }

    {
        undoManager->beginNewTransaction ("2");
        EXPECT_EQ (undoManager->getNumTransactions(), 2);
        undoManager->perform (new TestAction (actionFlag));
        EXPECT_EQ (undoManager->getNumTransactions(), 2);
        undoManager->perform (new TestAction (actionFlag));
        EXPECT_EQ (undoManager->getNumTransactions(), 2);
    }

    EXPECT_EQ (String ("1"), undoManager->getTransactionName (0));
    EXPECT_EQ (String ("2"), undoManager->getTransactionName (1));
}

TEST_F (UndoManagerTest, RedoAction)
{
    TestAction::Ptr action = new TestAction (actionFlag);
    undoManager->perform (action);
    EXPECT_TRUE (actionFlag);

    undoManager->undo();
    EXPECT_FALSE (actionFlag);

    EXPECT_TRUE (undoManager->redo());
    EXPECT_TRUE (actionFlag);
}

TEST_F (UndoManagerTest, SetEnabled)
{
    undoManager->setEnabled (false);
    EXPECT_FALSE (undoManager->isEnabled());

    TestAction::Ptr action = new TestAction (actionFlag);
    EXPECT_FALSE (undoManager->perform (action));
    EXPECT_FALSE (actionFlag);

    undoManager->setEnabled (true);
    EXPECT_TRUE (undoManager->isEnabled());
    EXPECT_TRUE (undoManager->perform (action));
    EXPECT_TRUE (actionFlag);
}

TEST_F (UndoManagerTest, ScopedTransaction)
{
    actionFlag = false;

    {
        UndoManager::ScopedTransaction transaction (*undoManager);

        TestAction::Ptr action1 = new TestAction (actionFlag);
        undoManager->perform (action1);
        EXPECT_TRUE (actionFlag);

        TestAction::Ptr action2 = new TestAction (actionFlag);
        undoManager->perform (action2);
        EXPECT_FALSE (actionFlag);
    }

    EXPECT_TRUE (undoManager->undo());
    EXPECT_FALSE (actionFlag);
}

TEST_F (UndoManagerTest, ScopedTransactionWithName)
{
    actionFlag = false;

    EXPECT_EQ (undoManager->getCurrentTransactionName(), "");

    {
        UndoManager::ScopedTransaction transaction (*undoManager, "custom name");

        TestAction::Ptr action1 = new TestAction (actionFlag);
        undoManager->perform (action1);
        EXPECT_TRUE (actionFlag);

        TestAction::Ptr action2 = new TestAction (actionFlag);
        undoManager->perform (action2);
        EXPECT_FALSE (actionFlag);

        EXPECT_EQ (undoManager->getCurrentTransactionName(), "custom name");
    }

    EXPECT_TRUE (undoManager->undo());
    EXPECT_FALSE (actionFlag);

    EXPECT_EQ (undoManager->getCurrentTransactionName(), "");
}

TEST_F (UndoManagerTest, PerformWithLambda)
{
    struct Object : ReferenceCountedObject
    {
    public:
        using Ptr = ReferenceCountedObjectPtr<Object>;

        int counter = 0;

    private:
        YUP_DECLARE_WEAK_REFERENCEABLE (Object)
    };

    auto lambdaAction = [] (Object::Ptr x, UndoableActionState s) -> bool
    {
        x->counter = (s == UndoableActionState::Undo) ? 1 : 2;
        return true;
    };

    Object::Ptr x = new Object;
    EXPECT_TRUE (undoManager->perform (x, lambdaAction));
    EXPECT_EQ (x->counter, 2);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (x->counter, 1);

    EXPECT_TRUE (undoManager->redo());
    EXPECT_EQ (x->counter, 2);
}

TEST_F (UndoManagerTest, ComplexPerformUndoRedo)
{
    ToggleAction::Ptr action1 = new ToggleAction (counter);
    ToggleAction::Ptr action2 = new ToggleAction (counter);

    undoManager->beginNewTransaction();
    EXPECT_TRUE (undoManager->perform (action1));
    EXPECT_EQ (counter, 1);

    undoManager->beginNewTransaction();
    EXPECT_TRUE (undoManager->perform (action2));
    EXPECT_EQ (counter, 2);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 1);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 0);

    EXPECT_TRUE (undoManager->redo());
    EXPECT_EQ (counter, 1);

    EXPECT_TRUE (undoManager->redo());
    EXPECT_EQ (counter, 2);
}

TEST_F (UndoManagerTest, RedoWithoutUndo)
{
    ToggleAction::Ptr action = new ToggleAction (counter);
    EXPECT_TRUE (undoManager->perform (action));
    EXPECT_EQ (counter, 1);

    EXPECT_FALSE (undoManager->redo());
    EXPECT_EQ (counter, 1);
}

TEST_F (UndoManagerTest, UndoWithoutPerform)
{
    EXPECT_FALSE (undoManager->undo());
}

TEST_F (UndoManagerTest, RedoAfterDisableEnable)
{
    ToggleAction::Ptr action = new ToggleAction (counter);
    EXPECT_TRUE (undoManager->perform (action));
    EXPECT_EQ (counter, 1);

    undoManager->undo();
    EXPECT_EQ (counter, 0);

    undoManager->setEnabled (false);
    EXPECT_FALSE (undoManager->redo());
    EXPECT_EQ (counter, 0);

    undoManager->setEnabled (true);
    EXPECT_FALSE (undoManager->redo());
    EXPECT_EQ (counter, 0);
}

TEST_F (UndoManagerTest, MaxHistorySize)
{
    undoManager = std::make_unique<UndoManager> (2, RelativeTime::milliseconds (0));

    ToggleAction::Ptr action1 = new ToggleAction (counter);
    ToggleAction::Ptr action2 = new ToggleAction (counter);
    ToggleAction::Ptr action3 = new ToggleAction (counter);

    undoManager->beginNewTransaction();
    EXPECT_TRUE (undoManager->perform (action1));
    EXPECT_EQ (counter, 1);

    undoManager->beginNewTransaction();
    EXPECT_TRUE (undoManager->perform (action2));
    EXPECT_EQ (counter, 2);

    undoManager->beginNewTransaction();
    EXPECT_TRUE (undoManager->perform (action3));
    EXPECT_EQ (counter, 3);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 2);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 1);

    EXPECT_FALSE (undoManager->undo()); // action1 should be removed due to max history size
    EXPECT_EQ (counter, 1);
}

TEST_F (UndoManagerTest, ScopedTransactionGrouping)
{
    {
        UndoManager::ScopedTransaction transaction (*undoManager);

        ToggleAction::Ptr action1 = new ToggleAction (counter);
        ToggleAction::Ptr action2 = new ToggleAction (counter);

        undoManager->perform (action1);
        EXPECT_EQ (counter, 1);

        undoManager->perform (action2);
        EXPECT_EQ (counter, 2);
    }

    EXPECT_EQ (counter, 2);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 0);
}

TEST_F (UndoManagerTest, DISABLED_NestedScopedTransactions)
{
    {
        UndoManager::ScopedTransaction transaction (*undoManager);

        ToggleAction::Ptr action1 = new ToggleAction (counter);
        EXPECT_TRUE (undoManager->perform (action1));
        EXPECT_EQ (counter, 1);

        {
            UndoManager::ScopedTransaction nestedTransaction (*undoManager);

            ToggleAction::Ptr action2 = new ToggleAction (counter);
            EXPECT_TRUE (undoManager->perform (action2));
            EXPECT_EQ (counter, 2);
        }

        ToggleAction::Ptr action3 = new ToggleAction (counter);
        EXPECT_TRUE (undoManager->perform (action3));
        EXPECT_EQ (counter, 3);
    }

    EXPECT_EQ (counter, 3);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 0);
}

// ==============================================================================
// Degenerate cases and boundary conditions
// ==============================================================================

TEST_F (UndoManagerTest, FreshManagerHasNoUndoRedo)
{
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_FALSE (undoManager->canRedo());
    EXPECT_FALSE (undoManager->undo());
    EXPECT_FALSE (undoManager->redo());
    EXPECT_EQ (undoManager->getNumTransactions(), 0);
}

TEST_F (UndoManagerTest, CanRedoIsFalseAfterPerformingWithoutUndo)
{
    ToggleAction::Ptr action = new ToggleAction (counter);
    EXPECT_TRUE (undoManager->perform (action));
    EXPECT_EQ (counter, 1);

    // A freshly performed action is pending undo, never redo.
    EXPECT_TRUE (undoManager->canUndo());
    EXPECT_FALSE (undoManager->canRedo());
    EXPECT_FALSE (undoManager->redo());
    EXPECT_EQ (counter, 1);
}

TEST_F (UndoManagerTest, ConsecutivePerformsGroupIntoSingleTransaction)
{
    ToggleAction::Ptr first = new ToggleAction (counter);
    ToggleAction::Ptr second = new ToggleAction (counter);

    undoManager->perform (first);
    undoManager->perform (second);
    EXPECT_EQ (counter, 2);
    EXPECT_EQ (undoManager->getNumTransactions(), 1);

    // A single undo reverts the whole group.
    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 0);
    EXPECT_FALSE (undoManager->canUndo());

    // And a single redo replays the whole group.
    EXPECT_TRUE (undoManager->redo());
    EXPECT_EQ (counter, 2);
    EXPECT_FALSE (undoManager->canRedo());
}

TEST_F (UndoManagerTest, UndoingEverythingThenUndoAgainReturnsFalse)
{
    undoManager->beginNewTransaction();
    undoManager->perform (new ToggleAction (counter));
    undoManager->beginNewTransaction();
    undoManager->perform (new ToggleAction (counter));

    EXPECT_TRUE (undoManager->undo());
    EXPECT_TRUE (undoManager->undo());
    EXPECT_FALSE (undoManager->undo());
    EXPECT_EQ (counter, 0);
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_TRUE (undoManager->canRedo());
}

TEST_F (UndoManagerTest, RedoingEverythingThenRedoAgainReturnsFalse)
{
    undoManager->beginNewTransaction();
    undoManager->perform (new ToggleAction (counter));
    undoManager->beginNewTransaction();
    undoManager->perform (new ToggleAction (counter));

    undoManager->undo();
    undoManager->undo();

    EXPECT_TRUE (undoManager->redo());
    EXPECT_TRUE (undoManager->redo());
    EXPECT_FALSE (undoManager->redo());
    EXPECT_EQ (counter, 2);
    EXPECT_TRUE (undoManager->canUndo());
    EXPECT_FALSE (undoManager->canRedo());
}

TEST_F (UndoManagerTest, NewEditAfterUndoInvalidatesRedo)
{
    undoManager->beginNewTransaction();
    undoManager->perform (new ToggleAction (counter));
    undoManager->beginNewTransaction();
    undoManager->perform (new ToggleAction (counter));

    undoManager->undo();
    EXPECT_EQ (counter, 1);
    EXPECT_TRUE (undoManager->canRedo());

    // A new edit must invalidate the redo-able history.
    undoManager->perform (new ToggleAction (counter));
    EXPECT_EQ (counter, 2);
    EXPECT_FALSE (undoManager->canRedo());
    EXPECT_FALSE (undoManager->redo());

    // Undoing now walks the new edit first, then the remaining original one.
    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 1);
    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 0);
    EXPECT_FALSE (undoManager->canUndo());
}

TEST_F (UndoManagerTest, EmptyTransactionsAreNotAddedToHistory)
{
    undoManager->beginNewTransaction();
    undoManager->beginNewTransaction();

    EXPECT_EQ (undoManager->getNumTransactions(), 1);

    EXPECT_FALSE (undoManager->undo());
    EXPECT_FALSE (undoManager->canUndo());
}

TEST_F (UndoManagerTest, UndoFlushesPendingTransactionBeforeUndoing)
{
    // No explicit beginNewTransaction: the action sits in a pending transaction.
    undoManager->perform (new ToggleAction (counter));
    EXPECT_EQ (undoManager->getNumTransactions(), 1);

    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 0);
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_TRUE (undoManager->canRedo());
}

TEST_F (UndoManagerTest, ClearResetsUndoRedoAndTransactionCount)
{
    undoManager->beginNewTransaction();
    undoManager->perform (new ToggleAction (counter));
    undoManager->beginNewTransaction();
    undoManager->perform (new ToggleAction (counter));

    undoManager->undo();
    EXPECT_TRUE (undoManager->canRedo());

    undoManager->clear();
    EXPECT_EQ (undoManager->getNumTransactions(), 0);
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_FALSE (undoManager->canRedo());
    EXPECT_FALSE (undoManager->undo());
    EXPECT_FALSE (undoManager->redo());
}

TEST_F (UndoManagerTest, ClearDropsPendingTransaction)
{
    undoManager->perform (new ToggleAction (counter)); // pending, never flushed
    EXPECT_TRUE (undoManager->canUndo());

    undoManager->clear();
    EXPECT_EQ (undoManager->getNumTransactions(), 0);
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_FALSE (undoManager->undo());
}

TEST_F (UndoManagerTest, DisablingClearsHistory)
{
    undoManager->beginNewTransaction();
    undoManager->perform (new ToggleAction (counter));

    undoManager->setEnabled (false);
    EXPECT_FALSE (undoManager->isEnabled());
    EXPECT_EQ (undoManager->getNumTransactions(), 0);
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_FALSE (undoManager->canRedo());
    EXPECT_FALSE (undoManager->undo());

    undoManager->setEnabled (true);
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_FALSE (undoManager->undo());
}

TEST_F (UndoManagerTest, TransactionNamesAreEmptyWhenOutOfRange)
{
    undoManager->beginNewTransaction ("only");
    undoManager->perform (new TestAction (actionFlag));

    EXPECT_EQ (String ("only"), undoManager->getTransactionName (0));
    EXPECT_TRUE (undoManager->getTransactionName (-1).isEmpty());
    EXPECT_TRUE (undoManager->getTransactionName (1).isEmpty());
    EXPECT_TRUE (undoManager->getTransactionName (99).isEmpty());
}

TEST_F (UndoManagerTest, SetCurrentTransactionNameWithoutTransactionIsNoOp)
{
    EXPECT_TRUE (undoManager->getCurrentTransactionName().isEmpty());

    undoManager->setCurrentTransactionName ("nameless");
    EXPECT_TRUE (undoManager->getCurrentTransactionName().isEmpty());
    EXPECT_TRUE (undoManager->getTransactionName (0).isEmpty());
}

TEST_F (UndoManagerTest, FailedRedoIsNotAddedToHistory)
{
    FailingAction::Ptr action = new FailingAction (actionFlag, UndoableActionState::Redo);

    EXPECT_FALSE (undoManager->perform (action));
    EXPECT_FALSE (actionFlag);
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_EQ (undoManager->getNumTransactions(), 0);
}

TEST_F (UndoManagerTest, FailedActionInTransactionIsDroppedButOthersUndo)
{
    undoManager->beginNewTransaction();

    ToggleAction::Ptr good = new ToggleAction (counter);
    FailingAction::Ptr bad = new FailingAction (actionFlag, UndoableActionState::Undo);

    undoManager->perform (good);
    undoManager->perform (bad);

    EXPECT_EQ (counter, 1);
    EXPECT_TRUE (actionFlag);

    // The failing action is dropped from the transaction; the good one still undoes.
    EXPECT_TRUE (undoManager->undo());
    EXPECT_EQ (counter, 0);
    EXPECT_TRUE (actionFlag);

    // Redo replays only the surviving action.
    EXPECT_TRUE (undoManager->redo());
    EXPECT_EQ (counter, 1);
}

TEST_F (UndoManagerTest, DeletedObjectLambdaActionCannotBeUndone)
{
    struct Object : ReferenceCountedObject
    {
    public:
        using Ptr = ReferenceCountedObjectPtr<Object>;

        int counter = 0;

    private:
        YUP_DECLARE_WEAK_REFERENCEABLE (Object)
    };

    auto lambdaAction = [] (Object::Ptr x, UndoableActionState s) -> bool
    {
        x->counter = (s == UndoableActionState::Undo) ? 1 : 2;
        return true;
    };

    Object::Ptr x = new Object;
    EXPECT_TRUE (undoManager->perform (x, lambdaAction));
    EXPECT_EQ (x->counter, 2);

    x = nullptr; // the referenced object is gone

    EXPECT_FALSE (undoManager->undo());
    EXPECT_EQ (undoManager->getNumTransactions(), 1);
}

TEST_F (UndoManagerTest, MaxHistorySizeOfZeroKeepsNoHistory)
{
    undoManager = std::make_unique<UndoManager> (0, RelativeTime::milliseconds (0));

    ToggleAction::Ptr action = new ToggleAction (counter);
    EXPECT_TRUE (undoManager->perform (action));
    EXPECT_EQ (counter, 1);

    EXPECT_TRUE (undoManager->canUndo()); // still pending
    EXPECT_FALSE (undoManager->undo());   // flushed then immediately trimmed away
    EXPECT_EQ (counter, 1);
    EXPECT_EQ (undoManager->getNumTransactions(), 0);
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_FALSE (undoManager->canRedo());
}

TEST_F (UndoManagerTest, EmptyScopedTransactionUndoesNothing)
{
    {
        UndoManager::ScopedTransaction transaction (*undoManager);
    }

    EXPECT_FALSE (undoManager->undo());
    EXPECT_FALSE (undoManager->canUndo());
}
