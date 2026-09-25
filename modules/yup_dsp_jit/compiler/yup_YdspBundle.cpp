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

namespace
{

constexpr int ydspBundleFormatVersion = 2;
constexpr int ydspProjectBundleFormatVersion = 3;
constexpr int ydspBundleLanguageVersion = 4;
constexpr int ydspBundleRuntimeAbiVersion = 2;
constexpr int ydspBundleNativeAbiVersion = 2;
constexpr int ydspBundleCodegenRevision = 14;
constexpr int ydspBundleLegacyLanguageVersion = 2;

constexpr uint32_t fourCC (char a, char b, char c, char d)
{
    return static_cast<uint32_t> (static_cast<uint8_t> (a))
         | (static_cast<uint32_t> (static_cast<uint8_t> (b)) << 8)
         | (static_cast<uint32_t> (static_cast<uint8_t> (c)) << 16)
         | (static_cast<uint32_t> (static_cast<uint8_t> (d)) << 24);
}

constexpr auto riff = fourCC ('R', 'I', 'F', 'F');
constexpr auto form = fourCC ('Y', 'D', 'S', 'P');
constexpr auto vers = fourCC ('V', 'E', 'R', 'S');
constexpr auto proj = fourCC ('P', 'R', 'O', 'J');
constexpr auto meta = fourCC ('M', 'E', 'T', 'A');
constexpr auto list = fourCC ('L', 'I', 'S', 'T');
constexpr auto sour = fourCC ('S', 'O', 'U', 'R');
constexpr auto srcf = fourCC ('S', 'R', 'C', 'F');
constexpr auto diag = fourCC ('D', 'I', 'A', 'G');
constexpr auto native = fourCC ('N', 'A', 'T', 'V');
constexpr auto wasm = fourCC ('W', 'A', 'S', 'M');

void writeBytes (MemoryOutputStream& stream, const std::vector<uint8_t>& bytes)
{
    stream.writeInt (static_cast<int> (bytes.size()));
    stream.write (bytes.data(), bytes.size());
}

bool readBytes (InputStream& stream, std::vector<uint8_t>& bytes)
{
    if (stream.getNumBytesRemaining() < 4)
        return false;
    const auto size = stream.readInt();
    if (size <= 0 || size > stream.getNumBytesRemaining())
        return false;
    bytes.resize (static_cast<size_t> (size));
    return stream.read (bytes.data(), size) == size;
}

void writeString (MemoryOutputStream& stream, const String& value)
{
    const auto size = static_cast<uint32_t> (value.getNumBytesAsUTF8());
    stream.writeInt (static_cast<int> (size));
    stream.write (value.toRawUTF8(), size);
}

bool readString (InputStream& stream, String& result)
{
    if (stream.getNumBytesRemaining() < 4)
        return false;

    const auto size = static_cast<uint32_t> (stream.readInt());
    if (size > static_cast<uint32_t> (stream.getNumBytesRemaining()))
        return false;

    if (size == 0)
    {
        result.clear();
        return true;
    }

    MemoryBlock bytes (size, false);
    if (stream.read (bytes.getData(), static_cast<int> (size)) != static_cast<int> (size))
        return false;

    result = String::fromUTF8 (static_cast<const char*> (bytes.getData()), static_cast<int> (size));
    return result.isEmpty() == (size == 0);
}

template <typename Function>
void writeChunk (MemoryOutputStream& stream, uint32_t id, Function&& function)
{
    stream.writeInt (static_cast<int> (id));

    const auto sizePosition = stream.getPosition();
    stream.writeInt (0);

    const auto start = stream.getPosition();

    function();

    const auto end = stream.getPosition();

    stream.setPosition (sizePosition);
    stream.writeInt (static_cast<int> (end - start));
    stream.setPosition (end);

    if ((end - start) & 1u)
        stream.writeByte (0);
}

} // namespace

//==============================================================================

Result YdspBundle::saveToStream (OutputStream& output) const
{
    MemoryOutputStream data;
    writeChunk (data, riff, [&]
                {
        data.writeInt (static_cast<int> (form));
        writeChunk (data, vers, [&]
                    {
            data.writeInt (projectMain.isEmpty() ? ydspBundleFormatVersion : ydspProjectBundleFormatVersion);
        });

        if (projectMain.isNotEmpty())
            writeChunk (data, proj, [&]
            {
                writeString (data, projectMain);
                writeString (data, JSON::toString (projectMetadata));
            });

        writeChunk (data, meta, [&]
                    {
            data.writeInt (ydspBundleLanguageVersion);
            data.writeInt (ydspBundleRuntimeAbiVersion);
            data.writeInt (ydspBundleNativeAbiVersion);
            data.writeInt (ydspBundleCodegenRevision);
            data.writeByte (fastMath ? 1 : 0);
            data.writeByte (enableTracing ? 1 : 0);
            data.writeByte (hasWasm ? 1 : 0);
            data.writeInt (nativeTargets.size());
            for (const auto& target : nativeTargets)
                writeString (data, target);
        });

        writeChunk (data, list, [&]
                    {
            data.writeInt (static_cast<int> (sour));
            for (const auto& source : sources)
            {
                writeChunk (data, srcf, [&]
                            {
                    writeString (data, source.id);
                    data.writeByte (source.isRoot ? 1 : 0);
                    writeString (data, source.source);
                });
            }

            for (const auto& edge : importEdges)
            {
                writeChunk (data, fourCC ('I', 'M', 'A', 'P'), [&]
                            {
                    writeString (data, edge.importingSourceId);
                    writeString (data, edge.spelling);
                    writeString (data, edge.importedSourceId);
                });
            }
        });

        for (const auto& artifact : nativeArtifacts)
        {
            writeChunk (data, native, [&]
                        {
                data.writeInt (static_cast<int> (artifact.target.operatingSystem));
                data.writeInt (static_cast<int> (artifact.target.architecture));
                data.writeInt (static_cast<int> (artifact.kernels.size()));
                for (const auto& kernel : artifact.kernels)
                {
                    writeBytes (data, kernel.code);
                    data.writeInt (static_cast<int> (kernel.symbols.size()));
                    for (const auto& symbol : kernel.symbols)
                    {
                        writeString (data, symbol.name);
                        data.writeInt64 (static_cast<int64_t> (symbol.offset));
                    }
                }
            });
        }

        if (! wasmModules.empty())
        {
            writeChunk (data, wasm, [&]
                        {
                data.writeInt (static_cast<int> (wasmModules.size()));
                for (const auto& module : wasmModules)
                    writeBytes (data, module);
            });
        }

        writeChunk (data, diag, [&]
                    {
            data.writeInt (diagnostics.getCount());
            for (int i = 0; i < diagnostics.getCount(); ++i)
            {
                const auto& item = diagnostics.getItem (i);
                data.writeInt (static_cast<int> (item.severity));
                writeString (data, item.sourceId);
                data.writeInt (item.range.startLine);
                data.writeInt (item.range.startColumn);
                data.writeInt (item.range.endLine);
                data.writeInt (item.range.endColumn);
                writeString (data, item.code);
                writeString (data, item.message);
            }
        });
    });

    return output.write (data.getData(), data.getDataSize())
             ? Result::ok()
             : Result::fail ("YdspBundle: failed to write stream");
}

Result YdspBundle::saveToFile (const File& file) const
{
    FileOutputStream stream (file);

    if (stream.failedToOpen())
        return Result::fail ("YdspBundle: failed to open output file");

    stream.truncate();
    return saveToStream (stream);
}

Result YdspBundle::saveToMemoryBlock (MemoryBlock& block) const
{
    MemoryOutputStream stream (block, false);
    return saveToStream (stream);
}

ResultValue<YdspBundle> YdspBundle::loadFromStream (InputStream& input)
{
    if (input.getNumBytesRemaining() < 12 || static_cast<uint32_t> (input.readInt()) != riff)
        return makeResultValueFail ("YdspBundle: invalid RIFF header");

    const auto size = static_cast<uint32_t> (input.readInt());
    if (size < 4 || size - 4 > static_cast<uint32_t> (input.getNumBytesRemaining()) || static_cast<uint32_t> (input.readInt()) != form)
        return makeResultValueFail ("YdspBundle: invalid RIFF payload");

    MemoryBlock payload (size - 4, false);
    if (input.read (payload.getData(), static_cast<int> (size - 4)) != static_cast<int> (size - 4))
        return makeResultValueFail ("YdspBundle: truncated RIFF payload");

    MemoryInputStream stream (payload, false);
    YdspBundle bundle;
    bool versionFound = false;
    bool metadataFound = false;
    bool projectFound = false;
    int formatVersion = 0;

    while (stream.getNumBytesRemaining() >= 8)
    {
        const auto id = static_cast<uint32_t> (stream.readInt());
        const auto chunkSize = static_cast<uint32_t> (stream.readInt());
        if (chunkSize > static_cast<uint32_t> (stream.getNumBytesRemaining()))
            return makeResultValueFail ("YdspBundle: malformed chunk size");

        MemoryBlock chunk (chunkSize, false);
        stream.read (chunk.getData(), static_cast<int> (chunkSize));
        if (chunkSize & 1u)
        {
            if (stream.getNumBytesRemaining() < 1)
                return makeResultValueFail ("YdspBundle: missing chunk padding");
            stream.skipNextBytes (1);
        }

        MemoryInputStream body (chunk, false);

        if (id == vers)
        {
            if (versionFound || chunkSize != 4)
                return makeResultValueFail ("YdspBundle: invalid version chunk");
            formatVersion = body.readInt();
            if (formatVersion != ydspBundleFormatVersion && formatVersion != ydspProjectBundleFormatVersion)
                return makeResultValueFail ("YdspBundle: unsupported format version");

            versionFound = true;
        }
        else if (id == proj)
        {
            String metadata;
            if (projectFound || ! readString (body, bundle.projectMain) || bundle.projectMain.isEmpty()
                || ! readString (body, metadata) || body.getNumBytesRemaining() != 0
                || JSON::parse (metadata, bundle.projectMetadata).failed() || ! bundle.projectMetadata.isObject())
                return makeResultValueFail ("YdspBundle: invalid project metadata");
            projectFound = true;
        }
        else if (id == list && chunkSize >= 4 && static_cast<uint32_t> (body.readInt()) == sour)
        {
            while (body.getNumBytesRemaining() >= 8)
            {
                const auto recordId = static_cast<uint32_t> (body.readInt());
                const auto recordSize = static_cast<uint32_t> (body.readInt());
                if (recordSize > static_cast<uint32_t> (body.getNumBytesRemaining()))
                    return makeResultValueFail ("YdspBundle: truncated source record");

                MemoryBlock recordData (recordSize, false);
                body.read (recordData.getData(), static_cast<int> (recordSize));
                if (recordSize & 1u)
                    body.skipNextBytes (1);

                MemoryInputStream record (recordData, false);
                if (recordId == srcf)
                {
                    YdspBundle::SourceFile source;
                    if (! readString (record, source.id) || record.getNumBytesRemaining() < 1)
                        return makeResultValueFail ("YdspBundle: malformed source record");

                    source.isRoot = record.readByte() != 0;

                    if (! readString (record, source.source) || source.id.isEmpty())
                        return makeResultValueFail ("YdspBundle: malformed source record");

                    for (const auto& existing : bundle.sources)
                        if (existing.id == source.id)
                            return makeResultValueFail ("YdspBundle: duplicate source ID");

                    bundle.sources.push_back (std::move (source));
                }
                else if (recordId == fourCC ('I', 'M', 'A', 'P'))
                {
                    YdspBundle::ImportEdge edge;
                    if (! readString (record, edge.importingSourceId)
                        || ! readString (record, edge.spelling)
                        || ! readString (record, edge.importedSourceId))
                        return makeResultValueFail ("YdspBundle: malformed import record");

                    bundle.importEdges.push_back (std::move (edge));
                }
                else
                {
                    return makeResultValueFail ("YdspBundle: invalid source record");
                }
            }
        }
        else if (id == native)
        {
            if (body.getNumBytesRemaining() < 12)
                return makeResultValueFail ("YdspBundle: truncated native target");

            const auto os = body.readInt();
            const auto arch = body.readInt();
            const auto count = body.readInt();
            if (os < 0 || os > static_cast<int> (YdspTargetOperatingSystem::windowsTarget)
                || arch < 0 || arch > static_cast<int> (YdspTargetArchitecture::x64)
                || count < 0 || count > body.getNumBytesRemaining() / 8)
                return makeResultValueFail ("YdspBundle: invalid native target");

            NativeTarget artifact;
            artifact.target = { static_cast<YdspTargetOperatingSystem> (os), static_cast<YdspTargetArchitecture> (arch) };

            for (const auto& existing : bundle.nativeArtifacts)
                if (existing.target == artifact.target)
                    return makeResultValueFail ("YdspBundle: duplicate native target");

            for (int i = 0; i < count; ++i)
            {
                YdspNativeArtifact kernel;
                if (! readBytes (body, kernel.code) || body.getNumBytesRemaining() < 4)
                    return makeResultValueFail ("YdspBundle: malformed native kernel");

                const auto symbols = body.readInt();
                if (symbols < 0 || symbols > body.getNumBytesRemaining() / 12)
                    return makeResultValueFail ("YdspBundle: invalid symbol count");

                for (int j = 0; j < symbols; ++j)
                {
                    YdspNativeArtifact::Symbol symbol;
                    if (! readString (body, symbol.name) || symbol.name.isEmpty() || body.getNumBytesRemaining() < 8)
                        return makeResultValueFail ("YdspBundle: malformed native symbol");
                    symbol.offset = static_cast<uint64_t> (body.readInt64());
                    if (symbol.offset > kernel.code.size() || kernel.code.size() - static_cast<size_t> (symbol.offset) < 8)
                        return makeResultValueFail ("YdspBundle: invalid symbol offset");
                    kernel.symbols.push_back (std::move (symbol));
                }

                artifact.kernels.push_back (std::move (kernel));
            }

            if (body.getNumBytesRemaining() != 0)
                return makeResultValueFail ("YdspBundle: trailing native target data");

            bundle.nativeArtifacts.push_back (std::move (artifact));
        }
        else if (id == wasm)
        {
            if (! bundle.wasmModules.empty() || body.getNumBytesRemaining() < 4)
                return makeResultValueFail ("YdspBundle: invalid WebAssembly chunk");

            const auto count = body.readInt();
            if (count <= 0 || count > body.getNumBytesRemaining() / 4)
                return makeResultValueFail ("YdspBundle: invalid WebAssembly kernel count");

            for (int i = 0; i < count; ++i)
            {
                std::vector<uint8_t> bytes;

                if (! readBytes (body, bytes) || bytes.size() < 8
                    || bytes[0] != 0 || bytes[1] != 'a' || bytes[2] != 's' || bytes[3] != 'm'
                    || bytes[4] != 1 || bytes[5] != 0 || bytes[6] != 0 || bytes[7] != 0)
                    return makeResultValueFail ("YdspBundle: invalid WebAssembly module");

                bundle.wasmModules.push_back (std::move (bytes));
            }

            if (body.getNumBytesRemaining() != 0)
                return makeResultValueFail ("YdspBundle: trailing WebAssembly data");
        }
        else if (id == meta)
        {
            if (metadataFound || chunkSize < 23)
                return makeResultValueFail ("YdspBundle: invalid metadata");

            const auto languageVersion = body.readInt();
            if (languageVersion == ydspBundleLegacyLanguageVersion)
                return makeResultValueFail ("YdspBundle: 'input value' has been replaced by 'input parameter' and 'output value' by 'output parameter'; migrate the source and regenerate the bundle");

            if (languageVersion != ydspBundleLanguageVersion
                || body.readInt() != ydspBundleRuntimeAbiVersion
                || body.readInt() != ydspBundleNativeAbiVersion
                || body.readInt() != ydspBundleCodegenRevision)
                return makeResultValueFail ("YdspBundle: incompatible language, runtime ABI or codegen revision");

            metadataFound = true;

            bundle.fastMath = body.readByte() != 0;
            bundle.enableTracing = body.readByte() != 0;
            bundle.hasWasm = body.readByte() != 0;

            if (body.getNumBytesRemaining() >= 4)
            {
                const auto targetCount = body.readInt();
                if (targetCount < 0 || targetCount > 1024)
                    return makeResultValueFail ("YdspBundle: invalid native target count");

                for (int i = 0; i < targetCount; ++i)
                {
                    String target;
                    if (! readString (body, target) || target.isEmpty() || bundle.nativeTargets.contains (target))
                        return makeResultValueFail ("YdspBundle: invalid native target");

                    bundle.nativeTargets.add (target);
                }
            }
        }
        else if (id == diag)
        {
            if (body.getNumBytesRemaining() < 4)
                return makeResultValueFail ("YdspBundle: malformed diagnostics");

            const auto count = body.readInt();
            if (count < 0 || count > 100000)
                return makeResultValueFail ("YdspBundle: invalid diagnostic count");

            for (int i = 0; i < count; ++i)
            {
                if (body.getNumBytesRemaining() < 4)
                    return makeResultValueFail ("YdspBundle: malformed diagnostic");

                YdspDiagnostic item;

                item.severity = static_cast<YdspSeverity> (body.readInt());
                if ((item.severity != YdspSeverity::error && item.severity != YdspSeverity::warning && item.severity != YdspSeverity::info)
                    || ! readString (body, item.sourceId)
                    || body.getNumBytesRemaining() < 16)
                    return makeResultValueFail ("YdspBundle: malformed diagnostic");

                item.range.startLine = body.readInt();
                item.range.startColumn = body.readInt();
                item.range.endLine = body.readInt();
                item.range.endColumn = body.readInt();

                if (! readString (body, item.code) || ! readString (body, item.message))
                    return makeResultValueFail ("YdspBundle: malformed diagnostic");

                bundle.diagnostics.add (std::move (item));
            }
        }
    }

    if (! versionFound || ! metadataFound || (projectFound != (formatVersion == ydspProjectBundleFormatVersion)) || bundle.sources.empty()
        || (bundle.nativeArtifacts.empty() && bundle.wasmModules.empty())
        || bundle.hasWasm != ! bundle.wasmModules.empty())
    {
        return makeResultValueFail ("YdspBundle: missing required chunks");
    }

    StringArray emittedTargets;
    for (const auto& artifact : bundle.nativeArtifacts)
    {
        const String os = artifact.target.operatingSystem == YdspTargetOperatingSystem::macosTarget   ? "macos-"
                        : artifact.target.operatingSystem == YdspTargetOperatingSystem::windowsTarget ? "windows-"
                                                                                                      : "linux-";

        emittedTargets.add (os + (artifact.target.architecture == YdspTargetArchitecture::arm64 ? "arm64" : "x64"));
    }

    emittedTargets.sort (false);

    auto declaredTargets = bundle.nativeTargets;
    declaredTargets.sort (false);

    if (emittedTargets != declaredTargets)
        return makeResultValueFail ("YdspBundle: target metadata does not match emitted artifacts");

    int rootCount = 0;
    for (const auto& source : bundle.sources)
        rootCount += source.isRoot ? 1 : 0;

    if (rootCount != 1)
        return makeResultValueFail ("YdspBundle: invalid root source count");

    for (const auto& edge : bundle.importEdges)
    {
        const auto sourceExists = [&] (const String& id)
        {
            return std::any_of (bundle.sources.begin(), bundle.sources.end(), [&] (const YdspBundle::SourceFile& source)
                                {
                return source.id == id;
            });
        };

        if (! sourceExists (edge.importingSourceId) || ! sourceExists (edge.importedSourceId))
            return makeResultValueFail ("YdspBundle: dangling import map entry");
    }

    for (const auto& source : bundle.sources)
        bundle.diagnostics.registerSource (source.id, source.source);

    return makeResultValueOk (std::move (bundle));
}

ResultValue<YdspBundle> YdspBundle::loadFromFile (const File& file)
{
    FileInputStream stream (file);

    if (stream.failedToOpen())
        return makeResultValueFail ("YdspBundle: failed to open input file");

    return loadFromStream (stream);
}

ResultValue<YdspBundle> YdspBundle::loadFromData (const void* data, size_t size)
{
    MemoryInputStream stream (data, size, false);
    return loadFromStream (stream);
}

ResultValue<YdspBundle> YdspBundle::loadFromMemoryBlock (const MemoryBlock& block)
{
    return loadFromData (block.getData(), block.getSize());
}

ResultValue<YdspAudioGraph> YdspBundle::instantiate() const
{
    const auto root = std::find_if (sources.begin(), sources.end(), [] (const SourceFile& source) { return source.isRoot; });
    if (root == sources.end())
        return ResultValue<YdspAudioGraph>::fail ("YdspBundle: no root source");

    YdspCompiler compiler;
    YdspCompileOptions options;
    options.fastMath = fastMath;
    options.enableTracing = enableTracing;
    options.optimizationTier = YdspOptimizationTier::baseline;
    options.targetPolicy = YdspTargetPolicy::baseline;
    return compiler.compileInternal (root->source, options, root->id, nullptr, nullptr, this, nullptr);
}

} // namespace yup
