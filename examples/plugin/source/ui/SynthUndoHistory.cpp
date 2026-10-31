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

#include "SynthUndoHistory.h"

namespace SynthUndoHistory
{
yup::UndoManager* find (yup::Component& component)
{
    for (auto* parent = component.getParentComponent(); parent != nullptr; parent = parent->getParentComponent())
    {
        if (auto* owner = dynamic_cast<UndoHistoryOwner*> (parent))
            return &owner->getUndoHistory();
    }

    return nullptr;
}

void beginStep (yup::Component& component)
{
    if (auto* history = find (component))
        history->beginNewTransaction();
}
} // namespace SynthUndoHistory
