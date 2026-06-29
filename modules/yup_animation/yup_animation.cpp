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

#ifdef YUP_ANIMATION_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#include "yup_animation.h"

//==============================================================================
#include "core/yup_AnimationEasing.cpp"
#include "core/yup_AnimationTransform.cpp"
#include "model/yup_AnimationPathData.cpp"
#include "model/yup_AnimationShape.cpp"
#include "model/yup_AnimationPaint.cpp"
#include "model/yup_AnimationModifier.cpp"
#include "model/yup_AnimationGroup.cpp"
#include "model/yup_AnimationLayer.cpp"
#include "model/yup_ShapeLayer.cpp"
#include "model/yup_AnimationComposition.cpp"
#include "model/yup_AnimationKeyPath.cpp"
#include "io/yup_LottieReader.cpp"
#include "io/yup_LottieExpressionEvaluator.cpp"
#include "io/yup_LottieWriter.cpp"
#include "renderer/yup_AnimationRenderer.cpp"
#include "renderer/yup_AnimationFrameExporter.cpp"
#include "animation/yup_Animation.cpp"
#include "animation/yup_AnimationPlayer.cpp"
