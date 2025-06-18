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
        assetInfo.uniquePath = String (asset.uniqueFilename());
        assetInfo.extension = String (asset.fileExtension());

        return assetCallback (assetInfo, Span<const uint8>{ inBandBytes.data(), inBandBytes.size() }, *factory);
    }

private:
    ArtboardFile::AssetLoadCallback assetCallback;
};

} // namespace

//==============================================================================

ArtboardFile::ArtboardFile (std::unique_ptr<rive::File> rivFile)
    : rivFile (std::move (rivFile))
{
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

//==============================================================================

ArtboardFile::LoadResult ArtboardFile::load (const File& file, rive::Factory& factory)
{
    return load (file, factory, nullptr);
}

ArtboardFile::LoadResult ArtboardFile::load (const File& file, rive::Factory& factory, const AssetLoadCallback& assetCallback)
{
    if (! file.existsAsFile())
        return LoadResult::fail ("Failed to find artboard file to load");

    auto is = file.createInputStream();
    if (is == nullptr || ! is->openedOk())
        return LoadResult::fail ("Failed to open artboard file for reading");

    return load (*is, factory, assetCallback);
}

//==============================================================================

ArtboardFile::LoadResult ArtboardFile::load (InputStream& is, rive::Factory& factory)
{
    return load (is, factory, nullptr);
}

ArtboardFile::LoadResult ArtboardFile::load (InputStream& is, rive::Factory& factory, const AssetLoadCallback& assetCallback)
{
    yup::MemoryBlock mb;
    is.readIntoMemoryBlock (mb);

    rive::ImportResult result;
    std::unique_ptr<rive::File> rivFile;

    if (assetCallback != nullptr)
    {
        rivFile = rive::File::import (
            { static_cast<const uint8_t*> (mb.getData()), mb.getSize() },
            std::addressof (factory),
            std::addressof (result),
            rive::make_rcp<LambdaAssetLoader> (assetCallback));
    }
    else
    {
        rivFile = rive::File::import (
            { static_cast<const uint8_t*> (mb.getData()), mb.getSize() },
            std::addressof (factory),
            std::addressof (result));
    }

    if (result == rive::ImportResult::malformed)
        return LoadResult::fail ("Malformed artboard file");

    if (result == rive::ImportResult::unsupportedVersion)
        return LoadResult::fail ("Unsupported artboard file for current runtime");

    if (rivFile == nullptr)
        return LoadResult::fail ("Failed to import artboard file");

    return LoadResult::ok (std::shared_ptr<ArtboardFile> (new ArtboardFile { std::move (rivFile) }));
}

} // namespace yup
