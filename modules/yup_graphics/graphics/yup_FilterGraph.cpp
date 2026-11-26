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

namespace yup
{

namespace
{

constexpr uint8 filterGraphShaderBundle[] = {
#include "yup_graphics_shaders/yup_FilterGraph.inc"
};

// The operations of yup_FilterGraph.frag, which also reads the values of these two enumerations
enum class FilterPass
{
    copy,
    blur,
    morphology,
    offset,
    flood,
    colorMatrix,
    composite,
    blend,
    lighting,
    displacement,
    shadow,
    toLinear,
    toSrgb
};

static_assert (static_cast<int> (BlendMode::Multiply) == 11 && static_cast<int> (BlendMode::Additive) == 16);
static_assert (static_cast<int> (FilterGraph::CompositeOperator::arithmetic) == 6);

// The FilterParams block of yup_FilterGraph.frag
struct FilterUniforms
{
    float metrics[4] {};
    float a[4] {};
    float b[4] {};
    float c[4] {};
    float rows[4][4] {};
    float bias[4] {};
};

float toLinearChannel (float value) noexcept
{
    return value <= 0.04045f ? value / 12.92f : std::pow ((value + 0.055f) / 1.055f, 2.4f);
}

/* Returns what prevents a `shader` primitive from being drawn with a graphics API, known before any primitive is: an empty
   text when it has a shader reading the two pictures a filter gives at most, with a variant for that API. */
String findShaderPrimitiveProblem (const FilterGraph::Primitive& primitive, GpuPlatform platform)
{
    if (primitive.shader == nullptr)
        return "The shader primitive has no shader";

    if (const auto numTextures = primitive.shader->getNumInputTextures(); numTextures > 2)
        return "The shader reads " + String (numTextures) + " textures, a filter gives it two at most";

    if (! primitive.shader->canDrawWith (platform))
        return "The shader has no variant for the graphics API of the device";

    return {};
}

} // namespace

//==============================================================================
/* What every state of a device renders with. */
struct FilterGraph::Shared
{
    /* Returns the one of a device, made when no state has it yet. */
    static std::shared_ptr<Shared> get (const GpuDevice::Ptr& device)
    {
        static CriticalSection registryLock;
        static std::vector<std::weak_ptr<Shared>> registry;

        const ScopedLock scope (registryLock);

        std::erase_if (registry, [] (const auto& entry)
        {
            return entry.expired();
        });

        for (const auto& entry : registry)
            if (auto shared = entry.lock(); shared != nullptr && shared->device == device)
                return shared;

        auto shared = std::make_shared<Shared>();
        shared->device = device;

        // A pipeline that can't be made is not tried again: the filters of the device leave pictures as they are
        if (auto bundle = ShaderBundle::loadFromData (filterGraphShaderBundle, sizeof (filterGraphShaderBundle)); bundle.wasOk())
        {
            GpuPipelineOptions options;
            options.colorTargets.emplace_back().blendEnabled = false;

            if (auto compiled = GpuPipeline::compileFromBundle (device, bundle.getReference(), options); compiled.wasOk())
                shared->pipeline = compiled.getValue();
        }

        registry.push_back (shared);
        return shared;
    }

    GpuDevice::Ptr device;
    GpuPipeline::Ptr pipeline;

    // The textures the primitives render in, all as large as the picture last filtered
    std::vector<GpuTarget::Ptr> scratch;
    CriticalSection lock;
};

//==============================================================================
FilterGraph::State::State() = default;

FilterGraph::State::~State() = default;

void FilterGraph::State::releaseResources()
{
    output = nullptr;
    shared = nullptr;
    refusedShader = nullptr;
    shaderError.clear();
    failedPrimitive = -1;
}

//==============================================================================
/* Renders the primitives of a filter, each in a texture kept until the last primitive reading it is done. */
class FilterGraph::Renderer
{
public:
    Renderer (const FilterGraph& filterToRender, Shared& sharedToUse, GpuFrame& frameToUse, GpuTexture::Ptr sourceToFilter, Size<float> scaleToUse)
        : filter (filterToRender)
        , shared (sharedToUse)
        , frame (frameToUse)
        , source (std::move (sourceToFilter))
        , scale (scaleToUse)
        , width (source->getWidth())
        , height (source->getHeight())
        , used (shared.scratch.size(), false)
    {
    }

    bool render (GpuTarget& output)
    {
        const int numPrimitives = static_cast<int> (filter.primitives.size());

        std::vector<std::vector<int>> reads (static_cast<size_t> (numPrimitives));

        for (int i = 0; i < numPrimitives; ++i)
        {
            const auto& primitive = filter.primitives[static_cast<size_t> (i)];
            auto& inputs = reads[static_cast<size_t> (i)];

            if (primitive.type == Type::merge)
            {
                for (const auto& name : primitive.inputs)
                    inputs.push_back (findInput (name, i));
            }
            else if (primitive.type == Type::shader)
            {
                // A shader reads as many pictures as it declares textures
                const int numTextures = primitive.shader != nullptr ? primitive.shader->getNumInputTextures() : 0;

                if (numTextures > 0)
                    inputs.push_back (findInput (primitive.in, i));

                if (numTextures > 1)
                    inputs.push_back (findInput (primitive.in2, i));
            }
            else if (primitive.type != Type::flood)
            {
                inputs.push_back (findInput (primitive.in, i));

                if (primitive.type == Type::composite || primitive.type == Type::blend || primitive.type == Type::displacement)
                    inputs.push_back (findInput (primitive.in2, i));
            }
        }

        // Only what the last primitive ends up reading is rendered, and kept until its last reader is done
        std::vector<int> lastReader (static_cast<size_t> (numPrimitives), -1);
        lastReader.back() = numPrimitives;

        for (int i = numPrimitives; --i >= 0;)
        {
            if (lastReader[static_cast<size_t> (i)] < 0)
                continue;

            for (const auto input : reads[static_cast<size_t> (i)])
                if (input >= 0)
                    lastReader[static_cast<size_t> (input)] = jmax (lastReader[static_cast<size_t> (input)], i);
        }

        if (filter.linearColors)
        {
            const int linear = acquire();
            if (linear < 0 || ! pass (linear, FilterPass::toLinear, 0, source, nullptr, {}))
                return false;

            source = textureOf (linear);
        }

        results.assign (static_cast<size_t> (numPrimitives), -1);

        for (int i = 0; i < numPrimitives; ++i)
        {
            if (lastReader[static_cast<size_t> (i)] < 0)
                continue;

            const int target = acquire();
            if (target < 0 || ! render (filter.primitives[static_cast<size_t> (i)], reads[static_cast<size_t> (i)], target))
                return false;

            results[static_cast<size_t> (i)] = target;

            for (int j = 0; j < i; ++j)
            {
                if (results[static_cast<size_t> (j)] >= 0 && lastReader[static_cast<size_t> (j)] <= i)
                    release (std::exchange (results[static_cast<size_t> (j)], -1));
            }
        }

        return pass (output, filter.linearColors ? FilterPass::toSrgb : FilterPass::copy, 0, textureOf (results.back()), nullptr, {});
    }

    /* Returns the `shader` primitive the device refused when that is what made render() fail, with the reason it gave. */
    const Primitive* getRefusedPrimitive() const noexcept { return refusedPrimitive; }

    const String& getShaderError() const noexcept { return shaderError; }

private:
    // What a primitive reads when it is not the result of another one
    static constexpr int sourceInput = -1;
    static constexpr int alphaInput = -2;
    static constexpr int emptyInput = -3;

    /* Returns what a name stands for, for the primitive at an index: the primitive before it when it names nothing known. */
    int findInput (const String& name, int index) const
    {
        if (name == sourceGraphic)
            return sourceInput;

        if (name == sourceAlpha)
            return alphaInput;

        if (name == "BackgroundImage" || name == "BackgroundAlpha" || name == "FillPaint" || name == "StrokePaint")
            return emptyInput;

        if (name.isNotEmpty())
        {
            for (int i = index; --i >= 0;)
                if (filter.primitives[static_cast<size_t> (i)].result == name)
                    return i;
        }

        return index > 0 ? index - 1 : sourceInput;
    }

    //==============================================================================
    int acquire()
    {
        for (size_t i = 0; i < used.size(); ++i)
        {
            if (! used[i])
            {
                used[i] = true;
                return static_cast<int> (i);
            }
        }

        auto target = GpuTarget::create (shared.device, width, height);
        if (target == nullptr)
            return -1;

        shared.scratch.push_back (std::move (target));
        used.push_back (true);
        return static_cast<int> (used.size()) - 1;
    }

    void release (int target)
    {
        if (target >= 0)
            used[static_cast<size_t> (target)] = false;
    }

    GpuTexture::Ptr textureOf (int target) const
    {
        return target >= 0 ? shared.scratch[static_cast<size_t> (target)]->asTexture() : nullptr;
    }

    GpuTexture::Ptr getInput (int input)
    {
        if (input >= 0)
            return textureOf (results[static_cast<size_t> (input)]);

        if (input == alphaInput)
        {
            if (alphaTarget < 0)
            {
                FilterUniforms uniforms;
                uniforms.rows[3][3] = 1.0f;

                alphaTarget = acquire();
                if (alphaTarget < 0 || ! pass (alphaTarget, FilterPass::colorMatrix, 0, source, nullptr, uniforms))
                    return nullptr;
            }

            return textureOf (alphaTarget);
        }

        if (input == emptyInput)
        {
            if (emptyTarget < 0)
            {
                emptyTarget = acquire();
                if (emptyTarget < 0 || ! pass (emptyTarget, FilterPass::flood, 0, source, nullptr, {}))
                    return nullptr;
            }

            return textureOf (emptyTarget);
        }

        return source;
    }

    //==============================================================================
    bool pass (int target, FilterPass operation, int variant, const GpuTexture::Ptr& first, const GpuTexture::Ptr& second, FilterUniforms uniforms)
    {
        return target >= 0 && pass (*shared.scratch[static_cast<size_t> (target)], operation, variant, first, second, uniforms);
    }

    bool pass (GpuTarget& target, FilterPass operation, int variant, const GpuTexture::Ptr& first, const GpuTexture::Ptr& second, FilterUniforms uniforms)
    {
        if (first == nullptr)
            return false;

        uniforms.metrics[0] = 1.0f / static_cast<float> (width);
        uniforms.metrics[1] = 1.0f / static_cast<float> (height);
        uniforms.metrics[2] = static_cast<float> (operation);
        uniforms.metrics[3] = static_cast<float> (variant);

        auto renderPass = target.beginRenderPass (frame, { true, Colors::transparentBlack });
        if (! renderPass.isValid())
            return false;

        renderPass.setPipeline (shared.pipeline);
        renderPass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
        renderPass.setTexture (0, 1, first);
        renderPass.setTexture (0, 3, second != nullptr ? second : first);
        renderPass.draw (3);
        renderPass.finish();
        return true;
    }

    /* Sets the taps of a blur or of a morphology along an axis: at most 64 on each side, further apart when it reaches further. */
    void setTaps (FilterUniforms& uniforms, float reach, bool horizontal) const
    {
        const float step = jmax (1.0f, reach / 64.0f);

        uniforms.a[0] = horizontal ? step / static_cast<float> (width) : 0.0f;
        uniforms.a[1] = horizontal ? 0.0f : step / static_cast<float> (height);
        uniforms.a[3] = std::floor (reach / step + 0.5f);
    }

    void setColor (float* values, Color color) const
    {
        const float alpha = color.getAlphaFloat();
        const float channels[] = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat() };

        for (int i = 0; i < 3; ++i)
            values[i] = (filter.linearColors ? toLinearChannel (channels[i]) : channels[i]) * alpha;

        values[3] = alpha;
    }

    //==============================================================================
    bool render (const Primitive& primitive, const std::vector<int>& inputs, int target)
    {
        const auto first = inputs.empty() ? source : getInput (inputs[0]);
        const auto second = inputs.size() > 1 ? getInput (inputs[1]) : first;

        if (first == nullptr || second == nullptr)
            return false;

        // What a unit of the filter covers of the picture, whose coordinates go from 0 to 1
        const float unitX = scale.getWidth() / static_cast<float> (width);
        const float unitY = scale.getHeight() / static_cast<float> (height);

        FilterUniforms uniforms;

        switch (primitive.type)
        {
            case Type::blur:
                return blur (first, primitive.deviation, target);

            case Type::morphology:
                return separable (FilterPass::morphology, first, primitive.radius, primitive.dilate ? 1.0f : 0.0f, target);

            case Type::offset:
                uniforms.a[0] = primitive.offset.getX() * unitX;
                uniforms.a[1] = primitive.offset.getY() * unitY;
                return pass (target, FilterPass::offset, 0, first, nullptr, uniforms);

            case Type::flood:
                setColor (uniforms.a, primitive.color);
                return pass (target, FilterPass::flood, 0, first, nullptr, uniforms);

            case Type::colorMatrix:
                for (int row = 0; row < 4; ++row)
                {
                    for (int column = 0; column < 4; ++column)
                        uniforms.rows[row][column] = primitive.matrix[static_cast<size_t> (row * 5 + column)];

                    uniforms.bias[row] = primitive.matrix[static_cast<size_t> (row * 5 + 4)];
                }

                return pass (target, FilterPass::colorMatrix, 0, first, nullptr, uniforms);

            case Type::composite:
                std::copy (primitive.coefficients.begin(), primitive.coefficients.end(), uniforms.a);
                return pass (target, FilterPass::composite, static_cast<int> (primitive.compositeOperator), first, second, uniforms);

            case Type::blend:
                return pass (target, FilterPass::blend, static_cast<int> (primitive.blendMode), first, second, uniforms);

            case Type::merge:
                return merge (inputs, target);

            case Type::lighting:
                uniforms.a[0] = primitive.scale;
                uniforms.a[1] = primitive.lightingConstant;
                uniforms.a[2] = primitive.specularExponent;

                if (primitive.light == Light::distant)
                {
                    uniforms.b[0] = std::cos (primitive.lightAzimuth) * std::cos (primitive.lightElevation);
                    uniforms.b[1] = std::sin (primitive.lightAzimuth) * std::cos (primitive.lightElevation);
                    uniforms.b[2] = std::sin (primitive.lightElevation);
                }
                else
                {
                    uniforms.b[0] = primitive.lightPosition[0] * scale.getWidth();
                    uniforms.b[1] = primitive.lightPosition[1] * scale.getHeight();
                    uniforms.b[2] = primitive.lightPosition[2] * (scale.getWidth() + scale.getHeight()) * 0.5f;
                }

                setColor (uniforms.c, primitive.color.withAlpha (1.0f));
                return pass (target, FilterPass::lighting, (primitive.light == Light::point ? 2 : 0) + (primitive.specular ? 1 : 0), first, nullptr, uniforms);

            case Type::displacement:
                uniforms.a[0] = primitive.scale * unitX;
                uniforms.a[1] = primitive.scale * unitY;
                uniforms.b[static_cast<int> (primitive.xChannel)] = 1.0f;
                uniforms.c[static_cast<int> (primitive.yChannel)] = 1.0f;
                return pass (target, FilterPass::displacement, 0, first, second, uniforms);

            case Type::dropShadow:
            {
                const int blurred = acquire();
                if (blurred < 0 || ! blur (first, primitive.deviation, blurred))
                    return false;

                setColor (uniforms.a, primitive.color);
                uniforms.b[0] = primitive.offset.getX() * unitX;
                uniforms.b[1] = primitive.offset.getY() * unitY;

                const bool done = pass (target, FilterPass::shadow, 0, textureOf (blurred), first, uniforms);
                release (blurred);
                return done;
            }

            case Type::shader:
            {
                // What else prevents a shader from being drawn was found before the frame was begun
                if (primitive.shader == nullptr)
                    return false;

                const auto drawn = primitive.shader->draw (shared.device, frame, *shared.scratch[static_cast<size_t> (target)], primitive.uniforms, first, second);

                if (drawn.failed())
                {
                    refusedPrimitive = std::addressof (primitive);
                    shaderError = drawn.getErrorMessage();
                }

                return drawn.wasOk();
            }
        }

        return false;
    }

    bool blur (const GpuTexture::Ptr& input, Point<float> deviation, int target)
    {
        // The blur reaches three deviations away, where it has faded out
        return separable (FilterPass::blur, input, { deviation.getX() * 3.0f, deviation.getY() * 3.0f }, 0.0f, target);
    }

    /* Runs an operation along x then along y, reaching a distance given in the units of the filter. */
    bool separable (FilterPass operation, const GpuTexture::Ptr& input, Point<float> reach, float option, int target)
    {
        const int across = acquire();
        if (across < 0)
            return false;

        const auto makeUniforms = [&] (float reachInTexels, bool horizontal)
        {
            FilterUniforms uniforms;
            setTaps (uniforms, jmax (0.0f, reachInTexels), horizontal);

            // A blur is given its deviation in taps, the others their option
            uniforms.a[2] = operation == FilterPass::blur ? jmax (0.0f, reachInTexels) / 3.0f / jmax (1.0f, reachInTexels / 64.0f) : option;
            return uniforms;
        };

        const bool done = pass (across, operation, 0, input, nullptr, makeUniforms (reach.getX() * scale.getWidth(), true))
                       && pass (target, operation, 0, textureOf (across), nullptr, makeUniforms (reach.getY() * scale.getHeight(), false));

        release (across);
        return done;
    }

    bool merge (const std::vector<int>& inputs, int target)
    {
        if (inputs.empty())
            return pass (target, FilterPass::flood, 0, source, nullptr, {});

        if (inputs.size() == 1)
            return pass (target, FilterPass::copy, 0, getInput (inputs[0]), nullptr, {});

        int stacked = -1;

        for (size_t i = 1; i < inputs.size(); ++i)
        {
            const int next = i + 1 == inputs.size() ? target : acquire();
            const auto below = stacked >= 0 ? textureOf (stacked) : getInput (inputs[0]);

            if (next < 0 || ! pass (next, FilterPass::composite, static_cast<int> (CompositeOperator::over), getInput (inputs[i]), below, {}))
                return false;

            release (std::exchange (stacked, next));
        }

        return true;
    }

    //==============================================================================
    const FilterGraph& filter;
    Shared& shared;
    GpuFrame& frame;
    GpuTexture::Ptr source;
    const Size<float> scale;
    const int width, height;

    std::vector<bool> used;
    std::vector<int> results;
    int alphaTarget = -1, emptyTarget = -1;
    const Primitive* refusedPrimitive = nullptr;
    String shaderError;
};

//==============================================================================
GpuTexture::Ptr FilterGraph::render (GraphicsContext& context, const GpuTexture::Ptr& source, Size<float> scale, State& state) const
{
    if (primitives.empty() || source == nullptr || ! source->isValid())
        return source;

    const int width = source->getWidth();
    const int height = source->getHeight();

    const auto device = context.getGpuDevice();
    if (device == nullptr || device->isDeviceLost() || ! context.isGpuAvailable() || width <= 0 || height <= 0)
        return source;

    if (state.shared == nullptr || state.shared->device != device)
    {
        state.output = nullptr;
        state.shared = Shared::get (device);
        state.refusedShader = nullptr;
    }

    auto& shared = *state.shared;
    if (shared.pipeline == nullptr)
        return source;

    // What prevents a shader from running is found before anything is drawn for the primitives ahead of it
    for (size_t i = 0; i < primitives.size(); ++i)
    {
        const auto& primitive = primitives[i];

        if (primitive.type != Type::shader)
            continue;

        // A shader the device refused is not drawn again with it: the reason it gave still stands
        const bool wasRefused = primitive.shader != nullptr && primitive.shader == state.refusedShader;
        auto problem = wasRefused ? state.shaderError : findShaderPrimitiveProblem (primitive, device->getPlatform());

        if (problem.isEmpty())
            continue;

        if (! wasRefused)
            state.refusedShader = nullptr;

        // Told once, however many renders it stands for
        if (problem != state.shaderError)
        {
            YUP_DBG ("FilterGraph: a filter leaves its picture as it is. " << problem);
        }

        state.shaderError = std::move (problem);
        state.failedPrimitive = static_cast<int> (i);
        return source;
    }

    state.refusedShader = nullptr;
    state.shaderError.clear();
    state.failedPrimitive = -1;

    const ScopedLock scope (shared.lock);

    if (! shared.scratch.empty() && (shared.scratch.front()->getWidth() != width || shared.scratch.front()->getHeight() != height))
        shared.scratch.clear();

    if (state.output == nullptr || state.output->getWidth() != width || state.output->getHeight() != height)
        state.output = GpuTarget::create (device, width, height);

    if (state.output == nullptr)
        return source;

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return source;

    Renderer renderer (*this, shared, frame, source, scale);
    const bool rendered = renderer.render (*state.output);
    frame.submit();

    if (const auto* refused = renderer.getRefusedPrimitive(); refused != nullptr && ! rendered)
    {
        state.refusedShader = refused->shader;
        state.shaderError = renderer.getShaderError();
        state.failedPrimitive = static_cast<int> (refused - primitives.data());

        YUP_DBG ("FilterGraph: a filter leaves its picture as it is, the device can't draw a shader it runs. " << state.shaderError);
    }

    return rendered ? state.output->asTexture() : source;
}

//==============================================================================
std::array<float, 20> FilterGraph::saturationMatrix (float amount) noexcept
{
    const float s = amount;

    return {
        0.213f + 0.787f * s, 0.715f - 0.715f * s, 0.072f - 0.072f * s, 0.0f, 0.0f,
        0.213f - 0.213f * s, 0.715f + 0.285f * s, 0.072f - 0.072f * s, 0.0f, 0.0f,
        0.213f - 0.213f * s, 0.715f - 0.715f * s, 0.072f + 0.928f * s, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f
    };
}

std::array<float, 20> FilterGraph::hueRotationMatrix (float radians) noexcept
{
    const float c = std::cos (radians);
    const float s = std::sin (radians);

    return {
        0.213f + c * 0.787f - s * 0.213f, 0.715f - c * 0.715f - s * 0.715f, 0.072f - c * 0.072f + s * 0.928f, 0.0f, 0.0f,
        0.213f - c * 0.213f + s * 0.143f, 0.715f + c * 0.285f + s * 0.140f, 0.072f - c * 0.072f - s * 0.283f, 0.0f, 0.0f,
        0.213f - c * 0.213f - s * 0.787f, 0.715f - c * 0.715f + s * 0.715f, 0.072f + c * 0.928f + s * 0.072f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f
    };
}

std::array<float, 20> FilterGraph::luminanceToAlphaMatrix() noexcept
{
    return {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.2125f, 0.7154f, 0.0721f, 0.0f, 0.0f
    };
}

} // namespace yup
