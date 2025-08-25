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

namespace yup
{

//==============================================================================
/**
    A powerful query system for extracting data from DataTree hierarchies using both fluent API and XPath-like syntax.

    DataTreeQuery provides efficient querying capabilities for DataTree structures, supporting both
    method chaining for programmatic queries and XPath-like string syntax for declarative queries.

    ## Key Features:
    - **Fluent API**: Method chaining for readable, composable queries
    - **XPath Syntax**: String-based queries using familiar XPath-like expressions
    - **Lazy Evaluation**: Results computed only when accessed for performance
    - **Multiple Result Types**: Support for nodes, properties, and transformed results
    - **Predicate Support**: Custom filtering with lambda expressions
    - **Performance Optimized**: Efficient traversal and caching strategies

    ## Fluent API Examples:
    @code
    // Find all enabled buttons
    auto enabledButtons = DataTreeQuery::from(root)
        .descendants()
        .where([](const DataTree& node) { return node.getType() == "Button"; })
        .where([](const DataTree& node) { return node.getProperty("enabled", false); })
        .nodes();

    // Extract button text properties
    auto buttonTexts = DataTreeQuery::from(root)
        .descendants("Button")
        .property("text")
        .strings();

    // Complex multi-level query
    auto dialogTitles = DataTreeQuery::from(mainWindow)
        .children("Panel")
        .descendants("Dialog")
        .where([](const DataTree& node) {
            return node.getProperty("modal", false) && node.hasProperty("title");
        })
        .property("title")
        .strings();
    @endcode

    ## XPath-Like String Syntax:
    @code
    // Basic node selection
    auto buttons = DataTreeQuery::from(root).xpath("//Button").nodes();

    // Property-based filtering
    auto enabled = DataTreeQuery::from(root).xpath("//Button[@enabled='true']").nodes();

    // Property extraction
    auto titles = DataTreeQuery::from(root).xpath("//Dialog/@title").strings();

    // Complex conditions
    auto modals = DataTreeQuery::from(root)
        .xpath("//Dialog[@modal='true' and @title]")
        .nodes();

    // Position-based selection
    auto firstChild = DataTreeQuery::from(parent).xpath("*[1]").node();
    auto lastButton = DataTreeQuery::from(root).xpath("//Button[last()]").node();
    @endcode

    ## XPath Syntax Reference:
    - `//NodeType`: All descendants of type NodeType
    - `/NodeType`: Direct children of type NodeType
    - `*`: Any node type
    - `[@property]`: Nodes with property
    - `[@property='value']`: Nodes where property equals value
    - `[@property!='value']`: Nodes where property does not equal value
    - `[position()]`: Position-based selection (1-indexed)
    - `[first()]` / `[last()]`: First or last matching node
    - `and`, `or`, `not()`: Logical operators
    - `text()`: Node's text content (if applicable)
    - `count()`: Count of matching nodes

    @see DataTree, DataTree::Listener
*/
class YUP_API DataTreeQuery
{
    //==============================================================================
    struct VarHasher
    {
        std::size_t operator() (const var& v) const
        {
            return std::hash<String>() (v.toString());
        }
    };

public:
    //==============================================================================
    /**
        Result container that holds query results and supports lazy evaluation.

        QueryResult provides a unified interface for accessing different types of
        query results (nodes, properties, transformed values) while supporting
        efficient lazy evaluation and iteration.
    */
    class QueryResult
    {
    public:
        /**
            Iterator for traversing query results.
        */
        class Iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = DataTree;
            using difference_type = std::ptrdiff_t;
            using pointer = const DataTree*;
            using reference = const DataTree&;

            Iterator() = default;

            Iterator (const QueryResult* result, int index)
                : result (result)
                , index (index)
            {
            }

            reference operator*() const { return result->getNode (index); }

            pointer operator->() const { return &result->getNode (index); }

            Iterator& operator++()
            {
                ++index;
                return *this;
            }

            Iterator operator++ (int)
            {
                Iterator temp = *this;
                ++index;
                return temp;
            }

            bool operator== (const Iterator& other) const
            {
                return result == other.result && index == other.index;
            }

            bool operator!= (const Iterator& other) const { return ! (*this == other); }

        private:
            const QueryResult* result = nullptr;
            int index = 0;
        };

        //==============================================================================
        /** Creates an empty result. */
        QueryResult();

        /** Creates a result from a vector of DataTree nodes. */
        explicit QueryResult (std::vector<DataTree> nodes);

        /** Creates a result from a vector of property values. */
        explicit QueryResult (std::vector<var> properties);

        /** Creates a result with a custom evaluation function. */
        explicit QueryResult (std::function<std::vector<DataTree>()> evaluator);

        //==============================================================================
        /** Returns the number of results. */
        int size() const;

        /** Returns true if there are no results. */
        bool empty() const { return size() == 0; }

        /** Returns the node at the specified index. */
        const DataTree& getNode (int index) const;

        /** Returns the property value at the specified index. */
        const var& getProperty (int index) const;

        //==============================================================================
        /** Returns all results as a vector of DataTree nodes. */
        std::vector<DataTree> nodes() const;

        /** Returns the first result node, or invalid DataTree if empty. */
        DataTree node() const;

        /** Returns all property values as a vector of vars. */
        std::vector<var> properties() const;

        /** Returns all property values converted to strings. */
        StringArray strings() const;

        /** Returns all property values converted to the specified type. */
        template <typename T>
        std::vector<T> values() const;

        //==============================================================================
        /** Iterator support for range-based for loops. */
        Iterator begin() const { return Iterator (this, 0); }

        Iterator end() const { return Iterator (this, size()); }

    private:
        void ensureEvaluated() const;

        mutable std::vector<DataTree> cachedNodes;
        mutable std::vector<var> cachedProperties;
        mutable std::function<std::vector<DataTree>()> evaluator;
        mutable bool evaluated = false;
    };

    //==============================================================================
    /** Creates an empty query. Use from() to set the root node. */
    DataTreeQuery();

    /** Copy constructor. */
    DataTreeQuery (const DataTreeQuery& other) = default;

    /** Move constructor. */
    DataTreeQuery (DataTreeQuery&& other) noexcept = default;

    /** Copy assignment. */
    DataTreeQuery& operator= (const DataTreeQuery& other) = default;

    /** Move assignment. */
    DataTreeQuery& operator= (DataTreeQuery&& other) noexcept = default;

    //==============================================================================
    /**
        Starts a new query from the specified root DataTree.
        
        This is the primary entry point for creating DataTreeQuery instances.
        The returned query can be chained with additional methods to build complex queries.
        
        @param root The DataTree node to use as the starting point for queries
        @returns A new DataTreeQuery instance rooted at the specified node
        
        @code
        auto buttons = DataTreeQuery::from(mainWindow)
            .descendants("Button")
            .where([](const DataTree& node) { return node.getProperty("enabled", false); })
            .nodes();
        @endcode
    */
    static DataTreeQuery from (const DataTree& root);

    /**
        Executes an XPath-like query string and returns results directly.
        
        This static method provides a convenient way to execute simple XPath queries
        without creating a DataTreeQuery instance. For more complex queries or when
        you need to chain operations, use from() instead.
        
        @param root The DataTree node to query against
        @param query The XPath-like query string to execute
        @returns QueryResult containing the matching nodes or properties
        
        @code
        // Find all enabled buttons
        auto enabled = DataTreeQuery::xpath(root, "//Button[@enabled='true']");
        
        // Extract dialog titles
        auto titles = DataTreeQuery::xpath(root, "//Dialog/@title").strings();
        @endcode
    */
    static QueryResult xpath (const DataTree& root, const String& query);

    //==============================================================================
    /**
        Sets or changes the root DataTree for this query.
        
        This method allows you to change the starting point of an existing query.
        All subsequent operations will be performed relative to the new root.
        
        @param newRoot The new DataTree node to use as the query root
        @returns Reference to this query for method chaining
        
        @code
        DataTreeQuery query;
        query.root(mainWindow).descendants("Button").nodes();
        @endcode
    */
    DataTreeQuery& root (const DataTree& newRoot);

    /**
        Executes an XPath-like query string on the current query result.
        
        This method applies an XPath query to the current set of nodes in the query.
        It can be used to further filter or navigate from the current results.
        
        @param query The XPath-like query string to execute
        @returns Reference to this query for method chaining
        
        @code
        auto result = DataTreeQuery::from(root)
            .children("Panel")
            .xpath(".//Button[@enabled='true']")  // Find buttons within panels
            .nodes();
        @endcode
    */
    DataTreeQuery& xpath (const String& query);

    //==============================================================================
    /**
        Selects direct children of current nodes.
        
        This method navigates to all immediate child nodes of the current selection,
        regardless of their type. It's equivalent to the XPath expression "\/\/*".

        @returns Reference to this query for method chaining
        
        @code
        // Get all direct children of the root
        auto children = DataTreeQuery::from(root).children().nodes();
        @endcode
    */
    DataTreeQuery& children();

    /**
        Selects direct children of the specified type.
        
        This method navigates to immediate child nodes that match the given type.
        It's equivalent to the XPath expression "/NodeType".
        
        @param type The node type to match (e.g., "Button", "Panel")
        @returns Reference to this query for method chaining
        
        @code
        // Get all Button children of panels
        auto buttons = DataTreeQuery::from(root)
            .descendants("Panel")
            .children("Button")
            .nodes();
        @endcode
    */
    DataTreeQuery& children (const Identifier& type);

    /**
        Selects all descendants of current nodes.
        
        This method performs a deep traversal to find all descendant nodes at any level
        below the current selection. It's equivalent to the XPath expression "//".
        
        @returns Reference to this query for method chaining
        
        @code
        // Find all descendants of a specific panel
        auto allNodes = DataTreeQuery::from(panel).descendants().nodes();
        @endcode
    */
    DataTreeQuery& descendants();

    /**
        Selects all descendants of the specified type.
        
        This method performs a deep traversal to find all descendant nodes of a specific
        type at any level below the current selection. It's equivalent to "//NodeType".
        
        @param type The node type to match during traversal
        @returns Reference to this query for method chaining
        
        @code
        // Find all buttons anywhere in the tree
        auto allButtons = DataTreeQuery::from(root).descendants("Button").nodes();
        @endcode
    */
    DataTreeQuery& descendants (const Identifier& type);

    /**
        Selects the parent of current nodes.
        
        This method navigates up one level to the parent nodes of the current selection.
        If a node has no parent (root node), it will be excluded from results.
        
        @returns Reference to this query for method chaining
        
        @code
        // Find parents of all buttons
        auto buttonParents = DataTreeQuery::from(root)
            .descendants("Button")
            .parent()
            .nodes();
        @endcode
    */
    DataTreeQuery& parent();

    /**
        Selects ancestors (all parents up to root).
        
        This method traverses up the tree hierarchy to collect all ancestor nodes
        from the current selection up to (but not including) the root.
        
        @returns Reference to this query for method chaining
        
        @code
        // Get the full hierarchy path for a specific node
        auto hierarchy = DataTreeQuery::from(deepNode).ancestors().nodes();
        @endcode
    */
    DataTreeQuery& ancestors();

    /**
        Selects siblings of current nodes.
        
        This method finds all nodes that share the same parent as the current selection.
        The current nodes themselves are excluded from the results.
        
        @returns Reference to this query for method chaining
        
        @code
        // Find sibling buttons of a selected button
        auto siblingButtons = DataTreeQuery::from(selectedButton)
            .siblings()
            .ofType("Button")
            .nodes();
        @endcode
    */
    DataTreeQuery& siblings();

    //==============================================================================
    /**
        Filters nodes using a predicate function.
        
        This method applies a custom filter function to each node in the current selection.
        Only nodes for which the predicate returns true will be included in the result.
        
        @param predicate A function that takes a const DataTree& and returns bool
        @returns Reference to this query for method chaining
        
        @code
        // Find visible and enabled buttons
        auto activeButtons = DataTreeQuery::from(root)
            .descendants("Button")
            .where([](const DataTree& node) {
                return node.getProperty("visible", false) && 
                       node.getProperty("enabled", false);
            })
            .nodes();
        @endcode
    */
    template <typename Predicate>
    DataTreeQuery& where (Predicate predicate);

    /**
        Filters nodes by type.
        
        This method keeps only nodes that match the specified type identifier.
        It's equivalent to using where() with a type check predicate.
        
        @param type The node type to match
        @returns Reference to this query for method chaining
        
        @code
        // Filter mixed results to keep only buttons
        auto buttons = DataTreeQuery::from(root)
            .descendants()
            .ofType("Button")
            .nodes();
        @endcode
    */
    DataTreeQuery& ofType (const Identifier& type);

    /**
        Filters nodes that have the specified property.
        
        This method keeps only nodes that contain the named property,
        regardless of the property's value.
        
        @param propertyName The name of the property to check for
        @returns Reference to this query for method chaining
        
        @code
        // Find all nodes with a 'tooltip' property
        auto nodesWithTooltips = DataTreeQuery::from(root)
            .descendants()
            .hasProperty("tooltip")
            .nodes();
        @endcode
    */
    DataTreeQuery& hasProperty (const Identifier& propertyName);

    /**
        Filters nodes where property equals the specified value.
        
        This method keeps only nodes where the named property exists and
        equals the provided value using var's comparison operators.
        
        @param propertyName The name of the property to check
        @param value The value to compare against
        @returns Reference to this query for method chaining
        
        @code
        // Find buttons with specific text
        auto okButtons = DataTreeQuery::from(root)
            .descendants("Button")
            .propertyEquals("text", "OK")
            .nodes();
        @endcode
    */
    DataTreeQuery& propertyEquals (const Identifier& propertyName, const var& value);

    /**
        Filters nodes where property does not equal the specified value.
        
        This method keeps only nodes where the named property either doesn't exist
        or exists but has a different value than the one specified.
        
        @param propertyName The name of the property to check
        @param value The value to compare against (nodes with different values pass)
        @returns Reference to this query for method chaining
        
        @code
        // Find buttons that are not disabled
        auto enabledButtons = DataTreeQuery::from(root)
            .descendants("Button")
            .propertyNotEquals("enabled", false)
            .nodes();
        @endcode
    */
    DataTreeQuery& propertyNotEquals (const Identifier& propertyName, const var& value);

    /**
        Filters nodes where property matches a predicate.
        
        This method applies a custom predicate to a specific property value after
        converting it to the specified type T. Only nodes where the predicate
        returns true will be included in the result.
        
        @tparam T The type to convert the property value to
        @param propertyName The name of the property to check
        @param predicate A function that takes a T and returns bool
        @returns Reference to this query for method chaining
        
        @code
        // Find panels with width greater than 200
        auto widePanels = DataTreeQuery::from(root)
            .descendants("Panel")
            .propertyWhere<int>("width", [](int w) { return w > 200; })
            .nodes();
        @endcode
    */
    template <typename T, typename Predicate>
    DataTreeQuery& propertyWhere (const Identifier& propertyName, Predicate predicate);

    //==============================================================================
    /**
        Selects a specific property from the current nodes.
        
        This method changes the query to return property values instead of nodes.
        The resulting QueryResult will contain the property values from each node
        that has the specified property.
        
        @param propertyName The name of the property to extract
        @returns Reference to this query for method chaining
        
        @code
        // Extract button text values
        auto buttonTexts = DataTreeQuery::from(root)
            .descendants("Button")
            .property("text")
            .strings();
        @endcode
    */
    DataTreeQuery& property (const Identifier& propertyName);

    /**
        Selects multiple properties from the current nodes.
        
        This method extracts multiple property values from each node, creating
        a flattened result containing all requested property values.
        
        @param propertyNames List of property names to extract
        @returns Reference to this query for method chaining
        
        @code
        // Extract both text and tooltip properties
        auto properties = DataTreeQuery::from(root)
            .descendants("Button")
            .properties({"text", "tooltip"})
            .strings();
        @endcode
    */
    DataTreeQuery& properties (const std::initializer_list<Identifier>& propertyNames);

    /**
        Transforms results using a function.
        
        This method applies a custom transformation to each node in the current selection.
        The transformer function can return any type that can be converted to var.
        
        @tparam Transformer Function type that takes const DataTree& and returns any convertible type
        @param transformer The transformation function to apply
        @returns Reference to this query for method chaining
        
        @code
        // Transform nodes to their display names
        auto displayNames = DataTreeQuery::from(root)
            .descendants()
            .select([](const DataTree& node) {
                return node.getProperty("name", "Unnamed").toString() + 
                       " (" + node.getType().toString() + ")";
            })
            .strings();
        @endcode
    */
    template <typename Transformer>
    DataTreeQuery& select (Transformer transformer);

    //==============================================================================
    /**
        Limits results to the first N items.
        
        This method keeps only the first 'count' items from the current selection,
        effectively implementing pagination or result limiting.
        
        @param count Maximum number of items to keep (must be >= 0)
        @returns Reference to this query for method chaining
        
        @code
        // Get first 5 buttons
        auto firstButtons = DataTreeQuery::from(root)
            .descendants("Button")
            .take(5)
            .nodes();
        @endcode
    */
    DataTreeQuery& take (int count);

    /**
        Skips the first N items.
        
        This method discards the first 'count' items from the current selection,
        keeping everything that follows. Useful for pagination.
        
        @param count Number of items to skip from the beginning (must be >= 0)
        @returns Reference to this query for method chaining
        
        @code
        // Skip first 10 items, useful for pagination
        auto remainingButtons = DataTreeQuery::from(root)
            .descendants("Button")
            .skip(10)
            .nodes();
        @endcode
    */
    DataTreeQuery& skip (int count);

    /**
        Selects items at specific positions (0-based).
        
        This method keeps only items at the specified zero-based indices.
        Invalid indices are silently ignored.
        
        @param positions List of zero-based positions to select
        @returns Reference to this query for method chaining
        
        @code
        // Select 1st, 3rd, and 5th buttons (0-based indexing)
        auto specificButtons = DataTreeQuery::from(root)
            .descendants("Button")
            .at({0, 2, 4})
            .nodes();
        @endcode
    */
    DataTreeQuery& at (const std::initializer_list<int>& positions);

    /**
        Selects the first item.
        
        This method keeps only the first item from the current selection.
        If the selection is empty, the result will also be empty.
        
        @returns Reference to this query for method chaining
        
        @code
        // Get the first button found
        auto firstButton = DataTreeQuery::from(root)
            .descendants("Button")
            .first()
            .node();
        @endcode
    */
    DataTreeQuery& first();

    /**
        Selects the last item.
        
        This method keeps only the last item from the current selection.
        If the selection is empty, the result will also be empty.
        
        @returns Reference to this query for method chaining
        
        @code
        // Get the last panel in the tree
        auto lastPanel = DataTreeQuery::from(root)
            .descendants("Panel")
            .last()
            .node();
        @endcode
    */
    DataTreeQuery& last();

    //==============================================================================
    /**
        Orders results by a key function.
        
        This method sorts the current selection using a custom key extraction function.
        The key function should return a value that can be compared using < operator.
        
        @tparam KeySelector Function type that takes const DataTree& and returns a comparable type
        @param keySelector Function to extract the sort key from each node
        @returns Reference to this query for method chaining
        
        @code
        // Sort buttons by their width property
        auto sortedButtons = DataTreeQuery::from(root)
            .descendants("Button")
            .orderBy([](const DataTree& node) {
                return node.getProperty("width", 0);
            })
            .nodes();
        @endcode
    */
    template <typename KeySelector>
    DataTreeQuery& orderBy (KeySelector keySelector);

    /**
        Orders results by a property value.
        
        This method sorts the current selection by comparing the values of the
        specified property. Nodes without the property are treated as having a default value.
        
        @param propertyName The property name to use for sorting
        @returns Reference to this query for method chaining
        
        @code
        // Sort panels by their 'priority' property
        auto sortedPanels = DataTreeQuery::from(root)
            .descendants("Panel")
            .orderByProperty("priority")
            .nodes();
        @endcode
    */
    DataTreeQuery& orderByProperty (const Identifier& propertyName);

    /**
        Reverses the order of results.
        
        This method reverses the current order of items in the selection.
        Can be used after sorting to get descending order, or simply to reverse any sequence.
        
        @returns Reference to this query for method chaining
        
        @code
        // Get buttons in reverse document order
        auto reversedButtons = DataTreeQuery::from(root)
            .descendants("Button")
            .reverse()
            .nodes();
        @endcode
    */
    DataTreeQuery& reverse();

    //==============================================================================
    /**
        Removes duplicate nodes from results.
        
        This method eliminates duplicate DataTree nodes from the current selection
        based on node identity (same DataTree object). The first occurrence is kept.
        
        @returns Reference to this query for method chaining
        
        @code
        // Remove duplicates that might occur from complex queries
        auto uniqueNodes = DataTreeQuery::from(root)
            .descendants()
            .where([](const DataTree& node) { return true; })
            .distinct()
            .nodes();
        @endcode
    */
    DataTreeQuery& distinct();

    /**
        Groups results by a key function.
        
        This method groups the current selection into a map where the key is determined
        by the keySelector function and the value is a vector of nodes with that key.
        This is a terminal operation that returns the grouped results immediately.
        
        @tparam KeySelector Function type that takes const DataTree& and returns a grouping key
        @param keySelector Function to extract the grouping key from each node
        @returns Map of grouped results with var keys and DataTree vectors as values
        
        @code
        // Group buttons by their type property
        auto buttonsByType = DataTreeQuery::from(root)
            .descendants("Button")
            .groupBy([](const DataTree& node) {
                return node.getProperty("buttonType", "default");
            });
        
        for (const auto& [type, buttons] : buttonsByType) {
            // Process each group...
        }
        @endcode
    */
    template <typename KeySelector>
    std::unordered_map<var, std::vector<DataTree>, VarHasher> groupBy (KeySelector keySelector) const;

    //==============================================================================
    /**
        Executes the query and returns results.
        
        This method triggers the lazy evaluation of all queued operations and returns
        a QueryResult containing the final results. The QueryResult can then be used
        to access nodes, properties, or convert to various formats.
        
        @returns QueryResult containing the query results
        
        @code
        auto query = DataTreeQuery::from(root).descendants("Button");
        QueryResult result = query.execute();
        
        // Access results through QueryResult methods
        auto nodes = result.nodes();
        int count = result.size();
        @endcode
    */
    QueryResult execute() const;

    /**
        Implicit conversion to QueryResult for convenience.
        
        This allows DataTreeQuery to be used directly where a QueryResult is expected,
        automatically executing the query when needed.
        
        @code
        // Implicit conversion allows direct assignment
        QueryResult result = DataTreeQuery::from(root).descendants("Button");
        @endcode
    */
    operator QueryResult() const { return execute(); }

    //==============================================================================
    // Convenience methods that execute the query immediately

    /**
        Returns all matching DataTree nodes.
        
        This convenience method immediately executes the query and returns all
        matching nodes as a vector. Equivalent to execute().nodes().
        
        @returns Vector containing all matching DataTree nodes
    */
    std::vector<DataTree> nodes() const { return execute().nodes(); }

    /**
        Returns the first matching DataTree node.
        
        This convenience method immediately executes the query and returns the first
        matching node, or an invalid DataTree if no matches are found.
        
        @returns First matching DataTree node, or invalid DataTree if empty
    */
    DataTree node() const { return execute().node(); }

    /**
        Returns all property values.
        
        This convenience method immediately executes the query and returns all
        property values as a vector of vars. Only valid after using property() or select().
        
        @returns Vector containing all property values as vars
    */
    std::vector<var> properties() const { return execute().properties(); }

    /**
        Returns all property values as strings.
        
        This convenience method immediately executes the query and converts all
        property values to strings using var's toString() method.
        
        @returns StringArray containing all property values as strings
    */
    StringArray strings() const { return execute().strings(); }

    /**
        Returns the number of matching results.
        
        This convenience method immediately executes the query and returns the
        total count of results (nodes or properties).
        
        @returns Number of items in the query result
    */
    int count() const { return execute().size(); }

    /**
        Returns true if any results match the query.
        
        This convenience method checks if the query produces any results without
        creating the full result set, making it efficient for existence checks.
        
        @returns True if there are any matching results, false otherwise
    */
    bool any() const { return count() > 0; }

    /**
        Checks if all nodes satisfy a condition.
        
        This method executes the query and applies the predicate to all resulting nodes.
        Returns true only if the predicate returns true for every node.
        
        @tparam Predicate Function type that takes const DataTree& and returns bool
        @param predicate Function to test each node
        @returns True if predicate returns true for all nodes, false otherwise
        
        @code
        // Check if all buttons are enabled
        bool allEnabled = DataTreeQuery::from(root)
            .descendants("Button")
            .all([](const DataTree& node) {
                return node.getProperty("enabled", false);
            });
        @endcode
    */
    template <typename Predicate>
    bool all (Predicate predicate) const;

    /**
        Finds the first node that satisfies a condition.
        
        This method executes the query and returns the first node for which the
        predicate returns true. Returns an invalid DataTree if no node matches.
        
        @tparam Predicate Function type that takes const DataTree& and returns bool
        @param predicate Function to test each node
        @returns First node matching the predicate, or invalid DataTree if none found
        
        @code
        // Find first visible button
        auto visibleButton = DataTreeQuery::from(root)
            .descendants("Button")
            .firstWhere([](const DataTree& node) {
                return node.getProperty("visible", false);
            });
        @endcode
    */
    template <typename Predicate>
    DataTree firstWhere (Predicate predicate) const;

private:
    //==============================================================================
    class XPathParser;

    //==============================================================================
    struct QueryOperation
    {
        enum Type
        {
            Root,
            Children,
            ChildrenOfType,
            Descendants,
            DescendantsOfType,
            Parent,
            Ancestors,
            Siblings,
            Where,
            OfType,
            HasProperty,
            PropertyEquals,
            PropertyNotEquals,
            PropertyWhere,
            Property,
            Properties,
            Select,
            Take,
            Skip,
            At,
            First,
            Last,
            OrderBy,
            OrderByProperty,
            Reverse,
            Distinct,
            XPath
        };

        Type type;
        var parameter1;
        var parameter2;
        std::function<bool (const DataTree&)> predicate;
        std::function<var (const DataTree&)> transformer;
        
        // For XPath predicates that need position information
        std::shared_ptr<void> xpathPredicate;

        QueryOperation (Type t)
            : type (t)
        {
        }

        QueryOperation (Type t, var p1)
            : type (t)
            , parameter1 (std::move (p1))
        {
        }

        QueryOperation (Type t, var p1, var p2)
            : type (t)
            , parameter1 (std::move (p1))
            , parameter2 (std::move (p2))
        {
        }
    };

    DataTreeQuery& addOperation (QueryOperation operation);
    std::vector<DataTree> executeOperations() const;
    static std::vector<DataTree> applyOperation (const QueryOperation& op, const std::vector<DataTree>& input, const DataTree& rootNode);

    std::vector<QueryOperation> operations;
    DataTree rootNode;

    YUP_LEAK_DETECTOR (DataTreeQuery)
};

//==============================================================================
// Template method implementations

template <typename T>
std::vector<T> DataTreeQuery::QueryResult::values() const
{
    auto props = properties();
    std::vector<T> result;
    result.reserve (props.size());

    for (const auto& prop : props)
    {
        try
        {
            result.push_back (VariantConverter<T>::fromVar (prop));
        }
        catch (...)
        {
            result.push_back (T {});
        }
    }

    return result;
}

template <typename Predicate>
DataTreeQuery& DataTreeQuery::where (Predicate predicate)
{
    QueryOperation op (QueryOperation::Where);
    op.predicate = [predicate] (const DataTree& node) -> bool
    {
        return predicate (node);
    };
    return addOperation (std::move (op));
}

template <typename T, typename Predicate>
DataTreeQuery& DataTreeQuery::propertyWhere (const Identifier& propertyName, Predicate predicate)
{
    QueryOperation op (QueryOperation::PropertyWhere, propertyName.toString());
    op.predicate = [propertyName, predicate] (const DataTree& node) -> bool
    {
        if (! node.hasProperty (propertyName))
            return false;

        try
        {
            T value = VariantConverter<T>::fromVar (node.getProperty (propertyName));
            return predicate (value);
        }
        catch (...)
        {
            return false;
        }
    };
    return addOperation (std::move (op));
}

template <typename Transformer>
DataTreeQuery& DataTreeQuery::select (Transformer transformer)
{
    QueryOperation op (QueryOperation::Select);
    op.transformer = [transformer] (const DataTree& node) -> var
    {
        return VariantConverter<decltype (transformer (node))>::toVar (transformer (node));
    };
    return addOperation (std::move (op));
}

template <typename KeySelector>
DataTreeQuery& DataTreeQuery::orderBy (KeySelector keySelector)
{
    QueryOperation op (QueryOperation::OrderBy);
    op.transformer = [keySelector] (const DataTree& node) -> var
    {
        return VariantConverter<decltype (keySelector (node))>::toVar (keySelector (node));
    };
    return addOperation (std::move (op));
}

template <typename KeySelector>
std::unordered_map<var, std::vector<DataTree>, DataTreeQuery::VarHasher> DataTreeQuery::groupBy (KeySelector keySelector) const
{
    auto results = nodes();
    std::unordered_map<var, std::vector<DataTree>, VarHasher> groups;

    for (const auto& node : results)
    {
        auto key = VariantConverter<decltype (keySelector (node))>::toVar (keySelector (node));
        groups[key].push_back (node);
    }

    return groups;
}

template <typename Predicate>
bool DataTreeQuery::all (Predicate predicate) const
{
    auto results = nodes();
    return std::all_of (results.begin(), results.end(), predicate);
}

template <typename Predicate>
DataTree DataTreeQuery::firstWhere (Predicate predicate) const
{
    auto results = nodes();
    auto it = std::find_if (results.begin(), results.end(), predicate);
    return it != results.end() ? *it : DataTree();
}

} // namespace yup
