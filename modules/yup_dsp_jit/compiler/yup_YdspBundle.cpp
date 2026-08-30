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
constexpr auto meta = fourCC ('M', 'E', 'T', 'A');
constexpr auto list = fourCC ('L', 'I', 'S', 'T');
constexpr auto sour = fourCC ('S', 'O', 'U', 'R');
constexpr auto srcf = fourCC ('S', 'R', 'C', 'F');
constexpr auto diag = fourCC ('D', 'I', 'A', 'G');

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
        writeChunk (data, vers, [&] { data.writeInt (1); });

        writeChunk (data, meta, [&]
        {
            data.writeInt (2); // language version
            data.writeInt (1); // graph/runtime ABI
            data.writeInt (1); // native ABI
            data.writeInt (1); // codegen revision
            data.writeByte (fastMath ? 1 : 0);
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
             ? Result::ok() : Result::fail ("YdspBundle: failed to write stream");
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
            if (chunkSize != 4 || body.readInt() != 1)
                return makeResultValueFail ("YdspBundle: unsupported format version");

            versionFound = true;
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
                if (recordSize & 1u) body.skipNextBytes (1);

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
        else if (id == meta && chunkSize >= 6)
        {
            body.readInt(); body.readInt(); body.readInt(); body.readInt();
            bundle.fastMath = body.readByte() != 0;
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

    if (! versionFound || bundle.sources.empty())
        return makeResultValueFail ("YdspBundle: missing required chunks");

    int rootCount = 0;
    for (const auto& source : bundle.sources)
        rootCount += source.isRoot ? 1 : 0;
    if (rootCount != 1)
        return makeResultValueFail ("YdspBundle: invalid root source count");

    for (const auto& edge : bundle.importEdges)
    {
        const auto sourceExists = [&] (const String& id)
        {
            return std::any_of (bundle.sources.begin(), bundle.sources.end(), [&] (const YdspBundle::SourceFile& source) { return source.id == id; });
        };

        if (! sourceExists (edge.importingSourceId) || ! sourceExists (edge.importedSourceId))
            return makeResultValueFail ("YdspBundle: dangling import map entry");
    }

    return makeResultValueOk (std::move (bundle));
}

ResultValue<YdspBundle> YdspBundle::loadFromFile (const File& file)
{
    FileInputStream stream (file);
    if (stream.failedToOpen()) return makeResultValueFail ("YdspBundle: failed to open input file");
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
    return compiler.compile (root->source, options);
}

} // namespace yup
