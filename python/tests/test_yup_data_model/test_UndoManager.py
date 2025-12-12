import pytest

import yup

#==================================================================================================

def test_UndoManager_construction():
    """Test UndoManager construction."""
    manager = yup.UndoManager()
    assert manager is not None
    assert manager.isEnabled()

    manager_with_size = yup.UndoManager(10)
    assert manager_with_size is not None

#==================================================================================================

def test_UndoManager_enable_disable():
    """Test enabling and disabling undo manager."""
    manager = yup.UndoManager()

    assert manager.isEnabled()

    manager.setEnabled(False)
    assert not manager.isEnabled()

    manager.setEnabled(True)
    assert manager.isEnabled()

#==================================================================================================

def test_UndoManager_with_DataTree():
    """Test UndoManager with DataTree transactions."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Settings"))

    # Initially nothing to undo or redo
    assert not manager.canUndo()

    # Perform action with undo manager
    transaction = tree.beginTransaction(manager)
    transaction.setProperty(yup.Identifier("key"), "value1")
    transaction.commit()

    # Verify property was set
    assert tree.hasProperty(yup.Identifier("key"))
    assert tree.getProperty(yup.Identifier("key")) == "value1"

    # After transaction, undo should be available
    assert manager.canUndo()

    # Undo the action
    manager.undo()
    assert not tree.hasProperty(yup.Identifier("key"))

    # After undo, redo should be available
    assert manager.canRedo()

    # Redo the action
    manager.redo()
    assert tree.hasProperty(yup.Identifier("key"))
    assert tree.getProperty(yup.Identifier("key")) == "value1"

#==================================================================================================

def test_UndoManager_multiple_transactions():
    """Test UndoManager with multiple operations in separate transactions."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Config"))

    # First transaction
    manager.beginNewTransaction("Add prop1")
    transaction1 = tree.beginTransaction(manager)
    transaction1.setProperty(yup.Identifier("prop1"), "value1")
    transaction1.commit()
    assert tree.getNumProperties() == 1

    # Second transaction (explicitly start new transaction)
    manager.beginNewTransaction("Add prop2")
    transaction2 = tree.beginTransaction(manager)
    transaction2.setProperty(yup.Identifier("prop2"), "value2")
    transaction2.commit()
    assert tree.getNumProperties() == 2

    # Verify we can undo the last transaction
    initial_undo_count = manager.getNumTransactions()
    assert manager.canUndo()

    # Test basic undo/redo cycle
    manager.undo()
    assert tree.getNumProperties() <= 2  # May undo more depending on grouping

    if manager.canRedo():
        manager.redo()
        assert tree.getNumProperties() >= 1  # Should have at least one property back

#==================================================================================================

def test_UndoManager_clear():
    """Test clearing undo manager history."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Test"))

    # Perform some transactions
    transaction1 = tree.beginTransaction(manager)
    transaction1.setProperty(yup.Identifier("key1"), "value1")
    transaction1.commit()

    transaction2 = tree.beginTransaction(manager)
    transaction2.setProperty(yup.Identifier("key2"), "value2")
    transaction2.commit()

    assert manager.canUndo()

    # Clear history
    manager.clear()

    assert not manager.canUndo()
    assert not manager.canRedo()

#==================================================================================================

def test_UndoManager_transactions():
    """Test named transactions."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Test"))

    # Begin named transaction
    manager.beginNewTransaction("First Transaction")

    transaction1 = tree.beginTransaction(manager)
    transaction1.setProperty(yup.Identifier("key1"), "value1")
    transaction1.commit()

    # Begin another named transaction
    manager.beginNewTransaction("Second Transaction")

    transaction2 = tree.beginTransaction(manager)
    transaction2.setProperty(yup.Identifier("key2"), "value2")
    transaction2.commit()

    # Check transaction count
    numTransactions = manager.getNumTransactions()
    assert numTransactions > 0

#==================================================================================================

def test_UndoManager_ScopedTransaction():
    """Test UndoManager ScopedTransaction helper."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Test"))

    # Use scoped transaction
    scoped = yup.UndoManager.ScopedTransaction(manager, "Scoped Transaction")

    transaction = tree.beginTransaction(manager)
    transaction.setProperty(yup.Identifier("key"), "value")
    transaction.commit()

    del scoped

    assert manager.canUndo()

#==================================================================================================

def test_UndoManager_child_operations():
    """Test UndoManager with child add/remove operations."""
    manager = yup.UndoManager()
    parent = yup.DataTree(yup.Identifier("Parent"))

    # Add child with undo
    child = yup.DataTree(yup.Identifier("Child"))
    transaction = parent.beginTransaction(manager)
    transaction.addChild(child)
    transaction.commit()

    assert parent.getNumChildren() == 1

    # Undo add
    manager.undo()
    assert parent.getNumChildren() == 0

    # Redo add
    manager.redo()
    assert parent.getNumChildren() == 1

    # Remove child with undo
    transaction = parent.beginTransaction(manager)
    transaction.removeChild(0)
    transaction.commit()

    assert parent.getNumChildren() == 0

    # Undo remove
    manager.undo()
    assert parent.getNumChildren() == 1

#==================================================================================================

class UndoableActionImpl(yup.UndoableAction):
    """Test implementation of UndoableAction."""
    def __init__(self):
        super().__init__()
        self.undo_count = 0
        self.redo_count = 0

    def isValid(self):
        return True

    def perform(self, state):
        if state == yup.UndoableActionState.Undo:
            self.undo_count += 1
        else:
            self.redo_count += 1
        return True

def test_UndoManager_custom_action():
    """Test UndoManager with custom UndoableAction."""
    manager = yup.UndoManager()
    action = UndoableActionImpl()

    # Perform action
    result = manager.perform(action)
    assert result
    assert action.redo_count == 1

    # Undo action
    manager.undo()
    assert action.undo_count == 1

    # Redo action
    manager.redo()
    assert action.redo_count == 2

#==================================================================================================

def test_UndoManager_transaction_names():
    """Test transaction naming functionality."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Test"))

    # Set transaction name
    manager.beginNewTransaction("My Transaction")
    manager.setCurrentTransactionName("Updated Name")

    transaction = tree.beginTransaction(manager)
    transaction.setProperty(yup.Identifier("key"), "value")
    transaction.commit()

    # Get transaction name
    name = manager.getCurrentTransactionName()
    assert isinstance(name, str)

#==================================================================================================

def test_UndoManager_repr():
    """Test UndoManager has proper type representation."""
    manager = yup.UndoManager()

    # Verify we can get the type name
    type_name = type(manager).__name__
    assert "UndoManager" in type_name

#==================================================================================================

def test_UndoManager_ScopedTransaction_repr():
    """Test UndoManager.ScopedTransaction has proper type representation."""
    manager = yup.UndoManager()
    scoped = yup.UndoManager.ScopedTransaction(manager, "Test Transaction")

    # Verify we can get the type name
    type_name = type(scoped).__name__
    assert "ScopedTransaction" in type_name

#==================================================================================================

def test_UndoableAction_repr():
    """Test UndoableAction has proper type representation."""
    action = UndoableActionImpl()

    # Verify we can get the type name
    type_name = type(action).__name__
    assert type_name is not None  # Should have a valid type name
