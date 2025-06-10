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

#include <algorithm>
#include <type_traits>

using namespace yup;

namespace
{
namespace ArrayBaseTestsHelpers
{
class TriviallyCopyableType
{
public:
    TriviallyCopyableType() = default;

    TriviallyCopyableType (int v)
        : value (v)
    {
    }

    TriviallyCopyableType (float v)
        : value ((int) v)
    {
    }

    bool operator== (const TriviallyCopyableType& other) const
    {
        return getValue() == other.getValue();
    }

    int getValue() const { return value; }

private:
    int value { -1111 };
};

class NonTriviallyCopyableType
{
public:
    NonTriviallyCopyableType() = default;

    NonTriviallyCopyableType (int v)
        : value (v)
    {
    }

    NonTriviallyCopyableType (float v)
        : value ((int) v)
    {
    }

    NonTriviallyCopyableType (const NonTriviallyCopyableType& other)
        : value (other.value)
    {
    }

    NonTriviallyCopyableType& operator= (const NonTriviallyCopyableType& other)
    {
        value = other.value;
        return *this;
    }

    bool operator== (const NonTriviallyCopyableType& other) const
    {
        return getValue() == other.getValue();
    }

    int getValue() const { return *ptr; }

private:
    int value { -1111 };
    int* ptr = &value;
};

bool operator== (const TriviallyCopyableType& tct, const NonTriviallyCopyableType& ntct)
{
    return tct.getValue() == ntct.getValue();
}

bool operator== (const NonTriviallyCopyableType& ntct, const TriviallyCopyableType& tct)
{
    return tct == ntct;
}
} // namespace ArrayBaseTestsHelpers

using CopyableType = ArrayBaseTestsHelpers::TriviallyCopyableType;
using NoncopyableType = ArrayBaseTestsHelpers::NonTriviallyCopyableType;

template <class A, class B>
void checkEqual (const ArrayBase<A, DummyCriticalSection>& a,
                 const ArrayBase<B, DummyCriticalSection>& b)
{
    EXPECT_EQ (a.size(), b.size());
    for (int i = 0; i < a.size(); ++i)
        EXPECT_EQ (a[i], b[i]);
}

template <class A, class B>
void checkEqual (const ArrayBase<A, DummyCriticalSection>& a,
                 const std::vector<B>& b)
{
    EXPECT_EQ (a.size(), b.size());
    for (int i = 0; i < a.size(); ++i)
        EXPECT_EQ (a[i], b[size_t (i)]);
}

template <class A, class B, class C>
void checkEqual (const ArrayBase<A, DummyCriticalSection>& a,
                 const ArrayBase<B, DummyCriticalSection>& b,
                 const std::vector<C>& c)
{
    checkEqual (a, b);
    checkEqual (a, c);
    checkEqual (b, c);
}

void addData (std::vector<CopyableType>& referenceContainer,
              ArrayBase<CopyableType, DummyCriticalSection>& copyableContainer,
              ArrayBase<NoncopyableType, DummyCriticalSection>& noncopyableContainer,
              int numValues)
{
    for (int i = 0; i < numValues; ++i)
    {
        referenceContainer.emplace_back (i);
        copyableContainer.add ({ i });
        noncopyableContainer.add ({ i });
    }
}
} // namespace

TEST (ArrayBaseTests, GrowCapacity)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    int originalCapacity = 4;
    referenceContainer.reserve ((size_t) originalCapacity);
    EXPECT_EQ (referenceContainer.capacity(), originalCapacity);

    copyableContainer.setAllocatedSize (originalCapacity);
    EXPECT_EQ (copyableContainer.capacity(), originalCapacity);

    noncopyableContainer.setAllocatedSize (originalCapacity);
    EXPECT_EQ (noncopyableContainer.capacity(), originalCapacity);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    addData (referenceContainer, copyableContainer, noncopyableContainer, 33);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    EXPECT_NE (referenceContainer.capacity(), originalCapacity);
    EXPECT_NE (copyableContainer.capacity(), originalCapacity);
    EXPECT_NE (noncopyableContainer.capacity(), originalCapacity);
}

TEST (ArrayBaseTests, ShrinkCapacity)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    int numElements = 45;
    addData (referenceContainer, copyableContainer, noncopyableContainer, numElements);

    copyableContainer.shrinkToNoMoreThan (numElements);
    noncopyableContainer.setAllocatedSize (numElements + 1);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    referenceContainer.clear();
    copyableContainer.removeElements (0, numElements);
    noncopyableContainer.removeElements (0, numElements);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    copyableContainer.setAllocatedSize (0);
    noncopyableContainer.setAllocatedSize (0);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    addData (referenceContainer, copyableContainer, noncopyableContainer, numElements);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);
}

TEST (ArrayBaseTests, Equality)
{
    std::vector<int> referenceContainer = { 1, 2, 3 };
    ArrayBase<int, DummyCriticalSection> testContainer1, testContainer2;

    for (auto i : referenceContainer)
    {
        testContainer1.add (i);
        testContainer2.add (i);
    }

    EXPECT_EQ (testContainer1, referenceContainer);
    EXPECT_EQ (testContainer2, testContainer1);

    testContainer1.ensureAllocatedSize (257);
    referenceContainer.shrink_to_fit();

    EXPECT_EQ (testContainer1, referenceContainer);
    EXPECT_EQ (testContainer2, testContainer1);

    testContainer1.removeElements (0, 1);

    EXPECT_NE (testContainer1, referenceContainer);
    EXPECT_NE (testContainer2, testContainer1);
}

TEST (ArrayBaseTests, Accessors)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    addData (referenceContainer, copyableContainer, noncopyableContainer, 3);

    int testValue = -123;
    referenceContainer[0] = testValue;
    copyableContainer[0] = testValue;
    noncopyableContainer[0] = testValue;

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    EXPECT_EQ (copyableContainer.getFirst().getValue(), testValue);
    EXPECT_EQ (noncopyableContainer.getFirst().getValue(), testValue);

    auto last = referenceContainer.back().getValue();
    EXPECT_EQ (copyableContainer.getLast().getValue(), last);
    EXPECT_EQ (noncopyableContainer.getLast().getValue(), last);

    ArrayBase<CopyableType, DummyCriticalSection> copyableEmpty;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableEmpty;

    auto defaultValue = CopyableType().getValue();
    EXPECT_EQ (defaultValue, NoncopyableType().getValue());

    EXPECT_EQ (copyableEmpty.getFirst().getValue(), defaultValue);
    EXPECT_EQ (noncopyableEmpty.getFirst().getValue(), defaultValue);
    EXPECT_EQ (copyableEmpty.getLast().getValue(), defaultValue);
    EXPECT_EQ (noncopyableEmpty.getLast().getValue(), defaultValue);

    ArrayBase<float*, DummyCriticalSection> floatPointers;
    EXPECT_EQ (floatPointers.getValueWithDefault (-3), nullptr);
}

TEST (ArrayBaseTests, AddMoved)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    for (int i = 0; i < 5; ++i)
    {
        CopyableType ref (-i);
        CopyableType ct (-i);
        NoncopyableType nct (-i);
        referenceContainer.push_back (std::move (ref));
        copyableContainer.add (std::move (ct));
        noncopyableContainer.add (std::move (nct));
    }

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);
}

TEST (ArrayBaseTests, AddMultiple)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    for (int i = 4; i < 7; ++i)
        referenceContainer.push_back ({ -i });

    copyableContainer.add (CopyableType (-4), CopyableType (-5), CopyableType (-6));
    noncopyableContainer.add (NoncopyableType (-4), NoncopyableType (-5), NoncopyableType (-6));

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);
}

TEST (ArrayBaseTests, AddArrayFromPointer)
{
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    std::vector<CopyableType> copyableData = { 3, 4, 5 };
    std::vector<NoncopyableType> noncopyableData = { 3, 4, 5 };

    copyableContainer.addArray (copyableData.data(), (int) copyableData.size());
    noncopyableContainer.addArray (noncopyableData.data(), (int) noncopyableData.size());

    checkEqual (copyableContainer, noncopyableContainer, copyableData);
}

TEST (ArrayBaseTests, AddArrayFromPointerOfDifferentType)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    std::vector<float> floatData = { 1.4f, 2.5f, 3.6f };

    for (auto f : floatData)
        referenceContainer.push_back ({ f });

    copyableContainer.addArray (floatData.data(), (int) floatData.size());
    noncopyableContainer.addArray (floatData.data(), (int) floatData.size());

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);
}

TEST (ArrayBaseTests, AddArrayFromInitializerList)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    std::initializer_list<CopyableType> ilct { { 3 }, { 4 }, { 5 } };
    std::initializer_list<NoncopyableType> ilnct { { 3 }, { 4 }, { 5 } };

    for (auto v : ilct)
        referenceContainer.push_back (v);

    copyableContainer.addArray (ilct);
    noncopyableContainer.addArray (ilnct);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);
}

TEST (ArrayBaseTests, AddArrayFromContainers)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    addData (referenceContainer, copyableContainer, noncopyableContainer, 5);

    std::vector<CopyableType> referenceContainerCopy (referenceContainer);
    std::vector<NoncopyableType> noncopyableReferenceContainerCopy;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainerCopy;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainerCopy;

    for (auto& v : referenceContainerCopy)
        noncopyableReferenceContainerCopy.push_back ({ v.getValue() });

    for (size_t i = 0; i < referenceContainerCopy.size(); ++i)
    {
        auto value = referenceContainerCopy[i].getValue();
        copyableContainerCopy.add ({ value });
        noncopyableContainerCopy.add ({ value });
    }

    // From self-types
    copyableContainer.addArray (copyableContainerCopy);
    noncopyableContainer.addArray (noncopyableContainerCopy);

    for (auto v : referenceContainerCopy)
        referenceContainer.push_back (v);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    // From std containers
    copyableContainer.addArray (referenceContainerCopy);
    noncopyableContainer.addArray (noncopyableReferenceContainerCopy);

    for (auto v : referenceContainerCopy)
        referenceContainer.push_back (v);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    // From std containers with offset
    int offset = 5;
    copyableContainer.addArray (referenceContainerCopy, offset);
    noncopyableContainer.addArray (noncopyableReferenceContainerCopy, offset);

    for (size_t i = 5; i < referenceContainerCopy.size(); ++i)
        referenceContainer.push_back (referenceContainerCopy[i]);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);
}

TEST (ArrayBaseTests, Insert)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    addData (referenceContainer, copyableContainer, noncopyableContainer, 8);

    referenceContainer.insert (referenceContainer.begin(), -4);
    copyableContainer.insert (0, -4, 1);
    noncopyableContainer.insert (0, -4, 1);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    for (int i = 0; i < 3; ++i)
        referenceContainer.insert (referenceContainer.begin() + 1, -3);

    copyableContainer.insert (1, -3, 3);
    noncopyableContainer.insert (1, -3, 3);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    for (int i = 0; i < 50; ++i)
        referenceContainer.insert (referenceContainer.end() - 1, -9);

    copyableContainer.insert (copyableContainer.size() - 2, -9, 50);
    noncopyableContainer.insert (noncopyableContainer.size() - 2, -9, 50);

    //checkEqual(copyableContainer, noncopyableContainer, referenceContainer);
}

TEST (ArrayBaseTests, InsertArray)
{
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    std::vector<CopyableType> copyableData = { 3, 4, 5, 6, 7, 8 };
    std::vector<NoncopyableType> noncopyableData = { 3, 4, 5, 6, 7, 8 };

    std::vector<CopyableType> referenceContainer { copyableData };

    copyableContainer.insertArray (0, copyableData.data(), (int) copyableData.size());
    noncopyableContainer.insertArray (0, noncopyableData.data(), (int) noncopyableData.size());

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    int insertPos = copyableContainer.size() - 1;

    for (auto it = copyableData.end(); it != copyableData.begin(); --it)
        referenceContainer.insert (referenceContainer.begin() + insertPos, CopyableType (*(it - 1)));

    copyableContainer.insertArray (insertPos, copyableData.data(), (int) copyableData.size());
    noncopyableContainer.insertArray (insertPos, noncopyableData.data(), (int) noncopyableData.size());

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);
}

TEST (ArrayBaseTests, Remove)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    addData (referenceContainer, copyableContainer, noncopyableContainer, 17);

    for (int i = 0; i < 4; ++i)
    {
        referenceContainer.erase (referenceContainer.begin() + i);
        copyableContainer.removeElements (i, 1);
        noncopyableContainer.removeElements (i, 1);
    }

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    addData (referenceContainer, copyableContainer, noncopyableContainer, 17);
    int blockSize = 3;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < blockSize; ++j)
            referenceContainer.erase (referenceContainer.begin() + i);

        copyableContainer.removeElements (i, blockSize);
        noncopyableContainer.removeElements (i, blockSize);
    }

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);

    auto numToRemove = copyableContainer.size() - 2;

    for (int i = 0; i < numToRemove; ++i)
        referenceContainer.erase (referenceContainer.begin() + 1);

    copyableContainer.removeElements (1, numToRemove);
    noncopyableContainer.removeElements (1, numToRemove);

    checkEqual (copyableContainer, noncopyableContainer, referenceContainer);
}

TEST (ArrayBaseTests, Move)
{
    std::vector<CopyableType> referenceContainer;
    ArrayBase<CopyableType, DummyCriticalSection> copyableContainer;
    ArrayBase<NoncopyableType, DummyCriticalSection> noncopyableContainer;

    addData (referenceContainer, copyableContainer, noncopyableContainer, 6);

    std::vector<std::pair<int, int>> testValues = { { 2, 4 }, { 0, 5 }, { 4, 1 }, { 5, 0 } };

    for (auto p : testValues)
    {
        if (p.second > p.first)
            std::rotate (referenceContainer.begin() + p.first,
                         referenceContainer.begin() + p.first + 1,
                         referenceContainer.begin() + p.second + 1);
        else
            std::rotate (referenceContainer.begin() + p.second,
                         referenceContainer.begin() + p.first,
                         referenceContainer.begin() + p.first + 1);

        copyableContainer.move (p.first, p.second);
        noncopyableContainer.move (p.first, p.second);

        checkEqual (copyableContainer, noncopyableContainer, referenceContainer);
    }
}

TEST (ArrayBaseTests, MoveConstructionTransfersOwnership)
{
    struct Base
    {
        virtual ~Base() = default;
    };

    struct Derived : public Base
    {
    };

    Derived obj;
    ArrayBase<Derived*, DummyCriticalSection> derived;
    derived.setAllocatedSize (5);
    derived.add (&obj);

    ArrayBase<Base*, DummyCriticalSection> base { std::move (derived) };

    EXPECT_EQ (base.capacity(), 5);
    EXPECT_EQ (base.size(), 1);
    EXPECT_EQ (base.getFirst(), &obj);
    EXPECT_EQ (derived.capacity(), 0);
    EXPECT_EQ (derived.size(), 0);
    EXPECT_EQ (derived.data(), nullptr);
}

TEST (ArrayBaseTests, MoveAssignmentTransfersOwnership)
{
    struct Base
    {
        virtual ~Base() = default;
    };

    struct Derived : public Base
    {
    };

    Derived obj;
    ArrayBase<Derived*, DummyCriticalSection> derived;
    derived.setAllocatedSize (5);
    derived.add (&obj);

    ArrayBase<Base*, DummyCriticalSection> base;
    base = std::move (derived);

    EXPECT_EQ (base.capacity(), 5);
    EXPECT_EQ (base.size(), 1);
    EXPECT_EQ (base.getFirst(), &obj);
    EXPECT_EQ (derived.capacity(), 0);
    EXPECT_EQ (derived.size(), 0);
    EXPECT_EQ (derived.data(), nullptr);
}
