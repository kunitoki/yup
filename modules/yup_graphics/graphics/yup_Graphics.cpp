/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

//==============================================================================
rive::StrokeJoin toStrokeJoin (StrokeJoin join) noexcept
{
    return static_cast<rive::StrokeJoin> (join);
}

rive::StrokeCap toStrokeCap (StrokeCap cap) noexcept
{
    return static_cast<rive::StrokeCap> (cap);
}

static_assert (static_cast<int> (StrokePosition::Inside) == static_cast<int> (rive::StrokePosition::inside));
static_assert (static_cast<int> (StrokePosition::Center) == static_cast<int> (rive::StrokePosition::center));
static_assert (static_cast<int> (StrokePosition::Outside) == static_cast<int> (rive::StrokePosition::outside));

rive::StrokePosition toStrokePosition (StrokePosition position) noexcept
{
    return static_cast<rive::StrokePosition> (position);
}

static_assert (static_cast<int> (ImageWrap::Clamp) == static_cast<int> (rive::ImageWrap::clamp));
static_assert (static_cast<int> (ImageWrap::Repeat) == static_cast<int> (rive::ImageWrap::repeat));
static_assert (static_cast<int> (ImageWrap::Mirror) == static_cast<int> (rive::ImageWrap::mirror));
static_assert (static_cast<int> (ImageFilter::Linear) == static_cast<int> (rive::ImageFilter::bilinear));
static_assert (static_cast<int> (ImageFilter::Nearest) == static_cast<int> (rive::ImageFilter::nearest));

rive::StrokeParams toStrokeParams (const StrokeType& stroke) noexcept
{
    rive::StrokeParams params;
    params.thickness = stroke.getWidth();
    params.join = toStrokeJoin (stroke.getJoin());
    params.cap = toStrokeCap (stroke.getCap());
    params.position = toStrokePosition (stroke.getPosition());
    return params;
}

static_assert (static_cast<int> (LayerMaskMode::Alpha) == static_cast<int> (rive::LayerMaskMode::alpha));
static_assert (static_cast<int> (LayerMaskMode::InvertedAlpha) == static_cast<int> (rive::LayerMaskMode::invertedAlpha));
static_assert (static_cast<int> (LayerMaskMode::Luminance) == static_cast<int> (rive::LayerMaskMode::luminance));
static_assert (static_cast<int> (LayerMaskMode::InvertedLuminance) == static_cast<int> (rive::LayerMaskMode::invertedLuminance));

rive::LayerMaskMode toLayerMaskMode (LayerMaskMode mode) noexcept
{
    return static_cast<rive::LayerMaskMode> (mode);
}

rive::ImageSampler toImageSampler (const ImageSampling& sampling) noexcept
{
    rive::ImageSampler sampler;
    sampler.wrapX = static_cast<rive::ImageWrap> (sampling.wrapX);
    sampler.wrapY = static_cast<rive::ImageWrap> (sampling.wrapY);
    sampler.filter = static_cast<rive::ImageFilter> (sampling.filter);
    return sampler;
}

//==============================================================================
rive::BlendMode toBlendMode (BlendMode blendMode) noexcept
{
    switch (blendMode)
    {
        case BlendMode::SrcOver:
            return rive::BlendMode::srcOver;

        case BlendMode::Screen:
            return rive::BlendMode::screen;

        case BlendMode::Overlay:
            return rive::BlendMode::overlay;

        case BlendMode::Darken:
            return rive::BlendMode::darken;

        case BlendMode::Lighten:
            return rive::BlendMode::lighten;

        case BlendMode::ColorDodge:
            return rive::BlendMode::colorDodge;

        case BlendMode::ColorBurn:
            return rive::BlendMode::colorBurn;

        case BlendMode::HardLight:
            return rive::BlendMode::hardLight;

        case BlendMode::SoftLight:
            return rive::BlendMode::softLight;

        case BlendMode::Difference:
            return rive::BlendMode::difference;

        case BlendMode::Exclusion:
            return rive::BlendMode::exclusion;

        case BlendMode::Multiply:
            return rive::BlendMode::multiply;

        case BlendMode::Hue:
            return rive::BlendMode::hue;

        case BlendMode::Saturation:
            return rive::BlendMode::saturation;

        case BlendMode::Color:
            return rive::BlendMode::color;

        case BlendMode::Luminosity:
            return rive::BlendMode::luminosity;

        case BlendMode::Additive:
            return rive::BlendMode::additive;

        default:
            return rive::BlendMode::srcOver;
    }
}

//==============================================================================
void convertRawPathToRenderPath (const rive::RawPath& input, rive::RenderPath* output)
{
    input.addTo (output);
}

void convertRawPathToRenderPath (const rive::RawPath& input, rive::RenderPath* output, const AffineTransform& transform)
{
    if (transform.isIdentity())
    {
        convertRawPathToRenderPath (input, output);
    }
    else
    {
        auto newInput = input.transform (transform.toMat2D());
        newInput.addTo (output);
    }
}

//==============================================================================
rive::rcp<rive::RenderShader> toColorGradient (rive::Factory& factory, const ColorGradient& gradient, const AffineTransform& transform)
{
    const auto& colorStops = gradient.getStops();

    if (colorStops.empty())
        return nullptr;

    // Handle single color stop as solid color
    if (colorStops.size() == 1)
    {
        float stops[] = { 0.0f };
        auto color = (rive::ColorInt) colorStops[0].color;
        return factory.makeLinearGradient (0.0f, 0.0f, 1.0f, 0.0f, &color, stops, 1);
    }

    // Create dynamic arrays for colors and stops
    std::vector<rive::ColorInt> colors;
    std::vector<float> stops;

    colors.reserve (colorStops.size());
    stops.reserve (colorStops.size());

    for (const auto& stop : colorStops)
    {
        colors.push_back ((rive::ColorInt) stop.color);
        stops.push_back (stop.delta);
    }

    if (gradient.getType() == ColorGradient::Linear)
    {
        float x1 = gradient.getStartX();
        float y1 = gradient.getStartY();
        float x2 = gradient.getFinishX();
        float y2 = gradient.getFinishY();

        return factory.makeLinearGradient (x1, y1, x2, y2, colors.data(), stops.data(), colors.size());
    }
    else
    {
        float centerX = gradient.getStartX();
        float centerY = gradient.getStartY();
        float radiusX = gradient.getRadius();
        [[maybe_unused]] float radiusY = gradient.getRadius();

        return factory.makeRadialGradient (centerX, centerY, radiusX, colors.data(), stops.data(), colors.size());
    }
}

//==============================================================================
StyledText::HorizontalAlign toHorizontalAlign (Justification justification)
{
    if (justification.testFlags (Justification::left))
        return StyledText::left;
    else if (justification.testFlags (Justification::right))
        return StyledText::right;
    else if (justification.testFlags (Justification::horizontalCenter))
        return StyledText::center;
    else
        return StyledText::center;
}

StyledText::VerticalAlign toVerticalAlign (Justification justification)
{
    if (justification.testFlags (Justification::top))
        return StyledText::top;
    else if (justification.testFlags (Justification::bottom))
        return StyledText::bottom;
    else if (justification.testFlags (Justification::verticalCenter))
        return StyledText::middle;
    else
        return StyledText::middle;
}

// Text paths are drawn relative to the top-left of their shaped lines, not the drawing's origin.
Point<float> textOrigin (const StyledText& text, const Rectangle<float>& rect)
{
    return { rect.getX(), rect.getY() + text.getOffset (rect).getY() };
}

//==============================================================================
rive::Factory* getOffscreenFactory (GraphicsContext& context, RenderableTarget* target) noexcept
{
    if (target != nullptr)
        if (auto* renderContext = target->getRenderContext())
            return renderContext;

    return context.getFactory();
}

std::unique_ptr<rive::Renderer> makeOffscreenRenderer (GraphicsContext& context, RenderableTarget* target, int width, int height)
{
    if (target != nullptr)
        if (auto* renderContext = target->getRenderContext())
            return std::make_unique<rive::RiveRenderer> (renderContext);

    return context.makeRenderer (width, height);
}

//==============================================================================
// Feathered fills are only rendered with the clockwise fill rule, which matches the
// path's own fill rule once its filled area is resolved into clockwise outlines. The
// outline is copied, as it may be a cached polygon whose fill rule must not change.
rive::rcp<rive::RiveRenderPath> toClockwiseFillPath (const Path& path)
{
    auto renderPath = rive::make_rcp<rive::RiveRenderPath>();
    renderPath->fillRule (rive::FillRule::clockwise);
    renderPath->addRenderPath (path.createFillPolygon().getRenderPath(), {});
    return renderPath;
}

// The area a stroke clip keeps. Inside and outside bands are made like the renderer makes them:
// a stroke twice as wide, kept inside the path or cut out of it.
Path createStrokeClipOutline (const Path& path, const StrokeType& stroke)
{
    if (stroke.getPosition() == StrokePosition::Center)
        return path.createStrokePolygon (stroke.getWidth(), stroke.getJoin(), stroke.getCap());

    const auto band = path.createStrokePolygon (stroke.getWidth() * 2.0f, stroke.getJoin(), stroke.getCap());
    return band.combinedWith (path, stroke.getPosition() == StrokePosition::Inside ? Path::BooleanOperation::Intersect
                                                                                   : Path::BooleanOperation::Subtract);
}

// An offscreen Graphics covering a transparency layer's area, in layer-local coordinates.
std::unique_ptr<Graphics> createLayerGraphics (Graphics& parent, Rectangle<float> targetArea)
{
    const int width = static_cast<int> (std::ceil (targetArea.getWidth()));
    const int height = static_cast<int> (std::ceil (targetArea.getHeight()));

    if (width <= 0 || height <= 0)
        return nullptr;

    auto target = parent.getGraphicsContext().getGpuDevice()->createRenderableTarget (width, height);
    if (target == nullptr)
        return nullptr;

    auto graphics = std::make_unique<Graphics> (parent.getGraphicsContext(), std::move (target), 0x00000000);

    if (! graphics->isOffscreen())
        return nullptr;

    graphics->setDrawingArea ({ 0.0f, 0.0f, targetArea.getWidth(), targetArea.getHeight() });

    // A base state the layer returns to before its mask is applied
    graphics->getRenderer()->save();
    return graphics;
}

} // namespace

//==============================================================================
Graphics::SavedState::SavedState (Graphics& g)
    : g (std::addressof (g))
{
}

Graphics::SavedState::SavedState (SavedState&& other)
    : g (std::exchange (other.g, nullptr))
{
}

Graphics::SavedState& Graphics::SavedState::operator= (SavedState&& other)
{
    g = std::exchange (other.g, nullptr);
    return *this;
}

Graphics::SavedState::~SavedState()
{
    restore();
}

void Graphics::SavedState::restore()
{
    if (auto graphics = std::exchange (g, nullptr))
        graphics->restoreState();
}

//==============================================================================
Graphics::Graphics (GraphicsContext& context, rive::Renderer& renderer, float scale) noexcept
    : context (context)
    , offscreenTarget (nullptr)
    , factory (*context.getFactory())
    , ownedRenderer (nullptr)
    , renderer (renderer)
    , contextScale (scale)
{
    renderOptions.emplace_back();

    currentRenderOptions().scale = scale;
}

Graphics::Graphics (GraphicsContext& context, Image& image, uint32_t clearColor) noexcept
    : Graphics (context, context.getGpuDevice()->createRenderableTarget (image.getWidth(), image.getHeight()), clearColor)
{
    offscreenTargetImage = std::addressof (image);
}

Graphics::Graphics (GraphicsContext& context, std::unique_ptr<RenderableTarget> target, uint32_t clearColor) noexcept
    : context (context)
    , ownedOffscreenTarget (std::move (target))
    , offscreenTarget (ownedOffscreenTarget.get())
    , factory (*getOffscreenFactory (context, offscreenTarget))
    , ownedRenderer (makeOffscreenRenderer (context,
                                            offscreenTarget,
                                            offscreenTarget != nullptr ? offscreenTarget->getWidth() : 0,
                                            offscreenTarget != nullptr ? offscreenTarget->getHeight() : 0))
    , renderer (*ownedRenderer)
    , contextScale (1.0f)
{
    renderOptions.emplace_back();
    currentRenderOptions().scale = 1.0f;

    beginOffscreenFrame ({ .clearColor = GpuColor (clearColor) });
}

Graphics::Graphics (GraphicsContext& context, RenderableTarget& target, uint32_t clearColor) noexcept
    : context (context)
    , offscreenTarget (std::addressof (target))
    , factory (*getOffscreenFactory (context, offscreenTarget))
    , ownedRenderer (makeOffscreenRenderer (context, offscreenTarget, target.getWidth(), target.getHeight()))
    , renderer (*ownedRenderer)
    , contextScale (1.0f)
{
    renderOptions.emplace_back();
    currentRenderOptions().scale = 1.0f;

    beginOffscreenFrame ({ .clearColor = GpuColor (clearColor) });
}

Graphics::Graphics (GraphicsContext& context, RenderableTarget& target, const GpuFrameDescriptor& frameDesc, float scale) noexcept
    : context (context)
    , offscreenTarget (std::addressof (target))
    , factory (*getOffscreenFactory (context, offscreenTarget))
    , ownedRenderer (makeOffscreenRenderer (context, offscreenTarget, target.getWidth(), target.getHeight()))
    , renderer (*ownedRenderer)
    , contextScale (scale)
{
    renderOptions.emplace_back();
    currentRenderOptions().scale = scale;

    beginOffscreenFrame (frameDesc);
}

void Graphics::beginOffscreenFrame (const GpuFrameDescriptor& frameDesc)
{
    if (offscreenTarget == nullptr)
        return;

    auto desc = frameDesc;
    desc.renderTargetWidth = static_cast<uint32_t> (offscreenTarget->getWidth());
    desc.renderTargetHeight = static_cast<uint32_t> (offscreenTarget->getHeight());

    context.getGpuDevice()->beginOffscreen (*offscreenTarget, desc);

    currentRenderOptions().drawingArea = { 0.0f,
                                           0.0f,
                                           static_cast<float> (offscreenTarget->getWidth()) / contextScale,
                                           static_cast<float> (offscreenTarget->getHeight()) / contextScale };
}

Graphics::~Graphics()
{
    if (offscreenTarget != nullptr && ! committed)
        context.getGpuDevice()->endOffscreen (*offscreenTarget);
}

//==============================================================================

bool Graphics::isOffscreen() const noexcept
{
    return offscreenTarget != nullptr;
}

bool Graphics::commitToImage()
{
    if (offscreenTargetImage == nullptr || ! commitOffscreenTarget())
        return false;

    if (auto canvas = offscreenTarget->getRenderCanvas())
        offscreenTargetImage->setGpuTexture (GpuTexture::fromRenderCanvas (context.getGpuDevice(), std::move (canvas), offscreenTargetImage->getWidth(), offscreenTargetImage->getHeight()));
    else if (auto tex = offscreenTarget->adoptAsTexture())
        offscreenTargetImage->setGpuTexture (GpuTexture::fromGpuTexture (context.getGpuDevice(), std::move (tex), offscreenTargetImage->getWidth(), offscreenTargetImage->getHeight()));

    return true;
}

bool Graphics::commitOffscreenTarget()
{
    if (offscreenTarget == nullptr || committed)
        return false;

    context.getGpuDevice()->endOffscreen (*offscreenTarget);
    committed = true;

    return true;
}

bool Graphics::readPixelsToImage()
{
    if (offscreenTarget == nullptr || offscreenTargetImage == nullptr)
        return false;

    if (! committed)
        commitToImage();

    auto span = offscreenTargetImage->getRawData();
    return context.getGpuDevice()->readOffscreenPixels (*offscreenTarget, span.data(), span.size());
}

//==============================================================================

float Graphics::getContextScale() const
{
    return contextScale;
}

//==============================================================================
GraphicsContext& Graphics::getGraphicsContext()
{
    return context;
}

rive::Factory* Graphics::getFactory()
{
    return std::addressof (factory);
}

rive::Renderer* Graphics::getRenderer()
{
    return std::addressof (renderer);
}

//==============================================================================
Graphics::RenderOptions& Graphics::currentRenderOptions()
{
    jassert (! renderOptions.empty());

    return renderOptions.back();
}

const Graphics::RenderOptions& Graphics::currentRenderOptions() const
{
    jassert (! renderOptions.empty());

    return renderOptions.back();
}

//==============================================================================
Graphics::SavedState Graphics::saveState()
{
    jassert (! renderOptions.empty());

    renderOptions.emplace_back (renderOptions.back());

    renderer.save();

    return { *this };
}

void Graphics::restoreState()
{
    renderer.restore();

    renderOptions.pop_back();
}

//==============================================================================
Graphics::TransparencyLayer::TransparencyLayer (TransparencyLayer&& other) noexcept
    : parent (std::exchange (other.parent, nullptr))
    , targetArea (other.targetArea)
    , opacity (other.opacity)
    , graphics (std::move (other.graphics))
    , masks (std::move (other.masks))
    , committed (std::exchange (other.committed, true))
{
}

Graphics::TransparencyLayer& Graphics::TransparencyLayer::operator= (TransparencyLayer&& other) noexcept
{
    if (this != std::addressof (other))
    {
        parent = std::exchange (other.parent, nullptr);
        targetArea = other.targetArea;
        opacity = other.opacity;
        graphics = std::move (other.graphics);
        masks = std::move (other.masks);
        committed = std::exchange (other.committed, true);
    }

    return *this;
}

Graphics::TransparencyLayer::~TransparencyLayer() = default;

Graphics::TransparencyLayer::TransparencyLayer (Graphics& parent, Rectangle<float> targetArea, float opacity)
    : parent (std::addressof (parent))
    , targetArea (targetArea)
    , opacity (jlimit (0.0f, 1.0f, opacity))
    , graphics (createLayerGraphics (parent, targetArea))
{
}

bool Graphics::TransparencyLayer::isValid() const noexcept
{
    return parent != nullptr && graphics != nullptr && ! committed;
}

Graphics& Graphics::TransparencyLayer::getGraphics() const noexcept
{
    jassert (graphics != nullptr);
    return *graphics;
}

Graphics* Graphics::TransparencyLayer::addMask (LayerMaskMode mode)
{
    if (! isValid())
        return nullptr;

    // Same size as the layer, as a mask has to cover it pixel for pixel
    auto maskGraphics = createLayerGraphics (*parent, targetArea);
    if (maskGraphics == nullptr)
        return nullptr;

    return masks.emplace_back (Mask { std::move (maskGraphics), mode }).graphics.get();
}

bool Graphics::TransparencyLayer::commit()
{
    if (! isValid())
        return false;

    auto textureOf = [] (Graphics& offscreen) -> rive::rcp<rive::gpu::Texture>
    {
        if (auto canvas = offscreen.offscreenTarget->getRenderCanvas())
            return canvas->renderImage()->refTexture();

        return offscreen.offscreenTarget->adoptAsTexture();
    };

    // The masks must be the last things drawn into the layer before it is finished, from the layer's
    // base state: a clip left on the layer Graphics would otherwise limit where they apply
    bool atBaseState = false;

    for (auto& mask : masks)
    {
        if (! mask.graphics->commitOffscreenTarget())
            continue;

        auto maskTexture = textureOf (*mask.graphics);
        if (maskTexture == nullptr)
            continue;

        if (! std::exchange (atBaseState, true))
        {
            for (std::size_t i = 1; i < graphics->renderOptions.size(); ++i)
                graphics->renderer.restore();

            graphics->renderer.restore();
        }

        const auto maskImage = rive::make_rcp<rive::RiveRenderImage> (std::move (maskTexture));
        graphics->renderer.applyLayerMask (maskImage.get(), rive::ImageSampler::LinearClamp(), toLayerMaskMode (mask.mode));
    }

    if (! graphics->commitOffscreenTarget())
        return false;

    auto releaseCommittedLayer = [this]
    {
        committed = true;
        graphics.reset();
        masks.clear();
        parent = nullptr;
    };

    auto texture = textureOf (*graphics);

    if (texture == nullptr)
    {
        releaseCommittedLayer();
        return false;
    }

    auto parentState = parent->saveState();
    parent->setOpacity (parent->getOpacity() * opacity);

    if (! parent->renderTexture (std::move (texture), targetArea))
    {
        releaseCommittedLayer();
        return false;
    }

    releaseCommittedLayer();

    return true;
}

//==============================================================================
void Graphics::setFillColor (Color color)
{
    currentRenderOptions().fillColor = color;
    currentRenderOptions().isCurrentFillColor = true;
    currentRenderOptions().fillImage.reset();
}

Color Graphics::getFillColor() const
{
    return currentRenderOptions().fillColor;
}

//==============================================================================
void Graphics::setStrokeColor (Color color)
{
    currentRenderOptions().strokeColor = color;
    currentRenderOptions().isCurrentStrokeColor = true;
    currentRenderOptions().strokeImage.reset();
}

Color Graphics::getStrokeColor() const
{
    return currentRenderOptions().strokeColor;
}

//==============================================================================
void Graphics::setFillColorGradient (ColorGradient gradient)
{
    currentRenderOptions().fillGradient = std::move (gradient);
    currentRenderOptions().isCurrentFillColor = false;
    currentRenderOptions().fillImage.reset();
}

ColorGradient Graphics::getFillColorGradient() const
{
    return currentRenderOptions().fillGradient;
}

void Graphics::setFillImage (const Image& image, const AffineTransform& imageTransform, ImageSampling sampling)
{
    currentRenderOptions().fillImage = makeImagePaint (image, imageTransform, sampling);
}

//==============================================================================
void Graphics::setStrokeColorGradient (ColorGradient gradient)
{
    currentRenderOptions().strokeGradient = std::move (gradient);
    currentRenderOptions().isCurrentStrokeColor = false;
    currentRenderOptions().strokeImage.reset();
}

ColorGradient Graphics::getStrokeColorGradient() const
{
    return currentRenderOptions().strokeGradient;
}

void Graphics::setStrokeImage (const Image& image, const AffineTransform& imageTransform, ImageSampling sampling)
{
    currentRenderOptions().strokeImage = makeImagePaint (image, imageTransform, sampling);
}

Graphics::ImagePaint Graphics::makeImagePaint (const Image& image, const AffineTransform& imageTransform, ImageSampling sampling)
{
    // Made on the caller's Image, so its texture is kept and reused by later frames.
    image.createTextureIfNotPresent (context);

    return { image.getTexture(), image.getWidth(), image.getHeight(), imageTransform, sampling };
}

//==============================================================================
void Graphics::setFeather (float feather)
{
    currentRenderOptions().feather = jmax (0.0f, feather);
}

float Graphics::getFeather() const
{
    return currentRenderOptions().feather;
}

//==============================================================================
void Graphics::setOpacity (float opacity)
{
    currentRenderOptions().opacity = jlimit (0.0f, 1.0f, opacity);
}

float Graphics::getOpacity() const
{
    return currentRenderOptions().opacity;
}

Graphics::TransparencyLayer Graphics::beginTransparencyLayer (Rectangle<float> targetArea, float opacity)
{
    return { *this, targetArea, opacity };
}

void Graphics::setTint (Color tint)
{
    currentRenderOptions().tint = tint;
}

Color Graphics::getTint() const
{
    return currentRenderOptions().tint;
}

//==============================================================================
void Graphics::setStrokeType (StrokeType strokeType)
{
    auto& options = currentRenderOptions();

    options.strokeWidth = jmax (0.0f, strokeType.getWidth());
    options.join = strokeType.getJoin();
    options.cap = strokeType.getCap();
    options.strokePosition = strokeType.getPosition();
}

StrokeType Graphics::getStrokeType() const
{
    auto& options = currentRenderOptions();

    return StrokeType (options.strokeWidth, options.join, options.cap).withPosition (options.strokePosition);
}

void Graphics::setStrokeWidth (float strokeWidth)
{
    currentRenderOptions().strokeWidth = jmax (0.0f, strokeWidth);
}

float Graphics::getStrokeWidth() const
{
    return currentRenderOptions().strokeWidth;
}

void Graphics::setStrokeJoin (StrokeJoin join)
{
    currentRenderOptions().join = join;
}

StrokeJoin Graphics::getStrokeJoin() const
{
    return currentRenderOptions().join;
}

void Graphics::setStrokeCap (StrokeCap cap)
{
    currentRenderOptions().cap = cap;
}

StrokeCap Graphics::getStrokeCap() const
{
    return currentRenderOptions().cap;
}

void Graphics::setStrokePosition (StrokePosition position)
{
    currentRenderOptions().strokePosition = position;
}

StrokePosition Graphics::getStrokePosition() const
{
    return currentRenderOptions().strokePosition;
}

void Graphics::setStrokeMiterLimit ([[maybe_unused]] float limit)
{
    // Rive has a hardcoded miter limit of 4.0, so we don't need to set it here.
}

//==============================================================================
void Graphics::setBlendMode (BlendMode blendMode)
{
    currentRenderOptions().blendMode = blendMode;
}

BlendMode Graphics::getBlendMode() const
{
    return currentRenderOptions().blendMode;
}

void Graphics::setAdditiveAmount (float amount)
{
    currentRenderOptions().additiveAmount = jlimit (0.0f, 1.0f, amount);
}

float Graphics::getAdditiveAmount() const
{
    return currentRenderOptions().additiveAmount;
}

//==============================================================================
void Graphics::setDrawingArea (const Rectangle<float>& drawingArea)
{
    currentRenderOptions().drawingArea = drawingArea;
}

Rectangle<float> Graphics::getDrawingArea() const
{
    return currentRenderOptions().drawingArea;
}

//==============================================================================
void Graphics::setTransform (const AffineTransform& transform)
{
    currentRenderOptions().transform = transform;
}

void Graphics::addTransform (const AffineTransform& transform)
{
    currentRenderOptions().transform = currentRenderOptions().transform.followedBy (transform);
}

AffineTransform Graphics::getTransform() const
{
    return currentRenderOptions().transform;
}

//==============================================================================
void Graphics::setClipPath (const Rectangle<float>& clipRect)
{
    Path path;
    path.addRectangle (clipRect);

    setClipPath (path);
}

void Graphics::setClipPath (const Path& clipPath)
{
    auto& options = currentRenderOptions();

    options.clipPath = clipPath;
    options.clipStroke.reset();
    options.clipTransform = options.getTransform();

    auto renderPath = rive::make_rcp<rive::RiveRenderPath>();
    renderPath->fillRule (clipPath.isUsingNonZeroWinding() ? rive::FillRule::nonZero : rive::FillRule::evenOdd);
    renderPath->addRenderPath (clipPath.getRenderPath(), options.clipTransform.toMat2D());

    renderer.clipPath (renderPath.get());
}

void Graphics::setClipStroke (const Path& path, const StrokeType& stroke)
{
    auto& options = currentRenderOptions();
    const auto transform = options.getTransform();
    const auto clipStroke = stroke.withWidth (jmax (0.0f, stroke.getWidth()));

    // The outline is only built when getClipPath() asks for it
    options.clipPath = path;
    options.clipStroke = clipStroke;
    options.clipTransform = transform;

    // Nothing drawn under a degenerate transform is visible, so clip everything away. Tiny
    // determinants count too, as their inverse would overflow.
    if (std::abs (transform.getDeterminant()) < std::numeric_limits<float>::min())
    {
        auto emptyPath = rive::make_rcp<rive::RiveRenderPath>();
        renderer.clipPath (emptyPath.get());
        return;
    }

    // Clipped under the transform rather than baked into the path, so the stroke width scales
    // with it. Restoring would drop the clip too, so the transform is undone by its inverse.
    renderer.transform (transform.toMat2D());
    renderer.clipStroke (path.getRenderPath(), toStrokeParams (clipStroke));
    renderer.transform (transform.inverted().toMat2D());
}

void Graphics::setClipStroke (const Path& path)
{
    setClipStroke (path, getStrokeType());
}

Path Graphics::getClipPath() const
{
    const auto& options = currentRenderOptions();

    if (options.clipPath.isEmpty())
        return {};

    const auto clipPath = options.clipStroke.has_value() ? createStrokeClipOutline (options.clipPath, *options.clipStroke)
                                                         : options.clipPath;

    const auto transform = options.getTransform();

    if (transform == options.clipTransform)
        return clipPath;

    if (transform.getDeterminant() == 0.0f)
        return {};

    return clipPath.transformed (options.clipTransform.followedBy (transform.inverted()));
}

//==============================================================================
void Graphics::strokeLine (float x1, float y1, float x2, float y2)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.reserveSpace (2);
    path.moveTo (x1, y1);
    path.lineTo (x2, y2);

    renderStrokePath (path, options, options.getTransform());
}

void Graphics::strokeLine (const Point<float>& p1, const Point<float>& p2)
{
    strokeLine (p1.getX(), p1.getY(), p2.getX(), p2.getY());
}

//==============================================================================
void Graphics::fillAll()
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRectangle (options.getDrawingArea().withZeroPosition());

    renderFillPath (path, options, options.getTransform());
}

//==============================================================================
void Graphics::fillRect (float x, float y, float width, float height)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRectangle (x, y, width, height);

    renderFillPath (path, options, options.getTransform());
}

void Graphics::fillRect (const Rectangle<float>& r)
{
    fillRect (r.getX(), r.getY(), r.getWidth(), r.getHeight());
}

//==============================================================================
void Graphics::strokeRect (float x, float y, float width, float height)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRectangle (x, y, width, height);

    renderStrokePath (path, options, options.getTransform());
}

void Graphics::strokeRect (const Rectangle<float>& r)
{
    strokeRect (r.getX(), r.getY(), r.getWidth(), r.getHeight());
}

//==============================================================================
void Graphics::fillRoundedRect (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRoundedRectangle (
        x, y, width, height, radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);

    renderFillPath (path, options, options.getTransform());
}

void Graphics::fillRoundedRect (float x, float y, float width, float height, float radius)
{
    fillRoundedRect (x, y, width, height, radius, radius, radius, radius);
}

void Graphics::fillRoundedRect (const Rectangle<float>& r, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    fillRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);
}

void Graphics::fillRoundedRect (const Rectangle<float>& r, float radius)
{
    fillRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radius, radius, radius, radius);
}

//==============================================================================
void Graphics::strokeRoundedRect (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRoundedRectangle (
        x, y, width, height, radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);

    renderStrokePath (path, options, options.getTransform());
}

void Graphics::strokeRoundedRect (float x, float y, float width, float height, float radius)
{
    strokeRoundedRect (x, y, width, height, radius, radius, radius, radius);
}

void Graphics::strokeRoundedRect (const Rectangle<float>& r, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    strokeRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);
}

void Graphics::strokeRoundedRect (const Rectangle<float>& r, float radius)
{
    strokeRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radius, radius, radius, radius);
}

//==============================================================================
void Graphics::fillEllipse (const Rectangle<float>& r)
{
    Path path;
    path.addEllipse (r);

    fillPath (path);
}

void Graphics::fillEllipse (float x, float y, float width, float height)
{
    Path path;
    path.addEllipse (x, y, width, height);

    fillPath (path);
}

void Graphics::strokeEllipse (const Rectangle<float>& r)
{
    Path path;
    path.addEllipse (r);

    strokePath (path);
}

void Graphics::strokeEllipse (float x, float y, float width, float height)
{
    Path path;
    path.addEllipse (x, y, width, height);

    strokePath (path);
}

//==============================================================================
void Graphics::strokePath (const Path& path)
{
    const auto& options = currentRenderOptions();

    renderStrokePath (path, options, options.getTransform());
}

//==============================================================================
void Graphics::fillPath (const Path& path)
{
    const auto& options = currentRenderOptions();

    renderFillPath (path, options, options.getTransform());
}

//==============================================================================
void Graphics::renderStrokePath (const Path& path, const RenderOptions& options, const AffineTransform& transform)
{
    rive::RiveRenderPaint paint;
    if (! setupStrokePaint (paint, options, transform))
        return;

    renderer.save();
    renderer.transform (transform.toMat2D());
    applyModulation (options);
    renderer.drawPath (path.getRenderPath(), std::addressof (paint));
    renderer.restore();
}

void Graphics::renderFillPath (const Path& path, const RenderOptions& options, const AffineTransform& transform)
{
    rive::RiveRenderPaint paint;
    if (! setupFillPaint (paint, options, transform))
        return;

    const auto renderPath = options.feather > 0.0f ? toClockwiseFillPath (path) : rive::ref_rcp (path.getRenderPath());

    renderer.save();
    renderer.transform (transform.toMat2D());
    applyModulation (options);
    renderer.drawPath (renderPath.get(), std::addressof (paint));
    renderer.restore();
}

//==============================================================================
bool Graphics::setupFillPaint (rive::RiveRenderPaint& paint, const RenderOptions& options, const AffineTransform& transform, Point<float> imageOffset)
{
    paint.style (rive::RenderPaintStyle::fill);
    setupPaintBlend (paint, options);

    if (options.fillImage.has_value())
        return setupImagePaint (paint, *options.fillImage, imageOffset);

    if (options.isFillColor())
        paint.color ((rive::ColorInt) options.getFillColor());
    else
        paint.shader (toColorGradient (factory, options.getFillColorGradient(), transform));

    return true;
}

bool Graphics::setupStrokePaint (rive::RiveRenderPaint& paint, const RenderOptions& options, const AffineTransform& transform, Point<float> imageOffset)
{
    paint.style (rive::RenderPaintStyle::stroke);
    setupPaintBlend (paint, options);
    paint.thickness (options.getStrokeWidth());
    paint.join (toStrokeJoin (options.join));
    paint.cap (toStrokeCap (options.cap));
    paint.strokePosition (toStrokePosition (options.strokePosition));

    if (options.strokeImage.has_value())
        return setupImagePaint (paint, *options.strokeImage, imageOffset);

    if (options.isStrokeColor())
        paint.color ((rive::ColorInt) options.getStrokeColor());
    else
        paint.shader (toColorGradient (factory, options.getStrokeColorGradient(), transform));

    return true;
}

void Graphics::setupPaintBlend (rive::RiveRenderPaint& paint, const RenderOptions& options)
{
    paint.blendMode (toBlendMode (options.blendMode));
    paint.additiveness (options.blendMode == BlendMode::Additive ? options.additiveAmount : 0.0f);
    paint.feather (options.feather);
}

bool Graphics::setupImagePaint (rive::RiveRenderPaint& paint, const ImagePaint& imagePaint, Point<float> imageOffset)
{
    if (imagePaint.texture == nullptr)
        return false;

    // The image modulates the paint color, so white shows it unchanged.
    paint.color (0xffffffff);

    // Rive maps the unit square onto the path, so scale it to the image's pixels first.
    const auto imageTransform = AffineTransform::scaling (static_cast<float> (imagePaint.width), static_cast<float> (imagePaint.height))
                                    .followedBy (imagePaint.transform)
                                    .translated (-imageOffset.getX(), -imageOffset.getY());

    const auto renderImage = rive::make_rcp<rive::RiveRenderImage> (imagePaint.texture);
    paint.modulatedImage (renderImage.get(), toImageSampler (imagePaint.sampling), imageTransform.toMat2D());
    return true;
}

void Graphics::applyModulation (const RenderOptions& options)
{
    renderer.modulateOpacity (options.opacity);
    renderer.modulateColor ((rive::ColorInt) options.tint);
}

//==============================================================================
void Graphics::drawImageAt (const Image& image, const Point<float>& pos)
{
    drawImage (image, Rectangle<float> (pos.getX(), pos.getY(), static_cast<float> (image.getWidth()), static_cast<float> (image.getHeight())));
}

void Graphics::drawImage (const Image& image, const Rectangle<float>& targetArea)
{
    if (! image.createTextureIfNotPresent (context))
        return;

    renderTexture (image.getTexture(), targetArea);
}

void Graphics::drawImageMesh (const Image& image, const ImageMesh& mesh, ImageSampling sampling)
{
    if (! image.createTextureIfNotPresent (context))
        return;

    const auto* buffers = mesh.updateGpuBuffers (factory, context.getGpuDevice());
    if (buffers == nullptr)
        return;

    const auto& options = currentRenderOptions();
    const auto renderImage = rive::make_rcp<rive::RiveRenderImage> (image.getTexture());

    renderer.save();
    renderer.transform (options.getTransform().toMat2D());
    renderer.modulateColor ((rive::ColorInt) options.tint);
    renderer.drawImageMesh (renderImage.get(),
                            toImageSampler (sampling),
                            buffers->vertices,
                            buffers->textureCoordinates,
                            buffers->indices,
                            static_cast<uint32_t> (mesh.getVertices().size()),
                            static_cast<uint32_t> (mesh.getIndices().size()),
                            toBlendMode (options.blendMode),
                            options.opacity,
                            options.blendMode == BlendMode::Additive ? options.additiveAmount : 0.0f);
    renderer.restore();
}

void Graphics::drawImageMeshInstanced (const Image& image, const ImageMesh& mesh, Span<const ImageMeshInstance> instances, ImageSampling sampling)
{
    if (instances.empty())
        return;

    if (! image.createTextureIfNotPresent (context))
        return;

    const auto* buffers = mesh.updateGpuBuffers (factory, context.getGpuDevice());
    if (buffers == nullptr)
        return;

    auto meshInstances = factory.makeImageMeshInstances (instances.size());
    if (meshInstances == nullptr)
        return;

    auto instanceData = meshInstances->edit();
    for (std::size_t i = 0; i < instances.size(); ++i)
    {
        const auto& instance = instances[i];
        auto& data = instanceData[i];

        data.transform = instance.transform.toMat2D();
        data.uvTranslate = { instance.textureOffset.getX(), instance.textureOffset.getY() };
        data.uvScale = { instance.textureScale.getX(), instance.textureScale.getY() };
        data.opacity = jlimit (0.0f, 1.0f, instance.opacity);
        data.additiveness = jlimit (0.0f, 1.0f, instance.additiveAmount);
    }
    meshInstances->endEdit();

    const auto& options = currentRenderOptions();
    const auto renderImage = rive::make_rcp<rive::RiveRenderImage> (image.getTexture());

    renderer.save();
    renderer.transform (options.getTransform().toMat2D());
    applyModulation (options);
    renderer.drawImageMeshInstanced (renderImage.get(),
                                     toImageSampler (sampling),
                                     buffers->vertices,
                                     buffers->textureCoordinates,
                                     buffers->indices,
                                     static_cast<uint32_t> (mesh.getVertices().size()),
                                     static_cast<uint32_t> (mesh.getIndices().size()),
                                     std::move (meshInstances));
    renderer.restore();
}

void Graphics::drawTexture (const GpuTexture::Ptr& texture, const Rectangle<float>& targetArea)
{
    if (texture == nullptr)
        return;

    // Graphics is a friend of GpuTexture - may access private getOrAdoptGpuTexture()
    renderTexture (texture->getOrAdoptGpuTexture(), targetArea);
}

bool Graphics::renderTexture (rive::rcp<rive::gpu::Texture> texture, const Rectangle<float>& targetArea)
{
    auto renderContext = context.getRenderContext();
    if (renderContext == nullptr || texture == nullptr)
        return false;

    if (targetArea.isEmpty())
        return false;

    const auto textureWidth = static_cast<float> (texture->width());
    const auto textureHeight = static_cast<float> (texture->height());
    if (textureWidth <= 0.0f || textureHeight <= 0.0f)
        return false;

    const auto& options = currentRenderOptions();

    // Draw through the renderer's image path instead of a path with an image
    // paint: frames in atomic interlock mode (e.g. iOS simulator, or when
    // raster ordering is disabled) do not support image paints on paths, and
    // drawImage() falls back to a dedicated image-rect draw there.
    auto renderImage = rive::make_rcp<rive::RiveRenderImage> (std::move (texture));

    // drawImage() maps the image to the rect [0, 0, width, height].
    const auto imageTransform = AffineTransform::scaling (targetArea.getWidth() / textureWidth,
                                                          targetArea.getHeight() / textureHeight)
                                    .translated (targetArea.getX(), targetArea.getY())
                                    .followedBy (options.getTransform());

    renderer.save();
    renderer.transform (imageTransform.toMat2D());
    renderer.modulateColor ((rive::ColorInt) options.tint);
    renderer.drawImage (renderImage.get(),
                        rive::ImageSampler::LinearClamp(),
                        toBlendMode (options.blendMode),
                        options.opacity,
                        options.blendMode == BlendMode::Additive ? options.additiveAmount : 0.0f);
    renderer.restore();

    return true;
}

//==============================================================================
void Graphics::fillFittedText (const StyledText& text, const Rectangle<float>& rect)
{
    jassert (! text.needsUpdate());
    if (text.needsUpdate() || text.isEmpty())
        return;

    bool hasStylePaints = false;
    for (auto style : text.getRenderStyles())
    {
        if (style->paint != nullptr)
        {
            hasStylePaints = true;
        }
        else
        {
            hasStylePaints = false;
            break;
        }
    }

    if (hasStylePaints)
    {
        renderFittedText (text, rect, nullptr);
        return;
    }

    const auto& options = currentRenderOptions();

    rive::RiveRenderPaint paint;
    if (setupFillPaint (paint, options, options.getTransform(), textOrigin (text, rect)))
        renderFittedText (text, rect, std::addressof (paint));
}

void Graphics::fillFittedText (const String& text, const Font& font, const Rectangle<float>& rect, Justification justification)
{
    if (text.isEmpty())
        return;

    StyledText styledText;
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (rect.getSize());
        modifier.appendText (text, font);
        modifier.setHorizontalAlign (toHorizontalAlign (justification));
        modifier.setVerticalAlign (toVerticalAlign (justification));
    }

    fillFittedText (styledText, rect);
}

void Graphics::strokeFittedText (const StyledText& text, const Rectangle<float>& rect)
{
    jassert (! text.needsUpdate());
    if (text.needsUpdate() || text.isEmpty())
        return;

    const auto& options = currentRenderOptions();

    rive::RiveRenderPaint paint;
    if (! setupStrokePaint (paint, options, options.getTransform(), textOrigin (text, rect)))
        return;

    renderFittedText (text, rect, std::addressof (paint));
}

void Graphics::strokeFittedText (const String& text, const Font& font, const Rectangle<float>& rect, Justification justification)
{
    if (text.isEmpty())
        return;

    StyledText styledText;
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (rect.getSize());
        modifier.appendText (text, font);
        modifier.setHorizontalAlign (toHorizontalAlign (justification));
        modifier.setVerticalAlign (toVerticalAlign (justification));
    }

    strokeFittedText (styledText, rect);
}

void Graphics::renderFittedText (const StyledText& text, const Rectangle<float>& rect, rive::RiveRenderPaint* paint)
{
    jassert (! text.needsUpdate());
    if (text.needsUpdate() || text.isEmpty())
        return;

    const auto& options = currentRenderOptions();

    renderer.save();
    applyModulation (options);

    if (text.getOverflow() != StyledText::visible)
    {
        rive::RawPath path;
        path.addRect (rect.toAABB());
        path.transformInPlace (options.getTransform().toMat2D());
        auto renderPath = rive::make_rcp<rive::RiveRenderPath> (rive::FillRule::clockwise, path);
        renderer.clipPath (renderPath.get());
    }

    const auto origin = textOrigin (text, rect); // Horizontal alignment is already baked into the shaped glyph paths.
    auto transform = options.getTransform (origin.getX(), origin.getY());
    renderer.transform (transform.toMat2D());

    const bool isFeatheredFill = paint != nullptr && paint->getFeather() > 0.0f && ! paint->getIsStroked();

    for (auto style : text.getRenderStyles())
    {
        if (isFeatheredFill)
            renderer.drawPath (toClockwiseFillPath (Path (rive::ref_rcp (static_cast<rive::RiveRenderPath*> (style->path.get())))).get(), paint);
        else
            renderer.drawPath (style->path.get(), (paint != nullptr) ? paint : style->paint.get());
    }

    renderer.restore();
}

} // namespace yup
