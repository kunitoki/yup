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

    ID:                 yup_audio_graph
    vendor:             yup
    version:            2.0.0
    name:               YUP Audio Graph
    description:        AudioProcessor-based audio and MIDI processing graph.
    website:            https://github.com/kunitoki/yup
    license:            ISC
    minimumCppStandard: 17

    dependencies:       yup_audio_processors yup_data_model
    searchpaths:        native

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AUDIO_GRAPH_H_INCLUDED

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include <yup_audio_processors/yup_audio_processors.h>
#include <yup_data_model/yup_data_model.h>

//==============================================================================
#include "graph/yup_AudioGraphNodeID.h"
#include "graph/yup_AudioGraphEndpoint.h"
#include "graph/yup_AudioGraphConnection.h"
#include "graph/yup_AudioGraphAllocationStats.h"
#include "graph/yup_AudioGraphNodeProperties.h"
#include "graph/yup_AudioGraphModel.h"
#include "graph/yup_AudioGraphProcessor.h"
