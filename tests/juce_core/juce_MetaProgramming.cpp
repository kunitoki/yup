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

#include <string>

using namespace yup;

namespace
{

struct HaveIt
{
    bool existingMethod (int, float);
    bool existingMethod2 (int, float, int);

    std::string field;
};

struct DontHaveIt
{
    uint32_t field;
};

template <class T>
using hasExistingMethod = decltype (&T::existingMethod);

template <class T>
using hasExistingMethod2 = decltype (&T::existingMethod2);

template <class T>
using hasField = decltype (T::field);

} // namespace

TEST (MetaProgrammingTests, DependentBoolValue)
{
    static_assert (dependentBoolValue<true>);
    static_assert (! dependentBoolValue<false>);
    static_assert (! dependentFalse<>);
}

TEST (MetaProgrammingTests, IsDetected)
{
    static_assert (isDetected<hasExistingMethod, HaveIt>);
    static_assert (! isDetected<hasExistingMethod, DontHaveIt>);
}

TEST (MetaProgrammingTests, IsDetectedExact)
{
    static_assert (isDetectedExact<decltype (&HaveIt::existingMethod), hasExistingMethod, HaveIt>);
    static_assert (! isDetectedExact<decltype (&HaveIt::existingMethod), hasExistingMethod2, HaveIt>);
    static_assert (! isDetectedExact<decltype (&HaveIt::existingMethod), hasExistingMethod, DontHaveIt>);
}

TEST (MetaProgrammingTests, IsDetectedConvertible)
{
    static_assert (isDetectedConvertible<std::string_view, hasField, HaveIt>);
    static_assert (! isDetectedConvertible<std::size_t, hasField, HaveIt>);
    static_assert (! isDetectedConvertible<std::string_view, hasField, DontHaveIt>);
    static_assert (isDetectedConvertible<std::size_t, hasField, DontHaveIt>);
}

TEST (MetaProgrammingTests, DetectedType)
{
    static_assert (std::is_same_v<detectedType<hasExistingMethod, HaveIt>, decltype (&HaveIt::existingMethod)>);
    static_assert (std::is_same_v<detectedType<hasExistingMethod, DontHaveIt>, yup::NoneSuch>);
}

TEST (MetaProgrammingTests, DetectedOr)
{
    static_assert (std::is_same_v<detectedOr<int, hasExistingMethod, HaveIt>, decltype (&HaveIt::existingMethod)>);
    static_assert (std::is_same_v<detectedOr<int, hasExistingMethod, DontHaveIt>, int>);
}
