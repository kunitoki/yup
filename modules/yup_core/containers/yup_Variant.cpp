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

enum VariantStreamMarkers
{
    varMarker_Int = 1,
    varMarker_BoolTrue = 2,
    varMarker_BoolFalse = 3,
    varMarker_Double = 4,
    varMarker_String = 5,
    varMarker_Int64 = 6,
    varMarker_Array = 7,
    varMarker_Binary = 8,
    varMarker_Undefined = 9
};

//==============================================================================
struct var::MethodHolder final : public ReferenceCountedObject
{
    explicit MethodHolder (NativeFunction&& f) noexcept
        : function (std::move (f))
    {
        incReferenceCount();
    }

    NativeFunction function;
};

//==============================================================================
inline const String* var::getString() const noexcept
{
    return unalignedPointerCast<const String*> (value.stringValue);
}

MemoryBlock* var::copyBinary (const MemoryBlock& source)
{
    return new MemoryBlock (source);
}

void var::deleteBinary (MemoryBlock* block) noexcept
{
    delete block;
}

//==============================================================================
var::var (NativeFunction m) noexcept
    : type (Type::method)
{
    value.objectValue = new MethodHolder (std::move (m));
}

var::var (const Array<var>& v)
    : type (Type::array)
{
    value.objectValue = new RefCountedArray (v);
}

var::var (Array<var>&& v)
    : type (Type::array)
{
    value.objectValue = new RefCountedArray (std::move (v));
}

var::var (const String& v)
    : type (Type::string)
{
    new (value.stringValue) String (v);
}

var::var (String&& v)
    : type (Type::string)
{
    new (value.stringValue) String (std::move (v));
}

var::var (StringRef v)
    : type (Type::string)
{
    new (value.stringValue) String (v);
}

var::var (const char* const v)
    : type (Type::string)
{
    new (value.stringValue) String (v);
}

var::var (const wchar_t* const v)
    : type (Type::string)
{
    new (value.stringValue) String (v);
}

var::var (const void* v, size_t sz)
    : type (Type::binary)
{
    value.binaryValue = new MemoryBlock (v, sz);
}

var::var (const MemoryBlock& v)
    : type (Type::binary)
{
    value.binaryValue = new MemoryBlock (v);
}

var::var (MemoryBlock&& v)
    : type (Type::binary)
{
    value.binaryValue = new MemoryBlock (std::move (v));
}

var::var (const StringArray& v)
    : type (Type::array)
{
    Array<var> strings;
    strings.ensureStorageAllocated (v.size());

    for (auto& i : v)
        strings.add (var (i));

    value.objectValue = new RefCountedArray (std::move (strings));
}

var::var (ReferenceCountedObject* const object)
    : type (Type::object)
    , flags (dynamic_cast<DynamicObject*> (object) != nullptr ? dynamicObjectFlag : (uint8) 0)
{
    value.objectValue = object;

    if (object != nullptr)
        object->incReferenceCount();
}

var::var (std::unique_ptr<DynamicObject> object)
    : type (Type::object)
    , flags (object != nullptr ? dynamicObjectFlag : (uint8) 0)
{
    value.objectValue = object.release();

    if (value.objectValue != nullptr)
        value.objectValue->incReferenceCount();
}

//==============================================================================
var& var::operator= (const char* const v) { return *this = var (v); }

var& var::operator= (const wchar_t* const v) { return *this = var (v); }

var& var::operator= (const String& v) { return *this = var (v); }

var& var::operator= (String&& v) { return *this = var (std::move (v)); }

var& var::operator= (StringRef v) { return *this = var (v); }

var& var::operator= (const MemoryBlock& v) { return *this = var (v); }

var& var::operator= (MemoryBlock&& v) { return *this = var (std::move (v)); }

var& var::operator= (const Array<var>& v) { return *this = var (v); }

var& var::operator= (Array<var>&& v) { return *this = var (std::move (v)); }

var& var::operator= (ReferenceCountedObject* v) { return *this = var (v); }

var& var::operator= (std::unique_ptr<DynamicObject> v) { return *this = var (std::move (v)); }

var& var::operator= (NativeFunction v) { return *this = var (std::move (v)); }

//==============================================================================
var var::array (std::initializer_list<var> items)
{
    return var (Array<var> (items));
}

var var::object (std::initializer_list<std::pair<Identifier, var>> properties)
{
    auto result = std::make_unique<DynamicObject>();

    for (const auto& [name, propertyValue] : properties)
        result->setProperty (name, propertyValue);

    return var (std::move (result));
}

//==============================================================================
int var::convertToInt() const noexcept
{
    switch (type)
    {
        case Type::int_:
            return value.intValue;
        case Type::int64_:
            return (int) value.int64Value;
        case Type::boolean:
            return value.boolValue ? 1 : 0;
        case Type::double_:
            return (int) value.doubleValue;
        case Type::string:
            return getString()->getIntValue();
        default:
            return 0;
    }
}

int64 var::convertToInt64() const noexcept
{
    switch (type)
    {
        case Type::int_:
            return (int64) value.intValue;
        case Type::int64_:
            return value.int64Value;
        case Type::boolean:
            return value.boolValue ? 1 : 0;
        case Type::double_:
            return (int64) value.doubleValue;
        case Type::string:
            return getString()->getLargeIntValue();
        default:
            return 0;
    }
}

double var::convertToDouble() const noexcept
{
    switch (type)
    {
        case Type::int_:
            return (double) value.intValue;
        case Type::int64_:
            return (double) value.int64Value;
        case Type::boolean:
            return value.boolValue ? 1.0 : 0.0;
        case Type::double_:
            return value.doubleValue;
        case Type::string:
            return getString()->getDoubleValue();
        default:
            return 0.0;
    }
}

bool var::convertToBool() const noexcept
{
    switch (type)
    {
        case Type::int_:
            return value.intValue != 0;
        case Type::int64_:
            return value.int64Value != 0;
        case Type::boolean:
            return value.boolValue;
        case Type::double_:
            return ! exactlyEqual (value.doubleValue, 0.0);

        case Type::string:
        {
            if (getString()->getIntValue() != 0)
                return true;

            const auto trimmed = getString()->trim();
            return trimmed.equalsIgnoreCase ("true") || trimmed.equalsIgnoreCase ("yes");
        }

        case Type::object:
        case Type::array:
        case Type::method:
            return value.objectValue != nullptr;

        default:
            return false;
    }
}

String var::toString() const
{
    switch (type)
    {
        case Type::undefined:
            return "undefined";
        case Type::int_:
            return String (value.intValue);
        case Type::int64_:
            return String (value.int64Value);
        case Type::boolean:
            return String::charToString (value.boolValue ? (yup_wchar) '1' : (yup_wchar) '0');
        case Type::double_:
            return serialiseDouble (value.doubleValue);
        case Type::string:
            return *getString();
        case Type::object:
            return "Object 0x" + String::toHexString ((int) (pointer_sized_int) value.objectValue);
        case Type::array:
            return "[Array]";
        case Type::binary:
            return value.binaryValue->toBase64Encoding();
        case Type::method:
            return "Method";
        default:
            return {};
    }
}

//==============================================================================
bool var::equals (const var& other) const noexcept
{
    // Same-type integers are the common case, keep them clear of the heavier paths below
    if (type == other.type)
    {
        if (type == Type::int_)
            return value.intValue == other.value.intValue;

        if (type == Type::int64_)
            return value.int64Value == other.value.int64Value;
    }

    switch (type)
    {
        case Type::void_:
        case Type::undefined:
            return other.type == Type::void_ || other.type == Type::undefined;

        case Type::int_:
            if (other.type == Type::int64_)
                return (int64) value.intValue == other.value.int64Value;

            // Defers to the wider type, so that e.g. var (1).equals (var (1.0)) compares as doubles
            if (other.type == Type::double_ || other.type == Type::string)
                return other.equals (*this);

            return (int) other == value.intValue;

        case Type::int64_:
            if (other.type == Type::double_ || other.type == Type::string)
                return other.equals (*this);

            return (int64) other == value.int64Value;

        case Type::boolean:
            return (bool) other == value.boolValue;

        case Type::double_:
            return std::abs ((double) other - value.doubleValue) < std::numeric_limits<double>::epsilon();

        case Type::string:
            return other.type == Type::string ? *other.getString() == *getString()
                                              : other.toString() == *getString();

        case Type::object:
            return other.getObject() == value.objectValue;

        case Type::array:
        {
            const auto* thisArray = getArray();
            const auto* otherArray = other.getArray();
            return otherArray != nullptr && (otherArray == thisArray || *otherArray == *thisArray);
        }

        case Type::binary:
            return other.type == Type::binary && *other.value.binaryValue == *value.binaryValue;

        case Type::method:
            return other.type == Type::method && other.value.objectValue == value.objectValue;
    }

    return false;
}

bool canCompare (const var& v1, const var& v2)
{
    return v1.isComparable() && v2.isComparable();
}

std::partial_ordering var::compare (const var& v1, const var& v2)
{
    if (! canCompare (v1, v2))
        return std::partial_ordering::unordered;

    if (v1.isString() && v2.isString())
        return v1.toString().compare (v2.toString()) <=> 0;

    const auto diff = static_cast<double> (v1) - static_cast<double> (v2);

    if (exactlyEqual (diff, 0.0))
        return std::partial_ordering::equivalent;

    return diff < 0 ? std::partial_ordering::less : std::partial_ordering::greater;
}

bool operator== (const var& v1, const var& v2) { return v1.equals (v2); }

bool operator!= (const var& v1, const var& v2) { return ! v1.equals (v2); }

//==============================================================================
var var::clone() const noexcept
{
    switch (type)
    {
        case Type::object:
            if (auto* d = getDynamicObject())
                return d->clone();

            jassertfalse; // can only clone DynamicObjects!
            return {};

        case Type::array:
        {
            const auto& source = *getArray();

            Array<var> arrayCopy;
            arrayCopy.ensureStorageAllocated (source.size());

            for (const auto& i : source)
                arrayCopy.add (i.clone());

            return var (std::move (arrayCopy));
        }

        case Type::method:
            return var (getNativeFunction());

        default:
            return *this;
    }
}

//==============================================================================
const var& var::operator[] (const Identifier& propertyName) const
{
    if (auto* o = getDynamicObject())
        return o->getProperty (propertyName);

    return getNullVarRef();
}

const var& var::operator[] (const char* const propertyName) const
{
    return operator[] (Identifier (propertyName));
}

const var& var::operator[] (StringRef propertyName) const
{
    return operator[] (Identifier (propertyName));
}

var var::getProperty (const Identifier& propertyName, const var& defaultReturnValue) const
{
    if (auto* o = getDynamicObject())
        return o->getProperties().getWithDefault (propertyName, defaultReturnValue);

    return defaultReturnValue;
}

bool var::hasProperty (const Identifier& propertyName) const noexcept
{
    if (auto* o = getDynamicObject())
        return o->hasProperty (propertyName);

    return false;
}

var var::getMethod (const Identifier& methodName, const var& defaultReturnValue) const
{
    if (auto* o = getDynamicObject())
    {
        if (auto method = o->getProperties().getWithDefault (methodName, var()); method.isMethod())
            return method;
    }

    return defaultReturnValue;
}

bool var::hasMethod (const Identifier& methodName) const noexcept
{
    if (auto* o = getDynamicObject())
        return o->hasMethod (methodName);

    return false;
}

var::NativeFunction var::getNativeFunction() const
{
    return isMethod() ? static_cast<MethodHolder*> (value.objectValue)->function : nullptr;
}

var var::invoke (const Identifier& method, const var* arguments, int numArguments) const
{
    if (auto* o = getDynamicObject())
        return o->invokeMethod (method, var::NativeFunctionArgs (*this, arguments, numArguments));

    return {};
}

var var::call (const Identifier& method) const
{
    return invoke (method, nullptr, 0);
}

var var::call (const Identifier& method, const var& arg1) const
{
    return invoke (method, &arg1, 1);
}

var var::call (const Identifier& method, const var& arg1, const var& arg2) const
{
    var args[] = { arg1, arg2 };
    return invoke (method, args, 2);
}

var var::call (const Identifier& method, const var& arg1, const var& arg2, const var& arg3)
{
    var args[] = { arg1, arg2, arg3 };
    return invoke (method, args, 3);
}

var var::call (const Identifier& method, const var& arg1, const var& arg2, const var& arg3, const var& arg4) const
{
    var args[] = { arg1, arg2, arg3, arg4 };
    return invoke (method, args, 4);
}

var var::call (const Identifier& method, const var& arg1, const var& arg2, const var& arg3, const var& arg4, const var& arg5) const
{
    var args[] = { arg1, arg2, arg3, arg4, arg5 };
    return invoke (method, args, 5);
}

//==============================================================================
int var::size() const
{
    if (auto array = getArray())
        return array->size();

    return 0;
}

const var& var::operator[] (int arrayIndex) const
{
    auto array = getArray();

    // When using this method, the var must actually be an array, and the index
    // must be in-range!
    jassert (array != nullptr && isPositiveAndBelow (arrayIndex, array->size()));

    return array->getReference (arrayIndex);
}

var& var::operator[] (int arrayIndex)
{
    auto array = getArray();

    // When using this method, the var must actually be an array, and the index
    // must be in-range!
    jassert (array != nullptr && isPositiveAndBelow (arrayIndex, array->size()));

    return array->getReference (arrayIndex);
}

Array<var>* var::convertToArray()
{
    if (auto array = getArray())
        return array;

    Array<var> tempVar;

    if (! isVoid())
        tempVar.add (std::move (*this));

    *this = var (std::move (tempVar));
    return getArray();
}

void var::append (const var& n)
{
    convertToArray()->add (n);
}

void var::append (var&& n)
{
    convertToArray()->add (std::move (n));
}

void var::remove (const int index)
{
    if (auto array = getArray())
        array->remove (index);
}

void var::insert (const int index, const var& n)
{
    convertToArray()->insert (index, n);
}

void var::resize (const int numArrayElementsWanted)
{
    convertToArray()->resize (numArrayElementsWanted);
}

int var::indexOf (const var& n) const
{
    if (auto array = getArray())
        return array->indexOf (n);

    return -1;
}

//==============================================================================
void var::writeToStream (OutputStream& output) const
{
    switch (type)
    {
        case Type::void_:
            output.writeCompressedInt (0);
            break;

        case Type::undefined:
            output.writeCompressedInt (1);
            output.writeByte (varMarker_Undefined);
            break;

        case Type::int_:
            output.writeCompressedInt (5);
            output.writeByte (varMarker_Int);
            output.writeInt (value.intValue);
            break;

        case Type::int64_:
            output.writeCompressedInt (9);
            output.writeByte (varMarker_Int64);
            output.writeInt64 (value.int64Value);
            break;

        case Type::boolean:
            output.writeCompressedInt (1);
            output.writeByte (value.boolValue ? (char) varMarker_BoolTrue : (char) varMarker_BoolFalse);
            break;

        case Type::double_:
            output.writeCompressedInt (9);
            output.writeByte (varMarker_Double);
            output.writeDouble (value.doubleValue);
            break;

        case Type::string:
            // Marker plus the UTF-8 bytes including the terminator
            output.writeCompressedInt ((int) getString()->getNumBytesAsUTF8() + 2);
            output.writeByte (varMarker_String);
            output.writeString (*getString());
            break;

        case Type::array:
        {
            const auto& array = *getArray();

            MemoryOutputStream buffer (512);
            buffer.writeCompressedInt (array.size());

            for (const auto& i : array)
                i.writeToStream (buffer);

            output.writeCompressedInt (1 + (int) buffer.getDataSize());
            output.writeByte (varMarker_Array);
            output << buffer;
            break;
        }

        case Type::binary:
            output.writeCompressedInt (1 + (int) value.binaryValue->getSize());
            output.writeByte (varMarker_Binary);
            output << *value.binaryValue;
            break;

        case Type::object:
        case Type::method:
            jassertfalse; // Can't write an object or a method to a stream!
            output.writeCompressedInt (0);
            break;
    }
}

static var readVarFromStream (InputStream& input, int depth)
{
    // Kept well below the 1024 used for parsing so deep input cannot exhaust small thread stacks such as WebAssembly's
    constexpr int maxNestingDepth = 128;

    const int numBytes = input.readCompressedInt();

    if (depth > maxNestingDepth)
    {
        if (numBytes > 0)
            input.skipNextBytes (numBytes);

        return {};
    }

    if (numBytes > 0)
    {
        switch (input.readByte())
        {
            case varMarker_Int:
                return var (input.readInt());
            case varMarker_Int64:
                return var (input.readInt64());
            case varMarker_BoolTrue:
                return var (true);
            case varMarker_BoolFalse:
                return var (false);
            case varMarker_Double:
                return var (input.readDouble());

            case varMarker_String:
            {
                MemoryOutputStream mo;
                mo.writeFromInputStream (input, numBytes - 1);
                return var (mo.toUTF8());
            }

            case varMarker_Binary:
            {
                MemoryOutputStream mo;
                mo.writeFromInputStream (input, numBytes - 1);
                return var (mo.getMemoryBlock());
            }

            case varMarker_Array:
            {
                Array<var> destArray;

                for (int i = input.readCompressedInt(); --i >= 0 && ! input.isExhausted();)
                    destArray.add (readVarFromStream (input, depth + 1));

                return var (std::move (destArray));
            }

            default:
                input.skipNextBytes (numBytes - 1);
                break;
        }
    }

    return {};
}

var var::readFromStream (InputStream& input)
{
    return readVarFromStream (input, 0);
}

var::NativeFunctionArgs::NativeFunctionArgs (const var& t) noexcept
    : NativeFunctionArgs (t, nullptr, 0)
{
}

var::NativeFunctionArgs::NativeFunctionArgs (const var& t, const var* args, int numArgs) noexcept
    : thisObject (t)
    , arguments (args)
    , numArguments (numArgs)
{
}

} // namespace yup
