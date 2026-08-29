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

namespace
{

//==============================================================================
// LEB128 encodings (see https://webassembly.github.io/spec/core/binary/values.html)

void appendUnsignedLeb (std::vector<uint8_t>& dst, uint64_t value)
{
    while (value >= 0x80)
    {
        dst.push_back (static_cast<uint8_t> (value) | 0x80);
        value >>= 7;
    }

    dst.push_back (static_cast<uint8_t> (value));
}

void appendSignedLeb (std::vector<uint8_t>& dst, int64_t value)
{
    bool more = true;

    while (more)
    {
        const auto byte = static_cast<uint8_t> (value) & 0x7F;
        value >>= 7;

        const bool signBitSet = (byte & 0x40) != 0;
        more = ! ((value == 0 && ! signBitSet) || (value == -1 && signBitSet));

        dst.push_back (static_cast<uint8_t> (byte | (more ? 0x80 : 0x00)));
    }
}

void appendName (std::vector<uint8_t>& dst, const char* name)
{
    const auto length = std::strlen (name);
    appendUnsignedLeb (dst, static_cast<uint64_t> (length));

    for (size_t i = 0; i < length; ++i)
        dst.push_back (static_cast<uint8_t> (name[i]));
}

} // namespace

//==============================================================================

void YdspWasmEmitter::u32 (uint32_t value)
{
    appendUnsignedLeb (target(), value);
}

void YdspWasmEmitter::s32 (int32_t value)
{
    appendSignedLeb (target(), value);
}

void YdspWasmEmitter::u64 (uint64_t value)
{
    appendUnsignedLeb (target(), value);
}

void YdspWasmEmitter::s64 (int64_t value)
{
    appendSignedLeb (target(), value);
}

void YdspWasmEmitter::bytes (const void* data, size_t numBytes)
{
    if (numBytes == 0)
        return;

    const auto* first = static_cast<const uint8_t*> (data);
    auto& dst = target();
    dst.insert (dst.end(), first, first + numBytes);
}

//==============================================================================

void YdspWasmEmitter::beginModule()
{
    out.clear();
    sectionPayload.clear();
    body.clear();
    localRuns.clear();
    currentSectionId = 0;
    typeCount = 0;
    importCount = 0;
    funcImportCount = 0;
    funcCount = 0;
    exportCount = 0;
    inSection = false;
    inBody = false;

    // Magic "\0asm" and version 1 (MVP). The version is a fixed 4-byte
    // little-endian u32 in the binary format (not LEB128).
    u8 (0x00);
    u8 (0x61);
    u8 (0x73);
    u8 (0x6D);
    u8 (0x01);
    u8 (0x00);
    u8 (0x00);
    u8 (0x00);
}

void YdspWasmEmitter::beginSection (uint8_t sectionId)
{
    jassert (! inSection);
    jassert (! inBody);

    inSection = true;
    currentSectionId = sectionId;
    sectionPayload.clear();
}

void YdspWasmEmitter::endSection()
{
    jassert (inSection);

    // Write the section id and size prefix directly into the final buffer
    // (the routed writers would otherwise append into sectionPayload).
    out.push_back (currentSectionId);
    appendUnsignedLeb (out, sectionPayload.size());
    out.insert (out.end(), sectionPayload.begin(), sectionPayload.end());

    inSection = false;
    sectionPayload.clear();
}

//==============================================================================

int YdspWasmEmitter::funcType (std::initializer_list<ValType> params, std::initializer_list<ValType> results)
{
    sectionPayload.push_back (0x60); // functype form

    appendUnsignedLeb (sectionPayload, params.size());

    for (const auto type : params)
        sectionPayload.push_back (static_cast<uint8_t> (type));

    appendUnsignedLeb (sectionPayload, results.size());

    for (const auto type : results)
        sectionPayload.push_back (static_cast<uint8_t> (type));

    return static_cast<int> (typeCount++);
}

void YdspWasmEmitter::importMemory (const char* moduleName, const char* name, uint32_t minPages, uint32_t maxPages, bool shared)
{
    appendName (sectionPayload, moduleName);
    appendName (sectionPayload, name);

    sectionPayload.push_back (importKindMemory);

    // Limits flags: 0x00 = min only, 0x01 = min+max, 0x03 = shared min+max.
    // A shared import must declare a maximum (per the wasm spec); encoding
    // 0x02 (shared min only) is invalid and never emitted.
    if (shared)
    {
        jassert (maxPages > 0);
        sectionPayload.push_back (0x03);
        appendUnsignedLeb (sectionPayload, minPages);
        appendUnsignedLeb (sectionPayload, maxPages);
    }
    else if (maxPages > 0)
    {
        sectionPayload.push_back (0x01);
        appendUnsignedLeb (sectionPayload, minPages);
        appendUnsignedLeb (sectionPayload, maxPages);
    }
    else
    {
        sectionPayload.push_back (0x00);
        appendUnsignedLeb (sectionPayload, minPages);
    }

    ++importCount;
}

void YdspWasmEmitter::importFunction (const char* moduleName, const char* name, uint32_t typeIndex)
{
    appendName (sectionPayload, moduleName);
    appendName (sectionPayload, name);

    sectionPayload.push_back (importKindFunc);
    appendUnsignedLeb (sectionPayload, typeIndex);

    ++importCount;
    ++funcImportCount;
}

int YdspWasmEmitter::functionEntry (uint32_t typeIndex)
{
    appendUnsignedLeb (sectionPayload, typeIndex);

    // Function indices come from the function index space: imported functions
    // first, then defined functions. Memory/table/global imports live in their
    // own index spaces and do not count here.
    const auto index = static_cast<int> (funcImportCount) + static_cast<int> (funcCount);
    ++funcCount;

    return index;
}

void YdspWasmEmitter::exportFunction (const char* name, uint32_t funcIndex)
{
    appendName (sectionPayload, name);

    sectionPayload.push_back (exportKindFunc);
    appendUnsignedLeb (sectionPayload, funcIndex);

    ++exportCount;
}

//==============================================================================

void YdspWasmEmitter::beginBody()
{
    jassert (! inBody);
    jassert (inSection);

    inBody = true;
    body.clear();
    localRuns.clear();
}

void YdspWasmEmitter::declareLocals (uint32_t count, ValType type)
{
    jassert (inBody);
    jassert (count > 0);

    localRuns.emplace_back (count, type);
}

void YdspWasmEmitter::endBody()
{
    jassert (inBody);

    // Flatten the local runs into `vec(locals)` = count of (count, type) groups.
    std::vector<uint8_t> localsDecl;
    appendUnsignedLeb (localsDecl, localRuns.size());

    for (const auto& [count, type] : localRuns)
    {
        appendUnsignedLeb (localsDecl, count);
        localsDecl.push_back (static_cast<uint8_t> (type));
    }

    // Body = locals declaration + instructions + trailing `end` opcode.
    const auto bodySize = static_cast<uint64_t> (localsDecl.size()) + body.size() + 1;

    appendUnsignedLeb (sectionPayload, bodySize);
    sectionPayload.insert (sectionPayload.end(), localsDecl.begin(), localsDecl.end());
    sectionPayload.insert (sectionPayload.end(), body.begin(), body.end());
    sectionPayload.push_back (opEnd);

    inBody = false;
    body.clear();
    localRuns.clear();
}

//==============================================================================

void YdspWasmEmitter::op (uint8_t opcode)
{
    jassert (inBody);
    body.push_back (opcode);
}

void YdspWasmEmitter::block()
{
    op (opBlock);
    body.push_back (emptyBlockType);
}

void YdspWasmEmitter::loop()
{
    op (opLoop);
    body.push_back (emptyBlockType);
}

void YdspWasmEmitter::if_()
{
    op (opIf);
    body.push_back (emptyBlockType);
}

void YdspWasmEmitter::ifValue (ValType resultType)
{
    op (opIf);
    body.push_back (static_cast<uint8_t> (resultType));
}

void YdspWasmEmitter::else_()
{
    op (opElse);
}

void YdspWasmEmitter::end()
{
    op (opEnd);
}

void YdspWasmEmitter::br (uint32_t depth)
{
    op (opBr);
    appendUnsignedLeb (body, depth);
}

void YdspWasmEmitter::brIf (uint32_t depth)
{
    op (opBrIf);
    appendUnsignedLeb (body, depth);
}

void YdspWasmEmitter::ret()
{
    op (opReturn);
}

void YdspWasmEmitter::call (uint32_t funcIndex)
{
    op (opCall);
    appendUnsignedLeb (body, funcIndex);
}

void YdspWasmEmitter::drop()
{
    op (opDrop);
}

void YdspWasmEmitter::select()
{
    op (opSelect);
}

void YdspWasmEmitter::localGet (uint32_t index)
{
    op (opLocalGet);
    appendUnsignedLeb (body, index);
}

void YdspWasmEmitter::localSet (uint32_t index)
{
    op (opLocalSet);
    appendUnsignedLeb (body, index);
}

void YdspWasmEmitter::localTee (uint32_t index)
{
    op (opLocalTee);
    appendUnsignedLeb (body, index);
}

void YdspWasmEmitter::i32Const (int32_t value)
{
    op (opI32Const);
    appendSignedLeb (body, value);
}

void YdspWasmEmitter::i64Const (int64_t value)
{
    op (opI64Const);
    appendSignedLeb (body, value);
}

void YdspWasmEmitter::f32Const (float value)
{
    op (opF32Const);

    uint32_t bits = 0;
    std::memcpy (&bits, &value, sizeof (bits));

    for (int i = 0; i < 4; ++i)
        body.push_back (static_cast<uint8_t> (bits >> (i * 8)));
}

void YdspWasmEmitter::f64Const (double value)
{
    op (opF64Const);

    uint64_t bits = 0;
    std::memcpy (&bits, &value, sizeof (bits));

    for (int i = 0; i < 8; ++i)
        body.push_back (static_cast<uint8_t> (bits >> (i * 8)));
}

void YdspWasmEmitter::load (uint8_t opcode, uint32_t alignLog2, uint32_t offset)
{
    op (opcode);
    appendUnsignedLeb (body, alignLog2);
    appendUnsignedLeb (body, offset);
}

void YdspWasmEmitter::store (uint8_t opcode, uint32_t alignLog2, uint32_t offset)
{
    op (opcode);
    appendUnsignedLeb (body, alignLog2);
    appendUnsignedLeb (body, offset);
}

void YdspWasmEmitter::nop()
{
    op (opNop);
}

void YdspWasmEmitter::unreachable()
{
    op (opUnreachable);
}

#if defined (__wasm_simd128__)

void YdspWasmEmitter::simdOp (uint32_t subopcode)
{
    op (simdPrefix);
    appendUnsignedLeb (body, subopcode);
}

void YdspWasmEmitter::simdLoad (uint32_t subopcode, uint32_t alignLog2, uint32_t offset)
{
    op (simdPrefix);
    appendUnsignedLeb (body, subopcode);
    appendUnsignedLeb (body, alignLog2);
    appendUnsignedLeb (body, offset);
}

void YdspWasmEmitter::simdStore (uint32_t subopcode, uint32_t alignLog2, uint32_t offset)
{
    op (simdPrefix);
    appendUnsignedLeb (body, subopcode);
    appendUnsignedLeb (body, alignLog2);
    appendUnsignedLeb (body, offset);
}

void YdspWasmEmitter::v128Const (const uint8_t data[16])
{
    op (simdPrefix);
    appendUnsignedLeb (body, 0x0C);
    bytes (data, 16);
}

void YdspWasmEmitter::i8x16Shuffle (const uint8_t mask[16])
{
    op (simdPrefix);
    appendUnsignedLeb (body, opI8x16Shuffle);
    bytes (mask, 16);
}

void YdspWasmEmitter::f32x4ExtractLane (uint32_t lane)
{
    jassert (lane < 4);

    op (simdPrefix);
    appendUnsignedLeb (body, opF32x4ExtractLane);
    u8 (static_cast<uint8_t> (lane));
}

#endif

} // namespace yup
