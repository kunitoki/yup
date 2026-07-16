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
// RIFF format helpers (file-private)
//==============================================================================

namespace
{

constexpr uint32_t makeFourCC (char a, char b, char c, char d) noexcept
{
    return static_cast<uint32_t> (static_cast<unsigned char> (a))
         | (static_cast<uint32_t> (static_cast<unsigned char> (b)) << 8)
         | (static_cast<uint32_t> (static_cast<unsigned char> (c)) << 16)
         | (static_cast<uint32_t> (static_cast<unsigned char> (d)) << 24);
}

constexpr uint32_t kFourCC_YSLB = makeFourCC ('Y', 'S', 'L', 'B');
constexpr uint32_t kFourCC_RIFF = makeFourCC ('R', 'I', 'F', 'F');
constexpr uint32_t kFourCC_LIST = makeFourCC ('L', 'I', 'S', 'T');
constexpr uint32_t kFourCC_VERS = makeFourCC ('V', 'E', 'R', 'S');
constexpr uint32_t kFourCC_SHAD = makeFourCC ('S', 'H', 'A', 'D');
constexpr uint32_t kFourCC_SHDR = makeFourCC ('S', 'H', 'D', 'R');
constexpr uint32_t kFourCC_SPVB = makeFourCC ('S', 'P', 'V', 'B');
constexpr uint32_t kFourCC_VARS = makeFourCC ('V', 'A', 'R', 'S');
constexpr uint32_t kFourCC_VART = makeFourCC ('V', 'A', 'R', 'T');
constexpr uint32_t kFourCC_REFL = makeFourCC ('R', 'E', 'F', 'L');
constexpr uint32_t kFourCC_ISRC = makeFourCC ('I', 'S', 'R', 'C');

constexpr uint32_t kCurrentVersion = 2;

//==============================================================================
// Write helpers

void writeStringRaw (MemoryOutputStream& buf, const String& s)
{
    const auto len = static_cast<int32> (s.getNumBytesAsUTF8());
    buf.writeInt (len);
    buf.write (s.toRawUTF8(), static_cast<size_t> (len));
}

void writeBinaryChunk (MemoryOutputStream& buf, uint32_t fourcc, const MemoryBlock& data)
{
    const auto len = static_cast<uint32_t> (data.getSize());
    buf.writeInt (static_cast<int> (fourcc));
    buf.writeInt (static_cast<int> (len));
    buf.write (data.getData(), len);
    if (len & 1u)
        buf.writeByte (0);
}

//==============================================================================
// Read helpers

String readStringRaw (InputStream& s)
{
    const auto len = s.readInt();
    if (len <= 0)
        return {};
    MemoryBlock buf (static_cast<size_t> (len), false);
    s.read (buf.getData(), len);
    return String::fromUTF8 (static_cast<const char*> (buf.getData()), len);
}

// Reads each [fourcc, size, data] chunk from `s`, calling handler(fourcc, subStream) for each.
// The sub-stream contains exactly `size` bytes; padding is consumed before the next iteration.
template <typename Handler>
void iterateChunks (InputStream& s, Handler&& handler)
{
    while (s.getNumBytesRemaining() >= 8)
    {
        const auto fourcc = static_cast<uint32_t> (s.readInt());
        const auto size = static_cast<uint32_t> (s.readInt());

        MemoryBlock data (static_cast<size_t> (size), false);
        if (size > 0)
            s.read (data.getData(), static_cast<int> (size));
        if (size & 1u)
            s.skipNextBytes (1);

        MemoryInputStream sub (data, false);
        handler (fourcc, sub);
    }
}

} // namespace

//==============================================================================
// ShaderBundle implementation
//==============================================================================

void ShaderBundle::addShader (ShaderInfo info)
{
    shaders.push_back (std::move (info));
}

void ShaderBundle::setSPIRV (ShaderStage stage, ShaderLanguage sourceLang, MemoryBlock spirv)
{
    spirvBinaries[stage] = { sourceLang, std::move (spirv) };
}

const std::vector<ShaderInfo>& ShaderBundle::getShaders() const
{
    return shaders;
}

const ShaderInfo* ShaderBundle::findShader (ShaderStage stage, ShaderLanguage language) const
{
    for (const auto& info : shaders)
        if (info.stage == stage && info.language == language)
            return &info;
    return nullptr;
}

//==============================================================================
// Save

Result ShaderBundle::saveToStream (OutputStream& stream) const
{
    MemoryOutputStream buf;

    // Writes [fourcc][size][body()][pad] - size is back-patched after body() returns.
    auto writeChunk = [&] (uint32_t fourcc, const auto& body)
    {
        buf.writeInt (static_cast<int> (fourcc));
        const auto sizePos = buf.getPosition();
        buf.writeInt (0);
        const auto dataStart = buf.getPosition();
        body();
        const auto endPos = buf.getPosition();
        const auto dataSize = static_cast<uint32_t> (endPos - dataStart);
        buf.setPosition (sizePos);
        buf.writeInt (static_cast<int> (dataSize));
        buf.setPosition (endPos);
        if (dataSize & 1u)
            buf.writeByte (0);
    };

    // Writes a LIST chunk: [LIST][size][listType][body()]
    auto writeList = [&] (uint32_t listType, const auto& body)
    {
        writeChunk (kFourCC_LIST, [&]
        {
            buf.writeInt (static_cast<int> (listType));
            body();
        });
    };

    // Collect unique stages: from shaders first (preserves insertion order), then SPIR-V-only stages.
    std::vector<ShaderStage> stages;
    for (const auto& s : shaders)
        if (std::find (stages.begin(), stages.end(), s.stage) == stages.end())
            stages.push_back (s.stage);
    for (const auto& [stage, _] : spirvBinaries)
        if (std::find (stages.begin(), stages.end(), stage) == stages.end())
            stages.push_back (stage);

    writeChunk (kFourCC_RIFF, [&]
    {
        buf.writeInt (static_cast<int> (kFourCC_YSLB));

        writeChunk (kFourCC_VERS, [&]
        {
            buf.writeInt (static_cast<int> (kCurrentVersion));
        });

        writeList (kFourCC_SHAD, [&]
        {
            for (const auto stage : stages)
            {
                const auto spirvIt = spirvBinaries.find (stage);
                const auto srcLang = (spirvIt != spirvBinaries.end())
                                       ? spirvIt->second.sourceLang
                                       : ShaderLanguage::glsl;

                writeChunk (kFourCC_SHDR, [&]
                {
                    buf.writeInt (static_cast<int> (stage));
                    buf.writeInt (static_cast<int> (srcLang));

                    writeBinaryChunk (buf, kFourCC_SPVB, spirvIt != spirvBinaries.end() ? spirvIt->second.spirv : MemoryBlock {});

                    writeList (kFourCC_VARS, [&]
                    {
                        for (const auto& info : shaders)
                        {
                            if (info.stage != stage)
                                continue;

                            writeChunk (kFourCC_VART, [&]
                            {
                                buf.writeInt (static_cast<int> (info.language));
                                writeStringRaw (buf, info.entryPoint);
                                writeStringRaw (buf, info.source);

                                if (info.inputSource.isNotEmpty())
                                    writeChunk (kFourCC_ISRC, [&]
                                    {
                                        buf.write (info.inputSource.toRawUTF8(), info.inputSource.getNumBytesAsUTF8());
                                    });

                                MemoryOutputStream reflBuf;
                                BinaryOutputArchive reflArchive (reflBuf);
                                detail::doSave (reflArchive, info.reflection);
                                writeBinaryChunk (buf, kFourCC_REFL, MemoryBlock (reflBuf.getData(), reflBuf.getDataSize()));
                            });
                        }
                    });
                });
            }
        });
    });

    if (! stream.write (buf.getData(), buf.getDataSize()))
        return Result::fail ("ShaderBundle: failed to write to stream");

    return Result::ok();
}

Result ShaderBundle::saveToFile (const File& file) const
{
    FileOutputStream fos (file);
    if (fos.failedToOpen())
        return Result::fail ("ShaderBundle: failed to open file for writing: " + file.getFullPathName());
    fos.setPosition (0);
    fos.truncate();
    return saveToStream (fos);
}

Result ShaderBundle::saveToMemoryBlock (MemoryBlock& block) const
{
    MemoryOutputStream mos (block, false);
    return saveToStream (mos);
}

//==============================================================================
// Load

ResultValue<ShaderBundle> ShaderBundle::loadFromStream (InputStream& stream)
{
    if (stream.getNumBytesRemaining() < 12)
        return makeResultValueFail ("ShaderBundle: stream too short for RIFF header");

    if (static_cast<uint32_t> (stream.readInt()) != kFourCC_RIFF)
        return makeResultValueFail ("ShaderBundle: missing RIFF signature");

    const auto riffSize = static_cast<uint32_t> (stream.readInt());

    if (static_cast<uint32_t> (stream.readInt()) != kFourCC_YSLB)
        return makeResultValueFail ("ShaderBundle: not an YSLB bundle (wrong form type)");

    const auto dataLen = static_cast<size_t> (riffSize - 4);
    MemoryBlock riffData (dataLen, false);
    if (stream.read (riffData.getData(), static_cast<int> (dataLen)) != static_cast<int> (dataLen))
        return makeResultValueFail ("ShaderBundle: truncated RIFF data");

    MemoryInputStream riffStream (riffData, false);

    ShaderBundle bundle;
    bool hasVersion = false;
    String loadError;

    // Parses a VART sub-stream into a ShaderInfo and appends it to the bundle.
    auto parseVariant = [&] (MemoryInputStream& vartStream, ShaderStage stage)
    {
        ShaderInfo info;
        info.stage = stage;
        info.language = static_cast<ShaderLanguage> (vartStream.readInt());
        info.entryPoint = readStringRaw (vartStream);
        info.source = readStringRaw (vartStream);

        iterateChunks (vartStream, [&] (uint32_t fourcc, MemoryInputStream& sub)
        {
            if (fourcc == kFourCC_ISRC)
            {
                const auto len = static_cast<int> (sub.getNumBytesRemaining());
                MemoryBlock srcData (static_cast<size_t> (len), false);
                sub.read (srcData.getData(), len);
                info.inputSource = String::fromUTF8 (static_cast<const char*> (srcData.getData()), len);
            }
            else if (fourcc == kFourCC_REFL)
            {
                BinaryInputArchive archive (sub);
                detail::doLoad (archive, info.reflection);
            }
        });

        bundle.shaders.push_back (std::move (info));
    };

    // Parses a SHDR sub-stream: reads stage/srcLang, then SPVB and LIST VARS chunks.
    auto parseShader = [&] (MemoryInputStream& shdrStream)
    {
        if (shdrStream.getNumBytesRemaining() < 8)
            return;

        const auto stage = static_cast<ShaderStage> (shdrStream.readInt());
        const auto srcLang = static_cast<ShaderLanguage> (shdrStream.readInt());
        MemoryBlock spirv;

        iterateChunks (shdrStream, [&] (uint32_t fourcc, MemoryInputStream& sub)
        {
            if (fourcc == kFourCC_SPVB)
            {
                const auto spvSize = static_cast<size_t> (sub.getNumBytesRemaining());
                if (spvSize > 0)
                {
                    spirv.setSize (spvSize, false);
                    sub.read (spirv.getData(), static_cast<int> (spvSize));
                }
            }
            else if (fourcc == kFourCC_LIST && sub.getNumBytesRemaining() >= 4)
            {
                if (static_cast<uint32_t> (sub.readInt()) == kFourCC_VARS)
                    iterateChunks (sub, [&] (uint32_t vartFourCC, MemoryInputStream& vartStream)
                    {
                        if (vartFourCC == kFourCC_VART)
                            parseVariant (vartStream, stage);
                    });
            }
        });

        if (! spirv.isEmpty())
            bundle.spirvBinaries[stage] = { srcLang, std::move (spirv) };
    };

    iterateChunks (riffStream, [&] (uint32_t fourcc, MemoryInputStream& chunkStream)
    {
        if (! loadError.isEmpty())
            return;

        if (fourcc == kFourCC_VERS)
        {
            if (chunkStream.getNumBytesRemaining() < 4)
                return;
            const auto version = static_cast<uint32_t> (chunkStream.readInt());
            if (version != kCurrentVersion)
                loadError = "ShaderBundle: bundle version " + String (version)
                          + " is not supported (expected " + String (kCurrentVersion) + ")";
            else
                hasVersion = true;
        }
        else if (fourcc == kFourCC_LIST && chunkStream.getNumBytesRemaining() >= 4)
        {
            if (static_cast<uint32_t> (chunkStream.readInt()) == kFourCC_SHAD)
                iterateChunks (chunkStream, [&] (uint32_t shdrFourCC, MemoryInputStream& shdrStream)
                {
                    if (shdrFourCC == kFourCC_SHDR)
                        parseShader (shdrStream);
                });
        }
    });

    if (! loadError.isEmpty())
        return makeResultValueFail (loadError);

    if (! hasVersion)
        return makeResultValueFail ("ShaderBundle: missing VERS chunk");

    return makeResultValueOk (std::move (bundle));
}

ResultValue<ShaderBundle> ShaderBundle::loadFromFile (const File& file)
{
    FileInputStream fis (file);
    if (fis.failedToOpen())
        return makeResultValueFail ("ShaderBundle: failed to open file: " + file.getFullPathName());
    return loadFromStream (fis);
}

ResultValue<ShaderBundle> ShaderBundle::loadFromData (const void* data, size_t size)
{
    MemoryInputStream mis (data, size, false);
    return loadFromStream (mis);
}

ResultValue<ShaderBundle> ShaderBundle::loadFromMemoryBlock (const MemoryBlock& block)
{
    return loadFromData (block.getData(), block.getSize());
}

} // namespace yup
