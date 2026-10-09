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

#pragma once

namespace yup
{

//==============================================================================
/** The GPU outcome of one GpuFrame, readable after the frame itself is gone.

    Frames are submitted without waiting, so whether the GPU managed to execute
    one (rather than running out of memory, faulting or timing out) is only known
    once it completed. Take the status with GpuFrame::getStatus() to find out
    later without blocking, for instance at the start of the next frame.

    A frame whose status was taken, or that was waited on with
    GpuFrame::waitForGPU(), reports its failure there only: it never shows up in
    GpuDevice::takeGpuFailure().

    Copies share the same outcome. Thread safe.

    @see GpuFrame, GpuDevice::takeGpuFailure
*/
class YUP_API GpuFrameStatus
{
public:
    //==============================================================================
    /** Creates an invalid status, which belongs to no frame. */
    GpuFrameStatus() = default;

    /** Returns true if this status belongs to a frame. */
    bool isValid() const noexcept { return block != nullptr; }

    /** Returns the outcome once the GPU finished the frame, without blocking.

        @returns Nothing while the frame is still pending, otherwise Result::ok()
                 or the failure the GPU reported. Always fails for an invalid status.
    */
    std::optional<Result> poll() const;

    /** Blocks until the GPU finished the frame and returns its outcome.

        @returns Result::ok() or the failure the GPU reported. Fails at once for
                 an invalid status or a frame that was never submitted.
    */
    Result wait() const;

    //==============================================================================
    /** @internal The state a frame shares with its device and its statuses. */
    struct Block;

private:
    friend class GpuFrame;
    friend class GpuDevice;

    explicit GpuFrameStatus (std::shared_ptr<Block> sharedBlock) noexcept;

    std::shared_ptr<Block> block;
};

} // namespace yup
