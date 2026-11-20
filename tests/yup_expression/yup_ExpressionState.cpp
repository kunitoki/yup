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

#include <yup_expression/yup_expression.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class ExpressionStateTests : public ::testing::Test
{
protected:
    struct Item
    {
        float value = 0.0f;
        int id = 0;
        double weight = 0.0;
    };

    struct ItemResult
    {
        float y = 0.0f;
        int tag = -1;
    };

    void SetUp() override
    {
        value = context.addInput<float> ("value");
        scale = context.addInput<float> ("scale");
        row = context.addInput<int> ("i");
        name = context.addInput<String> ("name");
        values = context.addArray<float> ("values");
    }

    CompiledExpression compile (StringRef text)
    {
        auto result = CompiledExpression::compile (text, context);
        EXPECT_TRUE (result.wasOk()) << (result.failed() ? result.getErrorMessage() : String()).toStdString();
        return result.wasOk() ? result.getValue() : CompiledExpression();
    }

    template <class T>
    static ExpressionOutput<T> output (const CompiledExpression& program, StringRef outputName)
    {
        auto result = program.getOutput<T> (outputName);
        EXPECT_TRUE (result.wasOk());
        return result.wasOk() ? result.getValue() : ExpressionOutput<T> {};
    }

    static std::vector<float> makeValues (int numRows)
    {
        std::vector<float> result;

        for (int i = 0; i < numRows; ++i)
            result.push_back ((float) std::sin (i * 0.37) * 2.0f);

        return result;
    }

    ExpressionContext context;
    ExpressionInput<float> value, scale;
    ExpressionInput<int> row;
    ExpressionInput<String> name;
    ExpressionArray<float> values;
};

//==============================================================================
TEST_F (ExpressionStateTests, EvaluatesScalars)
{
    const auto program = compile ("y = value * scale + i");
    const auto y = output<float> (program, "y");

    ExpressionState state (program);
    state.set (value, 2.0f);
    state.set (scale, 3.0f);
    state.set (row, 1);
    state.evaluate();
    EXPECT_FLOAT_EQ (7.0f, state.get (y));

    state.set (scale, 0.5f);
    state.evaluate();
    EXPECT_FLOAT_EQ (2.0f, state.get (y));
}

TEST_F (ExpressionStateTests, BatchMatchesScalarEvaluation)
{
    constexpr int numRows = 200;
    const auto data = makeValues (numRows);

    const auto program = compile ("let t = value * scale; y = t > 1 ? sin (t) : t * t + i; k = i * 2 + (value > 0 ? 1 : 0)");
    const auto y = output<float> (program, "y");
    const auto k = output<int> (program, "k");

    std::vector<float> ys (numRows);
    std::vector<int> ks (numRows);

    ExpressionState batch (program, 16);
    batch.set (scale, 1.5f);
    batch.bindColumn (value, data.data());
    batch.bindRowIndex (row);
    batch.bindOutputColumn (y, ys.data());
    batch.bindOutputColumn (k, ks.data());
    batch.evaluateRows (numRows);

    ExpressionState scalar (program);
    scalar.set (scale, 1.5f);
    scalar.bindColumn (value, data.data());
    scalar.bindRowIndex (row);

    for (int r = 0; r < numRows; ++r)
    {
        scalar.evaluate (r);
        EXPECT_FLOAT_EQ (scalar.get (y), ys[(size_t) r]) << "row " << r;
        EXPECT_EQ (scalar.get (k), ks[(size_t) r]) << "row " << r;

        const auto t = data[(size_t) r] * 1.5f;
        EXPECT_FLOAT_EQ (t > 1.0f ? std::sin (t) : t * t + (float) r, ys[(size_t) r]) << "row " << r;
    }
}

TEST_F (ExpressionStateTests, ReadsAndWritesStridedColumns)
{
    const Item items[] = { { 1.0f, 10, 0.5 }, { 2.0f, 20, 1.5 }, { 3.0f, 30, 2.5 } };
    ItemResult results[3];

    const auto program = compile ("y = value * scale; tag = i + 1");

    ExpressionState state (program);
    state.set (scale, 2.0f);
    state.bindColumn (value, &items[0].value, sizeof (Item));
    state.bindColumn (row, &items[0].id, sizeof (Item));
    state.bindOutputColumn (output<float> (program, "y"), &results[0].y, sizeof (ItemResult));
    state.bindOutputColumn (output<int> (program, "tag"), &results[0].tag, sizeof (ItemResult));
    state.evaluateRows (3);

    for (int r = 0; r < 3; ++r)
    {
        EXPECT_FLOAT_EQ (items[r].value * 2.0f, results[r].y);
        EXPECT_EQ (items[r].id + 1, results[r].tag);
    }
}

TEST_F (ExpressionStateTests, EvaluatesBlocksIntoLanes)
{
    const auto program = compile ("y = i * 10");
    const auto y = output<int> (program, "y");

    ExpressionState state (program, 8);
    state.bindRowIndex (row);

    EXPECT_EQ (8, state.getBlockSize());
    EXPECT_EQ (8, state.evaluateBlock (100, 20));
    EXPECT_EQ (0, state.evaluateBlock (0, 0));

    for (int lane = 0; lane < 8; ++lane)
        EXPECT_EQ ((100 + lane) * 10, state.get (y, lane));

    EXPECT_EQ (3, state.evaluateBlock (5, 3));
    EXPECT_EQ (70, state.get (y, 2));
}

TEST_F (ExpressionStateTests, UniformInputsChangeBetweenRuns)
{
    const auto data = makeValues (100);
    std::vector<float> ys (100);

    const auto program = compile ("y = value * scale");
    const auto y = output<float> (program, "y");

    ExpressionState state (program, 32);
    state.bindColumn (value, data.data());
    state.bindOutputColumn (y, ys.data());

    state.set (scale, 1.0f);
    state.evaluateRows (100);
    EXPECT_FLOAT_EQ (data[99], ys[99]);

    state.set (scale, 3.0f);
    state.evaluate (0);
    state.evaluateRows (100);
    EXPECT_FLOAT_EQ (data[50] * 3.0f, ys[50]);
    EXPECT_FLOAT_EQ (data[99] * 3.0f, ys[99]);
}

TEST_F (ExpressionStateTests, SetRemovesColumnBindings)
{
    const auto data = makeValues (10);
    std::vector<float> ys (10);

    const auto program = compile ("y = value");
    ExpressionState state (program);
    state.bindColumn (value, data.data());
    state.bindOutputColumn (output<float> (program, "y"), ys.data());

    state.set (value, 5.0f);
    state.evaluateRows (10);

    for (auto v : ys)
        EXPECT_FLOAT_EQ (5.0f, v);

    state.clearBindings();
    ys.assign (10, 0.0f);
    state.evaluateRows (10);

    for (auto v : ys)
        EXPECT_FLOAT_EQ (0.0f, v);
}

TEST_F (ExpressionStateTests, ArraysAreReadPerRow)
{
    const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    std::vector<float> ys (6);

    const auto program = compile ("y = values[i] * len (values)");
    ExpressionState state (program, 4);
    state.bindArray (values, data, 4);
    state.bindRowIndex (row);
    state.bindOutputColumn (output<float> (program, "y"), ys.data());
    state.evaluateRows (6);

    EXPECT_EQ ((std::vector<float> { 4.0f, 8.0f, 12.0f, 16.0f, 16.0f, 16.0f }), ys);
}

TEST_F (ExpressionStateTests, StringsPerLane)
{
    const auto program = compile ("label = name + \"#\" + string (i)");
    const auto label = output<String> (program, "label");
    EXPECT_FALSE (program.isAllocationFree());

    ExpressionState state (program, 8);
    state.set (name, String ("row"));
    state.bindRowIndex (row);
    EXPECT_EQ (5, state.evaluateBlock (10, 5));

    for (int lane = 0; lane < 5; ++lane)
        EXPECT_EQ ("row#" + String (10 + lane), state.get (label, lane));

    std::vector<String> labels (3);
    state.bindOutputColumn (label, labels.data());
    state.evaluateRows (3);
    EXPECT_EQ (String ("row#2"), labels[2]);
}

TEST_F (ExpressionStateTests, SetsAndGetsVars)
{
    const auto program = compile ("y = value * 2; text = name + \"!\"; flag = i > 2");

    ExpressionState state (program);
    EXPECT_TRUE (state.setVar ("value", 1.25).wasOk());
    EXPECT_TRUE (state.setVar ("name", "hi").wasOk());
    EXPECT_TRUE (state.setVar (row.index, 3).wasOk());
    EXPECT_EQ (String ("Unknown input 'missing'"), state.setVar ("missing", 1).getErrorMessage());
    EXPECT_EQ (String ("The input 'values' of type 'float[]' can't be set from a var"), state.setVar ("values", 1).getErrorMessage());
    EXPECT_TRUE (state.setVar (100, 1).failed());

    state.evaluate();
    EXPECT_DOUBLE_EQ (2.5, (double) state.getVar (0));
    EXPECT_EQ (String ("hi!"), state.getVar (1).toString());
    EXPECT_TRUE ((bool) state.getVar (2));
    EXPECT_TRUE (state.getVar (3).isVoid());
}

TEST_F (ExpressionStateTests, UnusedInputsAreIgnored)
{
    const auto program = compile ("y = scale * 2");
    ExpressionState state (program);

    state.set (value, 100.0f);
    state.set (scale, 2.0f);
    EXPECT_TRUE (state.setVar ("i", 7).wasOk());
    state.evaluate();

    EXPECT_FLOAT_EQ (4.0f, state.get (output<float> (program, "y")));
}

TEST_F (ExpressionStateTests, UniformValuesAreComputedOncePerChange)
{
    int numCalls = 0;
    context.addFunction ("counted", [&numCalls] (float x)
    {
        ++numCalls;
        return x * 2.0f;
    });

    const auto data = makeValues (100);
    std::vector<float> ys (100);

    const auto program = compile ("y = counted (scale) + value");
    const auto y = output<float> (program, "y");

    ExpressionState state (program, 16);
    state.set (scale, 2.0f);
    state.bindColumn (value, data.data());
    state.bindOutputColumn (y, ys.data());

    state.evaluateRows (100);
    state.evaluateRows (100);
    EXPECT_EQ (1, numCalls);
    EXPECT_FLOAT_EQ (data[99] + 4.0f, ys[99]);

    state.set (scale, 3.0f);
    state.evaluateRows (100);
    EXPECT_EQ (2, numCalls);
    EXPECT_FLOAT_EQ (data[50] + 6.0f, ys[50]);

    for (int r = 0; r < 10; ++r)
        state.evaluate (r);

    EXPECT_EQ (2, numCalls);
    EXPECT_FLOAT_EQ (data[9] + 6.0f, state.get (y));
}

TEST_F (ExpressionStateTests, BindingsChangeWhatIsComputedPerRow)
{
    int numCalls = 0;
    context.addFunction ("counted", [&numCalls] (float x)
    {
        ++numCalls;
        return x * 2.0f;
    });

    const auto data = makeValues (10);
    std::vector<float> ys (10);

    const auto program = compile ("y = counted (value)");
    ExpressionState state (program, 4);
    state.bindOutputColumn (output<float> (program, "y"), ys.data());

    state.set (value, 1.0f);
    state.evaluateRows (10);
    EXPECT_EQ (1, numCalls);
    EXPECT_EQ (std::vector<float> (10, 2.0f), ys);

    state.bindColumn (value, data.data());
    state.evaluateRows (10);
    EXPECT_EQ (11, numCalls);

    for (int r = 0; r < 10; ++r)
        EXPECT_FLOAT_EQ (data[(size_t) r] * 2.0f, ys[(size_t) r]);

    state.set (value, 4.0f);
    state.evaluateRows (10);
    EXPECT_EQ (12, numCalls);
    EXPECT_EQ (std::vector<float> (10, 8.0f), ys);
}

TEST_F (ExpressionStateTests, ArraysChangedInPlaceAreReadAgain)
{
    int numCalls = 0;
    context.addFunction ("counted", [&numCalls] (float x)
    {
        ++numCalls;
        return x * 2.0f;
    });

    std::vector<float> data { 1.0f, 2.0f, 3.0f };
    std::vector<float> ys (4);

    const auto program = compile ("y = counted (values[1]) + i");
    ExpressionState state (program, 4);
    state.bindArray (values, data.data(), (int) data.size());
    state.bindRowIndex (row);
    state.bindOutputColumn (output<float> (program, "y"), ys.data());

    state.evaluateRows (4);
    EXPECT_EQ (1, numCalls);
    EXPECT_EQ ((std::vector<float> { 4.0f, 5.0f, 6.0f, 7.0f }), ys);

    data[1] = 5.0f;
    state.evaluateRows (4);
    EXPECT_EQ (2, numCalls);
    EXPECT_EQ ((std::vector<float> { 10.0f, 11.0f, 12.0f, 13.0f }), ys);
}

TEST_F (ExpressionStateTests, ImpureFunctionsRunPerRow)
{
    int numCalls = 0;
    context.addFunction ("tick", [&numCalls] (float x)
    {
        ++numCalls;
        return x + (float) numCalls;
    }, false);

    std::vector<float> ys (40);

    const auto program = compile ("y = tick (scale)");
    ExpressionState state (program, 16);
    state.set (scale, 1.0f);
    state.bindOutputColumn (output<float> (program, "y"), ys.data());
    state.evaluateRows (40);

    EXPECT_EQ (40, numCalls);
    EXPECT_FLOAT_EQ (2.0f, ys[0]);
    EXPECT_FLOAT_EQ (41.0f, ys[39]);
}

TEST_F (ExpressionStateTests, GetLanesReturnsTheLastBlock)
{
    const auto program = compile ("y = i * 10");
    const auto y = output<int> (program, "y");

    ExpressionState state (program, 8);
    state.bindRowIndex (row);
    EXPECT_TRUE (state.getLanes (y).empty());
    EXPECT_TRUE (state.getLanes (ExpressionOutput<int> {}).empty());

    state.evaluateBlock (4, 3);
    const auto lanes = state.getLanes (y);
    ASSERT_EQ (3u, lanes.size());
    EXPECT_EQ (40, lanes[0]);
    EXPECT_EQ (60, lanes[2]);
}

TEST_F (ExpressionStateTests, ReportsChangedOutputsAndStopsAtUnchangedValues)
{
    int numCalls = 0;
    context.addFunction ("counted", [&numCalls] (bool x)
    {
        ++numCalls;
        return x ? 1.0f : 0.0f;
    });

    const auto program = compile ("positive = value > 0; level = counted (positive) * scale");
    const auto positive = output<bool> (program, "positive");
    const auto level = output<float> (program, "level");

    ExpressionState state (program);
    EXPECT_TRUE (state.hasChanged());

    state.set (value, 1.0f);
    state.set (scale, 2.0f);
    state.evaluate();
    EXPECT_TRUE (state.hasChanged (positive));
    EXPECT_TRUE (state.hasChanged (level));
    EXPECT_EQ (1, numCalls);

    state.evaluate();
    EXPECT_FALSE (state.hasChanged());

    state.set (value, 2.0f);
    state.evaluate();
    EXPECT_FALSE (state.hasChanged());
    EXPECT_EQ (1, numCalls);

    state.set (scale, 2.0f);
    state.evaluate();
    EXPECT_FALSE (state.hasChanged());

    state.set (scale, 3.0f);
    state.evaluate();
    EXPECT_FALSE (state.hasChanged (positive));
    EXPECT_TRUE (state.hasChanged (level));
    EXPECT_EQ (1, numCalls);
    EXPECT_FLOAT_EQ (3.0f, state.get (level));

    state.set (value, -1.0f);
    state.evaluate();
    EXPECT_TRUE (state.hasChanged (positive));
    EXPECT_TRUE (state.hasChanged (level));
    EXPECT_EQ (2, numCalls);
    EXPECT_FLOAT_EQ (0.0f, state.get (level));
}

TEST_F (ExpressionStateTests, OutputsComputedPerRowAlwaysChange)
{
    const auto program = compile (R"(y = i * 2; s = scale; n = name + "!")");
    const auto y = output<int> (program, "y");
    const auto s = output<float> (program, "s");
    const auto n = output<String> (program, "n");

    ExpressionState state (program, 4);
    state.bindRowIndex (row);
    state.set (name, String ("a"));
    state.evaluateRows (8);
    state.evaluateRows (8);
    EXPECT_TRUE (state.hasChanged (y));
    EXPECT_FALSE (state.hasChanged (s));
    EXPECT_FALSE (state.hasChanged (n));

    state.set (name, String ("a"));
    state.set (scale, -0.0f);
    state.evaluateRows (8);
    EXPECT_FALSE (state.hasChanged (n));
    EXPECT_TRUE (state.hasChanged (s));

    state.set (name, String ("b"));
    state.evaluateRows (8);
    EXPECT_TRUE (state.hasChanged (n));
    EXPECT_EQ (String ("b!"), state.get (n));

    state.clearBindings();
    state.set (row, 1);
    state.evaluate();
    EXPECT_TRUE (state.hasChanged (y));
    state.evaluate();
    EXPECT_FALSE (state.hasChanged());
}
