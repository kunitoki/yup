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

#if ! YUP_MODULE_AVAILABLE_yup_graphics
#error This binding file requires adding the yup_graphics module in the project
#else
#include <yup_graphics/yup_graphics.h>
#endif

#include "yup_YupCore_bindings.h"

#include "../utilities/yup_PyBind11Includes.h"
#include "../pybind11/trampoline_self_life_support.h"

#include <memory>
#include <string>

namespace yup::Bindings
{

// =================================================================================================

void registerYupGraphicsBindings (pybind11::module_& m);

// =================================================================================================

/*
    Trampoline that lets Python subclasses implement a decoder.

    Python instances are constructed from raw bytes (the data is copied into an
    internal MemoryInputStream, so Python never hands an owned stream to C++).
    readSourceBytes() exposes the full source data to Python overrides.

    The trampoline derives from pybind11::trampoline_self_life_support, which
    (together with registering the class with py::smart_holder) keeps the Python
    wrapper alive while the C++ object is owned by C++, so virtual dispatch keeps
    reaching the Python overrides even after ownership crossed a C++ boundary.
*/
struct PyImageFormatReader : yup::ImageFormatReader, pybind11::trampoline_self_life_support
{
    PyImageFormatReader (yup::InputStream* sourceStream, const yup::String& formatName)
        : yup::ImageFormatReader (sourceStream, formatName)
    {
    }

    yup::Image readImage() override
    {
        PYBIND11_OVERRIDE_PURE (yup::Image, yup::ImageFormatReader, readImage);
    }

    yup::Image readFrame (int frameIndex) override
    {
        PYBIND11_OVERRIDE (yup::Image, yup::ImageFormatReader, readFrame, frameIndex);
    }

    bool readFrame (int frameIndex, yup::Image& dest) override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormatReader, readFrame, frameIndex, dest);
    }

    bool isAnimated() const override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormatReader, isAnimated);
    }

    int getFrameCount() const override
    {
        PYBIND11_OVERRIDE (int, yup::ImageFormatReader, getFrameCount);
    }

    int getLoopCount() const override
    {
        PYBIND11_OVERRIDE (int, yup::ImageFormatReader, getLoopCount);
    }

    int getFrameDelayMs (int frameIndex) const override
    {
        PYBIND11_OVERRIDE (int, yup::ImageFormatReader, getFrameDelayMs, frameIndex);
    }

    /** Returns the full contents of the source stream (position is reset to 0). */
    std::string readSourceBytes()
    {
        input->setPosition (0);

        yup::MemoryBlock block;
        input->readIntoMemoryBlock (block);
        input->setPosition (0);

        return std::string (static_cast<const char*> (block.getData()), block.getSize());
    }
};

// =================================================================================================

/*
    Trampoline that lets Python subclasses implement an encoder.

    Python instances wrap an OutputStream whose ownership is transferred to the
    writer; writeRawData() lets Python push encoded bytes into that stream and
    flushStream() flushes the underlying output. Self-life-support semantics are
    the same as PyImageFormatReader.
*/
struct PyImageFormatWriter : yup::ImageFormatWriter, pybind11::trampoline_self_life_support
{
    PyImageFormatWriter (yup::OutputStream* destStream, const yup::String& formatName, yup::PixelFormat pixelFormat)
        : yup::ImageFormatWriter (destStream, formatName, pixelFormat)
    {
    }

    bool writeImage (const yup::Image& image) override
    {
        PYBIND11_OVERRIDE_PURE (bool, yup::ImageFormatWriter, writeImage, image);
    }

    bool flush() override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormatWriter, flush);
    }

    bool supportsAnimation() const override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormatWriter, supportsAnimation);
    }

    bool beginAnimation (int loopCount) override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormatWriter, beginAnimation, loopCount);
    }

    bool writeFrame (const yup::Image& frame, int delayMs) override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormatWriter, writeFrame, frame, delayMs);
    }

    bool endAnimation() override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormatWriter, endAnimation);
    }

    /** Writes raw bytes into the owned output stream. */
    bool writeRawData (const std::string& data)
    {
        return output != nullptr && output->write (data.data(), data.size());
    }

    /** Flushes the owned output stream. */
    bool flushStream()
    {
        if (output == nullptr)
            return false;

        output->flush();
        return true;
    }

    /** Returns the underlying MemoryOutputStream when this writer was created
        with an in-memory destination, or nullptr otherwise. */
    yup::MemoryOutputStream* getMemoryOutputStream() noexcept
    {
        return dynamic_cast<yup::MemoryOutputStream*> (output.get());
    }
};

// =================================================================================================

/*
    Trampoline that lets Python subclasses implement yup::ImageFormat.

    Pure virtual members fall back to empty/neutral values when a Python subclass
    does not override them, so a Python format can decide which parts (detection,
    decode, encode) it actually implements. Returning None from createReaderFor()
    or createWriterFor() reports "not handled" to the ImageFormatManager.

    Formats registered through ImageFormatManager::registerFormat() transfer
    ownership to the manager. Self-life-support (py::smart_holder registration)
    keeps the Python wrapper alive while the manager owns the format, so the
    Python overrides remain reachable for as long as the format stays registered.
*/
struct PyImageFormat : yup::ImageFormat, pybind11::trampoline_self_life_support
{
    PyImageFormat() = default;

    const yup::String& getFormatName() const override
    {
        pybind11::gil_scoped_acquire gil;
        pybind11::function overrideFn = pybind11::get_override (static_cast<const yup::ImageFormat*> (this), "getFormatName");
        if (overrideFn)
        {
            nameCache = overrideFn().cast<yup::String>();
            return nameCache;
        }

        nameCache.clear();
        return nameCache;
    }

    yup::StringArray getFileExtensions (yup::ImageFormat::Mode mode) const override
    {
        pybind11::gil_scoped_acquire gil;
        pybind11::function overrideFn = pybind11::get_override (static_cast<const yup::ImageFormat*> (this), "getFileExtensions");
        if (overrideFn)
        {
            pybind11::object result = overrideFn (mode);
            return toStringArray (result);
        }

        return {};
    }

    bool canHandleFile (const yup::File& file, yup::ImageFormat::Mode mode) const override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormat, canHandleFile, file, mode);
    }

    bool canHandleStream (yup::InputStream& stream, yup::ImageFormat::Mode mode) const override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormat, canHandleStream, stream, mode);
    }

    std::unique_ptr<yup::ImageFormatReader> createReaderFor (yup::InputStream* sourceStream, const yup::ImageFormat::Options& options = {}) override
    {
        pybind11::gil_scoped_acquire gil;
        pybind11::function overrideFn = pybind11::get_override (static_cast<yup::ImageFormat*> (this), "createReaderFor");
        if (! overrideFn)
            return nullptr;

        pybind11::object result = overrideFn (sourceStream, options);
        if (result.is_none())
            return nullptr;

        return result.cast<std::unique_ptr<yup::ImageFormatReader>>();
    }

    std::unique_ptr<yup::ImageFormatWriter> createWriterFor (yup::OutputStream* destStream,
                                                             yup::PixelFormat pixelFormat,
                                                             const yup::StringPairArray& metadataValues,
                                                             int qualityOptionIndex) override
    {
        pybind11::gil_scoped_acquire gil;
        pybind11::function overrideFn = pybind11::get_override (static_cast<yup::ImageFormat*> (this), "createWriterFor");
        if (! overrideFn)
            return nullptr;

        pybind11::object result = overrideFn (destStream, pixelFormat, metadataValues, qualityOptionIndex);
        if (result.is_none())
            return nullptr;

        return result.cast<std::unique_ptr<yup::ImageFormatWriter>>();
    }

    yup::Array<yup::PixelFormat> getPossiblePixelFormats() const override
    {
        pybind11::gil_scoped_acquire gil;
        pybind11::function overrideFn = pybind11::get_override (static_cast<const yup::ImageFormat*> (this), "getPossiblePixelFormats");
        if (overrideFn)
        {
            yup::Array<yup::PixelFormat> result;

            for (auto item : overrideFn())
            {
                try
                {
                    result.add (item.cast<yup::PixelFormat>());
                }
                catch (const pybind11::cast_error&)
                {
                    result.add (static_cast<yup::PixelFormat> (item.cast<int>()));
                }
            }

            return result;
        }

        return {};
    }

    bool isCompressed() const override
    {
        PYBIND11_OVERRIDE (bool, yup::ImageFormat, isCompressed);
    }

    yup::StringArray getQualityOptions() const override
    {
        pybind11::gil_scoped_acquire gil;
        pybind11::function overrideFn = pybind11::get_override (static_cast<const yup::ImageFormat*> (this), "getQualityOptions");
        if (overrideFn)
            return toStringArray (overrideFn());

        return yup::ImageFormat::getQualityOptions();
    }

private:
    static yup::StringArray toStringArray (const pybind11::object& sequence)
    {
        yup::StringArray result;

        if (! sequence.is_none())
        {
            for (auto item : sequence)
                result.add (item.cast<yup::String>());
        }

        return result;
    }

    mutable yup::String nameCache;
};

// =================================================================================================

} // namespace yup::Bindings
