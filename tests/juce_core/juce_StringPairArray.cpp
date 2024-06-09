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

static String operator"" _S(const char* chars, size_t)
{
    return String { chars };
}

class StringPairArrayTests : public ::testing::Test
{
protected:
    void addDefaultPairs(StringPairArray& spa)
    {
        spa.set("key1", "value1");
        spa.set("key2", "value2");
        spa.set("key3", "value3");
    }
};

TEST_F (StringPairArrayTests, EmptyOnInitialization)
{
    StringPairArray spa;
    EXPECT_EQ(spa.size(), 0);
    EXPECT_TRUE(spa.getIgnoresCase());  // Default should ignore case
}

TEST_F (StringPairArrayTests, ParameterizedConstructorCaseSensitivity)
{
    StringPairArray caseSensitive(false);
    EXPECT_FALSE(caseSensitive.getIgnoresCase());

    StringPairArray caseInsensitive(true);
    EXPECT_TRUE(caseInsensitive.getIgnoresCase());
}

TEST_F (StringPairArrayTests, CopyConstructor)
{
    StringPairArray original;
    addDefaultPairs(original);
    StringPairArray copy(original);

    EXPECT_EQ(copy.size(), 3);
    EXPECT_EQ(copy["key1"], "value1");
    EXPECT_EQ(copy["key2"], "value2");
    EXPECT_EQ(copy["key3"], "value3");
    EXPECT_TRUE(original == copy);  // Ensure the copy is identical
}

TEST_F (StringPairArrayTests, MoveConstructor)
{
    StringPairArray original;
    addDefaultPairs(original);

    StringPairArray moved(std::move(original));

    EXPECT_EQ(moved.size(), 3);
    EXPECT_EQ(moved["key1"], "value1");
    EXPECT_EQ(moved["key2"], "value2");
    EXPECT_EQ(moved["key3"], "value3");
}

TEST_F (StringPairArrayTests, CopyAssignmentOperator)
{
    StringPairArray original;
    addDefaultPairs(original);
    StringPairArray copy;
    copy = original;

    EXPECT_EQ(copy.size(), 3);
    EXPECT_EQ(copy["key1"], "value1");
    EXPECT_EQ(copy["key2"], "value2");
    EXPECT_EQ(copy["key3"], "value3");
    EXPECT_TRUE(original == copy);
}

TEST_F (StringPairArrayTests, MoveAssignmentOperator)
{
    StringPairArray original;
    addDefaultPairs(original);
    StringPairArray moved;
    moved = std::move(original);

    EXPECT_EQ(moved.size(), 3);
    EXPECT_EQ(moved["key1"], "value1");
    EXPECT_EQ(moved["key2"], "value2");
    EXPECT_EQ(moved["key3"], "value3");
}

TEST_F (StringPairArrayTests, SetAndGetValues)
{
    StringPairArray spa;
    addDefaultPairs(spa);
    EXPECT_EQ(spa["key1"], "value1");
    EXPECT_EQ(spa["key2"], "value2");
    EXPECT_EQ(spa["key3"], "value3");
    EXPECT_EQ(spa.size(), 3);
}

TEST_F (StringPairArrayTests, ContainsKey)
{
    StringPairArray spa;
    addDefaultPairs(spa);
    EXPECT_TRUE(spa.containsKey("key1"));
    EXPECT_FALSE(spa.containsKey("nonexistentKey"));
}

TEST_F (StringPairArrayTests, CaseSensitivity)
{
    StringPairArray spa(true);
    spa.set("Key", "value");
    EXPECT_EQ(spa["key"], "value");
    EXPECT_EQ(spa["KEY"], "value");

    spa.setIgnoresCase(false);
    EXPECT_TRUE(spa["key"].isEmpty());
}

TEST_F (StringPairArrayTests, RemoveByKey)
{
    StringPairArray spa;
    addDefaultPairs(spa);
    spa.remove("key2");
    EXPECT_FALSE(spa.containsKey("key2"));
    EXPECT_EQ(spa.size(), 2);
}

TEST_F (StringPairArrayTests, RemoveByIndex)
{
    StringPairArray spa;
    addDefaultPairs(spa);
    spa.remove(1); // Should remove "key2"
    EXPECT_FALSE(spa.containsKey("key2"));
    EXPECT_EQ(spa.size(), 2);
}

TEST_F (StringPairArrayTests, ClearAll)
{
    StringPairArray spa;
    addDefaultPairs(spa);
    spa.clear();
    EXPECT_EQ(spa.size(), 0);
}

TEST_F (StringPairArrayTests, AssignmentOperator)
{
    StringPairArray spa1;
    addDefaultPairs(spa1);
    StringPairArray spa2 = spa1; // Test copy assignment
    EXPECT_EQ(spa2["key1"], "value1");
}

TEST_F (StringPairArrayTests, EqualityOperator)
{
    StringPairArray spa1, spa2;
    addDefaultPairs(spa1);
    addDefaultPairs(spa2);
    EXPECT_TRUE(spa1 == spa2);
    spa2.set("key1", "modifiedValue");
    EXPECT_FALSE(spa1 == spa2);
}

TEST_F (StringPairArrayTests, AddArray)
{
    StringPairArray spa1, spa2;
    addDefaultPairs(spa1);
    spa2.set("key4", "value4");
    spa2.addArray(spa1);
    EXPECT_EQ(spa2.size(), 4);
    EXPECT_EQ(spa2["key1"], "value1");
}

TEST_F (StringPairArrayTests, DescriptionNotEmpty)
{
    StringPairArray spa;
    addDefaultPairs(spa);
    EXPECT_FALSE(spa.getDescription().isEmpty());
}

TEST_F (StringPairArrayTests, MinimiseStorageOverheads)
{
    StringPairArray spa;
    addDefaultPairs(spa);
    spa.minimiseStorageOverheads();
    EXPECT_EQ(spa.size(), 3);
}

TEST_F (StringPairArrayTests, AddMapRespectsCaseSensitivity)
{
    StringPairArray insensitive{true};  // Case insensitive
    insensitive.addMap({{"duplicate", "a"}, {"Duplicate", "b"}});
    EXPECT_EQ(insensitive.size(), 1);
    EXPECT_EQ(insensitive["DUPLICATE"], "a"_S);

    StringPairArray sensitive{false};  // Case sensitive
    sensitive.addMap({{"duplicate", "a"_S}, {"Duplicate", "b"_S}});
    EXPECT_EQ(sensitive.size(), 2);
    EXPECT_EQ(sensitive["duplicate"], "a"_S);
    EXPECT_EQ(sensitive["Duplicate"], "b"_S);
    EXPECT_EQ(sensitive["DUPLICATE"], ""_S);
}

TEST_F (StringPairArrayTests, AddMapOverwritesExistingPairs)
{
    StringPairArray insensitive{true};
    insensitive.set("key", "value");
    insensitive.addMap({{"KEY", "VALUE"}});
    EXPECT_EQ(insensitive.size(), 1);
    EXPECT_EQ(insensitive.getAllKeys()[0], "key"_S);
    EXPECT_EQ(insensitive.getAllValues()[0], "VALUE"_S);

    StringPairArray sensitive{false};
    sensitive.set("key", "value");
    sensitive.addMap({{"KEY", "VALUE"}, {"key", "another value"}});
    EXPECT_EQ(sensitive.size(), 2);
    EXPECT_EQ(sensitive.getAllKeys(), (StringArray{"key", "KEY"}));
    EXPECT_EQ(sensitive.getAllValues(), (StringArray{"another value", "VALUE"}));
}

TEST_F (StringPairArrayTests, AddMapDoesNotChangeOrderOfExistingKeys)
{
    StringPairArray array;
    array.set("a", "a");
    array.set("z", "z");
    array.set("b", "b");
    array.set("y", "y");
    array.set("c", "c");
    array.addMap({{"B", "B"}, {"0", "0"}, {"Z", "Z"}});

    EXPECT_EQ(array.getAllKeys(), (StringArray{"a", "z", "b", "y", "c", "0"}));
    EXPECT_EQ(array.getAllValues(), (StringArray{"a", "Z", "B", "y", "c", "0"}));
}

TEST_F (StringPairArrayTests, AddMapHasEquivalentBehaviourToAddArray)
{
    StringPairArray initial;
    initial.set("aaa", "aaa");
    initial.set("zzz", "zzz");
    initial.set("bbb", "bbb");

    auto withAddMap = initial;
    withAddMap.addMap({{"ZZZ", "ZZZ"}, {"ddd", "ddd"}});

    auto withAddArray = initial;
    withAddArray.addArray([]
    {
        StringPairArray toAdd;
        toAdd.set("ZZZ", "ZZZ");
        toAdd.set("ddd", "ddd");
        return toAdd;
    }());

    EXPECT_EQ(withAddMap, withAddArray);
}
