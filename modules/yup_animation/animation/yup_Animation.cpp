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

//==============================================================================
// Animation

Animation::Animation (AnimationComposition::Ptr comp)
    : composition_ (std::move (comp))
{
}

bool Animation::isValid() const noexcept
{
    return composition_ != nullptr;
}

AnimationComposition* Animation::getComposition() const noexcept
{
    return composition_.get();
}

//==============================================================================

Animation Animation::loadFromFile (const File& file, const LoadOptions& opts)
{
    LottieLoadOptions loaderOpts;
    loaderOpts.resourceDirectory = opts.resourceDirectory;

    auto comp = LottieReader::parseFile (file, loaderOpts);
    if (comp == nullptr)
        return {};

    return Animation (std::move (comp));
}

Animation Animation::loadFromData (const String& jsonText, const LoadOptions& opts)
{
    LottieLoadOptions loaderOpts;
    loaderOpts.resourceDirectory = opts.resourceDirectory;

    auto comp = LottieReader::parseData (jsonText, loaderOpts);
    if (comp == nullptr)
        return {};

    return Animation (std::move (comp));
}

Animation Animation::fromComposition (AnimationComposition::Ptr comp)
{
    return Animation (std::move (comp));
}

//==============================================================================

float Animation::totalFrames() const noexcept
{
    if (composition_ == nullptr)
        return 0.0f;
    return composition_->totalFrames();
}

float Animation::frameRate() const noexcept
{
    if (composition_ == nullptr)
        return 0.0f;
    return composition_->frameRate;
}

float Animation::duration() const noexcept
{
    if (composition_ == nullptr)
        return 0.0f;
    return composition_->duration();
}

Size<float> Animation::size() const noexcept
{
    if (composition_ == nullptr)
        return {};
    return composition_->size;
}

//==============================================================================

void Animation::renderFrame (Graphics& g,
                             float frameNo,
                             Rectangle<float> bounds,
                             bool keepAspectRatio) const
{
    if (composition_ == nullptr)
        return;

    AnimationRenderer::renderComposition (g, *composition_, frameNo, bounds, keepAspectRatio);
}

void Animation::renderAtTime (Graphics& g,
                              float timeInSeconds,
                              Rectangle<float> bounds,
                              bool keepAspectRatio) const
{
    if (composition_ == nullptr)
        return;

    const float frame = composition_->frameAtTime (timeInSeconds);
    AnimationRenderer::renderComposition (g, *composition_, frame, bounds, keepAspectRatio);
}

void Animation::renderAtProgress (Graphics& g,
                                  float progress,
                                  Rectangle<float> bounds,
                                  bool keepAspectRatio) const
{
    if (composition_ == nullptr)
        return;

    const float frame = composition_->frameAtProgress (progress);
    AnimationRenderer::renderComposition (g, *composition_, frame, bounds, keepAspectRatio);
}

//==============================================================================

String Animation::toJson (bool prettyPrint) const
{
    if (composition_ == nullptr)
        return {};
    return LottieWriter::toJson (*composition_, prettyPrint);
}

Result Animation::saveToFile (const File& destination, bool prettyPrint) const
{
    if (composition_ == nullptr)
        return Result::fail ("No composition loaded");
    return LottieWriter::toFile (*composition_, destination, prettyPrint);
}

} // namespace yup
