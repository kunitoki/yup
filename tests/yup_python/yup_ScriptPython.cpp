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

#include <PythonStandardLibrary.h>

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
        engine = std::make_unique<ScriptEngine> (ScriptEngine::prepareScriptingHome (
            YUPApplication::getInstance()->getApplicationName(),
            File::getSpecialLocation (File::tempDirectory),
            [] (const char*) -> MemoryBlock
        {
            return { PythonStandardLibrary_data, PythonStandardLibrary_size };
        }));
    }

    void TearDown() override
    {
        // Cleanup after each test
    }

    std::unique_ptr<ScriptEngine> engine;
};

TEST_F (ScriptPythonTest, RunPythonTests)
{
    auto script = String (R"(
        import importlib

        package = 'pytest'

        try:
            importlib.import_module(package)
        except ImportError:
            import pip
            pip.main(['install', package, '--prefix', '{{root_path}}'])
        finally:
            globals()[package] = importlib.import_module(package)

        pytest.main(['-x', '{{test_path}}', '-vv'])
    )")
                      .dedentLines()
                      .replace ("{{root_path}}", engine->getScriptingHome().getFullPathName())
                      .replace ("{{test_path}}", getPytestTestFolder().getFullPathName());

    auto result = engine->runScript (script);

    EXPECT_TRUE (result.wasOk()) << "Pytest failed: " << result.getErrorMessage();
}
