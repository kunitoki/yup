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

    ID:                 yup_ai
    vendor:             yup
    version:            1.0.0
    name:               YUP AI
    description:        LLM client and AI integration classes.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_core yup_events

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AI_H_INCLUDED

#include <yup_events/yup_events.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

//==============================================================================
// Core LLM types and abstract client.
#include "llm/yup_LLMMessage.h"
#include "llm/yup_LLMTool.h"
#include "llm/yup_LLMToolRegistry.h"
#include "llm/yup_LLMResponse.h"
#include "llm/yup_LLMClient.h"
#include "llm/yup_LLMHttpClient.h"
#include "llm/yup_LLMSchema.h"

// Embedding.
#include "embedding/yup_EmbeddingModel.h"

// MCP (Model Context Protocol).
#include "mcp/yup_MCPTypes.h"
#include "mcp/yup_MCPTransport.h"
#include "mcp/yup_MCPClient.h"
#include "mcp/yup_MCPServer.h"

// Provider implementations (include before the factory).
#include "providers/yup_LLMOpenAIChatClient.h"
#include "providers/yup_LLMAnthropicClient.h"
#include "providers/yup_LLMGeminiClient.h"
#include "providers/yup_LLMOpenAIResponsesClient.h"

// Factory — depends on all provider types above.
#include "llm/yup_LLMClientFactory.h"
