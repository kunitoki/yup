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

namespace
{

ShaderLanguage cacheShaderLanguageForApi (GraphicsContext::Api api)
{
    switch (api)
    {
        case GraphicsContext::Metal:
            return ShaderLanguage::msl;
        case GraphicsContext::Direct3D:
            return ShaderLanguage::hlsl;
        case GraphicsContext::OpenGLES:
            return ShaderLanguage::essl;
        case GraphicsContext::WebGPU:
            return ShaderLanguage::wgsl;
        default:
            return ShaderLanguage::glsl;
    }
}

const ShaderInfo* findShaderVariant (const ShaderBundle& bundle, ShaderStage stage, ShaderLanguage targetLang)
{
    const ShaderInfo* info = bundle.findShader (stage, targetLang);
    if (info == nullptr && targetLang == ShaderLanguage::essl)
        info = bundle.findShader (stage, ShaderLanguage::glsl);

    return info;
}

void appendPipelineOptions (String& payload, const GpuPipelineOptions& o)
{
    payload << "|topo:" << (int) o.topology
            << "|idx:" << (int) o.indexFormat
            << "|cull:" << (int) o.cullMode
            << "|wind:" << (int) o.winding
            << "|ctc:" << (int) o.colorTargetCount;

    for (uint32_t i = 0; i < o.colorTargetCount && i < 4; ++i)
    {
        const auto& t = o.colorTargets[i];
        payload << "|ct" << (int) i << ':' << (int) t.format << ',' << (int) t.blendEnabled
                << ',' << (int) t.blend.srcColor << ',' << (int) t.blend.dstColor << ',' << (int) t.blend.colorOp
                << ',' << (int) t.blend.srcAlpha << ',' << (int) t.blend.dstAlpha << ',' << (int) t.blend.alphaOp;
    }

    payload << "|ds:" << (int) o.depthStencil.enabled << ',' << (int) o.depthStencil.format
            << ',' << (int) o.depthStencil.depthCompare << ',' << (int) o.depthStencil.depthWriteEnabled;

    auto appendFace = [&payload] (const GpuStencilFaceState& f)
    {
        payload << ',' << (int) f.compare << ',' << (int) f.failOp << ',' << (int) f.depthFailOp << ',' << (int) f.passOp;
    };
    payload << "|sf";
    appendFace (o.stencilFront);
    payload << "|sb";
    appendFace (o.stencilBack);

    payload << "|srm:" << (int) o.stencilReadMask << "|swm:" << (int) o.stencilWriteMask
            << "|smp:" << (int) o.sampleCount
            << "|vbc:" << (int) o.vertexBufferCount;

    for (uint32_t i = 0; i < o.vertexBufferCount; ++i)
    {
        const auto& vb = o.vertexBuffers[i];
        payload << "|vb" << (int) i << ':' << (int) vb.stride << ',' << (int) vb.stepMode << ',' << (int) vb.attributeCount;

        for (uint32_t a = 0; a < vb.attributeCount; ++a)
        {
            const auto& at = vb.attributes[a];
            payload << ",a" << (int) a << ':' << (int) at.format << ',' << (int) at.offset << ',' << (int) at.shaderLocation;
        }
    }
}

} // namespace

//==============================================================================

GpuPipelineCache::GpuPipelineCache (GraphicsContext& contextToUse)
    : context (contextToUse)
{
}

GpuPipelineCache::~GpuPipelineCache() = default;

//==============================================================================

String GpuPipelineCache::generateCacheKey (const ShaderBundle& bundle,
                                           const GpuPipelineOptions& options,
                                           GraphicsContext::Api api)
{
    const auto targetLang = cacheShaderLanguageForApi (api);

    const ShaderInfo* vs = findShaderVariant (bundle, ShaderStage::vertex, targetLang);
    const ShaderInfo* fs = findShaderVariant (bundle, ShaderStage::fragment, targetLang);

    String payload;
    payload << "api:" << (int) api;

    if (vs != nullptr)
        payload << "|vs:" << vs->source << "|vse:" << vs->entryPoint;
    else
        payload << "|vs:<none>";

    if (fs != nullptr)
        payload << "|fs:" << fs->source << "|fse:" << fs->entryPoint;
    else
        payload << "|fs:<none>";

    appendPipelineOptions (payload, options);

    SHA1 sha1 (payload.toRawUTF8(), payload.getNumBytesAsUTF8());
    return sha1.toHexString();
}

//==============================================================================

ResultValue<GpuPipeline::Ptr> GpuPipelineCache::getOrCompile (const ShaderBundle& bundle,
                                                              const GpuPipelineOptions& options)
{
    const auto key = generateCacheKey (bundle, options, context.getApi());
    return getOrCompile (key, bundle, options);
}

ResultValue<GpuPipeline::Ptr> GpuPipelineCache::getOrCompile (const String& cacheKey,
                                                              const ShaderBundle& bundle,
                                                              const GpuPipelineOptions& options)
{
    {
        const CriticalSection::ScopedLockType sl (lock);

        if (auto it = cache.find (cacheKey); it != cache.end())
        {
            it->second.lastAccessOrder = ++accessCounter;
            return makeResultValueOk (it->second.pipeline);
        }
    }

    auto result = GpuPipeline::compileFromBundle (context, bundle, options);

    if (result.failed())
        return result;

    store (cacheKey, result.getValue());

    return result;
}

//==============================================================================

void GpuPipelineCache::store (const String& key, GpuPipeline::Ptr pipeline)
{
    const CriticalSection::ScopedLockType sl (lock);

    Entry entry;
    entry.pipeline = std::move (pipeline);
    entry.lastAccessOrder = ++accessCounter;

    cache.insert_or_assign (key, std::move (entry));

    evictIfNeeded();
}

//==============================================================================

bool GpuPipelineCache::contains (const String& key) const
{
    const CriticalSection::ScopedLockType sl (lock);
    return cache.find (key) != cache.end();
}

//==============================================================================

void GpuPipelineCache::remove (const String& key)
{
    const CriticalSection::ScopedLockType sl (lock);
    cache.erase (key);
}

//==============================================================================

void GpuPipelineCache::clear()
{
    const CriticalSection::ScopedLockType sl (lock);
    cache.clear();
}

//==============================================================================

size_t GpuPipelineCache::getNumEntries() const
{
    const CriticalSection::ScopedLockType sl (lock);
    return cache.size();
}

//==============================================================================

void GpuPipelineCache::setMaxEntries (size_t max)
{
    const CriticalSection::ScopedLockType sl (lock);
    maxEntries = max;
    evictIfNeeded();
}

//==============================================================================

size_t GpuPipelineCache::getMaxEntries() const
{
    const CriticalSection::ScopedLockType sl (lock);
    return maxEntries;
}

//==============================================================================

void GpuPipelineCache::evictIfNeeded()
{
    // lock must already be held by the caller

    if (maxEntries == 0)
        return;

    while (cache.size() > maxEntries)
    {
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
