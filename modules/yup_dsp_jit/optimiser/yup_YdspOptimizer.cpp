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

int YdspIrFunction::getInstructionCount() const noexcept
{
    int count = 0;

    for (const auto& block : blocks)
        count += static_cast<int> (block.insts.size());

    return count;
}

//==============================================================================

namespace
{

String vectorizationReasonText (YdspVectorizationReason reason)
{
    switch (reason)
    {
        case YdspVectorizationReason::widened:
            return "widened";

        case YdspVectorizationReason::notVectorizable:
            return "the loop is not vectorizable";

        case YdspVectorizationReason::unsupportedLoopBound:
            return "the loop bound is not a compile-time constant or blockSize";

        case YdspVectorizationReason::multiBlockBody:
            return "the loop body contains control flow (an if or a nested loop)";

        case YdspVectorizationReason::nonConstantStart:
            return "the loop start is not a compile-time constant";

        case YdspVectorizationReason::nonzeroStart:
            return "a blockSize loop must start at 0";

        case YdspVectorizationReason::shortTripCount:
            return "the loop span is shorter than one vector";

        case YdspVectorizationReason::missingHeaderCompare:
            return "the loop header does not hold exactly one `i < bound` compare";

        case YdspVectorizationReason::unrecognizedInductionUpdate:
            return "the loop body does not end with `next = i + 1; i = next`";

        case YdspVectorizationReason::inductionUsedAsValue:
            return "the loop variable is used as a value instead of only as an array/stream index";

        case YdspVectorizationReason::indirectAccess:
            return "an element is reached through an index other than the loop variable";

        case YdspVectorizationReason::unrecognizedAccumulation:
            return "a value carried across iterations is not of the form `acc = acc + x`";

        case YdspVectorizationReason::loopCarriedValue:
            return "the loop writes a value that escapes the loop (scalar state, `'`, `@` or `smooth`)";

        case YdspVectorizationReason::unsupportedWidenedOp:
            return "an unsupported operation consumes a widened value";

        case YdspVectorizationReason::unsupportedWidenedType:
            return "a widened value is not float32";

        case YdspVectorizationReason::stateWriteInBody:
            return "the loop writes scalar state, a param or an event field";

        case YdspVectorizationReason::invariantStreamStore:
            return "a stream store does not go through the loop variable";

        case YdspVectorizationReason::emitInBody:
            return "the loop emits an output event";

        case YdspVectorizationReason::nothingToWiden:
            return "the loop has no array or stream access to widen";

        case YdspVectorizationReason::runtimeBoundWithoutStreams:
            return "a blockSize-bound loop needs a stream access at the loop variable";

        default:
            break;
    }

    return "the loop is not vectorizable";
}

} // namespace

//==============================================================================

String YdspVectorizationResult::describe() const
{
    if (reason == YdspVectorizationReason::widened)
        return "loop " + String (loopId) + " was widened to " + String (laneCount) + " lanes";

    return "loop " + String (loopId) + " was not vectorized: " + vectorizationReasonText (reason);
}

int YdspVectorizationReport::countWidened() const noexcept
{
    int count = 0;

    for (const auto& result : loops)
        if (result.widened())
            ++count;

    return count;
}

StringArray YdspVectorizationReport::rejectionReasons() const
{
    StringArray reasons;

    for (const auto& result : loops)
    {
        if (result.widened())
            continue;

        const auto text = vectorizationReasonText (result.reason);

        if (! reasons.contains (text))
            reasons.add (text);
    }

    return reasons;
}

//==============================================================================

YdspOptimizer::YdspOptimizer (YdspDiagnostics& diagnostics)
    : diagnostics (diagnostics)
{
}

//==============================================================================

std::unique_ptr<YdspIrProgram> YdspOptimizer::build (const YdspAnalyzedProgram& program)
{
    auto ir = std::make_unique<YdspIrProgram>();
    ir->analyzed = &program;

    for (const auto& processor : program.processors)
    {
        // The state layout is computed once per processor and shared by the
        // kernel, the init kernel and every event-handler function.
        const auto layout = detail::computeStateLayout (processor);

        auto function = std::make_unique<YdspIrFunction>();

        YdspIrBuilder::Config builderConfig;
        builderConfig.programFunctions = &program.functions;

        YdspIrBuilder builder (*function, processor, diagnostics, layout, builderConfig);
        builder.build();

        // Resolve the voice-activity flag to a byte offset in the scalar
        // segment, whose region order is [f32][i32][f64][i64]. This has to run
        // after build(): the hidden ' / @ / smooth slots it allocates grow
        // float32Scalars past the declared layout and so shift the i32 region
        // base (the declared slot index itself is stable). The optimiser passes
        // never change the counts, so running before or after them is the same.
        if (processor.activityState != nullptr)
        {
            const auto slot = layout.int32ScalarSlots.find (processor.activityState);

            if (slot != layout.int32ScalarSlots.end())
                function->activityByteOffset = function->float32Scalars * 4 + slot->second * 4;
        }

        function->name = processor.decl->name;

        runPasses (*function);

        ir->kernels.push_back (std::move (function));

        // A processor with an init block gets a second one-shot kernel that
        // runs once before audio (it shares the processor's state layout).
        if (processor.decl->init != nullptr)
        {
            auto initFunction = std::make_unique<YdspIrFunction>();

            YdspIrBuilder::Config initConfig;
            initConfig.isInit = true;
            initConfig.sharedLayout = ir->kernels.back().get();
            initConfig.programFunctions = &program.functions;

            YdspIrBuilder initBuilder (*initFunction, processor, diagnostics, layout, initConfig);
            initBuilder.build();

            initFunction->name = processor.decl->name;

            runPasses (*initFunction);

            ir->kernels.push_back (std::move (initFunction));
        }

        // One generated machine function per event handler, sharing the
        // kernel's state layout (ir->kernels.back() is the init kernel when
        // one exists, which itself adopted the kernel's final counts).
        for (const auto& handler : processor.eventHandlers)
        {
            auto handlerFunction = std::make_unique<YdspIrFunction>();

            YdspIrBuilder::Config handlerConfig;
            handlerConfig.isEventHandler = true;
            handlerConfig.eventHandler = &handler;
            handlerConfig.sharedLayout = ir->kernels.back().get();
            handlerConfig.programFunctions = &program.functions;

            YdspIrBuilder handlerBuilder (*handlerFunction, processor, diagnostics, layout, handlerConfig);
            handlerBuilder.build();

            handlerFunction->name = processor.decl->name + "." + handler.decl->shapeName;

            runPasses (*handlerFunction);

            ir->eventHandlers.push_back (std::move (handlerFunction));
        }
    }

    if (diagnostics.hasErrors())
        return nullptr;

    return ir;
}

//==============================================================================

void YdspOptimizer::buildReport (const YdspIrProgram& program, YdspExecutionReport& report)
{
    report.getKernels().clear();

    for (const auto& kernel : program.kernels)
    {
        YdspKernelReport entry;
        entry.name = String (kernel->name);
        entry.instructionCount = kernel->getInstructionCount();

        int boundedIterations = 1;

        for (const auto& loop : kernel->loops)
        {
            if (loop.bound.kind == YdspLoopBoundKind::constant)
                boundedIterations *= loop.bound.constant;

            // blockSize-derived bounds contribute one symbolic unit.
            entry.loopBounds.add (String (loop.bound.toString()));
        }

        entry.boundedIterationCount = boundedIterations;
        entry.provenRealtimeSafe = true;
        entry.vectorized = kernel->vectorized;
        entry.vectorWidth = kernel->vectorWidth;
        entry.loopVectorization.loops = kernel->vectorizationResults;

        entry.unrolled = std::any_of (kernel->loops.begin(), kernel->loops.end(), [] (const YdspIrLoop& loop)
        {
            return loop.unrolled;
        });

        entry.reductionSplit = kernel->reductionSplit;

        report.getKernels().push_back (std::move (entry));
    }
}

//==============================================================================

void YdspOptimizer::runPasses (YdspIrFunction& fn)
{
    for (int i = 0; i < 4; ++i)
    {
        constantFolding (fn);
        algebraicSimplification (fn);
        ifConversion (fn);
        storeToLoadForwarding (fn);
        copyPropagation (fn);
        deadCodeElimination (fn);
    }

    loopInvariantCodeMotion (fn);
    deadCodeElimination (fn);

    // The vectoriser runs last, and deliberately so: it relies on LICM having
    // already hoisted the loop-invariant work (a bank's shared drive term, its
    // coefficients) into the preheader, where it stays scalar and is broadcast
    // once, and none of the passes above knows what a lane is - constant
    // folding a `vsplat` of a literal, say, would silently drop its width.
    if (vectorizationEnabled)
        if (YdspVectorizer::run (fn, vectorWidth))
            deadCodeElimination (fn);

    // After the vectoriser, so a widened loop is unrolled at its widened trip
    // count - four copies of a four-lane body, not sixteen of a scalar one -
    // and because unrolling first would leave nothing loop-shaped to widen.
    // The DCE that follows collects the bound compare the unroll orphans.
    if (unrollingEnabled)
    {
        fullyUnrollBoundedLoops (fn);
        deadCodeElimination (fn);
    }

    // Only reachable once the copies exist: a rolled loop has one add into the
    // accumulator per body, so there is no chain in any single block to split.
    // Nothing downstream re-associates, so this runs last.
    if (reductionSplittingEnabled)
        splitWidenedReductionChains (fn);

    // After the loop transforms, deliberately: contraction forms a widened
    // operation only when the selected target supports packed FMA. Running it
    // earlier would otherwise fuse work the vectoriser was about to widen and
    // then have to decline. The dead-code pass collects the multiplies it
    // orphans - without it the pass adds an instruction rather than removing
    // one.
    if (contractionEnabled)
    {
        contractMultiplyAdd (fn);
        deadCodeElimination (fn);
    }

    // Last of all, and outside the switch above: a patch that spells `fma()`
    // out has an `fmaF` to lower whether or not anything was contracted, and
    // the wasm backend rejects the opcode outright rather than guessing.
    if (! targetHasFusedMultiplyAdd)
        lowerFusedMultiplyAdd (fn);
}

} // namespace yup
