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

class DrawingStyleTest : public ::testing::Test
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

    static void configure (Slider& slider)
    {
        slider.setRange (0.0, 1.0, 0.0);
        slider.setValue (0.5, dontSendNotification);
        slider.setBounds (0.0f, 0.0f, 48.0f, 48.0f);
    }

    static DataTree parseDrawing (const String& xml)
    {
        auto element = XmlDocument::parse (xml);
        return element != nullptr ? DataTree::fromXml (*element) : DataTree();
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (DrawingStyleTest, CreatesStyleForValidDrawing)
{
    Slider slider (Slider::LinearHorizontal);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseDrawing ("<drawing><rect bounds=\"bounds\" fill=\"trackColor\"/></drawing>"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();
    EXPECT_NE (style.getReference().get(), nullptr);
}

TEST_F (DrawingStyleTest, FailsForInvalidDrawing)
{
    Slider slider (Slider::LinearHorizontal);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseDrawing ("<drawing><blob/></drawing>"));
    EXPECT_TRUE (style.failed());
}

TEST_F (DrawingStyleTest, SetStyleIsReturnedByComponentStyleLookup)
{
    Slider slider (Slider::LinearHorizontal);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseDrawing ("<drawing><rect bounds=\"bounds\" fill=\"trackColor\"/></drawing>"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    slider.setStyle (style.getReference());

    EXPECT_EQ (ApplicationTheme::findComponentStyle (slider).get(), style.getReference().get());
}

TEST_F (DrawingStyleTest, PaintsThroughComponentStyleWithoutThrowing)
{
    Slider slider (Slider::LinearHorizontal);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseDrawing ("<drawing><circle center=\"bounds.center\" radius=\"normalized * 20\" fill=\"trackColor\"/></drawing>"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    slider.setStyle (style.getReference());

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (48, 48);
    Graphics g (*context, *renderer);

    auto resolved = ApplicationTheme::findComponentStyle (slider);
    ASSERT_NE (resolved.get(), nullptr);

    EXPECT_NO_THROW (resolved->paint (g, *theme, slider));
}
