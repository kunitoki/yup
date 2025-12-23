/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#ifndef DOXYGEN

namespace yup::ump
{

#ifdef Status
#undef Status
#endif

using uint2_t = std::uint8_t;
using uint4_t = std::uint8_t;
using uint7_t = std::uint8_t;
using uint8_t = std::uint8_t;
using uint14_t = std::uint16_t;
using uint16_t = std::uint16_t;
using uint28_t = std::uint32_t;
using uint32_t = std::uint32_t;
using int32_t = std::int32_t;
using size_t = std::size_t;

using Group = uint4_t;
using Status = uint8_t;
using Channel = uint4_t;
using NoteNumber = uint7_t;
using ControllerNumber = uint7_t;
using ProgramNumber = uint7_t;
using Muid = uint28_t;
using ManufacturerId = uint32_t;
using Protocol = uint8_t;
using Extensions = uint8_t;

constexpr uint7_t downsample16To7Bit (uint16_t v) { return uint7_t (v >> 9u); }

constexpr uint7_t downsample32To7Bit (uint32_t v) { return uint7_t (v >> 25u); }

constexpr uint14_t downsample32To14Bit (uint32_t v) { return uint14_t (v >> 18u); }

constexpr uint16_t upsample7To16Bit (uint7_t v)
{
    uint16_t result = uint16_t (v) << 9u;

    if (v > 0x40)
    {
        const auto bits = uint16_t (v & 0x3f);
        result |= (uint16_t) ((bits << 3u) | (bits >> 3u));
    }

    return result;
}

constexpr uint32_t upsample7To32Bit (uint7_t v)
{
    uint32_t result = uint32_t (v) << 25u;

    if (v > 0x40)
    {
        uint32_t bits = uint32_t (v & 0x3f);
        bits |= (bits << 6u);
        result |= (bits << 13u) | (bits << 1u) | (bits >> 11u);
    }

    return result;
}

constexpr uint32_t upsample14To32Bit (uint14_t v)
{
    uint32_t result = uint32_t (v) << 18u;

    if (v > 0x2000)
    {
        const auto bits = uint32_t (v & 0x1fff);
        result |= (bits << 5u) | (bits >> 8u);
    }

    return result;
}

constexpr uint32_t upsampleXToYBit (uint32_t v, uint8_t x, uint8_t y)
{
    jassert (x > 1 && y <= 32 && x < y);

    const auto scaleBits = uint8_t (y - x);
    const auto center = uint32_t (1u << (x - 1u));

    uint32_t result = v << scaleBits;
    if (v <= center)
        return result;

    const auto repeatBits = uint8_t (x - 1u);
    const auto repeatMask = uint32_t ((1u << repeatBits) - 1u);
    uint32_t repeatValue = v & repeatMask;

    if (scaleBits > repeatBits)
        repeatValue <<= (scaleBits - repeatBits);
    else
        repeatValue >>= (repeatBits - scaleBits);

    while (repeatValue != 0)
    {
        result |= repeatValue;
        repeatValue >>= repeatBits;
    }

    return result;
}

struct Velocity
{
    uint16_t value { 0x8000 };

    constexpr Velocity() = default;

    constexpr explicit Velocity (uint16_t v)
        : value (v)
    {
    }

    constexpr explicit Velocity (uint7_t v)
        : value (upsample7To16Bit (v))
    {
    }

    constexpr explicit Velocity (float v);
    constexpr explicit Velocity (double v);

    constexpr float asFloat() const;
    constexpr double asDouble() const;

    constexpr uint7_t asUInt7() const { return downsample16To7Bit (value); }

    constexpr bool operator== (const Velocity& other) const { return value == other.value; }

    constexpr bool operator!= (const Velocity& other) const { return value != other.value; }
};

struct PitchBend
{
    uint32_t value { 0x80000000 };

    constexpr PitchBend() = default;

    constexpr explicit PitchBend (uint32_t v)
        : value (v)
    {
    }

    constexpr explicit PitchBend (uint14_t v)
        : value (upsample14To32Bit (v))
    {
    }

    constexpr explicit PitchBend (float v);
    constexpr explicit PitchBend (double v);

    constexpr float asFloat() const { return static_cast<float> (asDouble()); }

    constexpr double asDouble() const;

    constexpr uint14_t asUInt14() const { return downsample32To14Bit (value); }

    constexpr void reset() { value = 0x80000000; }

    constexpr bool operator== (const PitchBend& other) const { return value == other.value; }

    constexpr bool operator!= (const PitchBend& other) const { return value != other.value; }
};

struct PitchIncrement
{
    int32_t value { 0 };

    constexpr PitchIncrement() = default;

    constexpr explicit PitchIncrement (int32_t v)
        : value (v)
    {
    }

    constexpr explicit PitchIncrement (float v);
    constexpr explicit PitchIncrement (double v);

    constexpr void operator+= (const PitchIncrement& inc);
    constexpr PitchIncrement operator+ (const PitchIncrement& inc) const;

    constexpr bool operator== (const PitchIncrement& other) const { return value == other.value; }

    constexpr bool operator!= (const PitchIncrement& other) const { return value != other.value; }
};

struct Pitch7_9
{
    uint16_t value { 0 };

    constexpr Pitch7_9() = default;

    constexpr explicit Pitch7_9 (uint16_t v)
        : value (v)
    {
    }

    constexpr explicit Pitch7_9 (NoteNumber v)
        : value (uint16_t (v) << 9)
    {
    }

    constexpr explicit Pitch7_9 (float v);
    constexpr explicit Pitch7_9 (double v);

    constexpr float asFloat() const;
    constexpr double asDouble() const;

    constexpr NoteNumber noteNumber() const { return NoteNumber (value >> 9); }

    constexpr bool operator== (const Pitch7_9& other) const { return value == other.value; }

    constexpr bool operator!= (const Pitch7_9& other) const { return value != other.value; }
};

struct ControllerValue;

struct Pitch7_25
{
    uint32_t value { 0 };

    constexpr Pitch7_25() = default;

    constexpr explicit Pitch7_25 (uint32_t v)
        : value (v)
    {
    }

    constexpr explicit Pitch7_25 (NoteNumber v)
        : value (uint32_t (v) << 25)
    {
    }

    constexpr explicit Pitch7_25 (Pitch7_9 v) { *this = v; }

    constexpr explicit Pitch7_25 (const ControllerValue& v);
    constexpr explicit Pitch7_25 (float v);
    constexpr explicit Pitch7_25 (double v);

    constexpr float asFloat() const { return static_cast<float> (asDouble()); }

    constexpr double asDouble() const;

    constexpr NoteNumber noteNumber() const { return NoteNumber (value >> 25); }

    constexpr Pitch7_25& operator= (Pitch7_9 v)
    {
        value = uint32_t (v.value) << 16;
        return *this;
    }

    constexpr Pitch7_25 operator+ (const PitchIncrement& inc) const;

    constexpr Pitch7_25 operator+ (float v) const { return Pitch7_25 { asFloat() + v }; }

    constexpr Pitch7_25 operator+ (double v) const { return Pitch7_25 { asDouble() + v }; }

    constexpr void operator+= (const PitchIncrement& inc);

    constexpr bool operator== (const Pitch7_25& other) const { return value == other.value; }

    constexpr bool operator!= (const Pitch7_25& other) const { return value != other.value; }
};

struct PitchBendSensitivity : Pitch7_25
{
    constexpr PitchBendSensitivity()
        : Pitch7_25 { NoteNumber { 2 } }
    {
    }

    using Pitch7_25::Pitch7_25;
};

constexpr PitchIncrement operator* (const PitchBend& bend, const PitchBendSensitivity& sens)
{
    auto result = int64_t (bend.value) - int64_t (0x80000000u);
    if (result == 0)
        return PitchIncrement { int32_t { 0 } };

    result *= sens.value;
    result /= 2147483648;

    return PitchIncrement { static_cast<int32_t> (std::clamp (result,
                                                              int64_t (std::numeric_limits<int32_t>::min()),
                                                              int64_t (std::numeric_limits<int32_t>::max()))) };
}

constexpr PitchIncrement operator* (const PitchBendSensitivity& sens, const PitchBend& bend)
{
    return bend * sens;
}

struct ControllerIncrement
{
    int32_t value { 0 };

    constexpr ControllerIncrement() = default;

    constexpr explicit ControllerIncrement (int32_t v)
        : value (v)
    {
    }

    constexpr bool operator== (const ControllerIncrement& other) const { return value == other.value; }

    constexpr bool operator!= (const ControllerIncrement& other) const { return value != other.value; }
};

struct ControllerValue
{
    uint32_t value { 0 };

    constexpr ControllerValue() = default;

    constexpr explicit ControllerValue (uint32_t v)
        : value (v)
    {
    }

    constexpr explicit ControllerValue (uint14_t v)
        : value (upsample14To32Bit (v))
    {
    }

    constexpr explicit ControllerValue (uint7_t v)
        : value (upsample7To32Bit (v))
    {
    }

    constexpr explicit ControllerValue (const Pitch7_25& v)
        : value (v.value)
    {
    }

    constexpr explicit ControllerValue (float v);
    constexpr explicit ControllerValue (double v);

    constexpr float asFloat() const;
    constexpr double asDouble() const;

    constexpr uint14_t asUInt14() const { return downsample32To14Bit (value); }

    constexpr uint7_t asUInt7() const { return downsample32To7Bit (value); }

    constexpr void operator+= (ControllerIncrement inc);
    constexpr ControllerValue operator+ (ControllerIncrement inc) const;

    constexpr bool operator== (const ControllerValue& other) const { return value == other.value; }

    constexpr bool operator!= (const ControllerValue& other) const { return value != other.value; }
};

#pragma pack(push, 1)

struct DeviceIdentity
{
    ManufacturerId manufacturer {};
    uint14_t family {};
    uint14_t model {};
    uint28_t revision {};
};

#pragma pack(pop)

namespace detail
{
template <typename F>
constexpr uint64_t round_to_uint (F v)
{
    return v <= F (0) ? uint64_t (0) : uint64_t (v + F (0.5));
}

template <typename T, typename F>
constexpr T from_float_0_1 (F v)
{
    if (v <= F (0))
        return 0;

    if (v >= F (1))
        return std::numeric_limits<T>::max();

    constexpr auto max = std::numeric_limits<T>::max();
    if (v <= F (0.5))
    {
        constexpr auto scale = static_cast<double> (max) + 1.0;
        return static_cast<T> (v * scale);
    }

    constexpr auto mid = (max >> 1) + 1;
    constexpr auto scale = static_cast<double> (max);
    return mid + static_cast<T> ((v - F (0.5)) * scale);
}

template <typename T, typename F>
constexpr F to_float_0_1 (T value)
{
    constexpr auto max = std::numeric_limits<T>::max();
    constexpr auto center = (max >> 1) + 1;

    if (value <= center)
        return static_cast<F> (value / static_cast<double> (center) / 2.0);

    return static_cast<F> (value / static_cast<double> (max));
}
} // namespace detail

constexpr Velocity::Velocity (float v)
    : value (detail::from_float_0_1<uint16_t, float> (v))
{
}

constexpr Velocity::Velocity (double v)
    : value (detail::from_float_0_1<uint16_t, double> (v))
{
}

constexpr float Velocity::asFloat() const { return detail::to_float_0_1<uint16_t, float> (value); }

constexpr double Velocity::asDouble() const { return detail::to_float_0_1<uint16_t, double> (value); }

constexpr PitchBend::PitchBend (float v)
    : value (detail::from_float_0_1<uint32_t, float> ((v + 1.0f) / 2.0f))
{
}

constexpr PitchBend::PitchBend (double v)
    : value (detail::from_float_0_1<uint32_t, double> ((v + 1.0) / 2.0))
{
}

constexpr double PitchBend::asDouble() const
{
    if (value >= 0x80000000u)
        return (value - 0x80000000u) / static_cast<double> (0x7fffffffu);

    return (0x80000000u - value) / (-static_cast<double> (0x80000000u));
}

constexpr PitchIncrement::PitchIncrement (float v)
{
    if (v >= 64.0f)
    {
        value = std::numeric_limits<int32_t>::max();
        return;
    }

    if (v <= -64.0f)
    {
        value = std::numeric_limits<int32_t>::min();
        return;
    }

    if (v >= 0.0f)
    {
        value = static_cast<int32_t> (Pitch7_25 { v }.value);
        return;
    }

    value = -static_cast<int32_t> (Pitch7_25 { -v }.value);
}

constexpr PitchIncrement::PitchIncrement (double v)
{
    if (v >= 64.0)
    {
        value = std::numeric_limits<int32_t>::max();
        return;
    }

    if (v <= -64.0)
    {
        value = std::numeric_limits<int32_t>::min();
        return;
    }

    if (v >= 0.0)
    {
        const auto result = Pitch7_25 { v }.value;
        value = result <= uint32_t (std::numeric_limits<int32_t>::max()) ? static_cast<int32_t> (result)
                                                                         : std::numeric_limits<int32_t>::max();
        return;
    }

    value = -static_cast<int32_t> (Pitch7_25 { -v }.value);
}

constexpr void PitchIncrement::operator+= (const PitchIncrement& inc)
{
    const auto sum = static_cast<int64_t> (value) + inc.value;
    value = static_cast<int32_t> (std::clamp (sum,
                                              int64_t (std::numeric_limits<int32_t>::min()),
                                              int64_t (std::numeric_limits<int32_t>::max())));
}

constexpr PitchIncrement PitchIncrement::operator+ (const PitchIncrement& inc) const
{
    PitchIncrement out { *this };
    out += inc;
    return out;
}

constexpr Pitch7_9::Pitch7_9 (float v)
{
    constexpr int fractionalBits = 9;

    if (v <= 0.0f)
        value = 0;
    else if (v >= 128.0f)
        value = 0xffff;
    else
        value = static_cast<uint16_t> (detail::round_to_uint (v * float (1 << fractionalBits)));
}

constexpr Pitch7_9::Pitch7_9 (double v)
{
    constexpr int fractionalBits = 9;

    if (v <= 0.0)
        value = 0;
    else if (v >= 128.0)
        value = 0xffff;
    else
        value = static_cast<uint16_t> (detail::round_to_uint (v * double (1 << fractionalBits)));
}

constexpr float Pitch7_9::asFloat() const
{
    constexpr int fractionalBits = 9;
    return static_cast<float> (value) / float (1 << fractionalBits);
}

constexpr double Pitch7_9::asDouble() const
{
    constexpr int fractionalBits = 9;
    return static_cast<double> (value) / double (1 << fractionalBits);
}

constexpr Pitch7_25::Pitch7_25 (float v)
{
    constexpr int fractionalBits = 25;

    if (v <= 0.0f)
        value = 0;
    else if (v >= 128.0f)
        value = 0xffffffffu;
    else
        value = static_cast<uint32_t> (detail::round_to_uint (v * float (1 << fractionalBits)));
}

constexpr Pitch7_25::Pitch7_25 (double v)
{
    constexpr int fractionalBits = 25;

    if (v <= 0.0)
        value = 0;
    else if (v >= 128.0)
        value = 0xffffffffu;
    else
        value = static_cast<uint32_t> (detail::round_to_uint (v * double (1 << fractionalBits)));
}

constexpr double Pitch7_25::asDouble() const
{
    constexpr int fractionalBits = 25;
    return static_cast<double> (value) / double (1 << fractionalBits);
}

constexpr Pitch7_25::Pitch7_25 (const ControllerValue& v)
    : value (v.value)
{
}

constexpr Pitch7_25 Pitch7_25::operator+ (const PitchIncrement& inc) const
{
    const auto sum = static_cast<int64_t> (value) + inc.value;
    const auto clamped = std::clamp (sum, int64_t (0), int64_t (0xffffffffu));
    return Pitch7_25 { static_cast<uint32_t> (clamped) };
}

constexpr void Pitch7_25::operator+= (const PitchIncrement& inc)
{
    *this = *this + inc;
}

constexpr ControllerValue::ControllerValue (float v)
    : value (detail::from_float_0_1<uint32_t, float> (v))
{
}

constexpr ControllerValue::ControllerValue (double v)
    : value (detail::from_float_0_1<uint32_t, double> (v))
{
}

constexpr float ControllerValue::asFloat() const
{
    return detail::to_float_0_1<uint32_t, float> (value);
}

constexpr double ControllerValue::asDouble() const
{
    return detail::to_float_0_1<uint32_t, double> (value);
}

constexpr void ControllerValue::operator+= (ControllerIncrement inc)
{
    const auto sum = static_cast<int64_t> (value) + inc.value;
    value = static_cast<uint32_t> (std::clamp (sum, int64_t (0), int64_t (0xffffffffu)));
}

constexpr ControllerValue ControllerValue::operator+ (ControllerIncrement inc) const
{
    ControllerValue out { *this };
    out += inc;
    return out;
}

} // namespace yup::ump

#endif
