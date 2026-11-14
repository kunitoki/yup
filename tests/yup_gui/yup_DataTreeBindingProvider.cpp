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

class DataTreeBindingProviderTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty ("cutoff", 0.4);
        transaction.setProperty ("steps", 4);
        transaction.setProperty ("bypass", false);
        transaction.setProperty ("name", "Lead");
        transaction.setProperty ("curve", var (Array<var> { 1, 2 }));
    }

    struct RecordingListener : BindingProvider::Listener
    {
        void bindingValueChanged (BindingProvider&, const Identifier& name) override { changes.add (name.toString()); }

        StringArray changes;
    };

    DataTree tree { "params" };
};

TEST_F (DataTreeBindingProviderTests, InfersThePropertyTypesFromTheValues)
{
    DataTreeBindingProvider provider (tree);
    const auto properties = provider.getProperties();
    ASSERT_EQ (properties.size(), 5u);

    const auto typeOf = [&] (const char* name)
    {
        for (const auto& property : properties)
            if (property.name == Identifier (name))
                return std::optional<ComponentProperty::Type> (property.type);

        return std::optional<ComponentProperty::Type>();
    };

    EXPECT_EQ (typeOf ("cutoff"), ComponentProperty::Type::number);
    EXPECT_EQ (typeOf ("steps"), ComponentProperty::Type::number);
    EXPECT_EQ (typeOf ("bypass"), ComponentProperty::Type::boolean);
    EXPECT_EQ (typeOf ("name"), ComponentProperty::Type::string);
    EXPECT_EQ (typeOf ("curve"), ComponentProperty::Type::any);
}

TEST_F (DataTreeBindingProviderTests, ReadsAndWritesTheTree)
{
    DataTreeBindingProvider provider (tree);

    EXPECT_DOUBLE_EQ (static_cast<double> (provider.getValue ("cutoff")), 0.4);

    provider.setValue ("cutoff", 0.8);
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty ("cutoff")), 0.8);
}

TEST_F (DataTreeBindingProviderTests, NotifiesTheChangesOfTheTree)
{
    DataTreeBindingProvider provider (tree);
    RecordingListener listener;
    provider.addListener (&listener);

    tree.beginTransaction().setProperty ("cutoff", 0.1);
    provider.setValue ("steps", 8);
    provider.setValue ("steps", 8);

    EXPECT_EQ (listener.changes, StringArray ("cutoff", "steps"));

    provider.removeListener (&listener);
}

TEST_F (DataTreeBindingProviderTests, MakesEachGestureAnUndoTransaction)
{
    UndoManager undoManager;
    DataTreeBindingProvider provider (tree, &undoManager);

    provider.beginGesture ("cutoff");
    provider.setValue ("cutoff", 0.5);
    provider.setValue ("cutoff", 0.6);
    provider.endGesture ("cutoff");

    provider.beginGesture ("cutoff");
    provider.setValue ("cutoff", 0.9);
    provider.endGesture ("cutoff");

    EXPECT_EQ (undoManager.getNumTransactions(), 2);

    undoManager.undo();
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty ("cutoff")), 0.6);

    undoManager.undo();
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty ("cutoff")), 0.4);
}
