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
        transaction.setProperty ("names", var (Array<var> { "a", "b" }));
        transaction.setProperty ("mixed", var (Array<var> { 1, "b" }));
        transaction.setProperty ("empty", var (Array<var>()));
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
    ASSERT_EQ (properties.size(), 8u);

    const auto propertyOf = [&] (const char* name)
    {
        for (const auto& property : properties)
            if (property.name == Identifier (name))
                return property;

        return BindingProvider::Property {};
    };

    const auto typeOf = [&] (const char* name)
    {
        return propertyOf (name).type;
    };

    EXPECT_EQ (typeOf ("cutoff"), ComponentProperty::Type::number);
    EXPECT_EQ (typeOf ("steps"), ComponentProperty::Type::number);
    EXPECT_EQ (typeOf ("bypass"), ComponentProperty::Type::boolean);
    EXPECT_EQ (typeOf ("name"), ComponentProperty::Type::string);

    EXPECT_EQ (typeOf ("curve"), ComponentProperty::Type::array);
    EXPECT_EQ (propertyOf ("curve").elementType, ComponentProperty::Type::number);
    EXPECT_EQ (propertyOf ("names").elementType, ComponentProperty::Type::string);
    EXPECT_EQ (propertyOf ("mixed").elementType, ComponentProperty::Type::any);
    EXPECT_EQ (propertyOf ("empty").elementType, ComponentProperty::Type::number);
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

TEST_F (DataTreeBindingProviderTests, ExposesPropertiesAddedAfterConstructionAndClosesGestureTransactions)
{
    UndoManager undoManager;
    DataTreeBindingProvider provider (tree, &undoManager);

    tree.beginTransaction().setProperty ("resonance", 0.7);

    const auto properties = provider.getProperties();
    const auto it = std::find_if (properties.begin(), properties.end(), [] (const BindingProvider::Property& prop)
    {
        return prop.name == Identifier ("resonance");
    });

    ASSERT_NE (it, properties.end());
    EXPECT_EQ (it->type, ComponentProperty::Type::number);
    EXPECT_DOUBLE_EQ (static_cast<double> (provider.getValue ("resonance")), 0.7);

    provider.beginGesture ("cutoff");
    provider.setValue ("cutoff", 0.5);
    provider.setValue ("cutoff", 0.6);
    provider.endGesture ("cutoff");

    // Writing outside the gesture must start a separate UndoManager transaction
    provider.setValue ("steps", 12);
    EXPECT_EQ (undoManager.getNumTransactions(), 2);

    ASSERT_TRUE (undoManager.undo());
    EXPECT_EQ (static_cast<int> (tree.getProperty ("steps")), 4);
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty ("cutoff")), 0.6);

    ASSERT_TRUE (undoManager.undo());
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty ("cutoff")), 0.4);
}

TEST_F (DataTreeBindingProviderTests, FindsIndividualPropertiesAndHandlesMissingNames)
{
    DataTreeBindingProvider provider (tree);

    const auto cutoff = provider.findProperty ("cutoff");
    ASSERT_TRUE (cutoff.has_value());
    EXPECT_EQ (cutoff->name, Identifier ("cutoff"));
    EXPECT_EQ (cutoff->type, ComponentProperty::Type::number);

    const auto curve = provider.findProperty ("curve");
    ASSERT_TRUE (curve.has_value());
    EXPECT_EQ (curve->type, ComponentProperty::Type::array);
    EXPECT_EQ (curve->elementType, ComponentProperty::Type::number);

    const auto mixed = provider.findProperty ("mixed");
    ASSERT_TRUE (mixed.has_value());
    EXPECT_EQ (mixed->type, ComponentProperty::Type::array);
    EXPECT_EQ (mixed->elementType, ComponentProperty::Type::any);

    EXPECT_FALSE (provider.findProperty ("missing").has_value());

    tree.beginTransaction().setProperty ("drive", 0.9);
    const auto drive = provider.findProperty ("drive");
    ASSERT_TRUE (drive.has_value());
    EXPECT_EQ (drive->type, ComponentProperty::Type::number);
}
