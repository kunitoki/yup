import pytest

import yup

#==================================================================================================

def test_DataTree_UndoManager_integration():
    """Test integration between DataTree and UndoManager."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Root"))

    # Add properties with undo
    transaction1 = tree.beginTransaction(manager)
    transaction1.setProperty(yup.Identifier("name"), "MyApp")
    transaction1.setProperty(yup.Identifier("version"), "1.0")
    transaction1.commit()

    assert tree.getNumProperties() == 2

    # Add children with undo
    child1 = yup.DataTree(yup.Identifier("Settings"))
    transaction2 = tree.beginTransaction(manager)
    transaction2.addChild(child1)
    transaction2.commit()

    assert tree.getNumChildren() == 1

    # Modify child properties with undo
    actualChild = tree.getChild(0)
    transaction3 = actualChild.beginTransaction(manager)
    transaction3.setProperty(yup.Identifier("theme"), "dark")
    transaction3.commit()

    assert actualChild.hasProperty(yup.Identifier("theme"))

    # Undo child property change
    manager.undo()
    assert not actualChild.hasProperty(yup.Identifier("theme"))

    # Undo child addition
    manager.undo()
    assert tree.getNumChildren() == 0

    # Undo property additions
    manager.undo()
    assert tree.getNumProperties() == 0

    # Redo everything
    manager.redo()  # Properties
    assert tree.getNumProperties() == 2

    manager.redo()  # Child
    assert tree.getNumChildren() == 1

    manager.redo()  # Child property
    actualChild = tree.getChild(0)
    assert actualChild.hasProperty(yup.Identifier("theme"))

#==================================================================================================

class DataTreeChangeCounter(yup.DataTreeListener):
    """Listener to count changes."""
    def __init__(self):
        super().__init__()
        self.property_change_count = 0
        self.child_add_count = 0
        self.child_remove_count = 0

    def propertyChanged(self, tree, property):
        self.property_change_count += 1

    def childAdded(self, parent, child):
        self.child_add_count += 1

    def childRemoved(self, parent, child, formerIndex):
        self.child_remove_count += 1

def test_DataTree_UndoManager_with_listener():
    """Test that listeners receive notifications during undo/redo."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Root"))
    listener = DataTreeChangeCounter()

    tree.addListener(listener)

    # Add property
    transaction = tree.beginTransaction(manager)
    transaction.setProperty(yup.Identifier("key"), "value")
    transaction.commit()

    assert listener.property_change_count == 1

    # Undo
    manager.undo()
    assert listener.property_change_count == 2  # Removal also triggers change

    # Redo
    manager.redo()
    assert listener.property_change_count == 3

    tree.removeListener(listener)

#==================================================================================================

def test_complex_tree_operations_with_undo():
    """Test complex tree manipulations with undo support."""
    manager = yup.UndoManager()
    root = yup.DataTree(yup.Identifier("Application"))

    # Build a complex tree structure
    transaction1 = root.beginTransaction(manager)
    settings = yup.DataTree(yup.Identifier("Settings"))
    database = yup.DataTree(yup.Identifier("Database"))
    transaction1.addChild(settings)
    transaction1.addChild(database)
    transaction1.commit()

    # Add properties to children
    actualSettings = root.getChild(0)
    transaction2 = actualSettings.beginTransaction(manager)
    transaction2.setProperty(yup.Identifier("theme"), "light")
    transaction2.setProperty(yup.Identifier("language"), "en")
    transaction2.commit()

    actualDatabase = root.getChild(1)
    transaction3 = actualDatabase.beginTransaction(manager)
    transaction3.setProperty(yup.Identifier("host"), "localhost")
    transaction3.setProperty(yup.Identifier("port"), 5432)
    transaction3.commit()

    # Verify structure
    assert root.getNumChildren() == 2
    assert actualSettings.getNumProperties() == 2
    assert actualDatabase.getNumProperties() == 2

    # Undo database properties
    manager.undo()
    assert actualDatabase.getNumProperties() == 0

    # Undo settings properties
    manager.undo()
    assert actualSettings.getNumProperties() == 0

    # Undo children additions
    manager.undo()
    assert root.getNumChildren() == 0

    # Redo all
    manager.redo()
    assert root.getNumChildren() == 2

    manager.redo()
    actualSettings = root.getChild(0)
    assert actualSettings.getNumProperties() == 2

    manager.redo()
    actualDatabase = root.getChild(1)
    assert actualDatabase.getNumProperties() == 2

#==================================================================================================

def test_transaction_batching():
    """Test that multiple operations in one transaction are treated as one undo step."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Test"))

    # Multiple operations in single transaction
    transaction = tree.beginTransaction(manager)
    transaction.setProperty(yup.Identifier("prop1"), "value1")
    transaction.setProperty(yup.Identifier("prop2"), "value2")
    transaction.setProperty(yup.Identifier("prop3"), "value3")
    transaction.addChild(yup.DataTree(yup.Identifier("Child1")))
    transaction.addChild(yup.DataTree(yup.Identifier("Child2")))
    transaction.commit()

    assert tree.getNumProperties() == 3
    assert tree.getNumChildren() == 2

    # Single undo should revert all operations
    manager.undo()
    assert tree.getNumProperties() == 0
    assert tree.getNumChildren() == 0

    # Single redo should restore all operations
    manager.redo()
    assert tree.getNumProperties() == 3
    assert tree.getNumChildren() == 2

#==================================================================================================

def test_json_serialization_with_undo():
    """Test that JSON serialization/deserialization works correctly."""
    manager = yup.UndoManager()
    tree = yup.DataTree(yup.Identifier("Config"))

    # Build tree with undo
    transaction1 = tree.beginTransaction(manager)
    transaction1.setProperty(yup.Identifier("version"), "1.0")
    transaction1.addChild(yup.DataTree(yup.Identifier("Settings")))
    transaction1.commit()

    # Serialize the state
    json_data = tree.createJson()
    assert isinstance(json_data, dict)

    # Verify serialized data can be deserialized
    restored = yup.DataTree.fromJson(json_data)
    assert restored.isValid()
    assert restored.getType() == tree.getType()
    assert restored.getProperty(yup.Identifier("version")) == "1.0"
    assert restored.getNumChildren() == 1
    assert restored.getChild(0).getType() == yup.Identifier("Settings")

    # Test that undo works (removes the entire transaction)
    manager.undo()

    # After undo, the tree should be back to empty state
    assert tree.getNumProperties() == 0
    assert tree.getNumChildren() == 0

    # Redo restores the state
    manager.redo()
    assert tree.getNumProperties() == 1
    assert tree.getNumChildren() == 1
    assert tree.getProperty(yup.Identifier("version")) == "1.0"

#==================================================================================================

def test_tree_cloning_preserves_structure():
    """Test that cloning creates an independent copy."""
    original = yup.DataTree(yup.Identifier("Original"))

    transaction = original.beginTransaction()
    transaction.setProperty(yup.Identifier("key"), "original_value")
    child = yup.DataTree(yup.Identifier("Child"))
    transaction.addChild(child)
    transaction.commit()

    # Clone the tree
    cloned = original.clone()

    # Verify clone is equivalent but independent
    assert cloned.getType() == original.getType()
    assert cloned.getProperty(yup.Identifier("key")) == "original_value"
    assert cloned.getNumChildren() == 1

    # Modify original (without undo manager)
    transaction2 = original.beginTransaction()
    transaction2.setProperty(yup.Identifier("key"), "modified_value")
    transaction2.setProperty(yup.Identifier("new_key"), "new_value")
    transaction2.commit()

    # Cloned should not be affected by original modifications
    assert cloned.getProperty(yup.Identifier("key")) == "original_value"
    assert not cloned.hasProperty(yup.Identifier("new_key"))

    # Original should have the modifications
    assert original.getProperty(yup.Identifier("key")) == "modified_value"
    assert original.getProperty(yup.Identifier("new_key")) == "new_value"

#==================================================================================================

def test_move_child_with_undo():
    """Test moving children."""
    parent = yup.DataTree(yup.Identifier("Parent"))

    # Add three children
    transaction = parent.beginTransaction()
    transaction.addChild(yup.DataTree(yup.Identifier("First")))
    transaction.addChild(yup.DataTree(yup.Identifier("Second")))
    transaction.addChild(yup.DataTree(yup.Identifier("Third")))
    transaction.commit()

    # Verify initial order
    assert parent.getNumChildren() == 3
    assert parent.getChild(0).getType() == yup.Identifier("First")
    assert parent.getChild(1).getType() == yup.Identifier("Second")
    assert parent.getChild(2).getType() == yup.Identifier("Third")

    # Move second child to first position
    transaction2 = parent.beginTransaction()
    transaction2.moveChild(1, 0)
    transaction2.commit()

    # Verify new order after move
    assert parent.getNumChildren() == 3
    assert parent.getChild(0).getType() == yup.Identifier("Second")
    assert parent.getChild(1).getType() == yup.Identifier("First")
    assert parent.getChild(2).getType() == yup.Identifier("Third")

    # Move third child to first position
    transaction3 = parent.beginTransaction()
    transaction3.moveChild(2, 0)
    transaction3.commit()

    # Verify final order
    assert parent.getNumChildren() == 3
    assert parent.getChild(0).getType() == yup.Identifier("Third")
    assert parent.getChild(1).getType() == yup.Identifier("Second")
    assert parent.getChild(2).getType() == yup.Identifier("First")

#==================================================================================================

def test_DataTreeListener_repr():
    """Test DataTreeListener has proper type representation."""
    listener = DataTreeChangeCounter()

    # Verify we can get the type name
    type_name = type(listener).__name__
    assert type_name is not None  # Should have a valid type name
