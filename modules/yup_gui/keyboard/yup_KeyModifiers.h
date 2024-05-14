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

namespace yup
{

//==============================================================================
/**
 * @brief Represents modifier keys (like Shift, Ctrl, etc.) pressed during a keyboard event.
 *
 * This class encapsulates the state of modifier keys during keyboard interactions, providing methods to check if specific
 * modifiers are active and manipulate the modifier state.
 */
class JUCE_API KeyModifiers
{
public:
    //==============================================================================
    /**
     * @brief Default constructor, initializes with no modifiers active.
     */
    constexpr KeyModifiers() noexcept = default;

    /**
     * @brief Constructs KeyModifiers with a specific set of modifiers.
     *
     * @param newModifiers An integer bitmask representing the initial state of the modifiers.
     */
    constexpr KeyModifiers (int newModifiers) noexcept
        : modifiers (newModifiers)
    {
    }

    //==============================================================================
    constexpr KeyModifiers (const KeyModifiers& other) noexcept = default;
    constexpr KeyModifiers (KeyModifiers&& other) noexcept = default;
    constexpr KeyModifiers& operator= (const KeyModifiers& other) noexcept = default;
    constexpr KeyModifiers& operator= (KeyModifiers&& other) noexcept = default;

    //==============================================================================
    /**
     * @brief Checks if the Shift key is down.
     *
     * @return True if Shift is active, false otherwise.
     */
    constexpr bool isShiftDown() const noexcept
    {
        return modifiers & shiftMask;
    }

    /**
     * @brief Checks if the Control key is down.
     *
     * @return True if Control is active, false otherwise.
     */
    constexpr bool isControlDown() const noexcept
    {
        return modifiers & controlMask;
    }

    /**
     * @brief Checks if the Alt key is down.
     *
     * @return True if Alt is active, false otherwise.
     */
    constexpr bool isAltDown() const noexcept
    {
        return modifiers & altMask;
    }

    /**
     * @brief Checks if the Super (Windows or Command) key is down.
     *
     * @return True if Super is active, false otherwise.
     */
    constexpr bool isSuperDown() const noexcept
    {
        return modifiers & superMask;
    }

    /**
     * @brief Checks if the Caps Lock is active.
     *
     * @return True if Caps Lock is active, false otherwise.
     */
    constexpr bool isCapsLockDown() const noexcept
    {
        return modifiers & capsLockMask;
    }

    /**
     * @brief Checks if the Num Lock is active.
     *
     * @return True if Num Lock is active, false otherwise.
     */
    constexpr bool isNumLockDown() const noexcept
    {
        return modifiers & numLockMask;
    }

    //==============================================================================
    /**
     * @brief Gets the current flags representing the active modifiers.
     *
     * @return The bitmask of active modifiers.
     */
    constexpr int getFlags() const noexcept
    {
        return modifiers;
    }

    //==============================================================================
    /**
     * @brief Returns a new KeyModifiers object with additional flags set.
     *
     * @param modifiersToAdd The modifier flags to add.
     *
     * @return A new KeyModifiers object with the added flags.
     */
    constexpr KeyModifiers withFlags (int modifiersToAdd) const noexcept
    {
        return { modifiers | modifiersToAdd };
    }

    /**
     * @brief Returns a new KeyModifiers object with specific flags removed.
     *
     * @param modifiersToRemove The modifier flags to remove.
     *
     * @return A new KeyModifiers object with the removed flags.
     */
    constexpr KeyModifiers withoutFlags (int modifiersToRemove) const noexcept
    {
        return { modifiers & ~modifiersToRemove };
    }

    //==============================================================================
    /**
     * @brief Tests if specific modifier flags are active.
     *
     * @param modifiersToTest The modifier flags to test.
     *
     * @return True if all specified flags are active, false otherwise.
     */
    constexpr bool testFlags (int modifiersToTest) const noexcept
    {
        return modifiers & modifiersToTest;
    }

    //==============================================================================
    /**
     * @brief Compares this KeyModifiers object to another for equality.
     *
     * @param other The other KeyModifiers object to compare against.
     *
     * @return True if the modifiers are the same, false otherwise.
     */
    constexpr bool operator== (const KeyModifiers& other) const noexcept
    {
        return modifiers == other.modifiers;
    }

    /**
     * @brief Compares this KeyModifiers object to another for inequality.
     *
     * @param other The other KeyModifiers object to compare against.
     *
     * @return True if the modifiers are not the same, false otherwise.
     */
    constexpr bool operator!= (const KeyModifiers& other) const noexcept
    {
        return !(*this == other);
    }

private:
    static constexpr int shiftMask = 0x0001;
    static constexpr int controlMask = 0x0002;
    static constexpr int altMask = 0x0004;
    static constexpr int superMask = 0x0008;
    static constexpr int capsLockMask = 0x0010;
    static constexpr int numLockMask = 0x0020;

    int32_t modifiers = 0;
};

} // namespace yup
