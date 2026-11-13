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

    ID:                   yup_expression
    vendor:               yup
    version:              2.0.1
    name:                 YUP Expression
    description:          A statically typed expression language, compiled once and evaluated per row or in batches.
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_core

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_EXPRESSION_H_INCLUDED

#include <yup_core/yup_core.h>

#include <cstring>

//==============================================================================

#include "types/yup_ExpressionType.h"
#include "types/yup_ExpressionFunction.h"
#include "language/yup_Expression.h"
#include "compiler/yup_ExpressionContext.h"
#include "compiler/yup_CompiledExpression.h"
#include "runtime/yup_ExpressionState.h"
#include "runtime/yup_ExpressionCache.h"
