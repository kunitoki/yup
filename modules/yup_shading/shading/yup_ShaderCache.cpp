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

//==============================================================================
namespace yup
{

ShaderCache::ShaderCache (ShaderTranspiler::Ptr transpilerToUse)
    : transpiler (transpilerToUse)
{
    jassert (transpiler != nullptr);
}

ShaderCache::~ShaderCache() = default;

//==============================================================================
ResultValue<MemoryBlock> ShaderCache::getOrCompile (const String& cacheKey,
                                                    const String& source,
                                                    ShaderStage stage,
                                                    ShaderLanguage sourceLang,
                                                    const TranspileOptions& options)
{
    {
        const CriticalSection::ScopedLockType sl (lock);

        if (auto it = cache.find (cacheKey); it != cache.end())
        {
            it->second.lastAccessOrder = ++accessCounter;
            return makeResultValueOk (MemoryBlock (it->second.spirv.getData(), it->second.spirv.getSize()));
        }
    }

    auto result = transpiler->compileToSPIRV (source, stage, sourceLang, options);

    if (result.failed())
        return result;

    {
        const CriticalSection::ScopedLockType sl (lock);
        evictIfNeeded();
    }

    store (cacheKey, result.getValue());

    return result;
}

//==============================================================================
ResultValue<String> ShaderCache::getOrTranspile (const String& cacheKey,
                                                 const String& source,
                                                 ShaderStage stage,
                                                 ShaderLanguage sourceLang,
                                                 ShaderLanguage targetLang,
                                                 const TranspileOptions& options)
{
    // WGSL target bypasses SPIR-V for code generation (no SPIR-V→WGSL backend).
    // Route through transpile() which handles the WGSL path internally.
    if (targetLang == ShaderLanguage::wgsl)
        return transpiler->transpile (source, stage, sourceLang, targetLang, options);

    auto spirvResult = getOrCompile (cacheKey, source, stage, sourceLang, options);

    if (spirvResult.failed())
        return makeResultValueFail (spirvResult.getErrorMessage());

    return transpiler->decompileFromSPIRV (spirvResult.getValue(), targetLang, options);
}

//==============================================================================
void ShaderCache::store (const String& key, MemoryBlock spirv)
{
    const CriticalSection::ScopedLockType sl (lock);

    Entry entry;
    entry.spirv = std::move (spirv);
    entry.lastAccessOrder = ++accessCounter;

    cache.insert_or_assign (key, std::move (entry));

    evictIfNeeded();
}

//==============================================================================
bool ShaderCache::contains (const String& key) const
{
    const CriticalSection::ScopedLockType sl (lock);
    return cache.find (key) != cache.end();
}

//==============================================================================
void ShaderCache::remove (const String& key)
{
    const CriticalSection::ScopedLockType sl (lock);
    cache.erase (key);
}

//==============================================================================
void ShaderCache::clear()
{
    const CriticalSection::ScopedLockType sl (lock);
    cache.clear();
}

//==============================================================================
size_t ShaderCache::getNumEntries() const
{
    const CriticalSection::ScopedLockType sl (lock);
    return cache.size();
}

//==============================================================================
size_t ShaderCache::getMemoryUsage() const
{
    const CriticalSection::ScopedLockType sl (lock);

    size_t total = 0;

    for (const auto& [k, entry] : cache)
        total += entry.spirv.getSize();

    return total;
}

//==============================================================================
void ShaderCache::setMaxEntries (size_t max)
{
    const CriticalSection::ScopedLockType sl (lock);
    maxEntries = max;
    evictIfNeeded();
}

//==============================================================================
size_t ShaderCache::getMaxEntries() const
{
    const CriticalSection::ScopedLockType sl (lock);
    return maxEntries;
}

//==============================================================================
String ShaderCache::generateCacheKey (const String& source,
                                      ShaderStage stage,
                                      ShaderLanguage sourceLang,
                                      const TranspileOptions& options)
{
    String payload;
    payload << source
            << "|stage:" << static_cast<int> (stage)
            << "|lang:" << static_cast<int> (sourceLang)
            << '|' << options.toCacheKeyPayload();

    SHA1 sha1 (payload.toRawUTF8(), payload.getNumBytesAsUTF8());
    return sha1.toHexString();
}

//==============================================================================
void ShaderCache::evictIfNeeded()
{
    // lock must already be held by the caller

    if (maxEntries == 0)
        return;

    while (cache.size() > maxEntries)
    {
        // Evict least recently accessed
        auto oldest = cache.begin();
        uint64 oldestOrder = std::numeric_limits<uint64>::max();

        for (auto it = cache.begin(); it != cache.end(); ++it)
        {
            if (it->second.lastAccessOrder < oldestOrder)
            {
                oldestOrder = it->second.lastAccessOrder;
                oldest = it;
            }
        }

        cache.erase (oldest);
    }
}

} // namespace yup
