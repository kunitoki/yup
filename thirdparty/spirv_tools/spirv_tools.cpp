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

#include "upstream/source/assembly_grammar.cpp"
#include "upstream/source/binary.cpp"
#include "upstream/source/diagnostic.cpp"
#include "upstream/source/disassemble.cpp"
#include "upstream/source/ext_inst.cpp"
#include "upstream/source/extensions.cpp"
#include "upstream/source/libspirv.cpp"
//#include "upstream/source/mimalloc.cpp"
#include "upstream/source/name_mapper.cpp"
#include "upstream/source/opcode.cpp"
#include "upstream/source/operand.cpp"
#include "upstream/source/parsed_operand.cpp"
#include "upstream/source/print.cpp"
#include "upstream/source/software_version.cpp"
#include "upstream/source/spirv_endian.cpp"
#include "upstream/source/spirv_fuzzer_options.cpp"
#include "upstream/source/spirv_optimizer_options.cpp"
#define kDefaultStepLimit kDefaultStepLimit_spirvReducerOptions
#include "upstream/source/spirv_reducer_options.cpp"
#undef kDefaultStepLimit
#include "upstream/source/spirv_target_env.cpp"
#include "upstream/source/spirv_validator_options.cpp"
#include "upstream/source/table.cpp"
#include "upstream/source/table2.cpp"
#include "upstream/source/text.cpp"
#include "upstream/source/text_handler.cpp"
#include "upstream/source/to_string.cpp"

#include "upstream/source/util/bit_vector.cpp"
#include "upstream/source/util/parse_number.cpp"
#include "upstream/source/util/string_utils.cpp"
#include "upstream/source/util/timer.cpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
