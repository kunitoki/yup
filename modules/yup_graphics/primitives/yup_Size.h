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

template <class ValueType>
class JUCE_API Size
{
public:
    constexpr Size() noexcept = default;

    constexpr Size (ValueType newWidth, ValueType newHeight) noexcept
        : width (newWidth)
        , height (newHeight)
    {
    }

    constexpr Size (const Size& other) noexcept = default;
    constexpr Size (Size&& other) noexcept = default;
    constexpr Size& operator=(const Size& other) noexcept = default;
    constexpr Size& operator=(Size&& other) noexcept = default;

    constexpr ValueType getWidth() const noexcept
    {
        return width;
    }

    constexpr ValueType getHeight() const noexcept
    {
        return height;
    }

    constexpr Size withWidth (ValueType newWidth) const noexcept
    {
        return { newWidth, height };
    }

    constexpr Size withHeight (ValueType newHeight) const noexcept
    {
        return { width, newHeight };
    }

    template <class T>
    constexpr Size<T> to() const noexcept
    {
        return { static_cast<T> (width), static_cast<T> (height) };
    }

    constexpr bool operator== (const Size& other) const noexcept
    {
        return width == other.width && height == other.height;
    }

    constexpr bool operator!= (const Size& other) const noexcept
    {
        return !(*this == other);
    }

private:
    ValueType width = 0;
    ValueType height = 0;
};

template <std::size_t I, class ValueType>
ValueType get (const Size<ValueType>& point)
{
    if constexpr (I == 0)
        return point.getWidth();
    else if constexpr (I == 1)
        return point.getHeight();
    else
        return {}; // TODO - error
}

} // namespace juce

namespace std
{

template <class ValueType>
struct tuple_size<juce::Size<ValueType>>
{
    inline static constexpr std::size_t value = 2;
};

template <std::size_t I, class ValueType>
struct tuple_element<I, juce::Size<ValueType>>
{
    using type = ValueType;
};

} // namespace std
