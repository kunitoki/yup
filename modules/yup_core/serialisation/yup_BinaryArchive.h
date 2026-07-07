/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#pragma once

namespace yup
{

namespace detail
{

template <typename T>
struct is_named_type : std::false_type
{
};

template <typename T>
struct is_named_type<Named<T>> : std::true_type
{
};

template <typename T>
constexpr bool is_named_v = is_named_type<std::decay_t<T>>::value;

template <typename T>
struct is_serialisation_size_type : std::false_type
{
};

template <typename T>
struct is_serialisation_size_type<SerialisationSize<T>> : std::true_type
{
};

template <typename T>
constexpr bool is_serialisation_size_v = is_serialisation_size_type<std::decay_t<T>>::value;

} // namespace detail

//==============================================================================
/**
    An archive that serialises objects to a binary OutputStream.

    Works with the SerialisationTraits system: types that implement marshallingVersion +
    serialise/save/load are serialised recursively. Primitives (bool, integers, float,
    double, String, enums) are written directly to the stream in little-endian byte order.

    Named<T> values have their names stripped (names are not stored in the binary format).
    SerialisationSize<T> values are written as a single int64.

    @code
    MemoryOutputStream buf;
    BinaryOutputArchive archive (buf);
    archive (myObject);
    @endcode

    @see BinaryInputArchive, SerialisationTraits
*/
class BinaryOutputArchive
{
public:
    /** Constructs an archive that writes to the given stream. */
    explicit BinaryOutputArchive (OutputStream& stream) noexcept
        : stream_ (stream)
    {
    }

    /** Returns nullopt - versioning is handled at the file-format level, not per-item. */
    std::optional<int> getVersion() const { return std::nullopt; }

    /** Serialise one or more values. */
    template <typename... Ts>
    bool operator() (Ts&&... ts)
    {
        return (doWrite (std::forward<Ts> (ts)) && ...);
    }

private:
    template <typename T>
    bool doWrite (T&& t)
    {
        using D = std::decay_t<T>;

        if constexpr (detail::is_named_v<D>)
        {
            return doWrite (t.value);
        }
        else if constexpr (detail::is_serialisation_size_v<D>)
        {
            return stream_.writeInt64 (static_cast<int64> (t.size));
        }
        else if constexpr (detail::serialisationKind<D> == detail::SerialisationKind::primitive)
        {
            return writePrimitive (t);
        }
        else
        {
            detail::doSave (*this, t);
            return true;
        }
    }

    template <typename T>
    bool writePrimitive (const T& t)
    {
        if constexpr (std::is_enum_v<T>)
        {
            return writePrimitive (static_cast<std::underlying_type_t<T>> (t));
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return stream_.writeBool (t);
        }
        else if constexpr (std::is_same_v<T, String>)
        {
            const auto len = static_cast<int32> (t.getNumBytesAsUTF8());
            return stream_.writeInt (len)
                && stream_.write (t.toRawUTF8(), static_cast<size_t> (len));
        }
        else if constexpr (std::is_integral_v<T>)
        {
            if constexpr (sizeof (T) == 1)
                return stream_.writeByte (static_cast<char> (t));
            else if constexpr (sizeof (T) == 2)
                return stream_.writeShort (static_cast<short> (t));
            else if constexpr (sizeof (T) == 4)
                return stream_.writeInt (static_cast<int> (t));
            else if constexpr (sizeof (T) == 8)
                return stream_.writeInt64 (static_cast<int64> (t));
            else
                static_assert (detail::delayStaticAssert<T>, "Unsupported integer width in BinaryOutputArchive");
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return stream_.writeFloat (t);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return stream_.writeDouble (t);
        }
        else
        {
            static_assert (detail::delayStaticAssert<T>, "Unsupported primitive type in BinaryOutputArchive");
        }

        return true;
    }

    OutputStream& stream_;
};

//==============================================================================
/**
    An archive that deserialises objects from a binary InputStream.

    Mirror of BinaryOutputArchive - reads data in the same order and format.
    SerialisationSize<T> values are read as int64 and assigned back to the
    size reference so that dynamically-sized containers can be resized before
    reading their elements.

    @code
    MemoryInputStream buf (data, size, false);
    BinaryInputArchive archive (buf);
    archive (myObject);
    @endcode

    @see BinaryOutputArchive, SerialisationTraits
*/
class BinaryInputArchive
{
public:
    /** Constructs an archive that reads from the given stream. */
    explicit BinaryInputArchive (InputStream& stream) noexcept
        : stream_ (stream)
    {
    }

    /** Returns nullopt - versioning is handled at the file-format level, not per-item. */
    std::optional<int> getVersion() const { return std::nullopt; }

    /** Deserialise one or more values. */
    template <typename... Ts>
    bool operator() (Ts&&... ts)
    {
        return (doRead (std::forward<Ts> (ts)) && ...);
    }

private:
    template <typename T>
    bool doRead (T&& t)
    {
        using D = std::decay_t<T>;

        if constexpr (detail::is_named_v<D>)
        {
            return doRead (t.value);
        }
        else if constexpr (detail::is_serialisation_size_v<D>)
        {
            using SizeT = std::remove_cv_t<std::remove_reference_t<decltype (t.size)>>;
            t.size = static_cast<SizeT> (stream_.readInt64());
            return true;
        }
        else if constexpr (detail::serialisationKind<D> == detail::SerialisationKind::primitive)
        {
            return readPrimitive (t);
        }
        else
        {
            detail::doLoad (*this, t);
            return true;
        }
    }

    template <typename T>
    bool readPrimitive (T& t)
    {
        if constexpr (std::is_enum_v<T>)
        {
            std::underlying_type_t<T> underlying {};
            readPrimitive (underlying);
            t = static_cast<T> (underlying);
            return true;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            t = stream_.readBool();
            return true;
        }
        else if constexpr (std::is_same_v<T, String>)
        {
            const auto len = stream_.readInt();
            if (len < 0)
                return false;
            MemoryBlock buf (static_cast<size_t> (len), false);
            if (stream_.read (buf.getData(), len) != len)
                return false;
            t = String::fromUTF8 (static_cast<const char*> (buf.getData()), len);
            return true;
        }
        else if constexpr (std::is_integral_v<T>)
        {
            if constexpr (sizeof (T) == 1)
                t = static_cast<T> (stream_.readByte());
            else if constexpr (sizeof (T) == 2)
                t = static_cast<T> (stream_.readShort());
            else if constexpr (sizeof (T) == 4)
                t = static_cast<T> (stream_.readInt());
            else if constexpr (sizeof (T) == 8)
                t = static_cast<T> (stream_.readInt64());
            else
                static_assert (detail::delayStaticAssert<T>, "Unsupported integer width in BinaryInputArchive");
            return true;
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            t = stream_.readFloat();
            return true;
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            t = stream_.readDouble();
            return true;
        }
        else
        {
            static_assert (detail::delayStaticAssert<T>, "Unsupported primitive type in BinaryInputArchive");
        }

        return true;
    }

    InputStream& stream_;
};

} // namespace yup
