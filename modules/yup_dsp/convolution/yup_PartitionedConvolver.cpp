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

class PartitionedConvolver::FFTLayer
{
public:
    FFTLayer() = default;
    ~FFTLayer() = default;

    FFTLayer (FFTLayer&& other) = default;
    FFTLayer& operator= (FFTLayer&& other) = default;

    void configure (int newHopSize)
    {
        hopSize = newHopSize;
        fftSize = hopSize * 2;

        fftProcessor.setSize (fftSize);
        fftProcessor.setScaling (FFTProcessor<float>::FFTScaling::none);

        overlapBuffer.assign (static_cast<std::size_t> (hopSize), 0.0f);
        timeBuffer.assign (static_cast<std::size_t> (fftSize), 0.0f);
        frequencyBuffer.assign (static_cast<std::size_t> (fftSize), 0.0f);
        tempBuffer.assign (static_cast<std::size_t> (fftSize), 0.0f); // Zero padded time domain input

        fdlIndex = 0;
        configured = true;
    }

    int getHopSize() const { return hopSize; }

    int getFFTSize() const { return fftSize; }

    bool isConfigured() const { return configured; }

    void setImpulseResponse (const float* impulseResponse, std::size_t length, float scaling)
    {
        jassert (configured);

        if (fftSize <= 0 || hopSize <= 0)
        {
            resetState();
            return;
        }

        frequencyPartitions.clear();
        frequencyDelayLine.clear();

        if (length == 0 || impulseResponse == nullptr)
        {
            resetState();
            return;
        }

        const auto numPartitions = (length + static_cast<std::size_t> (hopSize) - 1) / static_cast<std::size_t> (hopSize);
        if (numPartitions == 0)
        {
            resetState();
            return;
        }

        // The unscaled inverse FFT returns fftSize times the convolution: fold 1 / fftSize into the partitions
        const float partitionScaling = scaling / static_cast<float> (fftSize);

        frequencyPartitions.reserve (numPartitions);

        for (std::size_t p = 0; p < numPartitions; ++p)
        {
            std::vector<float> partition;
            partition.resize (static_cast<std::size_t> (fftSize));

            std::fill (tempBuffer.begin(), tempBuffer.end(), 0.0f);

            const std::size_t offset = p * static_cast<std::size_t> (hopSize);
            const std::size_t copyCount = std::min (static_cast<std::size_t> (hopSize), length - offset);

            if (copyCount > 0 && offset < length)
            {
                for (std::size_t i = 0; i < copyCount && offset + i < length; ++i)
                    tempBuffer[i] = impulseResponse[offset + i] * partitionScaling;
            }

            fftProcessor.performRealFFTForwardPacked (tempBuffer.data(), partition.data());

            frequencyPartitions.push_back (std::move (partition));
        }

        frequencyDelayLine.assign (numPartitions, std::vector<float> (static_cast<std::size_t> (fftSize), 0.0f));
        fdlIndex = 0;

        resetState();
    }

    void resetState()
    {
        fdlIndex = 0;

        for (auto& partition : frequencyDelayLine)
            std::fill (partition.begin(), partition.end(), 0.0f);

        std::fill (overlapBuffer.begin(), overlapBuffer.end(), 0.0f);
        std::fill (timeBuffer.begin(), timeBuffer.end(), 0.0f);
        std::fill (frequencyBuffer.begin(), frequencyBuffer.end(), 0.0f);
    }

    void processHop (const float* inputHop, float* outputAccumulator)
    {
        jassert (configured);

        if (frequencyPartitions.empty())
            return;

        // 1) Transform the zero padded input hop straight into the frequency delay line (most recent at fdlIndex)
        fdlIndex = (fdlIndex == 0) ? static_cast<int> (frequencyDelayLine.size()) - 1 : fdlIndex - 1;

        FloatVectorOperations::copy (tempBuffer.data(), inputHop, hopSize);
        FloatVectorOperations::clear (tempBuffer.data() + hopSize, hopSize);
        fftProcessor.performRealFFTForwardPacked (tempBuffer.data(), frequencyDelayLine[static_cast<std::size_t> (fdlIndex)].data());

        // 2) Frequency domain convolution: Y = sum(X[k-p] * H[p])
        FloatVectorOperations::clear (frequencyBuffer.data(), fftSize);

        int xIndex = fdlIndex;
        for (std::size_t p = 0; p < frequencyPartitions.size(); ++p)
        {
            const float* X = frequencyDelayLine[static_cast<std::size_t> (xIndex)].data();
            const float* H = frequencyPartitions[p].data();

            ComplexVectorOperations::multiplyAccumulate (X, H, frequencyBuffer.data(), fftSize / 2, ComplexLayout::packedReal);

            // Move to next older spectrum
            xIndex++;
            if (xIndex >= static_cast<int> (frequencyDelayLine.size()))
                xIndex = 0;
        }

        // 3) Inverse FFT back to time domain
        fftProcessor.performRealFFTInversePacked (frequencyBuffer.data(), timeBuffer.data());

        // 4) Overlap-Add: output first hopSize samples, store last hopSize as overlap
        for (int i = 0; i < hopSize; ++i)
        {
            outputAccumulator[i] += timeBuffer[i] + overlapBuffer[i];
            overlapBuffer[i] = timeBuffer[i + hopSize];
        }
    }

    bool hasImpulseResponse() const { return ! frequencyPartitions.empty(); }

private:
    int hopSize = 0;
    int fftSize = 0;

    FFTProcessor<float> fftProcessor;

    // IR partitions in frequency domain
    std::vector<std::vector<float>> frequencyPartitions;

    // Frequency Delay Line (most recent at fdlIndex)
    std::vector<std::vector<float>> frequencyDelayLine;
    int fdlIndex = 0;

    // Processing buffers
    std::vector<float> overlapBuffer;
    std::vector<float> timeBuffer;
    std::vector<float> frequencyBuffer;
    std::vector<float> tempBuffer;

    bool configured = false;
};

//==============================================================================

class PartitionedConvolver::CircularBuffer
{
public:
    CircularBuffer() = default;

    void resize (std::size_t size)
    {
        buffer.resize (size);
        clear();
    }

    void clear (std::size_t delaySamples = 0)
    {
        jassert (buffer.empty() || delaySamples <= buffer.size()); // empty until the convolver is prepared

        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writeIndex = delaySamples % std::max (buffer.size(), std::size_t (1));
        readIndex = 0;
        availableForRead = std::min (delaySamples, buffer.size());
    }

    std::size_t getAvailableForRead() const { return availableForRead; }

    std::size_t getAvailableForWrite() const { return buffer.size() - availableForRead; }

    std::size_t getSize() const { return buffer.size(); }

    void write (const float* data, std::size_t numSamples)
    {
        jassert (numSamples <= getAvailableForWrite());
        numSamples = std::min (numSamples, getAvailableForWrite());

        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, buffer.size() - writeIndex);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (data, data + beforeWrap, buffer.begin() + writeIndex);
        if (afterWrap > 0)
            std::copy (data + beforeWrap, data + numSamples, buffer.begin());

        writeIndex = (writeIndex + numSamples) % buffer.size();
        availableForRead += numSamples;
    }

    void read (float* data, std::size_t numSamples)
    {
        jassert (numSamples <= getAvailableForRead());
        numSamples = std::min (numSamples, getAvailableForRead());

        if (numSamples == 0)
            return;

        const std::size_t beforeWrap = std::min (numSamples, buffer.size() - readIndex);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (buffer.begin() + readIndex, buffer.begin() + readIndex + beforeWrap, data);
        if (afterWrap > 0)
            std::copy (buffer.begin(), buffer.begin() + afterWrap, data + beforeWrap);

        readIndex = (readIndex + numSamples) % buffer.size();
        availableForRead -= numSamples;
    }

    void peek (float* data, std::size_t numSamples, std::size_t offset = 0) const
    {
        jassert (numSamples + offset <= getAvailableForRead());
        numSamples = std::min (numSamples, getAvailableForRead() - offset);

        if (numSamples == 0)
            return;

        const std::size_t startIndex = (readIndex + offset) % buffer.size();
        const std::size_t beforeWrap = std::min (numSamples, buffer.size() - startIndex);
        const std::size_t afterWrap = numSamples - beforeWrap;

        std::copy (buffer.begin() + startIndex, buffer.begin() + startIndex + beforeWrap, data);
        if (afterWrap > 0)
            std::copy (buffer.begin(), buffer.begin() + afterWrap, data + beforeWrap);
    }

    void skip (std::size_t numSamples)
    {
        jassert (numSamples <= getAvailableForRead());
        numSamples = std::min (numSamples, getAvailableForRead());

        readIndex = (readIndex + numSamples) % buffer.size();
        availableForRead -= numSamples;
    }

private:
    std::vector<float> buffer;
    std::size_t writeIndex = 0;
    std::size_t readIndex = 0;
    std::size_t availableForRead = 0;
};

//==============================================================================

class PartitionedConvolver::Impl
{
public:
    Impl() = default;
    ~Impl() = default;

    void configureLayers (std::size_t directFIRCoefficients, const std::vector<LayerSpec>& newLayers)
    {
        layers.clear();
        layers.resize (newLayers.size());
        layerHopSizes.resize (newLayers.size());
        layerOffsets.resize (newLayers.size());

        std::size_t offset = directFIRCoefficients;
        std::size_t maximumHopSize = 0;

        for (std::size_t i = 0; i < newLayers.size(); ++i)
        {
            const auto hopSize = static_cast<std::size_t> (newLayers[i].hopSize);

            layers[i].configure (newLayers[i].hopSize);
            layerHopSizes[i] = newLayers[i].hopSize;
            offset = std::max (offset, hopSize);
            layerOffsets[i] = offset;

            maximumHopSize = std::max (maximumHopSize, hopSize);
        }

        directFIRCoefficientCount = layerOffsets.empty() ? directFIRCoefficients : layerOffsets.front();
        maxHopSize = maximumHopSize;

        // Resize per-layer circular buffers - will be allocated in prepare()
        layerInputBuffers.resize (layers.size());
        layerOutputBuffers.resize (layers.size());

        layerTempOutput.clear();
        tempLayerHop.clear();

        // Clear working buffers - will be allocated in prepare()
        workingOutput.clear();
        layerBlockOutput.clear();

        isPrepared = false;
    }

    void prepare (std::size_t maxBlockSize)
    {
        this->maxBlockSize = maxBlockSize;

        for (std::size_t i = 0; i < layerInputBuffers.size(); ++i)
        {
            const std::size_t layerHopSize = static_cast<std::size_t> (layers[i].getHopSize());

            // Input: less than a hop left over plus the incoming block
            layerInputBuffers[i].resize (layerHopSize + maxBlockSize);

            // Output: the layer offset as a delay (the output is read that many samples after its input was
            // written), plus the hops a block can complete
            layerOutputBuffers[i].resize (layerOffsets[i] + layerHopSize + maxBlockSize);
        }

        // Allocate temp buffers
        if (maxHopSize > 0)
        {
            layerTempOutput.resize (maxHopSize);
            tempLayerHop.resize (maxHopSize);
        }

        // Allocate working buffers
        workingOutput.resize (maxBlockSize);
        layerBlockOutput.resize (maxBlockSize);

        resetStateUnsafe();

        isPrepared = true;
    }

    std::size_t trimSilenceFromEnd (const float* impulseResponse, std::size_t length, float thresholdDb)
    {
        if (impulseResponse == nullptr || length == 0)
            return 0;

        const float threshold = std::pow (10.0f, thresholdDb / 20.0f);

        // For short IRs, use smaller window size and be more conservative
        const std::size_t minRetainLength = std::max (std::size_t (32), length / 4);
        const std::size_t windowSize = std::min (std::size_t (1024), std::max (std::size_t (64), length / 20));

        // First pass: scan from end to find significant content
        std::size_t significantContentEnd = 0;
        for (std::size_t i = length; i > windowSize; i -= windowSize)
        {
            const std::size_t startIdx = i - windowSize;
            const std::size_t endIdx = std::min (i, length);
            const std::size_t samples = endIdx - startIdx;

            if (samples == 0)
                continue;

            float rmsSquared = 0.0f;
            for (std::size_t j = startIdx; j < endIdx; ++j)
                rmsSquared += impulseResponse[j] * impulseResponse[j];

            const float rms = std::sqrt (rmsSquared / static_cast<float> (samples));
            if (rms >= threshold)
            {
                significantContentEnd = endIdx;
                break;
            }
        }

        // If no significant content found, check the beginning more carefully
        if (significantContentEnd == 0)
        {
            const std::size_t checkLength = std::min (minRetainLength, length);

            float rmsSquared = 0.0f;
            for (std::size_t j = 0; j < checkLength; ++j)
                rmsSquared += impulseResponse[j] * impulseResponse[j];

            const float rms = std::sqrt (rmsSquared / static_cast<float> (checkLength));
            if (rms < threshold)
                return 1;
        }

        // Return the found significant content end, but respect minimum for short IRs
        if (length <= 200) // Short IR protection
            return std::max (significantContentEnd, minRetainLength);
        else
            return std::max (significantContentEnd, windowSize);
    }

    void setImpulseResponse (const float* impulseResponse, std::size_t length, const PartitionedConvolver::IRLoadOptions& options)
    {
        DirectFIRFloat newFIR;
        std::vector<FFTLayer> newLayers (layerHopSizes.size());

        // Configured even for an empty impulse response, so the layout survives it
        for (std::size_t i = 0; i < newLayers.size(); ++i)
            newLayers[i].configure (layerHopSizes[i]);

        std::size_t trimmedLength = length;

        // Safety check
        if (impulseResponse != nullptr && trimmedLength > 0)
        {
            // Always apply peak headroom
            float headroomScale = std::pow (10.0f, options.headroomDb / 20.0f);

            // Normalize peaks
            if (options.normalize)
            {
                const auto minMax = FloatVectorOperations::findMinAndMax (impulseResponse, trimmedLength);

                const float peak = std::max (std::abs (minMax.getStart()), std::abs (minMax.getEnd()));
                if (peak > 0.0f)
                    headroomScale /= peak;
            }

            // Trim end silence if requested
            if (options.trimEndSilenceBelowDb)
                trimmedLength = trimSilenceFromEnd (impulseResponse, length, *options.trimEndSilenceBelowDb);

            // Update DirectFIR in-place
            const auto directCoefficientsCount = std::min (directFIRCoefficientCount, trimmedLength);
            newFIR.setCoefficients (impulseResponse, directCoefficientsCount, headroomScale);

            // Update FFT layers: each one convolves the taps from its offset to the next layer's offset
            for (std::size_t i = 0; i < newLayers.size(); ++i)
            {
                auto& layer = newLayers[i];

                const std::size_t start = std::min (layerOffsets[i], trimmedLength);
                const std::size_t end = (i + 1 < newLayers.size()) ? std::min (layerOffsets[i + 1], trimmedLength) : trimmedLength;

                if (start < end)
                    layer.setImpulseResponse (impulseResponse + start, end - start, headroomScale);
                else
                    layer.setImpulseResponse (nullptr, 0, headroomScale);
            }
        }

        {
            SpinLock::ScopedLockType lock (processingLock);

            std::swap (directFIR, newFIR);
            std::swap (layers, newLayers);
            finalImpulseLength = trimmedLength;

            resetStateUnsafe();
        }
    }

    std::size_t getImpulseLength() const
    {
        return finalImpulseLength;
    }

    void reset()
    {
        SpinLock::ScopedLockType lock (processingLock);

        resetStateUnsafe();
    }

    void process (const float* input, float* output, std::size_t numSamples)
    {
        if (numSamples == 0)
            return;

        SpinLock::ScopedLockType lock (processingLock);

        processUnsafe (input, output, numSamples);
    }

private:
    void resetStateUnsafe()
    {
        directFIR.reset();

        for (auto& buffer : layerInputBuffers)
            buffer.clear();

        for (std::size_t i = 0; i < layerOutputBuffers.size(); ++i)
            layerOutputBuffers[i].clear (layerOffsets[i]);

        for (auto& layer : layers)
            layer.resetState();
    }

    void processUnsafe (const float* input, float* output, std::size_t numSamples)
    {
        jassert (isPrepared);
        jassert (numSamples <= maxBlockSize);
        if (! isPrepared || numSamples > maxBlockSize)
            return;

        FloatVectorOperations::clear (workingOutput.data(), numSamples);

        // Process direct FIR (no block size constraints)
        directFIR.process (input, workingOutput.data(), numSamples);
        if (layers.empty())
        {
            FloatVectorOperations::add (output, workingOutput.data(), numSamples);
            return;
        }

        for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex)
        {
            auto& layer = layers[layerIndex];
            if (! layer.hasImpulseResponse())
                continue;

            const auto layerHopSize = static_cast<std::size_t> (layer.getHopSize());
            auto& inputBuffer = layerInputBuffers[layerIndex];
            auto& outputBuffer = layerOutputBuffers[layerIndex];

            inputBuffer.write (input, numSamples);

            while (inputBuffer.getAvailableForRead() >= layerHopSize)
            {
                inputBuffer.read (tempLayerHop.data(), layerHopSize);
                FloatVectorOperations::clear (layerTempOutput.data(), layer.getHopSize());

                layer.processHop (tempLayerHop.data(), layerTempOutput.data());

                outputBuffer.write (layerTempOutput.data(), layerHopSize);
            }

            // The layer offset is at least its hop, so the samples for this block are always ready
            outputBuffer.read (layerBlockOutput.data(), numSamples);
            FloatVectorOperations::add (workingOutput.data(), layerBlockOutput.data(), numSamples);
        }

        // Copy final result to output (accumulate)
        FloatVectorOperations::add (output, workingOutput.data(), numSamples);
    }

private:
    std::size_t directFIRCoefficientCount = 0;
    std::vector<int> layerHopSizes;
    std::vector<std::size_t> layerOffsets;
    std::size_t maxHopSize = 0;
    std::size_t maxBlockSize = 0;
    std::size_t finalImpulseLength = 0;
    bool isPrepared = false;

    DirectFIRFloat directFIR;
    std::vector<FFTLayer> layers;

    // Working buffers
    std::vector<float> workingOutput;
    std::vector<float> layerBlockOutput;

    // Per-layer circular buffering
    std::vector<CircularBuffer> layerInputBuffers;
    std::vector<CircularBuffer> layerOutputBuffers;
    std::vector<float> tempLayerHop;
    std::vector<float> layerTempOutput;

    mutable SpinLock processingLock;
};

//==============================================================================
// PartitionedConvolver implementation
//==============================================================================

PartitionedConvolver::PartitionedConvolver()
    : impl (std::make_unique<Impl>())
{
}

PartitionedConvolver::~PartitionedConvolver() = default;

PartitionedConvolver::PartitionedConvolver (PartitionedConvolver&& other) noexcept
    : impl (std::move (other.impl))
{
}

PartitionedConvolver& PartitionedConvolver::operator= (PartitionedConvolver&& other) noexcept
{
    if (this != &other)
        impl = std::move (other.impl);
    return *this;
}

void PartitionedConvolver::configureLayers (std::size_t directFIRCoefficients, const std::vector<LayerSpec>& layers)
{
    impl->configureLayers (directFIRCoefficients, layers);
}

void PartitionedConvolver::setTypicalLayout (std::size_t directCoefficients, const std::vector<int>& hops)
{
    std::vector<LayerSpec> layerSpecs;
    layerSpecs.reserve (hops.size());

    for (int hop : hops)
    {
        if (hop < 64)
            directCoefficients += static_cast<std::size_t> (hop);
        else
            layerSpecs.push_back ({ nextPowerOfTwo (hop) });
    }

    configureLayers (directCoefficients, layerSpecs);
}

void PartitionedConvolver::setImpulseResponse (const float* impulseResponse, std::size_t length, const IRLoadOptions& options)
{
    impl->setImpulseResponse (impulseResponse, length, options);
}

void PartitionedConvolver::setImpulseResponse (const std::vector<float>& impulseResponse, const IRLoadOptions& options)
{
    setImpulseResponse (impulseResponse.data(), impulseResponse.size(), options);
}

std::size_t PartitionedConvolver::getImpulseLength() const
{
    return impl->getImpulseLength();
}

void PartitionedConvolver::prepare (std::size_t maxBlockSize)
{
    impl->prepare (maxBlockSize);
}

void PartitionedConvolver::reset()
{
    impl->reset();
}

void PartitionedConvolver::process (const float* input, float* output, std::size_t numSamples)
{
    impl->process (input, output, numSamples);
}

} // namespace yup
