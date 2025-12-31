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

namespace
{

constexpr MFBYTESTREAM_SEEK_ORIGIN kSeekCurrent = static_cast<MFBYTESTREAM_SEEK_ORIGIN> (1);
constexpr MFBYTESTREAM_SEEK_ORIGIN kSeekEnd = static_cast<MFBYTESTREAM_SEEK_ORIGIN> (2);

template <typename T>
static void safeRelease (T** value)
{
    if (value != nullptr && *value != nullptr)
    {
        (*value)->Release();
        *value = nullptr;
    }
}

static LONGLONG samplesToHns (int64 samples, double sampleRate)
{
    if (sampleRate <= 0.0)
        return 0;

    return (LONGLONG) ((double) samples * 10000000.0 / sampleRate + 0.5);
}

class MediaFoundationAsyncState : public IUnknown
{
public:
    explicit MediaFoundationAsyncState (ULONG bytesProcessedIn)
        : bytesProcessed (bytesProcessedIn)
    {
    }

    STDMETHODIMP QueryInterface (REFIID riid, void** ppvObject) override
    {
        if (ppvObject == nullptr)
            return E_POINTER;

        if (riid == __uuidof (IUnknown))
        {
            *ppvObject = static_cast<IUnknown*> (this);
            AddRef();
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_ (ULONG)

    AddRef() override
    {
        return (ULONG) InterlockedIncrement (&refCount);
    }

    STDMETHODIMP_ (ULONG)

    Release() override
    {
        const auto count = (ULONG) InterlockedDecrement (&refCount);
        if (count == 0)
            delete this;
        return count;
    }

    ULONG bytesProcessed = 0;

private:
    ~MediaFoundationAsyncState() = default;

    LONG refCount = 1;
};

class MediaFoundationInputByteStream final : public IMFByteStream
{
public:
    explicit MediaFoundationInputByteStream (InputStream* streamIn)
        : stream (streamIn)
    {
        if (stream == nullptr)
            return;

        const auto position = stream->getPosition();
        canSeek = stream->setPosition (position);
        stream->setPosition (position);

        const auto totalLength = stream->getTotalLength();
        if (totalLength >= 0)
        {
            length = (QWORD) totalLength;
            lengthKnown = true;
        }
    }

    STDMETHODIMP QueryInterface (REFIID riid, void** ppvObject) override
    {
        if (ppvObject == nullptr)
            return E_POINTER;

        if (riid == __uuidof (IUnknown) || riid == __uuidof (IMFByteStream))
        {
            *ppvObject = static_cast<IMFByteStream*> (this);
            AddRef();
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_ (ULONG)

    AddRef() override
    {
        return (ULONG) InterlockedIncrement (&refCount);
    }

    STDMETHODIMP_ (ULONG)

    Release() override
    {
        const auto count = (ULONG) InterlockedDecrement (&refCount);
        if (count == 0)
            delete this;
        return count;
    }

    STDMETHODIMP GetCapabilities (DWORD* pdwCapabilities) override
    {
        if (pdwCapabilities == nullptr)
            return E_POINTER;

        DWORD caps = MFBYTESTREAM_IS_READABLE;
        if (canSeek)
            caps |= MFBYTESTREAM_IS_SEEKABLE;

        *pdwCapabilities = caps;
        return S_OK;
    }

    STDMETHODIMP GetLength (QWORD* pqwLength) override
    {
        if (pqwLength == nullptr)
            return E_POINTER;

        if (! lengthKnown)
            return MF_E_BYTESTREAM_UNKNOWN_LENGTH;

        *pqwLength = length;
        return S_OK;
    }

    STDMETHODIMP SetLength (QWORD) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetCurrentPosition (QWORD* pqwPosition) override
    {
        if (pqwPosition == nullptr)
            return E_POINTER;

        if (stream == nullptr)
            return E_FAIL;

        *pqwPosition = (QWORD) stream->getPosition();
        return S_OK;
    }

    STDMETHODIMP SetCurrentPosition (QWORD qwPosition) override
    {
        if (stream == nullptr)
            return E_FAIL;

        return stream->setPosition ((int64) qwPosition) ? S_OK : E_NOTIMPL;
    }

    STDMETHODIMP IsEndOfStream (BOOL* pfEndOfStream) override
    {
        if (pfEndOfStream == nullptr)
            return E_POINTER;

        if (stream == nullptr)
            return E_FAIL;

        *pfEndOfStream = stream->isExhausted() ? TRUE : FALSE;
        return S_OK;
    }

    STDMETHODIMP Read (BYTE* pb, ULONG cb, ULONG* pcbRead) override
    {
        if (pcbRead != nullptr)
            *pcbRead = 0;

        if (stream == nullptr || pb == nullptr)
            return E_POINTER;

        const auto bytesRead = stream->read (pb, (int) cb);
        if (bytesRead < 0)
            return E_FAIL;

        if (pcbRead != nullptr)
            *pcbRead = (ULONG) bytesRead;

        return bytesRead == (int) cb ? S_OK : S_FALSE;
    }

    STDMETHODIMP BeginRead (BYTE* pb,
                            ULONG cb,
                            IMFAsyncCallback* pCallback,
                            IUnknown* punkState) override
    {
        ULONG bytesRead = 0;
        const auto hr = Read (pb, cb, &bytesRead);

        auto* asyncState = new MediaFoundationAsyncState (bytesRead);
        IMFAsyncResult* asyncResult = nullptr;
        const auto hrResult = MFCreateAsyncResult (asyncState, pCallback, punkState, &asyncResult);

        if (FAILED (hrResult))
        {
            asyncState->Release();
            return hrResult;
        }

        asyncResult->SetStatus (hr);
        if (pCallback != nullptr)
            pCallback->Invoke (asyncResult);

        asyncResult->Release();
        asyncState->Release();
        return S_OK;
    }

    STDMETHODIMP EndRead (IMFAsyncResult* pResult, ULONG* pcbRead) override
    {
        if (pResult == nullptr || pcbRead == nullptr)
            return E_POINTER;

        *pcbRead = 0;
        const auto hr = pResult->GetStatus();

        IUnknown* state = nullptr;
        if (SUCCEEDED (pResult->GetObject (&state)) && state != nullptr)
        {
            auto* asyncState = static_cast<MediaFoundationAsyncState*> (state);
            *pcbRead = asyncState->bytesProcessed;
            state->Release();
        }

        return hr;
    }

    STDMETHODIMP Write (const BYTE*, ULONG, ULONG*) override
    {
        return MF_E_INVALIDREQUEST;
    }

    STDMETHODIMP BeginWrite (const BYTE*,
                             ULONG,
                             IMFAsyncCallback*,
                             IUnknown*) override
    {
        return MF_E_INVALIDREQUEST;
    }

    STDMETHODIMP EndWrite (IMFAsyncResult*, ULONG*) override
    {
        return MF_E_INVALIDREQUEST;
    }

    STDMETHODIMP Seek (MFBYTESTREAM_SEEK_ORIGIN seekOrigin,
                       LONGLONG llSeekOffset,
                       DWORD,
                       QWORD* pqwCurrentPosition) override
    {
        if (stream == nullptr)
            return E_FAIL;

        int64 basePosition = 0;

        if (seekOrigin == kSeekCurrent)
            basePosition = stream->getPosition();
        else if (seekOrigin == kSeekEnd)
        {
            if (! lengthKnown)
                return MF_E_BYTESTREAM_UNKNOWN_LENGTH;

            basePosition = (int64) length;
        }

        const auto targetPosition = basePosition + (int64) llSeekOffset;
        if (targetPosition < 0)
            return E_INVALIDARG;

        if (! stream->setPosition (targetPosition))
            return E_FAIL;

        if (pqwCurrentPosition != nullptr)
            *pqwCurrentPosition = (QWORD) targetPosition;

        return S_OK;
    }

    STDMETHODIMP Flush() override
    {
        return S_OK;
    }

    STDMETHODIMP Close() override
    {
        return S_OK;
    }

private:
    ~MediaFoundationInputByteStream() = default;

    LONG refCount = 1;
    InputStream* stream = nullptr;
    bool canSeek = false;
    bool lengthKnown = false;
    QWORD length = 0;
};

class MediaFoundationOutputByteStream final : public IMFByteStream
{
public:
    explicit MediaFoundationOutputByteStream (OutputStream* streamIn)
        : stream (streamIn)
    {
        if (stream == nullptr)
            return;

        const auto position = stream->getPosition();
        canSeek = stream->setPosition (position);
        stream->setPosition (position);
        size = (QWORD) position;
    }

    STDMETHODIMP QueryInterface (REFIID riid, void** ppvObject) override
    {
        if (ppvObject == nullptr)
            return E_POINTER;

        if (riid == __uuidof (IUnknown) || riid == __uuidof (IMFByteStream))
        {
            *ppvObject = static_cast<IMFByteStream*> (this);
            AddRef();
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_ (ULONG)

    AddRef() override
    {
        return (ULONG) InterlockedIncrement (&refCount);
    }

    STDMETHODIMP_ (ULONG)

    Release() override
    {
        const auto count = (ULONG) InterlockedDecrement (&refCount);
        if (count == 0)
            delete this;
        return count;
    }

    STDMETHODIMP GetCapabilities (DWORD* pdwCapabilities) override
    {
        if (pdwCapabilities == nullptr)
            return E_POINTER;

        DWORD caps = MFBYTESTREAM_IS_WRITABLE;
        if (canSeek)
            caps |= MFBYTESTREAM_IS_SEEKABLE;

        *pdwCapabilities = caps;
        return S_OK;
    }

    STDMETHODIMP GetLength (QWORD* pqwLength) override
    {
        if (pqwLength == nullptr)
            return E_POINTER;

        *pqwLength = size;
        return S_OK;
    }

    STDMETHODIMP SetLength (QWORD qwLength) override
    {
        if (stream == nullptr)
            return E_FAIL;

        if (qwLength > size)
        {
            if (! stream->setPosition ((int64) size))
                return E_FAIL;

            const auto gap = (size_t) (qwLength - size);
            if (gap > 0 && ! stream->writeRepeatedByte (0, gap))
                return E_FAIL;
        }
        else if (! stream->setPosition ((int64) qwLength))
        {
            return E_FAIL;
        }

        size = qwLength;
        return S_OK;
    }

    STDMETHODIMP GetCurrentPosition (QWORD* pqwPosition) override
    {
        if (pqwPosition == nullptr)
            return E_POINTER;

        if (stream == nullptr)
            return E_FAIL;

        *pqwPosition = (QWORD) stream->getPosition();
        return S_OK;
    }

    STDMETHODIMP SetCurrentPosition (QWORD qwPosition) override
    {
        if (stream == nullptr)
            return E_FAIL;

        return stream->setPosition ((int64) qwPosition) ? S_OK : E_FAIL;
    }

    STDMETHODIMP IsEndOfStream (BOOL* pfEndOfStream) override
    {
        if (pfEndOfStream == nullptr)
            return E_POINTER;

        *pfEndOfStream = FALSE;
        return S_OK;
    }

    STDMETHODIMP Read (BYTE*, ULONG, ULONG*) override
    {
        return MF_E_INVALIDREQUEST;
    }

    STDMETHODIMP BeginRead (BYTE*,
                            ULONG,
                            IMFAsyncCallback*,
                            IUnknown*) override
    {
        return MF_E_INVALIDREQUEST;
    }

    STDMETHODIMP EndRead (IMFAsyncResult*, ULONG*) override
    {
        return MF_E_INVALIDREQUEST;
    }

    STDMETHODIMP Write (const BYTE* pb, ULONG cb, ULONG* pcbWritten) override
    {
        if (pcbWritten != nullptr)
            *pcbWritten = 0;

        if (stream == nullptr || pb == nullptr)
            return E_POINTER;

        const auto currentPosition = stream->getPosition();
        if (! stream->write (pb, cb))
            return E_FAIL;

        const auto endPosition = (QWORD) currentPosition + cb;
        if (endPosition > size)
            size = endPosition;

        if (pcbWritten != nullptr)
            *pcbWritten = cb;

        return S_OK;
    }

    STDMETHODIMP BeginWrite (const BYTE* pb,
                             ULONG cb,
                             IMFAsyncCallback* pCallback,
                             IUnknown* punkState) override
    {
        ULONG bytesWritten = 0;
        const auto hr = Write (pb, cb, &bytesWritten);

        auto* asyncState = new MediaFoundationAsyncState (bytesWritten);
        IMFAsyncResult* asyncResult = nullptr;
        const auto hrResult = MFCreateAsyncResult (asyncState, pCallback, punkState, &asyncResult);

        if (FAILED (hrResult))
        {
            asyncState->Release();
            return hrResult;
        }

        asyncResult->SetStatus (hr);
        if (pCallback != nullptr)
            pCallback->Invoke (asyncResult);

        asyncResult->Release();
        asyncState->Release();
        return S_OK;
    }

    STDMETHODIMP EndWrite (IMFAsyncResult* pResult, ULONG* pcbWritten) override
    {
        if (pResult == nullptr || pcbWritten == nullptr)
            return E_POINTER;

        *pcbWritten = 0;
        const auto hr = pResult->GetStatus();

        IUnknown* state = nullptr;
        if (SUCCEEDED (pResult->GetObject (&state)) && state != nullptr)
        {
            auto* asyncState = static_cast<MediaFoundationAsyncState*> (state);
            *pcbWritten = asyncState->bytesProcessed;
            state->Release();
        }

        return hr;
    }

    STDMETHODIMP Seek (MFBYTESTREAM_SEEK_ORIGIN seekOrigin,
                       LONGLONG llSeekOffset,
                       DWORD,
                       QWORD* pqwCurrentPosition) override
    {
        if (stream == nullptr)
            return E_FAIL;

        int64 basePosition = 0;

        if (seekOrigin == kSeekCurrent)
            basePosition = stream->getPosition();
        else if (seekOrigin == kSeekEnd)
            basePosition = (int64) size;

        const auto targetPosition = basePosition + (int64) llSeekOffset;
        if (targetPosition < 0)
            return E_INVALIDARG;

        if (! stream->setPosition (targetPosition))
            return E_FAIL;

        if ((QWORD) targetPosition > size)
            size = (QWORD) targetPosition;

        if (pqwCurrentPosition != nullptr)
            *pqwCurrentPosition = (QWORD) targetPosition;

        return S_OK;
    }

    STDMETHODIMP Flush() override
    {
        if (stream != nullptr)
            stream->flush();
        return S_OK;
    }

    STDMETHODIMP Close() override
    {
        if (stream != nullptr)
            stream->flush();
        return S_OK;
    }

private:
    ~MediaFoundationOutputByteStream() = default;

    LONG refCount = 1;
    OutputStream* stream = nullptr;
    bool canSeek = false;
    QWORD size = 0;
};

class WindowsMediaAudioFormatReader : public AudioFormatReader
{
public:
    explicit WindowsMediaAudioFormatReader (InputStream* sourceStream);
    ~WindowsMediaAudioFormatReader() override;

    bool readSamples (float* const* destChannels,
                      int numDestChannels,
                      int startOffsetInDestBuffer,
                      int64 startSampleInFile,
                      int numSamples) override;

private:
    bool openFromStream (InputStream* sourceStream);
    bool configureOutputType();
    bool seekToSample (int64 samplePosition);
    bool copyInterleavedToDest (const void* source,
                                int numFrames,
                                float* const* destChannels,
                                int numDestChannels,
                                int destOffset);

    IMFSourceReader* reader = nullptr;
    IMFMediaType* outputType = nullptr;
    IMFByteStream* byteStream = nullptr;

    HeapBlock<uint8> leftoverBuffer;
    size_t leftoverCapacity = 0;
    int leftoverFrames = 0;

    bool isOpen = false;
    bool comInitialized = false;
    bool mfInitialized = false;
    bool outputIsFloat = false;
    int bytesPerSample = 0;
    int64 currentSamplePosition = 0;
};

WindowsMediaAudioFormatReader::WindowsMediaAudioFormatReader (InputStream* sourceStream)
    : AudioFormatReader (sourceStream, "Windows Media")
{
    if (sourceStream != nullptr)
        isOpen = openFromStream (sourceStream);
}

WindowsMediaAudioFormatReader::~WindowsMediaAudioFormatReader()
{
    safeRelease (&reader);
    safeRelease (&outputType);
    safeRelease (&byteStream);

    if (mfInitialized)
        MFShutdown();

    if (comInitialized)
        CoUninitialize();
}

bool WindowsMediaAudioFormatReader::openFromStream (InputStream* sourceStream)
{
    HRESULT hr = CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED (hr) && hr != RPC_E_CHANGED_MODE)
        return false;

    if (hr == S_OK || hr == S_FALSE)
        comInitialized = true;

    hr = MFStartup (MF_VERSION);
    if (FAILED (hr))
        return false;

    mfInitialized = true;

    byteStream = new MediaFoundationInputByteStream (sourceStream);
    hr = MFCreateSourceReaderFromByteStream (byteStream, nullptr, &reader);
    if (FAILED (hr))
        return false;

    if (! configureOutputType())
        return false;

    PROPVARIANT prop;
    PropVariantInit (&prop);
    if (SUCCEEDED (reader->GetPresentationAttribute (MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &prop)))
    {
        if (prop.vt == VT_UI8 || prop.vt == VT_I8)
        {
            lengthInSamples = (int64) ((double) prop.hVal.QuadPart * sampleRate / 10000000.0);
        }
    }
    PropVariantClear (&prop);

    return sampleRate > 0.0 && numChannels > 0;
}

bool WindowsMediaAudioFormatReader::configureOutputType()
{
    IMFMediaType* nativeType = nullptr;
    HRESULT hr = reader->GetNativeMediaType (MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &nativeType);
    if (FAILED (hr))
    {
        safeRelease (&nativeType);
        return false;
    }

    UINT32 channels = 0;
    UINT32 samplesPerSecond = 0;
    nativeType->GetUINT32 (MF_MT_AUDIO_NUM_CHANNELS, &channels);
    nativeType->GetUINT32 (MF_MT_AUDIO_SAMPLES_PER_SECOND, &samplesPerSecond);
    safeRelease (&nativeType);

    if (channels == 0 || samplesPerSecond == 0)
        return false;

    IMFMediaType* desiredType = nullptr;
    hr = MFCreateMediaType (&desiredType);
    if (FAILED (hr))
        return false;

    hr = desiredType->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    hr = desiredType->SetGUID (MF_MT_SUBTYPE, MFAudioFormat_Float);
    hr = desiredType->SetUINT32 (MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
    hr = desiredType->SetUINT32 (MF_MT_AUDIO_NUM_CHANNELS, channels);
    hr = desiredType->SetUINT32 (MF_MT_AUDIO_SAMPLES_PER_SECOND, samplesPerSecond);
    hr = desiredType->SetUINT32 (MF_MT_AUDIO_BLOCK_ALIGNMENT, channels * sizeof (float));
    hr = desiredType->SetUINT32 (MF_MT_AUDIO_AVG_BYTES_PER_SECOND, samplesPerSecond * channels * sizeof (float));

    hr = reader->SetCurrentMediaType (MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, desiredType);
    if (FAILED (hr))
    {
        safeRelease (&desiredType);
        hr = MFCreateMediaType (&desiredType);
        if (FAILED (hr))
            return false;

        hr = desiredType->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        hr = desiredType->SetGUID (MF_MT_SUBTYPE, MFAudioFormat_PCM);
        hr = desiredType->SetUINT32 (MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        hr = desiredType->SetUINT32 (MF_MT_AUDIO_NUM_CHANNELS, channels);
        hr = desiredType->SetUINT32 (MF_MT_AUDIO_SAMPLES_PER_SECOND, samplesPerSecond);
        hr = desiredType->SetUINT32 (MF_MT_AUDIO_BLOCK_ALIGNMENT, channels * sizeof (int16));
        hr = desiredType->SetUINT32 (MF_MT_AUDIO_AVG_BYTES_PER_SECOND, samplesPerSecond * channels * sizeof (int16));

        hr = reader->SetCurrentMediaType (MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, desiredType);
        if (FAILED (hr))
        {
            safeRelease (&desiredType);
            return false;
        }
    }

    safeRelease (&outputType);
    hr = reader->GetCurrentMediaType (MF_SOURCE_READER_FIRST_AUDIO_STREAM, &outputType);
    safeRelease (&desiredType);

    if (FAILED (hr) || outputType == nullptr)
        return false;

    GUID subtype = {};
    outputType->GetGUID (MF_MT_SUBTYPE, &subtype);

    UINT32 actualChannels = 0;
    UINT32 actualSampleRate = 0;
    UINT32 actualBitsPerSample = 0;
    outputType->GetUINT32 (MF_MT_AUDIO_NUM_CHANNELS, &actualChannels);
    outputType->GetUINT32 (MF_MT_AUDIO_SAMPLES_PER_SECOND, &actualSampleRate);
    outputType->GetUINT32 (MF_MT_AUDIO_BITS_PER_SAMPLE, &actualBitsPerSample);

    numChannels = (int) actualChannels;
    sampleRate = (double) actualSampleRate;
    bitsPerSample = (int) actualBitsPerSample;

    outputIsFloat = (subtype == MFAudioFormat_Float);
    usesFloatingPointData = outputIsFloat;

    if (outputIsFloat && bitsPerSample == 0)
        bitsPerSample = 32;
    if (! outputIsFloat && bitsPerSample == 0)
        bitsPerSample = 16;

    bytesPerSample = bitsPerSample / 8;
    return bytesPerSample > 0 && sampleRate > 0.0 && numChannels > 0;
}

bool WindowsMediaAudioFormatReader::seekToSample (int64 samplePosition)
{
    const auto clampedPosition = samplePosition < 0 ? 0 : samplePosition;
    const auto seekTarget = samplesToHns (clampedPosition, sampleRate);

    PROPVARIANT prop;
    PropVariantInit (&prop);
    if (FAILED (InitPropVariantFromInt64 (seekTarget, &prop)))
        return false;

    reader->Flush (MF_SOURCE_READER_FIRST_AUDIO_STREAM);
    const auto hr = reader->SetCurrentPosition (GUID_NULL, prop);
    PropVariantClear (&prop);

    if (FAILED (hr))
        return false;

    currentSamplePosition = clampedPosition;
    leftoverFrames = 0;
    return true;
}

bool WindowsMediaAudioFormatReader::copyInterleavedToDest (const void* source,
                                                           int numFrames,
                                                           float* const* destChannels,
                                                           int numDestChannels,
                                                           int destOffset)
{
    if (numFrames <= 0)
        return true;

    HeapBlock<float*> offsetDestChannels;
    offsetDestChannels.malloc ((size_t) numDestChannels);
    for (int ch = 0; ch < numDestChannels; ++ch)
        offsetDestChannels[ch] = destChannels[ch] + destOffset;

    if (outputIsFloat)
    {
        using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
        using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

        AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { static_cast<const float*> (source), numChannels },
                                        AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numDestChannels },
                                        numFrames);
        return true;
    }

    if (bitsPerSample == 16)
    {
        using SourceFormat = AudioData::Format<AudioData::Int16, AudioData::LittleEndian>;
        using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

        AudioData::deinterleaveSamples (AudioData::InterleavedSource<SourceFormat> { reinterpret_cast<const uint16*> (source), numChannels },
                                        AudioData::NonInterleavedDest<DestFormat> { offsetDestChannels.getData(), numDestChannels },
                                        numFrames);
        return true;
    }

    return false;
}

bool WindowsMediaAudioFormatReader::readSamples (float* const* destChannels,
                                                 int numDestChannels,
                                                 int startOffsetInDestBuffer,
                                                 int64 startSampleInFile,
                                                 int numSamples)
{
    if (! isOpen)
        return false;

    if (numSamples <= 0)
        return true;

    const auto numChannelsToRead = jmin (numDestChannels, (int) numChannels);
    if (numChannelsToRead <= 0)
        return true;

    if (startSampleInFile != currentSamplePosition)
    {
        if (! seekToSample (startSampleInFile))
            return false;
    }

    const int bytesPerFrame = bytesPerSample * numChannels;
    if (bytesPerFrame <= 0)
        return false;

    int framesRemaining = numSamples;
    int framesWritten = 0;

    if (leftoverFrames > 0)
    {
        const auto framesToCopy = jmin (framesRemaining, leftoverFrames);
        if (! copyInterleavedToDest (leftoverBuffer.getData(), framesToCopy, destChannels, numChannelsToRead, startOffsetInDestBuffer))
            return false;

        framesWritten += framesToCopy;
        framesRemaining -= framesToCopy;
        leftoverFrames -= framesToCopy;

        if (leftoverFrames > 0)
        {
            const auto bytesToMove = (size_t) leftoverFrames * (size_t) bytesPerFrame;
            memmove (leftoverBuffer.getData(),
                     leftoverBuffer.getData() + (size_t) framesToCopy * (size_t) bytesPerFrame,
                     bytesToMove);
        }
    }

    while (framesRemaining > 0)
    {
        IMFSample* sample = nullptr;
        IMFMediaBuffer* mediaBuffer = nullptr;
        DWORD flags = 0;
        LONGLONG timestamp = 0;

        const auto hr = reader->ReadSample (MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                            0,
                                            nullptr,
                                            &flags,
                                            &timestamp,
                                            &sample);
        if (FAILED (hr))
        {
            safeRelease (&sample);
            safeRelease (&mediaBuffer);
            return false;
        }

        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
        {
            safeRelease (&sample);
            safeRelease (&mediaBuffer);
            return false;
        }

        if (sample == nullptr)
        {
            safeRelease (&sample);
            safeRelease (&mediaBuffer);
            continue;
        }

        if (FAILED (sample->ConvertToContiguousBuffer (&mediaBuffer)))
        {
            safeRelease (&sample);
            safeRelease (&mediaBuffer);
            return false;
        }

        BYTE* bufferData = nullptr;
        DWORD bufferLength = 0;
        if (FAILED (mediaBuffer->Lock (&bufferData, nullptr, &bufferLength)))
        {
            safeRelease (&mediaBuffer);
            safeRelease (&sample);
            return false;
        }

        const auto framesAvailable = (int) (bufferLength / (DWORD) bytesPerFrame);
        if (framesAvailable > 0)
        {
            const auto framesToCopy = jmin (framesRemaining, framesAvailable);
            if (! copyInterleavedToDest (bufferData,
                                         framesToCopy,
                                         destChannels,
                                         numChannelsToRead,
                                         startOffsetInDestBuffer + framesWritten))
            {
                mediaBuffer->Unlock();
                safeRelease (&mediaBuffer);
                safeRelease (&sample);
                return false;
            }

            framesWritten += framesToCopy;
            framesRemaining -= framesToCopy;

            const auto extraFrames = framesAvailable - framesToCopy;
            if (extraFrames > 0)
            {
                const auto extraBytes = (size_t) extraFrames * (size_t) bytesPerFrame;
                if (extraBytes > leftoverCapacity)
                {
                    leftoverCapacity = extraBytes;
                    leftoverBuffer.allocate (leftoverCapacity, false);
                }

                memcpy (leftoverBuffer.getData(),
                        bufferData + (size_t) framesToCopy * (size_t) bytesPerFrame,
                        extraBytes);
                leftoverFrames = extraFrames;
            }
        }

        mediaBuffer->Unlock();
        safeRelease (&mediaBuffer);
        safeRelease (&sample);
    }

    currentSamplePosition = startSampleInFile + framesWritten;
    return framesRemaining == 0;
}

class WindowsMediaAudioFormatWriter : public AudioFormatWriter
{
public:
    WindowsMediaAudioFormatWriter (OutputStream* destStream,
                                   double sampleRateIn,
                                   int numberOfChannels,
                                   int bitsPerSampleIn,
                                   const StringPairArray& metadataValues,
                                   int qualityOptionIndex);
    ~WindowsMediaAudioFormatWriter() override;

    bool write (const float* const* samplesToWrite, int numSamples) override;
    bool flush() override;

    bool isValid() const { return isOpen && sinkWriter != nullptr; }

private:
    bool openWriter (OutputStream* stream,
                     double sampleRateIn,
                     int numberOfChannels,
                     int bitsPerSampleIn,
                     int qualityOptionIndex);
    GUID resolveContainerType (OutputStream* stream) const;
    int resolveBitRate (int qualityOptionIndex) const;

    IMFSinkWriter* sinkWriter = nullptr;
    IMFByteStream* byteStream = nullptr;
    DWORD streamIndex = 0;

    HeapBlock<float> interleavedBuffer;
    size_t interleavedCapacity = 0;

    int numChannels = 0;
    double sampleRate = 0.0;
    int bitsPerSample = 0;
    int64 totalSamplesWritten = 0;

    bool comInitialized = false;
    bool mfInitialized = false;
    bool finalized = false;
    bool isOpen = false;
};

WindowsMediaAudioFormatWriter::WindowsMediaAudioFormatWriter (OutputStream* destStream,
                                                              double sampleRateIn,
                                                              int numberOfChannels,
                                                              int bitsPerSampleIn,
                                                              const StringPairArray& metadataValues,
                                                              int qualityOptionIndex)
    : AudioFormatWriter (destStream, "Windows Media", sampleRateIn, numberOfChannels, bitsPerSampleIn)
{
    ignoreUnused (metadataValues);
    isOpen = openWriter (destStream, sampleRateIn, numberOfChannels, bitsPerSampleIn, qualityOptionIndex);
}

WindowsMediaAudioFormatWriter::~WindowsMediaAudioFormatWriter()
{
    flush();

    safeRelease (&sinkWriter);
    safeRelease (&byteStream);

    if (mfInitialized)
        MFShutdown();

    if (comInitialized)
        CoUninitialize();
}

GUID WindowsMediaAudioFormatWriter::resolveContainerType (OutputStream* stream) const
{
    if (auto* fileStream = dynamic_cast<FileOutputStream*> (stream))
    {
        const auto extension = fileStream->getFile().getFileExtension().toLowerCase();
        if (extension == ".aac")
            return MFTranscodeContainerType_ADTS;
        if (extension == ".mp4" || extension == ".m4a")
            return MFTranscodeContainerType_MPEG4;
    }

    return MFTranscodeContainerType_MPEG4;
}

int WindowsMediaAudioFormatWriter::resolveBitRate (int qualityOptionIndex) const
{
    static const int bitRates[] = { 96000, 128000, 192000 };
    const int numRates = (int) (sizeof (bitRates) / sizeof (bitRates[0]));
    const int index = jlimit (0, numRates - 1, qualityOptionIndex);
    return bitRates[index];
}

bool WindowsMediaAudioFormatWriter::openWriter (OutputStream* stream,
                                                double sampleRateIn,
                                                int numberOfChannels,
                                                int bitsPerSampleIn,
                                                int qualityOptionIndex)
{
    if (stream == nullptr)
        return false;

    HRESULT hr = CoInitializeEx (nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED (hr) && hr != RPC_E_CHANGED_MODE)
        return false;

    if (hr == S_OK || hr == S_FALSE)
        comInitialized = true;

    hr = MFStartup (MF_VERSION);
    if (FAILED (hr))
        return false;

    mfInitialized = true;

    byteStream = new MediaFoundationOutputByteStream (stream);

    IMFAttributes* attributes = nullptr;
    hr = MFCreateAttributes (&attributes, 1);
    if (FAILED (hr))
        return false;

    const auto containerType = resolveContainerType (stream);
    attributes->SetGUID (MF_TRANSCODE_CONTAINERTYPE, containerType);

    hr = MFCreateSinkWriterFromURL (nullptr, byteStream, attributes, &sinkWriter);
    safeRelease (&attributes);

    if (FAILED (hr) || sinkWriter == nullptr)
        return false;

    IMFMediaType* outputType = nullptr;
    hr = MFCreateMediaType (&outputType);
    if (FAILED (hr))
        return false;

    const auto bitRate = resolveBitRate (qualityOptionIndex);

    outputType->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    outputType->SetGUID (MF_MT_SUBTYPE, MFAudioFormat_AAC);
    outputType->SetUINT32 (MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
    outputType->SetUINT32 (MF_MT_AUDIO_NUM_CHANNELS, (UINT32) numberOfChannels);
    outputType->SetUINT32 (MF_MT_AUDIO_SAMPLES_PER_SECOND, (UINT32) sampleRateIn);
    outputType->SetUINT32 (MF_MT_AUDIO_AVG_BYTES_PER_SECOND, (UINT32) (bitRate / 8));

    hr = sinkWriter->AddStream (outputType, &streamIndex);
    safeRelease (&outputType);
    if (FAILED (hr))
        return false;

    IMFMediaType* inputType = nullptr;
    hr = MFCreateMediaType (&inputType);
    if (FAILED (hr))
        return false;

    inputType->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    inputType->SetGUID (MF_MT_SUBTYPE, MFAudioFormat_Float);
    inputType->SetUINT32 (MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
    inputType->SetUINT32 (MF_MT_AUDIO_NUM_CHANNELS, (UINT32) numberOfChannels);
    inputType->SetUINT32 (MF_MT_AUDIO_SAMPLES_PER_SECOND, (UINT32) sampleRateIn);
    inputType->SetUINT32 (MF_MT_AUDIO_BLOCK_ALIGNMENT, (UINT32) (numberOfChannels * sizeof (float)));
    inputType->SetUINT32 (MF_MT_AUDIO_AVG_BYTES_PER_SECOND, (UINT32) (sampleRateIn * numberOfChannels * sizeof (float)));

    hr = sinkWriter->SetInputMediaType (streamIndex, inputType, nullptr);
    safeRelease (&inputType);
    if (FAILED (hr))
        return false;

    hr = sinkWriter->BeginWriting();
    if (FAILED (hr))
        return false;

    numChannels = numberOfChannels;
    sampleRate = sampleRateIn;
    bitsPerSample = bitsPerSampleIn;
    return true;
}

bool WindowsMediaAudioFormatWriter::write (const float* const* samplesToWrite, int numSamples)
{
    if (! isOpen || sinkWriter == nullptr || numSamples <= 0)
        return false;

    const auto totalSamples = (size_t) numSamples * (size_t) numChannels;
    if (totalSamples > interleavedCapacity)
    {
        interleavedCapacity = totalSamples;
        interleavedBuffer.allocate (interleavedCapacity, false);
    }

    using SourceFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;
    using DestFormat = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    AudioData::interleaveSamples (AudioData::NonInterleavedSource<SourceFormat> { samplesToWrite, numChannels },
                                  AudioData::InterleavedDest<DestFormat> { interleavedBuffer.getData(), numChannels },
                                  numSamples);

    const auto byteCount = (DWORD) (totalSamples * sizeof (float));
    IMFMediaBuffer* buffer = nullptr;
    IMFSample* sample = nullptr;

    HRESULT hr = MFCreateMemoryBuffer (byteCount, &buffer);
    if (FAILED (hr))
        return false;

    hr = MFCreateSample (&sample);
    if (FAILED (hr))
    {
        safeRelease (&buffer);
        return false;
    }

    BYTE* destData = nullptr;
    if (FAILED (buffer->Lock (&destData, nullptr, nullptr)))
    {
        safeRelease (&buffer);
        safeRelease (&sample);
        return false;
    }

    memcpy (destData, interleavedBuffer.getData(), byteCount);
    buffer->Unlock();
    buffer->SetCurrentLength (byteCount);
    sample->AddBuffer (buffer);

    const auto sampleTime = samplesToHns (totalSamplesWritten, sampleRate);
    const auto sampleDuration = samplesToHns (numSamples, sampleRate);
    sample->SetSampleTime (sampleTime);
    sample->SetSampleDuration (sampleDuration);

    hr = sinkWriter->WriteSample (streamIndex, sample);

    safeRelease (&buffer);
    safeRelease (&sample);

    if (FAILED (hr))
        return false;

    totalSamplesWritten += numSamples;
    return true;
}

bool WindowsMediaAudioFormatWriter::flush()
{
    if (! finalized && sinkWriter != nullptr)
    {
        sinkWriter->Finalize();
        finalized = true;
    }

    if (output != nullptr)
        output->flush();

    return true;
}

} // namespace

//==============================================================================
// WindowsMediaAudioFormat implementation
WindowsMediaAudioFormat::WindowsMediaAudioFormat()
    : formatName ("Windows Media")
{
}

WindowsMediaAudioFormat::~WindowsMediaAudioFormat() = default;

const String& WindowsMediaAudioFormat::getFormatName() const
{
    return formatName;
}

Array<String> WindowsMediaAudioFormat::getFileExtensions() const
{
    return { ".m4a", ".mp4", ".aac", ".wma", ".wm", ".wmv", ".asf", ".mp3" };
}

std::unique_ptr<AudioFormatReader> WindowsMediaAudioFormat::createReaderFor (InputStream* sourceStream)
{
    auto reader = std::make_unique<WindowsMediaAudioFormatReader> (sourceStream);

    if (reader->sampleRate > 0 && reader->numChannels > 0)
        return reader;

    return nullptr;
}

std::unique_ptr<AudioFormatWriter> WindowsMediaAudioFormat::createWriterFor (OutputStream* streamToWriteTo,
                                                                             double sampleRate,
                                                                             int numberOfChannels,
                                                                             int bitsPerSample,
                                                                             const StringPairArray& metadataValues,
                                                                             int qualityOptionIndex)
{
    if (streamToWriteTo == nullptr)
        return nullptr;

    if (numberOfChannels < 1 || numberOfChannels > 2)
        return nullptr;

    if (sampleRate <= 0.0)
        return nullptr;

    if (bitsPerSample != 32)
        return nullptr;

    auto writer = std::make_unique<WindowsMediaAudioFormatWriter> (streamToWriteTo,
                                                                   sampleRate,
                                                                   numberOfChannels,
                                                                   bitsPerSample,
                                                                   metadataValues,
                                                                   qualityOptionIndex);

    return writer->isValid() ? std::move (writer) : nullptr;
}

Array<int> WindowsMediaAudioFormat::getPossibleBitDepths() const
{
    return { 32 };
}

Array<int> WindowsMediaAudioFormat::getPossibleSampleRates() const
{
    return { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000, 192000 };
}

StringArray WindowsMediaAudioFormat::getQualityOptions() const
{
    return { "Low (96 kbps)", "Medium (128 kbps)", "High (192 kbps)" };
}

} // namespace yup
