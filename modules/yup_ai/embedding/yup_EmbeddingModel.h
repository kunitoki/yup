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
/** OpenAI-compatible HTTP embedding model.

    @tags{AI}
*/
class YUP_API EmbeddingModel
{
public:
    struct Options
    {
        String model;
        String baseUrl = "http://localhost:11434/v1";
        String apiKey;
        int timeoutMs = 60000;
    };

    struct Embedding
    {
        std::vector<float> values;
        int index = 0;

        /** Returns the number of embedding dimensions. */
        int dimensions() const noexcept { return static_cast<int> (values.size()); }
    };

    explicit EmbeddingModel (Options options);
    ~EmbeddingModel();

    /** Embeds one text input. */
    Embedding embed (const String& text);

    /** Embeds a batch of text inputs. */
    std::vector<Embedding> embedBatch (const std::vector<String>& texts);

    /** Returns cosine similarity in the range [-1, 1] for non-zero vectors. */
    static float cosineSimilarity (const Embedding& a, const Embedding& b);

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace yup
