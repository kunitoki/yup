/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "yoga/Yoga.h"
#include "yoga/grid/YGGridCompat.h"
#include "yoga/grid/AutoPlacement.h"
#include "yoga/grid/GridTrack.h"
#include "yoga/event/event.h"
#include "yoga/YGNode.h"
#include <vector>

namespace facebook::yoga {

void calculateGridLayoutInternal(
    YGNode* node,
    float availableWidth,
    float availableHeight,
    YGDirection ownerDirection,
    YGMeasureMode widthSizingMode,
    YGMeasureMode heightSizingMode,
    float ownerWidth,
    float ownerHeight,
    bool performLayout,
    LayoutPassReason reason,
    YGConfigRef config,
    LayoutData& layoutMarkerData,
    void* layoutContext,
    uint32_t depth,
    uint32_t generationCount);

struct GridTracks {
  std::vector<GridTrack> columnTracks;
  std::vector<GridTrack> rowTracks;
};
// Creates implicit grid tracks based on the auto placement result
GridTracks createGridTracks(
    YGNode* node,
    const ResolvedAutoPlacement& autoPlacement);

} // namespace facebook::yoga
