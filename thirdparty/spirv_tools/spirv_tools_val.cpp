/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wunused-member-function"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wnewline-eof"
#pragma clang diagnostic ignored "-Wcast-qual"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wrestrict"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4018)
#pragma warning(disable : 4100)
#pragma warning(disable : 4146)
#pragma warning(disable : 4189)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4305)
#pragma warning(disable : 4389)
#pragma warning(disable : 4456)
#pragma warning(disable : 4457)
#pragma warning(disable : 4702)
#pragma warning(disable : 4800)
#pragma warning(disable : 4996)
#endif

#include "spirv_tools.h"

#if YUP_SPIRV_TOOLS_ENABLE_LINTER
#include "upstream/source/val/basic_block.cpp"
#include "upstream/source/val/construct.cpp"
#include "upstream/source/val/function.cpp"
#include "upstream/source/val/instruction.cpp"
#include "upstream/source/val/validate.cpp"
#include "upstream/source/val/validate_adjacency.cpp"
#include "upstream/source/val/validate_annotation.cpp"
#include "upstream/source/val/validate_arithmetics.cpp"
#include "upstream/source/val/validate_atomics.cpp"
#include "upstream/source/val/validate_barriers.cpp"
#include "upstream/source/val/validate_bitwise.cpp"
#include "upstream/source/val/validate_builtins.cpp"
#include "upstream/source/val/validate_capability.cpp"
#include "upstream/source/val/validate_cfg.cpp"
#include "upstream/source/val/validate_composites.cpp"
#include "upstream/source/val/validate_constants.cpp"
#include "upstream/source/val/validate_conversion.cpp"
#include "upstream/source/val/validate_debug.cpp"
#include "upstream/source/val/validate_decorations.cpp"
#include "upstream/source/val/validate_derivatives.cpp"
#include "upstream/source/val/validate_dot_product.cpp"
#include "upstream/source/val/validate_execution_limitations.cpp"
#include "upstream/source/val/validate_extensions.cpp"
#include "upstream/source/val/validate_function.cpp"
#include "upstream/source/val/validate_graph.cpp"
#include "upstream/source/val/validate_group.cpp"
#include "upstream/source/val/validate_id.cpp"
#include "upstream/source/val/validate_image.cpp"
#include "upstream/source/val/validate_instruction.cpp"
#include "upstream/source/val/validate_interfaces.cpp"
#include "upstream/source/val/validate_invalid_type.cpp"
#include "upstream/source/val/validate_layout.cpp"
#include "upstream/source/val/validate_literals.cpp"
#include "upstream/source/val/validate_logical_pointers.cpp"
#include "upstream/source/val/validate_logicals.cpp"
#include "upstream/source/val/validate_memory.cpp"
#include "upstream/source/val/validate_memory_semantics.cpp"
#include "upstream/source/val/validate_mesh_shading.cpp"
#include "upstream/source/val/validate_misc.cpp"
#include "upstream/source/val/validate_mode_setting.cpp"
#include "upstream/source/val/validate_non_uniform.cpp"
#include "upstream/source/val/validate_pipe.cpp"
#include "upstream/source/val/validate_primitives.cpp"
#define ValidateRayQueryPointer ValidateRayQueryPointer_rayQuery
#define GetArrayLength GetArrayLength_rayQuery
#include "upstream/source/val/validate_ray_query.cpp"
#undef ValidateRayQueryPointer
#undef GetArrayLength
#include "upstream/source/val/validate_ray_tracing.cpp"
#include "upstream/source/val/validate_ray_tracing_reorder.cpp"
#include "upstream/source/val/validate_scopes.cpp"
#include "upstream/source/val/validate_small_type_uses.cpp"
#include "upstream/source/val/validate_tensor.cpp"
#include "upstream/source/val/validate_tensor_layout.cpp"
#include "upstream/source/val/validate_type.cpp"
#include "upstream/source/val/validation_state.cpp"

#include "upstream/source/lint/divergence_analysis.cpp"
#include "upstream/source/lint/lint_divergent_derivatives.cpp"
#include "upstream/source/lint/linter.cpp"
#endif // YUP_SPIRV_TOOLS_ENABLE_LINTER

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
