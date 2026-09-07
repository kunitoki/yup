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

#include "asmjit_library.h"

// ===== core =====
#include "upstream/asmjit/core/arch_traits.cpp"
#include "upstream/asmjit/core/assembler.cpp"
#include "upstream/asmjit/core/builder.cpp"
#include "upstream/asmjit/core/code_holder.cpp"
#include "upstream/asmjit/core/code_writer.cpp"
#include "upstream/asmjit/core/compiler.cpp"
#include "upstream/asmjit/core/const_pool.cpp"
#include "upstream/asmjit/core/cpu_info.cpp"
#include "upstream/asmjit/core/debug_utils.cpp"
#include "upstream/asmjit/core/emit_helper.cpp"
#include "upstream/asmjit/core/emitter_utils.cpp"
#include "upstream/asmjit/core/emitter.cpp"
#include "upstream/asmjit/core/environment.cpp"
#include "upstream/asmjit/core/error_handler.cpp"
#include "upstream/asmjit/core/error.cpp"
#include "upstream/asmjit/core/formatter.cpp"
#include "upstream/asmjit/core/func_args_context.cpp"
#include "upstream/asmjit/core/func.cpp"
#include "upstream/asmjit/core/inst_db.cpp"
#include "upstream/asmjit/core/inst.cpp"
#include "upstream/asmjit/core/jit_allocator.cpp"
#include "upstream/asmjit/core/jit_runtime.cpp"
#include "upstream/asmjit/core/logger.cpp"
#include "upstream/asmjit/core/os_utils.cpp"
#include "upstream/asmjit/core/ra_local.cpp"
#include "upstream/asmjit/core/ra_pass.cpp"
#include "upstream/asmjit/core/ra_stack.cpp"
#include "upstream/asmjit/core/string.cpp"
#include "upstream/asmjit/core/target.cpp"
#include "upstream/asmjit/core/type.cpp"
#include "upstream/asmjit/core/virt_mem.cpp"

// ===== axl =====
#include "upstream/asmjit/axl/arena.cpp"
#include "upstream/asmjit/axl/arena_bit_set.cpp"
#include "upstream/asmjit/axl/arena_hash.cpp"
#include "upstream/asmjit/axl/arena_vector.cpp"
