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

namespace yup
{

//==============================================================================
/** A stable identifier for each Node type, without RTTI.

    The id is a 64-bit FNV-1a hash of the name of the type, computed at compile time, so it
    is the same in every run, every module and every build made with the same compiler. It
    can be stored, for example to save scenes, and the Scene uses it to find the NodePool of
    a type.

    The name is the fully qualified name of the type as spelled by the compiler, for example
    "yup::MeshNode". Renaming the type, moving it to another namespace or switching to a
    compiler that spells templates differently changes it. A type can pin its name, and so
    its id, by declaring it:

    @code
    class Projectile : public yup::Node
    {
    public:
        static constexpr std::string_view nodeTypeName = "mygame.Projectile";
    };
    @endcode

    Two types hashing to the same id are detected by the Scene in debug builds: give one of
    them a nodeTypeName to resolve it.

    @see NodePool, Scene
*/
class YUP_API NodeTypeId
{
public:
    //==============================================================================
    /** Returns the name of a type: its nodeTypeName if it declares one, else the qualified
        name spelled by the compiler.
    */
    template <class T>
    static constexpr std::string_view getName() noexcept
    {
        if constexpr (requires { std::string_view { T::nodeTypeName }; })
            return T::nodeTypeName;
        else
            return getCompilerName<T>();
    }

    /** Returns the id of a type, the hash of its name. */
    template <class T>
    static constexpr uint64 get() noexcept
    {
        return hash (getName<T>());
    }

    /** Returns a small dense index for a type, assigned on first use.

        Unlike get(), the index depends on the order types are first used in a run, so it must
        never be stored. Scenes use it to find a pool with an array access.
    */
    template <class T>
    static uint32 getIndex() noexcept
    {
        static const uint32 index = nextIndex();
        return index;
    }

    /** Returns the 64-bit FNV-1a hash of a name, the id of a type with that name. */
    static constexpr uint64 hash (std::string_view name) noexcept
    {
        uint64 result = 14695981039346656037ull;

        for (const auto c : name)
        {
            result ^= static_cast<uint8> (c);
            result *= 1099511628211ull;
        }

        return result;
    }

private:
    //==============================================================================
    static uint32 nextIndex() noexcept
    {
        static std::atomic<uint32> counter { 0 };
        return counter.fetch_add (1, std::memory_order_relaxed);
    }

    template <class T>
    static constexpr std::string_view getCompilerName() noexcept
    {
#if defined(_MSC_VER) && ! defined(__clang__)
        constexpr std::string_view signature = __FUNCSIG__;
        constexpr std::string_view prefix = "getCompilerName<";
        constexpr auto first = signature.find (prefix) + prefix.size();
        constexpr auto last = signature.rfind (">(void)");
#else
        constexpr std::string_view signature = __PRETTY_FUNCTION__;
        constexpr std::string_view prefix = "T = ";
        constexpr auto first = signature.find (prefix) + prefix.size();
        constexpr auto last = signature.find_first_of (";]", first);
#endif
        constexpr auto name = signature.substr (first, last - first);

        // MSVC spells "class yup::MeshNode", the others "yup::MeshNode"
        for (const std::string_view keyword : { std::string_view ("class "), std::string_view ("struct ") })
        {
            if (name.substr (0, keyword.size()) == keyword)
                return name.substr (keyword.size());
        }

        return name;
    }
};

} // namespace yup
