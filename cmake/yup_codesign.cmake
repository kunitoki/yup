# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2026 - kunitoki@gmail.com
#
#   YUP is an open source library subject to open-source licensing.
#
#   The code included in this file is provided under the terms of the ISC license
#   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#   to use, copy, modify, and/or distribute this software for any purpose with or
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

    # yup_codesign_target(target_name bundle_path [sign_identity] [hardened_runtime] [entitlements_path])
    #
    # ARGV0 = target_name
    # ARGV1 = bundle_path
    # ARGV2 = signing identity (optional, default "-" for ad-hoc)
    # ARGV3 = hardened runtime (optional, TRUE|FALSE, default FALSE)
    # ARGV4 = entitlements path (optional)

    set (_codesign_identity "-")
    set (_codesign_hardened_runtime OFF)
    set (_codesign_entitlements "")

    if (ARGC GREATER 2 AND ARGV2)
        set (_codesign_identity "${ARGV2}")
    endif()
    if (ARGC GREATER 3 AND ARGV3)
        set (_codesign_hardened_runtime "${ARGV3}")
    endif()
    if (ARGC GREATER 4 AND ARGV4)
        set (_codesign_entitlements "${ARGV4}")
    endif()

    set (_codesign_options "--force")
    if (_codesign_hardened_runtime)
        list (APPEND _codesign_options "--options" "runtime")
    endif()
    if (_codesign_entitlements)
        list (APPEND _codesign_options "--entitlements" "${_codesign_entitlements}")
    endif()

    add_custom_command (TARGET ${target_name} POST_BUILD
        COMMAND codesign ${_codesign_options} --sign "${_codesign_identity}" "${bundle_path}"
        COMMENT "Codesigning ${target_name}"
        VERBATIM)
endfunction()
