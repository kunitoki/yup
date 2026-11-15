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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

class YuiTreeDiffTests : public ::testing::Test
{
protected:
    static DataTree parse (const String& text)
    {
        const auto result = YuiParser::parse (text);
        EXPECT_TRUE (result.wasOk()) << (result.wasOk() ? String() : result.diagnostics.front().toString());
        return result.document;
    }

    static StringArray getPropertyNames (const DataTree& element)
    {
        StringArray names;

        for (int i = 0; i < element.getNumProperties(); ++i)
            names.add (element.getPropertyName (i).toString());

        return names;
    }
};

TEST_F (YuiTreeDiffTests, PatchesTheTreeKeepingTheElementsStillThere)
{
    auto target = parse ("Component { width: 10; Slider { id: knob; value: 1; } }");
    const auto source = parse ("Component { height: 20; Slider { id: knob; value: 2; } }");
    const auto knob = target.getChild (0).getChild (0);

    EXPECT_TRUE (YuiTreeDiff::apply (target, source));

    EXPECT_TRUE (target.isEquivalentTo (source));
    EXPECT_TRUE (target.getChild (0).getChild (0) == knob);
    EXPECT_FALSE (YuiTreeDiff::apply (target, source));
}

TEST_F (YuiTreeDiffTests, MatchesChildrenByIdThenByTypeAndPosition)
{
    auto target = parse ("Component { Label { } Slider { id: a; } Slider { id: b; } }");
    const auto source = parse ("Component { Slider { id: b; } Label { text: \"x\"; } Slider { id: c; } }");

    const auto root = target.getChild (0);
    const auto label = root.getChild (0);
    const auto a = root.getChild (1);
    const auto b = root.getChild (2);

    YuiTreeDiff::apply (target, source);

    EXPECT_TRUE (target.isEquivalentTo (source));
    EXPECT_TRUE (root.getChild (0) == b);
    EXPECT_TRUE (root.getChild (1) == label);
    EXPECT_TRUE (root.getChild (2) == a);
}

TEST_F (YuiTreeDiffTests, ReplacesTheElementsWhoseTypeChanged)
{
    auto target = parse ("Component { Label { id: x; } Slider { } }");
    const auto source = parse ("Component { TextButton { id: x; } Slider { } Slider { } }");

    const auto root = target.getChild (0);
    const auto label = root.getChild (0);
    const auto slider = root.getChild (1);

    YuiTreeDiff::apply (target, source);

    EXPECT_TRUE (target.isEquivalentTo (source));
    EXPECT_FALSE (root.getChild (0) == label);
    EXPECT_TRUE (root.getChild (1) == slider);
}

TEST_F (YuiTreeDiffTests, KeepsThePropertiesInTheOrderOfTheSource)
{
    auto target = parse ("Slider { value: 5; }");
    const auto source = parse ("Slider { maxValue: 10; value: 5; }");

    YuiTreeDiff::apply (target, source);

    EXPECT_EQ (getPropertyNames (target.getChild (0)), StringArray ("maxValue", "value"));
}

TEST_F (YuiTreeDiffTests, RecordsTheChangesAsOneUndoableTransaction)
{
    auto target = parse ("Component { Label { text: \"a\"; } }");
    const auto original = target.clone();
    const auto source = parse ("Component { Label { text: \"b\"; } Slider { } }");

    UndoManager undoManager;
    YuiTreeDiff::apply (target, source, &undoManager);
    EXPECT_TRUE (target.isEquivalentTo (source));

    EXPECT_TRUE (undoManager.undo());
    EXPECT_TRUE (target.isEquivalentTo (original));
}

TEST_F (YuiTreeDiffTests, ReorderingPropertiesNeverExposesEmptyStateAndUsesOneUndoStep)
{
    struct EmptyPropertyWatcher final : DataTree::Listener
    {
        void propertyChanged (DataTree& changed, const Identifier&) override
        {
            if (changed.getNumProperties() == 0)
                sawEmptyProperties = true;
        }

        bool sawEmptyProperties = false;
    };

    auto target = parse ("Slider { minValue: 0; value: 5; }");
    const auto original = target.clone();
    const auto source = parse ("Slider { maxValue: 10; minValue: 0; value: 5; }");

    auto sliderNode = target.getChild (0);
    EmptyPropertyWatcher watcher;
    sliderNode.addListener (&watcher);

    UndoManager undoManager;
    EXPECT_TRUE (YuiTreeDiff::apply (target, source, &undoManager));

    sliderNode.removeListener (&watcher);

    EXPECT_FALSE (watcher.sawEmptyProperties);
    EXPECT_EQ (getPropertyNames (sliderNode), StringArray ("maxValue", "minValue", "value"));
    EXPECT_EQ (static_cast<int> (sliderNode.getProperty ("value")), 5);
    EXPECT_EQ (undoManager.getNumTransactions(), 1);

    EXPECT_TRUE (undoManager.undo());
    EXPECT_TRUE (target.isEquivalentTo (original));
    EXPECT_EQ (getPropertyNames (sliderNode), StringArray ("minValue", "value"));
}
