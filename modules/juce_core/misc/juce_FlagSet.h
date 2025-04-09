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

#include <algorithm>

namespace juce
{

//==============================================================================
/**
    @brief Manages a set of flags using integral types.

    The FlagSet class is designed to operate on flags efficiently using bitwise operations. It utilizes templates to support any integral
    type as a flag container, ensuring type safety and compile-time checks. It can be used to manipulate sets of options or features
    represented as bits within an integral type. This class allows setting, unsetting, testing, and combining flag sets, as well as creating
    custom flag sets from specified bits.

    @tparam T An integral type used for storing the flags.
    @tparam Ts Variadic template types representing individual flag positions or meanings.

    @code

    namespace detail {
    struct verboseLog;
    struct noErrorLog;
    }

    using LogOption = FlagSet<uint32, detail::verboseLog, detail::noErrorLog>;
    static inline constexpr LogOption defaultLog = LogOption();
    static inline constexpr LogOption verboseLog = LogOption::declareValue<detail::verboseLog>();
    static inline constexpr LogOption noErrorLog = LogOption::declareValue<detail::noErrorLog>();

    LogOption option = verboseLog | noErrorLog;
    if (option.test (verboseLog))
        ...

    @endcode
*/
template <class T, class... Ts>
class FlagSet
{
    static_assert (std::is_integral_v<T>);

public:
    //==============================================================================
    /** Constructs a default FlagSet with all flags cleared. */
    constexpr FlagSet() noexcept = default;

    //==============================================================================
    /** Declares a new FlagSet with specific flags set using template parameters.

        Utilizes template metaprogramming to generate a FlagSet with flags set at positions determined by the template
        arguments relative to the base template types. This static method allows for compile-time declaration of flag values.

        @tparam Us Variadic template types used to determine which flags to set.

        @return A FlagSet with the specified flags set.
    */
    template <class... Us>
    static constexpr FlagSet declareValue() noexcept
    {
        return FlagSet { mask<Us...>() };
    }

    //==============================================================================
    /** Sets flags that are set in another FlagSet to this one.

        Performs a bitwise OR on the current flags with another set of flags, modifying this FlagSet to include all
        lags that are either already set or set in the other FlagSet.

        @param other Another FlagSet whose flags are to be set in this FlagSet.
    */
    constexpr void set (FlagSet other) noexcept
    {
        flags |= other.flags;
    }

    /** Creates a new FlagSet with combined flags from this and another FlagSet.

        Returns a new FlagSet resulting from the bitwise OR of this FlagSet's flags and another FlagSet's flags.
        This method does not modify the original FlagSet.

        @param other Another FlagSet whose flags are combined with this one.

        @return A new FlagSet with the combined flags.
    */
    constexpr FlagSet withSet (FlagSet other) const noexcept
    {
        FlagSet result { flags };
        result.flags |= other.flags;
        return result;
    }

    //==============================================================================
    /** Unsets flags that are set in another FlagSet from this one.

        Performs a bitwise AND with the complement of the other FlagSet's flags, effectively unsetting flags that are
        set in the other FlagSet.

        @param other Another FlagSet whose flags are to be unset in this FlagSet.
    */
    constexpr void unset (FlagSet other) noexcept
    {
        flags &= ~other.flags;
    }

    /** Creates a new FlagSet with flags unset from this one based on another FlagSet.

        Returns a new FlagSet resulting from the bitwise AND of this FlagSet's flags with the complement of another
        FlagSet's flags. This method creates a new FlagSet with specific flags unset.

        @param other Another FlagSet whose flags are used to unset flags from this FlagSet.

        @return A new FlagSet with the specified flags unset.
    */
    constexpr FlagSet withUnset (FlagSet other) const noexcept
    {
        FlagSet result { flags };
        result.flags &= ~other.flags;
        return result;
    }

    //==============================================================================
    /** Tests if any flags from another FlagSet are set in this one.

        Checks if any flags that are set in another FlagSet are also set in this FlagSet using a bitwise AND operation.

        @param other Another FlagSet to test against this FlagSet.

        @return True if any flags are both set in this and the other FlagSet, false otherwise.
    */
    constexpr bool test (FlagSet other) const noexcept
    {
        return (flags & other.flags) != 0;
    }

    //==============================================================================
    /** Binary OR operator to combine flags from two FlagSets.

        Combines the flags of this FlagSet with another by performing a bitwise OR operation, producing a new FlagSet.

        @param other Another FlagSet to combine with this one.

        @return A new FlagSet with the combined flags.
    */
    constexpr FlagSet operator| (FlagSet other) const noexcept
    {
        return FlagSet (flags | other.flags);
    }

    /** Binary OR operator to combine this FlagSet with another.

        Combines the flags of this FlagSet with another by performing a bitwise OR operation.

        @param other Another FlagSet to combine with this one.

        @return The current FlagSet.
    */
    constexpr FlagSet& operator|= (FlagSet other) noexcept
    {
        flags = (flags | other.flags);
        return *this;
    }

    /** Binary AND operator to intersect flags from two FlagSets.

        Produces a new FlagSet containing only the flags that are set in both this and another FlagSet by performing a
        bitwise AND operation.

        @param other Another FlagSet to intersect with this one.

        @return A new FlagSet with the intersected flags.
    */
    constexpr FlagSet operator& (FlagSet other) const noexcept
    {
        return FlagSet (flags & other.flags);
    }

    /** Binary AND operator to combine this FlagSet with another.

        Combines the flags of this FlagSet with another by performing a bitwise AND operation.

        @param other Another FlagSet to combine with this one.

        @return The current FlagSet.
    */
    constexpr FlagSet& operator&= (FlagSet other) noexcept
    {
        flags = (flags & other.flags);
        return *this;
    }

    /** Unary NOT operator to invert flags in the FlagSet.

        Creates a new FlagSet where all flags are inverted (i.e., 1's become 0's and vice versa) from this FlagSet.

        @return A new FlagSet with all flags inverted.
    */
    constexpr FlagSet operator~() const noexcept
    {
        return FlagSet (~flags);
    }

    //==============================================================================
    /** Equality operator between two FlagSet. */
    constexpr bool operator== (const FlagSet& other) const noexcept
    {
        return flags == other.flags;
    }

    /** Inequality operator between two FlagSet. */
    constexpr bool operator!= (const FlagSet& other) const noexcept
    {
        return ! (*this == other);
    }

    //==============================================================================
    /** Converts the flag bits to a string representation.

        Constructs a binary string representation of the flags, where each bit is represented by '1' or '0'. 

        @return A string representing the binary state of the flags.
    */
    String toString() const
    {
        String result;

        (result.append ((mask<Ts>() & flags) ? "1" : "0", 1), ...);

        return result;
    }

    /** Converts a string of '0' and '1' to a flag set.

        @return A string representing the binary state of the flags.
    */
    static FlagSet fromString (const String& text)
    {
        int flags = 0;

        const int maxLength = jmin (text.length(), static_cast<int> (sizeof...(Ts)));

        for (int index = 0; index < maxLength; ++index)
        {
            const int currentFlag = 1 << index;

            if (text[index] == juce_wchar ('1'))
                flags |= currentFlag;
        }

        return FlagSet (flags);
    }

private:
    template <class U, class V, class... Us>
    static constexpr T indexOf() noexcept
    {
        if constexpr (std::is_same_v<U, V>)
            return static_cast<T> (0);
        else
            return static_cast<T> (1) + indexOf<U, Us...>();
    }

    template <class... Us>
    static constexpr T mask() noexcept
    {
        return ((static_cast<T> (1) << indexOf<Us, Ts...>()) | ...);
    }

    constexpr FlagSet (T flagValues) noexcept
        : flags (flagValues)
    {
    }

    T flags = T (0);
};

} // namespace juce
