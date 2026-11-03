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

# Applies the platform settings of a yup_standalone_app target. Called from yup_standalone_app,
# so it reads its parsed YUP_ARG_* arguments and target_* variables directly.
function (_yup_platform_setup_standalone_app target_name)
    # An .ico next to the icon image, with the same name, becomes the icon of the executable
    get_filename_component (target_icon_directory "${target_icon}" DIRECTORY)
    get_filename_component (target_icon_name "${target_icon}" NAME_WLE)
    set (target_icon_ico "${target_icon_directory}/${target_icon_name}.ico")

    if (NOT "${target_console}" AND NOT "${target_wheel}" AND NOT "${target_static_library}" AND EXISTS "${target_icon_ico}")
        set (target_icon_rc "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_icon.rc")
        file (WRITE "${target_icon_rc}" "1 ICON \"${target_icon_ico}\"\n")
        target_sources (${target_name} PRIVATE "${target_icon_rc}")
    endif()

    if (YUP_ARG_BUNDLE_RESOURCES)
        yup_add_bundled_resources (${target_name} RESOURCES ${YUP_ARG_BUNDLE_RESOURCES})
    endif()
endfunction()
