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
    An improved tree-based data structure that addresses ValueTree shortcomings.

    DataTree provides:
    - Type-safe property access with validation
    - Enhanced listener system with lambda support
    - Better memory management with RAII
    - Query capabilities with predicates
    - Thread-safe immutable operations
    - Performance optimizations with copy-on-write

    @see Property, DataTreeListener
*/
class YUP_API DataTree
{
public:
    //==============================================================================
    /** Creates an invalid DataTree. */
    DataTree() noexcept;

    /** Creates a DataTree with the specified type identifier. */
    explicit DataTree (const Identifier& type);

    /** Copy constructor. */
    DataTree (const DataTree& other) noexcept;

    /** Move constructor. */
    DataTree (DataTree&& other) noexcept;

    /** Destructor. */
    ~DataTree();

    /** Copy assignment operator. */
    DataTree& operator= (const DataTree& other) noexcept;

    /** Move assignment operator. */
    DataTree& operator= (DataTree&& other) noexcept;

    //==============================================================================
    /** Returns true if this DataTree is valid (has been initialized). */
    bool isValid() const noexcept;

    /** Returns true if this DataTree is valid (has been initialized). */
    explicit operator bool() const noexcept { return isValid(); }

    /** Returns the type identifier for this DataTree. */
    Identifier getType() const noexcept;

    /** Creates a copy of this DataTree. */
    DataTree clone() const;

    //==============================================================================
    // Property Management

    /** Returns the number of properties in this DataTree. */
    int getNumProperties() const noexcept;

    /** Returns the name of the property at the given index. */
    Identifier getPropertyName (int index) const noexcept;

    /** Returns true if the specified property exists. */
    bool hasProperty (const Identifier& name) const noexcept;

    /** Returns the value of the specified property, or the default if it doesn't exist. */
    var getProperty (const Identifier& name, const var& defaultValue = {}) const;

    //==============================================================================
    // Child Management

    /** Returns the number of child DataTrees. */
    int getNumChildren() const noexcept;

    /** Returns the child at the specified index. */
    DataTree getChild (int index) const noexcept;

    /** Returns the first child with the specified type. */
    DataTree getChildWithName (const Identifier& type) const noexcept;

    /** Returns the index of the specified child, or -1 if not found. */
    int indexOf (const DataTree& child) const noexcept;

    //==============================================================================
    // Navigation

    /** Returns the parent DataTree, or an invalid DataTree if this is the root. */
    DataTree getParent() const noexcept;

    /** Returns the root of the tree that contains this DataTree. */
    DataTree getRoot() const noexcept;

    /** Returns true if this DataTree is a descendant of the specified tree. */
    bool isAChildOf (const DataTree& possibleParent) const noexcept;

    /** Returns the depth of this DataTree in the tree (root = 0). */
    int getDepth() const noexcept;

    //==============================================================================
    // Querying and Iteration

    /** Calls the specified function for each child. */
    template<typename Callback>
    void forEachChild (Callback callback) const;

    /** Calls the specified function for each descendant (depth-first). */
    template<typename Callback>
    void forEachDescendant (Callback callback) const;

    /** Returns all children matching the predicate. */
    template<typename Predicate>
    void findChildren (std::vector<DataTree>& results, Predicate predicate) const;

    /** Returns the first child matching the predicate. */
    template<typename Predicate>
    DataTree findChild (Predicate predicate) const;

    /** Returns all descendants matching the predicate. */
    template<typename Predicate>
    void findDescendants (std::vector<DataTree>& results, Predicate predicate) const;

    /** Returns the first descendant matching the predicate. */
    template<typename Predicate>
    DataTree findDescendant (Predicate predicate) const;

    //==============================================================================
    // Serialization

    /** Creates an XmlElement representing this DataTree. */
    std::unique_ptr<XmlElement> createXml() const;

    /** Recreates a DataTree from an XmlElement. */
    static DataTree fromXml (const XmlElement& xml);

    /** Writes this DataTree to a binary stream. */
    void writeToBinaryStream (OutputStream& output) const;

    /** Reads a DataTree from a binary stream. */
    static DataTree readFromBinaryStream (InputStream& input);

    //==============================================================================
    // Listeners

    /** Base class for objects that want to be notified about DataTree changes. */
    class YUP_API Listener
    {
    public:
        virtual ~Listener() = default;

        /** Called when a property has been changed. */
        virtual void propertyChanged (DataTree& tree, const Identifier& property) {}

        /** Called when a child has been added. */
        virtual void childAdded (DataTree& parent, DataTree& child) {}

        /** Called when a child has been removed. */
        virtual void childRemoved (DataTree& parent, DataTree& child, int formerIndex) {}

        /** Called when a child has been moved to a different index. */
        virtual void childMoved (DataTree& parent, DataTree& child, int oldIndex, int newIndex) {}

        /** Called when the tree structure has been replaced. */
        virtual void treeRedirected (DataTree& tree) {}
    };

    /** Adds a listener to be notified of changes to this DataTree. */
    void addListener (Listener* listener);

    /** Removes a listener. */
    void removeListener (Listener* listener);


    /** Removes all listeners. */
    void removeAllListeners();

    //==============================================================================
    // Comparison

    /** Returns true if this DataTree refers to the same internal object as the other. */
    bool operator== (const DataTree& other) const noexcept;

    /** Returns true if this DataTree refers to a different internal object than the other. */
    bool operator!= (const DataTree& other) const noexcept;

    /** Returns true if the content of this DataTree is equivalent to the other. */
    bool isEquivalentTo (const DataTree& other) const;

    //==============================================================================
    // Transaction Support

    /** RAII class for batching multiple DataTree mutations into a single undo transaction. */
    class YUP_API Transaction
    {
    public:
        /** Begins a transaction on the specified DataTree. */
        Transaction (DataTree& tree, const String& description, UndoManager* undoManager = nullptr);

        /** Move constructor. */
        Transaction (Transaction&& other) noexcept;

        /** Move assignment operator. */
        Transaction& operator= (Transaction&& other) noexcept;

        /** Destructor commits the transaction if not already committed or aborted. */
        ~Transaction();

        /** Explicitly commits the transaction. */
        void commit();

        /** Aborts the transaction, rolling back all changes. */
        void abort();

        /** Returns true if the transaction is still active. */
        bool isActive() const noexcept { return active; }

        // Mutation methods - all changes are batched until commit/destructor
        void setProperty (const Identifier& name, const var& newValue);
        void removeProperty (const Identifier& name);
        void removeAllProperties();

        void addChild (const DataTree& child, int index = -1);
        void removeChild (const DataTree& child);
        void removeChild (int index);
        void removeAllChildren();
        void moveChild (int currentIndex, int newIndex);

    private:
        friend class TransactionAction;

        // Transaction state tracking
        struct PropertyChange
        {
            enum Type { Set, Remove, RemoveAll };

            Type type;
            Identifier name;
            var newValue;
            var oldValue;
        };

        struct ChildChange;  // Forward declaration

        void captureInitialState();
        void applyChanges();
        void rollbackChanges();

        DataTree& dataTree;
        UndoManager* undoManager;
        String description;
        bool active = true;
        std::vector<PropertyChange> propertyChanges;
        std::vector<ChildChange> childChanges;
        NamedValueSet originalProperties;
        std::vector<DataTree> originalChildren;

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Transaction)
    };

    /** Begins a transaction for batching multiple mutations. */
    Transaction beginTransaction (const String& description = "DataTree Changes", UndoManager* undoManager = nullptr)
    {
        return Transaction (*this, description, undoManager);
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
    friend class TransactionAction;

    class DataObject : public std::enable_shared_from_this<DataObject>
    {
    public:
        //==============================================================================
        Identifier type;
        NamedValueSet properties;
        std::vector<DataTree> children;
        std::weak_ptr<DataObject> parent;
        ListenerList<DataTree::Listener> listeners;
        DataTree* ownerTree = nullptr;

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

        DataTree* getDataTree() const;

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
template<typename Callback>
void DataTree::forEachChild (Callback callback) const
{
    const int numChildren = getNumChildren();
    for (int i = 0; i < numChildren; ++i)
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
template<typename Callback>
void DataTree::forEachDescendant (Callback callback) const
{
    std::function<bool(const DataTree&)> traverse = [&] (const DataTree& tree) -> bool
    {
        const int numChildren = tree.getNumChildren();
        for (int i = 0; i < numChildren; ++i)
        {
            auto child = tree.getChild (i);

            if constexpr (std::is_void_v<std::invoke_result_t<Callback, DataTree>>)
            {
                callback (child);
                traverse (child);
            }
            else
            {
                if (callback (child))
                    return true;
                if (traverse (child))
                    return true;
            }
        }
        return false;
    };

    traverse (*this);
}

//==============================================================================
template<typename Predicate>
void DataTree::findChildren (std::vector<DataTree>& results, Predicate predicate) const
{
    forEachChild ([&] (const DataTree& child)
    {
        if (predicate (child))
            results.push_back (child);
    });
}

//==============================================================================
template<typename Predicate>
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
template<typename Predicate>
void DataTree::findDescendants (std::vector<DataTree>& results, Predicate predicate) const
{
    forEachDescendant ([&] (const DataTree& descendant)
    {
        if (predicate (descendant))
            results.push_back (descendant);
    });
}

//==============================================================================
template<typename Predicate>
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
