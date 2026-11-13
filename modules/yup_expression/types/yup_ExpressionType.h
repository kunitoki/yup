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

//==============================================================================
/** Native routine executed by a CompiledExpression instruction.

    It processes @p numRows rows at once: every argument points to @p numRows
    contiguous values of the argument type, and @p output points to @p numRows
    already constructed values of the result type, which must be assigned.

    @param state    Opaque per-function state (for example the stored callable).
    @param output   The destination lanes.
    @param args     One pointer to the lanes of each argument.
    @param numRows  The number of rows to process.

    @see ExpressionContext::addFunction
*/
using ExpressionThunk = void (*) (const void* state, void* output, const void* const* args, int numRows);

//==============================================================================
/** View of an array input bound to an ExpressionState.

    Arrays are read-only for expressions, indexing is clamped to the valid range.

    @see ExpressionContext::addArray, ExpressionState::bindArray
*/
template <class T>
struct ExpressionArrayView
{
    const T* data = nullptr;
    int size = 0;
};

//==============================================================================
/** Describes a value type known to the expression language.

    Built-in types are `bool`, `int` (int32), `float`, `double` and `string` (yup::String).
    Custom C++ types are registered through ExpressionContext::addType, which returns an
    ExpressionTypeBuilder used to describe constructors, members, methods, operators and
    conversions.

    Registered types must be default constructible and copy assignable. Values of types that
    are not trivially copyable (like `string`) are supported, but evaluating programs that use
    them may allocate.

    @see ExpressionContext, ExpressionTypeBuilder

    @tags{Core}
*/
class YUP_API ExpressionType
{
public:
    //==============================================================================
    /** Creates a type descriptor for the C++ type T. */
    template <class T>
    static std::shared_ptr<ExpressionType> create (const String& name)
    {
        static_assert (std::is_default_constructible_v<T> && std::is_copy_assignable_v<T>,
                       "Expression types must be default constructible and copy assignable");

        auto type = std::shared_ptr<ExpressionType> (new ExpressionType (name, typeid (T), sizeof (T), alignof (T)));
        type->triviallyCopyable = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;
        type->constructFunction = &Operations<T>::construct;
        type->destroyFunction = &Operations<T>::destroy;
        type->copyFunction = &Operations<T>::copy;
        type->selectThunk = &Operations<T>::select;

        if constexpr (std::is_trivially_copyable_v<T>)
            type->equalsFunction = &Operations<T>::equalBytes;
        else if constexpr (std::equality_comparable<T>)
            type->equalsFunction = &Operations<T>::equal;

        return type;
    }

    /** Creates the array type descriptor for arrays of the given element type. */
    template <class T>
    static std::shared_ptr<ExpressionType> createArray (const ExpressionType& elementType)
    {
        jassert (elementType.is<T>());

        auto type = create<ExpressionArrayView<T>> (elementType.getName() + "[]");
        type->elementType = &elementType;
        type->indexThunk = &ArrayOperations<T>::index;
        type->lengthThunk = &ArrayOperations<T>::length;
        return type;
    }

    //==============================================================================
    /** The built-in `bool` type. */
    static const ExpressionType& boolType();
    /** The built-in `int` type (32 bit signed integer). */
    static const ExpressionType& intType();
    /** The built-in `float` type (32 bit floating point). */
    static const ExpressionType& floatType();
    /** The built-in `double` type (64 bit floating point). */
    static const ExpressionType& doubleType();
    /** The built-in `string` type (yup::String). */
    static const ExpressionType& stringType();

    /** Returns the built-in type matching the C++ type T, or nullptr if T is not a built-in type. */
    template <class T>
    static const ExpressionType* getBuiltin() noexcept
    {
        if constexpr (std::is_same_v<T, bool>)
            return &boolType();
        else if constexpr (std::is_same_v<T, int>)
            return &intType();
        else if constexpr (std::is_same_v<T, float>)
            return &floatType();
        else if constexpr (std::is_same_v<T, double>)
            return &doubleType();
        else if constexpr (std::is_same_v<T, String>)
            return &stringType();
        else
            return nullptr;
    }

    /** Returns the built-in type with the given name, or nullptr. */
    static const ExpressionType* findBuiltin (StringRef name) noexcept;

    //==============================================================================
    /** Returns the name used for this type in expressions and error messages. */
    const String& getName() const noexcept { return name; }

    /** Returns the C++ type this descriptor refers to. */
    std::type_index getCppType() const noexcept { return cppType; }

    /** Returns true if this descriptor refers to the C++ type T. */
    template <class T>
    bool is() const noexcept
    {
        return cppType == std::type_index (typeid (T));
    }

    /** Returns the size in bytes of a single value. */
    size_t getSize() const noexcept { return size; }

    /** Returns the alignment in bytes of a single value. */
    size_t getAlignment() const noexcept { return alignment; }

    /** Returns true if values can be copied with memcpy and never allocate. */
    bool isTriviallyCopyable() const noexcept { return triviallyCopyable; }

    /** Returns true for the built-in `bool`, `int`, `float` and `double` types. */
    bool isNumeric() const noexcept;

    /** Returns the element type for array types, nullptr otherwise. */
    const ExpressionType* getElementType() const noexcept { return elementType; }

    /** Returns the component names declared for swizzling, empty if none.
        @see ExpressionTypeBuilder::components
    */
    const StringArray& getComponentNames() const noexcept { return componentNames; }

    //==============================================================================
    /** Default constructs @p count consecutive values in uninitialised memory. */
    void construct (void* destination, int count) const { constructFunction (destination, count); }

    /** Destroys @p count consecutive values. */
    void destroy (void* destination, int count) const { destroyFunction (destination, count); }

    /** Assigns @p count values, using the given byte strides (a source stride of 0 broadcasts a single value). */
    void copy (void* destination, size_t destinationStride, const void* source, size_t sourceStride, int count) const
    {
        copyFunction (destination, destinationStride, source, sourceStride, count);
    }

    /** Returns true if two values are known to be interchangeable.

        Trivially copyable values are compared bitwise (so `0.0` and `-0.0` differ), other values with
        their `operator==` when they have one. Returns false when values can't be compared.
    */
    bool areEqual (const void* a, const void* b) const { return equalsFunction != nullptr && equalsFunction (a, b); }

    /** Allocates and default constructs a single value, owned by the returned pointer. */
    std::shared_ptr<void> createValue() const;

    /** Returns the thunk implementing `condition ? a : b` for this type. */
    ExpressionThunk getSelectThunk() const noexcept { return selectThunk; }

    /** Returns the thunk implementing `array[index]` for array types. */
    ExpressionThunk getIndexThunk() const noexcept { return indexThunk; }

    /** Returns the thunk implementing `len (array)` for array types. */
    ExpressionThunk getLengthThunk() const noexcept { return lengthThunk; }

private:
    //==============================================================================
    template <class T>
    struct Operations
    {
        static void construct (void* destination, int count)
        {
            std::uninitialized_value_construct_n (static_cast<T*> (destination), count);
        }

        static void destroy (void* destination, int count)
        {
            std::destroy_n (static_cast<T*> (destination), count);
        }

        static void copy (void* destination, size_t destinationStride, const void* source, size_t sourceStride, int count)
        {
            auto* d = static_cast<uint8*> (destination);
            auto* s = static_cast<const uint8*> (source);

            for (int i = 0; i < count; ++i)
                *reinterpret_cast<T*> (d + (size_t) i * destinationStride) = *reinterpret_cast<const T*> (s + (size_t) i * sourceStride);
        }

        static bool equalBytes (const void* a, const void* b)
        {
            return std::memcmp (a, b, sizeof (T)) == 0;
        }

        static bool equal (const void* a, const void* b)
        {
            return *static_cast<const T*> (a) == *static_cast<const T*> (b);
        }

        static void select (const void*, void* output, const void* const* args, int numRows)
        {
            auto* result = static_cast<T*> (output);
            auto* condition = static_cast<const bool*> (args[0]);
            auto* a = static_cast<const T*> (args[1]);
            auto* b = static_cast<const T*> (args[2]);

            for (int i = 0; i < numRows; ++i)
                result[i] = condition[i] ? a[i] : b[i];
        }
    };

    template <class T>
    struct ArrayOperations
    {
        static void index (const void*, void* output, const void* const* args, int numRows)
        {
            auto* result = static_cast<T*> (output);
            auto* arrays = static_cast<const ExpressionArrayView<T>*> (args[0]);
            auto* indices = static_cast<const int*> (args[1]);

            for (int i = 0; i < numRows; ++i)
                result[i] = arrays[i].size > 0 ? arrays[i].data[jlimit (0, arrays[i].size - 1, indices[i])] : T();
        }

        static void length (const void*, void* output, const void* const* args, int numRows)
        {
            auto* result = static_cast<int*> (output);
            auto* arrays = static_cast<const ExpressionArrayView<T>*> (args[0]);

            for (int i = 0; i < numRows; ++i)
                result[i] = arrays[i].size;
        }
    };

    ExpressionType (const String& typeName, const std::type_info& type, size_t typeSize, size_t typeAlignment);

    template <class>
    friend class ExpressionTypeBuilder;

    String name;
    std::type_index cppType;
    size_t size = 0, alignment = 0;
    bool triviallyCopyable = false;
    void (*constructFunction) (void*, int) = nullptr;
    void (*destroyFunction) (void*, int) = nullptr;
    void (*copyFunction) (void*, size_t, const void*, size_t, int) = nullptr;
    bool (*equalsFunction) (const void*, const void*) = nullptr;
    ExpressionThunk selectThunk = nullptr;
    ExpressionThunk indexThunk = nullptr;
    ExpressionThunk lengthThunk = nullptr;
    const ExpressionType* elementType = nullptr;
    StringArray componentNames;

    YUP_DECLARE_NON_COPYABLE (ExpressionType)
};

} // namespace yup
