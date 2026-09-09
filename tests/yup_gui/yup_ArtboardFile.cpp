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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../mocks/rive_gpu.h"

using namespace yup;

//==============================================================================
// ArtboardFile loading, asset resolution and ViewModel schema access.
//
// Covers the load() overloads (File and InputStream, with and without an asset
// callback), the failure paths, and the schema accessors.
//==============================================================================

namespace
{

const File getArtboardFileTestDataDirectory()
{
    // Try source-relative path first (works on desktop builds)
    auto dir = File (__FILE__)
                   .getParentDirectory()
                   .getParentDirectory()
                   .getChildFile ("data")
                   .getChildFile ("rive");

    if (dir.exists())
        return dir;

    dir = File::getCurrentWorkingDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getChildFile ("tests")
              .getChildFile ("data")
              .getChildFile ("rive");

    if (dir.exists())
        return dir;

    return File ("/data/rive");
}

} // namespace

class ArtboardFileTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        riveFile = getArtboardFileTestDataDirectory().getChildFile ("data-binding.riv");

        if (! riveFile.existsAsFile())
            GTEST_SKIP() << "Missing test asset: tests/data/rive/data-binding.riv";
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    File riveFile;
};

//==============================================================================
// Loading from a file
//==============================================================================

TEST_F (ArtboardFileTests, LoadFromFileSucceeds)
{
    auto result = ArtboardFile::load (riveFile, factory);

    ASSERT_FALSE (result.failed());

    auto file = result.getValue();
    ASSERT_NE (nullptr, file.get());
    EXPECT_NE (nullptr, file->getRiveFile());
}

TEST_F (ArtboardFileTests, LoadFromMissingFileFails)
{
    const auto missing = getArtboardFileTestDataDirectory().getChildFile ("definitely-not-here.riv");

    auto result = ArtboardFile::load (missing, factory);

    EXPECT_TRUE (result.failed());
    EXPECT_TRUE (result.getErrorMessage().contains ("find"));
}

TEST_F (ArtboardFileTests, LoadFromEmptyStreamFails)
{
    MemoryBlock nothing;
    MemoryInputStream empty (nothing, false);

    // An unreadable stream reports its own error rather than the misleading
    // "Malformed artboard file".
    auto result = ArtboardFile::load (empty, factory);

    EXPECT_TRUE (result.failed());
    EXPECT_TRUE (result.getErrorMessage().contains ("read"));
}

TEST_F (ArtboardFileTests, LoadFromGarbageStreamReportsMalformed)
{
    const char garbage[] = "this is definitely not a rive file, but it is not empty either";
    MemoryInputStream stream (garbage, sizeof (garbage), false);

    auto result = ArtboardFile::load (stream, factory);

    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (ArtboardFileTests, LoadFromInputStreamMatchesLoadFromFile)
{
    auto stream = riveFile.createInputStream();
    ASSERT_NE (nullptr, stream.get());

    auto fromStream = ArtboardFile::load (*stream, factory);
    ASSERT_FALSE (fromStream.failed());

    auto fromFile = ArtboardFile::load (riveFile, factory);
    ASSERT_FALSE (fromFile.failed());

    EXPECT_EQ (fromFile.getValue()->getNumViewModels(),
               fromStream.getValue()->getNumViewModels());
    EXPECT_EQ (fromFile.getValue()->getViewModelNames(),
               fromStream.getValue()->getViewModelNames());
}

TEST_F (ArtboardFileTests, EachLoadProducesAnIndependentFile)
{
    auto first = ArtboardFile::load (riveFile, factory);
    auto second = ArtboardFile::load (riveFile, factory);

    ASSERT_FALSE (first.failed());
    ASSERT_FALSE (second.failed());

    EXPECT_NE (first.getValue().get(), second.getValue().get());
    EXPECT_NE (first.getValue()->getRiveFile(), second.getValue()->getRiveFile());
}

//==============================================================================
// Asset resolution callback
//==============================================================================

TEST_F (ArtboardFileTests, AssetCallbackIsInvokedForReferencedAssets)
{
    int assetCalls = 0;
    StringArray assetNames;

    auto result = ArtboardFile::load (riveFile,
                                      factory,
                                      [&] (const ArtboardFile::AssetInfo& info, Span<const uint8>, rive::Factory&)
                                      {
                                          ++assetCalls;
                                          assetNames.add (info.uniqueName);
                                          return false;
                                      });

    // Declining every asset is not an import failure.
    ASSERT_FALSE (result.failed());
    ASSERT_NE (nullptr, result.getValue().get());

    if (assetCalls == 0)
    {
        GTEST_SKIP() << "Test asset references no external assets";
        return;
    }

    EXPECT_GT (assetCalls, 0);
    EXPECT_EQ (assetCalls, assetNames.size());
}

TEST_F (ArtboardFileTests, AssetCallbackReceivesADescribedAsset)
{
    bool sawAnyAsset = false;

    auto result = ArtboardFile::load (riveFile,
                                      factory,
                                      [&] (const ArtboardFile::AssetInfo& info, Span<const uint8> inBandBytes, rive::Factory&)
                                      {
                                          sawAnyAsset = true;

                                          // uniqueFilename is the authored name decorated with the asset
                                          // id, then the extension: "logo-1234.png". It is a bare file
                                          // name, so it must never look like a path.
                                          EXPECT_FALSE (info.uniqueFilename.containsChar (File::getSeparatorChar()));

                                          if (info.extension.isNotEmpty())
                                              EXPECT_TRUE (info.uniqueFilename.endsWith ("." + info.extension));

                                          if (info.uniqueName.isNotEmpty())
                                              EXPECT_TRUE (info.uniqueFilename.startsWith (info.uniqueName));

                                          EXPECT_TRUE (inBandBytes.data() != nullptr || inBandBytes.size() == 0);
                                          return false;
                                      });

    ASSERT_FALSE (result.failed());

    if (! sawAnyAsset)
        GTEST_SKIP() << "Test asset references no external assets";
}

TEST_F (ArtboardFileTests, EmptyAssetCallbackFallsBackToInBandLoading)
{
    auto result = ArtboardFile::load (riveFile, factory, nullptr);

    ASSERT_FALSE (result.failed());
    EXPECT_NE (nullptr, result.getValue()->getRiveFile());
}

//==============================================================================
// ViewModel schema access
//==============================================================================

class LoadedArtboardFileTests : public ArtboardFileTests
{
protected:
    void SetUp() override
    {
        ArtboardFileTests::SetUp();

        if (::testing::Test::IsSkipped())
            return;

        auto result = ArtboardFile::load (riveFile, factory);
        if (result.failed())
        {
            GTEST_SKIP() << "Failed to load test asset: " << result.getErrorMessage();
            return;
        }

        artboardFile = result.getValue();
    }

    std::shared_ptr<ArtboardFile> artboardFile;
};

TEST_F (LoadedArtboardFileTests, ViewModelNamesMatchTheirCount)
{
    const auto names = artboardFile->getViewModelNames();

    EXPECT_EQ (artboardFile->getNumViewModels(), names.size());
}

TEST_F (LoadedArtboardFileTests, EveryViewModelIsReachableByIndexAndName)
{
    const auto names = artboardFile->getViewModelNames();

    if (names.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no ViewModel schemas";
        return;
    }

    for (int index = 0; index < names.size(); ++index)
    {
        auto byIndex = artboardFile->getArtboardViewModelAt (index);
        ASSERT_NE (nullptr, byIndex.get());
        EXPECT_EQ (names[index], byIndex->getName());

        auto byName = artboardFile->getArtboardViewModel (names[index]);
        ASSERT_NE (nullptr, byName.get());
        EXPECT_EQ (names[index], byName->getName());

        EXPECT_EQ (artboardFile.get(), byIndex->getArtboardFile());
    }
}

TEST_F (LoadedArtboardFileTests, UnknownViewModelsResolveToNull)
{
    EXPECT_EQ (nullptr, artboardFile->getArtboardViewModel ("NoSuchSchema").get());
    EXPECT_EQ (nullptr, artboardFile->getArtboardViewModelAt (-1).get());
    EXPECT_EQ (nullptr, artboardFile->getArtboardViewModelAt (artboardFile->getNumViewModels() + 10).get());

    EXPECT_EQ (nullptr, artboardFile->createArtboardViewModelInstance ("NoSuchSchema").get());
    EXPECT_EQ (nullptr, artboardFile->createArtboardViewModelInstance ("NoSuchSchema", "NoSuchInstance").get());
}

TEST_F (LoadedArtboardFileTests, HandlesKeepTheFileAlive)
{
    const auto names = artboardFile->getViewModelNames();

    if (names.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no ViewModel schemas";
        return;
    }

    auto instance = artboardFile->createArtboardViewModelInstance (names[0]);
    ASSERT_NE (nullptr, instance.get());

    auto* rawFile = artboardFile.get();

    // Dropping the caller's reference must not release the file while a handle
    // created from it is still alive.
    artboardFile.reset();

    EXPECT_EQ (rawFile, instance->getArtboardFile());
    EXPECT_NO_THROW (instance->getName());
}

TEST_F (LoadedArtboardFileTests, BothRiveFileAccessorsAgree)
{
    const auto* constFile = std::as_const (*artboardFile).getRiveFile();

    EXPECT_EQ (constFile, artboardFile->getRiveFile());
}
