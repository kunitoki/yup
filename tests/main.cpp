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
        return "yup_tests";
    }

    yup::String getApplicationVersion() override
    {
        return "1.0";
    }

    void initialise (const yup::String& commandLineParameters) override
    {
        yup::Array<char*> argv;

        auto applicationName = getApplicationName();
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

        parseXmlOutputSettings (commandLineParameters);

        testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
        delete listeners.Release (listeners.default_result_printer());
        listeners.Append (new CompactPrinter (*this));

        programStart = std::chrono::steady_clock::now();

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
    yup::File originalXmlOutputPath;

    void parseXmlOutputSettings (const yup::String& commandLineParameters)
    {
        auto args = yup::StringArray::fromTokens (commandLineParameters, true);
        for (auto& arg : args)
        {
            if (! arg.startsWith ("--gtest_output=xml:"))
                continue;

            auto originalXmlPath = arg.fromFirstOccurrenceOf (":", false, false);
            if (yup::File::isAbsolutePath (originalXmlPath))
                originalXmlOutputPath = yup::File (originalXmlPath);
            else
                originalXmlOutputPath = yup::File::getCurrentWorkingDirectory().getChildFile (originalXmlPath);

            std::cout << "Will generate XML report to: " << originalXmlOutputPath.getFullPathName() << std::endl;
            break;
        }
    }

    void runNextSuite (int suiteIndex)
    {
        auto* unitTest = ::testing::UnitTest::GetInstance();

        if (suiteIndex >= unitTest->total_test_suite_count())
        {
            generateXmlReport();
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

    void generateXmlReport()
    {
        std::cout << "\n========================================\n";

        if (originalXmlOutputPath == yup::File())
        {
            std::cout << "No XML output path specified, skipping XML generation" << std::endl;
            return;
        }

        try
        {
            auto testsuites = std::make_unique<yup::XmlElement> ("testsuites");

            int totalTests = 0;
            int totalFailures = 0;
            int totalErrors = 0;
            double totalTime = 0.0;

            for (const auto& suiteResult : allSuiteResults)
            {
                auto testsuite = new yup::XmlElement ("testsuite");
                testsuite->setAttribute ("name", yup::String (suiteResult.name));
                testsuite->setAttribute ("tests", suiteResult.tests);
                testsuite->setAttribute ("failures", suiteResult.failures);
                testsuite->setAttribute ("errors", suiteResult.errors);
                testsuite->setAttribute ("time", suiteResult.timeSeconds);

                for (const auto& testCase : suiteResult.testCases)
                {
                    auto testcase = new yup::XmlElement ("testcase");
                    testcase->setAttribute ("name", yup::String (testCase.name));
                    testcase->setAttribute ("classname", yup::String (testCase.className));
                    testcase->setAttribute ("time", testCase.timeSeconds);

                    if (! testCase.passed && ! testCase.failureMessage.empty())
                    {
                        auto failure = new yup::XmlElement ("failure");
                        failure->setAttribute ("message", "Test failed");
                        failure->setAttribute ("type", "");
                        failure->addTextElement (yup::String (testCase.failureMessage));
                        testcase->addChildElement (failure);
                    }

                    testsuite->addChildElement (testcase);
                }

                testsuites->addChildElement (testsuite);

                totalTests += suiteResult.tests;
                totalFailures += suiteResult.failures;
                totalErrors += suiteResult.errors;
                totalTime += suiteResult.timeSeconds;
            }

            testsuites->setAttribute ("tests", totalTests);
            testsuites->setAttribute ("failures", totalFailures);
            testsuites->setAttribute ("errors", totalErrors);
            testsuites->setAttribute ("time", totalTime);
            testsuites->setAttribute ("name", "AllTests");
            testsuites->writeTo (originalXmlOutputPath);

            std::cout << "Generating XML report (" << allSuiteResults.size()
                      << " suites): " << originalXmlOutputPath.getFullPathName() << std::endl;
        }
        catch (...)
        {
            std::cout << "Warning: Failed to generate XML report" << std::endl;
        }
    }

    void reportSummary()
    {
        auto totalElapsed = std::chrono::steady_clock::now() - programStart;

        if (! failedTests.empty())
        {
            std::cout << "\n========================================\n";
            std::cout << "*** FAILURES (" << failedTests.size() << "):\n";
            for (const auto& fail : failedTests)
                std::cout << "\n--- " << fail.name << "\n"
                          << fail.failureDetails << "\n";
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

    struct TestCaseResult
    {
        std::string name;
        std::string className;
        bool passed;
        double timeSeconds;
        std::string failureMessage;
    };

    struct TestSuiteResult
    {
        std::string name;
        int tests = 0;
        int failures = 0;
        int errors = 0;
        double timeSeconds = 0.0;
        std::vector<TestCaseResult> testCases;
    };

    std::vector<TestSuiteResult> allSuiteResults;
    TestSuiteResult* currentSuite = nullptr;

    struct CompactPrinter : testing::EmptyTestEventListener
    {
        explicit CompactPrinter (TestApplication& app)
            : owner (app)
        {
        }

        void OnTestSuiteStart (const testing::TestSuite& test_suite) override
        {
            owner.allSuiteResults.emplace_back();
            owner.currentSuite = &owner.allSuiteResults.back();
            owner.currentSuite->name = test_suite.name();

            suiteStartTime = std::chrono::steady_clock::now();
        }

        void OnTestSuiteEnd (const testing::TestSuite& test_suite) override
        {
            if (owner.currentSuite)
            {
                auto suiteElapsed = std::chrono::steady_clock::now() - suiteStartTime;
                owner.currentSuite->timeSeconds = std::chrono::duration<double> (suiteElapsed).count();
                owner.currentSuite = nullptr;
            }
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
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds> (elapsed).count();
            auto elapsedSeconds = std::chrono::duration<double> (elapsed).count();

            owner.totalTests++;

            std::ostringstream line;
            line << (std::string (info.test_suite_name()) + "." + info.name());

            bool testPassed = ! info.result()->Failed();

            if (testPassed)
            {
                std::cout << "--- PASS - " << line.str() << " (" << elapsedMs << " ms)\n";
                owner.passedTests++;
            }
            else
            {
                std::cout << "*** FAIL - " << line.str() << " (" << elapsedMs << " ms)\n";
                owner.failedTests.push_back (
                    { std::string (info.test_suite_name()) + "." + info.name(),
                      failureStream.str() });
            }

            if (owner.currentSuite)
            {
                TestCaseResult testCase;
                testCase.name = info.name();
                testCase.className = info.test_suite_name();
                testCase.passed = testPassed;
                testCase.timeSeconds = elapsedSeconds;
                testCase.failureMessage = testPassed ? "" : failureStream.str();

                owner.currentSuite->testCases.push_back (testCase);
                owner.currentSuite->tests++;
                if (! testPassed)
                    owner.currentSuite->failures++;
            }
        }

    private:
        TestApplication& owner;
        std::chrono::steady_clock::time_point testStart;
        std::chrono::steady_clock::time_point suiteStartTime;
        std::stringstream failureStream;
    };
};

START_YUP_APPLICATION (TestApplication)
