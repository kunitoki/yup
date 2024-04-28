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

namespace juce
{

//==============================================================================

class JUCE_API KeyModifiers
{
public:
    constexpr KeyModifiers() noexcept = default;

    constexpr KeyModifiers (int newModifiers) noexcept
        : modifiers (newModifiers)
    {
    }

    constexpr KeyModifiers (const KeyModifiers& other) noexcept = default;
    constexpr KeyModifiers (KeyModifiers&& other) noexcept = default;
    constexpr KeyModifiers& operator= (const KeyModifiers& other) noexcept = default;
    constexpr KeyModifiers& operator= (KeyModifiers&& other) noexcept = default;

    constexpr bool isShiftDown() const noexcept
    {
        return modifiers & shiftMask;
    }

    constexpr bool isControlDown() const noexcept
    {
        return modifiers & controlMask;
    }

    constexpr bool isAltDown() const noexcept
    {
        return modifiers & altMask;
    }

    constexpr bool isSuperDown() const noexcept
    {
        return modifiers & superMask;
    }

    constexpr bool isCapsLockDown() const noexcept
    {
        return modifiers & capsLockMask;
    }

    constexpr bool isNumLockDown() const noexcept
    {
        return modifiers & numLockMask;
    }

    constexpr KeyModifiers withFlags (int modifiersToAdd) const noexcept
    {
        return { modifiers | modifiersToAdd };
    }

    constexpr KeyModifiers withoutFlags (int modifiersToRemove) const noexcept
    {
        return { modifiers & ~modifiersToRemove };
    }

    constexpr bool testFlags (int modifiersToTest) const noexcept
    {
        return modifiers & modifiersToTest;
    }

    constexpr int getFlags() const noexcept
    {
        return modifiers;
    }

    constexpr bool operator== (const KeyModifiers& other) const noexcept
    {
        return modifiers == other.modifiers;
    }

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

} // namespace juce
