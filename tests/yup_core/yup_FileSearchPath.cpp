/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <yup_core/yup_core.h>

using namespace yup;

TEST (FileSearchPathTests, RemoveRedundantPaths)
{
#if YUP_WINDOWS
    const String prefix = "C:";
#else
    const String prefix = "";
#endif

    {
        FileSearchPath fsp { prefix + "/a/b/c/d;" + prefix + "/a/b/c/e;" + prefix + "/a/b/c" };
        fsp.removeRedundantPaths();
        EXPECT_EQ (fsp.toString(), prefix + "/a/b/c");
    }

    {
        FileSearchPath fsp { prefix + "/a/b/c;" + prefix + "/a/b/c/d;" + prefix + "/a/b/c/e" };
        fsp.removeRedundantPaths();
        EXPECT_EQ (fsp.toString(), prefix + "/a/b/c");
    }

    {
        FileSearchPath fsp { prefix + "/a/b/c/d;" + prefix + "/a/b/c;" + prefix + "/a/b/c/e" };
        fsp.removeRedundantPaths();
        EXPECT_EQ (fsp.toString(), prefix + "/a/b/c");
    }

    {
        FileSearchPath fsp { "%FOO%;" + prefix + "/a/b/c;%FOO%;" + prefix + "/a/b/c/d" };
        fsp.removeRedundantPaths();
        EXPECT_EQ (fsp.toString(), "%FOO%;" + prefix + "/a/b/c");
    }
}