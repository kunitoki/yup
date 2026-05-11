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

#include "luau.h"

#include "upstream/VM/src/lapi.cpp"
#include "upstream/VM/src/laux.cpp"
#include "upstream/VM/src/lbaselib.cpp"
#include "upstream/VM/src/lbitlib.cpp"
#include "upstream/VM/src/lbuffer.cpp"
#include "upstream/VM/src/lbuflib.cpp"
#include "upstream/VM/src/lbuiltins.cpp"
#include "upstream/VM/src/lcorolib.cpp"
#include "upstream/VM/src/ldblib.cpp"
#include "upstream/VM/src/ldebug.cpp"
#include "upstream/VM/src/ldo.cpp"
#include "upstream/VM/src/lfunc.cpp"
#include "upstream/VM/src/lgc.cpp"
#include "upstream/VM/src/lgcdebug.cpp"
#include "upstream/VM/src/linit.cpp"
#include "upstream/VM/src/lmathlib.cpp"
#include "upstream/VM/src/lmem.cpp"
#include "upstream/VM/src/lnumprint.cpp"
#include "upstream/VM/src/lobject.cpp"
#include "upstream/VM/src/loslib.cpp"
#include "upstream/VM/src/lstate.cpp"
#include "upstream/VM/src/lstring.cpp"
#include "upstream/VM/src/lstrlib.cpp"
#include "upstream/VM/src/ltable.cpp"
#include "upstream/VM/src/ltablib.cpp"
#include "upstream/VM/src/ltm.cpp"
#include "upstream/VM/src/ludata.cpp"
#include "upstream/VM/src/lutf8lib.cpp"
#define createmetatable createmetatable_vec
#include "upstream/VM/src/lveclib.cpp"
#undef createmetatable
#include "upstream/VM/src/lvmexecute.cpp"
#include "upstream/VM/src/lvmload.cpp"
#include "upstream/VM/src/lvmutils.cpp"

#include "upstream/VM/src/lperf.cpp"
