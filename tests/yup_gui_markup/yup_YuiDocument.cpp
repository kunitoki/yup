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

#include <yup_gui_markup/yup_gui_markup.h>

#include <gtest/gtest.h>

#include "yup_YuiTestHelpers.h"

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
        if (folder != File())
            folder.deleteRecursively();

        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    // Writes a file of the folder the tests reading assets share, made when the first one is written
    File write (const String& path, const String& text)
    {
        if (folder == File())
        {
            folder = File::getSpecialLocation (File::tempDirectory)
                         .getChildFile ("YUP_YuiDocumentTests_" + String::toHexString (Random::getSystemRandom().nextInt()));

            folder.deleteRecursively();
        }

        auto file = folder.getChildFile (path);
        file.getParentDirectory().createDirectory();
        file.replaceWithText (text);
        return file;
    }

    static MemoryBlock bytesOf (const String& text)
    {
        return MemoryBlock (text.toRawUTF8(), text.getNumBytesAsUTF8());
    }

    static String textOf (const std::optional<MemoryBlock>& bytes)
    {
        return bytes.has_value() ? bytes->toString() : String ("<nothing>");
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
    File folder;
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
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_TRUE (document.getTree().isValid());

    auto* root = document.getRoot();
    ASSERT_NE (root, nullptr);
    ASSERT_NE (document.getStyleSheet(), nullptr);
    EXPECT_EQ (root->getStyleSheet(), document.getStyleSheet());

    auto* knob = document.findById<Slider> ("knob");
    ASSERT_NE (knob, nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> (knob->findProperty (Identifier ("trackWidth"))), 6.0);
}

TEST_F (YuiDocumentTests, CachesToTextureTheComponentsAnElementOrAStyleRuleAsksTo)
{
    YuiDocument document;
    const auto result = document.loadFromString (R"yui(
        style {
            .heavy { cached: true; }
        }

        Component {
            Component { id: panel; cached: true; }
            Component { id: plate; class: "heavy"; }
            Component { id: plain; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* plate = document.findById ("plate");
    ASSERT_NE (plate, nullptr);

    EXPECT_TRUE (document.findById ("panel")->isCachedToTexture());
    EXPECT_TRUE (plate->isCachedToTexture());
    EXPECT_FALSE (document.findById ("plain")->isCachedToTexture());

    plate->removeStyleClass (Identifier ("heavy"));
    document.getRoot()->getStyleContext()->restyleNow();

    EXPECT_FALSE (plate->isCachedToTexture());
}

TEST_F (YuiDocumentTests, APaletteWithADarkAndALightVersionFollowsTheTheme)
{
    theme->setPalette (ThemePalette ({}, ThemePalette::Mode::dark));

    YuiDocument document;
    const auto result = document.loadFromString (R"yui(
        palette Skin { colors: "#101418 #ff8800 #f2efe9"; }
        palette Skin { mode: light; colors: "#f4f1ea #0066ff #202020"; }

        style {
            #styled { palette: Skin; }
        }

        Component {
            Component { id: styled; Label { id: inStyled; } }
            Component { id: given; palette: Skin; Label { id: inGiven; } }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto& context = *document.getRoot()->getStyleContext();
    const auto* inStyled = document.findById ("inStyled");
    const auto* inGiven = document.findById ("inGiven");
    ASSERT_NE (inStyled, nullptr);
    ASSERT_NE (inGiven, nullptr);

    const auto backgroundOf = [this] (const Component& component)
    {
        return theme->getPalette (component).getColor (ThemePalette::Role::background);
    };

    context.refresh (0.0);
    EXPECT_EQ (backgroundOf (*inStyled), Color (0xff101418));
    EXPECT_EQ (backgroundOf (*inGiven), Color (0xff101418));

    theme->setPalette (ThemePalette ({}, ThemePalette::Mode::light));
    context.refresh (0.0);
    EXPECT_EQ (backgroundOf (*inStyled), Color (0xfff4f1ea));
    EXPECT_EQ (backgroundOf (*inGiven), Color (0xfff4f1ea));

    theme->setPalette (ThemePalette ({}, ThemePalette::Mode::dark));
    context.refresh (0.0);
    EXPECT_EQ (backgroundOf (*inStyled), Color (0xff101418));
    EXPECT_EQ (backgroundOf (*inGiven), Color (0xff101418));
}

TEST_F (YuiDocumentTests, ARepeatOfComponentsFollowsItsModelAndKeepsTheStyles)
{
    DataTree tree ("sequencer");
    tree.beginTransaction().setProperty ("length", 2);

    DataTreeBindingProvider sequencer (tree, nullptr);

    YuiDocument document;
    document.setModel ("seq", sequencer);

    const auto result = document.loadFromString (R"yui(
        style {
            .step { opacity: 0.5; }
        }

        Row {
            id: row;
            repeat i in 0 .. seq.length { Label { class: step; text: format ("%d", i); } }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
    ASSERT_EQ (document.findById ("row")->getNumChildComponents(), 2);

    const auto sheet = document.getRoot()->getStyleSheet();

    tree.beginTransaction().setProperty ("length", 3);
    document.applyPendingChanges();

    auto* row = document.findById ("row");
    ASSERT_EQ (row->getNumChildComponents(), 3);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    // The document is as it was, so its stylesheet is kept and what it runs goes on
    EXPECT_EQ (document.getRoot()->getStyleSheet(), sheet);

    // The component added is styled as the others
    row->getStyleContext()->restyleNow();
    EXPECT_NEAR (row->getChildComponent (2)->getOpacity(), 0.5f, 0.01f);
}

TEST_F (YuiDocumentTests, ARepeatOfComponentsOnlyHoldsComponents)
{
    YuiDocument document;

    EXPECT_TRUE (document.loadFromString ("Row { repeat i in 0 .. 2 { rect { } } }").failed());
    EXPECT_TRUE (document.loadFromString ("Row { repeat i in 0 .. 2 { Label { } repeat j in 0 .. 2 { Label { } } } }").wasOk());
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

TEST_F (YuiDocumentTests, InitializersSetUpTheComponentsEachLoadCreates)
{
    YuiDocument document;
    StringArray initialized;

    document.setInitializer<Label> ("title", [&] (Label& label) { initialized.add ("the " + label.getComponentID()); });
    document.setInitializer<Label> ([&] (Label& label) { initialized.add ("any " + label.getComponentID()); });

    ASSERT_TRUE (document.loadFromString ("Component { Label { id: title; } }").wasOk());
    EXPECT_EQ (initialized.joinIntoString (", "), "any title, the title");

    // A reload keeps the component it still describes, and sets up the one it creates
    ASSERT_TRUE (document.loadFromString ("Component { Label { id: title; } Label { id: caption; } }").wasOk());
    EXPECT_EQ (initialized.joinIntoString (", "), "any title, the title, any caption");

    document.setInitializer<Label> ("title", nullptr);
    document.setInitializer<Label> (nullptr);
    document.clear();

    ASSERT_TRUE (document.loadFromString ("Component { Label { id: title; } }").wasOk());
    EXPECT_EQ (initialized.size(), 3);
}

TEST_F (YuiDocumentTests, ReloadingWithTheSameStyleKeepsTheStyleSheetAndItsTransitions)
{
    const String style = "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }";
    const auto components = [] (const String& classes, const String& caption)
    {
        return "Component { Component { id: knob; class: \"" + classes + "\"; } Label { id: caption; text: \"" + caption + "\"; } }";
    };

    const auto glow = [] (const Component& c) { return static_cast<double> (c.findProperty (Identifier ("glow"))); };

    YuiDocument document;
    ASSERT_TRUE (document.loadFromString (style + components ("knob", "a")).wasOk());

    const auto sheet = document.getStyleSheet();
    auto* knob = document.findById ("knob");
    ASSERT_NE (knob, nullptr);
    auto& context = *document.getRoot()->getStyleContext();

    // The style is unchanged, so the class added by the reload transitions like one added from code
    ASSERT_TRUE (document.loadFromString (style + components ("knob lit", "a")).wasOk());
    EXPECT_EQ (document.getStyleSheet(), sheet);
    context.refresh (0.0);
    context.refresh (0.25);
    EXPECT_DOUBLE_EQ (glow (*knob), 2.5);

    ASSERT_TRUE (document.loadFromString (style + components ("knob lit", "b")).wasOk());
    EXPECT_EQ (document.getStyleSheet(), sheet);
    EXPECT_EQ (document.findById<Label> ("caption")->getText(), "b");
    EXPECT_DOUBLE_EQ (glow (*knob), 2.5);

    context.refresh (0.25);
    EXPECT_DOUBLE_EQ (glow (*knob), 5.0);

    // Moving the style after the components changes where its declarations are written
    ASSERT_TRUE (document.loadFromString (components ("knob lit", "b") + style).wasOk());
    EXPECT_NE (document.getStyleSheet(), sheet);

    const auto moved = document.getStyleSheet();
    ASSERT_TRUE (document.loadFromString ("style { .knob { glow: 1; } }" + components ("knob", "b")).wasOk());
    EXPECT_NE (document.getStyleSheet(), moved);
    EXPECT_DOUBLE_EQ (glow (*document.findById ("knob")), 1.0);
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

        void documentDiagnosed (YuiDocument& document, const DataTreeDiagnostic& diagnostic) override
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
    ASSERT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());
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

        void documentDiagnosed (YuiDocument& document, const DataTreeDiagnostic& diagnostic) override
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

TEST_F (YuiDocumentTests, BoundEnabledHoldsUnderAnAncestorEnabledByALaterSibling)
{
    DataTree params ("params");
    {
        auto transaction = params.beginTransaction();
        transaction.setProperty ("power", true);
        transaction.setProperty ("section", false);
    }
    DataTreeBindingProvider provider (params);

    YuiDocument document;
    document.setModel ("params", provider);

    const auto result = document.loadFromString (R"yui(
        Component {
            Component {
                id: deck;
                enabled: power.checked;

                Component {
                    id: section;
                    enabled: params.section;

                    Slider { id: knob; }
                }
            }

            ToggleButton { id: power; checked <=> params.power; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* deck = document.findById ("deck");
    auto* section = document.findById ("section");
    auto* knob = document.findById ("knob");
    ASSERT_NE (deck, nullptr);
    ASSERT_NE (section, nullptr);
    ASSERT_NE (knob, nullptr);

    EXPECT_TRUE (deck->isEnabled());
    EXPECT_FALSE (section->isEnabled());
    EXPECT_FALSE (knob->isEnabled());

    params.beginTransaction().setProperty ("section", true);
    EXPECT_TRUE (section->isEnabled());
    EXPECT_TRUE (knob->isEnabled());

    params.beginTransaction().setProperty ("power", false);
    EXPECT_FALSE (deck->isEnabled());
    EXPECT_FALSE (knob->isEnabled());

    {
        auto transaction = params.beginTransaction();
        transaction.setProperty ("section", false);
        transaction.setProperty ("power", true);
    }
    EXPECT_TRUE (deck->isEnabled());
    EXPECT_FALSE (section->isEnabled());
    EXPECT_FALSE (knob->isEnabled());
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
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* cutoff = document.findById ("cutoff");
    ASSERT_NE (cutoff, nullptr);
    auto* knob = dynamic_cast<Slider*> (cutoff->getChildComponent (0));
    ASSERT_NE (knob, nullptr);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.4f);

    ComponentTestHelper<YuiBuilder>::triggerMouseDown (*knob, MouseEvent {});
    knob->setValue (0.6);
    knob->setValue (0.8);
    ComponentTestHelper<YuiBuilder>::triggerMouseUp (*knob, MouseEvent {});

    EXPECT_FLOAT_EQ (static_cast<float> (params.getProperty ("cutoff")), 0.8f);
    EXPECT_EQ (undoManager.getNumTransactions(), 1);

    undoManager.undo();
    EXPECT_FLOAT_EQ (static_cast<float> (params.getProperty ("cutoff")), 0.4f);
    EXPECT_FLOAT_EQ (static_cast<float> (knob->getValue()), 0.4f);
}

TEST_F (YuiDocumentTests, ClearNotifiesListenersAndFailedLoadClearsStaleSourceMap)
{
    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument& doc, const Result& result) override
        {
            results.add (result.wasOk());
            roots.add (doc.getRoot());
        }

        void documentDiagnosed (YuiDocument&, const DataTreeDiagnostic& diagnostic) override
        {
            lateDiagnostics.add (diagnostic.toString());
        }

        Array<bool> results;
        Array<Component*> roots;
        StringArray lateDiagnostics;
    };

    YuiDocument document;
    Recorder recorder;
    document.addListener (&recorder);

    ASSERT_TRUE (document.loadFromString ("style {\n\n\n    .late { cursor: finger; }\n}\nComponent { Slider { id: knob; } }").wasOk());
    ASSERT_EQ (recorder.results.size(), 1);
    EXPECT_NE (recorder.roots[0], nullptr);

    const auto failed = document.loadFromString ("Component {\n    Slider {" );
    EXPECT_TRUE (failed.failed());
    ASSERT_FALSE (document.getDiagnostics().empty());
    EXPECT_EQ (document.getDiagnostics().front().line, 2);

    auto* knob = document.findById ("knob");
    ASSERT_NE (knob, nullptr);
    knob->addStyleClass ("late");
    document.getRoot()->getStyleContext()->restyleNow();
    EXPECT_TRUE (recorder.lateDiagnostics.isEmpty());

    document.clear();
    ASSERT_EQ (recorder.results.size(), 3);
    EXPECT_TRUE (recorder.results[2]);
    EXPECT_EQ (recorder.roots[2], nullptr);

    document.removeListener (&recorder);
}

TEST_F (YuiDocumentTests, DrawingsReadBindingProviderModelsDirectly)
{
    DataTree params ("params");
    params.beginTransaction().setProperty ("cutoff", 0.5);
    DataTreeBindingProvider provider (params);

    YuiDocument document;
    document.setModel ("params", provider);

    const auto result = document.loadFromString (R"yui(
        drawing Meter {
            rect { bounds: rect (0, 0, bounds.width * params.cutoff, bounds.height); fill: #ff0000; }
        }
        style {
            #meter { drawing: Meter; }
        }
        Component {
            Component { id: meter; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    params.beginTransaction().setProperty ("cutoff", 0.9);
    EXPECT_TRUE (document.getDiagnostics().empty());

    document.removeModel ("params");
    params.beginTransaction().setProperty ("cutoff", 0.1);
    EXPECT_TRUE (document.getDiagnostics().empty());
}

TEST_F (YuiDocumentTests, ReportsBindingCyclesDuringLoadAndAtRuntimeWithSourceLocations)
{
    {
        YuiDocument document;
        const auto result = document.loadFromString ("Component {\n"
                                                     "    Slider { id: a; minimum: 0; maximum: 100; value: b.value + 1; }\n"
                                                     "    Slider { id: b; minimum: 0; maximum: 100; value: a.value + 1; }\n"
                                                     "}");

        ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
        ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
        EXPECT_TRUE (document.getDiagnostics().front().line == 2 || document.getDiagnostics().front().line == 3);
        EXPECT_TRUE (document.getDiagnostics().front().toString().contains ("Binding cycle detected"));
    }

    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument&, const Result&) override {}

        void documentDiagnosed (YuiDocument&, const DataTreeDiagnostic& diagnostic) override
        {
            diagnosed.add (diagnostic.toString());
        }

        StringArray diagnosed;
    };

    YuiDocument document;
    Recorder recorder;
    document.addListener (&recorder);

    const auto result = document.loadFromString ("Component {\n"
                                                 "    ToggleButton { id: gate; checked: false; }\n"
                                                 "    Slider { id: a; minimum: 0; maximum: 100; value: gate.checked ? b.value + 1 : 0; }\n"
                                                 "    Slider { id: b; minimum: 0; maximum: 100; value: a.value + 1; }\n"
                                                 "}");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* gate = document.findById<ToggleButton> ("gate");
    ASSERT_NE (gate, nullptr);
    gate->setToggleState (true, dontSendNotification);

    ASSERT_EQ (recorder.diagnosed.size(), 1);
    EXPECT_TRUE (recorder.diagnosed[0].contains ("Binding cycle detected"));
    ASSERT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_TRUE (document.getDiagnostics().front().line == 3 || document.getDiagnostics().front().line == 4);

    document.removeListener (&recorder);
}

TEST_F (YuiDocumentTests, LoadFromStringWithoutImportResolverFailsWhenDocumentContainsImport)
{
    YuiDocument document;
    const auto result = document.loadFromString ("import \"missing.yui\";\nComponent { }");

    EXPECT_TRUE (result.failed());
    ASSERT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_EQ (document.getDiagnostics().front().toString(), "1:1: Can't import 'missing.yui'");
}

TEST_F (YuiDocumentTests, ValidatesDocumentTreesAgainstSchemaAndReportsLocations)
{
    const auto schema = YuiDocument::createSchema();
    ASSERT_NE (schema, nullptr);
    EXPECT_TRUE (schema->isValid());
    EXPECT_TRUE (schema->hasNodeType (Identifier ("yui")));
    EXPECT_TRUE (schema->hasNodeType (Identifier ("style")));
    EXPECT_TRUE (schema->hasNodeType (Identifier ("rule")));
    EXPECT_TRUE (schema->hasNodeType (Identifier ("Slider")));

    const auto validParsed = YuiParser::parse ("style { Slider { trackWidth: 4; } }\nColumn { Slider { id: knob; } }");
    ASSERT_TRUE (validParsed.wasOk());
    EXPECT_TRUE (schema->validate (validParsed.document).wasOk());

    // Loading a valid document first so we verify a malformed DataTree keeps the loaded UI intact.
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString ("Column { Slider { id: knob; } }").wasOk());
    auto* existingKnob = document.findById ("knob");
    ASSERT_NE (existingKnob, nullptr);

    // Construct a malformed DataTree: a `rule` missing its required `selector` property, with a source map entry.
    DataTree brokenDoc ("yui");
    DataTree styleBlock ("style");
    DataTree badRule ("rule");
    DataTree rootComp ("Column");
    styleBlock.beginTransaction().addChild (badRule);
    {
        auto tx = brokenDoc.beginTransaction();
        tx.addChild (styleBlock);
        tx.addChild (rootComp);
    }

    EXPECT_TRUE (schema->validate (brokenDoc).failed());

    DataTreeSourceMap map;
    map.add (badRule, {}, 7, 5, "broken.yui");

    const auto badLoad = document.load (brokenDoc, map);
    EXPECT_TRUE (badLoad.failed());
    EXPECT_EQ (document.findById ("knob"), existingKnob);
    ASSERT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_EQ (document.getDiagnostics().front().source, "broken.yui");
    EXPECT_EQ (document.getDiagnostics().front().line, 7);
    EXPECT_EQ (document.getDiagnostics().front().column, 5);
}

TEST_F (YuiDocumentTests, HoldsATreeBuiltInCodeToTheDepthOfAText)
{
    const auto nested = [] (int depth)
    {
        DataTree document ("yui");
        auto parent = document;

        for (int level = 0; level < depth; ++level)
        {
            DataTree child ("Component");
            parent.beginTransaction().addChild (child);
            parent = child;
        }

        return document;
    };

    YuiDocument document;
    ASSERT_TRUE (document.load (nested (8)).wasOk());
    auto* root = document.getRoot();

    const auto result = document.load (nested (200));
    EXPECT_TRUE (result.failed());
    EXPECT_TRUE (result.getErrorMessage().contains ("nested")) << result.getErrorMessage();
    EXPECT_EQ (document.getRoot(), root);
}

TEST_F (YuiDocumentTests, LiveTreeMutationsUpdateComponentsAndStyleSheetAutomatically)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString (R"yui(
        style {
            Slider { trackWidth: 6; }
        }
        Column {
            Slider { id: knob; value: 0.2; }
            Label { id: title; text: "Init"; }
        }
    )yui")
                     .wasOk());

    auto* knob = document.findById<Slider> ("knob");
    auto* title = document.findById<Label> ("title");
    ASSERT_NE (knob, nullptr);
    ASSERT_NE (title, nullptr);
    EXPECT_DOUBLE_EQ (static_cast<double> (knob->findProperty (Identifier ("trackWidth"))), 6.0);

    // Mutate the live DataTree returned by getTree(): both style declarations and component properties update in place.
    auto liveTree = document.getTree();
    auto styleDecl = liveTree.getChild (0).getChild (0).getChild (0);
    auto labelNode = liveTree.getChild (1).getChild (1);

    labelNode.beginTransaction().setProperty ("text", "\"Live\"");

    // The interface follows once the changes are over, from the message loop, which a test stands for
    EXPECT_EQ (title->getText(), "Init");
    document.applyPendingChanges();

    EXPECT_EQ (document.findById<Label> ("title"), title);
    EXPECT_EQ (title->getText(), "Live");

    styleDecl.beginTransaction().setProperty ("value", "12");
    document.applyPendingChanges();

    EXPECT_EQ (document.findById<Slider> ("knob"), knob);
    EXPECT_DOUBLE_EQ (static_cast<double> (knob->findProperty (Identifier ("trackWidth"))), 12.0);
}

TEST_F (YuiDocumentTests, TellsListenersEachTimeTheInterfaceIsBuiltAgainWithoutALoad)
{
    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument& document, const Result& result) override
        {
            loads.add (result.wasOk());
            numProblems.add (static_cast<int> (document.getDiagnostics().size()));
        }

        void documentDiagnosed (YuiDocument&, const DataTreeDiagnostic&) override
        {
            ++numDiagnosed;
        }

        Array<bool> loads;
        Array<int> numProblems;
        int numDiagnosed = 0;
    };

    DataTree tree ("sequencer");
    tree.beginTransaction().setProperty ("length", 1);

    DataTreeBindingProvider sequencer (tree, nullptr);

    YuiDocument document;
    document.setModel ("seq", sequencer);

    Recorder recorder;
    document.addListener (&recorder);

    ASSERT_TRUE (document.loadFromString ("Row { id: row; Slider { id: knob; value: 0.5; } repeat i in 0 .. seq.length { Label {} } }").wasOk());
    auto* root = document.getRoot();

    // A change of the tree that keeps the root, and has a problem to list
    auto row = document.getTree().getChild (0);
    row.getChild (0).beginTransaction().setProperty ("value", "missing");
    EXPECT_EQ (recorder.loads, Array<bool> ({ true }));

    document.applyPendingChanges();
    EXPECT_EQ (document.getRoot(), root);
    EXPECT_EQ (recorder.loads, Array<bool> ({ true, true }));
    EXPECT_EQ (recorder.numProblems, Array<int> ({ 0, 1 }));

    // A change of a range
    tree.beginTransaction().setProperty ("length", 3);
    document.applyPendingChanges();
    EXPECT_EQ (document.findById ("row")->getNumChildComponents(), 4);
    EXPECT_EQ (recorder.loads, Array<bool> ({ true, true, true }));

    // A change of the tree the interface can't be built for, which is kept as it was
    row.getChild (0).beginTransaction().setProperty ("value", "0.25");
    row.beginTransaction().addChild (DataTree ("Missing"));
    document.applyPendingChanges();
    EXPECT_EQ (document.findById ("row")->getNumChildComponents(), 4);
    EXPECT_EQ (recorder.loads, Array<bool> ({ true, true, true, false }));

    // Both changing lead to one build
    row.beginTransaction().removeChild (2);
    tree.beginTransaction().setProperty ("length", 2);
    document.applyPendingChanges();
    EXPECT_EQ (document.findById ("row")->getNumChildComponents(), 3);
    EXPECT_DOUBLE_EQ (document.findById<Slider> ("knob")->getValue(), 0.25);
    EXPECT_EQ (recorder.loads, Array<bool> ({ true, true, true, false, true }));
    EXPECT_EQ (recorder.numProblems.getLast(), 0);

    // The problems of a build are told with it, not one by one
    EXPECT_EQ (recorder.numDiagnosed, 0);

    document.removeListener (&recorder);
}

TEST_F (YuiDocumentTests, AProblemHasNoPositionOnceElementsAreAddedRemovedOrMovedInTheTree)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString ("Column {\n    Slider { id: knob; value: missing; }\n    Label { id: caption; }\n}").wasOk());

    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().line, 2);

    // A property changing moves no element: the problem is still where it was written
    auto column = document.getTree().getChild (0);
    column.getChild (1).beginTransaction().setProperty ("text", "\"Gain\"");
    document.applyPendingChanges();

    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().line, 2);
    EXPECT_TRUE (document.getSourceMap() != nullptr);

    // An element added before the slider puts it where the label was written, which is not its line
    column.beginTransaction().addChild (DataTree ("Label"), 0);
    document.applyPendingChanges();

    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_TRUE (document.getDiagnostics().front().message.startsWith ("Slider.value: ")) << document.getDiagnostics().front().message;
    EXPECT_EQ (document.getDiagnostics().front().line, 0);
    EXPECT_TRUE (document.getSourceMap() == nullptr);

    // Until a text is loaded
    ASSERT_TRUE (document.loadFromString ("Column {\n    Label {}\n    Slider { id: knob; value: missing; }\n}").wasOk());
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().line, 3);
}

TEST_F (YuiDocumentTests, ADrawingTakesArraysForTheValuesNumbersMake)
{
    YuiDocument document;
    const auto result = document.loadFromString (R"yui(
        drawing Marks {
            rect { id: box; bounds: [4, 6, 20, 10]; cornerSize: [3, 2]; fill: #ffffff; stroke: #ffffff; dash: [6, 3]; }
            circle { id: dot; center: [30, 30]; radius: [3, 5, 8][1]; fill: #ffffff; stroke: #ffffff; dash: [8, 6, 2, 6, 1, 6, 3]; }
            line { from: [0, 0]; to: [10, 10]; stroke: #ffffff; }
        }

        Component {
            Component { id: painted; drawing: Marks; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* painted = document.findById ("painted");
    auto* style = painted != nullptr ? dynamic_cast<DrawingStyle*> (painted->getStyle().get()) : nullptr;
    ASSERT_NE (style, nullptr);

    const auto boundsOf = [measured = style->measureElements (*painted)] (const String& id)
    {
        for (const auto& element : measured)
            if (element.id == id)
                return element.bounds;

        return Rectangle<float>();
    };

    EXPECT_EQ (boundsOf ("box"), Rectangle<float> (4.0f, 6.0f, 20.0f, 10.0f));
    EXPECT_EQ (boundsOf ("dot"), Rectangle<float> (25.0f, 25.0f, 10.0f, 10.0f));
}

TEST_F (YuiDocumentTests, ADrawingSetOnAComponentReadsTheComponentsOfTheLastLoad)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString (R"yui(
        drawing Dot { circle { id: dot; center: point (0, 0); radius: src.width; fill: #ffffff; } }

        Component {
            Slider { id: src; width: 10; }
            Component { id: painted; drawing: Dot; }
        })yui")
                     .wasOk());

    const auto dotWidth = [&document]
    {
        auto* painted = document.findById ("painted");
        auto* style = painted != nullptr ? dynamic_cast<DrawingStyle*> (painted->getStyle().get()) : nullptr;
        const auto measured = style != nullptr ? style->measureElements (*painted) : std::vector<DrawingStyle::ElementBounds>();

        return measured.empty() ? -1.0f : measured.front().bounds.getWidth();
    };

    EXPECT_FLOAT_EQ (dotWidth(), 20.0f);

    // The component the drawing reads is made again, the one it paints is kept
    auto* painted = document.findById ("painted");
    const auto previousStyle = painted->getStyle();

    ASSERT_TRUE (document.loadFromString (R"yui(
        drawing Dot { circle { id: dot; center: point (0, 0); radius: src.width; fill: #ffffff; } }

        Component {
            Label { id: src; width: 30; }
            Component { id: painted; drawing: Dot; }
        })yui")
                     .wasOk());

    EXPECT_EQ (document.findById ("painted"), painted);
    EXPECT_FLOAT_EQ (dotWidth(), 60.0f);

    // The drawing is made again too: the one of the load before finds components where that load kept them
    EXPECT_NE (painted->getStyle(), previousStyle);

    // Kept by something, that one finds nothing of this load: it shows what it read last
    auto* kept = dynamic_cast<DrawingStyle*> (previousStyle.get());
    ASSERT_NE (kept, nullptr);

    const auto measured = kept->measureElements (*painted);
    ASSERT_FALSE (measured.empty());
    EXPECT_FLOAT_EQ (measured.front().bounds.getWidth(), 20.0f);
}

TEST_F (YuiDocumentTests, AnElementLosesTheDrawingItCantSelectAnymore)
{
    const String dot ("drawing Dot { circle { id: dot; center: point (0, 0); radius: src.width; fill: #ffffff; } }");

    const auto textOf = [] (const String& drawing, const String& selected)
    {
        return drawing + " Component { Slider { id: src; width: 10; } Component { id: painted; " + selected + " } }";
    };

    YuiDocument document;
    ASSERT_TRUE (document.loadFromString (textOf (dot, "drawing: Dot;")).wasOk());

    auto* painted = document.findById ("painted");
    ASSERT_NE (painted, nullptr);

    // A load makes the drawing of an element again or leaves it none: the one of the load before is the drawing as it
    // was, reading the components where that load had them
    const auto expectNoDrawing = [&] (const String& text, const String& problem)
    {
        ASSERT_TRUE (document.loadFromString (textOf (dot, "drawing: Dot;")).wasOk());
        ASSERT_EQ (document.findById ("painted"), painted);
        ASSERT_NE (painted->getStyle(), nullptr);

        ASSERT_TRUE (document.loadFromString (text).wasOk()) << text;
        ASSERT_EQ (document.findById ("painted"), painted);
        EXPECT_EQ (painted->getStyle(), nullptr) << text;

        // What prevents the drawing is told once, by the build
        const auto& diagnostics = document.getDiagnostics();
        ASSERT_EQ (diagnostics.size(), problem.isEmpty() ? 0u : 1u) << DataTreeDiagnostic::join (diagnostics);

        if (problem.isNotEmpty())
            EXPECT_TRUE (diagnostics.front().message.startsWith (problem)) << diagnostics.front().message;
    };

    // A drawing that does not compile anymore
    expectNoDrawing (textOf ("drawing Dot { circle { id: dot; center: point (0, 0); radius: src.width; fill: nope; } }", "drawing: Dot;"),
                     "Component.drawing: Dot/circle#dot.fill: ");

    // A name that is no drawing, written bare or as a text
    expectNoDrawing (textOf (dot, "drawing: Missing;"), "Component.drawing: unknown drawing 'Missing'");
    expectNoDrawing (textOf (dot, "drawing: \"Missing\";"), "Component.drawing: unknown drawing 'Missing'");

    // No drawing selected anymore
    expectNoDrawing (textOf (dot, ""), {});
}

TEST_F (YuiDocumentTests, LoadingTheDocumentThatIsLoadedIsNoStepToUndo)
{
    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument&, const Result&) override
        {
            ++loads;
        }

        int loads = 0;
    };

    UndoManager undoManager;
    YuiDocument document;
    Recorder recorder;
    document.setUndoManager (&undoManager);
    document.addListener (&recorder);

    // The style has a problem in both texts, the first has one of its own
    const String style ("style { #knob { cursor: finger; } } ");
    const String broken (style + "Component { Slider { id: knob; value: missing; } }");
    const String fixed (style + "Component { Slider { id: knob; value: 0.5; } }");

    ASSERT_TRUE (document.loadFromString (broken).wasOk());
    ASSERT_TRUE (document.loadFromString (fixed).wasOk());
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    document.getBuilder().onDiagnostic ({ .message = "told since the load" });
    ASSERT_EQ (document.getDiagnostics().size(), 2u);

    // The text that is loaded is loaded again as any other: the listeners are told, and its problems are the ones
    // the builder and the stylesheet find again. Nothing is recorded
    const auto numTransactions = undoManager.getNumTransactions();

    ASSERT_TRUE (document.loadFromString (fixed).wasOk());
    EXPECT_EQ (recorder.loads, 3);
    EXPECT_EQ (undoManager.getNumTransactions(), numTransactions);
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().message, "Unknown cursor 'finger'");

    // So one step back is the text before
    ASSERT_TRUE (undoManager.undo());
    EXPECT_EQ (document.getDiagnostics().size(), 2u);

    // Which keeps what can be redone when it is loaded again
    ASSERT_TRUE (document.loadFromString (broken).wasOk());
    EXPECT_EQ (document.getDiagnostics().size(), 2u);

    ASSERT_TRUE (undoManager.canRedo());
    ASSERT_TRUE (undoManager.redo());
    EXPECT_EQ (document.getDiagnostics().size(), 1u);

    // Another text is a step to undo
    ASSERT_TRUE (document.loadFromString (broken).wasOk());
    EXPECT_EQ (document.getDiagnostics().size(), 2u);

    ASSERT_TRUE (undoManager.undo());
    EXPECT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_TRUE (undoManager.canRedo());

    document.removeListener (&recorder);
}

TEST_F (YuiDocumentTests, ARangeChangeKeepsTheProblemsTheBuilderDidNotTell)
{
    DataTree tree ("sequencer");
    tree.beginTransaction().setProperty ("length", 1);

    DataTreeBindingProvider sequencer (tree, nullptr);

    YuiDocument document;
    document.setModel ("seq", sequencer);

    ASSERT_TRUE (document.loadFromString (R"yui(
        style { #row { cursor: finger; } }

        Row {
            id: row;
            repeat i in 0 .. seq.length { Slider { value: missing; } }
        })yui")
                     .wasOk());

    // A component tells what it finds wrong later where the builder tells its own, as a scene does for a model it can't read
    document.getBuilder().onDiagnostic ({ .message = "Model3D.source: can't read 'ship.glb'" });

    const auto countOf = [&document] (const String& start)
    {
        const auto& diagnostics = document.getDiagnostics();

        return std::count_if (diagnostics.begin(), diagnostics.end(), [&start] (const DataTreeDiagnostic& diagnostic)
        {
            return diagnostic.message.startsWith (start);
        });
    };

    ASSERT_EQ (document.getDiagnostics().size(), 3u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (countOf ("Slider.value: "), 1);
    EXPECT_EQ (countOf ("Unknown cursor 'finger'"), 1);
    EXPECT_EQ (countOf ("Model3D.source: "), 1);

    // The repeat has no turn left: what the builder told about them is gone, what the others told still holds
    tree.beginTransaction().setProperty ("length", 0);
    document.applyPendingChanges();

    ASSERT_EQ (document.findById ("row")->getNumChildComponents(), 0);
    EXPECT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (countOf ("Unknown cursor 'finger'"), 1);
    EXPECT_EQ (countOf ("Model3D.source: "), 1);

    // And the builder tells its own again with the turns
    tree.beginTransaction().setProperty ("length", 2);
    document.applyPendingChanges();

    EXPECT_EQ (document.getDiagnostics().size(), 3u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (countOf ("Slider.value: "), 1);
}

TEST_F (YuiDocumentTests, RepeatsInsideOneAnotherShareTheTurnsARepeatRuns)
{
    YuiDocument document;
    const auto result = document.loadFromString (R"yui(
        Component {
            id: grid;
            repeat i in 0 .. 64 { repeat j in 0 .. 64 { Component {} } }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (document.findById ("grid")->getNumChildComponents(), 1024);
    EXPECT_TRUE (DataTreeDiagnostic::join (document.getDiagnostics()).contains ("A repeat runs at most 16 times")) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (YuiDocumentTests, ARepeatStillFollowsItsModelAfterALoadThatFailed)
{
    DataTree tree ("sequencer");
    tree.beginTransaction().setProperty ("length", 2);

    DataTreeBindingProvider sequencer (tree, nullptr);

    YuiDocument document;
    document.setModel ("seq", sequencer);

    ASSERT_TRUE (document.loadFromString ("Row { id: row; repeat i in 0 .. seq.length { Label {} } }").wasOk());
    EXPECT_TRUE (document.loadFromString ("Row {").failed());
    ASSERT_EQ (document.getDiagnostics().size(), 1u);

    tree.beginTransaction().setProperty ("length", 3);
    document.applyPendingChanges();

    EXPECT_EQ (document.findById ("row")->getNumChildComponents(), 3);

    // The problem of the load is still the one to show
    EXPECT_EQ (document.getDiagnostics().size(), 1u);
}

TEST_F (YuiDocumentTests, ReadsTheBytesOfAnAssetThroughItsReader)
{
    static constexpr uint8 bundle[] = { 0x52, 0x49, 0x46, 0x46, 0x00, 0xff, 0x01, 0x00 };

    int numAsked = 0;

    YuiDocument document;
    document.setAssetReader ([&numAsked] (const String& path) -> std::optional<MemoryBlock>
    {
        ++numAsked;

        if (path == "fx/crt.ysl")
            return MemoryBlock (bundle, sizeof (bundle));

        return std::nullopt;
    });

    const auto read = document.readAsset ("fx/crt.ysl");
    ASSERT_TRUE (read.has_value());
    EXPECT_TRUE (read->matches (bundle, sizeof (bundle)));

    // What the reader doesn't have is read from its file, and there is none
    EXPECT_FALSE (document.readAsset ("fx/missing.ysl").has_value());
    EXPECT_EQ (numAsked, 2);

    // An empty path is no asset: the reader isn't even asked
    EXPECT_FALSE (document.readAsset (String()).has_value());
    EXPECT_EQ (numAsked, 2);

    document.setAssetReader (nullptr);
    EXPECT_FALSE (document.readAsset ("fx/crt.ysl").has_value());
}

TEST_F (YuiDocumentTests, TheAssetReaderWinsOverAFileAndLeavesWhatItDoesntHaveToTheFiles)
{
    write ("fx/crt.frag", "crt from the file");
    write ("fx/glow.frag", "glow from the file");

    YuiDocument document;
    document.setBaseDirectory (folder);
    document.setAssetReader ([] (const String& path) -> std::optional<MemoryBlock>
    {
        if (path == "fx/crt.frag")
            return bytesOf ("crt from the reader");

        return std::nullopt;
    });

    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "crt from the reader");
    EXPECT_EQ (textOf (document.readAsset ("fx/glow.frag")), "glow from the file");

    document.setAssetReader (nullptr);
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "crt from the file");
}

TEST_F (YuiDocumentTests, ReadsTheAssetsThatAreFilesOfTheBaseDirectoryOrAtAnAbsolutePath)
{
    const auto shader = write ("fx/crt.frag", "void main() {}");
    const auto noise = write ("textures/noise.png", "pixels");

    YuiDocument document;

    // An absolute path is read as it is, with or without a base directory
    EXPECT_EQ (textOf (document.readAsset (shader.getFullPathName())), "void main() {}");

    document.setBaseDirectory (folder.getChildFile ("fx"));
    EXPECT_EQ (textOf (document.readAsset ("crt.frag")), "void main() {}");
    EXPECT_EQ (textOf (document.readAsset ("../textures/noise.png")), "pixels");
    EXPECT_EQ (textOf (document.readAsset (noise.getFullPathName())), "pixels");
}

TEST_F (YuiDocumentTests, AMissingAssetAndAnEmptyPathGiveNothing)
{
    write ("fx/crt.frag", "void main() {}");

    YuiDocument document;
    document.setBaseDirectory (folder);

    EXPECT_FALSE (document.readAsset ("fx/missing.frag").has_value());
    EXPECT_FALSE (document.readAsset (folder.getChildFile ("fx/missing.frag").getFullPathName()).has_value());
    EXPECT_FALSE (document.readAsset (String()).has_value());

    // A folder isn't a file
    EXPECT_FALSE (document.readAsset ("fx").has_value());
}

TEST_F (YuiDocumentTests, ReturnsTheFileAnAssetPathStandsFor)
{
    const auto shader = write ("fx/crt.frag", "void main() {}");
    const auto workingDirectory = File::getCurrentWorkingDirectory();

    YuiDocument document;

    // With no base directory a relative path is one of the working directory, as for Image and the drawings
    EXPECT_EQ (document.getAssetFile ("fx/crt.frag"), workingDirectory.getChildFile ("fx/crt.frag"));
    EXPECT_EQ (document.getAssetFile (shader.getFullPathName()), shader);
    EXPECT_EQ (document.getAssetFile (String()), File());

    // The base directory is looked into first, a file it doesn't have is one of the working directory
    document.setBaseDirectory (folder);
    EXPECT_EQ (document.getAssetFile ("fx/crt.frag"), shader);
    EXPECT_EQ (document.getAssetFile ("fx/missing.frag"), workingDirectory.getChildFile ("fx/missing.frag"));

    // What a reader serves with no folder to look into isn't a file, unless its path is absolute
    document.setBaseDirectory ({});
    document.setAssetReader ([] (const String&) -> std::optional<MemoryBlock>
    {
        return std::nullopt;
    });

    EXPECT_EQ (document.getAssetFile ("fx/crt.frag"), File());
    EXPECT_EQ (document.getAssetFile (shader.getFullPathName()), shader);

    document.setBaseDirectory (folder);
    EXPECT_EQ (document.getAssetFile ("fx/crt.frag"), shader);
}

TEST_F (YuiDocumentTests, ReadsTheAssetsOfAComponentThroughItsDocument)
{
    const auto shader = write ("fx/crt.frag", "crt from the file");

    YuiDocument document;
    document.setAssetReader ([] (const String& path) -> std::optional<MemoryBlock>
    {
        if (path == "fx/crt.frag")
            return bytesOf ("crt from the reader");

        return std::nullopt;
    });

    ASSERT_TRUE (document.loadFromString ("Component { Slider { id: knob; } }").wasOk());

    auto* knob = document.findById ("knob");
    ASSERT_NE (knob, nullptr);

    EXPECT_EQ (textOf (YuiDocument::readAssetFor (document.getRoot(), "fx/crt.frag")), "crt from the reader");
    EXPECT_EQ (textOf (YuiDocument::readAssetFor (knob, "fx/crt.frag")), "crt from the reader");

    // A child added in code to a component of the document reads through the document too
    Component added;
    knob->addChildComponent (added);
    EXPECT_EQ (textOf (YuiDocument::readAssetFor (&added, "fx/crt.frag")), "crt from the reader");

    // A component of no document reads the file by itself, with no reader and no base directory
    Component alone;
    EXPECT_FALSE (YuiDocument::readAssetFor (&alone, "fx/crt.frag").has_value());
    EXPECT_EQ (textOf (YuiDocument::readAssetFor (&alone, shader.getFullPathName())), "crt from the file");
    EXPECT_EQ (textOf (YuiDocument::readAssetFor (nullptr, shader.getFullPathName())), "crt from the file");
    EXPECT_FALSE (YuiDocument::readAssetFor (nullptr, String()).has_value());
}

TEST_F (YuiDocumentTests, ListsTheProblemsItsComponentsReportWhereTheyAreWritten)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString ("Component {\n    Slider { id: knob; }\n}").wasOk());

    auto* knob = document.findById ("knob");
    ASSERT_NE (knob, nullptr);

    // A component finds its document and reports through it, with no element of its own to give
    auto* found = ComponentDocument::findFor (knob);
    ASSERT_EQ (found, static_cast<ComponentDocument*> (&document));

    found->reportDiagnostic ({ .message = "Slider.value: out of range", .property = "value" }, knob);
    found->reportDiagnostic ({ .message = "Looks odd", .severity = DataTreeDiagnostic::Severity::warning });

    ASSERT_EQ (document.getDiagnostics().size(), 2u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics()[0].toString(), "2:5: Slider.value: out of range");
    EXPECT_EQ (document.getDiagnostics()[1].toString(), "warning: Looks odd");
}

TEST_F (YuiDocumentTests, TellsTheListenersOfItsComponentsAfterItsOwn)
{
    struct Own final : YuiDocument::Listener
    {
        explicit Own (StringArray& callsToAddTo)
            : calls (callsToAddTo)
        {
        }

        void documentLoaded (YuiDocument&, const Result&) override { calls.add ("own loaded"); }

        void documentAssetChanged (YuiDocument&, const String& path) override { calls.add ("own " + path); }

        StringArray& calls;
    };

    struct OfComponent final : ComponentDocument::Listener
    {
        explicit OfComponent (StringArray& callsToAddTo)
            : calls (callsToAddTo)
        {
        }

        void documentLoaded (ComponentDocument&) override { calls.add ("component loaded"); }

        void documentAssetChanged (ComponentDocument&, const String& path) override { calls.add ("component " + path); }

        StringArray& calls;
    };

    StringArray calls;
    Own own (calls);
    OfComponent ofComponent (calls);

    YuiDocument document;
    document.addListener (&ofComponent);
    document.addListener (&own);

    ASSERT_TRUE (document.loadFromString ("Component { }").wasOk());
    document.notifyAssetChanged ("fx/crt.frag");

    EXPECT_EQ (calls.joinIntoString (", "), "own loaded, component loaded, own fx/crt.frag, component fx/crt.frag");

    document.removeListener (&own);
    document.removeListener (&ofComponent);
}

TEST_F (YuiDocumentTests, TellsListenersAboutTheAssetsThatChanged)
{
    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument&, const Result&) override {}

        void documentAssetChanged (YuiDocument& document, const String& path) override
        {
            documents.add (&document);
            paths.add (path);
        }

        Array<YuiDocument*> documents;
        StringArray paths;
    };

    YuiDocument document;
    Recorder recorder;
    document.addListener (&recorder);

    document.notifyAssetChanged ("fx/crt.frag");
    document.notifyAssetChanged ("models/device.glb");

    EXPECT_EQ (recorder.paths, StringArray ({ "fx/crt.frag", "models/device.glb" }));
    EXPECT_EQ (recorder.documents, Array<YuiDocument*> ({ &document, &document }));

    document.removeListener (&recorder);
    document.notifyAssetChanged ("fx/crt.frag");
    EXPECT_EQ (recorder.paths.size(), 2);
}

TEST_F (YuiDocumentTests, LoadsShadersAndMakesTheStyleSheetAgainWhenOneChanges)
{
    const String rest = "shader Hologram { material: \"fx/hologram.glsl\"; property rim: color = #00ffff; } Component { Component { id: screen; intensity: 0.25; } }";
    const auto crt = [] (const String& intensity)
    {
        return "shader Crt { fragment: \"fx/crt.frag\"; property intensity: number = " + intensity + "; } ";
    };

    YuiDocument document;
    const auto result = document.loadFromString (crt ("0.5") + rest);

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto sheet = document.getStyleSheet();
    ASSERT_NE (sheet, nullptr);
    EXPECT_TRUE (sheet->findShader ("Crt").isValid());
    EXPECT_TRUE (sheet->findShader ("Hologram").isValid());

    // The same shaders keep the stylesheet, a changed one makes it again
    ASSERT_TRUE (document.loadFromString (crt ("0.5") + rest).wasOk());
    EXPECT_EQ (document.getStyleSheet(), sheet);

    ASSERT_TRUE (document.loadFromString (crt ("0.8") + rest).wasOk());
    ASSERT_NE (document.getStyleSheet(), nullptr);
    EXPECT_NE (document.getStyleSheet(), sheet);
    EXPECT_EQ (document.getStyleSheet()->findShader ("Crt").getChild (0).getProperty ("value").toString(), "0.8");

    // An invalid shader keeps the interface loaded, and is reported where it is written
    auto* screen = document.findById ("screen");
    const auto failed = document.loadFromString ("Component { }\nshader Crt { }");

    EXPECT_EQ (failed.getErrorMessage(), "2:1: The shader 'Crt' needs a 'fragment' or a 'material'");
    EXPECT_EQ (document.findById ("screen"), screen);
}

TEST_F (YuiDocumentTests, TheSchemaDescribesShaders)
{
    const auto schema = YuiDocument::createSchema();
    ASSERT_NE (schema, nullptr);
    EXPECT_TRUE (schema->hasNodeType (Identifier ("shader")));

    const auto names = schema->getPropertyNames (Identifier ("shader"));
    EXPECT_EQ (names.size(), 3);

    for (const auto* name : { "name", "fragment", "material" })
        EXPECT_TRUE (names.contains (name)) << name;

    EXPECT_TRUE (schema->getPropertyInfo (Identifier ("shader"), Identifier ("name")).required);
    EXPECT_FALSE (schema->getPropertyInfo (Identifier ("shader"), Identifier ("fragment")).required);
    EXPECT_FALSE (schema->getPropertyInfo (Identifier ("shader"), Identifier ("material")).required);
    EXPECT_EQ (schema->getChildConstraints (Identifier ("shader")).allowedTypes, StringArray ("property"));
    EXPECT_TRUE (schema->getChildConstraints (Identifier ("yui")).allowedTypes.contains ("shader"));

    const auto parsed = YuiParser::parse ("shader Crt { fragment: \"fx/crt.frag\"; property intensity: number = 0.5; }\nColumn { }");
    ASSERT_TRUE (parsed.wasOk());
    EXPECT_TRUE (schema->validate (parsed.document).wasOk());

    // A tree made by hand is held to the same shape
    YuiDocument document;

    EXPECT_TRUE (document.load (DataTree ("yui", { DataTree ("shader", { { "fragment", "fx/crt.frag" } }), DataTree ("Column") })).failed());
    ASSERT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_EQ (document.getDiagnostics().front().message, "Required property 'name' is missing on 'shader'");

    EXPECT_TRUE (document.load (DataTree ("yui", { DataTree ("shader", { { "name", "Crt" }, { "fragment", "fx/crt.frag" } }, { DataTree ("rect") }), DataTree ("Column") })).failed());
    ASSERT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_EQ (document.getDiagnostics().front().message, "Child type 'rect' is not allowed in parent 'shader'");
}

TEST_F (YuiDocumentTests, AFilterTakesTheShadersItRunsAndWritesThemBack)
{
    const String text (R"yui(
        shader Crt { fragment: "fx/crt.frag"; property intensity: number = 0.5; }

        drawing Screen {
            layer {
                filter { blur { deviation: 2; result: soft; } use Crt { in: soft; intensity: 0.8; } }
                rect { bounds: bounds; fill: #ffffff; }
            }
        }

        Component { }
    )yui");

    // The grammar takes a `use` among the primitives of a filter
    const auto schema = YuiDocument::createSchema();
    ASSERT_NE (schema, nullptr);
    EXPECT_TRUE (schema->getChildConstraints (Identifier ("filter")).allowedTypes.contains ("use"));

    const auto parsed = YuiParser::parse (text);
    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    const auto filter = parsed.document.getChildWithName ("drawing").getChildWithName ("layer").getChildWithName ("filter");
    ASSERT_EQ (filter.getNumChildren(), 2);

    const auto use = filter.getChild (1);
    EXPECT_EQ (use.getType().toString(), "use");
    EXPECT_EQ (use.getProperty ("drawing").toString(), "Crt");
    EXPECT_EQ (use.getProperty ("in").toString(), "soft");
    EXPECT_EQ (use.getProperty ("intensity").toString(), "0.8");

    // It is written back where it is, and read again as it was
    const auto written = YuiWriter::write (parsed.document);
    EXPECT_TRUE (written.contains ("            use Crt {\n                in: soft;\n                intensity: 0.8;\n            }\n")) << written;

    const auto reparsed = YuiParser::parse (written);
    ASSERT_TRUE (reparsed.wasOk()) << written;
    EXPECT_TRUE (reparsed.document.isEquivalentTo (parsed.document)) << written;

    // A document holding it loads: a drawing nothing paints loads no shader
    YuiDocument document;
    const auto result = document.loadFromString (text);

    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

TEST_F (YuiDocumentTests, TheSchemaDescribesTheGrammarAndTheComponentTypes)
{
    const auto schema = YuiDocument::createSchema();
    ASSERT_NE (schema, nullptr);

    const auto childrenOf = [&schema] (const char* type)
    {
        return schema->getChildConstraints (Identifier (type));
    };

    const auto propertyOf = [&schema] (const char* type, const char* name)
    {
        return schema->getPropertyInfo (Identifier (type), Identifier (name));
    };

    // Each element of the grammar once, and each registered type
    EXPECT_EQ (schema->getNodeTypeNames().size(), 53 + static_cast<int> (ComponentTypeRegistry::getInstance().getRegisteredTypes().size()));

    // The document holds its blocks, then the components of every type
    const auto inDocument = childrenOf ("yui").allowedTypes;
    const StringArray blocks ({ "import", "tokens", "palette", "style", "drawing", "shader", "component" });
    ASSERT_GT (inDocument.size(), blocks.size());

    for (int index = 0; index < blocks.size(); ++index)
        EXPECT_EQ (inDocument[index], blocks[index]);

    EXPECT_TRUE (inDocument.contains ("Slider"));
    EXPECT_EQ (childrenOf ("yui").maxCount, -1);

    // The properties of an element, with what tells them and whether they have to be written
    EXPECT_EQ (schema->getPropertyNames (Identifier ("palette")).size(), 3);
    EXPECT_TRUE (propertyOf ("palette", "name").required);
    EXPECT_EQ (propertyOf ("palette", "name").description, "Palette name.");
    EXPECT_EQ (propertyOf ("palette", "colors").type, "string");
    EXPECT_FALSE (propertyOf ("palette", "mode").required);
    EXPECT_EQ (propertyOf ("palette", "mode").description, "Palette mode (dark or light).");
    EXPECT_EQ (childrenOf ("palette").maxCount, 0);

    const auto at = propertyOf ("keyframe", "at");
    EXPECT_EQ (at.type, "number");
    EXPECT_TRUE (at.required);
    ASSERT_TRUE (at.minimum.has_value() && at.maximum.has_value());
    EXPECT_DOUBLE_EQ (*at.minimum, 0.0);
    EXPECT_DOUBLE_EQ (*at.maximum, 1.0);
    EXPECT_EQ (childrenOf ("keyframe").allowedTypes, StringArray ("declaration"));

    // The elements each one holds, in their order
    const StringArray drawn ({ "let", "repeat", "group", "clip", "layer", "viewport", "use", "drawing", "pattern", "marker", "rect", "ellipse",
                               "circle", "line", "arc", "path", "polygon", "star", "vertex", "part", "text", "image", "svg" });

    auto inDrawing = drawn;
    inDrawing.insert (0, "property");

    auto inLayer = drawn;
    inLayer.add ("clipPath");
    inLayer.add ("mask");
    inLayer.add ("filter");

    EXPECT_EQ (childrenOf ("group").allowedTypes, drawn);
    EXPECT_EQ (childrenOf ("repeat").allowedTypes, drawn);
    EXPECT_EQ (childrenOf ("drawing").allowedTypes, inDrawing);
    EXPECT_EQ (childrenOf ("layer").allowedTypes, inLayer);
    EXPECT_EQ (childrenOf ("style").allowedTypes, StringArray ({ "rule", "palette", "drawing", "keyframes", "property" }));
    EXPECT_EQ (childrenOf ("filter").allowedTypes, StringArray ({ "blur", "offset", "flood", "colorMatrix", "composite", "blend", "morphology", "lighting", "displacement", "dropShadow", "use" }));

    // A component block holds anything, a shape nothing
    EXPECT_TRUE (propertyOf ("component", "base").required);
    EXPECT_TRUE (childrenOf ("component").allowedTypes.isEmpty());
    EXPECT_EQ (childrenOf ("component").maxCount, -1);
    EXPECT_TRUE (schema->getPropertyNames (Identifier ("rect")).isEmpty());
    EXPECT_EQ (childrenOf ("rect").maxCount, 0);

    // A component type has what every element is written with, its properties and signals and the ones of its bases
    const auto ofSlider = schema->getPropertyNames (Identifier ("Slider"));

    for (const auto* name : { "id", "class", "palette", "drawing", "value", "minimum", "visible", "onValueChanged" })
        EXPECT_TRUE (ofSlider.contains (name)) << name;

    EXPECT_EQ (propertyOf ("Slider", "id").description, "Component identifier.");
    EXPECT_EQ (propertyOf ("Slider", "minimum").description, "The lowest value of the range.");
    EXPECT_FALSE (propertyOf ("Slider", "value").required);
    EXPECT_TRUE (childrenOf ("Slider").allowedTypes.isEmpty());

    // A tree is held to the grammar and its components to what builds them, which takes the values a tree made in code
    // holds as they are: numbers, where a text has what was written
    YuiDocument document;
    const auto result = document.load (DataTree ("yui", { DataTree ("Column", { DataTree ("Slider", { { "id", "knob" }, { "maximum", 10 }, { "value", 2.5 } }) }) }));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    ASSERT_NE (document.findById<Slider> ("knob"), nullptr);
    EXPECT_DOUBLE_EQ (document.findById<Slider> ("knob")->getValue(), 2.5);
}

TEST_F (YuiDocumentTests, AChangeInsideAComponentOrTokensBlockKeepsTheStyleSheetUnlessItDeclaresOtherProperties)
{
    const auto text = [] (const String& tokens, const String& declared, const String& caption, const String& classes)
    {
        return "tokens { " + tokens + " } "
             + "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } } "
             + "component Panel : Column { " + declared + " Label { text: \"" + caption + "\"; } } "
             + "Component { Component { id: knob; class: \"" + classes + "\"; } Panel { id: panel; } }";
    };

    const auto glow = [] (const Component& c) { return static_cast<double> (c.findProperty (Identifier ("glow"))); };

    YuiDocument document;
    ASSERT_TRUE (document.loadFromString (text ("property spacing: number = 4;", "property title: string = \"a\";", "one", "knob")).wasOk());

    const auto sheet = document.getStyleSheet();
    auto* knob = document.findById ("knob");
    ASSERT_NE (knob, nullptr);
    auto& context = *document.getRoot()->getStyleContext();

    // A stylesheet reads neither what a component is made of nor the value of a token: it is kept, and the class the
    // same load adds transitions like one added from code
    ASSERT_TRUE (document.loadFromString (text ("property spacing: number = 8;", "property title: string = \"a\";", "two", "knob lit")).wasOk());
    EXPECT_EQ (document.getStyleSheet(), sheet);
    EXPECT_EQ (document.findById ("knob"), knob);
    EXPECT_EQ (dynamic_cast<Label*> (document.findById ("panel")->getChildComponent (0))->getText(), "two");
    context.refresh (0.0);
    context.refresh (0.25);
    EXPECT_DOUBLE_EQ (glow (*knob), 2.5);

    // Nor the value a declared property starts with
    ASSERT_TRUE (document.loadFromString (text ("property spacing: number = 8;", "property title: string = \"b\";", "two", "knob lit")).wasOk());
    EXPECT_EQ (document.getStyleSheet(), sheet);
    EXPECT_DOUBLE_EQ (glow (*knob), 2.5);

    context.refresh (0.25);
    EXPECT_DOUBLE_EQ (glow (*knob), 5.0);

    // It reads the properties the blocks declare: another type, one more or another name make it again
    ASSERT_TRUE (document.loadFromString (text ("property spacing: number = 8;", "property title: color = #ff0000;", "two", "knob lit")).wasOk());
    EXPECT_NE (document.getStyleSheet(), sheet);

    const auto retyped = document.getStyleSheet();
    ASSERT_TRUE (document.loadFromString (text ("property spacing: number = 8; property corner: number = 2;", "property title: color = #ff0000;", "two", "knob lit")).wasOk());
    EXPECT_NE (document.getStyleSheet(), retyped);

    const auto extended = document.getStyleSheet();
    ASSERT_TRUE (document.loadFromString (text ("property unit: number = 8; property corner: number = 2;", "property title: color = #ff0000;", "two", "knob lit")).wasOk());
    EXPECT_NE (document.getStyleSheet(), extended);
}

TEST_F (YuiDocumentTests, AChangeOfTheTreeKeepsTheStyleSheetAndItsTransitionsWhenTheStyleIsTheSame)
{
    YuiDocument document;
    ASSERT_TRUE (document.loadFromString ("style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }\n"
                                          "Component {\n"
                                          "    Component { id: knob; class: \"knob\"; }\n"
                                          "    Slider { id: slider; value: missing; }\n"
                                          "    Label { id: caption; text: \"a\"; }\n"
                                          "}")
                     .wasOk());

    // The line the problem of the slider is listed at, or none when it is not listed
    const auto findSliderProblem = [&document]
    {
        for (const auto& diagnostic : document.getDiagnostics())
            if (diagnostic.message.startsWith ("Slider.value: "))
                return diagnostic.line;

        return -1;
    };

    EXPECT_EQ (findSliderProblem(), 4);

    const auto glow = [] (const Component& c) { return static_cast<double> (c.findProperty (Identifier ("glow"))); };

    const auto sheet = document.getStyleSheet();
    const auto map = document.getSourceMap();
    auto* knob = document.findById ("knob");
    ASSERT_NE (knob, nullptr);
    auto& context = *document.getRoot()->getStyleContext();

    // The style is as it was, so the class set in the tree transitions like one set from code
    auto root = document.getTree().getChildWithName ("Component");
    root.getChild (0).beginTransaction().setProperty ("class", "\"knob lit\"");
    document.applyPendingChanges();

    EXPECT_EQ (document.getStyleSheet(), sheet);
    EXPECT_EQ (document.findById ("knob"), knob);
    context.refresh (0.0);
    context.refresh (0.25);
    EXPECT_DOUBLE_EQ (glow (*knob), 2.5);

    // The problems are forgotten and told again where they are written. The map is another one, which is how a
    // component with a problem of its own knows it has to tell it again while the stylesheet is the same
    ASSERT_TRUE (document.getSourceMap() != nullptr);
    EXPECT_TRUE (document.getSourceMap() != map);
    EXPECT_EQ (findSliderProblem(), 4);

    root.getChild (2).beginTransaction().setProperty ("text", "\"b\"");
    document.applyPendingChanges();

    EXPECT_EQ (document.getStyleSheet(), sheet);
    EXPECT_EQ (document.findById<Label> ("caption")->getText(), "b");
    EXPECT_DOUBLE_EQ (glow (*knob), 2.5);

    context.refresh (0.25);
    EXPECT_DOUBLE_EQ (glow (*knob), 5.0);
}

TEST_F (YuiDocumentTests, AChangeOfTheTreeMakesTheStyleSheetAgainWhenItReadsSomethingElse)
{
    const String text ("tokens { property spacing: number = 4; }\n"
                       "style { .lit { opacity: 0.5; } }\n"
                       "component Panel : Column { property title: string = \"a\"; Label { text: \"one\"; } }\n"
                       "Component { Label { id: reading; width: spacing * 10; } Panel { id: panel; } }");

    YuiDocument document;
    ASSERT_TRUE (document.loadFromString (text).wasOk());
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto sheet = document.getStyleSheet();

    // Applies the changes made to the tree, and tells whether the stylesheet is the one there was
    const auto keepsTheStyleSheet = [&document, &sheet]
    {
        document.applyPendingChanges();

        const auto isKept = document.getStyleSheet() == sheet;
        sheet = document.getStyleSheet();
        return isKept;
    };

    const auto tree = document.getTree();
    auto token = tree.getChildWithName ("tokens").getChildWithName ("property");
    auto declared = tree.getChildWithName ("component").getChildWithName ("property");
    auto label = tree.getChildWithName ("component").getChildWithName ("Column").getChildWithName ("Label");
    auto declaration = tree.getChildWithName ("style").getChildWithName ("rule").getChildWithName ("declaration");
    ASSERT_TRUE (token.isValid() && declared.isValid() && label.isValid() && declaration.isValid());

    // What a component is made of, the value of a token and the one a declared property starts with are not read by it
    label.beginTransaction().setProperty ("text", "\"two\"");
    EXPECT_TRUE (keepsTheStyleSheet());
    EXPECT_EQ (dynamic_cast<Label*> (document.findById ("panel")->getChildComponent (0))->getText(), "two");

    token.beginTransaction().setProperty ("value", "8");
    EXPECT_TRUE (keepsTheStyleSheet());
    EXPECT_FLOAT_EQ (document.findById ("reading")->getWidth(), 80.0f);

    declared.beginTransaction().setProperty ("value", "\"b\"");
    EXPECT_TRUE (keepsTheStyleSheet());

    // The properties declared are, by their name and their type, and so is all of a style block
    declared.beginTransaction().setProperty ("name", "heading");
    EXPECT_FALSE (keepsTheStyleSheet());

    token.beginTransaction().setProperty ("type", "integer");
    EXPECT_FALSE (keepsTheStyleSheet());

    declaration.beginTransaction().setProperty ("value", "0.25");
    EXPECT_FALSE (keepsTheStyleSheet());

    // An element added leaves no map to tell a build from the one before: the stylesheet is made again whatever changes
    tree.getChildWithName ("Component").beginTransaction().addChild (DataTree ("Label"));
    EXPECT_FALSE (keepsTheStyleSheet());
    EXPECT_TRUE (document.getSourceMap() == nullptr);

    label.beginTransaction().setProperty ("text", "\"three\"");
    EXPECT_FALSE (keepsTheStyleSheet());

    // Until a text is loaded
    ASSERT_TRUE (document.loadFromString (text).wasOk());
    sheet = document.getStyleSheet();

    document.getTree().getChildWithName ("component").getChildWithName ("Column").getChildWithName ("Label").beginTransaction().setProperty ("text", "\"four\"");
    EXPECT_TRUE (keepsTheStyleSheet());
}

