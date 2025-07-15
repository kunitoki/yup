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

        // Add our custom minimalist listener
        testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
        delete listeners.Release (listeners.default_result_printer());
        listeners.Append (new CompactPrinter (*this));

        programStart = std::chrono::steady_clock::now();

        // Start running suites one by one
        runNextSuite (0);
    }

    void shutdown() override {}

private:
    struct FailedTest
    {
        std::string name;
        std::string failureDetails;
    };

    std::chrono::steady_clock::time_point programStart;
    std::vector<FailedTest> failedTests;
    int totalTests = 0;
    int passedTests = 0;

    void runNextSuite (int suiteIndex)
    {
        auto* unitTest = ::testing::UnitTest::GetInstance();

        if (suiteIndex >= unitTest->total_test_suite_count())
        {
            reportSummary();
            return;
        }

        auto* testSuite = unitTest->GetTestSuite (suiteIndex);
        std::string suiteName = testSuite->name();

        ::testing::GTEST_FLAG (filter) = suiteName + ".*";

        yup::MessageManager::callAsync ([this, suiteIndex, suiteName]
        {
            (void) RUN_ALL_TESTS();

            runNextSuite (suiteIndex + 1);
        });
    }

    void reportSummary()
    {
        auto totalElapsed = std::chrono::steady_clock::now() - programStart;
        std::cout << "\n========================================\n";

        if (! failedTests.empty())
        {
            std::cout << "*** FAILURES (" << failedTests.size() << "):\n";
            for (const auto& fail : failedTests)
            {
                std::cout << "\n--- " << fail.name << "\n"
                          << fail.failureDetails << "\n";
            }
        }

        std::cout << "\n========================================\n";
        std::cout << "RESULT: "
                  << (failedTests.empty() ? "ALL PASSED" : "SOME FAILED")
                  << " (" << passedTests << "/" << totalTests << " tests) in "
                  << std::chrono::duration_cast<std::chrono::milliseconds> (totalElapsed).count()
                  << " ms\n";

        setApplicationReturnValue (failedTests.empty() ? 0 : 1);

        quit();
    }

    // --- Custom Compact Printer ---
    struct CompactPrinter : testing::EmptyTestEventListener
    {
        explicit CompactPrinter (TestApplication& app)
            : owner (app)
        {
        }

        void OnTestStart (const testing::TestInfo& info) override
        {
            testStart = std::chrono::steady_clock::now();

            failureStream.str ("");
            failureStream.clear();
        }

        void OnTestPartResult (const testing::TestPartResult& result) override
        {
            if (result.failed())
            {
                failureStream << result.file_name() << ":" << result.line_number() << ": "
                              << result.summary() << "\n";
            }
        }

        void OnTestEnd (const testing::TestInfo& info) override
        {
            auto elapsed = std::chrono::steady_clock::now() - testStart;
            owner.totalTests++;

            std::ostringstream line;
            line << (std::string (info.test_suite_name()) + "." + info.name());

            if (info.result()->Failed())
            {
                std::cout << "*** FAIL - " << line.str()
                          << " (" << std::chrono::duration_cast<std::chrono::milliseconds> (elapsed).count() << " ms)\n";

                owner.failedTests.push_back (
                    { std::string (info.test_suite_name()) + "." + info.name(),
                      failureStream.str() });
            }
            else
            {
                std::cout << "--- PASS - " << line.str()
                          << " (" << std::chrono::duration_cast<std::chrono::milliseconds> (elapsed).count() << " ms)\n";

                owner.passedTests++;
            }
        }

    private:
        TestApplication& owner;
        std::chrono::steady_clock::time_point testStart;
        std::stringstream failureStream;
    };
};

START_YUP_APPLICATION (TestApplication)
