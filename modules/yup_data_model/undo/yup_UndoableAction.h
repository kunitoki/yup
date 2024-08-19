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
    @enum ActionState

    Represents the state of an action in the undo/redo system.

    The ActionState enum is used to indicate whether an action should be undone or redone.

    @see UndoManager
*/
enum class UndoableActionState
{
    Undo, /**< Indicates that the action should be undone. */
    Redo  /**< Indicates that the action should be redone. */
};

//==============================================================================
/**
    @class UndoableAction

    The base class for all actions in the timeline.

    You can subclass from this class to define your actions,
    but a better way is to just use a lambda with a WeakReferenceable object.
*/
class UndoableAction : public ReferenceCountedObject
{
public:
    using Array = ReferenceCountedArray<UndoableAction>;
    using Ptr = ReferenceCountedObjectPtr<UndoableAction>;

    /**
        @brief Destructor.
    */
    ~UndoableAction() override {}

    /**
        @brief Checks if the action is valid.

        This should return true if the action is invalidated (e.g., because the object it operates on was deleted).

        @return True if the action is valid, false otherwise.
    */
    virtual bool isValid() const = 0;

    /**
        @brief Performs the undo action.

        This function performs the undo action based on the given state.

        @param stateToPerform The state of the action to perform (Undo or Redo).

        @return True if the undo action was successful, false otherwise.
    */
    virtual bool perform (UndoableActionState stateToPerform) = 0;
};

} // namespace yup
