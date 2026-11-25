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

#include <yup_gui_markup/yup_gui_markup.h>

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

    // A panel in a folder of its own, with the source of a shader in that folder and a compiled one beside it
    static const uint8* findPanelResource (const char* path, std::size_t& size)
    {
        static const char* panel = "Component { Slider { id: knob; } }";
        static const char* shader = "void main() {}";

        const String key (path);
        const uint8* data = nullptr;
        size = 0;

        if (key == "ui/panel.yui")
        {
            data = reinterpret_cast<const uint8*> (panel);
            size = std::strlen (panel);
        }
        else if (key == "ui/fx/crt.frag")
        {
            data = reinterpret_cast<const uint8*> (shader);
            size = std::strlen (shader);
        }
        else if (key == "shared/crt.ysl")
        {
            data = panelBundle;
            size = sizeof (panelBundle);
        }

        return data;
    }

    static String textOf (const std::optional<MemoryBlock>& bytes)
    {
        return bytes.has_value() ? bytes->toString() : String ("<nothing>");
    }

    // Records the loads of a document and the assets it is told about, for as long as it lives
    struct AssetRecorder final : YuiDocument::Listener
    {
        explicit AssetRecorder (YuiDocument& documentToRecord)
            : document (documentToRecord)
        {
            document.addListener (this);
        }

        ~AssetRecorder() override
        {
            document.removeListener (this);
        }

        void documentLoaded (YuiDocument&, const Result&) override
        {
            ++loads;
        }

        void documentAssetChanged (YuiDocument&, const String& path) override
        {
            assets.add (path);

            if (readsAgain)
                texts.add (textOf (document.readAsset (path)));
        }

        YuiDocument& document;
        int loads = 0;
        StringArray assets;

        // What a load keeps holds what it read of an asset, and reads it again when told about it
        bool readsAgain = false;
        StringArray texts;
    };

    static constexpr uint8 panelBundle[] = { 0x52, 0x49, 0x46, 0x46, 0x00, 0xff, 0x01, 0x00 };

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

TEST_F (YuiLoaderTests, ResolvesRelativeImageAndSvgAssetsAgainstTheLoadedYuiDirectory)
{
    static constexpr const char* redDotPngBase64 =
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";

    MemoryOutputStream pngBytes;
    ASSERT_TRUE (Base64::convertFromBase64 (pngBytes, redDotPngBase64));

    auto pngFile = folder.getChildFile ("assets/dot.png");
    pngFile.getParentDirectory().createDirectory();
    ASSERT_TRUE (pngFile.replaceWithData (pngBytes.getData(), pngBytes.getDataSize()));

    write ("assets/icon.svg", "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 8 8'><rect width='8' height='8' fill='#ff0000'/></svg>");
    write ("sub/panel.yui", R"yui(
        drawing RelativeBadge {
            image { bounds: bounds; source: "../assets/dot.png"; }
            svg { bounds: bounds; source: "../assets/icon.svg"; }
        }
        style {
            #drawn { drawing: RelativeBadge; }
        }
        Row {
            Image { id: pngImg; source: "../assets/dot.png"; }
            Image { id: svgImg; source: "../assets/icon.svg"; }
            Component { id: drawn; }
        })yui");

    YuiDocument document;
    YuiLoader loader (document);
    const auto result = loader.loadFile (folder.getChildFile ("sub/panel.yui"));

    ASSERT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_EQ (document.getBaseDirectory(), folder.getChildFile ("sub"));

    const auto* pngImg = document.findById<ImageComponent> ("pngImg");
    ASSERT_NE (pngImg, nullptr);
    EXPECT_TRUE (pngImg->getImage().isValid());

    const auto* svgImg = document.findById<ImageComponent> ("svgImg");
    ASSERT_NE (svgImg, nullptr);
    EXPECT_NE (svgImg->getDrawable(), nullptr);

    const auto* drawn = document.findById ("drawn");
    ASSERT_NE (drawn, nullptr);
    ASSERT_NE (drawn->getStyle(), nullptr);

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (16, 16);
    Graphics g (*context, *renderer);
    Component panel;
    panel.setBounds (0.0f, 0.0f, 16.0f, 16.0f);
    drawn->getStyle()->paint (g, *theme, panel);
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

TEST_F (YuiLoaderTests, AResourceSourceServesTheAssetsOfItsFinder)
{
    write ("fx/glow.frag", "glow");

    YuiDocument document;
    YuiLoader loader (document);

    ASSERT_TRUE (loader.loadResource ("ui/panel.yui", &YuiLoaderTests::findPanelResource).wasOk());

    // The paths are relative to the folder of the main file, as they are for a file source
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "void main() {}");
    EXPECT_EQ (textOf (YuiDocument::readAssetFor (document.findById ("knob"), "fx/crt.frag")), "void main() {}");

    const auto bundle = document.readAsset ("../shared/crt.ysl");
    ASSERT_TRUE (bundle.has_value());
    EXPECT_TRUE (bundle->matches (panelBundle, sizeof (panelBundle)));

    EXPECT_FALSE (document.readAsset ("fx/missing.frag").has_value());

    // What is embedded is no file, a file at an absolute path still is
    EXPECT_EQ (document.getAssetFile ("fx/crt.frag"), File());
    EXPECT_EQ (textOf (document.readAsset (folder.getChildFile ("fx/glow.frag").getFullPathName())), "glow");
}

TEST_F (YuiLoaderTests, AFileSourceServesTheAssetsOfItsFolder)
{
    write ("fx/crt.frag", "void main() {}");
    write ("sub/fx/glow.frag", "glow");
    write ("sub/panel.yui", "Component { Slider { id: knob; } }");
    ASSERT_TRUE (folder.getChildFile ("sub/fx/crt.ysl").replaceWithData (panelBundle, sizeof (panelBundle)));

    YuiDocument document;
    YuiLoader loader (document);
    AssetRecorder recorder (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("sub/panel.yui")).wasOk());

    EXPECT_EQ (textOf (document.readAsset ("fx/glow.frag")), "glow");
    EXPECT_EQ (textOf (document.readAsset ("../fx/crt.frag")), "void main() {}");
    EXPECT_EQ (textOf (YuiDocument::readAssetFor (document.findById ("knob"), "fx/glow.frag")), "glow");

    const auto bundle = document.readAsset ("fx/crt.ysl");
    ASSERT_TRUE (bundle.has_value());
    EXPECT_TRUE (bundle->matches (panelBundle, sizeof (panelBundle)));

    EXPECT_FALSE (document.readAsset ("fx/missing.frag").has_value());
    EXPECT_EQ (document.getAssetFile ("fx/glow.frag"), folder.getChildFile ("sub/fx/glow.frag"));

    // The document finds these files under its base directory by itself. Only the loader looks after the ones it
    // served, by the path they were read by: a file read by its absolute path is none of its source
    EXPECT_EQ (textOf (document.readAsset (folder.getChildFile ("fx/crt.frag").getFullPathName())), "void main() {}");

    write ("sub/fx/glow.frag", "glowing");
    write ("fx/crt.frag", "void main() { discard; }");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/glow.frag", "../fx/crt.frag" }));
}

TEST_F (YuiLoaderTests, AnOverrideWinsOverTheAssetOfTheSource)
{
    write ("fx/crt.frag", "saved");

    YuiDocument document;
    YuiLoader loader (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "saved");

    loader.setOverride ("fx/crt.frag", "unsaved");
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "unsaved");
    EXPECT_EQ (textOf (document.readAsset ("./fx/../fx/crt.frag")), "unsaved");

    // A file that isn't saved yet can be overridden too
    loader.setOverride ("fx/new.frag", "never saved");
    EXPECT_EQ (textOf (document.readAsset ("fx/new.frag")), "never saved");

    loader.clearOverrides();
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "saved");
    EXPECT_FALSE (document.readAsset ("fx/new.frag").has_value());

    // The overrides of resources are the paths the finder knows
    ASSERT_TRUE (loader.loadResource ("ui/panel.yui", &YuiLoaderTests::findPanelResource).wasOk());
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "void main() {}");

    loader.setOverride ("ui/fx/crt.frag", "unsaved");
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "unsaved");
}

TEST_F (YuiLoaderTests, ServesTheAssetsOfTheSourceLoadedLastAndNoneOnceDestroyed)
{
    write ("fx/crt.frag", "from the folder");

    const auto inWorkingDirectory = File::getCurrentWorkingDirectory().getChildFile ("fx/crt.frag");

    YuiDocument document;

    {
        YuiLoader loader (document);

        ASSERT_TRUE (loader.loadResource ("ui/panel.yui", &YuiLoaderTests::findPanelResource).wasOk());
        EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "void main() {}");

        ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
        EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "from the folder");
        EXPECT_FALSE (document.readAsset ("../shared/crt.ysl").has_value());

        ASSERT_TRUE (loader.loadResource ("ui/panel.yui", &YuiLoaderTests::findPanelResource).wasOk());
        EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "void main() {}");
        EXPECT_EQ (document.getAssetFile ("fx/crt.frag"), File());
    }

    // The reader went with the loader, which leaves the document with the files of the working directory
    EXPECT_FALSE (document.readAsset ("fx/crt.frag").has_value());
    EXPECT_EQ (document.getAssetFile ("fx/crt.frag"), inWorkingDirectory);
}

TEST_F (YuiLoaderTests, ReloadingReportsTheAssetsWhoseOverrideChanged)
{
    write ("fx/crt.frag", "saved");
    write ("fx/glow.frag", "glow");

    YuiDocument document;
    YuiLoader loader (document);
    AssetRecorder recorder (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "saved");

    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_TRUE (recorder.assets.isEmpty());

    // An override set on an asset that was read is reported by the next load, and only by that one
    loader.setOverride ("fx/crt.frag", "unsaved");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ("fx/crt.frag"));
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "unsaved");

    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets.size(), 1);

    // So is another text, a load from a string being a load as any other
    loader.setOverride ("fx/crt.frag", "unsaved again");
    ASSERT_TRUE (loader.reloadFromString (folder.getChildFile ("ui.yui").loadFileAsString()).wasOk());
    EXPECT_EQ (recorder.assets.size(), 2);

    // And so is the override being removed
    loader.clearOverrides();
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/crt.frag", "fx/crt.frag" }));
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "saved");

    // An asset nothing read is never reported, whatever happens to it
    loader.setOverride ("fx/glow.frag", "unsaved glow");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets.size(), 3);

    // What read an asset while it was something else is told too, even once it is back to what it was
    loader.setOverride ("fx/crt.frag", "passing");
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "passing");

    loader.clearOverrides();
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets.size(), 4);
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "saved");

    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets.size(), 4);
}

TEST_F (YuiLoaderTests, ReloadingReportsTheAssetsWhoseFileChanged)
{
    write ("fx/crt.frag", "one");
    write ("fx/glow.frag", "glow");

    YuiDocument document;
    YuiLoader loader (document);
    AssetRecorder recorder (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "one");
    EXPECT_FALSE (document.readAsset ("fx/new.frag").has_value());

    // A file of another size was written again, one nothing read isn't looked at
    write ("fx/crt.frag", "three");
    write ("fx/glow.frag", "glow again");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ("fx/crt.frag"));
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "three");

    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets.size(), 1);

    // A file that was missing when it was asked for is reported once it is there
    write ("fx/new.frag", "new");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/new.frag" }));

    // The file of an overridden asset isn't what is read: it can change
    loader.setOverride ("fx/crt.frag", "unsaved");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets.size(), 3);

    write ("fx/crt.frag", "fourth!");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/new.frag", "fx/crt.frag" }));
}

TEST_F (YuiLoaderTests, ReloadingAResourceSourceOnlyReportsTheAssetsOverridden)
{
    YuiDocument document;
    YuiLoader loader (document);
    AssetRecorder recorder (document);

    ASSERT_TRUE (loader.loadResource ("ui/panel.yui", &YuiLoaderTests::findPanelResource).wasOk());
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "void main() {}");
    EXPECT_TRUE (document.readAsset ("../shared/crt.ysl").has_value());

    // What is embedded doesn't change
    ASSERT_TRUE (loader.reload().wasOk());
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_TRUE (recorder.assets.isEmpty());

    // An override does, and the asset is reported by the path the document read it by
    loader.setOverride ("ui/fx/crt.frag", "unsaved");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ("fx/crt.frag"));
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "unsaved");

    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets.size(), 1);

    loader.clearOverrides();
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/crt.frag" }));
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "void main() {}");
}

TEST_F (YuiLoaderTests, LoadingTheSameFileAgainTellsWhatReadItsAssets)
{
    write ("fx/crt.frag", "one");

    YuiDocument document;
    YuiLoader loader (document);
    AssetRecorder recorder (document);
    recorder.readsAgain = true;

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "one");

    // A load is in place and keeps what read the asset, which is told once to read it again
    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_EQ (recorder.assets, StringArray ("fx/crt.frag"));
    EXPECT_EQ (recorder.texts, StringArray ("one"));

    // Reading it again is what has its next change reported
    write ("fx/crt.frag", "three");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/crt.frag" }));
    EXPECT_EQ (recorder.texts, StringArray ({ "one", "three" }));
}

TEST_F (YuiLoaderTests, LoadingAnotherFolderTellsWhatReadTheAssetsOfTheOneBefore)
{
    write ("first/ui.yui", "Component { Slider { id: knob; } }");
    write ("first/fx/crt.frag", "one");
    write ("first/fx/glow.frag", "glow");
    write ("second/ui.yui", "Component { Slider { id: knob; } }");
    write ("second/fx/crt.frag", "three");
    write ("second/fx/glow.frag", "another glow");

    YuiDocument document;
    YuiLoader loader (document);
    AssetRecorder recorder (document);
    recorder.readsAgain = true;

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("first/ui.yui")).wasOk());
    auto* knob = document.findById ("knob");
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "one");
    EXPECT_EQ (textOf (document.readAsset ("fx/glow.frag")), "glow");

    // The load is in place and keeps the components: what read an asset is told, and reads the one of that folder
    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("second/ui.yui")).wasOk());
    EXPECT_EQ (document.findById ("knob"), knob);
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/glow.frag" }));
    EXPECT_EQ (recorder.texts, StringArray ({ "three", "another glow" }));

    // Whose files are the ones looked at from then on
    write ("first/fx/crt.frag", "of the folder before");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets.size(), 2);

    write ("second/fx/glow.frag", "seventeen");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/glow.frag", "fx/glow.frag" }));
    EXPECT_EQ (recorder.texts, StringArray ({ "three", "another glow", "seventeen" }));

    // What is told and doesn't read an asset again isn't told about it anymore
    recorder.readsAgain = false;
    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("first/ui.yui")).wasOk());
    EXPECT_EQ (recorder.assets.size(), 5);

    write ("first/fx/crt.frag", "one more");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets.size(), 5);
}

TEST_F (YuiLoaderTests, SwitchingBetweenResourcesAndAFileTellsWhatReadTheAssets)
{
    write ("fx/crt.frag", "from the folder");

    YuiDocument document;
    YuiLoader loader (document);
    AssetRecorder recorder (document);
    recorder.readsAgain = true;

    ASSERT_TRUE (loader.loadResource ("ui/panel.yui", &YuiLoaderTests::findPanelResource).wasOk());
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "void main() {}");

    // As when an interface embedded in the binary is loaded from its files to be edited live
    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_EQ (recorder.assets, StringArray ("fx/crt.frag"));
    EXPECT_EQ (recorder.texts, StringArray ("from the folder"));

    write ("fx/crt.frag", "edited");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/crt.frag" }));
    EXPECT_EQ (recorder.texts, StringArray ({ "from the folder", "edited" }));

    // And back to what is embedded
    ASSERT_TRUE (loader.loadResource ("ui/panel.yui", &YuiLoaderTests::findPanelResource).wasOk());
    EXPECT_EQ (recorder.assets.size(), 3);
    EXPECT_EQ (recorder.texts, StringArray ({ "from the folder", "edited", "void main() {}" }));
}

TEST_F (YuiLoaderTests, ReportsTheAssetsBeforeLoadingSoWhatALoadCreatesIsNotToldAboutThem)
{
    // Stands for what a load creates: it reads its asset while the document loads
    struct Reader final : YuiDocument::Listener
    {
        void documentLoaded (YuiDocument& document, const Result&) override
        {
            const auto bytes = document.readAsset ("fx/crt.frag");
            events.add ("loaded with " + (bytes.has_value() ? bytes->toString() : String()));
        }

        void documentAssetChanged (YuiDocument&, const String& path) override
        {
            events.add ("told about " + path);
        }

        StringArray events;
    };

    write ("fx/crt.frag", "one");

    YuiDocument document;
    YuiLoader loader (document);

    // Something read the asset before it changed
    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "one");

    Reader reader;
    document.addListener (&reader);

    write ("fx/crt.frag", "three");
    ASSERT_TRUE (loader.reload().wasOk());

    // What read it before is told first, then the document loads: what reads it then has it as it is
    EXPECT_EQ (reader.events, StringArray ({ "told about fx/crt.frag", "loaded with three" }));

    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (reader.events, StringArray ({ "told about fx/crt.frag", "loaded with three", "loaded with three" }));

    // Another source tells before it loads too, and what its load creates reads the asset as that source has it
    write ("other/ui.yui", "Component { Slider { id: knob; } }");
    write ("other/fx/crt.frag", "seven");
    reader.events.clear();

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("other/ui.yui")).wasOk());
    EXPECT_EQ (reader.events, StringArray ({ "told about fx/crt.frag", "loaded with seven" }));

    document.removeListener (&reader);
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

TEST_F (YuiLoaderTests, TellsTheDocumentWhenAWatchedAssetChangesThenLoadsItAgain)
{
    write ("fx/crt.frag", "one");
    write ("fx/glow.frag", "glow");

    YuiDocument document;
    YuiLoader loader (document);
    AssetRecorder recorder (document);

    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("ui.yui")).wasOk());

    // An asset is watched from the first time it is read
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "one");
    EXPECT_EQ (textOf (document.readAsset ("fx/glow.frag")), "glow");
    recorder.loads = 0;

    loader.setWatching (true);

    // Give the file system watcher time to start
    MessageManager::getInstance()->runDispatchLoopUntil (300);
    write ("fx/crt.frag", "three");

    for (int i = 0; i < 100 && recorder.assets.isEmpty(); ++i)
        MessageManager::getInstance()->runDispatchLoopUntil (50);

    // What read the asset is told, then the document is loaded again: what it lists as wrong is what still is
    EXPECT_EQ (recorder.assets, StringArray ("fx/crt.frag"));
    EXPECT_EQ (recorder.loads, 1);
    EXPECT_EQ (textOf (document.readAsset ("fx/crt.frag")), "three");

    // The file of an overridden asset isn't reported, the others still are
    loader.setOverride ("fx/crt.frag", "unsaved");
    ASSERT_TRUE (loader.reload().wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/crt.frag" }));

    recorder.assets.clear();
    recorder.loads = 0;
    write ("fx/crt.frag", "fourth!");
    write ("fx/glow.frag", "glow again");

    for (int i = 0; i < 100 && recorder.assets.isEmpty(); ++i)
        MessageManager::getInstance()->runDispatchLoopUntil (50);

    EXPECT_EQ (recorder.assets, StringArray ("fx/glow.frag"));
    EXPECT_EQ (recorder.loads, 1);

    // A text changed along with an asset reloads the document, which reports the asset as any reload does
    recorder.assets.clear();
    write ("fx/glow.frag", "glow once more");
    writeStyle (11);

    for (int i = 0; i < 100 && getTrackWidth (document) != 11.0; ++i)
        MessageManager::getInstance()->runDispatchLoopUntil (50);

    EXPECT_DOUBLE_EQ (getTrackWidth (document), 11.0);
    EXPECT_EQ (recorder.assets, StringArray ("fx/glow.frag"));

    // Another source tells what read the assets of the one before, and watches the ones read again from it
    recorder.assets.clear();
    recorder.readsAgain = true;
    write ("other.yui", "Component { Slider { id: knob; } }");
    ASSERT_TRUE (loader.loadFile (folder.getChildFile ("other.yui")).wasOk());
    EXPECT_EQ (recorder.assets, StringArray ({ "fx/crt.frag", "fx/glow.frag" }));
    EXPECT_EQ (recorder.texts, StringArray ({ "fourth!", "glow once more" }));

    recorder.assets.clear();
    recorder.loads = 0;
    write ("fx/glow.frag", "the last glow");

    for (int i = 0; i < 100 && recorder.assets.isEmpty(); ++i)
        MessageManager::getInstance()->runDispatchLoopUntil (50);

    EXPECT_EQ (recorder.assets, StringArray ("fx/glow.frag"));
    EXPECT_EQ (recorder.texts, StringArray ({ "fourth!", "glow once more", "the last glow" }));
    EXPECT_EQ (recorder.loads, 1);

    loader.setWatching (false);
}
#endif
