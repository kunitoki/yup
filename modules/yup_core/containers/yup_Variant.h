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
    A variant class, that can be used to hold a range of primitive values.

    A var object can hold a range of simple primitive values, strings, or
    any kind of ReferenceCountedObject. The var class is intended to act like
    the kind of values used in dynamic scripting languages.

    You can save/load var objects either in a small, proprietary binary format
    using writeToStream()/readFromStream(), or as JSON by using the JSON class.

    @see JSON, DynamicObject

    @tags{Core}
*/
class YUP_API var
{
public:
    //==============================================================================
    /** This structure is passed to a NativeFunction callback, and contains invocation
        details about the function's arguments and context.
    */
    struct YUP_API NativeFunctionArgs
    {
        NativeFunctionArgs (const var& thisObject) noexcept;
        NativeFunctionArgs (const var& thisObject, const var* args, int numArgs) noexcept;

        const var& thisObject;
        const var* arguments;
        int numArguments;
    };

    using NativeFunction = std::function<var (const NativeFunctionArgs&)>;

    //==============================================================================
    /** Creates a void variant. */
    var() noexcept;

    /** Destructor. */
    ~var() noexcept;

    var (const var& valueToCopy);
    var (int value) noexcept;
    var (int64 value) noexcept;
    var (bool value) noexcept;
    var (double value) noexcept;
    var (const char* value);
    var (const wchar_t* value);
    var (const String& value);
    var (StringRef value);
    var (const Array<var>& value);
    var (const StringArray& value);
    var (ReferenceCountedObject* object);
    var (std::unique_ptr<DynamicObject> object);
    var (NativeFunction method) noexcept;
    var (const void* binaryData, size_t dataSize);
    var (const MemoryBlock& binaryData);

    var& operator= (const var& valueToCopy);
    var& operator= (int value);
    var& operator= (int64 value);
    var& operator= (bool value);
    var& operator= (double value);
    var& operator= (const char* value);
    var& operator= (const wchar_t* value);
    var& operator= (const String& value);
    var& operator= (StringRef value);
    var& operator= (const MemoryBlock& value);
    var& operator= (const Array<var>& value);
    var& operator= (ReferenceCountedObject* object);
    var& operator= (std::unique_ptr<DynamicObject> object);
    var& operator= (NativeFunction method);

    var (var&&) noexcept;
    var (String&&);
    var (MemoryBlock&&);
    var (Array<var>&&);
    var& operator= (var&&) noexcept;
    var& operator= (String&&);
    var& operator= (MemoryBlock&&);
    var& operator= (Array<var>&&);

    void swapWith (var& other) noexcept;

    /** Returns a var object that can be used where you need the javascript "undefined" value. */
    [[nodiscard]] static var undefined() noexcept;

    /** Creates an array var holding the given values.

        @code
        auto list = var::array ({ 1, "two", 3.0 });
        @endcode

        @see object
    */
    [[nodiscard]] static var array (std::initializer_list<var> items);

    /** Creates a var holding a new DynamicObject with the given properties.

        Values can be any var, including nested arrays and objects. If a name appears more
        than once, the last value wins.

        @code
        auto config = var::object ({ { "name", "yup" },
                                     { "size", var::array ({ 640, 480 }) } });
        @endcode

        @see array
    */
    [[nodiscard]] static var object (std::initializer_list<std::pair<Identifier, var>> properties);

    //==============================================================================
    operator int() const noexcept;
    operator int64() const noexcept;
    operator bool() const noexcept;
    operator float() const noexcept;
    operator double() const noexcept;
    operator String() const;
    [[nodiscard]] String toString() const;

    /** If this variant holds an array, this provides access to it.
        NOTE: Beware when you use this - the array pointer is only valid for the lifetime
        of the variant that returned it, so be very careful not to call this method on temporary
        var objects that are the return-value of a function, and which may go out of scope before
        you use the array!
    */
    [[nodiscard]] Array<var>* getArray() const noexcept;

    /** If this variant holds a memory block, this provides access to it.
        NOTE: Beware when you use this - the MemoryBlock pointer is only valid for the lifetime
        of the variant that returned it, so be very careful not to call this method on temporary
        var objects that are the return-value of a function, and which may go out of scope before
        you use the MemoryBlock!
    */
    [[nodiscard]] MemoryBlock* getBinaryData() const noexcept;

    [[nodiscard]] ReferenceCountedObject* getObject() const noexcept;
    [[nodiscard]] DynamicObject* getDynamicObject() const noexcept;

    //==============================================================================
    [[nodiscard]] bool isVoid() const noexcept;
    [[nodiscard]] bool isUndefined() const noexcept;
    [[nodiscard]] bool isInt() const noexcept;
    [[nodiscard]] bool isInt64() const noexcept;
    [[nodiscard]] bool isBool() const noexcept;
    [[nodiscard]] bool isDouble() const noexcept;
    [[nodiscard]] bool isString() const noexcept;
    [[nodiscard]] bool isObject() const noexcept;
    [[nodiscard]] bool isArray() const noexcept;
    [[nodiscard]] bool isBinaryData() const noexcept;
    [[nodiscard]] bool isMethod() const noexcept;

    /** Returns true if this var has the same value as the one supplied.
        Note that this ignores the type, so a string var "123" and an integer var with the
        value 123 are considered to be equal.

        Note that equality checking depends on the "wrapped" type of the object on which
        equals() is called. That means the following code will convert the right-hand-side
        argument to a string and compare the string values, because the object on the
        left-hand-side was initialised from a string:
        @code var ("123").equals (var (123)) @endcode
        However, the following code will convert the right-hand-side argument to a double
        and compare the values as doubles, because the object on the left-hand-side was
        initialised from a double:
        @code var (45.6).equals ("45.6000") @endcode

        @see equalsWithSameType
    */
    [[nodiscard]] bool equals (const var& other) const noexcept;

    /** Returns true if this var has the same value and type as the one supplied.
        This differs from equals() because e.g. "123" and 123 will be considered different.
        @see equals
    */
    [[nodiscard]] bool equalsWithSameType (const var& other) const noexcept;

    /** Returns true if this var has the same type as the one supplied. */
    [[nodiscard]] bool hasSameTypeAs (const var& other) const noexcept;

    /** Orders two var objects, providing the <, <=, > and >= operators.

        Two strings are compared lexically; any other mix of void, int, int64, bool, double and
        string values is compared numerically. The result is unordered when either side is
        undefined, an object, an array, binary data or a method, so all four relational
        operators return false for such values.
    */
    friend std::partial_ordering operator<=> (const var& a, const var& b) { return compare (a, b); }

    /** Returns a deep copy of this object.
        For simple types this just returns a copy, but if the object contains any arrays
        or DynamicObjects, they will be cloned (recursively).
    */
    [[nodiscard]] var clone() const noexcept;

    //==============================================================================
    /** If the var is an array, this returns the number of elements.
        If the var isn't actually an array, this will return 0.
    */
    [[nodiscard]] int size() const;

    /** Returns a pointer to the first element, if the var is an array.

        Together with end() this lets a range-based for loop iterate over an array var. A var
        that isn't an array behaves as an empty range.

        @code
        for (const auto& item : var::array ({ 1, 2, 3 }))
            total += (int) item;
        @endcode

        NOTE: like getArray(), the returned pointers are only valid while the array is alive and
        isn't resized, so don't modify the array while iterating it.
    */
    [[nodiscard]] var* begin() noexcept;

    /** Returns a pointer past the last element, if the var is an array. @see begin */
    [[nodiscard]] var* end() noexcept;

    /** Returns a pointer to the first element, if the var is an array. @see begin */
    [[nodiscard]] const var* begin() const noexcept;

    /** Returns a pointer past the last element, if the var is an array. @see begin */
    [[nodiscard]] const var* end() const noexcept;

    /** If the var is an array, this can be used to return one of its elements.
        To call this method, you must make sure that the var is actually an array, and
        that the index is a valid number. If these conditions aren't met, behaviour is
        undefined.
        For more control over the array's contents, you can call getArray() and manipulate
        it directly as an Array\<var\>.
    */
    [[nodiscard]] const var& operator[] (int arrayIndex) const;

    /** If the var is an array, this can be used to return one of its elements.
        To call this method, you must make sure that the var is actually an array, and
        that the index is a valid number. If these conditions aren't met, behaviour is
        undefined.
        For more control over the array's contents, you can call getArray() and manipulate
        it directly as an Array\<var\>.
    */
    var& operator[] (int arrayIndex);

    /** Appends an element to the var, converting it to an array if it isn't already one.
        If the var isn't an array, it will be converted to one, and if its value was non-void,
        this value will be kept as the first element of the new array. The parameter value
        will then be appended to it.
        For more control over the array's contents, you can call getArray() and manipulate
        it directly as an Array\<var\>.
    */
    void append (const var& valueToAppend);

    /** Appends an element to the var by moving it, converting the var to an array if it isn't
        already one.
        @see append
    */
    void append (var&& valueToAppend);

    /** Inserts an element to the var, converting it to an array if it isn't already one.
        If the var isn't an array, it will be converted to one, and if its value was non-void,
        this value will be kept as the first element of the new array. The parameter value
        will then be inserted into it.
        For more control over the array's contents, you can call getArray() and manipulate
        it directly as an Array\<var\>.
    */
    void insert (int index, const var& value);

    /** If the var is an array, this removes one of its elements.
        If the index is out-of-range or the var isn't an array, nothing will be done.
        For more control over the array's contents, you can call getArray() and manipulate
        it directly as an Array\<var\>.
    */
    void remove (int index);

    /** Treating the var as an array, this resizes it to contain the specified number of elements.
        If the var isn't an array, it will be converted to one, and if its value was non-void,
        this value will be kept as the first element of the new array before resizing.
        For more control over the array's contents, you can call getArray() and manipulate
        it directly as an Array\<var\>.
    */
    void resize (int numArrayElementsWanted);

    /** If the var is an array, this searches it for the first occurrence of the specified value,
        and returns its index.
        If the var isn't an array, or if the value isn't found, this returns -1.
    */
    [[nodiscard]] int indexOf (const var& value) const;

    //==============================================================================
    /** If this variant is an object, this returns one of its properties. */
    [[nodiscard]] const var& operator[] (const Identifier& propertyName) const;
    /** If this variant is an object, this returns one of its properties. */
    [[nodiscard]] const var& operator[] (const char* propertyName) const;
    /** If this variant is an object, this returns one of its properties. */
    [[nodiscard]] const var& operator[] (StringRef propertyName) const;
    /** If this variant is an object, this returns one of its properties, or a default fallback value if the property is not set. */
    [[nodiscard]] var getProperty (const Identifier& propertyName, const var& defaultReturnValue) const;
    /** Returns true if this variant is an object and if it has the given property. */
    [[nodiscard]] bool hasProperty (const Identifier& propertyName) const noexcept;
    /** If this variant is an object, this returns one of its methods, or a default fallback value if the property is not set. */
    [[nodiscard]] var getMethod (const Identifier& methodName, const var& defaultReturnValue) const;
    /** Returns true if this variant is an object and if it has the given method. */
    [[nodiscard]] bool hasMethod (const Identifier& methodName) const noexcept;

    /** Invokes a named method call with no arguments. */
    var call (const Identifier& method) const;
    /** Invokes a named method call with one argument. */
    var call (const Identifier& method, const var& arg1) const;
    /** Invokes a named method call with 2 arguments. */
    var call (const Identifier& method, const var& arg1, const var& arg2) const;
    /** Invokes a named method call with 3 arguments. */
    var call (const Identifier& method, const var& arg1, const var& arg2, const var& arg3);
    /** Invokes a named method call with 4 arguments. */
    var call (const Identifier& method, const var& arg1, const var& arg2, const var& arg3, const var& arg4) const;
    /** Invokes a named method call with 5 arguments. */
    var call (const Identifier& method, const var& arg1, const var& arg2, const var& arg3, const var& arg4, const var& arg5) const;
    /** Invokes a named method call with a list of arguments. */
    var invoke (const Identifier& method, const var* arguments, int numArguments) const;
    /** If this object is a method, this returns the function pointer. */
    [[nodiscard]] NativeFunction getNativeFunction() const;

    //==============================================================================
    /** Writes a binary representation of this value to a stream.
        The data can be read back later using readFromStream().
        @see JSON
    */
    void writeToStream (OutputStream& output) const;

    /** Reads back a stored binary representation of a value.
        The data in the stream must have been written using writeToStream(), or this
        will have unpredictable results. Values nested inside more than 128 levels of
        arrays are read as void.
        @see JSON
    */
    [[nodiscard]] static var readFromStream (InputStream& input);

private:
    //==============================================================================
    // The order matters: every type from string onwards owns a payload, every type from object
    // onwards holds a ReferenceCountedObject, and every type up to string (except undefined)
    // can be compared with the relational operators.
    enum class Type : uint8
    {
        void_,
        undefined,
        int_,
        int64_,
        boolean,
        double_,
        string,
        binary,
        object,
        array,
        method
    };

    static constexpr uint8 dynamicObjectFlag = 1;

    struct RefCountedArray;
    struct MethodHolder;

    union ValueUnion
    {
        int64 int64Value;
        int intValue;
        bool boolValue;
        double doubleValue;
        char stringValue[sizeof (String)];
        ReferenceCountedObject* objectValue;
        MemoryBlock* binaryValue;
    };

    friend bool canCompare (const var&, const var&);

    Type type = Type::void_;
    uint8 flags = 0;
    ValueUnion value {};

    bool isComparable() const noexcept { return type <= Type::string && type != Type::undefined; }

    static std::partial_ordering compare (const var&, const var&);

    const String* getString() const noexcept;

    // dest must hold a bitwise copy of source: this turns it into an owning copy
    static void acquirePayload (Type, ValueUnion& dest, const ValueUnion& source);
    static void releasePayload (Type, ValueUnion&) noexcept;
    static MemoryBlock* copyBinary (const MemoryBlock&);
    static void deleteBinary (MemoryBlock*) noexcept;

    int convertToInt() const noexcept;
    int64 convertToInt64() const noexcept;
    double convertToDouble() const noexcept;
    bool convertToBool() const noexcept;
    Array<var>* convertToArray();

    // This is needed to prevent the wrong constructor/operator being called
    var (const ReferenceCountedObject*) = delete;
    var& operator= (const ReferenceCountedObject*) = delete;
    var (const void*) = delete;
    var& operator= (const void*) = delete;
};

//==============================================================================
struct var::RefCountedArray final : public ReferenceCountedObject
{
    explicit RefCountedArray (const Array<var>& a)
        : array (a)
    {
        incReferenceCount();
    }

    explicit RefCountedArray (Array<var>&& a) noexcept
        : array (std::move (a))
    {
        incReferenceCount();
    }

    Array<var> array;
};

//==============================================================================
inline void var::acquirePayload (const Type t, ValueUnion& dest, const ValueUnion& source)
{
    if (t < Type::string)
        return;

    if (t >= Type::object)
    {
        if (dest.objectValue != nullptr)
            dest.objectValue->incReferenceCount();
    }
    else if (t == Type::string)
    {
        new (dest.stringValue) String (*unalignedPointerCast<const String*> (source.stringValue));
    }
    else
    {
        dest.binaryValue = copyBinary (*source.binaryValue);
    }
}

inline void var::releasePayload (const Type t, ValueUnion& v) noexcept
{
    if (t < Type::string)
        return;

    if (t >= Type::object)
    {
        if (v.objectValue != nullptr)
            v.objectValue->decReferenceCount();
    }
    else if (t == Type::string)
    {
        unalignedPointerCast<String*> (v.stringValue)->~String();
    }
    else
    {
        deleteBinary (v.binaryValue);
    }
}

//==============================================================================
inline var::var() noexcept {}

inline var::~var() noexcept
{
    releasePayload (type, value);
}

inline var::var (const var& valueToCopy)
    : type (valueToCopy.type)
    , flags (valueToCopy.flags)
    , value (valueToCopy.value)
{
    acquirePayload (type, value, valueToCopy.value);
}

inline var::var (var&& other) noexcept
    : type (other.type)
    , flags (other.flags)
    , value (other.value)
{
    other.type = Type::void_;
    other.flags = 0;
}

inline var::var (const int v) noexcept
    : type (Type::int_)
{
    value.intValue = v;
}

inline var::var (const int64 v) noexcept
    : type (Type::int64_)
{
    value.int64Value = v;
}

inline var::var (const bool v) noexcept
    : type (Type::boolean)
{
    value.boolValue = v;
}

inline var::var (const double v) noexcept
    : type (Type::double_)
{
    value.doubleValue = v;
}

inline var var::undefined() noexcept
{
    var v;
    v.type = Type::undefined;
    return v;
}

inline var& var::operator= (const var& valueToCopy)
{
    // Acquire the new payload before releasing ours, which may own the source (v = v or v = v[0])
    const auto newType = valueToCopy.type;
    const auto newFlags = valueToCopy.flags;
    auto newValue = valueToCopy.value;
    acquirePayload (newType, newValue, valueToCopy.value);

    releasePayload (type, value);

    type = newType;
    flags = newFlags;
    value = newValue;
    return *this;
}

inline var& var::operator= (var&& other) noexcept
{
    // Detach the source before releasing our payload, which may own the source (v = std::move (v[0]))
    const auto newType = other.type;
    const auto newFlags = other.flags;
    const auto newValue = other.value;
    other.type = Type::void_;
    other.flags = 0;

    releasePayload (type, value);

    type = newType;
    flags = newFlags;
    value = newValue;
    return *this;
}

inline var& var::operator= (const int v) { return *this = var (v); }

inline var& var::operator= (const int64 v) { return *this = var (v); }

inline var& var::operator= (const bool v) { return *this = var (v); }

inline var& var::operator= (const double v) { return *this = var (v); }

inline void var::swapWith (var& other) noexcept
{
    std::swap (type, other.type);
    std::swap (flags, other.flags);
    std::swap (value, other.value);
}

//==============================================================================
inline bool var::isVoid() const noexcept { return type == Type::void_; }

inline bool var::isUndefined() const noexcept { return type == Type::undefined; }

inline bool var::isInt() const noexcept { return type == Type::int_; }

inline bool var::isInt64() const noexcept { return type == Type::int64_; }

inline bool var::isBool() const noexcept { return type == Type::boolean; }

inline bool var::isDouble() const noexcept { return type == Type::double_; }

inline bool var::isString() const noexcept { return type == Type::string; }

inline bool var::isObject() const noexcept { return type == Type::object || type == Type::array; }

inline bool var::isArray() const noexcept { return type == Type::array; }

inline bool var::isBinaryData() const noexcept { return type == Type::binary; }

inline bool var::isMethod() const noexcept { return type == Type::method; }

inline bool var::hasSameTypeAs (const var& other) const noexcept { return type == other.type; }

inline bool var::equalsWithSameType (const var& other) const noexcept { return hasSameTypeAs (other) && equals (other); }

inline ReferenceCountedObject* var::getObject() const noexcept { return type == Type::object ? value.objectValue : nullptr; }

inline Array<var>* var::getArray() const noexcept { return type == Type::array ? &static_cast<RefCountedArray*> (value.objectValue)->array : nullptr; }

inline MemoryBlock* var::getBinaryData() const noexcept { return type == Type::binary ? value.binaryValue : nullptr; }

inline var* var::begin() noexcept { return isArray() ? getArray()->begin() : nullptr; }

inline var* var::end() noexcept { return isArray() ? getArray()->end() : nullptr; }

inline const var* var::begin() const noexcept { return isArray() ? getArray()->begin() : nullptr; }

inline const var* var::end() const noexcept { return isArray() ? getArray()->end() : nullptr; }

//==============================================================================
inline var::operator int() const noexcept { return type == Type::int_ ? value.intValue : convertToInt(); }

inline var::operator int64() const noexcept { return type == Type::int64_ ? value.int64Value : convertToInt64(); }

inline var::operator bool() const noexcept { return type == Type::boolean ? value.boolValue : convertToBool(); }

inline var::operator float() const noexcept { return (float) operator double(); }

inline var::operator double() const noexcept { return type == Type::double_ ? value.doubleValue : convertToDouble(); }

inline var::operator String() const { return toString(); }

/** Compares the values of two var objects, using the var::equals() comparison. */
YUP_API bool operator== (const var&, const var&);
/** Compares the values of two var objects, using the var::equals() comparison. */
YUP_API bool operator!= (const var&, const var&);

} // namespace yup
