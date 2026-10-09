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

#ifdef YUP_3D_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#include "yup_3d.h"

//==============================================================================

#include <tinygltf/tinygltf.h>

#include <cmath>
#include <cstring>

//==============================================================================

#include "resources/yup_Texture.cpp"
#include "resources/yup_EnvironmentMap.cpp"
#include "resources/yup_Mesh.cpp"
#include "scene/yup_EntityNode.cpp"
#include "nodes/yup_CameraNode.cpp"
#include "scene/yup_Scene.cpp"
#include "gltf/yup_GltfModel.cpp"
#include "rendering/yup_SceneDrawList.cpp"
#include "rendering/yup_SceneRenderer.cpp"
#include "rendering/yup_SceneComponent.cpp"
