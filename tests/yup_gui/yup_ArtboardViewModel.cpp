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
// ArtboardViewModel schema handle against tests/data/rive/responsive-sliders.riv
//
// responsive-sliders.riv is the Rive data binding showcase: two ViewModel
// schemas ("Slider_instance" and "Main"), authored instances per schema, and a
// "Main" artboard designed against the "Main" viewmodel. These tests assert the
// concrete schema contents of that fixture.
//==============================================================================

namespace
{

const File findResponsiveSlidersSchemaTestFile()
{
    // Try source-relative path first (works on desktop builds)
    auto dir = File (__FILE__)
                   .getParentDirectory()
                   .getParentDirectory()
                   .getChildFile ("data")
                   .getChildFile ("rive");

    if (dir.exists())
        return dir.getChildFile ("responsive-sliders.riv");

    dir = File::getCurrentWorkingDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getChildFile ("tests")
              .getChildFile ("data")
              .getChildFile ("rive");

    if (dir.exists())
        return dir.getChildFile ("responsive-sliders.riv");

    return File ("/data/rive/responsive-sliders.riv");
}

} // namespace

class ResponsiveSlidersSchemaTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = findResponsiveSlidersSchemaTestFile();
        if (! file.existsAsFile())
        {
            GTEST_SKIP() << "Missing test asset: tests/data/rive/responsive-sliders.riv";
            return;
        }

        auto result = ArtboardFile::load (file, factory);
        if (result.failed())
        {
            GTEST_SKIP() << "Failed to load test asset: " << result.getErrorMessage();
            return;
        }

        artboardFile = result.getValue();
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
};

//==============================================================================

TEST_F (ResponsiveSlidersSchemaTests, FileReportsBothViewModelsInFileOrder)
{
    ASSERT_EQ (2, artboardFile->getNumViewModels());

    const auto names = artboardFile->getViewModelNames();
    ASSERT_EQ (2, names.size());
    EXPECT_EQ (String ("Slider_instance"), names[0]);
    EXPECT_EQ (String ("Main"), names[1]);
}

TEST_F (ResponsiveSlidersSchemaTests, SliderInstanceSchemaExposesTypedProperties)
{
    auto viewModel = artboardFile->getArtboardViewModel ("Slider_instance");
    ASSERT_NE (nullptr, viewModel.get());
    EXPECT_EQ (String ("Slider_instance"), viewModel->getName());
    EXPECT_EQ (artboardFile.get(), viewModel->getArtboardFile());

    ASSERT_EQ (5, viewModel->getNumProperties());

    EXPECT_EQ (String ("Color_Main"), viewModel->getPropertyAt (0).name);
    EXPECT_EQ (ArtboardViewModel::PropertyType::color, viewModel->getPropertyAt (0).type);

    const String numberNames[] = { "max", "min", "slider_value", "width" };
    for (int i = 0; i < 4; ++i)
    {
        const auto info = viewModel->getPropertyAt (i + 1);
        EXPECT_EQ (numberNames[i], info.name);
        EXPECT_EQ (ArtboardViewModel::PropertyType::number, info.type);
        EXPECT_TRUE (info.enumValues.isEmpty());
    }

    EXPECT_TRUE (viewModel->hasProperty ("slider_value"));
    EXPECT_FALSE (viewModel->hasProperty ("nonexistentProperty"));
}

TEST_F (ResponsiveSlidersSchemaTests, MainSchemaExposesNestedAndNumberProperties)
{
    auto viewModel = artboardFile->getArtboardViewModel ("Main");
    ASSERT_NE (nullptr, viewModel.get());
    EXPECT_EQ (String ("Main"), viewModel->getName());

    ASSERT_EQ (8, viewModel->getNumProperties());

    const String nestedNames[] = { "ins_slider_Sugar", "ins_slider_Water", "ins_slider_Flour", "ins_slider_Carrots" };
    for (int i = 0; i < 4; ++i)
    {
        const auto info = viewModel->getPropertyAt (i);
        EXPECT_EQ (nestedNames[i], info.name);
        EXPECT_EQ (ArtboardViewModel::PropertyType::viewModel, info.type);
    }

    const String numberNames[] = { "Sugar", "Carrots", "Flour", "Water" };
    for (int i = 0; i < 4; ++i)
    {
        const auto info = viewModel->getPropertyAt (i + 4);
        EXPECT_EQ (numberNames[i], info.name);
        EXPECT_EQ (ArtboardViewModel::PropertyType::number, info.type);
    }
}

TEST_F (ResponsiveSlidersSchemaTests, AuthoredInstancesAreExposedPerSchema)
{
    auto sliderInstanceSchema = artboardFile->getArtboardViewModel ("Slider_instance");
    ASSERT_NE (nullptr, sliderInstanceSchema.get());
    ASSERT_EQ (4, sliderInstanceSchema->getNumInstances());

    const auto sliderNames = sliderInstanceSchema->getInstanceNames();
    ASSERT_EQ (4, sliderNames.size());
    EXPECT_EQ (String ("Sugar"), sliderNames[0]);
    EXPECT_EQ (String ("Carrots"), sliderNames[1]);
    EXPECT_EQ (String ("Water"), sliderNames[2]);
    EXPECT_EQ (String ("Flour"), sliderNames[3]);

    auto mainSchema = artboardFile->getArtboardViewModel ("Main");
    ASSERT_NE (nullptr, mainSchema.get());
    ASSERT_EQ (1, mainSchema->getNumInstances());

    const auto mainNames = mainSchema->getInstanceNames();
    ASSERT_EQ (1, mainNames.size());
    EXPECT_EQ (String ("Main"), mainNames[0]);
}

TEST_F (ResponsiveSlidersSchemaTests, CloningAuthoredInstancesSucceedsByName)
{
    auto sliderInstanceSchema = artboardFile->getArtboardViewModel ("Slider_instance");
    ASSERT_NE (nullptr, sliderInstanceSchema.get());

    for (const auto& instanceName : sliderInstanceSchema->getInstanceNames())
    {
        auto instance = artboardFile->createArtboardViewModelInstance ("Slider_instance", instanceName);
        ASSERT_NE (nullptr, instance.get());
        EXPECT_EQ (String ("Slider_instance"), instance->getName());
    }

    auto mainInstance = artboardFile->createArtboardViewModelInstance ("Main", "Main");
    ASSERT_NE (nullptr, mainInstance.get());
    EXPECT_EQ (String ("Main"), mainInstance->getName());
}

TEST_F (ResponsiveSlidersSchemaTests, IndexAndNamePropertyLookupsAgree)
{
    for (const auto& viewModelName : artboardFile->getViewModelNames())
    {
        auto viewModel = artboardFile->getArtboardViewModel (viewModelName);
        ASSERT_NE (nullptr, viewModel.get());

        for (int i = 0; i < viewModel->getNumProperties(); ++i)
        {
            const auto byIndex = viewModel->getPropertyAt (i);
            const auto byName = viewModel->getProperty (byIndex.name);

            EXPECT_EQ (byName.name, byIndex.name);
            EXPECT_EQ (byName.type, byIndex.type);
            EXPECT_EQ (byName.enumValues.size(), byIndex.enumValues.size());
        }

        EXPECT_EQ (ArtboardViewModel::PropertyType::none, viewModel->getProperty ("nonexistentProperty").type);
        EXPECT_EQ (nullptr, artboardFile->getArtboardViewModel ("nonexistentViewModel").get());
    }
}

TEST_F (ResponsiveSlidersSchemaTests, SchemasExposeNoEnumOrTriggerProperties)
{
    for (const auto& viewModelName : artboardFile->getViewModelNames())
    {
        auto viewModel = artboardFile->getArtboardViewModel (viewModelName);
        ASSERT_NE (nullptr, viewModel.get());

        for (int i = 0; i < viewModel->getNumProperties(); ++i)
        {
            EXPECT_NE (ArtboardViewModel::PropertyType::enumType, viewModel->getPropertyAt (i).type);
            EXPECT_NE (ArtboardViewModel::PropertyType::trigger, viewModel->getPropertyAt (i).type);
        }
    }
}

//==============================================================================
// ArtboardViewModel schema handle against tests/data/rive/data-binding.riv
//
// These tests discover the fixture's viewmodel content dynamically so they
// pass for any .riv that ships at least one ViewModel schema; they skip when
// the fixture exposes none.
//==============================================================================

namespace
{

const File findViewModelSchemaTestFile()
{
    // Try source-relative path first (works on desktop builds)
    auto dir = File (__FILE__)
                   .getParentDirectory()
                   .getParentDirectory()
                   .getChildFile ("data")
                   .getChildFile ("rive");

    if (dir.exists())
        return dir.getChildFile ("data-binding.riv");

    dir = File::getCurrentWorkingDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getChildFile ("tests")
              .getChildFile ("data")
              .getChildFile ("rive");

    if (dir.exists())
        return dir.getChildFile ("data-binding.riv");

    return File ("/data/rive/data-binding.riv");
}

} // namespace

class ArtboardViewModelSchemaTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = findViewModelSchemaTestFile();
        if (! file.existsAsFile())
        {
            GTEST_SKIP() << "Missing test asset: tests/data/rive/data-binding.riv";
            return;
        }

        auto result = ArtboardFile::load (file, factory);
        if (result.failed())
        {
            GTEST_SKIP() << "Failed to load test asset: " << result.getErrorMessage();
            return;
        }

        artboardFile = result.getValue();

        if (artboardFile->getNumViewModels() == 0)
        {
            GTEST_SKIP() << "Test asset exposes no ViewModel schemas: tests/data/rive/data-binding.riv";
            return;
        }
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
};

//==============================================================================

TEST_F (ArtboardViewModelSchemaTests, FileReportsConsistentViewModelNames)
{
    const auto names = artboardFile->getViewModelNames();

    EXPECT_EQ (names.size(), artboardFile->getNumViewModels());
    EXPECT_GT (names.size(), 0);

    for (int i = 0; i < names.size(); ++i)
    {
        auto viewModel = artboardFile->getArtboardViewModelAt (i);
        ASSERT_NE (nullptr, viewModel.get());
        EXPECT_EQ (viewModel->getName(), names[i]);
        EXPECT_EQ (viewModel->getArtboardFile(), artboardFile.get());
    }
}

TEST_F (ArtboardViewModelSchemaTests, UnknownViewModelsReturnNull)
{
    EXPECT_EQ (nullptr, artboardFile->getArtboardViewModelAt (artboardFile->getNumViewModels()).get());
    EXPECT_EQ (nullptr, artboardFile->getArtboardViewModel ("nonexistentViewModel").get());
}

TEST_F (ArtboardViewModelSchemaTests, PropertyInfoMatchesIndexAndNameLookup)
{
    auto viewModel = artboardFile->getArtboardViewModelAt (0);
    ASSERT_NE (nullptr, viewModel.get());

    for (int i = 0; i < viewModel->getNumProperties(); ++i)
    {
        const auto byIndex = viewModel->getPropertyAt (i);
        const auto byName = viewModel->getProperty (byIndex.name);

        EXPECT_FALSE (byIndex.name.isEmpty());
        EXPECT_EQ (byName.name, byIndex.name);
        EXPECT_EQ (byName.type, byIndex.type);
        EXPECT_EQ (byName.isInput, byIndex.isInput);
        EXPECT_EQ (byName.isOutput, byIndex.isOutput);
        EXPECT_TRUE (viewModel->hasProperty (byIndex.name));
    }
}

TEST_F (ArtboardViewModelSchemaTests, UnknownPropertiesReportEmptyInfo)
{
    auto viewModel = artboardFile->getArtboardViewModelAt (0);
    ASSERT_NE (nullptr, viewModel.get());

    const auto unknown = viewModel->getProperty ("nonexistentProperty");

    EXPECT_TRUE (unknown.name.isEmpty());
    EXPECT_EQ (ArtboardViewModel::PropertyType::none, unknown.type);
    EXPECT_FALSE (viewModel->hasProperty ("nonexistentProperty"));
}

TEST_F (ArtboardViewModelSchemaTests, AuthoredInstancesAreReportedConsistently)
{
    auto viewModel = artboardFile->getArtboardViewModelAt (0);
    ASSERT_NE (nullptr, viewModel.get());

    const auto instanceNames = viewModel->getInstanceNames();
    EXPECT_EQ (instanceNames.size(), viewModel->getNumInstances());

    // When the fixture ships authored instances, cloning by name must succeed.
    if (instanceNames.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no authored viewmodel instances";
        return;
    }

    auto instance = artboardFile->createArtboardViewModelInstance (viewModel->getName(), instanceNames[0]);
    ASSERT_NE (nullptr, instance.get());
    EXPECT_EQ (instance->getName(), viewModel->getName());
}

//==============================================================================
// Schema type coverage across every shipped fixture
//
// The schema tests above bind to one fixture at a time. This walks every schema
// of every fixture so toPropertyType() runs for each property type the fixtures
// actually provide, rather than only the ones the two fixtures above expose.
//==============================================================================

namespace
{

const StringArray& allSchemaFixtureFileNames()
{
    static const StringArray names { "data-binding.riv", "game-animation.riv", "layout-ui.riv", "responsive-sliders.riv", "viewmodel-lab.riv" };
    return names;
}

const File findAllSchemaFixturesDirectory()
{
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

// viewmodel-lab.riv ships ScriptAssets, which the Rive runtime can only import when
// it is compiled with WITH_RIVE_SCRIPTING (the rive module's `defines:`): without it
// a script's in-band FileAssetContents is routed to the previous asset's importer and
// trips the assert in FileAssetImporter::onFileAssetContents.
constexpr bool isViewModelLabSchemaScriptingAvailable() noexcept
{
#ifdef WITH_RIVE_SCRIPTING
    return true;
#else
    return false;
#endif
}

} // namespace

TEST (ArtboardViewModelFixtureCoverage, EverySchemaPropertyReportsAConsistentType)
{
    const auto directory = findAllSchemaFixturesDirectory();

    for (const auto& fileName : allSchemaFixtureFileNames())
    {
        const auto file = directory.getChildFile (fileName);
        if (! file.existsAsFile())
            continue;

        // A scripting-less Rive build cannot import the ScriptAssets of this
        // fixture, so it is covered by ViewModelLabSchemaTests instead.
        if (fileName == "viewmodel-lab.riv" && ! isViewModelLabSchemaScriptingAvailable())
            continue;

        ::testing::NiceMock<MockRiveFactory> factory;
        auto result = ArtboardFile::load (file, factory);
        if (result.failed())
            continue;

        auto artboardFile = result.getValue();

        EXPECT_EQ (artboardFile->getNumViewModels(), artboardFile->getViewModelNames().size());

        for (const auto& viewModelName : artboardFile->getViewModelNames())
        {
            auto viewModel = artboardFile->getArtboardViewModel (viewModelName);
            ASSERT_NE (nullptr, viewModel.get());
            EXPECT_EQ (viewModelName, viewModel->getName());
            EXPECT_EQ (artboardFile.get(), viewModel->getArtboardFile());

            const auto instanceNames = viewModel->getInstanceNames();
            EXPECT_EQ (viewModel->getNumInstances(), instanceNames.size());

            for (int i = 0; i < viewModel->getNumProperties(); ++i)
            {
                const auto byIndex = viewModel->getPropertyAt (i);
                ASSERT_FALSE (byIndex.name.isEmpty());

                const auto byName = viewModel->getProperty (byIndex.name);
                EXPECT_EQ (byIndex.name, byName.name);
                EXPECT_EQ (byIndex.type, byName.type);
                EXPECT_EQ (byIndex.isInput, byName.isInput);
                EXPECT_EQ (byIndex.isOutput, byName.isOutput);
                EXPECT_EQ (byIndex.enumValues.size(), byName.enumValues.size());

                EXPECT_TRUE (viewModel->hasProperty (byIndex.name));

                // Enum options are only ever populated for enum-typed properties.
                if (byIndex.type != ArtboardViewModel::PropertyType::enumType)
                    EXPECT_TRUE (byIndex.enumValues.isEmpty());
            }

            EXPECT_FALSE (viewModel->hasProperty ("definitelyNotAProperty"));
            EXPECT_EQ (ArtboardViewModel::PropertyType::none, viewModel->getProperty ("definitelyNotAProperty").type);
        }
    }
}

//==============================================================================
// ArtboardViewModel schema handle against tests/data/rive/viewmodel-lab.riv
//
// viewmodel-lab.riv is the ViewModel coverage fixture: four schemas authored in
// a known order, authored instances per schema, and a "Lab" schema declaring
// every property type the API models. The expected content asserted below is
// the one tools/rive_inspect.py reports for the fixture.
//==============================================================================

namespace
{

const File findViewModelLabSchemaTestFile()
{
    // Try source-relative path first (works on desktop builds)
    auto dir = File (__FILE__)
                   .getParentDirectory()
                   .getParentDirectory()
                   .getChildFile ("data")
                   .getChildFile ("rive");

    if (dir.exists())
        return dir.getChildFile ("viewmodel-lab.riv");

    dir = File::getCurrentWorkingDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getChildFile ("tests")
              .getChildFile ("data")
              .getChildFile ("rive");

    if (dir.exists())
        return dir.getChildFile ("viewmodel-lab.riv");

    return File ("/data/rive/viewmodel-lab.riv");
}

} // namespace

class ViewModelLabSchemaTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (! isViewModelLabSchemaScriptingAvailable())
        {
            GTEST_SKIP() << "tests/data/rive/viewmodel-lab.riv ships ScriptAssets, which needs Rive built with WITH_RIVE_SCRIPTING";
            return;
        }

        const auto file = findViewModelLabSchemaTestFile();
        if (! file.existsAsFile())
        {
            GTEST_SKIP() << "Missing test asset: tests/data/rive/viewmodel-lab.riv";
            return;
        }

        auto result = ArtboardFile::load (file, factory);
        if (result.failed())
        {
            GTEST_SKIP() << "Failed to load test asset: " << result.getErrorMessage();
            return;
        }

        artboardFile = result.getValue();
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
};

//==============================================================================

TEST_F (ViewModelLabSchemaTests, FileReportsEverySchemaInFileOrder)
{
    ASSERT_EQ (4, artboardFile->getNumViewModels());

    const auto names = artboardFile->getViewModelNames();
    ASSERT_EQ (4, names.size());
    EXPECT_EQ (String ("Details"), names[0]);
    EXPECT_EQ (String ("Row"), names[1]);
    EXPECT_EQ (String ("Panel"), names[2]);
    EXPECT_EQ (String ("Lab"), names[3]);
}

TEST_F (ViewModelLabSchemaTests, LabSchemaDeclaresEveryPropertyType)
{
    auto viewModel = artboardFile->getArtboardViewModel ("Lab");
    ASSERT_NE (nullptr, viewModel.get());
    EXPECT_EQ (String ("Lab"), viewModel->getName());
    EXPECT_EQ (artboardFile.get(), viewModel->getArtboardFile());

    struct ExpectedProperty
    {
        const char* name;
        ArtboardViewModel::PropertyType type;
    };

    const ExpectedProperty expected[] = {
        { "title", ArtboardViewModel::PropertyType::string },
        { "count", ArtboardViewModel::PropertyType::number },
        { "progress", ArtboardViewModel::PropertyType::number },
        { "enabled", ArtboardViewModel::PropertyType::boolean },
        { "accent", ArtboardViewModel::PropertyType::color },
        { "status", ArtboardViewModel::PropertyType::enumType },
        { "ping", ArtboardViewModel::PropertyType::trigger },
        { "note", ArtboardViewModel::PropertyType::string },
        { "details", ArtboardViewModel::PropertyType::viewModel },
        { "rows", ArtboardViewModel::PropertyType::list },
        { "icon", ArtboardViewModel::PropertyType::assetImage },
        { "panel", ArtboardViewModel::PropertyType::artboard },
        { "panelData", ArtboardViewModel::PropertyType::viewModel },
    };

    ASSERT_EQ (13, viewModel->getNumProperties());

    int index = 0;
    for (const auto& expectedProperty : expected)
    {
        const auto info = viewModel->getPropertyAt (index++);

        EXPECT_EQ (String (expectedProperty.name), info.name);
        EXPECT_EQ (expectedProperty.type, info.type);
        EXPECT_TRUE (viewModel->hasProperty (expectedProperty.name));
    }
}

TEST_F (ViewModelLabSchemaTests, EnumPropertyExposesItsDisplayValues)
{
    auto viewModel = artboardFile->getArtboardViewModel ("Lab");
    ASSERT_NE (nullptr, viewModel.get());

    const auto status = viewModel->getProperty ("status");
    EXPECT_EQ (ArtboardViewModel::PropertyType::enumType, status.type);

    ASSERT_EQ (3, status.enumValues.size());
    EXPECT_EQ (String ("Idle"), status.enumValues[0]);
    EXPECT_EQ (String ("Running"), status.enumValues[1]);
    EXPECT_EQ (String ("Failed"), status.enumValues[2]);

    // Options are only ever reported for enum properties.
    EXPECT_TRUE (viewModel->getProperty ("title").enumValues.isEmpty());
    EXPECT_TRUE (viewModel->getProperty ("count").enumValues.isEmpty());
    EXPECT_TRUE (viewModel->getProperty ("ping").enumValues.isEmpty());
}

TEST_F (ViewModelLabSchemaTests, RowSchemaPropertiesAreInputs)
{
    auto viewModel = artboardFile->getArtboardViewModel ("Row");
    ASSERT_NE (nullptr, viewModel.get());
    ASSERT_EQ (4, viewModel->getNumProperties());

    struct ExpectedProperty
    {
        const char* name;
        ArtboardViewModel::PropertyType type;
        bool isInput;
    };

    const ExpectedProperty expected[] = {
        { "label", ArtboardViewModel::PropertyType::string, true },
        { "amount", ArtboardViewModel::PropertyType::number, true },
        { "done", ArtboardViewModel::PropertyType::boolean, true },
        { "index", ArtboardViewModel::PropertyType::symbolListIndex, false },
    };

    int index = 0;
    for (const auto& expectedProperty : expected)
    {
        const auto info = viewModel->getPropertyAt (index++);

        EXPECT_EQ (String (expectedProperty.name), info.name);
        EXPECT_EQ (expectedProperty.type, info.type);
        EXPECT_EQ (expectedProperty.isInput, info.isInput);
        EXPECT_FALSE (info.isOutput);
    }

    // "Lab" drives an artboard of its own, so nothing is annotated as a binding.
    auto lab = artboardFile->getArtboardViewModel ("Lab");
    ASSERT_NE (nullptr, lab.get());

    for (int i = 0; i < lab->getNumProperties(); ++i)
    {
        EXPECT_FALSE (lab->getPropertyAt (i).isInput);
        EXPECT_FALSE (lab->getPropertyAt (i).isOutput);
    }
}

TEST_F (ViewModelLabSchemaTests, EverySchemaReportsItsAuthoredInstances)
{
    const auto expectInstanceNames = [] (const ArtboardViewModel::Ptr& viewModel, const StringArray& expected)
    {
        ASSERT_NE (nullptr, viewModel.get());

        const auto names = viewModel->getInstanceNames();
        ASSERT_EQ (expected.size(), names.size());
        EXPECT_EQ (expected.size(), viewModel->getNumInstances());

        for (int i = 0; i < expected.size(); ++i)
            EXPECT_EQ (expected[i], names[i]);
    };

    expectInstanceNames (artboardFile->getArtboardViewModel ("Details"), StringArray { "Default" });
    expectInstanceNames (artboardFile->getArtboardViewModel ("Row"), StringArray ({ "Default", "Alpha", "Beta", "Gamma" }));
    expectInstanceNames (artboardFile->getArtboardViewModel ("Panel"), StringArray { "Default" });
    expectInstanceNames (artboardFile->getArtboardViewModel ("Lab"), StringArray ({ "Default", "Preset" }));
}

TEST_F (ViewModelLabSchemaTests, EveryAuthoredInstanceClonesUnderItsSchemaName)
{
    for (const auto& schemaName : artboardFile->getViewModelNames())
    {
        auto viewModel = artboardFile->getArtboardViewModel (schemaName);
        ASSERT_NE (nullptr, viewModel.get());

        for (const auto& instanceName : viewModel->getInstanceNames())
        {
            auto instance = artboardFile->createArtboardViewModelInstance (viewModel->getName(), instanceName);
            ASSERT_NE (nullptr, instance.get()) << instanceName;
            EXPECT_EQ (viewModel->getName(), instance->getName());
            EXPECT_EQ (artboardFile.get(), instance->getArtboardFile());
        }
    }
}
