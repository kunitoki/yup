/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#if !YUP_MODULE_AVAILABLE_yup_graphics
#error This binding file requires adding the yup_graphics module in the project
#else
//YUP_BEGIN_IGNORE_WARNINGS_MSVC(4244)
#include <yup_graphics/yup_graphics.h>
//YUP_END_IGNORE_WARNINGS_MSVC
#endif

#include "../utilities/yup_PyBind11Includes.h"

namespace yup::Bindings {

// =================================================================================================

void registerYupGraphicsBindings (pybind11::module_& m);

// =================================================================================================

/*
struct PyImageType : yup::ImageType
{
    yup::ImagePixelData::Ptr create (yup::Image::PixelFormat format, int width, int height, bool shouldClearImage) const override
    {
        PYBIND11_OVERRIDE_PURE (yup::ImagePixelData::Ptr, yup::ImageType, create, format, width, height, shouldClearImage);
    }

    int getTypeID() const override
    {
        PYBIND11_OVERRIDE_PURE (int, yup::ImageType, getTypeID);
    }

    yup::Image convert (const yup::Image& source) const override
    {
        PYBIND11_OVERRIDE (yup::Image, yup::ImageType, convert, source);
    }
};

// =================================================================================================

struct PyImageFileFormat : yup::ImageFileFormat
{
    using yup::ImageFileFormat::ImageFileFormat;

    yup::String getFormatName() override
    {
        PYBIND11_OVERRIDE_PURE (yup::String, yup::ImageFileFormat, getFormatName);
    }

    bool canUnderstand (yup::InputStream& input) override
    {
        PYBIND11_OVERRIDE_PURE (bool, yup::ImageFileFormat, canUnderstand, input);
    }

    bool usesFileExtension (const yup::File& possibleFile) override
    {
        PYBIND11_OVERRIDE_PURE (bool, yup::ImageFileFormat, usesFileExtension, possibleFile);
    }

    yup::Image decodeImage (yup::InputStream& input) override
    {
        PYBIND11_OVERRIDE_PURE (yup::Image, yup::ImageFileFormat, decodeImage, input);
    }

    bool writeImageToStream (const yup::Image& sourceImage, yup::OutputStream& destStream) override
    {
        PYBIND11_OVERRIDE_PURE (bool, yup::ImageFileFormat, writeImageToStream, sourceImage, destStream);
    }
};

// =================================================================================================

template <class Base = yup::LowLevelGraphicsContext>
struct PyLowLevelGraphicsContext : Base
{
    using Base::Base;

    bool isVectorDevice() const override
    {
        PYBIND11_OVERRIDE_PURE (bool, Base, isVectorDevice);
    }

    void setOrigin (yup::Point<int> origin) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, setOrigin, origin);
    }

    void addTransform (const yup::AffineTransform& transform) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, addTransform, transform);
    }

    float getPhysicalPixelScaleFactor() const override
    {
        PYBIND11_OVERRIDE_PURE (float, Base, getPhysicalPixelScaleFactor);
    }

    bool clipToRectangle (const yup::Rectangle<int>& rect) override
    {
        PYBIND11_OVERRIDE_PURE (bool, Base, clipToRectangle, rect);
    }

    bool clipToRectangleList (const yup::RectangleList<int>& rects) override
    {
        PYBIND11_OVERRIDE_PURE (bool, Base, clipToRectangleList, rects);
    }

    void excludeClipRectangle (const yup::Rectangle<int>& rect) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, excludeClipRectangle, rect);
    }

    void clipToPath (const yup::Path& path, const yup::AffineTransform& transform) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, clipToPath, path, transform);
    }

    void clipToImageAlpha (const yup::Image& image, const yup::AffineTransform& transform) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, clipToImageAlpha, image, transform);
    }

    bool clipRegionIntersects (const yup::Rectangle<int>& rect) override
    {
        PYBIND11_OVERRIDE_PURE (bool, Base, clipRegionIntersects, rect);
    }

    yup::Rectangle<int> getClipBounds() const override
    {
        PYBIND11_OVERRIDE_PURE (yup::Rectangle<int>, Base, getClipBounds);
    }

    bool isClipEmpty() const override
    {
        PYBIND11_OVERRIDE_PURE (bool, Base, isClipEmpty);
    }

    void saveState() override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, saveState);
    }

    void restoreState() override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, restoreState);
    }

    void beginTransparencyLayer (float opacity) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, beginTransparencyLayer, opacity);
    }

    void endTransparencyLayer() override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, endTransparencyLayer);
    }

    void setFill (const yup::FillType& fill) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, setFill, fill);
    }

    void setOpacity (float opacity) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, setOpacity, opacity);
    }

    void setInterpolationQuality (yup::Graphics::ResamplingQuality quality) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, setInterpolationQuality, quality);
    }

    void fillAll() override
    {
        PYBIND11_OVERRIDE (void, Base, fillAll);
    }

    void fillRect (const yup::Rectangle<int>& rect, bool replaceExistingContents) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, fillRect, rect, replaceExistingContents);
    }

    void fillRect (const yup::Rectangle<float>& rect) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, fillRect, rect);
    }

    void fillRectList (const yup::RectangleList<float>& rects) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, fillRectList, rects);
    }

    void fillPath (const yup::Path& path, const yup::AffineTransform& transform) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, fillPath, path, transform);
    }

    void drawImage (const yup::Image& image, const yup::AffineTransform& transform) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, drawImage, image, transform);
    }

    void drawLine (const yup::Line<float>& line) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, drawLine, line);
    }

    void setFont (const yup::Font& font) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, setFont, font);
    }

    const yup::Font& getFont() override
    {
        PYBIND11_OVERRIDE_PURE (const yup::Font&, Base, getFont);
    }

    void drawGlyphs (yup::Span<const uint16_t> glyphs,
                     yup::Span<const yup::Point<float>> glyphsPositions,
                     const yup::AffineTransform& transform) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, drawGlyphs, glyphs, glyphsPositions, transform);
    }

    uint64_t getFrameId() const override
    {
        PYBIND11_OVERRIDE_PURE (uint64_t, Base, getFrameId);
    }
};
*/

} // namespace yup::Bindings
