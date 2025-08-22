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
template <class ObjectType, class CriticalSectionType = DummyCriticalSection>
class DataTreeObjectList : public DataTree::Listener
{
public:
    DataTreeObjectList(const DataTree& parentTree)
        : parent(parentTree)
    {
        parent.addListener(this);
    }

    ~DataTreeObjectList()
    {
        jassert(objects.size() == 0); // must call freeObjects() in the subclass destructor!
    }

    // call in the sub-class when being created
    void rebuildObjects()
    {
        jassert(objects.size() == 0); // must only call this method once at construction

        for (const auto& v : parent)
        {
            if (isSuitableType(v))
            {
                if (ObjectType* newObject = createNewObject(v))
                    objects.add(newObject);
            }
        }
    }

    // call in the sub-class when being destroyed
    void freeObjects()
    {
        parent.removeListener(this);
        deleteAllObjects();
    }

    //==============================================================================
    virtual bool isSuitableType(const DataTree&) const = 0;
    virtual ObjectType* createNewObject(const DataTree&) = 0;
    virtual void deleteObject(ObjectType*) = 0;

    virtual void newObjectAdded(ObjectType*) = 0;
    virtual void objectRemoved(ObjectType*) = 0;
    virtual void objectOrderChanged() = 0;

    //==============================================================================
    void childAdded(DataTree&, DataTree& tree) override
    {
        if (isChildTree(tree))
        {
            const int index = parent.indexOf(tree);
            jassert(index >= 0);

            if (ObjectType* newObject = createNewObject(tree))
            {
                {
                    const ScopedLockType sl(arrayLock);

                    if (index == parent.getNumChildren() - 1)
                        objects.add(newObject);
                    else
                        objects.addSorted(*this, newObject);
                }

                newObjectAdded(newObject);
            }
            else
                jassertfalse;
        }
    }

    void childRemoved(DataTree& exParent, DataTree& tree, int) override
    {
        if (parent == exParent && isSuitableType(tree))
        {
            const int oldIndex = indexOf(tree);

            if (oldIndex >= 0)
            {
                ObjectType* o;

                {
                    const ScopedLockType sl(arrayLock);
                    o = objects.removeAndReturn(oldIndex);
                }

                objectRemoved(o);
                deleteObject(o);
            }
        }
    }

    void childMoved(DataTree& tree, DataTree&, int, int) override
    {
        if (tree == parent)
        {
            {
                const ScopedLockType sl(arrayLock);
                sortArray();
            }

            objectOrderChanged();
        }
    }

    void propertyChanged(DataTree&, const Identifier&) override {}

    void treeRedirected(DataTree&) override { jassertfalse; } // may need to add handling if this is hit

    Array<ObjectType*> objects;
    CriticalSectionType arrayLock;

    using ScopedLockType = typename CriticalSectionType::ScopedLockType;

protected:
    DataTree parent;

    void deleteAllObjects()
    {
        const ScopedLockType sl(arrayLock);

        while (objects.size() > 0)
            deleteObject(objects.removeAndReturn(objects.size() - 1));
    }

    bool isChildTree(DataTree& v) const
    {
        return isSuitableType(v) && v.getParent() == parent;
    }

    int indexOf(const DataTree& v) const noexcept
    {
        for (int i = 0; i < objects.size(); ++i)
            if (objects.getUnchecked(i)->state == v)
                return i;

        return -1;
    }

    void sortArray()
    {
        objects.sort(*this);
    }

public:
    int compareElements(ObjectType* first, ObjectType* second) const
    {
        int index1 = parent.indexOf(first->state);
        int index2 = parent.indexOf(second->state);
        return index1 - index2;
    }

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DataTreeObjectList)
};

} // namespace yup
