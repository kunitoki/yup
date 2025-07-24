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
{
    initializeFifo();
}

SpectrumAnalyzerState::SpectrumAnalyzerState (int fftSizeToUse)
    : fftSize (fftSizeToUse)
{
    initializeFifo();
}

void SpectrumAnalyzerState::initializeFifo()
{
    fftDataReady = false;
    fifoSize = fftSize * 4;
    hopSize = static_cast<int> (fftSize * (1.0f - overlapFactor));

    audioFifo = std::make_unique<AbstractFifo> (fifoSize);

    sampleBuffer.resize (fifoSize, 0.0f);
}

SpectrumAnalyzerState::~SpectrumAnalyzerState()
{
}

//==============================================================================
void SpectrumAnalyzerState::pushSample (float sample) noexcept
{
    // Lock-free write to FIFO - safe for audio thread
    const auto writeScope = audioFifo->write (1);

    if (writeScope.blockSize1 > 0)
        sampleBuffer[static_cast<size_t> (writeScope.startIndex1)] = sample;

    // Check if we have enough samples for FFT processing with overlap
    if (audioFifo->getNumReady() >= fftSize)
        fftDataReady = true;
}

void SpectrumAnalyzerState::pushSamples (const float* samples, int numSamples) noexcept
{
    jassert (samples != nullptr);
    jassert (numSamples >= 0);

    if (numSamples <= 0 || samples == nullptr)
        return;

    // Lock-free write to FIFO - safe for audio thread
    const auto writeScope = audioFifo->write (numSamples);

    // Copy first block
    if (writeScope.blockSize1 > 0)
    {
        std::copy_n (samples, writeScope.blockSize1, &sampleBuffer[static_cast<size_t> (writeScope.startIndex1)]);
    }

    // Copy second block (wrap-around case)
    if (writeScope.blockSize2 > 0)
    {
        std::copy_n (samples + writeScope.blockSize1, writeScope.blockSize2, &sampleBuffer[static_cast<size_t> (writeScope.startIndex2)]);
    }

    // Check if we have enough samples for FFT processing with overlap
    if (audioFifo->getNumReady() >= fftSize)
        fftDataReady = true;
}

//==============================================================================
bool SpectrumAnalyzerState::isFFTDataReady() const noexcept
{
    return fftDataReady.load() && (audioFifo->getNumReady() >= fftSize);
}

bool SpectrumAnalyzerState::getFFTData (float* destBuffer) noexcept
{
    jassert (destBuffer != nullptr);

    if (destBuffer == nullptr || ! isFFTDataReady())
        return false;

    // Use prepareToRead to get read positions without consuming data
    int startIndex1, blockSize1, startIndex2, blockSize2;
    audioFifo->prepareToRead (fftSize, startIndex1, blockSize1, startIndex2, blockSize2);

    // Copy first block
    if (blockSize1 > 0)
    {
        std::copy_n (&sampleBuffer[static_cast<size_t> (startIndex1)],
                     blockSize1,
                     destBuffer);
    }

    // Copy second block (wrap-around case)
    if (blockSize2 > 0)
    {
        std::copy_n (&sampleBuffer[static_cast<size_t> (startIndex2)],
                     blockSize2,
                     destBuffer + blockSize1);
    }

    // Check if we read the full FFT size
    const int actualReadSize = blockSize1 + blockSize2;
    if (actualReadSize == fftSize)
    {
        // Advance read position by hopSize (not full FFT size) for overlap processing
        audioFifo->finishedRead (hopSize);

        // Check if we still have enough samples for next FFT
        fftDataReady = (audioFifo->getNumReady() >= fftSize);

        return true;
    }

    // If we couldn't read the full FFT size, reset flag and return false
    fftDataReady = false;
    return false;
}

//==============================================================================
void SpectrumAnalyzerState::reset() noexcept
{
    audioFifo->reset();

    fftDataReady = false;

    // Clear the sample buffer
    std::fill (sampleBuffer.begin(), sampleBuffer.end(), 0.0f);
}

void SpectrumAnalyzerState::setFftSize (int newSize)
{
    jassert (isPowerOfTwo (newSize) && newSize >= 64 && newSize <= 16384);

    if (fftSize != newSize)
    {
        fftSize = newSize;

        initializeFifo();
    }
}

int SpectrumAnalyzerState::getNumAvailableSamples() const noexcept
{
    return audioFifo->getNumReady();
}

int SpectrumAnalyzerState::getFreeSpace() const noexcept
{
    return audioFifo->getFreeSpace();
}

void SpectrumAnalyzerState::setOverlapFactor (float newOverlapFactor)
{
    jassert (newOverlapFactor >= 0.0f && newOverlapFactor < 1.0f);

    if (overlapFactor != newOverlapFactor)
    {
        overlapFactor = jlimit (0.0f, 0.95f, newOverlapFactor);
        hopSize = static_cast<int> (fftSize * (1.0f - overlapFactor));
        hopSize = jmax (1, hopSize); // Ensure minimum hop size of 1
    }
}

int SpectrumAnalyzerState::getHopSize() const noexcept
{
    return hopSize;
}

} // namespace yup
