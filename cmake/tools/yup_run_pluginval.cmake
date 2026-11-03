# ==============================================================================
#
#  This file is part of the YUP library.
#  Copyright (c) 2026 - kunitoki@gmail.com
#
#  YUP is an open source library subject to open-source licensing.
#
#  The code included in this file is provided under the terms of the ISC license
#  http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#  to use, copy, modify, and/or distribute this software for any purpose with or
#  without fee is hereby granted provided that the above copyright notice and
#  this permission notice appear in all copies.
#
#  YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#  DISCLAIMED.
#
# ==============================================================================

# Runs PLUGINVAL with the PLUGINVAL_ARGUMENTS list on PLUGIN_PATH, failing when it fails.
#
# JUCE's LV2 host first tries the bundle path as a plugin URI, then reloads the bundle lilv already
# found in the LV2 folder. Both print harmless lilv messages that IDEs report as build errors, so
# those two messages are rewritten before the output is printed.

execute_process (
    COMMAND "${PLUGINVAL}"
        ${PLUGINVAL_ARGUMENTS}
        --validate "${PLUGIN_PATH}"
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    RESULT_VARIABLE result)

string (REGEX REPLACE "([A-Za-z_]+\\(\\): )?(error|warning|note): (attempt to map invalid URI|Reloading plugin)" "LV2 info - \\3" output "${output}")
message (NOTICE "${output}")

if (NOT result EQUAL 0)
    message (FATAL_ERROR "pluginval failed on ${PLUGIN_PATH} (${result})")
endif()
