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

#ifdef YUP_ARTBOARD_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#include "yup_artboard.h"

//==============================================================================

#include <unordered_set>

//==============================================================================

#include <rive/layout.hpp>
#include <rive/file_asset_loader.hpp>
#include <rive/assets/file_asset.hpp>
#include <rive/custom_property_number.hpp>
#include <rive/custom_property_boolean.hpp>
#include <rive/custom_property_string.hpp>
#include <rive/animation/state_machine_instance.hpp>
#include <rive/animation/state_machine_input_instance.hpp>
#include <rive/container_component.hpp>
#include <rive/transform_component.hpp>
#include <rive/node.hpp>
#include <rive/shapes/shape.hpp>
#include <rive/shapes/rectangle.hpp>
#include <rive/shapes/ellipse.hpp>
#include <rive/shapes/image.hpp>
#include <rive/text/text.hpp>
#include <rive/bones/bone.hpp>
#include <rive/solo.hpp>
#include <rive/nested_artboard.hpp>
#include <rive/viewmodel/viewmodel.hpp>
#include <rive/viewmodel/viewmodel_property.hpp>
#include <rive/viewmodel/viewmodel_property_number.hpp>
#include <rive/viewmodel/viewmodel_property_string.hpp>
#include <rive/viewmodel/viewmodel_property_boolean.hpp>
#include <rive/viewmodel/viewmodel_property_color.hpp>
#include <rive/viewmodel/viewmodel_property_list.hpp>
#include <rive/viewmodel/viewmodel_property_enum.hpp>
#include <rive/viewmodel/viewmodel_property_trigger.hpp>
#include <rive/viewmodel/viewmodel_property_viewmodel.hpp>
#include <rive/viewmodel/viewmodel_property_symbol_list_index.hpp>
#include <rive/viewmodel/viewmodel_property_asset_image.hpp>
#include <rive/viewmodel/viewmodel_property_asset_font.hpp>
#include <rive/viewmodel/viewmodel_property_asset_blob.hpp>
#include <rive/viewmodel/viewmodel_property_artboard.hpp>
#include <rive/viewmodel/viewmodel_instance.hpp>
#include <rive/viewmodel/viewmodel_instance_value.hpp>
#include <rive/viewmodel/viewmodel_instance_number.hpp>
#include <rive/viewmodel/viewmodel_instance_string.hpp>
#include <rive/viewmodel/viewmodel_instance_boolean.hpp>
#include <rive/viewmodel/viewmodel_instance_color.hpp>
#include <rive/viewmodel/viewmodel_instance_enum.hpp>
#include <rive/viewmodel/viewmodel_instance_trigger.hpp>
#include <rive/viewmodel/viewmodel_instance_list.hpp>
#include <rive/viewmodel/viewmodel_instance_list_item.hpp>
#include <rive/viewmodel/viewmodel_instance_viewmodel.hpp>
#include <rive/viewmodel/viewmodel_instance_asset_image.hpp>
#include <rive/viewmodel/viewmodel_instance_asset_font.hpp>
#include <rive/viewmodel/viewmodel_instance_asset_blob.hpp>
#include <rive/assets/blob_asset.hpp>
#include <rive/assets/script_asset.hpp>
#include <rive/renderer/render_context.hpp>
#include <rive/renderer/rive_render_image.hpp>
#include <rive/renderer/cmd/deferred_host.hpp>
#include <rive/view_model_type.hpp>
#include <rive/math/transform_components.hpp>

//==============================================================================

#include "artboard/yup_ArtboardViewModel.cpp"
#include "artboard/yup_ArtboardViewModelInstance.cpp"
#include "artboard/yup_ArtboardFile.cpp"
#include "artboard/yup_ArtboardNode.cpp"
#include "artboard/yup_Artboard.cpp"
