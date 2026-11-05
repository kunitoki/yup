/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "BitUtils.h"
#include "YGFloatOptional.h"
#include "Yoga-internal.h"

struct YGLayout {
  std::array<float, 4> position = {};
  std::array<float, 2> dimensions = {{YGUndefined, YGUndefined}};
  std::array<float, 4> margin = {};
  std::array<float, 4> border = {};
  std::array<float, 4> padding = {};

private:
  static constexpr size_t directionOffset = 0;
  static constexpr size_t hadOverflowOffset =
      directionOffset + facebook::yoga::detail::bitWidthFn<YGDirection>();
  uint8_t flags = 0;

public:
  // rive: bit i is set when cachedMeasurements[i] sized something below this
  // node to the space it was offered rather than its content, so it can't
  // stand in for a measurement under other limits. In the padding after
  // flags.
  uint8_t limitDependentMeasurements = 0;
  // rive: bit i is set when cachedMeasurements[i] holds a container with
  // children that measures as nothing in either axis, which a limit that
  // only just holds it would leave nothing past its margins.
  uint8_t emptyContainerMeasurements = 0;
  // rive: YGNodeContentFlags' answer for this node, kept until it's dirtied.
  uint8_t contentFlags = 0;
  static_assert(
      YG_MAX_CACHED_RESULT_COUNT <= 8,
      "the measurement masks have a bit per cached measurement");
  uint32_t computedFlexBasisGeneration = 0;
  YGFloatOptional computedFlexBasis = {};

  // Instead of recomputing the entire layout every single time, we cache some
  // information to break early when nothing changed
  uint32_t generationCount = 0;
  YGDirection lastOwnerDirection = YGDirectionInherit;

  uint32_t nextCachedMeasurementsIndex = 0;
  std::array<YGCachedMeasurement, YG_MAX_CACHED_RESULT_COUNT>
      cachedMeasurements = {};
  std::array<float, 2> measuredDimensions = {{YGUndefined, YGUndefined}};

  YGCachedMeasurement cachedLayout = YGCachedMeasurement();

  // Grid line positions (column/row) in this node's local content space,
  // matching the coordinate space of child layout positions. Populated only
  // for grid nodes during a performLayout pass; empty (no heap) otherwise.
  // Exposes the cell/track geometry the layout engine otherwise discards, so
  // tooling can hit-test a point to a grid cell.
  std::vector<float> gridColumnLineOffsets = {};
  std::vector<float> gridRowLineOffsets = {};
  // rive: the gaps between those lines' tracks, as content distribution
  // spaced them.
  float gridColumnGap = 0.0f;
  float gridRowGap = 0.0f;

  YGDirection direction() const {
    return facebook::yoga::detail::getEnumData<YGDirection>(
        flags, directionOffset);
  }

  void setDirection(YGDirection direction) {
    facebook::yoga::detail::setEnumData<YGDirection>(
        flags, directionOffset, direction);
  }

  bool hadOverflow() const {
    return facebook::yoga::detail::getBooleanData(flags, hadOverflowOffset);
  }
  void setHadOverflow(bool hadOverflow) {
    facebook::yoga::detail::setBooleanData(
        flags, hadOverflowOffset, hadOverflow);
  }

  bool operator==(YGLayout layout) const;
  bool operator!=(YGLayout layout) const { return !(*this == layout); }
};
