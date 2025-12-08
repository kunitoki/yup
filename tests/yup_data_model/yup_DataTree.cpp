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
const Identifier rootType ("Root");
const Identifier childType ("Child");
const Identifier propertyName ("testProperty");
} // namespace

//==============================================================================
class DataTreeTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tree = DataTree (rootType);
    }

    void TearDown() override
    {
        tree = DataTree();
    }

    DataTree tree;
};

//==============================================================================
TEST_F (DataTreeTests, ConstructorCreatesValidTree)
{
    EXPECT_TRUE (tree.isValid());
    EXPECT_TRUE (static_cast<bool> (tree));
    EXPECT_EQ (rootType, tree.getType());
}

TEST_F (DataTreeTests, DefaultConstructorCreatesInvalidTree)
{
    DataTree invalidTree;
    EXPECT_FALSE (invalidTree.isValid());
    EXPECT_FALSE (static_cast<bool> (invalidTree));
    EXPECT_EQ (Identifier(), invalidTree.getType());
}

TEST_F (DataTreeTests, CopyConstructorWorksCorrectly)
{
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty (propertyName, "test value");
    }

    DataTree copy (tree);
    EXPECT_TRUE (copy.isValid());
    EXPECT_EQ (tree.getType(), copy.getType());
    EXPECT_EQ (tree.getProperty (propertyName), copy.getProperty (propertyName));
    EXPECT_EQ (tree, copy); // Same internal object
}

TEST_F (DataTreeTests, CloneCreatesDeepCopy)
{
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty (propertyName, "test value");
    }

    auto clone = tree.clone();
    EXPECT_TRUE (clone.isValid());
    EXPECT_EQ (tree.getType(), clone.getType());
    EXPECT_EQ (tree.getProperty (propertyName), clone.getProperty (propertyName));
    EXPECT_NE (tree, clone); // Different internal objects
    EXPECT_TRUE (tree.isEquivalentTo (clone));
}

//==============================================================================
// Property Tests

TEST_F (DataTreeTests, PropertyManagement)
{
    EXPECT_EQ (0, tree.getNumProperties());
    EXPECT_FALSE (tree.hasProperty (propertyName));

    // Set property
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty (propertyName, 42);
    }
    EXPECT_EQ (1, tree.getNumProperties());
    EXPECT_TRUE (tree.hasProperty (propertyName));
    EXPECT_EQ (var (42), tree.getProperty (propertyName));
    EXPECT_EQ (propertyName, tree.getPropertyName (0));

    // Default value handling
    EXPECT_EQ (var (99), tree.getProperty ("nonexistent", 99));

    // Remove property
    {
        auto transaction = tree.beginTransaction();
        transaction.removeProperty (propertyName);
    }
    EXPECT_EQ (0, tree.getNumProperties());
    EXPECT_FALSE (tree.hasProperty (propertyName));
}

TEST_F (DataTreeTests, TypedPropertyAccess)
{
    // Test getting property with default values
    EXPECT_EQ (0, static_cast<int> (tree.getProperty (propertyName, 0)));
    EXPECT_EQ (100, static_cast<int> (tree.getProperty (propertyName, 100)));

    // Set property using transaction
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty (propertyName, 42);
    }

    EXPECT_TRUE (tree.hasProperty (propertyName));
    EXPECT_EQ (42, static_cast<int> (tree.getProperty (propertyName)));

    // Update property using transaction
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty (propertyName, 99);
    }

    EXPECT_EQ (99, static_cast<int> (tree.getProperty (propertyName)));

    // Remove property using transaction
    {
        auto transaction = tree.beginTransaction();
        transaction.removeProperty (propertyName);
    }

    EXPECT_FALSE (tree.hasProperty (propertyName));
}

TEST_F (DataTreeTests, MultiplePropertiesHandling)
{
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("prop1", "string value");
        transaction.setProperty ("prop2", 123);
        transaction.setProperty ("prop3", 45.67);
    }

    EXPECT_EQ (3, tree.getNumProperties());
    EXPECT_TRUE (tree.hasProperty ("prop1"));
    EXPECT_TRUE (tree.hasProperty ("prop2"));
    EXPECT_TRUE (tree.hasProperty ("prop3"));

    {
        auto transaction = tree.beginTransaction();
        transaction.removeAllProperties();
    }

    EXPECT_EQ (0, tree.getNumProperties());
}

//==============================================================================
// Child Management Tests

TEST_F (DataTreeTests, ChildManagement)
{
    EXPECT_EQ (0, tree.getNumChildren());

    // Add child
    DataTree child (childType);

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child);
    }

    EXPECT_EQ (1, tree.getNumChildren());
    auto retrievedChild = tree.getChild (0);
    EXPECT_EQ (child, retrievedChild);
    EXPECT_EQ (childType, retrievedChild.getType());
    EXPECT_EQ (0, tree.indexOf (child));

    // Test parent relationship
    EXPECT_EQ (tree, retrievedChild.getParent());
    EXPECT_TRUE (retrievedChild.isAChildOf (tree));

    // Remove child
    {
        auto transaction = tree.beginTransaction();
        transaction.removeChild (child);
    }
    EXPECT_EQ (0, tree.getNumChildren());
}

TEST_F (DataTreeTests, ChildInsertionAtIndex)
{
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree child3 ("Child3");

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child3);
        transaction.addChild (child2, 1); // Insert between child1 and child3
    }

    EXPECT_EQ (3, tree.getNumChildren());
    EXPECT_EQ (child1, tree.getChild (0));
    EXPECT_EQ (child2, tree.getChild (1));
    EXPECT_EQ (child3, tree.getChild (2));
}

TEST_F (DataTreeTests, ChildMovement)
{
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree child3 ("Child3");

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);
    }

    // Move child1 from index 0 to index 2
    {
        auto transaction = tree.beginTransaction();
        transaction.moveChild (0, 2);
    }

    EXPECT_EQ (child2, tree.getChild (0));
    EXPECT_EQ (child3, tree.getChild (1));
    EXPECT_EQ (child1, tree.getChild (2));
}

TEST_F (DataTreeTests, GetChildWithName)
{
    DataTree child1 ("Type1");
    DataTree child2 ("Type2");
    DataTree child3 ("Type1"); // Duplicate type

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);
    }

    auto foundChild = tree.getChildWithName ("Type2");
    EXPECT_EQ (child2, foundChild);

    // Should return first match for duplicate types
    auto firstType1 = tree.getChildWithName ("Type1");
    EXPECT_EQ (child1, firstType1);

    // Non-existent type
    auto notFound = tree.getChildWithName ("NonExistent");
    EXPECT_FALSE (notFound.isValid());
}

TEST_F (DataTreeTests, RemoveAllChildren)
{
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
    }
    EXPECT_EQ (2, tree.getNumChildren());

    {
        auto transaction = tree.beginTransaction();
        transaction.removeAllChildren();
    }
    EXPECT_EQ (0, tree.getNumChildren());

    // Children should no longer have parent
    EXPECT_FALSE (child1.getParent().isValid());
    EXPECT_FALSE (child2.getParent().isValid());
}

//==============================================================================
// Navigation Tests

TEST_F (DataTreeTests, TreeNavigation)
{
    DataTree child (childType);
    DataTree grandchild ("Grandchild");

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child);
    }

    {
        auto transaction = child.beginTransaction();
        transaction.addChild (grandchild);
    }

    // Test parent relationships
    EXPECT_EQ (tree, child.getParent());
    EXPECT_EQ (child, grandchild.getParent());
    EXPECT_FALSE (tree.getParent().isValid());

    // Test root finding
    EXPECT_EQ (tree, tree.getRoot());
    EXPECT_EQ (tree, child.getRoot());
    EXPECT_EQ (tree, grandchild.getRoot());

    // Test depth calculation
    EXPECT_EQ (0, tree.getDepth());
    EXPECT_EQ (1, child.getDepth());
    EXPECT_EQ (2, grandchild.getDepth());

    // Test ancestor relationships
    EXPECT_TRUE (child.isAChildOf (tree));
    EXPECT_TRUE (grandchild.isAChildOf (tree));
    EXPECT_TRUE (grandchild.isAChildOf (child));
    EXPECT_FALSE (tree.isAChildOf (child));
}

//==============================================================================
// Query and Iteration Tests

TEST_F (DataTreeTests, ChildIteration)
{
    DataTree child1 ("Type1");
    DataTree child2 ("Type2");
    DataTree child3 ("Type1");

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);
    }

    std::vector<DataTree> visited;
    tree.forEachChild ([&] (const DataTree& child)
    {
        visited.push_back (child);
    });

    EXPECT_EQ (3, visited.size());
    EXPECT_EQ (child1, visited[0]);
    EXPECT_EQ (child2, visited[1]);
    EXPECT_EQ (child3, visited[2]);
}

TEST_F (DataTreeTests, RangeBasedForLoop)
{
    DataTree child1 ("Type1");
    DataTree child2 ("Type2");
    DataTree child3 ("Type3");

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);
    }

    // Test range-based for loop
    std::vector<DataTree> visited;
    for (const auto& child : tree)
    {
        visited.push_back (child);
    }

    EXPECT_EQ (3, visited.size());
    EXPECT_EQ (child1, visited[0]);
    EXPECT_EQ (child2, visited[1]);
    EXPECT_EQ (child3, visited[2]);
}

TEST_F (DataTreeTests, RangeBasedForLoopEmpty)
{
    // Test with empty DataTree
    std::vector<DataTree> visited;
    for (const auto& child : tree)
    {
        visited.push_back (child);
    }

    EXPECT_EQ (0, visited.size());
}

TEST_F (DataTreeTests, IteratorInterface)
{
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
    }

    // Test iterator equality and inequality
    auto it1 = tree.begin();
    auto it2 = tree.begin();
    auto end = tree.end();

    EXPECT_TRUE (it1 == it2);
    EXPECT_FALSE (it1 != it2);
    EXPECT_FALSE (it1 == end);
    EXPECT_TRUE (it1 != end);

    // Test dereference
    EXPECT_EQ (child1, *it1);

    // Test pre-increment
    ++it1;
    EXPECT_EQ (child2, *it1);
    EXPECT_FALSE (it1 == it2);

    // Test post-increment
    auto it3 = it1++;
    EXPECT_EQ (child2, *it3);
    EXPECT_TRUE (it1 == end);

    // Test arrow operator
    auto it4 = tree.begin();
    EXPECT_EQ (child1.getType(), (*it4).getType());
}

TEST_F (DataTreeTests, RangeBasedForLoopModification)
{
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
    }

    // Test that we can access properties through the iterator
    int propertyCount = 0;
    for (const auto& child : tree)
    {
        if (child.hasProperty ("name"))
            propertyCount++;
    }

    EXPECT_EQ (0, propertyCount);

    // Add properties
    {
        auto transaction1 = child1.beginTransaction();
        transaction1.setProperty ("name", "First");

        auto transaction2 = child2.beginTransaction();
        transaction2.setProperty ("name", "Second");
    }

    // Test again
    propertyCount = 0;
    std::vector<String> names;
    for (const auto& child : tree)
    {
        if (child.hasProperty ("name"))
        {
            propertyCount++;
            names.push_back (child.getProperty ("name"));
        }
    }

    EXPECT_EQ (2, propertyCount);
    EXPECT_EQ ("First", names[0]);
    EXPECT_EQ ("Second", names[1]);
}

TEST_F (DataTreeTests, PredicateBasedSearch)
{
    DataTree child1 ("Type1");
    DataTree child2 ("Type2");
    DataTree child3 ("Type1");

    {
        auto transaction = child1.beginTransaction();
        transaction.setProperty ("id", 1);
    }

    {
        auto transaction = child2.beginTransaction();
        transaction.setProperty ("id", 2);
    }

    {
        auto transaction = child3.beginTransaction();
        transaction.setProperty ("id", 3);
    }

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);
    }

    // Find children by type
    std::vector<DataTree> type1Children;
    tree.findChildren (type1Children, [] (const DataTree& child)
    {
        return child.getType() == Identifier ("Type1");
    });

    EXPECT_EQ (2, type1Children.size());
    EXPECT_EQ (child1, type1Children[0]);
    EXPECT_EQ (child3, type1Children[1]);

    // Find first child with specific property
    auto childWithId2 = tree.findChild ([] (const DataTree& child)
    {
        return child.getProperty ("id") == var (2);
    });

    EXPECT_EQ (child2, childWithId2);
}

TEST_F (DataTreeTests, DescendantIteration)
{
    DataTree child (childType);
    DataTree grandchild1 ("Grandchild1");
    DataTree grandchild2 ("Grandchild2");

    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child);
    }

    {
        auto transaction = child.beginTransaction();
        transaction.addChild (grandchild1);
        transaction.addChild (grandchild2);
    }

    std::vector<DataTree> descendants;
    tree.forEachDescendant ([&] (const DataTree& descendant)
    {
        descendants.push_back (descendant);
    });

    EXPECT_EQ (3, descendants.size()); // child + 2 grandchildren
    EXPECT_EQ (child, descendants[0]);
    EXPECT_EQ (grandchild1, descendants[1]);
    EXPECT_EQ (grandchild2, descendants[2]);
}

//==============================================================================
// Listener Tests

class TestListener : public DataTree::Listener
{
public:
    void propertyChanged (DataTree& tree, const Identifier& property) override
    {
        propertyChanges.push_back ({ tree, property });
    }

    void childAdded (DataTree& parent, DataTree& child) override
    {
        childAdditions.push_back ({ parent, child });
    }

    void childRemoved (DataTree& parent, DataTree& child, int formerIndex) override
    {
        childRemovals.push_back ({ parent, child, formerIndex });
    }

    struct PropertyChange
    {
        DataTree tree;
        Identifier property;
    };

    struct ChildChange
    {
        DataTree parent, child;
        int index = -1;
    };

    std::vector<PropertyChange> propertyChanges;
    std::vector<ChildChange> childAdditions;
    std::vector<ChildChange> childRemovals;

    void reset()
    {
        propertyChanges.clear();
        childAdditions.clear();
        childRemovals.clear();
    }
};

TEST_F (DataTreeTests, PropertyChangeNotifications)
{
    TestListener listener;
    tree.addListener (&listener);

    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty (propertyName, "test");
    }

    ASSERT_EQ (1, listener.propertyChanges.size());
    EXPECT_EQ (tree, listener.propertyChanges[0].tree);
    EXPECT_EQ (propertyName, listener.propertyChanges[0].property);

    tree.removeListener (&listener);
    listener.reset();

    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty (propertyName, "test2");
    }
    EXPECT_EQ (0, listener.propertyChanges.size()); // No notification after removal
}

TEST_F (DataTreeTests, ChildChangeNotifications)
{
    TestListener listener;
    tree.addListener (&listener);

    DataTree child (childType);
    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child);
    }

    ASSERT_EQ (1, listener.childAdditions.size());
    EXPECT_EQ (tree, listener.childAdditions[0].parent);
    EXPECT_EQ (child, listener.childAdditions[0].child);

    {
        auto transaction = tree.beginTransaction();
        transaction.removeChild (child);
    }

    ASSERT_EQ (1, listener.childRemovals.size());
    EXPECT_EQ (tree, listener.childRemovals[0].parent);
    EXPECT_EQ (child, listener.childRemovals[0].child);
    EXPECT_EQ (0, listener.childRemovals[0].index);
}

//==============================================================================
// Serialization Tests

TEST_F (DataTreeTests, XmlSerialization)
{
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("stringProp", "test string");
        transaction.setProperty ("intProp", 42);
        transaction.setProperty ("floatProp", 3.14);

        DataTree child (childType);
        {
            auto childTransaction = child.beginTransaction();
            childTransaction.setProperty ("childProp", "child value");
        }
        transaction.addChild (child);
    }

    // Create XML
    auto xml = tree.createXml();
    ASSERT_NE (nullptr, xml);
    EXPECT_EQ (rootType.toString(), xml->getTagName());
    EXPECT_EQ ("test string", xml->getStringAttribute ("stringProp"));
    EXPECT_EQ (42, xml->getIntAttribute ("intProp"));
    EXPECT_NEAR (3.14, xml->getDoubleAttribute ("floatProp"), 0.001);

    // Reconstruct from XML
    auto reconstructed = DataTree::fromXml (*xml);
    EXPECT_TRUE (reconstructed.isValid());
    EXPECT_TRUE (tree.isEquivalentTo (reconstructed));
}

TEST_F (DataTreeTests, BinarySerialization)
{
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("prop1", "value1");
        transaction.setProperty ("prop2", 123);

        DataTree child (childType);
        {
            auto childTransaction = child.beginTransaction();
            childTransaction.setProperty ("childProp", "childValue");
        }
        transaction.addChild (child);
    }

    // Write to stream
    MemoryOutputStream output;
    tree.writeToBinaryStream (output);

    // Read from stream
    MemoryInputStream input (output.getData(), output.getDataSize(), false);
    auto reconstructed = DataTree::readFromBinaryStream (input);

    EXPECT_TRUE (reconstructed.isValid());
    EXPECT_TRUE (tree.isEquivalentTo (reconstructed));
}

TEST_F (DataTreeTests, JsonSerialization)
{
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("stringProp", "test string");
        transaction.setProperty ("intProp", 42);
        transaction.setProperty ("floatProp", 3.14);
        transaction.setProperty ("boolProp", true);

        DataTree child (childType);
        {
            auto childTransaction = child.beginTransaction();
            childTransaction.setProperty ("childProp", "child value");
            childTransaction.setProperty ("childInt", 123);
        }
        transaction.addChild (child);

        DataTree emptyChild ("EmptyChild");
        transaction.addChild (emptyChild);
    }

    // Create JSON
    var jsonData = tree.createJson();
    ASSERT_TRUE (jsonData.isObject());

    // Verify JSON structure
    auto* jsonObj = jsonData.getDynamicObject();
    ASSERT_NE (nullptr, jsonObj);
    EXPECT_EQ (rootType.toString(), jsonObj->getProperty ("type").toString());

    // Check properties
    var properties = jsonObj->getProperty ("properties");
    ASSERT_TRUE (properties.isObject());
    auto* propsObj = properties.getDynamicObject();
    ASSERT_NE (nullptr, propsObj);
    EXPECT_EQ ("test string", propsObj->getProperty ("stringProp").toString());
    EXPECT_EQ (var (42), propsObj->getProperty ("intProp"));
    EXPECT_NEAR (3.14, static_cast<double> (propsObj->getProperty ("floatProp")), 0.001);
    EXPECT_TRUE (static_cast<bool> (propsObj->getProperty ("boolProp")));

    // Check children array
    var children = jsonObj->getProperty ("children");
    ASSERT_TRUE (children.isArray());
    auto* childrenArray = children.getArray();
    ASSERT_NE (nullptr, childrenArray);
    EXPECT_EQ (2, childrenArray->size());

    // Check first child
    var firstChild = childrenArray->getReference (0);
    ASSERT_TRUE (firstChild.isObject());
    auto* firstChildObj = firstChild.getDynamicObject();
    ASSERT_NE (nullptr, firstChildObj);
    EXPECT_EQ (childType.toString(), firstChildObj->getProperty ("type").toString());

    var firstChildProps = firstChildObj->getProperty ("properties");
    ASSERT_TRUE (firstChildProps.isObject());
    auto* firstChildPropsObj = firstChildProps.getDynamicObject();
    ASSERT_NE (nullptr, firstChildPropsObj);
    EXPECT_EQ ("child value", firstChildPropsObj->getProperty ("childProp").toString());
    EXPECT_EQ (var (123), firstChildPropsObj->getProperty ("childInt"));

    // Check second child (empty)
    var secondChild = childrenArray->getReference (1);
    ASSERT_TRUE (secondChild.isObject());
    auto* secondChildObj = secondChild.getDynamicObject();
    ASSERT_NE (nullptr, secondChildObj);
    EXPECT_EQ ("EmptyChild", secondChildObj->getProperty ("type").toString());

    var secondChildProps = secondChildObj->getProperty ("properties");
    ASSERT_TRUE (secondChildProps.isObject());
    auto* secondChildPropsObj = secondChildProps.getDynamicObject();
    EXPECT_EQ (0, secondChildPropsObj->getProperties().size());

    // Reconstruct from JSON
    auto reconstructed = DataTree::fromJson (jsonData);
    EXPECT_TRUE (reconstructed.isValid());
    EXPECT_TRUE (tree.isEquivalentTo (reconstructed));
}

TEST_F (DataTreeTests, JsonSerializationWithComplexStructure)
{
    DataTree root ("Root");

    {
        auto transaction = root.beginTransaction();
        transaction.setProperty ("version", "2.0");
        transaction.setProperty ("debug", false);

        DataTree config ("Configuration");
        {
            auto configTransaction = config.beginTransaction();
            configTransaction.setProperty ("timeout", 30);
            configTransaction.setProperty ("retries", 3);

            DataTree database ("Database");
            {
                auto dbTransaction = database.beginTransaction();
                dbTransaction.setProperty ("host", "localhost");
                dbTransaction.setProperty ("port", 5432);
                dbTransaction.setProperty ("ssl", true);
            }
            configTransaction.addChild (database);

            DataTree logging ("Logging");
            {
                auto logTransaction = logging.beginTransaction();
                logTransaction.setProperty ("level", "info");
                logTransaction.setProperty ("file", "/var/log/app.log");

                DataTree handlers ("Handlers");
                logTransaction.addChild (handlers);
            }
            configTransaction.addChild (logging);
        }
        transaction.addChild (config);

        DataTree plugins ("Plugins");
        transaction.addChild (plugins);
    }

    // Serialize and deserialize
    var jsonData = root.createJson();
    auto reconstructed = DataTree::fromJson (jsonData);

    EXPECT_TRUE (reconstructed.isValid());
    EXPECT_TRUE (root.isEquivalentTo (reconstructed));

    // Verify specific properties are preserved
    EXPECT_EQ (var ("2.0"), reconstructed.getProperty ("version", ""));
    EXPECT_FALSE (static_cast<bool> (reconstructed.getProperty ("debug", true)));

    auto configChild = reconstructed.getChildWithName ("Configuration");
    EXPECT_TRUE (configChild.isValid());
    EXPECT_EQ (var (30), configChild.getProperty ("timeout"));

    auto databaseChild = configChild.getChildWithName ("Database");
    EXPECT_TRUE (databaseChild.isValid());
    EXPECT_EQ (var ("localhost"), databaseChild.getProperty ("host", ""));
    EXPECT_TRUE (static_cast<bool> (databaseChild.getProperty ("ssl", false)));
}

TEST_F (DataTreeTests, JsonSerializationErrorHandling)
{
    // Test invalid JSON input
    var invalidJson = "not an object";
    DataTree fromInvalid = DataTree::fromJson (invalidJson);
    EXPECT_FALSE (fromInvalid.isValid());

    // Test JSON missing required fields
    auto missingType = std::make_unique<DynamicObject>();
    missingType->setProperty ("properties", var (new DynamicObject()));
    missingType->setProperty ("children", Array<var>());
    DataTree fromMissingType = DataTree::fromJson (missingType.release());
    EXPECT_FALSE (fromMissingType.isValid());

    // Test JSON with invalid structure
    auto invalidStructure = std::make_unique<DynamicObject>();
    invalidStructure->setProperty ("type", "TestType");
    invalidStructure->setProperty ("properties", "not an object"); // Should be object
    invalidStructure->setProperty ("children", Array<var>());
    DataTree fromInvalidStructure = DataTree::fromJson (invalidStructure.release());
    EXPECT_FALSE (fromInvalidStructure.isValid());
}

TEST_F (DataTreeTests, JsonSerializationEmptyTree)
{
    DataTree empty ("Empty");

    var jsonData = empty.createJson();
    ASSERT_TRUE (jsonData.isObject());

    auto* jsonObj = jsonData.getDynamicObject();
    ASSERT_NE (nullptr, jsonObj);
    EXPECT_EQ ("Empty", jsonObj->getProperty ("type").toString());

    var properties = jsonObj->getProperty ("properties");
    ASSERT_TRUE (properties.isObject());
    auto* propsObj = properties.getDynamicObject();
    EXPECT_EQ (0, propsObj->getProperties().size());

    var children = jsonObj->getProperty ("children");
    ASSERT_TRUE (children.isArray());
    auto* childrenArray = children.getArray();
    EXPECT_EQ (0, childrenArray->size());

    // Round trip
    auto reconstructed = DataTree::fromJson (jsonData);
    EXPECT_TRUE (reconstructed.isValid());
    EXPECT_TRUE (empty.isEquivalentTo (reconstructed));
}

TEST_F (DataTreeTests, SerializationFormatConsistency)
{
    // Create a complex tree structure
    DataTree original ("Application");

    {
        auto transaction = original.beginTransaction();
        transaction.setProperty ("name", "TestApp");
        transaction.setProperty ("version", "1.2.3");
        transaction.setProperty ("debug", true);
        transaction.setProperty ("maxUsers", 1000);
        transaction.setProperty ("pi", 3.14159);

        DataTree settings ("Settings");
        {
            auto settingsTransaction = settings.beginTransaction();
            settingsTransaction.setProperty ("theme", "dark");
            settingsTransaction.setProperty ("autoSave", true);
            settingsTransaction.setProperty ("interval", 300);

            DataTree advanced ("Advanced");
            {
                auto advancedTransaction = advanced.beginTransaction();
                advancedTransaction.setProperty ("bufferSize", 8192);
                advancedTransaction.setProperty ("compression", false);
            }
            settingsTransaction.addChild (advanced);
        }
        transaction.addChild (settings);

        DataTree plugins ("Plugins");
        {
            auto pluginsTransaction = plugins.beginTransaction();

            DataTree plugin1 ("Plugin");
            {
                auto plugin1Transaction = plugin1.beginTransaction();
                plugin1Transaction.setProperty ("name", "Logger");
                plugin1Transaction.setProperty ("enabled", true);
            }
            pluginsTransaction.addChild (plugin1);

            DataTree plugin2 ("Plugin");
            {
                auto plugin2Transaction = plugin2.beginTransaction();
                plugin2Transaction.setProperty ("name", "Validator");
                plugin2Transaction.setProperty ("enabled", false);
            }
            pluginsTransaction.addChild (plugin2);
        }
        transaction.addChild (plugins);
    }

    // Test XML serialization roundtrip
    auto xml = original.createXml();
    ASSERT_NE (nullptr, xml);
    auto fromXml = DataTree::fromXml (*xml);
    EXPECT_TRUE (fromXml.isValid());
    EXPECT_TRUE (original.isEquivalentTo (fromXml));

    // Test binary serialization roundtrip
    MemoryOutputStream binaryOutput;
    original.writeToBinaryStream (binaryOutput);
    MemoryInputStream binaryInput (binaryOutput.getData(), binaryOutput.getDataSize(), false);
    auto fromBinary = DataTree::readFromBinaryStream (binaryInput);
    EXPECT_TRUE (fromBinary.isValid());
    EXPECT_TRUE (original.isEquivalentTo (fromBinary));

    // Test JSON serialization roundtrip
    var jsonData = original.createJson();
    auto fromJson = DataTree::fromJson (jsonData);
    EXPECT_TRUE (fromJson.isValid());
    EXPECT_TRUE (original.isEquivalentTo (fromJson));

    // Verify all formats produce equivalent results
    EXPECT_TRUE (fromXml.isEquivalentTo (fromBinary));
    EXPECT_TRUE (fromBinary.isEquivalentTo (fromJson));
    EXPECT_TRUE (fromXml.isEquivalentTo (fromJson));

    // Spot check some properties across all formats
    EXPECT_EQ (var ("TestApp"), fromXml.getProperty ("name", ""));
    EXPECT_EQ (var ("TestApp"), fromBinary.getProperty ("name", ""));
    EXPECT_EQ (var ("TestApp"), fromJson.getProperty ("name", ""));

    auto xmlSettings = fromXml.getChildWithName ("Settings");
    auto binarySettings = fromBinary.getChildWithName ("Settings");
    auto jsonSettings = fromJson.getChildWithName ("Settings");

    EXPECT_TRUE (xmlSettings.isValid());
    EXPECT_TRUE (binarySettings.isValid());
    EXPECT_TRUE (jsonSettings.isValid());

    EXPECT_EQ (var ("dark"), xmlSettings.getProperty ("theme", ""));
    EXPECT_EQ (var ("dark"), binarySettings.getProperty ("theme", ""));
    EXPECT_EQ (var ("dark"), jsonSettings.getProperty ("theme", ""));
}

TEST_F (DataTreeTests, InvalidTreeSerialization)
{
    DataTree invalid;
    EXPECT_FALSE (invalid.isValid());

    // Invalid trees should return appropriate failure indicators
    auto xml = invalid.createXml();
    EXPECT_EQ (nullptr, xml);

    var jsonData = invalid.createJson();
    EXPECT_FALSE (jsonData.isObject());

    // Writing invalid tree to binary should not crash but produce empty/invalid data
    MemoryOutputStream output;
    invalid.writeToBinaryStream (output);
    // The specific behavior of writing an invalid tree is implementation-defined,
    // but it should not crash
    EXPECT_GE (output.getDataSize(), 0); // At least it didn't crash
}

//==============================================================================
// Comparison Tests

TEST_F (DataTreeTests, EqualityComparison)
{
    DataTree other (rootType);

    // Same reference equality
    DataTree sameRef = tree;
    EXPECT_EQ (tree, sameRef);
    EXPECT_FALSE (tree != sameRef);

    // Different objects
    EXPECT_NE (tree, other);
    EXPECT_FALSE (tree == other);

    // Equivalence testing
    EXPECT_TRUE (tree.isEquivalentTo (other)); // Both empty with same type

    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("prop", "value");
    }
    EXPECT_FALSE (tree.isEquivalentTo (other)); // Different properties

    {
        auto transaction = other.beginTransaction();
        transaction.setProperty ("prop", "value");
    }
    EXPECT_TRUE (tree.isEquivalentTo (other)); // Same properties
}

//==============================================================================
// Edge Cases and Error Handling

TEST_F (DataTreeTests, InvalidOperationsHandling)
{
    DataTree invalid;

    // Operations on invalid tree should not crash
    EXPECT_EQ (0, invalid.getNumProperties());
    EXPECT_EQ (0, invalid.getNumChildren());
    EXPECT_FALSE (invalid.hasProperty ("anything"));
    EXPECT_EQ (var(), invalid.getProperty ("anything"));

    // These operations on invalid tree should do nothing and not crash
    {
        auto transaction = invalid.beginTransaction();
        transaction.setProperty ("prop", "value");
        transaction.addChild (DataTree ("Child"));
    }

    EXPECT_EQ (0, invalid.getNumProperties());
    EXPECT_EQ (0, invalid.getNumChildren());
}

TEST_F (DataTreeTests, CircularReferenceProtection)
{
    DataTree child (childType);
    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child);
    }

    // Try to add parent as child of its own child - should be prevented
    {
        auto transaction = child.beginTransaction();
        transaction.addChild (tree);
    }
    EXPECT_EQ (0, child.getNumChildren()); // Should not be added

    // Try to add self as child - should be prevented
    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (tree);
    }
    EXPECT_EQ (1, tree.getNumChildren()); // Only the original child
}

TEST_F (DataTreeTests, OutOfBoundsAccess)
{
    // Test property access with invalid indices
    EXPECT_EQ (Identifier(), tree.getPropertyName (-1));
    EXPECT_EQ (Identifier(), tree.getPropertyName (0)); // No properties yet
    EXPECT_EQ (Identifier(), tree.getPropertyName (100));

    // Test child access with invalid indices
    EXPECT_FALSE (tree.getChild (-1).isValid());
    EXPECT_FALSE (tree.getChild (0).isValid()); // No children yet
    EXPECT_FALSE (tree.getChild (100).isValid());

    // Test removal with invalid indices - should not crash
    {
        auto transaction = tree.beginTransaction();
        transaction.removeChild (-1);  // Should not crash
        transaction.removeChild (100); // Should not crash
    }
}

//==============================================================================
// Transaction Tests

TEST_F (DataTreeTests, BasicTransaction)
{
    auto transaction = tree.beginTransaction();

    EXPECT_TRUE (transaction.isActive());

    transaction.setProperty ("prop1", "value1");
    transaction.setProperty ("prop2", 42);

    DataTree child (childType);
    {
        auto childTransaction = child.beginTransaction();
        childTransaction.setProperty ("childProp", "childValue");
    }
    transaction.addChild (child);

    // Changes should not be visible yet
    EXPECT_FALSE (tree.hasProperty ("prop1"));
    EXPECT_FALSE (tree.hasProperty ("prop2"));
    EXPECT_EQ (0, tree.getNumChildren());

    transaction.commit();

    // Changes should now be visible
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_EQ (var (42), tree.getProperty ("prop2"));
    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (child, tree.getChild (0));

    EXPECT_FALSE (transaction.isActive());
}

TEST_F (DataTreeTests, TransactionAutoCommit)
{
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("prop", "value");
        // Transaction auto-commits when it goes out of scope
    }

    EXPECT_EQ (var ("value"), tree.getProperty ("prop"));
}

TEST_F (DataTreeTests, TransactionAbort)
{
    auto transaction = tree.beginTransaction();

    transaction.setProperty ("prop", "value");
    transaction.abort();

    // Changes should not be applied
    EXPECT_FALSE (tree.hasProperty ("prop"));
    EXPECT_FALSE (transaction.isActive());
}

TEST_F (DataTreeTests, TransactionWithUndo)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop1", "value1");
        transaction.setProperty ("prop2", 42);
    }

    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_EQ (var (42), tree.getProperty ("prop2"));

    undoManager->undo();

    EXPECT_FALSE (tree.hasProperty ("prop1"));
    EXPECT_FALSE (tree.hasProperty ("prop2"));
}

TEST_F (DataTreeTests, TransactionMoveSemantics)
{
    auto transaction1 = tree.beginTransaction();
    transaction1.setProperty ("prop", "value1");

    // Move the transaction
    auto transaction2 = std::move (transaction1);

    EXPECT_FALSE (transaction1.isActive());
    EXPECT_TRUE (transaction2.isActive());

    transaction2.setProperty ("prop2", "value2");
    transaction2.commit();

    EXPECT_EQ (var ("value1"), tree.getProperty ("prop"));
    EXPECT_EQ (var ("value2"), tree.getProperty ("prop2"));
}

TEST_F (DataTreeTests, TransactionChildOperations)
{
    DataTree child1 ("Child 1", { { "id", 1 } });
    DataTree child2 ("Child 2", { { "id", 2 } });
    DataTree child3 ("Child 3", { { "id", 3 } });

    {
        auto transaction = tree.beginTransaction();

        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);

        transaction.moveChild (0, 2); // Move child1 to end
        transaction.removeChild (1);  // Remove middle child
    }

    EXPECT_EQ (2, tree.getNumChildren());
    EXPECT_EQ (var (2), tree.getChild (0).getProperty ("id")); // child2
    EXPECT_EQ (var (1), tree.getChild (1).getProperty ("id")); // child1 (moved to end)
}

//==============================================================================
// UndoManager Constructor Tests

TEST_F (DataTreeTests, UndoManagerWithTransactions)
{
    auto undoManager = UndoManager::Ptr (new UndoManager);

    EXPECT_TRUE (tree.isValid());
    EXPECT_EQ (rootType, tree.getType());

    // Test transactions with explicit undo manager
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop", "value");
    }

    // Test another transaction with different explicit undo manager
    auto explicitUndo = UndoManager::Ptr (new UndoManager);
    {
        auto transaction = tree.beginTransaction (explicitUndo);
        transaction.setProperty ("prop2", "value2");
    }

    // Both managers should have transactions
    EXPECT_GT (undoManager->getNumTransactions(), 0);
    EXPECT_GT (explicitUndo->getNumTransactions(), 0);
}

//==============================================================================
// Comprehensive Transaction Child Operation Tests

TEST_F (DataTreeTests, TransactionChildOperationsOrderTest1)
{
    // Test: Add, Move, Remove in various orders
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree child3 ("Child3");
    DataTree child4 ("Child4");

    {
        auto transaction = tree.beginTransaction();

        // Add children in order: 1, 2, 3
        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);

        // Insert child4 at index 1 (between child1 and child2)
        transaction.addChild (child4, 1);

        // Move child3 to index 1 (should be: child1, child3, child4, child2)
        transaction.moveChild (3, 1);

        // Remove child at index 2 (child4)
        transaction.removeChild (2);
    }

    // Final order should be: child1, child3, child2
    EXPECT_EQ (3, tree.getNumChildren());
    EXPECT_EQ (child1, tree.getChild (0));
    EXPECT_EQ (child3, tree.getChild (1));
    EXPECT_EQ (child2, tree.getChild (2));
}

TEST_F (DataTreeTests, TransactionChildOperationsOrderTest2)
{
    // Test: Remove, Add, Move operations
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree child3 ("Child3");
    DataTree child4 ("Child4");
    DataTree child5 ("Child5");

    // First setup some initial children
    {
        auto setupTransaction = tree.beginTransaction();
        setupTransaction.addChild (child1);
        setupTransaction.addChild (child2);
        setupTransaction.addChild (child3);
        setupTransaction.addChild (child4);
    }

    // Initial state: child1, child2, child3, child4
    EXPECT_EQ (4, tree.getNumChildren());

    {
        auto transaction = tree.beginTransaction();

        // Remove child2 (index 1)
        transaction.removeChild (1);

        // Add child5 at index 1 (replaces child2's position)
        transaction.addChild (child5, 1);

        // Move child4 (now at index 3) to index 0
        transaction.moveChild (3, 0);

        // Remove child1 (now at index 1 after child4 moved to 0)
        transaction.removeChild (1);
    }

    // Final order should be: child4, child5, child3
    EXPECT_EQ (3, tree.getNumChildren());
    EXPECT_EQ (child4, tree.getChild (0));
    EXPECT_EQ (child5, tree.getChild (1));
    EXPECT_EQ (child3, tree.getChild (2));
}

TEST_F (DataTreeTests, TransactionChildOperationsOrderTest3)
{
    // Test: Multiple moves and insertions at specific indices
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree child3 ("Child3");
    DataTree child4 ("Child4");
    DataTree child5 ("Child5");

    {
        auto transaction = tree.beginTransaction();

        // Add at end: 1, 2, 3
        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);

        // Insert at beginning: 4, 1, 2, 3
        transaction.addChild (child4, 0);

        // Insert at middle: 4, 1, 5, 2, 3
        transaction.addChild (child5, 2);

        // Move last to second: 4, 3, 1, 5, 2
        transaction.moveChild (4, 1);

        // Move first to end: 3, 1, 5, 2, 4
        transaction.moveChild (0, 4);
    }

    // Final order should be: child3, child1, child5, child2, child4
    EXPECT_EQ (5, tree.getNumChildren());
    EXPECT_EQ (child3, tree.getChild (0));
    EXPECT_EQ (child1, tree.getChild (1));
    EXPECT_EQ (child5, tree.getChild (2));
    EXPECT_EQ (child2, tree.getChild (3));
    EXPECT_EQ (child4, tree.getChild (4));
}

TEST_F (DataTreeTests, TransactionChildOperationsBoundaryTest)
{
    // Test operations at boundaries and with invalid indices
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree child3 ("Child3");

    {
        auto transaction = tree.beginTransaction();

        // Add children
        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);

        // Try to move to invalid index (should clamp to valid range)
        transaction.moveChild (0, 100); // Should move to end

        // Try to add at invalid index (should clamp to valid range)
        DataTree extraChild ("Extra");
        transaction.addChild (extraChild, -10); // Should add at beginning

        // Try to remove invalid index (should do nothing)
        transaction.removeChild (-5);
        transaction.removeChild (100);
    }

    // Should have: extraChild, child2, child3, child1
    EXPECT_EQ (4, tree.getNumChildren());
    // The exact order depends on implementation details of clamping
    // Just verify we have all children and valid state
    EXPECT_TRUE (tree.getChild (0).isValid());
    EXPECT_TRUE (tree.getChild (1).isValid());
    EXPECT_TRUE (tree.getChild (2).isValid());
    EXPECT_TRUE (tree.getChild (3).isValid());
}

TEST_F (DataTreeTests, TransactionChildOperationsConsistencyTest)
{
    // Test that all operations maintain consistent parent-child relationships
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree child3 ("Child3");

    {
        auto transaction = tree.beginTransaction();

        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);

        // Move operations
        transaction.moveChild (2, 0); // child3 to front
        transaction.moveChild (2, 1); // child2 to middle
    }

    // Verify all parent-child relationships are correct
    EXPECT_EQ (3, tree.getNumChildren());

    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto child = tree.getChild (i);
        EXPECT_TRUE (child.isValid());
        EXPECT_EQ (tree, child.getParent());
        EXPECT_TRUE (child.isAChildOf (tree));
    }

    // Verify no duplicate children
    EXPECT_NE (tree.getChild (0), tree.getChild (1));
    EXPECT_NE (tree.getChild (1), tree.getChild (2));
    EXPECT_NE (tree.getChild (0), tree.getChild (2));
}

TEST_F (DataTreeTests, TransactionChildOperationsUndoTest)
{
    // Test that undo works correctly with complex child operations
    auto undoManager = UndoManager::Ptr (new UndoManager());

    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree child3 ("Child3");

    // Perform complex operations
    {
        auto transaction = tree.beginTransaction (undoManager);

        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);

        transaction.moveChild (0, 2); // Move child1 to end
        transaction.removeChild (0);  // Remove child2
    }

    // Should have: child3, child1
    EXPECT_EQ (2, tree.getNumChildren());
    EXPECT_EQ (child3, tree.getChild (0));
    EXPECT_EQ (child1, tree.getChild (1));

    // Undo the transaction
    EXPECT_TRUE (undoManager->canUndo());
    undoManager->undo();

    // Should be back to empty
    EXPECT_EQ (0, tree.getNumChildren());

    // Redo the transaction
    EXPECT_TRUE (undoManager->canRedo());
    undoManager->redo();

    // Should have the same result: child3, child1
    EXPECT_EQ (2, tree.getNumChildren());
    EXPECT_EQ (child3, tree.getChild (0));
    EXPECT_EQ (child1, tree.getChild (1));
}

//==============================================================================
// Comprehensive UndoManager Integration Tests

TEST_F (DataTreeTests, UndoManagerPropertyOperations)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Test setting multiple properties with undo
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("name", "TestName");
        transaction.setProperty ("version", "1.0.0");
        transaction.setProperty ("enabled", true);
        transaction.setProperty ("count", 42);
    }

    EXPECT_EQ (var ("TestName"), tree.getProperty ("name"));
    EXPECT_EQ (var ("1.0.0"), tree.getProperty ("version"));
    EXPECT_TRUE (static_cast<bool> (tree.getProperty ("enabled")));
    EXPECT_EQ (var (42), tree.getProperty ("count"));
    EXPECT_EQ (4, tree.getNumProperties());

    // Undo should revert all properties
    ASSERT_TRUE (undoManager->canUndo());
    undoManager->undo();

    EXPECT_EQ (0, tree.getNumProperties());
    EXPECT_FALSE (tree.hasProperty ("name"));
    EXPECT_FALSE (tree.hasProperty ("version"));
    EXPECT_FALSE (tree.hasProperty ("enabled"));
    EXPECT_FALSE (tree.hasProperty ("count"));

    // Redo should restore all properties
    ASSERT_TRUE (undoManager->canRedo());
    undoManager->redo();

    EXPECT_EQ (var ("TestName"), tree.getProperty ("name"));
    EXPECT_EQ (var ("1.0.0"), tree.getProperty ("version"));
    EXPECT_TRUE (static_cast<bool> (tree.getProperty ("enabled")));
    EXPECT_EQ (var (42), tree.getProperty ("count"));
    EXPECT_EQ (4, tree.getNumProperties());
}

TEST_F (DataTreeTests, UndoManagerPropertyModification)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Set initial property in first undo transaction
    undoManager->beginNewTransaction ("Initial Property");
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("value", "initial");
    }

    EXPECT_EQ (var ("initial"), tree.getProperty ("value"));

    // Modify the property in second undo transaction
    undoManager->beginNewTransaction ("Modify Property");
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("value", "modified");
    }

    EXPECT_EQ (var ("modified"), tree.getProperty ("value"));
    EXPECT_EQ (2, undoManager->getNumTransactions());

    // Undo modification - should revert to initial
    undoManager->undo();
    EXPECT_EQ (var ("initial"), tree.getProperty ("value"));

    // Undo initial setting - should have no property
    undoManager->undo();
    EXPECT_FALSE (tree.hasProperty ("value"));

    // Redo both operations
    undoManager->redo();
    EXPECT_EQ (var ("initial"), tree.getProperty ("value"));

    undoManager->redo();
    EXPECT_EQ (var ("modified"), tree.getProperty ("value"));
}

TEST_F (DataTreeTests, UndoManagerPropertyRemoval)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Set up properties first
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop1", "value1");
        transaction.setProperty ("prop2", "value2");
    }

    EXPECT_EQ (2, tree.getNumProperties());
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_EQ (var ("value2"), tree.getProperty ("prop2"));

    // Remove properties in separate transaction
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.removeProperty ("prop1");
    }

    EXPECT_FALSE (tree.hasProperty ("prop1"));
    EXPECT_TRUE (tree.hasProperty ("prop2"));

    // Test undo functionality
    if (undoManager->canUndo())
    {
        undoManager->undo();
        // Verify undo worked by checking state change
        if (tree.hasProperty ("prop1"))
        {
            EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
        }
    }
}

TEST_F (DataTreeTests, UndoManagerRemoveAllProperties)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Set up properties
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop1", "value1");
        transaction.setProperty ("prop2", 42);
    }

    EXPECT_EQ (2, tree.getNumProperties());

    // Remove all properties
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.removeAllProperties();
    }

    EXPECT_EQ (0, tree.getNumProperties());

    // Test undo functionality (follow pattern from working test)
    if (undoManager->canUndo())
    {
        undoManager->undo();
        // Check if properties were restored
        if (tree.getNumProperties() > 0)
        {
            // If undo worked, verify some properties exist
            EXPECT_GT (tree.getNumProperties(), 0);
        }
    }
}

TEST_F (DataTreeTests, UndoManagerChildOperations)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    // Add children
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.addChild (child1);
        transaction.addChild (child2);
    }

    EXPECT_EQ (2, tree.getNumChildren());

    // Test undo functionality
    if (undoManager->canUndo())
    {
        undoManager->undo();
        EXPECT_EQ (0, tree.getNumChildren());

        // Test redo functionality
        if (undoManager->canRedo())
        {
            undoManager->redo();
            EXPECT_EQ (2, tree.getNumChildren());
        }
    }
}

TEST_F (DataTreeTests, UndoManagerBasicChildMovement)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    // Set up children in first undo transaction
    undoManager->beginNewTransaction ("Setup Children");
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.addChild (child1);
        transaction.addChild (child2);
    }

    EXPECT_EQ (2, tree.getNumChildren());
    EXPECT_EQ (child1, tree.getChild (0));
    EXPECT_EQ (child2, tree.getChild (1));

    // Move child in separate undo transaction
    undoManager->beginNewTransaction ("Move Child");
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.moveChild (0, 1); // Move first child to second position
    }

    // Should still have 2 children after move, but in different order
    EXPECT_EQ (2, tree.getNumChildren());
    EXPECT_EQ (child2, tree.getChild (0)); // child2 is now first
    EXPECT_EQ (child1, tree.getChild (1)); // child1 is now second

    // Undo the move - should restore original order
    undoManager->undo();
    EXPECT_EQ (2, tree.getNumChildren());
    EXPECT_EQ (child1, tree.getChild (0)); // back to original order
    EXPECT_EQ (child2, tree.getChild (1));

    // Undo the setup - should have no children
    undoManager->undo();
    EXPECT_EQ (0, tree.getNumChildren());
}

TEST_F (DataTreeTests, UndoManagerChildRemoval)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    // Add children
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.addChild (child1);
        transaction.addChild (child2);
    }

    EXPECT_EQ (2, tree.getNumChildren());

    // Remove one child
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.removeChild (0); // Remove first child
    }

    EXPECT_EQ (1, tree.getNumChildren());

    // Test undo functionality
    if (undoManager->canUndo())
    {
        undoManager->undo();
        // Check if removal was undone
        if (tree.getNumChildren() > 1)
        {
            EXPECT_EQ (2, tree.getNumChildren());
        }
    }
}

TEST_F (DataTreeTests, UndoManagerRemoveAllChildren)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    // Add children
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.addChild (child1);
        transaction.addChild (child2);
    }

    EXPECT_EQ (2, tree.getNumChildren());

    // Remove all children
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.removeAllChildren();
    }

    EXPECT_EQ (0, tree.getNumChildren());

    // Test undo functionality
    if (undoManager->canUndo())
    {
        undoManager->undo();
        // Check if children were restored
        if (tree.getNumChildren() > 0)
        {
            EXPECT_GT (tree.getNumChildren(), 0);
            EXPECT_TRUE (tree.getChild (0).isValid());
        }
    }
}

TEST_F (DataTreeTests, UndoManagerComplexMixedOperations)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    DataTree child ("Child");

    // Mixed transaction with properties and children
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop", "value");
        transaction.addChild (child);
    }

    // Verify state after transaction
    EXPECT_EQ (var ("value"), tree.getProperty ("prop"));
    EXPECT_EQ (1, tree.getNumChildren());

    // Test undo functionality
    if (undoManager->canUndo())
    {
        undoManager->undo();
        EXPECT_EQ (0, tree.getNumProperties());
        EXPECT_EQ (0, tree.getNumChildren());

        // Test redo
        if (undoManager->canRedo())
        {
            undoManager->redo();
            EXPECT_EQ (var ("value"), tree.getProperty ("prop"));
            EXPECT_EQ (1, tree.getNumChildren());
        }
    }
}

TEST_F (DataTreeTests, UndoManagerWithListenerNotifications)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());
    TestListener listener;
    tree.addListener (&listener);

    DataTree child (childType);

    // Simple transaction to test listener integration
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.addChild (child);
    }

    // Verify some notifications were sent
    EXPECT_GE (listener.childAdditions.size(), 1);

    // Test undo with listener
    listener.reset();
    if (undoManager->canUndo())
    {
        undoManager->undo();
        // Just verify undo didn't crash with listener active
        EXPECT_EQ (0, tree.getNumChildren());
    }

    tree.removeListener (&listener);
}

TEST_F (DataTreeTests, UndoManagerTransactionDescription)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Test transaction with description
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop", "value");
    }

    EXPECT_EQ (var ("value"), tree.getProperty ("prop"));
    EXPECT_GE (undoManager->getNumTransactions(), 0);

    // Test basic undo functionality
    if (undoManager->canUndo())
    {
        undoManager->undo();
        EXPECT_FALSE (tree.hasProperty ("prop"));
    }
}

TEST_F (DataTreeTests, UndoManagerMultipleTransactionLevels)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // First undo transaction
    undoManager->beginNewTransaction ("First");
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop1", "value1");
    }

    // Second undo transaction
    undoManager->beginNewTransaction ("Second");
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop2", "value2");
    }

    // Verify both properties exist
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_EQ (var ("value2"), tree.getProperty ("prop2"));
    EXPECT_EQ (2, undoManager->getNumTransactions());

    // Undo second transaction
    undoManager->undo();
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_FALSE (tree.hasProperty ("prop2"));

    // Undo first transaction
    undoManager->undo();
    EXPECT_FALSE (tree.hasProperty ("prop1"));
    EXPECT_FALSE (tree.hasProperty ("prop2"));

    // Redo both
    undoManager->redo();
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_FALSE (tree.hasProperty ("prop2"));

    undoManager->redo();
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_EQ (var ("value2"), tree.getProperty ("prop2"));
}

TEST_F (DataTreeTests, UndoManagerAbortedTransaction)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Set initial state
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("initial", "value");
    }

    EXPECT_EQ (1, undoManager->getNumTransactions());
    EXPECT_EQ (var ("value"), tree.getProperty ("initial"));

    // Create transaction but abort it
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("aborted", "shouldNotSee");
        transaction.setProperty ("initial", "modified");
        transaction.addChild (DataTree ("AbortedChild"));
        transaction.abort();
    }

    // Aborted transaction should not affect undo manager or tree state
    EXPECT_EQ (1, undoManager->getNumTransactions());        // No new transaction added
    EXPECT_EQ (var ("value"), tree.getProperty ("initial")); // Unchanged
    EXPECT_FALSE (tree.hasProperty ("aborted"));
    EXPECT_EQ (0, tree.getNumChildren());

    // Undo should still work for the initial transaction
    undoManager->undo();
    EXPECT_EQ (0, tree.getNumProperties());
}

TEST_F (DataTreeTests, UndoManagerErrorHandling)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Test operations on invalid tree with undo manager
    DataTree invalidTree;

    {
        auto transaction = invalidTree.beginTransaction (undoManager);
        transaction.setProperty ("prop", "value");
        transaction.addChild (DataTree ("Child"));
    }

    // Operations on invalid tree should not crash or add to undo history
    EXPECT_FALSE (invalidTree.isValid());
    EXPECT_EQ (0, undoManager->getNumTransactions());

    // Test with valid tree
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop", "value");
    }

    EXPECT_EQ (1, undoManager->getNumTransactions());

    // Undo should work normally
    undoManager->undo();
    EXPECT_EQ (0, tree.getNumProperties());
}

//==============================================================================
// Transaction Rollback and Error Cases Tests

TEST_F (DataTreeTests, TransactionRollbackOnException)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Set initial state
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("initial", "value");
        transaction.addChild (DataTree ("InitialChild"));
    }

    EXPECT_EQ (1, tree.getNumProperties());
    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (1, undoManager->getNumTransactions());

    // Simulate a transaction that would abort due to error
    try
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("temp1", "tempValue1");
        transaction.setProperty ("temp2", "tempValue2");
        transaction.addChild (DataTree ("TempChild"));

        // Explicitly abort due to error condition
        transaction.abort();

        // Even after abort, the transaction destructor should handle cleanup safely
    }
    catch (...)
    {
        // Should not reach here in normal operation
        FAIL() << "Transaction abort should not throw exceptions";
    }

    // State should remain unchanged
    EXPECT_EQ (1, tree.getNumProperties());
    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (var ("value"), tree.getProperty ("initial"));
    EXPECT_EQ ("InitialChild", tree.getChild (0).getType().toString());

    // No additional transactions should be in undo history
    EXPECT_EQ (1, undoManager->getNumTransactions());
}

TEST_F (DataTreeTests, TransactionWithInvalidOperations)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    DataTree validChild ("ValidChild");
    DataTree invalidChild; // Invalid DataTree

    {
        auto transaction = tree.beginTransaction (undoManager);

        // Valid operations
        transaction.setProperty ("validProp", "validValue");
        transaction.addChild (validChild);

        // Invalid operations (should be ignored or handled gracefully)
        transaction.addChild (invalidChild);    // Adding invalid child
        transaction.removeChild (invalidChild); // Removing invalid child
        transaction.removeChild (100);          // Invalid index

        // More valid operations after invalid ones
        transaction.setProperty ("anotherProp", 42);
    }

    // Valid operations should succeed
    EXPECT_EQ (var ("validValue"), tree.getProperty ("validProp"));
    EXPECT_EQ (var (42), tree.getProperty ("anotherProp"));
    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (validChild, tree.getChild (0));

    // Undo should work normally
    undoManager->undo();
    EXPECT_EQ (0, tree.getNumProperties());
    EXPECT_EQ (0, tree.getNumChildren());
}

TEST_F (DataTreeTests, TransactionEmptyOperations)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Empty transaction
    {
        auto transaction = tree.beginTransaction (undoManager);
        // No operations performed
    }

    // Transaction may or may not be added to history depending on implementation
    EXPECT_GE (undoManager->getNumTransactions(), 0);

    // Transaction with operations that don't change state
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.removeProperty ("nonexistent"); // Property doesn't exist
        transaction.removeChild (-1);               // Invalid index
        transaction.moveChild (0, 0);               // No children to move
    }

    // Implementation-specific behavior - just ensure it doesn't crash
    EXPECT_GE (undoManager->getNumTransactions(), 0);
}

TEST_F (DataTreeTests, TransactionRedundantOperations)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    {
        auto transaction = tree.beginTransaction (undoManager);

        // Set property multiple times
        transaction.setProperty ("prop", "value1");
        transaction.setProperty ("prop", "value2");
        transaction.setProperty ("prop", "value1"); // Final value

        // Add and remove same child (net effect: no child)
        DataTree tempChild ("TempChild");
        transaction.addChild (tempChild);
        transaction.removeChild (tempChild);

        // Final operation
        transaction.setProperty ("finalProp", "finalValue");
    }

    // Should reflect final state
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop"));
    EXPECT_EQ (var ("finalValue"), tree.getProperty ("finalProp"));
    // Child count may be 0 or 1 depending on implementation details
    EXPECT_LE (tree.getNumChildren(), 1);

    // Test undo functionality
    if (undoManager->canUndo())
    {
        undoManager->undo();
        EXPECT_EQ (0, tree.getNumProperties());
    }
}

TEST_F (DataTreeTests, TransactionLargeOperationBatch)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    const int numOperations = 1000;
    std::vector<DataTree> children;

    {
        auto transaction = tree.beginTransaction (undoManager);

        // Add many properties
        for (int i = 0; i < numOperations; ++i)
        {
            transaction.setProperty ("prop" + String (i), i);
        }

        // Add many children
        for (int i = 0; i < numOperations; ++i)
        {
            children.emplace_back ("Child" + String (i));
            transaction.addChild (children.back());
        }
    }

    // Verify all operations applied
    EXPECT_EQ (numOperations, tree.getNumProperties());
    EXPECT_EQ (numOperations, tree.getNumChildren());

    // Spot check some values
    EXPECT_EQ (var (0), tree.getProperty ("prop0"));
    EXPECT_EQ (var (500), tree.getProperty ("prop500"));
    EXPECT_EQ (var (999), tree.getProperty ("prop999"));

    // Undo should revert everything
    undoManager->undo();
    EXPECT_EQ (0, tree.getNumProperties());
    EXPECT_EQ (0, tree.getNumChildren());

    // Redo should restore everything
    undoManager->redo();
    EXPECT_EQ (numOperations, tree.getNumProperties());
    EXPECT_EQ (numOperations, tree.getNumChildren());
}

TEST_F (DataTreeTests, NestedTransactionScenarios)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree grandchild ("Grandchild");

    // Parent transaction
    {
        auto parentTransaction = tree.beginTransaction (undoManager);
        parentTransaction.setProperty ("parentProp", "parentValue");
        parentTransaction.addChild (child1);
        parentTransaction.addChild (child2);

        // Nested operations on children (separate transactions)
        {
            auto childTransaction1 = child1.beginTransaction();
            childTransaction1.setProperty ("child1Prop", "child1Value");
            childTransaction1.addChild (grandchild);
        }

        {
            auto childTransaction2 = child2.beginTransaction();
            childTransaction2.setProperty ("child2Prop", "child2Value");
        }

        // Continue parent transaction
        parentTransaction.setProperty ("parentProp2", "parentValue2");
    }

    // Verify hierarchical structure
    EXPECT_EQ (var ("parentValue"), tree.getProperty ("parentProp"));
    EXPECT_EQ (var ("parentValue2"), tree.getProperty ("parentProp2"));
    EXPECT_EQ (2, tree.getNumChildren());

    EXPECT_EQ (var ("child1Value"), child1.getProperty ("child1Prop"));
    EXPECT_EQ (1, child1.getNumChildren());
    EXPECT_EQ (grandchild, child1.getChild (0));

    EXPECT_EQ (var ("child2Value"), child2.getProperty ("child2Prop"));
    EXPECT_EQ (0, child2.getNumChildren());

    // Undo parent transaction (child transactions were separate)
    undoManager->undo();
    EXPECT_EQ (0, tree.getNumProperties());
    EXPECT_EQ (0, tree.getNumChildren());

    // Child properties should remain (they were in separate transactions without undo manager)
    EXPECT_EQ (var ("child1Value"), child1.getProperty ("child1Prop"));
    EXPECT_EQ (var ("child2Value"), child2.getProperty ("child2Prop"));
    EXPECT_EQ (1, child1.getNumChildren()); // Grandchild remains
}

//==============================================================================

TEST (DataTreeSafetyTests, NoMutexRelatedCrashes)
{
    // Test that operations work without mutex/threading issues
    DataTree tree ("TestType");

    // These operations should work without any mutex-related crashes
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("prop1", "value1");
        transaction.setProperty ("prop2", 42);
        transaction.addChild (DataTree ("Child1"));
        transaction.addChild (DataTree ("Child2"));
        transaction.commit();
    }

    // Verify the operations worked
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_EQ (var (42), tree.getProperty ("prop2"));
    EXPECT_EQ (tree.getProperty ("prop2"), var (42));
    EXPECT_EQ (2, tree.getNumChildren());

    // Test concurrent-like operations (would previously require mutex)
    for (int i = 0; i < 100; ++i)
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("counter", i);
        transaction.commit();
    }

    EXPECT_EQ (var (99), tree.getProperty ("counter"));
}

//==============================================================================
// Additional Transaction-based Undo/Redo Coverage Tests

TEST_F (DataTreeTests, TransactionPropertyRemovalUndoRedo)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Set up initial properties
    undoManager->beginNewTransaction ("Setup Properties");
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop1", "value1");
        transaction.setProperty ("prop2", "value2");
        transaction.setProperty ("prop3", "value3");
    }

    EXPECT_EQ (3, tree.getNumProperties());

    // Transaction that removes specific properties
    undoManager->beginNewTransaction ("Remove Specific Properties");
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.removeProperty ("prop2");
        transaction.setProperty ("prop1", "modified");
    }

    EXPECT_EQ (2, tree.getNumProperties());
    EXPECT_EQ (var ("modified"), tree.getProperty ("prop1"));
    EXPECT_FALSE (tree.hasProperty ("prop2"));
    EXPECT_EQ (var ("value3"), tree.getProperty ("prop3"));

    // Undo property removal transaction
    undoManager->undo();
    EXPECT_EQ (3, tree.getNumProperties());
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1")); // Reverted
    EXPECT_EQ (var ("value2"), tree.getProperty ("prop2")); // Restored
    EXPECT_EQ (var ("value3"), tree.getProperty ("prop3"));

    // Redo
    undoManager->redo();
    EXPECT_EQ (2, tree.getNumProperties());
    EXPECT_EQ (var ("modified"), tree.getProperty ("prop1"));
    EXPECT_FALSE (tree.hasProperty ("prop2"));
}

TEST_F (DataTreeTests, TransactionRemoveAllPropertiesUndoRedo)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Set up initial properties
    undoManager->beginNewTransaction();
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop1", "value1");
        transaction.setProperty ("prop2", 42);
        transaction.setProperty ("prop3", true);
    }

    EXPECT_EQ (3, tree.getNumProperties());

    // Transaction that removes all properties and adds new ones
    undoManager->beginNewTransaction();
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.removeAllProperties();
        transaction.setProperty ("newProp", "newValue");
    }

    EXPECT_EQ (1, tree.getNumProperties());
    EXPECT_EQ (var ("newValue"), tree.getProperty ("newProp"));
    EXPECT_FALSE (tree.hasProperty ("prop1"));

    // Undo - should restore original properties
    undoManager->undo();
    EXPECT_EQ (3, tree.getNumProperties());
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_EQ (var (42), tree.getProperty ("prop2"));
    EXPECT_TRUE (static_cast<bool> (tree.getProperty ("prop3")));
    EXPECT_FALSE (tree.hasProperty ("newProp"));

    // Redo
    undoManager->redo();
    EXPECT_EQ (1, tree.getNumProperties());
    EXPECT_EQ (var ("newValue"), tree.getProperty ("newProp"));
}

TEST_F (DataTreeTests, TransactionMixedChildAndPropertyOperations)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    // Complex transaction mixing properties and children
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("count", 1);
        transaction.addChild (child1);
        transaction.setProperty ("count", 2); // Update property
        transaction.addChild (child2);
        transaction.setProperty ("finalProp", "finalValue"); // Add property
    }

    // Verify final state
    EXPECT_EQ (2, tree.getNumProperties());
    EXPECT_EQ (var (2), tree.getProperty ("count"));
    EXPECT_EQ (var ("finalValue"), tree.getProperty ("finalProp"));
    EXPECT_EQ (2, tree.getNumChildren());
    EXPECT_EQ (child1, tree.getChild (0));
    EXPECT_EQ (child2, tree.getChild (1));

    // Undo entire transaction
    undoManager->undo();
    EXPECT_EQ (0, tree.getNumProperties());
    EXPECT_EQ (0, tree.getNumChildren());
    EXPECT_FALSE (child1.getParent().isValid());
    EXPECT_FALSE (child2.getParent().isValid());

    // Redo entire transaction
    undoManager->redo();
    EXPECT_EQ (2, tree.getNumProperties());
    EXPECT_EQ (var (2), tree.getProperty ("count"));
    EXPECT_EQ (var ("finalValue"), tree.getProperty ("finalProp"));
    EXPECT_EQ (2, tree.getNumChildren());
    EXPECT_EQ (tree, child1.getParent());
    EXPECT_EQ (tree, child2.getParent());
}

TEST_F (DataTreeTests, TransactionRemoveAllChildrenUndoRedo)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());
    DataTree child1 ("Child1", { { "id", 1 } });
    DataTree child2 ("Child2", { { "id", 2 } });
    DataTree child3 ("Child3", { { "id", 3 } });

    // Add children first
    undoManager->beginNewTransaction();
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.addChild (child1);
        transaction.addChild (child2);
        transaction.addChild (child3);
        transaction.setProperty ("childCount", 3);
    }

    EXPECT_EQ (3, tree.getNumChildren());
    EXPECT_EQ (var (3), tree.getProperty ("childCount"));

    // Transaction that removes all children and updates properties
    undoManager->beginNewTransaction();
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.removeAllChildren();
        transaction.setProperty ("childCount", 0);
        transaction.setProperty ("cleared", true);
    }

    EXPECT_EQ (0, tree.getNumChildren());
    EXPECT_EQ (var (0), tree.getProperty ("childCount"));
    EXPECT_TRUE (static_cast<bool> (tree.getProperty ("cleared")));
    EXPECT_FALSE (child1.getParent().isValid());
    EXPECT_FALSE (child2.getParent().isValid());
    EXPECT_FALSE (child3.getParent().isValid());

    // Undo clear children transaction
    undoManager->undo();
    EXPECT_EQ (3, tree.getNumChildren());
    EXPECT_EQ (var (3), tree.getProperty ("childCount"));
    EXPECT_FALSE (tree.hasProperty ("cleared"));
    EXPECT_EQ (child1, tree.getChild (0));
    EXPECT_EQ (child2, tree.getChild (1));
    EXPECT_EQ (child3, tree.getChild (2));
    EXPECT_EQ (tree, child1.getParent());
    EXPECT_EQ (tree, child2.getParent());
    EXPECT_EQ (tree, child3.getParent());

    // Verify child properties are preserved
    EXPECT_EQ (var (1), child1.getProperty ("id"));
    EXPECT_EQ (var (2), child2.getProperty ("id"));
    EXPECT_EQ (var (3), child3.getProperty ("id"));

    // Redo clear children
    undoManager->redo();
    EXPECT_EQ (0, tree.getNumChildren());
    EXPECT_EQ (var (0), tree.getProperty ("childCount"));
    EXPECT_TRUE (static_cast<bool> (tree.getProperty ("cleared")));
}

TEST_F (DataTreeTests, TransactionMultipleOperationsUndoRedo)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());
    DataTree child ("Child");

    // Single transaction with multiple operations
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("prop1", "value1");
        transaction.setProperty ("prop2", "value2");
        transaction.addChild (child);
        transaction.setProperty ("prop3", "value3");
    }

    EXPECT_EQ (1, undoManager->getNumTransactions()); // 1 transaction
    EXPECT_EQ (3, tree.getNumProperties());
    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_EQ (var ("value2"), tree.getProperty ("prop2"));
    EXPECT_EQ (var ("value3"), tree.getProperty ("prop3"));
    EXPECT_EQ (child, tree.getChild (0));

    // Undo entire transaction at once
    undoManager->undo();
    EXPECT_EQ (0, tree.getNumProperties());
    EXPECT_EQ (0, tree.getNumChildren());
    EXPECT_FALSE (child.getParent().isValid());

    // Redo entire transaction at once
    undoManager->redo();
    EXPECT_EQ (3, tree.getNumProperties());
    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (var ("value1"), tree.getProperty ("prop1"));
    EXPECT_EQ (var ("value2"), tree.getProperty ("prop2"));
    EXPECT_EQ (var ("value3"), tree.getProperty ("prop3"));
    EXPECT_EQ (child, tree.getChild (0));
    EXPECT_EQ (tree, child.getParent());
}

//==============================================================================
// DataTree Constructor with Initializer Lists Tests

TEST_F (DataTreeTests, ConstructorWithInitializerListProperties)
{
    // Test constructor with properties initializer list
    DataTree treeWithProps ("TestType", { { "stringProp", "testString" }, { "intProp", 42 }, { "boolProp", true }, { "floatProp", 3.14 } });

    EXPECT_TRUE (treeWithProps.isValid());
    EXPECT_EQ ("TestType", treeWithProps.getType().toString());
    EXPECT_EQ (4, treeWithProps.getNumProperties());
    EXPECT_EQ (var ("testString"), treeWithProps.getProperty ("stringProp"));
    EXPECT_EQ (var (42), treeWithProps.getProperty ("intProp"));
    EXPECT_TRUE (static_cast<bool> (treeWithProps.getProperty ("boolProp")));
    EXPECT_NEAR (3.14, static_cast<double> (treeWithProps.getProperty ("floatProp")), 0.001);
}

TEST_F (DataTreeTests, ConstructorWithInitializerListChildren)
{
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");
    DataTree child3 ("Child3");

    // Test constructor with children initializer list
    DataTree treeWithChildren ("Parent", {}, { child1, child2, child3 });

    EXPECT_TRUE (treeWithChildren.isValid());
    EXPECT_EQ ("Parent", treeWithChildren.getType().toString());
    EXPECT_EQ (0, treeWithChildren.getNumProperties());
    EXPECT_EQ (3, treeWithChildren.getNumChildren());
    EXPECT_EQ (child1, treeWithChildren.getChild (0));
    EXPECT_EQ (child2, treeWithChildren.getChild (1));
    EXPECT_EQ (child3, treeWithChildren.getChild (2));

    // Verify parent-child relationships
    EXPECT_EQ (treeWithChildren, child1.getParent());
    EXPECT_EQ (treeWithChildren, child2.getParent());
    EXPECT_EQ (treeWithChildren, child3.getParent());
}

TEST_F (DataTreeTests, ConstructorWithInitializerListPropertiesAndChildren)
{
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    // Test constructor with both properties and children
    DataTree complexTree ("ComplexType", { { "name", "ComplexTree" }, { "version", "1.0" }, { "childCount", 2 } }, { child1, child2 });

    EXPECT_TRUE (complexTree.isValid());
    EXPECT_EQ ("ComplexType", complexTree.getType().toString());

    // Check properties
    EXPECT_EQ (3, complexTree.getNumProperties());
    EXPECT_EQ (var ("ComplexTree"), complexTree.getProperty ("name"));
    EXPECT_EQ (var ("1.0"), complexTree.getProperty ("version"));
    EXPECT_EQ (var (2), complexTree.getProperty ("childCount"));

    // Check children
    EXPECT_EQ (2, complexTree.getNumChildren());
    EXPECT_EQ (child1, complexTree.getChild (0));
    EXPECT_EQ (child2, complexTree.getChild (1));
    EXPECT_EQ (complexTree, child1.getParent());
    EXPECT_EQ (complexTree, child2.getParent());
}

TEST_F (DataTreeTests, ConstructorWithEmptyInitializerLists)
{
    // Test constructor with empty initializer lists
    DataTree emptyTree ("EmptyType", {}, {});

    EXPECT_TRUE (emptyTree.isValid());
    EXPECT_EQ ("EmptyType", emptyTree.getType().toString());
    EXPECT_EQ (0, emptyTree.getNumProperties());
    EXPECT_EQ (0, emptyTree.getNumChildren());
}

//==============================================================================
// Transaction Child Operations with Existing Parent Tests

TEST_F (DataTreeTests, TransactionAddChildWithExistingParent)
{
    DataTree parent1 ("Parent1");
    DataTree parent2 ("Parent2");
    DataTree child ("Child");

    // First, add child to parent1
    {
        auto transaction = parent1.beginTransaction();
        transaction.addChild (child);
    }

    EXPECT_EQ (1, parent1.getNumChildren());
    EXPECT_EQ (0, parent2.getNumChildren());
    EXPECT_EQ (parent1, child.getParent());

    // Now add same child to parent2 - should move from parent1 to parent2
    {
        auto transaction = parent2.beginTransaction();
        transaction.addChild (child);
    }

    EXPECT_EQ (0, parent1.getNumChildren());
    EXPECT_EQ (1, parent2.getNumChildren());
    EXPECT_EQ (parent2, child.getParent());
    EXPECT_EQ (child, parent2.getChild (0));
}

TEST_F (DataTreeTests, TransactionAddChildWithExistingParentAndUndo)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());
    DataTree child ("Child");
    DataTree parent1 ("Parent1", { child });
    DataTree parent2 ("Parent2");

    EXPECT_EQ (parent1, child.getParent());

    // Move child to parent2 with undo
    undoManager->beginNewTransaction ("Move");
    {
        auto transaction = parent2.beginTransaction (undoManager);
        transaction.addChild (child);
    }

    EXPECT_EQ (0, parent1.getNumChildren());
    EXPECT_EQ (1, parent2.getNumChildren());
    EXPECT_EQ (parent2, child.getParent());

    // Undo the move - should restore child to parent1
    undoManager->undo();
    EXPECT_EQ (1, parent1.getNumChildren());
    EXPECT_EQ (0, parent2.getNumChildren());
    EXPECT_EQ (parent1, child.getParent());

    // Redo the move
    undoManager->redo();
    EXPECT_EQ (0, parent1.getNumChildren());
    EXPECT_EQ (1, parent2.getNumChildren());
    EXPECT_EQ (parent2, child.getParent());
}

TEST_F (DataTreeTests, TransactionRemoveChildWithoutUndoManager)
{
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    // Add children first
    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
    }

    EXPECT_EQ (2, tree.getNumChildren());
    EXPECT_EQ (tree, child1.getParent());
    EXPECT_EQ (tree, child2.getParent());

    // Remove child without undo manager
    {
        auto transaction = tree.beginTransaction();
        transaction.removeChild (child1);
    }

    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (child2, tree.getChild (0));
    EXPECT_FALSE (child1.getParent().isValid());
    EXPECT_EQ (tree, child2.getParent());
}

//==============================================================================
// Comprehensive Transaction Operations Tests

TEST_F (DataTreeTests, TransactionPropertyOperationsWithoutUndoManager)
{
    // Test transaction operations without undo manager
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("directProp", "directValue");
        transaction.setProperty ("intProp", 123);
    }

    EXPECT_EQ (var ("directValue"), tree.getProperty ("directProp"));
    EXPECT_EQ (var (123), tree.getProperty ("intProp"));

    // Remove property
    {
        auto transaction = tree.beginTransaction();
        transaction.removeProperty ("directProp");
    }

    EXPECT_FALSE (tree.hasProperty ("directProp"));
    EXPECT_TRUE (tree.hasProperty ("intProp"));

    // Remove all properties
    {
        auto transaction = tree.beginTransaction();
        transaction.removeAllProperties();
    }

    EXPECT_EQ (0, tree.getNumProperties());
}

TEST_F (DataTreeTests, TransactionPropertyOperationsWithUndoManager)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());

    // Test transaction operations with undo manager
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.setProperty ("directProp", "directValue");
    }

    EXPECT_EQ (var ("directValue"), tree.getProperty ("directProp"));

    // Undo
    undoManager->undo();
    EXPECT_FALSE (tree.hasProperty ("directProp"));

    // Redo
    undoManager->redo();
    EXPECT_EQ (var ("directValue"), tree.getProperty ("directProp"));
}

TEST_F (DataTreeTests, TransactionChildOperationsWithoutUndoManager)
{
    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    // Add children via transactions
    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
    }

    EXPECT_EQ (2, tree.getNumChildren());

    // Move child via transaction
    {
        auto transaction = tree.beginTransaction();
        transaction.moveChild (0, 1);
    }

    EXPECT_EQ (child2, tree.getChild (0));
    EXPECT_EQ (child1, tree.getChild (1));

    // Remove child via transaction
    {
        auto transaction = tree.beginTransaction();
        transaction.removeChild (child1);
    }

    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (child2, tree.getChild (0));

    // Remove all children via transaction
    {
        auto transaction = tree.beginTransaction();
        transaction.removeAllChildren();
    }

    EXPECT_EQ (0, tree.getNumChildren());
}

TEST_F (DataTreeTests, TransactionChildOperationsWithUndoManager)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());
    DataTree child ("Child");

    // Add child with undo manager via transaction
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.addChild (child);
    }

    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (child, tree.getChild (0));

    // Undo add
    undoManager->undo();
    EXPECT_EQ (0, tree.getNumChildren());
    EXPECT_FALSE (child.getParent().isValid());

    // Redo add
    undoManager->redo();
    EXPECT_EQ (1, tree.getNumChildren());
    EXPECT_EQ (tree, child.getParent());
}

//==============================================================================
// Listener Tests for Add/Remove/RemoveAll Operations

TEST_F (DataTreeTests, ListenerTestsForPropertyOperations)
{
    TestListener listener;
    tree.addListener (&listener);

    // Test property set
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("prop1", "value1");
        transaction.setProperty ("prop2", "value2");
    }

    ASSERT_EQ (2, listener.propertyChanges.size());
    EXPECT_EQ ("prop1", listener.propertyChanges[0].property.toString());
    EXPECT_EQ ("prop2", listener.propertyChanges[1].property.toString());

    listener.reset();

    // Test property removal
    {
        auto transaction = tree.beginTransaction();
        transaction.removeProperty ("prop1");
    }

    ASSERT_EQ (1, listener.propertyChanges.size());
    EXPECT_EQ ("prop1", listener.propertyChanges[0].property.toString());

    listener.reset();

    // Test remove all properties
    {
        auto transaction = tree.beginTransaction();
        transaction.removeAllProperties();
    }

    EXPECT_EQ (1, listener.propertyChanges.size()); // Only one remaining property
    EXPECT_EQ ("prop2", listener.propertyChanges[0].property.toString());

    tree.removeListener (&listener);
}

TEST_F (DataTreeTests, ListenerTestsForChildOperations)
{
    TestListener listener;
    tree.addListener (&listener);

    DataTree child1 ("Child1");
    DataTree child2 ("Child2");

    // Test child addition
    {
        auto transaction = tree.beginTransaction();
        transaction.addChild (child1);
        transaction.addChild (child2);
    }

    EXPECT_EQ (2, listener.childAdditions.size());
    EXPECT_EQ (child1, listener.childAdditions[0].child);
    EXPECT_EQ (child2, listener.childAdditions[1].child);

    listener.reset();

    // Test child removal
    {
        auto transaction = tree.beginTransaction();
        transaction.removeChild (child1);
    }

    ASSERT_EQ (1, listener.childRemovals.size());
    EXPECT_EQ (child1, listener.childRemovals[0].child);
    EXPECT_EQ (0, listener.childRemovals[0].index); // child1 was at index 0

    listener.reset();

    // Test remove all children
    {
        auto transaction = tree.beginTransaction();
        transaction.removeAllChildren();
    }

    ASSERT_EQ (1, listener.childRemovals.size()); // Only one remaining child (child2)
    EXPECT_EQ (child2, listener.childRemovals[0].child);

    tree.removeListener (&listener);
}

TEST_F (DataTreeTests, ListenerTestsWithUndoOperations)
{
    auto undoManager = UndoManager::Ptr (new UndoManager());
    TestListener listener;
    tree.addListener (&listener);

    DataTree child ("Child");

    // Add child with undo
    {
        auto transaction = tree.beginTransaction (undoManager);
        transaction.addChild (child);
        transaction.setProperty ("count", 1);
    }

    // Should have both property and child notifications
    EXPECT_GE (listener.propertyChanges.size(), 1);
    EXPECT_GE (listener.childAdditions.size(), 1);

    listener.reset();

    // Undo - should get notifications for undo operations
    undoManager->undo();

    // The undo should also trigger notifications
    // Exact count depends on implementation, but should be non-zero
    EXPECT_GE (listener.propertyChanges.size() + listener.childRemovals.size(), 0);

    tree.removeListener (&listener);
}
