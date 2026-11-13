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

#include <memory>

using namespace yup;

class ComponentPropertyAttachmentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);

        tree = DataTree ("Model");
    }

    void TearDown() override
    {
        tree = DataTree();
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    void setModelValue (const Identifier& property, const var& value)
    {
        auto transaction = tree.beginTransaction();
        transaction.setProperty (property, value);
        transaction.commit();
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
    DataTree tree;
    const Identifier valueId { "value" };
    const Identifier textId { "text" };
    const Identifier modelKey { "x" };
};

TEST_F (ComponentPropertyAttachmentTest, SeedsModelFromComponentWhenPropertyAbsent)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);
    slider.setValue (3.0, dontSendNotification);

    ComponentPropertyAttachment attachment (slider, valueId, tree, modelKey);

    EXPECT_TRUE (tree.hasProperty (modelKey));
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty (modelKey)), 3.0);
    EXPECT_TRUE (attachment.isAttached());
}

TEST_F (ComponentPropertyAttachmentTest, ModelWinsWhenPropertyPresent)
{
    setModelValue (modelKey, 7.0);

    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);
    slider.setValue (1.0, dontSendNotification);

    ComponentPropertyAttachment attachment (slider, valueId, tree, modelKey);

    EXPECT_DOUBLE_EQ (slider.getValue(), 7.0);
}

TEST_F (ComponentPropertyAttachmentTest, ComponentEditPropagatesToModel)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);

    ComponentPropertyAttachment attachment (slider, valueId, tree, modelKey);

    slider.setValue (5.0);

    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty (modelKey)), 5.0);
}

TEST_F (ComponentPropertyAttachmentTest, ComponentEditIsUndoableWithUndoManager)
{
    UndoManager::Ptr undoManager = new UndoManager();

    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);
    slider.setValue (2.0, dontSendNotification);

    ComponentPropertyAttachment attachment (slider, valueId, tree, modelKey, undoManager);

    // The initial seed must not be undoable.
    EXPECT_FALSE (undoManager->canUndo());
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty (modelKey)), 2.0);

    slider.setValue (6.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty (modelKey)), 6.0);
    EXPECT_TRUE (undoManager->canUndo());

    undoManager->undo();
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty (modelKey)), 2.0);
    EXPECT_DOUBLE_EQ (slider.getValue(), 2.0);

    undoManager->redo();
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty (modelKey)), 6.0);
    EXPECT_DOUBLE_EQ (slider.getValue(), 6.0);
}

TEST_F (ComponentPropertyAttachmentTest, ModelEditPropagatesToComponent)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);

    ComponentPropertyAttachment attachment (slider, valueId, tree, modelKey);

    setModelValue (modelKey, 9.0);

    EXPECT_DOUBLE_EQ (slider.getValue(), 9.0);
}

TEST_F (ComponentPropertyAttachmentTest, TextEditorTextRoundTrips)
{
    TextEditor editor;

    ComponentPropertyAttachment attachment (editor, textId, tree, modelKey);

    editor.setText ("hello");
    EXPECT_EQ (tree.getProperty (modelKey).toString(), String ("hello"));

    setModelValue (modelKey, "world");
    EXPECT_EQ (editor.getText(), String ("world"));
}

TEST_F (ComponentPropertyAttachmentTest, SurvivesComponentDeletion)
{
    auto slider = std::make_unique<Slider> (Slider::LinearHorizontal);
    slider->setRange (0.0, 10.0, 0.0);

    ComponentPropertyAttachment attachment (*slider, valueId, tree, modelKey);

    slider.reset();

    EXPECT_FALSE (attachment.isAttached());
    EXPECT_EQ (attachment.getComponent(), nullptr);

    // A later model change must not touch the deleted component.
    setModelValue (modelKey, 4.0);
    EXPECT_DOUBLE_EQ (static_cast<double> (tree.getProperty (modelKey)), 4.0);
}
