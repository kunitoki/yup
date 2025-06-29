/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
#include <gmock/gmock.h>

//==============================================================================

struct TestApplication : yup::YUPApplication
{
    TestApplication() = default;

    yup::String getApplicationName() override
    {
        return "yup! tests";
    }

    yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        yup::Array<char*> argv;

        auto applicationName = yup::String ("yup_tests");
        argv.add (const_cast<char*> (applicationName.toRawUTF8()));

        auto commandLineArgs = yup::StringArray::fromTokens (commandLineParameters, true);
        for (auto& arg : commandLineArgs)
        {
            if (arg.isNotEmpty())
                argv.add (const_cast<char*> (arg.toRawUTF8()));
        }

        argv.add (nullptr);

        int argc = argv.size() - 1;
        testing::InitGoogleMock (&argc, argv.data());

        yup::MessageManager::callAsync ([this, commandLineParameters]
        {
            auto result = RUN_ALL_TESTS();

            setApplicationReturnValue (result);

            quit();
        });
    }

    void shutdown() override
    {
    }
};

START_YUP_APPLICATION (TestApplication)
