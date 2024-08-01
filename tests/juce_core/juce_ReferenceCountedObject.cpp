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

class TestClass : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<TestClass>;

    void doSomething() const {}
};

class SingleThreadedTestClass : public SingleThreadedReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<SingleThreadedTestClass>;

    void doSomething() const {}
};

TEST (ReferenceCountedObjectTest, IncDecReferenceCount)
{
    TestClass::Ptr obj = new TestClass();
    EXPECT_EQ (obj->getReferenceCount(), 1);

    obj->incReferenceCount();
    EXPECT_EQ (obj->getReferenceCount(), 2);

    obj->decReferenceCount();
    EXPECT_EQ (obj->getReferenceCount(), 1);

    obj = nullptr; // this should decrement the count and delete the object
}

TEST (ReferenceCountedObjectTest, DecReferenceCountWithoutDeleting)
{
    TestClass::Ptr obj = new TestClass();
    EXPECT_EQ (obj->getReferenceCount(), 1);

    EXPECT_TRUE (obj->decReferenceCountWithoutDeleting());
    EXPECT_EQ (obj->getReferenceCount(), 0);

    delete obj;
}

// SingleThreadedReferenceCountedObject tests
TEST (SingleThreadedReferenceCountedObjectTest, IncDecReferenceCount)
{
    SingleThreadedTestClass::Ptr obj = new SingleThreadedTestClass();
    EXPECT_EQ (obj->getReferenceCount(), 1);

    obj->incReferenceCount();
    EXPECT_EQ (obj->getReferenceCount(), 2);

    obj->decReferenceCount();
    EXPECT_EQ (obj->getReferenceCount(), 1);

    obj = nullptr; // this should decrement the count and delete the object
}

TEST (SingleThreadedReferenceCountedObjectTest, DecReferenceCountWithoutDeleting)
{
    SingleThreadedTestClass::Ptr obj = new SingleThreadedTestClass();
    EXPECT_EQ (obj->getReferenceCount(), 1);

    EXPECT_TRUE (obj->decReferenceCountWithoutDeleting());
    EXPECT_EQ (obj->getReferenceCount(), 0);

    delete obj;
}

// ReferenceCountedObjectPtr tests
TEST (ReferenceCountedObjectPtrTest, PointerAssignment)
{
    TestClass::Ptr obj1 = new TestClass();
    TestClass::Ptr obj2 = obj1;

    EXPECT_EQ (obj1->getReferenceCount(), 2);
    EXPECT_EQ (obj2->getReferenceCount(), 2);

    obj1 = nullptr;
    EXPECT_EQ (obj2->getReferenceCount(), 1);
    obj2 = nullptr; // this should delete the object
}

TEST (ReferenceCountedObjectPtrTest, PointerComparison)
{
    TestClass::Ptr obj1 = new TestClass();
    TestClass::Ptr obj2 = obj1;

    EXPECT_EQ (obj1, obj2);
    EXPECT_NE (obj1, nullptr);

    obj1 = nullptr;
    EXPECT_EQ (obj1, nullptr);
    EXPECT_NE (obj2, nullptr);
}

TEST (ReferenceCountedObjectPtrTest, PointerDereference)
{
    TestClass::Ptr obj = new TestClass();
    EXPECT_NO_THROW (obj->doSomething());
    EXPECT_NO_THROW (*obj);
}

TEST (ReferenceCountedObjectPtrTest, Reset)
{
    TestClass::Ptr obj = new TestClass();
    EXPECT_EQ (obj->getReferenceCount(), 1);

    obj.reset();
    EXPECT_EQ (obj, nullptr);
}

// SingleThreadedReferenceCountedObjectPtr tests
TEST (SingleThreadedReferenceCountedObjectPtrTest, PointerAssignment)
{
    SingleThreadedTestClass::Ptr obj1 = new SingleThreadedTestClass();
    SingleThreadedTestClass::Ptr obj2 = obj1;

    EXPECT_EQ (obj1->getReferenceCount(), 2);
    EXPECT_EQ (obj2->getReferenceCount(), 2);

    obj1 = nullptr;
    EXPECT_EQ (obj2->getReferenceCount(), 1);
    obj2 = nullptr; // this should delete the object
}

TEST (SingleThreadedReferenceCountedObjectPtrTest, PointerComparison)
{
    SingleThreadedTestClass::Ptr obj1 = new SingleThreadedTestClass();
    SingleThreadedTestClass::Ptr obj2 = obj1;

    EXPECT_EQ (obj1, obj2);
    EXPECT_NE (obj1, nullptr);

    obj1 = nullptr;
    EXPECT_EQ (obj1, nullptr);
    EXPECT_NE (obj2, nullptr);
}

TEST (SingleThreadedReferenceCountedObjectPtrTest, PointerDereference)
{
    SingleThreadedTestClass::Ptr obj = new SingleThreadedTestClass();
    EXPECT_NO_THROW (obj->doSomething());
    EXPECT_NO_THROW (*obj);
}

TEST (SingleThreadedReferenceCountedObjectPtrTest, Reset)
{
    SingleThreadedTestClass::Ptr obj = new SingleThreadedTestClass();
    EXPECT_EQ (obj->getReferenceCount(), 1);

    obj.reset();
    EXPECT_EQ (obj, nullptr);
}
