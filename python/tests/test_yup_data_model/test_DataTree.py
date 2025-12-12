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
