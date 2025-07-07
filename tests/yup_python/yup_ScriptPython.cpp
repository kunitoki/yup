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

#include <yup_python/yup_python.h>

#include <gtest/gtest.h>

#if __has_include(<PythonStandardLibrary.h>)
#define YUP_HAS_EMBEDDED_PYTHON_STANDARD_LIBRARY 1
#include <PythonStandardLibrary.h>
#endif

using namespace yup;

class ScriptPythonTest : public ::testing::Test
{
protected:
    static File getPytestTestFolder()
    {
        return File (__FILE__)
            .getParentDirectory()
            .getParentDirectory()
            .getParentDirectory()
            .getChildFile ("python")
            .getChildFile ("tests");
    }

    void SetUp() override
    {
#if YUP_HAS_EMBEDDED_PYTHON_STANDARD_LIBRARY
        engine = std::make_unique<ScriptEngine> (ScriptEngine::prepareScriptingHome (
            YUPApplication::getInstance()->getApplicationName(),
            File::getSpecialLocation (File::tempDirectory),
            [] (const char*) -> MemoryBlock
        {
            return { PythonStandardLibrary_data, PythonStandardLibrary_size };
        }));
#else
        engine = std::make_unique<ScriptEngine>();
#endif
    }

    void TearDown() override
    {
        // Cleanup after each test
    }

    std::unique_ptr<ScriptEngine> engine;
};

TEST_F (ScriptPythonTest, RunPythonTests)
{
    auto currentWorkingDirectory = File::getCurrentWorkingDirectory();
    auto restoreWorkingDirectoryAtScopeExit = ErasedScopeGuard ([&]
    {
        currentWorkingDirectory.setAsCurrentWorkingDirectory();
    });

    auto scriptingVersion = engine->getScriptingVersion();
    auto scriptingVersionCompact = scriptingVersion.upToLastOccurrenceOf (".", false, false);

    auto scriptingHome = engine->getScriptingHome();
    if (scriptingHome == File())
    {
        scriptingHome = File::getSpecialLocation (File::userHomeDirectory).getChildFile ("yup_python");
        scriptingHome.createDirectory();

        auto binDirectory = scriptingHome.getChildFile ("bin");
        binDirectory.createDirectory();

        auto sitePackages = scriptingHome
                                .getChildFile ("lib")
                                .getChildFile ("python" + scriptingVersionCompact)
                                .getChildFile ("site-packages");
        sitePackages.createDirectory();
    }

    auto baseFolder = getPytestTestFolder().getParentDirectory();
    baseFolder.setAsCurrentWorkingDirectory();

    auto script = String (R"(
        import importlib
        import runpy
        import sys

        sys.path.append('{{root_path}}/lib/python{{version}}/site-packages')
        sys.path.append('{{root_path}}/local/lib/python{{version}}/site-packages')

        package = 'pytest'

        try:
            import pytest
        except ImportError:
            old_argv = [x for x in sys.argv]
            sys.argv = ['pip', 'install', 'pytest', '--root', '{{root_path}}']
            try:
                runpy.run_module('pip', run_name='__main__')
            except SystemExit as ex:
                print(str(ex))
            finally:
                sys.argv = old_argv
 
            os.system('ls -la {{root_path}}/local')
            os.system('ls -la {{root_path}}/local/*')
            os.system('ls -la {{root_path}}/local/*/*')

            import pytest

        pytest.main(['-x', '{{test_path}}', '-vvv'])
    )");

    script = script
                 .dedentLines()
                 .replace ("{{version}}", scriptingVersionCompact)
                 .replace ("{{root_path}}", scriptingHome.getFullPathName())
                 .replace ("{{test_path}}", getPytestTestFolder().getFullPathName());

    auto result = engine->runScript (script);

    EXPECT_TRUE (result.wasOk()) << "Pytest failed: " << result.getErrorMessage();
}
