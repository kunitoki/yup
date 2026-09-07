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
// ArtboardViewModelInstance handle against tests/data/rive/data-binding.riv
//
// These tests discover the fixture's viewmodel content dynamically so they
// pass for any .riv that ships at least one ViewModel schema; they skip when
// the fixture exposes none or does not contain properties of a given type.
//==============================================================================

namespace
{

const File findViewModelInstanceTestFile()
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

class ArtboardViewModelInstanceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = findViewModelInstanceTestFile();
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

        viewModel = artboardFile->getArtboardViewModelAt (0);
        if (viewModel == nullptr)
        {
            GTEST_SKIP() << "Test asset exposes no viewmodel at index 0";
            return;
        }

        instance = artboardFile->createArtboardViewModelInstance (viewModel->getName());
        if (instance == nullptr)
        {
            GTEST_SKIP() << "Failed to create a viewmodel instance from the test asset";
            return;
        }
    }

    String findPropertyName (ArtboardViewModel::PropertyType type) const
    {
        for (int i = 0; i < viewModel->getNumProperties(); ++i)
            if (viewModel->getPropertyAt (i).type == type)
                return viewModel->getPropertyAt (i).name;

        return {};
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
    ArtboardViewModel::Ptr viewModel;
    ArtboardViewModelInstance::Ptr instance;
};

//==============================================================================

TEST_F (ArtboardViewModelInstanceTests, HandleReportsItsIdentity)
{
    EXPECT_EQ (instance->getName(), viewModel->getName());
    EXPECT_EQ (instance->getArtboardFile(), artboardFile.get());
}

TEST_F (ArtboardViewModelInstanceTests, UnknownPropertiesAreRejectedSafely)
{
    EXPECT_FALSE (instance->hasProperty ("nonexistentProperty"));
    EXPECT_TRUE (instance->getProperty ("nonexistentProperty").isVoid());
    EXPECT_EQ (std::nullopt, instance->getBoolProperty ("nonexistentProperty"));
    EXPECT_EQ (std::nullopt, instance->getNumberProperty ("nonexistentProperty"));
    EXPECT_EQ (std::nullopt, instance->getStringProperty ("nonexistentProperty"));
    EXPECT_EQ (std::nullopt, instance->getColorProperty ("nonexistentProperty"));
    EXPECT_EQ (std::nullopt, instance->getEnumProperty ("nonexistentProperty"));
    EXPECT_FALSE (instance->setBoolProperty ("nonexistentProperty", true));
    EXPECT_FALSE (instance->setNumberProperty ("nonexistentProperty", 1.0));
    EXPECT_FALSE (instance->setStringProperty ("nonexistentProperty", "x"));
    EXPECT_FALSE (instance->setColorProperty ("nonexistentProperty", Color (0xFF112233u)));
    EXPECT_FALSE (instance->setEnumProperty ("nonexistentProperty", "x"));
    EXPECT_FALSE (instance->trigger ("nonexistentProperty"));
    EXPECT_FALSE (instance->setProperty ("nonexistentProperty", var (true)));
    EXPECT_EQ (-1, instance->getListSize ("nonexistentProperty"));
    EXPECT_EQ (nullptr, instance->getNestedInstance ("nonexistentProperty").get());
    EXPECT_EQ (nullptr, instance->getListItem ("nonexistentProperty", 0).get());
}

TEST_F (ArtboardViewModelInstanceTests, BooleanPropertyRoundTrips)
{
    const auto name = findPropertyName (ArtboardViewModel::PropertyType::boolean);
    if (name.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no boolean property";
        return;
    }

    EXPECT_TRUE (instance->setBoolProperty (name, true));
    EXPECT_EQ (std::optional<bool> (true), instance->getBoolProperty (name));
    EXPECT_TRUE (instance->getProperty (name).isBool());

    EXPECT_TRUE (instance->setProperty (name, var (false)));
    EXPECT_EQ (std::optional<bool> (false), instance->getBoolProperty (name));

    // Wrong var type must not clobber the property.
    EXPECT_FALSE (instance->setProperty (name, var (42)));
    EXPECT_EQ (std::optional<bool> (false), instance->getBoolProperty (name));
}

TEST_F (ArtboardViewModelInstanceTests, NumberPropertyRoundTrips)
{
    const auto name = findPropertyName (ArtboardViewModel::PropertyType::number);
    if (name.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no number property";
        return;
    }

    EXPECT_TRUE (instance->setNumberProperty (name, 12.5));
    EXPECT_EQ (std::optional<double> (12.5), instance->getNumberProperty (name));
    EXPECT_TRUE (instance->getProperty (name).isDouble());

    EXPECT_TRUE (instance->setProperty (name, var (static_cast<int64> (7))));
    EXPECT_EQ (std::optional<double> (7.0), instance->getNumberProperty (name));

    EXPECT_FALSE (instance->setProperty (name, var ("nope")));
}

TEST_F (ArtboardViewModelInstanceTests, StringPropertyRoundTrips)
{
    const auto name = findPropertyName (ArtboardViewModel::PropertyType::string);
    if (name.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no string property";
        return;
    }

    EXPECT_TRUE (instance->setStringProperty (name, "hello"));
    EXPECT_EQ (std::optional<String> ("hello"), instance->getStringProperty (name));
    EXPECT_TRUE (instance->getProperty (name).isString());

    EXPECT_TRUE (instance->setProperty (name, var (String ("world"))));
    EXPECT_EQ (std::optional<String> ("world"), instance->getStringProperty (name));

    EXPECT_FALSE (instance->setProperty (name, var (3.14)));
}

TEST_F (ArtboardViewModelInstanceTests, ColorPropertyRoundTrips)
{
    const auto name = findPropertyName (ArtboardViewModel::PropertyType::color);
    if (name.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no color property";
        return;
    }

    const Color expected (0xFF112233u);
    EXPECT_TRUE (instance->setColorProperty (name, expected));
    EXPECT_EQ (expected.getARGB(), instance->getColorProperty (name)->getARGB());

    EXPECT_TRUE (instance->setProperty (name, var (static_cast<int64> (expected.getARGB()))));
    EXPECT_EQ (expected.getARGB(), instance->getColorProperty (name)->getARGB());
}

TEST_F (ArtboardViewModelInstanceTests, EnumPropertyRoundTrips)
{
    const auto name = findPropertyName (ArtboardViewModel::PropertyType::enumType);
    if (name.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no enum property";
        return;
    }

    const auto info = viewModel->getProperty (name);
    if (info.enumValues.isEmpty())
    {
        GTEST_SKIP() << "Test asset's enum property exposes no options";
        return;
    }

    const auto& firstOption = info.enumValues[0];
    EXPECT_TRUE (instance->setEnumProperty (name, firstOption));
    EXPECT_EQ (std::optional<String> (firstOption), instance->getEnumProperty (name));

    EXPECT_FALSE (instance->setEnumProperty (name, "nonexistentOption"));
}

TEST_F (ArtboardViewModelInstanceTests, TriggerPropertyFires)
{
    const auto name = findPropertyName (ArtboardViewModel::PropertyType::trigger);
    if (name.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no trigger property";
        return;
    }

    EXPECT_TRUE (instance->trigger (name));
    EXPECT_FALSE (instance->trigger ("nonexistentProperty"));
}

TEST_F (ArtboardViewModelInstanceTests, PropertyChangedCallbackFiresOnWrites)
{
    const auto name = findPropertyName (ArtboardViewModel::PropertyType::number);
    if (name.isEmpty())
    {
        GTEST_SKIP() << "Test asset exposes no number property";
        return;
    }

    int callCount = 0;
    String lastPath;
    var lastValue;

    instance->setPropertyChangedCallback ([&] (ArtboardViewModelInstance&, const String& path, const var& value)
                                          {
                                              ++callCount;
                                              lastPath = path;
                                              lastValue = value;
                                          });

    EXPECT_TRUE (instance->setNumberProperty (name, 3.25));

    EXPECT_EQ (1, callCount);
    EXPECT_EQ (name, lastPath);
    EXPECT_EQ (var (3.25), lastValue);

    // Clearing the callback detaches the observers and stops notifications.
    instance->setPropertyChangedCallback ({});

    EXPECT_TRUE (instance->setNumberProperty (name, 9.0));
    EXPECT_EQ (1, callCount);
}

TEST_F (ArtboardViewModelInstanceTests, BindViewModelInstanceRequiresMatchingFile)
{
    // An artboard can only bind instances created from its own file.
    auto otherFile = ArtboardFile::load (findViewModelInstanceTestFile(), factory);
    if (otherFile.failed() || otherFile.getValue()->getNumViewModels() == 0)
    {
        GTEST_SKIP() << "Test asset is not loadable twice";
        return;
    }

    auto foreignInstance = otherFile.getValue()->createArtboardViewModelInstance (viewModel->getName());
    ASSERT_NE (nullptr, foreignInstance.get());

    Artboard artboard ("bindTest", artboardFile);
    artboard.setBounds (0.0f, 0.0f, 200.0f, 200.0f);

    // Foreign file instance must be rejected; same-file instance accepted.
    EXPECT_FALSE (artboard.bindViewModelInstance (foreignInstance));

    // Only exercise binding when the artboard is actually designed against the viewmodel.
    if (artboard.getViewModelName() != viewModel->getName())
    {
        GTEST_SKIP() << "Test asset's artboard is not designed against a viewmodel";
        return;
    }

    EXPECT_TRUE (artboard.bindViewModelInstance (instance));
    EXPECT_EQ (instance.get(), artboard.getBoundViewModelInstance().get());

    for (int frame = 0; frame < 5; ++frame)
        EXPECT_NO_THROW (artboard.advanceAndApply (0.016f));

    artboard.unbindViewModelInstance();
    EXPECT_EQ (nullptr, artboard.getBoundViewModelInstance().get());

    // Rebinding after unbind works again.
    EXPECT_TRUE (artboard.bindViewModelInstance (instance));
    EXPECT_EQ (instance.get(), artboard.getBoundViewModelInstance().get());
}

//==============================================================================
// ArtboardViewModelInstance handle against tests/data/rive/responsive-sliders.riv
//
// responsive-sliders.riv models a recipe with four ingredients, each driven by
// a nested "Slider_instance" viewmodel (color + min/max/slider_value/width),
// aggregated by a "Main" viewmodel (four nested viewmodels + four numbers) with
// an authored "Main" instance. These tests exercise nested viewmodels, dotted
// paths, cross-handle visibility and change callbacks on that real fixture.
//==============================================================================

namespace
{

const File findResponsiveSlidersInstanceTestFile()
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

class ResponsiveSlidersInstanceTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = findResponsiveSlidersInstanceTestFile();
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

        if (artboardFile->getArtboardViewModel ("Main") == nullptr)
        {
            GTEST_SKIP() << "Test asset exposes no 'Main' viewmodel";
            return;
        }

        mainInstance = artboardFile->createArtboardViewModelInstance ("Main", "Main");
        if (mainInstance == nullptr)
        {
            GTEST_SKIP() << "Failed to clone the authored 'Main' instance";
            return;
        }
    }

    ArtboardViewModelInstance::Ptr nestedInstance (const String& name) const
    {
        return mainInstance->getNestedInstance (name);
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
    ArtboardViewModelInstance::Ptr mainInstance;
};

//==============================================================================

TEST_F (ResponsiveSlidersInstanceTests, AuthoredMainCloneExposesFullStructure)
{
    EXPECT_EQ (String ("Main"), mainInstance->getName());
    EXPECT_EQ (artboardFile.get(), mainInstance->getArtboardFile());

    const String nestedNames[] = { "ins_slider_Sugar", "ins_slider_Water", "ins_slider_Flour", "ins_slider_Carrots" };
    for (const auto& nestedName : nestedNames)
    {
        EXPECT_TRUE (mainInstance->hasProperty (nestedName));
        EXPECT_TRUE (mainInstance->hasProperty (nestedName + ".slider_value"));
        EXPECT_TRUE (mainInstance->hasProperty (nestedName + ".Color_Main"));

        auto nested = mainInstance->getNestedInstance (nestedName);
        ASSERT_NE (nullptr, nested.get());
        EXPECT_EQ (String ("Slider_instance"), nested->getName());
        EXPECT_EQ (artboardFile.get(), nested->getArtboardFile());
    }

    const String numberNames[] = { "Sugar", "Carrots", "Flour", "Water" };
    for (const auto& numberName : numberNames)
    {
        EXPECT_TRUE (mainInstance->hasProperty (numberName));
        EXPECT_TRUE (mainInstance->getProperty (numberName).isDouble());
    }
}

TEST_F (ResponsiveSlidersInstanceTests, SchemaCreatedInstanceDefaultsToZero)
{
    auto fresh = artboardFile->createArtboardViewModelInstance ("Main");
    ASSERT_NE (nullptr, fresh.get());

    EXPECT_EQ (0.0, fresh->getNumberProperty ("Sugar").value_or (-1.0));
    EXPECT_EQ (0.0, fresh->getNumberProperty ("Carrots").value_or (-1.0));

    auto nested = fresh->getNestedInstance ("ins_slider_Sugar");
    ASSERT_NE (nullptr, nested.get());
    EXPECT_EQ (0.0, nested->getNumberProperty ("slider_value").value_or (-1.0));
    EXPECT_EQ (0.0, nested->getNumberProperty ("width").value_or (-1.0));
}

TEST_F (ResponsiveSlidersInstanceTests, NestedNumbersRoundTripThroughDottedPath)
{
    const String path = "ins_slider_Sugar.slider_value";

    const auto initial = mainInstance->getNumberProperty (path).value_or (0.0);

    EXPECT_TRUE (mainInstance->setNumberProperty (path, initial + 12.5));
    EXPECT_EQ (std::optional<double> (initial + 12.5), mainInstance->getNumberProperty (path));

    EXPECT_TRUE (mainInstance->setProperty (path, var (initial + 25.0)));
    EXPECT_EQ (std::optional<double> (initial + 25.0), mainInstance->getNumberProperty (path));
}

TEST_F (ResponsiveSlidersInstanceTests, NestedValuesAreSharedBetweenHandles)
{
    auto sugar = nestedInstance ("ins_slider_Sugar");
    ASSERT_NE (nullptr, sugar.get());

    const auto value = sugar->getNumberProperty ("slider_value").value_or (0.0);

    // Writes through the nested handle are visible through the parent's dotted path…
    EXPECT_TRUE (sugar->setNumberProperty ("slider_value", value + 3.0));
    EXPECT_EQ (std::optional<double> (value + 3.0), mainInstance->getNumberProperty ("ins_slider_Sugar.slider_value"));

    // …and vice versa.
    EXPECT_TRUE (mainInstance->setNumberProperty ("ins_slider_Sugar.slider_value", value + 7.0));
    EXPECT_EQ (std::optional<double> (value + 7.0), sugar->getNumberProperty ("slider_value"));
}

TEST_F (ResponsiveSlidersInstanceTests, NestedColorRoundTrips)
{
    auto sugar = nestedInstance ("ins_slider_Sugar");
    ASSERT_NE (nullptr, sugar.get());

    const Color expected (0xFFC05640u);
    EXPECT_TRUE (sugar->setColorProperty ("Color_Main", expected));
    EXPECT_EQ (expected.getARGB(), sugar->getColorProperty ("Color_Main")->getARGB());

    // Same value readable through the parent's dotted path.
    EXPECT_EQ (expected.getARGB(), mainInstance->getColorProperty ("ins_slider_Sugar.Color_Main")->getARGB());

    // Top-level color lookup is unknown on the "Main" schema.
    EXPECT_EQ (std::nullopt, mainInstance->getColorProperty ("Color_Main"));
}

TEST_F (ResponsiveSlidersInstanceTests, AuthoredMinMaxSliderValuesAreSane)
{
    const String nestedNames[] = { "ins_slider_Sugar", "ins_slider_Water", "ins_slider_Flour", "ins_slider_Carrots" };
    for (const auto& nestedName : nestedNames)
    {
        const auto min = mainInstance->getNumberProperty (nestedName + ".min").value_or (0.0);
        const auto max = mainInstance->getNumberProperty (nestedName + ".max").value_or (0.0);
        const auto value = mainInstance->getNumberProperty (nestedName + ".slider_value").value_or (0.0);

        EXPECT_LT (min, max);
        EXPECT_GE (value, min);
        EXPECT_LE (value, max);
    }
}

TEST_F (ResponsiveSlidersInstanceTests, UnknownNestedPathsReturnSafeDefaults)
{
    EXPECT_FALSE (mainInstance->hasProperty ("ins_slider_Sugar.nonexistentProperty"));
    EXPECT_FALSE (mainInstance->setNumberProperty ("ins_slider_Sugar.nonexistentProperty", 1.0));
    EXPECT_TRUE (mainInstance->getProperty ("ins_slider_Sugar.nonexistentProperty").isVoid());
    EXPECT_EQ (std::nullopt, mainInstance->getBoolProperty ("ins_slider_Sugar.nonexistentProperty"));
    EXPECT_EQ (std::nullopt, mainInstance->getColorProperty ("ins_slider_Sugar.nonexistentProperty"));
    EXPECT_EQ (-1, mainInstance->getListSize ("ins_slider_Sugar"));
    EXPECT_EQ (nullptr, nestedInstance ("nonexistentNested").get());
}

TEST_F (ResponsiveSlidersInstanceTests, PropertyChangedCallbackReportsDottedPaths)
{
    int callCount = 0;
    StringArray paths;
    Array<var> values;

    mainInstance->setPropertyChangedCallback ([&] (ArtboardViewModelInstance&, const String& path, const var& value)
                                              {
                                                  ++callCount;
                                                  paths.add (path);
                                                  values.add (value);
                                              });

    const String path = "ins_slider_Water.slider_value";
    const auto initial = mainInstance->getNumberProperty (path).value_or (0.0);

    EXPECT_TRUE (mainInstance->setNumberProperty (path, initial + 4.0));

    EXPECT_EQ (1, callCount);
    ASSERT_EQ (1, paths.size());
    EXPECT_EQ (path, paths[0]);
    EXPECT_TRUE (values[0].isDouble());

    mainInstance->setPropertyChangedCallback ({});
}

TEST_F (ResponsiveSlidersInstanceTests, PropertyChangedCallbackFiresForWritesThroughNestedHandle)
{
    int callCount = 0;
    String lastPath;

    mainInstance->setPropertyChangedCallback ([&] (ArtboardViewModelInstance&, const String& path, const var&)
                                              {
                                                  ++callCount;
                                                  lastPath = path;
                                              });

    auto water = nestedInstance ("ins_slider_Water");
    ASSERT_NE (nullptr, water.get());

    const auto initial = water->getNumberProperty ("width").value_or (0.0);
    EXPECT_TRUE (water->setNumberProperty ("width", initial + 1.0));

    // The parent observes its whole subtree, so writes through a nested handle
    // are reported with the dotted path.
    EXPECT_GE (callCount, 1);
    EXPECT_EQ (String ("ins_slider_Water.width"), lastPath);

    mainInstance->setPropertyChangedCallback ({});
}

//==============================================================================
// Artboard binding against the responsive-sliders.riv "Main" artboard
//==============================================================================

class ResponsiveSlidersBindingTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto file = findResponsiveSlidersInstanceTestFile();
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

        artboard = std::make_unique<Artboard> ("responsiveBind", artboardFile);
        artboard->setLayout (Artboard::Layout::layout);
        artboard->setBounds (0.0f, 0.0f, 918.0f, 552.0f);
    }

    ArtboardViewModelInstance::Ptr createMainInstance() const
    {
        return artboardFile->createArtboardViewModelInstance ("Main", "Main");
    }

    ::testing::NiceMock<MockRiveFactory> factory;
    std::shared_ptr<ArtboardFile> artboardFile;
    std::unique_ptr<Artboard> artboard;
};

//==============================================================================

TEST_F (ResponsiveSlidersBindingTests, DefaultArtboardIsDesignedAgainstMainViewModel)
{
    EXPECT_EQ (String ("Main"), artboard->getViewModelName());
}

TEST_F (ResponsiveSlidersBindingTests, BindingAuthoredMainInstanceSucceedsAndSurvivesAdvances)
{
    auto instance = createMainInstance();
    ASSERT_NE (nullptr, instance.get());

    EXPECT_TRUE (artboard->bindViewModelInstance (instance));
    EXPECT_EQ (instance.get(), artboard->getBoundViewModelInstance().get());

    // Advancing the bound artboard exercises the nested artboards, state
    // machines and data bindings of the fixture.
    for (int frame = 0; frame < 30; ++frame)
        EXPECT_NO_THROW (artboard->advanceAndApply (0.016f));
}

TEST_F (ResponsiveSlidersBindingTests, ValuesWrittenWhileBoundAreAppliedOnAdvance)
{
    auto instance = createMainInstance();
    ASSERT_NE (nullptr, instance.get());

    ASSERT_TRUE (artboard->bindViewModelInstance (instance));

    const String path = "ins_slider_Carrots.slider_value";
    const auto initial = instance->getNumberProperty (path).value_or (0.0);

    EXPECT_TRUE (instance->setNumberProperty (path, initial + 11.0));
    EXPECT_EQ (std::optional<double> (initial + 11.0), instance->getNumberProperty (path));

    for (int frame = 0; frame < 10; ++frame)
        EXPECT_NO_THROW (artboard->advanceAndApply (0.016f));
}

TEST_F (ResponsiveSlidersBindingTests, UnbindStopsAndAllowsRebinding)
{
    auto instance = createMainInstance();
    ASSERT_NE (nullptr, instance.get());

    ASSERT_TRUE (artboard->bindViewModelInstance (instance));
    artboard->unbindViewModelInstance();

    EXPECT_EQ (nullptr, artboard->getBoundViewModelInstance().get());

    for (int frame = 0; frame < 5; ++frame)
        EXPECT_NO_THROW (artboard->advanceAndApply (0.016f));

    EXPECT_TRUE (artboard->bindViewModelInstance (instance));
    EXPECT_EQ (instance.get(), artboard->getBoundViewModelInstance().get());
}

TEST_F (ResponsiveSlidersBindingTests, ForeignFileInstanceIsRejected)
{
    auto otherFile = ArtboardFile::load (findResponsiveSlidersInstanceTestFile(), factory);
    if (otherFile.failed())
    {
        GTEST_SKIP() << "Test asset is not loadable twice";
        return;
    }

    auto foreignInstance = otherFile.getValue()->createArtboardViewModelInstance ("Main", "Main");
    ASSERT_NE (nullptr, foreignInstance.get());

    EXPECT_FALSE (artboard->bindViewModelInstance (foreignInstance));
    EXPECT_EQ (nullptr, artboard->getBoundViewModelInstance().get());
}
