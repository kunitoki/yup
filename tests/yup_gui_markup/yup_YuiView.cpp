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

using namespace yup;

class YuiViewTests : public ::testing::Test
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

TEST_F (YuiViewTests, HostsTheRootFillingItsBounds)
{
    YuiView view;
    view.setSize (200, 100);

    EXPECT_TRUE (view.getLoader().reloadFromString ("Column {").failed());
    EXPECT_EQ (view.getNumChildComponents(), 0);

    const auto result = view.getLoader().reloadFromString ("Column { Slider { id: knob; } }");
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();

    auto* root = view.getDocument().getRoot();
    ASSERT_NE (root, nullptr);
    EXPECT_EQ (root->getParentComponent(), &view);
    EXPECT_TRUE (root->isVisible());
    EXPECT_EQ (root->getBounds(), view.getLocalBounds());

    view.setSize (300, 120);
    EXPECT_EQ (root->getBounds(), view.getLocalBounds());
}

TEST_F (YuiViewTests, HostsTheRootReplacingThePreviousOne)
{
    YuiView view;
    view.setSize (200, 100);

    ASSERT_TRUE (view.getDocument().loadFromString ("Column { Slider { id: knob; } }").wasOk());
    auto* column = view.getDocument().getRoot();

    ASSERT_TRUE (view.getDocument().loadFromString ("Column { Slider { id: knob; } Label {} }").wasOk());
    EXPECT_EQ (view.getDocument().getRoot(), column);
    EXPECT_EQ (view.getNumChildComponents(), 1);

    ASSERT_TRUE (view.getDocument().loadFromString ("Row { Slider { id: knob; } }").wasOk());

    auto* root = view.getDocument().getRoot();
    ASSERT_NE (root, nullptr);
    EXPECT_EQ (view.getNumChildComponents(), 1);
    EXPECT_EQ (root->getParentComponent(), &view);
    EXPECT_EQ (root->getBounds(), view.getLocalBounds());
}

TEST_F (YuiViewTests, TheInterfaceFollowsTheEnabledStateOfTheView)
{
    YuiView view;
    view.setEnabled (false);

    ASSERT_TRUE (view.getDocument().loadFromString ("Column { Slider { id: knob; } }").wasOk());

    auto* knob = view.getDocument().findById ("knob");
    ASSERT_NE (knob, nullptr);
    EXPECT_FALSE (knob->isEnabled());

    view.setEnabled (true);
    EXPECT_TRUE (knob->isEnabled());
}

TEST_F (YuiViewTests, ClearingTheDocumentUnmountsTheRootComponent)
{
    YuiView view;
    view.setSize (200, 100);

    ASSERT_TRUE (view.getDocument().loadFromString ("Column { Slider { id: knob; } }").wasOk());
    EXPECT_EQ (view.getNumChildComponents(), 1);

    view.getDocument().clear();

    EXPECT_EQ (view.getDocument().getRoot(), nullptr);
    EXPECT_EQ (view.getNumChildComponents(), 0);
}

TEST_F (YuiViewTests, HostsTheRootALiveTreeEditReplaces)
{
    YuiView view;
    view.setSize (200, 100);

    ASSERT_TRUE (view.getDocument().loadFromString ("Column { Slider { id: knob; } }").wasOk());

    auto tree = view.getDocument().getTree();

    {
        auto transaction = tree.beginTransaction();
        transaction.removeChild (0);
        transaction.addChild (DataTree ("Row"));
    }

    auto* root = view.getDocument().getRoot();
    ASSERT_NE (root, nullptr);
    EXPECT_EQ (root->getTypeInfo().getTypeName().toString(), "Row");
    EXPECT_EQ (root->getParentComponent(), &view);
    EXPECT_EQ (view.getNumChildComponents(), 1);
}
