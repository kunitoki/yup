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

    ID:                   yup_3d
    vendor:               yup
    version:              2.0.0
    name:                 YUP 3D
    description:          3D scenes made of entities and attached parts, glTF loading and PBR rendering through the RHI.
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_rhi yup_gui tinygltf
    searchpaths:          native

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_3D_H_INCLUDED

#include <yup_rhi/yup_rhi.h>
#include <yup_gui/yup_gui.h>

//==============================================================================

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

//==============================================================================

#include "scene/yup_Node.h"
#include "resources/yup_BoundingBox.h"
#include "resources/yup_Texture.h"
#include "resources/yup_EnvironmentMap.h"
#include "resources/yup_Material.h"
#include "resources/yup_Mesh.h"
#include "scene/yup_EntityNode.h"
#include "nodes/yup_MeshNode.h"
#include "nodes/yup_MaterialNode.h"
#include "nodes/yup_CameraNode.h"
#include "nodes/yup_LightNode.h"
#include "scene/yup_Scene.h"
#include "gltf/yup_GltfModel.h"
#include "rendering/yup_SceneDrawList.h"
#include "rendering/yup_SceneRenderer.h"
#include "rendering/yup_SceneComponent.h"

