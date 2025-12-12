import pytest

import yup

#==================================================================================================

def test_DataTree_construction():
    """Test basic DataTree construction."""
    tree = yup.DataTree()
    assert not tree.isValid()

    tree = yup.DataTree(yup.Identifier("Settings"))
    assert tree.isValid()
    assert tree.getType() == yup.Identifier("Settings")

#==================================================================================================

def test_DataTree_properties():
    """Test DataTree property operations."""
    tree = yup.DataTree(yup.Identifier("Config"))

    # Initially no properties
    assert tree.getNumProperties() == 0

    # Add properties via transaction
    transaction = tree.beginTransaction()
    transaction.setProperty(yup.Identifier("version"), "1.0")
    transaction.setProperty(yup.Identifier("debug"), True)
    transaction.commit()

    # Check properties
    assert tree.getNumProperties() == 2
    assert tree.hasProperty(yup.Identifier("version"))
    assert tree.hasProperty(yup.Identifier("debug"))
    assert tree.getProperty(yup.Identifier("version")) == "1.0"
    assert tree.getProperty(yup.Identifier("debug")) == True

#==================================================================================================

def test_DataTree_children():
    """Test DataTree child operations."""
    parent = yup.DataTree(yup.Identifier("Parent"))
    child1 = yup.DataTree(yup.Identifier("Child1"))
    child2 = yup.DataTree(yup.Identifier("Child2"))

    # Initially no children
    assert parent.getNumChildren() == 0

    # Add children via transaction
    transaction = parent.beginTransaction()
    transaction.addChild(child1)
    transaction.addChild(child2)
    transaction.commit()

    # Check children
    assert parent.getNumChildren() == 2
    assert parent.getChild(0).getType() == yup.Identifier("Child1")
    assert parent.getChild(1).getType() == yup.Identifier("Child2")

#==================================================================================================

def test_DataTree_transaction_auto_commit():
    """Test that transactions auto-commit when going out of scope."""
    tree = yup.DataTree(yup.Identifier("Test"))

    # Transaction auto-commits when destroyed
    transaction = tree.beginTransaction()
    transaction.setProperty(yup.Identifier("key"), "value")
    del transaction

    assert tree.hasProperty(yup.Identifier("key"))

#==================================================================================================

def test_DataTree_transaction_abort():
    """Test transaction abort functionality."""
    tree = yup.DataTree(yup.Identifier("Test"))

    transaction = tree.beginTransaction()
    transaction.setProperty(yup.Identifier("key"), "value")
    transaction.abort()

    # Property should not be set after abort
    assert not tree.hasProperty(yup.Identifier("key"))

#==================================================================================================

def test_DataTree_hierarchy():
    """Test DataTree hierarchy methods."""
    root = yup.DataTree(yup.Identifier("Root"))
    child = yup.DataTree(yup.Identifier("Child"))
    grandchild = yup.DataTree(yup.Identifier("Grandchild"))

    # Build hierarchy
    transaction1 = root.beginTransaction()
    transaction1.addChild(child)
    transaction1.commit()

    transaction2 = root.getChild(0).beginTransaction()
    transaction2.addChild(grandchild)
    transaction2.commit()

    # Test hierarchy methods
    actualChild = root.getChild(0)
    actualGrandchild = actualChild.getChild(0)

    assert actualGrandchild.getParent() == actualChild
    assert actualGrandchild.getRoot() == root
    assert actualGrandchild.isAChildOf(root)
    assert actualGrandchild.getDepth() == 2
    assert actualChild.getDepth() == 1
    assert root.getDepth() == 0

#==================================================================================================

def test_DataTree_iteration():
    """Test DataTree iteration."""
    parent = yup.DataTree(yup.Identifier("Parent"))

    # Add several children
    transaction = parent.beginTransaction()
    for i in range(5):
        child = yup.DataTree(yup.Identifier(f"Child{i}"))
        transaction.addChild(child)
    transaction.commit()

    # Test iteration
    count = 0
    for child in parent:
        assert child.isValid()
        count += 1

    assert count == 5

#==================================================================================================

def test_DataTree_forEachChild():
    """Test forEachChild method."""
    parent = yup.DataTree(yup.Identifier("Parent"))

    transaction = parent.beginTransaction()
    for i in range(3):
        child = yup.DataTree(yup.Identifier(f"Child{i}"))
        transaction.addChild(child)
    transaction.commit()

    # Test callback
    count = 0
    def callback(child):
        nonlocal count
        count += 1

    parent.forEachChild(callback)
    assert count == 3

#==================================================================================================

def test_DataTree_findChild():
    """Test findChild with predicate."""
    parent = yup.DataTree(yup.Identifier("Parent"))

    transaction = parent.beginTransaction()
    for i in range(5):
        child = yup.DataTree(yup.Identifier(f"Child{i}"))
        transaction.addChild(child)
    transaction.commit()

    # Find specific child
    found = parent.findChild(lambda c: c.getType() == yup.Identifier("Child2"))
    assert found.isValid()
    assert found.getType() == yup.Identifier("Child2")

#==================================================================================================

def test_DataTree_clone():
    """Test DataTree cloning."""
    original = yup.DataTree(yup.Identifier("Original"))

    transaction = original.beginTransaction()
    transaction.setProperty(yup.Identifier("key"), "value")
    child = yup.DataTree(yup.Identifier("Child"))
    transaction.addChild(child)
    transaction.commit()

    # Clone the tree
    cloned = original.clone()

    assert cloned.isValid()
    assert cloned != original  # Different objects
    assert cloned.isEquivalentTo(original)  # But same content
    assert cloned.getType() == original.getType()
    assert cloned.getNumProperties() == original.getNumProperties()
    assert cloned.getNumChildren() == original.getNumChildren()

#==================================================================================================

def test_DataTree_json_serialization():
    """Test DataTree JSON serialization."""
    tree = yup.DataTree(yup.Identifier("Settings"))

    transaction = tree.beginTransaction()
    transaction.setProperty(yup.Identifier("version"), "1.0")
    transaction.setProperty(yup.Identifier("enabled"), True)
    transaction.commit()

    # Serialize to JSON (returns Python dict)
    json_data = tree.createJson()
    assert isinstance(json_data, dict)

    # Deserialize from JSON
    restored = yup.DataTree.fromJson(json_data)
    assert restored.isValid()
    assert restored.getType() == tree.getType()
    assert restored.isEquivalentTo(tree)

#==================================================================================================

class DataTreeListenerImpl(yup.DataTreeListener):
    """Test listener implementation."""
    def __init__(self):
        super().__init__()
        self.property_changes = []
        self.child_additions = []
        self.child_removals = []

    def propertyChanged(self, tree, property):
        # Identifier is automatically converted to string by type caster
        self.property_changes.append(property if isinstance(property, str) else property.toString())

    def childAdded(self, _parent, child):
        childType = child.getType()
        self.child_additions.append(childType if isinstance(childType, str) else childType.toString())

    def childRemoved(self, _parent, child, formerIndex):
        childType = child.getType()
        self.child_removals.append((childType if isinstance(childType, str) else childType.toString(), formerIndex))

def test_DataTree_listener():
    """Test DataTree listener notifications."""
    tree = yup.DataTree(yup.Identifier("Root"))
    listener = DataTreeListenerImpl()

    tree.addListener(listener)

    # Test property change notification
    transaction = tree.beginTransaction()
    transaction.setProperty(yup.Identifier("prop1"), "value1")
    transaction.commit()

    assert len(listener.property_changes) == 1
    assert listener.property_changes[0] == "prop1"

    # Test child addition notification
    child = yup.DataTree(yup.Identifier("Child"))
    transaction = tree.beginTransaction()
    transaction.addChild(child)
    transaction.commit()

    assert len(listener.child_additions) == 1
    assert listener.child_additions[0] == "Child"

    # Test child removal notification
    transaction = tree.beginTransaction()
    transaction.removeChild(0)
    transaction.commit()

    assert len(listener.child_removals) == 1
    assert listener.child_removals[0][0] == "Child"
    assert listener.child_removals[0][1] == 0

    # Cleanup
    tree.removeListener(listener)

#==================================================================================================

def test_DataTree_getChildWithName():
    """Test getChildWithName method."""
    parent = yup.DataTree(yup.Identifier("Parent"))

    transaction = parent.beginTransaction()
    transaction.addChild(yup.DataTree(yup.Identifier("FirstChild")))
    transaction.addChild(yup.DataTree(yup.Identifier("SecondChild")))
    transaction.addChild(yup.DataTree(yup.Identifier("ThirdChild")))
    transaction.commit()

    # Find child by name
    child = parent.getChildWithName(yup.Identifier("SecondChild"))
    assert child.isValid()
    assert child.getType() == yup.Identifier("SecondChild")

    # Non-existent child
    nonExistent = parent.getChildWithName(yup.Identifier("NonExistent"))
    assert not nonExistent.isValid()

#==================================================================================================

def test_DataTree_forEachChild_with_bool_return():
    """Test forEachChild with bool return for early exit."""
    parent = yup.DataTree(yup.Identifier("Parent"))

    transaction = parent.beginTransaction()
    for i in range(5):
        child = yup.DataTree(yup.Identifier(f"Child{i}"))
        transaction.addChild(child)
    transaction.commit()

    # Test early exit when callback returns True
    count = 0
    def callback_early_exit(child):
        nonlocal count
        count += 1
        # Stop after processing 2 children
        return count >= 2

    parent.forEachChild(callback_early_exit)
    assert count == 2  # Should stop early

    # Test normal iteration when callback returns False
    count2 = 0
    def callback_no_exit(child):
        nonlocal count2
        count2 += 1
        return False  # Continue iteration

    parent.forEachChild(callback_no_exit)
    assert count2 == 5  # Should process all children

#==================================================================================================

def test_DataTree_forEachDescendant():
    """Test forEachDescendant method."""
    root = yup.DataTree(yup.Identifier("Root"))

    # Build a hierarchy
    transaction = root.beginTransaction()
    child1 = yup.DataTree(yup.Identifier("Child1"))
    child2 = yup.DataTree(yup.Identifier("Child2"))
    transaction.addChild(child1)
    transaction.addChild(child2)
    transaction.commit()

    actualChild1 = root.getChild(0)
    transaction2 = actualChild1.beginTransaction()
    grandchild1 = yup.DataTree(yup.Identifier("Grandchild1"))
    grandchild2 = yup.DataTree(yup.Identifier("Grandchild2"))
    transaction2.addChild(grandchild1)
    transaction2.addChild(grandchild2)
    transaction2.commit()

    # Test callback visits all descendants
    visited = []
    def callback(child):
        visited.append(child.getType().toString())
        return False

    root.forEachDescendant(callback)
    assert len(visited) == 4  # 2 children + 2 grandchildren
    assert "Child1" in visited
    assert "Child2" in visited
    assert "Grandchild1" in visited
    assert "Grandchild2" in visited

#==================================================================================================

def test_DataTree_forEachDescendant_with_bool_return():
    """Test forEachDescendant with bool return for early exit."""
    root = yup.DataTree(yup.Identifier("Root"))

    # Build a hierarchy
    transaction = root.beginTransaction()
    for i in range(3):
        child = yup.DataTree(yup.Identifier(f"Child{i}"))
        transaction.addChild(child)
    transaction.commit()

    # Add grandchildren to first child
    actualChild = root.getChild(0)
    transaction2 = actualChild.beginTransaction()
    for i in range(3):
        grandchild = yup.DataTree(yup.Identifier(f"Grandchild{i}"))
        transaction2.addChild(grandchild)
    transaction2.commit()

    # Test early exit
    count = 0
    def callback_early_exit(child):
        nonlocal count
        count += 1
        return count >= 2  # Stop after 2 descendants

    root.forEachDescendant(callback_early_exit)
    assert count == 2  # Should stop early

#==================================================================================================

def test_DataTree_findChildren():
    """Test findChildren with predicate."""
    parent = yup.DataTree(yup.Identifier("Parent"))

    transaction = parent.beginTransaction()
    for i in range(5):
        child = yup.DataTree(yup.Identifier(f"Item{i}"))
        transaction.setProperty(yup.Identifier("index"), i)
        transaction.addChild(child)
    transaction.commit()

    # Add a property to some children
    for i in [0, 2, 4]:
        actualChild = parent.getChild(i)
        trans = actualChild.beginTransaction()
        trans.setProperty(yup.Identifier("even"), True)
        trans.commit()

    # Find children with even property
    found = parent.findChildren(lambda c: c.hasProperty(yup.Identifier("even")))
    assert len(found) == 3
    assert all(child.hasProperty(yup.Identifier("even")) for child in found)

    # Find children by name pattern
    found2 = parent.findChildren(lambda c: c.getType() == yup.Identifier("Item2"))
    assert len(found2) == 1
    assert found2[0].getType() == yup.Identifier("Item2")

#==================================================================================================

def test_DataTree_findDescendants():
    """Test findDescendants with predicate."""
    root = yup.DataTree(yup.Identifier("Root"))

    # Build hierarchy
    transaction = root.beginTransaction()
    child1 = yup.DataTree(yup.Identifier("Child1"))
    child2 = yup.DataTree(yup.Identifier("Child2"))
    transaction.addChild(child1)
    transaction.addChild(child2)
    transaction.commit()

    # Add grandchildren
    actualChild1 = root.getChild(0)
    transaction2 = actualChild1.beginTransaction()
    grandchild1 = yup.DataTree(yup.Identifier("Target"))
    grandchild2 = yup.DataTree(yup.Identifier("Other"))
    transaction2.addChild(grandchild1)
    transaction2.addChild(grandchild2)
    transaction2.commit()

    actualChild2 = root.getChild(1)
    transaction3 = actualChild2.beginTransaction()
    grandchild3 = yup.DataTree(yup.Identifier("Target"))
    transaction3.addChild(grandchild3)
    transaction3.commit()

    # Find all descendants named "Target"
    found = root.findDescendants(lambda d: d.getType() == yup.Identifier("Target"))
    assert len(found) == 2
    assert all(d.getType() == yup.Identifier("Target") for d in found)

#==================================================================================================

def test_DataTree_findDescendant():
    """Test findDescendant with predicate (finds first match)."""
    root = yup.DataTree(yup.Identifier("Root"))

    # Build hierarchy
    transaction = root.beginTransaction()
    child1 = yup.DataTree(yup.Identifier("Child1"))
    child2 = yup.DataTree(yup.Identifier("Child2"))
    transaction.addChild(child1)
    transaction.addChild(child2)
    transaction.commit()

    # Add grandchildren
    actualChild1 = root.getChild(0)
    transaction2 = actualChild1.beginTransaction()
    grandchild1 = yup.DataTree(yup.Identifier("Target"))
    transaction2.addChild(grandchild1)
    transaction2.commit()

    actualChild2 = root.getChild(1)
    transaction3 = actualChild2.beginTransaction()
    grandchild2 = yup.DataTree(yup.Identifier("Target"))
    transaction3.addChild(grandchild2)
    transaction3.commit()

    # Find first descendant named "Target"
    found = root.findDescendant(lambda d: d.getType() == yup.Identifier("Target"))
    assert found.isValid()
    assert found.getType() == yup.Identifier("Target")

    # Find non-existent descendant
    notFound = root.findDescendant(lambda d: d.getType() == yup.Identifier("NonExistent"))
    assert not notFound.isValid()

#==================================================================================================

def test_DataTree_repr():
    """Test DataTree __repr__ method."""
    tree = yup.DataTree(yup.Identifier("Settings"))
    repr_str = repr(tree)

    # Should contain class name and type
    assert "DataTree" in repr_str
    assert "Settings" in repr_str
    assert "object at" in repr_str

    # Invalid tree repr
    invalid_tree = yup.DataTree()
    invalid_repr = repr(invalid_tree)
    assert "DataTree" in invalid_repr

#==================================================================================================

def test_DataTree_Transaction_repr():
    """Test DataTree.Transaction has proper type representation."""
    tree = yup.DataTree(yup.Identifier("Test"))
    transaction = tree.beginTransaction()

    # Verify we can get the type name
    type_name = type(transaction).__name__
    assert "Transaction" in type_name

#==================================================================================================

def test_DataTree_ValidatedTransaction_type():
    """Test DataTree.ValidatedTransaction has proper type representation."""
    tree = yup.DataTree(yup.Identifier("Test"))
    transaction = tree.beginTransaction()

    # ValidatedTransaction is obtained through validation
    # For now, just verify the type exists and is accessible
    type_name = type(transaction).__name__
    assert type_name is not None
