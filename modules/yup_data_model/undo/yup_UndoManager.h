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
/**
    @class UndoManager

    @brief Manages undo and redo functionality for a set of actions.

    The UndoManager class provides a way to manage undo and redo functionality
    for a set of actions. It allows you to perform actions, undo them, redo them,
    enable or disable the undo manager, and group actions together as a single action.

    To use the UndoManager, create an instance of the class and call the `perform`
    method to add actions to the timeline. You can also use the `undo` and `redo`
    methods to reverse or perform the action in the current timeline position.

    The UndoManager class also provides a ScopedActionIsolator helper class that
    allows you to group certain actions as a single action. This can be useful
    when you want to ensure that multiple actions are treated as a single unit
    for undo and redo purposes.

    @see UndoableAction
*/
class UndoManager : private Timer
{
public:
    //==============================================================================
    /**
        Creates a new UndoManager and starts the timer.
    */
    UndoManager();

    /**
        Creates a new UndoManager and starts the timer.

        @param maxHistorySize The maximum number of items to keep in the history.
    */
    UndoManager (int maxHistorySize);

    /**
        Creates a new UndoManager and starts the timer.

        @param actionGroupThreshold The time used to coalesce actions in the same transaction.
    */
    UndoManager (RelativeTime actionGroupThreshold);

    /**
        Creates a new UndoManager and starts the timer.

        @param maxHistorySize The maximum number of items to keep in the history.
        @param actionGroupThreshold The time used to coalesce actions in the same transaction.
    */
    UndoManager (int maxHistorySize, RelativeTime actionGroupThreshold);

    //==============================================================================
    /**
        Adds a new action to the timeline and performs its `redo` method.

        @param f The action to be performed.

        @return true if the action was added and performed successfully, false otherwise.
    */
    bool perform (UndoableAction::Ptr f);

    //==============================================================================
    /**
        Adds a new action to the timeline and performs its `redo` method.

        This method allows you to create an action using a weak referenceable object and a lambda
        that will be performed if the object is still alive.

        @tparam T The type of the object.

        @param object The object to be used in the action.
        @param f The lambda function to be performed.

        @return true if the action was added and performed successfully, false otherwise.
    */
    template <class T, class F>
    bool perform (T object, F&& function)
    {
        static_assert (std::is_base_of_v<ReferenceCountedObject, typename T::ReferencedType>);

        return perform (new Item<typename T::ReferencedType> (object, std::forward<F> (function)));
    }

    //==============================================================================
    /**
        Begins a new transaction.
    */
    void beginNewTransaction();

    /**
        Begins a new transaction with a given name.

        @param transactionName The name of the transaction.
    */
    void beginNewTransaction (StringRef transactionName);

    //==============================================================================
    /**
        Check if undo action can be performed.

        @return true if an undo action can be performed, false otherwise.
    */
    bool canUndo() const;

    /**
        Reverses the action in the current timeline position.

        @return true if an action was reversed, false otherwise.
    */
    bool undo();

    //==============================================================================
    /**
        Check if redo action can be performed.

        @return true if a redo action can be performed, false otherwise.
    */
    bool canRedo() const;

    /**
        Performs the action in the current timeline position.

        @return true if an action was performed, false otherwise.
    */
    bool redo();

    //==============================================================================
    /**
        Enables or disables the undo manager.

        @param shouldBeEnabled true to enable the undo manager, false to disable it.

        Disabling the undo manager will clear the history and stop the timer.
    */
    void setEnabled (bool shouldBeEnabled);

    /**
        Checks if the undo manager is enabled.

        @return true if the undo manager is enabled, false otherwise.
    */
    bool isEnabled() const;

    //==============================================================================
    /**
        Helper class to ensure that certain actions are grouped as a single action.

        By default, the undo manager groups all actions within a 500ms time window
        into one action. If you need to have a separate item in the timeline for
        certain actions, you can use this class.

        Example usage:

        @code
        void doSomething()
        {
            UndoManager::ScopedTransaction transaction (um);

            um.perform (action1);
            um.perform (action2);
        }
        @endcode
    */
    struct ScopedTransaction
    {
        /**
            Constructs a ScopedTransaction object.

            @param undoManager The UndoManager to be used.
        */
        ScopedTransaction (UndoManager& undoManager);

        /**
            Constructs a ScopedTransaction object.

            @param undoManager The UndoManager to be used.
            @param transactionName The name of the transaction.
        */
        ScopedTransaction (UndoManager& undoManager, StringRef transactionName);

        /**
            Destructs the ScopedTransaction object.
        */
        ~ScopedTransaction();

    private:
        UndoManager& undoManager;
    };

private:
    template <class T>
    struct Item : public UndoableAction
    {
        using PerformCallback = std::function<bool (typename T::Ptr, UndoableActionState)>;

        Item (typename T::Ptr object, PerformCallback function)
            : object (object)
            , function (std::move (function))
        {
            jassert (function != nullptr);
        }

        bool perform (UndoableActionState stateToPerform) override
        {
            if (object.wasObjectDeleted())
                return false;

            return function (*object, stateToPerform);
        }

        bool isEmpty() const override
        {
            return ! object.wasObjectDeleted();
        }

    private:
        WeakReference<T> object;
        PerformCallback function;
    };

    struct Transaction : public UndoableAction
    {
        using Ptr = ReferenceCountedObjectPtr<Transaction>;

        Transaction() = default;
        explicit Transaction (StringRef name);

        void add (UndoableAction::Ptr action);
        int size() const;

        String getTransactionName() const;
        void setTransactionName (StringRef newName);

        bool perform (UndoableActionState stateToPerform) override;
        bool isEmpty() const override;

    private:
        String transactionName;
        ReferenceCountedArray<UndoableAction> childItems;
    };

    /** @internal */
    void timerCallback() override;
    /** @internal */
    bool internalPerform (UndoableActionState stateToPerform);
    /** @internal */
    bool flushCurrentTransaction();

    int maxHistorySize;
    RelativeTime actionGroupThreshold;

    UndoableAction::List undoHistory;
    Transaction::Ptr currentTransaction;

    // the position in the timeline for the next actions.
    int nextUndoAction = -1;
    int nextRedoAction = -1;

    bool isUndoEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UndoManager)
};

} // namespace yup
