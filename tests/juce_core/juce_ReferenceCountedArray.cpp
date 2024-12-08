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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

#include <functional>

using namespace juce;

namespace
{
struct TestBaseObj : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<TestBaseObj>;

    TestBaseObj() = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestBaseObj)
};

struct TestDerivedObj final : public TestBaseObj
{
    using Ptr = ReferenceCountedObjectPtr<TestDerivedObj>;

    TestDerivedObj() = default;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestDerivedObj)
};

struct DestructorObj final : public ReferenceCountedObject
{
    DestructorObj (std::function<void (const DestructorObj&)> tester, ReferenceCountedArray<DestructorObj>& arr)
        : tester (std::move (tester))
        , objectArray (arr)
    {
    }

    ~DestructorObj()
    {
        data = 0;
        tester (*this);
    }

    std::function<void (const DestructorObj&)> tester;
    ReferenceCountedArray<DestructorObj>& objectArray;
    int data = 374;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DestructorObj)
};
} // namespace

TEST (ReferenceCountedArrayTests, AddDerivedObjects)
{
    ReferenceCountedArray<TestDerivedObj> derivedArray;
    derivedArray.add (static_cast<TestDerivedObj*> (new TestBaseObj()));
    EXPECT_EQ (derivedArray.size(), 1);
    EXPECT_EQ (derivedArray.getObjectPointer (0)->getReferenceCount(), 1);
    EXPECT_EQ (derivedArray[0]->getReferenceCount(), 2);

    for (auto o : derivedArray)
        EXPECT_EQ (o->getReferenceCount(), 1);

    ReferenceCountedArray<TestBaseObj> baseArray;
    baseArray.addArray (derivedArray);

    for (auto o : baseArray)
        EXPECT_EQ (o->getReferenceCount(), 2);

    derivedArray.clearQuick();
    baseArray.clearQuick();

    auto* baseObject = new TestBaseObj();
    TestBaseObj::Ptr baseObjectPtr = baseObject;
    EXPECT_EQ (baseObject->getReferenceCount(), 1);

    auto* derivedObject = new TestDerivedObj();
    TestDerivedObj::Ptr derivedObjectPtr = derivedObject;
    EXPECT_EQ (derivedObject->getReferenceCount(), 1);

    baseArray.add (baseObject);
    baseArray.add (derivedObject);

    for (auto o : baseArray)
        EXPECT_EQ (o->getReferenceCount(), 2);

    EXPECT_EQ (baseObject->getReferenceCount(), 2);
    EXPECT_EQ (derivedObject->getReferenceCount(), 2);

    derivedArray.add (derivedObject);

    for (auto o : derivedArray)
        EXPECT_EQ (o->getReferenceCount(), 3);

    derivedArray.clearQuick();
    baseArray.clearQuick();

    EXPECT_EQ (baseObject->getReferenceCount(), 1);
    EXPECT_EQ (derivedObject->getReferenceCount(), 1);

    baseArray.add (baseObjectPtr);
    baseArray.add (derivedObjectPtr.get());

    for (auto o : baseArray)
        EXPECT_EQ (o->getReferenceCount(), 2);

    derivedArray.add (derivedObjectPtr);

    for (auto o : derivedArray)
        EXPECT_EQ (o->getReferenceCount(), 3);
}

TEST (ReferenceCountedArrayTests, IterateInDestructor)
{
    auto tester = [] (const DestructorObj& obj)
    {
        for (auto* o : obj.objectArray)
        {
            EXPECT_TRUE (o != nullptr);
            EXPECT_TRUE (o != &obj);

            if (o != nullptr)
                EXPECT_EQ (o->data, 374);
        }
    };

    {
        ReferenceCountedArray<DestructorObj> arr;

        for (int i = 0; i < 2; ++i)
            arr.add (new DestructorObj (tester, arr));
    }

    ReferenceCountedArray<DestructorObj> arr;

    for (int i = 0; i < 1025; ++i)
        arr.add (new DestructorObj (tester, arr));

    while (! arr.isEmpty())
        arr.remove (0);

    for (int i = 0; i < 1025; ++i)
        arr.add (new DestructorObj (tester, arr));

    arr.removeRange (1, arr.size() - 3);

    for (int i = 0; i < 1025; ++i)
        arr.add (new DestructorObj (tester, arr));

    arr.set (500, new DestructorObj (tester, arr));
}
