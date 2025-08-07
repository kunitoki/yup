# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2024 - kunitoki@gmail.com
#
#   YUP is an open source library subject to open-source licensing.
#
#   The code included in this file is provided under the terms of the ISC license
#   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#   To use, copy, modify, and/or distribute this software for any purpose with or
#   without fee is hereby granted provided that the above copyright notice and
#   this permission notice appear in all copies.
#
#   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#   DISCLAIMED.
#
# ==============================================================================

#==============================================================================

function (yup_prepare_python_stdlib target_name python_tools_path output_variable)
    set (options "")
    set (one_value_args "")
    set (multi_value_args IGNORED_LIBRARY_PATTERNS)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (default_ignored_library_patterns "lib2to3" "pydoc_data" "_xxtestfuzz*")

    set (ignored_library_patterns ${default_ignored_library_patterns})
    list (APPEND ignored_library_patterns ${YUP_ARG_IGNORED_LIBRARY_PATTERNS})

    get_filename_component (python_tools_path "${python_tools_path}" REALPATH)
    get_filename_component (python_root_path "${Python_LIBRARY_DIRS}/.." REALPATH)

    set (python_standard_library "${CMAKE_CURRENT_BINARY_DIR}/python${Python_VERSION_MAJOR}${Python_VERSION_MINOR}.zip")

    _yup_message (STATUS "Executing python stdlib archive generator tool")
    _yup_message (STATUS " * CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
    _yup_message (STATUS " * Python_EXECUTABLE: ${Python_EXECUTABLE}")
    _yup_message (STATUS " * Python_LIBRARY_DIRS: ${Python_LIBRARY_DIRS}")
    _yup_message (STATUS " * Python_VERSION_MAJOR: ${Python_VERSION_MAJOR}")
    _yup_message (STATUS " * Python_VERSION_MINOR: ${Python_VERSION_MINOR}")
    _yup_message (STATUS " * python_root_path: ${python_root_path}")
    _yup_message (STATUS " * python_tools_path: ${python_tools_path}")
    _yup_message (STATUS " * ignored_library_patterns: ${ignored_library_patterns}")

    execute_process (
        COMMAND
            "${Python_EXECUTABLE}" "${python_tools_path}/ArchivePythonStdlib.py"
                -r "${python_root_path}" -o "${CMAKE_CURRENT_BINARY_DIR}" -M "${Python_VERSION_MAJOR}" -m "${Python_VERSION_MINOR}"
                -x "\"${ignored_library_patterns}\""
        COMMAND_ECHO STDOUT
        COMMAND_ERROR_IS_FATAL ANY)

    set (${output_variable} ${python_standard_library} PARENT_SCOPE)
endfunction ()
