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
// Forward declarations
class DataTreeSchema;

//==============================================================================
/**
    A hierarchical data structure for storing properties and child nodes with transactional support.

    DataTree is an enhanced tree-based data structure designed to replace ValueTree with improved
    performance, safety, and usability. Each DataTree node has a type identifier and can contain
    both properties (key-value pairs using var) and child DataTree nodes.

    ## Key Features:
    - **Transactional Operations**: All mutations must go through Transaction objects for atomicity
    - **Type Safety**: Type identifiers for nodes and var-based property storage
    - **Change Notifications**: Listener system for observing structural and property changes
    - **Query Support**: Predicate-based searching with lambda expressions
    - **Serialization**: Built-in XML and binary serialization support
    - **Memory Management**: Efficient copy-on-write semantics and RAII design
    - **Undo Support**: Integration with UndoManager for reversible operations

    ## Basic Usage:
    @code
    // Create a DataTree with a type identifier
    DataTree config("AppSettings");

    // Use transactions to modify the tree
    {
        auto transaction = config.beginTransaction("Set initial values");
        transaction.setProperty("version", "1.0");
        transaction.setProperty("debug", true);

        DataTree server("ServerConfig");
        transaction.addChild(server);
        // Transaction commits automatically when it goes out of scope
    }

    // Read properties and navigate the tree
    String version = config.getProperty("version", "unknown");
    DataTree serverConfig = config.getChildWithName("ServerConfig");
    @endcode

    ## Advanced Features:
    @code
    // Query with predicates
    std::vector<DataTree> debugNodes;
    config.findChildren(debugNodes, [](const DataTree& child) {
        return child.getProperty("debug", false);
    });

    // Listen to changes
    class MyListener : public DataTree::Listener {
        void propertyChanged(DataTree& tree, const Identifier& property) override {
            // Handle property changes
        }
    };
    MyListener listener;
    config.addListener(&listener);
    @endcode

    @note All structural modifications (adding/removing children, setting properties) must be
          performed through Transaction objects. Direct mutation methods are private.

    @see Transaction, Listener, CachedValue, AtomicCachedValue
*/
class YUP_API DataTree
{
public:
    //==============================================================================
    /**
        Creates an invalid DataTree that contains no data.

        Invalid DataTrees return false for isValid() and can be used as placeholders
        or to indicate failure conditions. They can later be assigned a valid DataTree.

        @see isValid()
    */
    DataTree() noexcept;

    /**
        Creates a new DataTree with the specified type identifier.

        The type identifier is used to distinguish different kinds of DataTree nodes
        and is often used in queries and serialization.

        @param type The type identifier for this DataTree node

        @code
        DataTree settings("UserSettings");
        DataTree connection("DatabaseConnection");
        @endcode
    */
    explicit DataTree (const Identifier& type);

    DataTree (const Identifier& type,
              const std::initializer_list<std::pair<Identifier, var>>& properties);

    DataTree (const Identifier& type,
              const std::initializer_list<DataTree>& children);

    DataTree (const Identifier& type,
              const std::initializer_list<std::pair<Identifier, var>>& properties,
              const std::initializer_list<DataTree>& children);

    /**
        Copy constructor - creates a shallow copy that shares the same internal data.

        DataTree uses copy-on-write semantics, so copying is efficient and both copies
        initially point to the same internal data. The data is only duplicated when
        one of the copies is modified through a transaction.

        @param other The DataTree to copy from
    */
    DataTree (const DataTree& other) noexcept;

    /**
        Move constructor - transfers ownership of internal data.

        @param other The DataTree to move from (will become invalid)
    */
    DataTree (DataTree&& other) noexcept;

    /**
        Destructor - automatically cleans up internal resources.

        If this DataTree has registered listeners, they will be automatically
        removed during destruction.
    */
    ~DataTree();

    /**
        Copy assignment - creates a shallow copy that shares the same internal data.

        @param other The DataTree to copy from
        @return Reference to this DataTree
        @see DataTree(const DataTree&)
    */
    DataTree& operator= (const DataTree& other) noexcept;

    /**
        Move assignment - transfers ownership of internal data.

        @param other The DataTree to move from (will become invalid)
        @return Reference to this DataTree
    */
    DataTree& operator= (DataTree&& other) noexcept;

    //==============================================================================
    /**
        Returns true if this DataTree contains valid data.

        Invalid DataTrees are created by the default constructor or when operations
        fail (such as getChild with an invalid index). Invalid DataTrees cannot be
        used for most operations.

        @return true if this DataTree is valid and can be used
        @see operator bool()
    */
    bool isValid() const noexcept;

    /**
        Boolean conversion operator - returns true if this DataTree is valid.

        This allows DataTree to be used in conditional expressions:
        @code
        DataTree child = parent.getChild(0);
        if (child) {
            // Child exists and is valid
        }
        @endcode

        @return true if this DataTree is valid
        @see isValid()
    */
    explicit operator bool() const noexcept { return isValid(); }

    /**
        Returns the type identifier that was used to create this DataTree.

        The type identifier distinguishes different kinds of nodes in the tree
        and is preserved during serialization.

        @return The type identifier, or empty Identifier for invalid DataTrees
    */
    Identifier getType() const noexcept;

    /**
        Creates a deep copy of this DataTree and all its children.

        Unlike the copy constructor which creates a shallow copy with shared data,
        clone() creates a completely independent copy. Changes to the clone will
        not affect the original.

        @return A new DataTree with the same content but independent internal data
        @see DataTree(const DataTree&)
    */
    DataTree clone() const;

    //==============================================================================
    /**
        Returns the number of properties stored in this DataTree.

        @return Number of properties, or 0 for invalid DataTrees
    */
    int getNumProperties() const noexcept;

    /**
        Returns the name of the property at the specified index.

        The order of properties is stable but not necessarily alphabetical.

        @param index Zero-based index of the property (0 to getNumProperties()-1)
        @return The property name, or empty Identifier if index is invalid
        @see getNumProperties(), hasProperty()
    */
    Identifier getPropertyName (int index) const noexcept;

    /**
        Checks if a property with the given name exists.

        @param name The property name to check for
        @return true if the property exists, false otherwise
        @see getProperty(), getNumProperties()
    */
    bool hasProperty (const Identifier& name) const noexcept;

    /**
        Returns the value of a property, or a default value if it doesn't exist.

        This is the primary way to read property values from a DataTree. If the
        property doesn't exist, the default value is returned.

        @param name The name of the property to retrieve
        @param defaultValue The value to return if the property doesn't exist
        @return The property value or the default value

        @code
        String username = tree.getProperty("username", "guest");
        int timeout = tree.getProperty("timeout", 30);
        bool enabled = tree.getProperty("enabled", true);
        @endcode

        @see hasProperty(), Transaction::setProperty()
    */
    var getProperty (const Identifier& name, const var& defaultValue = {}) const;

    //==============================================================================
    /**
        Returns the number of child DataTrees.

        @return Number of child nodes, or 0 for invalid DataTrees
        @see getChild(), Transaction::addChild()
    */
    int getNumChildren() const noexcept;

    /**
        Returns the child DataTree at the specified index.

        @param index Zero-based index of the child (0 to getNumChildren()-1)
        @return The child DataTree, or an invalid DataTree if index is out of range
        @see getNumChildren(), getChildWithName()
    */
    DataTree getChild (int index) const noexcept;

    /**
        Returns the first child with the specified type identifier.

        This searches only direct children, not descendants. If multiple children
        have the same type, returns the first one found.

        @param type The type identifier to search for
        @return The child DataTree with matching type, or invalid DataTree if not found
        @see getChild(), findChild()
    */
    DataTree getChildWithName (const Identifier& type) const noexcept;

    /**
        Returns the index of the specified child DataTree.

        @param child The child DataTree to find
        @return Zero-based index of the child, or -1 if not found as a direct child
        @see getChild(), getNumChildren()
    */
    int indexOf (const DataTree& child) const noexcept;

    //==============================================================================
    /**
        Returns the parent DataTree that contains this node.

        @return The parent DataTree, or an invalid DataTree if this is a root node
        @see getRoot(), isAChildOf()
    */
    DataTree getParent() const noexcept;

    /**
        Returns the root node of the tree that contains this DataTree.

        Traverses up the parent chain until it reaches a node with no parent.

        @return The root DataTree of this tree
        @see getParent(), getDepth()
    */
    DataTree getRoot() const noexcept;

    /**
        Checks if this DataTree is a descendant of the specified node.

        Returns true if this node is anywhere in the subtree rooted at possibleParent,
        including being a direct child or deeper descendant.

        @param possibleParent The DataTree that might be an ancestor
        @return true if this node is a descendant of possibleParent
        @see getParent(), getRoot()
    */
    bool isAChildOf (const DataTree& possibleParent) const noexcept;

    /**
        Returns the depth of this DataTree in the tree hierarchy.

        The root node has depth 0, its children have depth 1, etc.

        @return The depth level (0 for root nodes)
        @see getParent(), getRoot()
    */
    int getDepth() const noexcept;

    //==============================================================================
    /**
        Iterator class for range-based for loop support over child DataTrees.

        This provides standard C++ iterator interface for iterating over direct children
        of a DataTree, enabling natural syntax like:

        @code
        for (const auto& child : dataTree) {
            // Process each child
        }
        @endcode
    */
    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = DataTree;
        using difference_type = std::ptrdiff_t;
        using pointer = DataTree*;
        using reference = DataTree;

        Iterator() = default;

        Iterator (const DataTree* parent, int index)
            : parent (parent)
            , index (index)
        {
        }

        reference operator*() const { return parent->getChild (index); }

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
            return parent == other.parent && index == other.index;
        }

        bool operator!= (const Iterator& other) const { return ! (*this == other); }

    private:
        const DataTree* parent = nullptr;
        int index = 0;
    };

    /**
        Returns an iterator to the first child DataTree.

        @return Iterator pointing to the first child, or end() if no children
        @see end(), Iterator
    */
    Iterator begin() const noexcept { return Iterator (this, 0); }

    /**
        Returns an iterator past the last child DataTree.

        @return Iterator representing the end of children iteration
        @see begin(), Iterator
    */
    Iterator end() const noexcept { return Iterator (this, getNumChildren()); }

    //==============================================================================
    /**
        Calls a function for each direct child of this DataTree.

        The callback can return void or bool. If it returns bool and returns true,
        the iteration stops early.

        @param callback Function to call for each child: (const DataTree&) -> void or bool

        @code
        tree.forEachChild([](const DataTree& child) {
            std::cout << child.getType().toString() << std::endl;
        });

        // Early termination
        tree.forEachChild([](const DataTree& child) {
            if (child.getType() == "target")
                return true; // Stop iteration
            return false;    // Continue
        });
        @endcode

        @see forEachDescendant(), findChild()
    */
    template <typename Callback>
    void forEachChild (Callback callback) const;

    /**
        Calls a function for each descendant of this DataTree using depth-first traversal.

        This visits all nodes in the subtree rooted at this DataTree, excluding
        this DataTree itself. The callback can return void or bool for early termination.

        @param callback Function to call for each descendant: (const DataTree&) -> void or bool

        @code
        tree.forEachDescendant([](const DataTree& descendant) {
            if (descendant.hasProperty("enabled"))
                descendant.getProperty("enabled", false);
        });
        @endcode

        @see forEachChild(), findDescendant()
    */
    template <typename Callback>
    void forEachDescendant (Callback callback) const;

    //==============================================================================
    /**
        Finds all direct children matching a predicate and adds them to the results vector.

        The predicate function is called for each child and should return true for
        nodes that should be included in the results.

        @param results Vector to store the matching children (not cleared first)
        @param predicate Function to test each child: (const DataTree&) -> bool

        @code
        std::vector<DataTree> enabledChildren;
        tree.findChildren(enabledChildren, [](const DataTree& child) {
            return child.getProperty("enabled", false);
        });
        @endcode

        @see findChild(), findDescendants()
    */
    template <typename Predicate>
    void findChildren (std::vector<DataTree>& results, Predicate predicate) const;

    /**
        Returns the first direct child matching a predicate.

        @param predicate Function to test each child: (const DataTree&) -> bool
        @return The first matching child, or an invalid DataTree if none found

        @code
        DataTree config = tree.findChild ([](const DataTree& child)
        {
            return child.getType() == "Configuration";
        });
        @endcode

        @see findChildren(), getChildWithName()
    */
    template <typename Predicate>
    DataTree findChild (Predicate predicate) const;

    /**
        Finds all descendants matching a predicate and adds them to the results vector.

        Uses depth-first traversal to search the entire subtree rooted at this DataTree.

        @param results Vector to store the matching descendants (not cleared first)
        @param predicate Function to test each descendant: (const DataTree&) -> bool

        @code
        std::vector<DataTree> allSettings;
        root.findDescendants (allSettings, [](const DataTree& node)
        {
            return node.getType().toString().endsWith ("Settings");
        });
        @endcode

        @see findDescendant(), forEachDescendant()
    */
    template <typename Predicate>
    void findDescendants (std::vector<DataTree>& results, Predicate predicate) const;

    /**
        Returns the first descendant matching a predicate using depth-first search.

        @param predicate Function to test each descendant: (const DataTree&) -> bool
        @return The first matching descendant, or an invalid DataTree if none found
        @see findDescendants(), findChild()
    */
    template <typename Predicate>
    DataTree findDescendant (Predicate predicate) const;

    //==============================================================================
    /**
        Creates an XML representation of this DataTree and its entire subtree.

        The DataTree structure is serialized to XML with the type as the element name,
        properties as attributes, and children as nested elements.

        @return A unique_ptr to the root XmlElement, or nullptr if this DataTree is invalid
        @see fromXml(), writeToBinaryStream()

        @code
        DataTree settings ("AppSettings");
        // ... populate settings ...
        auto xml = settings.createXml();
        String xmlString = xml->toString();
        @endcode
    */
    std::unique_ptr<XmlElement> createXml() const;

    /**
        Recreates a DataTree from an XmlElement.

        This reverses the process of createXml(), reconstructing the DataTree
        hierarchy from the XML structure.

        @param xml The XmlElement to deserialize from
        @return A new DataTree representing the XML content, or invalid DataTree on failure
        @see createXml()
    */
    static DataTree fromXml (const XmlElement& xml);

    /**
        Writes this DataTree to a binary stream in a compact format.

        The binary format is more efficient than XML for storage and transmission,
        preserving all data including type information and the complete tree structure.

        @param output The OutputStream to write to
        @see readFromBinaryStream(), createXml()
    */
    void writeToBinaryStream (OutputStream& output) const;

    /**
        Reads a DataTree from a binary stream.

        This reverses the process of writeToBinaryStream(), reconstructing the
        DataTree from the binary representation.

        @param input The InputStream to read from
        @return A new DataTree from the binary data, or invalid DataTree on failure
        @see writeToBinaryStream()
    */
    static DataTree readFromBinaryStream (InputStream& input);

    /**
        Creates a JSON representation of this DataTree and its entire subtree.

        The DataTree structure is serialized to JSON with the following format:
        @code
        {
            "type": "NodeType",
            "properties": {
                "property1": "value1",
                "property2": 42
            },
            "children": [
                {
                    "type": "ChildType",
                    "properties": {},
                    "children": []
                }
            ]
        }
        @endcode

        @return A var containing the JSON object representation, or invalid var if this DataTree is invalid
        @see fromJson(), createXml()

        @code
        DataTree settings ("AppSettings");
        // ... populate settings ...
        var jsonData = settings.createJson();
        String jsonString = JSON::toString (jsonData);
        @endcode
    */
    var createJson() const;

    /**
        Recreates a DataTree from a JSON representation.

        This reverses the process of createJson(), reconstructing the DataTree
        hierarchy from the JSON structure. The JSON must follow the format
        produced by createJson().

        @param jsonData The JSON var object to deserialize from
        @return A new DataTree representing the JSON content, or invalid DataTree on failure
        @see createJson()

        @code
        String jsonString = "{ \"type\": \"Settings\", \"properties\": { \"version\": \"1.0\" }, \"children\": [] }";
        var jsonData;
        if (JSON::parse (jsonString, jsonData).wasOk())
        {
            DataTree tree = DataTree::fromJson (jsonData);
        }
        @endcode
    */
    static DataTree fromJson (const var& jsonData);

    //==============================================================================
    /**
        Base class for objects that want to receive notifications about DataTree changes.

        Listeners are automatically removed when the DataTree is destroyed, but should
        be explicitly removed if the listener is destroyed first to avoid dangling pointers.

        @code
        class MyListener : public DataTree::Listener
        {
        public:
            void propertyChanged (DataTree& tree, const Identifier& property) override
            {
                std::cout << "Property " << property.toString() << " changed" << std::endl;
            }

            void childAdded (DataTree& parent, DataTree& child) override
            {
                std::cout << "Child of type " << child.getType().toString() << " added" << std::endl;
            }
        };
        @endcode

        @see addListener(), removeListener()
    */
    class YUP_API Listener
    {
    public:
        virtual ~Listener() = default;

        /**
            Called after a property has been changed via a transaction.

            @param tree The DataTree that was modified
            @param property The identifier of the property that changed
        */
        virtual void propertyChanged (DataTree& tree, const Identifier& property) {}

        /**
            Called after a child has been added via a transaction.

            @param parent The DataTree that received the new child
            @param child The child DataTree that was added
        */
        virtual void childAdded (DataTree& parent, DataTree& child) {}

        /**
            Called after a child has been removed via a transaction.

            @param parent The DataTree that lost the child
            @param child The child DataTree that was removed
            @param formerIndex The index where the child used to be
        */
        virtual void childRemoved (DataTree& parent, DataTree& child, int formerIndex) {}

        /**
            Called after a child has been moved to a different index via a transaction.

            @param parent The DataTree containing the moved child
            @param child The child DataTree that was moved
            @param oldIndex The previous index of the child
            @param newIndex The new index of the child
        */
        virtual void childMoved (DataTree& parent, DataTree& child, int oldIndex, int newIndex) {}

        /**
            Called when the internal tree structure has been completely replaced.

            This is a rare event that occurs during certain internal operations.

            @param tree The DataTree whose structure was replaced
        */
        virtual void treeRedirected (DataTree& tree) {}
    };

    /**
        Adds a listener to receive notifications about changes to this DataTree.

        The listener will be called whenever this DataTree is modified through
        a transaction. The same listener can be added multiple times but will
        only be called once per event.

        @param listener Pointer to the listener object (must remain valid until removed)
        @see removeListener(), removeAllListeners()

        @warning The listener pointer must remain valid until explicitly removed
                 or until this DataTree is destroyed.
    */
    void addListener (Listener* listener);

    /**
        Removes a previously added listener.

        @param listener Pointer to the listener to remove
        @see addListener(), removeAllListeners()
    */
    void removeListener (Listener* listener);

    /**
        Removes all listeners from this DataTree.

        @see addListener(), removeListener()
    */
    void removeAllListeners();

    //==============================================================================
    /**
        Tests if this DataTree refers to the same internal object as another.

        This performs identity comparison, not content comparison. Two DataTrees
        are equal if they share the same internal data object.

        @param other The DataTree to compare with
        @return true if both DataTrees refer to the same internal object
        @see operator!=(), isEquivalentTo()
    */
    bool operator== (const DataTree& other) const noexcept;

    /**
        Tests if this DataTree refers to a different internal object than another.

        @param other The DataTree to compare with
        @return true if the DataTrees refer to different internal objects
        @see operator==(), isEquivalentTo()
    */
    bool operator!= (const DataTree& other) const noexcept;

    /**
        Tests if this DataTree has the same content as another, regardless of identity.

        This performs a deep content comparison, checking that both DataTrees have
        the same type, properties, and child structure.

        @param other The DataTree to compare content with
        @return true if both DataTrees have identical content
        @see operator==(), clone()
    */
    bool isEquivalentTo (const DataTree& other) const;

    //==============================================================================
    /**
        RAII class for batching multiple DataTree mutations into a single atomic operation.

        Transaction provides the only way to modify a DataTree's structure or properties.
        All changes within a transaction are batched together and applied atomically when
        the transaction commits (either explicitly or when it goes out of scope).

        ## Key Features:
        - **Atomic Operations**: All changes succeed or fail together
        - **Automatic Commit**: Commits on destruction unless explicitly aborted
        - **Undo Support**: Can integrate with UndoManager for reversible operations
        - **Listener Notifications**: Sends notifications after successful commit

        ## Usage Patterns:
        @code
        // Basic usage with auto-commit
        {
            auto transaction = tree.beginTransaction();
            transaction.setProperty ("version", "2.0");
            transaction.setProperty ("debug", false);
            // Commits automatically when transaction goes out of scope
        }

        // Explicit commit with error handling
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("config", configData);
        if (configData.isValid())
            transaction.commit();
        else
            transaction.abort(); // Rollback changes

        // With undo support
        UndoManager undoManager;
        {
            auto transaction = tree.beginTransaction (&undoManager);
            // ... make changes ...
        }
        // Later: undoManager.undo();
        @endcode

        @warning Do not store Transaction objects beyond their intended scope.
                 They are designed for short-lived, local modifications.

        @see beginTransaction(), UndoManager
    */
    class YUP_API Transaction
    {
    public:
        /**
            Constructs a transaction for the specified DataTree.

            This constructor is typically called indirectly via beginTransaction().

            @param tree The DataTree to operate on
            @param undoManager Optional UndoManager for undo/redo support

            @see DataTree::beginTransaction()
        */
        Transaction (DataTree& tree, UndoManager* undoManager = nullptr);

        /**
            Move constructor - transfers ownership of the transaction.

            The moved-from transaction becomes inactive.
        */
        Transaction (Transaction&& other) noexcept;

        /**
            Move assignment - transfers ownership of the transaction.

            The moved-from transaction becomes inactive.
        */
        Transaction& operator= (Transaction&& other) noexcept;

        /**
            Destructor - automatically commits the transaction if still active.

            This enables the RAII pattern where transactions commit when they
            go out of scope, unless explicitly aborted.
        */
        ~Transaction();

        /**
            Explicitly commits all batched changes to the DataTree.

            After calling commit(), the transaction becomes inactive and no
            further operations can be performed. Listeners are notified of
            all changes after the commit succeeds.

            @see abort(), isActive()
        */
        void commit();

        /**
            Aborts the transaction, discarding all batched changes.

            The DataTree remains unchanged and the transaction becomes inactive.
            No listeners are notified.

            @see commit(), isActive()
        */
        void abort();

        /**
            Checks if this transaction is still active and can accept operations.

            @return true if the transaction hasn't been committed or aborted
        */
        bool isActive() const noexcept { return active; }

        /**
            Sets a property value (batched until commit).

            @param name The property name
            @param newValue The new value to set

            @see DataTree::getProperty()
        */
        void setProperty (const Identifier& name, const var& newValue);

        /**
            Removes a property (batched until commit).

            @param name The property name to remove
        */
        void removeProperty (const Identifier& name);

        /**
            Removes all properties (batched until commit).
        */
        void removeAllProperties();

        /**
            Adds a child DataTree (batched until commit).

            @param child The child DataTree to add
            @param index Position to insert at, or -1 to append at the end
        */
        void addChild (const DataTree& child, int index = -1);

        /**
            Removes a specific child DataTree (batched until commit).

            @param child The child DataTree to remove
        */
        void removeChild (const DataTree& child);

        /**
            Removes the child at the specified index (batched until commit).

            @param index The index of the child to remove
        */
        void removeChild (int index);

        /**
            Removes all children (batched until commit).
        */
        void removeAllChildren();

        /**
            Moves a child from one index to another (batched until commit).

            @param currentIndex The current index of the child
            @param newIndex The new index for the child
        */
        void moveChild (int currentIndex, int newIndex);

    private:
        friend class TransactionAction;

        struct PropertyChange;
        struct ChildChange;

        DataTree& dataTree;
        UndoManager* undoManager;
        std::vector<PropertyChange> propertyChanges;
        std::vector<ChildChange> childChanges;
        bool active = true;

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Transaction)
    };

    //==============================================================================
    /**
        A validated transaction that enforces schema constraints during mutations.

        This transaction wrapper validates all property changes and child additions
        against a DataTreeSchema before applying them. Invalid operations are rejected
        with detailed error messages.

        @code
        DataTreeSchema schema = DataTreeSchema::fromJsonSchema (schemaJson);
        auto validatedTransaction = tree.beginTransaction (schema, "Update settings");

        // This will validate that "fontSize" accepts numbers and is within range
        auto result = validatedTransaction.setProperty ("fontSize", 14);
        if (result.failed())
            DBG("Invalid fontSize: " << result.getErrorMessage());

        // Auto-commits if all operations were valid
        @endcode
    */
    class YUP_API ValidatedTransaction
    {
    public:
        /**
            Creates a validated transaction for the specified DataTree.
        */
        ValidatedTransaction (DataTree& tree,
                              ReferenceCountedObjectPtr<DataTreeSchema> schema,
                              UndoManager* undoManager = nullptr);

        /**
            Move constructor - transfers ownership of the transaction.
        */
        ValidatedTransaction (ValidatedTransaction&& other) noexcept;

        /**
            Move assignment - transfers ownership of the transaction.
        */
        ValidatedTransaction& operator= (ValidatedTransaction&& other) noexcept;

        /**
            Destructor - commits the transaction if still active and valid.
        */
        ~ValidatedTransaction();

        /**
            Sets a property value with schema validation.

            @param name The property name
            @param newValue The new value to set

            @return Result indicating success or validation failure
        */
        yup::Result setProperty (const Identifier& name, const var& newValue);

        /**
            Removes a property with schema validation.

            Checks if the property is required and prevents removal if so.

            @param name The property name to remove

            @return Result indicating success or validation failure
        */
        yup::Result removeProperty (const Identifier& name);

        /**
            Adds a child node with schema validation.

            Validates child type, count constraints, and compatibility.

            @param child The child DataTree to add
            @param index Position to insert at, or -1 to append

            @return Result indicating success or validation failure
        */
        yup::Result addChild (const DataTree& child, int index = -1);

        /**
            Creates and adds a new child node of the specified type.

            Uses the schema to create a properly initialized child with defaults.

            @param childType The type of child to create and add
            @param index Position to insert at, or -1 to append

            @return ResultValue containing the created child, or error on failure
        */
        yup::ResultValue<DataTree> createAndAddChild (const Identifier& childType, int index = -1);

        /**
            Removes a child node with schema validation.

            Checks minimum child count constraints.

            @param child The child to remove

            @return Result indicating success or validation failure
        */
        yup::Result removeChild (const DataTree& child);

        /**
            Commits all validated changes to the DataTree.

            Only commits if all operations were valid.

            @return Result indicating success or failure of the commit
        */
        yup::Result commit();

        /**
            Aborts the transaction, discarding all batched changes.
        */
        void abort();

        /**
            Checks if this transaction is still active.
        */
        bool isActive() const;

        /**
            Gets the underlying DataTree transaction.

            Advanced users can access the raw transaction for operations that
            don't need validation, but this bypasses schema enforcement.
        */
        Transaction& getTransaction();

    private:
        std::unique_ptr<Transaction> transaction;
        ReferenceCountedObjectPtr<DataTreeSchema> schema;
        Identifier nodeType;
        bool hasValidationErrors = false;

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValidatedTransaction)
    };

    /**
        Creates a new transaction for modifying this DataTree.

        This is the primary way to make changes to a DataTree. All structural
        modifications (properties and children) must be performed within a transaction.

        @param undoManager Optional UndoManager to enable undo/redo functionality

        @return A new Transaction object that will modify this DataTree

        @code
        // Basic usage
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("version", "2.0");
        transaction.addChild (DataTree ("NewSection"));
        // Auto-commits when transaction goes out of scope

        // With undo support
        UndoManager undoManager;
        {
            auto transaction = tree.beginTransaction (&undoManager);
            // ... make changes ...
        }
        // Later: undoManager.undo();
        @endcode

        @see Transaction
    */
    Transaction beginTransaction (UndoManager* undoManager = nullptr)
    {
        return Transaction (*this, undoManager);
    }

    /**
        Creates a validated transaction for modifying this DataTree with schema enforcement.

        This overload creates a ValidatedTransaction that validates all operations against
        the provided schema before applying them to the DataTree.

        @param schema The DataTreeSchema to validate against (reference-counted)
        @param undoManager Optional UndoManager for undo/redo functionality
        @return A ValidatedTransaction that enforces schema constraints

        @code
        auto schema = DataTreeSchema::fromJsonSchema (schemaJson);
        {
            auto transaction = tree.beginValidatedTransaction (schema);
            transaction.setProperty ("theme", "dark"); // Validates against schema
            // Auto-commits when transaction goes out of scope if all validations pass
        }
        @endcode

        @see ValidatedTransaction, DataTreeSchema
    */
    ValidatedTransaction beginValidatedTransaction (ReferenceCountedObjectPtr<DataTreeSchema> schema,
                                                    UndoManager* undoManager = nullptr)
    {
        return ValidatedTransaction (*this, schema, undoManager);
    }

private:
    friend class Transaction;
    friend class PropertySetAction;
    friend class PropertyRemoveAction;
    friend class RemoveAllPropertiesAction;
    friend class AddChildAction;
    friend class RemoveChildAction;
    friend class RemoveAllChildrenAction;
    friend class MoveChildAction;
    friend class CompoundAction;

    class DataObject : public std::enable_shared_from_this<DataObject>
    {
    public:
        //==============================================================================
        Identifier type;
        NamedValueSet properties;
        std::vector<DataTree> children;
        std::weak_ptr<DataObject> parent;
        ListenerList<DataTree::Listener> listeners;

        //==============================================================================
        DataObject() = default;
        explicit DataObject (const Identifier& treeType);
        ~DataObject();

        //==============================================================================
        void sendPropertyChangeMessage (const Identifier& property);
        void sendChildAddedMessage (const DataTree& child);
        void sendChildRemovedMessage (const DataTree& child, int formerIndex);
        void sendChildMovedMessage (const DataTree& child, int oldIndex, int newIndex);

        //==============================================================================
        std::shared_ptr<DataObject> clone() const;

    private:
        YUP_DECLARE_NON_COPYABLE (DataObject)
    };

    explicit DataTree (std::shared_ptr<DataObject> objectToUse);
    void sendPropertyChangeMessage (const Identifier& property) const;
    void sendChildAddedMessage (const DataTree& child) const;
    void sendChildRemovedMessage (const DataTree& child, int formerIndex) const;
    void sendChildMovedMessage (const DataTree& child, int oldIndex, int newIndex) const;

    // Private mutation methods - only accessible through Transaction
    void setProperty (const Identifier& name, const var& newValue, UndoManager* undoManager = nullptr);
    void removeProperty (const Identifier& name, UndoManager* undoManager = nullptr);
    void removeAllProperties (UndoManager* undoManager = nullptr);
    void addChild (const DataTree& child, int index = -1, UndoManager* undoManager = nullptr);
    void removeChild (const DataTree& child, UndoManager* undoManager = nullptr);
    void removeChild (int index, UndoManager* undoManager = nullptr);
    void removeAllChildren (UndoManager* undoManager = nullptr);
    void moveChild (int currentIndex, int newIndex, UndoManager* undoManager = nullptr);

    std::shared_ptr<DataObject> object;

    YUP_LEAK_DETECTOR (DataTree)
};

//==============================================================================
template <typename Callback>
void DataTree::forEachChild (Callback callback) const
{
    for (int i = 0; i < getNumChildren(); ++i)
    {
        if constexpr (std::is_void_v<std::invoke_result_t<Callback, DataTree>>)
        {
            callback (getChild (i));
        }
        else
        {
            if (callback (getChild (i)))
                break;
        }
    }
}

//==============================================================================
template <typename Callback>
void DataTree::forEachDescendant (Callback callback) const
{
    std::function<bool (const DataTree&)> traverse = [&] (const DataTree& tree) -> bool
    {
        for (int i = 0; i < tree.getNumChildren(); ++i)
        {
            auto child = tree.getChild (i);

            if constexpr (std::is_void_v<std::invoke_result_t<Callback, DataTree>>)
            {
                callback (child);
                traverse (child);
            }
            else
            {
                if (callback (child) || traverse (child))
                    return true;
            }
        }

        return false;
    };

    traverse (*this);
}

//==============================================================================
template <typename Predicate>
void DataTree::findChildren (std::vector<DataTree>& results, Predicate predicate) const
{
    forEachChild ([&] (const DataTree& child)
    {
        if (predicate (child))
            results.push_back (child);
    });
}

//==============================================================================
template <typename Predicate>
DataTree DataTree::findChild (Predicate predicate) const
{
    DataTree result;

    forEachChild ([&] (const DataTree& child)
    {
        if (predicate (child))
        {
            result = child;
            return true; // Stop iteration
        }

        return false;
    });

    return result;
}

//==============================================================================
template <typename Predicate>
void DataTree::findDescendants (std::vector<DataTree>& results, Predicate predicate) const
{
    forEachDescendant ([&] (const DataTree& descendant)
    {
        if (predicate (descendant))
            results.push_back (descendant);
    });
}

//==============================================================================
template <typename Predicate>
DataTree DataTree::findDescendant (Predicate predicate) const
{
    DataTree result;

    forEachDescendant ([&] (const DataTree& descendant)
    {
        if (predicate (descendant))
        {
            result = descendant;
            return true; // Stop iteration
        }

        return false;
    });

    return result;
}

} // namespace yup
