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

#pragma once

#include <yup_core/yup_core.h>

#include "../utilities/yup_PyBind11Includes.h"

#include <functional>
#include <memory>

namespace yup
{

//==============================================================================

/** The ScriptEngine class provides a C++ interface for running Python scripts

    The ScriptEngine class encapsulates the functionality to run Python scripts within a C++ application.
    It utilizes the pybind11 library to interact with the Python interpreter and provides a way to run
    custom Python code within the C++ environment.
 */
class YUP_API ScriptEngine
{
public:
    //==============================================================================

    /** Construct a new ScriptEngine object.

        Initializes a ScriptEngine object.
     */
    ScriptEngine();

    /** Construct a new ScriptEngine object.

        @param config A custom python config to initialize the Python interpreter.

        Initializes a ScriptEngine object.
     */
    ScriptEngine (std::unique_ptr<PyConfig> config);

    /** Construct a new ScriptEngine object.

        Initializes a ScriptEngine object with the specified custom modules.

        @param modules An array of module names to be imported in the Python interpreter.
        @param config A custom python config to initialize the Python interpreter.

        @warning Ensure that the provided modules are available and compatible with the Python interpreter.
     */
    ScriptEngine (StringArray modules, std::unique_ptr<PyConfig> config = {});

    /** Destroy the ScriptEngine object.

        Cleans up resources associated with the ScriptEngine object.
     */
    ~ScriptEngine();

    //==============================================================================

    /** Run a Python script.

        Executes the given Python code within the Python interpreter.

        @param code The Python code to be executed.
        @param locals A python dictionary containing local variables.
        @param globals A python dictionary containing global variables.

        @return A Result object indicating the success or failure of the script execution.
     */
    Result runScript (const String& code, pybind11::dict locals = {}, pybind11::dict globals = pybind11::globals());

    /** Run a Python script file.

        Executes the given Python file within the Python interpreter.

        @param script The Python file to be executed.
        @param locals A python dictionary containing local variables.
        @param globals A python dictionary containing global variables.

        @return A Result object indicating the success or failure of the script execution.
     */
    Result runScript (const File& script, pybind11::dict locals = {}, pybind11::dict globals = pybind11::globals());

    //==============================================================================

    /** Get the version of the scripting engine.

        This returns the version of python in form of major.minor.micro (3.13.5).

        @return The version of the scripting engine.
     */
    String getScriptingVersion() const;

    /** Get the scripting home directory.

        When the scripting home is prepared using the prepareScriptingHome method,
        this method will return the destination folder used to prepare the home.
        This home folder can be used as either `PYTHONHOME` or `--prefix` when installing
        python packages using pip.

        @return The scripting home directory if present, or an empty `File` if not.
     */
    File getScriptingHome() const;

    //==============================================================================

    /** Prepare a valid python home and return the config to use.

        @param destinationFolder The destination folder to use for preparing the home.
        @param standardLibraryCallback The callback to provide the standard library archive.
        @param forceInstall If true, the home will be fully rebuilt.
     */
    static std::unique_ptr<PyConfig> prepareScriptingHome (
        const File& destinationFolder,
        std::function<MemoryBlock (const char*)> standardLibraryCallback,
        bool forceInstall = false);

private:
    Result runScriptInternal (const String& code, pybind11::dict locals, pybind11::dict globals);

    File scriptingHome;
    StringArray customModules;
    String currentScriptCode;
    File currentScriptFile;

    YUP_DECLARE_WEAK_REFERENCEABLE (ScriptEngine)
};

} // namespace yup
