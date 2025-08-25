# DataTree System Tutorial

This tutorial provides a comprehensive guide to using YUP's DataTree system, including DataTree, DataTreeSchema, DataTreeObjectList, and CachedValue classes. The DataTree system provides a robust, transactional, schema-validated hierarchical data structure perfect for managing application state, configuration data, and complex object relationships.

## Table of Contents

1. [DataTree Basics](#datatree-basics)
2. [Transactions and Mutations](#transactions-and-mutations)
3. [DataTreeQuery - Powerful Querying System](#datatreequery---powerful-querying-system)
4. [DataTreeSchema - Validation and Structure](#datatreeschema---validation-and-structure)
5. [CachedValue - Reactive Properties](#cachedvalue---reactive-properties)
6. [DataTreeObjectList - Managing Collections](#datatreeobjectlist---managing-collections)

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

## DataTreeQuery - Powerful Querying System

DataTreeQuery is a sophisticated querying engine designed to make extracting data from complex DataTree hierarchies both intuitive and efficient. Think of it as SQL for your hierarchical data structures, but with the flexibility of both programmatic method chaining and declarative XPath-like syntax.

The system addresses a common challenge in hierarchical data management: how to efficiently find, filter, and extract specific nodes or properties from deeply nested structures without writing verbose traversal code. DataTreeQuery solves this by providing two complementary approaches:

- **Fluent API**: Method chaining that reads like natural language and provides full IDE support with autocompletion
- **XPath-like syntax**: Familiar string-based queries for developers comfortable with XML/HTML querying

Both approaches benefit from lazy evaluation, meaning queries are built up as lightweight operation chains and only executed when you actually need the results. This provides excellent performance characteristics, especially for complex queries that might not need to examine the entire tree.

### Getting Started with DataTreeQuery

Before diving into query examples, let's establish a realistic DataTree structure that represents a typical GUI application. This will serve as our playground for exploring DataTreeQuery capabilities.

```cpp
// Sample DataTree structure for examples
DataTree appRoot("Application");
{
    auto tx = appRoot.beginTransaction("Create sample structure");
    
    // Main window
    DataTree mainWindow("Window");
    auto windowTx = mainWindow.beginTransaction("Setup window");
    windowTx.setProperty("title", "My Application");
    windowTx.setProperty("width", 800);
    windowTx.setProperty("height", 600);
    windowTx.setProperty("visible", true);
    
    // Add panels to main window
    DataTree leftPanel("Panel");
    auto leftTx = leftPanel.beginTransaction("Setup left panel");
    leftTx.setProperty("name", "LeftPanel");
    leftTx.setProperty("width", 200);
    leftTx.setProperty("docked", true);
    
    DataTree rightPanel("Panel");
    auto rightTx = rightPanel.beginTransaction("Setup right panel");
    rightTx.setProperty("name", "RightPanel");
    rightTx.setProperty("width", 150);
    rightTx.setProperty("docked", false);
    
    // Add buttons to left panel
    DataTree saveButton("Button");
    auto saveTx = saveButton.beginTransaction("Setup save button");
    saveTx.setProperty("text", "Save");
    saveTx.setProperty("enabled", true);
    saveTx.setProperty("x", 10);
    saveTx.setProperty("y", 20);
    
    DataTree loadButton("Button");
    auto loadTx = loadButton.beginTransaction("Setup load button");
    loadTx.setProperty("text", "Load");
    loadTx.setProperty("enabled", false);
    loadTx.setProperty("x", 10);
    loadTx.setProperty("y", 60);
    
    // Build hierarchy
    leftTx.addChild(saveButton);
    leftTx.addChild(loadButton);
    windowTx.addChild(leftPanel);
    windowTx.addChild(rightPanel);
    tx.addChild(mainWindow);
    
    // Add settings dialog
    DataTree settingsDialog("Dialog");
    auto dialogTx = settingsDialog.beginTransaction("Setup dialog");
    dialogTx.setProperty("title", "Settings");
    dialogTx.setProperty("modal", true);
    dialogTx.setProperty("visible", false);
    tx.addChild(settingsDialog);
}
```

This sample structure creates a realistic hierarchy with windows, panels, buttons, and dialogs - each with relevant properties like dimensions, states, and identifiers. Notice how we use transactions to build the structure safely, following DataTree best practices.

### Basic Fluent API Queries

The fluent API is DataTreeQuery's most intuitive interface, allowing you to chain method calls in a way that reads almost like English. Each method returns a DataTreeQuery object, enabling smooth composition of complex queries.

Let's start with simple queries that demonstrate the core concepts:

```cpp
// Find all buttons in the application
auto allButtons = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .nodes();

DBG("Found " << allButtons.size() << " buttons");

// Find enabled buttons only
auto enabledButtons = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .where([](const DataTree& node) {
        return node.getProperty("enabled", false);
    })
    .nodes();

// Get the first enabled button
auto firstEnabledButton = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .where([](const DataTree& node) {
        return node.getProperty("enabled", false);
    })
    .first()
    .node();

if (firstEnabledButton.isValid())
{
    DBG("First enabled button: " << firstEnabledButton.getProperty("text").toString());
}
```

These examples show the fundamental pattern: start with `DataTreeQuery::from()`, add filtering or navigation operations, and terminate with a result extraction method like `nodes()` or `node()`. The `where()` method accepts any predicate function, giving you complete flexibility in defining your filtering logic.

Notice how we check `isValid()` on individual nodes - this is important because query operations can return invalid DataTree objects when no matches are found, similar to how database queries might return null results.

### Navigation and Traversal

DataTree navigation is one of DataTreeQuery's strongest features. Unlike manual tree traversal which requires recursive functions and careful null checking, DataTreeQuery provides declarative methods that handle all the complexity internally.

The navigation methods mirror common tree traversal patterns:

```cpp
// Find all direct children of the main window
auto windowChildren = DataTreeQuery::from(appRoot)
    .descendants("Window")
    .first()
    .children()
    .nodes();

// Find all panels in the application
auto panels = DataTreeQuery::from(appRoot)
    .descendants("Panel")
    .nodes();

// Find parent window of buttons
auto buttonParents = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .parent()
    .distinct()  // Remove duplicates
    .nodes();

// Find siblings of the first button
auto firstButton = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .first()
    .node();

if (firstButton.isValid())
{
    auto siblings = DataTreeQuery::from(firstButton)
        .siblings()
        .nodes();
    DBG("Button has " << siblings.size() << " siblings");
}
```

Navigation methods can be chained freely - you might find all buttons, navigate to their parents, then find siblings of those parents. The `distinct()` method is particularly useful when navigation might produce duplicate nodes, ensuring clean result sets.

The `siblings()` method is especially handy for UI applications where you need to find related controls at the same hierarchical level, such as buttons in the same toolbar or panels in the same container.

### Property-Based Filtering

Property-based filtering is where DataTreeQuery really shines for application data. Most real-world queries aren't just about structure ("find all buttons") but about data ("find all enabled buttons with specific text").

DataTreeQuery provides several specialized methods that make property-based queries both efficient and readable:

```cpp
// Find nodes with specific properties
auto namedPanels = DataTreeQuery::from(appRoot)
    .descendants("Panel")
    .hasProperty("name")
    .nodes();

// Find panels with specific names
auto leftPanels = DataTreeQuery::from(appRoot)
    .descendants("Panel")
    .propertyEquals("name", "LeftPanel")
    .nodes();

// Find wide panels (width > 180)
auto widePanels = DataTreeQuery::from(appRoot)
    .descendants("Panel")
    .propertyWhere<int>("width", [](int width) {
        return width > 180;
    })
    .nodes();

// Find non-docked panels
auto floatingPanels = DataTreeQuery::from(appRoot)
    .descendants("Panel")
    .propertyNotEquals("docked", true)
    .nodes();
```

These property filtering methods are designed to be composable - you can chain multiple property conditions to create complex filters. The `propertyWhere<T>()` method is particularly powerful because it provides type-safe access to property values, automatically handling the conversion from `var` to your desired type.

For numeric comparisons, `propertyWhere<int>()` is often more readable than using XPath syntax, especially when the logic becomes complex or when you need to call other C++ functions within the predicate.

### Property Extraction and Transformation

Often, you don't want the DataTree nodes themselves, but rather specific properties or computed values derived from those nodes. DataTreeQuery's transformation system handles this elegantly, converting node-based queries into property-based or computed results.

The transformation system works in two stages: selection (what to extract) and conversion (how to format the results):

```cpp
// Extract button texts
auto buttonTexts = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .property("text")
    .strings();

for (const String& text : buttonTexts)
{
    DBG("Button text: " << text);
}

// Extract multiple properties from windows
auto windowProps = DataTreeQuery::from(appRoot)
    .descendants("Window")
    .properties({"title", "width", "height"})
    .properties();  // Returns std::vector<var>

// Transform nodes to custom format
auto buttonInfo = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .select([](const DataTree& button) {
        return button.getProperty("text").toString() + 
               " (" + String(button.getProperty("enabled", false) ? "enabled" : "disabled") + ")";
    })
    .strings();

for (const String& info : buttonInfo)
{
    DBG("Button info: " << info);
}
```

Property extraction is particularly useful for data binding scenarios - you can extract button labels for populating lists, configuration values for initializing components, or any other property-based data your application needs.

The `select()` transformation method is incredibly powerful, allowing you to compute derived values, format strings, or even create complex data structures from your DataTree nodes. Think of it as the "SELECT" clause in SQL, but with the full power of C++ lambda expressions.

### Ordering and Pagination

Large hierarchical structures often need sorting and pagination to be manageable. DataTreeQuery provides comprehensive ordering capabilities that work seamlessly with all other query operations.

Sorting can be based on properties, computed values, or any comparable criteria:

```cpp
// Sort buttons by their text
auto sortedButtons = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .orderByProperty("text")
    .nodes();

// Sort panels by width (custom ordering)
auto sortedPanels = DataTreeQuery::from(appRoot)
    .descendants("Panel")
    .orderBy([](const DataTree& panel) {
        return panel.getProperty("width", 0);
    })
    .nodes();

// Get first 3 nodes, skip first 2
auto paginatedResults = DataTreeQuery::from(appRoot)
    .descendants()
    .skip(2)
    .take(3)
    .nodes();

// Get specific positions
auto specificNodes = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .at({0, 2})  // First and third buttons
    .nodes();

// Get last button
auto lastButton = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .last()
    .node();
```

The ordering methods return sorted results while maintaining all DataTree relationships and properties. This is particularly useful for UI applications where you need to display hierarchical data in a specific order - sorted by name, priority, creation date, or any other criteria.

Pagination methods (`skip()`, `take()`, `at()`) are essential for performance when dealing with large data sets. Instead of retrieving thousands of nodes and processing them in your application code, you can limit the query results at the source.

### XPath-Like String Queries

For developers familiar with XPath from XML/HTML processing, DataTreeQuery offers a familiar string-based syntax that maps XPath concepts to DataTree structures. This approach is particularly valuable for:

- **Configuration-driven queries**: Store query strings in config files or databases
- **Dynamic queries**: Build query strings programmatically based on user input
- **Rapid prototyping**: Quick exploration of data structures without writing full C++ code
- **Domain-specific languages**: Building query interfaces for non-programmers

The XPath syntax in DataTreeQuery covers the most commonly used XPath features, adapted for DataTree semantics:

```cpp
// Basic descendant selection
auto buttons = DataTreeQuery::xpath(appRoot, "//Button").nodes();

// Property-based filtering
auto enabledButtons = DataTreeQuery::xpath(appRoot, "//Button[@enabled='true']").nodes();
auto namedPanels = DataTreeQuery::xpath(appRoot, "//Panel[@name]").nodes();

// Property extraction
auto buttonTexts = DataTreeQuery::xpath(appRoot, "//Button/@text").strings();
auto dialogTitles = DataTreeQuery::xpath(appRoot, "//Dialog/@title").strings();

// Position-based selection
auto firstButton = DataTreeQuery::xpath(appRoot, "//Button[1]").node();  // 1-indexed
auto lastPanel = DataTreeQuery::xpath(appRoot, "//Panel[last()]").node();
auto firstTwoButtons = DataTreeQuery::xpath(appRoot, "//Button[position() <= 2]").nodes();

// Complex conditions
auto modalDialogs = DataTreeQuery::xpath(appRoot, 
    "//Dialog[@modal='true' and @visible='false']").nodes();

// Text content access
auto buttonLabels = DataTreeQuery::xpath(appRoot, "//Button/text()").strings();

// Comparison operators
auto widePanels = DataTreeQuery::xpath(appRoot, "//Panel[@width > 180]").nodes();
auto enabledButtons = DataTreeQuery::xpath(appRoot, "//Button[@enabled != 'false']").nodes();
```

XPath queries are particularly elegant for simple, well-defined queries. The property extraction syntax (`/@propertyName`) is often more concise than the equivalent fluent API calls, especially when you're extracting the same property from many nodes.

Position-based predicates like `[1]` and `[last()]` are invaluable for UI queries where you need specific items from lists - the first button in a toolbar, the last panel in a layout, or the second dialog in a stack.

### Advanced XPath Syntax Reference

Understanding the full XPath syntax available in DataTreeQuery helps you write more sophisticated queries. Here's a comprehensive reference with explanations of when each feature is most useful:

```cpp
// Axis and path expressions
"//NodeType"           // All descendants of type NodeType
"/NodeType"           // Direct children of type NodeType  
"*"                   // Any node type
"."                   // Current node
".."                  // Parent node

// Property predicates
"[@property]"         // Nodes with property
"[@property='value']" // Property equals value
"[@property!='value']" // Property not equals value
"[@property > 100]"   // Numeric comparison (>, <, >=, <=)

// Position predicates
"[1]"                 // First child (1-indexed)
"[2]"                 // Second child
"[last()]"            // Last child
"[position() > 2]"    // Position greater than 2

// Logical operators
"[@a='x' and @b='y']" // Both conditions
"[@a='x' or @b='y']"  // Either condition
"[not(@disabled)]"    // Negation

// Functions
"text()"              // Text content
"count()"             // Count of nodes
```

This syntax reference shows how DataTreeQuery maps XPath concepts to DataTree operations. The position-based predicates are 1-indexed (following XPath convention), while the fluent API uses 0-based indexing (following C++ convention).

Logical operators (`and`, `or`, `not()`) enable complex filtering that would be verbose with multiple fluent API calls. Functions like `text()` provide semantic access to common data patterns.

### Combining Fluent API with XPath

One of DataTreeQuery's most powerful features is the ability to seamlessly mix fluent API calls with XPath strings within the same query. This hybrid approach lets you use the best tool for each part of your query:

- Use fluent API for complex programmatic logic and IDE support
- Use XPath for simple, well-defined patterns and configuration-driven queries

The combination is particularly effective for building reusable query components:

```cpp
// Start with fluent API, then use XPath
auto complexQuery = DataTreeQuery::from(appRoot)
    .descendants("Window")
    .xpath(".//Button[@enabled='true']")  // XPath on current selection
    .orderByProperty("text")
    .take(5)
    .nodes();

// Mix and match approaches
auto mixedQuery = DataTreeQuery::from(appRoot)
    .xpath("//Panel[@docked='true']")     // XPath to find docked panels
    .children()                           // Fluent API to get children
    .ofType("Button")                     // Filter to buttons only
    .where([](const DataTree& btn) {      // Custom predicate
        return btn.getProperty("text").toString().startsWith("S");
    })
    .nodes();
```

This hybrid approach is especially valuable in larger applications where different parts of the query might be maintained by different team members, or where part of the query logic needs to be configurable while other parts require programmatic flexibility.

The `.xpath()` method can be called on any DataTreeQuery object, applying the XPath expression to the current selection rather than starting from the root. This enables powerful composition patterns.

### Grouping and Aggregation

While DataTreeQuery excels at finding and filtering individual nodes, real applications often need to analyze patterns across collections of nodes. The grouping system provides SQL-like GROUP BY functionality for hierarchical data.

Grouping is particularly useful for:

```cpp
// Group buttons by their enabled state
auto buttonsByState = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .groupBy([](const DataTree& button) {
        return button.getProperty("enabled", false) ? "enabled" : "disabled";
    });

for (const auto& [state, buttons] : buttonsByState)
{
    DBG(state.toString() << ": " << buttons.size() << " buttons");
}

// Group panels by width ranges
auto panelsBySize = DataTreeQuery::from(appRoot)
    .descendants("Panel")
    .groupBy([](const DataTree& panel) {
        int width = panel.getProperty("width", 0);
        if (width < 150) return var("small");
        if (width < 250) return var("medium");
        return var("large");
    });
```

Grouping creates a map where keys represent the grouping criteria and values contain vectors of nodes that match that criteria. This is incredibly useful for categorizing UI elements, analyzing configuration patterns, or building summary reports from hierarchical data.

The grouping key can be any value that's convertible to `var` - strings, numbers, booleans, or even complex computed values. This flexibility enables sophisticated analytical queries.

### Conditional Operations

Sometimes you don't need the actual data, but rather answers to questions about the data: "Are there any disabled buttons?" or "Do all panels have names?" DataTreeQuery provides efficient conditional operations that can answer these questions without building complete result sets.

```cpp
// Check if any buttons are disabled
bool hasDisabledButtons = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .any([](const DataTree& button) {
        return !button.getProperty("enabled", true);
    });

// Check if all panels are docked
bool allPanelsDocked = DataTreeQuery::from(appRoot)
    .descendants("Panel")
    .all([](const DataTree& panel) {
        return panel.getProperty("docked", false);
    });

// Find first button with specific text
auto saveButton = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .firstWhere([](const DataTree& button) {
        return button.getProperty("text").toString() == "Save";
    });

// Count matching elements
int enabledButtonCount = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .where([](const DataTree& button) {
        return button.getProperty("enabled", false);
    })
    .count();
```

These conditional operations are optimized for early termination - `any()` stops as soon as it finds one matching element, and `all()` stops as soon as it finds one non-matching element. This makes them much more efficient than retrieving all results and checking them in application code.

The `firstWhere()` method combines filtering and selection, returning the first node that matches your criteria. This is often more efficient than filtering all nodes and then selecting the first result.

### Performance Considerations

DataTreeQuery's lazy evaluation system is designed to minimize unnecessary work, but understanding how it works helps you write more efficient queries. The key insight is that DataTreeQuery builds operation chains without executing them until you request actual results.

This lazy approach provides several performance benefits:

```cpp
// Lazy evaluation - query is built but not executed
DataTreeQuery query = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .where([](const DataTree& node) { return expensiveCheck(node); });

// Execution only happens when results are accessed
if (query.any())  // Executes query and stops at first match
{
    auto results = query.nodes();  // Re-executes query for full results
}

// Cache results for multiple accesses
auto result = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .execute();  // Explicit execution

// Multiple accesses to same result are efficient
auto nodes = result.nodes();
auto count = result.size();
bool hasResults = !result.empty();

// Use early termination for existence checks
bool hasEnabledButtons = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .where([](const DataTree& btn) { return btn.getProperty("enabled", false); })
    .any();  // Stops at first match
```

The most important performance principle is to use the right result extraction method for your needs. If you only need to know whether results exist, use `any()` rather than `nodes().empty()`. If you only need the count, use `count()` rather than `nodes().size()`.

For complex queries that will be used multiple times, consider caching the `QueryResult` object rather than re-executing the entire query. The `execute()` method gives you explicit control over when evaluation happens.

### Error Handling and Validation

Robust applications need to handle edge cases gracefully. DataTreeQuery is designed to be forgiving - it returns empty results rather than throwing exceptions for most error conditions, making it safe to use in production code without extensive error handling.

However, there are still important validation patterns to follow:

```cpp
// XPath syntax errors are handled gracefully
auto result = DataTreeQuery::xpath(appRoot, "//Invalid[Syntax");
if (result.empty())
{
    DBG("Query returned no results (possibly due to syntax error)");
}

// Check for valid results
auto buttons = DataTreeQuery::from(appRoot).descendants("Button").nodes();
if (buttons.empty())
{
    DBG("No buttons found");
}
else
{
    DBG("Found " << buttons.size() << " buttons");
}

// Safe property access
auto buttonTexts = DataTreeQuery::from(appRoot)
    .descendants("Button")
    .where([](const DataTree& btn) {
        return btn.hasProperty("text");  // Ensure property exists
    })
    .property("text")
    .strings();
```

The key to robust DataTreeQuery usage is defensive programming - always check for empty results before using them, validate that properties exist before accessing them, and use appropriate default values when data might be missing.

XPath syntax errors (malformed expressions) are handled gracefully by returning empty results, but it's still good practice to validate complex XPath strings, especially if they come from external sources.

### Summary

DataTreeQuery transforms hierarchical data access from a tedious, error-prone manual process into an expressive, efficient querying system. By providing both programmatic and declarative interfaces, it accommodates different development styles and use cases while maintaining consistent performance characteristics.

The combination of lazy evaluation, comprehensive filtering options, and seamless API integration makes DataTreeQuery an essential tool for any application working with complex hierarchical data. Whether you're building UI frameworks, configuration systems, or data processing pipelines, DataTreeQuery provides the abstraction layer that makes hierarchical data feel as natural to work with as relational databases.

Key advantages of adopting DataTreeQuery in your applications:

- **Reduced boilerplate**: Eliminate manual tree traversal code
- **Improved readability**: Queries read like natural language descriptions of what you want
- **Better performance**: Lazy evaluation and early termination optimize execution
- **Fewer bugs**: Declarative queries are less prone to off-by-one errors and null pointer exceptions
- **Enhanced maintainability**: Changes to data structure require minimal query updates
- **Flexible approaches**: Choose between fluent API and XPath based on the situation

As your DataTree structures grow in complexity, DataTreeQuery grows with them, providing the tools you need to efficiently access and manipulate hierarchical data at any scale.

## DataTreeSchema - Validation and Structure

DataTreeSchema provides JSON Schema-based validation for DataTree structures, ensuring data integrity and enabling smart defaults.

### Creating Schemas

Now that we've explored the powerful querying capabilities of DataTreeQuery, let's examine how DataTreeSchema brings structure and validation to our hierarchical data.

```cpp
// DataTreeSchema uses JSON Schema syntax to define the structure,
// validation rules, and default values for DataTree hierarchies.
// This approach provides a familiar, standardized way to describe
// data constraints that can be shared across languages and tools.

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