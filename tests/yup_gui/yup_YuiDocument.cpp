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
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());
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
    ASSERT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_EQ (document.getDiagnostics().front().line, 2);
    EXPECT_EQ (result.getErrorMessage(), document.getDiagnostics().front().toString());
}

TEST_F (YuiDocumentTests, KeepsTheInterfaceWhenTheComponentsCantBeCreated)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString ("Component { Slider { id: knob; } }").wasOk());

    auto* knob = document.findById ("knob");
    const auto* styleSheet = document.getStyleSheet().get();

    const auto result = document.loadFromString ("Component { Slidr { id: knob; } }");

    EXPECT_TRUE (result.failed());
    EXPECT_EQ (document.findById ("knob"), knob);
    EXPECT_EQ (document.getStyleSheet().get(), styleSheet);
    ASSERT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_EQ (result.getErrorMessage(), "1:13: Unknown component type 'Slidr'");

    // The tree goes back to the loaded one, so fixing the text keeps the components
    ASSERT_TRUE (document.loadFromString ("Component { Slider { id: knob; } }").wasOk());
    EXPECT_EQ (document.findById ("knob"), knob);
}

TEST_F (YuiDocumentTests, TellsListenersAboutEveryLoad)
{
    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument& document, const Result& result) override
        {
            loads.add (result.wasOk());
            roots.add (document.getRoot());
        }

        Array<bool> loads;
        Array<Component*> roots;
    };

    YuiDocument document;
    Recorder recorder;
    document.addListener (&recorder);

    ASSERT_TRUE (document.loadFromString ("Component { Slider { id: knob; } }").wasOk());
    auto* root = document.getRoot();

    EXPECT_TRUE (document.loadFromString ("Component { Slider { id: ; } }").failed());
    EXPECT_TRUE (document.loadFromString ("Component { Slidr { id: knob; } }").failed());

    UndoManager undoManager;
    document.setUndoManager (&undoManager);
    ASSERT_TRUE (document.loadFromString ("Column { Slider { id: knob; } }").wasOk());
    auto* column = document.getRoot();

    ASSERT_TRUE (undoManager.undo());
    document.setUndoManager (nullptr);

    EXPECT_EQ (recorder.loads, Array<bool> ({ true, false, false, true, true }));
    EXPECT_EQ (recorder.roots, Array<Component*> ({ root, root, root, column, document.getRoot() }));

    document.removeListener (&recorder);
    ASSERT_TRUE (document.loadFromString ("Component {}").wasOk());
    EXPECT_EQ (recorder.loads.size(), 5);
}

TEST_F (YuiDocumentTests, ReloadingUpdatesTheInterfaceInPlace)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString ("Component { Slider { id: knob; value: 0.25; } Label { id: caption; text: \"a\"; } }").wasOk());

    auto* root = document.getRoot();
    auto* knob = document.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);
    knob->setValue (0.75);

    ASSERT_TRUE (document.loadFromString ("Component { Slider { id: knob; value: 0.25; } Label { id: caption; text: \"b\"; } }").wasOk());

    EXPECT_EQ (document.getRoot(), root);
    EXPECT_EQ (document.findById ("knob"), knob);
    EXPECT_DOUBLE_EQ (knob->getValue(), 0.75);
    EXPECT_EQ (document.findById<Label> ("caption")->getText(), "b");
}

TEST_F (YuiDocumentTests, ReloadsAreUndoable)
{
    UndoManager undoManager;
    YuiDocument document;
    document.setUndoManager (&undoManager);

    ASSERT_TRUE (document.loadFromString ("Component { Label { id: caption; text: \"a\"; } }").wasOk());
    auto* caption = document.findById<Label> ("caption");

    ASSERT_TRUE (document.loadFromString ("Component { Label { id: caption; text: \"b\"; } }").wasOk());
    EXPECT_FALSE (document.loadFromString ("Component { Missing { } }").wasOk());

    EXPECT_TRUE (undoManager.undo());
    EXPECT_EQ (document.findById ("caption"), caption);
    EXPECT_EQ (caption->getText(), "a");

    EXPECT_TRUE (undoManager.redo());
    EXPECT_EQ (caption->getText(), "b");

    EXPECT_TRUE (undoManager.undo());
    EXPECT_TRUE (undoManager.undo());
    EXPECT_EQ (document.getRoot(), nullptr);
}

TEST_F (YuiDocumentTests, ReportsTheValuesThatCantBeApplied)
{
    YuiDocument document;
    const auto result = document.loadFromString ("Component { Slider { id: knob; value: missing * 2; } }");

    EXPECT_TRUE (result.wasOk());
    EXPECT_NE (document.findById ("knob"), nullptr);
    EXPECT_EQ (document.getDiagnostics().size(), 1u);
}

TEST_F (YuiDocumentTests, ReportsWhereTheProblemsOfALoadAreWritten)
{
    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument& document, const Result& result) override {}

        void documentDiagnosed (YuiDocument& document, const YuiDiagnostic& diagnostic) override
        {
            diagnosed.add (diagnostic.toString());
        }

        StringArray diagnosed;
    };

    YuiDocument document;
    Recorder recorder;
    document.addListener (&recorder);

    const auto result = document.loadFromString ("style {\n"
                                                 "    .knob { cursor: finger; }\n"
                                                 "}\n"
                                                 "Component {\n"
                                                 "    Slider { class: \"knob\"; value: missing * 2; }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_EQ (document.getDiagnostics().size(), 2u) << YuiDiagnostic::join (document.getDiagnostics());
    EXPECT_TRUE (document.getDiagnostics()[0].toString().startsWith ("5:29: Slider.value: "));
    EXPECT_EQ (document.getDiagnostics()[1].toString(), "2:13: Unknown cursor 'finger'");

    // The problems of a load come with the load itself
    EXPECT_TRUE (recorder.diagnosed.isEmpty());

    document.removeListener (&recorder);
}

TEST_F (YuiDocumentTests, ReportsWhereTheErrorFailingALoadIsWritten)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString ("Component { }").wasOk());

    const auto result = document.loadFromString ("style {\n    Slider::before { opacity: 1; }\n}\nComponent { }");

    EXPECT_EQ (result.getErrorMessage(), "2:5: Pseudo-elements are not supported in selector 'Slider::before'");
    ASSERT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_EQ (document.getDiagnostics().front().line, 2);
}

TEST_F (YuiDocumentTests, TellsListenersAboutStyleProblemsMetLater)
{
    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument& document, const Result& result) override {}

        void documentDiagnosed (YuiDocument& document, const YuiDiagnostic& diagnostic) override
        {
            diagnosed.add (diagnostic.toString());
            counts.add (static_cast<int> (document.getDiagnostics().size()));
        }

        StringArray diagnosed;
        Array<int> counts;
    };

    YuiDocument document;
    Recorder recorder;
    document.addListener (&recorder);

    ASSERT_TRUE (document.loadFromString ("style {\n    .late { cursor: finger; }\n}\nComponent { Slider { id: knob; } }").wasOk());
    EXPECT_TRUE (document.getDiagnostics().empty());

    auto* knob = document.findById ("knob");
    ASSERT_NE (knob, nullptr);
    auto& context = *document.getRoot()->getStyleContext();

    knob->addStyleClass ("late");
    context.restyleNow();

    EXPECT_EQ (recorder.diagnosed, StringArray ("2:13: Unknown cursor 'finger'"));
    EXPECT_EQ (recorder.counts, Array<int> ({ 1 }));

    // Restyling meets the same problem again, it is reported once
    knob->removeStyleClass ("late");
    context.restyleNow();
    knob->addStyleClass ("late");
    context.restyleNow();

    EXPECT_EQ (recorder.diagnosed.size(), 1);
    EXPECT_EQ (document.getDiagnostics().size(), 1u);

    document.removeListener (&recorder);
}

TEST_F (YuiDocumentTests, UndoingALoadReportsWhereTheRestoredTextWasWritten)
{
    UndoManager undoManager;
    YuiDocument document;
    document.setUndoManager (&undoManager);

    const auto firstLocation = [&]
    {
        return document.getDiagnostics().empty() ? String() : document.getDiagnostics().front().toString().upToFirstOccurrenceOf (" ", false, false);
    };

    ASSERT_TRUE (document.loadFromString ("Component {\n\n    Slider { value: missing; }\n}").wasOk());
    EXPECT_EQ (firstLocation(), "3:14:");

    ASSERT_TRUE (document.loadFromString ("Component { Slider { value: missing; } }").wasOk());
    EXPECT_EQ (firstLocation(), "1:22:");

    ASSERT_TRUE (undoManager.undo());
    EXPECT_EQ (firstLocation(), "3:14:");

    ASSERT_TRUE (undoManager.redo());
    EXPECT_EQ (firstLocation(), "1:22:");

    document.clear();
    EXPECT_TRUE (document.getDiagnostics().empty());
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
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());

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
