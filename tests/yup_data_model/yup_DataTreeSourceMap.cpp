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

#include <yup_data_model/yup_data_model.h>

#include <gtest/gtest.h>

using namespace yup;

class DataTreeSourceMapTests : public ::testing::Test
{
protected:
    // A document `yui > Column > Slider`, the slider with a value
    static DataTree makeDocument()
    {
        DataTree slider ("Slider");
        slider.beginTransaction().setProperty ("value", 1);

        return DataTree ("yui", { DataTree ("Column", { slider }) });
    }
};

TEST_F (DataTreeSourceMapTests, LocatesPropertiesThenElementsThenAncestors)
{
    const auto document = makeDocument();
    const auto column = document.getChild (0);
    auto slider = column.getChild (0);

    DataTreeSourceMap map;
    map.add (column, {}, 1, 1, "main.yui");
    map.add (slider, {}, 2, 5);
    map.add (slider, "value", 2, 14);

    const auto valueError = map.locate ({ .message = "Bad value", .element = slider, .property = "value" });
    EXPECT_EQ (valueError.toString(), "2:14: Bad value");

    const auto propertyWithoutPosition = map.locate ({ .message = "Bad", .element = slider, .property = "minimum" });
    EXPECT_EQ (propertyWithoutPosition.toString(), "2:5: Bad");

    DataTree bind ("bind");
    slider.beginTransaction().addChild (bind);

    const auto ancestor = map.locate ({ .message = "Bad link", .element = bind });
    EXPECT_EQ (ancestor.toString(), "2:5: Bad link");

    EXPECT_EQ (map.locate ({ .message = "Bad", .element = column, .property = "gap" }).toString(), "main.yui:1:1: Bad");
}

TEST_F (DataTreeSourceMapTests, AppliesToCopiesOfTheDocument)
{
    const auto document = makeDocument();

    DataTreeSourceMap map;
    map.add (document.getChild (0).getChild (0), "value", 3, 7);

    const auto copy = document.clone();
    EXPECT_EQ (map.locate ({ .message = "Bad", .element = copy.getChild (0).getChild (0), .property = "value" }).toString(), "3:7: Bad");
}

TEST_F (DataTreeSourceMapTests, KeepsDiagnosticsItCantLocate)
{
    const auto document = makeDocument();

    DataTreeSourceMap map;
    map.add (document.getChild (0), {}, 1, 1);

    const DataTreeDiagnostic located { .message = "Parse error", .element = document.getChild (0), .line = 9, .column = 3 };
    EXPECT_EQ (map.locate (located).toString(), "9:3: Parse error");

    EXPECT_EQ (map.locate ({ .message = "No element" }).toString(), "No element");
    EXPECT_EQ (map.locate ({ .message = "Root", .element = document }).toString(), "Root");
    EXPECT_EQ (map.locate ({ .message = "Other tree", .element = DataTree ("Slider") }).toString(), "Other tree");
}
