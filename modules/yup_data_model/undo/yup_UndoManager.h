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

        @param maxHistorySize The maximum number of items to keep in the history. Default is 100.
    */
    UndoManager (int maxHistorySize = 100);

    //==============================================================================
    /**
        Adds a new action to the timeline and performs its `redo` method.

        @param f The action to be performed.

        @return true if the action was added and performed successfully, false otherwise.
    */
    bool perform (UndoableAction::Ptr f);

    //==============================================================================
    /**
        @brief A type alias for an action callback function.

        This type alias represents a std::function that takes a reference to a WeakReferenceable object
        and an UndoableActionState object as parameters, and returns a boolean value.

        The callback function is used by the UndoManager to perform an undoable action on a WeakReferenceable
        object. It should return true if the action was successfully performed, and false otherwise.

        @tparam WeakReferenceable The type of the object that can be weakly referenced.

        @param referenceable A reference to the WeakReferenceable object on which the action should be performed.
        @param actionState The state of the undoable action.

        @return true if the action was successfully performed, false otherwise.
    */
    template <class WeakReferenceable>
    using ActionCallback = std::function<bool (WeakReferenceable&, UndoableActionState)>;

    /**
        Adds a new action to the timeline and performs its `redo` method.

        This method allows you to create an action using a weak referenceable object and a lambda
        that will be performed if the object is still alive.

        @tparam T The type of the object.

        @param obj The object to be used in the action.
        @param f The lambda function to be performed.

        @return true if the action was added and performed successfully, false otherwise.
    */
    template <class T>
    bool perform (T& obj, const ActionCallback<T>& f)
    {
        return perform (new Item<T> (obj, f));
    }

    //==============================================================================
    /**
        Reverses the action in the current timeline position.

        @return true if an action was reversed, false otherwise.
    */
    bool undo();

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
        Item (T& obj, std::function<bool (T&, UndoableActionState)> function)
            : object (std::addressof (object))
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

        bool isValid() const override
        {
            return ! object.wasObjectDeleted();
        }

    private:
        WeakReference<T> object;
        std::function<bool (T&, bool)> function;
    };

    struct CoalescedItem : public UndoableAction
    {
        using Ptr = ReferenceCountedObjectPtr<CoalescedItem>;

        CoalescedItem() = default;

        void add (UndoableAction::Ptr action);

        bool perform (UndoableActionState stateToPerform) override;
        bool isValid() const override;

    private:
        ReferenceCountedArray<UndoableAction> childItems;
    };

    /** @internal */
    void timerCallback() override;
    /** @internal */
    bool internalPerform (UndoableActionState stateToPerform);
    /** @internal */
    bool flushCurrentAction();

    int maxHistorySize;

    UndoableAction::List undoHistory;
    CoalescedItem::Ptr currentlyBuiltAction;

    // the position in the timeline for the next actions.
    int nextUndoAction = -1;
    int nextRedoAction = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UndoManager)
};

} // namespace yup
