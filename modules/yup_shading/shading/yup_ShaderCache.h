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

#pragma once

#include <yup_core/yup_core.h>

namespace yup
{

class ShaderTranspiler;
struct TranspileOptions;
enum class ShaderLanguage;
enum class ShaderStage;

//==============================================================================
/**
    An in-memory cache for compiled SPIR-V shader binaries.

    Caches the results of ShaderTranspiler::compileToSPIRV() keyed by a
    hash of the source code plus compile options. When a subsequent request
    matches an existing cache key, the cached SPIR-V binary is returned
    instantly without recompiling.

    The cache is thread-safe. Eviction uses a configurable entry-count limit.

    The cache references an externally-owned ShaderTranspiler, which must
    outlive the cache.

    @code
    auto transpiler = makeReferenceCounted<ShaderTranspiler>();
    ShaderCache cache (*transpiler);

    auto key = ShaderCache::generateCacheKey (source, ShaderStage::vertex,
                                               ShaderLanguage::glsl, opts);
    auto spirv = cache.getOrCompile (key, source, ShaderStage::vertex,
                                      ShaderLanguage::glsl, opts);
    @endcode

    @see ShaderTranspiler
*/
class YUP_API ShaderCache final
{
public:
    //==========================================================================
    /**
        Creates a cache that uses the given transpiler for miss compilations.

        The transpiler must outlive this cache.
    */
    explicit ShaderCache (ShaderTranspiler::Ptr transpilerToUse);
    ~ShaderCache();

    //==========================================================================
    /**
        Looks up a cached SPIR-V binary, or compiles one if not found.

        @param cacheKey     Pre-computed cache key (see generateCacheKey).
        @param source       The shader source code (used on cache miss).
        @param stage        Pipeline stage (used on cache miss).
        @param sourceLang   Source language (used on cache miss).
        @param options      Compilation options (used on cache miss).

        @returns The SPIR-V binary on success, or an error on failure.
    */
    ResultValue<MemoryBlock> getOrCompile (const String& cacheKey,
                                           const String& source,
                                           ShaderStage stage,
                                           ShaderLanguage sourceLang,
                                           const TranspileOptions& options = {});

    //==========================================================================
    /**
        Looks up a cached transpiled result, or transpiles if not found.

        @param cacheKey     Pre-computed cache key.
        @param source       The shader source code (used on cache miss).
        @param stage        Pipeline stage (used on cache miss).
        @param sourceLang   Source language (used on cache miss).
        @param targetLang   Target language (used on cache miss).
        @param options      Transpilation options (used on cache miss).

        @returns The target language source code on success, or an error.
    */
    ResultValue<String> getOrTranspile (const String& cacheKey,
                                        const String& source,
                                        ShaderStage stage,
                                        ShaderLanguage sourceLang,
                                        ShaderLanguage targetLang,
                                        const TranspileOptions& options = {});

    //==========================================================================
    /** Store a SPIR-V binary directly into the cache under the given key. */
    void store (const String& key, MemoryBlock spirv);

    //==========================================================================
    /** Returns true if the cache contains an entry for the given key. */
    bool contains (const String& key) const;

    //==========================================================================
    /** Remove a single entry from the cache. */
    void remove (const String& key);

    //==========================================================================
    /** Remove all entries from the cache. */
    void clear();

    //==========================================================================
    /** Returns the number of currently cached entries. */
    size_t getNumEntries() const;

    //==========================================================================
    /** Returns an approximate total byte usage of all cached entries. */
    size_t getMemoryUsage() const;

    //==========================================================================
    /** Sets the maximum number of entries before eviction. 0 = unlimited. */
    void setMaxEntries (size_t maxEntriesToUse);

    //==========================================================================
    /** Returns the maximum number of entries. */
    size_t getMaxEntries() const;

    //==========================================================================
    /**
        Generates a deterministic cache key from source + compile parameters.

        Uses SHA1 internally. The key includes the source code, shader stage,
        source language, and all transpile options.

        @param source       The shader source code.
        @param stage        Pipeline stage.
        @param sourceLang   Source language.
        @param options      Compilation options.

        @returns A hex-encoded cache key string.
    */
    static String generateCacheKey (const String& source,
                                    ShaderStage stage,
                                    ShaderLanguage sourceLang,
                                    const TranspileOptions& options = {});

private:
    struct Entry
    {
        MemoryBlock spirv;
        uint64 lastAccessOrder = 0;
    };

    void evictIfNeeded();

    ShaderTranspiler::Ptr transpiler;
    std::map<String, Entry> cache;
    size_t maxEntries = 256;
    uint64 accessCounter = 0;
    mutable CriticalSection lock;
};

} // namespace yup
