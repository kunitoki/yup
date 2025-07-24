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

    // Check if we have enough samples for FFT processing
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
        std::copy_n (samples, writeScope.blockSize1,
                    &sampleBuffer[static_cast<size_t> (writeScope.startIndex1)]);
    }

    // Copy second block (wrap-around case)
    if (writeScope.blockSize2 > 0)
    {
        std::copy_n (samples + writeScope.blockSize1, writeScope.blockSize2,
                    &sampleBuffer[static_cast<size_t> (writeScope.startIndex2)]);
    }

    // Check if we have enough samples for FFT processing
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

    if (destBuffer == nullptr || !isFFTDataReady())
        return false;

    // Lock-free read from FIFO - safe for UI thread
    const auto readScope = audioFifo->read (fftSize);

    // Copy first block
    if (readScope.blockSize1 > 0)
    {
        std::copy_n (&sampleBuffer[static_cast<size_t> (readScope.startIndex1)],
                    readScope.blockSize1, destBuffer);
    }

    // Copy second block (wrap-around case)
    if (readScope.blockSize2 > 0)
    {
        std::copy_n (&sampleBuffer[static_cast<size_t> (readScope.startIndex2)],
                    readScope.blockSize2, destBuffer + readScope.blockSize1);
    }

    // Reset the ready flag since we've consumed the data
    fftDataReady = false;

    return (readScope.blockSize1 + readScope.blockSize2) == fftSize;
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

} // namespace yup
