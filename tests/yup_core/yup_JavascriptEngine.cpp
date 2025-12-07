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

#include <yup_core/yup_core.h>

using namespace yup;

TEST (JavascriptEngineTests, ExecuteValidCode)
{
    JavascriptEngine engine;
    Result result = engine.execute ("var x = 10; var y = 20; var z = x + y;");
    EXPECT_TRUE (result.wasOk());
}

TEST (JavascriptEngineTests, ExecuteInvalidCode)
{
    JavascriptEngine engine;

    Result result = engine.execute ("var x = 10; var y = ;");
    EXPECT_FALSE (result.wasOk());
}

TEST (JavascriptEngineTests, EvaluateValidExpression)
{
    JavascriptEngine engine;

    auto error = Result::fail ("fail");
    var result = engine.evaluate ("10 + 20", &error);
    EXPECT_TRUE (error.wasOk());
    EXPECT_EQ (static_cast<int> (result), 30);
}

TEST (JavascriptEngineTests, EvaluateInvalidExpression)
{
    JavascriptEngine engine;

    auto error = Result::ok();
    var result = engine.evaluate ("10 + ", &error);
    EXPECT_FALSE (error.wasOk());
    EXPECT_EQ (result, var::undefined());
}

TEST (JavascriptEngineTests, CallFunction)
{
    JavascriptEngine engine;

    engine.execute ("function add (a, b) { return a + b; }");

    auto error = Result::fail ("fail");
    var args[] = { 10, 20 };

    var result = engine.callFunction ("add", var::NativeFunctionArgs (var(), args, numElementsInArray (args)), &error);
    EXPECT_TRUE (error.wasOk());
    EXPECT_EQ (static_cast<int> (result), 30);
}

TEST (JavascriptEngineTests, CallFunctionThatThrows)
{
    JavascriptEngine engine;

    engine.execute ("function add (a, b) { if (a + b == 30) throw; else return a + b; }");

    auto error = Result::fail ("fail");
    var args[] = { 10, 20 };

    var result = engine.callFunction ("add", var::NativeFunctionArgs (var(), args, numElementsInArray (args)), &error);
    EXPECT_FALSE (error.wasOk());
    EXPECT_EQ (result, var::undefined());
}

TEST (JavascriptEngineTests, CallUndefinedFunction)
{
    JavascriptEngine engine;

    auto error = Result::ok();
    var args[] = { 10, 20 };

    var result = engine.callFunction ("nonexistentFunction", var::NativeFunctionArgs (var(), args, numElementsInArray (args)), &error);
    EXPECT_FALSE (error.wasOk());
    EXPECT_EQ (result, var::undefined());
}

TEST (JavascriptEngineTests, RegisterNativeObject)
{
    JavascriptEngine engine;

    struct TestObject : public DynamicObject
    {
    };

    DynamicObject::Ptr testObject = new TestObject();
    testObject->setMethod ("add", [] (const var::NativeFunctionArgs& args) -> var
    {
        if (args.numArguments != 2)
            return 0;

        return static_cast<int> (args.arguments[0]) + static_cast<int> (args.arguments[1]);
    });

    engine.registerNativeObject ("testObject", testObject.get());

    auto error = Result::fail ("fail");
    var result = engine.evaluate ("testObject.add (10, 20)", &error);
    EXPECT_TRUE (error.wasOk());
    EXPECT_EQ (static_cast<int> (result), 30);
}

TEST (JavascriptEngineTests, MaximumExecutionTime)
{
    JavascriptEngine engine;
    engine.maximumExecutionTime = RelativeTime::milliseconds (200);

    Result result = engine.execute ("while(true) {}");
    EXPECT_FALSE (result.wasOk());
}

#if ! YUP_WASM
TEST (JavascriptEngineTests, StopExecution)
{
    JavascriptEngine engine;
    engine.maximumExecutionTime = RelativeTime::seconds (3600);

    WaitableEvent startEvent;

    auto executionThread = std::thread ([&]
    {
        startEvent.wait();
        engine.execute ("while (true) {}");
    });

    startEvent.signal();
    std::this_thread::sleep_for (std::chrono::milliseconds (100));

    engine.stop();
    executionThread.join();

    SUCCEED();
}
#endif

// ============================================================================
// Arithmetic Operators
// ============================================================================

TEST (JavascriptEngineTests, ArithmeticOperators)
{
    JavascriptEngine engine;

    EXPECT_EQ (30, (int) engine.evaluate ("10 + 20"));
    EXPECT_EQ (10, (int) engine.evaluate ("30 - 20"));
    EXPECT_EQ (50, (int) engine.evaluate ("10 * 5"));
    EXPECT_EQ (2.5, (double) engine.evaluate ("5 / 2"));
    EXPECT_EQ (3, (int) engine.evaluate ("13 % 5"));
    EXPECT_EQ (-5, (int) engine.evaluate ("-5"));
}

TEST (JavascriptEngineTests, StringConcatenation)
{
    JavascriptEngine engine;

    EXPECT_EQ (String ("hello world"), engine.evaluate ("'hello' + ' world'").toString());
    EXPECT_EQ (String ("value: 42"), engine.evaluate ("'value: ' + 42").toString());
}

// ============================================================================
// Comparison Operators
// ============================================================================

TEST (JavascriptEngineTests, ComparisonOperators)
{
    JavascriptEngine engine;

    EXPECT_TRUE ((bool) engine.evaluate ("10 == 10"));
    EXPECT_FALSE ((bool) engine.evaluate ("10 == 20"));
    EXPECT_TRUE ((bool) engine.evaluate ("10 != 20"));
    EXPECT_FALSE ((bool) engine.evaluate ("10 != 10"));
    EXPECT_TRUE ((bool) engine.evaluate ("5 < 10"));
    EXPECT_FALSE ((bool) engine.evaluate ("10 < 5"));
    EXPECT_TRUE ((bool) engine.evaluate ("5 <= 5"));
    EXPECT_TRUE ((bool) engine.evaluate ("10 > 5"));
    EXPECT_FALSE ((bool) engine.evaluate ("5 > 10"));
    EXPECT_TRUE ((bool) engine.evaluate ("5 >= 5"));
}

TEST (JavascriptEngineTests, TypeEqualityOperators)
{
    JavascriptEngine engine;

    EXPECT_TRUE ((bool) engine.evaluate ("10 === 10"));
    EXPECT_FALSE ((bool) engine.evaluate ("10 === '10'"));
    EXPECT_TRUE ((bool) engine.evaluate ("10 !== '10'"));
    EXPECT_FALSE ((bool) engine.evaluate ("10 !== 10"));
    EXPECT_TRUE ((bool) engine.evaluate ("undefined === undefined"));
}

TEST (JavascriptEngineTests, StringComparison)
{
    JavascriptEngine engine;

    EXPECT_TRUE ((bool) engine.evaluate ("'abc' < 'def'"));
    EXPECT_TRUE ((bool) engine.evaluate ("'abc' == 'abc'"));
    EXPECT_TRUE ((bool) engine.evaluate ("'abc' != 'def'"));
}

// ============================================================================
// Logical Operators
// ============================================================================

TEST (JavascriptEngineTests, LogicalOperators)
{
    JavascriptEngine engine;

    EXPECT_TRUE ((bool) engine.evaluate ("true && true"));
    EXPECT_FALSE ((bool) engine.evaluate ("true && false"));
    EXPECT_TRUE ((bool) engine.evaluate ("true || false"));
    EXPECT_FALSE ((bool) engine.evaluate ("false || false"));
    EXPECT_FALSE ((bool) engine.evaluate ("!true"));
    EXPECT_TRUE ((bool) engine.evaluate ("!false"));
    EXPECT_TRUE ((bool) engine.evaluate ("!0"));
}

// ============================================================================
// Bitwise Operators
// ============================================================================

TEST (JavascriptEngineTests, BitwiseOperators)
{
    JavascriptEngine engine;

    // 10 = 1010, 4 = 0100
    EXPECT_EQ (14, (int) engine.evaluate ("10 | 4"));  // 1110 = 14
    EXPECT_EQ (0, (int) engine.evaluate ("10 & 4"));   // 0000 = 0
    EXPECT_EQ (14, (int) engine.evaluate ("10 ^ 4"));  // 1110 = 14
    EXPECT_EQ (40, (int) engine.evaluate ("10 << 2")); // 101000 = 40
    EXPECT_EQ (2, (int) engine.evaluate ("10 >> 2"));  // 10 = 2
    EXPECT_EQ (2, (int) engine.evaluate ("10 >>> 2")); // 10 = 2
}

// ============================================================================
// In-Place Assignment Operators
// ============================================================================

TEST (JavascriptEngineTests, InPlaceOperators)
{
    JavascriptEngine engine;

    engine.execute ("var x = 10;");
    EXPECT_EQ (15, (int) engine.evaluate ("x += 5"));
    EXPECT_EQ (10, (int) engine.evaluate ("x -= 5"));
    EXPECT_EQ (30, (int) engine.evaluate ("x *= 3"));
    EXPECT_EQ (15, (int) engine.evaluate ("x /= 2"));
    EXPECT_EQ (1, (int) engine.evaluate ("x %= 2"));
    engine.execute ("x = 10;");
    EXPECT_EQ (40, (int) engine.evaluate ("x <<= 2"));
    engine.execute ("x = 10;");
    EXPECT_EQ (2, (int) engine.evaluate ("x >>= 2"));
}

// ============================================================================
// Increment/Decrement Operators
// ============================================================================

TEST (JavascriptEngineTests, IncrementDecrementOperators)
{
    JavascriptEngine engine;

    engine.execute ("var x = 10;");
    EXPECT_EQ (11, (int) engine.evaluate ("++x"));
    EXPECT_EQ (11, (int) engine.evaluate ("x"));

    engine.execute ("x = 10;");
    EXPECT_EQ (10, (int) engine.evaluate ("x++"));
    EXPECT_EQ (11, (int) engine.evaluate ("x"));

    engine.execute ("x = 10;");
    EXPECT_EQ (9, (int) engine.evaluate ("--x"));
    EXPECT_EQ (9, (int) engine.evaluate ("x"));

    engine.execute ("x = 10;");
    EXPECT_EQ (10, (int) engine.evaluate ("x--"));
    EXPECT_EQ (9, (int) engine.evaluate ("x"));
}

// ============================================================================
// Control Flow: If/Else
// ============================================================================

TEST (JavascriptEngineTests, IfElseStatement)
{
    JavascriptEngine engine;

    engine.execute ("var result = 0; if (true) result = 1; else result = 2;");
    EXPECT_EQ (1, (int) engine.evaluate ("result"));

    engine.execute ("result = 0; if (false) result = 1; else result = 2;");
    EXPECT_EQ (2, (int) engine.evaluate ("result"));

    engine.execute ("result = 0; if (5 > 3) result = 10;");
    EXPECT_EQ (10, (int) engine.evaluate ("result"));
}

TEST (JavascriptEngineTests, NestedIfStatement)
{
    JavascriptEngine engine;

    engine.execute ("var result = 0; if (true) { if (true) result = 5; }");
    EXPECT_EQ (5, (int) engine.evaluate ("result"));
}

// ============================================================================
// Control Flow: Loops
// ============================================================================

TEST (JavascriptEngineTests, WhileLoop)
{
    JavascriptEngine engine;

    engine.execute ("var sum = 0; var i = 1; while (i <= 10) { sum += i; i++; }");
    EXPECT_EQ (55, (int) engine.evaluate ("sum"));
}

TEST (JavascriptEngineTests, DoWhileLoop)
{
    JavascriptEngine engine;

    engine.execute ("var sum = 0; var i = 1; do { sum += i; i++; } while (i <= 10);");
    EXPECT_EQ (55, (int) engine.evaluate ("sum"));

    engine.execute ("var executed = 0; do { executed = 1; } while (false);");
    EXPECT_EQ (1, (int) engine.evaluate ("executed"));
}

TEST (JavascriptEngineTests, ForLoop)
{
    JavascriptEngine engine;

    engine.execute ("var sum = 0; for (var i = 1; i <= 10; i++) sum += i;");
    EXPECT_EQ (55, (int) engine.evaluate ("sum"));

    engine.execute ("var count = 0; for (var i = 0; ; i++) { count++; if (i >= 5) break; }");
    EXPECT_EQ (6, (int) engine.evaluate ("count"));
}

TEST (JavascriptEngineTests, BreakStatement)
{
    JavascriptEngine engine;

    engine.execute ("var sum = 0; for (var i = 1; i <= 10; i++) { if (i > 5) break; sum += i; }");
    EXPECT_EQ (15, (int) engine.evaluate ("sum"));
}

TEST (JavascriptEngineTests, ContinueStatement)
{
    JavascriptEngine engine;

    engine.execute ("var sum = 0; for (var i = 1; i <= 10; i++) { if (i % 2 == 0) continue; sum += i; }");
    EXPECT_EQ (25, (int) engine.evaluate ("sum"));
}

// ============================================================================
// Functions
// ============================================================================

TEST (JavascriptEngineTests, FunctionDeclaration)
{
    JavascriptEngine engine;

    engine.execute ("function multiply (a, b) { return a * b; }");
    EXPECT_EQ (20, (int) engine.evaluate ("multiply (4, 5)"));
}

TEST (JavascriptEngineTests, FunctionWithoutReturn)
{
    JavascriptEngine engine;

    engine.execute ("function noReturn() { var x = 5; }");
    var result = engine.evaluate ("noReturn()");
    // Functions without explicit return return undefined or void
    EXPECT_TRUE (result.isUndefined() || result.isVoid());
}

TEST (JavascriptEngineTests, FunctionWithMultipleStatements)
{
    JavascriptEngine engine;

    engine.execute ("function complex (a, b) { var sum = a + b; var product = a * b; return sum + product; }");
    // sum = 3 + 4 = 7, product = 3 * 4 = 12, return 7 + 12 = 19
    EXPECT_EQ (19, (int) engine.evaluate ("complex (3, 4)"));
}

TEST (JavascriptEngineTests, InlineFunctionExpression)
{
    JavascriptEngine engine;

    engine.execute ("var square = function (x) { return x * x; };");
    EXPECT_EQ (25, (int) engine.evaluate ("square (5)"));
}

TEST (JavascriptEngineTests, FunctionClosure)
{
    JavascriptEngine engine;

    engine.execute ("var counter = 0; function increment() { counter++; return counter; }");
    EXPECT_EQ (1, (int) engine.evaluate ("increment()"));
    EXPECT_EQ (2, (int) engine.evaluate ("increment()"));
}

// ============================================================================
// Ternary Operator
// ============================================================================

TEST (JavascriptEngineTests, TernaryOperator)
{
    JavascriptEngine engine;

    EXPECT_EQ (5, (int) engine.evaluate ("true ? 5 : 10"));
    EXPECT_EQ (10, (int) engine.evaluate ("false ? 5 : 10"));
    EXPECT_EQ (String ("yes"), engine.evaluate ("10 > 5 ? 'yes' : 'no'").toString());
}

// ============================================================================
// Arrays
// ============================================================================

TEST (JavascriptEngineTests, ArrayLiteral)
{
    JavascriptEngine engine;

    engine.execute ("var arr = [1, 2, 3, 4, 5];");
    EXPECT_EQ (5, (int) engine.evaluate ("arr.length"));
    EXPECT_EQ (1, (int) engine.evaluate ("arr[0]"));
    EXPECT_EQ (3, (int) engine.evaluate ("arr[2]"));
    EXPECT_EQ (5, (int) engine.evaluate ("arr[4]"));
}

TEST (JavascriptEngineTests, ArrayPush)
{
    JavascriptEngine engine;

    engine.execute ("var arr = [1, 2, 3]; arr.push (4); arr.push (5, 6);");
    EXPECT_EQ (6, (int) engine.evaluate ("arr.length"));
    EXPECT_EQ (6, (int) engine.evaluate ("arr[5]"));
}

TEST (JavascriptEngineTests, ArrayContains)
{
    JavascriptEngine engine;

    engine.execute ("var arr = [1, 2, 3];");
    EXPECT_TRUE ((bool) engine.evaluate ("arr.contains (2)"));
    EXPECT_FALSE ((bool) engine.evaluate ("arr.contains (5)"));
}

TEST (JavascriptEngineTests, ArrayIndexOf)
{
    JavascriptEngine engine;

    engine.execute ("var arr = [10, 20, 30, 20];");
    EXPECT_EQ (1, (int) engine.evaluate ("arr.indexOf (20)"));
    EXPECT_EQ (3, (int) engine.evaluate ("arr.indexOf (20, 2)"));
    EXPECT_EQ (-1, (int) engine.evaluate ("arr.indexOf (99)"));
}

TEST (JavascriptEngineTests, ArrayRemove)
{
    JavascriptEngine engine;

    engine.execute ("var arr = [1, 2, 3, 2, 4]; arr.remove (2);");
    EXPECT_EQ (3, (int) engine.evaluate ("arr.length"));
    EXPECT_EQ (1, (int) engine.evaluate ("arr[0]"));
    EXPECT_EQ (3, (int) engine.evaluate ("arr[1]"));
}

TEST (JavascriptEngineTests, ArrayJoin)
{
    JavascriptEngine engine;

    engine.execute ("var arr = [1, 2, 3];");
    EXPECT_EQ (String ("1,2,3"), engine.evaluate ("arr.join (',')").toString());
    EXPECT_EQ (String ("1-2-3"), engine.evaluate ("arr.join ('-')").toString());
}

TEST (JavascriptEngineTests, ArraySplice)
{
    JavascriptEngine engine;

    engine.execute ("var arr = [1, 2, 3, 4, 5]; var removed = arr.splice (1, 2);");
    EXPECT_EQ (3, (int) engine.evaluate ("arr.length"));
    EXPECT_EQ (2, (int) engine.evaluate ("removed.length"));
    EXPECT_EQ (2, (int) engine.evaluate ("removed[0]"));
}

// ============================================================================
// Objects
// ============================================================================

TEST (JavascriptEngineTests, ObjectLiteral)
{
    JavascriptEngine engine;

    engine.execute ("var obj = { x: 10, y: 20 };");
    EXPECT_EQ (10, (int) engine.evaluate ("obj.x"));
    EXPECT_EQ (20, (int) engine.evaluate ("obj.y"));
}

TEST (JavascriptEngineTests, ObjectBracketAccess)
{
    JavascriptEngine engine;

    engine.execute ("var obj = { x: 10 };");
    EXPECT_EQ (10, (int) engine.evaluate ("obj['x']"));

    engine.execute ("obj['y'] = 20;");
    EXPECT_EQ (20, (int) engine.evaluate ("obj.y"));
}

TEST (JavascriptEngineTests, ObjectClone)
{
    JavascriptEngine engine;

    engine.execute ("var obj = { x: 10 }; var copy = obj.clone();");
    engine.execute ("copy.x = 20;");
    EXPECT_EQ (10, (int) engine.evaluate ("obj.x"));
    EXPECT_EQ (20, (int) engine.evaluate ("copy.x"));
}

TEST (JavascriptEngineTests, NewOperator)
{
    JavascriptEngine engine;

    engine.execute ("function Point (x, y) { this.x = x; this.y = y; } var p = new Point (3, 4);");
    EXPECT_EQ (3, (int) engine.evaluate ("p.x"));
    EXPECT_EQ (4, (int) engine.evaluate ("p.y"));
}

// ============================================================================
// String Methods
// ============================================================================

TEST (JavascriptEngineTests, StringLength)
{
    JavascriptEngine engine;

    EXPECT_EQ (5, (int) engine.evaluate ("'hello'.length"));
}

TEST (JavascriptEngineTests, StringSubstring)
{
    JavascriptEngine engine;

    EXPECT_EQ (String ("ell"), engine.evaluate ("'hello'.substring (1, 4)").toString());
}

TEST (JavascriptEngineTests, StringContains)
{
    JavascriptEngine engine;

    EXPECT_TRUE ((bool) engine.evaluate ("'hello world'.contains ('world')"));
    EXPECT_FALSE ((bool) engine.evaluate ("'hello world'.contains ('xyz')"));
}

TEST (JavascriptEngineTests, StringStartsWithEndsWith)
{
    JavascriptEngine engine;

    EXPECT_TRUE ((bool) engine.evaluate ("'hello'.startsWith ('hel')"));
    EXPECT_FALSE ((bool) engine.evaluate ("'hello'.startsWith ('llo')"));
    EXPECT_TRUE ((bool) engine.evaluate ("'hello'.endsWith ('llo')"));
    EXPECT_FALSE ((bool) engine.evaluate ("'hello'.endsWith ('hel')"));
}

TEST (JavascriptEngineTests, StringReplace)
{
    JavascriptEngine engine;

    EXPECT_EQ (String ("hallo"), engine.evaluate ("'hello'.replace ('e', 'a', 1)").toString());
}

TEST (JavascriptEngineTests, StringToUpperLowerCase)
{
    JavascriptEngine engine;

    EXPECT_EQ (String ("HELLO"), engine.evaluate ("'hello'.toUpperCase()").toString());
    EXPECT_EQ (String ("hello"), engine.evaluate ("'HELLO'.toLowerCase()").toString());
}

TEST (JavascriptEngineTests, StringTrim)
{
    JavascriptEngine engine;

    EXPECT_EQ (String ("hello"), engine.evaluate ("'  hello  '.trim()").toString());
}

TEST (JavascriptEngineTests, StringIndexOf)
{
    JavascriptEngine engine;

    EXPECT_EQ (1, (int) engine.evaluate ("'hello'.indexOf ('e')"));
    EXPECT_EQ (-1, (int) engine.evaluate ("'hello'.indexOf ('z')"));
}

TEST (JavascriptEngineTests, StringCharAt)
{
    JavascriptEngine engine;

    EXPECT_EQ (String ("e"), engine.evaluate ("'hello'.charAt (1)").toString());
}

TEST (JavascriptEngineTests, StringCharCodeAt)
{
    JavascriptEngine engine;

    EXPECT_EQ (104, (int) engine.evaluate ("'hello'.charCodeAt (0)"));
}

TEST (JavascriptEngineTests, StringSplit)
{
    JavascriptEngine engine;

    engine.execute ("var parts = 'a,b,c'.split (',');");
    EXPECT_EQ (3, (int) engine.evaluate ("parts.length"));
    EXPECT_EQ (String ("a"), engine.evaluate ("parts[0]").toString());
    EXPECT_EQ (String ("b"), engine.evaluate ("parts[1]").toString());
}

// ============================================================================
// Math Class
// ============================================================================

TEST (JavascriptEngineTests, MathConstants)
{
    JavascriptEngine engine;

    EXPECT_NEAR (3.14159, (double) engine.evaluate ("Math.PI"), 0.001);
    EXPECT_NEAR (2.71828, (double) engine.evaluate ("Math.E"), 0.001);
}

TEST (JavascriptEngineTests, MathBasicFunctions)
{
    JavascriptEngine engine;

    EXPECT_EQ (5, (int) engine.evaluate ("Math.abs (-5)"));
    EXPECT_EQ (3, (int) engine.evaluate ("Math.round (3.4)"));
    EXPECT_EQ (4, (int) engine.evaluate ("Math.round (3.6)"));
    EXPECT_EQ (4, (int) engine.evaluate ("Math.ceil (3.1)"));
    EXPECT_EQ (3, (int) engine.evaluate ("Math.floor (3.9)"));
    EXPECT_EQ (10, (int) engine.evaluate ("Math.max (5, 10)"));
    EXPECT_EQ (5, (int) engine.evaluate ("Math.min (5, 10)"));
    EXPECT_EQ (1, (int) engine.evaluate ("Math.sign (42)"));
    EXPECT_EQ (-1, (int) engine.evaluate ("Math.sign (-42)"));
}

TEST (JavascriptEngineTests, MathTrigFunctions)
{
    JavascriptEngine engine;

    EXPECT_NEAR (0.0, (double) engine.evaluate ("Math.sin (0)"), 0.001);
    EXPECT_NEAR (1.0, (double) engine.evaluate ("Math.cos (0)"), 0.001);
    EXPECT_NEAR (0.0, (double) engine.evaluate ("Math.tan (0)"), 0.001);
}

TEST (JavascriptEngineTests, MathPowerAndRoot)
{
    JavascriptEngine engine;

    EXPECT_EQ (8, (int) engine.evaluate ("Math.pow (2, 3)"));
    EXPECT_EQ (25, (int) engine.evaluate ("Math.sqr (5)"));
    EXPECT_EQ (5, (int) engine.evaluate ("Math.sqrt (25)"));
}

TEST (JavascriptEngineTests, MathLogarithm)
{
    JavascriptEngine engine;

    EXPECT_NEAR (2.302, (double) engine.evaluate ("Math.log (10)"), 0.001);
    EXPECT_NEAR (1.0, (double) engine.evaluate ("Math.log10 (10)"), 0.001);
}

// ============================================================================
// Type System
// ============================================================================

TEST (JavascriptEngineTests, TypeofOperator)
{
    JavascriptEngine engine;

    EXPECT_EQ (String ("number"), engine.evaluate ("typeof 42").toString());
    EXPECT_EQ (String ("string"), engine.evaluate ("typeof 'hello'").toString());
    EXPECT_EQ (String ("undefined"), engine.evaluate ("typeof undefined").toString());
    EXPECT_EQ (String ("object"), engine.evaluate ("typeof {}").toString());
    EXPECT_EQ (String ("object"), engine.evaluate ("typeof []").toString());

    engine.execute ("function test() {}");
    EXPECT_EQ (String ("function"), engine.evaluate ("typeof test").toString());
}

TEST (JavascriptEngineTests, UndefinedAndNull)
{
    JavascriptEngine engine;

    EXPECT_TRUE (engine.evaluate ("undefined").isUndefined());
    EXPECT_TRUE (engine.evaluate ("null").isVoid());
    EXPECT_TRUE ((bool) engine.evaluate ("undefined == null"));
}

// ============================================================================
// Numeric Literals
// ============================================================================

TEST (JavascriptEngineTests, HexadecimalLiteral)
{
    JavascriptEngine engine;

    EXPECT_EQ (255, (int) engine.evaluate ("0xFF"));
    EXPECT_EQ (16, (int) engine.evaluate ("0x10"));
}

TEST (JavascriptEngineTests, OctalLiteral)
{
    JavascriptEngine engine;

    EXPECT_EQ (8, (int) engine.evaluate ("010"));
    EXPECT_EQ (64, (int) engine.evaluate ("0100"));
}

TEST (JavascriptEngineTests, FloatLiteral)
{
    JavascriptEngine engine;

    EXPECT_NEAR (3.14, (double) engine.evaluate ("3.14"), 0.001);
    EXPECT_NEAR (1.23e2, (double) engine.evaluate ("1.23e2"), 0.001);
    EXPECT_NEAR (0.5, (double) engine.evaluate (".5"), 0.001);
}

// ============================================================================
// JSON Class
// ============================================================================

TEST (JavascriptEngineTests, JSONStringify)
{
    JavascriptEngine engine;

    engine.execute ("var obj = { x: 10, y: 20 };");
    String json = engine.evaluate ("JSON.stringify (obj)").toString();
    EXPECT_TRUE (json.contains ("10"));
    EXPECT_TRUE (json.contains ("20"));
}

// ============================================================================
// Integer Parsing
// ============================================================================

TEST (JavascriptEngineTests, ParseInt)
{
    JavascriptEngine engine;

    EXPECT_EQ (42, (int) engine.evaluate ("parseInt ('42')"));
    EXPECT_EQ (255, (int) engine.evaluate ("parseInt ('0xFF')"));
    EXPECT_EQ (8, (int) engine.evaluate ("parseInt ('010')"));
}

TEST (JavascriptEngineTests, ParseFloat)
{
    JavascriptEngine engine;

    EXPECT_NEAR (3.14, (double) engine.evaluate ("parseFloat ('3.14')"), 0.001);
}

// ============================================================================
// Comments
// ============================================================================

TEST (JavascriptEngineTests, SingleLineComments)
{
    JavascriptEngine engine;

    engine.execute ("var x = 10; // This is a comment");
    EXPECT_EQ (10, (int) engine.evaluate ("x"));
}

TEST (JavascriptEngineTests, MultiLineComments)
{
    JavascriptEngine engine;

    engine.execute ("var x = /* comment */ 10;");
    EXPECT_EQ (10, (int) engine.evaluate ("x"));

    Result result = engine.execute ("/* Unclosed comment");
    EXPECT_FALSE (result.wasOk());
}

// ============================================================================
// Error Handling
// ============================================================================

TEST (JavascriptEngineTests, ThrowStatement)
{
    JavascriptEngine engine;

    Result result = engine.execute ("throw 'error message';");
    EXPECT_FALSE (result.wasOk());
    EXPECT_TRUE (result.getErrorMessage().contains ("error"));
}

TEST (JavascriptEngineTests, SyntaxErrors)
{
    JavascriptEngine engine;

    EXPECT_FALSE (engine.execute ("var x =").wasOk());
    EXPECT_FALSE (engine.execute ("function () {}").wasOk());
    EXPECT_FALSE (engine.execute ("if (true { }").wasOk());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST (JavascriptEngineTests, DivisionByZero)
{
    JavascriptEngine engine;

    auto result = (double) engine.evaluate ("10 / 0");
    EXPECT_TRUE (std::isinf (result));
}

TEST (JavascriptEngineTests, ModuloByZero)
{
    JavascriptEngine engine;

    auto result = (double) engine.evaluate ("10 % 0");
    EXPECT_TRUE (std::isinf (result));
}

TEST (JavascriptEngineTests, EmptyArraySubscript)
{
    JavascriptEngine engine;

    engine.execute ("var arr = []; arr[5] = 10;");
    EXPECT_TRUE (engine.evaluate ("arr[0]").isUndefined());
    EXPECT_EQ (10, (int) engine.evaluate ("arr[5]"));
}

TEST (JavascriptEngineTests, NestedFunctionCalls)
{
    JavascriptEngine engine;

    engine.execute ("function add (a, b) { return a + b; } function multiply (a, b) { return a * b; }");
    EXPECT_EQ (50, (int) engine.evaluate ("multiply (add (3, 2), 10)"));
}

TEST (JavascriptEngineTests, VarDeclarationMultiple)
{
    JavascriptEngine engine;

    engine.execute ("var x = 1, y = 2, z = 3;");
    EXPECT_EQ (1, (int) engine.evaluate ("x"));
    EXPECT_EQ (2, (int) engine.evaluate ("y"));
    EXPECT_EQ (3, (int) engine.evaluate ("z"));
}

TEST (JavascriptEngineTests, BlockScope)
{
    JavascriptEngine engine;

    engine.execute ("{ var x = 10; { var y = 20; } }");
    EXPECT_EQ (10, (int) engine.evaluate ("x"));
    EXPECT_EQ (20, (int) engine.evaluate ("y"));
}

TEST (JavascriptEngineTests, CallFunctionObject)
{
    JavascriptEngine engine;

    DynamicObject::Ptr scope = new DynamicObject();
    scope->setProperty ("value", 42);

    engine.execute ("function getValue() { return this.value; }");
    var funcObject = engine.getRootObjectProperties()["getValue"];

    var args[] = {};
    Result result = Result::fail ("wrong");
    var returnValue = engine.callFunctionObject (scope, funcObject, var::NativeFunctionArgs (scope.get(), args, 0), &result);

    EXPECT_TRUE (result.wasOk());
    EXPECT_EQ (42, (int) returnValue);
}

TEST (JavascriptEngineTests, UndefinedFunctionCall)
{
    JavascriptEngine engine;

    Result result = engine.execute ("nonExistentFunction();");
    EXPECT_FALSE (result.wasOk());
}

TEST (JavascriptEngineTests, ConstructorFunctionSetsProperties)
{
    JavascriptEngine engine;

    // Test that constructor functions can set properties on 'this'
    engine.execute ("function MyObject (val) { this.x = val; } var obj = new MyObject (10);");
    EXPECT_EQ (10, (int) engine.evaluate ("obj.x"));

    // Test creating multiple instances
    engine.execute ("var obj2 = new MyObject (20);");
    EXPECT_EQ (10, (int) engine.evaluate ("obj.x"));
    EXPECT_EQ (20, (int) engine.evaluate ("obj2.x"));
}
