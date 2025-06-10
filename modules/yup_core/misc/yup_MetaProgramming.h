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
template <bool Value, auto... Args>
inline constexpr bool dependentBoolValue = Value;

template <auto... Args>
inline constexpr bool dependentFalse = dependentBoolValue<false, Args...>;

//==============================================================================
struct NoneSuch
{
    NoneSuch() = delete;
    ~NoneSuch() = delete;
    NoneSuch(NoneSuch const&) = delete;
    void operator=(NoneSuch const&) = delete;
    NoneSuch(NoneSuch&&) = delete;
    void operator=(NoneSuch&&) = delete;
};

namespace detail {
template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
struct Detector
{
    using ValueType = std::false_type;
    using Type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct Detector<Default, std::void_t<Op<Args...>>, Op, Args...>
{
    using ValueType = std::true_type;
    using Type = Op<Args...>;
};

template <template <class...> class Op, class... Args>
using isDetected = typename Detector<NoneSuch, void, Op, Args...>::ValueType;

template <class Default, template <class...> class Op, class... Args>
using detectedOr = Detector<Default, void, Op, Args...>;
} // namespace detail

template <template <class...> class Op, class... Args>
inline constexpr bool isDetected = detail::isDetected<Op, Args...>::value;

template <template <class...> class Op, class... Args>
using detectedType = typename detail::Detector<NoneSuch, void, Op, Args...>::Type;

template <class Default, template <class...> class Op, class... Args>
using detectedOr = typename detail::detectedOr<Default, Op, Args...>::Type;

namespace detail {
template <class Expected, template <class...> class Op, class... Args>
using isDetectedExact = std::is_same<Expected, detectedType<Op, Args...>>;

template <class To, template <class...> class Op, class... Args>
using isDetectedConvertible = std::is_convertible<detectedType<Op, Args...>, To>;
} // namespace detail

template <class Expected, template <class...> class Op, class... Args>
inline constexpr bool isDetectedExact = detail::isDetectedExact<Expected, Op, Args...>::value;

template <class To, template <class...> class Op, class... Args>
inline constexpr bool isDetectedConvertible = detail::isDetectedConvertible<To, Op, Args...>::value;

} // namespace yup
