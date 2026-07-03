# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2026 - kunitoki@gmail.com
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

# ==============================================================================

function (yup_codesign_target target_name bundle_path)
    if (NOT YUP_PLATFORM_MAC)
        return()
    endif()

    if (ARGC GREATER 2 AND ARGV2)
        add_custom_command (TARGET ${target_name} POST_BUILD
            COMMAND codesign --force --sign - --entitlements "${ARGV2}" "${bundle_path}"
            COMMENT "Codesigning ${target_name}"
            VERBATIM)
    else()
        add_custom_command (TARGET ${target_name} POST_BUILD
            COMMAND codesign --force --sign - "${bundle_path}"
            COMMENT "Codesigning ${target_name}"
            VERBATIM)
    endif()
endfunction()
