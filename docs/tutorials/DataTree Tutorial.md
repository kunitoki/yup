# DataTree System Tutorial

This tutorial provides a comprehensive guide to using YUP's DataTree system, including DataTree, DataTreeSchema, DataTreeObjectList, and CachedValue classes. The DataTree system provides a robust, transactional, schema-validated hierarchical data structure perfect for managing application state, configuration data, and complex object relationships.

## Table of Contents

1. [DataTree Basics](#datatree-basics)
2. [Transactions and Mutations](#transactions-and-mutations)
3. [DataTreeSchema - Validation and Structure](#datatreeschema---validation-and-structure)
4. [CachedValue - Reactive Properties](#cachedvalue---reactive-properties)
5. [DataTreeObjectList - Managing Collections](#datatreeobjectlist---managing-collections)

## DataTree Basics

DataTree is a hierarchical data structure that replaces traditional ValueTree with enhanced performance, safety, and usability. Each DataTree node has a type identifier and can contain both properties (key-value pairs) and child nodes.

### Creating and Basic Usage

```cpp
#include <yup_data_model/yup_data_model.h>
using namespace yup;

// Create a DataTree with a type identifier
DataTree appSettings("AppSettings");

// Use transactions to modify the tree
{
    auto transaction = appSettings.beginTransaction("Set initial values");
    transaction.setProperty("version", "1.0.0");
    transaction.setProperty("debug", true);
    transaction.setProperty("maxConnections", 100);
    // Transaction commits automatically when it goes out of scope
}

// Read properties
String version = appSettings.getProperty("version", "unknown");
bool debugMode = appSettings.getProperty("debug", false);
int maxConn = appSettings.getProperty("maxConnections", 50);

DBG("App version: " << version);
DBG("Debug mode: " << (debugMode ? "enabled" : "disabled"));
```

### Working with Child Nodes

```cpp
// Create child nodes
DataTree serverConfig("ServerConfig");
DataTree uiConfig("UIConfig");

// Add children using transactions
{
    auto transaction = appSettings.beginTransaction("Add configuration sections");
    transaction.addChild(serverConfig);
    transaction.addChild(uiConfig);
}

// Navigate the tree
DataTree foundServer = appSettings.getChildWithName("ServerConfig");
if (foundServer.isValid())
{
    auto serverTx = foundServer.beginTransaction("Configure server");
    serverTx.setProperty("port", 8080);
    serverTx.setProperty("hostname", "localhost");
}

// Iterate over children
for (const auto& child : appSettings)
{
    DBG("Child type: " << child.getType().toString());
    DBG("Properties: " << child.getNumProperties());
}
```

### Querying and Searching

```cpp
// Find children with predicates
std::vector<DataTree> configNodes;
appSettings.findChildren(configNodes, [](const DataTree& child)
{
    return child.getType().toString().endsWith("Config");
});

// Search descendants
DataTree debugNode = appSettings.findDescendant([](const DataTree& node)
{
    return node.hasProperty("debug") && static_cast<bool>(node.getProperty("debug"));
});
```

## Transactions and Mutations

All DataTree modifications must be performed through transactions, ensuring atomicity and proper change notifications.

### Transaction Patterns

```cpp
DataTree settings("Settings");

// Basic transaction
{
    auto tx = settings.beginTransaction("Update theme");
    tx.setProperty("theme", "dark");
    tx.setProperty("fontSize", 14);
    // Auto-commits on scope exit
}

// Explicit commit/abort
auto tx = settings.beginTransaction("Conditional update");
tx.setProperty("experimental", true);

if (someCondition)
    tx.commit();
else
    tx.abort(); // Discard changes

// Transaction with undo support
UndoManager undoManager;
{
    auto tx = settings.beginTransaction("Undoable changes", &undoManager);
    tx.setProperty("language", "en");
    tx.setProperty("region", "US");
}
// Later: undoManager.undo();
```

### Child Management

```cpp
DataTree parent("Parent");
DataTree child1("Child");
DataTree child2("Child");

{
    auto tx = parent.beginTransaction("Manage children");

    // Add children
    tx.addChild(child1, 0);    // Insert at index 0
    tx.addChild(child2);       // Append at end

    // Move children
    tx.moveChild(1, 0);        // Move from index 1 to 0

    // Remove children
    tx.removeChild(child1);    // Remove specific child
    tx.removeChild(0);         // Remove by index
}
```

## DataTreeSchema - Validation and Structure

DataTreeSchema provides JSON Schema-based validation for DataTree structures, ensuring data integrity and enabling smart defaults.

### Creating Schemas

```cpp
// Define schema in JSON
String schemaJson = R"({
    "nodeTypes": {
        "AppSettings": {
            "description": "Application configuration root",
            "properties": {
                "version": {
                    "type": "string",
                    "required": true,
                    "default": "1.0.0",
                    "pattern": "^\\d+\\.\\d+\\.\\d+$"
                },
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
                "features": {
                    "type": "array",
                    "description": "Enabled feature flags"
                }
            },
            "children": {
                "allowedTypes": ["ServerConfig", "UIConfig"],
                "maxCount": 10
            }
        },
        "ServerConfig": {
            "properties": {
                "port": {
                    "type": "number",
                    "default": 8080,
                    "minimum": 1,
                    "maximum": 65535
                },
                "hostname": {
                    "type": "string",
                    "default": "localhost"
                }
            },
            "children": {
                "maxCount": 0
            }
        }
    }
})";

// Load schema
auto schema = DataTreeSchema::fromJsonSchemaString(schemaJson);
if (!schema)
{
    DBG("Failed to load schema");
    return;
}
```

### Schema-Driven Node Creation

```cpp
// Create nodes with defaults applied automatically
auto appSettings = schema->createNode("AppSettings");
// appSettings now has version="1.0.0", theme="light", fontSize=12

// Create valid child nodes
auto serverConfig = schema->createChildNode("AppSettings", "ServerConfig");
// serverConfig has port=8080, hostname="localhost"

// Query schema metadata
auto themeInfo = schema->getPropertyInfo("AppSettings", "theme");
DBG("Theme type: " << themeInfo.type);
DBG("Default theme: " << themeInfo.defaultValue.toString());
DBG("Allowed values: " << themeInfo.enumValues.size());

// Check node type capabilities
auto childConstraints = schema->getChildConstraints("AppSettings");
DBG("Max children: " << childConstraints.maxCount);
DBG("Allowed child types: " << childConstraints.allowedTypes.size());
```

### Validated Transactions

```cpp
// Schema-validated transactions prevent invalid data
auto settings = schema->createNode("AppSettings");
auto transaction = settings.beginTransaction(schema, "Update settings");

// Valid operations
auto result1 = transaction.setProperty("theme", "dark"); // Valid enum
EXPECT_TRUE(result1.wasOk());

auto result2 = transaction.setProperty("fontSize", 16); // Within range
EXPECT_TRUE(result2.wasOk());

// Invalid operations are rejected
auto result3 = transaction.setProperty("theme", "invalid"); // Bad enum
EXPECT_TRUE(result3.failed());
DBG("Error: " << result3.getErrorMessage());

auto result4 = transaction.setProperty("fontSize", 100); // Out of range
EXPECT_TRUE(result4.failed());

// Create and add valid children
auto childResult = transaction.createAndAddChild("ServerConfig");
if (childResult.wasOk())
{
    DataTree server = childResult.getValue();
    // server has all default properties set
}

// Transaction only commits if all operations were valid
```

### Complete Tree Validation

```cpp
// Validate entire tree structure
auto validationResult = schema->validate(appSettings);
if (validationResult.failed())
{
    DBG("Validation failed: " << validationResult.getErrorMessage());
    // Handle validation errors
}
else
{
    DBG("Tree structure is valid");
    // Safe to proceed with application logic
}
```

## CachedValue - Reactive Properties

CachedValue provides efficient, cached access to DataTree properties with automatic invalidation when the underlying data changes.

### Basic Usage

```cpp
class AppComponent
{
public:
    AppComponent(const DataTree& settingsTree)
        : settings(settingsTree)
        , theme(settingsTree, "theme", "light")       // Property with default
        , fontSize(settingsTree, "fontSize", 12)      // Numeric property
        , isEnabled(settingsTree, "enabled", true)    // Boolean property
    {
    }

    void updateTheme()
    {
        // Reading from CachedValue is fast (cached)
        String currentTheme = theme.get();
        DBG("Current theme: " << currentTheme);

        // Setting triggers cache invalidation and change notifications
        theme.set("dark");

        // Next read will be from cache again
        DBG("New theme: " << theme.get());
    }

    void updateFontSize(int newSize)
    {
        // CachedValue handles type conversion automatically
        fontSize.set(newSize);
    }

private:
    DataTree settings;
    CachedValue<String> theme;
    CachedValue<int> fontSize;
    CachedValue<bool> isEnabled;
};
```

### Reactive Updates

```cpp
// CachedValue automatically updates when the underlying DataTree changes
AppComponent component(settingsTree);

// External change to DataTree
{
    auto tx = settingsTree.beginTransaction("External update");
    tx.setProperty("theme", "dark");
}

// CachedValue automatically reflects the change
String newTheme = component.theme.get(); // Returns "dark"
```

### Thread-Safe Cached Values

```cpp
// AtomicCachedValue for thread-safe access
class ThreadSafeComponent
{
public:
    ThreadSafeComponent(const DataTree& tree)
        : connectionCount(tree, "connections", 0)
        , status(tree, "status", "disconnected")
    {
    }

    void incrementConnections()
    {
        // Thread-safe operations
        int current = connectionCount.get();
        connectionCount.set(current + 1);
    }

    String getStatus() const
    {
        return status.get(); // Thread-safe read
    }

private:
    AtomicCachedValue<int> connectionCount;
    AtomicCachedValue<String> status;
};
```

## DataTreeObjectList - Managing Collections

DataTreeObjectList manages collections of C++ objects backed by DataTree nodes, providing automatic synchronization and lifecycle management.

### Basic Object Management

```cpp
// Define a component class using CachedValue
class UIComponent
{
public:
    UIComponent(const DataTree& tree)
        : dataTree(tree)
        , name(tree, "name", "")
        , visible(tree, "visible", true)
        , x(tree, "x", 0.0f)
        , y(tree, "y", 0.0f)
    {
        DBG("Created component: " << getName());
    }

    ~UIComponent()
    {
        DBG("Destroyed component: " << getName());
    }

    // Getters using CachedValue
    String getName() const { return name.get(); }
    bool isVisible() const { return visible.get(); }
    float getX() const { return x.get(); }
    float getY() const { return y.get(); }

    // Setters using CachedValue
    void setName(const String& newName) { name.set(newName); }
    void setVisible(bool isVisible) { visible.set(isVisible); }
    void setPosition(float newX, float newY)
    {
        x.set(newX);
        y.set(newY);
    }

    DataTree getDataTree() const { return dataTree; }

private:
    DataTree dataTree;
    CachedValue<String> name;
    CachedValue<bool> visible;
    CachedValue<float> x, y;
};

// ObjectList implementation
class UIComponentList : public DataTreeObjectList<UIComponent>
{
public:
    UIComponentList(const DataTree& parentTree)
        : DataTreeObjectList<UIComponent>(parentTree)
    {
        rebuildObjects(); // Initialize from existing children
    }

    ~UIComponentList()
    {
        freeObjects(); // Clean up all objects
    }

protected:
    // Determine which DataTree nodes should have corresponding objects
    bool isSuitableType(const DataTree& tree) const override
    {
        return tree.getType() == "UIComponent" && tree.hasProperty("name");
    }

    // Create new object for a DataTree node
    UIComponent* createNewObject(const DataTree& tree) override
    {
        return new UIComponent(tree);
    }

    // Delete object when no longer needed
    void deleteObject(UIComponent* obj) override
    {
        delete obj;
    }

    // Optional: receive notifications
    void newObjectAdded(UIComponent* object) override
    {
        DBG("UI Component added: " << object->getName());
        // Update UI, register callbacks, etc.
    }

    void objectRemoved(UIComponent* object) override
    {
        DBG("UI Component removed: " << object->getName());
        // Clean up UI, unregister callbacks, etc.
    }

    void objectOrderChanged() override
    {
        DBG("UI Component order changed");
        // Update rendering order, etc.
    }
};
```

### Using the Object List

```cpp
// Create parent DataTree for components
DataTree uiRoot("UIRoot");
UIComponentList components(uiRoot);

// Add components via DataTree
{
    auto tx = uiRoot.beginTransaction("Add UI components");

    DataTree button("UIComponent");
    auto buttonTx = button.beginTransaction("Setup button");
    buttonTx.setProperty("name", "SubmitButton");
    buttonTx.setProperty("x", 100.0f);
    buttonTx.setProperty("y", 50.0f);

    tx.addChild(button);
}

// Objects are automatically created and managed
EXPECT_EQ(1, components.objects.size());
UIComponent* buttonObj = components.objects[0];
EXPECT_EQ("SubmitButton", buttonObj->getName());

// Modify object through DataTree - object reflects changes automatically
{
    auto tx = uiRoot.getChild(0).beginTransaction("Move button");
    tx.setProperty("x", 200.0f);
}

EXPECT_EQ(200.0f, buttonObj->getX()); // CachedValue reflects change

// Remove component via DataTree
{
    auto tx = uiRoot.beginTransaction("Remove button");
    tx.removeChild(0);
}

// Object is automatically destroyed
EXPECT_EQ(0, components.objects.size());
```

This tutorial provides a solid foundation for using the YUP DataTree system effectively. The combination of DataTree, DataTreeSchema, CachedValue, and DataTreeObjectList provides a powerful, type-safe, and efficient way to manage hierarchical data in your applications.