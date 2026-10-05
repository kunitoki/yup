/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// rive: compatibility layer for the yoga 3.x grid algorithm backport
// (facebook/yoga #1894). Bridges the 3.x internal API surface the grid
// code was written against onto the 2.0.1 equivalents.

#pragma once

#include "yoga/Utils.h"
#include "yoga/YGNode.h"
#include "yoga/Yoga-internal.h"
#include "yoga/Yoga.h"
#include "yoga/event/event.h"

// Yoga.cpp internals exposed for the grid algorithm (previously static)
float YGNodeBoundAxis(
    YGNodeRef node,
    YGFlexDirection axis,
    float value,
    float axisSize,
    float widthSize);
float YGNodePaddingAndBorderForAxis(
    YGNodeConstRef node,
    YGFlexDirection axis,
    float widthSize);
YGAlign YGNodeAlignItem(const YGNode* node, const YGNode* child);
void YGConstrainMaxSizeForMode(
    YGNodeConstRef node,
    YGFlexDirection axis,
    float ownerAxisSize,
    float ownerWidth,
    YGMeasureMode* mode,
    float* size);
float YGNodeCalculateAvailableInnerDim(
    YGNodeConstRef node,
    YGDimension dimension,
    float availableDim,
    float paddingAndBorder,
    float ownerDim);
float YGBaseline(YGNodeRef node, void* layoutContext);
void YGZeroOutLayoutRecursively(YGNodeRef node, void* layoutContext);
void YGNodeSetChildTrailingPosition(
    YGNodeRef node,
    YGNodeRef child,
    YGFlexDirection axis);
void YGNodeAbsoluteLayoutChild(
    YGNodeRef node,
    YGNodeRef child,
    float width,
    YGMeasureMode widthMode,
    float height,
    YGDirection direction,
    YGConfigRef config,
    facebook::yoga::LayoutData& layoutMarkerData,
    void* layoutContext,
    uint32_t depth,
    uint32_t generationCount);
bool YGLayoutNodeInternal(
    YGNodeRef node,
    float availableWidth,
    float availableHeight,
    YGDirection ownerDirection,
    YGMeasureMode widthMeasureMode,
    YGMeasureMode heightMeasureMode,
    float ownerWidth,
    float ownerHeight,
    bool performLayout,
    facebook::yoga::LayoutPassReason reason,
    YGConfigRef config,
    facebook::yoga::LayoutData& layoutMarkerData,
    void* layoutContext,
    uint32_t depth,
    uint32_t generationCount);

namespace facebook {
namespace yoga {

inline bool isDefined(float value) {
  return !isUndefined(value);
}

inline bool inexactEquals(float a, float b) {
  return YGFloatsEqual(a, b);
}

// axis -> dimension (3.x FlexDirection.h)
inline YGDimension dimension(YGFlexDirection axis) {
  return (axis == YGFlexDirectionRow || axis == YGFlexDirectionRowReverse)
      ? YGDimensionWidth
      : YGDimensionHeight;
}

// 3.x signature carries direction; 2.x bounding is direction-independent
inline float boundAxis(
    YGNodeRef node,
    YGFlexDirection axis,
    YGDirection /*direction*/,
    float value,
    float axisSize,
    float widthSize) {
  return YGNodeBoundAxis(node, axis, value, axisSize, widthSize);
}

inline YGJustify resolveChildJustification(
    const YGNode* node,
    const YGNode* child) {
  return child->style().justifySelf() == YGJustifyAuto
      ? node->style().justifyItems()
      : child->style().justifySelf();
}

// Fallback alignment to use on overflow
// https://www.w3.org/TR/css-align-3/#distribution-values
constexpr YGAlign fallbackAlignment(YGAlign align) {
  switch (align) {
    case YGAlignSpaceBetween:
    case YGAlignStretch:
    case YGAlignSpaceAround:
    case YGAlignSpaceEvenly:
      return YGAlignFlexStart;
    default:
      return align;
  }
}

constexpr YGJustify fallbackAlignment(YGJustify align) {
  switch (align) {
    case YGJustifySpaceBetween:
    case YGJustifySpaceAround:
    case YGJustifySpaceEvenly:
      return YGJustifyFlexStart;
    default:
      return align;
  }
}

inline void constrainMaxSizeForMode(
    YGNodeConstRef node,
    YGDirection /*direction*/,
    YGFlexDirection axis,
    float ownerAxisSize,
    float ownerWidth,
    YGMeasureMode* mode,
    float* size) {
  YGConstrainMaxSizeForMode(node, axis, ownerAxisSize, ownerWidth, mode, size);
}

inline float calculateAvailableInnerDimension(
    YGNodeConstRef node,
    YGDirection /*direction*/,
    YGDimension dimension,
    float availableDim,
    float paddingAndBorder,
    float ownerDim,
    float /*ownerWidth*/) {
  return YGNodeCalculateAvailableInnerDim(
      node, dimension, availableDim, paddingAndBorder, ownerDim);
}

inline float paddingAndBorderForAxis(
    YGNodeConstRef node,
    YGFlexDirection axis,
    YGDirection /*direction*/,
    float widthSize) {
  return YGNodePaddingAndBorderForAxis(node, axis, widthSize);
}

inline YGAlign resolveChildAlignment(const YGNode* node, const YGNode* child) {
  return YGNodeAlignItem(node, child);
}

// Flip a position to be relative to the opposite edge (RTL handling)
inline float getPositionOfOppositeEdge(
    float position,
    YGFlexDirection axis,
    const YGNode* containerNode,
    const YGNode* node) {
  return containerNode->getLayout()
             .measuredDimensions[dimension(axis)] -
      position - node->getLayout().measuredDimensions[dimension(axis)];
}

} // namespace yoga
} // namespace facebook
