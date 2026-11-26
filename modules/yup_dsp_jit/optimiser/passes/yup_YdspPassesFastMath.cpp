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

bool isFastMathLowerableOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::tanhF:
        case YdspIrOp::sinF:
        case YdspIrOp::cosF:
        case YdspIrOp::tanF:
        case YdspIrOp::atanF:
        case YdspIrOp::atan2F:
        case YdspIrOp::expF:
        case YdspIrOp::logF:
        case YdspIrOp::log10F:
        case YdspIrOp::powF:
            return true;

        default:
            return false;
    }
}

class FastMathBuilder
{
public:
    FastMathBuilder (YdspIrFunction& functionToLower, std::vector<YdspIrInst>& outInsts, std::vector<YdspIrInst>& constOutInsts)
        : fn (functionToLower)
        , out (outInsts)
        , constOut (constOutInsts)
    {
    }

    int newValue (YdspValueType type, int lanes)
    {
        const auto id = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (type);

        if (! fn.valueLanes.empty())
        {
            fn.valueLanes.push_back (lanes);
        }
        else if (lanes > 1)
        {
            fn.valueLanes.resize (static_cast<size_t> (id), 1);
            fn.valueLanes.push_back (lanes);
        }

        return id;
    }

    int ensureLanes (int value, int lanes)
    {
        if (lanes <= 1 || fn.laneCountOf (value) == lanes)
            return value;

        const auto type = value < 0 || static_cast<size_t> (value) >= fn.valueTypes.size()
                            ? YdspValueType::float32Type
                            : fn.valueTypes[static_cast<size_t> (value)];
        const auto splatted = newValue (type, lanes);
        out.push_back ({ YdspIrOp::vsplat, splatted, value });
        return splatted;
    }

    int constF (float value, int lanes)
    {
        const auto bits = std::bit_cast<uint32_t> (value);
        int scalarId = -1;

        if (const auto it = scalarFloatConsts.find (bits); it != scalarFloatConsts.end())
        {
            scalarId = it->second;
        }
        else
        {
            scalarId = newValue (YdspValueType::float32Type, 1);
            constOut.push_back ({ YdspIrOp::constF, scalarId, -1, -1, -1, -1, static_cast<double> (value) });
            scalarFloatConsts.emplace (bits, scalarId);
        }

        if (lanes <= 1)
            return scalarId;

        const auto vecKey = (static_cast<uint64_t> (bits) << 16) | static_cast<uint64_t> (lanes);

        if (const auto it = vectorFloatConsts.find (vecKey); it != vectorFloatConsts.end())
            return it->second;

        const auto vecId = newValue (YdspValueType::float32Type, lanes);
        constOut.push_back ({ YdspIrOp::vsplat, vecId, scalarId });
        vectorFloatConsts.emplace (vecKey, vecId);
        return vecId;
    }

    int scalarConstI (int32_t value)
    {
        if (const auto it = scalarIntConsts.find (value); it != scalarIntConsts.end())
            return it->second;

        const auto scalarId = newValue (YdspValueType::int32Type, 1);
        constOut.push_back ({ YdspIrOp::constI, scalarId, -1, -1, -1, -1, 0.0, static_cast<int64_t> (value) });
        scalarIntConsts.emplace (value, scalarId);
        return scalarId;
    }

    int constI (int32_t value, int lanes)
    {
        const auto scalarId = scalarConstI (value);

        if (lanes <= 1)
            return scalarId;

        const auto vecKey = (static_cast<uint64_t> (static_cast<uint32_t> (value)) << 16) | static_cast<uint64_t> (lanes);

        if (const auto it = vectorIntConsts.find (vecKey); it != vectorIntConsts.end())
            return it->second;

        const auto vecId = newValue (YdspValueType::int32Type, lanes);
        constOut.push_back ({ YdspIrOp::vsplat, vecId, scalarId });
        vectorIntConsts.emplace (vecKey, vecId);
        return vecId;
    }

    int emitUnary (YdspIrOp op, YdspValueType type, int a, int lanes, int dst = -1)
    {
        const auto id = dst >= 0 ? dst : newValue (type, lanes);
        out.push_back ({ op, id, a });
        return id;
    }

    int emitBinary (YdspIrOp op, YdspValueType type, int a, int b, int lanes, int dst = -1)
    {
        const auto id = dst >= 0 ? dst : newValue (type, lanes);
        out.push_back ({ op, id, a, b });
        return id;
    }

    int emitTernary (YdspIrOp op, YdspValueType type, int a, int b, int c, int lanes, int dst = -1)
    {
        const auto id = dst >= 0 ? dst : newValue (type, lanes);
        out.push_back ({ op, id, a, b, c });
        return id;
    }

    // Shift amounts stay scalar (lanes == 1) so vector backends take their
    // packed-by-scalar shift path.
    int emitShift (YdspIrOp op, int a, int32_t shiftAmount, int lanes)
    {
        const auto id = newValue (YdspValueType::int32Type, lanes);
        out.push_back ({ op, id, a, scalarConstI (shiftAmount) });
        return id;
    }

    int emitFtoi (int a, int lanes, bool bounded)
    {
        const auto id = newValue (YdspValueType::int32Type, lanes);
        YdspIrInst inst { YdspIrOp::ftoi, id, a };
        inst.boundedFloatToInt = bounded;
        out.push_back (inst);
        return id;
    }

    int emitItof (int a, int lanes) { return emitUnary (YdspIrOp::itof, YdspValueType::float32Type, a, lanes); }
    int emitBitcastFtoI (int a, int lanes) { return emitUnary (YdspIrOp::bitcastFtoI, YdspValueType::int32Type, a, lanes); }
    int emitBitcastItoF (int a, int lanes, int dst = -1) { return emitUnary (YdspIrOp::bitcastItoF, YdspValueType::float32Type, a, lanes, dst); }

    int addF (int a, int b, int lanes, int dst = -1) { return emitBinary (YdspIrOp::addF, YdspValueType::float32Type, a, b, lanes, dst); }
    int subF (int a, int b, int lanes, int dst = -1) { return emitBinary (YdspIrOp::subF, YdspValueType::float32Type, a, b, lanes, dst); }
    int mulF (int a, int b, int lanes, int dst = -1) { return emitBinary (YdspIrOp::mulF, YdspValueType::float32Type, a, b, lanes, dst); }
    int divF (int a, int b, int lanes, int dst = -1) { return emitBinary (YdspIrOp::divF, YdspValueType::float32Type, a, b, lanes, dst); }
    int maxF (int a, int b, int lanes) { return emitBinary (YdspIrOp::maxF, YdspValueType::float32Type, a, b, lanes); }
    int minF (int a, int b, int lanes) { return emitBinary (YdspIrOp::minF, YdspValueType::float32Type, a, b, lanes); }
    int absF (int a, int lanes) { return emitUnary (YdspIrOp::absF, YdspValueType::float32Type, a, lanes); }
    int rintF (int a, int lanes) { return emitUnary (YdspIrOp::rintF, YdspValueType::float32Type, a, lanes); }
    int clampF (int a, int lo, int hi, int lanes, int dst = -1) { return emitTernary (YdspIrOp::clampF, YdspValueType::float32Type, a, lo, hi, lanes, dst); }

    int cmpF (YdspIrOp op, int a, int b, int lanes) { return emitBinary (op, YdspValueType::boolType, a, b, lanes); }
    int cmpI (YdspIrOp op, int a, int b, int lanes) { return emitBinary (op, YdspValueType::boolType, a, b, lanes); }
    int andB (int a, int b, int lanes) { return emitBinary (YdspIrOp::andB, YdspValueType::boolType, a, b, lanes); }
    int selectF (int cond, int whenTrue, int whenFalse, int lanes, int dst = -1) { return emitTernary (YdspIrOp::selectB, YdspValueType::float32Type, cond, whenTrue, whenFalse, lanes, dst); }

    int addI (int a, int b, int lanes) { return emitBinary (YdspIrOp::addI, YdspValueType::int32Type, a, b, lanes); }
    int subI (int a, int b, int lanes) { return emitBinary (YdspIrOp::subI, YdspValueType::int32Type, a, b, lanes); }
    int andI (int a, int b, int lanes) { return emitBinary (YdspIrOp::andI, YdspValueType::int32Type, a, b, lanes); }
    int orI (int a, int b, int lanes) { return emitBinary (YdspIrOp::orI, YdspValueType::int32Type, a, b, lanes); }
    int xorI (int a, int b, int lanes) { return emitBinary (YdspIrOp::xorI, YdspValueType::int32Type, a, b, lanes); }

    //==============================================================================

    void lowerTanh (int x, int lanes, int dst)
    {
        // Hybrid <= 1 ULP across float32:
        //   |x| <= 0.625: ax - (ax * x^2) * P(x^2) / Q(x^2) (Pade [7/6])
        //   |x| >  0.625: 1 - 2 / (exp(2 * min(|x|, 9.5)) + 1)
        // Sign bit is reapplied at the end so -0.0 and NaN propagate cleanly.
        const auto ax = absF (x, lanes);
        const auto x2 = mulF (x, x, lanes);

        auto p = addF (constF (2772.0f, lanes), mulF (constF (27.0f, lanes), x2, lanes), lanes);
        p = addF (constF (45045.0f, lanes), mulF (x2, p, lanes), lanes);

        auto q = addF (constF (3150.0f, lanes), mulF (constF (28.0f, lanes), x2, lanes), lanes);
        q = addF (constF (62370.0f, lanes), mulF (x2, q, lanes), lanes);
        q = addF (constF (135135.0f, lanes), mulF (x2, q, lanes), lanes);

        const auto corr = mulF (mulF (ax, x2, lanes), divF (p, q, lanes), lanes);
        const auto smallMag = subF (ax, corr, lanes);

        const auto useExp = cmpF (YdspIrOp::gtF, ax, constF (0.625f, lanes), lanes);
        const auto clampedAx = minF (ax, constF (9.5f, lanes), lanes);
        const auto exp2x = emitExpCore (mulF (constF (2.0f, lanes), clampedAx, lanes), lanes);
        const auto one = constF (1.0f, lanes);
        const auto largeMag = subF (one, divF (constF (2.0f, lanes), addF (exp2x, one, lanes), lanes), lanes);

        const auto mag = selectF (useExp, largeMag, smallMag, lanes);
        const auto signMask = constI (static_cast<int32_t> (0x80000000u), lanes);
        const auto signBit = andI (emitBitcastFtoI (x, lanes), signMask, lanes);
        emitBitcastItoF (orI (emitBitcastFtoI (mag, lanes), signBit, lanes), lanes, dst);
    }

    void lowerSinCos (int x, bool isCos, int lanes, int dst, bool highPrecision = false)
    {
        // Quadrant reduction modulo pi/2 using 3-part Cody-Waite constants
        // (12-bit hi/mid are exact products up to |k| <= 4096).
        const auto k = rintF (mulF (x, constF (0.63661977236758134308f, lanes), lanes), lanes);

        auto r = subF (x, mulF (k, constF (1.5703125f, lanes), lanes), lanes);
        r = subF (r, mulF (k, constF (4.837512969970703e-4f, lanes), lanes), lanes);
        r = subF (r, mulF (k, constF (7.549789948768648e-8f, lanes), lanes), lanes);

        const auto r2 = mulF (r, r, lanes);
        const auto r3 = mulF (r, r2, lanes);
        const auto r4 = mulF (r2, r2, lanes);

        int sinPoly = -1;
        int cosPoly = -1;

        if (highPrecision)
        {
            // Degree-11 sin / degree-10 cos gives <= 1 ULP across |x| <= 128.
            auto sinTail = addF (constF (2.755731922398589e-6f, lanes),
                                 mulF (r2, constF (-2.505210838544172e-8f, lanes), lanes),
                                 lanes);
            sinTail = addF (constF (-1.984126984126984e-4f, lanes), mulF (r2, sinTail, lanes), lanes);
            sinTail = addF (constF (0.008333333333333333f, lanes), mulF (r2, sinTail, lanes), lanes);
            sinTail = addF (constF (-0.16666666666666666f, lanes), mulF (r2, sinTail, lanes), lanes);
            sinPoly = addF (r, mulF (r3, sinTail, lanes), lanes);

            auto cosTail = addF (constF (2.48015873015873e-5f, lanes),
                                 mulF (r2, constF (-2.755731922398589e-7f, lanes), lanes),
                                 lanes);
            cosTail = addF (constF (-0.001388888888888889f, lanes), mulF (r2, cosTail, lanes), lanes);
            cosTail = addF (constF (0.041666666666666664f, lanes), mulF (r2, cosTail, lanes), lanes);
            const auto cosBase = subF (constF (1.0f, lanes), mulF (constF (0.5f, lanes), r2, lanes), lanes);
            cosPoly = addF (cosBase, mulF (r4, cosTail, lanes), lanes);
        }
        else
        {
            auto sinTail = addF (constF (0.008331988006830215f, lanes),
                                 mulF (r2, constF (-0.0001949660509126261f, lanes), lanes),
                                 lanes);
            sinTail = addF (constF (-0.16666650772094727f, lanes), mulF (r2, sinTail, lanes), lanes);
            sinPoly = addF (r, mulF (r3, sinTail, lanes), lanes);

            auto cosTail = addF (constF (-0.0013887365348637104f, lanes),
                                 mulF (r2, constF (2.4438231776002795e-5f, lanes), lanes),
                                 lanes);
            cosTail = addF (constF (0.04166664555668831f, lanes), mulF (r2, cosTail, lanes), lanes);
            const auto cosBase = subF (constF (1.0f, lanes), mulF (constF (0.5f, lanes), r2, lanes), lanes);
            cosPoly = addF (cosBase, mulF (r4, cosTail, lanes), lanes);
        }

        const auto ki = emitFtoi (k, lanes, false);
        const auto q = isCos ? addI (ki, constI (1, lanes), lanes) : ki;

        const auto qBit0 = andI (q, constI (1, lanes), lanes);
        const auto useCos = cmpI (YdspIrOp::neI, qBit0, constI (0, lanes), lanes);
        const auto mag = selectF (useCos, cosPoly, sinPoly, lanes);

        const auto qBit1 = andI (q, constI (2, lanes), lanes);
        const auto signBit = emitShift (YdspIrOp::shlI, qBit1, 30, lanes);
        const auto magBits = emitBitcastFtoI (mag, lanes);
        const auto resBits = xorI (magBits, signBit, lanes);
        emitBitcastItoF (resBits, lanes, dst);
    }

    void lowerTan (int x, int lanes, int dst)
    {
        // Quadrant reduction modulo pi/2 using the 3-part Cody-Waite constants.
        const auto k = rintF (mulF (x, constF (0.63661977236758134308f, lanes), lanes), lanes);

        auto r = subF (x, mulF (k, constF (1.5703125f, lanes), lanes), lanes);
        r = subF (r, mulF (k, constF (4.837512969970703e-4f, lanes), lanes), lanes);
        r = subF (r, mulF (k, constF (7.549789948768648e-8f, lanes), lanes), lanes);

        const auto r2 = mulF (r, r, lanes);
        const auto r3 = mulF (r, r2, lanes);

        auto acc = addF (constF (0.005091926082968712f, lanes),
                         mulF (r2, constF (0.008254697546362877f, lanes), lanes),
                         lanes);
        acc = addF (constF (0.02316485531628132f, lanes), mulF (r2, acc, lanes), lanes);
        acc = addF (constF (0.053773827850818634f, lanes), mulF (r2, acc, lanes), lanes);
        acc = addF (constF (0.13334380090236664f, lanes), mulF (r2, acc, lanes), lanes);
        acc = addF (constF (0.33333325386047363f, lanes), mulF (r2, acc, lanes), lanes);
        const auto t = addF (r, mulF (r3, acc, lanes), lanes);

        const auto ki = emitFtoi (k, lanes, false);
        const auto qBit0 = andI (ki, constI (1, lanes), lanes);
        const auto isOdd = cmpI (YdspIrOp::neI, qBit0, constI (0, lanes), lanes);
        const auto negInv = divF (constF (-1.0f, lanes), t, lanes);
        selectF (isOdd, negInv, t, lanes, dst);
    }

    int emitAtanPoly (int z, int lanes)
    {
        // Minimax odd degree-13 polynomial for atan(z) on [0, 1] (< 5e-7 error).
        const auto z2 = mulF (z, z, lanes);
        const auto z3 = mulF (z, z2, lanes);

        auto acc = addF (constF (-0.03528965637087822f, lanes),
                         mulF (z2, constF (0.007298804819583893f, lanes), lanes),
                         lanes);
        acc = addF (constF (0.08190269768238068f, lanes), mulF (z2, acc, lanes), lanes);
        acc = addF (constF (-0.13384714722633362f, lanes), mulF (z2, acc, lanes), lanes);
        acc = addF (constF (0.19858497381210327f, lanes), mulF (z2, acc, lanes), lanes);
        acc = addF (constF (-0.3332512080669403f, lanes), mulF (z2, acc, lanes), lanes);

        return addF (z, mulF (z3, acc, lanes), lanes);
    }

    void lowerAtan (int x, int lanes, int dst)
    {
        const auto ax = absF (x, lanes);
        const auto one = constF (1.0f, lanes);
        const auto gt1 = cmpF (YdspIrOp::gtF, ax, one, lanes);
        const auto z = selectF (gt1, divF (one, ax, lanes), ax, lanes);
        const auto poly = emitAtanPoly (z, lanes);
        const auto mag = selectF (gt1, subF (constF (1.5707963267948966f, lanes), poly, lanes), poly, lanes);

        const auto signMask = constI (static_cast<int32_t> (0x80000000u), lanes);
        const auto signBit = andI (emitBitcastFtoI (x, lanes), signMask, lanes);
        const auto resBits = orI (emitBitcastFtoI (mag, lanes), signBit, lanes);
        emitBitcastItoF (resBits, lanes, dst);
    }

    void lowerAtan2 (int y, int x, int lanes, int dst)
    {
        const auto ax = absF (x, lanes);
        const auto ay = absF (y, lanes);
        const auto mx = maxF (ax, ay, lanes);
        const auto mn = minF (ax, ay, lanes);
        const auto safeMx = maxF (mx, constF (1.17549435e-38f, lanes), lanes);
        const auto ratio = divF (mn, safeMx, lanes);
        const auto poly = emitAtanPoly (ratio, lanes);

        const auto ayGtAx = cmpF (YdspIrOp::gtF, ay, ax, lanes);
        auto angle = selectF (ayGtAx, subF (constF (1.5707963267948966f, lanes), poly, lanes), poly, lanes);

        const auto zero = constF (0.0f, lanes);
        const auto xLt0 = cmpF (YdspIrOp::ltF, x, zero, lanes);
        angle = selectF (xLt0, subF (constF (3.1415926535897932f, lanes), angle, lanes), angle, lanes);

        const auto signMask = constI (static_cast<int32_t> (0x80000000u), lanes);
        const auto signBit = andI (emitBitcastFtoI (y, lanes), signMask, lanes);
        const auto resBits = orI (emitBitcastFtoI (angle, lanes), signBit, lanes);
        emitBitcastItoF (resBits, lanes, dst);
    }

    int emitExpCore (int x, int lanes, int dst = -1)
    {
        // k clamped to [-126, 127] before ftoi so the integer shift stays in normal range.
        const auto rawK = rintF (mulF (x, constF (1.4426950408889634f, lanes), lanes), lanes);
        const auto k = clampF (rawK, constF (-126.0f, lanes), constF (127.0f, lanes), lanes);

        auto r = subF (x, mulF (k, constF (0.693115234375f, lanes), lanes), lanes);
        r = subF (r, mulF (k, constF (3.193318843841553e-5f, lanes), lanes), lanes);
        r = subF (r, mulF (k, constF (1.2996506981721723e-8f, lanes), lanes), lanes);

        const auto r2 = mulF (r, r, lanes);
        const auto r4 = mulF (r2, r2, lanes);
        auto tail = addF (constF (0.0013888888889f, lanes),
                          mulF (r, constF (0.0001984127f, lanes), lanes),
                          lanes);
        tail = addF (constF (0.0083333333333f, lanes), mulF (r, tail, lanes), lanes);
        tail = addF (constF (0.04166666667f, lanes), mulF (r, tail, lanes), lanes);
        tail = addF (constF (0.16666666667f, lanes), mulF (r, tail, lanes), lanes);
        tail = addF (constF (0.5f, lanes), mulF (r, tail, lanes), lanes);
        (void) r4;

        const auto poly = addF (constF (1.0f, lanes), addF (r, mulF (r2, tail, lanes), lanes), lanes);

        const auto ki = emitFtoi (k, lanes, true);
        const auto biased = addI (ki, constI (127, lanes), lanes);
        const auto pow2Bits = emitShift (YdspIrOp::shlI, biased, 23, lanes);
        const auto pow2 = emitBitcastItoF (pow2Bits, lanes);

        auto res = mulF (poly, pow2, lanes);
        const auto underflow = cmpF (YdspIrOp::ltF, x, constF (-87.336548f, lanes), lanes);
        res = selectF (underflow, constF (0.0f, lanes), res, lanes);

        const auto overflow = cmpF (YdspIrOp::gtF, x, constF (88.722839f, lanes), lanes);
        return selectF (overflow, constF (std::numeric_limits<float>::infinity(), lanes), res, lanes, dst);
    }

    int emitLogCore (int x, int lanes)
    {
        // Clamp to FLT_MIN so the sign bit is 0 and shrI acts as logical shift.
        const auto safeX = maxF (x, constF (1.17549435e-38f, lanes), lanes);
        const auto bits = emitBitcastFtoI (safeX, lanes);

        const auto expBiased = emitShift (YdspIrOp::shrI, bits, 23, lanes);
        const auto expInt = subI (expBiased, constI (127, lanes), lanes);
        const auto expFloat = emitItof (expInt, lanes);

        const auto mantMasked = andI (bits, constI (0x007fffff, lanes), lanes);
        const auto mantBits = orI (mantMasked, constI (0x3f800000, lanes), lanes);
        const auto m = emitBitcastItoF (mantBits, lanes);

        const auto needAdjust = cmpF (YdspIrOp::gtF, m, constF (1.4142135623730951f, lanes), lanes);
        const auto mAdj = selectF (needAdjust, mulF (m, constF (0.5f, lanes), lanes), m, lanes);
        const auto eAdj = selectF (needAdjust, addF (expFloat, constF (1.0f, lanes), lanes), expFloat, lanes);

        const auto one = constF (1.0f, lanes);
        const auto s = divF (subF (mAdj, one, lanes), addF (mAdj, one, lanes), lanes);
        const auto s2 = mulF (s, s, lanes);
        const auto s3 = mulF (s, s2, lanes);

        auto tail = addF (constF (0.39973825216293335f, lanes),
                          mulF (s2, constF (0.29956185817718506f, lanes), lanes),
                          lanes);
        tail = addF (constF (0.6666681170463562f, lanes), mulF (s2, tail, lanes), lanes);

        const auto lm = addF (mulF (constF (2.0f, lanes), s, lanes), mulF (s3, tail, lanes), lanes);
        const auto eHi = mulF (eAdj, constF (0.693115234375f, lanes), lanes);
        const auto eLo = mulF (eAdj, constF (3.1946184426080435e-5f, lanes), lanes);
        return addF (eHi, addF (eLo, lm, lanes), lanes);
    }

    void lowerLog (int x, bool base10, int lanes, int dst)
    {
        auto logVal = emitLogCore (x, lanes);

        if (base10)
            logVal = mulF (logVal, constF (0.4342944819032518f, lanes), lanes);

        const auto zero = constF (0.0f, lanes);
        const auto isZero = cmpF (YdspIrOp::eqF, x, zero, lanes);
        const auto isNeg = cmpF (YdspIrOp::ltF, x, zero, lanes);

        const auto zeroHandled = selectF (isZero, constF (-std::numeric_limits<float>::infinity(), lanes), logVal, lanes);
        selectF (isNeg, constF (std::numeric_limits<float>::quiet_NaN(), lanes), zeroHandled, lanes, dst);
    }

    void lowerPow (int a, int b, int lanes, int dst)
    {
        const auto logA = emitLogCore (a, lanes);
        const auto prod = mulF (b, logA, lanes);
        const auto expVal = emitExpCore (prod, lanes);

        const auto zero = constF (0.0f, lanes);
        const auto aZero = cmpF (YdspIrOp::eqF, a, zero, lanes);
        const auto bPos = cmpF (YdspIrOp::gtF, b, zero, lanes);
        const auto bNeg = cmpF (YdspIrOp::ltF, b, zero, lanes);
        const auto bZero = cmpF (YdspIrOp::eqF, b, zero, lanes);
        const auto aNeg = cmpF (YdspIrOp::ltF, a, zero, lanes);

        auto res = selectF (andB (aZero, bPos, lanes), zero, expVal, lanes);
        res = selectF (andB (aZero, bNeg, lanes), constF (std::numeric_limits<float>::infinity(), lanes), res, lanes);

        const auto negBase = selectF (bZero, constF (1.0f, lanes), constF (std::numeric_limits<float>::quiet_NaN(), lanes), lanes);
        selectF (aNeg, negBase, res, lanes, dst);
    }

private:
    YdspIrFunction& fn;
    std::vector<YdspIrInst>& out;
    std::vector<YdspIrInst>& constOut;
    std::unordered_map<uint32_t, int> scalarFloatConsts;
    std::unordered_map<uint64_t, int> vectorFloatConsts;
    std::unordered_map<int32_t, int> scalarIntConsts;
    std::unordered_map<uint64_t, int> vectorIntConsts;
};

} // namespace

//==============================================================================

bool YdspOptimizer::lowerFastMathTranscendentals (YdspIrFunction& fn)
{
    if (! fn.fastMath)
        return false;

    int maxResultId = -1;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result > maxResultId)
                maxResultId = inst.result;

    if (maxResultId >= 0 && static_cast<size_t> (maxResultId) >= fn.valueTypes.size())
        fn.valueTypes = resolveValueTypes (fn);

    const auto valueTypeOf = [&fn] (int id) noexcept -> YdspValueType
    {
        if (id < 0 || static_cast<size_t> (id) >= fn.valueTypes.size())
            return YdspValueType::boolType;

        return fn.valueTypes[static_cast<size_t> (id)];
    };

    const auto isEligible = [&valueTypeOf] (const YdspIrInst& inst) noexcept -> bool
    {
        if (! isFastMathLowerableOp (inst.op))
            return false;

        if (valueTypeOf (inst.result) != YdspValueType::float32Type
            || valueTypeOf (inst.a) != YdspValueType::float32Type)
            return false;

        if ((inst.op == YdspIrOp::powF || inst.op == YdspIrOp::atan2F)
            && valueTypeOf (inst.b) != YdspValueType::float32Type)
            return false;

        return true;
    };

    const auto findConstPreheader = [&fn, &isEligible] (int blockIndex) -> int
    {
        int bestPreheader = -1;

        for (const auto& loop : fn.loops)
        {
            if (loop.unrolled || loop.headerBlock < 1 || loop.exitBlock <= loop.headerBlock)
                continue;

            if (blockIndex < loop.headerBlock || blockIndex >= loop.exitBlock)
                continue;

            const int preheader = loop.headerBlock - 1;
            const auto& preBlock = fn.blocks[static_cast<size_t> (preheader)];
            const bool validPreheader = preBlock.term == YdspIrTerm::fallthrough
                                     || (preBlock.term == YdspIrTerm::branch && preBlock.termTarget == loop.headerBlock);
            if (! validPreheader)
                continue;

            bool hasRemainingCall = false;
            for (int b = loop.headerBlock; b < loop.exitBlock && b < static_cast<int> (fn.blocks.size()); ++b)
            {
                for (const auto& inst : fn.blocks[static_cast<size_t> (b)].insts)
                {
                    if ((isCallBoundary (inst.op) || inst.op == YdspIrOp::expF) && ! isEligible (inst))
                    {
                        hasRemainingCall = true;
                        break;
                    }
                }
                if (hasRemainingCall)
                    break;
            }

            if (! hasRemainingCall && (bestPreheader < 0 || preheader < bestPreheader))
                bestPreheader = preheader;
        }

        return bestPreheader;
    };

    bool changed = false;

    for (size_t blockIndex = 0; blockIndex < fn.blocks.size(); ++blockIndex)
    {
        auto& block = fn.blocks[blockIndex];

        if (std::none_of (block.insts.begin(), block.insts.end(), isEligible))
            continue;

        const int preheader = findConstPreheader (static_cast<int> (blockIndex));
        std::vector<YdspIrInst> lowered;
        std::vector<YdspIrInst> hoistedConsts;
        lowered.reserve (block.insts.size() * 8);

        FastMathBuilder builder (fn, lowered, preheader >= 0 ? hoistedConsts : lowered);

        for (const auto& inst : block.insts)
        {
            if (! isEligible (inst))
            {
                lowered.push_back (inst);
                continue;
            }

            changed = true;
            const int lanes = fn.laneCountOf (inst.result);
            const int a = builder.ensureLanes (inst.a, lanes);

            switch (inst.op)
            {
                case YdspIrOp::tanhF:
                    builder.lowerTanh (a, lanes, inst.result);
                    break;

                case YdspIrOp::sinF:
                    builder.lowerSinCos (a, false, lanes, inst.result);
                    break;

                case YdspIrOp::cosF:
                    builder.lowerSinCos (a, true, lanes, inst.result);
                    break;

                case YdspIrOp::tanF:
                    builder.lowerTan (a, lanes, inst.result);
                    break;

                case YdspIrOp::atanF:
                    builder.lowerAtan (a, lanes, inst.result);
                    break;

                case YdspIrOp::atan2F:
                {
                    const int b = builder.ensureLanes (inst.b, lanes);
                    builder.lowerAtan2 (a, b, lanes, inst.result);
                    break;
                }

                case YdspIrOp::expF:
                    builder.emitExpCore (a, lanes, inst.result);
                    break;

                case YdspIrOp::logF:
                    builder.lowerLog (a, false, lanes, inst.result);
                    break;

                case YdspIrOp::log10F:
                    builder.lowerLog (a, true, lanes, inst.result);
                    break;

                case YdspIrOp::powF:
                {
                    const int b = builder.ensureLanes (inst.b, lanes);
                    builder.lowerPow (a, b, lanes, inst.result);
                    break;
                }

                default:
                    lowered.push_back (inst);
                    break;
            }
        }

        block.insts = std::move (lowered);

        if (preheader >= 0 && ! hoistedConsts.empty())
        {
            auto& preInsts = fn.blocks[static_cast<size_t> (preheader)].insts;
            preInsts.insert (preInsts.end(), hoistedConsts.begin(), hoistedConsts.end());
        }
    }

    return changed;
}

bool YdspOptimizer::lowerStrictVectorTranscendentals (YdspIrFunction& fn)
{
    if (fn.fastMath || ! fn.vectorMathEnabled)
        return false;

    int maxResultId = -1;
    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result > maxResultId)
                maxResultId = inst.result;

    if (maxResultId >= 0 && static_cast<size_t> (maxResultId) >= fn.valueTypes.size())
        fn.valueTypes = resolveValueTypes (fn);

    const auto valueTypeOf = [&fn] (int id) noexcept -> YdspValueType
    {
        if (id < 0 || static_cast<size_t> (id) >= fn.valueTypes.size())
            return YdspValueType::boolType;

        return fn.valueTypes[static_cast<size_t> (id)];
    };

    // Track provable |x| upper bounds across the function so 4-lane sin/cos
    // inside [-128, 128] can lower to the <= 1 ULP degree-11/10 SIMD polynomial.
    std::vector<std::optional<double>> maxAbs (fn.valueTypes.size(), std::nullopt);
    std::unordered_map<int, std::optional<double>> arrayMaxAbs;
    std::vector<int> defCount (fn.valueTypes.size(), 0);
    std::unordered_map<int, int> floorInputs;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
        {
            if (inst.result >= 0 && static_cast<size_t> (inst.result) < defCount.size())
                ++defCount[static_cast<size_t> (inst.result)];

            if (inst.op == YdspIrOp::floorF && inst.result >= 0 && inst.a >= 0)
                floorInputs[inst.result] = inst.a;
        }

    const auto isDeclaredStateArray = [&fn] (int memIndex) noexcept -> bool
    {
        for (const auto& array : fn.stateArrays)
            if (isFloatValueType (array.type) && memIndex >= array.base && memIndex < array.base + array.length)
                return true;

        return false;
    };

    const auto boundOf = [&maxAbs] (int id) noexcept -> std::optional<double>
    {
        if (id < 0 || static_cast<size_t> (id) >= maxAbs.size())
            return std::nullopt;
        return maxAbs[static_cast<size_t> (id)];
    };

    for (int pass = 0; pass < 3; ++pass)
    {
        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.op == YdspIrOp::storeStateArrayF)
                {
                    if (isDeclaredStateArray (inst.memIndex))
                        continue;

                    const auto b = boundOf (inst.b);
                    auto& slot = arrayMaxAbs[inst.memIndex];
                    if (b.has_value())
                        slot = slot.has_value() ? std::max (*slot, *b) : *b;
                    else
                        slot = std::numeric_limits<double>::infinity();
                    continue;
                }

                if (inst.result < 0 || static_cast<size_t> (inst.result) >= maxAbs.size())
                    continue;

                if (defCount[static_cast<size_t> (inst.result)] != 1)
                    continue;

                std::optional<double> b;
                switch (inst.op)
                {
                    case YdspIrOp::constF:
                        b = std::abs (inst.fvalue);
                        break;
                    case YdspIrOp::vsplat:
                    case YdspIrOp::movF:
                    case YdspIrOp::negF:
                    case YdspIrOp::absF:
                        b = boundOf (inst.a);
                        break;
                    case YdspIrOp::sinF:
                    case YdspIrOp::cosF:
                    case YdspIrOp::tanhF:
                        b = 1.0;
                        break;
                    case YdspIrOp::mulF:
                    {
                        const auto ba = boundOf (inst.a);
                        const auto bb = boundOf (inst.b);
                        if (ba.has_value() && bb.has_value())
                            b = *ba * *bb;
                        break;
                    }
                    case YdspIrOp::addF:
                    {
                        const auto ba = boundOf (inst.a);
                        const auto bb = boundOf (inst.b);
                        if (ba.has_value() && bb.has_value())
                            b = *ba + *bb;
                        break;
                    }
                    case YdspIrOp::subF:
                    {
                        if (const auto fl = floorInputs.find (inst.b); fl != floorInputs.end() && fl->second == inst.a)
                        {
                            b = 1.0;
                        }
                        else
                        {
                            const auto ba = boundOf (inst.a);
                            const auto bb = boundOf (inst.b);
                            if (ba.has_value() && bb.has_value())
                                b = *ba + *bb;
                        }
                        break;
                    }
                    case YdspIrOp::loadStateArrayF:
                    {
                        if (const auto it = arrayMaxAbs.find (inst.memIndex);
                            it != arrayMaxAbs.end() && it->second.has_value() && std::isfinite (*it->second))
                            b = *it->second;
                        break;
                    }
                    default:
                        break;
                }

                if (b.has_value() && std::isfinite (*b))
                    maxAbs[static_cast<size_t> (inst.result)] = *b;
            }
        }
    }

    const auto isEligible = [&fn, &valueTypeOf, &boundOf] (const YdspIrInst& inst) noexcept -> bool
    {
        if (inst.result < 0 || fn.laneCountOf (inst.result) <= 1)
            return false;

        if (valueTypeOf (inst.result) != YdspValueType::float32Type
            || valueTypeOf (inst.a) != YdspValueType::float32Type)
            return false;

        if (inst.op == YdspIrOp::tanhF)
            return true;

        if (inst.op == YdspIrOp::sinF || inst.op == YdspIrOp::cosF)
        {
            const auto b = boundOf (inst.a);
            return b.has_value() && *b <= 128.0;
        }

        return false;
    };

    const auto findConstPreheader = [&fn, &isEligible] (int blockIndex) -> int
    {
        int bestPreheader = -1;

        for (const auto& loop : fn.loops)
        {
            if (loop.unrolled || loop.headerBlock < 1 || loop.exitBlock <= loop.headerBlock)
                continue;

            if (blockIndex < loop.headerBlock || blockIndex >= loop.exitBlock)
                continue;

            const int preheader = loop.headerBlock - 1;
            const auto& preBlock = fn.blocks[static_cast<size_t> (preheader)];
            const bool validPreheader = preBlock.term == YdspIrTerm::fallthrough
                                     || (preBlock.term == YdspIrTerm::branch && preBlock.termTarget == loop.headerBlock);
            if (! validPreheader)
                continue;

            bool hasRemainingCall = false;
            for (int b = loop.headerBlock; b < loop.exitBlock && b < static_cast<int> (fn.blocks.size()); ++b)
            {
                for (const auto& inst : fn.blocks[static_cast<size_t> (b)].insts)
                {
                    if ((isCallBoundary (inst.op) || inst.op == YdspIrOp::expF) && ! isEligible (inst))
                    {
                        hasRemainingCall = true;
                        break;
                    }
                }
                if (hasRemainingCall)
                    break;
            }

            if (! hasRemainingCall && (bestPreheader < 0 || preheader < bestPreheader))
                bestPreheader = preheader;
        }

        return bestPreheader;
    };

    bool changed = false;

    for (size_t blockIndex = 0; blockIndex < fn.blocks.size(); ++blockIndex)
    {
        auto& block = fn.blocks[blockIndex];

        if (std::none_of (block.insts.begin(), block.insts.end(), isEligible))
            continue;

        const int preheader = findConstPreheader (static_cast<int> (blockIndex));
        std::vector<YdspIrInst> lowered;
        std::vector<YdspIrInst> hoistedConsts;
        lowered.reserve (block.insts.size() * 8);

        FastMathBuilder builder (fn, lowered, preheader >= 0 ? hoistedConsts : lowered);

        for (const auto& inst : block.insts)
        {
            if (! isEligible (inst))
            {
                lowered.push_back (inst);
                continue;
            }

            changed = true;
            const int lanes = fn.laneCountOf (inst.result);
            const int a = builder.ensureLanes (inst.a, lanes);

            if (inst.op == YdspIrOp::tanhF)
                builder.lowerTanh (a, lanes, inst.result);
            else if (inst.op == YdspIrOp::sinF)
                builder.lowerSinCos (a, false, lanes, inst.result, true);
            else if (inst.op == YdspIrOp::cosF)
                builder.lowerSinCos (a, true, lanes, inst.result, true);
        }

        block.insts = std::move (lowered);

        if (preheader >= 0 && ! hoistedConsts.empty())
        {
            auto& preInsts = fn.blocks[static_cast<size_t> (preheader)].insts;
            preInsts.insert (preInsts.end(), hoistedConsts.begin(), hoistedConsts.end());
        }
    }

    return changed;
}

} // namespace yup
