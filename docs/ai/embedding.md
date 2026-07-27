# Embeddings

`EmbeddingModel` converts text to dense vector embeddings via any
OpenAI-compatible embeddings endpoint. Use it for semantic search, clustering,
recommendations, and retrieval-augmented generation (RAG).

## Creating a model

```cpp
#include <yup_ai/yup_ai.h>

yup::EmbeddingModel::Options opts;
opts.model   = "text-embedding-3-small";
opts.baseUrl = "https://api.openai.com/v1";
opts.apiKey  = "sk-...";

yup::EmbeddingModel embeddingModel (opts);
```

For Ollama, point `baseUrl` to your local server:

```cpp
yup::EmbeddingModel::Options opts;
opts.model   = "nomic-embed-text";
opts.baseUrl = "http://localhost:11434/v1";
```

## Embedding text

### Single input

```cpp
auto embedding = embeddingModel.embed ("Machine learning is fascinating.");
DBG ("Dimensions: " << embedding.dimensions());  // e.g. 1536

// Access the vector
for (float v : embedding.values)
    process (v);
```

### Batch input

```cpp
auto embeddings = embeddingModel.embedBatch ({
    "What is a neural network?",
    "How does backpropagation work?",
    "The weather is nice today."
});

for (auto& e : embeddings)
    DBG ("Index " << e.index << ": " << e.dimensions() << " dimensions");
```

## Similarity

Compute cosine similarity between two embeddings (range [-1, 1]):

```cpp
auto e1 = embeddingModel.embed ("artificial intelligence");
auto e2 = embeddingModel.embed ("machine learning");
auto e3 = embeddingModel.embed ("cooking recipes");

float scoreAIvsML  = yup::EmbeddingModel::cosineSimilarity (e1, e2);  // ~0.85
float scoreAIvsCooking = yup::EmbeddingModel::cosineSimilarity (e1, e3);  // ~0.15
```

```{note}
`cosineSimilarity()` returns `0.0f` for zero vectors and handles
floating-point rounding at the [-1, 1] boundaries.
```

## Semantic search example

```cpp
// Build a knowledge base
std::vector<std::pair<String, yup::EmbeddingModel::Embedding>> knowledgeBase;

auto addDocument = [&](const String& content)
{
    knowledgeBase.push_back ({ content, embeddingModel.embed (content) });
};

addDocument ("C++ is a statically typed, compiled language.");
addDocument ("Python is dynamically typed and interpreted.");
addDocument ("Rust guarantees memory safety without a garbage collector.");

// Search
auto query = embeddingModel.embed ("Which language is compiled?");
std::sort (knowledgeBase.begin(), knowledgeBase.end(),
    [&](const auto& a, const auto& b)
    {
        return yup::EmbeddingModel::cosineSimilarity (query, a.second)
             > yup::EmbeddingModel::cosineSimilarity (query, b.second);
    });

// Top match: "C++ is a statically typed, compiled language."
DBG (knowledgeBase.front().first);
```
