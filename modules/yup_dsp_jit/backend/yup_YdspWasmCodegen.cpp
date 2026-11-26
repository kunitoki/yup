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

using ValType = YdspWasmEmitter::ValType;

//==============================================================================

constexpr int kernelInputsOff = 0;
constexpr int kernelOutputsOff = 4;
constexpr int kernelParamsOff = 8;
constexpr int kernelParamOutOff = 12;
constexpr int kernelStateOff = 16;
constexpr int kernelStateArraysOff = 20;
constexpr int kernelSampleRateOff = 24;
constexpr int kernelNumSamplesOff = 28;
constexpr int kernelOutputEventsOff = 32;
constexpr int kernelTraceQueueOff = 36;

constexpr int eventStateOff = 0;
constexpr int eventStateArraysOff = 4;
constexpr int eventParamsOff = 8;
constexpr int eventSampleRateOff = 12;
constexpr int eventChannelOff = 48;
constexpr int eventSampleOffsetOff = 52;
constexpr int eventOutputEventsOff = 56;
constexpr int eventTraceQueueOff = 60;

constexpr int wasmEventPayloadOffset (int hostOffset) noexcept
{
    return hostOffset - static_cast<int> (offsetof (YdspEventContext, pitch)) + 16;
}

#if YUP_WASM
static_assert (offsetof (YdspKernelContext, inputs) == kernelInputsOff);
static_assert (offsetof (YdspKernelContext, outputs) == kernelOutputsOff);
static_assert (offsetof (YdspKernelContext, params) == kernelParamsOff);
static_assert (offsetof (YdspKernelContext, paramOut) == kernelParamOutOff);
static_assert (offsetof (YdspKernelContext, state) == kernelStateOff);
static_assert (offsetof (YdspKernelContext, stateArrays) == kernelStateArraysOff);
static_assert (offsetof (YdspKernelContext, sampleRate) == kernelSampleRateOff);
static_assert (offsetof (YdspKernelContext, numSamples) == kernelNumSamplesOff);
static_assert (offsetof (YdspKernelContext, outputEvents) == kernelOutputEventsOff);
static_assert (offsetof (YdspKernelContext, traceQueue) == kernelTraceQueueOff);
static_assert (offsetof (YdspEventContext, traceQueue) == eventTraceQueueOff);

static_assert (offsetof (YdspEventContext, state) == eventStateOff);
static_assert (offsetof (YdspEventContext, stateArrays) == eventStateArraysOff);
static_assert (offsetof (YdspEventContext, params) == eventParamsOff);
static_assert (offsetof (YdspEventContext, sampleRate) == eventSampleRateOff);
static_assert (offsetof (YdspEventContext, channel) == eventChannelOff);
static_assert (offsetof (YdspEventContext, sampleOffset) == eventSampleOffsetOff);
static_assert (offsetof (YdspEventContext, outputEvents) == eventOutputEventsOff);
#endif

//==============================================================================
// Small type helpers

ValType toWasmType (YdspValueType type) noexcept
{
    switch (type)
    {
        case YdspValueType::float32Type:
            return ValType::f32;
        case YdspValueType::float64Type:
            return ValType::f64;
        case YdspValueType::int64Type:
            return ValType::i64;
        case YdspValueType::int32Type:
        case YdspValueType::boolType:
            return ValType::i32;
    }

    return ValType::i32;
}

uint32_t scaleLog2 (YdspValueType type) noexcept
{
    return is64BitValueType (type) ? 3u : 2u;
}

//==============================================================================
// Opcode selection helpers (dispatch on value width where wasm splits ops)

uint8_t loadOpcode (YdspValueType type) noexcept
{
    switch (type)
    {
        case YdspValueType::float32Type:
            return YdspWasmEmitter::opF32Load;
        case YdspValueType::float64Type:
            return YdspWasmEmitter::opF64Load;
        case YdspValueType::int64Type:
            return YdspWasmEmitter::opI64Load;
        case YdspValueType::int32Type:
        case YdspValueType::boolType:
            return YdspWasmEmitter::opI32Load;
    }

    return YdspWasmEmitter::opI32Load;
}

uint8_t storeOpcode (YdspValueType type) noexcept
{
    switch (type)
    {
        case YdspValueType::float32Type:
            return YdspWasmEmitter::opF32Store;
        case YdspValueType::float64Type:
            return YdspWasmEmitter::opF64Store;
        case YdspValueType::int64Type:
            return YdspWasmEmitter::opI64Store;
        case YdspValueType::int32Type:
        case YdspValueType::boolType:
            return YdspWasmEmitter::opI32Store;
    }

    return YdspWasmEmitter::opI32Store;
}

uint8_t floatBinaryOpcode (YdspIrOp op, bool is64) noexcept
{
    switch (op)
    {
        case YdspIrOp::addF:
            return is64 ? YdspWasmEmitter::opF64Add : YdspWasmEmitter::opF32Add;
        case YdspIrOp::subF:
            return is64 ? YdspWasmEmitter::opF64Sub : YdspWasmEmitter::opF32Sub;
        case YdspIrOp::mulF:
            return is64 ? YdspWasmEmitter::opF64Mul : YdspWasmEmitter::opF32Mul;
        case YdspIrOp::divF:
            return is64 ? YdspWasmEmitter::opF64Div : YdspWasmEmitter::opF32Div;
        case YdspIrOp::minF:
            return is64 ? YdspWasmEmitter::opF64Min : YdspWasmEmitter::opF32Min;
        case YdspIrOp::maxF:
            return is64 ? YdspWasmEmitter::opF64Max : YdspWasmEmitter::opF32Max;
        default:
            break;
    }

    return 0;
}

uint8_t floatUnaryOpcode (YdspIrOp op, bool is64) noexcept
{
    switch (op)
    {
        case YdspIrOp::negF:
            return is64 ? YdspWasmEmitter::opF64Neg : YdspWasmEmitter::opF32Neg;
        case YdspIrOp::absF:
            return is64 ? YdspWasmEmitter::opF64Abs : YdspWasmEmitter::opF32Abs;
        case YdspIrOp::sqrtF:
            return is64 ? YdspWasmEmitter::opF64Sqrt : YdspWasmEmitter::opF32Sqrt;
        case YdspIrOp::floorF:
            return is64 ? YdspWasmEmitter::opF64Floor : YdspWasmEmitter::opF32Floor;
        case YdspIrOp::ceilF:
            return is64 ? YdspWasmEmitter::opF64Ceil : YdspWasmEmitter::opF32Ceil;
        case YdspIrOp::rintF:
            return is64 ? YdspWasmEmitter::opF64Nearest : YdspWasmEmitter::opF32Nearest;
        default:
            break;
    }

    return 0;
}

#if defined(__wasm_simd128__)

// Packed f32x4 twins of the scalar float opcodes the vectoriser widens. The
// returned subopcode is emitted behind the 0xFD prefix; 0 means "not packed".
uint32_t packedFloatBinarySimdOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::addF:
            return YdspWasmEmitter::opF32x4Add;
        case YdspIrOp::subF:
            return YdspWasmEmitter::opF32x4Sub;
        case YdspIrOp::mulF:
            return YdspWasmEmitter::opF32x4Mul;
        case YdspIrOp::divF:
            return YdspWasmEmitter::opF32x4Div;
        case YdspIrOp::minF:
            return YdspWasmEmitter::opF32x4Min;
        case YdspIrOp::maxF:
            return YdspWasmEmitter::opF32x4Max;
        default:
            break;
    }

    return 0;
}

uint32_t packedFloatUnarySimdOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::negF:
            return YdspWasmEmitter::opF32x4Neg;
        case YdspIrOp::absF:
            return YdspWasmEmitter::opF32x4Abs;
        case YdspIrOp::sqrtF:
            return YdspWasmEmitter::opF32x4Sqrt;
        case YdspIrOp::floorF:
            return YdspWasmEmitter::opF32x4Floor;
        case YdspIrOp::ceilF:
            return YdspWasmEmitter::opF32x4Ceil;
        case YdspIrOp::rintF:
            return YdspWasmEmitter::opF32x4Nearest;
        default:
            break;
    }

    return 0;
}

// Packed f32x4 twins of the scalar float comparisons. A widened compare is
// the AsmJit cmpps/fcmeq semantics: a per-lane all-ones/all-zeros mask, which
// is exactly what wasm f32x4.* produces - so a widened selectB can consume the
// result directly through v128.bitselect. 0 means "not packed".
uint32_t packedFloatCompareSimdOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::eqF:
            return YdspWasmEmitter::opF32x4Eq;
        case YdspIrOp::neF:
            return YdspWasmEmitter::opF32x4Ne;
        case YdspIrOp::ltF:
            return YdspWasmEmitter::opF32x4Lt;
        case YdspIrOp::leF:
            return YdspWasmEmitter::opF32x4Le;
        case YdspIrOp::gtF:
            return YdspWasmEmitter::opF32x4Gt;
        case YdspIrOp::geF:
            return YdspWasmEmitter::opF32x4Ge;
        default:
            break;
    }

    return 0;
}

uint32_t packedIntCompareSimdOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::eqI:
            return YdspWasmEmitter::opI32x4Eq;
        case YdspIrOp::neI:
            return YdspWasmEmitter::opI32x4Ne;
        case YdspIrOp::ltI:
            return YdspWasmEmitter::opI32x4LtS;
        case YdspIrOp::leI:
            return YdspWasmEmitter::opI32x4LeS;
        case YdspIrOp::gtI:
            return YdspWasmEmitter::opI32x4GtS;
        case YdspIrOp::geI:
            return YdspWasmEmitter::opI32x4GeS;
        default:
            break;
    }

    return 0;
}

#endif

// Round-toward-zero (used by the modF lowering; there is no IR op for it).
uint8_t floatTruncOpcode (bool is64) noexcept
{
    return is64 ? YdspWasmEmitter::opF64Trunc : YdspWasmEmitter::opF32Trunc;
}

uint8_t intBinaryOpcode (YdspIrOp op, bool is64) noexcept
{
    switch (op)
    {
        case YdspIrOp::addI:
            return is64 ? YdspWasmEmitter::opI64Add : YdspWasmEmitter::opI32Add;
        case YdspIrOp::subI:
            return is64 ? YdspWasmEmitter::opI64Sub : YdspWasmEmitter::opI32Sub;
        case YdspIrOp::mulI:
            return is64 ? YdspWasmEmitter::opI64Mul : YdspWasmEmitter::opI32Mul;
        case YdspIrOp::andI:
        case YdspIrOp::andB:
            return is64 ? YdspWasmEmitter::opI64And : YdspWasmEmitter::opI32And;
        case YdspIrOp::orI:
        case YdspIrOp::orB:
            return is64 ? YdspWasmEmitter::opI64Or : YdspWasmEmitter::opI32Or;
        case YdspIrOp::xorI:
            return is64 ? YdspWasmEmitter::opI64Xor : YdspWasmEmitter::opI32Xor;
        default:
            break;
    }

    return 0;
}

uint8_t intDivRemOpcode (YdspIrOp op, bool is64) noexcept
{
    if (op == YdspIrOp::divI)
        return is64 ? YdspWasmEmitter::opI64DivS : YdspWasmEmitter::opI32DivS;

    return is64 ? YdspWasmEmitter::opI64RemS : YdspWasmEmitter::opI32RemS;
}

uint8_t floatCompareOpcode (YdspIrOp op, bool is64) noexcept
{
    switch (op)
    {
        case YdspIrOp::eqF:
            return is64 ? YdspWasmEmitter::opF64Eq : YdspWasmEmitter::opF32Eq;
        case YdspIrOp::neF:
            return is64 ? YdspWasmEmitter::opF64Ne : YdspWasmEmitter::opF32Ne;
        case YdspIrOp::ltF:
            return is64 ? YdspWasmEmitter::opF64Lt : YdspWasmEmitter::opF32Lt;
        case YdspIrOp::leF:
            return is64 ? YdspWasmEmitter::opF64Le : YdspWasmEmitter::opF32Le;
        case YdspIrOp::gtF:
            return is64 ? YdspWasmEmitter::opF64Gt : YdspWasmEmitter::opF32Gt;
        case YdspIrOp::geF:
            return is64 ? YdspWasmEmitter::opF64Ge : YdspWasmEmitter::opF32Ge;
        default:
            break;
    }

    return 0;
}

uint8_t intCompareOpcode (YdspIrOp op, bool is64) noexcept
{
    switch (op)
    {
        case YdspIrOp::eqI:
            return is64 ? YdspWasmEmitter::opI64Eq : YdspWasmEmitter::opI32Eq;
        case YdspIrOp::neI:
            return is64 ? YdspWasmEmitter::opI64Ne : YdspWasmEmitter::opI32Ne;
        case YdspIrOp::ltUI:
            return is64 ? YdspWasmEmitter::opI64LtU : YdspWasmEmitter::opI32LtU;
        case YdspIrOp::ltI:
            return is64 ? YdspWasmEmitter::opI64LtS : YdspWasmEmitter::opI32LtS;
        case YdspIrOp::leI:
            return is64 ? YdspWasmEmitter::opI64LeS : YdspWasmEmitter::opI32LeS;
        case YdspIrOp::gtI:
            return is64 ? YdspWasmEmitter::opI64GtS : YdspWasmEmitter::opI32GtS;
        case YdspIrOp::geI:
            return is64 ? YdspWasmEmitter::opI64GeS : YdspWasmEmitter::opI32GeS;
        default:
            break;
    }

    return 0;
}

YdspIrOp flippedComparisonOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::ltF: return YdspIrOp::gtF;
        case YdspIrOp::leF: return YdspIrOp::geF;
        case YdspIrOp::gtF: return YdspIrOp::ltF;
        case YdspIrOp::geF: return YdspIrOp::leF;
        case YdspIrOp::ltI: return YdspIrOp::gtI;
        case YdspIrOp::leI: return YdspIrOp::geI;
        case YdspIrOp::gtI: return YdspIrOp::ltI;
        case YdspIrOp::geI: return YdspIrOp::leI;
        default:            return op;
    }
}

// Conversion opcode for `dst = convert (src)`.
uint8_t convertOpcode (YdspIrOp op, YdspValueType src, YdspValueType dst) noexcept
{
    const bool srcIs64 = is64BitValueType (src);
    const bool dstIs64 = is64BitValueType (dst);

    switch (op)
    {
        case YdspIrOp::itof: // int -> float
            if (srcIs64)
                return dstIs64 ? YdspWasmEmitter::opF64ConvertI64S : YdspWasmEmitter::opF32ConvertI64S;

            return dstIs64 ? YdspWasmEmitter::opF64ConvertI32S : YdspWasmEmitter::opF32ConvertI32S;

        case YdspIrOp::ftoi: // float -> int (truncate toward zero)
            if (srcIs64)
                return dstIs64 ? YdspWasmEmitter::opI64TruncF64S : YdspWasmEmitter::opI32TruncF64S;

            return dstIs64 ? YdspWasmEmitter::opI64TruncF32S : YdspWasmEmitter::opI32TruncF32S;

        default:
            break;
    }

    return 0;
}

//==============================================================================
// libm intrinsics: names and signatures. f32 variants use the C `*f` name
// ("sinf"), f64 variants the C name ("sin"). The JS glue provides them as
// `env.<name>` imports backed by Math.* (with exact C semantics where the JS
// built-in differs, e.g. round and copysign).

const char* libmBaseName (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::sinF:
            return "sin";
        case YdspIrOp::cosF:
            return "cos";
        case YdspIrOp::tanF:
            return "tan";
        case YdspIrOp::asinF:
            return "asin";
        case YdspIrOp::acosF:
            return "acos";
        case YdspIrOp::atanF:
            return "atan";
        case YdspIrOp::sinhF:
            return "sinh";
        case YdspIrOp::coshF:
            return "cosh";
        case YdspIrOp::tanhF:
            return "tanh";
        case YdspIrOp::asinhF:
            return "asinh";
        case YdspIrOp::acoshF:
            return "acosh";
        case YdspIrOp::atanhF:
            return "atanh";
        case YdspIrOp::roundF:
            return "round";
        case YdspIrOp::expF:
            return "exp";
        case YdspIrOp::logF:
            return "log";
        case YdspIrOp::log10F:
            return "log10";
        case YdspIrOp::powF:
            return "pow";
        case YdspIrOp::atan2F:
            return "atan2";
        case YdspIrOp::fmodF:
            return "fmod";
        case YdspIrOp::copysignF:
            return "copysign";
        default:
            return "";
    }
}

bool isLibmBinaryOp (YdspIrOp op) noexcept
{
    return op == YdspIrOp::powF || op == YdspIrOp::atan2F || op == YdspIrOp::fmodF || op == YdspIrOp::copysignF;
}

//==============================================================================
// A unique wasm function-type signature of a libm import.

struct LibmSignature
{
    bool is64 = false;
    bool isBinary = false;

    bool operator<(const LibmSignature& other) const noexcept
    {
        if (is64 != other.is64)
            return is64 < other.is64;

        return isBinary < other.isBinary;
    }
};

//==============================================================================
// Renders an emitted module in the WebAssembly text format — a readable debug
// dump of types, imports, exports, locals and instructions. It is a one-way
// listing (the wasm counterpart of the asmjit assembly log), not a
// round-trippable wat source. Only the MVP instruction subset emitted by
// YdspWasmEmitter is decoded; anything else renders as raw hex.

class WasmTextRenderer
{
public:
    static String render (const std::vector<uint8_t>& bytes)
    {
        if (bytes.size() < 8 || bytes[0] != 0x00 || bytes[1] != 0x61 || bytes[2] != 0x73 || bytes[3] != 0x6D)
            return {};

        WasmTextRenderer renderer (bytes);
        return renderer.renderModule();
    }

private:
    struct Section
    {
        uint8_t id = 0;
        size_t begin = 0;
        size_t end = 0;
    };

    struct FuncTypeInfo
    {
        std::vector<uint8_t> params;
        std::vector<uint8_t> results;
    };

    explicit WasmTextRenderer (const std::vector<uint8_t>& bytes)
        : bytes (bytes)
    {
    }

    //==============================================================================
    // Byte readers (position-local so sections can be decoded independently)

    uint8_t readByte (size_t& pos) const
    {
        return pos < bytes.size() ? bytes[pos++] : 0;
    }

    uint32_t readLebU (size_t& pos) const
    {
        uint32_t value = 0;
        uint32_t shift = 0;

        while (pos < bytes.size())
        {
            const auto byte = bytes[pos++];

            if (shift < 32)
                value |= static_cast<uint32_t> (byte & 0x7F) << shift;

            shift += 7;

            if ((byte & 0x80) == 0)
                break;
        }

        return value;
    }

    int32_t readLebS (size_t& pos) const
    {
        int32_t value = 0;
        int32_t shift = 0;
        uint8_t byte = 0;

        do
        {
            byte = readByte (pos);

            if (shift < 32)
                value |= static_cast<int32_t> (byte & 0x7F) << shift;

            shift += 7;
        } while ((byte & 0x80) != 0);

        if (shift < 32 && (byte & 0x40) != 0)
            value |= static_cast<int32_t> (-1) << shift;

        return value;
    }

    int64_t readLebS64 (size_t& pos) const
    {
        int64_t value = 0;
        int64_t shift = 0;
        uint8_t byte = 0;

        do
        {
            byte = readByte (pos);

            if (shift < 64)
                value |= static_cast<int64_t> (byte & 0x7F) << shift;

            shift += 7;
        } while ((byte & 0x80) != 0);

        if (shift < 64 && (byte & 0x40) != 0)
            value |= static_cast<int64_t> (-1) << shift;

        return value;
    }

    String readName (size_t& pos) const
    {
        const auto length = readLebU (pos);
        String name;

        for (uint32_t i = 0; i < length; ++i)
            name += String::charToString (static_cast<yup_wchar> (readByte (pos)));

        return name;
    }

    //==============================================================================
    // Rendering helpers

    static void indentLine (std::string& result, const String& text, int indent)
    {
        for (int i = 0; i < indent; ++i)
            result += "  ";

        result += text.toRawUTF8();
        result += '\n';
    }

    static String hex (uint32_t value, int digits)
    {
        String result = "0x";

        for (int i = digits - 1; i >= 0; --i)
        {
            const auto nibble = (value >> (i * 4)) & 0xF;
            result += String::charToString (static_cast<yup_wchar> (nibble < 10 ? '0' + nibble : 'a' + nibble - 10));
        }

        return result;
    }

    static String valTypeName (uint8_t type)
    {
        switch (type)
        {
            case 0x7F:
                return "i32";
            case 0x7E:
                return "i64";
            case 0x7D:
                return "f32";
            case 0x7C:
                return "f64";
#if defined(__wasm_simd128__)
            case 0x7B:
                return "v128";
#endif
            default:
                return hex (type, 2);
        }
    }

    // Renders the blocktype suffix for block/loop/if: "" for the empty type,
    // otherwise " (result <valtype>)".
    static String blockTypeSuffix (uint8_t type)
    {
        if (type == 0x40)
            return {};

        return " (result " + valTypeName (type) + ")";
    }

#if defined(__wasm_simd128__)
    static String simdMnemonic (uint32_t subopcode)
    {
        switch (subopcode)
        {
            case YdspWasmEmitter::opV128Load:
                return "v128.load";
            case YdspWasmEmitter::opV128Store:
                return "v128.store";
            case YdspWasmEmitter::opV128Const:
                return "v128.const";
            case YdspWasmEmitter::opI8x16Shuffle:
                return "i8x16.shuffle";
            case YdspWasmEmitter::opI32x4Splat:
                return "i32x4.splat";
            case YdspWasmEmitter::opF32x4Splat:
                return "f32x4.splat";
            case YdspWasmEmitter::opI32x4ExtractLane:
                return "i32x4.extract_lane";
            case YdspWasmEmitter::opI32x4ReplaceLane:
                return "i32x4.replace_lane";
            case YdspWasmEmitter::opF32x4ExtractLane:
                return "f32x4.extract_lane";
            case YdspWasmEmitter::opF32x4ReplaceLane:
                return "f32x4.replace_lane";
            case YdspWasmEmitter::opF32x4Ceil:
                return "f32x4.ceil";
            case YdspWasmEmitter::opF32x4Floor:
                return "f32x4.floor";
            case YdspWasmEmitter::opF32x4Nearest:
                return "f32x4.nearest";
            case YdspWasmEmitter::opI32x4Eq:
                return "i32x4.eq";
            case YdspWasmEmitter::opI32x4Ne:
                return "i32x4.ne";
            case YdspWasmEmitter::opI32x4LtS:
                return "i32x4.lt_s";
            case YdspWasmEmitter::opI32x4GtS:
                return "i32x4.gt_s";
            case YdspWasmEmitter::opI32x4LeS:
                return "i32x4.le_s";
            case YdspWasmEmitter::opI32x4GeS:
                return "i32x4.ge_s";
            case YdspWasmEmitter::opV128Not:
                return "v128.not";
            case YdspWasmEmitter::opV128And:
                return "v128.and";
            case YdspWasmEmitter::opV128Andnot:
                return "v128.andnot";
            case YdspWasmEmitter::opV128Or:
                return "v128.or";
            case YdspWasmEmitter::opV128Xor:
                return "v128.xor";
            case YdspWasmEmitter::opI32x4Abs:
                return "i32x4.abs";
            case YdspWasmEmitter::opI32x4Neg:
                return "i32x4.neg";
            case YdspWasmEmitter::opI32x4Shl:
                return "i32x4.shl";
            case YdspWasmEmitter::opI32x4ShrS:
                return "i32x4.shr_s";
            case YdspWasmEmitter::opI32x4Add:
                return "i32x4.add";
            case YdspWasmEmitter::opI32x4Sub:
                return "i32x4.sub";
            case YdspWasmEmitter::opI32x4Mul:
                return "i32x4.mul";
            case YdspWasmEmitter::opI32x4MinS:
                return "i32x4.min_s";
            case YdspWasmEmitter::opI32x4MaxS:
                return "i32x4.max_s";
            case YdspWasmEmitter::opI64x2ExtmulLowI32x4S:
                return "i64x2.extmul_low_i32x4_s";
            case YdspWasmEmitter::opI64x2ExtmulHighI32x4S:
                return "i64x2.extmul_high_i32x4_s";
            case YdspWasmEmitter::opI32x4TruncSatF32x4S:
                return "i32x4.trunc_sat_f32x4_s";
            case YdspWasmEmitter::opF32x4ConvertI32x4S:
                return "f32x4.convert_i32x4_s";
            case YdspWasmEmitter::opF32x4Abs:
                return "f32x4.abs";
            case YdspWasmEmitter::opF32x4Neg:
                return "f32x4.neg";
            case YdspWasmEmitter::opF32x4Sqrt:
                return "f32x4.sqrt";
            case YdspWasmEmitter::opF32x4Add:
                return "f32x4.add";
            case YdspWasmEmitter::opF32x4Sub:
                return "f32x4.sub";
            case YdspWasmEmitter::opF32x4Mul:
                return "f32x4.mul";
            case YdspWasmEmitter::opF32x4Div:
                return "f32x4.div";
            case YdspWasmEmitter::opF32x4Min:
                return "f32x4.min";
            case YdspWasmEmitter::opF32x4Max:
                return "f32x4.max";
            case YdspWasmEmitter::opF32x4Eq:
                return "f32x4.eq";
            case YdspWasmEmitter::opF32x4Ne:
                return "f32x4.ne";
            case YdspWasmEmitter::opF32x4Lt:
                return "f32x4.lt";
            case YdspWasmEmitter::opF32x4Gt:
                return "f32x4.gt";
            case YdspWasmEmitter::opF32x4Le:
                return "f32x4.le";
            case YdspWasmEmitter::opF32x4Ge:
                return "f32x4.ge";
            case YdspWasmEmitter::opV128Bitselect:
                return "v128.bitselect";
            default:
                return "simd." + String (static_cast<int> (subopcode));
        }
    }
#endif

    String renderModule()
    {
        // ---- parse the sections ----
        std::vector<Section> sections;
        size_t pos = 8;

        while (pos < bytes.size())
        {
            Section section;
            section.id = readByte (pos);
            const auto size = readLebU (pos);

            if (size > bytes.size() - pos)
                break;

            section.begin = pos;
            section.end = pos + size;
            pos = section.end;
            sections.push_back (section);
        }

        // ---- type section: function types ----
        std::vector<FuncTypeInfo> types;

        for (const auto& section : sections)
        {
            if (section.id != YdspWasmEmitter::sectionType)
                continue;

            size_t p = section.begin;
            const auto count = readLebU (p);

            for (uint32_t i = 0; i < count && p < section.end; ++i)
            {
                const auto form = readByte (p);
                (void) form; // 0x60 functype

                FuncTypeInfo type;

                const auto numParams = readLebU (p);
                for (uint32_t j = 0; j < numParams; ++j)
                    type.params.push_back (readByte (p));

                const auto numResults = readLebU (p);
                for (uint32_t j = 0; j < numResults; ++j)
                    type.results.push_back (readByte (p));

                types.push_back (type);
            }
        }

        // ---- function section: type index per defined function ----
        std::vector<uint32_t> functionTypes;

        for (const auto& section : sections)
        {
            if (section.id != YdspWasmEmitter::sectionFunction)
                continue;

            size_t p = section.begin;
            const auto count = readLebU (p);

            for (uint32_t i = 0; i < count && p < section.end; ++i)
                functionTypes.push_back (readLebU (p));
        }

        // Lines accumulate into a growth-friendly buffer: yup::String += has
        // no capacity reserve, so appending a large dump line by line would be
        // quadratic (every += reallocates and copies the whole prefix) - a big
        // kernel (e.g. TX81Z) would take effectively forever to compile.
        std::string result = "(module\n";

        // ---- render types ----
        for (size_t i = 0; i < types.size(); ++i)
        {
            String line = "(type $t" + String (static_cast<int> (i)) + " (func";

            if (! types[i].params.empty())
            {
                line += " (param";

                for (const auto type : types[i].params)
                    line += " " + valTypeName (type);

                line += ")";
            }

            if (! types[i].results.empty())
            {
                line += " (result";

                for (const auto type : types[i].results)
                    line += " " + valTypeName (type);

                line += ")";
            }

            line += "))";
            indentLine (result, line, 1);
        }

        // ---- render imports (memory first, then libm functions) ----
        int funcImportIndex = 0;

        for (const auto& section : sections)
        {
            if (section.id != YdspWasmEmitter::sectionImport)
                continue;

            size_t p = section.begin;
            const auto count = readLebU (p);

            for (uint32_t i = 0; i < count && p < section.end; ++i)
            {
                const auto moduleName = readName (p);
                const auto name = readName (p);
                const auto kind = readByte (p);

                String line = "(import \"" + moduleName + "\" \"" + name + "\" ";

                if (kind == YdspWasmEmitter::importKindMemory)
                {
                    const auto limitsFlag = readByte (p);
                    const auto minPages = readLebU (p);
                    const auto maxPages = (limitsFlag & 0x01) != 0 ? readLebU (p) : 0;

                    line += "(memory " + String (static_cast<int> (minPages));

                    if ((limitsFlag & 0x01) != 0)
                        line += " " + String (static_cast<int> (maxPages));

                    line += "))";
                }
                else
                {
                    const auto typeIndex = readLebU (p);
                    line += "(func $" + String (funcImportIndex++) + " (type $t" + String (static_cast<int> (typeIndex)) + ")))";
                }

                indentLine (result, line, 1);
            }
        }

        // ---- render defined functions (function section + code bodies) ----
        size_t codePos = 0;
        int definedIndex = 0;

        for (const auto& section : sections)
        {
            if (section.id != YdspWasmEmitter::sectionCode)
                continue;

            codePos = section.begin;
            const auto numBodies = readLebU (codePos);

            for (uint32_t b = 0; b < numBodies && codePos < section.end; ++b)
            {
                const auto funcIndex = funcImportIndex + definedIndex;
                const auto typeIndex = b < functionTypes.size() ? functionTypes[static_cast<size_t> (b)] : 0;

                String line = "(func $" + String (funcIndex) + " (type $t" + String (static_cast<int> (typeIndex)) + ")";
                indentLine (result, line, 1);

                renderBody (result, section, codePos);

                indentLine (result, ")", 1);
                ++definedIndex;
            }
        }

        // ---- render exports ----
        for (const auto& section : sections)
        {
            if (section.id != YdspWasmEmitter::sectionExport)
                continue;

            size_t p = section.begin;
            const auto count = readLebU (p);

            for (uint32_t i = 0; i < count && p < section.end; ++i)
            {
                const auto name = readName (p);
                const auto kind = readByte (p);
                const auto index = readLebU (p);

                String line = "(export \"" + name + "\" ";

                if (kind == YdspWasmEmitter::exportKindFunc)
                    line += "(func $" + String (static_cast<int> (index)) + "))";
                else
                    line += "0x" + hex (index, 8) + ")";

                indentLine (result, line, 1);
            }
        }

        result += ")\n";
        return String (result.c_str());
    }

    // Renders one code-section body: the locals declaration followed by the
    // instruction listing (up to the trailing `end`, which is implicit).
    void renderBody (std::string& result, const Section& section, size_t& p)
    {
        const auto bodySize = readLebU (p);

        // Defensive: the body must stay within its section.
        const auto bodyEnd = std::min (p + bodySize, section.end);

        int localIndex = 0;
        const auto numGroups = readLebU (p);

        for (uint32_t g = 0; g < numGroups && p < bodyEnd; ++g)
        {
            const auto count = readLebU (p);
            const auto type = readByte (p);

            for (uint32_t c = 0; c < count; ++c)
                indentLine (result, "(local $" + String (localIndex++) + " " + valTypeName (type) + ")", 2);
        }

        int depth = 2;

        while (p < bodyEnd && p + 1 < bodyEnd) // the final byte is the function's `end`
        {
            const auto opcode = readByte (p);
            String line = mnemonic (opcode);

            switch (opcode)
            {
                case 0x02: // block
                case 0x03: // loop
                case 0x04: // if
                    line += blockTypeSuffix (readByte (p));
                    indentLine (result, line, depth);
                    ++depth;
                    break;

                case 0x05: // else
                    if (depth > 1)
                        --depth;

                    indentLine (result, line, depth);
                    ++depth;
                    break;

                case 0x0B: // end
                    if (depth > 1)
                        --depth;

                    indentLine (result, line, depth);
                    break;

                case 0x0C: // br
                case 0x0D: // br_if
                case 0x10: // call
                case 0x20: // local.get
                case 0x21: // local.set
                case 0x22: // local.tee
                    line += " " + String (static_cast<int> (readLebU (p)));
                    indentLine (result, line, depth);
                    break;

                case 0x41: // i32.const
                    line += " " + String (static_cast<int> (readLebS (p)));
                    indentLine (result, line, depth);
                    break;

                case 0x42: // i64.const
                    line += " " + String (static_cast<int64_t> (readLebS64 (p)));
                    indentLine (result, line, depth);
                    break;

                case 0x43: // f32.const (raw bits)
                {
                    uint32_t bits = 0;

                    for (int i = 0; i < 4; ++i)
                        bits |= static_cast<uint32_t> (readByte (p)) << (i * 8);

                    line += " " + hex (bits, 8);
                    indentLine (result, line, depth);
                    break;
                }

                case 0x44: // f64.const (raw bits)
                {
                    uint64_t bits = 0;

                    for (int i = 0; i < 8; ++i)
                        bits |= static_cast<uint64_t> (readByte (p)) << (i * 8);

                    line += " " + hex (static_cast<uint32_t> (bits >> 32), 8) + hex (static_cast<uint32_t> (bits), 8);
                    indentLine (result, line, depth);
                    break;
                }

                case 0xFC:
                {
                    const auto sub = readLebU (p);
                    static constexpr const char* names[] { "i32.trunc_sat_f32_s", "i32.trunc_sat_f32_u", "i32.trunc_sat_f64_s", "i32.trunc_sat_f64_u",
                                                          "i64.trunc_sat_f32_s", "i64.trunc_sat_f32_u", "i64.trunc_sat_f64_s", "i64.trunc_sat_f64_u" };
                    indentLine (result, sub < 8 ? String (names[sub]) : String ("unknown 0xFC"), depth);
                    break;
                }

                case 0x28: // i32.load
                case 0x29: // i64.load
                case 0x2A: // f32.load
                case 0x2B: // f64.load
                case 0x36: // i32.store
                case 0x37: // i64.store
                case 0x38: // f32.store
                case 0x39: // f64.store
                {
                    const auto align = readLebU (p);
                    const auto offset = readLebU (p);

                    line += " offset=" + String (static_cast<int> (offset));
                    line += " align=" + String (static_cast<int> (align));
                    indentLine (result, line, depth);
                    break;
                }

#if defined(__wasm_simd128__)
                case YdspWasmEmitter::simdPrefix: // 0xFD SIMD prefix
                {
                    const auto sub = readLebU (p);
                    line = simdMnemonic (sub);

                    switch (sub)
                    {
                        case YdspWasmEmitter::opV128Load:
                        case YdspWasmEmitter::opV128Store:
                        {
                            const auto align = readLebU (p);
                            const auto offset = readLebU (p);

                            line += " offset=" + String (static_cast<int> (offset));
                            line += " align=" + String (static_cast<int> (align));
                            break;
                        }

                        case YdspWasmEmitter::opV128Const:
                        {
                            uint32_t words[4] = {};

                            for (int w = 0; w < 4; ++w)
                                for (int b = 0; b < 4; ++b)
                                    words[w] |= static_cast<uint32_t> (readByte (p)) << (b * 8);

                            line += " i32x4 " + hex (words[0], 8) + " " + hex (words[1], 8) + " " + hex (words[2], 8) + " " + hex (words[3], 8);
                            break;
                        }

                        case YdspWasmEmitter::opI8x16Shuffle:
                            line += " [" + String (static_cast<int> (readByte (p)));

                            for (int i = 1; i < 16; ++i)
                                line += " " + String (static_cast<int> (readByte (p)));

                            line += "]";
                            break;

                        case YdspWasmEmitter::opI32x4ExtractLane:
                        case YdspWasmEmitter::opI32x4ReplaceLane:
                        case YdspWasmEmitter::opF32x4ExtractLane:
                        case YdspWasmEmitter::opF32x4ReplaceLane:
                            line += " " + String (static_cast<int> (readByte (p)));
                            break;

                        default:
                            break;
                    }

                    indentLine (result, line, depth);
                    break;
                }
#endif

                default:
                    indentLine (result, line, depth);
                    break;
            }
        }

        // consume the trailing `end`
        if (p < bodyEnd)
            ++p;
    }

    static String mnemonic (uint8_t opcode)
    {
        switch (opcode)
        {
            case 0x00:
                return "unreachable";
            case 0x01:
                return "nop";
            case 0x02:
                return "block";
            case 0x03:
                return "loop";
            case 0x04:
                return "if";
            case 0x05:
                return "else";
            case 0x0B:
                return "end";
            case 0x0C:
                return "br";
            case 0x0D:
                return "br_if";
            case 0x0F:
                return "return";
            case 0x10:
                return "call";
            case 0x1A:
                return "drop";
            case 0x1B:
                return "select";
            case 0x20:
                return "local.get";
            case 0x21:
                return "local.set";
            case 0x22:
                return "local.tee";
            case 0x28:
                return "i32.load";
            case 0x29:
                return "i64.load";
            case 0x2A:
                return "f32.load";
            case 0x2B:
                return "f64.load";
            case 0x36:
                return "i32.store";
            case 0x37:
                return "i64.store";
            case 0x38:
                return "f32.store";
            case 0x39:
                return "f64.store";
            case 0x41:
                return "i32.const";
            case 0x42:
                return "i64.const";
            case 0x43:
                return "f32.const";
            case 0x44:
                return "f64.const";
            case 0x45:
                return "i32.eqz";
            case 0x46:
                return "i32.eq";
            case 0x47:
                return "i32.ne";
            case 0x48:
                return "i32.lt_s";
            case 0x49:
                return "i32.lt_u";
            case 0x4A:
                return "i32.gt_s";
            case 0x4B:
                return "i32.gt_u";
            case 0x4C:
                return "i32.le_s";
            case 0x4E:
                return "i32.ge_s";
            case 0x50:
                return "i64.eqz";
            case 0x51:
                return "i64.eq";
            case 0x52:
                return "i64.ne";
            case 0x53:
                return "i64.lt_s";
            case 0x54:
                return "i64.lt_u";
            case 0x55:
                return "i64.gt_s";
            case 0x56:
                return "i64.gt_u";
            case 0x57:
                return "i64.le_s";
            case 0x59:
                return "i64.ge_s";
            case 0x5B:
                return "f32.eq";
            case 0x5C:
                return "f32.ne";
            case 0x5D:
                return "f32.lt";
            case 0x5E:
                return "f32.gt";
            case 0x5F:
                return "f32.le";
            case 0x60:
                return "f32.ge";
            case 0x61:
                return "f64.eq";
            case 0x62:
                return "f64.ne";
            case 0x63:
                return "f64.lt";
            case 0x64:
                return "f64.gt";
            case 0x65:
                return "f64.le";
            case 0x66:
                return "f64.ge";
            case 0x6A:
                return "i32.add";
            case 0x6B:
                return "i32.sub";
            case 0x6C:
                return "i32.mul";
            case 0x6D:
                return "i32.div_s";
            case 0x6E:
                return "i32.div_u";
            case 0x6F:
                return "i32.rem_s";
            case 0x71:
                return "i32.and";
            case 0x72:
                return "i32.or";
            case 0x73:
                return "i32.xor";
            case 0x74:
                return "i32.shl";
            case 0x75:
                return "i32.shr_s";
            case 0x7C:
                return "i64.add";
            case 0x7D:
                return "i64.sub";
            case 0x7E:
                return "i64.mul";
            case 0x7F:
                return "i64.div_s";
            case 0x80:
                return "i64.div_u";
            case 0x81:
                return "i64.rem_s";
            case 0x83:
                return "i64.and";
            case 0x84:
                return "i64.or";
            case 0x85:
                return "i64.xor";
            case 0x86:
                return "i64.shl";
            case 0x87:
                return "i64.shr_s";
            case 0x88:
                return "i64.shr_u";
            case 0x8B:
                return "f32.abs";
            case 0x8C:
                return "f32.neg";
            case 0x8D:
                return "f32.ceil";
            case 0x8E:
                return "f32.floor";
            case 0x8F:
                return "f32.trunc";
            case 0x90:
                return "f32.nearest";
            case 0x91:
                return "f32.sqrt";
            case 0x92:
                return "f32.add";
            case 0x93:
                return "f32.sub";
            case 0x94:
                return "f32.mul";
            case 0x95:
                return "f32.div";
            case 0x96:
                return "f32.min";
            case 0x97:
                return "f32.max";
            case 0x98:
                return "f32.copysign";
            case 0x99:
                return "f64.abs";
            case 0x9A:
                return "f64.neg";
            case 0x9B:
                return "f64.ceil";
            case 0x9C:
                return "f64.floor";
            case 0x9D:
                return "f64.trunc";
            case 0x9E:
                return "f64.nearest";
            case 0x9F:
                return "f64.sqrt";
            case 0xA0:
                return "f64.add";
            case 0xA1:
                return "f64.sub";
            case 0xA2:
                return "f64.mul";
            case 0xA3:
                return "f64.div";
            case 0xA4:
                return "f64.min";
            case 0xA5:
                return "f64.max";
            case 0xA6:
                return "f64.copysign";
            case 0xA7:
                return "i32.wrap_i64";
            case 0xA8:
                return "i32.trunc_f32_s";
            case 0xAA:
                return "i32.trunc_f64_s";
            case 0xAC:
                return "i64.extend_i32_s";
            case 0xAD:
                return "i64.extend_i32_u";
            case 0xAE:
                return "i64.trunc_f32_s";
            case 0xB0:
                return "i64.trunc_f64_s";
            case 0xB2:
                return "f32.convert_i32_s";
            case 0xB4:
                return "f32.convert_i64_s";
            case 0xB6:
                return "f32.demote_f64";
            case 0xB7:
                return "f64.convert_i32_s";
            case 0xB9:
                return "f64.convert_i64_s";
            case 0xBB:
                return "f64.promote_f32";
            case 0xBC:
                return "i32.reinterpret_f32";
            case 0xBE:
                return "f32.reinterpret_i32";
            default:
                return hex (opcode, 2);
        }
    }

    const std::vector<uint8_t>& bytes;
};

//==============================================================================

class YdspWasmCodegenImpl
{
public:
    std::vector<uint8_t> compile (const YdspIrFunction& fn, YdspDiagnostics& diagnostics)
    {
        this->fn = &fn;
        this->diagnostics = &diagnostics;

        const bool isEvent = fn.isEventHandler;

        stateOff = isEvent ? eventStateOff : kernelStateOff;
        stateArraysOff = isEvent ? eventStateArraysOff : kernelStateArraysOff;
        paramsOff = isEvent ? eventParamsOff : kernelParamsOff;
        sampleRateOff = isEvent ? eventSampleRateOff : kernelSampleRateOff;
        outputEventsOff = isEvent ? eventOutputEventsOff : kernelOutputEventsOff;
        traceQueueOff = isEvent ? eventTraceQueueOff : kernelTraceQueueOff;

        // Cumulative byte offsets for the heterogeneous param/meter blocks.
        // The scalar/array segment layout is derived from YdspIrFunction's
        // shared helpers at every state access (see stateScalarBase/stateArrayBase).
        paramOffsets = ydspEndpointByteOffsets (fn.paramTypes);
        paramOutOffsets = ydspEndpointByteOffsets (fn.paramOutTypes);

        if (! analyze (fn))
            return {};

        // Loop headers index into fn.loops. A fully unrolled loop is skipped:
        // its entry survives so the report can still state the worst-case
        // iteration count, but its header and body are empty and fall through,
        // so there is no back edge left to wrap in a wasm loop region.
        loopByHeader.clear();

        for (size_t i = 0; i < fn.loops.size(); ++i)
            if (! fn.loops[i].unrolled)
                loopByHeader[fn.loops[i].headerBlock] = static_cast<int> (i);

        emitter.beginModule();

        // ---- type section ----
        emitter.beginSection (YdspWasmEmitter::sectionType);
        emitter.u32 (static_cast<uint32_t> (1 + libmSignatures.size() + (usesEmitEvent ? 1 : 0) + (usesTrace ? 3 : 0)));

        kernelTypeIndex = emitter.funcType ({ ValType::i32 }, {});

        for (const auto& sig : libmSignatures)
            sigTypeIndex[sig] = emitter.funcType (sig.isBinary ? std::initializer_list<ValType> { sig.is64 ? ValType::f64 : ValType::f32,
                                                                                                  sig.is64 ? ValType::f64 : ValType::f32 }
                                                               : std::initializer_list<ValType> { sig.is64 ? ValType::f64 : ValType::f32 },
                                                  { sig.is64 ? ValType::f64 : ValType::f32 });

        if (usesEmitEvent)
            emitEventTypeIndex = emitter.funcType ({ ValType::i32, ValType::i32, ValType::i32, ValType::i32 }, {});
        if (usesTrace)
        {
            traceIntType = emitter.funcType ({ ValType::i32, ValType::i32, ValType::i32, ValType::i32 }, {});
            traceFloatType = emitter.funcType ({ ValType::i32, ValType::i32, ValType::f64 }, {});
            traceCommitType = emitter.funcType ({ ValType::i32, ValType::i32 }, {});
        }

        emitter.endSection();

        // ---- import section: memory first, then the libm functions, then the
        // output-event commit function (if used) ----
        emitter.beginSection (YdspWasmEmitter::sectionImport);
        emitter.u32 (static_cast<uint32_t> (1 + libmNames.size() + (usesEmitEvent ? 1 : 0) + (usesTrace ? 3 : 0)));

        // Shared min+max limits: emscripten's wasmMemory is a shared, growable
        // memory, and a shared import must declare a maximum. Limits matching
        // requires import.min <= host.min and (if declared) import.max >=
        // host.max; 65536 pages (4 GiB, the wasm32 address-space limit) is
        // therefore compatible with any host maximum.
        emitter.importMemory ("env", "memory", 1, 65536, true);

        // Function imports are indexed in the function index space, which
        // starts at 0 and does not include the memory import.
        int importIndex = 0;

        for (const auto& name : libmNames)
        {
            const auto sig = signatureOf (name);
            emitter.importFunction ("env", name.c_str(), static_cast<uint32_t> (sigTypeIndex[sig]));
            libmFuncIndex[name] = importIndex++;
        }

        if (usesEmitEvent)
        {
            emitter.importFunction ("env", "ydspCommitOutputEvent", static_cast<uint32_t> (emitEventTypeIndex));
            emitEventFuncIndex = importIndex++;
        }
        if (usesTrace)
        {
            emitter.importFunction ("env", "ydspTraceInt", static_cast<uint32_t> (traceIntType));
            traceIntFunc = importIndex++;
            emitter.importFunction ("env", "ydspTraceFloat", static_cast<uint32_t> (traceFloatType));
            traceFloatFunc = importIndex++;
            emitter.importFunction ("env", "ydspTraceCommit", static_cast<uint32_t> (traceCommitType));
            traceCommitFunc = importIndex++;
        }

        emitter.endSection();

        // ---- function section ----
        emitter.beginSection (YdspWasmEmitter::sectionFunction);
        emitter.u32 (1);
        kernelFuncIndex = emitter.functionEntry (static_cast<uint32_t> (kernelTypeIndex));
        emitter.endSection();

        // ---- export section ----
        emitter.beginSection (YdspWasmEmitter::sectionExport);
        emitter.u32 (1);
        emitter.exportFunction ("ydsp_kernel", static_cast<uint32_t> (kernelFuncIndex));
        emitter.endSection();

        // ---- code section ----
        emitter.beginSection (YdspWasmEmitter::sectionCode);
        emitter.u32 (1);

        emitter.beginBody();
        declareValueLocals();
        emitPrologue();
        emitBlocks (0, static_cast<int> (fn.blocks.size()));

        if (! labels.empty())
        {
            diagnostics.addError ({}, "Wasm codegen: unbalanced control-flow structure in the IR");
            return {};
        }

        emitter.endBody();
        emitter.endSection();

        auto bytes = emitter.takeBytes();
        if (! bytes.empty())
            diagnostics.addInfo ({}, WasmTextRenderer::render (bytes));
        return bytes;
    }

private:
    //==============================================================================
    // Analysis: value types, max value id, and the used libm imports.

    struct UsageStats
    {
        int count = 0;
        bool inLoop = false;

        void record (bool insideLoop) noexcept
        {
            ++count;
            inLoop = inLoop || insideLoop;
        }

        bool shouldHoist() const noexcept
        {
            return inLoop || count >= 2;
        }
    };

    bool analyze (const YdspIrFunction& fn)
    {
        int maxResult = -1;

        std::vector<bool> blockInLoop (fn.blocks.size(), false);

        for (const auto& loop : fn.loops)
        {
            if (loop.unrolled)
                continue;

            const int startBlock = std::max (0, loop.headerBlock);
            const int endBlock = std::min (static_cast<int> (fn.blocks.size()), loop.exitBlock);

            for (int b = startBlock; b < endBlock; ++b)
                blockInLoop[static_cast<size_t> (b)] = true;
        }

        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            const auto& block = fn.blocks[b];
            const bool inLoop = blockInLoop[b];

            for (const auto& inst : block.insts)
            {
                if (inst.result > maxResult)
                    maxResult = inst.result;

                if (inst.op == YdspIrOp::traceCommit)
                    usesTrace = true;

                if (inst.op == YdspIrOp::emitEvent)
                    usesEmitEvent = true;

                switch (inst.op)
                {
                    case YdspIrOp::loadStateF:
                    case YdspIrOp::loadStateI:
                    case YdspIrOp::storeStateF:
                    case YdspIrOp::storeStateI:
                        stateUsage.record (inLoop);
                        break;

                    case YdspIrOp::loadStateArrayF:
                    case YdspIrOp::loadStateArrayI:
                    case YdspIrOp::storeStateArrayF:
                    case YdspIrOp::storeStateArrayI:
                        stateArraysUsage.record (inLoop);
                        break;

                    case YdspIrOp::loadParam:
                    case YdspIrOp::storeParam:
                        paramsUsage.record (inLoop);
                        break;

                    case YdspIrOp::loadParamOut:
                    case YdspIrOp::storeParamOut:
                        paramOutUsage.record (inLoop);
                        break;

                    case YdspIrOp::storeEventFieldF:
                    case YdspIrOp::storeEventFieldI:
                    case YdspIrOp::emitEvent:
                        outputEventsUsage.record (inLoop);
                        break;

                    case YdspIrOp::traceInt:
                    case YdspIrOp::traceFloat:
                    case YdspIrOp::traceCommit:
                        traceQueueUsage.record (inLoop);
                        break;

                    case YdspIrOp::loadSampleRate:
                        sampleRateUsage.record (inLoop);
                        break;

                    case YdspIrOp::loadBlockSize:
                        numSamplesUsage.record (inLoop);
                        break;

                    case YdspIrOp::loadInput:
                    case YdspIrOp::loadInputAddress:
                        if (inst.memIndex >= 0)
                            inputStreamUsage[inst.memIndex].record (inLoop);
                        break;

                    case YdspIrOp::loadOutput:
                    case YdspIrOp::storeOutput:
                    case YdspIrOp::loadOutputAddress:
                        if (inst.memIndex >= 0)
                            outputStreamUsage[inst.memIndex].record (inLoop);
                        break;

                    default:
                        break;
                }

#if defined(__wasm_simd128__)
                if (inst.op == YdspIrOp::vreduceAddF || fn.laneCountOf (inst.result) > 1)
                    usesSimdScratch = true;
#endif

                if (isLibmOp (inst.op))
                {
                    const bool is64 = inst.a >= 0 && is64BitValueType (inferOperandType (fn, inst, inst.a));
                    const auto name = std::string (libmBaseName (inst.op)) + (is64 ? "" : "f");

                    libmNames.insert (name);

                    const auto sig = signatureOf (name);
                    if (sigTypeIndex.find (sig) == sigTypeIndex.end())
                    {
                        libmSignatures.push_back (sig);
                        sigTypeIndex[sig] = 0; // placeholder; real index assigned at emission
                    }
                }
            }
        }

        if (maxResult < 0)
            return true; // no values; a valid (empty) kernel

        valueTypes = resolveValueTypes (fn);
        valueUses.assign (valueTypes.size(), 0);

        const auto countUse = [this] (int valueId)
        {
            if (valueId >= 0 && static_cast<size_t> (valueId) < valueUses.size())
                ++valueUses[static_cast<size_t> (valueId)];
        };

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.op == YdspIrOp::phi)
                {
                    for (const auto& [predBlock, valueId] : inst.phiIncoming)
                    {
                        (void) predBlock;
                        countUse (valueId);
                    }

                    continue;
                }

                if (! ydspFirstOperandIsSlot (inst.op))
                    countUse (inst.a);

                countUse (inst.b);
                countUse (inst.c);
            }

            if (block.term == YdspIrTerm::branchIf)
                countUse (block.termCond);
        }

        return true;
    }

    YdspValueType valueType (int valueId) const
    {
        return valueTypes[static_cast<size_t> (valueId)];
    }

    // The type of a libm operand, inferred from the producing opcode when the
    // optimiser's valueTypes are not authoritative.
    YdspValueType inferOperandType (const YdspIrFunction& fn, const YdspIrInst& inst, int operand) const
    {
        if (static_cast<size_t> (operand) < fn.valueTypes.size())
            return fn.valueTypes[static_cast<size_t> (operand)];

        return inferTypeFromOp (inst.op);
    }

    bool isLibmOp (YdspIrOp op) const noexcept
    {
        return libmBaseName (op)[0] != '\0';
    }

    LibmSignature signatureOf (const std::string& name) const noexcept
    {
        const bool is64 = name.empty() || name.back() != 'f';
        const bool isBinary = name == "pow" || name == "powf" || name == "atan2" || name == "atan2f"
                           || name == "fmod" || name == "fmodf" || name == "copysign" || name == "copysignf";

        return { is64, isBinary };
    }

    //==============================================================================
    // Locals: one wasm local per value id (local 0 is the ctx parameter).

    // The wasm local type of a value id: v128 for a widened value, the scalar
    // type otherwise.
    YdspWasmEmitter::ValType localTypeOf (int valueId) const
    {
#if defined(__wasm_simd128__)
        if (fn->laneCountOf (valueId) > 1)
            return ValType::v128;
#endif

        return toWasmType (valueTypes[static_cast<size_t> (valueId)]);
    }

    void declareValueLocals()
    {
        // Group consecutive same-typed value ids into runs.
        size_t i = 0;

        while (i < valueTypes.size())
        {
            const auto type = localTypeOf (static_cast<int> (i));
            size_t count = 1;

            while (i + count < valueTypes.size() && localTypeOf (static_cast<int> (i + count)) == type)
                ++count;

            emitter.declareLocals (static_cast<uint32_t> (count), type);
            i += count;
        }

        uint32_t nextLocal = static_cast<uint32_t> (valueTypes.size()) + 1u;

#if defined(__wasm_simd128__)
        // Scratch v128 locals for SIMD reductions and widened integer ops.
        if (usesSimdScratch)
        {
            simdScratchBase = nextLocal;
            emitter.declareLocals (4, ValType::v128);
            nextLocal += 4u;
        }
#endif

        uint32_t hoistedI32Count = 0;
        const auto allocI32Local = [&] () -> int
        {
            ++hoistedI32Count;
            return static_cast<int> (nextLocal++);
        };

        if (stateUsage.shouldHoist())
            stateBaseLocal = allocI32Local();
        if (stateArraysUsage.shouldHoist())
            stateArraysBaseLocal = allocI32Local();
        if (paramsUsage.shouldHoist())
            paramsBaseLocal = allocI32Local();
        if (paramOutUsage.shouldHoist())
            paramOutBaseLocal = allocI32Local();
        if (outputEventsUsage.shouldHoist())
            outputEventsBaseLocal = allocI32Local();
        if (traceQueueUsage.shouldHoist())
            traceQueueBaseLocal = allocI32Local();
        if (numSamplesUsage.shouldHoist())
            numSamplesLocal = allocI32Local();

        for (const auto& [memIndex, stats] : inputStreamUsage)
            if (stats.shouldHoist())
                inStreamLocals[memIndex] = allocI32Local();

        for (const auto& [memIndex, stats] : outputStreamUsage)
            if (stats.shouldHoist())
                outStreamLocals[memIndex] = allocI32Local();

        if (hoistedI32Count > 0)
            emitter.declareLocals (hoistedI32Count, ValType::i32);

        if (sampleRateUsage.shouldHoist())
        {
            sampleRateLocal = static_cast<int> (nextLocal++);
            emitter.declareLocals (1, ValType::f32);
        }
    }

    void emitStreamTableHoist (int tableFieldOffset, const std::map<int, int>& streamLocals)
    {
        if (streamLocals.empty())
            return;

        pushCtxLoad (tableFieldOffset, YdspValueType::int32Type);

        if (streamLocals.size() == 1)
        {
            const auto& [memIndex, localIdx] = *streamLocals.begin();
            emitter.load (YdspWasmEmitter::opI32Load, 2, static_cast<uint32_t> (memIndex * 4));
            emitter.localSet (static_cast<uint32_t> (localIdx));
            return;
        }

        // Reuse the last stream's local to hold the pointer table during
        // prologue initialization; local.tee leaves the table pointer on the
        // stack for the first stream load.
        const auto tableLocal = static_cast<uint32_t> (streamLocals.rbegin()->second);
        emitter.localTee (tableLocal);

        size_t index = 0;

        for (const auto& [memIndex, localIdx] : streamLocals)
        {
            if (index > 0)
                emitter.localGet (tableLocal);

            emitter.load (YdspWasmEmitter::opI32Load, 2, static_cast<uint32_t> (memIndex * 4));
            emitter.localSet (static_cast<uint32_t> (localIdx));
            ++index;
        }
    }

    void emitPrologue()
    {
        const auto hoistI32Field = [this] (int localIdx, int fieldOff)
        {
            if (localIdx < 0)
                return;

            pushCtxLoad (fieldOff, YdspValueType::int32Type);
            emitter.localSet (static_cast<uint32_t> (localIdx));
        };

        hoistI32Field (stateBaseLocal, stateOff);
        hoistI32Field (stateArraysBaseLocal, stateArraysOff);
        hoistI32Field (paramsBaseLocal, paramsOff);
        hoistI32Field (paramOutBaseLocal, kernelParamOutOff);
        hoistI32Field (outputEventsBaseLocal, outputEventsOff);
        hoistI32Field (traceQueueBaseLocal, traceQueueOff);
        hoistI32Field (numSamplesLocal, kernelNumSamplesOff);

        if (sampleRateLocal >= 0)
        {
            pushCtxLoad (sampleRateOff, YdspValueType::float32Type);
            emitter.localSet (static_cast<uint32_t> (sampleRateLocal));
        }

        emitStreamTableHoist (kernelInputsOff, inStreamLocals);
        emitStreamTableHoist (kernelOutputsOff, outStreamLocals);
    }

    uint32_t localIndex (int valueId) const
    {
        return static_cast<uint32_t> (valueId) + 1;
    }

    void pushValue (int valueId)
    {
        if (pendingCount > 0 && pendingOnStack[0] == valueId)
        {
            if (pendingCount == 2)
                pendingOnStack[0] = pendingOnStack[1];

            --pendingCount;
            return;
        }

        jassert (pendingCount == 0);
        emitter.localGet (localIndex (valueId));
    }

    void pushCtxLoad (int fieldOffset, YdspValueType fieldType)
    {
        emitter.localGet (0);
        emitter.load (loadOpcode (fieldType), scaleLog2 (fieldType), static_cast<uint32_t> (fieldOffset));
    }

    void pushBasePointer (int baseFieldOffset, int hoistedLocal)
    {
        if (hoistedLocal >= 0)
            emitter.localGet (static_cast<uint32_t> (hoistedLocal));
        else
            pushCtxLoad (baseFieldOffset, YdspValueType::int32Type);
    }

    void pushStreamBase (bool isInput, int memIndex)
    {
        const auto& locals = isInput ? inStreamLocals : outStreamLocals;

        if (const auto it = locals.find (memIndex); it != locals.end())
        {
            emitter.localGet (static_cast<uint32_t> (it->second));
            return;
        }

        pushCtxLoad (isInput ? kernelInputsOff : kernelOutputsOff, YdspValueType::int32Type);

        if (memIndex >= 0)
        {
            emitter.load (YdspWasmEmitter::opI32Load, 2, static_cast<uint32_t> (memIndex * 4));
        }
        else
        {
            emitter.i32Const (memIndex * 4);
            emitter.op (YdspWasmEmitter::opI32Add);
            emitter.load (YdspWasmEmitter::opI32Load, 2, 0);
        }
    }

    void emitLoadAtOffset (int baseFieldOffset, int hoistedLocal, int byteOffset, YdspValueType type)
    {
        pushBasePointer (baseFieldOffset, hoistedLocal);

        if (byteOffset >= 0)
        {
            emitter.load (loadOpcode (type), scaleLog2 (type), static_cast<uint32_t> (byteOffset));
            return;
        }

        emitter.i32Const (byteOffset);
        emitter.op (YdspWasmEmitter::opI32Add);
        emitter.load (loadOpcode (type), scaleLog2 (type), 0);
    }

    void emitStoreAtOffset (int baseFieldOffset, int hoistedLocal, int byteOffset, int valueId, YdspValueType type)
    {
        pushBasePointer (baseFieldOffset, hoistedLocal);

        if (byteOffset >= 0)
        {
            pushValue (valueId);
            emitter.store (storeOpcode (type), scaleLog2 (type), static_cast<uint32_t> (byteOffset));
            return;
        }

        emitter.i32Const (byteOffset);
        emitter.op (YdspWasmEmitter::opI32Add);
        pushValue (valueId);
        emitter.store (storeOpcode (type), scaleLog2 (type), 0);
    }

    // Pushes the effective address = (indexValueId << scaleLog2(elementType)) + basePtr.
    // Pushing index first lets single-use index values flow directly from the stack.
    void pushIndexedBaseAddress (int baseFieldOffset, int hoistedLocal, int indexValueId, YdspValueType elementType)
    {
        pushValue (indexValueId);
        emitter.i32Const (static_cast<int32_t> (scaleLog2 (elementType)));
        emitter.op (YdspWasmEmitter::opI32Shl);
        pushBasePointer (baseFieldOffset, hoistedLocal);
        emitter.op (YdspWasmEmitter::opI32Add);
    }

    void pushStreamIndexedAddress (bool isInput, int memIndex, int indexValueId, YdspValueType elementType)
    {
        pushValue (indexValueId);
        emitter.i32Const (static_cast<int32_t> (scaleLog2 (elementType)));
        emitter.op (YdspWasmEmitter::opI32Shl);
        pushStreamBase (isInput, memIndex);
        emitter.op (YdspWasmEmitter::opI32Add);
    }

    void loadFloatConst (double value, YdspValueType type)
    {
        if (type == YdspValueType::float64Type)
            emitter.f64Const (value);
        else
            emitter.f32Const (static_cast<float> (value));
    }

    //==============================================================================
    // State layout (derived from YdspIrFunction's shared helpers).

    int stateScalarBase (YdspValueType type, int slot) const
    {
        return fn->stateScalarByteOffset (type, slot);
    }

    int stateArrayBase (YdspValueType type, int element) const
    {
        return fn->stateArrayByteOffset (type, element);
    }

    //==============================================================================
    // Control flow lowering.
    //
    // The YDSP CFG is structured (verified): every branch target is a recorded
    // loop header (back edge), a recorded loop exit, or the join of an if/else
    // region. Each construct maps to wasm control as follows:
    //
    //   loop region  ->  block(exit) { loop(header) { ... br_if exit ... br header } }
    //   if/else      ->  block(join) { <cond>; if { then...; br join } else { else...; br join } }
    //
    // Labels are tracked on a stack; br depth = stack size - label index - 1.

    struct Label
    {
        enum Kind
        {
            block, // a join or loop-exit block
            loop,  // a loop header (back-edge target)
            if_
        };

        Kind kind = block;
        int irBlock = -1; // IR block this label leads to (-1 for if)
    };

    int pushLabel (Label::Kind kind, int irBlock)
    {
        labels.push_back ({ kind, irBlock });
        return static_cast<int> (labels.size()) - 1;
    }

    void popLabel (int labelIndex)
    {
        jassert (labelIndex == static_cast<int> (labels.size()) - 1);
        labels.pop_back();
    }

    uint32_t depthOf (int labelIndex) const
    {
        return static_cast<uint32_t> (labels.size()) - 1u - static_cast<uint32_t> (labelIndex);
    }

    // Emits br/br_if to the open label whose irBlock matches the target.
    void emitBranchTo (int targetBlock, bool conditional)
    {
        for (int i = static_cast<int> (labels.size()) - 1; i >= 0; --i)
        {
            if (labels[static_cast<size_t> (i)].irBlock == targetBlock)
            {
                if (conditional)
                    emitter.brIf (depthOf (i));
                else
                    emitter.br (depthOf (i));

                return;
            }
        }

        diagnostics->addError ({}, "Wasm codegen: branch target " + String (targetBlock) + " is not reachable from a structured control-flow region");
    }

    void emitBlocks (int start, int end)
    {
        YdspRecursionGuard guard (recursionDepth);

        if (guard.exceeded())
        {
            diagnostics->addError ({}, "Wasm codegen: control flow nested too deeply");
            return;
        }

        int i = start;

        while (i < end)
        {
            if (loopByHeader.find (i) != loopByHeader.end())
            {
                i = emitLoop (i);
                continue;
            }

            const auto& block = fn->blocks[static_cast<size_t> (i)];

            if (block.term == YdspIrTerm::branchIf && block.termTarget == i + 1 && block.termTarget2 != -1)
            {
                i = emitIfElse (i);
                continue;
            }

            emitInstructions (block, false);

            switch (block.term)
            {
                case YdspIrTerm::fallthrough:
                    break;

                case YdspIrTerm::branch:
                    if (block.termTarget != i + 1)
                        emitBranchTo (block.termTarget, false);
                    break;

                case YdspIrTerm::branchIf:
                {
                    // A conditional branch that is neither a loop header nor
                    // an if/else cond (then == block + 1) - the IR is not
                    // structured as expected.
                    String message = "Wasm codegen: unexpected conditional branch in the IR control flow (block " + String (i) + ": target="
                                   + String (block.termTarget) + ", target2=" + String (block.termTarget2) + ")";
                    diagnostics->addError ({}, message);
                    break;
                }
            }

            ++i;
        }
    }

    int emitLoop (int headerIndex)
    {
        const auto& loop = fn->loops[static_cast<size_t> (loopByHeader[headerIndex])];
        const auto& header = fn->blocks[static_cast<size_t> (headerIndex)];

        const int exitLabel = pushLabel (Label::block, loop.exitBlock);
        emitter.block();

        const int headerLabel = pushLabel (Label::loop, headerIndex);
        emitter.loop();

        // Header terminator: branchIf(cond, body, exit) -> br_if exit. The
        // body immediately follows the header, so a false condition exits;
        // br_if branches on nonzero, so invert the condition first.
        const bool condValid = header.term == YdspIrTerm::branchIf && header.termTarget == headerIndex + 1;
        const bool condOnStack = emitInstructions (header, condValid);

        if (condValid)
        {
            if (! condOnStack && header.termCond >= 0)
                emitter.localGet (localIndex (header.termCond));

            emitter.op (YdspWasmEmitter::opI32Eqz);
            emitter.brIf (depthOf (exitLabel));
        }
        else
        {
            diagnostics->addError ({}, "Wasm codegen: a loop header must end with a conditional branch to the following block");
        }

        emitBlocks (headerIndex + 1, loop.exitBlock);

        popLabel (headerLabel);
        emitter.end(); // loop end

        popLabel (exitLabel);
        emitter.end(); // block end

        return loop.exitBlock;
    }

    int emitIfElse (int condIndex)
    {
        const auto& cond = fn->blocks[static_cast<size_t> (condIndex)];
        const int thenStart = condIndex + 1;
        int elseIndex = cond.termTarget2;
        int join = -1;

        if (elseIndex == -1)
        {
            diagnostics->addError ({}, "Wasm codegen: an if/else region is missing its else/join target");
            return condIndex + 1;
        }

        if (elseIndex > thenStart && fn->blocks[static_cast<size_t> (elseIndex - 1)].term == YdspIrTerm::branch
            && fn->blocks[static_cast<size_t> (elseIndex - 1)].termTarget != elseIndex)
        {
            // There is an else region: the block before it is the then-tail,
            // which branches to the join that follows the else region.
            join = fn->blocks[static_cast<size_t> (elseIndex - 1)].termTarget;
        }
        else
        {
            // No else region: termTarget2 is the join directly.
            join = elseIndex;
            elseIndex = -1;
        }

        const int joinLabel = pushLabel (Label::block, join);
        emitter.block();

        const bool condOnStack = emitInstructions (cond, cond.termCond >= 0);

        if (! condOnStack && cond.termCond >= 0)
            emitter.localGet (localIndex (cond.termCond));

        emitter.if_();

        const int ifLabel = pushLabel (Label::if_, -1);

        emitBlocks (thenStart, elseIndex != -1 ? elseIndex : join);

        if (elseIndex != -1)
        {
            emitter.else_();
            emitBlocks (elseIndex, join);
        }

        popLabel (ifLabel);
        emitter.end(); // if end

        popLabel (joinLabel);
        emitter.end(); // join block end

        return join;
    }

    //==============================================================================
    // Stack-top / local.tee planning and instruction lowering

    enum class EmitAction : uint8_t
    {
        none,
        localSet,
        localTee,
        drop
    };

    struct InstEmitPlan
    {
        EmitAction action = EmitAction::none;
        bool swapOperands = false;
        int consumedFromStack = 0; // 0, 1, or 2
    };

    struct OperandSpec
    {
        int first = -1;
        int second = -1;
        bool swappable = false;
        bool multiReadFirst = false;
        bool multiReadSecond = false;
    };

    OperandSpec describeOperands (const YdspIrInst& inst) const
    {
        const bool simd = fn->laneCountOf (inst.result) > 1;

        switch (inst.op)
        {
            case YdspIrOp::loadStateArrayF:
            case YdspIrOp::loadStateArrayI:
            case YdspIrOp::loadInput:
            case YdspIrOp::loadOutput:
            case YdspIrOp::storeOutput:
            case YdspIrOp::negF:
            case YdspIrOp::absF:
            case YdspIrOp::sqrtF:
            case YdspIrOp::floorF:
            case YdspIrOp::ceilF:
            case YdspIrOp::rintF:
            case YdspIrOp::itof:
            case YdspIrOp::ftoi:
            case YdspIrOp::extI:
            case YdspIrOp::truncI:
            case YdspIrOp::extF:
            case YdspIrOp::truncF:
            case YdspIrOp::bitcastFtoI:
            case YdspIrOp::bitcastItoF:
            case YdspIrOp::notB:
            case YdspIrOp::movF:
            case YdspIrOp::movI:
            case YdspIrOp::movB:
            case YdspIrOp::vsplat:
            case YdspIrOp::vreduceAddF:
            case YdspIrOp::vextractF:
            case YdspIrOp::advanceWrapI:
                return { inst.a, -1, false, false, false };

            case YdspIrOp::vinsertF:
                if (inst.a >= 0)
                    return { inst.a, inst.b, false, false, false };

                return {};

            case YdspIrOp::wrapI:
                if (inst.result != inst.b)
                    return { inst.a, -1, false, false, false };

                return {};

            case YdspIrOp::negI:
            case YdspIrOp::absI:
                if (simd)
                    return { inst.a, -1, false, false, false };

                return { inst.a, -1, false, true, false };

            case YdspIrOp::signF:
                return { inst.a, -1, false, true, false };

            case YdspIrOp::divI:
            case YdspIrOp::modI:
                return { inst.b, -1, false, true, false };

            case YdspIrOp::addF:
            case YdspIrOp::mulF:
            case YdspIrOp::minF:
            case YdspIrOp::maxF:
            case YdspIrOp::andI:
            case YdspIrOp::orI:
            case YdspIrOp::xorI:
            case YdspIrOp::andB:
            case YdspIrOp::orB:
            case YdspIrOp::eqF:
            case YdspIrOp::neF:
            case YdspIrOp::eqI:
            case YdspIrOp::neI:
            case YdspIrOp::ltF:
            case YdspIrOp::leF:
            case YdspIrOp::gtF:
            case YdspIrOp::geF:
            case YdspIrOp::ltI:
            case YdspIrOp::leI:
            case YdspIrOp::gtI:
            case YdspIrOp::geI:
                return { inst.a, inst.b, true, false, false };

            case YdspIrOp::subF:
            case YdspIrOp::divF:
            case YdspIrOp::ltUI:
            case YdspIrOp::clampF:
            case YdspIrOp::powF:
            case YdspIrOp::atan2F:
            case YdspIrOp::fmodF:
            case YdspIrOp::copysignF:
                return { inst.a, inst.b, false, false, false };

            case YdspIrOp::addI:
            case YdspIrOp::subI:
            case YdspIrOp::mulI:
                if (! inst.saturatingIntegerArithmetic)
                    return { inst.a, inst.b, inst.op != YdspIrOp::subI, false, false };

                if (simd)
                    return { inst.a, -1, false, false, false };

                if (inst.op == YdspIrOp::mulI)
                    return { inst.b, -1, false, true, false };

                return {};

            case YdspIrOp::shlI:
            case YdspIrOp::shrI:
                if (simd && fn->laneCountOf (inst.b) > 1)
                    return { inst.a, -1, false, false, false };

                return { inst.a, inst.b, false, false, false };

            case YdspIrOp::minI:
            case YdspIrOp::maxI:
                if (simd)
                    return { inst.a, inst.b, true, false, false };

                return { inst.a, inst.b, true, true, true };

            case YdspIrOp::clampI:
                if (simd)
                    return { inst.a, inst.b, false, false, false };

                return { inst.a, inst.b, false, true, true };

            case YdspIrOp::modF:
                return { inst.a, inst.b, false, true, true };

            case YdspIrOp::lerpF:
                return { inst.b, inst.a, false, false, true };

            case YdspIrOp::selectB:
                return { inst.b, inst.c, false, false, false };

            default:
                if (isLibmOp (inst.op) && ! isLibmBinaryOp (inst.op))
                    return { inst.a, -1, false, false, false };

                return {};
        }
    }

    bool planBlock (const YdspIrBlock& block, bool canForwardTermCond, std::vector<InstEmitPlan>& plans) const
    {
        struct StackEntry
        {
            int valueId = -1;
            int producerIndex = -1;
            bool needsLocalWrite = false;
        };

        plans.assign (block.insts.size(), {});
        std::vector<StackEntry> simStack;

        const auto spillEntry = [&] (size_t idx)
        {
            const auto entry = simStack[idx];
            const bool unused = entry.valueId >= 0
                             && static_cast<size_t> (entry.valueId) < valueUses.size()
                             && valueUses[static_cast<size_t> (entry.valueId)] == 0;
            plans[static_cast<size_t> (entry.producerIndex)].action = (! entry.needsLocalWrite && unused)
                                                                    ? EmitAction::drop
                                                                    : EmitAction::localSet;
            simStack.erase (simStack.begin() + static_cast<ptrdiff_t> (idx));
        };

        const auto requireInLocal = [&] (int valueId)
        {
            if (valueId < 0)
                return;

            for (size_t idx = 0; idx < simStack.size(); ++idx)
            {
                if (simStack[idx].valueId == valueId)
                {
                    simStack[idx].needsLocalWrite = true;

                    if (static_cast<size_t> (valueId) < valueUses.size() && valueUses[static_cast<size_t> (valueId)] <= 1)
                        spillEntry (idx);

                    break;
                }
            }
        };

        for (size_t k = 0; k < block.insts.size(); ++k)
        {
            const auto& inst = block.insts[k];
            auto& plan = plans[k];
            const auto spec = describeOperands (inst);

            if (spec.second >= 0 && simStack.size() >= 2)
            {
                const size_t sz = simStack.size();

                if (simStack[sz - 2].valueId == spec.first && simStack[sz - 1].valueId == spec.second)
                {
                    plan.consumedFromStack = 2;
                }
                else if (spec.swappable && simStack[sz - 2].valueId == spec.second && simStack[sz - 1].valueId == spec.first)
                {
                    plan.swapOperands = true;
                    plan.consumedFromStack = 2;
                }
            }

            if (plan.consumedFromStack == 0 && spec.first >= 0 && ! simStack.empty())
            {
                if (simStack.back().valueId == spec.first)
                {
                    plan.consumedFromStack = 1;
                }
                else if (spec.second >= 0 && spec.swappable && simStack.back().valueId == spec.second)
                {
                    plan.swapOperands = true;
                    plan.consumedFromStack = 1;
                }
            }

            int consumedOp0 = -1;
            int consumedOp1 = -1;

            if (plan.consumedFromStack == 2)
            {
                auto secondEntry = simStack.back();
                simStack.pop_back();
                auto firstEntry = simStack.back();
                simStack.pop_back();

                const bool firstNeedsWrite = firstEntry.needsLocalWrite || (plan.swapOperands ? spec.multiReadSecond : spec.multiReadFirst);
                const bool secondNeedsWrite = secondEntry.needsLocalWrite || (plan.swapOperands ? spec.multiReadFirst : spec.multiReadSecond);

                plans[static_cast<size_t> (firstEntry.producerIndex)].action = firstNeedsWrite ? EmitAction::localTee : EmitAction::none;
                plans[static_cast<size_t> (secondEntry.producerIndex)].action = secondNeedsWrite ? EmitAction::localTee : EmitAction::none;

                consumedOp0 = spec.first;
                consumedOp1 = spec.second;
            }
            else if (plan.consumedFromStack == 1)
            {
                auto topEntry = simStack.back();
                simStack.pop_back();

                const bool topNeedsWrite = topEntry.needsLocalWrite || (plan.swapOperands ? spec.multiReadSecond : spec.multiReadFirst);
                plans[static_cast<size_t> (topEntry.producerIndex)].action = topNeedsWrite ? EmitAction::localTee : EmitAction::none;

                consumedOp0 = plan.swapOperands ? spec.second : spec.first;
            }

            const int operandFields[3] = {
                ydspFirstOperandIsSlot (inst.op) ? -1 : inst.a,
                inst.b,
                inst.c
            };

            for (const int operandId : operandFields)
            {
                if (operandId < 0)
                    continue;

                if (operandId == consumedOp0)
                {
                    consumedOp0 = -1;
                    continue;
                }

                if (operandId == consumedOp1)
                {
                    consumedOp1 = -1;
                    continue;
                }

                requireInLocal (operandId);
            }

            if (inst.result >= 0)
            {
                // Non-SSA redefinition invalidates any prior unconsumed value
                // with the same id on the simulated stack.
                for (size_t idx = simStack.size(); idx > 0; --idx)
                    if (simStack[idx - 1].valueId == inst.result)
                        spillEntry (idx - 1);

                constexpr size_t maxSimulatedStackDepth = 16;

                if (simStack.size() >= maxSimulatedStackDepth)
                    spillEntry (0);

                const bool multiUse = static_cast<size_t> (inst.result) < valueUses.size()
                                   && valueUses[static_cast<size_t> (inst.result)] > 1;
                simStack.push_back ({ inst.result, static_cast<int> (k), multiUse });
            }
        }

        bool termCondOnStack = false;

        if (canForwardTermCond && block.termCond >= 0 && ! simStack.empty() && simStack.back().valueId == block.termCond)
        {
            const auto topEntry = simStack.back();
            simStack.pop_back();
            plans[static_cast<size_t> (topEntry.producerIndex)].action = topEntry.needsLocalWrite ? EmitAction::localTee : EmitAction::none;
            termCondOnStack = true;
        }

        while (! simStack.empty())
            spillEntry (simStack.size() - 1);

        return termCondOnStack;
    }

    void beginInstruction (const YdspIrInst& inst, const InstEmitPlan& plan)
    {
        if (plan.consumedFromStack == 0)
        {
            pendingCount = 0;
            return;
        }

        const auto spec = describeOperands (inst);

        if (plan.consumedFromStack == 2)
        {
            pendingOnStack[0] = plan.swapOperands ? spec.second : spec.first;
            pendingOnStack[1] = plan.swapOperands ? spec.first : spec.second;
            pendingCount = 2;
        }
        else
        {
            pendingOnStack[0] = plan.swapOperands ? spec.second : spec.first;
            pendingCount = 1;
        }
    }

    void finishInstructionResult (int valueId, EmitAction action)
    {
        switch (action)
        {
            case EmitAction::none:
                break;

            case EmitAction::localSet:
                emitter.localSet (localIndex (valueId));
                break;

            case EmitAction::localTee:
                emitter.localTee (localIndex (valueId));
                break;

            case EmitAction::drop:
                emitter.op (YdspWasmEmitter::opDrop);
                break;
        }
    }

    bool emitInstructions (const YdspIrBlock& block, bool canForwardTermCond)
    {
        std::vector<InstEmitPlan> plans;
        const bool termCondOnStack = planBlock (block, canForwardTermCond, plans);

        for (size_t k = 0; k < block.insts.size(); ++k)
        {
            const auto& inst = block.insts[k];
            const auto& plan = plans[k];

            if (isMoveOpcode (inst.op) && inst.result == inst.a
                && plan.consumedFromStack == 0 && plan.action == EmitAction::localSet)
                continue;

            beginInstruction (inst, plan);
            const bool ok = emitInstruction (inst, plan);
            jassert (pendingCount == 0);

            if (ok && inst.result >= 0)
                finishInstructionResult (inst.result, plan.action);
        }

        return termCondOnStack;
    }

    bool emitInstruction (const YdspIrInst& inst, const InstEmitPlan& plan)
    {
        switch (inst.op)
        {
            // ---- constants ----
            case YdspIrOp::constF:
                loadFloatConst (inst.fvalue, valueType (inst.result));
                return true;

            case YdspIrOp::constI:
                if (valueType (inst.result) == YdspValueType::int64Type)
                    emitter.i64Const (inst.ivalue);
                else
                    emitter.i32Const (static_cast<int32_t> (inst.ivalue));
                return true;

            case YdspIrOp::constB:
                emitter.i32Const (inst.bvalue ? 1 : 0);
                return true;

            // ---- runtime values ----
            case YdspIrOp::loadBlockSize:
                if (numSamplesLocal >= 0)
                    emitter.localGet (static_cast<uint32_t> (numSamplesLocal));
                else
                    pushCtxLoad (kernelNumSamplesOff, YdspValueType::int32Type);
                return true;

            case YdspIrOp::loadSampleRate:
                if (sampleRateLocal >= 0)
                    emitter.localGet (static_cast<uint32_t> (sampleRateLocal));
                else
                    pushCtxLoad (sampleRateOff, YdspValueType::float32Type);
                return true;

            case YdspIrOp::loadEventFieldF:
                pushCtxLoad (wasmEventPayloadOffset (inst.memIndex), YdspValueType::float32Type);
                return true;

            case YdspIrOp::loadEventFieldI:
                pushCtxLoad (wasmEventPayloadOffset (inst.memIndex), YdspValueType::int32Type);
                return true;

            case YdspIrOp::storeEventFieldF:
            case YdspIrOp::storeEventFieldI:
                emitStoreAtOffset (outputEventsOff, outputEventsBaseLocal, wasmEventPayloadOffset (inst.memIndex), inst.a, valueType (inst.a));
                return true;

            case YdspIrOp::traceInt:
                pushBasePointer (traceQueueOff, traceQueueBaseLocal);
                emitter.i32Const (inst.memIndex);
                pushValue (inst.a);
                if (is64BitValueType (valueType (inst.a)))
                    emitter.op (YdspWasmEmitter::opI32WrapI64);
                if (is64BitValueType (valueType (inst.a)))
                {
                    pushValue (inst.a);
                    emitter.i64Const (32);
                    emitter.op (YdspWasmEmitter::opI64ShrU);
                    emitter.op (YdspWasmEmitter::opI32WrapI64);
                }
                else
                    emitter.i32Const (0); // bool
                emitter.call (static_cast<uint32_t> (traceIntFunc));
                return true;

            case YdspIrOp::traceFloat:
                pushBasePointer (traceQueueOff, traceQueueBaseLocal);
                emitter.i32Const (inst.memIndex);
                pushValue (inst.a);
                emitter.call (static_cast<uint32_t> (traceFloatFunc));
                return true;

            case YdspIrOp::traceCommit:
                pushBasePointer (traceQueueOff, traceQueueBaseLocal);
                emitter.i32Const (inst.memIndex);
                emitter.call (static_cast<uint32_t> (traceCommitFunc));
                return true;

            case YdspIrOp::emitEvent:
                pushBasePointer (outputEventsOff, outputEventsBaseLocal);
                emitter.i32Const (static_cast<int32_t> (inst.ivalue));
                pushValue (inst.a);
                emitter.i32Const (inst.memIndex);
                emitter.call (static_cast<uint32_t> (emitEventFuncIndex));
                return true;

            // ---- params and meters ----
            case YdspIrOp::loadParam:
            case YdspIrOp::loadParamOut:
            {
                const auto type = valueType (inst.result);
                const auto& offsets = inst.op == YdspIrOp::loadParam ? paramOffsets : paramOutOffsets;
                const auto fieldOff = inst.op == YdspIrOp::loadParam ? paramsOff : kernelParamOutOff;
                const auto hoistedLocal = inst.op == YdspIrOp::loadParam ? paramsBaseLocal : paramOutBaseLocal;

                emitLoadAtOffset (fieldOff, hoistedLocal, offsets[static_cast<size_t> (inst.a)], type);
                return true;
            }

            case YdspIrOp::storeParam:
            case YdspIrOp::storeParamOut:
            {
                const auto type = valueType (inst.a);
                const auto& offsets = inst.op == YdspIrOp::storeParam ? paramOffsets : paramOutOffsets;
                const auto fieldOff = inst.op == YdspIrOp::storeParam ? paramsOff : kernelParamOutOff;
                const auto hoistedLocal = inst.op == YdspIrOp::storeParam ? paramsBaseLocal : paramOutBaseLocal;

                emitStoreAtOffset (fieldOff, hoistedLocal, offsets[static_cast<size_t> (inst.memIndex)], inst.a, type);
                return true;
            }

            // ---- state scalars ----
            case YdspIrOp::loadStateF:
            case YdspIrOp::loadStateI:
            {
                const auto type = valueType (inst.result);
                emitLoadAtOffset (stateOff, stateBaseLocal, stateScalarBase (type, inst.a), type);
                return true;
            }

            case YdspIrOp::storeStateF:
            case YdspIrOp::storeStateI:
            {
                const auto type = valueType (inst.a);
                emitStoreAtOffset (stateOff, stateBaseLocal, stateScalarBase (type, inst.memIndex), inst.a, type);
                return true;
            }

            // ---- state arrays ----
            case YdspIrOp::loadStateArrayF:
            case YdspIrOp::loadStateArrayI:
            {
                const auto type = valueType (inst.result);
                const auto baseOffset = stateArrayBase (type, inst.memIndex);
                pushIndexedBaseAddress (stateArraysOff, stateArraysBaseLocal, inst.a, type);

                uint32_t memOffset = 0;

                if (baseOffset >= 0)
                {
                    memOffset = static_cast<uint32_t> (baseOffset);
                }
                else
                {
                    emitter.i32Const (baseOffset);
                    emitter.op (YdspWasmEmitter::opI32Add);
                }

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                    emitter.simdLoad (YdspWasmEmitter::opV128Load, 2, memOffset);
                else
#endif
                    emitter.load (loadOpcode (type), scaleLog2 (type), memOffset);

                return true;
            }

            case YdspIrOp::storeStateArrayF:
            case YdspIrOp::storeStateArrayI:
            {
                const auto type = valueType (inst.b);
                const auto baseOffset = stateArrayBase (type, inst.memIndex);
                pushIndexedBaseAddress (stateArraysOff, stateArraysBaseLocal, inst.a, type);

                uint32_t memOffset = 0;

                if (baseOffset >= 0)
                {
                    memOffset = static_cast<uint32_t> (baseOffset);
                }
                else
                {
                    emitter.i32Const (baseOffset);
                    emitter.op (YdspWasmEmitter::opI32Add);
                }

                pushValue (inst.b);

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.b) > 1)
                    emitter.simdStore (YdspWasmEmitter::opV128Store, 2, memOffset);
                else
#endif
                    emitter.store (storeOpcode (type), scaleLog2 (type), memOffset);

                return true;
            }

            // ---- streams ----
            case YdspIrOp::loadInputAddress:
            case YdspIrOp::loadOutputAddress:
                pushStreamBase (inst.op == YdspIrOp::loadInputAddress, inst.memIndex);
                emitter.op (YdspWasmEmitter::opI64ExtendI32U);
                return true;

            case YdspIrOp::loadInput:
            case YdspIrOp::loadOutput:
            {
                const auto type = valueType (inst.result);
                pushStreamIndexedAddress (inst.op == YdspIrOp::loadInput, inst.memIndex, inst.a, type);

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                    emitter.simdLoad (YdspWasmEmitter::opV128Load, 2, 0);
                else
#endif
                    emitter.load (loadOpcode (type), scaleLog2 (type), 0);

                return true;
            }

            case YdspIrOp::storeOutput:
            {
                const auto type = valueType (inst.b);
                pushStreamIndexedAddress (false, inst.memIndex, inst.a, type);
                pushValue (inst.b);

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.b) > 1)
                    emitter.simdStore (YdspWasmEmitter::opV128Store, 2, 0);
                else
#endif
                    emitter.store (storeOpcode (type), scaleLog2 (type), 0);

                return true;
            }

            // ---- float arithmetic ----
            case YdspIrOp::addF:
            case YdspIrOp::subF:
            case YdspIrOp::mulF:
            case YdspIrOp::divF:
            case YdspIrOp::minF:
            case YdspIrOp::maxF:
            {
                const auto type = valueType (inst.result);
                const int lhs = plan.swapOperands ? inst.b : inst.a;
                const int rhs = plan.swapOperands ? inst.a : inst.b;

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (lhs);
                    pushValue (rhs);
                    emitter.simdOp (packedFloatBinarySimdOp (inst.op));
                    return true;
                }
#endif

                pushValue (lhs);
                pushValue (rhs);
                emitter.op (floatBinaryOpcode (inst.op, is64BitValueType (type)));
                return true;
            }

            case YdspIrOp::modF:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                // a % b = a + (-trunc(a / b)) * b
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (floatBinaryOpcode (YdspIrOp::divF, is64));
                emitter.op (floatTruncOpcode (is64));
                emitter.op (floatUnaryOpcode (YdspIrOp::negF, is64));
                pushValue (inst.b);
                emitter.op (floatBinaryOpcode (YdspIrOp::mulF, is64));
                pushValue (inst.a);
                emitter.op (floatBinaryOpcode (YdspIrOp::addF, is64));
                return true;
            }

            case YdspIrOp::negF:
            case YdspIrOp::absF:
            case YdspIrOp::sqrtF:
            case YdspIrOp::floorF:
            case YdspIrOp::ceilF:
            case YdspIrOp::rintF:
            {
                const auto type = valueType (inst.result);

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (inst.a);
                    emitter.simdOp (packedFloatUnarySimdOp (inst.op));
                    return true;
                }
#endif

                pushValue (inst.a);
                emitter.op (floatUnaryOpcode (inst.op, is64BitValueType (type)));
                return true;
            }

            case YdspIrOp::clampF:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    // f32x4.max (a, b), then f32x4.min (.., c)
                    pushValue (inst.a);
                    pushValue (inst.b);
                    emitter.simdOp (YdspWasmEmitter::opF32x4Max);
                    pushValue (inst.c);
                    emitter.simdOp (YdspWasmEmitter::opF32x4Min);
                    return true;
                }
#endif

                // min (max (a, b), c)
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (floatBinaryOpcode (YdspIrOp::maxF, is64));
                pushValue (inst.c);
                emitter.op (floatBinaryOpcode (YdspIrOp::minF, is64));
                return true;
            }

            case YdspIrOp::lerpF:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    // (b - a) * t + a
                    pushValue (inst.b);
                    pushValue (inst.a);
                    emitter.simdOp (YdspWasmEmitter::opF32x4Sub);
                    pushValue (inst.c);
                    emitter.simdOp (YdspWasmEmitter::opF32x4Mul);
                    pushValue (inst.a);
                    emitter.simdOp (YdspWasmEmitter::opF32x4Add);
                    return true;
                }
#endif

                // (b - a) * t + a
                pushValue (inst.b);
                pushValue (inst.a);
                emitter.op (floatBinaryOpcode (YdspIrOp::subF, is64));
                pushValue (inst.c);
                emitter.op (floatBinaryOpcode (YdspIrOp::mulF, is64));
                pushValue (inst.a);
                emitter.op (floatBinaryOpcode (YdspIrOp::addF, is64));
                return true;
            }

            // ---- int arithmetic ----
            case YdspIrOp::addI:
            case YdspIrOp::subI:
            case YdspIrOp::mulI:
            {
#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    if (! inst.saturatingIntegerArithmetic)
                    {
                        const int lhs = plan.swapOperands ? inst.b : inst.a;
                        const int rhs = plan.swapOperands ? inst.a : inst.b;
                        pushValue (lhs);
                        pushValue (rhs);
                        emitter.simdOp (inst.op == YdspIrOp::addI ? YdspWasmEmitter::opI32x4Add
                                      : inst.op == YdspIrOp::subI ? YdspWasmEmitter::opI32x4Sub
                                                                  : YdspWasmEmitter::opI32x4Mul);
                        return true;
                    }

                    const auto scratch0 = simdScratchBase;
                    const auto scratch1 = scratch0 + 1u;
                    const auto scratch2 = scratch0 + 2u;
                    const auto scratch3 = scratch0 + 3u;

                    pushValue (inst.a);
                    emitter.localSet (scratch0);
                    pushValue (inst.b);
                    emitter.localSet (scratch1);

                    if (inst.op == YdspIrOp::mulI)
                    {
                        static const uint8_t loShuffle[16] = { 0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27 };
                        static const uint8_t hiShuffle[16] = { 4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31 };

                        emitter.localGet (scratch0);
                        emitter.localGet (scratch1);
                        emitter.simdOp (YdspWasmEmitter::opI64x2ExtmulLowI32x4S);
                        emitter.localSet (scratch2);

                        emitter.localGet (scratch0);
                        emitter.localGet (scratch1);
                        emitter.simdOp (YdspWasmEmitter::opI64x2ExtmulHighI32x4S);
                        emitter.localSet (scratch3);

                        // lo32
                        emitter.localGet (scratch2);
                        emitter.localGet (scratch3);
                        emitter.i8x16Shuffle (loShuffle);

                        // limit = ((a ^ b) >> 31) ^ INT32_MAX
                        emitter.localGet (scratch0);
                        emitter.localGet (scratch1);
                        emitter.simdOp (YdspWasmEmitter::opV128Xor);
                        emitter.i32Const (31);
                        emitter.simdOp (YdspWasmEmitter::opI32x4ShrS);
                        emitter.i32Const (std::numeric_limits<int32_t>::max());
                        emitter.simdOp (YdspWasmEmitter::opI32x4Splat);
                        emitter.simdOp (YdspWasmEmitter::opV128Xor);

                        // ok = (hi32 == (lo32 >> 31))
                        emitter.localGet (scratch2);
                        emitter.localGet (scratch3);
                        emitter.i8x16Shuffle (hiShuffle);
                        emitter.localGet (scratch2);
                        emitter.localGet (scratch3);
                        emitter.i8x16Shuffle (loShuffle);
                        emitter.i32Const (31);
                        emitter.simdOp (YdspWasmEmitter::opI32x4ShrS);
                        emitter.simdOp (YdspWasmEmitter::opI32x4Eq);

                        emitter.simdOp (YdspWasmEmitter::opV128Bitselect);
                        return true;
                    }

                    // sum/diff = a +/- b
                    emitter.localGet (scratch0);
                    emitter.localGet (scratch1);
                    emitter.simdOp (inst.op == YdspIrOp::addI ? YdspWasmEmitter::opI32x4Add : YdspWasmEmitter::opI32x4Sub);
                    emitter.localSet (scratch2);

                    // limit = (a >> 31) ^ INT32_MAX
                    emitter.localGet (scratch0);
                    emitter.i32Const (31);
                    emitter.simdOp (YdspWasmEmitter::opI32x4ShrS);
                    emitter.i32Const (std::numeric_limits<int32_t>::max());
                    emitter.simdOp (YdspWasmEmitter::opI32x4Splat);
                    emitter.simdOp (YdspWasmEmitter::opV128Xor);

                    // sum/diff
                    emitter.localGet (scratch2);

                    // overflow = ((~(a ^ b) & (a ^ sum)) >> 31)  [add]
                    //          = (((a ^ b) & (a ^ diff)) >> 31)   [sub]
                    emitter.localGet (scratch0);
                    emitter.localGet (scratch1);
                    emitter.simdOp (YdspWasmEmitter::opV128Xor);
                    if (inst.op == YdspIrOp::addI)
                        emitter.simdOp (YdspWasmEmitter::opV128Not);
                    emitter.localGet (scratch0);
                    emitter.localGet (scratch2);
                    emitter.simdOp (YdspWasmEmitter::opV128Xor);
                    emitter.simdOp (YdspWasmEmitter::opV128And);
                    emitter.i32Const (31);
                    emitter.simdOp (YdspWasmEmitter::opI32x4ShrS);

                    emitter.simdOp (YdspWasmEmitter::opV128Bitselect);
                    return true;
                }
#endif

                if (! inst.saturatingIntegerArithmetic)
                {
                    const int lhs = plan.swapOperands ? inst.b : inst.a;
                    const int rhs = plan.swapOperands ? inst.a : inst.b;
                    pushValue (lhs);
                    pushValue (rhs);
                    emitter.op (intBinaryOpcode (inst.op, is64BitValueType (valueType (inst.result))));
                    return true;
                }
                const bool wide = is64BitValueType (valueType (inst.result));
                const auto constant = [&] (int64_t value)
                {
                    if (wide)
                        emitter.i64Const (value);
                    else
                        emitter.i32Const (static_cast<int32_t> (value));
                };
                const auto binary = [&] (YdspIrOp op) { emitter.op (intBinaryOpcode (op, wide)); };
                const auto negative = [&]
                {
                    pushValue (inst.a);
                    if (inst.op == YdspIrOp::mulI)
                    {
                        pushValue (inst.b);
                        binary (YdspIrOp::xorI);
                    }
                    constant (0);
                    emitter.op (intCompareOpcode (YdspIrOp::ltI, wide));
                };
                const auto raw = [&]
                {
                    pushValue (inst.a);
                    pushValue (inst.b);
                    binary (inst.op);
                };
                const auto minimum = wide ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int32_t>::min();
                const auto maximum = wide ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max();
                if (inst.op == YdspIrOp::mulI)
                {
                    pushValue (inst.b);
                    emitter.op (wide ? YdspWasmEmitter::opI64Eqz : YdspWasmEmitter::opI32Eqz);
                    emitter.ifValue (wide ? ValType::i64 : ValType::i32);
                    constant (0);
                    emitter.else_();
                }

                constant (minimum);
                constant (maximum);
                negative();
                emitter.select();
                raw();

                if (inst.op == YdspIrOp::mulI)
                {
                    const auto magnitude = [&] (int value)
                    {
                        constant (0);
                        pushValue (value);
                        binary (YdspIrOp::subI);
                        pushValue (value);
                        pushValue (value);
                        constant (0);
                        emitter.op (intCompareOpcode (YdspIrOp::ltI, wide));
                        emitter.select();
                    };
                    magnitude (inst.a);
                    // INT_MIN's bit pattern is its unsigned magnitude.
                    constant (minimum);
                    constant (maximum);
                    negative();
                    emitter.select();
                    magnitude (inst.b);
                    emitter.op (wide ? YdspWasmEmitter::opI64DivU : YdspWasmEmitter::opI32DivU);
                    emitter.op (wide ? YdspWasmEmitter::opI64GtU : YdspWasmEmitter::opI32GtU);
                }
                else
                {
                    pushValue (inst.a);
                    if (inst.op == YdspIrOp::addI)
                        raw();
                    else
                        pushValue (inst.b);
                    binary (YdspIrOp::xorI);
                    if (inst.op == YdspIrOp::addI)
                        pushValue (inst.b);
                    else
                        pushValue (inst.a);
                    raw();
                    binary (YdspIrOp::xorI);
                    binary (YdspIrOp::andI);
                    constant (0);
                    emitter.op (intCompareOpcode (YdspIrOp::ltI, wide));
                }
                emitter.select();
                if (inst.op == YdspIrOp::mulI)
                    emitter.end();
                return true;
            }
            case YdspIrOp::andI:
            case YdspIrOp::orI:
            case YdspIrOp::xorI:
            {
                const int lhs = plan.swapOperands ? inst.b : inst.a;
                const int rhs = plan.swapOperands ? inst.a : inst.b;

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (lhs);
                    pushValue (rhs);
                    emitter.simdOp (inst.op == YdspIrOp::andI ? YdspWasmEmitter::opV128And
                                  : inst.op == YdspIrOp::orI  ? YdspWasmEmitter::opV128Or
                                                              : YdspWasmEmitter::opV128Xor);
                    return true;
                }
#endif

                const auto type = valueType (inst.result);
                pushValue (lhs);
                pushValue (rhs);
                emitter.op (intBinaryOpcode (inst.op, is64BitValueType (type)));
                return true;
            }

            case YdspIrOp::shlI:
            case YdspIrOp::shrI:
            {
#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    if (fn->laneCountOf (inst.b) == 1)
                    {
                        pushValue (inst.a);
                        pushValue (inst.b);
                        emitter.simdOp (inst.op == YdspIrOp::shlI ? YdspWasmEmitter::opI32x4Shl : YdspWasmEmitter::opI32x4ShrS);
                        return true;
                    }

                    const auto scratch0 = simdScratchBase;
                    const auto scratch1 = scratch0 + 1u;
                    pushValue (inst.a);
                    emitter.localSet (scratch0);
                    pushValue (inst.b);
                    emitter.localSet (scratch1);

                    emitter.localGet (scratch0);

                    for (uint32_t lane = 0; lane < 4; ++lane)
                    {
                        emitter.localGet (scratch0);
                        emitter.i32x4ExtractLane (lane);
                        emitter.localGet (scratch1);
                        emitter.i32x4ExtractLane (lane);
                        emitter.op (inst.op == YdspIrOp::shlI ? YdspWasmEmitter::opI32Shl : YdspWasmEmitter::opI32ShrS);
                        emitter.i32x4ReplaceLane (lane);

                        if (lane < 3)
                            emitter.localTee (scratch0);
                    }

                    return true;
                }
#endif

                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                pushValue (inst.a);
                pushValue (inst.b);

                if (inst.op == YdspIrOp::shlI)
                    emitter.op (is64 ? YdspWasmEmitter::opI64Shl : YdspWasmEmitter::opI32Shl);
                else
                    emitter.op (is64 ? YdspWasmEmitter::opI64ShrS : YdspWasmEmitter::opI32ShrS);

                return true;
            }

            case YdspIrOp::negI:
            case YdspIrOp::absI:
            {
#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    const auto scratch0 = simdScratchBase;
                    pushValue (inst.a);
                    emitter.simdOp (inst.op == YdspIrOp::negI ? YdspWasmEmitter::opI32x4Neg : YdspWasmEmitter::opI32x4Abs);
                    emitter.localTee (scratch0);

                    emitter.localGet (scratch0);
                    emitter.i32Const (std::numeric_limits<int32_t>::min());
                    emitter.simdOp (YdspWasmEmitter::opI32x4Splat);
                    emitter.simdOp (YdspWasmEmitter::opI32x4Eq);
                    emitter.simdOp (YdspWasmEmitter::opI32x4Add);
                    return true;
                }
#endif

                const bool is64 = is64BitValueType (valueType (inst.result));
                pushValue (inst.a);
                if (is64)
                    emitter.i64Const (std::numeric_limits<int64_t>::min());
                else
                    emitter.i32Const (std::numeric_limits<int32_t>::min());
                emitter.op (is64 ? YdspWasmEmitter::opI64Eq : YdspWasmEmitter::opI32Eq);
                emitter.ifValue (is64 ? ValType::i64 : ValType::i32);
                if (is64)
                    emitter.i64Const (std::numeric_limits<int64_t>::max());
                else
                    emitter.i32Const (std::numeric_limits<int32_t>::max());
                emitter.else_();
                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);
                pushValue (inst.a);
                emitter.op (is64 ? YdspWasmEmitter::opI64Sub : YdspWasmEmitter::opI32Sub);
                if (inst.op == YdspIrOp::absI)
                {
                    pushValue (inst.a);
                    pushValue (inst.a);
                    if (is64)
                        emitter.i64Const (0);
                    else
                        emitter.i32Const (0);
                    emitter.op (intCompareOpcode (YdspIrOp::ltI, is64));
                    emitter.select();
                }
                emitter.end();
                return true;
            }

            case YdspIrOp::divI:
            case YdspIrOp::modI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                const auto eqzOp = is64 ? YdspWasmEmitter::opI64Eqz : YdspWasmEmitter::opI32Eqz;
                const auto eqOp = is64 ? YdspWasmEmitter::opI64Eq : YdspWasmEmitter::opI32Eq;

                // Zero divisors return zero; INT_MIN / -1 saturates, with remainder zero.
                pushValue (inst.b);
                emitter.op (eqzOp);
                emitter.ifValue (is64 ? ValType::i64 : ValType::i32);

                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);

                emitter.else_();

                pushValue (inst.a);
                if (is64)
                    emitter.i64Const (std::numeric_limits<int64_t>::min());
                else
                    emitter.i32Const (std::numeric_limits<int32_t>::min());
                emitter.op (eqOp); // a == INT_MIN

                pushValue (inst.b);
                if (is64)
                    emitter.i64Const (-1);
                else
                    emitter.i32Const (-1);
                emitter.op (eqOp); // b == -1

                emitter.op (YdspWasmEmitter::opI32And); // comparisons yield i32
                emitter.ifValue (is64 ? ValType::i64 : ValType::i32);

                if (is64)
                    emitter.i64Const (inst.op == YdspIrOp::divI ? std::numeric_limits<int64_t>::max() : 0);
                else
                    emitter.i32Const (inst.op == YdspIrOp::divI ? std::numeric_limits<int32_t>::max() : 0);

                emitter.else_();
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (intDivRemOpcode (inst.op, is64));
                emitter.end();

                emitter.end();

                return true;
            }

            case YdspIrOp::wrapI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                if (inst.result != inst.b)
                {
                    // (a < b) ? a : 0 - matches (a >= b) ? 0 : a while pushing `a` first once.
                    pushValue (inst.a);
                    emitter.localTee (localIndex (inst.result));

                    if (is64)
                        emitter.i64Const (0);
                    else
                        emitter.i32Const (0);

                    emitter.localGet (localIndex (inst.result));
                    pushValue (inst.b);
                    emitter.op (intCompareOpcode (YdspIrOp::ltI, is64));
                    emitter.select();
                    return true;
                }

                // (a >= b) ? 0 : a  - see YdspIrOp::wrapI.
                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);

                pushValue (inst.a);
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (intCompareOpcode (YdspIrOp::geI, is64));

                emitter.select();
                return true;
            }

            case YdspIrOp::advanceWrapI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                // next = a + 1; (next < bound) ? next : 0
                pushValue (inst.a);

                if (is64)
                    emitter.i64Const (1);
                else
                    emitter.i32Const (1);

                emitter.op (is64 ? YdspWasmEmitter::opI64Add : YdspWasmEmitter::opI32Add);
                emitter.localTee (localIndex (inst.result));

                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);

                emitter.localGet (localIndex (inst.result));

                if (is64)
                    emitter.i64Const (inst.ivalue);
                else
                    emitter.i32Const (static_cast<int32_t> (inst.ivalue));

                emitter.op (is64 ? YdspWasmEmitter::opI64LtS : YdspWasmEmitter::opI32LtS);
                emitter.select();
                return true;
            }

            case YdspIrOp::minI:
            case YdspIrOp::maxI:
            {
                const int lhs = plan.swapOperands ? inst.b : inst.a;
                const int rhs = plan.swapOperands ? inst.a : inst.b;

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (lhs);
                    pushValue (rhs);
                    emitter.simdOp (inst.op == YdspIrOp::minI ? YdspWasmEmitter::opI32x4MinS : YdspWasmEmitter::opI32x4MaxS);
                    return true;
                }
#endif

                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                pushValue (lhs);
                pushValue (rhs);

                pushValue (lhs);
                pushValue (rhs);
                emitter.op (intCompareOpcode (inst.op == YdspIrOp::minI ? YdspIrOp::ltI : YdspIrOp::gtI, is64));

                emitter.select();
                return true;
            }

            case YdspIrOp::clampI:
            {
#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (inst.a);
                    pushValue (inst.b);
                    emitter.simdOp (YdspWasmEmitter::opI32x4MaxS);
                    pushValue (inst.c);
                    emitter.simdOp (YdspWasmEmitter::opI32x4MinS);
                    return true;
                }
#endif

                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                pushValue (inst.a);
                pushValue (inst.b);
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (intCompareOpcode (YdspIrOp::gtI, is64));
                emitter.select();

                if (inst.result != inst.c)
                {
                    emitter.localTee (localIndex (inst.result));
                    pushValue (inst.c);
                    emitter.localGet (localIndex (inst.result));
                    pushValue (inst.c);
                    emitter.op (intCompareOpcode (YdspIrOp::ltI, is64));
                    emitter.select();
                    return true;
                }

                pushValue (inst.c);

                pushValue (inst.a);
                pushValue (inst.b);
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (intCompareOpcode (YdspIrOp::gtI, is64));
                emitter.select();
                pushValue (inst.c);
                emitter.op (intCompareOpcode (YdspIrOp::ltI, is64));

                emitter.select();
                return true;
            }

            case YdspIrOp::signI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                if (is64)
                    emitter.i64Const (1);
                else
                    emitter.i32Const (1);

                if (is64)
                    emitter.i64Const (-1);
                else
                    emitter.i32Const (-1);
                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);
                pushValue (inst.a);
                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);
                emitter.op (intCompareOpcode (YdspIrOp::ltI, is64));
                emitter.select();

                pushValue (inst.a);
                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);
                emitter.op (intCompareOpcode (YdspIrOp::gtI, is64));

                emitter.select();
                return true;
            }

            // ---- conversions ----
            case YdspIrOp::itof:
            case YdspIrOp::ftoi:
            {
#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (inst.a);
                    emitter.simdOp (inst.op == YdspIrOp::itof ? YdspWasmEmitter::opF32x4ConvertI32x4S
                                                              : YdspWasmEmitter::opI32x4TruncSatF32x4S);
                    return true;
                }
#endif

                const auto src = valueType (inst.a);
                const auto dst = valueType (inst.result);
                pushValue (inst.a);
                if (inst.op == YdspIrOp::ftoi && ! inst.boundedFloatToInt)
                {
                    emitter.op (0xFC); // saturating float-to-integer conversion
                    emitter.op (static_cast<uint8_t> ((is64BitValueType (dst) ? 4 : 0) + (is64BitValueType (src) ? 2 : 0)));
                }
                else
                    emitter.op (convertOpcode (inst.op, src, dst));
                return true;
            }

            case YdspIrOp::extI:
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opI64ExtendI32S);
                return true;

            case YdspIrOp::truncI:
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opI32WrapI64);
                return true;

            case YdspIrOp::extF:
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opF64PromoteF32);
                return true;

            case YdspIrOp::truncF:
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opF32DemoteF64);
                return true;

            case YdspIrOp::bitcastFtoI:
            {
#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (inst.a);
                    return true;
                }
#endif
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opI32ReinterpretF32);
                return true;
            }

            case YdspIrOp::bitcastItoF:
            {
#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (inst.a);
                    return true;
                }
#endif
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opF32ReinterpretI32);
                return true;
            }

            // ---- comparisons ----
            case YdspIrOp::eqF:
            case YdspIrOp::neF:
            case YdspIrOp::ltF:
            case YdspIrOp::leF:
            case YdspIrOp::gtF:
            case YdspIrOp::geF:
            {
                const int lhs = plan.swapOperands ? inst.b : inst.a;
                const int rhs = plan.swapOperands ? inst.a : inst.b;
                const auto cmpOp = plan.swapOperands ? flippedComparisonOp (inst.op) : inst.op;

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (lhs);
                    pushValue (rhs);
                    emitter.simdOp (packedFloatCompareSimdOp (cmpOp));
                    return true;
                }
#endif

                const auto type = valueType (inst.a);
                pushValue (lhs);
                pushValue (rhs);
                emitter.op (floatCompareOpcode (cmpOp, is64BitValueType (type)));
                return true;
            }

            case YdspIrOp::eqI:
            case YdspIrOp::neI:
            case YdspIrOp::ltI:
            case YdspIrOp::ltUI:
            case YdspIrOp::leI:
            case YdspIrOp::gtI:
            case YdspIrOp::geI:
            {
                const int lhs = plan.swapOperands ? inst.b : inst.a;
                const int rhs = plan.swapOperands ? inst.a : inst.b;
                const auto cmpOp = plan.swapOperands ? flippedComparisonOp (inst.op) : inst.op;

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (lhs);
                    pushValue (rhs);
                    emitter.simdOp (packedIntCompareSimdOp (cmpOp));
                    return true;
                }
#endif

                const auto type = valueType (inst.a);
                pushValue (lhs);
                pushValue (rhs);
                emitter.op (intCompareOpcode (cmpOp, is64BitValueType (type)));
                return true;
            }

            // ---- logic ----
            case YdspIrOp::andB:
            {
                const int lhs = plan.swapOperands ? inst.b : inst.a;
                const int rhs = plan.swapOperands ? inst.a : inst.b;

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (lhs);
                    pushValue (rhs);
                    emitter.simdOp (YdspWasmEmitter::opV128And);
                    return true;
                }
#endif
                pushValue (lhs);
                pushValue (rhs);
                emitter.op (YdspWasmEmitter::opI32And);
                return true;
            }

            case YdspIrOp::orB:
            {
                const int lhs = plan.swapOperands ? inst.b : inst.a;
                const int rhs = plan.swapOperands ? inst.a : inst.b;

#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (lhs);
                    pushValue (rhs);
                    emitter.simdOp (YdspWasmEmitter::opV128Or);
                    return true;
                }
#endif
                pushValue (lhs);
                pushValue (rhs);
                emitter.op (YdspWasmEmitter::opI32Or);
                return true;
            }

            case YdspIrOp::notB:
#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (inst.a);
                    emitter.simdOp (YdspWasmEmitter::opV128Not);
                    return true;
                }
#endif
                pushValue (inst.a);
                emitter.i32Const (1);
                emitter.op (YdspWasmEmitter::opI32Xor);
                return true;

            // ---- moves ----
            case YdspIrOp::movF:
            case YdspIrOp::movI:
            case YdspIrOp::movB:
                pushValue (inst.a);
                return true;

            // ---- select ----
            case YdspIrOp::selectB:
#if defined(__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    // Widened select: bitwise per-lane pick with the mask from
                    // a widened compare. v128.bitselect (mask ? b : c) pops its
                    // mask last, so the b, c, a push order matches the scalar
                    // select below.
                    pushValue (inst.b);
                    pushValue (inst.c);
                    pushValue (inst.a);
                    emitter.simdOp (YdspWasmEmitter::opV128Bitselect);
                    return true;
                }
#endif
                pushValue (inst.b);
                pushValue (inst.c);
                pushValue (inst.a);
                emitter.select();
                return true;

            // ---- sign ----
            case YdspIrOp::signF:
            {
                const auto operandType = valueType (inst.a);
                const auto resultType = valueType (inst.result);
                const bool is64 = is64BitValueType (operandType);

                // sign(x) = convert ((x > 0) - (x < 0))
                pushValue (inst.a);
                loadFloatConst (0.0, operandType);
                emitter.op (floatCompareOpcode (YdspIrOp::gtF, is64));

                pushValue (inst.a);
                loadFloatConst (0.0, operandType);
                emitter.op (floatCompareOpcode (YdspIrOp::ltF, is64));

                emitter.op (YdspWasmEmitter::opI32Sub);
                emitter.op (convertOpcode (YdspIrOp::itof, YdspValueType::int32Type, resultType));
                return true;
            }

            // ---- libm intrinsics ----
            case YdspIrOp::sinF:
            case YdspIrOp::cosF:
            case YdspIrOp::tanF:
            case YdspIrOp::asinF:
            case YdspIrOp::acosF:
            case YdspIrOp::atanF:
            case YdspIrOp::sinhF:
            case YdspIrOp::coshF:
            case YdspIrOp::tanhF:
            case YdspIrOp::asinhF:
            case YdspIrOp::acoshF:
            case YdspIrOp::atanhF:
            case YdspIrOp::roundF:
            case YdspIrOp::expF:
            case YdspIrOp::logF:
            case YdspIrOp::log10F:
            case YdspIrOp::powF:
            case YdspIrOp::atan2F:
            case YdspIrOp::fmodF:
            case YdspIrOp::copysignF:
            {
                const auto operandType = valueType (inst.a);
                const auto name = std::string (libmBaseName (inst.op)) + (is64BitValueType (operandType) ? "" : "f");

                if (isLibmBinaryOp (inst.op))
                {
                    pushValue (inst.a);
                    pushValue (inst.b);
                }
                else
                {
                    pushValue (inst.a);
                }

                emitter.call (static_cast<uint32_t> (libmFuncIndex[name]));
                return true;
            }

#if defined(__wasm_simd128__)
            // ---- lane movement (SIMD) ----
            case YdspIrOp::vsplat:
                pushValue (inst.a);
                emitter.simdOp (isFloatValueType (valueType (inst.result)) ? YdspWasmEmitter::opF32x4Splat
                                                                           : YdspWasmEmitter::opI32x4Splat);
                return true;

            case YdspIrOp::vinsertF:
            {
                if (inst.memIndex < 0 || inst.memIndex >= 4)
                {
                    pendingCount = 0;
                    diagnostics->addError ({}, "Wasm codegen: vinsertF lane index out of range");
                    return false;
                }

                if (inst.a >= 0)
                    pushValue (inst.a);
                else
                    emitter.v128ConstZero();

                pushValue (inst.b);
                emitter.f32x4ReplaceLane (static_cast<uint32_t> (inst.memIndex));
                return true;
            }

            case YdspIrOp::vextractF:
            {
                if (inst.memIndex < 0 || inst.memIndex >= 4)
                {
                    pendingCount = 0;
                    diagnostics->addError ({}, "Wasm codegen: vextractF lane index out of range");
                    return false;
                }

                pushValue (inst.a);
                emitter.f32x4ExtractLane (static_cast<uint32_t> (inst.memIndex));
                return true;
            }

            case YdspIrOp::vreduceAddF:
            {
                // Horizontal f32x4 sum as a shuffle tree. Park the input in
                // scratch via local.tee so it only needs one operand push (and
                // can consume from the stack top), then fold [2,3,0,1] and
                // [1,0,3,2] with local.tee on the intermediate partials.
                static const uint8_t swap64Mask[16] = { 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 };
                static const uint8_t swap32Mask[16] = { 4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11 };

                const auto scratch = simdScratchBase;

                pushValue (inst.a);
                emitter.localTee (scratch);
                emitter.localGet (scratch);
                emitter.i8x16Shuffle (swap64Mask);
                emitter.localGet (scratch);
                emitter.simdOp (YdspWasmEmitter::opF32x4Add);
                emitter.localTee (scratch);
                emitter.localGet (scratch);
                emitter.i8x16Shuffle (swap32Mask);
                emitter.localGet (scratch);
                emitter.simdOp (YdspWasmEmitter::opF32x4Add);
                emitter.f32x4ExtractLane (0);
                return true;
            }
#endif

            default:
                diagnostics->addError ({}, "Wasm codegen: unsupported instruction in the IR");
                return false;
        }
    }

    //==============================================================================

    const YdspIrFunction* fn = nullptr;
    YdspDiagnostics* diagnostics = nullptr;

    YdspWasmEmitter emitter;

    // Ctx field offsets (kernel or event layout).
    int stateOff = 0;
    int stateArraysOff = 0;
    int paramsOff = 0;
    int sampleRateOff = 0;
    int outputEventsOff = 0;

    std::vector<int> paramOffsets;    // byte offset per param slot (heterogeneous)
    std::vector<int> paramOutOffsets; // byte offset per meter slot (heterogeneous)

    std::vector<YdspValueType> valueTypes;
    std::vector<int> valueUses;

    // Usage tracking and hoisted Wasm local indices (-1 = not hoisted).
    UsageStats stateUsage;
    UsageStats stateArraysUsage;
    UsageStats paramsUsage;
    UsageStats paramOutUsage;
    UsageStats outputEventsUsage;
    UsageStats traceQueueUsage;
    UsageStats sampleRateUsage;
    UsageStats numSamplesUsage;
    std::map<int, UsageStats> inputStreamUsage;
    std::map<int, UsageStats> outputStreamUsage;

    int stateBaseLocal = -1;
    int stateArraysBaseLocal = -1;
    int paramsBaseLocal = -1;
    int paramOutBaseLocal = -1;
    int outputEventsBaseLocal = -1;
    int traceQueueBaseLocal = -1;
    int numSamplesLocal = -1;
    int sampleRateLocal = -1;
    std::map<int, int> inStreamLocals;
    std::map<int, int> outStreamLocals;

    // Stack-top operand tracking for the currently emitting instruction.
    int pendingOnStack[2] = { -1, -1 };
    int pendingCount = 0;

    // libm imports, deterministic order (sorted by name).
    std::set<std::string> libmNames;
    std::vector<LibmSignature> libmSignatures;
    std::map<LibmSignature, int> sigTypeIndex;
    std::map<std::string, int> libmFuncIndex;

    // The output-event commit import, only added when the function uses it.
    bool usesTrace = false;
    int traceQueueOff = 0;
    int traceIntType = 0, traceFloatType = 0, traceCommitType = 0;
    int traceIntFunc = 0, traceFloatFunc = 0, traceCommitFunc = 0;
    bool usesEmitEvent = false;
    int emitEventTypeIndex = 0;
    int emitEventFuncIndex = 0;

#if defined(__wasm_simd128__)
    // Set when the function uses widened operations or vreduceAddF, which
    // need scratch v128 locals declared after the value locals.
    bool usesSimdScratch = false;
    uint32_t simdScratchBase = 0;
#endif

    // IR loop headers -> index into fn.loops.
    std::map<int, int> loopByHeader;

    // Open wasm control constructs (innermost last).
    std::vector<Label> labels;

    int kernelTypeIndex = 0;
    int kernelFuncIndex = 0;

    int recursionDepth = 0;
};

} // namespace

//==============================================================================

std::vector<uint8_t> YdspWasmCodegen::compile (const YdspIrFunction& fn, YdspDiagnostics& diagnostics)
{
#if defined(__wasm_simd128__)
    // wasm SIMD is 128-bit: the f32x4 lowering covers YdspVectorizer's
    // portable width. The 8/16-lane widths are native-target only (AVX2/
    // AVX-512), so anything widened past f32x4 is rejected here.
    if (fn.vectorized && fn.vectorWidth > 4)
    {
        diagnostics.addError ({}, "The wasm backend lowers f32x4 SIMD only: this kernel was vectorized for width " + String (fn.vectorWidth));
        return {};
    }
#else
    // There is no `simd128` lowering in a build without -msimd128, so a
    // widened function would emit scalar code for packed values and silently
    // compute the wrong thing. The compiler only enables the vectoriser when
    // `__wasm_simd128__` is defined; this is the guard for anything that
    // reaches the wasm backend by another route.
    if (fn.vectorized)
    {
        diagnostics.addError ({}, "The wasm backend has no SIMD lowering without -msimd128: this kernel was vectorized for a native target");
        return {};
    }
#endif

    YdspWasmCodegenImpl impl;
    return impl.compile (fn, diagnostics);
}

String YdspWasmCodegen::toText (const std::vector<uint8_t>& module)
{
    return WasmTextRenderer::render (module);
}

} // namespace yup
