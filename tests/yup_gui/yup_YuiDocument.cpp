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
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());

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
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());

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
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());
    ASSERT_EQ (document.findById ("row")->getNumChildComponents(), 2);

    const auto sheet = document.getRoot()->getStyleSheet();

    tree.beginTransaction().setProperty ("length", 3);
    document.applyPendingChanges();

    auto* row = document.findById ("row");
    ASSERT_EQ (row->getNumChildComponents(), 3);
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());

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
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());

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

TEST_F (YuiDocumentTests, ClearNotifiesListenersAndFailedLoadClearsStaleSourceMap)
{
    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument& doc, const Result& result) override
        {
            results.add (result.wasOk());
            roots.add (doc.getRoot());
        }

        void documentDiagnosed (YuiDocument&, const YuiDiagnostic& diagnostic) override
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
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());

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
        ASSERT_EQ (document.getDiagnostics().size(), 1u) << YuiDiagnostic::join (document.getDiagnostics());
        EXPECT_TRUE (document.getDiagnostics().front().line == 2 || document.getDiagnostics().front().line == 3);
        EXPECT_TRUE (document.getDiagnostics().front().toString().contains ("Binding cycle detected"));
    }

    struct Recorder final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument&, const Result&) override {}

        void documentDiagnosed (YuiDocument&, const YuiDiagnostic& diagnostic) override
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
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());

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

    YuiSourceMap map;
    map.add (badRule, {}, 7, 5, "broken.yui");

    const auto badLoad = document.load (brokenDoc, map);
    EXPECT_TRUE (badLoad.failed());
    EXPECT_EQ (document.findById ("knob"), existingKnob);
    ASSERT_EQ (document.getDiagnostics().size(), 1u);
    EXPECT_EQ (document.getDiagnostics().front().source, "broken.yui");
    EXPECT_EQ (document.getDiagnostics().front().line, 7);
    EXPECT_EQ (document.getDiagnostics().front().column, 5);
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
    EXPECT_EQ (document.findById<Label> ("title"), title);
    EXPECT_EQ (title->getText(), "Live");

    styleDecl.beginTransaction().setProperty ("value", "12");
    EXPECT_EQ (document.findById<Slider> ("knob"), knob);
    EXPECT_DOUBLE_EQ (static_cast<double> (knob->findProperty (Identifier ("trackWidth"))), 12.0);
}



