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

bool isStateArrayStore (YdspIrOp op) noexcept
{
    return op == YdspIrOp::storeStateArrayF || op == YdspIrOp::storeStateArrayI;
}

YdspIrOp matchingStoreFor (YdspIrOp loadOp) noexcept
{
    return loadOp == YdspIrOp::loadStateArrayF ? YdspIrOp::storeStateArrayF : YdspIrOp::storeStateArrayI;
}

} // namespace

//==============================================================================

bool YdspOptimizer::storeToLoadForwarding (YdspIrFunction& fn)
{
    bool changed = false;
    const auto types = resolveValueTypes (fn);

    for (auto& block : fn.blocks)
    {
        auto& insts = block.insts;
        if (std::none_of (insts.begin(), insts.end(), [] (const auto& inst)
        {
            return isStateArrayStore (inst.op);
        }))
            continue;

        std::vector<std::optional<int64_t>> constants (types.size());
        std::vector<std::optional<int64_t>> indices (insts.size());
        for (size_t i = 0; i < insts.size(); ++i)
        {
            const auto& inst = insts[i];
            if ((isStateArrayStore (inst.op) || inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::loadStateArrayI)
                && inst.a >= 0)
                indices[i] = constants[static_cast<size_t> (inst.a)];
            if (inst.result >= 0)
            {
                auto& value = constants[static_cast<size_t> (inst.result)];
                value.reset();
                if (inst.op == YdspIrOp::constI && fn.laneCountOf (inst.result) == 1
                    && inst.ivalue >= 0
                    && inst.ivalue <= std::numeric_limits<int32_t>::max())
                    value = inst.ivalue;
            }
        }

        for (size_t j = 0; j < insts.size(); ++j)
        {
            auto& load = insts[j];

            if (load.op != YdspIrOp::loadStateArrayF && load.op != YdspIrOp::loadStateArrayI)
                continue;

            if (load.result < 0 || load.a < 0)
                continue;

            const auto storeOp = matchingStoreFor (load.op);
            const auto type = types[static_cast<size_t> (load.result)];
            const auto lanes = fn.laneCountOf (load.result);

            size_t source = 0;
            bool found = false;

            for (size_t k = j; k-- > 0;)
            {
                const auto& previous = insts[k];

                if (previous.result >= 0 && previous.result == load.a)
                    break;

                if (previous.op == YdspIrOp::emitEvent)
                    break;

                if (! isStateArrayStore (previous.op))
                    continue;

                if (previous.op != storeOp || previous.b < 0 || types[static_cast<size_t> (previous.b)] != type)
                    break;

                int64_t storeStart = previous.memIndex;
                int64_t loadStart = load.memIndex;
                if (indices[k].has_value() && indices[j].has_value())
                {
                    storeStart += *indices[k];
                    loadStart += *indices[j];
                }
                else if (previous.a != load.a)
                {
                    break;
                }

                const auto storedLanes = fn.laneCountOf (previous.b);
                if (storeStart == loadStart && storedLanes == lanes)
                {
                    source = k;
                    found = true;
                    break;
                }

                if (storeStart < loadStart + lanes && loadStart < storeStart + storedLanes)
                    break;
            }

            if (! found)
                continue;

            const auto stored = insts[source].b;

            if (stored < 0 || stored == load.result)
                continue;

            bool valueStable = true;

            for (size_t k = source + 1; k < j && valueStable; ++k)
                valueStable = insts[k].result < 0 || insts[k].result != stored;

            if (! valueStable)
                continue;

            changed = true;
            load.op = (load.op == YdspIrOp::loadStateArrayF) ? YdspIrOp::movF : YdspIrOp::movI;
            load.a = stored;
            load.b = -1;
            load.c = -1;
            load.memIndex = -1;
        }
    }
    return changed;
}
} // namespace yup
