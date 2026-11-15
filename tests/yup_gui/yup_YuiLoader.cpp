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

#if __has_include(<YuiTestResources.h>)
#define YUP_HAS_YUI_TEST_RESOURCES 1
#include <YuiTestResources.h>
#endif

#if __has_include(<yui-nested-resources.h>)
#define YUP_HAS_YUI_NESTED_RESOURCES 1
#include <yui-nested-resources.h>
#endif

using namespace yup;

class YuiLoaderTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);

        folder = File::getSpecialLocation (File::tempDirectory)
                     .getChildFile ("YUP_YuiLoaderTests_" + String::toHexString (Random::getSystemRandom().nextInt()));

        folder.deleteRecursively();
        folder.createDirectory();

        writeStyle (7);
        write ("ui.yui", R"yui(
            import "parts/style.yui";

            Component {
                Slider { id: knob; class: "knob"; }
            })yui");
    }

    void TearDown() override
    {
        folder.deleteRecursively();

        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    void write (const String& path, const String& text)
    {
        auto file = folder.getChildFile (path);
        file.getParentDirectory().createDirectory();
        file.replaceWithText (text);
    }

    void writeStyle (int trackWidth)
    {
        write ("parts/style.yui", "style { .knob { trackWidth: " + String (trackWidth) + "; } }");
    }

    double getTrackWidth (YuiDocument& document) const
    {
        auto* knob = document.findById<Slider> ("knob");
        return knob != nullptr ? static_cast<double> (knob->findProperty (Identifier ("trackWidth"))) : -1.0;
    }

    static const uint8* findResource (const char* path, std::size_t& size)
    {
        static const char* main = R"yui(
            import "style.yui";

            Component {
                Slider { id: knob; class: "knob"; }
            })yui";

        static const char* style = "style { .knob { trackWidth: 3; } }";

        const char* text = nullptr;

        if (String (path) == "ui.yui")
            text = main;
        else if (String (path) == "style.yui")
            text = style;

        size = text != nullptr ? std::strlen (text) : 0;
        return reinterpret_cast<const uint8*> (text);
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
    File folder;
};

TEST_F (YuiLoaderTests, LoadsAFileWithItsImportsRelativeToIt)
{
    YuiDocument document;
    YuiLoader loader (document);

    const auto result = loader.loadFile (folder.getChildFile ("ui.yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (loader.getFile(), folder.getChildFile ("ui.yui"));
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 7.0);
}

TEST_F (YuiLoaderTests, ReloadsTheFileInPlaceAndReportsEachLoad)
{
    YuiDocument document;
    YuiLoader loader (document);

    Array<bool> loads;
    loader.onLoad = [&] (const Result& result)
    {
        loads.add (result.wasOk());
    };

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    auto* knob = document.findById ("knob");

    writeStyle (9);
    const auto result = loader.reload();

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (document.findById ("knob"), knob);
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 9.0);
    EXPECT_EQ (loads, Array<bool> ({ true, true }));
}

TEST_F (YuiLoaderTests, KeepsTheInterfaceWhenAReloadFails)
{
    YuiDocument document;
    YuiLoader loader (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    auto* root = document.getRoot();

    Array<bool> loads;
    loader.onLoad = [&] (const Result& result)
    {
        loads.add (result.wasOk());
    };

    write ("parts/style.yui", "style { .knob { ");

    EXPECT_TRUE (loader.reload().failed());
    EXPECT_EQ (loads, Array<bool> ({ false }));
    EXPECT_EQ (document.getRoot(), root);
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 7.0);
    ASSERT_FALSE (document.getDiagnostics().empty());
    EXPECT_EQ (document.getDiagnostics().front().source, "parts/style.yui");
}

TEST_F (YuiLoaderTests, FailsWhenTheFileCantBeRead)
{
    YuiDocument document;
    YuiLoader loader (document);

    EXPECT_TRUE (loader.loadFile (folder.getChildFile ("missing.yui")).failed());
    EXPECT_EQ (document.getRoot(), nullptr);
}

TEST_F (YuiLoaderTests, ReloadsFromAStringWithTheImportsOfTheFile)
{
    YuiDocument document;
    YuiLoader loader (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());

    const auto result = loader.reloadFromString (R"yui(
        import "parts/style.yui";

        Component {
            Slider { id: knob; class: "knob"; }
            Label { id: title; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_NE (document.findById ("title"), nullptr);
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 7.0);

    // The string stands for the file until another source is loaded
    writeStyle (5);
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_NE (document.findById ("title"), nullptr);
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 5.0);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_EQ (document.findById ("title"), nullptr);
}

TEST_F (YuiLoaderTests, OverridesTheTextOfFilesUntilAnotherSourceIsLoaded)
{
    YuiDocument document;
    YuiLoader loader (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());

    // Paths are relative to the main file's folder, the main file is its own name
    loader.setOverride ("parts/style.yui", "style { .knob { trackWidth: 2; } }");
    loader.setOverride ("ui.yui", R"yui(
        import "parts/style.yui";

        Component {
            Slider { id: knob; class: "knob"; }
            Label { id: title; }
        })yui");

    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_NE (document.findById ("title"), nullptr);
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 2.0);

    writeStyle (5);
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 2.0);

    loader.clearOverrides();
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (document.findById ("title"), nullptr);
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 5.0);

    loader.setOverride ("parts/style.yui", "style { .knob { trackWidth: 2; } }");
    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 5.0);
}

TEST_F (YuiLoaderTests, ReturnsTheTextLoadedForEachPath)
{
    YuiDocument document;
    YuiLoader loader (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_EQ (loader.getLoadedText ("ui.yui"), folder.getChildFile ("ui.yui").loadFileAsString());
    EXPECT_EQ (loader.getLoadedText ("parts/style.yui"), folder.getChildFile ("parts/style.yui").loadFileAsString());
    EXPECT_FALSE (loader.getLoadedText ("knob.yui").has_value());

    // An override is the text loaded once a load used it
    loader.setOverride ("parts/style.yui", "style {}");
    EXPECT_EQ (loader.getLoadedText ("parts/style.yui"), folder.getChildFile ("parts/style.yui").loadFileAsString());

    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (loader.getLoadedText ("parts/style.yui"), String ("style {}"));

    ASSERT_TRUE (loader.loadResource ("ui.yui", &YuiLoaderTests::findResource).wasOk());
    EXPECT_EQ (loader.getLoadedText ("style.yui"), String ("style { .knob { trackWidth: 3; } }"));
    EXPECT_FALSE (loader.getLoadedText ("parts/style.yui").has_value());
}

TEST_F (YuiLoaderTests, LoadsAStringWithoutASource)
{
    YuiDocument document;
    YuiLoader loader (document);

    EXPECT_TRUE (loader.reload().failed());

    const String text ("Component { Slider { id: knob; } }");
    ASSERT_TRUE (loader.reloadFromString (text).wasOk());
    EXPECT_NE (document.findById ("knob"), nullptr);
    EXPECT_EQ (loader.getLoadedText (""), text);

    EXPECT_TRUE (loader.reloadFromString ("import \"style.yui\"; Component {}").failed());
    EXPECT_NE (document.findById ("knob"), nullptr);
}

TEST_F (YuiLoaderTests, LoadsEmbeddedResources)
{
    YuiDocument document;
    YuiLoader loader (document);

    const auto result = loader.loadResource ("ui.yui", &YuiLoaderTests::findResource);

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (loader.getFile(), File());
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 3.0);

    EXPECT_TRUE (loader.loadResource ("missing.yui", &YuiLoaderTests::findResource).failed());
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 3.0);
}

TEST_F (YuiLoaderTests, ResolvesNestedRelativeImportsInResourcesMatchingFiles)
{
    write ("shared/common.yui", "tokens { property sharedWidth: number = 6; }");
    write ("sub/knob.yui", "import \"../shared/common.yui\"; style { .knob { trackWidth: sharedWidth; } }");
    write ("sub/panel.yui", R"yui(
        import "knob.yui";

        Component {
            Slider { id: knob; class: "knob"; }
        })yui");

    YuiDocument fileDocument;
    YuiLoader fileLoader (fileDocument);
    ASSERT_TRUE (fileLoader.loadFile (folder.getChildFile ("sub/panel.yui")).wasOk());
    EXPECT_DOUBLE_EQ (getTrackWidth (fileDocument), 6.0);

    const auto findNestedResource = [] (const char* path, std::size_t& size) -> const uint8*
    {
        static const char* common = "tokens { property sharedWidth: number = 6; }";
        static const char* knob = "import \"../shared/common.yui\"; style { .knob { trackWidth: sharedWidth; } }";
        static const char* panel = "import \"knob.yui\"; Component { Slider { id: knob; class: \"knob\"; } }";

        const char* text = nullptr;
        const String key (path != nullptr ? path : "");

        if (key == "shared/common.yui")
            text = common;
        else if (key == "sub/knob.yui")
            text = knob;
        else if (key == "sub/panel.yui")
            text = panel;

        size = text != nullptr ? std::strlen (text) : 0;
        return reinterpret_cast<const uint8*> (text);
    };

    YuiDocument resourceDocument;
    YuiLoader resourceLoader (resourceDocument);
    const auto result = resourceLoader.loadResource ("sub/panel.yui", findNestedResource);
    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_DOUBLE_EQ (getTrackWidth (resourceDocument), 6.0);

    resourceLoader.setOverride ("./sub/../sub/knob.yui", "style { .knob { trackWidth: 12; } }");
    ASSERT_TRUE (resourceLoader.reload().wasOk());
    EXPECT_DOUBLE_EQ (getTrackWidth (resourceDocument), 12.0);
    EXPECT_EQ (resourceLoader.getLoadedText ("sub\\knob.yui"), String ("style { .knob { trackWidth: 12; } }"));
}

#if YUP_HAS_YUI_NESTED_RESOURCES
TEST_F (YuiLoaderTests, LoadsHyphenatedTargetAndNestedResourcesEmbeddedWithCMake)
{
    YuiDocument document;
    YuiLoader loader (document);

    const auto result = loader.loadResource ("parts/panel.yui", yui_nested_resources_find);

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 4.0);
}
#endif

#if YUP_HAS_YUI_TEST_RESOURCES
TEST_F (YuiLoaderTests, LoadsResourcesEmbeddedWithCMake)
{
    YuiDocument document;
    YuiLoader loader (document);

    const auto result = loader.loadResource ("ui.yui", YuiTestResources_find);

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 4.0);
}
#endif

#if YUP_LINUX || YUP_WINDOWS || YUP_MAC
TEST_F (YuiLoaderTests, ReloadsWhenAWatchedFileChanges)
{
    YuiDocument document;
    YuiLoader loader (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());

    int loads = 0;
    loader.onLoad = [&] (const Result&)
    {
        ++loads;
    };

    loader.setWatching (true);
    EXPECT_TRUE (loader.isWatching());

    // Give the file system watcher time to start
    MessageManager::getInstance()->runDispatchLoopUntil (300);
    writeStyle (11);

    for (int i = 0; i < 100 && getTrackWidth (document) != 11.0; ++i)
        MessageManager::getInstance()->runDispatchLoopUntil (50);

    EXPECT_GE (loads, 1);
    EXPECT_DOUBLE_EQ (getTrackWidth (document), 11.0);

    // An overridden file isn't read again, the others still reload the document
    loader.setOverride ("ui.yui", R"yui(
        import "parts/style.yui";

        Component {
            Slider { id: knob; class: "knob"; }
            Label { id: title; }
        })yui");

    ASSERT_TRUE (loader.reload().wasOk());
    writeStyle (13);

    for (int i = 0; i < 100 && getTrackWidth (document) != 13.0; ++i)
        MessageManager::getInstance()->runDispatchLoopUntil (50);

    EXPECT_DOUBLE_EQ (getTrackWidth (document), 13.0);
    EXPECT_NE (document.findById ("title"), nullptr);

    loader.setWatching (false);
    EXPECT_FALSE (loader.isWatching());
}
#endif
