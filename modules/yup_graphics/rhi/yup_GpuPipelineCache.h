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

class GraphicsContext;

//==============================================================================
/** A thread-safe cache for compiled GpuPipelines.

    Compiles-or-fetches GpuPipeline instances from a ShaderBundle keyed by a
    deterministic hash of the selected native shader sources, entry points,
    pipeline options, and graphics API. When a subsequent request matches an
    existing cache key, the cached pipeline is returned without recompilation.

    The cache references an externally-owned GraphicsContext, which must outlive
    the cache. Eviction uses a configurable entry-count limit (LRU by access
    order).

    @code
        GpuPipelineCache pipelines (ctx);
        auto pipe = pipelines.getOrCompile (bundle, options).getValue();
    @endcode

    @see GpuPipeline, ShaderBundle
*/
class YUP_API GpuPipelineCache final
{
public:
    //==============================================================================
    /** Creates a cache that uses the given context for miss compilations.

        The context must outlive this cache.
    */
    explicit GpuPipelineCache (GraphicsContext& contextToUse);

    /** Destructor. */
    ~GpuPipelineCache();

    //==============================================================================
    /** Looks up a cached pipeline, or compiles one if not found.

        The cache key is derived automatically from the bundle and options.
    */
    ResultValue<GpuPipeline::Ptr> getOrCompile (const ShaderBundle& bundle,
                                                const GpuPipelineOptions& options = {});

    /** Looks up a cached pipeline by explicit key, or compiles one if not found. */
    ResultValue<GpuPipeline::Ptr> getOrCompile (const String& cacheKey,
                                                const ShaderBundle& bundle,
                                                const GpuPipelineOptions& options = {});

    //==============================================================================
    /** Store a pipeline directly into the cache under the given key. */
    void store (const String& key, GpuPipeline::Ptr pipeline);

    /** Returns true if the cache contains an entry for the given key. */
    bool contains (const String& key) const;

    /** Remove a single entry from the cache. */
    void remove (const String& key);

    /** Remove all entries from the cache. */
    void clear();

    /** Returns the number of currently cached entries. */
    size_t getNumEntries() const;

    /** Sets the maximum number of entries before eviction. 0 = unlimited. */
    void setMaxEntries (size_t maxEntriesToUse);

    /** Returns the maximum number of entries. */
    size_t getMaxEntries() const;

    //==============================================================================
    /** Generates a deterministic cache key from a bundle + options + API.

        Uses SHA1 internally. The key includes the selected native vertex and
        fragment sources for the given API, both entry-point names, the pipeline
        options serialised by value, and the graphics API.
    */
    static String generateCacheKey (const ShaderBundle& bundle,
                                    const GpuPipelineOptions& options,
                                    GraphicsContext::Api api);

private:
    struct Entry
    {
        GpuPipeline::Ptr pipeline;
        uint64 lastAccessOrder = 0;
    };

    void evictIfNeeded();

    GraphicsContext& context;
    std::map<String, Entry> cache;
    size_t maxEntries = 256;
    uint64 accessCounter = 0;
    mutable CriticalSection lock;

    YUP_DECLARE_NON_COPYABLE (GpuPipelineCache)
};

} // namespace yup
