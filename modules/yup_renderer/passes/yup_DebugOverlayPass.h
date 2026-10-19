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
/** Draws the debug overlays over a finished frame: the edges of the drawn meshes
    (RenderSettings::wireframe) and the edges of bounding boxes (SceneRenderer::setBoundsOverlay()).

    The lines go through the unjittered camera, without a depth test, blended over the target with
    the straight alpha of their color. Its bindings: set 0 binding 0 the ViewBlock, set 1 binding 0
    the model matrix and the color of the draw.

    @internal
*/
class YUP_API DebugOverlayPass
{
public:
    //==============================================================================
    /** What to draw. */
    struct Lines
    {
        const SceneDrawList* wireframe = nullptr; ///< Draws the edges of its opaque and blended items, if not nullptr.
        Color wireframeColor;                     ///< The color of the edges of the items.
        Span<const BoundingBox> boxes;            ///< World space boxes to outline; empty ones are skipped.
        Color boxColor;                           ///< The color of the outlines of the boxes.

        /** True if there is nothing to draw. */
        bool isEmpty() const noexcept;
    };

    //==============================================================================
    /** Creates a pass for @p device. The pipelines are compiled by prepare(). */
    explicit DebugOverlayPass (GpuDevice::Ptr device);

    /** Destructor. */
    ~DebugOverlayPass();

    //==============================================================================
    /** Compiles the pipelines once for targets of @p format: a failure is remembered instead of
        retried every frame.
    */
    Result prepare (GpuTextureFormat format);

    /** Draws @p lines over the contents of @p target.

        @param beginScope  Called right before the pass records, to open its GPU scope; may be empty.

        @returns False if the pass is not prepared or could not be recorded.
    */
    bool render (GpuFrame& frame, GpuTarget& target, const ViewData& view, const Lines& lines,
                 const std::function<void()>& beginScope = {});

    /** The draw calls recorded by the last render, one per item and one per box. */
    int getNumDraws() const noexcept { return numDraws; }

private:
    GpuDevice::Ptr device;
    std::optional<Result> prepared;
    GpuPipeline::Ptr wireframePipeline;
    GpuPipeline::Ptr boxesPipeline;
    int numDraws = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DebugOverlayPass)
};

} // namespace yup
