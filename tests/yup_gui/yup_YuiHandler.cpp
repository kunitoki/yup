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
#include <yup_gui/markup/yup_YuiHandler.h>

#include <gtest/gtest.h>

using namespace yup;

class YuiHandlerTests : public ::testing::Test
{
protected:
};

TEST_F (YuiHandlerTests, ParsesSingleCallArrowParametersAndCallBlocks)
{
    const auto single = YuiHandler::parse ("action (\"save\", 1 + 2)");
    ASSERT_TRUE (single.wasOk()) << single.getErrorMessage();
    EXPECT_TRUE (single.getReference().argumentNames.isEmpty());
    ASSERT_EQ (single.getReference().calls.size(), 1u);
    EXPECT_TRUE (single.getReference().calls[0].target.isEmpty());
    EXPECT_EQ (single.getReference().calls[0].name, "action");
    EXPECT_EQ (single.getReference().calls[0].arguments.size(), 2u);

    const auto arrow = YuiHandler::parse ("(value) => root.onChanged (value * 2.0)");
    ASSERT_TRUE (arrow.wasOk()) << arrow.getErrorMessage();
    EXPECT_EQ (arrow.getReference().argumentNames, StringArray ({ "value" }));
    ASSERT_EQ (arrow.getReference().calls.size(), 1u);
    EXPECT_EQ (arrow.getReference().calls[0].target, "root");
    EXPECT_EQ (arrow.getReference().calls[0].name, "onChanged");
    EXPECT_EQ (arrow.getReference().calls[0].arguments.size(), 1u);

    const auto block = YuiHandler::parse ("(index, label) => { action (\"pick\", label); bus.select (index); }");
    ASSERT_TRUE (block.wasOk()) << block.getErrorMessage();
    EXPECT_EQ (block.getReference().argumentNames, StringArray ({ "index", "label" }));
    ASSERT_EQ (block.getReference().calls.size(), 2u);
    EXPECT_EQ (block.getReference().calls[0].name, "action");
    EXPECT_EQ (block.getReference().calls[1].target, "bus");
    EXPECT_EQ (block.getReference().calls[1].name, "select");

    const auto empty = YuiHandler::parse ("(a) => { }");
    ASSERT_TRUE (empty.wasOk()) << empty.getErrorMessage();
    EXPECT_TRUE (empty.getReference().calls.empty());
}

TEST_F (YuiHandlerTests, RejectsMalformedHandlerSyntax)
{
    EXPECT_TRUE (YuiHandler::parse ("").failed());
    EXPECT_TRUE (YuiHandler::parse ("notACall").failed());
    EXPECT_TRUE (YuiHandler::parse ("(a, a) => action (a)").failed());
    EXPECT_TRUE (YuiHandler::parse ("(a) => { notACall; }").failed());
    EXPECT_TRUE (YuiHandler::parse ("action (1) extra").failed());
}

TEST_F (YuiHandlerTests, HandlerArgumentsExposeSignalValuesByNameAndPropertyWatcherFiresOnChanges)
{
    YuiHandlerArguments args (
        { "index", "title" },
        { ComponentSignal::Argument { .name = Identifier ("idx"), .type = ComponentProperty::Type::integer },
          ComponentSignal::Argument { .name = Identifier ("name"), .type = ComponentProperty::Type::string } });

    args.setValues ({ 3, "Preset A" });
    EXPECT_EQ (static_cast<int> (args.getProperty (Identifier ("index"))), 3);
    EXPECT_EQ (args.getProperty (Identifier ("title")).toString(), "Preset A");

    Slider slider (Slider::LinearHorizontal);
    slider.setRange (0.0, 10.0, 0.0);

    Array<double> observed;
    {
        YuiPropertyWatcher watcher (slider, Identifier ("value"), [&] (const Array<var>& values)
        {
            ASSERT_EQ (values.size(), 1);
            observed.add (static_cast<double> (values[0]));
        });

        slider.setValue (4.5, dontSendNotification);
        slider.setValue (7.0, dontSendNotification);
    }

    // After watcher destruction, subsequent changes are ignored cleanly
    slider.setValue (9.0, dontSendNotification);
    ASSERT_EQ (observed.size(), 2);
    EXPECT_DOUBLE_EQ (observed[0], 4.5);
    EXPECT_DOUBLE_EQ (observed[1], 7.0);
}
