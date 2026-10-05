/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "YGStyle.h"
#include "Utils.h"

// Yoga specific properties, not compatible with flexbox specification
bool operator==(const YGStyle& lhs, const YGStyle& rhs) {
  bool areNonFloatValuesEqual = lhs.direction() == rhs.direction() &&
      lhs.flexDirection() == rhs.flexDirection() &&
      lhs.justifyContent() == rhs.justifyContent() &&
      lhs.alignContent() == rhs.alignContent() &&
      lhs.alignItems() == rhs.alignItems() &&
      lhs.alignSelf() == rhs.alignSelf() &&
      lhs.positionType() == rhs.positionType() &&
      lhs.flexWrap() == rhs.flexWrap() && lhs.overflow() == rhs.overflow() &&
      lhs.display() == rhs.display() &&
      YGValueEqual(lhs.flexBasis(), rhs.flexBasis()) &&
      lhs.margin() == rhs.margin() && lhs.position() == rhs.position() &&
      lhs.padding() == rhs.padding() && lhs.border() == rhs.border() &&
      lhs.gap() == rhs.gap() && lhs.dimensions() == rhs.dimensions() &&
      lhs.minDimensions() == rhs.minDimensions() &&
      lhs.maxDimensions() == rhs.maxDimensions();

  areNonFloatValuesEqual = areNonFloatValuesEqual &&
      lhs.flex().isUndefined() == rhs.flex().isUndefined();
  if (areNonFloatValuesEqual && !lhs.flex().isUndefined() &&
      !rhs.flex().isUndefined()) {
    areNonFloatValuesEqual = areNonFloatValuesEqual && lhs.flex() == rhs.flex();
  }

  areNonFloatValuesEqual = areNonFloatValuesEqual &&
      lhs.flexGrow().isUndefined() == rhs.flexGrow().isUndefined();
  if (areNonFloatValuesEqual && !lhs.flexGrow().isUndefined()) {
    areNonFloatValuesEqual =
        areNonFloatValuesEqual && lhs.flexGrow() == rhs.flexGrow();
  }

  areNonFloatValuesEqual = areNonFloatValuesEqual &&
      lhs.flexShrink().isUndefined() == rhs.flexShrink().isUndefined();
  if (areNonFloatValuesEqual && !rhs.flexShrink().isUndefined()) {
    areNonFloatValuesEqual =
        areNonFloatValuesEqual && lhs.flexShrink() == rhs.flexShrink();
  }

  if (!(lhs.aspectRatio().isUndefined() && rhs.aspectRatio().isUndefined())) {
    areNonFloatValuesEqual =
        areNonFloatValuesEqual && lhs.aspectRatio() == rhs.aspectRatio();
  }

  // rive: grid style backport (facebook/yoga PR #1893)
  areNonFloatValuesEqual = areNonFloatValuesEqual &&
      lhs.justifyItems() == rhs.justifyItems() &&
      lhs.justifySelf() == rhs.justifySelf() &&
      lhs.gridTemplateColumns() == rhs.gridTemplateColumns() &&
      lhs.gridTemplateRows() == rhs.gridTemplateRows() &&
      lhs.gridAutoColumns() == rhs.gridAutoColumns() &&
      lhs.gridAutoRows() == rhs.gridAutoRows() &&
      lhs.gridColumnStart() == rhs.gridColumnStart() &&
      lhs.gridColumnEnd() == rhs.gridColumnEnd() &&
      lhs.gridRowStart() == rhs.gridRowStart() &&
      lhs.gridRowEnd() == rhs.gridRowEnd();

  return areNonFloatValuesEqual;
}

// rive: yoga 3.x-style computed accessors used by the grid backport (#1894)

#include "YGNode.h"
#include "Utils.h"

namespace {

facebook::yoga::StyleSizeLength sizeLengthFromValue(const YGValue& value) {
  using facebook::yoga::StyleSizeLength;
  switch (value.unit) {
    case YGUnitPoint:
      return StyleSizeLength::points(value.value);
    case YGUnitPercent:
      return StyleSizeLength::percent(value.value);
    case YGUnitAuto:
      return StyleSizeLength::ofAuto();
    case YGUnitUndefined:
      return StyleSizeLength::ofUndefined();
  }
  return StyleSizeLength::ofUndefined();
}

float unwrapOrZero(const YGFloatOptional& value) {
  return value.isUndefined() ? 0.0f : value.unwrap();
}

} // namespace

float YGStyle::computeMarginForAxis(YGFlexDirection axis, float widthSize)
    const {
  auto leadingMargin = YGFlexDirectionIsRow(axis)
      ? YGNode::computeEdgeValueForRow(
            margin_, YGEdgeStart, leading[axis], facebook::yoga::detail::CompactValue::ofZero())
      : YGNode::computeEdgeValueForColumn(
            margin_, leading[axis], facebook::yoga::detail::CompactValue::ofZero());
  auto trailingMargin = YGFlexDirectionIsRow(axis)
      ? YGNode::computeEdgeValueForRow(
            margin_, YGEdgeEnd, trailing[axis], facebook::yoga::detail::CompactValue::ofZero())
      : YGNode::computeEdgeValueForColumn(
            margin_, trailing[axis], facebook::yoga::detail::CompactValue::ofZero());
  return unwrapOrZero(YGResolveValueMargin(leadingMargin, widthSize)) +
      unwrapOrZero(YGResolveValueMargin(trailingMargin, widthSize));
}

float YGStyle::computeInlineStartMargin(
    YGFlexDirection axis,
    YGDirection direction,
    float widthSize) const {
  const YGFlexDirection resolvedAxis = YGResolveFlexDirection(axis, direction);
  auto startMargin = YGFlexDirectionIsRow(resolvedAxis)
      ? YGNode::computeEdgeValueForRow(
            margin_,
            YGEdgeStart,
            leading[resolvedAxis],
            facebook::yoga::detail::CompactValue::ofZero())
      : YGNode::computeEdgeValueForColumn(
            margin_, leading[resolvedAxis], facebook::yoga::detail::CompactValue::ofZero());
  return unwrapOrZero(YGResolveValueMargin(startMargin, widthSize));
}

bool YGStyle::inlineStartMarginIsAuto(
    YGFlexDirection axis,
    YGDirection direction) const {
  const YGFlexDirection resolvedAxis = YGResolveFlexDirection(axis, direction);
  auto startMargin = YGFlexDirectionIsRow(resolvedAxis)
      ? YGNode::computeEdgeValueForRow(
            margin_,
            YGEdgeStart,
            leading[resolvedAxis],
            facebook::yoga::detail::CompactValue::ofZero())
      : YGNode::computeEdgeValueForColumn(
            margin_, leading[resolvedAxis], facebook::yoga::detail::CompactValue::ofZero());
  return startMargin.isAuto();
}

bool YGStyle::inlineEndMarginIsAuto(YGFlexDirection axis, YGDirection direction)
    const {
  const YGFlexDirection resolvedAxis = YGResolveFlexDirection(axis, direction);
  auto endMargin = YGFlexDirectionIsRow(resolvedAxis)
      ? YGNode::computeEdgeValueForRow(
            margin_,
            YGEdgeEnd,
            trailing[resolvedAxis],
            facebook::yoga::detail::CompactValue::ofZero())
      : YGNode::computeEdgeValueForColumn(
            margin_, trailing[resolvedAxis], facebook::yoga::detail::CompactValue::ofZero());
  return endMargin.isAuto();
}

float YGStyle::computeGapForDimension(
    YGDimension dimension,
    float availableSize) const {
  auto gap = dimension == YGDimensionWidth
      ? YGNode::computeColumnGap(gap_, facebook::yoga::detail::CompactValue::ofZero())
      : YGNode::computeRowGap(gap_, facebook::yoga::detail::CompactValue::ofZero());
  return YGFloatMax(unwrapOrZero(YGResolveValue(gap, availableSize)), 0.0f);
}

YGFloatOptional YGStyle::resolvedMinDimension(
    YGDirection /*direction*/,
    YGDimension dimension,
    float referenceLength,
    float /*ownerWidth*/) const {
  // 2.x has no box-sizing; content-box adjustment does not apply
  return YGResolveValue(minDimensions_[dimension], referenceLength);
}

YGFloatOptional YGStyle::resolvedMaxDimension(
    YGDirection /*direction*/,
    YGDimension dimension,
    float referenceLength,
    float /*ownerWidth*/) const {
  return YGResolveValue(maxDimensions_[dimension], referenceLength);
}

facebook::yoga::StyleSizeLength YGStyle::dimension(YGDimension dimension)
    const {
  return sizeLengthFromValue(dimensions_[dimension]);
}

facebook::yoga::StyleSizeLength YGStyle::minDimension(YGDimension dimension)
    const {
  return sizeLengthFromValue(minDimensions_[dimension]);
}

facebook::yoga::StyleSizeLength YGStyle::maxDimension(YGDimension dimension)
    const {
  return sizeLengthFromValue(maxDimensions_[dimension]);
}

float YGStyle::computeInlineEndMargin(
    YGFlexDirection axis,
    YGDirection direction,
    float widthSize) const {
  const YGFlexDirection resolvedAxis = YGResolveFlexDirection(axis, direction);
  auto endMargin = YGFlexDirectionIsRow(resolvedAxis)
      ? YGNode::computeEdgeValueForRow(
            margin_,
            YGEdgeEnd,
            trailing[resolvedAxis],
            facebook::yoga::detail::CompactValue::ofZero())
      : YGNode::computeEdgeValueForColumn(
            margin_,
            trailing[resolvedAxis],
            facebook::yoga::detail::CompactValue::ofZero());
  return unwrapOrZero(YGResolveValueMargin(endMargin, widthSize));
}

float YGStyle::computeInlineStartPadding(
    YGFlexDirection axis,
    YGDirection direction,
    float widthSize) const {
  const YGFlexDirection resolvedAxis = YGResolveFlexDirection(axis, direction);
  auto startPadding = YGFlexDirectionIsRow(resolvedAxis)
      ? YGNode::computeEdgeValueForRow(
            padding_,
            YGEdgeStart,
            leading[resolvedAxis],
            facebook::yoga::detail::CompactValue::ofZero())
      : YGNode::computeEdgeValueForColumn(
            padding_,
            leading[resolvedAxis],
            facebook::yoga::detail::CompactValue::ofZero());
  return YGFloatMax(
      unwrapOrZero(YGResolveValue(startPadding, widthSize)), 0.0f);
}

float YGStyle::computeInlineStartBorder(
    YGFlexDirection axis,
    YGDirection direction) const {
  const YGFlexDirection resolvedAxis = YGResolveFlexDirection(axis, direction);
  auto startBorder = YGFlexDirectionIsRow(resolvedAxis)
      ? YGNode::computeEdgeValueForRow(
            border_,
            YGEdgeStart,
            leading[resolvedAxis],
            facebook::yoga::detail::CompactValue::ofZero())
      : YGNode::computeEdgeValueForColumn(
            border_,
            leading[resolvedAxis],
            facebook::yoga::detail::CompactValue::ofZero());
  return YGFloatMax(unwrapOrZero(YGResolveValue(startBorder, 0.0f)), 0.0f);
}
