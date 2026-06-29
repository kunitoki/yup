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

    ID:                 yup_animation
    vendor:             yup
    version:            1.0.0
    name:               YUP Animation Classes
    description:        Lottie-compatible animation model with rendering and export.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_core yup_graphics
    searchpaths:        native

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_ANIMATION_H_INCLUDED

#include <yup_core/yup_core.h>
#include <yup_graphics/yup_graphics.h>

#include <optional>
#include <variant>
#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>

//==============================================================================
#include "core/yup_AnimationEasing.h"
#include "core/yup_AnimationProperty.h"
#include "core/yup_AnimationTransform.h"
#include "model/yup_AnimationPathData.h"
#include "model/yup_AnimationShape.h"
#include "model/yup_AnimationPaint.h"
#include "model/yup_AnimationModifier.h"
#include "model/yup_AnimationGroup.h"
#include "model/yup_AnimationLayer.h"
#include "model/yup_ShapeLayer.h"
#include "model/yup_AnimationKeyPath.h"
#include "model/yup_AnimationComposition.h"
#include "io/yup_LottieReader.h"
#include "io/yup_LottieWriter.h"
#include "animation/yup_Animation.h"
#include "animation/yup_AnimationPlayer.h"
#include "renderer/yup_AnimationRenderer.h"
#include "renderer/yup_AnimationFrameExporter.h"
