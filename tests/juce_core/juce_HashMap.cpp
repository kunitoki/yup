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

template <typename KeyType>
class RandomKeys
{
public:
    RandomKeys (int maxUniqueKeys, int seed) : r(seed)
    {
        for (int i = 0; i < maxUniqueKeys; ++i)
            keys.add (generateRandomKey(r));
    }

    KeyType next()
    {
        int i = r.nextInt (keys.size() - 1);
        return keys.getReference(i);
    }

private:
    juce::Random r;
    juce::Array<KeyType> keys;

    static KeyType generateRandomKey (juce::Random& rnd);
};

template <>
int RandomKeys<int>::generateRandomKey (juce::Random& rnd) { return rnd.nextInt(); }

template <>
void* RandomKeys<void*>::generateRandomKey (juce::Random& rnd) { return reinterpret_cast<void*> (static_cast<uintptr_t> (rnd.nextInt64())); }

template <>
juce::String RandomKeys<juce::String>::generateRandomKey (juce::Random& rnd)
{
    juce::String str;
    int len = rnd.nextInt (8) + 1;
    for (int i = 0; i < len; ++i)
        str += static_cast<char> (rnd.nextInt (95) + 32);
    return str;
}

template <typename KeyType, typename ValueType>
struct AssociativeMap
{
    struct KeyValuePair { KeyType key; ValueType value; };

    juce::Array<KeyValuePair> pairs;

    ValueType* find (KeyType key)
    {
        for (auto& pair : pairs)
            if (pair.key == key)
                return &pair.value;
        return nullptr;
    }

    void add (KeyType key, ValueType value)
    {
        if (ValueType* v = find (key))
            *v = value;
        else
            pairs.add ({ key, value });
    }

    int size() const { return pairs.size(); }
};

template <typename KeyType, typename ValueType>
void fillWithRandomValues (juce::HashMap<KeyType, int>& hashMap, AssociativeMap<KeyType, ValueType>& groundTruth)
{
    RandomKeys<KeyType> keyOracle (300, 3827829);
    juce::Random valueOracle (48735);

    for (int i = 0; i < 10000; ++i)
    {
        auto key = keyOracle.next();
        auto value = valueOracle.nextInt();

        groundTruth.add (key, value);
        hashMap.set (key, value);
    }
}

class HashMapTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        fillWithRandomValues (hashMap, groundTruth);

        threeMap.set (1, "one");
        threeMap.set (2, "two");
        threeMap.set (3, "three");
    }

    juce::HashMap<int, int> hashMap;
    AssociativeMap<int, int> groundTruth;

    juce::HashMap<int, std::string> threeMap;
};

TEST_F (HashMapTests, BasicOperations)
{
    juce::HashMap<int, std::string> map;
    map.set (1, "one");
    map.set (2, "two");

    EXPECT_EQ (map[1], "one");
    EXPECT_EQ (map[2], "two");
    EXPECT_EQ (map.size(), 2);
}

TEST_F (HashMapTests, NonExistingKey)
{
    juce::HashMap<int, std::string> map;
    EXPECT_EQ (map[999], "");  // Default string is empty
}

TEST_F (HashMapTests, ContainsKey)
{
    juce::HashMap<int, std::string> map;
    map.set (1, "one");

    EXPECT_TRUE (map.contains (1));
    EXPECT_FALSE (map.contains (2));
}

TEST_F (HashMapTests, ContainsValue)
{
    juce::HashMap<int, std::string> map;
    map.set (1, "unique");
    map.set (2, "unique");

    EXPECT_TRUE (map.containsValue ("unique"));
    EXPECT_FALSE (map.containsValue ("missing"));
}

TEST_F (HashMapTests, RemoveKey)
{
    juce::HashMap<int, std::string> map;
    map.set (1, "one");
    map.set (2, "two");
    map.remove (1);

    EXPECT_FALSE (map.contains (1));
    EXPECT_TRUE (map.contains (2));
    EXPECT_EQ (map.size(), 1);
}

TEST_F (HashMapTests, RemoveValue)
{
    juce::HashMap<int, std::string> map;
    map.set (1, "value");
    map.set (2, "value");
    map.removeValue ("value");

    EXPECT_FALSE (map.contains (1));
    EXPECT_FALSE (map.contains (2));
    EXPECT_EQ (map.size(), 0);
}

TEST_F (HashMapTests, Clear)
{
    juce::HashMap<int, std::string> map;
    map.set (1, "one");
    map.set (2, "two");
    map.clear();

    EXPECT_EQ (map.size(), 0);
}

TEST_F (HashMapTests, Iterator)
{
    juce::HashMap<int, std::string> map;
    map.set (1, "one");
    map.set (2, "two");
    map.set (3, "three");

    std::vector<int> keys;
    for (juce::HashMap<int, std::string>::Iterator it (map); it.next();)
    {
        keys.push_back (it.getKey());
        std::string value = it.getValue();
        EXPECT_TRUE (value == "one" || value == "two" || value == "three");
    }

    EXPECT_EQ (keys.size(), 3);
}

TEST_F (HashMapTests, GetReferenceAddsNonExistingKey)
{
    std::string& value = threeMap.getReference (4);
    EXPECT_EQ (value, "");
    value = "four";
    EXPECT_EQ (threeMap[4], "four");
}

TEST_F (HashMapTests, CopyConstruction)
{
    juce::HashMap<int, std::string> copiedMap (threeMap);
    EXPECT_EQ (copiedMap[1], "one");
    EXPECT_EQ (copiedMap[2], "two");
    EXPECT_EQ (copiedMap[3], "three");
}

TEST_F (HashMapTests, Assignment)
{
    juce::HashMap<int, std::string> assignedMap;
    assignedMap = threeMap;
    EXPECT_EQ (assignedMap[1], "one");
    EXPECT_EQ (assignedMap[2], "two");
    EXPECT_EQ (assignedMap[3], "three");
}

TEST_F (HashMapTests, RemapTable)
{
    // Initial number of slots
    int initialSlots = threeMap.getNumSlots();
    // Adding more elements to trigger remapping
    threeMap.set (4, "four");
    threeMap.set (5, "five");
    threeMap.set (6, "six");
    threeMap.set (7, "seven");
    threeMap.set (8, "eight");
    // Remap manually and check
    threeMap.remapTable (2 * initialSlots);
    EXPECT_GT (threeMap.getNumSlots(), initialSlots);
    EXPECT_EQ (threeMap[4], "four");
}

TEST_F (HashMapTests, SwapMaps)
{
    juce::HashMap<int, std::string> otherMap;
    otherMap.set (10, "ten");
    threeMap.swapWith (otherMap);

    EXPECT_FALSE (threeMap.contains (1));
    EXPECT_TRUE (threeMap.contains (10));
    EXPECT_TRUE (otherMap.contains (1));
}

TEST_F (HashMapTests, IteratorValidityAcrossModifications)
{
    juce::HashMap<int, std::string>::Iterator it(threeMap);
    it.next(); // move to first element
    threeMap.set (4, "four"); // Modify map after iterator creation

    // Test whether iterator continues safely
    EXPECT_NO_THROW ({ while (it.next()); });
}

TEST_F (HashMapTests, MultipleIdenticalValues)
{
    threeMap.set (4, "three");
    EXPECT_TRUE (threeMap.containsValue ("three"));
    EXPECT_EQ (threeMap.size(), 4);
    threeMap.removeValue ("three");
    EXPECT_EQ (threeMap.size(), 2);
    EXPECT_TRUE (threeMap.contains (2) || threeMap.contains (1));
}

TEST_F (HashMapTests, LoadFactorAndResizing)
{
    int n = 20; // Insert more elements than default size to force resize
    for (int i = 4; i <= n; ++i)
        threeMap.set (i, "value" + std::to_string (i));

    EXPECT_GT (threeMap.getNumSlots(), 10); // Default is likely less than the number of entries added
    EXPECT_EQ (threeMap.size(), n);
}

TEST_F (HashMapTests, NonDefaultHashFunction)
{
    // Define a custom hash function that collides more
    struct BadHashFunction
    {
        int generateHash (int key, int upperLimit) const noexcept
        {
            return key % 5; // Intentionally bad hashing for testing
        }
    };

    juce::HashMap<int, std::string, BadHashFunction> badHashMap;
    badHashMap.set (1, "one");
    badHashMap.set (6, "six"); // This should collide with '1' in the hash table

    EXPECT_EQ (badHashMap[1], "one");
    EXPECT_EQ (badHashMap[6], "six");
    EXPECT_EQ (badHashMap.getNumSlots(), 101); // Default slots should be used
}

TEST_F (HashMapTests, AddElements)
{
    for (const auto& pair : groundTruth.pairs)
        EXPECT_EQ (hashMap[pair.key], pair.value);
}

TEST_F (HashMapTests, AccessTest)
{
    for (auto pair : groundTruth.pairs)
        EXPECT_EQ (hashMap[pair.key], pair.value);
}

TEST_F (HashMapTests, RemoveTest)
{
    auto n = groundTruth.size();
    juce::Random r (3827387);

    for (int i = 0; i < 100; ++i)
    {
        auto idx = r.nextInt (n-- - 1);
        auto key = groundTruth.pairs.getReference (idx).key;

        groundTruth.pairs.remove (idx);
        hashMap.remove (key);

        EXPECT_FALSE (hashMap.contains (key));

        for (auto pair : groundTruth.pairs)
            EXPECT_EQ (hashMap[pair.key], pair.value);
    }
}

TEST_F (HashMapTests, PersistentMemoryLocationOfValues)
{
    struct AddressAndValue { int value; const int* valueAddress; };

    AssociativeMap<int, AddressAndValue> addresses;
    RandomKeys<int> keyOracle (300, 3827829);
    juce::Random valueOracle (48735);

    for (int i = 0; i < 1000; ++i)
    {
        auto key = keyOracle.next();
        auto value = valueOracle.nextInt();

        hashMap.set (key, value);

        if (auto* existing = addresses.find (key))
        {
            existing->value = value;
        }
        else
        {
            addresses.add (key, { value, &hashMap.getReference (key) });
        }

        for (auto& pair : addresses.pairs)
        {
            const auto& hashMapValue = hashMap.getReference (pair.key);

            EXPECT_EQ (hashMapValue, pair.value.value);
            EXPECT_EQ (&hashMapValue, pair.value.valueAddress);
        }
    }
}
