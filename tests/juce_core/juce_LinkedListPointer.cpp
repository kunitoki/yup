/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <juce_core/juce_core.h>

using namespace juce;

struct TestObject
{
    int value;
    LinkedListPointer<TestObject> nextListItem;

    TestObject (int val)
        : value (val)
    {
    }
};

TEST (LinkedListPointerTests, DefaultConstructor)
{
    LinkedListPointer<TestObject> list;
    EXPECT_EQ (list.get(), nullptr);
}

TEST (LinkedListPointerTests, ConstructorWithHeadItem)
{
    auto* obj = new TestObject (1);
    LinkedListPointer<TestObject> list (obj);
    EXPECT_EQ (list.get(), obj);
    delete obj;
}

TEST (LinkedListPointerTests, AssignmentOperator)
{
    auto* obj = new TestObject (1);
    LinkedListPointer<TestObject> list;
    list = obj;
    EXPECT_EQ (list.get(), obj);
    delete obj;
}

TEST (LinkedListPointerTests, MoveConstructor)
{
    auto* obj = new TestObject (1);
    LinkedListPointer<TestObject> list1 (obj);
    LinkedListPointer<TestObject> list2 (std::move (list1));
    EXPECT_EQ (list2.get(), obj);
    EXPECT_EQ (list1.get(), nullptr);
    delete obj;
}

TEST (LinkedListPointerTests, MoveAssignmentOperator)
{
    auto* obj = new TestObject (1);
    LinkedListPointer<TestObject> list1 (obj);
    LinkedListPointer<TestObject> list2;
    list2 = std::move (list1);
    EXPECT_EQ (list2.get(), obj);
    EXPECT_EQ (list1.get(), nullptr);
    delete obj;
}

/*
TEST (LinkedListPointerTests, GetLast)
{
    auto* obj1 = new TestObject(1);
    auto* obj2 = new TestObject(2);
    LinkedListPointer<TestObject> list(obj1);
    list.append(obj2);
    EXPECT_EQ(list.getLast().get(), obj2);
    delete obj1;
    delete obj2;
}
*/

TEST (LinkedListPointerTests, Size)
{
    LinkedListPointer<TestObject> list;
    EXPECT_EQ (list.size(), 0);

    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    list.append (obj1);
    list.append (obj2);
    EXPECT_EQ (list.size(), 2);

    delete obj1;
    delete obj2;
}

TEST (LinkedListPointerTests, Contains)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    LinkedListPointer<TestObject> list (obj1);
    list.append (obj2);
    EXPECT_TRUE (list.contains (obj1));
    EXPECT_TRUE (list.contains (obj2));
    EXPECT_FALSE (list.contains (nullptr));
    delete obj1;
    delete obj2;
}

TEST (LinkedListPointerTests, InsertNext)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    LinkedListPointer<TestObject> list;
    list.insertNext (obj1);
    list.insertNext (obj2);
    EXPECT_EQ (list.get(), obj2);
    EXPECT_EQ (list.get()->nextListItem.get(), obj1);
    delete obj1;
    delete obj2;
}

TEST (LinkedListPointerTests, InsertAtIndex)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    auto* obj3 = new TestObject (3);
    LinkedListPointer<TestObject> list;
    list.insertAtIndex (0, obj1);
    list.insertAtIndex (1, obj2);
    list.insertAtIndex (1, obj3);
    EXPECT_EQ (list.get(), obj1);
    EXPECT_EQ (list[1].get(), obj3);
    EXPECT_EQ (list[2].get(), obj2);
    delete obj1;
    delete obj2;
    delete obj3;
}

TEST (LinkedListPointerTests, ReplaceNext)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    LinkedListPointer<TestObject> list (obj1);
    auto* oldItem = list.replaceNext (obj2);
    EXPECT_EQ (list.get(), obj2);
    EXPECT_EQ (oldItem, obj1);
    delete obj1;
    delete obj2;
}

TEST (LinkedListPointerTests, RemoveNext)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    LinkedListPointer<TestObject> list (obj1);
    list.append (obj2);
    auto* removedItem = list.removeNext();
    EXPECT_EQ (removedItem, obj1);
    EXPECT_EQ (list.get(), obj2);
    delete obj1;
    delete obj2;
}

TEST (LinkedListPointerTests, Remove)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    LinkedListPointer<TestObject> list (obj1);
    list.append (obj2);
    list.remove (obj1);
    EXPECT_EQ (list.get(), obj2);
    EXPECT_EQ (list.size(), 1);
    delete obj1;
    delete obj2;
}

TEST (LinkedListPointerTests, DeleteAll)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    LinkedListPointer<TestObject> list (obj1);
    list.append (obj2);
    list.deleteAll();
    EXPECT_EQ (list.get(), nullptr);
}

TEST (LinkedListPointerTests, CopyToArray)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    LinkedListPointer<TestObject> list (obj1);
    list.append (obj2);
    TestObject* array[2];
    list.copyToArray (array);
    EXPECT_EQ (array[0], obj1);
    EXPECT_EQ (array[1], obj2);
    delete obj1;
    delete obj2;
}

TEST (LinkedListPointerTests, SwapWith)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    LinkedListPointer<TestObject> list1 (obj1);
    LinkedListPointer<TestObject> list2 (obj2);
    list1.swapWith (list2);
    EXPECT_EQ (list1.get(), obj2);
    EXPECT_EQ (list2.get(), obj1);
    delete obj1;
    delete obj2;
}

TEST (LinkedListPointerTests, Appender)
{
    LinkedListPointer<TestObject> list;
    LinkedListPointer<TestObject>::Appender appender (list);
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    appender.append (obj1);
    appender.append (obj2);
    EXPECT_EQ (list.get(), obj1);
    EXPECT_EQ (list.get()->nextListItem.get(), obj2);
    delete obj1;
    delete obj2;
}

TEST (LinkedListPointerTests, FindPointerTo)
{
    auto* obj1 = new TestObject (1);
    auto* obj2 = new TestObject (2);
    auto* obj3 = new TestObject (3);
    LinkedListPointer<TestObject> list (obj1);
    list.append (obj2);
    list.append (obj3);
    auto* pointer = list.findPointerTo (obj2);
    EXPECT_EQ (pointer->get(), obj2);
    EXPECT_EQ (pointer->get()->nextListItem.get(), obj3);
    EXPECT_EQ (list.findPointerTo (nullptr), nullptr);
    delete obj1;
    delete obj2;
    delete obj3;
}

/*
TEST (LinkedListPointerTests, AddCopyOfList)
{
    auto* obj1 = new TestObject(1);
    auto* obj2 = new TestObject(2);
    LinkedListPointer<TestObject> list1;
    list1.append(obj1);
    list1.append(obj2);

    LinkedListPointer<TestObject> list2;
    list2.addCopyOfList(list1);

    EXPECT_EQ(list2.size(), 2);
    EXPECT_NE(list2.get(), obj1);  // Ensure deep copy
    EXPECT_EQ(list2.get()->value, obj1->value);
    EXPECT_NE(list2.get()->nextListItem.get(), obj2);  // Ensure deep copy
    EXPECT_EQ(list2.get()->nextListItem.get()->value, obj2->value);

    list2.deleteAll();
    delete obj1;
    delete obj2;
}
*/
