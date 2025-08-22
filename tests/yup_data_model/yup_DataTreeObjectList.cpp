/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

namespace
{

//==============================================================================
// Test object using CachedValue for property management

class TestObject
{
public:
    TestObject (const DataTree& tree)
        : name (tree, "name", "")
        , enabled (tree, "enabled", true)
        , treeReference (tree)
    {
        constructorCallCount++;
    }

    ~TestObject()
    {
        destructorCallCount++;
    }

    DataTree getDataTree() const { return treeReference; }

    String getName() const { return name.get(); }

    void setName (const String& newName) { name.set (newName); }

    bool isEnabled() const { return enabled.get(); }

    void setEnabled (bool newEnabled) { enabled.set (newEnabled); }

    static int constructorCallCount;
    static int destructorCallCount;

    static void resetCounts() { constructorCallCount = destructorCallCount = 0; }

private:
    CachedValue<String> name;
    CachedValue<bool> enabled;
    DataTree treeReference;
};

int TestObject::constructorCallCount = 0;
int TestObject::destructorCallCount = 0;

//==============================================================================
// Example DataTreeObjectList implementation

class TestObjectList : public DataTreeObjectList<TestObject>
{
public:
    TestObjectList (const DataTree& parent)
        : DataTreeObjectList<TestObject> (parent)
    {
        rebuildObjects();
    }

    ~TestObjectList()
    {
        freeObjects();
    }

    bool isSuitableType (const DataTree& tree) const override
    {
        return tree.hasProperty ("name");
    }

    TestObject* createNewObject (const DataTree& tree) override
    {
        return new TestObject (tree);
    }

    void deleteObject (TestObject* obj) override
    {
        delete obj;
    }

    void newObjectAdded (TestObject* object) override
    {
        addedObjects.push_back (object->getName());
    }

    void objectRemoved (TestObject* object) override
    {
        removedObjects.push_back (object->getName());
    }

    void objectOrderChanged() override
    {
        orderChangedCount++;
    }

    std::vector<String> addedObjects;
    std::vector<String> removedObjects;
    int orderChangedCount = 0;
};

} // namespace

//==============================================================================
class DataTreeObjectListTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        TestObject::resetCounts();
        rootTree = DataTree ("Root");
    }

    void TearDown() override
    {
        rootTree = DataTree();
    }

    DataTree rootTree;
};

//==============================================================================
TEST_F (DataTreeObjectListTests, BasicUsage)
{
    // Create an object list that monitors the root tree
    TestObjectList objectList (rootTree);

    // Initially empty
    EXPECT_EQ (0, objectList.objects.size());
    EXPECT_EQ (0, TestObject::constructorCallCount);

    // Add some objects to the DataTree
    DataTree obj1 ("Object");
    DataTree obj2 ("Object");

    {
        auto transaction1 = obj1.beginTransaction ("Setup Object 1");
        transaction1.setProperty ("name", "Button1");
    }
    {
        auto transaction2 = obj2.beginTransaction ("Setup Object 2");
        transaction2.setProperty ("name", "Label1");
    }
    {
        auto rootTransaction = rootTree.beginTransaction ("Add Objects");
        rootTransaction.addChild (obj1);
        rootTransaction.addChild (obj2);
    }

    // Objects should be automatically created
    EXPECT_EQ (2, objectList.objects.size());
    EXPECT_EQ (2, TestObject::constructorCallCount);

    // Check object properties via getter methods
    EXPECT_EQ ("Button1", objectList.objects[0]->getName());
    EXPECT_EQ ("Label1", objectList.objects[1]->getName());

    // Check callback notifications
    EXPECT_EQ (2, objectList.addedObjects.size());
    EXPECT_EQ ("Button1", objectList.addedObjects[0]);
    EXPECT_EQ ("Label1", objectList.addedObjects[1]);
}

TEST_F (DataTreeObjectListTests, SelectiveObjectCreation)
{
    TestObjectList objectList (rootTree);

    // Add different types - some with name property, some without
    DataTree obj1 ("Object");
    DataTree obj2 ("Object");
    DataTree obj3 ("Object");

    {
        auto transaction1 = obj1.beginTransaction ("Setup Object 1");
        transaction1.setProperty ("name", "Named Object 1");
    }
    {
        auto transaction2 = obj2.beginTransaction ("Setup Object 2");
        transaction2.setProperty ("name", "Named Object 2");
    }
    {
        // obj3 has no name property - should not be included
        auto transaction3 = obj3.beginTransaction ("Setup Object 3");
        transaction3.setProperty ("id", 123);
    }
    {
        auto rootTransaction = rootTree.beginTransaction ("Add Mixed Objects");
        rootTransaction.addChild (obj1);
        rootTransaction.addChild (obj3); // This won't be included
        rootTransaction.addChild (obj2);
    }

    // Only objects with name property should be in the list
    EXPECT_EQ (2, objectList.objects.size());
    EXPECT_EQ ("Named Object 1", objectList.objects[0]->getName());
    EXPECT_EQ ("Named Object 2", objectList.objects[1]->getName());

    // Check notifications
    EXPECT_EQ (2, objectList.addedObjects.size());
    EXPECT_EQ ("Named Object 1", objectList.addedObjects[0]);
    EXPECT_EQ ("Named Object 2", objectList.addedObjects[1]);
}

TEST_F (DataTreeObjectListTests, ObjectRemoval)
{
    TestObjectList objectList (rootTree);

    // Add some objects
    DataTree obj1 ("Object");
    DataTree obj2 ("Object");
    DataTree obj3 ("Object");

    {
        auto transaction1 = obj1.beginTransaction ("Setup Object 1");
        transaction1.setProperty ("name", "Obj1");
    }
    {
        auto transaction2 = obj2.beginTransaction ("Setup Object 2");
        transaction2.setProperty ("name", "Obj2");
    }
    {
        auto transaction3 = obj3.beginTransaction ("Setup Object 3");
        transaction3.setProperty ("name", "Obj3");
    }
    {
        auto rootTransaction = rootTree.beginTransaction ("Add Objects");
        rootTransaction.addChild (obj1);
        rootTransaction.addChild (obj2);
        rootTransaction.addChild (obj3);
    }

    EXPECT_EQ (3, objectList.objects.size());
    EXPECT_EQ (3, TestObject::constructorCallCount);

    // Remove middle object
    {
        auto transaction = rootTree.beginTransaction ("Remove Object");
        transaction.removeChild (obj2);
    }

    EXPECT_EQ (2, objectList.objects.size());
    EXPECT_EQ (1, TestObject::destructorCallCount);

    // Remaining objects should be correct
    EXPECT_EQ ("Obj1", objectList.objects[0]->getName());
    EXPECT_EQ ("Obj3", objectList.objects[1]->getName());

    // Check removal notification
    EXPECT_EQ (1, objectList.removedObjects.size());
    EXPECT_EQ ("Obj2", objectList.removedObjects[0]);
}

TEST_F (DataTreeObjectListTests, ObjectReordering)
{
    TestObjectList objectList (rootTree);

    // Add objects
    DataTree obj1 ("Object");
    DataTree obj2 ("Object");
    DataTree obj3 ("Object");

    {
        auto transaction1 = obj1.beginTransaction ("Setup Object 1");
        transaction1.setProperty ("name", "First");
    }
    {
        auto transaction2 = obj2.beginTransaction ("Setup Object 2");
        transaction2.setProperty ("name", "Second");
    }
    {
        auto transaction3 = obj3.beginTransaction ("Setup Object 3");
        transaction3.setProperty ("name", "Third");
    }
    {
        auto rootTransaction = rootTree.beginTransaction ("Add Objects");
        rootTransaction.addChild (obj1);
        rootTransaction.addChild (obj2);
        rootTransaction.addChild (obj3);
    }

    // Move first object to end
    {
        auto transaction = rootTree.beginTransaction ("Reorder Objects");
        transaction.moveChild (0, 2);
    }

    // Order should be updated
    EXPECT_EQ ("Second", objectList.objects[0]->getName());
    EXPECT_EQ ("Third", objectList.objects[1]->getName());
    EXPECT_EQ ("First", objectList.objects[2]->getName());

    EXPECT_EQ (1, objectList.orderChangedCount);
}

TEST_F (DataTreeObjectListTests, ObjectStateSync)
{
    TestObjectList objectList (rootTree);

    // Add an object
    DataTree objTree ("Object");
    {
        auto transaction = objTree.beginTransaction ("Setup Object");
        transaction.setProperty ("name", "Test Object");
        transaction.setProperty ("enabled", true);
    }
    {
        auto rootTransaction = rootTree.beginTransaction ("Add Object");
        rootTransaction.addChild (objTree);
    }

    EXPECT_EQ (1, objectList.objects.size());
    TestObject* object = objectList.objects[0];

    // Test initial state via getter methods
    EXPECT_EQ ("Test Object", object->getName());
    EXPECT_TRUE (object->isEnabled());

    // Modify through setter methods
    object->setEnabled (false);
    EXPECT_FALSE (object->isEnabled());

    // Verify DataTree is updated
    EXPECT_FALSE (static_cast<bool> (objTree.getProperty ("enabled")));

    // Modify through DataTree
    {
        auto transaction = objTree.beginTransaction ("Enable Object");
        transaction.setProperty ("enabled", true);
    }

    // Object should reflect the change automatically via CachedValue
    EXPECT_TRUE (object->isEnabled());
}

TEST_F (DataTreeObjectListTests, ArrayLikeAccess)
{
    TestObjectList objectList (rootTree);

    // Add objects
    for (int i = 0; i < 5; ++i)
    {
        DataTree obj ("Object");
        {
            auto transaction = obj.beginTransaction ("Setup Object");
            transaction.setProperty ("name", "Object" + String (i));
        }
        {
            auto rootTransaction = rootTree.beginTransaction ("Add Object");
            rootTransaction.addChild (obj);
        }
    }

    EXPECT_EQ (5, objectList.objects.size());

    // Test array-like access
    for (int i = 0; i < objectList.objects.size(); ++i)
    {
        EXPECT_EQ ("Object" + String (i), objectList.objects[i]->getName());
    }

    // Test iterator-style usage
    int index = 0;
    for (auto* object : objectList.objects)
    {
        EXPECT_EQ ("Object" + String (index), object->getName());
        ++index;
    }
}

TEST_F (DataTreeObjectListTests, LifecycleManagement)
{
    {
        TestObjectList objectList (rootTree);

        // Add objects
        DataTree obj1 ("Object");
        DataTree obj2 ("Object");

        {
            auto transaction1 = obj1.beginTransaction ("Setup Object 1");
            transaction1.setProperty ("name", "Obj1");
        }
        {
            auto transaction2 = obj2.beginTransaction ("Setup Object 2");
            transaction2.setProperty ("name", "Obj2");
        }
        {
            auto rootTransaction = rootTree.beginTransaction ("Add Objects");
            rootTransaction.addChild (obj1);
            rootTransaction.addChild (obj2);
        }

        EXPECT_EQ (2, TestObject::constructorCallCount);
        EXPECT_EQ (0, TestObject::destructorCallCount);

    } // TestObjectList goes out of scope

    // All objects should be destroyed
    EXPECT_EQ (2, TestObject::destructorCallCount);
}

TEST_F (DataTreeObjectListTests, EmptyListBehavior)
{
    TestObjectList objectList (rootTree);

    // Test empty list
    EXPECT_EQ (0, objectList.objects.size());
    EXPECT_EQ (0, objectList.addedObjects.size());
    EXPECT_EQ (0, objectList.removedObjects.size());

    // Add and immediately remove
    DataTree obj ("Object");
    {
        auto transaction = obj.beginTransaction ("Setup Object");
        transaction.setProperty ("name", "TempObject");
    }
    {
        auto rootTransaction = rootTree.beginTransaction ("Add Object");
        rootTransaction.addChild (obj);
    }

    EXPECT_EQ (1, objectList.objects.size());

    {
        auto transaction = rootTree.beginTransaction ("Remove Object");
        transaction.removeChild (obj);
    }

    EXPECT_EQ (0, objectList.objects.size());
    EXPECT_EQ (1, objectList.addedObjects.size());
    EXPECT_EQ (1, objectList.removedObjects.size());
}

TEST_F (DataTreeObjectListTests, RangeBasedForLoopIntegration)
{
    // Add some objects to the root tree
    DataTree obj1 ("Object");
    DataTree obj2 ("Object");
    DataTree obj3 ("Object");

    {
        auto transaction1 = obj1.beginTransaction ("Setup Object 1");
        transaction1.setProperty ("name", "First");
    }
    {
        auto transaction2 = obj2.beginTransaction ("Setup Object 2");
        transaction2.setProperty ("name", "Second");
    }
    {
        auto transaction3 = obj3.beginTransaction ("Setup Object 3");
        transaction3.setProperty ("name", "Third");
    }
    {
        auto rootTransaction = rootTree.beginTransaction ("Add Objects");
        rootTransaction.addChild (obj1);
        rootTransaction.addChild (obj2);
        rootTransaction.addChild (obj3);
    }

    // Now create the object list after adding children
    TestObjectList objectList (rootTree);
    EXPECT_EQ (3, objectList.objects.size());

    // Verify the range-based for loop works with DataTree
    std::vector<String> childNames;
    for (const auto& child : rootTree)
    {
        if (child.hasProperty ("name"))
            childNames.push_back (child.getProperty ("name"));
    }

    EXPECT_EQ (3, childNames.size());
    EXPECT_EQ ("First", childNames[0]);
    EXPECT_EQ ("Second", childNames[1]);
    EXPECT_EQ ("Third", childNames[2]);

    // Verify objects match the DataTree children
    for (int i = 0; i < objectList.objects.size(); ++i)
    {
        EXPECT_EQ (childNames[i], objectList.objects[i]->getName());
    }
}
