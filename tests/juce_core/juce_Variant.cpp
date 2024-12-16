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

#include <gtest/gtest.h>

#include <juce_core/juce_core.h>

#include <memory>
#include <functional>
#include <string>
#include <sstream>

using namespace juce;

class VariantTests : public ::testing::Test
{
protected:
    // Helper function to create a var from a string
    var makeVar (const std::string& value)
    {
        return var (String (value));
    }

    // Helper function to create a var::NativeFunction
    var::NativeFunction createNativeFunction (std::function<var (const var::NativeFunctionArgs&)> func)
    {
        return var::NativeFunction (func);
    }

    // Helper function to compare two vars
    bool varsAreEqual (const var& a, const var& b)
    {
        return a.equals (b);
    }
};

// Test default constructor
TEST_F (VariantTests, DefaultConstructor)
{
    var v;
    EXPECT_TRUE (v.isVoid());
    EXPECT_FALSE (v.isUndefined());
    EXPECT_EQ (v, var());
}

// Test constructor with int
TEST_F (VariantTests, IntConstructor)
{
    var v (42);
    EXPECT_TRUE (v.isInt());
    EXPECT_EQ (static_cast<int> (v), 42);
}

// Test constructor with int64
TEST_F (VariantTests, Int64Constructor)
{
    int64 largeValue = 123456789012345;
    var v (largeValue);
    EXPECT_TRUE (v.isInt64());
    EXPECT_EQ (static_cast<int64> (v), largeValue);
}

// Test constructor with bool
TEST_F (VariantTests, BoolConstructor)
{
    var v (true);
    EXPECT_TRUE (v.isBool());
    EXPECT_EQ (static_cast<bool> (v), true);
}

// Test constructor with double
TEST_F (VariantTests, DoubleConstructor)
{
    var v (3.14159);
    EXPECT_TRUE (v.isDouble());
    EXPECT_EQ (static_cast<double> (v), 3.14159);
}

// Test constructor with const char*
TEST_F (VariantTests, CStrConstructor)
{
    const char* text = "Hello, World!";
    var v (text);
    EXPECT_TRUE (v.isString());
    EXPECT_EQ (v.toString(), String (text));
}

// Test constructor with const wchar_t*
TEST_F (VariantTests, WCStrConstructor)
{
    const wchar_t* text = L"Wide Hello!";
    var v (text);
    EXPECT_TRUE (v.isString());
    EXPECT_EQ (v.toString(), String (text));
}

// Test constructor with String
TEST_F (VariantTests, StringConstructor)
{
    var x1 (String ("text"));
    EXPECT_TRUE (x1.isString());
    EXPECT_EQ (x1.toString(), "text");

    var x2 (StringRef ("text"));
    EXPECT_TRUE (x2.isString());
    EXPECT_EQ (x2.toString(), "text");
}

// Test copy constructor
TEST_F (VariantTests, CopyConstructor)
{
    var original (100);
    var copy (original);
    EXPECT_TRUE (copy.isInt());
    EXPECT_EQ (static_cast<int> (copy), 100);
}

// Test move constructor
TEST_F (VariantTests, MoveConstructor)
{
    var original (200);
    var moved (std::move (original));
    EXPECT_TRUE (moved.isInt());
    EXPECT_EQ (static_cast<int> (moved), 200);
}

// Test copy assignment
TEST_F (VariantTests, CopyAssignment)
{
    var original (300);
    var copy;
    copy = original;
    EXPECT_TRUE (copy.isInt());
    EXPECT_EQ (static_cast<int> (copy), 300);
}

// Test move assignment
TEST_F (VariantTests, MoveAssignment)
{
    var original (400);
    var moved;
    moved = std::move (original);
    EXPECT_TRUE (moved.isInt());
    EXPECT_EQ (static_cast<int> (moved), 400);
}

// Test assignment operators
TEST_F (VariantTests, AssignmentOperators)
{
    var v;
    v = 10;
    EXPECT_TRUE (v.isInt());
    EXPECT_EQ (static_cast<int> (v), 10);

    v = 20.5;
    EXPECT_TRUE (v.isDouble());
    EXPECT_EQ (static_cast<double> (v), 20.5);

    v = true;
    EXPECT_TRUE (v.isBool());
    EXPECT_EQ (static_cast<bool> (v), true);

    v = "Test String";
    EXPECT_TRUE (v.isString());
    EXPECT_EQ (v.toString(), String ("Test String"));

    String str ("Another String");
    v = str;
    EXPECT_TRUE (v.isString());
    EXPECT_EQ (v.toString(), str);
}

// Test equality operators
TEST_F (VariantTests, EqualityOperators)
{
    var v1 (50);
    var v2 (50);
    var v3 (60);
    var v4 ("50");
    var v5 (true);

    EXPECT_TRUE (v1 == v2);
    EXPECT_TRUE (v1.equals (v2));
    EXPECT_TRUE (v1.equalsWithSameType (v2));

    EXPECT_FALSE (v1 == v3);
    EXPECT_FALSE (v1.equals (v3));
    EXPECT_FALSE (v1.equalsWithSameType (v3));

    EXPECT_TRUE (v1 == v4);
    EXPECT_TRUE (v1.equals (v4));
    EXPECT_FALSE (v1.equalsWithSameType (v4));

    EXPECT_FALSE (v1 == v5);
    EXPECT_FALSE (v1.equals (v5));
    EXPECT_FALSE (v1.equalsWithSameType (v5));

    EXPECT_TRUE (v1 != v3);
    EXPECT_FALSE (v1 != v4);
    EXPECT_TRUE (v1 != v5);
    EXPECT_FALSE (v1 != v2);
}

// Test isType methods
TEST_F (VariantTests, IsTypeMethods)
{
    var vVoid;
    var vUndefined = var::undefined();
    var vInt (1);
    var vInt64 (int64 (2));
    var vBool (true);
    var vDouble (3.14);
    var vString ("test");
    var vArray { Array<var>() };
    var vBinaryData { MemoryBlock() };
    var vObject { new DynamicObject() };
    var vMethod { createNativeFunction ([] (const var::NativeFunctionArgs&) -> var
                                        {
                                            return var();
                                        }) };

    EXPECT_TRUE (vVoid.isVoid());
    EXPECT_FALSE (vVoid.isUndefined());

    EXPECT_FALSE (vUndefined.isVoid());
    EXPECT_TRUE (vUndefined.isUndefined());

    EXPECT_TRUE (vInt.isInt());
    EXPECT_FALSE (vInt.isInt64());
    EXPECT_FALSE (vInt.isBool());
    EXPECT_FALSE (vInt.isDouble());
    EXPECT_FALSE (vInt.isString());
    EXPECT_FALSE (vInt.isArray());
    EXPECT_FALSE (vInt.isBinaryData());
    EXPECT_FALSE (vInt.isObject());
    EXPECT_FALSE (vInt.isMethod());

    EXPECT_TRUE (vInt64.isInt64());
    EXPECT_FALSE (vInt64.isInt());
    EXPECT_FALSE (vInt64.isBool());
    EXPECT_FALSE (vInt64.isDouble());
    EXPECT_FALSE (vInt64.isString());
    EXPECT_FALSE (vInt64.isArray());
    EXPECT_FALSE (vInt64.isBinaryData());
    EXPECT_FALSE (vInt64.isObject());
    EXPECT_FALSE (vInt64.isMethod());

    EXPECT_TRUE (vBool.isBool());
    EXPECT_FALSE (vBool.isInt());
    EXPECT_FALSE (vBool.isInt64());
    EXPECT_FALSE (vBool.isDouble());
    EXPECT_FALSE (vBool.isString());
    EXPECT_FALSE (vBool.isArray());
    EXPECT_FALSE (vBool.isBinaryData());
    EXPECT_FALSE (vBool.isObject());
    EXPECT_FALSE (vBool.isMethod());

    EXPECT_TRUE (vDouble.isDouble());
    EXPECT_FALSE (vDouble.isInt());
    EXPECT_FALSE (vDouble.isInt64());
    EXPECT_FALSE (vDouble.isBool());
    EXPECT_FALSE (vDouble.isString());
    EXPECT_FALSE (vDouble.isArray());
    EXPECT_FALSE (vDouble.isBinaryData());
    EXPECT_FALSE (vDouble.isObject());
    EXPECT_FALSE (vDouble.isMethod());

    EXPECT_TRUE (vString.isString());
    EXPECT_FALSE (vString.isInt());
    EXPECT_FALSE (vString.isInt64());
    EXPECT_FALSE (vString.isBool());
    EXPECT_FALSE (vString.isDouble());
    EXPECT_FALSE (vString.isArray());
    EXPECT_FALSE (vString.isBinaryData());
    EXPECT_FALSE (vString.isObject());
    EXPECT_FALSE (vString.isMethod());

    EXPECT_TRUE (vArray.isArray());
    EXPECT_FALSE (vArray.isInt());
    EXPECT_FALSE (vArray.isInt64());
    EXPECT_FALSE (vArray.isBool());
    EXPECT_FALSE (vArray.isDouble());
    EXPECT_FALSE (vArray.isString());
    EXPECT_FALSE (vArray.isBinaryData());
    EXPECT_TRUE (vArray.isObject()); // TODO - super strange
    EXPECT_FALSE (vArray.isMethod());

    EXPECT_TRUE (vBinaryData.isBinaryData());
    EXPECT_FALSE (vBinaryData.isInt());
    EXPECT_FALSE (vBinaryData.isInt64());
    EXPECT_FALSE (vBinaryData.isBool());
    EXPECT_FALSE (vBinaryData.isDouble());
    EXPECT_FALSE (vBinaryData.isString());
    EXPECT_FALSE (vBinaryData.isArray());
    EXPECT_FALSE (vBinaryData.isObject());
    EXPECT_FALSE (vBinaryData.isMethod());

    EXPECT_TRUE (vObject.isObject());
    EXPECT_FALSE (vObject.isInt());
    EXPECT_FALSE (vObject.isInt64());
    EXPECT_FALSE (vObject.isBool());
    EXPECT_FALSE (vObject.isDouble());
    EXPECT_FALSE (vObject.isString());
    EXPECT_FALSE (vObject.isArray());
    EXPECT_FALSE (vObject.isBinaryData());
    EXPECT_FALSE (vObject.isMethod());

    EXPECT_TRUE (vMethod.isMethod());
    EXPECT_FALSE (vMethod.isInt());
    EXPECT_FALSE (vMethod.isInt64());
    EXPECT_FALSE (vMethod.isBool());
    EXPECT_FALSE (vMethod.isDouble());
    EXPECT_FALSE (vMethod.isString());
    EXPECT_FALSE (vMethod.isArray());
    EXPECT_FALSE (vMethod.isBinaryData());
    EXPECT_FALSE (vMethod.isObject());
}

// Test clone method
TEST_F (VariantTests, CloneMethod)
{
    var original (100);
    var cloneVar = original.clone();
    EXPECT_TRUE (cloneVar.isInt());
    EXPECT_EQ (static_cast<int> (cloneVar), 100);

    var originalStr ("Original");
    var cloneStr = originalStr.clone();
    EXPECT_TRUE (cloneStr.isString());
    EXPECT_EQ (cloneStr.toString(), String ("Original"));

    Array<var> array;
    array.add (1);
    array.add (2.2);
    var originalArray (array);
    var cloneArray = originalArray.clone();
    EXPECT_TRUE (cloneArray.isArray());
    EXPECT_EQ (cloneArray.size(), 2);
    EXPECT_EQ (static_cast<int> (cloneArray[0]), 1);
    EXPECT_EQ (static_cast<double> (cloneArray[1]), 2.2);

    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty ("key", var (50));
    var originalObj (obj);
    var cloneObj = originalObj.clone();
    EXPECT_TRUE (cloneObj.isObject());
    DynamicObject* clonedDynamicObj = cloneObj.getDynamicObject();
    EXPECT_TRUE (clonedDynamicObj->hasProperty ("key"));
    EXPECT_EQ (clonedDynamicObj->getProperty ("key"), var (50));
}

// Test array operations
TEST_F (VariantTests, ArrayOperations)
{
    var vArray;
    EXPECT_TRUE (vArray.isVoid());

    // Append elements
    vArray.append (10);
    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 1);
    EXPECT_EQ (static_cast<int> (vArray[0]), 10);

    vArray.append (20.5);
    EXPECT_EQ (vArray.size(), 2);
    EXPECT_EQ (static_cast<double> (vArray[1]), 20.5);

    // Insert element
    vArray.insert (1, 15);
    EXPECT_EQ (vArray.size(), 3);
    EXPECT_EQ (static_cast<int> (vArray[1]), 15);
    EXPECT_EQ (static_cast<double> (vArray[2]), 20.5);

    // Remove element
    vArray.remove (0);
    EXPECT_EQ (vArray.size(), 2);
    EXPECT_EQ (static_cast<int> (vArray[0]), 15);
    EXPECT_EQ (static_cast<double> (vArray[1]), 20.5);

    // Resize array
    vArray.resize (4);
    EXPECT_EQ (vArray.size(), 4);
    EXPECT_EQ (static_cast<int> (vArray[0]), 15);
    EXPECT_EQ (static_cast<double> (vArray[1]), 20.5);
    EXPECT_EQ (static_cast<int> (vArray[2]), 0); // Default initialized
    EXPECT_EQ (static_cast<int> (vArray[3]), 0); // Default initialized

    // IndexOf
    EXPECT_EQ (vArray.indexOf (15), 0);
    EXPECT_EQ (vArray.indexOf (20.5), 1);
    EXPECT_EQ (vArray.indexOf (0), 2);
    EXPECT_EQ (vArray.indexOf (999), -1);
}

// Test object operations
TEST_F (VariantTests, ObjectOperations)
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty ("name", var ("TestObject"));
    obj->setProperty ("value", var (123));

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    EXPECT_TRUE (vObject.hasProperty ("name"));
    EXPECT_TRUE (vObject.hasProperty ("value"));
    EXPECT_EQ (vObject["name"], var ("TestObject"));
    EXPECT_EQ (vObject["value"], var (123));

    /*
    // Set new property
    vObject["newProp"] = var(456.78);
    EXPECT_TRUE(vObject.hasProperty("newProp"));
    EXPECT_EQ(vObject["newProp"], var(456.78));

    // Get property with default
    EXPECT_EQ(vObject.getProperty("nonExistent", var("default")), var("default"));

    // Remove property
    obj->removeProperty("value");
    EXPECT_FALSE(vObject.hasProperty("value"));
    */
}

// Test method operations
TEST_F (VariantTests, MethodOperations)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("increment");

    double counter = 0.0;
    var::NativeFunction incrementFunc = createNativeFunction ([&counter] (const var::NativeFunctionArgs& args) -> var
                                                              {
                                                                  counter += 1.0;
                                                                  return var (counter);
                                                              });

    obj->setMethod (methodName, incrementFunc);

    var vObject (obj);
    EXPECT_FALSE (vObject.isMethod());

    // Invoke method
    var argsArray[] = {};
    var result = vObject.invoke (Identifier ("increment"), argsArray, 0);
    EXPECT_EQ (static_cast<double> (result), 1.0);
    EXPECT_EQ (counter, 1.0);

    // Invoke again
    result = vObject.invoke (Identifier ("increment"), argsArray, 0);
    EXPECT_EQ (static_cast<double> (result), 2.0);
    EXPECT_EQ (counter, 2.0);
}

// Test binary data operations
TEST_F (VariantTests, BinaryDataOperations)
{
    MemoryBlock mem;
    mem.append ("binary", 6);
    var vBinary (mem);
    EXPECT_TRUE (vBinary.isBinaryData());

    MemoryBlock* retrievedMem = vBinary.getBinaryData();
    ASSERT_NE (retrievedMem, nullptr);
    EXPECT_EQ (mem.getSize(), retrievedMem->getSize());
    EXPECT_EQ (mem, *retrievedMem);

    // Clone binary data
    var cloneBinary = vBinary.clone();
    EXPECT_TRUE (cloneBinary.isBinaryData());
    MemoryBlock* clonedMem = cloneBinary.getBinaryData();
    ASSERT_NE (clonedMem, nullptr);
    EXPECT_EQ (clonedMem->getSize(), mem.getSize());
    EXPECT_EQ (*clonedMem, mem);
}

// Test writing and reading from stream
TEST_F (VariantTests, StreamOperations)
{
    var originalVar (123.456);

    MemoryOutputStream stream;
    originalVar.writeToStream (stream);

    MemoryInputStream inputStream (stream.getMemoryBlock());
    var readVar = var::readFromStream (inputStream);

    EXPECT_TRUE (readVar.isDouble());
    EXPECT_EQ (static_cast<double> (readVar), 123.456);
}

// Test JSON serialization
TEST_F (VariantTests, JSONSerialization)
{
    {
        var vString ("Test");

        MemoryOutputStream oss;
        vString.writeToStream (oss);

        MemoryInputStream iss (oss.getMemoryBlock());
        var parsedVar = var::readFromStream (iss);

        EXPECT_EQ (parsedVar, vString);
    }

    {
        var vInt (100);

        MemoryOutputStream oss;
        vInt.writeToStream (oss);

        MemoryInputStream iss (oss.getMemoryBlock());
        var parsedVar = var::readFromStream (iss);

        EXPECT_EQ (parsedVar, vInt);
    }

    {
        var vDouble (99.99);

        MemoryOutputStream oss;
        vDouble.writeToStream (oss);

        MemoryInputStream iss (oss.getMemoryBlock());
        var parsedVar = var::readFromStream (iss);

        EXPECT_EQ (parsedVar, vDouble);
    }

    {
        Array<var> array;
        array.add (var ("Test"));
        array.add (var (100));
        array.add (var (99.99));

        var vArray (array);

        MemoryOutputStream oss;
        vArray.writeToStream (oss);

        MemoryInputStream iss (oss.getMemoryBlock());
        var parsedVar = var::readFromStream (iss);

        EXPECT_EQ (parsedVar, vArray);
    }
}

// Test invoking methods with arguments
TEST_F (VariantTests, InvokeMethodWithArguments)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("add");

    var::NativeFunction addFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                        {
                                                            if (args.numArguments >= 2 && args.arguments[0].isDouble() && args.arguments[1].isDouble())
                                                            {
                                                                double a = args.arguments[0];
                                                                double b = args.arguments[1];
                                                                return var (a + b);
                                                            }
                                                            return var();
                                                        });

    obj->setMethod (methodName, addFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());
    EXPECT_FALSE (vObject.hasProperty ("add"));
    EXPECT_TRUE (vObject.hasMethod ("add"));

    var argsArray[] = { var (10.5), var (20.25) };
    var result = vObject.invoke (Identifier ("add"), argsArray, 2);
    EXPECT_TRUE (result.isDouble());
    EXPECT_EQ (static_cast<double> (result), 30.75);
}

// Test accessing array elements
TEST_F (VariantTests, AccessArrayElements)
{
    Array<var> array;
    array.add (1);
    array.add (2.2);
    array.add ("three");
    var vArray (array);

    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 3);
    EXPECT_EQ (static_cast<int> (vArray[0]), 1);
    EXPECT_EQ (static_cast<double> (vArray[1]), 2.2);
    EXPECT_EQ (vArray[2].toString(), String ("three"));

    // Modify array elements
    vArray[0] = var (10);
    vArray[1] = var (20.5);
    vArray[2] = var ("thirty");

    EXPECT_EQ (static_cast<int> (vArray[0]), 10);
    EXPECT_EQ (static_cast<double> (vArray[1]), 20.5);
    EXPECT_EQ (vArray[2].toString(), String ("thirty"));
}

// Test invoking a method that returns a string
TEST_F (VariantTests, InvokeMethodReturnsString)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("greet");

    var::NativeFunction greetFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                          {
                                                              return var ("Hello, JUCE!");
                                                          });

    obj->setMethod (methodName, greetFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());
    EXPECT_FALSE (vObject.hasProperty ("greet"));
    EXPECT_TRUE (vObject.hasMethod ("greet"));

    var argsArray[] = {};
    var result = vObject.invoke (Identifier ("greet"), argsArray, 0);
    EXPECT_TRUE (result.isString());
    EXPECT_EQ (result.toString(), String ("Hello, JUCE!"));
}

// Test adding and accessing multiple types in array
TEST_F (VariantTests, MixedTypeArray)
{
    var vArray;
    vArray.append (1);
    vArray.append (2.5);
    vArray.append ("three");
    vArray.append (true);

    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 4);

    EXPECT_EQ (static_cast<int> (vArray[0]), 1);
    EXPECT_EQ (static_cast<double> (vArray[1]), 2.5);
    EXPECT_EQ (vArray[2].toString(), String ("three"));
    EXPECT_EQ (static_cast<bool> (vArray[3]), true);
}

// Test operator[] with Identifier
TEST_F (VariantTests, OperatorWithIdentifier)
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty ("key1", var (100));
    obj->setProperty ("key2", var ("value2"));

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    EXPECT_EQ (vObject[Identifier ("key1")], var (100));
    EXPECT_EQ (vObject[Identifier ("key2")], var ("value2"));

    // Access non-existent key
    EXPECT_EQ (vObject[Identifier ("key3")], var());
}

// Test getProperty with default value
TEST_F (VariantTests, GetPropertyWithDefault)
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty ("existing", var (50));

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    EXPECT_EQ (vObject.getProperty (Identifier ("existing"), var (0)), var (50));
    EXPECT_EQ (vObject.getProperty (Identifier ("nonExistent"), var (999)), var (999));
}

// Test compare with same type
TEST_F (VariantTests, EqualsWithSameType)
{
    var v1 (25.5);
    var v2 (25.5);
    var v3 (30.0);
    var v4 ("25.5");

    EXPECT_TRUE (v1.equalsWithSameType (v2));
    EXPECT_FALSE (v1.equalsWithSameType (v3));
    EXPECT_FALSE (v1.equalsWithSameType (v4));
}

// Test hasSameTypeAs
TEST_F (VariantTests, HasSameTypeAs)
{
    var v1 (10);
    var v2 (20);
    var v3 (15.5);
    var v4 ("Test");

    EXPECT_TRUE (v1.hasSameTypeAs (v2));
    EXPECT_FALSE (v1.hasSameTypeAs (v3));
    EXPECT_FALSE (v1.hasSameTypeAs (v4));
}

// Test converting var to String
TEST_F (VariantTests, ToString)
{
    var v1 ("Hello");
    var v2 (123);
    var v3 (45.67);
    var v4 (true);
    var v5;

    EXPECT_EQ (v1.toString(), String ("Hello"));
    EXPECT_EQ (v2.toString(), String ("123"));
    EXPECT_EQ (v3.toString(), String ("45.67"));
    EXPECT_EQ (v4.toString(), String ("1")); // Assuming true converts to "1"
    EXPECT_EQ (v5.toString(), String (""));  // Void var converts to empty string
}

// Test invoking a method that modifies external state
TEST_F (VariantTests, InvokeMethodModifiesExternalState)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("increaseCounter");

    int counter = 0;
    var::NativeFunction increaseFunc = createNativeFunction ([&counter] (const var::NativeFunctionArgs& args) -> var
                                                             {
                                                                 counter += 5;
                                                                 return var (counter);
                                                             });

    obj->setMethod (methodName, increaseFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());
    EXPECT_FALSE (vObject.hasProperty ("increaseCounter"));
    EXPECT_TRUE (vObject.hasMethod ("increaseCounter"));

    var argsArray[] = {};
    var result = vObject.invoke (Identifier ("increaseCounter"), argsArray, 0);
    EXPECT_EQ (static_cast<int> (result), 5);
    EXPECT_EQ (counter, 5);

    result = vObject.invoke (Identifier ("increaseCounter"), argsArray, 0);
    EXPECT_EQ (static_cast<int> (result), 10);
    EXPECT_EQ (counter, 10);
}

// Test adding binary data
TEST_F (VariantTests, AddBinaryData)
{
    MemoryBlock mem;
    mem.append ("binarydata", 10);
    var vBinary (mem);

    EXPECT_TRUE (vBinary.isBinaryData());
    MemoryBlock* retrievedMem = vBinary.getBinaryData();
    ASSERT_NE (retrievedMem, nullptr);
    EXPECT_EQ (retrievedMem->getSize(), 10);
    EXPECT_EQ (mem, *retrievedMem);
}

// Test appending to array and accessing
TEST_F (VariantTests, AppendToArray)
{
    var vArray;
    vArray.append (1);
    vArray.append ("two");
    vArray.append (3.0);

    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 3);
    EXPECT_EQ (static_cast<int> (vArray[0]), 1);
    EXPECT_EQ (vArray[1].toString(), String ("two"));
    EXPECT_EQ (static_cast<double> (vArray[2]), 3.0);
}

// Test inserting into array
TEST_F (VariantTests, InsertIntoArray)
{
    var vArray;
    vArray.append ("first");
    vArray.append ("third");

    vArray.insert (1, "second");

    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 3);
    EXPECT_EQ (vArray[0].toString(), String ("first"));
    EXPECT_EQ (vArray[1].toString(), String ("second"));
    EXPECT_EQ (vArray[2].toString(), String ("third"));
}

// Test removing from array
TEST_F (VariantTests, RemoveFromArray)
{
    var vArray;
    vArray.append (1);
    vArray.append (2);
    vArray.append (3);

    vArray.remove (1);

    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 2);
    EXPECT_EQ (static_cast<int> (vArray[0]), 1);
    EXPECT_EQ (static_cast<int> (vArray[1]), 3);
}

// Test resizing array
TEST_F (VariantTests, ResizeArray)
{
    var vArray;
    vArray.append (1);
    vArray.append (2);

    vArray.resize (4);
    EXPECT_EQ (vArray.size(), 4);
    EXPECT_EQ (static_cast<int> (vArray[0]), 1);
    EXPECT_EQ (static_cast<int> (vArray[1]), 2);
    EXPECT_EQ (static_cast<int> (vArray[2]), 0);
    EXPECT_EQ (static_cast<int> (vArray[3]), 0);
}

// Test indexOf method
TEST_F (VariantTests, IndexOfMethod)
{
    var vArray;
    vArray.append ("apple");
    vArray.append ("banana");
    vArray.append ("cherry");
    vArray.append ("banana");

    EXPECT_EQ (vArray.indexOf ("banana"), 1);
    EXPECT_EQ (vArray.indexOf ("cherry"), 2);
    EXPECT_EQ (vArray.indexOf ("date"), -1);
}

// Test invoking undefined method
TEST_F (VariantTests, InvokeUndefinedMethod)
{
    DynamicObject::Ptr obj = new DynamicObject();
    var vObject (obj);

    var argsArray[] = {};
    var result = vObject.invoke (Identifier ("undefinedMethod"), argsArray, 0);
    EXPECT_TRUE (result.isVoid());
}

// Test setting and invoking method with multiple arguments
TEST_F (VariantTests, MethodWithMultipleArguments)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("multiply");

    var::NativeFunction multiplyFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                             {
                                                                 if (args.numArguments >= 2 && args.arguments[0].isDouble() && args.arguments[1].isDouble())
                                                                 {
                                                                     double a = args.arguments[0];
                                                                     double b = args.arguments[1];
                                                                     return var (a * b);
                                                                 }
                                                                 return var();
                                                             });

    obj->setMethod (methodName, multiplyFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());
    EXPECT_FALSE (vObject.hasProperty ("multiply"));
    EXPECT_TRUE (vObject.hasMethod ("multiply"));

    var argsArray[] = { var (3.0), var (4.0) };
    var result = vObject.invoke (Identifier ("multiply"), argsArray, 2);
    EXPECT_TRUE (result.isDouble());
    EXPECT_EQ (static_cast<double> (result), 12.0);
}

// Test modifying array through getArray()
TEST_F (VariantTests, ModifyArrayThroughGetArray)
{
    Array<var> array;
    array.add (1);
    array.add (2);
    var vArray (array);

    Array<var>* arrayPtr = vArray.getArray();
    ASSERT_NE (arrayPtr, nullptr);

    arrayPtr->add (3);
    EXPECT_EQ (vArray.size(), 3);
    EXPECT_EQ (static_cast<int> (vArray[2]), 3);
}

// Test method returning another object
TEST_F (VariantTests, MethodReturnsObject)
{
    DynamicObject::Ptr childObj = new DynamicObject();
    childObj->setProperty ("childProp", var (500));

    DynamicObject::Ptr parentObj = new DynamicObject();
    Identifier methodName ("getChild");

    var::NativeFunction getChildFunc = createNativeFunction ([childObj] (const var::NativeFunctionArgs& args) -> var
                                                             {
                                                                 return var (childObj);
                                                             });

    parentObj->setMethod (methodName, getChildFunc);

    var vParent (parentObj);
    EXPECT_TRUE (vParent.isObject());

    var argsArray[] = {};
    var result = vParent.invoke (Identifier ("getChild"), argsArray, 0);
    EXPECT_TRUE (result.isObject());

    DynamicObject* retrievedChild = result.getDynamicObject();
    ASSERT_NE (retrievedChild, nullptr);
    EXPECT_TRUE (retrievedChild->hasProperty ("childProp"));
    EXPECT_EQ (retrievedChild->getProperty ("childProp"), var (500));
}

// Test setting a method that returns a method
TEST_F (VariantTests, MethodReturnsMethod)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier outerMethod ("getInnerMethod");
    Identifier innerMethod ("inner");

    var::NativeFunction innerFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                          {
                                                              return var ("Inner Method Called");
                                                          });

    var::NativeFunction outerFunc = createNativeFunction ([innerFunc, &innerMethod] (const var::NativeFunctionArgs& args) -> var
                                                          {
                                                              DynamicObject::Ptr innerObj = new DynamicObject();
                                                              innerObj->setMethod (innerMethod, innerFunc);
                                                              return var (innerObj);
                                                          });

    obj->setMethod (outerMethod, outerFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    var argsArray[] = {};
    var result = vObject.invoke (Identifier ("getInnerMethod"), argsArray, 0);
    EXPECT_TRUE (result.isObject());

    var innerResult = result.invoke (Identifier ("inner"), argsArray, 0);
    EXPECT_EQ (innerResult.toString(), String ("Inner Method Called"));
}

// Test binary data cloning
TEST_F (VariantTests, CloneBinaryData)
{
    MemoryBlock mem;
    mem.append ("binarycontent", 13);
    var originalBinary (mem);

    var clonedBinary = originalBinary.clone();
    EXPECT_TRUE (clonedBinary.isBinaryData());

    MemoryBlock* originalMem = originalBinary.getBinaryData();
    MemoryBlock* clonedMem = clonedBinary.getBinaryData();

    ASSERT_NE (originalMem, nullptr);
    ASSERT_NE (clonedMem, nullptr);
    EXPECT_EQ (originalMem->getSize(), clonedMem->getSize());
    EXPECT_EQ (*originalMem, *clonedMem);

    // Modify cloned memory and ensure original is unaffected
    clonedMem->replaceAll ("changedcontent", 13);
    EXPECT_NE (*originalMem, *clonedMem);
}

// Test equals method
TEST_F (VariantTests, EqualsMethod)
{
    var v1 (100);
    var v2 (100.0);
    var v3 ("100");
    var v4 (100);

    EXPECT_TRUE (v1.equals (v2));
    EXPECT_FALSE (v1.equalsWithSameType (v2));
    EXPECT_TRUE (v1.equals (v3));
    EXPECT_FALSE (v1.equalsWithSameType (v3));
    EXPECT_TRUE (v1.equals (v4));
    EXPECT_TRUE (v1.equalsWithSameType (v4));
}

// Test hasSameTypeAs method
TEST_F (VariantTests, HasSameTypeAsMethod)
{
    var v1 (100);
    var v2 (100.0);
    var v3 ("100");
    var v4 (100);

    EXPECT_FALSE (v1.hasSameTypeAs (v2));
    EXPECT_TRUE (v1.hasSameTypeAs (v4));
    EXPECT_FALSE (v1.hasSameTypeAs (v3));
}

// Test method that returns void
TEST_F (VariantTests, MethodReturnsVoid)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("doNothing");

    var::NativeFunction doNothingFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                              {
                                                                  // Does nothing, returns void
                                                                  return var();
                                                              });

    obj->setMethod (methodName, doNothingFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());
    EXPECT_FALSE (vObject.hasProperty ("doNothing"));
    EXPECT_TRUE (vObject.hasMethod ("doNothing"));

    var argsArray[] = {};
    var result = vObject.invoke (Identifier ("doNothing"), argsArray, 0);
    EXPECT_TRUE (result.isVoid());
}

// Test binary data serialization
TEST_F (VariantTests, BinaryDataSerialization)
{
    MemoryBlock mem;
    mem.append ("serialize", 9);
    var vBinary (mem);

    // Serialize to stream
    MemoryOutputStream oss;
    vBinary.writeToStream (oss);

    // Deserialize from stream
    MemoryInputStream iss (oss.toString());
    var deserializedVar = var::readFromStream (iss);

    EXPECT_TRUE (deserializedVar.isBinaryData());
    MemoryBlock* deserializedMem = deserializedVar.getBinaryData();
    ASSERT_NE (deserializedMem, nullptr);
    EXPECT_EQ (deserializedMem->getSize(), mem.getSize());
    EXPECT_EQ (*deserializedMem, mem);
}

// Test invoking method with incorrect argument types
TEST_F (VariantTests, InvokeMethodWithIncorrectArguments)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("concat");

    var::NativeFunction concatFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                           {
                                                               if (args.numArguments >= 2 && args.arguments[0].isString() && args.arguments[1].isString())
                                                               {
                                                                   return var (args.arguments[0].toString() + args.arguments[1].toString());
                                                               }
                                                               return var();
                                                           });

    obj->setMethod (methodName, concatFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());
    EXPECT_FALSE (vObject.hasProperty ("concat"));
    EXPECT_TRUE (vObject.hasMethod ("concat"));

    var argsArray[] = { var (123), var ("ABC") };
    var result = vObject.invoke (Identifier ("concat"), argsArray, 2);
    EXPECT_TRUE (result.isVoid()); // Since first argument is not a string
}

// Test operator!= with different types
TEST_F (VariantTests, OperatorNotEqualsDifferentTypes)
{
    var v1 (100);
    var v2 ("100");
    var v3 (100.0);

    EXPECT_FALSE (v1 != v2);
    EXPECT_FALSE (v1 != v3);
    EXPECT_TRUE (v2 != v3);

    var v4 (100);
    EXPECT_FALSE (v1 != v4);
}

// Test adding and accessing properties
TEST_F (VariantTests, AddAndAccessProperties)
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty ("prop1", var (10));
    obj->setProperty ("prop2", var ("value2"));

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    EXPECT_EQ (vObject["prop1"], var (10));
    EXPECT_EQ (vObject["prop2"], var ("value2"));

    /*
    // Modify property
    vObject["prop1"] = var(20);
    EXPECT_EQ(vObject["prop1"], var(20));
    */
}

// Test invoking method that returns array
TEST_F (VariantTests, MethodReturnsArray)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("createArray");

    var::NativeFunction createArrayFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                                {
                                                                    Array<var> array;
                                                                    array.add (1);
                                                                    array.add (2);
                                                                    array.add (3);
                                                                    return var (array);
                                                                });

    obj->setMethod (methodName, createArrayFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    var argsArray[] = {};
    var result = vObject.invoke (Identifier ("createArray"), argsArray, 0);
    EXPECT_TRUE (result.isArray());
    EXPECT_EQ (result.size(), 3);
    EXPECT_EQ (static_cast<int> (result[0]), 1);
    EXPECT_EQ (static_cast<int> (result[1]), 2);
    EXPECT_EQ (static_cast<int> (result[2]), 3);
}

// Test invoking method with extra arguments
TEST_F (VariantTests, InvokeMethodWithExtraArguments)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("sum");

    var::NativeFunction sumFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                        {
                                                            double total = 0.0;
                                                            for (int i = 0; i < args.numArguments; ++i)
                                                            {
                                                                if (args.arguments[i].isDouble())
                                                                    total += static_cast<double> (args.arguments[i]);
                                                            }
                                                            return var (total);
                                                        });

    obj->setMethod (methodName, sumFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    var argsArray[] = { var (1.0), var (2.0), var (3.0), var (4.0) };
    var result = vObject.invoke (Identifier ("sum"), argsArray, 4);
    EXPECT_TRUE (result.isDouble());
    EXPECT_EQ (static_cast<double> (result), 10.0);
}

// Test operator[] for non-object var
TEST_F (VariantTests, DISABLED_OperatorBracketNonObject)
{
    var vInt (100);
    EXPECT_FALSE (vInt.isObject());

    // Accessing operator[] on non-object is undefined, but we can check it doesn't crash
    // Here, we expect it to return a void var
    var result = vInt[0];
    EXPECT_TRUE (result.isVoid());
}

// Test appending to non-array var
TEST_F (VariantTests, AppendToNonArray)
{
    var v = 50;
    v.append (100);
    EXPECT_TRUE (v.isArray());
    EXPECT_EQ (v.size(), 2);
    EXPECT_EQ (static_cast<int> (v[0]), 50);
    EXPECT_EQ (static_cast<int> (v[1]), 100);
}

// Test inserting into non-array var
TEST_F (VariantTests, InsertIntoNonArray)
{
    var v = "start";
    v.insert (1, "middle");
    EXPECT_TRUE (v.isArray());
    EXPECT_EQ (v.size(), 2);
    EXPECT_EQ (v[0].toString(), String ("start"));
    EXPECT_EQ (v[1].toString(), String ("middle"));
}

// Test removing from non-array var
TEST_F (VariantTests, RemoveFromNonArray)
{
    var v = 123;
    v.remove (0); // Should do nothing
    EXPECT_TRUE (v.isInt());
    EXPECT_EQ (static_cast<int> (v), 123);
}

// Test resizing non-array var
TEST_F (VariantTests, ResizeNonArray)
{
    var v = "only one";
    v.resize (3);
    EXPECT_TRUE (v.isArray());
    EXPECT_EQ (v.size(), 3);
    EXPECT_EQ (v[0].toString(), String ("only one"));
    EXPECT_EQ (static_cast<int> (v[1]), 0);
    EXPECT_EQ (static_cast<int> (v[2]), 0);
}

// Test operator[] with out-of-range index
TEST_F (VariantTests, DISABLED_OperatorBracketOutOfRange)
{
    var vArray;
    vArray.append (1);
    vArray.append (2);

    EXPECT_EQ (vArray[5].isVoid(), true); // Undefined access returns void var
}

// Test equals operator with different internal states
TEST_F (VariantTests, EqualsDifferentStates)
{
    var v1;
    var v2 = var::undefined();
    var v3 (0);

    EXPECT_TRUE (v1.equals (v2));
    EXPECT_FALSE (v1.equals (v3));
    EXPECT_FALSE (v2.equals (v3));
}

// Test clone of array var
TEST_F (VariantTests, CloneArrayVar)
{
    Array<var> array;
    array.add (1);
    array.add ("two");
    array.add (3.0);
    var originalArray (array);

    var clonedArray = originalArray.clone();
    EXPECT_TRUE (clonedArray.isArray());
    EXPECT_EQ (clonedArray.size(), 3);
    EXPECT_EQ (static_cast<int> (clonedArray[0]), 1);
    EXPECT_EQ (clonedArray[1].toString(), String ("two"));
    EXPECT_EQ (static_cast<double> (clonedArray[2]), 3.0);

    // Modify cloned array and ensure original is unaffected
    clonedArray[0] = var (10);
    EXPECT_EQ (static_cast<int> (originalArray[0]), 1);
    EXPECT_EQ (static_cast<int> (clonedArray[0]), 10);
}

// Test cloning object var
TEST_F (VariantTests, CloneObjectVar)
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty ("key1", var (100));
    obj->setProperty ("key2", var ("value2"));
    var originalObj (obj);

    var clonedObj = originalObj.clone();
    EXPECT_TRUE (clonedObj.isObject());

    DynamicObject::Ptr clonedDynamicObj = clonedObj.getDynamicObject();
    ASSERT_NE (clonedDynamicObj, nullptr);
    EXPECT_TRUE (clonedDynamicObj->hasProperty ("key1"));
    EXPECT_TRUE (clonedDynamicObj->hasProperty ("key2"));
    EXPECT_EQ (clonedDynamicObj->getProperty ("key1"), var (100));
    EXPECT_EQ (clonedDynamicObj->getProperty ("key2"), var ("value2"));

    // Modify cloned object and ensure original is unaffected
    clonedDynamicObj->setProperty ("key1", var (200));
    EXPECT_EQ (originalObj.getProperty ("key1", var()), var (100));
    EXPECT_EQ (clonedDynamicObj->getProperty ("key1"), var (200));
}

// Test converting var to bool
TEST_F (VariantTests, ConvertToBool)
{
    var vTrue (true);
    var vFalse (false);
    var vInt (1);
    var vZero (0);

    EXPECT_EQ (static_cast<bool> (vTrue), true);
    EXPECT_EQ (static_cast<bool> (vFalse), false);
    EXPECT_EQ (static_cast<bool> (vInt), true);
    EXPECT_EQ (static_cast<bool> (vZero), false);
}

// Test invoke with null arguments
TEST_F (VariantTests, InvokeMethodWithNullArguments)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("sayHello");

    var::NativeFunction sayHelloFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                             {
                                                                 return var ("Hello!");
                                                             });

    obj->setMethod (methodName, sayHelloFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    var result = vObject.invoke (Identifier ("sayHello"), nullptr, 0);
    EXPECT_TRUE (result.isString());
    EXPECT_EQ (result.toString(), String ("Hello!"));
}

// Test append on array var with different types
TEST_F (VariantTests, AppendDifferentTypes)
{
    var vArray;
    vArray.append (1);
    vArray.append ("two");
    vArray.append (3.0);
    vArray.append (false);

    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 4);
    EXPECT_EQ (static_cast<int> (vArray[0]), 1);
    EXPECT_EQ (vArray[1].toString(), String ("two"));
    EXPECT_EQ (static_cast<double> (vArray[2]), 3.0);
    EXPECT_EQ (static_cast<bool> (vArray[3]), false);
}

// Test inserting into array at invalid index
TEST_F (VariantTests, InsertIntoArrayInvalidIndex)
{
    var vArray;
    vArray.append ("first");

    // Insert at out-of-range index, behavior is undefined, but we can check size increases
    vArray.insert (10, "second");
    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 2);
    EXPECT_EQ (vArray[1].toString(), String ("second"));
}

// Test remove from array with invalid index
TEST_F (VariantTests, RemoveFromArrayInvalidIndex)
{
    var vArray;
    vArray.append (1);
    vArray.append (2);

    // Remove at out-of-range index, should do nothing
    vArray.remove (5);
    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 2);
    EXPECT_EQ (static_cast<int> (vArray[0]), 1);
    EXPECT_EQ (static_cast<int> (vArray[1]), 2);
}

// Test resizing array to smaller size
TEST_F (VariantTests, ResizeArraySmaller)
{
    var vArray;
    vArray.append (1);
    vArray.append (2);
    vArray.append (3);

    vArray.resize (2);
    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 2);
    EXPECT_EQ (static_cast<int> (vArray[0]), 1);
    EXPECT_EQ (static_cast<int> (vArray[1]), 2);
}

// Test resizing array to larger size
TEST_F (VariantTests, ResizeArrayLarger)
{
    var vArray;
    vArray.append (1);
    vArray.append (2);

    vArray.resize (4);
    EXPECT_TRUE (vArray.isArray());
    EXPECT_EQ (vArray.size(), 4);
    EXPECT_EQ (static_cast<int> (vArray[0]), 1);
    EXPECT_EQ (static_cast<int> (vArray[1]), 2);
    EXPECT_EQ (static_cast<int> (vArray[2]), 0);
    EXPECT_EQ (static_cast<int> (vArray[3]), 0);
}

// Test invoking method that returns method
TEST_F (VariantTests, MethodReturnsAnotherMethod)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier outerMethod ("getMultiplier");
    Identifier innerMethod ("multiply");

    var::NativeFunction multiplyFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                             {
                                                                 if (args.numArguments >= 2 && args.arguments[0].isDouble() && args.arguments[1].isDouble())
                                                                 {
                                                                     double a = args.arguments[0];
                                                                     double b = args.arguments[1];
                                                                     return var (a * b);
                                                                 }
                                                                 return var();
                                                             });

    var::NativeFunction getMultiplierFunc = createNativeFunction ([multiplyFunc] (const var::NativeFunctionArgs& args) -> var
                                                                  {
                                                                      DynamicObject::Ptr multiplierObj = new DynamicObject();
                                                                      multiplierObj->setMethod ("multiply", multiplyFunc);
                                                                      return var (multiplierObj);
                                                                  });

    obj->setMethod (outerMethod, getMultiplierFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    var argsOuter[] = {};
    var multiplierVar = vObject.invoke (Identifier ("getMultiplier"), argsOuter, 0);
    EXPECT_TRUE (multiplierVar.isObject());

    var argsInner[] = { var (5.0), var (6.0) };
    var result = multiplierVar.invoke (Identifier ("multiply"), argsInner, 2);
    EXPECT_TRUE (result.isDouble());
    EXPECT_EQ (static_cast<double> (result), 30.0);
}

// Test cloning a method var
TEST_F (VariantTests, CloneMethodVar)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("getValue");

    var::NativeFunction getValueFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                             {
                                                                 return var (999);
                                                             });

    obj->setMethod (methodName, getValueFunc);

    var originalMethod = getValueFunc;
    var clonedMethod = originalMethod.clone();

    EXPECT_TRUE (clonedMethod.isMethod());
    EXPECT_TRUE (clonedMethod.equals (originalMethod));
}

// Test converting var to int
TEST_F (VariantTests, ConvertToInt)
{
    var vInt (42);
    var vDouble (3.14);
    var vBool (true);
    var vString ("100");

    EXPECT_EQ (static_cast<int> (vInt), 42);
    EXPECT_EQ (static_cast<int> (vDouble), 3);
    EXPECT_EQ (static_cast<int> (vBool), 1);
    EXPECT_EQ (static_cast<int> (vString), 100);
}

// Test converting var to double
TEST_F (VariantTests, ConvertToDouble)
{
    var vInt (42);
    var vDouble (3.14);
    var vBool (false);
    var vString ("3.14");

    EXPECT_DOUBLE_EQ (static_cast<double> (vInt), 42.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (vDouble), 3.14);
    EXPECT_DOUBLE_EQ (static_cast<double> (vBool), 0.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (vString), 3.14); // Assuming non-numeric string converts to 0.0
}

// Test operator[] with string
TEST_F (VariantTests, OperatorWithString)
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty ("status", var ("active"));

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    EXPECT_EQ (vObject["status"], var ("active"));
}

// Test operator[] on non-string identifier
TEST_F (VariantTests, OperatorWithNonStringIdentifier)
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty (Identifier ("key"), var (123));

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    EXPECT_EQ (vObject["key"], var (123));
    EXPECT_EQ (vObject["nonexistent"], var());
}

// Test call methods with varying arguments
TEST_F (VariantTests, CallMethodsWithVaryingArguments)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("concatenate");

    var::NativeFunction concatenateFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                                {
                                                                    std::string result;
                                                                    for (int i = 0; i < args.numArguments; ++i)
                                                                    {
                                                                        if (args.arguments[i].isString())
                                                                            result += args.arguments[i].toString().toStdString();
                                                                    }
                                                                    return var (result);
                                                                });

    obj->setMethod (methodName, concatenateFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    var result1 = vObject.call (Identifier ("concatenate"), var ("Hello"));
    EXPECT_EQ (result1.toString(), String ("Hello"));

    var result2 = vObject.call (Identifier ("concatenate"), var ("Hello"), var (" "), var ("World"));
    EXPECT_EQ (result2.toString(), String ("Hello World"));

    var result3 = vObject.call (Identifier ("concatenate"), var ("JUCE"), var (" "), var ("Var"), var (" Test"));
    EXPECT_EQ (result3.toString(), String ("JUCE Var Test"));
}

// Test invoke method with all possible call signatures
TEST_F (VariantTests, InvokeMethodVariousSignatures)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("compute");

    var::NativeFunction computeFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                            {
                                                                if (args.numArguments == 0)
                                                                    return var (0);
                                                                if (args.numArguments == 1 && args.arguments[0].isInt())
                                                                    return var (static_cast<int> (args.arguments[0]) * 2);
                                                                if (args.numArguments == 2 && args.arguments[0].isInt() && args.arguments[1].isInt())
                                                                    return var (static_cast<int> (args.arguments[0]) + static_cast<int> (args.arguments[1]));
                                                                return var();
                                                            });

    obj->setMethod (methodName, computeFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    // No arguments
    var result0 = vObject.call (Identifier ("compute"));
    EXPECT_TRUE (result0.isInt());
    EXPECT_EQ (static_cast<int> (result0), 0);

    // One argument
    var result1 = vObject.call (Identifier ("compute"), var (5));
    EXPECT_TRUE (result1.isInt());
    EXPECT_EQ (static_cast<int> (result1), 10);

    // Two arguments
    var result2 = vObject.call (Identifier ("compute"), var (7), var (3));
    EXPECT_TRUE (result2.isInt());
    EXPECT_EQ (static_cast<int> (result2), 10);

    // Three arguments (undefined behavior, expect default var)
    var result3 = vObject.invoke (Identifier ("compute"), nullptr, 3);
    EXPECT_TRUE (result3.isVoid());
}

// Test makeVar with custom objects
TEST_F (VariantTests, MakeVarWithCustomObject)
{
    DynamicObject::Ptr obj = new DynamicObject();
    obj->setProperty ("customKey", var (250));
    var vObject (obj);

    EXPECT_TRUE (vObject.isObject());
    EXPECT_TRUE (vObject.hasProperty ("customKey"));
    EXPECT_EQ (vObject["customKey"], var (250));
}

// Test reading and writing undefined var
TEST_F (VariantTests, UndefinedVarSerialization)
{
    var vUndefined = var::undefined();

    // Serialize to stream
    MemoryOutputStream oss;
    vUndefined.writeToStream (oss);

    // Deserialize from stream
    MemoryInputStream iss (oss.getMemoryBlock());
    var deserializedVar = var::readFromStream (iss);

    EXPECT_TRUE (deserializedVar.isVoid());
    EXPECT_FALSE (deserializedVar.isUndefined());
}

// Test method that returns a function
TEST_F (VariantTests, MethodReturnsFunction)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("getAdder");

    var::NativeFunction adderFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                          {
                                                              return var::NativeFunction ([] (const var::NativeFunctionArgs& innerArgs) -> var
                                                                                          {
                                                                                              if (innerArgs.numArguments >= 1 && innerArgs.arguments[0].isInt())
                                                                                                  return var (static_cast<int> (innerArgs.arguments[0]) + 10);

                                                                                              return var();
                                                                                          });
                                                          });

    obj->setMethod (methodName, adderFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    var argsOuter[] = {};
    var adderVar = vObject.invoke (Identifier ("getAdder"), argsOuter, 0);
    EXPECT_TRUE (adderVar.isMethod());

    var argsInner[] = { var (5) };
    var result = adderVar.getNativeFunction() (var::NativeFunctionArgs (var(), &argsInner[0], 1));
    EXPECT_EQ (static_cast<int> (result), 15);
}

// Test invoking method that returns undefined
TEST_F (VariantTests, MethodReturnsUndefined)
{
    DynamicObject::Ptr obj = new DynamicObject();
    Identifier methodName ("returnUndefined");

    var::NativeFunction undefinedFunc = createNativeFunction ([] (const var::NativeFunctionArgs& args) -> var
                                                              {
                                                                  return var::undefined();
                                                              });

    obj->setMethod (methodName, undefinedFunc);

    var vObject (obj);
    EXPECT_TRUE (vObject.isObject());

    var argsArray[] = {};
    var result = vObject.invoke (Identifier ("returnUndefined"), argsArray, 0);
    EXPECT_TRUE (result.isUndefined());
}

// Test swapWith method
TEST_F (VariantTests, SwapWithMethod)
{
    var v1 (100);
    var v2 ("swapTest");

    v1.swapWith (v2);

    EXPECT_TRUE (v1.isString());
    EXPECT_EQ (v1.toString(), String ("swapTest"));

    EXPECT_TRUE (v2.isInt());
    EXPECT_EQ (static_cast<int> (v2), 100);
}

// Test operator< with vars
TEST_F (VariantTests, OperatorLessThan)
{
    var v1 (50);
    var v2 (100);
    var v3 ("apple");
    var v4 ("banana");

    EXPECT_TRUE (v1 < v2);
    EXPECT_TRUE (v3 < v4);
    EXPECT_FALSE (v2 < v1);
    EXPECT_FALSE (v4 < v3);
}

// Test operator> with vars
TEST_F (VariantTests, OperatorGreaterThan)
{
    var v1 (150);
    var v2 (100);
    var v3 ("orange");
    var v4 ("apple");

    EXPECT_TRUE (v1 > v2);
    EXPECT_TRUE (v3 > v4);
    EXPECT_FALSE (v2 > v1);
    EXPECT_FALSE (v4 > v3);
}

// Test operator<= with vars
TEST_F (VariantTests, OperatorLessThanOrEqual)
{
    var v1 (50);
    var v2 (100);
    var v3 (50);
    var v4 ("apple");
    var v5 ("apple");
    var v6 ("banana");

    EXPECT_TRUE (v1 <= v2);
    EXPECT_TRUE (v1 <= v3);
    EXPECT_FALSE (v2 <= v1);

    EXPECT_TRUE (v4 <= v5);
    EXPECT_TRUE (v4 <= v6);
    EXPECT_FALSE (v6 <= v4);
}

// Test operator>= with vars
TEST_F (VariantTests, OperatorGreaterThanOrEqual)
{
    var v1 (100);
    var v2 (50);
    var v3 (100);
    var v4 ("banana");
    var v5 ("banana");
    var v6 ("apple");

    EXPECT_TRUE (v1 >= v2);
    EXPECT_TRUE (v1 >= v3);
    EXPECT_FALSE (v2 >= v1);

    EXPECT_TRUE (v4 >= v5);
    EXPECT_TRUE (v4 >= v6);
    EXPECT_FALSE (v6 >= v4);
}
