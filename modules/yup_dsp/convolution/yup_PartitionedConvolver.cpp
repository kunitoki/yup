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

#include <atomic>
#include <thread>

namespace yup
{

//==============================================================================
// Complex multiply-accumulate for interleaved real/imaginary format
// Y += A * B (complex multiplication)
//==============================================================================
static void complexMultiplyAccumulate (const float* A, const float* B, float* Y, int complexPairs)
{
    for (int i = 0; i < complexPairs; ++i)
    {
        const int ri = i * 2;
        const int ii = ri + 1;
        const float ar = A[ri];
        const float ai = A[ii];
        const float br = B[ri];
        const float bi = B[ii];
        // (ar + j*ai) * (br + j*bi) = (ar*br - ai*bi) + j*(ar*bi + ai*br)
        Y[ri] += ar * br - ai * bi;
        Y[ii] += ar * bi + ai * br;
    }
}

//==============================================================================
// DirectFIR - Brute-force FIR implementation for early taps
//==============================================================================
class PartitionedConvolver::DirectFIR
{
public:
    DirectFIR() = default;

    void setTaps (std::vector<float> taps, float scaling)
    {
        taps_ = std::move (taps);
        FloatVectorOperations::multiply (taps_.data(), scaling, taps_.size());

        history_.assign (taps_.size(), 0.0f);

        writeIndex_ = 0;
    }

    void reset()
    {
        std::fill (history_.begin(), history_.end(), 0.0f);
        writeIndex_ = 0;
    }

    void process (const float* input, float* output, std::size_t numSamples)
    {
        const std::size_t numTaps = taps_.size();
        if (numTaps == 0) return;

        for (std::size_t i = 0; i < numSamples; ++i)
        {
            history_[writeIndex_] = input[i];

            // Convolution: y[n] = sum(h[m] * x[n-m])
            float sum = 0.0f;
            std::size_t readIndex = writeIndex_;
            for (std::size_t m = 0; m < numTaps; ++m)
            {
                sum += taps_[m] * history_[readIndex];
                if (readIndex == 0)
                    readIndex = numTaps - 1;
                else
                    --readIndex;
            }

            output[i] += sum;

            // Advance circular buffer
            if (++writeIndex_ == numTaps)
                writeIndex_ = 0;
        }
    }

    std::size_t getNumTaps() const { return taps_.size(); }

private:
    std::vector<float> taps_;
    std::vector<float> history_;
    std::size_t writeIndex_ = 0;
};

//==============================================================================
// FFTLayer - Single uniform-partitioned OLA layer
//==============================================================================
class PartitionedConvolver::FFTLayer
{
public:
    FFTLayer() = default;
    ~FFTLayer() = default;

    FFTLayer (FFTLayer&& other) = default;
    FFTLayer& operator= (FFTLayer&& other) = default;

    void configure (int hopSize)
    {
        hopSize_ = hopSize;
        fftSize_ = hopSize * 2;

        fftProcessor_.setSize (fftSize_);
        fftProcessor_.setScaling (FFTProcessor::FFTScaling::asymmetric);

        overlapBuffer_.assign (static_cast<std::size_t> (hopSize_), 0.0f);
        timeBuffer_.assign (static_cast<std::size_t> (fftSize_), 0.0f);
        frequencyBuffer_.assign (static_cast<std::size_t> (fftSize_) * 2, 0.0f);
        tempBuffer_.assign (static_cast<std::size_t> (fftSize_) * 2, 0.0f);  // Must hold complex data for in-place FFT

        fdlIndex_ = 0;
        configured_ = true;
    }

    int getHopSize() const { return hopSize_; }
    int getFFTSize() const { return fftSize_; }
    bool isConfigured() const { return configured_; }

    std::size_t setImpulseResponse (const float* impulseResponse, std::size_t length, float scaling)
    {
        jassert (configured_);

        if (fftSize_ <= 0 || hopSize_ <= 0)
        {
            resetState();
            return 0;
        }

        frequencyPartitions_.clear();
        frequencyDelayLine_.clear();

        if (length == 0 || impulseResponse == nullptr)
        {
            resetState();
            return 0;
        }

        const auto numPartitions = (length + static_cast<std::size_t> (hopSize_) - 1) / static_cast<std::size_t> (hopSize_);
        if (numPartitions == 0)
        {
            resetState();
            return 0;
        }

        std::size_t processedSamples = 0;
        frequencyPartitions_.reserve (numPartitions);

        for (std::size_t p = 0; p < numPartitions; ++p)
        {
            std::vector<float> partition;
            partition.resize (static_cast<std::size_t> (fftSize_) * 2);

            std::fill (tempBuffer_.begin(), tempBuffer_.end(), 0.0f);

            const std::size_t offset = p * static_cast<std::size_t> (hopSize_);
            const std::size_t copyCount = std::min (static_cast<std::size_t> (hopSize_), length - offset);

            if (copyCount > 0 && offset < length)
            {
                for (std::size_t i = 0; i < copyCount && offset + i < length; ++i)
                    tempBuffer_[i] = impulseResponse[offset + i] * scaling;
            }

            fftProcessor_.performRealFFTForward (tempBuffer_.data(), partition.data());

            frequencyPartitions_.push_back (std::move (partition));

            processedSamples += copyCount;
        }

        frequencyDelayLine_.assign (numPartitions, std::vector<float> (static_cast<std::size_t> (fftSize_) * 2, 0.0f));
        fdlIndex_ = 0;

        resetState();

        return processedSamples;
    }

    void resetState()
    {
        fdlIndex_ = 0;

        for (auto& partition : frequencyDelayLine_)
            std::fill (partition.begin(), partition.end(), 0.0f);

        std::fill (overlapBuffer_.begin(), overlapBuffer_.end(), 0.0f);
        std::fill (timeBuffer_.begin(), timeBuffer_.end(), 0.0f);
        std::fill (frequencyBuffer_.begin(), frequencyBuffer_.end(), 0.0f);
    }

    void processHop (const float* inputHop, float* outputAccumulator)
    {
        jassert (configured_);

        if (frequencyPartitions_.empty())
            return;

        // 1) Transform current input hop to frequency domain
        std::fill (tempBuffer_.begin(), tempBuffer_.end(), 0.0f);
        for (int i = 0; i < hopSize_; ++i)
            tempBuffer_[i] = inputHop[i];

        fftProcessor_.performRealFFTForward (tempBuffer_.data(), tempBuffer_.data());

        // 2) Store in frequency delay line (circular buffer) - copy full complex buffer
        fdlIndex_ = (fdlIndex_ == 0) ? static_cast<int> (frequencyDelayLine_.size()) - 1 : fdlIndex_ - 1;
        std::copy (tempBuffer_.begin(), tempBuffer_.begin() + (fftSize_ * 2), frequencyDelayLine_[static_cast<std::size_t> (fdlIndex_)].begin());

        // 3) Frequency domain convolution: Y = sum(X[k-p] * H[p])
        std::fill (frequencyBuffer_.data(), frequencyBuffer_.data() + (fftSize_ * 2), 0.0f);

        int xIndex = fdlIndex_;
        for (std::size_t p = 0; p < frequencyPartitions_.size(); ++p)
        {
            const float* X = frequencyDelayLine_[static_cast<std::size_t> (xIndex)].data();
            const float* H = frequencyPartitions_[p].data();

            // fftSize_/2 gives the number of complex pairs for real FFT
            complexMultiplyAccumulate (X, H, frequencyBuffer_.data(), fftSize_ / 2);

            // Move to next older spectrum
            xIndex++;
            if (xIndex >= static_cast<int> (frequencyDelayLine_.size()))
                xIndex = 0;
        }

        // 4) Inverse FFT back to time domain
        fftProcessor_.performRealFFTInverse (frequencyBuffer_.data(), timeBuffer_.data());

        // 5) Overlap-Add: output first hopSize_ samples, store last hopSize_ as overlap
        for (int i = 0; i < hopSize_; ++i)
        {
            outputAccumulator[i] += timeBuffer_[i] + overlapBuffer_[i];
            overlapBuffer_[i] = timeBuffer_[i + hopSize_];
        }
    }

    bool hasImpulseResponse() const { return !frequencyPartitions_.empty(); }

private:
    int hopSize_ = 0;
    int fftSize_ = 0;

    FFTProcessor fftProcessor_;

    // IR partitions in frequency domain
    std::vector<std::vector<float>> frequencyPartitions_;

    // Frequency Delay Line (most recent at fdlIndex_)
    std::vector<std::vector<float>> frequencyDelayLine_;
    int fdlIndex_ = 0;

    // Processing buffers
    std::vector<float> overlapBuffer_;
    std::vector<float> timeBuffer_;
    std::vector<float> frequencyBuffer_;
    std::vector<float> tempBuffer_;

    bool configured_ = false;
};

//==============================================================================
// PartitionedConvolver::Impl - Implementation details
//==============================================================================
class PartitionedConvolver::Impl
{
public:
    Impl() = default;
    ~Impl() = default;

    void configureLayers (std::size_t directFIRTaps, const std::vector<LayerSpec>& layers)
    {
        directFIRTapCount_ = directFIRTaps;
        layers_.clear();
        layers_.resize (layers.size());

        std::size_t maximumHopSize = 0;

        baseHopSize_ = layers.empty() ? 0 : layers.front().hopSize;
        for (std::size_t i = 0; i < layers.size(); ++i)
        {
            layers_[i].configure (layers[i].hopSize);
            if (i == 0)
                baseHopSize_ = layers[i].hopSize;
            else
                baseHopSize_ = std::min (baseHopSize_, layers[i].hopSize);

            maximumHopSize = std::max (maximumHopSize, static_cast<std::size_t> (layers[i].hopSize));
        }

        maxHopSize_ = maximumHopSize;
        
        // Clear staging buffers - will be allocated in prepare()
        inputStaging_.clear();
        outputStaging_.clear();
        inputCarry_.clear();

        // Clear per-layer accumulators - will be allocated in prepare()
        layerInputAccumulators_.assign (layers.size(), std::vector<float>());
        layerOutputCarries_.assign (layers.size(), std::vector<float>());
        
        layerTempOutput_.clear();
        tempLayerHop_.clear();
        
        // Clear working buffers - will be allocated in prepare()
        workingInput_.clear();
        workingOutput_.clear();
        
        isPrepared_ = false;
    }

    void prepare (std::size_t maxBlockSize)
    {
        maxBlockSize_ = maxBlockSize;
        
        // Calculate buffer sizes based on block size and hop configurations
        const std::size_t maxBufferSize = std::max (maxBlockSize * 4, maxHopSize_ * 16);
        
        // Prepare staging buffers
        inputStaging_.clear();
        outputStaging_.assign (static_cast<std::size_t> (baseHopSize_), 0.0f);
        inputCarry_.clear();
        inputCarry_.reserve (maxBufferSize);

        // Prepare per-layer accumulators
        for (std::size_t i = 0; i < layerInputAccumulators_.size(); ++i)
        {
            layerInputAccumulators_[i].clear();
            layerInputAccumulators_[i].reserve (maxBufferSize);
            layerOutputCarries_[i].clear();
            layerOutputCarries_[i].reserve (maxBufferSize);
        }
        
        // Allocate temp buffers
        if (maxHopSize_ > 0)
        {
            layerTempOutput_.resize (maxHopSize_);
            tempLayerHop_.reserve (maxHopSize_);
        }
        
        // Allocate working buffers
        workingInput_.resize (maxBlockSize);
        workingOutput_.resize (maxBlockSize);
        
        isPrepared_ = true;
    }

    void setImpulseResponse (const float* impulseResponse, std::size_t length, const PartitionedConvolver::IRLoadOptions& options)
    {
        DirectFIR newFIR;
        std::vector<FFTLayer> newLayers (layers_.size());

        // Safety check
        if (impulseResponse != nullptr && length > 0)
        {
            // Always apply peak headroom
            float headroomScale = std::pow (10.0f, options.headroomDb / 20.0f);
            if (options.normalize)
            {
                const auto minMax = FloatVectorOperations::findMinAndMax (impulseResponse, length);

                const float peak = std::max (std::abs (minMax.getStart()), std::abs (minMax.getEnd()));
                if (peak > 0.0f)
                    headroomScale /= peak;
            }

            // Update DirectFIR in-place
            std::vector<float> directTaps;

            const auto directTapsCount = std::min (directFIRTapCount_, length);
            if (directTapsCount > 0)
            {
                directTaps.reserve (directTapsCount);
                directTaps.assign (impulseResponse, impulseResponse + directTapsCount);
            }

            newFIR.setTaps (std::move (directTaps), headroomScale);

            // Update FFT layers
            std::size_t consumed = directTapsCount;
            for (std::size_t i = 0; i < newLayers.size(); ++i)
            {
                auto& layer = newLayers[i];
                layer.configure (layers_[i].getHopSize());

                const std::size_t remaining = (consumed < length) ? (length - consumed) : 0;
                if (remaining == 0)
                {
                    layer.setImpulseResponse (nullptr, 0, headroomScale);
                    continue;
                }

                consumed += layer.setImpulseResponse (impulseResponse + consumed, remaining, headroomScale);
            }
        }

        {
            SpinLock::ScopedLockType lock (processingLock_);

            directFIR_ = std::move (newFIR);
            layers_ = std::move (newLayers);

            resetStateUnsafe();
        }
    }

    void reset()
    {
        SpinLock::ScopedLockType lock (processingLock_);

        resetStateUnsafe();
    }

    void process (const float* input, float* output, std::size_t numSamples)
    {
        if (numSamples == 0) return;

        SpinLock::ScopedLockType lock (processingLock_);

        processUnsafe (input, output, numSamples);
    }

private:
    void resetStateUnsafe()
    {
        directFIR_.reset();
        inputStaging_.clear();
        std::fill (outputStaging_.begin(), outputStaging_.end(), 0.0f);
        inputCarry_.clear();

        for (auto& acc : layerInputAccumulators_)
            acc.clear();

        for (auto& carry : layerOutputCarries_)
            carry.clear();

        for (auto& layer : layers_)
            layer.resetState();
    }

    void processUnsafe (const float* input, float* output, std::size_t numSamples)
    {
        // Ensure prepare() was called
        jassert (isPrepared_);
        jassert (numSamples <= maxBlockSize_);
        
        if (!isPrepared_ || numSamples > maxBlockSize_)
            return; // Fail gracefully in release builds

        FloatVectorOperations::copy (workingInput_.data(), input, numSamples);
        FloatVectorOperations::clear (workingOutput_.data(), numSamples);

        // Process direct FIR (no block size constraints)
        directFIR_.process (workingInput_.data(), workingOutput_.data(), numSamples);
        if (layers_.empty())
        {
            FloatVectorOperations::add (output, workingOutput_.data(), numSamples);
            return;
        }

        // Process FFT layers with hop-based processing
        safeAppendToBuffer (inputCarry_, workingInput_.data(), numSamples);

        std::size_t outputSamplesProduced = 0;
        while (inputCarry_.size() >= static_cast<std::size_t> (baseHopSize_))
        {
            const std::size_t hopSize = static_cast<std::size_t> (baseHopSize_);
            inputStaging_.assign (inputCarry_.begin(), inputCarry_.begin() + hopSize);

            FloatVectorOperations::clear (outputStaging_.data(), outputStaging_.size());

            for (std::size_t layerIndex = 0; layerIndex < layers_.size(); ++layerIndex)
            {
                auto& layer = layers_[layerIndex];
                const int layerHopSize = layer.getHopSize();

                safeAppendToBuffer (layerInputAccumulators_[layerIndex], inputStaging_.data(), hopSize);

                while (layerInputAccumulators_[layerIndex].size() >= static_cast<std::size_t> (layerHopSize))
                {
                    tempLayerHop_.assign (layerInputAccumulators_[layerIndex].begin(),
                                          layerInputAccumulators_[layerIndex].begin() + layerHopSize);

                    FloatVectorOperations::clear (layerTempOutput_.data(), layerHopSize);

                    if (layer.hasImpulseResponse())
                        layer.processHop (tempLayerHop_.data(), layerTempOutput_.data());

                    safeAppendToBuffer (layerOutputCarries_[layerIndex], layerTempOutput_.data(), static_cast<std::size_t> (layerHopSize));

                    layerInputAccumulators_[layerIndex].erase (
                        layerInputAccumulators_[layerIndex].begin(),
                        layerInputAccumulators_[layerIndex].begin() + layerHopSize);
                }

                if (layerOutputCarries_[layerIndex].size() >= hopSize)
                {
                    FloatVectorOperations::add (outputStaging_.data(), layerOutputCarries_[layerIndex].data(), hopSize);

                    layerOutputCarries_[layerIndex].erase (
                        layerOutputCarries_[layerIndex].begin(),
                        layerOutputCarries_[layerIndex].begin() + hopSize);
                }
            }

            // Add staging output to main output
            const std::size_t samplesToWrite = std::min (hopSize, numSamples - outputSamplesProduced);
            FloatVectorOperations::add (workingOutput_.data() + outputSamplesProduced, outputStaging_.data(), samplesToWrite);
            outputSamplesProduced += samplesToWrite;

            // Remove processed input from carry buffer
            inputCarry_.erase (inputCarry_.begin(), inputCarry_.begin() + hopSize);
        }

        // Copy final result to output (accumulate)
        FloatVectorOperations::add (output, workingOutput_.data(), numSamples);
    }

private:

    void safeAppendToBuffer (std::vector<float>& buffer, const float* data, std::size_t numSamples)
    {
        const std::size_t oldSize = buffer.size();
        const std::size_t newSize = oldSize + numSamples;

        // Ensure we never exceed the reserved capacity to avoid allocations
        jassert (newSize <= buffer.capacity());
        if (newSize > buffer.capacity())
        {
            // Truncate to prevent allocation - this is a safety measure
            const std::size_t maxSamples = buffer.capacity() - oldSize;
            if (maxSamples > 0)
            {
                buffer.resize (buffer.capacity());
                std::copy (data, data + maxSamples, buffer.begin() + oldSize);
            }
            return;
        }

        buffer.resize (newSize);
        std::copy (data, data + numSamples, buffer.begin() + oldSize);
    }

    std::size_t directFIRTapCount_ = 0;
    int baseHopSize_ = 0;
    std::size_t maxHopSize_ = 0;
    std::size_t maxBlockSize_ = 0;
    bool isPrepared_ = false;

    DirectFIR directFIR_;
    std::vector<FFTLayer> layers_;

    // Working buffers
    std::vector<float> workingInput_;
    std::vector<float> workingOutput_;

    // Staging for hop-based processing
    std::vector<float> inputCarry_;
    std::vector<float> inputStaging_;
    std::vector<float> outputStaging_;

    // Per-layer buffering
    std::vector<std::vector<float>> layerInputAccumulators_;
    std::vector<std::vector<float>> layerOutputCarries_;
    std::vector<float> tempLayerHop_;
    std::vector<float> layerTempOutput_;

    mutable SpinLock processingLock_;
};

//==============================================================================
// PartitionedConvolver implementation
//==============================================================================

PartitionedConvolver::PartitionedConvolver()
    : pImpl (std::make_unique<Impl>())
{
}

PartitionedConvolver::~PartitionedConvolver() = default;

PartitionedConvolver::PartitionedConvolver (PartitionedConvolver&& other) noexcept
    : pImpl (std::move (other.pImpl))
{
}

PartitionedConvolver& PartitionedConvolver::operator= (PartitionedConvolver&& other) noexcept
{
    if (this != &other)
        pImpl = std::move (other.pImpl);
    return *this;
}

void PartitionedConvolver::configureLayers (std::size_t directFIRTaps, const std::vector<LayerSpec>& layers)
{
    pImpl->configureLayers (directFIRTaps, layers);
}

void PartitionedConvolver::setTypicalLayout (std::size_t directTaps, const std::vector<int>& hops)
{
    std::vector<LayerSpec> layerSpecs;
    layerSpecs.reserve (hops.size());

    for (int hop : hops)
        layerSpecs.push_back ({hop});

    configureLayers (directTaps, layerSpecs);
}

void PartitionedConvolver::setImpulseResponse (const float* impulseResponse, std::size_t length, const IRLoadOptions& options)
{
    pImpl->setImpulseResponse (impulseResponse, length, options);
}

void PartitionedConvolver::setImpulseResponse (const std::vector<float>& impulseResponse, const IRLoadOptions& options)
{
    setImpulseResponse (impulseResponse.data(), impulseResponse.size(), options);
}

void PartitionedConvolver::prepare (std::size_t maxBlockSize)
{
    pImpl->prepare (maxBlockSize);
}

void PartitionedConvolver::reset()
{
    pImpl->reset();
}

void PartitionedConvolver::process (const float* input, float* output, std::size_t numSamples)
{
    pImpl->process (input, output, numSamples);
}

} // namespace yup
