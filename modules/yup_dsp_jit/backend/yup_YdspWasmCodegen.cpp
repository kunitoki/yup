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
// ABI: byte offsets of the context structs as compiled on wasm32 (4-byte
// pointers). The wasm backend hardcodes these; the static asserts below verify
// them whenever the module itself is compiled for a wasm target.

constexpr int kernelInputsOff = 0;
constexpr int kernelOutputsOff = 4;
constexpr int kernelParamsOff = 8;
constexpr int kernelParamOutOff = 12;
constexpr int kernelStateOff = 16;
constexpr int kernelStateArraysOff = 20;
constexpr int kernelSampleRateOff = 24;
constexpr int kernelNumSamplesOff = 28;
constexpr int kernelOutputEventsOff = 32;

// Only the event context's header is hardcoded here: the payload fields are
// addressed by the byte offsets the IR carries (see ydspEventShapes), which
// offsetof() already resolves for the target ABI. The channel, sampleOffset
// and outputEvents constants exist solely to pin the byte offsets of the
// fields appended to the ABI structs.
constexpr int eventStateOff = 0;
constexpr int eventStateArraysOff = 4;
constexpr int eventParamsOff = 8;
constexpr int eventSampleRateOff = 12;
constexpr int eventChannelOff = 48;
constexpr int eventSampleOffsetOff = 52;
constexpr int eventOutputEventsOff = 56;

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

#if defined (__wasm_simd128__)

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

    bool operator< (const LibmSignature& other) const noexcept
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

    static void indentLine (String& result, const String& text, int indent)
    {
        for (int i = 0; i < indent; ++i)
            result += "  ";

        result += text;
        result += "\n";
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
#if defined (__wasm_simd128__)
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

#if defined (__wasm_simd128__)
    static String simdMnemonic (uint32_t subopcode)
    {
        switch (subopcode)
        {
            case YdspWasmEmitter::opV128Load:
                return "v128.load";
            case YdspWasmEmitter::opV128Store:
                return "v128.store";
            case YdspWasmEmitter::opI8x16Shuffle:
                return "i8x16.shuffle";
            case YdspWasmEmitter::opF32x4Splat:
                return "f32x4.splat";
            case YdspWasmEmitter::opF32x4ExtractLane:
                return "f32x4.extract_lane";
            case YdspWasmEmitter::opF32x4Ceil:
                return "f32x4.ceil";
            case YdspWasmEmitter::opF32x4Floor:
                return "f32x4.floor";
            case YdspWasmEmitter::opF32x4Nearest:
                return "f32x4.nearest";
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

        String result = "(module\n";

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
        return result;
    }

    // Renders one code-section body: the locals declaration followed by the
    // instruction listing (up to the trailing `end`, which is implicit).
    void renderBody (String& result, const Section& section, size_t& p)
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

#if defined (__wasm_simd128__)
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

                        case YdspWasmEmitter::opI8x16Shuffle:
                            line += " [" + String (static_cast<int> (readByte (p)));

                            for (int i = 1; i < 16; ++i)
                                line += " " + String (static_cast<int> (readByte (p)));

                            line += "]";
                            break;

                        case YdspWasmEmitter::opF32x4ExtractLane:
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
            case 0x4A:
                return "i32.gt_s";
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
            case 0x55:
                return "i64.gt_s";
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

        // Segment byte offsets (region order [f32][i32][f64][i64] in each), in
        // sync with YdspAsmJitCodegen::stateSize/stateScalarSize on the host side.
        const auto f32ScalarBytes = static_cast<int> (fn.float32Scalars * 4);
        const auto i32ScalarBytes = static_cast<int> (fn.int32Scalars * 4);
        const auto f64ScalarBytes = static_cast<int> (fn.float64Scalars * 8);

        int32ScalarOffset = f32ScalarBytes;
        float64ScalarOffset = int32ScalarOffset + i32ScalarBytes;
        int64ScalarOffset = float64ScalarOffset + f64ScalarBytes;

        const auto f32ArrayBytes = static_cast<int> (fn.float32ArrayElements * 4);
        const auto i32ArrayBytes = static_cast<int> (fn.int32ArrayElements * 4);
        const auto f64ArrayBytes = static_cast<int> (fn.float64ArrayElements * 8);

        int32ArrayOffset = f32ArrayBytes;
        float64ArrayOffset = int32ArrayOffset + i32ArrayBytes;
        int64ArrayOffset = float64ArrayOffset + f64ArrayBytes;

        // Cumulative byte offsets for the heterogeneous param/meter blocks.
        paramOffsets.clear();
        int paramCursor = 0;

        for (const auto type : fn.paramTypes)
        {
            paramOffsets.push_back (paramCursor);
            paramCursor += elementSizeBytes (type);
        }

        paramOutOffsets.clear();
        paramCursor = 0;

        for (const auto type : fn.paramOutTypes)
        {
            paramOutOffsets.push_back (paramCursor);
            paramCursor += elementSizeBytes (type);
        }

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
        emitter.u32 (static_cast<uint32_t> (1 + libmSignatures.size() + (usesEmitEvent ? 1 : 0)));

        kernelTypeIndex = emitter.funcType ({ ValType::i32 }, {});

        for (const auto& sig : libmSignatures)
            sigTypeIndex[sig] = emitter.funcType (sig.isBinary ? std::initializer_list<ValType> { sig.is64 ? ValType::f64 : ValType::f32,
                                                                                                  sig.is64 ? ValType::f64 : ValType::f32 }
                                                               : std::initializer_list<ValType> { sig.is64 ? ValType::f64 : ValType::f32 },
                                                  { sig.is64 ? ValType::f64 : ValType::f32 });

        if (usesEmitEvent)
            emitEventTypeIndex = emitter.funcType ({ ValType::i32, ValType::i32, ValType::i32, ValType::i32 }, {});

        emitter.endSection();

        // ---- import section: memory first, then the libm functions, then the
        // output-event commit function (if used) ----
        emitter.beginSection (YdspWasmEmitter::sectionImport);
        emitter.u32 (static_cast<uint32_t> (1 + libmNames.size() + (usesEmitEvent ? 1 : 0)));

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
        emitBlocks (0, static_cast<int> (fn.blocks.size()));

        if (! labels.empty())
        {
            diagnostics.addError (0, 0, "Wasm codegen: unbalanced control-flow structure in the IR");
            return {};
        }

        emitter.endBody();
        emitter.endSection();

        auto bytes = emitter.takeBytes();
        if (! bytes.empty())
            diagnostics.addInfo (0, 0, WasmTextRenderer::render (bytes));
        return bytes;
    }

private:
    //==============================================================================
    // Analysis: value types, max value id, and the used libm imports.

    bool analyze (const YdspIrFunction& fn)
    {
        int maxResult = -1;

        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts)
            {
                if (inst.result > maxResult)
                    maxResult = inst.result;

                if (inst.op == YdspIrOp::emitEvent)
                    usesEmitEvent = true;

#if defined (__wasm_simd128__)
                if (inst.op == YdspIrOp::vreduceAddF)
                    usesVreduce = true;
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

        if (maxResult < 0)
            return true; // no values; a valid (empty) kernel

        valueTypes.assign (static_cast<size_t> (maxResult) + 1, YdspValueType::boolType);

        if (fn.valueTypes.size() >= valueTypes.size())
        {
            valueTypes = fn.valueTypes;
        }
        else
        {
            for (const auto& block : fn.blocks)
                for (const auto& inst : block.insts)
                    if (inst.result >= 0)
                        valueTypes[static_cast<size_t> (inst.result)] = inst.op == YdspIrOp::selectB
                                                                          ? valueTypes[static_cast<size_t> (inst.b)]
                                                                          : inferTypeFromOp (inst.op);
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
#if defined (__wasm_simd128__)
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

#if defined (__wasm_simd128__)
        // One scratch v128 local for the horizontal-reduction fold, parked
        // right after the value locals (index valueTypes.size() + 1).
        if (usesVreduce)
            emitter.declareLocals (1, ValType::v128);
#endif
    }

    uint32_t localIndex (int valueId) const
    {
        return static_cast<uint32_t> (valueId) + 1;
    }

    void pushValue (int valueId)
    {
        emitter.localGet (localIndex (valueId));
    }

    void setValue (int valueId)
    {
        emitter.localSet (localIndex (valueId));
    }

    void pushCtxLoad (int fieldOffset, YdspValueType fieldType)
    {
        emitter.localGet (0);
        emitter.load (loadOpcode (fieldType), scaleLog2 (fieldType), static_cast<uint32_t> (fieldOffset));
    }

    // Pushes the effective address = ctxField(baseFieldOffset) + byteOffset.
    void pushAddress (int baseFieldOffset, int byteOffset)
    {
        pushCtxLoad (baseFieldOffset, YdspValueType::int32Type);
        emitter.i32Const (byteOffset);
        emitter.op (YdspWasmEmitter::opI32Add);
    }

    // Pushes the effective address = ctxField(baseFieldOffset) + arrayBaseOffset
    // + (indexValueId << scaleLog2(elementType)).
    void pushIndexedAddress (int baseFieldOffset, int arrayBaseOffset, int indexValueId, YdspValueType elementType)
    {
        pushCtxLoad (baseFieldOffset, YdspValueType::int32Type);
        emitter.i32Const (arrayBaseOffset);
        emitter.op (YdspWasmEmitter::opI32Add);
        pushValue (indexValueId);
        emitter.i32Const (static_cast<int32_t> (scaleLog2 (elementType)));
        emitter.op (YdspWasmEmitter::opI32Shl);
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
    // State layout (mirrors the native backend's scalar/array segment math).

    int stateScalarBase (YdspValueType type, int slot) const
    {
        switch (type)
        {
            case YdspValueType::float32Type:
                return slot * 4;
            case YdspValueType::float64Type:
                return float64ScalarOffset + slot * 8;
            case YdspValueType::int32Type:
                return int32ScalarOffset + slot * 4;
            case YdspValueType::int64Type:
                return int64ScalarOffset + slot * 8;
            default:
                return slot * 4;
        }
    }

    int stateArrayBase (YdspValueType type, int element) const
    {
        switch (type)
        {
            case YdspValueType::float32Type:
                return element * 4;
            case YdspValueType::float64Type:
                return float64ArrayOffset + element * 8;
            case YdspValueType::int32Type:
                return int32ArrayOffset + element * 4;
            case YdspValueType::int64Type:
                return int64ArrayOffset + element * 8;
            default:
                return element * 4;
        }
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

        diagnostics->addError (0, 0, "Wasm codegen: branch target " + String (targetBlock) + " is not reachable from a structured control-flow region");
    }

    void emitBlocks (int start, int end)
    {
        YdspRecursionGuard guard (recursionDepth);

        if (guard.exceeded())
        {
            diagnostics->addError (0, 0, "Wasm codegen: control flow nested too deeply");
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

            emitInstructions (block);

            switch (block.term)
            {
                case YdspIrTerm::fallthrough:
                    break;

                case YdspIrTerm::branch:
                    emitBranchTo (block.termTarget, false);
                    break;

                case YdspIrTerm::branchIf:
                {
                    // A conditional branch that is neither a loop header nor
                    // an if/else cond (then == block + 1) — the IR is not
                    // structured as expected.
                    String message = "Wasm codegen: unexpected conditional branch in the IR control flow (block " + String (i) + ": target="
                                   + String (block.termTarget) + ", target2=" + String (block.termTarget2) + ")";
                    diagnostics->addError (0, 0, message);
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

        emitInstructions (header);

        // Header terminator: branchIf(cond, body, exit) -> br_if exit. The
        // body immediately follows the header, so a false condition exits;
        // br_if branches on nonzero, so invert the condition first.
        if (header.term == YdspIrTerm::branchIf && header.termTarget == headerIndex + 1)
        {
            pushValue (header.termCond);
            emitter.op (YdspWasmEmitter::opI32Eqz);
            emitter.brIf (depthOf (exitLabel));
        }
        else
        {
            diagnostics->addError (0, 0, "Wasm codegen: a loop header must end with a conditional branch to the following block");
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
            diagnostics->addError (0, 0, "Wasm codegen: an if/else region is missing its else/join target");
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

        emitInstructions (cond);

        pushValue (cond.termCond);
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
    // Instruction lowering

    void emitInstructions (const YdspIrBlock& block)
    {
        for (const auto& inst : block.insts)
            emitInstruction (inst);
    }

    void emitInstruction (const YdspIrInst& inst)
    {
        switch (inst.op)
        {
            // ---- constants ----
            case YdspIrOp::constF:
            {
                const auto type = valueType (inst.result);
                loadFloatConst (inst.fvalue, type);
                setValue (inst.result);
                return;
            }

            case YdspIrOp::constI:
            {
                if (valueType (inst.result) == YdspValueType::int64Type)
                    emitter.i64Const (inst.ivalue);
                else
                    emitter.i32Const (static_cast<int32_t> (inst.ivalue));

                setValue (inst.result);
                return;
            }

            case YdspIrOp::constB:
                emitter.i32Const (inst.bvalue ? 1 : 0);
                setValue (inst.result);
                return;

            // ---- runtime values ----
            case YdspIrOp::loadBlockSize:
                pushCtxLoad (kernelNumSamplesOff, YdspValueType::int32Type);
                setValue (inst.result);
                return;

            case YdspIrOp::loadSampleRate:
                pushCtxLoad (sampleRateOff, YdspValueType::float32Type);
                setValue (inst.result);
                return;

            case YdspIrOp::loadEventFieldF:
                pushCtxLoad (inst.memIndex, YdspValueType::float32Type);
                setValue (inst.result);
                return;

            case YdspIrOp::loadEventFieldI:
                pushCtxLoad (inst.memIndex, YdspValueType::int32Type);
                setValue (inst.result);
                return;

            case YdspIrOp::storeEventFieldF:
            case YdspIrOp::storeEventFieldI:
            {
                const auto type = valueType (inst.a);
                pushAddress (outputEventsOff, inst.memIndex);
                pushValue (inst.a);
                emitter.store (storeOpcode (type), scaleLog2 (type), 0);
                return;
            }

            case YdspIrOp::emitEvent:
                pushCtxLoad (outputEventsOff, YdspValueType::int32Type);
                emitter.i32Const (static_cast<int32_t> (inst.ivalue));
                pushValue (inst.a);
                emitter.i32Const (inst.memIndex);
                emitter.call (static_cast<uint32_t> (emitEventFuncIndex));
                return;

            // ---- params and meters ----
            case YdspIrOp::loadParam:
            case YdspIrOp::loadParamOut:
            {
                const auto type = valueType (inst.result);
                const auto& offsets = inst.op == YdspIrOp::loadParam ? paramOffsets : paramOutOffsets;
                const auto fieldOff = inst.op == YdspIrOp::loadParam ? paramsOff : kernelParamOutOff;

                pushAddress (fieldOff, offsets[static_cast<size_t> (inst.a)]);
                emitter.load (loadOpcode (type), scaleLog2 (type), 0);
                setValue (inst.result);
                return;
            }

            case YdspIrOp::storeParam:
            case YdspIrOp::storeParamOut:
            {
                const auto type = valueType (inst.a);
                const auto& offsets = inst.op == YdspIrOp::storeParam ? paramOffsets : paramOutOffsets;
                const auto fieldOff = inst.op == YdspIrOp::storeParam ? paramsOff : kernelParamOutOff;

                pushAddress (fieldOff, offsets[static_cast<size_t> (inst.memIndex)]);
                pushValue (inst.a);
                emitter.store (storeOpcode (type), scaleLog2 (type), 0);
                return;
            }

            // ---- state scalars ----
            case YdspIrOp::loadStateF:
            case YdspIrOp::loadStateI:
            {
                const auto type = valueType (inst.result);
                pushAddress (stateOff, stateScalarBase (type, inst.a));
                emitter.load (loadOpcode (type), scaleLog2 (type), 0);
                setValue (inst.result);
                return;
            }

            case YdspIrOp::storeStateF:
            case YdspIrOp::storeStateI:
            {
                const auto type = valueType (inst.a);
                pushAddress (stateOff, stateScalarBase (type, inst.memIndex));
                pushValue (inst.a);
                emitter.store (storeOpcode (type), scaleLog2 (type), 0);
                return;
            }

            // ---- state arrays ----
            case YdspIrOp::loadStateArrayF:
            case YdspIrOp::loadStateArrayI:
            {
                const auto type = valueType (inst.result);
                pushIndexedAddress (stateArraysOff, stateArrayBase (type, inst.memIndex), inst.a, type);

#if defined (__wasm_simd128__)
                if (inst.op == YdspIrOp::loadStateArrayF && fn->laneCountOf (inst.result) > 1)
                    emitter.simdLoad (YdspWasmEmitter::opV128Load, 2, 0);
                else
#endif
                    emitter.load (loadOpcode (type), scaleLog2 (type), 0);

                setValue (inst.result);
                return;
            }

            case YdspIrOp::storeStateArrayF:
            case YdspIrOp::storeStateArrayI:
            {
                const auto type = valueType (inst.b);
                pushIndexedAddress (stateArraysOff, stateArrayBase (type, inst.memIndex), inst.a, type);
                pushValue (inst.b);

#if defined (__wasm_simd128__)
                if (inst.op == YdspIrOp::storeStateArrayF && fn->laneCountOf (inst.b) > 1)
                    emitter.simdStore (YdspWasmEmitter::opV128Store, 2, 0);
                else
#endif
                    emitter.store (storeOpcode (type), scaleLog2 (type), 0);

                return;
            }

            // ---- streams ----
            case YdspIrOp::loadInput:
            case YdspIrOp::loadOutput:
            {
                const auto type = valueType (inst.result);
                const auto fieldOff = inst.op == YdspIrOp::loadInput ? kernelInputsOff : kernelOutputsOff;

                // inputs/outputs is a pointer array; first load the channel
                // pointer inputs/outputs[memIndex], then index into the stream.
                pushCtxLoad (fieldOff, YdspValueType::int32Type);
                emitter.i32Const (inst.memIndex * 4);
                emitter.op (YdspWasmEmitter::opI32Add);
                emitter.load (YdspWasmEmitter::opI32Load, 2, 0);

                // address = channel + (sampleIndex << scale)
                pushValue (inst.a);
                emitter.i32Const (static_cast<int32_t> (scaleLog2 (type)));
                emitter.op (YdspWasmEmitter::opI32Shl);
                emitter.op (YdspWasmEmitter::opI32Add);

#if defined (__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                    emitter.simdLoad (YdspWasmEmitter::opV128Load, 2, 0);
                else
#endif
                    emitter.load (loadOpcode (type), scaleLog2 (type), 0);

                setValue (inst.result);
                return;
            }

            case YdspIrOp::storeOutput:
            {
                const auto type = valueType (inst.b);

                pushCtxLoad (kernelOutputsOff, YdspValueType::int32Type);
                emitter.i32Const (inst.memIndex * 4);
                emitter.op (YdspWasmEmitter::opI32Add);
                emitter.load (YdspWasmEmitter::opI32Load, 2, 0);

                pushValue (inst.a);
                emitter.i32Const (static_cast<int32_t> (scaleLog2 (type)));
                emitter.op (YdspWasmEmitter::opI32Shl);
                emitter.op (YdspWasmEmitter::opI32Add);

                pushValue (inst.b);

#if defined (__wasm_simd128__)
                if (fn->laneCountOf (inst.b) > 1)
                    emitter.simdStore (YdspWasmEmitter::opV128Store, 2, 0);
                else
#endif
                    emitter.store (storeOpcode (type), scaleLog2 (type), 0);

                return;
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

#if defined (__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (inst.a);
                    pushValue (inst.b);
                    emitter.simdOp (packedFloatBinarySimdOp (inst.op));
                    setValue (inst.result);
                    return;
                }
#endif

                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (floatBinaryOpcode (inst.op, is64BitValueType (type)));
                setValue (inst.result);
                return;
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
                setValue (inst.result);
                return;
            }

            case YdspIrOp::negF:
            case YdspIrOp::absF:
            case YdspIrOp::sqrtF:
            case YdspIrOp::floorF:
            case YdspIrOp::ceilF:
            case YdspIrOp::rintF:
            {
                const auto type = valueType (inst.result);

#if defined (__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    pushValue (inst.a);
                    emitter.simdOp (packedFloatUnarySimdOp (inst.op));
                    setValue (inst.result);
                    return;
                }
#endif

                pushValue (inst.a);
                emitter.op (floatUnaryOpcode (inst.op, is64BitValueType (type)));
                setValue (inst.result);
                return;
            }

            case YdspIrOp::clampF:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

#if defined (__wasm_simd128__)
                if (fn->laneCountOf (inst.result) > 1)
                {
                    // f32x4.max (a, b), then f32x4.min (.., c)
                    pushValue (inst.a);
                    pushValue (inst.b);
                    emitter.simdOp (YdspWasmEmitter::opF32x4Max);
                    pushValue (inst.c);
                    emitter.simdOp (YdspWasmEmitter::opF32x4Min);
                    setValue (inst.result);
                    return;
                }
#endif

                // min (max (a, b), c)
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (floatBinaryOpcode (YdspIrOp::maxF, is64));
                pushValue (inst.c);
                emitter.op (floatBinaryOpcode (YdspIrOp::minF, is64));
                setValue (inst.result);
                return;
            }

            case YdspIrOp::lerpF:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

#if defined (__wasm_simd128__)
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
                    setValue (inst.result);
                    return;
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
                setValue (inst.result);
                return;
            }

            // ---- int arithmetic ----
            case YdspIrOp::addI:
            case YdspIrOp::subI:
            case YdspIrOp::mulI:
            case YdspIrOp::andI:
            case YdspIrOp::orI:
            case YdspIrOp::xorI:
            {
                const auto type = valueType (inst.result);
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (intBinaryOpcode (inst.op, is64BitValueType (type)));
                setValue (inst.result);
                return;
            }

            case YdspIrOp::shlI:
            case YdspIrOp::shrI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                pushValue (inst.a);
                pushValue (inst.b);

                if (inst.op == YdspIrOp::shlI)
                    emitter.op (is64 ? YdspWasmEmitter::opI64Shl : YdspWasmEmitter::opI32Shl);
                else
                    emitter.op (is64 ? YdspWasmEmitter::opI64ShrS : YdspWasmEmitter::opI32ShrS);

                setValue (inst.result);
                return;
            }

            case YdspIrOp::negI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);

                pushValue (inst.a);
                emitter.op (is64 ? YdspWasmEmitter::opI64Sub : YdspWasmEmitter::opI32Sub);
                setValue (inst.result);
                return;
            }

            case YdspIrOp::divI:
            case YdspIrOp::modI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                // b == 0 ? 0 : (a op b)  (wasm div_s/rem_s trap on a zero divisor)
                pushValue (inst.b);
                emitter.op (is64 ? YdspWasmEmitter::opI64Eqz : YdspWasmEmitter::opI32Eqz);
                emitter.ifValue (is64 ? ValType::i64 : ValType::i32);

                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);

                emitter.else_();
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (intDivRemOpcode (inst.op, is64));
                emitter.end();

                setValue (inst.result);
                return;
            }

            case YdspIrOp::wrapI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

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
                setValue (inst.result);
                return;
            }

            case YdspIrOp::advanceWrapI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);
                pushValue (inst.a);
                if (is64)
                    emitter.i64Const (1);
                else
                    emitter.i32Const (1);
                emitter.op (is64 ? YdspWasmEmitter::opI64Add : YdspWasmEmitter::opI32Add);
                if (is64)
                    emitter.i64Const (inst.ivalue);
                else
                    emitter.i32Const (static_cast<int32_t> (inst.ivalue));
                emitter.op (is64 ? YdspWasmEmitter::opI64LtS : YdspWasmEmitter::opI32LtS);
                emitter.ifValue (is64 ? ValType::i64 : ValType::i32);
                pushValue (inst.a);
                if (is64)
                    emitter.i64Const (1);
                else
                    emitter.i32Const (1);
                emitter.op (is64 ? YdspWasmEmitter::opI64Add : YdspWasmEmitter::opI32Add);
                emitter.else_();
                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);
                emitter.end();
                setValue (inst.result);
                return;
            }

            case YdspIrOp::minI:
            case YdspIrOp::maxI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                pushValue (inst.a);
                pushValue (inst.b);

                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (intCompareOpcode (inst.op == YdspIrOp::minI ? YdspIrOp::ltI : YdspIrOp::gtI, is64));

                emitter.select();
                setValue (inst.result);
                return;
            }

            case YdspIrOp::clampI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                pushValue (inst.a);
                pushValue (inst.b);
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (intCompareOpcode (YdspIrOp::gtI, is64));
                emitter.select();

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
                setValue (inst.result);
                return;
            }

            case YdspIrOp::absI:
            {
                const auto type = valueType (inst.result);
                const bool is64 = is64BitValueType (type);

                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);
                pushValue (inst.a);
                emitter.op (is64 ? YdspWasmEmitter::opI64Sub : YdspWasmEmitter::opI32Sub);

                pushValue (inst.a);

                pushValue (inst.a);
                if (is64)
                    emitter.i64Const (0);
                else
                    emitter.i32Const (0);
                emitter.op (intCompareOpcode (YdspIrOp::ltI, is64));

                emitter.select();
                setValue (inst.result);
                return;
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
                setValue (inst.result);
                return;
            }

            // ---- conversions ----
            case YdspIrOp::itof:
            case YdspIrOp::ftoi:
            {
                const auto src = valueType (inst.a);
                const auto dst = valueType (inst.result);
                pushValue (inst.a);
                emitter.op (convertOpcode (inst.op, src, dst));
                setValue (inst.result);
                return;
            }

            case YdspIrOp::extI:
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opI64ExtendI32S);
                setValue (inst.result);
                return;

            case YdspIrOp::truncI:
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opI32WrapI64);
                setValue (inst.result);
                return;

            case YdspIrOp::extF:
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opF64PromoteF32);
                setValue (inst.result);
                return;

            case YdspIrOp::truncF:
                pushValue (inst.a);
                emitter.op (YdspWasmEmitter::opF32DemoteF64);
                setValue (inst.result);
                return;

            // ---- comparisons ----
            case YdspIrOp::eqF:
            case YdspIrOp::neF:
            case YdspIrOp::ltF:
            case YdspIrOp::leF:
            case YdspIrOp::gtF:
            case YdspIrOp::geF:
            {
                const auto type = valueType (inst.a);
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (floatCompareOpcode (inst.op, is64BitValueType (type)));
                setValue (inst.result);
                return;
            }

            case YdspIrOp::eqI:
            case YdspIrOp::neI:
            case YdspIrOp::ltI:
            case YdspIrOp::leI:
            case YdspIrOp::gtI:
            case YdspIrOp::geI:
            {
                const auto type = valueType (inst.a);
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (intCompareOpcode (inst.op, is64BitValueType (type)));
                setValue (inst.result);
                return;
            }

            // ---- logic ----
            case YdspIrOp::andB:
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (YdspWasmEmitter::opI32And);
                setValue (inst.result);
                return;

            case YdspIrOp::orB:
                pushValue (inst.a);
                pushValue (inst.b);
                emitter.op (YdspWasmEmitter::opI32Or);
                setValue (inst.result);
                return;

            case YdspIrOp::notB:
                pushValue (inst.a);
                emitter.i32Const (1);
                emitter.op (YdspWasmEmitter::opI32Xor);
                setValue (inst.result);
                return;

            // ---- moves ----
            case YdspIrOp::movF:
            case YdspIrOp::movI:
            case YdspIrOp::movB:
                pushValue (inst.a);
                setValue (inst.result);
                return;

            // ---- select ----
            case YdspIrOp::selectB:
                pushValue (inst.b);
                pushValue (inst.c);
                pushValue (inst.a);
                emitter.select();
                setValue (inst.result);
                return;

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
                setValue (inst.result);
                return;
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
                setValue (inst.result);
                return;
            }

#if defined (__wasm_simd128__)
            // ---- lane movement (SIMD) ----
            case YdspIrOp::vsplat:
                pushValue (inst.a);
                emitter.simdOp (YdspWasmEmitter::opF32x4Splat);
                setValue (inst.result);
                return;

            case YdspIrOp::vreduceAddF:
            {
                // Horizontal f32x4 sum as a shuffle tree. i8x16.shuffle and
                // f32x4.add are binary, so the operand is re-pushed before
                // each of them: shuffle [2,3,0,1] over two copies of a, add a
                // back in, park the partials in the scratch v128 local, then
                // shuffle [1,0,3,2] over two copies of the partials and add
                // one copy back. Lane j of the accumulator already holds the
                // reassociated partial sum (see YdspVectorizer), so the
                // grouping (l0 + l2) + (l1 + l3) is exactly the tree the
                // widened reduction's semantics promise.
                static const uint8_t swap64Mask[16] = { 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 };
                static const uint8_t swap32Mask[16] = { 4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11 };

                const auto scratch = static_cast<uint32_t> (valueTypes.size()) + 1u;

                pushValue (inst.a);
                pushValue (inst.a);
                emitter.i8x16Shuffle (swap64Mask);
                pushValue (inst.a);
                emitter.simdOp (YdspWasmEmitter::opF32x4Add);
                emitter.localSet (scratch);
                emitter.localGet (scratch);
                emitter.localGet (scratch);
                emitter.i8x16Shuffle (swap32Mask);
                emitter.localGet (scratch);
                emitter.simdOp (YdspWasmEmitter::opF32x4Add);
                emitter.f32x4ExtractLane (0);
                setValue (inst.result);
                return;
            }
#endif

            default:
                diagnostics->addError (0, 0, "Wasm codegen: unsupported instruction in the IR");
                return;
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

    // Segment byte offsets (region order [f32][i32][f64][i64] in each).
    int int32ScalarOffset = 0;
    int float64ScalarOffset = 0;
    int int64ScalarOffset = 0;
    int int32ArrayOffset = 0;
    int float64ArrayOffset = 0;
    int int64ArrayOffset = 0;

    std::vector<int> paramOffsets;    // byte offset per param slot (heterogeneous)
    std::vector<int> paramOutOffsets; // byte offset per meter slot (heterogeneous)

    std::vector<YdspValueType> valueTypes;

    // libm imports, deterministic order (sorted by name).
    std::set<std::string> libmNames;
    std::vector<LibmSignature> libmSignatures;
    std::map<LibmSignature, int> sigTypeIndex;
    std::map<std::string, int> libmFuncIndex;

    // The output-event commit import, only added when the function uses it.
    bool usesEmitEvent = false;
    int emitEventTypeIndex = 0;
    int emitEventFuncIndex = 0;

#if defined (__wasm_simd128__)
    // Set when the function folds a widened reduction (vreduceAddF), which
    // needs the scratch v128 local declared after the value locals.
    bool usesVreduce = false;
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
#if defined (__wasm_simd128__)
    // wasm SIMD is 128-bit: the f32x4 lowering covers YdspVectorizer's
    // portable width. The 8/16-lane widths are native-target only (AVX2/
    // AVX-512), so anything widened past f32x4 is rejected here.
    if (fn.vectorized && fn.vectorWidth > 4)
    {
        diagnostics.addError (0, 0, "The wasm backend lowers f32x4 SIMD only: this kernel was vectorized for width " + String (fn.vectorWidth));
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
        diagnostics.addError (0, 0, "The wasm backend has no SIMD lowering without -msimd128: this kernel was vectorized for a native target");
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
