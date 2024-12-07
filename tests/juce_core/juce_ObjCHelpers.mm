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

#import <Foundation/Foundation.h>

#include <objc/message.h>

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

#include <juce_core/native/juce_ObjCHelpers_mac.h>

using namespace juce;

TEST (ObjCHelpers, Range)
{
    constexpr auto start = 10;
    constexpr auto length = 20;

    const auto juceRange = Range<int>::withStartAndLength(start, length);
    const auto nsRange = NSMakeRange(start, length);

    EXPECT_TRUE (nsRangeToJuce(nsRange) == juceRange);
    EXPECT_TRUE (NSEqualRanges(nsRange, juceRangeToNS(juceRange)));
}

TEST (ObjCHelpers, String)
{
    String juceString{"Hello world!"};
    NSString* nsString{@"Hello world!"};

    EXPECT_TRUE (nsStringToJuce(nsString) == juceString);
    EXPECT_TRUE ([nsString isEqualToString:juceStringToNS(juceString)]);
    EXPECT_TRUE ([nsString isEqualToString:nsStringLiteral("Hello world!")]);
}

TEST (ObjCHelpers, StringArray)
{
    const StringArray stringArray{"Hello world!", "this", "is", "a", "test"};
    NSArray* nsArray
    {
        @[ @"Hello world!", @"this", @"is", @"a", @"test" ]
    };

    EXPECT_TRUE ([nsArray isEqualToArray:createNSArrayFromStringArray(stringArray)]);
}

TEST (ObjCHelpers, Dictionary)
{
    DynamicObject::Ptr data{new DynamicObject()};
    data->setProperty("integer", 1);
    data->setProperty("double", 2.3);
    data->setProperty("boolean", true);
    data->setProperty("string", "Hello world!");

    Array<var> array{45, 67.8, true, "Hello array!"};
    data->setProperty("array", array);

    const auto* nsDictionary = varToNSDictionary(data.get());
    EXPECT_TRUE (nsDictionary != nullptr);

    const auto clone = nsDictionaryToVar(nsDictionary);
    EXPECT_TRUE (clone.isObject());

    EXPECT_TRUE (clone.getProperty("integer", {}).isInt());
    EXPECT_TRUE (clone.getProperty("double", {}).isDouble());
    EXPECT_TRUE (clone.getProperty("boolean", {}).isBool());
    EXPECT_TRUE (clone.getProperty("string", {}).isString());
    EXPECT_TRUE (clone.getProperty("array", {}).isArray());

    EXPECT_TRUE (clone.getProperty("integer", {}) == var{1});
    EXPECT_TRUE (clone.getProperty("double", {}) == var{2.3});
    EXPECT_TRUE (clone.getProperty("boolean", {}) == var{true});
    EXPECT_TRUE (clone.getProperty("string", {}) == var{"Hello world!"});
    EXPECT_TRUE (clone.getProperty("array", {}) == var{array});
}

TEST (ObjCHelpers, VarToNSDictionaryConvertVoidVariantToEmptyDictionary)
{
    var voidVariant;

    const auto* nsDictionary = varToNSDictionary(voidVariant);
    EXPECT_TRUE (nsDictionary != nullptr);

    const auto result = nsDictionaryToVar(nsDictionary);
    EXPECT_TRUE (result.isObject());
    EXPECT_TRUE (result.getDynamicObject()->getProperties().isEmpty());
}
