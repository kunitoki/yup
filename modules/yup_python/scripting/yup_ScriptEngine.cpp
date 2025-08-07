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

#include "yup_ScriptEngine.h"
#include "yup_ScriptBindings.h"
#include "yup_ScriptException.h"
#include "yup_ScriptUtilities.h"

#include <regex>

namespace yup
{

namespace py = pybind11;

namespace
{

//==============================================================================

[[maybe_unused]] String replaceBrokenLineNumbers (const String& input, const String& code)
{
    static const std::regex pattern ("<string>\\((\\d+)\\)");

    const auto codeLines = StringArray::fromLines (code);

    String output;
    std::string result = input.toStdString();
    std::ptrdiff_t startPos = 0;

    std::smatch match;
    while (std::regex_search (result, match, pattern))
    {
        if (match.size() > 1)
        {
            const int matchLine = std::stoi (match[1]) - 1;

            output
                << input.substring (static_cast<int> (startPos), static_cast<int> (match.position() - startPos))
                << "<string>(" << matchLine << "): \'" << codeLines[matchLine - 1] << "\'";

            startPos = match.position() + match.length();
        }

        result = match.suffix();
    }

    output << result;
    return output;
}

} // namespace

//==============================================================================

std::unique_ptr<PyConfig> ScriptEngine::prepareScriptingHome (
    const String& programName,
    const File& destinationFolder,
    std::function<MemoryBlock (const char*)> standardLibraryCallback,
    bool forceInstall)
{
    String pythonFolderName, pythonArchiveName;
    pythonFolderName << "python" << PY_MAJOR_VERSION << "." << PY_MINOR_VERSION;
    pythonArchiveName << "python" << PY_MAJOR_VERSION << PY_MINOR_VERSION << "_zip";

    if (! destinationFolder.isDirectory())
        destinationFolder.createDirectory();

    auto libFolder = destinationFolder.getChildFile ("lib");
    if (! libFolder.isDirectory())
        libFolder.createDirectory();

    auto pythonFolder = libFolder.getChildFile (pythonFolderName);
    if (! pythonFolder.isDirectory())
        pythonFolder.createDirectory();

    if (forceInstall && pythonFolder.getNumberOfChildFiles (File::findFilesAndDirectories) > 0)
    {
        pythonFolder.deleteRecursively();
        pythonFolder.createDirectory();
    }

    if (! pythonFolder.getChildFile ("lib-dynload").isDirectory())
    {
        MemoryBlock mb = standardLibraryCallback (pythonArchiveName.toRawUTF8());

        auto mis = MemoryInputStream (mb.getData(), mb.getSize(), false);

        auto zip = ZipFile (mis);
        zip.uncompressTo (pythonFolder.getParentDirectory());
    }

    PyPreConfig preconfig;
    PyPreConfig_InitIsolatedConfig (&preconfig);
    preconfig.utf8_mode = 1;

    if (PyStatus status = Py_PreInitialize(&preconfig); PyStatus_IsError (status))
    {
        YUP_DBG ("Failed Py_PreInitialize");
        return nullptr;
    }

    auto config = std::make_unique<PyConfig>();

    PyConfig_InitPythonConfig (config.get());
    config->parse_argv = 0;
    config->isolated = 1;
    config->install_signal_handlers = 0;

    if (auto status = PyConfig_SetBytesString (config.get(), &config->program_name, programName.toRawUTF8()); PyStatus_IsError (status))
    {
        YUP_DBG ("Failed config->program_name");
        return nullptr;
    }

    const auto homePath = destinationFolder.getFullPathName();
    if (auto status = PyConfig_SetBytesString (config.get(), &config->home, homePath.toRawUTF8()); PyStatus_IsError (status))
    {
        YUP_DBG ("Failed config->home");
        return nullptr;
    }

#if YUP_WINDOWS
    config->module_search_paths_set = 1;

    const auto prefixPath = destinationFolder.getChildFile("lib").getFullPathName();

    if (auto status = PyConfig_SetBytesString (config.get(), &config->prefix, prefixPath.toRawUTF8()); PyStatus_IsError (status))
    {
        YUP_DBG ("Failed config->prefix");
        return nullptr;
    }

    if (auto status = PyConfig_SetBytesString (config.get(), &config->exec_prefix, prefixPath.toRawUTF8()); PyStatus_IsError (status))
    {
        YUP_DBG ("Failed config->exec_prefix");
        return nullptr;
    }
#endif

    return config;
}

//==============================================================================

ScriptEngine::ScriptEngine()
    : ScriptEngine (StringArray {})
{
}

ScriptEngine::ScriptEngine (std::unique_ptr<PyConfig> config)
    : ScriptEngine (StringArray {}, std::move (config))
{
}

ScriptEngine::ScriptEngine (StringArray modules, std::unique_ptr<PyConfig> config)
    : customModules (std::move (modules))
{
    if (config != nullptr)
    {
        scriptingHome = String (config->home);
        pybind11::initialize_interpreter (config.get(), 0, nullptr, true);
    }
    else
    {
        pybind11::initialize_interpreter();
    }

    py::set_shared_data ("_YUP_ENGINE", this);
}

ScriptEngine::~ScriptEngine()
{
    py::set_shared_data ("_YUP_ENGINE", nullptr);

    pybind11::finalize_interpreter();
}

//==============================================================================

String ScriptEngine::getScriptingVersion() const
{
    String version;
    version << PY_MAJOR_VERSION << "." << PY_MINOR_VERSION << "." << PY_MICRO_VERSION;
    return version;
}

File ScriptEngine::getScriptingHome() const
{
    return scriptingHome;
}

//==============================================================================

Result ScriptEngine::runScript (const String& code, py::dict locals, py::dict globals)
{
    currentScriptCode = code;
    currentScriptFile = File();

    return runScriptInternal (currentScriptCode, std::move (globals), std::move (locals));
}

Result ScriptEngine::runScript (const File& script, py::dict locals, py::dict globals)
{
    {
        auto is = script.createInputStream();
        if (is == nullptr)
            return Result::fail ("Unable to open the requested script file");

        currentScriptCode = is->readEntireStreamAsString();
        currentScriptFile = script;
    }

    return runScriptInternal (currentScriptCode, std::move (globals), std::move (locals));
}

//==============================================================================

Result ScriptEngine::runScriptInternal (const String& code, py::dict locals, py::dict globals)
{
#if YUP_PYTHON_SCRIPT_CATCH_EXCEPTION
    try
#endif

    {
        py::gil_scoped_acquire acquire;

        [[maybe_unused]] const auto redirectStreamsUntilExit = ScriptStreamRedirection();

        for (const auto& m : customModules)
            globals[m.toRawUTF8()] = py::module_::import (m.toRawUTF8());

        py::str pythonCode { code.toRawUTF8(), code.getNumBytesAsUTF8() };

        py::exec (std::move (pythonCode), std::move (globals), std::move (locals));

        return Result::ok();
    }

#if YUP_PYTHON_SCRIPT_CATCH_EXCEPTION
    catch (const py::error_already_set& e)
    {
        return Result::fail (replaceBrokenLineNumbers (e.what(), code));
    }
    catch (...)
    {
        return Result::fail ("Unhandled exception while processing script");
    }
#endif
}

} // namespace yup
