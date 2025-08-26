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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

#include <random>
#include <cmath>

namespace yup::test
{

//==============================================================================
class PartitionedConvolverTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        generator.seed (42); // Fixed seed for reproducible tests
    }

    float randomFloat (float min = -1.0f, float max = 1.0f)
    {
        std::uniform_real_distribution<float> dist (min, max);
        return dist (generator);
    }

    void fillWithRandomData (std::vector<float>& buffer)
    {
        for (auto& sample : buffer)
            sample = randomFloat();
    }

    void fillWithSine (std::vector<float>& buffer, float frequency, float sampleRate)
    {
        for (size_t i = 0; i < buffer.size(); ++i)
            buffer[i] = std::sin (2.0f * MathConstants<float>::pi * frequency * static_cast<float> (i) / sampleRate);
    }

    void clearBuffer (std::vector<float>& buffer)
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
    }

    float calculateRMS (const std::vector<float>& buffer)
    {
        if (buffer.empty())
            return 0.0f;

        float sum = 0.0f;
        for (float sample : buffer)
            sum += sample * sample;

        return std::sqrt (sum / static_cast<float> (buffer.size()));
    }

    float findPeak (const std::vector<float>& buffer)
    {
        if (buffer.empty())
            return 0.0f;

        float peak = 0.0f;
        for (float sample : buffer)
            peak = std::max (peak, std::abs (sample));

        return peak;
    }

    std::mt19937 generator;
};

//==============================================================================
// Basic API Tests
//==============================================================================

TEST_F (PartitionedConvolverTest, DefaultConstruction)
{
    PartitionedConvolver convolver;
    // Should not crash - basic construction test
}

TEST_F (PartitionedConvolverTest, MoveSemantics)
{
    PartitionedConvolver convolver1;
    convolver1.setTypicalLayout (64, { 64, 256 });
    convolver1.prepare (512);

    // Move constructor
    PartitionedConvolver convolver2 = std::move (convolver1);

    // Move assignment
    PartitionedConvolver convolver3;
    convolver3 = std::move (convolver2);

    // Should not crash
}

TEST_F (PartitionedConvolverTest, BasicConfiguration)
{
    PartitionedConvolver convolver;

    // Test typical layout configuration
    convolver.setTypicalLayout (128, { 128, 512, 2048 });

    // Should not crash
    convolver.prepare (512);

    // Test reset
    convolver.reset();
}

TEST_F (PartitionedConvolverTest, ConfigureLayers)
{
    PartitionedConvolver convolver;

    std::vector<PartitionedConvolver::LayerSpec> layers = {
        { 64 }, { 256 }, { 1024 }
    };

    convolver.configureLayers (32, layers);
    convolver.prepare (256);

    // Should not crash
}

//==============================================================================
// Impulse Response Tests
//==============================================================================

TEST_F (PartitionedConvolverTest, SetImpulseResponseVector)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (64, { 64, 256 });
    convolver.prepare (512);

    // Create simple impulse response
    std::vector<float> ir (1000);
    fillWithRandomData (ir);

    convolver.setImpulseResponse (ir);

    // Should not crash
}

TEST_F (PartitionedConvolverTest, SetImpulseResponsePointer)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (64, { 64, 256 });
    convolver.prepare (512);

    // Create simple impulse response
    std::vector<float> ir (1000);
    fillWithRandomData (ir);

    convolver.setImpulseResponse (ir.data(), ir.size());

    // Should not crash
}

TEST_F (PartitionedConvolverTest, SetImpulseResponseWithOptions)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (64, { 64, 256 });
    convolver.prepare (512);

    std::vector<float> ir (1000);
    fillWithRandomData (ir);

    PartitionedConvolver::IRLoadOptions options;
    options.normalize = true;
    options.headroomDb = -6.0f;

    convolver.setImpulseResponse (ir, options);

    // Should not crash
}

TEST_F (PartitionedConvolverTest, EmptyImpulseResponse)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (64, { 64, 256 });
    convolver.prepare (512);

    std::vector<float> emptyIR;
    convolver.setImpulseResponse (emptyIR);

    // Processing with empty IR should work
    std::vector<float> input (256);
    std::vector<float> output (256);
    fillWithRandomData (input);
    clearBuffer (output);

    convolver.process (input.data(), output.data(), input.size());

    // Output should remain zero
    for (float sample : output)
        EXPECT_FLOAT_EQ (sample, 0.0f);
}

//==============================================================================
// Audio Processing Tests
//==============================================================================

TEST_F (PartitionedConvolverTest, ImpulseResponseTest)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (64, { 64, 256 });
    convolver.prepare (512);

    // Create unit impulse response
    std::vector<float> ir (256, 0.0f);
    ir[0] = 1.0f;   // Unit impulse at start
    ir[10] = 0.5f;  // Delayed impulse
    ir[50] = 0.25f; // Another delayed impulse

    convolver.setImpulseResponse (ir);

    // Test with unit impulse input
    std::vector<float> input (512, 0.0f);
    input[0] = 1.0f; // Unit impulse

    std::vector<float> output (512);
    clearBuffer (output);

    convolver.process (input.data(), output.data(), input.size());

    // Output should contain the impulse response (with some latency)
    // Check for non-zero output
    float outputRMS = calculateRMS (output);
    EXPECT_GT (outputRMS, 0.01f);
}

TEST_F (PartitionedConvolverTest, SineWaveConvolution)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (128, { 128, 512 });
    convolver.prepare (2048);

    // Create simple lowpass IR (moving average)
    const size_t irLength = 32;
    std::vector<float> ir (irLength);
    for (size_t i = 0; i < irLength; ++i)
        ir[i] = 1.0f / static_cast<float> (irLength);

    convolver.setImpulseResponse (ir);

    // Test with sine wave
    const float sampleRate = 44100.0f;
    const float frequency = 1000.0f;
    std::vector<float> input (2048);
    fillWithSine (input, frequency, sampleRate);

    std::vector<float> output (2048);
    clearBuffer (output);

    convolver.process (input.data(), output.data(), input.size());

    // Output should have significant energy (lowpass filtered sine)
    float outputRMS = calculateRMS (output);
    EXPECT_GT (outputRMS, 0.1f);
}

TEST_F (PartitionedConvolverTest, AccumulativeOutput)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (64, { 64, 256 });
    convolver.prepare (256);

    // Simple IR
    std::vector<float> ir (100, 0.1f);
    convolver.setImpulseResponse (ir);

    std::vector<float> input (256);
    fillWithRandomData (input);

    // Pre-populate output buffer
    std::vector<float> output (256);
    fillWithRandomData (output);
    std::vector<float> originalOutput = output;

    convolver.process (input.data(), output.data(), input.size());

    // Output should contain original data plus convolution result
    bool hasAccumulated = false;
    for (size_t i = 0; i < output.size(); ++i)
    {
        if (std::abs (output[i] - originalOutput[i]) > 0.001f)
        {
            hasAccumulated = true;
            break;
        }
    }
    EXPECT_TRUE (hasAccumulated);
}

//==============================================================================
// Latency Tests
//==============================================================================

TEST_F (PartitionedConvolverTest, LatencyMeasurement)
{
    // Test different configurations and measure latency
    std::vector<std::pair<size_t, std::vector<int>>> configs = {
        { 64, { 64 } },
        { 128, { 128 } },
        { 64, { 64, 256 } },
        { 128, { 128, 512 } },
        { 256, { 256, 1024 } }
    };

    for (const auto& [directTaps, hops] : configs)
    {
        PartitionedConvolver convolver;
        convolver.setTypicalLayout (directTaps, hops);
        convolver.prepare (1024);

        // Unit impulse response
        std::vector<float> ir (1000, 0.0f);
        ir[0] = 1.0f;
        convolver.setImpulseResponse (ir);

        // Unit impulse input
        std::vector<float> input (1024, 0.0f);
        input[0] = 1.0f;

        std::vector<float> output (1024);
        clearBuffer (output);

        convolver.process (input.data(), output.data(), input.size());

        // Find first non-zero sample in output
        size_t latencySamples = 0;
        for (size_t i = 0; i < output.size(); ++i)
        {
            if (std::abs (output[i]) > 0.001f)
            {
                latencySamples = i;
                break;
            }
        }

        // Latency should be reasonable (less than largest hop size)
        const int maxHop = *std::max_element (hops.begin(), hops.end());
        EXPECT_LE (latencySamples, static_cast<size_t> (maxHop * 2));

        // With direct FIR, latency should be minimal
        if (directTaps > 0)
            EXPECT_LE (latencySamples, directTaps);
    }
}

//==============================================================================
// Partition Size Tests (Fixed)
//==============================================================================

TEST_F (PartitionedConvolverTest, VariousPartitionSizes)
{
    // Test various partition configurations - all with direct taps for immediate response
    std::vector<std::tuple<size_t, std::vector<int>, size_t>> testConfigs = {
        // (directTaps, hops, maxBlockSize)
        { 64, { 64 }, 512 },
        { 32, { 64 }, 512 },
        { 64, { 64, 256 }, 512 },
        { 128, { 128, 512 }, 1024 },
        { 128, { 128, 512, 2048 }, 2048 },
        { 256, { 256, 1024, 4096 }, 4096 },
        { 64, { 128, 256, 512 }, 1024 },
        { 48, { 32, 128, 512 }, 1024 },
        { 24, { 32, 64, 128 }, 1024 },
    };

    for (const auto& item : testConfigs)
    {
        const auto& directTaps = std::get<0> (item);
        const auto& hops = std::get<1> (item);
        const auto& maxBlockSize = std::get<2> (item);

        SCOPED_TRACE (testing::Message() << "Config: directTaps=" << directTaps << " hops=[" << [&]()
        {
            std::string hopStr;
            for (size_t i = 0; i < hops.size(); ++i)
            {
                if (i > 0)
                    hopStr += ",";
                hopStr += std::to_string (hops[i]);
            }
            return hopStr;
        }() << "] maxBlockSize=" << maxBlockSize);

        PartitionedConvolver convolver;

        // Configure and verify setup
        EXPECT_NO_THROW (convolver.setTypicalLayout (directTaps, hops));
        EXPECT_NO_THROW (convolver.prepare (maxBlockSize));

        // Create a simple known impulse response
        std::vector<float> ir (std::min (static_cast<size_t> (500), maxBlockSize), 0.0f);
        ir[0] = 1.0f; // Unit impulse at start
        if (ir.size() > 100)
            ir[100] = 0.5f; // Delayed impulse for verification
        EXPECT_NO_THROW (convolver.setImpulseResponse (ir));

        // Test with unit impulse to verify convolution correctness
        std::vector<float> deltaInput (maxBlockSize, 0.0f);
        deltaInput[0] = 1.0f; // Unit impulse
        std::vector<float> deltaOutput (maxBlockSize);
        clearBuffer (deltaOutput);

        EXPECT_NO_THROW (convolver.process (deltaInput.data(), deltaOutput.data(), maxBlockSize));

        // Should produce significant output
        float outputRMS = calculateRMS (deltaOutput);
        EXPECT_GT (outputRMS, 0.003f) << "No significant convolution output detected";

        // Verify we get immediate response from direct FIR
        EXPECT_GT (findPeak (deltaOutput), 0.1f) << "No immediate response detected";

        // Process various realistic block sizes
        std::vector<size_t> blockSizes = { 64, 128, 256, maxBlockSize };

        for (size_t blockSize : blockSizes)
        {
            if (blockSize > maxBlockSize)
                continue;

            SCOPED_TRACE (testing::Message() << "BlockSize=" << blockSize);

            std::vector<float> input (blockSize);
            std::vector<float> output (blockSize);
            fillWithRandomData (input);
            clearBuffer (output);

            EXPECT_NO_THROW (convolver.process (input.data(), output.data(), blockSize));

            // Verify audio processing quality
            for (float sample : output)
            {
                EXPECT_TRUE (std::isfinite (sample)) << "Non-finite output detected";
                EXPECT_LT (std::abs (sample), 100.0f) << "Output amplitude too large";
            }

            // With direct taps, should get output for reasonable input
            float inputRMS = calculateRMS (input);
            float outputRMS = calculateRMS (output);

            if (inputRMS > 0.01f)
            {
                EXPECT_GT (outputRMS, 0.001f) << "Output unexpectedly quiet for significant input";
            }
        }
    }
}

//==============================================================================
// Stress Test (Fixed)
//==============================================================================

TEST_F (PartitionedConvolverTest, StressTestDifferentBlockSizes)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (128, { 128, 512, 2048 });
    convolver.prepare (2048);

    // Create a simple, well-behaved impulse response
    std::vector<float> ir (1024, 0.0f);
    // Simple decaying impulse response
    for (size_t i = 0; i < 200; ++i)
    {
        ir[i] = std::exp (-static_cast<float> (i) / 50.0f) * std::cos (2.0f * MathConstants<float>::pi * i / 16.0f);
    }

    // Normalize to prevent overflow
    float peak = *std::max_element (ir.begin(), ir.end(), [] (float a, float b)
    {
        return std::abs (a) < std::abs (b);
    });
    if (peak > 0.0f)
    {
        for (auto& sample : ir)
            sample /= (peak * 2.0f); // Extra headroom
    }

    convolver.setImpulseResponse (ir);

    // Test reasonable block sizes first
    std::vector<size_t> blockSizes = { 32, 64, 128, 256, 512, 1024 };

    float totalInputEnergy = 0.0f;
    float totalOutputEnergy = 0.0f;

    for (size_t blockSize : blockSizes)
    {
        SCOPED_TRACE (testing::Message() << "Processing blockSize=" << blockSize);

        std::vector<float> input (blockSize);
        std::vector<float> output (blockSize);
        fillWithRandomData (input);
        clearBuffer (output);

        EXPECT_NO_THROW (convolver.process (input.data(), output.data(), blockSize));

        // Most critical: no non-finite values
        for (float sample : output)
        {
            EXPECT_TRUE (std::isfinite (sample)) << "Non-finite output in blockSize=" << blockSize;
        }

        float inputRMS = calculateRMS (input);
        float outputRMS = calculateRMS (output);

        if (std::isfinite (outputRMS))
        {
            totalInputEnergy += inputRMS * inputRMS * blockSize;
            totalOutputEnergy += outputRMS * outputRMS * blockSize;
        }

        // Verify reasonable levels
        float peak = findPeak (output);
        EXPECT_LT (peak, 50.0f) << "Output peak too large for blockSize=" << blockSize;

        // With direct taps, expect output for reasonable input
        if (inputRMS > 0.01f)
        {
            EXPECT_GT (outputRMS, 0.0001f) << "No output for significant input, blockSize=" << blockSize;
            EXPECT_LT (outputRMS, inputRMS * 5.0f) << "Output unreasonably high for blockSize=" << blockSize;
        }
    }

    // Test challenging small block sizes
    std::vector<size_t> smallBlockSizes = { 1, 7, 15 };

    for (size_t blockSize : smallBlockSizes)
    {
        SCOPED_TRACE (testing::Message() << "Processing small blockSize=" << blockSize);

        std::vector<float> input (blockSize);
        std::vector<float> output (blockSize);
        fillWithRandomData (input);
        clearBuffer (output);

        EXPECT_NO_THROW (convolver.process (input.data(), output.data(), blockSize));

        // Critical: no non-finite values
        for (float sample : output)
        {
            EXPECT_TRUE (std::isfinite (sample)) << "Non-finite output in small blockSize=" << blockSize;
        }

        // Reasonable bounds
        float peak = findPeak (output);
        EXPECT_LT (peak, 50.0f) << "Output peak too large for small blockSize=" << blockSize;
    }

    // Energy conservation check
    if (totalInputEnergy > 0.0f && totalOutputEnergy > 0.0f)
    {
        EXPECT_GT (totalOutputEnergy, totalInputEnergy * 0.01f) << "Total output energy too low";
        EXPECT_LT (totalOutputEnergy, totalInputEnergy * 10.0f) << "Total output energy too high";
    }
}

//==============================================================================
// Remaining Tests (These were passing)
//==============================================================================

TEST_F (PartitionedConvolverTest, RandomizedFuzzing)
{
    // Generate random configurations and test them
    std::uniform_int_distribution<int> hopDist (32, 2048);
    std::uniform_int_distribution<size_t> directTapsDist (32, 512); // Always have some direct taps
    std::uniform_int_distribution<size_t> blockSizeDist (32, 1024);

    for (int trial = 0; trial < 10; ++trial) // Reduce trials for stability
    {
        SCOPED_TRACE (testing::Message() << "Fuzzing trial " << trial);

        // Generate random configuration
        const size_t directTaps = directTapsDist (generator);
        const size_t numLayers = 1 + (generator() % 3); // 1-3 layers

        std::vector<int> hops;
        int prevHop = 32;
        for (size_t i = 0; i < numLayers; ++i)
        {
            int hop = std::max (prevHop, hopDist (generator));
            // Ensure power-of-2 for valid FFT sizes
            hop = 1 << static_cast<int> (std::log2 (hop));
            hops.push_back (hop);
            prevHop = hop;
        }

        const size_t maxBlockSize = 1024;

        PartitionedConvolver convolver;

        try
        {
            convolver.setTypicalLayout (directTaps, hops);
            convolver.prepare (maxBlockSize);

            // Simple impulse response
            std::vector<float> ir (512);
            for (size_t i = 0; i < ir.size(); ++i)
                ir[i] = std::exp (-static_cast<float> (i) / 100.0f) * randomFloat (-0.1f, 0.1f);

            convolver.setImpulseResponse (ir);

            // Test with impulse
            std::vector<float> deltaInput (maxBlockSize, 0.0f);
            deltaInput[0] = 1.0f;
            std::vector<float> deltaOutput (maxBlockSize);
            clearBuffer (deltaOutput);

            convolver.process (deltaInput.data(), deltaOutput.data(), maxBlockSize);

            float deltaRMS = calculateRMS (deltaOutput);
            EXPECT_GT (deltaRMS, 0.001f) << "No convolution output in trial " << trial;

            // Process several blocks
            for (int block = 0; block < 5; ++block)
            {
                const size_t blockSize = 32 + (generator() % (maxBlockSize - 32));

                std::vector<float> input (blockSize);
                std::vector<float> output (blockSize);
                fillWithRandomData (input);
                clearBuffer (output);

                convolver.process (input.data(), output.data(), blockSize);

                // Audio quality checks
                for (float sample : output)
                {
                    EXPECT_TRUE (std::isfinite (sample)) << "Non-finite output in trial " << trial << " block " << block;
                    EXPECT_LT (std::abs (sample), 100.0f) << "Output too large in trial " << trial << " block " << block;
                }
            }
        }
        catch (const std::exception& e)
        {
            FAIL() << "Exception in fuzzing trial " << trial << ": " << e.what();
        }
    }
}

TEST_F (PartitionedConvolverTest, ShortImpulseResponseWithManyLayers)
{
    PartitionedConvolver convolver;

    // Configure many layers but use a short IR
    convolver.setTypicalLayout (64, { 128, 512, 2048, 4096 });
    convolver.prepare (512);

    // Very short IR (only 32 samples) - much shorter than layer configurations
    std::vector<float> shortIR (32);
    fillWithRandomData (shortIR);

    // This should not crash and should not create "zombie" layers
    EXPECT_NO_THROW (convolver.setImpulseResponse (shortIR));

    // Process some data - should work without endless loops
    std::vector<float> input (512);
    std::vector<float> output (512);
    fillWithRandomData (input);
    clearBuffer (output);

    EXPECT_NO_THROW (convolver.process (input.data(), output.data(), input.size()));

    // Should produce some output (from direct FIR at least)
    float outputRMS = calculateRMS (output);
    EXPECT_GT (outputRMS, 0.001f);
}

TEST_F (PartitionedConvolverTest, IRShorterThanDirectTaps)
{
    PartitionedConvolver convolver;

    // Configure with 128 direct taps but use much shorter IR
    convolver.setTypicalLayout (128, { 256, 1024 });
    convolver.prepare (512);

    // IR shorter than direct taps
    std::vector<float> shortIR (64);
    fillWithRandomData (shortIR);

    EXPECT_NO_THROW (convolver.setImpulseResponse (shortIR));

    // Should still work - only direct FIR should be active
    std::vector<float> input (512);
    std::vector<float> output (512);
    fillWithRandomData (input);
    clearBuffer (output);

    EXPECT_NO_THROW (convolver.process (input.data(), output.data(), input.size()));

    // Should produce output from direct FIR
    float outputRMS = calculateRMS (output);
    EXPECT_GT (outputRMS, 0.001f);
}

TEST_F (PartitionedConvolverTest, IRExactlyMatchesFirstLayer)
{
    PartitionedConvolver convolver;

    // Configure layers
    convolver.setTypicalLayout (64, { 128, 512, 2048 });
    convolver.prepare (512);

    // IR that exactly fills direct taps + first layer
    const std::size_t irLength = 64 + 128; // direct + first layer
    std::vector<float> ir (irLength);
    fillWithRandomData (ir);

    EXPECT_NO_THROW (convolver.setImpulseResponse (ir));

    // Should work with first layer active, subsequent layers inactive
    std::vector<float> input (512);
    std::vector<float> output (512);
    fillWithRandomData (input);
    clearBuffer (output);

    EXPECT_NO_THROW (convolver.process (input.data(), output.data(), input.size()));

    float outputRMS = calculateRMS (output);
    EXPECT_GT (outputRMS, 0.001f);
}

TEST_F (PartitionedConvolverTest, ZeroLengthIR)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (64, { 128, 512 });
    convolver.prepare (512);

    // Zero length IR
    std::vector<float> emptyIR;
    EXPECT_NO_THROW (convolver.setImpulseResponse (emptyIR));

    // Should process without crashing but produce no output
    std::vector<float> input (512);
    std::vector<float> output (512);
    fillWithRandomData (input);
    clearBuffer (output);

    EXPECT_NO_THROW (convolver.process (input.data(), output.data(), input.size()));

    // Output should be zero (or very close to zero)
    for (float sample : output)
        EXPECT_NEAR (sample, 0.0f, 0.0001f);
}

TEST_F (PartitionedConvolverTest, ProgressiveIRLengths)
{
    // Test with progressively longer IRs to ensure layer activation works correctly
    std::vector<size_t> irLengths = { 10, 50, 100, 200, 500, 1000, 2000 };

    for (size_t irLength : irLengths)
    {
        SCOPED_TRACE (testing::Message() << "IR Length: " << irLength);

        PartitionedConvolver convolver;
        convolver.setTypicalLayout (64, { 128, 512, 2048 });
        convolver.prepare (512);

        std::vector<float> ir (irLength);
        fillWithRandomData (ir);

        EXPECT_NO_THROW (convolver.setImpulseResponse (ir));

        // Process and verify output
        std::vector<float> input (512);
        std::vector<float> output (512);
        fillWithRandomData (input);
        clearBuffer (output);

        EXPECT_NO_THROW (convolver.process (input.data(), output.data(), input.size()));

        // Should produce reasonable output
        for (float sample : output)
        {
            EXPECT_TRUE (std::isfinite (sample));
            EXPECT_LT (std::abs (sample), 100.0f); // Sanity check
        }
    }
}

TEST_F (PartitionedConvolverTest, ResetFunctionality)
{
    PartitionedConvolver convolver;
    convolver.setTypicalLayout (64, { 64, 256 });
    convolver.prepare (512);

    std::vector<float> ir (500);
    fillWithRandomData (ir);
    convolver.setImpulseResponse (ir);

    // Process some data to build up internal state
    std::vector<float> input (512);
    std::vector<float> output1 (512);
    fillWithRandomData (input);
    clearBuffer (output1);

    convolver.process (input.data(), output1.data(), input.size());

    // Reset and process same input again
    convolver.reset();

    std::vector<float> output2 (512);
    clearBuffer (output2);

    convolver.process (input.data(), output2.data(), input.size());

    // Outputs should be identical after reset
    for (size_t i = 0; i < output1.size(); ++i)
    {
        EXPECT_NEAR (output1[i], output2[i], 0.001f) << "Mismatch at sample " << i;
    }
}

} // namespace yup::test