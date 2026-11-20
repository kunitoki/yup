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

class WindowComponentTests : public ::testing::Test
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

TEST_F (WindowComponentTests, IsATransparentStackOfTheWindowType)
{
    WindowComponent window ("main");

    EXPECT_EQ (window.getComponentID(), "main");
    EXPECT_FALSE (window.isOpaque());
    EXPECT_FALSE (WindowComponent().isOpaque());

    const auto& info = window.getTypeInfo();
    EXPECT_EQ (info.getTypeName(), Identifier ("Window"));
    EXPECT_TRUE (info.isSubtypeOf (StackComponent::getClassTypeInfo()));
    EXPECT_TRUE (info.hasFactory());
}

TEST_F (WindowComponentTests, StretchesItsChildrenOverItsBounds)
{
    WindowComponent window;
    Component content ("content");
    window.addAndMakeVisible (content);

    window.setBounds (0, 0, 100, 60);

    EXPECT_EQ (content.getBounds(), Rectangle<float> (0.0f, 0.0f, 100.0f, 60.0f));
}

TEST_F (WindowComponentTests, NamesItsBackgroundColorAsAProperty)
{
    WindowComponent window;
    EXPECT_FALSE (window.getColor (WindowComponent::Style::backgroundColorId).has_value());

    window.setColor (WindowComponent::Style::backgroundColorId, Color (0xff20242a));

    EXPECT_EQ (window.findPropertyAs<Color> (Identifier ("backgroundColor")).getARGB(), 0xff20242au);
}

TEST_F (WindowComponentTests, PaintsItsBackgroundOnlyWhenTheColorIsVisible)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (64, 64);
    Graphics g (*context, *renderer);

    WindowComponent window;
    window.setBounds (0, 0, 64, 64);

    g.setFillColor (Color (0xff010203));

    window.setColor (WindowComponent::Style::backgroundColorId, Color (0x0020242a));
    window.paint (g);
    EXPECT_EQ (g.getFillColor().getARGB(), 0xff010203u);

    window.setColor (WindowComponent::Style::backgroundColorId, Color (0xff20242a));
    window.paint (g);
    EXPECT_EQ (g.getFillColor().getARGB(), 0xff20242au);
}
