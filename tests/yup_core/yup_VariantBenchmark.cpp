/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#include <chrono>
#include <iostream>

using namespace yup;

//==============================================================================
/** Measures the hot operations of var.

    Disabled by default, run it in Release with:
    GTEST_ALSO_RUN_DISABLED_TESTS=1 just test Release "*VariantBenchmark*" yup_core
*/
class VariantBenchmarkTests : public ::testing::Test
{
protected:
    static constexpr int numIterations = 2000000;
    static constexpr int numRuns = 7;

    template <class F>
    static double measureSeconds (F&& run)
    {
        auto best = std::numeric_limits<double>::max();

        for (int i = 0; i < numRuns; ++i)
        {
            const auto start = std::chrono::steady_clock::now();
            run();
            best = jmin (best, std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count());
        }

        return best;
    }

    static void report (const String& name, double seconds, double divisor)
    {
        std::cout << "  " << name.paddedRight (' ', 32)
                  << String (seconds * 1.0e9 / divisor, 2).paddedLeft (' ', 12) << " ns/op\n";
    }

    template <class F>
    static void bench (const String& name, F&& body)
    {
        report (name, measureSeconds ([&]
        {
            for (int i = 0; i < numIterations; ++i)
                body (i);
        }),
                numIterations);
    }

    static Array<var> makeSamples()
    {
        auto object = std::make_unique<DynamicObject>();
        object->setProperty ("name", "yup");

        MemoryBlock block;
        block.append ("binary", 6);

        return {
            var(),
            var::undefined(),
            var (42),
            var ((int64) 1234567890123),
            var (true),
            var (3.25),
            var ("a short string"),
            var (std::move (object)),
            var (Array<var> { 1, 2, 3 }),
            var (std::move (block)),
            var (var::NativeFunction ([] (const var::NativeFunctionArgs&)
            {
                return var();
            }))
        };
    }

    static inline volatile int64 sink = 0;
};

//==============================================================================
TEST_F (VariantBenchmarkTests, DISABLED_HotOperations)
{
    const auto samples = makeSamples();
    const StringArray names { "void", "undefined", "int", "int64", "bool", "double", "string", "object", "array", "binary", "method" };

    std::cout << "\n  |==== BENCH ==== (var, " << numIterations << " iterations, best of " << numRuns << ")\n";

    for (int t = 0; t < samples.size(); ++t)
    {
        const auto& source = samples.getReference (t);

        bench ("copy+destroy " + names[t], [&] (int)
        {
            var copy (source);
            sink = sink + (copy.isVoid() ? 0 : 1);
        });

        var target;
        bench ("copy assign " + names[t], [&] (int i)
        {
            target = (i & 1) != 0 ? source : samples.getReference (2);
        });
    }

    bench ("type predicates", [&] (int i)
    {
        const auto& v = samples.getReference (i % samples.size());
        sink = sink + (v.isInt() ? 1 : 0) + (v.isString() ? 2 : 0) + (v.isArray() ? 4 : 0) + (v.isObject() ? 8 : 0) + (v.isVoid() ? 16 : 0);
    });

    bench ("hasSameTypeAs", [&] (int i)
    {
        sink = sink + (samples.getReference (i % samples.size()).hasSameTypeAs (samples.getReference ((i + 1) % samples.size())) ? 1 : 0);
    });

    const var intValue (42), doubleValue (3.25), stringValue ("123");
    bench ("int conversion", [&] (int)
    {
        sink = sink + (int) intValue;
    });

    bench ("double conversion", [&] (int)
    {
        sink = sink + (int64) (double) doubleValue;
    });

    bench ("string to int conversion", [&] (int)
    {
        sink = sink + (int) stringValue;
    });

    const var intA (7), intB (7), int64A ((int64) 7), doubleA (7.0), stringA ("seven"), stringB ("seven");
    bench ("equals int-int", [&] (int)
    {
        sink = sink + (intA == intB ? 1 : 0);
    });

    bench ("equals int-int64", [&] (int)
    {
        sink = sink + (intA == int64A ? 1 : 0);
    });

    bench ("equals int-double", [&] (int)
    {
        sink = sink + (intA == doubleA ? 1 : 0);
    });

    bench ("equals string-string", [&] (int)
    {
        sink = sink + (stringA == stringB ? 1 : 0);
    });

    bench ("relational int-double", [&] (int)
    {
        sink = sink + (intA < doubleA ? 1 : 0);
    });

    Array<var> elements;
    for (int i = 0; i < 64; ++i)
        elements.add (i);

    const var array (elements);
    bench ("array element access", [&] (int i)
    {
        sink = sink + (int) array[i & 63];
    });

    bench ("array size", [&] (int)
    {
        sink = sink + array.size();
    });

    auto object = std::make_unique<DynamicObject>();
    for (int i = 0; i < 8; ++i)
        object->setProperty (Identifier ("property" + String (i)), i);

    const var objectValue (std::move (object));
    const Identifier lookup ("property5");
    bench ("getDynamicObject", [&] (int)
    {
        sink = sink + (objectValue.getDynamicObject() != nullptr ? 1 : 0);
    });

    bench ("property lookup", [&] (int)
    {
        sink = sink + (int) objectValue[lookup];
    });

    const auto json = JSON::toString (var (elements), true);
    report ("json round-trip", measureSeconds ([&]
    {
        for (int i = 0; i < numIterations / 1000; ++i)
            sink = sink + JSON::toString (JSON::parse (json), true).length();
    }),
            numIterations / 1000);

    EXPECT_NE (sink, 0);
}
