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
    An abstract base class for managing a synchronized list of objects that mirror DataTree children.

    DataTreeObjectList maintains a collection of C++ objects that correspond to child DataTree nodes.
    It automatically creates objects when suitable children are added to the parent DataTree, removes
    objects when children are deleted, and keeps the object list in sync with the tree structure.

    This is useful for creating object models that mirror DataTree hierarchies, such as:
    - UI components that represent data tree nodes
    - Audio processors that map to configuration data
    - Any scenario where you need C++ objects to stay synchronized with a DataTree structure

    Key features:
    - Automatic object creation/destruction based on DataTree changes
    - Thread-safe operations when used with appropriate CriticalSectionType
    - Maintains object order matching the DataTree child order
    - Selective object creation based on DataTree content
    - Callback notifications for object lifecycle events

    Usage pattern:
    @code
    class MyObjectList : public DataTreeObjectList<MyObject>
    {
    public:
        MyObjectList(const DataTree& parent) : DataTreeObjectList<MyObject>(parent)
        {
            rebuildObjects(); // Initialize from existing children
        }

        ~MyObjectList()
        {
            freeObjects(); // Clean up in destructor
        }

        bool isSuitableType(const DataTree& tree) const override
        {
            return tree.hasProperty("myProperty");
        }

        MyObject* createNewObject(const DataTree& tree) override
        {
            return new MyObject(tree);
        }

        void deleteObject(MyObject* obj) override
        {
            delete obj;
        }

        // Optional notification callbacks
        void newObjectAdded(MyObject* object) override {}
        void objectRemoved(MyObject* object) override {}
        void objectOrderChanged() override {}
    };
    @endcode

    @tparam ObjectType The type of objects to manage. Must have a getDataTree() method
                       that returns the DataTree it represents.
    @tparam CriticalSectionType The synchronization mechanism. Use DummyCriticalSection
                                for single-threaded access or CriticalSection for thread safety.

    @see DataTree, DataTree::Listener
*/
template <class ObjectType, class CriticalSectionType = DummyCriticalSection>
class DataTreeObjectList : public DataTree::Listener
{
public:
    //==============================================================================
    /** Creates a DataTreeObjectList that monitors the specified parent DataTree.

        The constructor registers this object as a listener on the parent DataTree to receive notifications about child
        additions, removals, and reordering.

        @param parentTree The DataTree whose children will be monitored and mirrored as objects in this list.

        @note After construction, you must call rebuildObjects() to initialize the object list from any existing children in the parent DataTree.
    */
    DataTreeObjectList (const DataTree& parentTree)
        : parent (parentTree)
    {
        parent.addListener (this);
    }

    /** Destructor.

        @warning The destructor asserts that all objects have been freed. You must call freeObjects() in your
                 subclass destructor before this base destructor is called, otherwise you'll get an assertion failure.

        This design ensures proper cleanup order and prevents memory leaks.
    */
    ~DataTreeObjectList()
    {
        jassert (objects.size() == 0); // must call freeObjects() in the subclass destructor!
    }

    //==============================================================================
    /** Initializes the object list from existing children in the parent DataTree.

        This method scans all current children of the parent DataTree, creates objects for those that pass the
        isSuitableType() test, and adds them to the objects array.

        @warning This method must be called exactly once, typically in your subclass constructor, and only when
                 the objects array is empty.

        @note Objects are created in the same order as they appear in the parent DataTree.

        Example usage:
        @code
        MyObjectList(const DataTree& parent) : DataTreeObjectList<MyObject>(parent)
        {
            rebuildObjects(); // Initialize from existing children
        }
        @endcode
    */
    void rebuildObjects()
    {
        jassert (objects.size() == 0); // must only call this method once at construction

        for (int i = 0; i < parent.getNumChildren(); ++i)
        {
            if (const auto& v = parent.getChild (i); isSuitableType (v))
            {
                if (ObjectType* newObject = createNewObject (v))
                    objects.add (newObject);
            }
        }
    }

    /** Cleans up all objects and unregisters from the parent DataTree.

        This method removes the listener from the parent DataTree and deletes all managed objects. It should be called in
        your subclass destructor to ensure proper cleanup order.

        @warning This method must be called in your subclass destructor before the base class destructor is called.

        Example usage:
        @code
        ~MyObjectList()
        {
            freeObjects(); // Clean up before base destructor
        }
        @endcode
    */
    void freeObjects()
    {
        parent.removeListener (this);

        deleteAllObjects();
    }

    //==============================================================================
    /** Determines whether a DataTree child should have a corresponding object.

        This method is called whenever a DataTree child is encountered to determine if an object should be created
        for it. You can use this to filter which children are represented as objects based on their type, properties, or other criteria.

        @param tree The DataTree child to evaluate

        @return true if an object should be created for this DataTree child, false if it should be ignored

        Example implementations:
        @code
        // Create objects only for children with a specific property
        bool isSuitableType(const DataTree& tree) const override
        {
            return tree.hasProperty("name");
        }

        // Create objects only for children of a specific type
        bool isSuitableType(const DataTree& tree) const override
        {
            return tree.getType() == "MyObjectType";
        }
        @endcode
    */
    virtual bool isSuitableType (const DataTree&) const = 0;

    /** Creates a new object to represent the given DataTree.

        This method is called when a DataTree child passes the isSuitableType() test and needs an object to represent it. You
        should create and return a new object that corresponds to the given DataTree.

        @param tree The DataTree for which to create an object

        @return A pointer to the newly created object, or nullptr if creation failed

        @warning The returned object must have a getDataTree() method that returns  the DataTree it represents.

        Example implementation:
        @code
        ObjectType* createNewObject(const DataTree& tree) override
        {
            return new ObjectType(tree);
        }
        @endcode
    */
    virtual ObjectType* createNewObject (const DataTree&) = 0;

    /** Deletes an object that is no longer needed.

        This method is called when an object needs to be removed from the list, typically because its corresponding DataTree
        child has been removed. You are responsible for properly disposing of the object.

        @param object The object to delete

        Example implementation:
        @code
        void deleteObject(ObjectType* object) override
        {
            delete object;
        }
        @endcode
    */
    virtual void deleteObject (ObjectType*) = 0;

    //==============================================================================
    /**

        @note When using thread-safe operation (CriticalSectionType != DummyCriticalSection),
              you should lock arrayLock before accessing this array directly.
    */
    int getNumObjects() const
    {
        return objects.size();
    }

    /**

        @note When using thread-safe operation (CriticalSectionType != DummyCriticalSection),
              you should lock arrayLock before accessing this array directly.
    */
    ObjectType* getObject (int index)
    {
        jassert (isPositiveAndBelow (index, objects.size()));

        return objects.getUnchecked (index);
    }

    /**
        @note When using thread-safe operation (CriticalSectionType != DummyCriticalSection),
              you should lock arrayLock before accessing this array directly.
    */
    const ObjectType* getObject (int index) const
    {
        jassert (isPositiveAndBelow (index, objects.size()));

        return objects.getUnchecked (index);
    }

    //==============================================================================
    /** Called when a new object has been added to the list.

        This notification is sent after an object has been successfully created and added to the objects array. You can
        use this to perform additional setup or notify other parts of your application.

        @param object The object that was added
    */
    virtual void newObjectAdded (ObjectType*) {}

    /** Called when an object has been removed from the list.

        This notification is sent after an object has been removed from the objects array but before it is deleted. You can
        use this to perform cleanup or notify other parts of your application.

        @param object The object that was removed (will be deleted after this call)
    */
    virtual void objectRemoved (ObjectType*) {}

    /** Called when the order of objects in the list has changed.

        This notification is sent when the parent DataTree's children have been reordered and the objects array has been
        re-sorted to match. The objects array will already contain the objects in their new order when this is called.
    */
    virtual void objectOrderChanged() {}

    //==============================================================================
    /** The critical section used for thread-safe access to the objects array.

        When CriticalSectionType is not DummyCriticalSection, this lock protects the objects array from concurrent
        access. Use ScopedLockType to lock it:

        @code
        {
            const DataTreeObjectList<MyObject>::ScopedLockType lock (objectList.arrayLock);
            // Safe to access objects array here
            for (int index = 0; index < objectList.getNumObjects(); ++index)
                objectList.getObject (index)->doSomething();
        }
        @endcode
    */
    CriticalSectionType arrayLock;

    /** Type alias for scoped locking of the arrayLock. */
    using ScopedLockType = typename CriticalSectionType::ScopedLockType;

    //==============================================================================
    /** @internal Comparison function used for sorting objects to match DataTree order. */
    int compareElements (ObjectType* first, ObjectType* second) const
    {
        int index1 = parent.indexOf (first->getDataTree());
        int index2 = parent.indexOf (second->getDataTree());
        return index1 - index2;
    }

protected:
    //==============================================================================
    void childAdded (DataTree&, DataTree& tree) override
    {
        if (! isChildTree (tree))
            return;

        const int index = parent.indexOf (tree);
        jassert (index >= 0);

        if (ObjectType* newObject = createNewObject (tree))
        {
            {
                const ScopedLockType sl (arrayLock);

                if (index == parent.getNumChildren() - 1)
                    objects.add (newObject);
                else
                    objects.addSorted (*this, newObject);
            }

            newObjectAdded (newObject);
        }
        else
        {
            jassertfalse;
        }
    }

    void childRemoved (DataTree& exParent, DataTree& tree, int) override
    {
        if (parent != exParent || ! isSuitableType (tree))
            return;

        const int oldIndex = indexOf (tree);
        if (oldIndex < 0)
            return;

        ObjectType* o;

        {
            const ScopedLockType sl (arrayLock);
            o = objects.removeAndReturn (oldIndex);
        }

        objectRemoved (o);
        deleteObject (o);
    }

    void childMoved (DataTree& tree, DataTree&, int, int) override
    {
        if (tree != parent)
            return;

        {
            const ScopedLockType sl (arrayLock);
            sortArray();
        }

        objectOrderChanged();
    }

    void propertyChanged (DataTree&, const Identifier&) override
    {
    }

    void treeRedirected (DataTree&) override
    {
        jassertfalse; // may need to add handling if this is hit
    }

    //==============================================================================
    void deleteAllObjects()
    {
        const ScopedLockType sl (arrayLock);

        while (objects.size() > 0)
            deleteObject (objects.removeAndReturn (objects.size() - 1));
    }

    bool isChildTree (DataTree& v) const
    {
        return isSuitableType (v) && v.getParent() == parent;
    }

    int indexOf (const DataTree& v) const noexcept
    {
        for (int i = 0; i < objects.size(); ++i)
        {
            if (objects.getUnchecked (i)->getDataTree() == v)
                return i;
        }

        return -1;
    }

    void sortArray()
    {
        objects.sort (*this);
    }

    DataTree parent;
    Array<ObjectType*> objects;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataTreeObjectList)
};

} // namespace yup
