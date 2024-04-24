/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include "sheenbidi.h"

#include "source/SBBase.c"
#include "source/RunQueue.c"
#include "source/BidiTypeLookup.c"
#include "source/SBLog.c"
#include "source/SBLine.c"
#include "source/PairingLookup.c"
#include "source/BidiChain.c"
#include "source/SBCodepointSequence.c"
#include "source/ScriptStack.c"
#include "source/SBMirrorLocator.c"
#include "source/StatusStack.c"
#include "source/SheenBidi.c"
#include "source/SBScriptLocator.c"
#include "source/GeneralCategoryLookup.c"
#include "source/LevelRun.c"
#include "source/BracketQueue.c"
#include "source/ScriptLookup.c"
#include "source/SBParagraph.c"
#include "source/IsolatingRun.c"
#include "source/SBAlgorithm.c"
