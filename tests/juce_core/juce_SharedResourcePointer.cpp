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

#include <optional>

using namespace juce;

class SharedResourcePointerTests : public ::testing::Test
{
protected:
    template <class T>
    static auto getSharedObjectWithoutCreating()
    {
        return SharedResourcePointer<T>::getSharedObjectWithoutCreating();
    }
};

TEST_F (SharedResourcePointerTests, OnlyOneInstanceIsCreated)
{
    static int count = 0;

    struct CountIncrementer
    {
        CountIncrementer() { ++count; }
    };

    EXPECT_EQ (count, 0);

    const SharedResourcePointer<CountIncrementer> instance1;
    EXPECT_EQ (count, 1);

    const SharedResourcePointer<CountIncrementer> instance2;
    EXPECT_EQ (count, 1);

    EXPECT_EQ (&instance1.get(), &instance2.get());
}

TEST_F (SharedResourcePointerTests, SharedObjectDestroyedWhenReferenceCountReachesZero)
{
    static int count = 0;

    struct ReferenceCounter
    {
        ReferenceCounter() { ++count; }

        ~ReferenceCounter() { --count; }
    };

    EXPECT_EQ (count, 0);

    {
        const SharedResourcePointer<ReferenceCounter> instance1;
        const SharedResourcePointer<ReferenceCounter> instance2;
        EXPECT_EQ (count, 1);
    }

    EXPECT_EQ (count, 0);
}

TEST_F (SharedResourcePointerTests, GetInstanceWithoutCreating)
{
    struct Object
    {
    };

    EXPECT_EQ (getSharedObjectWithoutCreating<Object>(), std::nullopt);

    {
        const SharedResourcePointer<Object> instance;
        EXPECT_EQ (&getSharedObjectWithoutCreating<Object>()->get(), &instance.get());
    }

    EXPECT_EQ (getSharedObjectWithoutCreating<Object>(), std::nullopt);
}

TEST_F (SharedResourcePointerTests, CreateObjectsWithPrivateConstructors)
{
    class ObjectWithPrivateConstructor
    {
    private:
        ObjectWithPrivateConstructor() = default;
        friend SharedResourcePointer<ObjectWithPrivateConstructor>;
    };

    SharedResourcePointer<ObjectWithPrivateConstructor> instance;
}
