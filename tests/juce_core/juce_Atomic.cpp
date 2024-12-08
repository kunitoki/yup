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

namespace
{

template <class Type>
class AtomicTester
{
public:
    AtomicTester() = default;

    static void testInteger()
    {
        Atomic<Type> a, b;
        Type c;

        a.set ((Type) 10);
        c = (Type) 10;

        EXPECT_TRUE (a.value == c);
        EXPECT_TRUE (a.get() == c);

        a += 15;
        c += 15;
        EXPECT_TRUE (a.get() == c);
        a.memoryBarrier();

        a -= 5;
        c -= 5;
        EXPECT_TRUE (a.get() == c);

        EXPECT_TRUE (++a == ++c);
        ++a;
        ++c;
        EXPECT_TRUE (--a == --c);
        EXPECT_TRUE (a.get() == c);
        a.memoryBarrier();

        testFloat();
    }

    static void testFloat()
    {
        Atomic<Type> a, b;
        a = (Type) 101;
        a.memoryBarrier();

        EXPECT_TRUE (exactlyEqual (a.get(), (Type) 101));
        EXPECT_TRUE (! a.compareAndSetBool ((Type) 300, (Type) 200));
        EXPECT_TRUE (exactlyEqual (a.get(), (Type) 101));
        EXPECT_TRUE (a.compareAndSetBool ((Type) 200, a.get()));
        EXPECT_TRUE (exactlyEqual (a.get(), (Type) 200));

        EXPECT_TRUE (exactlyEqual (a.exchange ((Type) 300), (Type) 200));
        EXPECT_TRUE (exactlyEqual (a.get(), (Type) 300));

        b = a;
        EXPECT_TRUE (exactlyEqual (b.get(), a.get()));
    }
};

} // namespace

TEST (AtomicTests, Misc)
{
    char a1[7];
    EXPECT_TRUE (numElementsInArray (a1) == 7);
    int a2[3];
    EXPECT_TRUE (numElementsInArray (a2) == 3);

    EXPECT_TRUE (ByteOrder::swap ((uint16) 0x1122) == 0x2211);
    EXPECT_TRUE (ByteOrder::swap ((uint32) 0x11223344) == 0x44332211);
    EXPECT_TRUE (ByteOrder::swap ((uint64) 0x1122334455667788ULL) == (uint64) 0x8877665544332211LL);

    AtomicTester<int>::testInteger();
    AtomicTester<unsigned int>::testInteger();
    AtomicTester<int32>::testInteger();
    AtomicTester<uint32>::testInteger();
    AtomicTester<long>::testInteger();
    AtomicTester<int*>::testInteger();
    AtomicTester<float>::testFloat();
#if ! JUCE_64BIT_ATOMICS_UNAVAILABLE // 64-bit intrinsics aren't available on some old platforms
    AtomicTester<int64>::testInteger();
    AtomicTester<uint64>::testInteger();
    AtomicTester<double>::testFloat();
#endif

    Atomic<int*> a (a2);
    int* b (a2);
    EXPECT_TRUE (++a == ++b);

    {
        Atomic<void*> atomic;
        void* c;

        atomic.set ((void*) 10);
        c = (void*) 10;

        EXPECT_TRUE (atomic.value == c);
        EXPECT_TRUE (atomic.get() == c);
    }
}
