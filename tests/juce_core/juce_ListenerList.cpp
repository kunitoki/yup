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
#include <gmock/gmock.h>

#include <juce_core/juce_core.h>

using namespace juce;

namespace {
class MockListener
{
public:
    MOCK_METHOD (void, myCallbackMethod, (int, bool) );
};

class ListenerListTests : public ::testing::Test
{
protected:
    class TestListener
    {
    public:
        explicit TestListener (std::function<void()> cb)
            : callback (std::move (cb))
        {
        }

        void doCallback()
        {
            ++numCalls;
            callback();
        }

        int getNumCalls() const { return numCalls; }

    private:
        int numCalls = 0;
        std::function<void()> callback;
    };

    class TestObject
    {
    public:
        void addListener (std::function<void()> cb)
        {
            listeners.push_back (std::make_unique<TestListener> (std::move (cb)));
            listenerList.add (listeners.back().get());
        }

        void removeListener (int i) { listenerList.remove (listeners[(size_t) i].get()); }

        void callListeners()
        {
            ++callLevel;
            listenerList.call ([] (auto& l)
                               {
                                   l.doCallback();
                               });
            --callLevel;
        }

        int getNumListeners() const { return (int) listeners.size(); }

        auto& getListener (int i) { return *listeners[(size_t) i]; }

        int getCallLevel() const { return callLevel; }

        bool wereAllNonRemovedListenersCalled (int numCalls) const
        {
            return std::all_of (std::begin (listeners), std::end (listeners), [&] (auto& listener)
                                {
                                    return (! listenerList.contains (listener.get())) || listener->getNumCalls() == numCalls;
                                });
        }

    private:
        std::vector<std::unique_ptr<TestListener>> listeners;
        ListenerList<TestListener> listenerList;
        int callLevel = 0;
    };

    static std::set<int> chooseUnique (Random& random, int max, int numChosen)
    {
        std::set<int> result;
        while ((int) result.size() < numChosen)
            result.insert (random.nextInt ({ 0, max }));
        return result;
    }
};

class MyListenerType
{
public:
    void myCallbackMethod (int foo, bool bar)
    {
        std::lock_guard lock (mutex);

        lastFoo = foo;
        lastBar = bar;
        callbackCount++;
    }

    int getCallbackCount() const
    {
        return callbackCount;
    }

    int getLastFoo() const
    {
        return lastFoo;
    }

    bool getLastBar() const
    {
        return lastBar;
    }

private:
    std::mutex mutex;
    int lastFoo = 0;
    bool lastBar = false;
    int callbackCount = 0;
};

using ThreadSafeList = yup::ListenerList<MyListenerType, yup::Array<MyListenerType*, yup::CriticalSection>>;

class WeakListenerType
{
public:
    void myCallbackMethod (int foo, bool bar)
    {
        callbackCount++;
    }

    static int callbackCount;

    JUCE_DECLARE_WEAK_REFERENCEABLE (WeakListenerType);
};

int WeakListenerType::callbackCount = 0;

} // namespace

TEST_F (ListenerListTests, Add_Remove_Contains)
{
    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_TRUE (listeners.contains (&listener1));
    EXPECT_TRUE (listeners.contains (&listener2));

    listeners.remove (&listener1);
    EXPECT_FALSE (listeners.contains (&listener1));
    EXPECT_TRUE (listeners.contains (&listener2));

    listeners.clear();
    EXPECT_FALSE (listeners.contains (&listener2));
}

TEST_F (ListenerListTests, Call)
{
    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_CALL (listener1, myCallbackMethod (1234, true)).Times (1);
    EXPECT_CALL (listener2, myCallbackMethod (1234, true)).Times (1);

    listeners.call (&MockListener::myCallbackMethod, 1234, true);
}

TEST_F (ListenerListTests, Call_Excluding)
{
    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_CALL (listener1, myCallbackMethod (1234, true)).Times (1);
    EXPECT_CALL (listener2, myCallbackMethod (1234, true)).Times (0);

    listeners.callExcluding (&listener2, &MockListener::myCallbackMethod, 1234, true);
}

TEST_F (ListenerListTests, Call_Checked)
{
    struct BailOutChecker
    {
        int callCount = 0;

        bool shouldBailOut() const { return callCount > 0; }
    };

    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;
    BailOutChecker checker;

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_CALL (listener1, myCallbackMethod (1234, true)).Times (1);
    EXPECT_CALL (listener2, myCallbackMethod (1234, true)).Times (1);

    listeners.callChecked (checker, &MockListener::myCallbackMethod, 1234, true);
    checker.callCount++;
    listeners.callChecked (checker, &MockListener::myCallbackMethod, 1234, true);
}

TEST_F (ListenerListTests, Call_Checked_Excluding)
{
    struct BailOutChecker
    {
        int callCount = 0;

        bool shouldBailOut() const { return callCount > 0; }
    };

    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;
    BailOutChecker checker;

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_CALL (listener1, myCallbackMethod (1234, true)).Times (1);
    EXPECT_CALL (listener2, myCallbackMethod (1234, true)).Times (0);

    listeners.callCheckedExcluding (&listener2, checker, &MockListener::myCallbackMethod, 1234, true);
    checker.callCount++;
    listeners.callCheckedExcluding (&listener2, checker, &MockListener::myCallbackMethod, 1234, true);
}

TEST_F (ListenerListTests, Add_Scoped)
{
    ListenerList<MockListener> listeners;
    MockListener listener1;

    {
        auto guard = listeners.addScoped (listener1);
        EXPECT_TRUE (listeners.contains (&listener1));
    }

    EXPECT_FALSE (listeners.contains (&listener1));
}

TEST_F (ListenerListTests, Size_Is_Empty)
{
    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;

    EXPECT_TRUE (listeners.isEmpty());
    EXPECT_EQ (listeners.size(), 0);

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_FALSE (listeners.isEmpty());
    EXPECT_EQ (listeners.size(), 2);

    listeners.remove (&listener1);
    EXPECT_EQ (listeners.size(), 1);

    listeners.clear();
    EXPECT_EQ (listeners.size(), 0);
}

/*
TEST_F (ListenerListTests, Null_Pointer_Handling)
{
    ListenerList<MockListener> listeners;
    MockListener* nullListener = nullptr;

    listeners.add (nullListener);
    EXPECT_FALSE (listeners.contains (nullListener));

    listeners.remove (nullListener);
    EXPECT_FALSE (listeners.contains (nullListener));
}
*/

TEST_F (ListenerListTests, Multiple_Add_Remove)
{
    ListenerList<MockListener> listeners;
    MockListener listener1;

    listeners.add (&listener1);
    listeners.add (&listener1);

    EXPECT_EQ (listeners.size(), 1);

    listeners.remove (&listener1);
    EXPECT_FALSE (listeners.contains (&listener1));
    EXPECT_EQ (listeners.size(), 0);
}

TEST_F (ListenerListTests, Call_During_Callback)
{
    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_CALL (listener1, myCallbackMethod (1234, true))
        .WillOnce (testing::Invoke ([&] (int, bool)
                                    {
                                        listeners.add (&listener1);
                                    }));
    EXPECT_CALL (listener2, myCallbackMethod (1234, true)).Times (1);

    listeners.call (&MockListener::myCallbackMethod, 1234, true);
    EXPECT_EQ (listeners.size(), 2);
}

TEST_F (ListenerListTests, Remove_During_Callback)
{
    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_CALL (listener1, myCallbackMethod (1234, true))
        .WillOnce (testing::Invoke ([&] (int, bool)
                                    {
                                        listeners.remove (&listener2);
                                    }));

    EXPECT_CALL (listener2, myCallbackMethod (1234, true)).Times (0);

    listeners.call (&MockListener::myCallbackMethod, 1234, true);
    EXPECT_EQ (listeners.size(), 1);
    EXPECT_FALSE (listeners.contains (&listener2));
}

TEST_F (ListenerListTests, Clear_During_Callback)
{
    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_CALL (listener1, myCallbackMethod (1234, true)).Times (1).WillOnce (testing::Invoke ([&] (int, bool)
                                                                                                {
                                                                                                    listeners.clear();
                                                                                                }));
    EXPECT_CALL (listener2, myCallbackMethod (1234, true)).Times (0);

    listeners.call (&MockListener::myCallbackMethod, 1234, true);
    EXPECT_EQ (listeners.size(), 0);
}

TEST_F (ListenerListTests, Nested_Call)
{
    ListenerList<MockListener> listeners;
    MockListener listener1, listener2;

    listeners.add (&listener1);
    listeners.add (&listener2);

    EXPECT_CALL (listener1, myCallbackMethod (1234, true)).Times (1).WillOnce (testing::Invoke ([&] (int, bool)
                                                                                                {
                                                                                                    listeners.call (&MockListener::myCallbackMethod, 5678, false);
                                                                                                }));
    EXPECT_CALL (listener2, myCallbackMethod (1234, true)).Times (1);
    EXPECT_CALL (listener1, myCallbackMethod (5678, false)).Times (1);
    EXPECT_CALL (listener2, myCallbackMethod (5678, false)).Times (1);

    listeners.call (&MockListener::myCallbackMethod, 1234, true);
}

TEST_F (ListenerListTests, RemovingAlreadyCalledListener)
{
    TestObject test;
    for (int i = 0; i < 20; ++i)
    {
        test.addListener ([i, &test]
                          {
                              if (i == 5)
                                  test.removeListener (6);
                          });
    }

    test.callListeners();
    EXPECT_TRUE (test.wereAllNonRemovedListenersCalled (1));
}

TEST_F (ListenerListTests, RemovingYetUncalledListener)
{
    TestObject test;
    for (int i = 0; i < 20; ++i)
    {
        test.addListener ([i, &test]
                          {
                              if (i == 5)
                                  test.removeListener (4);
                          });
    }

    test.callListeners();
    EXPECT_TRUE (test.wereAllNonRemovedListenersCalled (1));
}

TEST_F (ListenerListTests, RemoveMultipleListenersInCallback)
{
    TestObject test;
    for (int i = 0; i < 20; ++i)
    {
        test.addListener ([i, &test]
                          {
                              if (i == 19)
                              {
                                  test.removeListener (19);
                                  test.removeListener (0);
                              }
                          });
    }

    test.callListeners();
    EXPECT_TRUE (test.wereAllNonRemovedListenersCalled (1));
}

TEST_F (ListenerListTests, RemovingListenersRandomly)
{
    auto random = Random::getSystemRandom();
    for (auto run = 0; run < 10; ++run)
    {
        const auto numListeners = random.nextInt ({ 10, 100 });
        const auto listenersThatRemoveListeners = chooseUnique (random, numListeners, random.nextInt ({ 0, numListeners / 2 }));

        std::map<int, std::set<int>> removals;
        for (auto i : listenersThatRemoveListeners)
            removals[i] = chooseUnique (random, numListeners, random.nextInt ({ 1, std::max (2, numListeners / 10) }));

        TestObject test;
        for (int i = 0; i < numListeners; ++i)
        {
            test.addListener ([i, &removals, &test]
                              {
                                  const auto iter = removals.find (i);
                                  if (iter == removals.end())
                                      return;

                                  for (auto j : iter->second)
                                      test.removeListener (j);
                              });
        }

        test.callListeners();
        EXPECT_TRUE (test.wereAllNonRemovedListenersCalled (1));
    }
}

TEST_F (ListenerListTests, AddListenerDuringIteration)
{
    TestObject test;
    const auto numStartingListeners = 20;
    for (int i = 0; i < numStartingListeners; ++i)
    {
        test.addListener ([i, &test]
                          {
                              if (i == 5 || i == 6)
                                  test.addListener ([] {});
                          });
    }

    test.callListeners();
    bool success = true;

    for (int i = 0; i < numStartingListeners; ++i)
        success = success && test.getListener (i).getNumCalls() == 1;

    for (int i = numStartingListeners; i < test.getNumListeners(); ++i)
        success = success && test.getListener (i).getNumCalls() == 0;

    EXPECT_TRUE (success);
}

TEST_F (ListenerListTests, NestedCall)
{
    TestObject test;
    for (int i = 0; i < 20; ++i)
    {
        test.addListener ([i, &test]
                          {
                              const auto callLevel = test.getCallLevel();
                              if (i == 6 && callLevel == 1)
                                  test.callListeners();

                              if (i == 5)
                              {
                                  if (callLevel == 1)
                                      test.removeListener (4);
                                  else if (callLevel == 2)
                                      test.removeListener (6);
                              }
                          });
    }

    test.callListeners();
    EXPECT_TRUE (test.wereAllNonRemovedListenersCalled (2));
}

TEST_F (ListenerListTests, RandomCall)
{
    const auto numListeners = 20;
    auto random = Random::getSystemRandom();

    for (int run = 0; run < 10; ++run)
    {
        TestObject test;
        auto numCalls = 0;

        auto listenersToRemove = chooseUnique (random, numListeners, numListeners / 2);
        for (int i = 0; i < numListeners; ++i)
        {
            test.addListener ([&]
                              {
                                  const auto callLevel = test.getCallLevel();
                                  if (callLevel < 4 && random.nextFloat() < 0.05f)
                                  {
                                      ++numCalls;
                                      test.callListeners();
                                  }

                                  if (random.nextFloat() < 0.5f)
                                  {
                                      const auto listenerToRemove = random.nextInt ({ 0, numListeners });
                                      if (listenersToRemove.erase (listenerToRemove) > 0)
                                          test.removeListener (listenerToRemove);
                                  }
                              });
        }

        while (listenersToRemove.size() > 0)
        {
            test.callListeners();
            ++numCalls;
        }

        EXPECT_TRUE (test.wereAllNonRemovedListenersCalled (numCalls));
    }
}

TEST_F (ListenerListTests, DeletingListenerListFromCallback)
{
    struct Listener
    {
        std::function<void()> onCallback;

        void notify() { onCallback(); }
    };

    auto listeners = std::make_unique<ListenerList<Listener>>();

    const auto callback = [&]
    {
        EXPECT_TRUE (listeners != nullptr);
        listeners.reset();
    };

    Listener listener1 { callback };
    Listener listener2 { callback };

    listeners->add (&listener1);
    listeners->add (&listener2);

    listeners->call (&Listener::notify);
    EXPECT_TRUE (listeners == nullptr);
}

TEST_F (ListenerListTests, BailOutChecker)
{
    struct Listener
    {
        std::function<void()> onCallback;

        void notify() { onCallback(); }
    };

    ListenerList<Listener> listeners;
    bool listener1Called = false;
    bool listener2Called = false;
    bool listener3Called = false;

    Listener listener1 { [&]
                         {
                             listener1Called = true;
                         } };
    Listener listener2 { [&]
                         {
                             listener2Called = true;
                         } };
    Listener listener3 { [&]
                         {
                             listener3Called = true;
                         } };

    listeners.add (&listener1);
    listeners.add (&listener2);
    listeners.add (&listener3);

    struct BailOutChecker
    {
        bool& bailOutBool;

        bool shouldBailOut() const { return bailOutBool; }
    };

    BailOutChecker bailOutChecker { listener2Called };
    listeners.callChecked (bailOutChecker, &Listener::notify);

    EXPECT_TRUE (listener1Called);
    EXPECT_TRUE (listener2Called);
    EXPECT_FALSE (listener3Called);
}

TEST_F (ListenerListTests, CriticalSection)
{
    struct Listener
    {
        std::function<void()> onCallback;

        void notify() { onCallback(); }
    };

    struct TestCriticalSection
    {
        TestCriticalSection() { isAlive() = true; }

        ~TestCriticalSection() { isAlive() = false; }

        static void enter() noexcept { numOutOfScopeCalls() += isAlive() ? 0 : 1; }

        static void exit() noexcept { numOutOfScopeCalls() += isAlive() ? 0 : 1; }

        static bool tryEnter() noexcept
        {
            numOutOfScopeCalls() += isAlive() ? 0 : 1;
            return true;
        }

        using ScopedLockType = GenericScopedLock<TestCriticalSection>;

        static bool& isAlive()
        {
            static bool inScope = false;
            return inScope;
        }

        static int& numOutOfScopeCalls()
        {
            static int numOutOfScopeCalls = 0;
            return numOutOfScopeCalls;
        }
    };

    auto listeners = std::make_unique<ListenerList<Listener, Array<Listener*, TestCriticalSection>>>();

    const auto callback = [&]
    {
        listeners.reset();
    };

    Listener listener { callback };

    listeners->add (&listener);
    listeners->call (&Listener::notify);

    EXPECT_TRUE (listeners == nullptr);
    EXPECT_EQ (TestCriticalSection::numOutOfScopeCalls(), 0);
}

TEST_F (ListenerListTests, AddListenerDuringCallback)
{
    struct Listener
    {
    };

    ListenerList<Listener> listeners;
    EXPECT_EQ (listeners.size(), 0);

    Listener listener;
    listeners.add (&listener);
    EXPECT_EQ (listeners.size(), 1);

    bool listenerCalled = false;

    listeners.call ([&] (auto& l)
                    {
                        listeners.remove (&l);
                        EXPECT_EQ (listeners.size(), 0);

                        listeners.add (&l);
                        EXPECT_EQ (listeners.size(), 1);

                        listenerCalled = true;
                    });

    EXPECT_TRUE (listenerCalled);
    EXPECT_EQ (listeners.size(), 1);
}

TEST_F (ListenerListTests, ClearListenersDuringCallback)
{
    struct Listener
    {
        Listener (std::function<void()> callbackIn)
            : callback (std::move (callbackIn))
        {
        }

        std::function<void()> callback;

        void notify() { callback(); }
    };

    yup::ListenerList<Listener> listeners;

    bool called = false;
    Listener listener1 { [&]
                         {
                             listeners.clear();
                         } };
    Listener listener2 { [&]
                         {
                             called = true;
                         } };

    listeners.add (&listener1);
    listeners.add (&listener2);

    listeners.call (&Listener::notify);
    EXPECT_FALSE (called);
}

TEST_F (ListenerListTests, ThreadSafeAddRemoveListeners)
{
    ThreadSafeList listeners;

    MyListenerType listener1, listener2, listener3;

    auto addListeners = [&]
    {
        for (int i = 0; i < 1000; ++i)
        {
            listeners.add (&listener1);
            listeners.add (&listener2);
            listeners.add (&listener3);
        }
    };

    auto removeListeners = [&]
    {
        for (int i = 0; i < 1000; ++i)
        {
            listeners.remove (&listener1);
            listeners.remove (&listener2);
            listeners.remove (&listener3);
        }
    };

    auto callListeners = [&]
    {
        for (int i = 0; i < 1000; ++i)
            listeners.call ([] (MyListenerType& l)
                            {
                                l.myCallbackMethod (1234, true);
                            });
    };

    std::thread thread1 (addListeners);
    std::thread thread2 (removeListeners);
    std::thread thread3 (callListeners);

    thread1.join();
    thread2.join();
    thread3.join();

    SUCCEED();
}

TEST_F (ListenerListTests, ThreadSafeCallListeners)
{
    ThreadSafeList listeners;

    MyListenerType listener1, listener2, listener3;
    listeners.add (&listener1);
    listeners.add (&listener2);
    listeners.add (&listener3);

    auto callListeners = [&]
    {
        for (int i = 0; i < 1000; ++i)
            listeners.call ([] (MyListenerType& l)
                            {
                                l.myCallbackMethod (1234, true);
                            });
    };

    std::thread thread1 (callListeners);
    std::thread thread2 (callListeners);

    thread1.join();
    thread2.join();

    // Verify that all listeners have been called
    EXPECT_EQ (listener1.getCallbackCount(), 2000);
    EXPECT_EQ (listener2.getCallbackCount(), 2000);
    EXPECT_EQ (listener3.getCallbackCount(), 2000);
}

TEST_F (ListenerListTests, ThreadSafeAddRemoveWhileCalling)
{
    ThreadSafeList listeners;

    MyListenerType listener1, listener2, listener3;
    listeners.add (&listener1);
    listeners.add (&listener2);
    listeners.add (&listener3);

    auto callListeners = [&]
    {
        for (int i = 0; i < 1000; ++i)
            listeners.call ([] (MyListenerType& l)
                            {
                                l.myCallbackMethod (1234, true);
                            });
    };

    auto addRemoveListeners = [&]
    {
        for (int i = 0; i < 1000; ++i)
        {
            listeners.remove (&listener1);
            listeners.add (&listener1);

            listeners.remove (&listener2);
            listeners.add (&listener2);
        }
    };

    std::thread thread1 (callListeners);
    std::thread thread2 (addRemoveListeners);

    thread1.join();
    thread2.join();

    // Verify that listeners have been called
    EXPECT_GE (listener1.getCallbackCount(), 0);
    EXPECT_GE (listener2.getCallbackCount(), 0);
    EXPECT_EQ (listener3.getCallbackCount(), 1000);
}

TEST_F (ListenerListTests, ListOfWeakReferenceable)
{
    using WeakListenerList = ListenerList<WeakListenerType, Array<WeakReference<WeakListenerType>>>;

    WeakListenerList listeners;

    {
        WeakListenerType listener1, listener2, listener3;
        listeners.add (&listener1);
        listeners.add (&listener2);
        listeners.add (&listener3);
    }

    listeners.call (&WeakListenerType::myCallbackMethod, 1, false);

    EXPECT_EQ (WeakListenerType::callbackCount, 0);
}
