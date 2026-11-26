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

#pragma once

namespace yup
{

class GraphicsContext;

//==============================================================================
/** A graph of image operations run on the GPU, like the primitives of an SVG `<filter>`.

    A filter is a list of primitives. Each one reads the picture being filtered, or
    the result of a primitive before it, and makes a picture of its own: the result
    of the last one is the result of the filter.

    A primitive names what it reads with `in` (and `in2` when it combines two
    pictures): @ref sourceGraphic, @ref sourceAlpha, or the `result` name of an
    earlier primitive. Without a name it reads the result of the primitive before
    it, the first one reading the source.

    @code
    FilterGraph glow;
    glow.primitives.push_back ({ .type = FilterGraph::Type::blur, .result = "soft", .deviation = { 4.0f, 4.0f } });
    glow.primitives.push_back ({ .type = FilterGraph::Type::composite, .in = FilterGraph::sourceGraphic, .in2 = "soft" });

    source.setFilter (glow);                                  // a PaintSource, drawn with Graphics::drawSource
    @endcode

    Lengths are in the units of the picture: the content units of a PaintSource.

    @see PaintSource::setFilter, Graphics::drawSource, SVGFilter
*/
class YUP_API FilterGraph
{
    struct Shared;
    class Renderer;

public:
    //==============================================================================
    /** The name of the picture being filtered, for `in` and `in2`. */
    static constexpr const char* sourceGraphic = "SourceGraphic";

    /** The name of the alpha of the picture being filtered, as black with that alpha. */
    static constexpr const char* sourceAlpha = "SourceAlpha";

    //==============================================================================
    /** What a primitive does. */
    enum class Type : uint8
    {
        blur,         /**< Blurs `in` by `deviation`. */
        offset,       /**< Moves `in` by `offset`. */
        flood,        /**< Fills everything with `color`. */
        colorMatrix,  /**< Multiplies the colors of `in` by `matrix`. */
        composite,    /**< Combines `in` with `in2` following `compositeOperator`. */
        blend,        /**< Blends `in` over `in2` with `blendMode`. */
        morphology,   /**< Thins `in` by `radius`, or fattens it when `dilate` is set. */
        merge,        /**< Stacks the `inputs`, the last one on top. */
        lighting,     /**< Lights the alpha of `in`, taken as a height map. */
        displacement, /**< Moves the pixels of `in` by the colors of `in2`. */
        dropShadow,   /**< Puts `in` over its shadow: its alpha blurred, moved and colored. */
        shader        /**< Draws `shader` with its `uniforms`, reading `in`, and `in2` when it declares two textures. */
    };

    /** How a `composite` primitive combines `in` with `in2`. */
    enum class CompositeOperator : uint8
    {
        over,        /**< `in` over `in2`. */
        in,          /**< `in` where `in2` is. */
        out,         /**< `in` where `in2` is not. */
        atop,        /**< `in` where `in2` is, over `in2`. */
        exclusiveOr, /**< `in` and `in2` where the other is not. */
        lighter,     /**< The sum of `in` and `in2`. */
        arithmetic   /**< `k1 * in * in2 + k2 * in + k3 * in2 + k4`, with `coefficients`. */
    };

    /** A color channel, for the `displacement` primitive. */
    enum class Channel : uint8
    {
        red,
        green,
        blue,
        alpha
    };

    /** The light of a `lighting` primitive. */
    enum class Light : uint8
    {
        distant, /**< A light infinitely far, shining along `lightAzimuth` and `lightElevation`. */
        point    /**< A light at `lightPosition`. */
    };

    //==============================================================================
    /** One operation of a filter. Each type reads the fields its description names. */
    struct Primitive
    {
        /** What the primitive does. */
        Type type = Type::offset;

        /** The picture read, the result of the primitive before when empty. */
        String in;

        /** The second picture read by `composite`, `blend` and `displacement`, and by a `shader` declaring two textures. */
        String in2;

        /** The name later primitives read this result with. */
        String result;

        /** The pictures a `merge` stacks, from the bottom one. */
        StringArray inputs;

        /** The standard deviation of the blur of `blur` and `dropShadow`, along x and y. */
        Point<float> deviation;

        /** How far `offset` moves the picture, and `dropShadow` the shadow. */
        Point<float> offset;

        /** How far `morphology` thins or fattens, along x and y. */
        Point<float> radius;

        /** True for a `morphology` that fattens instead of thinning. */
        bool dilate = false;

        /** The color of `flood`, of the shadow of `dropShadow` and of the light of `lighting`. */
        Color color = Colors::black;

        /** The rows of the `colorMatrix`: five numbers for each of red, green, blue and alpha,
            the fifth being added. Colors are not premultiplied and go from 0 to 1.
        */
        std::array<float, 20> matrix { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };

        /** How `composite` combines its pictures. */
        CompositeOperator compositeOperator = CompositeOperator::over;

        /** The `k1`, `k2`, `k3` and `k4` of an arithmetic `composite`. */
        std::array<float, 4> coefficients {};

        /** How `blend` blends `in` over `in2`. */
        BlendMode blendMode = BlendMode::SrcOver;

        /** True for a `lighting` giving the highlights of a shiny surface, instead of a matte one lit. */
        bool specular = false;

        /** The height the alpha of `lighting` stands for, and of `displacement` the largest move. */
        float scale = 1.0f;

        /** How much light a `lighting` surface gives back. */
        float lightingConstant = 1.0f;

        /** How tight the highlights of a specular `lighting` are, from 1. */
        float specularExponent = 1.0f;

        /** The light of `lighting`. */
        Light light = Light::distant;

        /** The direction a distant light comes from, in radians clockwise from the x axis. */
        float lightAzimuth = 0.0f;

        /** The angle of a distant light above the picture, in radians. */
        float lightElevation = 0.0f;

        /** Where a point light is: x, y and its height above the picture. */
        std::array<float, 3> lightPosition {};

        /** The channels of `in2` a `displacement` moves the pixels along x and along y with. */
        Channel xChannel = Channel::alpha, yChannel = Channel::alpha;

        /** What a `shader` primitive draws. Its first texture reads `in` and its second one `in2`,
            both as large as the picture and premultiplied, as what it writes is: one declaring
            no texture makes a picture of its own, as a `flood` does. With `linearColors` the
            colors it reads and writes are linear. Without a shader, with one declaring more
            than two textures, or with one the device can't draw, the primitive fails and the
            filter leaves the picture as it is: State::getShaderError() tells why.
        */
        GpuImageShader::Ptr shader;

        /** The uniforms `shader` is drawn with, from GpuImageShader::createUniforms(). */
        ShaderUniforms uniforms;

        bool operator== (const Primitive&) const = default;
    };

    //==============================================================================
    /** The GPU resources a filter is rendered with, kept from one render to the next.

        Keep one for each picture filtered: it holds the result of the last render.
        The pipeline and the working textures are shared by every state of a device.
    */
    class YUP_API State
    {
    public:
        /** Creates a state holding nothing. */
        State();

        /** Destructor. */
        ~State();

        /** Frees what the state holds. The next render makes it again. */
        void releaseResources();

        /** Returns what prevented the last render from running a `shader` primitive, which left
            the picture as it was: the primitive has no shader, its shader declares more than
            two textures, or the device could not draw it, for the reason GpuImageShader::draw()
            gave. It is empty when the last render ran every shader of its filter.

            A shader the device refused is not drawn again with it, nor are the primitives
            before it: the next renders of a filter running it leave their picture as it is
            for the same reason, until the state is given another device or releases what it
            holds.
        */
        const String& getShaderError() const noexcept { return shaderError; }

        /** Returns the index of the primitive getShaderError() is about in the filter that
            was rendered, or -1 when there is no error.
        */
        int getFailedPrimitive() const noexcept { return failedPrimitive; }

    private:
        friend class FilterGraph;

        std::shared_ptr<Shared> shared;
        GpuTarget::Ptr output;
        GpuImageShader::Ptr refusedShader;
        String shaderError;
        int failedPrimitive = -1;
    };

    //==============================================================================
    /** The primitives, in the order they run. */
    std::vector<Primitive> primitives;

    /** True to run the primitives on linear colors instead of the sRGB ones pictures have. */
    bool linearColors = false;

    //==============================================================================
    /** Returns true when the filter has no primitive, and leaves pictures as they are. */
    bool isEmpty() const noexcept { return primitives.empty(); }

    bool operator== (const FilterGraph&) const = default;

    //==============================================================================
    /** Runs the filter on a picture.

        @param context  The context of the texture.
        @param source   The picture, with premultiplied colors.
        @param scale    The texels of the picture each unit of the lengths of the primitives covers, along x and y.
        @param state    What the filter renders with, holding the result until its next render.

        @returns The filtered picture, as large as the source, or the source itself when
                 the filter is empty or the GPU can't run it. What prevented a `shader`
                 primitive from running is then told by State::getShaderError().
    */
    GpuTexture::Ptr render (GraphicsContext& context, const GpuTexture::Ptr& source, Size<float> scale, State& state) const;

    //==============================================================================
    /** Returns the color matrix scaling the saturation: 0 gives grays, 1 leaves the colors. */
    static std::array<float, 20> saturationMatrix (float amount) noexcept;

    /** Returns the color matrix turning the hues by an angle, in radians. */
    static std::array<float, 20> hueRotationMatrix (float radians) noexcept;

    /** Returns the color matrix making black with the luminance of the colors as its alpha. */
    static std::array<float, 20> luminanceToAlphaMatrix() noexcept;
};

} // namespace yup
