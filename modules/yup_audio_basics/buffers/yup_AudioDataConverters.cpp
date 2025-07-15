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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

YUP_BEGIN_IGNORE_DEPRECATION_WARNINGS

void AudioDataConverters::convertFloatToInt16LE (const float* source, void* dest, int numSamples, int destBytesPerSample)
{
    auto maxVal = (double) 0x7fff;
    auto intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            *unalignedPointerCast<uint16*> (intData) = ByteOrder::swapIfBigEndian ((uint16) (short) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            *unalignedPointerCast<uint16*> (intData) = ByteOrder::swapIfBigEndian ((uint16) (short) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
        }
    }
}

void AudioDataConverters::convertFloatToInt16BE (const float* source, void* dest, int numSamples, int destBytesPerSample)
{
    auto maxVal = (double) 0x7fff;
    auto intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            *unalignedPointerCast<uint16*> (intData) = ByteOrder::swapIfLittleEndian ((uint16) (short) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            *unalignedPointerCast<uint16*> (intData) = ByteOrder::swapIfLittleEndian ((uint16) (short) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
        }
    }
}

void AudioDataConverters::convertFloatToInt24LE (const float* source, void* dest, int numSamples, int destBytesPerSample)
{
    auto maxVal = (double) 0x7fffff;
    auto intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            ByteOrder::littleEndian24BitToChars (roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])), intData);
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            ByteOrder::littleEndian24BitToChars (roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])), intData);
        }
    }
}

void AudioDataConverters::convertFloatToInt24BE (const float* source, void* dest, int numSamples, int destBytesPerSample)
{
    auto maxVal = (double) 0x7fffff;
    auto intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            ByteOrder::bigEndian24BitToChars (roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])), intData);
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            ByteOrder::bigEndian24BitToChars (roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])), intData);
        }
    }
}

void AudioDataConverters::convertFloatToInt32LE (const float* source, void* dest, int numSamples, int destBytesPerSample)
{
    auto maxVal = (double) 0x7fffffff;
    auto intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            *unalignedPointerCast<uint32*> (intData) = ByteOrder::swapIfBigEndian ((uint32) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            *unalignedPointerCast<uint32*> (intData) = ByteOrder::swapIfBigEndian ((uint32) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
        }
    }
}

void AudioDataConverters::convertFloatToInt32BE (const float* source, void* dest, int numSamples, int destBytesPerSample)
{
    auto maxVal = (double) 0x7fffffff;
    auto intData = static_cast<char*> (dest);

    if (dest != (void*) source || destBytesPerSample <= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            *unalignedPointerCast<uint32*> (intData) = ByteOrder::swapIfLittleEndian ((uint32) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
            intData += destBytesPerSample;
        }
    }
    else
    {
        intData += destBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= destBytesPerSample;
            *unalignedPointerCast<uint32*> (intData) = ByteOrder::swapIfLittleEndian ((uint32) roundToInt (jlimit (-maxVal, maxVal, maxVal * source[i])));
        }
    }
}

void AudioDataConverters::convertFloatToFloat32LE (const float* source, void* dest, int numSamples, int destBytesPerSample)
{
    jassert (dest != (void*) source || destBytesPerSample <= 4); // This op can't be performed on in-place data!

    char* d = static_cast<char*> (dest);

    for (int i = 0; i < numSamples; ++i)
    {
        *unalignedPointerCast<float*> (d) = source[i];

#if YUP_BIG_ENDIAN
        *unalignedPointerCast<uint32*> (d) = ByteOrder::swap (*unalignedPointerCast<uint32*> (d));
#endif

        d += destBytesPerSample;
    }
}

void AudioDataConverters::convertFloatToFloat32BE (const float* source, void* dest, int numSamples, int destBytesPerSample)
{
    jassert (dest != (void*) source || destBytesPerSample <= 4); // This op can't be performed on in-place data!

    auto d = static_cast<char*> (dest);

    for (int i = 0; i < numSamples; ++i)
    {
        *unalignedPointerCast<float*> (d) = source[i];

#if YUP_LITTLE_ENDIAN
        *unalignedPointerCast<uint32*> (d) = ByteOrder::swap (*unalignedPointerCast<uint32*> (d));
#endif

        d += destBytesPerSample;
    }
}

//==============================================================================
void AudioDataConverters::convertInt16LEToFloat (const void* source, float* dest, int numSamples, int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fff;
    auto intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (short) ByteOrder::swapIfBigEndian (*unalignedPointerCast<const uint16*> (intData));
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (short) ByteOrder::swapIfBigEndian (*unalignedPointerCast<const uint16*> (intData));
        }
    }
}

void AudioDataConverters::convertInt16BEToFloat (const void* source, float* dest, int numSamples, int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fff;
    auto intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (short) ByteOrder::swapIfLittleEndian (*unalignedPointerCast<const uint16*> (intData));
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (short) ByteOrder::swapIfLittleEndian (*unalignedPointerCast<const uint16*> (intData));
        }
    }
}

void AudioDataConverters::convertInt24LEToFloat (const void* source, float* dest, int numSamples, int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fffff;
    auto intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (short) ByteOrder::littleEndian24Bit (intData);
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (short) ByteOrder::littleEndian24Bit (intData);
        }
    }
}

void AudioDataConverters::convertInt24BEToFloat (const void* source, float* dest, int numSamples, int srcBytesPerSample)
{
    const float scale = 1.0f / 0x7fffff;
    auto intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (short) ByteOrder::bigEndian24Bit (intData);
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (short) ByteOrder::bigEndian24Bit (intData);
        }
    }
}

void AudioDataConverters::convertInt32LEToFloat (const void* source, float* dest, int numSamples, int srcBytesPerSample)
{
    const float scale = 1.0f / (float) 0x7fffffff;
    auto intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (float) ByteOrder::swapIfBigEndian (*unalignedPointerCast<const uint32*> (intData));
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (float) ByteOrder::swapIfBigEndian (*unalignedPointerCast<const uint32*> (intData));
        }
    }
}

void AudioDataConverters::convertInt32BEToFloat (const void* source, float* dest, int numSamples, int srcBytesPerSample)
{
    const float scale = 1.0f / (float) 0x7fffffff;
    auto intData = static_cast<const char*> (source);

    if (source != (void*) dest || srcBytesPerSample >= 4)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = scale * (float) ByteOrder::swapIfLittleEndian (*unalignedPointerCast<const uint32*> (intData));
            intData += srcBytesPerSample;
        }
    }
    else
    {
        intData += srcBytesPerSample * numSamples;

        for (int i = numSamples; --i >= 0;)
        {
            intData -= srcBytesPerSample;
            dest[i] = scale * (float) ByteOrder::swapIfLittleEndian (*unalignedPointerCast<const uint32*> (intData));
        }
    }
}

void AudioDataConverters::convertFloat32LEToFloat (const void* source, float* dest, int numSamples, int srcBytesPerSample)
{
    auto s = static_cast<const char*> (source);

    for (int i = 0; i < numSamples; ++i)
    {
        dest[i] = *unalignedPointerCast<const float*> (s);

#if YUP_BIG_ENDIAN
        auto d = unalignedPointerCast<uint32*> (dest + i);
        *d = ByteOrder::swap (*d);
#endif

        s += srcBytesPerSample;
    }
}

void AudioDataConverters::convertFloat32BEToFloat (const void* source, float* dest, int numSamples, int srcBytesPerSample)
{
    auto s = static_cast<const char*> (source);

    for (int i = 0; i < numSamples; ++i)
    {
        dest[i] = *unalignedPointerCast<const float*> (s);

#if YUP_LITTLE_ENDIAN
        auto d = unalignedPointerCast<uint32*> (dest + i);
        *d = ByteOrder::swap (*d);
#endif

        s += srcBytesPerSample;
    }
}

//==============================================================================
void AudioDataConverters::convertFloatToFormat (DataFormat destFormat, const float* source, void* dest, int numSamples)
{
    switch (destFormat)
    {
        case int16LE:
            convertFloatToInt16LE (source, dest, numSamples);
            break;
        case int16BE:
            convertFloatToInt16BE (source, dest, numSamples);
            break;
        case int24LE:
            convertFloatToInt24LE (source, dest, numSamples);
            break;
        case int24BE:
            convertFloatToInt24BE (source, dest, numSamples);
            break;
        case int32LE:
            convertFloatToInt32LE (source, dest, numSamples);
            break;
        case int32BE:
            convertFloatToInt32BE (source, dest, numSamples);
            break;
        case float32LE:
            convertFloatToFloat32LE (source, dest, numSamples);
            break;
        case float32BE:
            convertFloatToFloat32BE (source, dest, numSamples);
            break;
        default:
            jassertfalse;
            break;
    }
}

void AudioDataConverters::convertFormatToFloat (DataFormat sourceFormat, const void* source, float* dest, int numSamples)
{
    switch (sourceFormat)
    {
        case int16LE:
            convertInt16LEToFloat (source, dest, numSamples);
            break;
        case int16BE:
            convertInt16BEToFloat (source, dest, numSamples);
            break;
        case int24LE:
            convertInt24LEToFloat (source, dest, numSamples);
            break;
        case int24BE:
            convertInt24BEToFloat (source, dest, numSamples);
            break;
        case int32LE:
            convertInt32LEToFloat (source, dest, numSamples);
            break;
        case int32BE:
            convertInt32BEToFloat (source, dest, numSamples);
            break;
        case float32LE:
            convertFloat32LEToFloat (source, dest, numSamples);
            break;
        case float32BE:
            convertFloat32BEToFloat (source, dest, numSamples);
            break;
        default:
            jassertfalse;
            break;
    }
}

//==============================================================================
void AudioDataConverters::interleaveSamples (const float** source, float* dest, int numSamples, int numChannels)
{
    using Format = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    AudioData::interleaveSamples (AudioData::NonInterleavedSource<Format> { source, numChannels },
                                  AudioData::InterleavedDest<Format> { dest, numChannels },
                                  numSamples);
}

void AudioDataConverters::deinterleaveSamples (const float* source, float** dest, int numSamples, int numChannels)
{
    using Format = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    AudioData::deinterleaveSamples (AudioData::InterleavedSource<Format> { source, numChannels },
                                    AudioData::NonInterleavedDest<Format> { dest, numChannels },
                                    numSamples);
}

YUP_END_IGNORE_DEPRECATION_WARNINGS

} // namespace yup
