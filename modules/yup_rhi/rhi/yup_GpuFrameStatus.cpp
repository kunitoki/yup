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

struct GpuFrameStatus::Block
{
    /** Shared with the device that completes the frame, and with every other frame of it. */
    std::shared_ptr<CriticalSection> lock;

    WaitableEvent done { true };
    Result result = Result::ok();
    String injectedFailure;
    bool submitted = false;
    bool completed = false;

    /** Set by GpuFrame::setTimingEnabled(), before the frame is submitted. */
    bool timingEnabled = false;

    /** Kept only for a measured frame that completed successfully. */
    GpuFrameTimings timings;

    /** The scopes of a frame that measures them, moved in when it is submitted. Completing the
        frame fills in their times and moves them into the timings, so nothing allocates under
        the lock. */
    std::vector<GpuScopeTiming> scopes;

    /** A waiter or an owner reads the outcome, so the device does not report it. */
    bool claimed = false;
};

//==============================================================================

GpuFrameStatus::GpuFrameStatus (std::shared_ptr<Block> sharedBlock) noexcept
    : block (std::move (sharedBlock))
{
}

std::optional<Result> GpuFrameStatus::poll() const
{
    if (block == nullptr)
        return Result::fail ("The frame status is not valid");

    const ScopedLock sl (*block->lock);

    if (! block->completed)
        return std::nullopt;

    return block->result;
}

Result GpuFrameStatus::wait() const
{
    if (block == nullptr)
        return Result::fail ("The frame status is not valid");

    {
        const ScopedLock sl (*block->lock);

        if (! block->submitted && ! block->completed)
            return Result::fail ("The frame was not submitted");
    }

    block->done.wait();

    const ScopedLock sl (*block->lock);
    return block->result;
}

GpuFrameTimings GpuFrameStatus::getTimings() const
{
    if (block == nullptr)
        return {};

    // Completing a frame writes its timings once, under the lock, so a completed frame's are read
    // without holding the lock its device's completions take
    {
        const ScopedLock sl (*block->lock);
        if (! block->completed)
            return {};
    }

    return block->timings;
}

} // namespace yup
