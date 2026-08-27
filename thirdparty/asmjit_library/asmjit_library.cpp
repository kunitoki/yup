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
#include "upstream/asmjit/core/archtraits.cpp"
#include "upstream/asmjit/core/assembler.cpp"
#include "upstream/asmjit/core/builder.cpp"
#include "upstream/asmjit/core/codeholder.cpp"
#include "upstream/asmjit/core/codewriter.cpp"
#include "upstream/asmjit/core/compiler.cpp"
#include "upstream/asmjit/core/constpool.cpp"
#include "upstream/asmjit/core/cpuinfo.cpp"
#include "upstream/asmjit/core/emithelper.cpp"
#include "upstream/asmjit/core/emitter.cpp"
#include "upstream/asmjit/core/emitterutils.cpp"
#include "upstream/asmjit/core/environment.cpp"
#include "upstream/asmjit/core/errorhandler.cpp"
#include "upstream/asmjit/core/formatter.cpp"
#include "upstream/asmjit/core/func.cpp"
#include "upstream/asmjit/core/funcargscontext.cpp"
#include "upstream/asmjit/core/globals.cpp"
#include "upstream/asmjit/core/inst.cpp"
#include "upstream/asmjit/core/instdb.cpp"
#include "upstream/asmjit/core/jitallocator.cpp"
#include "upstream/asmjit/core/jitruntime.cpp"
#include "upstream/asmjit/core/logger.cpp"
#include "upstream/asmjit/core/operand.cpp"
#include "upstream/asmjit/core/osutils.cpp"
#include "upstream/asmjit/core/ralocal.cpp"
#include "upstream/asmjit/core/rapass.cpp"
#include "upstream/asmjit/core/rastack.cpp"
#include "upstream/asmjit/core/string.cpp"
#include "upstream/asmjit/core/target.cpp"
#include "upstream/asmjit/core/type.cpp"
#include "upstream/asmjit/core/virtmem.cpp"

// ===== support =====
#include "upstream/asmjit/support/arena.cpp"
#include "upstream/asmjit/support/arenabitset.cpp"
#include "upstream/asmjit/support/arenahash.cpp"
#include "upstream/asmjit/support/arenalist.cpp"
#include "upstream/asmjit/support/arenatree.cpp"
#include "upstream/asmjit/support/arenavector.cpp"
#include "upstream/asmjit/support/support.cpp"
