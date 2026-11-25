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

class DataTreeDiagnosticTests : public ::testing::Test
{
};

TEST_F (DataTreeDiagnosticTests, FormatsTheKnownParts)
{
    EXPECT_EQ ((DataTreeDiagnostic { .message = "Oops", .source = "a.yui", .line = 2, .column = 5 }.toString()), "a.yui:2:5: Oops");
    EXPECT_EQ ((DataTreeDiagnostic { .message = "Oops", .line = 2, .column = 5 }.toString()), "2:5: Oops");
    EXPECT_EQ ((DataTreeDiagnostic { .message = "Oops", .source = "a.yui" }.toString()), "a.yui: Oops");
    EXPECT_EQ ((DataTreeDiagnostic { .message = "Oops" }.toString()), "Oops");

    EXPECT_EQ (DataTreeDiagnostic::join ({ { .message = "a" }, { .message = "b", .line = 1, .column = 2 } }, "; "), "a; 1:2: b");
    EXPECT_EQ (DataTreeDiagnostic::join ({}), String());
}
