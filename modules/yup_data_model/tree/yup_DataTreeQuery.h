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
// Forward declaration for VarHasher
struct VarHasher
{
    std::size_t operator() (const var& v) const
    {
        return std::hash<String>() (v.toString());
    }
};

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
    /** Starts a new query from the specified root DataTree. */
    static DataTreeQuery from (const DataTree& root);

    /** Executes an XPath-like query string and returns results. */
    static QueryResult xpath (const DataTree& root, const String& query);

    //==============================================================================
    /** Sets or changes the root DataTree for this query. */
    DataTreeQuery& root (const DataTree& newRoot);

    /** Executes an XPath-like query string on the current query result. */
    DataTreeQuery& xpath (const String& query);

    //==============================================================================
    /** Selects direct children of current nodes. */
    DataTreeQuery& children();

    /** Selects direct children of the specified type. */
    DataTreeQuery& children (const Identifier& type);

    /** Selects all descendants of current nodes. */
    DataTreeQuery& descendants();

    /** Selects all descendants of the specified type. */
    DataTreeQuery& descendants (const Identifier& type);

    /** Selects the parent of current nodes. */
    DataTreeQuery& parent();

    /** Selects ancestors (all parents up to root). */
    DataTreeQuery& ancestors();

    /** Selects siblings of current nodes. */
    DataTreeQuery& siblings();

    //==============================================================================
    /** Filters nodes using a predicate function. */
    template <typename Predicate>
    DataTreeQuery& where (Predicate predicate);

    /** Filters nodes by type. */
    DataTreeQuery& ofType (const Identifier& type);

    /** Filters nodes that have the specified property. */
    DataTreeQuery& hasProperty (const Identifier& propertyName);

    /** Filters nodes where property equals the specified value. */
    DataTreeQuery& propertyEquals (const Identifier& propertyName, const var& value);

    /** Filters nodes where property does not equal the specified value. */
    DataTreeQuery& propertyNotEquals (const Identifier& propertyName, const var& value);

    /** Filters nodes where property matches a predicate. */
    template <typename T, typename Predicate>
    DataTreeQuery& propertyWhere (const Identifier& propertyName, Predicate predicate);

    //==============================================================================
    /** Selects a specific property from the current nodes. */
    DataTreeQuery& property (const Identifier& propertyName);

    /** Selects multiple properties from the current nodes. */
    DataTreeQuery& properties (const std::initializer_list<Identifier>& propertyNames);

    /** Transforms results using a function. */
    template <typename Transformer>
    DataTreeQuery& select (Transformer transformer);

    //==============================================================================
    /** Limits results to the first N items. */
    DataTreeQuery& take (int count);

    /** Skips the first N items. */
    DataTreeQuery& skip (int count);

    /** Selects items at specific positions (0-based). */
    DataTreeQuery& at (const std::initializer_list<int>& positions);

    /** Selects the first item. */
    DataTreeQuery& first();

    /** Selects the last item. */
    DataTreeQuery& last();

    //==============================================================================
    /** Orders results by a key function. */
    template <typename KeySelector>
    DataTreeQuery& orderBy (KeySelector keySelector);

    /** Orders results by a property value. */
    DataTreeQuery& orderByProperty (const Identifier& propertyName);

    /** Reverses the order of results. */
    DataTreeQuery& reverse();

    //==============================================================================
    /** Removes duplicate nodes from results. */
    DataTreeQuery& distinct();

    /** Groups results by a key function. */
    template <typename KeySelector>
    std::unordered_map<var, std::vector<DataTree>, VarHasher> groupBy (KeySelector keySelector) const;

    //==============================================================================
    /** Executes the query and returns results. */
    QueryResult execute() const;

    /** Implicit conversion to QueryResult for convenience. */
    operator QueryResult() const { return execute(); }

    //==============================================================================
    // Convenience methods that execute the query immediately

    /** Returns all matching DataTree nodes. */
    std::vector<DataTree> nodes() const { return execute().nodes(); }

    /** Returns the first matching DataTree node. */
    DataTree node() const { return execute().node(); }

    /** Returns all property values. */
    std::vector<var> properties() const { return execute().properties(); }

    /** Returns all property values as strings. */
    StringArray strings() const { return execute().strings(); }

    /** Returns the number of matching results. */
    int count() const { return execute().size(); }

    /** Returns true if any results match the query. */
    bool any() const { return count() > 0; }

    /** Checks if all nodes satisfy a condition. */
    template <typename Predicate>
    bool all (Predicate predicate) const;

    /** Finds the first node that satisfies a condition. */
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
std::unordered_map<var, std::vector<DataTree>, VarHasher> DataTreeQuery::groupBy (KeySelector keySelector) const
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
