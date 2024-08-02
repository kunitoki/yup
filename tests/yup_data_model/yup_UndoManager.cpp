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

    TestAction (bool& flag) : flag (flag) {}

    bool perform (UndoableActionState) override
    {
        flag = !flag;
        return true;
    }

    bool isEmpty() const override
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

    ToggleAction (int& counter) : counter (counter) {}

    bool perform (UndoableActionState state) override
    {
        if (state == UndoableActionState::Redo)
            ++counter;
        else if (state == UndoableActionState::Undo)
            --counter;

        return true;
    }

    bool isEmpty() const override
    {
        return true;
    }

private:
    int& counter;
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

TEST_F(UndoManagerTest, PerformAction)
{
    TestAction::Ptr action = new TestAction(actionFlag);
    EXPECT_TRUE(undoManager->perform(action));
    EXPECT_TRUE(actionFlag);
}

TEST_F(UndoManagerTest, UndoAction)
{
    TestAction::Ptr action = new TestAction(actionFlag);
    undoManager->perform(action);
    EXPECT_TRUE(actionFlag);

    EXPECT_TRUE(undoManager->undo());
    EXPECT_FALSE(actionFlag);
}

TEST_F(UndoManagerTest, RedoAction)
{
    TestAction::Ptr action = new TestAction(actionFlag);
    undoManager->perform(action);
    EXPECT_TRUE(actionFlag);

    undoManager->undo();
    EXPECT_FALSE(actionFlag);

    EXPECT_TRUE(undoManager->redo());
    EXPECT_TRUE(actionFlag);
}

TEST_F(UndoManagerTest, SetEnabled)
{
    undoManager->setEnabled(false);
    EXPECT_FALSE(undoManager->isEnabled());

    TestAction::Ptr action = new TestAction(actionFlag);
    EXPECT_FALSE(undoManager->perform(action));
    EXPECT_FALSE(actionFlag);

    undoManager->setEnabled(true);
    EXPECT_TRUE(undoManager->isEnabled());
    EXPECT_TRUE(undoManager->perform(action));
    EXPECT_TRUE(actionFlag);
}

TEST_F(UndoManagerTest, ScopedTransaction)
{
    actionFlag = false;

    {
        UndoManager::ScopedTransaction transaction (*undoManager);

        TestAction::Ptr action1 = new TestAction(actionFlag);
        undoManager->perform(action1);
        EXPECT_TRUE(actionFlag);

        TestAction::Ptr action2 = new TestAction(actionFlag);
        undoManager->perform(action2);
        EXPECT_FALSE(actionFlag);
    }

    EXPECT_TRUE(undoManager->undo());
    EXPECT_FALSE(actionFlag);
}

TEST_F(UndoManagerTest, PerformWithLambda)
{
    struct Object : ReferenceCountedObject
    {
    public:
        using Ptr = ReferenceCountedObjectPtr<Object>;

        int counter = 0;

    private:
        JUCE_DECLARE_WEAK_REFERENCEABLE(Object)
    };

    auto lambdaAction = [](Object::Ptr x, UndoableActionState s) -> bool
    {
        x->counter = (s == UndoableActionState::Undo) ? 1 : 2;
        return true;
    };

    Object::Ptr x = new Object;
    EXPECT_TRUE(undoManager->perform (x, lambdaAction));
    EXPECT_EQ(x->counter, 2);

    EXPECT_TRUE(undoManager->undo());
    EXPECT_EQ(x->counter, 1);

    EXPECT_TRUE(undoManager->redo());
    EXPECT_EQ(x->counter, 2);
}

TEST_F(UndoManagerTest, ComplexPerformUndoRedo)
{
    ToggleAction::Ptr action1 = new ToggleAction(counter);
    ToggleAction::Ptr action2 = new ToggleAction(counter);

    undoManager->beginNewTransaction();
    EXPECT_TRUE(undoManager->perform(action1));
    EXPECT_EQ(counter, 1);

    undoManager->beginNewTransaction();
    EXPECT_TRUE(undoManager->perform(action2));
    EXPECT_EQ(counter, 2);

    EXPECT_TRUE(undoManager->undo());
    EXPECT_EQ(counter, 1);

    EXPECT_TRUE(undoManager->undo());
    EXPECT_EQ(counter, 0);

    EXPECT_TRUE(undoManager->redo());
    EXPECT_EQ(counter, 1);

    EXPECT_TRUE(undoManager->redo());
    EXPECT_EQ(counter, 2);
}

TEST_F(UndoManagerTest, RedoWithoutUndo)
{
    ToggleAction::Ptr action = new ToggleAction(counter);
    EXPECT_TRUE(undoManager->perform(action));
    EXPECT_EQ(counter, 1);

    EXPECT_FALSE(undoManager->redo());
    EXPECT_EQ(counter, 1);
}

TEST_F(UndoManagerTest, UndoWithoutPerform)
{
    EXPECT_FALSE(undoManager->undo());
}

TEST_F(UndoManagerTest, RedoAfterDisableEnable)
{
    ToggleAction::Ptr action = new ToggleAction(counter);
    EXPECT_TRUE(undoManager->perform(action));
    EXPECT_EQ(counter, 1);

    undoManager->undo();
    EXPECT_EQ(counter, 0);

    undoManager->setEnabled(false);
    EXPECT_FALSE(undoManager->redo());
    EXPECT_EQ(counter, 0);

    undoManager->setEnabled(true);
    EXPECT_FALSE(undoManager->redo());
    EXPECT_EQ(counter, 0);
}

TEST_F(UndoManagerTest, MaxHistorySize)
{
    undoManager = std::make_unique<UndoManager>(2, RelativeTime::milliseconds(0));

    ToggleAction::Ptr action1 = new ToggleAction(counter);
    ToggleAction::Ptr action2 = new ToggleAction(counter);
    ToggleAction::Ptr action3 = new ToggleAction(counter);

    undoManager->beginNewTransaction();
    EXPECT_TRUE(undoManager->perform(action1));
    EXPECT_EQ(counter, 1);

    undoManager->beginNewTransaction();
    EXPECT_TRUE(undoManager->perform(action2));
    EXPECT_EQ(counter, 2);

    undoManager->beginNewTransaction();
    EXPECT_TRUE(undoManager->perform(action3));
    EXPECT_EQ(counter, 3);

    EXPECT_TRUE(undoManager->undo());
    EXPECT_EQ(counter, 2);

    EXPECT_TRUE(undoManager->undo());
    EXPECT_EQ(counter, 1);

    EXPECT_FALSE(undoManager->undo()); // action1 should be removed due to max history size
    EXPECT_EQ(counter, 1);
}

TEST_F(UndoManagerTest, ScopedTransactionGrouping)
{
    {
        UndoManager::ScopedTransaction transaction (*undoManager);

        ToggleAction::Ptr action1 = new ToggleAction(counter);
        ToggleAction::Ptr action2 = new ToggleAction(counter);
        
        undoManager->perform(action1);
        EXPECT_EQ(counter, 1);
        
        undoManager->perform(action2);
        EXPECT_EQ(counter, 2);
    }

    EXPECT_EQ(counter, 2);

    EXPECT_TRUE(undoManager->undo());
    EXPECT_EQ(counter, 0);
}

TEST_F(UndoManagerTest, DISABLED_NestedScopedTransactions)
{
    {
        UndoManager::ScopedTransaction transaction(*undoManager);

        ToggleAction::Ptr action1 = new ToggleAction(counter);
        EXPECT_TRUE(undoManager->perform(action1));
        EXPECT_EQ(counter, 1);

        {
            UndoManager::ScopedTransaction nestedTransaction(*undoManager);

            ToggleAction::Ptr action2 = new ToggleAction(counter);
            EXPECT_TRUE(undoManager->perform(action2));
            EXPECT_EQ(counter, 2);
        }

        ToggleAction::Ptr action3 = new ToggleAction(counter);
        EXPECT_TRUE(undoManager->perform(action3));
        EXPECT_EQ(counter, 3);
    }

    EXPECT_EQ(counter, 3);

    EXPECT_TRUE(undoManager->undo());
    EXPECT_EQ(counter, 0);
}
