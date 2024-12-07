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

namespace {
struct ThrowOnMoveOrSwap
{
    ThrowOnMoveOrSwap() = default;

    ThrowOnMoveOrSwap(ThrowOnMoveOrSwap&&) { throw std::bad_alloc(); }
};

void swap(ThrowOnMoveOrSwap&, ThrowOnMoveOrSwap&) { throw std::bad_alloc(); }

struct ThrowOnCopy
{
    ThrowOnCopy() = default;

    // Put into an invalid state and throw
    ThrowOnCopy (const ThrowOnCopy&)
    {
        value = -100;
        throw std::bad_alloc {};
    }

    // Put into an invalid state and throw
    ThrowOnCopy& operator= (const ThrowOnCopy&)
    {
        value = -100;
        throw std::bad_alloc {};
    }

    ThrowOnCopy (ThrowOnCopy&&) noexcept = default;
    ThrowOnCopy& operator= (ThrowOnCopy&&) noexcept = default;

    ~ThrowOnCopy() = default;

    int value = 0;
};
} // namespace

TEST (OptionalTests, DefaultConstructedOptionalIsInvalid)
{
    Optional<int> o;
    EXPECT_FALSE(o.hasValue());
}

TEST (OptionalTests, ConstructingFromNulloptIsInvalid)
{
    Optional<int> o(nullopt);
    EXPECT_FALSE(o.hasValue());
}

TEST (OptionalTests, OptionalConstructedFromValueIsValid)
{
    Optional<int> o = 5;
    EXPECT_TRUE(o.hasValue());
    EXPECT_EQ(*o, 5);
}

TEST (OptionalTests, ConstructingFromMovedOptionalCallsAppropriateMemberFunctions)
{
    using Ptr = std::shared_ptr<int>;
    auto makePtr = [] { return std::make_shared<int>(); };

    auto ptr = makePtr();
    Optional<Ptr> original(ptr);
    EXPECT_EQ(ptr.use_count(), 2);

    auto other = std::move(original);
    EXPECT_TRUE(original.hasValue());
    EXPECT_TRUE(other.hasValue());
    EXPECT_EQ(ptr.use_count(), 2);
}

TEST (OptionalTests, MovingEmptyOptionalToPopulatedOneDestroysInstance)
{
    using Ptr = std::shared_ptr<int>;
    auto makePtr = [] { return std::make_shared<int>(); };

    auto ptr = makePtr();
    Optional<Ptr> original(ptr);
    EXPECT_EQ(ptr.use_count(), 2);

    original = Optional<Ptr>();
    EXPECT_EQ(ptr.use_count(), 1);
}

TEST (OptionalTests, CopyingEmptyOptionalToPopulatedOneDestroysInstance)
{
    using Ptr = std::shared_ptr<int>;
    auto makePtr = [] { return std::make_shared<int>(); };

    auto ptr = makePtr();
    Optional<Ptr> original(ptr);
    EXPECT_EQ(ptr.use_count(), 2);

    Optional<Ptr> empty;
    original = empty;
    EXPECT_EQ(ptr.use_count(), 1);
}

TEST (OptionalTests, MovingPopulatedOptionalCallsAppropriateMemberFunctions)
{
    using Ptr = std::shared_ptr<int>;
    auto makePtr = [] { return std::make_shared<int>(); };

    auto a = makePtr();
    auto b = makePtr();

    Optional<Ptr> aOpt(a);
    Optional<Ptr> bOpt(b);

    EXPECT_EQ(a.use_count(), 2);
    EXPECT_EQ(b.use_count(), 2);

    aOpt = std::move(bOpt);

    EXPECT_TRUE(aOpt.hasValue());
    EXPECT_TRUE(bOpt.hasValue());
    EXPECT_EQ(a.use_count(), 1);
    EXPECT_EQ(b.use_count(), 2);
}

TEST (OptionalTests, CopyingPopulatedOptionalCallsAppropriateMemberFunctions)
{
    using Ptr = std::shared_ptr<int>;
    auto makePtr = [] { return std::make_shared<int>(); };

    auto a = makePtr();
    auto b = makePtr();

    Optional<Ptr> aOpt(a);
    Optional<Ptr> bOpt(b);

    EXPECT_EQ(a.use_count(), 2);
    EXPECT_EQ(b.use_count(), 2);

    aOpt = bOpt;

    EXPECT_TRUE(aOpt.hasValue());
    EXPECT_TRUE(bOpt.hasValue());
    EXPECT_EQ(a.use_count(), 1);
    EXPECT_EQ(b.use_count(), 3);
}

TEST (OptionalTests, StrongExceptionSafetyWhenForwardingOverEmptyObject)
{
    bool threw = false;
    Optional<ThrowOnCopy> a;

    try
    {
        ThrowOnCopy t;
        a = t;
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    EXPECT_TRUE(threw);
    EXPECT_FALSE(a.hasValue());
}

TEST (OptionalTests, WeakExceptionSafetyWhenForwardingOverPopulatedObject)
{
    bool threw = false;
    Optional<ThrowOnCopy> a = ThrowOnCopy();
    a->value = 5;

    try
    {
        ThrowOnCopy t;
        a = t;
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    EXPECT_TRUE(threw);
    EXPECT_TRUE(a.hasValue());
    EXPECT_EQ(a->value, -100);
}

TEST (OptionalTests, StrongExceptionSafetyWhenCopyingOverEmptyObject)
{
    bool threw = false;
    Optional<ThrowOnCopy> a;

    try
    {
        Optional<ThrowOnCopy> t = ThrowOnCopy {};
        a = t;
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    EXPECT_TRUE(threw);
    EXPECT_FALSE(a.hasValue());
}

TEST (OptionalTests, WeakExceptionSafetyWhenCopyingOverPopulatedObject)
{
    bool threw = false;
    Optional<ThrowOnCopy> a = ThrowOnCopy();
    a->value = 5;

    try
    {
        Optional<ThrowOnCopy> t = ThrowOnCopy {};
        a = t;
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    EXPECT_TRUE(threw);
    EXPECT_TRUE(a.hasValue());
    EXPECT_EQ(a->value, -100);
}

TEST (OptionalTests, AssigningFromNulloptClearsInstance)
{
    using Ptr = std::shared_ptr<int>;
    auto makePtr = [] { return std::make_shared<int>(); };

    auto ptr = makePtr();
    Optional<Ptr> a(ptr);
    EXPECT_EQ(ptr.use_count(), 2);

    a = nullopt;
    EXPECT_EQ(ptr.use_count(), 1);
}

TEST (OptionalTests, CanBeConstructedAndAssignedAndCopiedAndMovedFromCompatibleType)
{
    struct Foo
    {
    };

    struct Bar final : public Foo
    {
    };

    {
        Optional<std::shared_ptr<Foo>> opt { std::make_shared<Bar>() };
        opt = std::make_shared<Bar>();
    }

    {
        auto ptr = std::make_shared<Bar>();
        Optional<std::shared_ptr<Bar>> bar (ptr);
        Optional<std::shared_ptr<Foo>> foo (bar);
        EXPECT_TRUE (ptr.use_count() == 3);
    }

    {
        auto ptr = std::make_shared<Bar>();
        Optional<std::shared_ptr<Foo>> foo (Optional<std::shared_ptr<Bar>> { ptr });
        EXPECT_TRUE (ptr.use_count() == 2);
    }

    {
        auto ptr = std::make_shared<Bar>();
        Optional<std::shared_ptr<Bar>> bar (ptr);
        Optional<std::shared_ptr<Foo>> foo;
        foo = bar;
        EXPECT_TRUE (ptr.use_count() == 3);
    }

    {
        auto ptr = std::make_shared<Bar>();
        Optional<std::shared_ptr<Foo>> foo;
        foo = Optional<std::shared_ptr<Bar>> (ptr);
        EXPECT_TRUE (ptr.use_count() == 2);
    }
}

TEST (OptionalTests, ExceptionThrownDuringEmplaceLeavesOptionalWithoutValue)
{
    Optional<ThrowOnCopy> opt { ThrowOnCopy {} };
    bool threw = false;

    try
    {
        ThrowOnCopy t;
        opt.emplace (t);
    }
    catch (const std::bad_alloc&)
    {
        threw = true;
    }

    EXPECT_TRUE (threw);
    EXPECT_TRUE (! opt.hasValue());
}

TEST (OptionalTests, SwapDoesNothingToTwoEmptyOptionals)
{
    using Ptr = std::shared_ptr<int>;

    Optional<Ptr> a, b;
    EXPECT_TRUE (! a.hasValue());
    EXPECT_TRUE (! b.hasValue());

    a.swap (b);

    EXPECT_TRUE (! a.hasValue());
    EXPECT_TRUE (! b.hasValue());
}

TEST (OptionalTests, SwapTransfersOwnershipIfOneOptionalContainsAValue)
{
    using Ptr = std::shared_ptr<int>;
    auto makePtr = [] { return std::make_shared<int>(); };

    {
        Ptr ptr = makePtr();
        Optional<Ptr> a, b = ptr;
        EXPECT_FALSE(a.hasValue());
        EXPECT_TRUE(b.hasValue());
        EXPECT_EQ(ptr.use_count(), 2);

        a.swap(b);

        EXPECT_TRUE(a.hasValue());
        EXPECT_FALSE(b.hasValue());
        EXPECT_EQ(ptr.use_count(), 2);
    }

    {
        auto ptr = makePtr();
        Optional<Ptr> a = ptr, b;
        EXPECT_TRUE (a.hasValue());
        EXPECT_FALSE (b.hasValue());
        EXPECT_EQ (ptr.use_count(), 2);

        a.swap (b);

        EXPECT_FALSE (a.hasValue());
        EXPECT_TRUE (b.hasValue());
        EXPECT_EQ (ptr.use_count(), 2);
    }
}

TEST (OptionalTests, SwapCallsStdSwapToSwapTwoPopulatedOptionals)
{
    using Ptr = std::shared_ptr<int>;
    auto makePtr = [] { return std::make_shared<int>(); };

    auto x = makePtr();
    auto y = makePtr();

    Optional<Ptr> a = x;
    Optional<Ptr> b = y;

    EXPECT_TRUE(a.hasValue());
    EXPECT_TRUE(b.hasValue());
    EXPECT_EQ(x.use_count(), 2);
    EXPECT_EQ(y.use_count(), 2);

    a.swap(b);

    EXPECT_TRUE(a.hasValue());
    EXPECT_TRUE(b.hasValue());
    EXPECT_EQ(x.use_count(), 2);
    EXPECT_EQ(y.use_count(), 2);
    EXPECT_EQ(*a, y);
    EXPECT_EQ(*b, x);
}

TEST (OptionalTests, ExceptionThrownDuringSwapLeavesObjectsIntact)
{
    {
        Optional<ThrowOnMoveOrSwap> a, b;
        a.emplace();

        EXPECT_TRUE (a.hasValue());
        EXPECT_TRUE (! b.hasValue());

        bool threw = false;

        try
        {
            a.swap (b);
        }
        catch (const std::bad_alloc&)
        {
            threw = true;
        }

        EXPECT_TRUE (threw);
        EXPECT_TRUE (a.hasValue());
        EXPECT_TRUE (! b.hasValue());
    }

    {
        Optional<ThrowOnMoveOrSwap> a, b;
        b.emplace();

        EXPECT_TRUE (! a.hasValue());
        EXPECT_TRUE (b.hasValue());

        bool threw = false;

        try
        {
            a.swap (b);
        }
        catch (const std::bad_alloc&)
        {
            threw = true;
        }

        EXPECT_TRUE (threw);
        EXPECT_TRUE (! a.hasValue());
        EXPECT_TRUE (b.hasValue());
    }

    {
        Optional<ThrowOnMoveOrSwap> a, b;
        a.emplace();
        b.emplace();

        EXPECT_TRUE (a.hasValue());
        EXPECT_TRUE (b.hasValue());

        bool threw = false;

        try
        {
            a.swap (b);
        }
        catch (const std::bad_alloc&)
        {
            threw = true;
        }

        EXPECT_TRUE (threw);
        EXPECT_TRUE (a.hasValue());
        EXPECT_TRUE (b.hasValue());
    }
}

TEST (OptionalTests, RelationalTests)
{
    EXPECT_TRUE (Optional<int> (1) == Optional<int> (1));
    EXPECT_TRUE (Optional<int>() == Optional<int>());
    EXPECT_TRUE (! (Optional<int> (1) == Optional<int>()));
    EXPECT_TRUE (! (Optional<int>() == Optional<int> (1)));
    EXPECT_TRUE (! (Optional<int> (1) == Optional<int> (2)));

    EXPECT_TRUE (Optional<int> (1) != Optional<int> (2));
    EXPECT_TRUE (! (Optional<int>() != Optional<int>()));
    EXPECT_TRUE (Optional<int> (1) != Optional<int>());
    EXPECT_TRUE (Optional<int>() != Optional<int> (1));
    EXPECT_TRUE (! (Optional<int> (1) != Optional<int> (1)));

    EXPECT_TRUE (Optional<int>() < Optional<int> (1));
    EXPECT_TRUE (! (Optional<int> (1) < Optional<int>()));
    EXPECT_TRUE (! (Optional<int>() < Optional<int>()));
    EXPECT_TRUE (Optional<int> (1) < Optional<int> (2));

    EXPECT_TRUE (Optional<int>() <= Optional<int> (1));
    EXPECT_TRUE (! (Optional<int> (1) <= Optional<int>()));
    EXPECT_TRUE (Optional<int>() <= Optional<int>());
    EXPECT_TRUE (Optional<int> (1) <= Optional<int> (2));

    EXPECT_TRUE (! (Optional<int>() > Optional<int> (1)));
    EXPECT_TRUE (Optional<int> (1) > Optional<int>());
    EXPECT_TRUE (! (Optional<int>() > Optional<int>()));
    EXPECT_TRUE (! (Optional<int> (1) > Optional<int> (2)));

    EXPECT_TRUE (! (Optional<int>() >= Optional<int> (1)));
    EXPECT_TRUE (Optional<int> (1) >= Optional<int>());
    EXPECT_TRUE (Optional<int>() >= Optional<int>());
    EXPECT_TRUE (! (Optional<int> (1) >= Optional<int> (2)));

    EXPECT_TRUE (Optional<int>() == nullopt);
    EXPECT_TRUE (! (Optional<int> (1) == nullopt));
    EXPECT_TRUE (nullopt == Optional<int>());
    EXPECT_TRUE (! (nullopt == Optional<int> (1)));

    EXPECT_TRUE (! (Optional<int>() != nullopt));
    EXPECT_TRUE (Optional<int> (1) != nullopt);
    EXPECT_TRUE (! (nullopt != Optional<int>()));
    EXPECT_TRUE (nullopt != Optional<int> (1));

    EXPECT_TRUE (! (Optional<int>() < nullopt));
    EXPECT_TRUE (! (Optional<int> (1) < nullopt));

    EXPECT_TRUE (! (nullopt < Optional<int>()));
    EXPECT_TRUE (nullopt < Optional<int> (1));

    EXPECT_TRUE (Optional<int>() <= nullopt);
    EXPECT_TRUE (! (Optional<int> (1) <= nullopt));

    EXPECT_TRUE (nullopt <= Optional<int>());
    EXPECT_TRUE (nullopt <= Optional<int> (1));

    EXPECT_TRUE (! (Optional<int>() > nullopt));
    EXPECT_TRUE (Optional<int> (1) > nullopt);

    EXPECT_TRUE (! (nullopt > Optional<int>()));
    EXPECT_TRUE (! (nullopt > Optional<int> (1)));

    EXPECT_TRUE (Optional<int>() >= nullopt);
    EXPECT_TRUE (Optional<int> (1) >= nullopt);

    EXPECT_TRUE (nullopt >= Optional<int>());
    EXPECT_TRUE (! (nullopt >= Optional<int> (1)));

    EXPECT_TRUE (! (Optional<int>() == 5));
    EXPECT_TRUE (! (Optional<int> (1) == 5));
    EXPECT_TRUE (Optional<int> (1) == 1);
    EXPECT_TRUE (! (5 == Optional<int>()));
    EXPECT_TRUE (! (5 == Optional<int> (1)));
    EXPECT_TRUE (1 == Optional<int> (1));

    EXPECT_TRUE (Optional<int>() != 5);
    EXPECT_TRUE (Optional<int> (1) != 5);
    EXPECT_TRUE (! (Optional<int> (1) != 1));
    EXPECT_TRUE (5 != Optional<int>());
    EXPECT_TRUE (5 != Optional<int> (1));
    EXPECT_TRUE (! (1 != Optional<int> (1)));

    EXPECT_TRUE (Optional<int>() < 5);
    EXPECT_TRUE (Optional<int> (1) < 5);
    EXPECT_TRUE (! (Optional<int> (1) < 1));
    EXPECT_TRUE (! (Optional<int> (1) < 0));

    EXPECT_TRUE (! (5 < Optional<int>()));
    EXPECT_TRUE (! (5 < Optional<int> (1)));
    EXPECT_TRUE (! (1 < Optional<int> (1)));
    EXPECT_TRUE (0 < Optional<int> (1));

    EXPECT_TRUE (Optional<int>() <= 5);
    EXPECT_TRUE (Optional<int> (1) <= 5);
    EXPECT_TRUE (Optional<int> (1) <= 1);
    EXPECT_TRUE (! (Optional<int> (1) <= 0));

    EXPECT_TRUE (! (5 <= Optional<int>()));
    EXPECT_TRUE (! (5 <= Optional<int> (1)));
    EXPECT_TRUE (1 <= Optional<int> (1));
    EXPECT_TRUE (0 <= Optional<int> (1));

    EXPECT_TRUE (! (Optional<int>() > 5));
    EXPECT_TRUE (! (Optional<int> (1) > 5));
    EXPECT_TRUE (! (Optional<int> (1) > 1));
    EXPECT_TRUE (Optional<int> (1) > 0);

    EXPECT_TRUE (5 > Optional<int>());
    EXPECT_TRUE (5 > Optional<int> (1));
    EXPECT_TRUE (! (1 > Optional<int> (1)));
    EXPECT_TRUE (! (0 > Optional<int> (1)));

    EXPECT_TRUE (! (Optional<int>() >= 5));
    EXPECT_TRUE (! (Optional<int> (1) >= 5));
    EXPECT_TRUE (Optional<int> (1) >= 1);
    EXPECT_TRUE (Optional<int> (1) >= 0);

    EXPECT_TRUE (5 >= Optional<int>());
    EXPECT_TRUE (5 >= Optional<int> (1));
    EXPECT_TRUE (1 >= Optional<int> (1));
    EXPECT_TRUE (! (0 >= Optional<int> (1)));
}
