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
#include <YuiTestResources.h>

using namespace yup;

/** What yup_add_yui_resources embeds besides the documents, see YuiTestResources in tests/CMakeLists.txt: an asset as
    it is, and the bundle a fragment shader is compiled into when building.
*/
class YuiResourcesTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    /** The bytes embedded under a path, or nothing. */
    static std::optional<MemoryBlock> find (const char* path)
    {
        std::size_t size = 0;
        const auto* data = YuiTestResources_find (path, size);

        return data != nullptr ? std::optional<MemoryBlock> (MemoryBlock (data, size)) : std::nullopt;
    }

    /** The bytes of tests/data/yui/assets/blob.bin, which are no text. */
    static constexpr uint8 blobBytes[] = { 0x00, 0x01, 0x7f, 0x80, 0xff, 0xfe, 0x0d, 0x0a, 0x1a, 0xc3, 0x28, 0x00 };

    /** A document showing two images, found with its images as the embedded files are: no image is embedded with the tests. */
    static const uint8* findImages (const char* path, std::size_t& size)
    {
        static constexpr const char* panel = "Row { Image { id: dot; source: \"img/dot.png\"; } Image { id: icon; source: \"img/icon.svg\"; } }";
        static constexpr const char* icon = "<svg xmlns='http://www.w3.org/2000/svg' width='20' height='10'><rect width='20' height='10' fill='#ff0000'/></svg>";

        const String key (path);
        const char* text = key == "images.yui" ? panel : (key == "img/icon.svg" ? icon : nullptr);

        if (key == "img/dot.png")
        {
            size = sizeof (dotBytes);
            return dotBytes;
        }

        size = text != nullptr ? std::strlen (text) : 0;
        return reinterpret_cast<const uint8*> (text);
    }

    /** A 4 x 2 opaque red PNG. */
    static constexpr uint8 dotBytes[] = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x7f, 0xa8, 0x7d,
        0x63, 0x00, 0x00, 0x00, 0x12, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0xf8, 0xcf, 0xc0, 0xf0,
        0x1f, 0x19, 0x33, 0xa0, 0x0b, 0x00, 0x00, 0x0f, 0x21, 0x0f, 0xf1, 0xfe, 0x45, 0x14, 0x63, 0x00,
        0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
    };

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (YuiResourcesTests, FindsAnAssetWithTheBytesOfItsFile)
{
    const auto blob = find ("assets/blob.bin");

    ASSERT_TRUE (blob.has_value());
    EXPECT_TRUE (blob->matches (blobBytes, sizeof (blobBytes)));
}

TEST_F (YuiResourcesTests, FindsTheBundleAFragmentShaderIsCompiledIntoAndNotItsSource)
{
    const auto bundle = find ("fx/tint.frag.ysl");
    ASSERT_TRUE (bundle.has_value());

    const auto created = GpuImageShader::createFromData (bundle->getData(), bundle->getSize());
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    // The fragment stage that was given, behind the vertex stage of an image shader
    const auto shader = created.getValue();
    EXPECT_EQ (shader->getNumInputTextures(), 1);
    EXPECT_TRUE (shader->createUniforms().hasMember ("amount"));

    EXPECT_FALSE (find ("fx/tint.frag").has_value());
}

TEST_F (YuiResourcesTests, ADocumentRunsAShaderFromItsEmbeddedBundle)
{
    StringArray asked;

    const auto findRecording = [&asked] (const char* path, std::size_t& size)
    {
        asked.add (path);
        return YuiTestResources_find (path, size);
    };

    YuiDocument document;
    YuiLoader loader (document);

    const auto loaded = loader.loadResource ("effects.yui", findRecording);

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* display = document.findById ("display");
    ASSERT_NE (display, nullptr);

    auto* effect = dynamic_cast<ShaderEffect*> (display->getComponentEffect().get());
    ASSERT_NE (effect, nullptr);
    ASSERT_NE (effect->getShader(), nullptr);
    EXPECT_EQ (effect->getShader()->getNumInputTextures(), 1);

    // The bundle is all the shader is read from: its source is not embedded, and is not asked for
    EXPECT_TRUE (asked.contains ("fx/tint.frag.ysl"));
    EXPECT_FALSE (asked.contains ("fx/tint.frag"));
}

TEST_F (YuiResourcesTests, ADocumentShowsTheImagesFoundWithItAndTheTextOverridingOne)
{
    YuiDocument document;
    YuiLoader loader (document);

    const auto loaded = loader.loadResource ("images.yui", findImages);

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* dot = document.findById<ImageComponent> ("dot");
    auto* icon = document.findById<ImageComponent> ("icon");
    ASSERT_NE (dot, nullptr);
    ASSERT_NE (icon, nullptr);

    // Neither is a file: the working directory has no such folder
    ASSERT_TRUE (dot->getImage().isValid());
    EXPECT_EQ (dot->getImage().getWidth(), 4);

    const auto shown = icon->getDrawable();
    ASSERT_NE (shown, nullptr);

    // The unsaved text of an editor is what the image shows once the loader reloads
    loader.setOverride ("img/icon.svg", "<svg xmlns='http://www.w3.org/2000/svg' width='30' height='10'><circle cx='15' cy='5' r='5' fill='#0000ff'/></svg>");
    ASSERT_TRUE (loader.reload().wasOk());

    EXPECT_EQ (document.findById<ImageComponent> ("icon"), icon);
    ASSERT_NE (icon->getDrawable(), nullptr);
    EXPECT_NE (icon->getDrawable(), shown);
    EXPECT_TRUE (dot->getImage().isValid());

    // A text that is no SVG is listed where the image is written, and no longer is once it is one again
    loader.setOverride ("img/icon.svg", "not a picture");
    ASSERT_TRUE (loader.reload().wasOk());

    EXPECT_EQ (icon->getDrawable(), nullptr);
    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_TRUE (document.getDiagnostics()[0].message.startsWith ("Image.source: ")) << document.getDiagnostics()[0].message;
    EXPECT_EQ (document.getDiagnostics()[0].line, 1);

    loader.clearOverrides();
    ASSERT_TRUE (loader.reload().wasOk());

    EXPECT_NE (icon->getDrawable(), nullptr);
    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

#endif
