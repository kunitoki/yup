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

# Returns the compile definitions and options every emscripten target compiling the YUP modules needs.
function (_yup_emscripten_get_compile_settings enable_webgpu output_definitions output_options)
    set (definitions "")
    set (options
        $<$<CONFIG:DEBUG>:-O0 -g>
        $<$<CONFIG:RELEASE>:-O3>
        -fexceptions
        -pthread
        -msimd128
        -Wno-nontrivial-memcall)

    if (enable_webgpu)
        list (APPEND definitions RIVE_WEBGPU=2)
        list (APPEND options --use-port=emdawnwebgpu)
    endif()

    set (${output_definitions} "${definitions}" PARENT_SCOPE)
    set (${output_options} "${options}" PARENT_SCOPE)
endfunction()

# ==============================================================================

# Applies the platform settings of a yup_standalone_app target. Called from yup_standalone_app,
# so it reads its parsed YUP_ARG_* arguments and target_* variables directly.
function (_yup_platform_setup_standalone_app target_name)
    if (NOT "${target_console}" AND NOT "${target_static_library}")
        set_target_properties (${target_name} PROPERTIES SUFFIX ".html")
    endif()

    _yup_set_default (YUP_ARG_CUSTOM_SHELL "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${YUP_PLATFORM}/shell.html")
    _yup_set_default (YUP_ARG_INITIAL_MEMORY 65536000) # 64mb
    _yup_set_default (YUP_ARG_PTHREAD_POOL_SIZE 8)
    _yup_set_default (YUP_ARG_STACK_SIZE 2097152) # 2mb
    _yup_set_default (YUP_ARG_EMSCRIPTEN_DEBUG_INFO FULL)

    # NAMES keeps only function names, skipping the slow DWARF and source map generation at link time
    if ("${YUP_ARG_EMSCRIPTEN_DEBUG_INFO}" STREQUAL "NAMES")
        set (debug_link_options -O0 -g2)
    elseif ("${YUP_ARG_EMSCRIPTEN_DEBUG_INFO}" STREQUAL "FULL")
        set (debug_link_options -O0 -gsource-map -g)
    else()
        _yup_message (FATAL_ERROR "${target_name} - EMSCRIPTEN_DEBUG_INFO must be FULL or NAMES, not '${YUP_ARG_EMSCRIPTEN_DEBUG_INFO}'")
    endif()

    _yup_emscripten_get_compile_settings ("${YUP_ARG_ENABLE_EMSCRIPTEN_WEBGPU}" emscripten_definitions emscripten_options)
    list (APPEND additional_definitions ${emscripten_definitions})
    list (APPEND additional_options ${emscripten_options})

    list (APPEND additional_link_options
        "$<$<CONFIG:DEBUG>:${debug_link_options}>"
        $<$<CONFIG:RELEASE>:-O2 -flto>
        -fexceptions
        -pthread
        -Wno-pthreads-mem-growth
        -sWASM=1
        -sWASM_BIGINT=1
        -sWASM_WORKERS=1
        -sAUDIO_WORKLET=1
        -sSHARED_MEMORY=1
        -sALLOW_MEMORY_GROWTH=1
        -sASSERTIONS=1
        -sEXIT_RUNTIME=1
        -sERROR_ON_UNDEFINED_SYMBOLS=1
        -sSTACK_OVERFLOW_CHECK=2
        -sSTACK_SIZE=${YUP_ARG_STACK_SIZE}
        -sINITIAL_MEMORY=${YUP_ARG_INITIAL_MEMORY}
        $<$<BOOL:${YUP_ARG_MAXIMUM_MEMORY}>:-sMAXIMUM_MEMORY=${YUP_ARG_MAXIMUM_MEMORY}>
        -sPTHREAD_POOL_SIZE=${YUP_ARG_PTHREAD_POOL_SIZE}
        -sFORCE_FILESYSTEM=1
        -sNODERAWFS=$<IF:$<BOOL:${YUP_ARG_ENABLE_EMSCRIPTEN_NODERAWFS}>,1,0>
        -sWASMFS=1
        -sFETCH=1
        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,wasmMemory
        -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$dynCall'
        --shell-file=${YUP_ARG_CUSTOM_SHELL})

    if (YUP_ARG_ENABLE_EMSCRIPTEN_GL_DEBUGGING)
        list (APPEND additional_link_options
            -sGL_ASSERTIONS=1
            -sGL_DEBUG=1)
    endif()

    if (YUP_ARG_ENABLE_EMSCRIPTEN_WEBGPU)
        list (APPEND additional_link_options
            --use-port=emdawnwebgpu
            "--pre-js=${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${YUP_PLATFORM}/webgpu.pre.js")
    endif()

    foreach (preload_file IN ITEMS ${YUP_ARG_PRELOAD_FILES})
        list (APPEND additional_link_options "--preload-file=${preload_file}")
    endforeach()

    if (YUP_ARG_BUNDLE_RESOURCES)
        yup_add_bundled_resources (${target_name} RESOURCES ${YUP_ARG_BUNDLE_RESOURCES})
    endif()

    if (NOT "${target_static_library}")
        set (target_copy_dest "$<TARGET_FILE_DIR:${target_name}>")
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${YUP_PLATFORM}/mini-coi.js"
                "${target_copy_dest}/mini-coi.js")
    endif()

    set (additional_definitions "${additional_definitions}" PARENT_SCOPE)
    set (additional_options "${additional_options}" PARENT_SCOPE)
    set (additional_link_options "${additional_link_options}" PARENT_SCOPE)
endfunction()
