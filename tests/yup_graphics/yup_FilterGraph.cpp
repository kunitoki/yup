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

#include <gtest/gtest.h>

#include <yup_graphics/yup_graphics.h>

using namespace yup;

//==============================================================================
/** Draws with the platform's GPU context, through GraphicsGpuPixelTestHost (see native/). */
class FilterGraphTests : public ::testing::Test
{
protected:
    /** A red square of 16 in the middle of a content box of 64, counting how often it is drawn. */
    struct SquareSource : PaintSource
    {
        Size<float> getContentSize() const override { return { 64.0f, 64.0f }; }

        void paintContent (Graphics& g) override
        {
            ++paintCount;

            g.setFillColor (Colors::red);
            g.fillRect (24.0f, 24.0f, 16.0f, 16.0f);
        }

        int paintCount = 0;
    };

    static void SetUpTestSuite()
    {
        host = std::make_unique<GraphicsGpuPixelTestHost>();
        gpuContext = host->getContext();
    }

    static void TearDownTestSuite()
    {
        gpuContext = nullptr;
        host.reset();
    }

    void SetUp() override
    {
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No GPU graphics context available";

        host->makeCurrent();
    }

    void TearDown() override
    {
        source.releaseResources();
    }

    /** Draws the source over the whole canvas through a filter, a texel for each of its units, returning the RGBA pixels. */
    std::vector<uint8> render (std::vector<FilterGraph::Primitive> primitives, bool linearColors = false)
    {
        FilterGraph filter;
        filter.primitives = std::move (primitives);
        filter.linearColors = linearColors;
        source.setFilter (std::move (filter));

        auto canvas = GpuCanvas::create (*gpuContext, size, size);
        if (canvas == nullptr)
            return {};

        const bool drawn = canvas->beginDraw().drawSource (source, { 0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size) });

        std::vector<uint8> pixels (static_cast<std::size_t> (size * size * 4));
        if (! drawn || ! canvas->commit() || ! canvas->readPixels (pixels.data(), pixels.size()))
            return {};

        return pixels;
    }

    static int channelAt (const std::vector<uint8>& pixels, int x, int y, int channel)
    {
        return pixels[static_cast<std::size_t> ((y * size + x) * 4 + channel)];
    }

    static int alphaAt (const std::vector<uint8>& pixels, int x, int y)
    {
        return channelAt (pixels, x, y, 3);
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    static GpuImageShader::Ptr compileShader (const char* fragment)
    {
        const auto created = GpuImageShader::createFromGlsl (fragment);
        EXPECT_TRUE (created.wasOk()) << created.getErrorMessage();
        return created.wasOk() ? created.getValue() : nullptr;
    }

    /** The picture it reads, with its red and its blue swapped. */
    static constexpr const char* swapFragment = R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    vec4 pixel = texture (sampler2D (source, sourceSampler), uv);
    color = vec4 (pixel.b, pixel.g, pixel.r, pixel.a);
})";

    /** The red of the first picture it reads with the green of the second one, opaque. */
    static constexpr const char* mixFragment = R"(#version 450
layout(set = 0, binding = 0) uniform texture2D base;
layout(set = 0, binding = 1) uniform texture2D overlay;
layout(set = 0, binding = 2) uniform sampler linearClamp;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = vec4 (texture (sampler2D (base, linearClamp), uv).r, texture (sampler2D (overlay, linearClamp), uv).g, 0.0, 1.0);
})";

    /** Its color everywhere, reading no picture. */
    static constexpr const char* fillFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;
} params;
layout(location = 0) out vec4 color;
void main()
{
    color = params.tint;
})";
#endif

    static constexpr int size = 64;
    static std::unique_ptr<GraphicsGpuPixelTestHost> host;
    static GraphicsContext* gpuContext;

    SquareSource source;
};

std::unique_ptr<GraphicsGpuPixelTestHost> FilterGraphTests::host;
GraphicsContext* FilterGraphTests::gpuContext = nullptr;

//==============================================================================
TEST (FilterGraphMatrixTests, SaturationAndHueMatricesLeaveColorsAtTheirNeutralValues)
{
    const FilterGraph::Primitive identity;

    for (const auto& matrix : { FilterGraph::saturationMatrix (1.0f), FilterGraph::hueRotationMatrix (0.0f) })
        for (size_t i = 0; i < matrix.size(); ++i)
            EXPECT_NEAR (matrix[i], identity.matrix[i], 0.001f) << "at " << i;

    // Without saturation every channel is the luminance
    const auto gray = FilterGraph::saturationMatrix (0.0f);
    for (size_t row = 0; row < 3; ++row)
        EXPECT_NEAR (gray[row * 5] + gray[row * 5 + 1] + gray[row * 5 + 2], 1.0f, 0.001f);

    EXPECT_FLOAT_EQ (gray[0], gray[5]);
    EXPECT_FLOAT_EQ (gray[1], gray[11]);

    const auto alpha = FilterGraph::luminanceToAlphaMatrix();
    EXPECT_NEAR (alpha[15] + alpha[16] + alpha[17], 1.0f, 0.001f);
    EXPECT_FLOAT_EQ (alpha[0] + alpha[6] + alpha[12] + alpha[18], 0.0f);
}

TEST_F (FilterGraphTests, SourceIsDrawnAsItIsWithoutAFilter)
{
    const auto pixels = render ({});
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (channelAt (pixels, 32, 32, 0), 200);
    EXPECT_GT (alphaAt (pixels, 32, 32), 200);
    EXPECT_LT (alphaAt (pixels, 8, 8), 10);
    EXPECT_LT (alphaAt (pixels, 46, 32), 10);
}

TEST_F (FilterGraphTests, OffsetMovesThePicture)
{
    const auto pixels = render ({ { .type = FilterGraph::Type::offset, .offset = { 16.0f, -8.0f } } });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (alphaAt (pixels, 48, 24), 200);
    EXPECT_LT (alphaAt (pixels, 28, 36), 10);
}

TEST_F (FilterGraphTests, BlurSpreadsThePictureAndKeepsItsMiddle)
{
    const auto pixels = render ({ { .type = FilterGraph::Type::blur, .deviation = { 3.0f, 3.0f } } });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (alphaAt (pixels, 32, 32), 200);

    // Half of what a pixel on the edge sees is the square, and three pixels out some of it still is
    EXPECT_NEAR (alphaAt (pixels, 24, 32), 128, 40);
    EXPECT_GT (alphaAt (pixels, 20, 32), 10);
    EXPECT_LT (alphaAt (pixels, 20, 32), 120);
    EXPECT_LT (alphaAt (pixels, 6, 32), 5);
}

TEST_F (FilterGraphTests, PrimitivesReadTheSourceAndTheResultsTheyName)
{
    // Green where the square is, made of a flood kept inside the alpha of the source
    const auto pixels = render ({ { .type = FilterGraph::Type::flood, .result = "green", .color = Color (0xff00ff00) },
                                  { .type = FilterGraph::Type::blur, .in = FilterGraph::sourceGraphic, .deviation = { 4.0f, 4.0f } },
                                  { .type = FilterGraph::Type::composite, .in = "green", .in2 = FilterGraph::sourceAlpha, .compositeOperator = FilterGraph::CompositeOperator::in } });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_LT (channelAt (pixels, 32, 32, 0), 10);
    EXPECT_GT (channelAt (pixels, 32, 32, 1), 200);
    EXPECT_GT (alphaAt (pixels, 32, 32), 200);

    // The blur nothing reads leaves no trace around the square
    EXPECT_LT (alphaAt (pixels, 20, 32), 5);
}

TEST_F (FilterGraphTests, CompositeOperatorsAndBlendModesCombineTwoPictures)
{
    const FilterGraph::Primitive moved { .type = FilterGraph::Type::offset, .in = FilterGraph::sourceGraphic, .result = "moved", .offset = { 8.0f, 0.0f } };

    // The square without what its copy moved to the right covers
    const auto out = render ({ moved, { .type = FilterGraph::Type::composite, .in = FilterGraph::sourceGraphic, .in2 = "moved", .compositeOperator = FilterGraph::CompositeOperator::out } });
    ASSERT_EQ (out.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_GT (alphaAt (out, 28, 32), 200);
    EXPECT_LT (alphaAt (out, 36, 32), 10);
    EXPECT_LT (alphaAt (out, 44, 32), 10);

    // A blue flood multiplied with the red square is black where they meet, and blue around it
    const auto multiplied = render ({ { .type = FilterGraph::Type::flood, .result = "blue", .color = Color (0xff0000ff) },
                                      { .type = FilterGraph::Type::blend, .in = FilterGraph::sourceGraphic, .in2 = "blue", .blendMode = BlendMode::Multiply } });
    ASSERT_EQ (multiplied.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_LT (channelAt (multiplied, 32, 32, 0), 10);
    EXPECT_LT (channelAt (multiplied, 32, 32, 2), 10);
    EXPECT_GT (alphaAt (multiplied, 32, 32), 200);
    EXPECT_GT (channelAt (multiplied, 8, 8, 2), 200);
}

TEST_F (FilterGraphTests, ColorMatrixChangesTheColors)
{
    FilterGraph::Primitive swap { .type = FilterGraph::Type::colorMatrix };
    swap.matrix = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f };

    const auto pixels = render ({ swap });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    // Red becomes blue at half the alpha, which the pixels are premultiplied by
    EXPECT_LT (channelAt (pixels, 32, 32, 0), 10);
    EXPECT_NEAR (channelAt (pixels, 32, 32, 2), 128, 8);
    EXPECT_NEAR (alphaAt (pixels, 32, 32), 128, 8);
}

TEST_F (FilterGraphTests, MorphologyThinsAndFattens)
{
    const auto thinner = render ({ { .type = FilterGraph::Type::morphology, .radius = { 4.0f, 4.0f } } });
    ASSERT_EQ (thinner.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_GT (alphaAt (thinner, 32, 32), 200);
    EXPECT_LT (alphaAt (thinner, 26, 32), 10);

    const auto fatter = render ({ { .type = FilterGraph::Type::morphology, .radius = { 4.0f, 4.0f }, .dilate = true } });
    ASSERT_EQ (fatter.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_GT (alphaAt (fatter, 22, 32), 200);
    EXPECT_LT (alphaAt (fatter, 16, 32), 10);
}

TEST_F (FilterGraphTests, DropShadowAndMergePutPicturesOverEachOther)
{
    const auto shadowed = render ({ { .type = FilterGraph::Type::dropShadow, .offset = { 12.0f, 12.0f }, .color = Color (0xff0000ff) } });
    ASSERT_EQ (shadowed.size(), static_cast<std::size_t> (size * size * 4));

    // The square is over its blue shadow, which shows below and to the right of it
    EXPECT_GT (channelAt (shadowed, 32, 32, 0), 200);
    EXPECT_LT (channelAt (shadowed, 38, 38, 2), 10);
    EXPECT_GT (channelAt (shadowed, 48, 48, 2), 200);
    EXPECT_LT (alphaAt (shadowed, 16, 16), 10);

    FilterGraph::Primitive merge { .type = FilterGraph::Type::merge };
    merge.inputs = { "green", FilterGraph::sourceGraphic };

    const auto merged = render ({ { .type = FilterGraph::Type::flood, .result = "green", .color = Color (0xff00ff00) }, merge });
    ASSERT_EQ (merged.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_GT (channelAt (merged, 32, 32, 0), 200);
    EXPECT_LT (channelAt (merged, 32, 32, 1), 10);
    EXPECT_GT (channelAt (merged, 8, 8, 1), 200);
}

TEST_F (FilterGraphTests, LightingShadesTheSlopesOfTheAlpha)
{
    // Lit from the left, the left slope of the blurred square is brighter than the flat around it, and the right one darker
    FilterGraph::Primitive light { .type = FilterGraph::Type::lighting };
    light.scale = 8.0f;
    light.color = Colors::white;
    light.lightAzimuth = MathConstants<float>::pi;
    light.lightElevation = MathConstants<float>::pi / 4.0f;

    const auto pixels = render ({ { .type = FilterGraph::Type::blur, .deviation = { 2.0f, 2.0f } }, light });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    const int flat = channelAt (pixels, 8, 8, 0);
    EXPECT_NEAR (flat, 180, 12);
    EXPECT_GT (channelAt (pixels, 24, 32, 0), flat + 20);
    EXPECT_LT (channelAt (pixels, 40, 32, 0), flat - 20);
    EXPECT_EQ (alphaAt (pixels, 8, 8), 255);
}

TEST_F (FilterGraphTests, LinearColorsLeaveFlatColorsAsTheyAre)
{
    const auto pixels = render ({ { .type = FilterGraph::Type::flood, .color = Color (0xff808080) } }, true);
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_NEAR (channelAt (pixels, 8, 8, 0), 128, 3);
    EXPECT_EQ (alphaAt (pixels, 8, 8), 255);
}

TEST_F (FilterGraphTests, ChangingTheFilterKeepsTheContentDrawn)
{
    render ({ { .type = FilterGraph::Type::offset, .offset = { 4.0f, 0.0f } } });
    render ({ { .type = FilterGraph::Type::offset, .offset = { 4.0f, 0.0f } } });
    const auto pixels = render ({ { .type = FilterGraph::Type::offset, .offset = { -16.0f, 0.0f } } });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_EQ (source.paintCount, 1);
    EXPECT_GT (alphaAt (pixels, 12, 32), 200);
    EXPECT_LT (alphaAt (pixels, 36, 32), 10);
}

TEST_F (FilterGraphTests, ShaderThatCantBeDrawnLeavesThePictureAsItIs)
{
    // With no shader to draw the filter fails as a whole, whatever its other primitives do
    const auto pixels = render ({ { .type = FilterGraph::Type::offset, .result = "moved", .offset = { 16.0f, 0.0f } },
                                  { .type = FilterGraph::Type::shader, .in = "moved" } });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (channelAt (pixels, 32, 32, 0), 200);
    EXPECT_GT (alphaAt (pixels, 32, 32), 200);
    EXPECT_LT (alphaAt (pixels, 46, 32), 10);
}

#if YUP_ENABLE_SHADER_TRANSPILER

TEST_F (FilterGraphTests, ShaderRunsOnThePictureItReads)
{
    const auto swap = compileShader (swapFragment);
    ASSERT_NE (swap, nullptr);
    EXPECT_EQ (swap->getNumInputTextures(), 1);

    // The red square becomes blue where the primitive before left it, up and to the right: each pixel reads its own
    const auto pixels = render ({ { .type = FilterGraph::Type::offset, .offset = { 16.0f, -8.0f } },
                                  { .type = FilterGraph::Type::shader, .shader = swap, .uniforms = swap->createUniforms() } });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_LT (channelAt (pixels, 48, 20, 0), 10);
    EXPECT_GT (channelAt (pixels, 48, 20, 2), 200);
    EXPECT_GT (alphaAt (pixels, 48, 20), 200);
    EXPECT_LT (alphaAt (pixels, 48, 44), 10);
    EXPECT_LT (alphaAt (pixels, 16, 20), 10);
}

TEST_F (FilterGraphTests, ShaderCombinesTheTwoPicturesItReads)
{
    const auto mix = compileShader (mixFragment);
    ASSERT_NE (mix, nullptr);
    EXPECT_EQ (mix->getNumInputTextures(), 2);

    const FilterGraph::Primitive green { .type = FilterGraph::Type::flood, .result = "green", .color = Color (0xff00ff00) };

    // The red of the square with the green of the flood where the square is, and the green alone around it
    const auto pixels = render ({ green, { .type = FilterGraph::Type::shader, .in = FilterGraph::sourceGraphic, .in2 = "green", .shader = mix, .uniforms = mix->createUniforms() } });
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (channelAt (pixels, 32, 32, 0), 200);
    EXPECT_GT (channelAt (pixels, 32, 32, 1), 200);
    EXPECT_LT (channelAt (pixels, 8, 8, 0), 10);
    EXPECT_GT (channelAt (pixels, 8, 8, 1), 200);
    EXPECT_EQ (alphaAt (pixels, 8, 8), 255);

    // The first texture reads `in` and the second one `in2`
    const auto swapped = render ({ green, { .type = FilterGraph::Type::shader, .in = "green", .in2 = FilterGraph::sourceGraphic, .shader = mix, .uniforms = mix->createUniforms() } });
    ASSERT_EQ (swapped.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_LT (channelAt (swapped, 32, 32, 0), 10);
    EXPECT_LT (channelAt (swapped, 32, 32, 1), 10);
    EXPECT_EQ (alphaAt (swapped, 32, 32), 255);
}

TEST_F (FilterGraphTests, ShaderReadingNoPictureMakesItsOwnFromItsUniforms)
{
    const auto fill = compileShader (fillFragment);
    ASSERT_NE (fill, nullptr);
    EXPECT_EQ (fill->getNumInputTextures(), 0);

    auto uniforms = fill->createUniforms();
    const float blue[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    ASSERT_TRUE (uniforms.set ("tint", blue));

    // Blue everywhere, the square it does not read included
    const auto filled = render ({ { .type = FilterGraph::Type::shader, .shader = fill, .uniforms = uniforms } });
    ASSERT_EQ (filled.size(), static_cast<std::size_t> (size * size * 4));

    for (const auto point : { Point<int> (8, 8), Point<int> (32, 32), Point<int> (56, 56) })
    {
        EXPECT_LT (channelAt (filled, point.getX(), point.getY(), 0), 10);
        EXPECT_GT (channelAt (filled, point.getX(), point.getY(), 2), 200);
        EXPECT_GT (alphaAt (filled, point.getX(), point.getY()), 200);
    }

    // The picture is one the primitives after it read by its name
    const auto merged = render ({ { .type = FilterGraph::Type::shader, .result = "fill", .shader = fill, .uniforms = uniforms },
                                  { .type = FilterGraph::Type::composite, .in = FilterGraph::sourceGraphic, .in2 = "fill" } });
    ASSERT_EQ (merged.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_GT (channelAt (merged, 32, 32, 0), 200);
    EXPECT_LT (channelAt (merged, 32, 32, 2), 10);
    EXPECT_GT (channelAt (merged, 8, 8, 2), 200);

    // The uniforms are the ones of the primitive: zeroed ones fill with nothing
    const auto empty = render ({ { .type = FilterGraph::Type::shader, .shader = fill, .uniforms = fill->createUniforms() } });
    ASSERT_EQ (empty.size(), static_cast<std::size_t> (size * size * 4));
    EXPECT_LT (alphaAt (empty, 32, 32), 10);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
