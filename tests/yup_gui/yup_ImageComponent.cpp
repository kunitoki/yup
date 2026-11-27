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

using namespace yup;

class ImageComponentTests : public ::testing::Test
{
protected:
    struct RecordingListener : public ComponentListener
    {
        void componentPropertyChanged (Component& component, const Identifier& name) override
        {
            ignoreUnused (component);
            names.add (name.toString());
        }

        StringArray names;
    };

    void SetUp() override
    {
        folder = File::getSpecialLocation (File::tempDirectory)
                     .getChildFile ("YUP_ImageComponentTests_" + String::toHexString (Random::getSystemRandom().nextInt()));

        folder.deleteRecursively();
        folder.createDirectory();

        folder.getChildFile ("logo.svg").replaceWithText (svgText());
        folder.getChildFile ("pixels.png").replaceWithData (pngData, sizeof (pngData));
    }

    void TearDown() override
    {
        if (themeReplaced)
            ApplicationTheme::setGlobalTheme (oldTheme.get());

        folder.deleteRecursively();
    }

    /** Makes a theme the global one until the test ends: an image is painted by the theme. */
    void useTheme (ApplicationTheme::Ptr theme)
    {
        if (! themeReplaced)
            oldTheme = ApplicationTheme::getGlobalTheme();

        themeReplaced = true;
        ApplicationTheme::setGlobalTheme (std::move (theme));
    }

    static String svgText()
    {
        return R"(<svg xmlns="http://www.w3.org/2000/svg" width="20" height="10"><rect width="20" height="10" fill="#ff0000"/></svg>)";
    }

    static String pngDataUri()
    {
        return "data:image/png;base64," + Base64::toBase64 (pngData, sizeof (pngData));
    }

    static MemoryBlock bytesOf (const String& text)
    {
        return MemoryBlock (text.toRawUTF8(), text.getNumBytesAsUTF8());
    }

    /** A document of one root: it serves the assets it is given, then the files of its folder when it has one, and
        keeps what it is asked for and what it is told.
    */
    struct FakeDocument final : ComponentDocument
    {
        explicit FakeDocument (Component& rootToUse)
            : root (rootToUse)
        {
            attachToRoot();
        }

        ~FakeDocument() override
        {
            detachFromRoot();
        }

        Component* getRoot() const override { return &root; }

        const File& getBaseDirectory() const override { return baseDirectory; }

        std::optional<MemoryBlock> readAsset (const String& path) const override
        {
            read.add (path);

            if (const auto found = assets.find (path); found != assets.end())
                return found->second;

            if (baseDirectory != File())
                return readAssetFile (resolveAssetFile (path, baseDirectory));

            return std::nullopt;
        }

        File getAssetFile (const String& path) const override
        {
            return baseDirectory != File() ? resolveAssetFile (path, baseDirectory) : File();
        }

        std::shared_ptr<const DataTreeSourceMap> getSourceMap() const override { return nullptr; }

        DrawingScope::ComponentResolver getComponentResolver() const override { return nullptr; }

        void reportDiagnostic (DataTreeDiagnostic diagnostic, const Component* subject = nullptr) override
        {
            problems.add (diagnostic.message + " [" + diagnostic.property.toString() + "]");
            subjects.push_back (subject);
        }

        using ComponentDocument::notifyDocumentAssetChanged;
        using ComponentDocument::notifyDocumentLoaded;

        Component& root;
        File baseDirectory;
        std::map<String, MemoryBlock> assets;
        mutable StringArray read;
        StringArray problems;
        std::vector<const Component*> subjects;
    };

    // A 4 x 2 opaque red RGBA image
    static constexpr uint8 pngData[] = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x7f, 0xa8, 0x7d,
        0x63, 0x00, 0x00, 0x00, 0x12, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0xf8, 0xcf, 0xc0, 0xf0,
        0x1f, 0x19, 0x33, 0xa0, 0x0b, 0x00, 0x00, 0x0f, 0x21, 0x0f, 0xf1, 0xfe, 0x45, 0x14, 0x63, 0x00,
        0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
    };

    ImageComponent image;
    File folder;
    ApplicationTheme::Ptr oldTheme;
    bool themeReplaced = false;
};

TEST_F (ImageComponentTests, StartsEmptyAndTransparent)
{
    EXPECT_FALSE (image.isOpaque());
    EXPECT_FALSE (image.getImage().isValid());
    EXPECT_EQ (image.getDrawable(), nullptr);
    EXPECT_TRUE (image.getSource().isEmpty());
    EXPECT_EQ (image.getFitting(), Fitting::scaleToFit);
    EXPECT_EQ (image.getJustification(), Justification::center);
    EXPECT_FLOAT_EQ (image.getImageOpacity(), 1.0f);

    EXPECT_EQ (ImageComponent ("backdrop").getComponentID(), "backdrop");
}

TEST_F (ImageComponentTests, IsTheImageTypeWithItsProperties)
{
    const auto& info = image.getTypeInfo();

    EXPECT_EQ (info.getTypeName(), Identifier ("Image"));
    EXPECT_TRUE (info.hasFactory());

    for (const auto* name : { "source", "fitting", "justification", "imageOpacity" })
        EXPECT_TRUE (info.hasProperty (Identifier (name))) << name;
}

TEST_F (ImageComponentTests, SetsTheFittingAndTheJustificationByName)
{
    image.setProperty (Identifier ("fitting"), "centerCrop");
    EXPECT_EQ (image.getFitting(), Fitting::centerCrop);
    EXPECT_EQ (image.getProperty (Identifier ("fitting")).toString(), "centerCrop");

    image.setProperty (Identifier ("justification"), "bottomRight");
    EXPECT_EQ (image.getJustification(), Justification::bottomRight);
    EXPECT_EQ (image.getProperty (Identifier ("justification")).toString(), "bottomRight");
}

TEST_F (ImageComponentTests, ClampsTheImageOpacity)
{
    image.setImageOpacity (-1.0f);
    EXPECT_FLOAT_EQ (image.getImageOpacity(), 0.0f);

    image.setImageOpacity (2.0f);
    EXPECT_FLOAT_EQ (image.getImageOpacity(), 1.0f);

    image.setProperty (Identifier ("imageOpacity"), 0.25);
    EXPECT_FLOAT_EQ (image.getImageOpacity(), 0.25f);
}

TEST_F (ImageComponentTests, ReportsThePropertiesThatChange)
{
    RecordingListener listener;
    image.addComponentListener (&listener);

    image.setFitting (Fitting::scaleToFit);
    image.setJustification (Justification::center);
    image.setImageOpacity (1.0f);
    image.setSource ({});
    EXPECT_TRUE (listener.names.isEmpty());

    image.setFitting (Fitting::fill);
    image.setJustification (Justification::topLeft);
    image.setImageOpacity (0.5f);
    image.setSource (svgText());
    EXPECT_EQ (listener.names, StringArray ({ "fitting", "justification", "imageOpacity", "source" }));

    image.removeComponentListener (&listener);
}

TEST_F (ImageComponentTests, LoadsSvgTextAsADrawable)
{
    image.setSource (svgText());

    EXPECT_EQ (image.getSource(), svgText());
    EXPECT_NE (image.getDrawable(), nullptr);
    EXPECT_FALSE (image.getImage().isValid());
}

TEST_F (ImageComponentTests, LoadsADataUriAsAnImage)
{
    image.setSource (pngDataUri());

    EXPECT_EQ (image.getDrawable(), nullptr);
    ASSERT_TRUE (image.getImage().isValid());
    EXPECT_EQ (image.getImage().getWidth(), 4);
    EXPECT_EQ (image.getImage().getHeight(), 2);
}

TEST_F (ImageComponentTests, LoadsFilesRelativeToTheFolderOfItsDocument)
{
    Component root;
    root.addChildComponent (image);

    FakeDocument document (root);
    document.baseDirectory = folder;

    image.setSource ("logo.svg");
    EXPECT_NE (image.getDrawable(), nullptr);
    EXPECT_FALSE (image.getImage().isValid());

    image.setSource ("pixels.png");
    EXPECT_EQ (image.getDrawable(), nullptr);
    ASSERT_TRUE (image.getImage().isValid());
    EXPECT_EQ (image.getImage().getWidth(), 4);
    EXPECT_EQ (image.getImage().getHeight(), 2);

    EXPECT_TRUE (document.problems.isEmpty()) << document.problems.joinIntoString ("\n");
}

TEST_F (ImageComponentTests, LoadsAnAbsolutePathInNoDocument)
{
    image.setSource (folder.getChildFile ("logo.svg").getFullPathName());
    EXPECT_NE (image.getDrawable(), nullptr);

    image.setSource (folder.getChildFile ("pixels.png").getFullPathName());
    EXPECT_TRUE (image.getImage().isValid());

    // An SVG file is read as any text is: its byte order mark tells how it is encoded
    const auto wide = folder.getChildFile ("wide.svg");
    ASSERT_TRUE (wide.replaceWithText (svgText(), true, true));

    image.setSource (wide.getFullPathName());
    EXPECT_NE (image.getDrawable(), nullptr);
}

TEST_F (ImageComponentTests, ShowsTheAssetItsDocumentGivesItAndNotAFile)
{
    Component root;
    root.addChildComponent (image);

    FakeDocument document (root);
    document.assets["memory/pixels.png"] = MemoryBlock (pngData, sizeof (pngData));
    document.assets["memory/logo.svg"] = bytesOf (svgText());

    image.setSource ("memory/pixels.png");
    EXPECT_EQ (image.getDrawable(), nullptr);
    ASSERT_TRUE (image.getImage().isValid());
    EXPECT_EQ (image.getImage().getWidth(), 4);
    EXPECT_EQ (image.getImage().getHeight(), 2);

    image.setSource ("  memory/logo.svg ");
    EXPECT_NE (image.getDrawable(), nullptr);
    EXPECT_FALSE (image.getImage().isValid());

    // Each is asked for once, by the path that was written without the spaces around it
    EXPECT_EQ (document.read, StringArray ({ "memory/pixels.png", "memory/logo.svg" }));
    EXPECT_TRUE (document.problems.isEmpty()) << document.problems.joinIntoString ("\n");
}

TEST_F (ImageComponentTests, TellsItsDocumentAboutASourceItCantLoadAndAgainAfterEachLoad)
{
    Component root;
    root.addChildComponent (image);

    FakeDocument document (root);
    document.assets["memory/pixels.png"] = MemoryBlock (pngData, sizeof (pngData));
    document.assets["memory/notes.png"] = bytesOf ("not a picture");

    image.setSource ("memory/missing.png");
    EXPECT_FALSE (image.getImage().isValid());
    EXPECT_EQ (document.problems, StringArray ({ "Image.source: the file 'memory/missing.png' can't be read [source]" }));
    ASSERT_EQ (document.subjects.size(), 1u);
    EXPECT_EQ (document.subjects.front(), &image);

    // A load forgets what it was told: the image tells it again
    document.problems.clear();
    document.notifyDocumentLoaded();
    EXPECT_EQ (document.problems, StringArray ({ "Image.source: the file 'memory/missing.png' can't be read [source]" }));

    // A file that is read and is no image is told too
    document.problems.clear();
    image.setSource ("memory/notes.png");
    EXPECT_FALSE (image.getImage().isValid());
    ASSERT_EQ (document.problems.size(), 1);
    EXPECT_TRUE (document.problems[0].startsWith ("Image.source: ")) << document.problems[0];
    EXPECT_TRUE (document.problems[0].endsWith (" [source]")) << document.problems[0];

    // A source that loads leaves nothing to tell
    document.problems.clear();
    image.setSource ("memory/pixels.png");
    document.notifyDocumentLoaded();
    EXPECT_TRUE (image.getImage().isValid());
    EXPECT_TRUE (document.problems.isEmpty()) << document.problems.joinIntoString ("\n");
}

TEST_F (ImageComponentTests, ReadsItsSourceAgainWhenItsDocumentTellsThatTheAssetChanged)
{
    Component root;
    root.addChildComponent (image);

    auto document = std::make_unique<FakeDocument> (root);

    image.setSource ("memory/pixels.png");
    EXPECT_FALSE (image.getImage().isValid());
    EXPECT_EQ (document->problems.size(), 1);

    // Another asset changing reads nothing again
    document->assets["memory/pixels.png"] = MemoryBlock (pngData, sizeof (pngData));
    document->notifyDocumentAssetChanged ("memory/other.png");
    EXPECT_FALSE (image.getImage().isValid());
    EXPECT_EQ (document->read.size(), 1);

    document->notifyDocumentAssetChanged ("memory/pixels.png");
    ASSERT_TRUE (image.getImage().isValid());
    EXPECT_EQ (image.getImage().getWidth(), 4);
    EXPECT_EQ (document->read.size(), 2);
    EXPECT_EQ (document->problems.size(), 1);

    // An asset that went is not shown anymore, and that is told
    document->assets.clear();
    document->notifyDocumentAssetChanged ("memory/pixels.png");
    EXPECT_FALSE (image.getImage().isValid());
    EXPECT_EQ (document->problems.size(), 2);

    // The image outlives its document, and reads its files by itself from then on
    document.reset();
    image.setSource (folder.getChildFile ("pixels.png").getFullPathName());
    EXPECT_TRUE (image.getImage().isValid());
}

TEST_F (ImageComponentTests, ShowsNothingForASourceThatCantBeLoaded)
{
    image.setSource (folder.getChildFile ("logo.svg").getFullPathName());
    ASSERT_NE (image.getDrawable(), nullptr);

    image.setSource ("missing.png");
    EXPECT_EQ (image.getDrawable(), nullptr);
    EXPECT_FALSE (image.getImage().isValid());

    image.setSource (pngDataUri());
    ASSERT_TRUE (image.getImage().isValid());

    image.setSource ("data:image/png;base64,AAAA");
    EXPECT_FALSE (image.getImage().isValid());

    image.setSource ("data:image/png,pixels");
    EXPECT_FALSE (image.getImage().isValid());

    image.setSource (pngDataUri());
    ASSERT_TRUE (image.getImage().isValid());

    image.setSource ("   ");
    EXPECT_FALSE (image.getImage().isValid());
}

TEST_F (ImageComponentTests, ShowsEitherAnImageOrADrawable)
{
    const auto drawable = std::make_shared<Drawable>();

    image.setImage (Image (4, 2));
    EXPECT_TRUE (image.getImage().isValid());

    image.setDrawable (drawable);
    EXPECT_EQ (image.getDrawable(), drawable);
    EXPECT_FALSE (image.getImage().isValid());

    image.setImage (Image (4, 2));
    EXPECT_EQ (image.getDrawable(), nullptr);
    EXPECT_TRUE (image.getImage().isValid());
}

TEST_F (ImageComponentTests, IsPaintedByTheStyleOfTheThemeOrByItsOwn)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (64, 64);
    Graphics g (*context, *renderer);

    image.setBounds (0, 0, 64, 64);
    image.setImage (Image (4, 2));

    ApplicationTheme::Ptr theme = new ApplicationTheme();
    useTheme (theme);

    int numThemePaints = 0, numOwnPaints = 0;
    const ImageComponent* painted = nullptr;

    theme->setComponentStyle<ImageComponent> (ComponentStyle::createStyle<ImageComponent> ([&] (Graphics&, const ApplicationTheme& paintingTheme, const ImageComponent& c)
    {
        EXPECT_EQ (std::addressof (paintingTheme), theme.get());

        ++numThemePaints;
        painted = std::addressof (c);
    }));

    image.paint (g);
    EXPECT_EQ (numThemePaints, 1);
    EXPECT_EQ (painted, std::addressof (image));

    image.setStyle (ComponentStyle::createStyle<ImageComponent> ([&numOwnPaints] (Graphics&, const ApplicationTheme&, const ImageComponent&)
    {
        ++numOwnPaints;
    }));

    image.paint (g);
    EXPECT_EQ (numThemePaints, 1);
    EXPECT_EQ (numOwnPaints, 1);
}

TEST_F (ImageComponentTests, PaintingLeavesTheGraphicsStateUntouched)
{
    useTheme (createThemeVersion1());

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (64, 64);
    Graphics g (*context, *renderer);

    image.setBounds (0, 0, 64, 64);
    image.setImageOpacity (0.5f);
    image.setImage (Image (4, 2));

    for (const auto fitting : { Fitting::scaleToFit, Fitting::none, Fitting::fitWidth, Fitting::fitHeight, Fitting::scaleToFill, Fitting::fill, Fitting::tile, Fitting::centerCrop, Fitting::centerInside, Fitting::stretchWidth, Fitting::stretchHeight })
    {
        for (const auto justification : { Justification::topLeft, Justification::center, Justification::bottomRight })
        {
            image.setFitting (fitting);
            image.setJustification (justification);
            image.paint (g);

            EXPECT_FLOAT_EQ (g.getOpacity(), 1.0f);
        }
    }

    image.setSource (svgText());
    ASSERT_NE (image.getDrawable(), nullptr);
    image.paint (g);
    EXPECT_FLOAT_EQ (g.getOpacity(), 1.0f);

    image.setImageOpacity (0.0f);
    image.paint (g);

    image.setImageOpacity (1.0f);
    image.setBounds (0, 0, 0, 0);
    image.paint (g);

    image.setDrawable (nullptr);
    image.setBounds (0, 0, 64, 64);
    image.paint (g);

    EXPECT_FLOAT_EQ (g.getOpacity(), 1.0f);
}

TEST_F (ImageComponentTests, TheThemeTilesTheImageFromTheCornerOfItsBounds)
{
    const auto gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});
    if (gpuContext == nullptr || ! gpuContext->isGpuAvailable())
        GTEST_SKIP() << "No Metal GPU context available";

    useTheme (createThemeVersion1());

    // Red on its left half, blue on its right one
    Image tile (4, 4);
    tile.fillColor (Color (0xff0000ff));

    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 2; ++x)
            tile.setPixelColor (x, y, Color (0xffff0000));

    image.setBounds (0, 0, 16, 16);
    image.setFitting (Fitting::tile);
    image.setImage (tile);

    const auto painted = image.snapshotToImage (*gpuContext);
    ASSERT_TRUE (painted.isValid());

    EXPECT_EQ (painted.getPixelColor (1, 8), Color (0xffff0000));
    EXPECT_EQ (painted.getPixelColor (2, 8), Color (0xff0000ff));
    EXPECT_EQ (painted.getPixelColor (5, 8), Color (0xffff0000));
    EXPECT_EQ (painted.getPixelColor (15, 15), Color (0xff0000ff));
}
