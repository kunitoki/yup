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

using namespace juce;

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

#if ! JUCE_WASM
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
