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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:               luau
    vendor:           luau
    version:          0.35
    name:             Luau Runtime
    description:      Luau is a fast, small, safe, gradually typed embeddable scripting language derived from Lua.
    website:          https://github.com/luau-lang/luau
    license:          MIT

    defines:          LUA_USE_LONGJMP=1 RIVE_LUAU=1
    searchpaths:      upstream/VM/include upstream/Common/include upstream/VM/src

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include "upstream/VM/include/lua.h"
#include "upstream/VM/include/lualib.h"
