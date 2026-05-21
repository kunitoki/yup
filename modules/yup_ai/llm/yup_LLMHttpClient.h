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

namespace yup
{

//==============================================================================
/** OpenAI-compatible HTTP chat completion client.

    @tags{AI}
*/
class YUP_API LLMHttpClient : public LLMClient
{
public:
    explicit LLMHttpClient (Options options);
    ~LLMHttpClient() override;

    LLMResponse complete (const Request& request) override;
    bool completeStreaming (const Request& request, ChunkCallback onChunk) override;

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace yup
