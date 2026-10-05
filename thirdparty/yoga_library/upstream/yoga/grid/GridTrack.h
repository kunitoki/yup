/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "yoga/YGGridTrack.h"

namespace facebook::yoga {

struct GridTrack : GridTrackSize {
  float baseSize = 0.0f;
  float growthLimit = 0.0f;
  bool infinitelyGrowable = false;

  // Transient per-pass scratch used by the track-sizing distribution steps.
  // Not part of grid state; each pass resets these before reading them. Kept
  // on the track (rather than in per-track hash maps/sets) to avoid the code
  // size and allocations of instantiating unordered_map/set<GridTrack*>.
  float plannedIncrease = 0.0f;
  float itemIncurredIncrease = 0.0f;
  bool frozen = false;

  explicit GridTrack(const GridTrackSize& trackSize)
      : GridTrackSize(trackSize) {}
};

} // namespace facebook::yoga
