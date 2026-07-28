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
namespace
{
var makeEmbeddingObject()
{
    return var (std::make_unique<DynamicObject>());
}

void setEmbeddingProperty (var& object, const Identifier& name, const var& value)
{
    if (auto* dynamicObject = object.getDynamicObject())
        dynamicObject->setProperty (name, value);
}

String makeEmbeddingEndpointUrl (const String& baseUrl, const String& path)
{
    return baseUrl.endsWithChar ('/') ? baseUrl.dropLastCharacters (1) + path
                                      : baseUrl + path;
}

String makeEmbeddingHeaders (const String& apiKey)
{
    String headers = "Content-Type: application/json\r\nAccept: application/json\r\n";

    if (apiKey.isNotEmpty())
        headers += "Authorization: Bearer " + apiKey + "\r\n";

    return headers;
}
} // namespace

struct EmbeddingModel::Pimpl
{
    explicit Pimpl (Options optionsToUse)
        : options (std::move (optionsToUse))
    {
    }

    std::vector<Embedding> embedBatch (const std::vector<String>& texts)
    {
        auto request = makeEmbeddingObject();

        if (options.model.isNotEmpty())
            setEmbeddingProperty (request, "model", options.model);

        var input;
        for (const auto& text : texts)
            input.append (text);

        setEmbeddingProperty (request, "input", input);

        int statusCode = 0;
        auto url = URL (makeEmbeddingEndpointUrl (options.baseUrl, "/embeddings"))
                       .withPOSTData (JSON::toString (request, true));
        auto streamOptions = URL::InputStreamOptions (URL::ParameterHandling::inPostData)
                                 .withExtraHeaders (makeEmbeddingHeaders (options.apiKey))
                                 .withConnectionTimeoutMs (options.timeoutMs)
                                 .withStatusCode (&statusCode)
                                 .withHttpRequestCmd ("POST");

        auto stream = url.createInputStream (streamOptions);
        if (stream == nullptr || statusCode < 200 || statusCode >= 300)
            return {};

        return parseEmbeddings (JSON::parse (stream->readEntireStreamAsString()));
    }

    static std::vector<Embedding> parseEmbeddings (const var& json)
    {
        std::vector<Embedding> result;

        if (auto* data = json["data"].getArray())
        {
            for (const auto& item : *data)
            {
                Embedding embedding;
                embedding.index = static_cast<int> (item["index"]);

                if (auto* values = item["embedding"].getArray())
                {
                    embedding.values.reserve (static_cast<size_t> (values->size()));

                    for (const auto& value : *values)
                        embedding.values.push_back (static_cast<float> (value));
                }

                result.push_back (std::move (embedding));
            }
        }

        return result;
    }

    Options options;
};

EmbeddingModel::EmbeddingModel (Options options)
    : pimpl (std::make_unique<Pimpl> (std::move (options)))
{
}

EmbeddingModel::~EmbeddingModel() = default;

EmbeddingModel::Embedding EmbeddingModel::embed (const String& text)
{
    auto results = embedBatch ({ text });
    return results.empty() ? Embedding {} : results.front();
}

std::vector<EmbeddingModel::Embedding> EmbeddingModel::embedBatch (const std::vector<String>& texts)
{
    return pimpl->embedBatch (texts);
}

float EmbeddingModel::cosineSimilarity (const Embedding& a, const Embedding& b)
{
    const auto count = std::min (a.values.size(), b.values.size());
    if (count == 0)
        return 0.0f;

    double dot = 0.0;
    double magnitudeA = 0.0;
    double magnitudeB = 0.0;

    for (size_t i = 0; i < count; ++i)
    {
        dot += static_cast<double> (a.values[i]) * static_cast<double> (b.values[i]);
        magnitudeA += static_cast<double> (a.values[i]) * static_cast<double> (a.values[i]);
        magnitudeB += static_cast<double> (b.values[i]) * static_cast<double> (b.values[i]);
    }

    if (magnitudeA <= 0.0 || magnitudeB <= 0.0)
        return 0.0f;

    return static_cast<float> (dot / (std::sqrt (magnitudeA) * std::sqrt (magnitudeB)));
}

} // namespace yup
