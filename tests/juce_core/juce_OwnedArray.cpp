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

using namespace juce;

struct OwnedArrayTests : public ::testing::Test
{
    struct Base
    {
        Base() = default;
        virtual ~Base() = default;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Base)
    };

    struct Derived final : public Base
    {
        Derived() = default;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Derived)
    };

    struct DestructorObj
    {
        DestructorObj (OwnedArrayTests& p, OwnedArray<DestructorObj>& arr)
            : parent (p)
            , objectArray (arr)
        {
        }

        ~DestructorObj()
        {
            data = 0;

            parent.testDestruction (this, objectArray);
        }

        OwnedArrayTests& parent;
        OwnedArray<DestructorObj>& objectArray;
        int data = 956;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DestructorObj)
    };

    void testDestruction (DestructorObj* self, const OwnedArray<DestructorObj>& objectArray)
    {
        for (auto* o : objectArray)
        {
            ASSERT_NE (o, nullptr);
            ASSERT_NE (o, self);

            if (o != nullptr)
                EXPECT_EQ (o->data, 956);
        }
    }
};

TEST_F (OwnedArrayTests, MoveConstructionTransfersOwnership)
{
    OwnedArray<Derived> derived;
    derived.add (new Derived());
    derived.add (new Derived());
    derived.add (new Derived());

    OwnedArray<Base> base (std::move (derived));

    EXPECT_EQ (base.size(), 3);
    EXPECT_EQ (derived.size(), 0);
}

TEST_F (OwnedArrayTests, MoveAssignmentTransfersOwnership)
{
    OwnedArray<Base> base;

    base = OwnedArray<Derived> { new Derived(), new Derived(), new Derived() };

    EXPECT_EQ (base.size(), 3);
}

TEST_F (OwnedArrayTests, IterateInDestructor)
{
    {
        OwnedArray<DestructorObj> arr;

        for (int i = 0; i < 2; ++i)
            arr.add (new DestructorObj (*this, arr));
    }

    OwnedArray<DestructorObj> arr;

    for (int i = 0; i < 1025; ++i)
        arr.add (new DestructorObj (*this, arr));

    while (! arr.isEmpty())
        arr.remove (0);

    for (int i = 0; i < 1025; ++i)
        arr.add (new DestructorObj (*this, arr));

    arr.removeRange (1, arr.size() - 3);

    for (int i = 0; i < 1025; ++i)
        arr.add (new DestructorObj (*this, arr));

    arr.set (500, new DestructorObj (*this, arr));
}
