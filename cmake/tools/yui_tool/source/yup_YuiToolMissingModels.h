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

#pragma once

#include "yup_YuiToolModels.h"

#include <yup_gui_markup/yup_gui_markup.h>

//==============================================================================
/** What a document binds to that its models lack. */
struct YuiToolMissingModels
{
    /** The models and properties lacking, in the shape of a models file: each property with a
        sample value of the type the document reads it as, 0 when nothing tells. Void when
        nothing lacks.
    */
    yup::var models;

    /** How many errors the document still has once they are added, which are not theirs to fix. */
    int numErrorsLeft = 0;
};

/** Finds what a file binds to that its models lack, by adding what its problems name until
    none names more.
*/
YuiToolMissingModels findYuiToolMissingModels (const yup::File& file, const YuiToolModels& models);
