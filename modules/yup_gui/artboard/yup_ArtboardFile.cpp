/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

//==============================================================================

class LambdaAssetLoader : public rive::FileAssetLoader
{
public:
    LambdaAssetLoader (const ArtboardFile::AssetLoadCallback& assetCallback)
        : assetCallback (assetCallback)
    {
    }

    bool loadContents (rive::FileAsset& asset,
                       rive::Span<const uint8_t> inBandBytes,
                       rive::Factory* factory) override
    {
        jassert (factory != nullptr);

        ArtboardFile::AssetInfo assetInfo;
        assetInfo.uniqueName = String (asset.uniqueName());
        assetInfo.uniqueFilename = String (asset.uniqueFilename());
        assetInfo.extension = String (asset.fileExtension());

        return assetCallback (assetInfo, Span<const uint8> { inBandBytes.data(), inBandBytes.size() }, *factory);
    }

private:
    ArtboardFile::AssetLoadCallback assetCallback;
};

} // namespace

//==============================================================================

ArtboardFile::ArtboardFile (rive::rcp<rive::File> rivFile,
                            rive::Factory& factory,
                            rive::gpu::RenderContext* renderContext,
                            std::unique_ptr<rive::cmd::DeferredSession> deferredSession)
    : deferredReplayer (deferredSession != nullptr ? std::make_unique<rive::cmd::DeferredReplayer>() : nullptr)
    , deferredSession (std::move (deferredSession))
    , rivFile (std::move (rivFile))
    , factory (std::addressof (factory))
    , renderContext (renderContext)
{
}

ArtboardFile::~ArtboardFile()
{
    // Tearing down the session drains its pending destroys into its streams.
    bindDeferredRecordingThread();
}

//==============================================================================

const rive::File* ArtboardFile::getRiveFile() const
{
    return rivFile.get();
}

rive::File* ArtboardFile::getRiveFile()
{
    return rivFile.get();
}

rive::Factory* ArtboardFile::getFactory() const noexcept
{
    return factory;
}

rive::gpu::RenderContext* ArtboardFile::getRenderContext() const noexcept
{
    return renderContext;
}

rive::cmd::DeferredReplayer& ArtboardFile::getDeferredReplayer() noexcept
{
    jassert (deferredReplayer != nullptr);
    return *deferredReplayer;
}

void ArtboardFile::bindDeferredRecordingThread()
{
    if (deferredSession == nullptr)
        return;

    deferredSession->commandBuffer().bindRecordingThread();

    // Rive offers no mutable access to the GPU stream, though the session that owns it is mutable.
    const_cast<rive::ore::cmd::OreCommandBuffer&> (deferredSession->oreContext().stream()).bindRecordingThread();
}

//==============================================================================

int ArtboardFile::getNumViewModels() const noexcept
{
    return rivFile != nullptr ? static_cast<int> (rivFile->viewModelCount()) : 0;
}

StringArray ArtboardFile::getViewModelNames() const
{
    StringArray names;

    if (rivFile != nullptr)
        for (std::size_t index = 0; index < rivFile->viewModelCount(); ++index)
            if (auto* viewModel = rivFile->viewModel (index))
                names.add (String (viewModel->name()));

    return names;
}

ArtboardViewModel::Ptr ArtboardFile::getArtboardViewModelAt (int index)
{
    return ArtboardViewModel::createFromFile (shared_from_this(), index);
}

ArtboardViewModel::Ptr ArtboardFile::getArtboardViewModel (StringRef name)
{
    return ArtboardViewModel::createFromFile (shared_from_this(), name);
}

ArtboardViewModelInstance::Ptr ArtboardFile::createArtboardViewModelInstance (StringRef viewModelName)
{
    return ArtboardViewModelInstance::createFromFile (shared_from_this(), viewModelName);
}

ArtboardViewModelInstance::Ptr ArtboardFile::createArtboardViewModelInstance (StringRef viewModelName, StringRef instanceName)
{
    return ArtboardViewModelInstance::createFromFile (shared_from_this(), viewModelName, instanceName);
}

//==============================================================================

StringArray ArtboardFile::getGlobalViewModelNames() const
{
    StringArray names;

    if (rivFile != nullptr)
        for (const auto& name : rivFile->globalViewModelNames())
            names.add (String (name));

    return names;
}

ArtboardViewModelInstance::Ptr ArtboardFile::getGlobalViewModelInstance (StringRef name)
{
    if (rivFile == nullptr)
        return nullptr;

    const String key (name);

    if (! globalViewModelInstances.contains (key))
    {
        auto* viewModel = rivFile->viewModel (key.toStdString());
        if (viewModel == nullptr || static_cast<rive::ViewModelType> (viewModel->viewModelType()) != rive::ViewModelType::global)
            return nullptr;

        auto instance = rivFile->createDefaultViewModelInstance (viewModel);
        if (instance == nullptr)
            return nullptr;

        globalViewModelInstances.set (key, std::move (instance));
    }

    return ArtboardViewModelInstance::createFromRive (shared_from_this(), globalViewModelInstances[key].get());
}

//==============================================================================

ResultValue<ArtboardFile::Ptr> ArtboardFile::load (const File& file, rive::Factory& factory)
{
    return load (file, factory, nullptr);
}

ResultValue<ArtboardFile::Ptr> ArtboardFile::load (const File& file, rive::Factory& factory, const AssetLoadCallback& assetCallback)
{
    if (! file.existsAsFile())
        return makeResultValueFail ("Failed to find artboard file to load");

    auto is = file.createInputStream();
    if (is == nullptr || ! is->openedOk())
        return makeResultValueFail ("Failed to open artboard file for reading");

    return load (*is, factory, assetCallback);
}

//==============================================================================

ResultValue<ArtboardFile::Ptr> ArtboardFile::load (InputStream& is, rive::Factory& factory)
{
    return load (is, factory, nullptr);
}

ResultValue<ArtboardFile::Ptr> ArtboardFile::load (InputStream& is, rive::Factory& factory, const AssetLoadCallback& assetCallback)
{
    yup::MemoryBlock mb;

    // Distinguish a stream that yielded nothing from a stream that yielded bytes
    // Rive could not parse, which would otherwise both report "Malformed".
    if (is.readIntoMemoryBlock (mb) == 0)
        return makeResultValueFail ("Failed to read artboard file");

    rive::rcp<rive::FileAssetLoader> assetLoader;
    if (assetCallback != nullptr)
        assetLoader = rive::make_rcp<LambdaAssetLoader> (assetCallback);

    const auto importFile = [&] (rive::Factory& importFactory, rive::ImportResult& importResult)
    {
        return rive::File::import (
            { static_cast<const uint8_t*> (mb.getData()), mb.getSize() },
            std::addressof (importFactory),
            std::addressof (importResult),
            assetLoader);
    };

    rive::ImportResult result;
    auto rivFile = importFile (factory, result);

    if (result == rive::ImportResult::malformed)
        return makeResultValueFail ("Malformed artboard file");

    if (result == rive::ImportResult::unsupportedVersion)
        return makeResultValueFail ("Unsupported artboard file for current runtime");

    if (rivFile == nullptr)
        return makeResultValueFail ("Failed to import artboard file");

    auto* renderContext = dynamic_cast<rive::gpu::RenderContext*> (std::addressof (factory));

    const auto assets = rivFile->assets();
    const bool hasScripts = std::any_of (assets.begin(), assets.end(), [] (const auto& asset)
    {
        return asset->template is<rive::ScriptAsset>();
    });

    if (hasScripts && renderContext != nullptr && renderContext->ore() != nullptr)
    {
        auto session = std::make_unique<rive::cmd::DeferredSession> (rive::ore::ReplayCaps::from (*renderContext->ore()));
        session->bindRenderContext (renderContext);

        auto& sessionFactory = *session;
        if (auto deferredFile = importFile (sessionFactory, result))
            return makeResultValueOk (ArtboardFile::Ptr (new ArtboardFile { std::move (deferredFile), sessionFactory, renderContext, std::move (session) }));
    }

    return makeResultValueOk (ArtboardFile::Ptr (new ArtboardFile { std::move (rivFile), factory, renderContext, nullptr }));
}

} // namespace yup
