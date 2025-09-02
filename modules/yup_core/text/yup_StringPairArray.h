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

namespace yup
{

//==============================================================================
/**
    A container for holding a set of strings which are keyed by another string.
    
    This class provides a map-like container that associates string keys with string values.
    It offers both case-sensitive and case-insensitive key comparison modes, and maintains
    insertion order of key-value pairs.
    
    Key features:
    - Case-sensitive or case-insensitive key matching
    - Maintains insertion order of pairs
    - Range-based for loop support via iterators
    - Convenient initializer list construction
    - Integration with standard library maps
    - Memory-efficient storage with StringArray backing
    
    Usage examples:
    @code
    // Basic usage
    StringPairArray config;
    config.set("host", "localhost");
    config.set("port", "8080");
    
    // Initializer list construction
    StringPairArray headers {
        { "Content-Type", "application/json" },
        { "User-Agent", "YUP/1.0" }
    };
    
    // Range-based iteration
    for (const auto& pair : config)
    {
        std::cout << pair.key << " = " << pair.value << std::endl;
    }
    
    // Case sensitivity control
    StringPairArray caseSensitive(false);
    caseSensitive.set("Key", "value1");
    caseSensitive.set("key", "value2"); // Different from "Key"
    @endcode

    @see StringArray, KeyValuePair, Iterator

    @tags{Core}
*/
class YUP_API StringPairArray
{
public:
    //==============================================================================
    /** A key-value pair structure for iterator support.
    
        This structure provides access to both the key and value when iterating
        over a StringPairArray using range-based for loops.
        
        @see Iterator, begin(), end()
    */
    struct KeyValuePair
    {
        StringRef key;   /**< Reference to the key string */
        StringRef value; /**< Reference to the value string */

        /** Constructs a key-value pair.

            @param k  The key string reference
            @param v  The value string reference
        */
        KeyValuePair (StringRef k, StringRef v)
            : key (k)
            , value (v)
        {
        }
    };

    //==============================================================================
    /** Creates an empty array with default case-insensitive key comparison. */
    StringPairArray() = default;

    /** Creates an empty array with specified case sensitivity.

        @param ignoreCaseWhenComparingKeys  If true, key comparisons will be case-insensitive
    */
    StringPairArray (bool ignoreCaseWhenComparingKeys);

    /** Creates an array from an initializer list with default case-insensitive key comparison.
        
        This constructor allows convenient initialization using brace syntax:

        @code
        StringPairArray spa {
            { "key1", "value1" },
            { "key2", "value2" }
        };
        @endcode
        
        @param stringPairs  Initializer list of key-value pairs
    */
    StringPairArray (const std::initializer_list<KeyValuePair>& stringPairs);

    /** Creates an array from an initializer list with specified case sensitivity.
        
        This constructor allows convenient initialization with custom case sensitivity:

        @code
        StringPairArray spa (false, {
            { "Key", "value1" },
            { "key", "value2" }  // Will be treated as different from "Key"
        });
        @endcode
        
        @param ignoreCaseWhenComparingKeys  If true, key comparisons will be case-insensitive
        @param stringPairs Initializer list of key-value pairs
    */
    StringPairArray (bool ignoreCaseWhenComparingKeys, const std::initializer_list<KeyValuePair>& stringPairs);

    /** Creates a copy of another array.

        @param other  The array to copy from
    */
    StringPairArray (const StringPairArray& other);

    /** Move constructs from another array.

        @param other  The array to move from
    */
    StringPairArray (StringPairArray&& other);

    /** Destructor. */
    ~StringPairArray() = default;

    /** Copies the contents of another string array into this one.

        @param other  The array to copy from

        @returns A reference to this array
    */
    StringPairArray& operator= (const StringPairArray& other);

    /** Moves the contents of another string array into this one.

        @param other  The array to move from

        @returns A reference to this array
    */
    StringPairArray& operator= (StringPairArray&& other);

    //==============================================================================
    /** Compares two arrays for equality.
        
        Comparisons are case-sensitive regardless of the ignoreCase setting.
        The arrays are considered equal if they contain the same key-value pairs,
        though not necessarily in the same order.
        
        @param other  The array to compare with

        @returns true only if the other array contains exactly the same strings with the same keys
    */
    bool operator== (const StringPairArray& other) const;

    /** Compares two arrays for inequality.
        
        @param other  The array to compare with

        @returns true if the arrays are not equal

        @see operator==
    */
    bool operator!= (const StringPairArray& other) const;

    //==============================================================================
    /** Finds the value corresponding to a key string.

        If no such key is found, this will just return an empty string. To check whether a given key actually exists
        (because it might actually be paired with an empty string), use the containsKey() method or getAllKeys()
        method.

        Obviously the reference returned shouldn't be stored for later use, as the
        string it refers to may disappear when the array changes.

        @param key  The key to search for

        @returns A reference to the value string, or an empty string if not found

        @see getValue, containsKey
    */
    const String& operator[] (StringRef key) const;

    /** Finds the value corresponding to a key string with a default fallback.
        
        This is safer than operator[] when you need to distinguish between a missing key and a key with an empty value.

        @param key The key to search for
        @param defaultReturnValue The value to return if the key is not found

        @returns The value associated with the key, or the default value

        @see operator[], containsKey
    */
    String getValue (StringRef key, const String& defaultReturnValue) const;

    /** Checks if a key exists in the array.
        
        This method respects the case sensitivity setting of the array.
        
        @param key  The key to search for

        @returns true if the key exists, false otherwise
    */
    bool containsKey (StringRef key) const noexcept;

    /** Returns a list of all keys in the array.
        
        The keys are returned in the order they were added to the array.
        
        @returns A reference to the internal StringArray containing all keys
    */
    const StringArray& getAllKeys() const noexcept { return keys; }

    /** Returns a list of all values in the array.
        
        The values are returned in the same order as their corresponding keys.
        
        @returns A reference to the internal StringArray containing all values

        @see getAllKeys
    */
    const StringArray& getAllValues() const noexcept { return values; }

    /** Returns the number of key-value pairs in the array.
        
        @returns The number of pairs stored in the array
    */
    inline int size() const noexcept { return keys.size(); }

    //==============================================================================
    /** Iterator class for range-based for loop support.
        
        This iterator allows you to iterate over key-value pairs using range-based for loops:

        @code
        StringPairArray spa;
        spa.set("key1", "value1");
        spa.set("key2", "value2");
        
        for (const auto& pair : spa)
            YUP_DBG (pair.key << " = " << pair.value);
        @endcode
        
        @see KeyValuePair, begin(), end()
    */
    class Iterator
    {
    public:
        /** Constructs an iterator.

            @param array  The StringPairArray to iterate over
            @param index  The starting index position
        */
        Iterator (const StringPairArray& array, int index)
            : spa (array)
            , idx (index)
        {
        }

        /** Dereferences the iterator to get the current key-value pair.

            @returns A KeyValuePair containing references to the current key and value
        */
        KeyValuePair operator*() const { return KeyValuePair (spa.keys[idx], spa.values[idx]); }

        /** Pre-increments the iterator to the next position.

            @returns A reference to this iterator
        */
        Iterator& operator++()
        {
            ++idx;
            return *this;
        }

        /** Compares two iterators for inequality.
            @param other  The iterator to compare with
            @returns      true if the iterators point to different positions
        */
        bool operator!= (const Iterator& other) const { return idx != other.idx; }

    private:
        const StringPairArray& spa;
        int idx;
    };

    /** Returns an iterator pointing to the beginning of the array.
        
        @returns An iterator positioned at the first key-value pair

        @see end(), Iterator
    */
    Iterator begin() const noexcept { return Iterator (*this, 0); }

    /** Returns an iterator pointing to the end of the array.
        
        @returns An iterator positioned one past the last key-value pair

        @see begin(), Iterator
    */
    Iterator end() const noexcept { return Iterator (*this, size()); }

    //==============================================================================
    /** Adds or amends a key/value pair.
        
        If a value already exists with this key, its value will be overwritten,
        otherwise the key/value pair will be added to the array. Key comparison
        respects the case sensitivity setting of this array.
        
        @param key The key string
        @param value  The value string to associate with the key
    */
    void set (const String& key, const String& value);

    /** Adds the items from another array to this one.
        
        This is equivalent to using set() to add each of the pairs from the other array.
        Existing keys will be overwritten with values from the other array.
        
        @param other  The StringPairArray whose pairs should be added
    */
    void addArray (const StringPairArray& other);

    //==============================================================================
    /** Removes all key-value pairs from the array.
        
        After calling this method, the array will be empty.
    */
    void clear();

    /** Removes a key-value pair from the array based on its key.
        
        Key comparison respects the case sensitivity setting of this array.
        If the key isn't found, nothing will happen.
        
        @param key  The key of the pair to remove
    */
    void remove (StringRef key);

    /** Removes a key-value pair from the array based on its index.
        
        If the index is out-of-range, no action will be taken.
        
        @param index  The zero-based index of the pair to remove
    */
    void remove (int index);

    //==============================================================================
    /** Sets whether to use case-insensitive search when looking up keys.
        
        This affects all key operations including lookup, containsKey, set, and remove.
        
        @param shouldIgnoreCase  If true, key comparisons will be case-insensitive
    */
    void setIgnoresCase (bool shouldIgnoreCase);

    /** Returns whether case-insensitive search is used when looking up keys.
        
        @returns true if key comparisons are case-insensitive, false if case-sensitive
    */
    bool getIgnoresCase() const noexcept;

    //==============================================================================
    /** Returns a descriptive string containing all key-value pairs.
        
        This creates a human-readable representation of the array contents,
        which is handy for debugging or logging the array state.
        
        @returns A string representation of all key-value pairs
    */
    String getDescription() const;

    //==============================================================================
    /** Reduces the amount of storage being used by the array.

        Arrays typically allocate slightly more storage than they need, and after
        removing elements, they may have quite a lot of unused space allocated.
        This method will reduce the amount of allocated storage to a minimum.
        
        This is useful for optimizing memory usage after many removals.
    */
    void minimiseStorageOverheads();

    //==============================================================================
    /** Adds the contents of a std::map to this StringPairArray.
        
        This method efficiently adds key-value pairs from a standard map.
        Existing keys will be overwritten. The case sensitivity setting of
        this array affects how duplicate keys are handled.
        
        @param mapToAdd  The map whose contents should be added
    */
    void addMap (const std::map<String, String>& mapToAdd);

    /** Adds the contents of a std::unordered_map to this StringPairArray.
        
        This method efficiently adds key-value pairs from a standard unordered map.
        Existing keys will be overwritten. The case sensitivity setting of
        this array affects how duplicate keys are handled.
        
        @param mapToAdd  The unordered map whose contents should be added
    */
    void addUnorderedMap (const std::unordered_map<String, String>& mapToAdd);

private:
    //==============================================================================
    template <typename Map>
    void addMapImpl (const Map& mapToAdd);

    StringArray keys, values;
    bool ignoreCase = true;

    YUP_LEAK_DETECTOR (StringPairArray)
};

} // namespace yup
