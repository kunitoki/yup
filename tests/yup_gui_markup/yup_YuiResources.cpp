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
    EXPECT_TRUE (document.getDiagnostics().empty()) << YuiDiagnostic::join (document.getDiagnostics());

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

#endif
