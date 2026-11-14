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

class YuiDocumentTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (YuiDocumentTests, LoadsTheInterfaceAndAttachesItsStyleSheet)
{
    YuiDocument document;
    const auto result = document.loadFromString (R"yui(
        style {
            .knob { trackWidth: 4 + 2; }
        }

        Component {
            Slider { id: knob; class: "knob"; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().isEmpty()) << document.getDiagnostics().joinIntoString ("\n");
    EXPECT_TRUE (document.getTree().isValid());

    auto* root = document.getRoot();
    ASSERT_NE (root, nullptr);
    ASSERT_NE (document.getStyleSheet(), nullptr);
    EXPECT_EQ (root->getStyleSheet(), document.getStyleSheet());

    auto* knob = document.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> (knob->findProperty (Identifier ("trackWidth"))), 6.0);
}

TEST_F (YuiDocumentTests, LoadsTheImportedFiles)
{
    YuiDocument document;
    const auto result = document.loadFromString (R"yui(
        import "parts.yui";

        Column { Caption { id: caption; } })yui",
                                                 [] (const String& path) -> std::optional<String>
    {
        if (path == "parts.yui")
            return String ("component Caption : Label { text: \"Imported\"; }");

        return std::nullopt;
    });

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* caption = document.findById<Label> ("caption");
    ASSERT_NE (caption, nullptr);
    EXPECT_EQ (caption->getText(), "Imported");
    EXPECT_NE (document.findTypeInfo ("Caption"), nullptr);
}

TEST_F (YuiDocumentTests, KeepsTheInterfaceWhenTheTextHasErrors)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString ("Component { Slider { id: knob; } }").wasOk());

    auto* root = document.getRoot();
    const auto result = document.loadFromString ("Component {\n    Slider { id: ; }\n}");

    EXPECT_TRUE (result.failed());
    EXPECT_EQ (document.getRoot(), root);
    EXPECT_NE (document.findById ("knob"), nullptr);
    ASSERT_TRUE (document.getParseError().has_value());
    EXPECT_EQ (document.getParseError()->line, 2);
    EXPECT_EQ (document.getDiagnostics().size(), 1);
}

TEST_F (YuiDocumentTests, LeavesNothingLoadedWhenTheComponentsCantBeCreated)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString ("Component { Slider { id: knob; } }").wasOk());

    const auto result = document.loadFromString ("Component { Missing { } }");

    EXPECT_TRUE (result.failed());
    EXPECT_EQ (document.getRoot(), nullptr);
    EXPECT_EQ (document.getStyleSheet(), nullptr);
    EXPECT_FALSE (document.getParseError().has_value());
}

TEST_F (YuiDocumentTests, ReportsTheValuesThatCantBeApplied)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Component { Slider { id: knob; value: missing * 2; } }");

    EXPECT_TRUE (result.wasOk());
    EXPECT_NE (document.findById ("knob"), nullptr);
    EXPECT_EQ (document.getDiagnostics().size(), 1);
}

TEST_F (YuiDocumentTests, BindingsFollowTheEnabledStateOfAnAncestor)
{
    YuiDocument document;
    const auto result = document.loadFromString (R"yui(
        Component {
            Column {
                Slider { id: knob; }
                Label { id: state; text: knob.enabled ? "on" : "off"; }
            }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* label = document.findById<Label> ("state");
    ASSERT_NE (label, nullptr);
    EXPECT_EQ (label->getText(), "on");

    document.getRoot()->setEnabled (false);
    EXPECT_EQ (label->getText(), "off");

    document.getRoot()->setEnabled (true);
    EXPECT_EQ (label->getText(), "on");
}

TEST_F (YuiDocumentTests, UndoesTheGesturesMadeOnACompositeLinkedToAModel)
{
    DataTree params ("params");
    params.beginTransaction().setProperty ("cutoff", 0.4);

    UndoManager undoManager;
    DataTreeBindingProvider provider (params, &undoManager);

    YuiDocument document;
    document.setModel ("params", provider);

    const auto result = document.loadFromString (R"yui(
        component LabeledKnob : Column {
            property alias value <=> knob.value;
            Slider { id: knob; }
        }

        Row { LabeledKnob { id: cutoff; value <=> params.cutoff; } })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().isEmpty()) << document.getDiagnostics().joinIntoString ("\n");

    auto* cutoff = document.findById ("cutoff");
    ASSERT_NE (cutoff, nullptr);
    auto* knob = dynamic_cast<Slider*> (cutoff->getChildComponent (0));
    ASSERT_NE (knob, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.4f);

    ComponentTestHelper<Slider>::triggerMouseDown (*knob, MouseEvent {});
    knob->setValue (0.6);
    knob->setValue (0.8);
    ComponentTestHelper<Slider>::triggerMouseUp (*knob, MouseEvent {});

    EXPECT_FLOAT_EQ (static_cast<float> (params.getProperty ("cutoff")), 0.8f);
    EXPECT_EQ (undoManager.getNumTransactions(), 1);

    undoManager.undo();
    EXPECT_FLOAT_EQ (static_cast<float> (params.getProperty ("cutoff")), 0.4f);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.4f);
}
