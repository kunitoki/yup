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

namespace yup
{

//==============================================================================
SpectrumAnalyzerState::SpectrumAnalyzerState()
    : SpectrumAnalyzerState (defaultFftSize, maxFftSize)
{
}

SpectrumAnalyzerState::SpectrumAnalyzerState (int fftSizeToUse)
    : SpectrumAnalyzerState (fftSizeToUse, maxFftSize)
{
}

SpectrumAnalyzerState::SpectrumAnalyzerState (int fftSizeToUse, int maxFftSizeToAllow)
    : largestFftSize (isFftSizeUpTo (maxFftSizeToAllow, maxFftSize) ? maxFftSizeToAllow : maxFftSize)
    , fftSize (jmin (defaultFftSize, largestFftSize))
    , audioFifo (largestFftSize * fifoSizeInFrames)
    , sampleBuffer (static_cast<size_t> (largestFftSize * fifoSizeInFrames), 0.0f)
{
    jassert (isFftSizeUpTo (maxFftSizeToAllow, maxFftSize)); // The maximum is a power of 2 between minFftSize and maxFftSize

    setFftSize (fftSizeToUse);
}

SpectrumAnalyzerState::~SpectrumAnalyzerState()
{
}

//==============================================================================
void SpectrumAnalyzerState::pushSample (float sample) noexcept
{
    pushSamples (&sample, 1);
}

void SpectrumAnalyzerState::pushSamples (const float* samples, int numSamples) noexcept
{
    jassert (samples != nullptr);
    jassert (numSamples >= 0);

    if (numSamples <= 0 || samples == nullptr)
        return;

    // What does not fit is dropped: the audio thread never waits for the reading side
    const auto writeScope = audioFifo.write (jmin (numSamples, getFreeSpace()));

    if (writeScope.blockSize1 > 0)
        std::copy_n (samples, writeScope.blockSize1, &sampleBuffer[static_cast<size_t> (writeScope.startIndex1)]);

    if (writeScope.blockSize2 > 0)
        std::copy_n (samples + writeScope.blockSize1, writeScope.blockSize2, &sampleBuffer[static_cast<size_t> (writeScope.startIndex2)]);
}

//==============================================================================
bool SpectrumAnalyzerState::isFFTDataReady() const noexcept
{
    return audioFifo.getNumReady() >= fftSize.load();
}

bool SpectrumAnalyzerState::getFFTData (float* destBuffer) noexcept
{
    jassert (destBuffer != nullptr);

    return destBuffer != nullptr && readFrame (destBuffer, fftSize.load());
}

bool SpectrumAnalyzerState::getFFTData (Span<float> destBuffer) noexcept
{
    return destBuffer.size() == static_cast<size_t> (fftSize.load())
        && readFrame (destBuffer.data(), static_cast<int> (destBuffer.size()));
}

bool SpectrumAnalyzerState::readFrame (float* destBuffer, int frameSize) noexcept
{
    int startIndex1, blockSize1, startIndex2, blockSize2;
    audioFifo.prepareToRead (frameSize, startIndex1, blockSize1, startIndex2, blockSize2);

    if (blockSize1 + blockSize2 != frameSize)
        return false;

    std::copy_n (&sampleBuffer[static_cast<size_t> (startIndex1)], blockSize1, destBuffer);
    std::copy_n (&sampleBuffer[static_cast<size_t> (startIndex2)], blockSize2, destBuffer + blockSize1);

    // Advance by the hop, not by the frame, so that consecutive frames overlap
    audioFifo.finishedRead (jmin (getHopSize(), frameSize));

    return true;
}

void SpectrumAnalyzerState::skipSamples (int numSamples) noexcept
{
    audioFifo.finishedRead (jlimit (0, audioFifo.getNumReady(), numSamples));
}

//==============================================================================
void SpectrumAnalyzerState::reset() noexcept
{
    skipSamples (audioFifo.getNumReady());
}

void SpectrumAnalyzerState::setFftSize (int newSize) noexcept
{
    const bool isValidSize = isFftSizeUpTo (newSize, largestFftSize);

    jassert (isValidSize);

    if (isValidSize)
        fftSize.store (newSize);
}

bool SpectrumAnalyzerState::isFftSizeUpTo (int size, int largestSize) noexcept
{
    return isPowerOfTwo (size) && size >= minFftSize && size <= largestSize;
}

int SpectrumAnalyzerState::getNumAvailableSamples() const noexcept
{
    return audioFifo.getNumReady();
}

int SpectrumAnalyzerState::getFreeSpace() const noexcept
{
    return jmax (0, fftSize.load() * fifoSizeInFrames - 1 - audioFifo.getNumReady());
}

void SpectrumAnalyzerState::setOverlapFactor (float newOverlapFactor) noexcept
{
    jassert (newOverlapFactor >= 0.0f && newOverlapFactor < 1.0f);

    if (std::isnan (newOverlapFactor))
        return;

    overlapFactor.store (jlimit (0.0f, 0.95f, newOverlapFactor));
}

int SpectrumAnalyzerState::getHopSize() const noexcept
{
    return jmax (1, static_cast<int> (static_cast<float> (fftSize.load()) * (1.0f - overlapFactor.load())));
}

} // namespace yup
