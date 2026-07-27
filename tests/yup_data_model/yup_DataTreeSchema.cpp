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
// Test schema definitions

const String simpleSchema = R"({
    "nodeTypes": {
        "Settings": {
            "description": "Application settings node",
            "properties": {
                "theme": {
                    "type": "string",
                    "default": "light",
                    "enum": ["light", "dark", "auto"]
                },
                "fontSize": {
                    "type": "number",
                    "default": 12,
                    "minimum": 8,
                    "maximum": 72
                },
                "enabled": {
                    "type": "boolean",
                    "default": true
                },
                "name": {
                    "type": "string",
                    "required": true,
                    "minLength": 1,
                    "maxLength": 100
                }
            },
            "children": {
                "maxCount": 0
            }
        },
        "Root": {
            "properties": {
                "version": {
                    "type": "string",
                    "required": true,
                    "default": "1.0.0"
                }
            },
            "children": {
                "allowedTypes": ["Settings", "UserData"],
                "minCount": 0,
                "maxCount": 10
            }
        },
        "UserData": {
            "properties": {
                "username": {
                    "type": "string",
                    "required": true
                },
                "age": {
                    "type": "number",
                    "minimum": 0,
                    "maximum": 150
                }
            },
            "children": {
                "allowedTypes": [],
                "maxCount": 0
            }
        }
    }
})";

} // namespace

//==============================================================================
class DataTreeSchemaTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        var result;
        ASSERT_TRUE (JSON::parse (simpleSchema, result));

        schema = DataTreeSchema::fromJsonSchema (result);
        ASSERT_NE (nullptr, schema);
    }

    DataTreeSchema::Ptr schema;
};

//==============================================================================
TEST_F (DataTreeSchemaTests, SchemaLoading)
{
    EXPECT_TRUE (schema->isValid());

    // Test invalid JSON
    auto invalidSchema = DataTreeSchema::fromJsonSchemaString ("invalid json");
    EXPECT_EQ (nullptr, invalidSchema);

    // Test empty schema
    auto emptySchema = DataTreeSchema::fromJsonSchemaString ("{}");
    EXPECT_EQ (nullptr, emptySchema);

    // Test empty schema from var
    auto emptySchemaVar = DataTreeSchema::fromJsonSchema (var());
    EXPECT_EQ (nullptr, emptySchemaVar);
}

TEST_F (DataTreeSchemaTests, NodeTypeQueries)
{
    // Test node type existence
    EXPECT_TRUE (schema->hasNodeType ("Settings"));
    EXPECT_TRUE (schema->hasNodeType ("Root"));
    EXPECT_TRUE (schema->hasNodeType ("UserData"));
    EXPECT_FALSE (schema->hasNodeType ("NonExistent"));

    // Test node type names
    auto nodeTypes = schema->getNodeTypeNames();
    EXPECT_EQ (3, nodeTypes.size());
    EXPECT_TRUE (nodeTypes.contains ("Settings"));
    EXPECT_TRUE (nodeTypes.contains ("Root"));
    EXPECT_TRUE (nodeTypes.contains ("UserData"));
}

TEST_F (DataTreeSchemaTests, PropertyInfoQueries)
{
    // Test Settings properties
    auto settingsProps = schema->getPropertyNames ("Settings");
    EXPECT_EQ (4, settingsProps.size());
    EXPECT_TRUE (settingsProps.contains ("theme"));
    EXPECT_TRUE (settingsProps.contains ("fontSize"));
    EXPECT_TRUE (settingsProps.contains ("enabled"));
    EXPECT_TRUE (settingsProps.contains ("name"));

    // Test required properties
    auto requiredProps = schema->getRequiredPropertyNames ("Settings");
    EXPECT_EQ (1, requiredProps.size());
    EXPECT_TRUE (requiredProps.contains ("name"));

    // Test specific property info
    auto themeInfo = schema->getPropertyInfo ("Settings", "theme");
    EXPECT_EQ ("string", themeInfo.type);
    EXPECT_FALSE (themeInfo.required);
    EXPECT_TRUE (themeInfo.hasDefault());
    EXPECT_EQ ("light", themeInfo.defaultValue.toString());
    EXPECT_TRUE (themeInfo.isEnum());
    EXPECT_EQ (3, themeInfo.enumValues.size());

    auto fontSizeInfo = schema->getPropertyInfo ("Settings", "fontSize");
    EXPECT_EQ ("number", fontSizeInfo.type);
    EXPECT_TRUE (fontSizeInfo.hasNumericConstraints());
    EXPECT_EQ (8.0, fontSizeInfo.minimum.value());
    EXPECT_EQ (72.0, fontSizeInfo.maximum.value());

    auto nameInfo = schema->getPropertyInfo ("Settings", "name");
    EXPECT_TRUE (nameInfo.required);
    EXPECT_TRUE (nameInfo.hasLengthConstraints());
    EXPECT_EQ (1, nameInfo.minLength.value());
    EXPECT_EQ (100, nameInfo.maxLength.value());
}

TEST_F (DataTreeSchemaTests, ChildConstraintsQueries)
{
    // Test Settings child constraints (no children allowed)
    auto settingsConstraints = schema->getChildConstraints ("Settings");
    EXPECT_FALSE (settingsConstraints.allowsChildren());
    EXPECT_EQ (0, settingsConstraints.maxCount);

    // Test Root child constraints
    auto rootConstraints = schema->getChildConstraints ("Root");
    EXPECT_TRUE (rootConstraints.allowsChildren());
    EXPECT_FALSE (rootConstraints.allowsAnyType());
    EXPECT_EQ (0, rootConstraints.minCount);
    EXPECT_EQ (10, rootConstraints.maxCount);
    EXPECT_EQ (2, rootConstraints.allowedTypes.size());
    EXPECT_TRUE (rootConstraints.allowedTypes.contains ("Settings"));
    EXPECT_TRUE (rootConstraints.allowedTypes.contains ("UserData"));
}

TEST_F (DataTreeSchemaTests, NodeCreationWithDefaults)
{
    // Create Settings node with defaults
    auto settingsNode = schema->createNode ("Settings");
    EXPECT_TRUE (settingsNode.isValid());
    EXPECT_EQ ("Settings", settingsNode.getType().toString());

    // Check default values were set
    EXPECT_EQ ("light", settingsNode.getProperty ("theme").toString());
    EXPECT_EQ (12, static_cast<int> (settingsNode.getProperty ("fontSize")));
    EXPECT_EQ (true, static_cast<bool> (settingsNode.getProperty ("enabled")));

    // Required property without default should not be set
    EXPECT_FALSE (settingsNode.hasProperty ("name"));

    // Test invalid node type
    auto invalidNode = schema->createNode ("NonExistent");
    EXPECT_FALSE (invalidNode.isValid());
}

TEST_F (DataTreeSchemaTests, ChildNodeCreation)
{
    // Create valid child for Root
    auto settingsChild = schema->createChildNode ("Root", "Settings");
    EXPECT_TRUE (settingsChild.isValid());
    EXPECT_EQ ("Settings", settingsChild.getType().toString());

    // Create invalid child for Root
    auto invalidChild = schema->createChildNode ("Root", "NonExistent");
    EXPECT_FALSE (invalidChild.isValid());

    // Try to create child for node that doesn't allow children
    auto noChild = schema->createChildNode ("Settings", "UserData");
    EXPECT_FALSE (noChild.isValid());
}

TEST_F (DataTreeSchemaTests, PropertyValidation)
{
    // Valid string enum value
    auto result1 = schema->validatePropertyValue ("Settings", "theme", "dark");
    EXPECT_TRUE (result1.wasOk());

    // Invalid string enum value
    auto result2 = schema->validatePropertyValue ("Settings", "theme", "invalid");
    EXPECT_TRUE (result2.failed());
    EXPECT_TRUE (result2.getErrorMessage().contains ("allowed values"));

    // Valid number within range
    auto result3 = schema->validatePropertyValue ("Settings", "fontSize", 14);
    EXPECT_TRUE (result3.wasOk());

    // Number below minimum
    auto result4 = schema->validatePropertyValue ("Settings", "fontSize", 5);
    EXPECT_TRUE (result4.failed());
    EXPECT_TRUE (result4.getErrorMessage().contains ("minimum"));

    // Number above maximum
    auto result5 = schema->validatePropertyValue ("Settings", "fontSize", 100);
    EXPECT_TRUE (result5.failed());
    EXPECT_TRUE (result5.getErrorMessage().contains ("maximum"));

    // Wrong type
    auto result6 = schema->validatePropertyValue ("Settings", "fontSize", "not a number");
    EXPECT_TRUE (result6.failed());
    EXPECT_TRUE (result6.getErrorMessage().contains ("number"));

    // Unknown property
    auto result7 = schema->validatePropertyValue ("Settings", "unknown", "value");
    EXPECT_TRUE (result7.failed());
    EXPECT_TRUE (result7.getErrorMessage().contains ("Unknown property"));
}

TEST_F (DataTreeSchemaTests, ChildAdditionValidation)
{
    // Valid child addition
    auto result1 = schema->validateChildAddition ("Root", "Settings", 0);
    EXPECT_TRUE (result1.wasOk());

    // Invalid child type
    auto result2 = schema->validateChildAddition ("Root", "NonExistent", 0);
    EXPECT_TRUE (result2.failed());
    EXPECT_TRUE (result2.getErrorMessage().contains ("not allowed"));

    // Too many children
    auto result3 = schema->validateChildAddition ("Root", "Settings", 10);
    EXPECT_TRUE (result3.failed());
    EXPECT_TRUE (result3.getErrorMessage().contains ("maximum"));

    // Child to node that doesn't allow children
    auto result4 = schema->validateChildAddition ("Settings", "UserData", 0);
    EXPECT_TRUE (result4.failed());
    EXPECT_TRUE (result4.getErrorMessage().contains ("maximum"));
}

TEST_F (DataTreeSchemaTests, CompleteTreeValidation)
{
    // Create a valid tree structure
    auto root = schema->createNode ("Root");
    auto settings = schema->createNode ("Settings");
    auto userData = schema->createNode ("UserData");

    // Set required properties
    {
        auto rootTx = root.beginTransaction();
        rootTx.setProperty ("version", "2.0.0");
    }
    {
        auto settingsTx = settings.beginTransaction();
        settingsTx.setProperty ("name", "MySettings");
    }
    {
        auto userTx = userData.beginTransaction();
        userTx.setProperty ("username", "testuser");
        userTx.setProperty ("age", 25);
    }

    // Add children
    {
        auto rootTx = root.beginTransaction();
        rootTx.addChild (settings);
        rootTx.addChild (userData);
    }

    // Validate complete tree
    auto validationResult = schema->validate (root);
    EXPECT_TRUE (validationResult.wasOk()) << validationResult.getErrorMessage();

    // Test validation failure - remove required property
    {
        auto settingsTx = settings.beginTransaction();
        settingsTx.removeProperty ("name");
    }

    auto failResult = schema->validate (root);
    EXPECT_TRUE (failResult.failed());
    EXPECT_TRUE (failResult.getErrorMessage().contains ("Required property"));
}

TEST_F (DataTreeSchemaTests, ValidatedTransactionSuccess)
{
    auto settingsTree = schema->createNode ("Settings");

    // Valid transaction operations
    auto transaction = settingsTree.beginValidatedTransaction (schema);

    auto result1 = transaction.setProperty ("name", "Test Settings");
    EXPECT_TRUE (result1.wasOk());

    auto result2 = transaction.setProperty ("theme", "dark");
    EXPECT_TRUE (result2.wasOk());

    auto result3 = transaction.setProperty ("fontSize", 16);
    EXPECT_TRUE (result3.wasOk());

    // Transaction should auto-commit successfully
    EXPECT_TRUE (transaction.isActive());

    auto commitResult = transaction.commit();
    EXPECT_TRUE (commitResult.wasOk());
    EXPECT_FALSE (transaction.isActive());

    // Verify changes were applied
    EXPECT_EQ ("Test Settings", settingsTree.getProperty ("name").toString());
    EXPECT_EQ ("dark", settingsTree.getProperty ("theme").toString());
    EXPECT_EQ (16, static_cast<int> (settingsTree.getProperty ("fontSize")));
}

TEST_F (DataTreeSchemaTests, ValidatedTransactionFailures)
{
    auto settingsTree = schema->createNode ("Settings");

    auto transaction = settingsTree.beginValidatedTransaction (schema);

    // Invalid property value should fail
    auto result1 = transaction.setProperty ("theme", "invalid");
    EXPECT_TRUE (result1.failed());
    EXPECT_TRUE (result1.getErrorMessage().contains ("allowed values"));

    // Out of range number should fail
    auto result2 = transaction.setProperty ("fontSize", 150);
    EXPECT_TRUE (result2.failed());
    EXPECT_TRUE (result2.getErrorMessage().contains ("maximum"));

    // Try to remove required property
    {
        auto validTx = settingsTree.beginTransaction();
        validTx.setProperty ("name", "Test");
    }

    auto result3 = transaction.removeProperty ("name");
    EXPECT_TRUE (result3.failed());
    EXPECT_TRUE (result3.getErrorMessage().contains ("required"));

    // Transaction should not commit due to validation errors
    auto commitResult = transaction.commit();
    EXPECT_TRUE (commitResult.failed());

    // Changes should not be applied to the tree
    EXPECT_EQ ("light", settingsTree.getProperty ("theme").toString());       // Default value
    EXPECT_EQ (12, static_cast<int> (settingsTree.getProperty ("fontSize"))); // Default value
}

TEST_F (DataTreeSchemaTests, ValidatedTransactionChildOperations)
{
    auto rootTree = schema->createNode ("Root");

    auto transaction = rootTree.beginValidatedTransaction (schema);

    // Create and add valid child
    auto childResult = transaction.createAndAddChild ("Settings");
    EXPECT_TRUE (childResult.wasOk());

    DataTree settingsChild = childResult.getValue();
    EXPECT_TRUE (settingsChild.isValid());
    EXPECT_EQ ("Settings", settingsChild.getType().toString());

    // Try to create invalid child type
    auto invalidResult = transaction.createAndAddChild ("NonExistent");
    EXPECT_TRUE (invalidResult.failed());

    // Manually create and add child
    auto userData = schema->createNode ("UserData");
    {
        auto userTx = userData.beginTransaction();
        userTx.setProperty ("username", "testuser");
    }

    auto addResult = transaction.addChild (userData);
    EXPECT_TRUE (addResult.wasOk());

    auto commitResult = transaction.commit();
    EXPECT_TRUE (commitResult.wasOk());

    // Verify children were added
    EXPECT_EQ (2, rootTree.getNumChildren());
}

TEST (DataTreeSchemaChildCountConstraints, ValidatedTransactionsHonorConstraints)
{
    const String schemaJson = R"({
        "nodeTypes": {
            "Root": {
                "children": {
                    "allowedTypes": ["Child"],
                    "minCount": 1,
                    "maxCount": 2
                }
            },
            "Child": {
                "children": { "maxCount": 0 }
            }
        }
    })";

    var schemaVar;
    ASSERT_TRUE (JSON::parse (schemaJson, schemaVar));
    auto schema = DataTreeSchema::fromJsonSchema (schemaVar);
    ASSERT_NE (nullptr, schema);

    auto root = schema->createNode ("Root");
    ASSERT_TRUE (root.isValid());

    // Attempt to add three children in a single validated transaction; the third should fail.
    auto addTx = root.beginValidatedTransaction (schema);
    EXPECT_TRUE (addTx.createAndAddChild ("Child").wasOk());
    EXPECT_TRUE (addTx.createAndAddChild ("Child").wasOk());

    auto thirdChild = addTx.createAndAddChild ("Child");
    EXPECT_TRUE (thirdChild.failed());
    EXPECT_TRUE (addTx.commit().failed());
    addTx.abort();

    // Create two children in a plain transaction to reach the minimum count.
    {
        auto tx = root.beginTransaction();
        tx.addChild (schema->createNode ("Child"));
        tx.addChild (schema->createNode ("Child"));
    }

    // Removing one child is ok, removing below minCount should be rejected.
    auto removeTx = root.beginValidatedTransaction (schema);
    auto removeFirst = removeTx.removeChild (root.getChild (0));
    EXPECT_TRUE (removeFirst.wasOk());

    auto removeSecond = removeTx.removeChild (root.getChild (1));
    EXPECT_TRUE (removeSecond.failed());
    EXPECT_TRUE (removeSecond.getErrorMessage().contains ("minimum"));
    removeTx.abort();
}

TEST_F (DataTreeSchemaTests, SchemaRoundtripSerialization)
{
    // Export schema to JSON
    var exportedJson = schema->toJsonSchema();
    EXPECT_TRUE (exportedJson.isObject());

    // Create new schema from exported JSON
    auto reimportedSchema = DataTreeSchema::fromJsonSchema (exportedJson);
    ASSERT_NE (nullptr, reimportedSchema);
    EXPECT_TRUE (reimportedSchema->isValid());

    // Verify node types are preserved
    auto originalTypes = schema->getNodeTypeNames();
    auto reimportedTypes = reimportedSchema->getNodeTypeNames();
    EXPECT_EQ (originalTypes.size(), reimportedTypes.size());

    for (const auto& typeName : originalTypes)
    {
        EXPECT_TRUE (reimportedTypes.contains (typeName));

        // Verify property info is preserved
        auto originalProps = schema->getPropertyNames (typeName);
        auto reimportedProps = reimportedSchema->getPropertyNames (typeName);
        EXPECT_EQ (originalProps.size(), reimportedProps.size());

        for (const auto& propName : originalProps)
        {
            auto originalInfo = schema->getPropertyInfo (typeName, propName);
            auto reimportedInfo = reimportedSchema->getPropertyInfo (typeName, propName);

            EXPECT_EQ (originalInfo.type, reimportedInfo.type);
            EXPECT_EQ (originalInfo.required, reimportedInfo.required);
            EXPECT_EQ (originalInfo.defaultValue, reimportedInfo.defaultValue);
        }
    }
}

TEST_F (DataTreeSchemaTests, RealWorldUsageExample)
{
    // Comprehensive example mimicking real application usage

    // 1. Create root application tree with schema defaults
    auto appTree = schema->createNode ("Root");
    EXPECT_EQ ("1.0.0", appTree.getProperty ("version").toString()); // Default applied

    // 2. Use validated transaction to build complete structure
    auto buildTransaction = appTree.beginValidatedTransaction (schema);

    // Create settings with custom values
    auto settingsResult = buildTransaction.createAndAddChild ("Settings");
    ASSERT_TRUE (settingsResult.wasOk());

    DataTree settings = settingsResult.getValue();
    auto settingsTx = settings.beginValidatedTransaction (schema);
    settingsTx.setProperty ("name", "Application Settings");
    settingsTx.setProperty ("theme", "dark");
    settingsTx.setProperty ("fontSize", 14);
    settingsTx.commit();

    // Create user data
    auto userResult = buildTransaction.createAndAddChild ("UserData");
    ASSERT_TRUE (userResult.wasOk());

    DataTree userData = userResult.getValue();
    auto userTx = userData.beginValidatedTransaction (schema);
    userTx.setProperty ("username", "john_doe");
    userTx.setProperty ("age", 30);
    userTx.commit();

    buildTransaction.commit();

    // 3. Validate complete application structure
    auto validationResult = schema->validate (appTree);
    EXPECT_TRUE (validationResult.wasOk()) << validationResult.getErrorMessage();

    // 4. Query and verify structure
    EXPECT_EQ (2, appTree.getNumChildren());

    auto foundSettings = appTree.getChildWithName ("Settings");
    EXPECT_TRUE (foundSettings.isValid());
    EXPECT_EQ ("Application Settings", foundSettings.getProperty ("name").toString());
    EXPECT_EQ ("dark", foundSettings.getProperty ("theme").toString());

    auto foundUser = appTree.getChildWithName ("UserData");
    EXPECT_TRUE (foundUser.isValid());
    EXPECT_EQ ("john_doe", foundUser.getProperty ("username").toString());
    EXPECT_EQ (30, static_cast<int> (foundUser.getProperty ("age")));

    // 5. Test runtime property updates with validation
    auto updateTx = foundSettings.beginValidatedTransaction (schema);
    auto themeUpdate = updateTx.setProperty ("theme", "auto");
    EXPECT_TRUE (themeUpdate.wasOk());
    updateTx.commit();

    EXPECT_EQ ("auto", foundSettings.getProperty ("theme").toString());

    // 6. Test validation prevents invalid updates
    auto invalidTx = foundSettings.beginValidatedTransaction (schema);
    auto invalidUpdate = invalidTx.setProperty ("fontSize", 200); // Exceeds maximum
    EXPECT_TRUE (invalidUpdate.failed());
    EXPECT_TRUE (invalidUpdate.getErrorMessage().contains ("maximum"));
}

//==============================================================================
TEST (DataTreeSchemaErrorHandling, EmptySchema)
{
    // Create empty schema through invalid JSON
    auto emptySchema = DataTreeSchema::fromJsonSchemaString ("{}");
    EXPECT_EQ (nullptr, emptySchema);

    // Create default-constructed schema
    DataTreeSchema defaultSchema;
    EXPECT_FALSE (defaultSchema.isValid());
    EXPECT_FALSE (defaultSchema.hasNodeType ("Any"));
    EXPECT_TRUE (defaultSchema.getNodeTypeNames().isEmpty());

    auto invalidNode = defaultSchema.createNode ("Any");
    EXPECT_FALSE (invalidNode.isValid());
}

TEST (DataTreeSchemaErrorHandling, MalformedJSON)
{
    // Test various malformed JSON scenarios
    auto schema1 = DataTreeSchema::fromJsonSchemaString ("not json at all");
    EXPECT_EQ (nullptr, schema1);

    auto schema2 = DataTreeSchema::fromJsonSchemaString (R"({"nodeTypes": "not an object"})");
    EXPECT_EQ (nullptr, schema2);

    auto schema3 = DataTreeSchema::fromJsonSchemaString (R"({"nodeTypes": {}})");
    EXPECT_EQ (nullptr, schema3); // Empty node types
}
