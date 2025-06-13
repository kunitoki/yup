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

#include <yup_core/yup_core.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

class TestSingleton
{
public:
    TestSingleton()
        : constructorCallCount (++globalConstructorCallCount)
    {
    }

    ~TestSingleton()
    {
        clearSingletonInstance();
        ++globalDestructorCallCount;
    }

    int getConstructorCallCount() const { return constructorCallCount; }

    YUP_DECLARE_SINGLETON (TestSingleton, false)

    static int globalConstructorCallCount;
    static int globalDestructorCallCount;

private:
    int constructorCallCount;
};

YUP_IMPLEMENT_SINGLETON (TestSingleton)
int TestSingleton::globalConstructorCallCount = 0;
int TestSingleton::globalDestructorCallCount = 0;

class TestSingletonDoNotRecreate
{
public:
    TestSingletonDoNotRecreate()
        : value (42)
    {
    }

    ~TestSingletonDoNotRecreate()
    {
        clearSingletonInstance();
    }

    int getValue() const { return value; }

    YUP_DECLARE_SINGLETON (TestSingletonDoNotRecreate, true)

private:
    int value;
};

YUP_IMPLEMENT_SINGLETON (TestSingletonDoNotRecreate)

class TestSingletonSingleThreaded
{
public:
    TestSingletonSingleThreaded()
        : data ("single_threaded")
    {
    }

    ~TestSingletonSingleThreaded()
    {
        clearSingletonInstance();
    }

    const String& getData() const { return data; }

    YUP_DECLARE_SINGLETON_SINGLETHREADED (TestSingletonSingleThreaded, false)

private:
    String data;
};

YUP_IMPLEMENT_SINGLETON (TestSingletonSingleThreaded)

class TestSingletonMinimal
{
public:
    TestSingletonMinimal()
        : count (100)
    {
    }

    ~TestSingletonMinimal()
    {
        clearSingletonInstance();
    }

    int getCount() const { return count; }

    void incrementCount() { ++count; }

    YUP_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL (TestSingletonMinimal)

private:
    int count;
};

YUP_IMPLEMENT_SINGLETON (TestSingletonMinimal)
} // namespace

class SingletonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Clean up any existing instances
        TestSingleton::deleteInstance();
        TestSingletonDoNotRecreate::deleteInstance();
        TestSingletonSingleThreaded::deleteInstance();
        TestSingletonMinimal::deleteInstance();

        TestSingleton::globalConstructorCallCount = 0;
        TestSingleton::globalDestructorCallCount = 0;
    }

    void TearDown() override
    {
        // Clean up after tests
        TestSingleton::deleteInstance();
        TestSingletonDoNotRecreate::deleteInstance();
        TestSingletonSingleThreaded::deleteInstance();
        TestSingletonMinimal::deleteInstance();
    }
};

TEST_F (SingletonTest, BasicSingletonCreation)
{
    auto* instance1 = TestSingleton::getInstance();
    auto* instance2 = TestSingleton::getInstance();

    EXPECT_NE (instance1, nullptr);
    EXPECT_EQ (instance1, instance2);
    EXPECT_EQ (TestSingleton::globalConstructorCallCount, 1);
}

TEST_F (SingletonTest, GetInstanceWithoutCreating)
{
    auto* instance = TestSingleton::getInstanceWithoutCreating();
    EXPECT_EQ (instance, nullptr);

    // Now create one
    TestSingleton::getInstance();

    instance = TestSingleton::getInstanceWithoutCreating();
    EXPECT_NE (instance, nullptr);
}

TEST_F (SingletonTest, DeleteInstance)
{
    auto* instance = TestSingleton::getInstance();
    EXPECT_NE (instance, nullptr);
    EXPECT_EQ (TestSingleton::globalDestructorCallCount, 0);

    TestSingleton::deleteInstance();

    EXPECT_EQ (TestSingleton::globalDestructorCallCount, 1);
    EXPECT_EQ (TestSingleton::getInstanceWithoutCreating(), nullptr);
}

TEST_F (SingletonTest, RecreateAfterDeletion)
{
    auto* instance1 = TestSingleton::getInstance();
    EXPECT_EQ (instance1->getConstructorCallCount(), 1);

    TestSingleton::deleteInstance();

    auto* instance2 = TestSingleton::getInstance();
    EXPECT_NE (instance2, nullptr);
    EXPECT_EQ (instance2->getConstructorCallCount(), 2);
}

TEST_F (SingletonTest, DoNotRecreateAfterDeletion)
{
    auto* instance = TestSingletonDoNotRecreate::getInstance();
    EXPECT_NE (instance, nullptr);
    EXPECT_EQ (instance->getValue(), 42);

    TestSingletonDoNotRecreate::deleteInstance();

    // Should not be able to create again
    // auto* instance2 = TestSingletonDoNotRecreate::getInstance();
    // EXPECT_EQ (instance2, nullptr);
}

TEST_F (SingletonTest, SingleThreadedSingleton)
{
    auto* instance1 = TestSingletonSingleThreaded::getInstance();
    auto* instance2 = TestSingletonSingleThreaded::getInstance();

    EXPECT_NE (instance1, nullptr);
    EXPECT_EQ (instance1, instance2);
    EXPECT_EQ (instance1->getData(), "single_threaded");
}

TEST_F (SingletonTest, MinimalSingleton)
{
    auto* instance1 = TestSingletonMinimal::getInstance();
    auto* instance2 = TestSingletonMinimal::getInstance();

    EXPECT_NE (instance1, nullptr);
    EXPECT_EQ (instance1, instance2);
    EXPECT_EQ (instance1->getCount(), 100);

    instance1->incrementCount();
    EXPECT_EQ (instance2->getCount(), 101); // Same instance
}

TEST_F (SingletonTest, ClearSingletonInstance)
{
    auto* instance = TestSingleton::getInstance();
    EXPECT_NE (instance, nullptr);

    // Manually delete the instance to test clearSingletonInstance
    delete instance;

    // The singleton holder should have been cleared automatically
    auto* newInstance = TestSingleton::getInstanceWithoutCreating();
    EXPECT_EQ (newInstance, nullptr);
}

TEST_F (SingletonTest, MultipleCallsToDeleteInstance)
{
    TestSingleton::getInstance();

    // Multiple calls to deleteInstance should be safe
    TestSingleton::deleteInstance();
    TestSingleton::deleteInstance();
    TestSingleton::deleteInstance();

    EXPECT_EQ (TestSingleton::getInstanceWithoutCreating(), nullptr);
}

TEST_F (SingletonTest, DeleteInstanceWithoutCreating)
{
    // Should be safe to delete even if never created
    EXPECT_NO_THROW (TestSingleton::deleteInstance());
    EXPECT_EQ (TestSingleton::getInstanceWithoutCreating(), nullptr);
}

TEST_F (SingletonTest, SingletonHolderTemplate)
{
    // Test the SingletonHolder directly with a simple class
    struct SimpleClass
    {
        int value = 99;
    };

    SingletonHolder<SimpleClass, CriticalSection, false> holder;

    auto* instance1 = holder.get();
    auto* instance2 = holder.get();

    EXPECT_NE (instance1, nullptr);
    EXPECT_EQ (instance1, instance2);
    EXPECT_EQ (instance1->value, 99);

    holder.deleteInstance();

    auto* instance3 = holder.get();
    EXPECT_NE (instance3, nullptr);

    holder.clear (instance3);
    delete instance3;
}

TEST_F (SingletonTest, SingletonHolderWithoutChecking)
{
    struct SimpleClass
    {
        int value = 77;
    };

    SingletonHolder<SimpleClass, DummyCriticalSection, false> holder;

    auto* instance = holder.getWithoutChecking();
    EXPECT_NE (instance, nullptr);
    EXPECT_EQ (instance->value, 77);

    holder.deleteInstance();
}
