/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "yoga/grid/GridLayout.h"
#include "yoga/grid/TrackSizing.h"

namespace facebook::yoga {

// rive: 2.x has no static-position containing blocks, so absolute layout only
// applies to direct children (same scope as the 2.x flex path)
static void layoutAbsoluteDescendants(
    YGNodeRef containingNode,
    YGNodeRef currentNode,
    YGMeasureMode widthSizingMode,
    YGDirection direction,
    YGConfigRef config,
    void* layoutContext,
    LayoutData& layoutMarkerData,
    uint32_t depth,
    uint32_t generationCount,
    float /*offsetX*/,
    float /*offsetY*/,
    float availableInnerWidth,
    float availableInnerHeight) {
  for (auto child : currentNode->getChildren()) {
    if (child->getStyle().positionType() == YGPositionTypeAbsolute) {
      // match the 2.x flex path: setPosition applies inset-based positions
      // before the absolute pass handles trailing/centered cases
      child->setPosition(
          child->resolveDirection(direction),
          availableInnerHeight,
          availableInnerWidth,
          availableInnerWidth);
      YGNodeAbsoluteLayoutChild(
          containingNode,
          child,
          availableInnerWidth,
          widthSizingMode,
          availableInnerHeight,
          direction,
          config,
          layoutMarkerData,
          layoutContext,
          depth,
          generationCount);
      // match flex STEP 11: convert reversed-axis positions to physical edges
      const YGFlexDirection absMainAxis = YGResolveFlexDirection(
          containingNode->getStyle().flexDirection(), direction);
      const YGFlexDirection absCrossAxis =
          YGFlexDirectionCross(absMainAxis, direction);
      if (absMainAxis == YGFlexDirectionRowReverse ||
          absMainAxis == YGFlexDirectionColumnReverse) {
        YGNodeSetChildTrailingPosition(containingNode, child, absMainAxis);
      }
      if (absCrossAxis == YGFlexDirectionRowReverse ||
          absCrossAxis == YGFlexDirectionColumnReverse) {
        YGNodeSetChildTrailingPosition(containingNode, child, absCrossAxis);
      }
    }
  }
}

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
    uint32_t generationCount) {
  (void)reason; // Unused parameter

  const auto& nodeStyle = node->style();
  const YGDirection direction = node->resolveDirection(ownerDirection);
  const float marginInline =
      nodeStyle.computeMarginForAxis(YGFlexDirectionRow, ownerWidth);
  const float marginBlock =
      nodeStyle.computeMarginForAxis(YGFlexDirectionColumn, ownerWidth);
  const float paddingAndBorderInline =
      paddingAndBorderForAxis(node, YGFlexDirectionRow, direction, ownerWidth);
  const float paddingAndBorderBlock = paddingAndBorderForAxis(
      node, YGFlexDirectionColumn, direction, ownerWidth);
  const float availableInnerWidth = calculateAvailableInnerDimension(
      node,
      direction,
      YGDimensionWidth,
      availableWidth - marginInline,
      paddingAndBorderInline,
      ownerWidth,
      ownerWidth);
  const float availableInnerHeight = calculateAvailableInnerDimension(
      node,
      direction,
      YGDimensionHeight,
      availableHeight - marginBlock,
      paddingAndBorderBlock,
      ownerHeight,
      ownerWidth);
  auto widthIsDefinite =
      (widthSizingMode == YGMeasureModeExactly &&
       isDefined(availableWidth));
  auto heightIsDefinite =
      (heightSizingMode == YGMeasureModeExactly &&
       isDefined(availableHeight));

  // 11. Grid Layout Algorithm
  // Step 1: Run the Grid Item Placement Algorithm to resolve the placement of
  // all grid items in the grid.
  auto autoPlacement = ResolvedAutoPlacement::resolveGridItemPlacements(node);
  // Create the grid tracks (auto and explicit = implicit grid)
  auto gridTracks = createGridTracks(node, autoPlacement);
  // At this point, we have grid items final positions and implicit grid tracks

  // Step 2: Find the size of the grid container, per § 5.2 Sizing Grid
  // Containers. If grid container size is not definite, we have to run the
  // track sizing algorithm to find the size of the grid container. Note: During
  // this phase, cyclic <percentage>s in track sizes are treated as auto.
  float containerInnerWidth =
      widthIsDefinite ? availableInnerWidth : YGUndefined;
  float containerInnerHeight =
      heightIsDefinite ? availableInnerHeight : YGUndefined;
  auto& rowTracks = gridTracks.rowTracks;
  auto& columnTracks = gridTracks.columnTracks;
  auto& gridItems = autoPlacement.gridItems;
  bool needsSecondTrackSizingPass = true;

  if (!widthIsDefinite || !heightIsDefinite) {
    auto trackSizing = TrackSizing(
        node,
        columnTracks,
        rowTracks,
        containerInnerWidth,
        containerInnerHeight,
        gridItems,
        widthSizingMode,
        heightSizingMode,
        direction,
        ownerWidth,
        ownerHeight,
        layoutMarkerData,
        depth,
        generationCount,
        config,
        layoutContext);

    trackSizing.runGridSizingAlgorithm();

    bool containerSizeChanged = false;

    if (!widthIsDefinite) {
      auto totalTrackWidth = trackSizing.getTotalBaseSize(YGDimensionWidth);
      containerInnerWidth = boundAxis(
          node,
          YGFlexDirectionRow,
          direction,
          totalTrackWidth,
          ownerWidth,
          ownerWidth);
      if (containerInnerWidth != totalTrackWidth) {
        containerSizeChanged = true;
      }
    }

    if (!heightIsDefinite) {
      auto totalTrackHeight = trackSizing.getTotalBaseSize(YGDimensionHeight);
      containerInnerHeight = boundAxis(
          node,
          YGFlexDirectionColumn,
          direction,
          totalTrackHeight,
          ownerHeight,
          ownerWidth);
      if (containerInnerHeight != totalTrackHeight) {
        containerSizeChanged = true;
      }
    }

    // We need to run track sizing again if:
    // 1. The container size changed due to min/max bounds or
    // 2. There are percentage tracks in indefinite dimensions that need
    // resolution
    bool hasPercentageTracksNeedingResolution =
        (!widthIsDefinite &&
         trackSizing.hasPercentageTracks(YGDimensionWidth)) ||
        (!heightIsDefinite &&
         trackSizing.hasPercentageTracks(YGDimensionHeight));
    needsSecondTrackSizingPass =
        containerSizeChanged || hasPercentageTracksNeedingResolution;
  }

  node->setLayoutMeasuredDimension(
      boundAxis(
          node,
          YGFlexDirectionRow,
          direction,
          containerInnerWidth + paddingAndBorderInline,
          ownerWidth,
          ownerWidth),
      YGDimensionWidth);

  node->setLayoutMeasuredDimension(
      boundAxis(
          node,
          YGFlexDirectionColumn,
          direction,
          containerInnerHeight + paddingAndBorderBlock,
          ownerHeight,
          ownerWidth),
      YGDimensionHeight);

  // If we are not performing layout, we can return early after sizing the grid
  // container.
  if (!performLayout) {
    return;
  }

  // Inititialize track sizing with the final container size
  auto trackSizing = TrackSizing(
      node,
      columnTracks,
      rowTracks,
      containerInnerWidth,
      containerInnerHeight,
      gridItems,
      widthSizingMode,
      heightSizingMode,
      direction,
      ownerWidth,
      ownerHeight,
      layoutMarkerData,
      depth,
      generationCount,
      config,
      layoutContext);

  // Step 3: Given the resulting grid container size, run the Grid Sizing
  // Algorithm to size the grid. Run track sizing with the new container
  // dimensions Note: During this phase, <percentage>s in track sizes are
  // resolved against the grid container size.

  // We only need to run track sizing again if:
  // 1. Both dimensions were definite or
  // 2. The container size changed due to min/max constraints in Step 2, or
  // 3. There are percentage tracks in indefinite dimensions that need
  // resolution
  if (needsSecondTrackSizingPass) {
    trackSizing.runGridSizingAlgorithm();
  }

  // Step 4: Lay out the grid items into their respective containing blocks.
  // Each grid area’s width and height are considered definite for this purpose.
  auto gridWidth = trackSizing.getTotalBaseSize(YGDimensionWidth);
  auto gridHeight = trackSizing.getTotalBaseSize(YGDimensionHeight);

  float leadingPaddingAndBorderInline =
      nodeStyle.computeInlineStartPadding(
          YGFlexDirectionRow, direction, ownerWidth) +
      nodeStyle.computeInlineStartBorder(YGFlexDirectionRow, direction);
  float leadingPaddingAndBorderBlock =
      nodeStyle.computeInlineStartPadding(
          YGFlexDirectionColumn, direction, ownerWidth) +
      nodeStyle.computeInlineStartBorder(YGFlexDirectionColumn, direction);

  // YGAlign content/YGJustify content
  float freeSpaceInlineAxis = containerInnerWidth - gridWidth;
  auto inlineDistribution = trackSizing.calculateContentDistribution(
      YGDimensionWidth, freeSpaceInlineAxis);
  float freeSpaceBlockAxis = containerInnerHeight - gridHeight;
  auto blockDistribution = trackSizing.calculateContentDistribution(
      YGDimensionHeight, freeSpaceBlockAxis);

  if (freeSpaceInlineAxis < 0.0f || freeSpaceBlockAxis < 0.0f) {
    node->setLayoutHadOverflow(true);
  }

  auto gridInlineStartOffset = inlineDistribution.startOffset;
  auto gridBlockStartOffset = blockDistribution.startOffset;
  auto finalEffectiveColumnGap = inlineDistribution.effectiveGap;
  auto finalEffectiveRowGap = blockDistribution.effectiveGap;

  std::vector<float> columnGridLineOffsets;
  columnGridLineOffsets.reserve(columnTracks.size() + 1);
  columnGridLineOffsets.push_back(0.0f);
  for (size_t i = 0; i < columnTracks.size(); i++) {
    float offset = columnGridLineOffsets[i] + columnTracks[i].baseSize;
    if (i < columnTracks.size() - 1) {
      offset += finalEffectiveColumnGap;
    }
    columnGridLineOffsets.push_back(offset);
  }

  std::vector<float> rowGridLineOffsets;
  rowGridLineOffsets.reserve(rowTracks.size() + 1);
  rowGridLineOffsets.push_back(0.0f);
  for (size_t i = 0; i < rowTracks.size(); i++) {
    float offset = rowGridLineOffsets[i] + rowTracks[i].baseSize;
    if (i < rowTracks.size() - 1) {
      offset += finalEffectiveRowGap;
    }
    rowGridLineOffsets.push_back(offset);
  }

  // Persist the resolved grid line positions in local content space (matching
  // child layout positions) so tooling can map a point back to a cell. The raw
  // offsets are relative to the grid content box; shift by the leading
  // padding/border and the grid's alignment start offset to land in the same
  // space as each item's layout position (see finalLeft/finalTop below).
  auto& gridLayout = node->getLayout();
  gridLayout.gridColumnLineOffsets.clear();
  gridLayout.gridColumnLineOffsets.reserve(columnGridLineOffsets.size());
  for (float o : columnGridLineOffsets) {
    gridLayout.gridColumnLineOffsets.push_back(
        leadingPaddingAndBorderInline + gridInlineStartOffset + o);
  }
  gridLayout.gridRowLineOffsets.clear();
  gridLayout.gridRowLineOffsets.reserve(rowGridLineOffsets.size());
  for (float o : rowGridLineOffsets) {
    gridLayout.gridRowLineOffsets.push_back(
        leadingPaddingAndBorderBlock + gridBlockStartOffset + o);
  }

  for (auto& item : gridItems) {
    // grid line offsets include the gap after each track (except the last).
    // so we subtract the trailing gap for items that do not end at the last
    // track.
    float containingBlockWidth = columnGridLineOffsets[item.columnEnd] -
        columnGridLineOffsets[item.columnStart];
    if (item.columnEnd < columnTracks.size()) {
      containingBlockWidth -= finalEffectiveColumnGap;
    }
    float containingBlockHeight =
        rowGridLineOffsets[item.rowEnd] - rowGridLineOffsets[item.rowStart];
    if (item.rowEnd < rowTracks.size()) {
      containingBlockHeight -= finalEffectiveRowGap;
    }
    float gridItemInlineStart = columnGridLineOffsets[item.columnStart];
    float gridItemBlockStart = rowGridLineOffsets[item.rowStart];
    const auto& itemStyle = item.node->style();

    const auto marginInlineStart = itemStyle.computeInlineStartMargin(
        YGFlexDirectionRow, direction, containingBlockWidth);
    const auto marginInlineEnd = itemStyle.computeInlineEndMargin(
        YGFlexDirectionRow, direction, containingBlockWidth);
    const auto marginBlockStart = itemStyle.computeInlineStartMargin(
        YGFlexDirectionColumn, direction, containingBlockWidth);
    const auto marginBlockEnd = itemStyle.computeInlineEndMargin(
        YGFlexDirectionColumn, direction, containingBlockWidth);

    auto itemConstraints = trackSizing.calculateItemConstraints(
        item, containingBlockWidth, containingBlockHeight);

    YGLayoutNodeInternal(
        item.node,
        itemConstraints.width,
        itemConstraints.height,
        direction,
        itemConstraints.widthSizingMode,
        itemConstraints.heightSizingMode,
        containingBlockWidth,
        containingBlockHeight,
        true,
        LayoutPassReason::kGridLayout,
            config,
        layoutMarkerData,
            layoutContext,
        depth,
        generationCount);

    auto justifySelf = resolveChildJustification(node, item.node);
    auto alignSelf = resolveChildAlignment(node, item.node);

    // since we know the item width and grid width, we can do the alignment
    // here. alignment of grid items happen in the grid area measured dimension
    // includes padding and border
    float actualItemWidth =
        item.node->getLayout().measuredDimensions[YGDimensionWidth];
    auto freeSpaceInlineAxisItem = containingBlockWidth - actualItemWidth -
        marginInlineStart - marginInlineEnd;
    float startAutoMarginOffset = 0.0f;
    // https://www.w3.org/TR/css-grid-1/#auto-margins
    // auto margins in either axis absorb positive free space prior to alignment
    // via the box alignment properties, thereby disabling the effects of any
    // self-alignment properties in that axis.
    if (freeSpaceInlineAxisItem > 0.0f) {
      if (itemStyle.inlineStartMarginIsAuto(YGFlexDirectionRow, direction) &&
          itemStyle.inlineEndMarginIsAuto(YGFlexDirectionRow, direction)) {
        startAutoMarginOffset = freeSpaceInlineAxisItem / 2;
        freeSpaceInlineAxisItem = 0.0f;
      } else if (itemStyle.inlineStartMarginIsAuto(
                     YGFlexDirectionRow, direction)) {
        startAutoMarginOffset = freeSpaceInlineAxisItem;
        freeSpaceInlineAxisItem = 0.0f;
      } else if (itemStyle.inlineEndMarginIsAuto(
                     YGFlexDirectionRow, direction)) {
        startAutoMarginOffset = 0.0f;
        freeSpaceInlineAxisItem = 0.0f;
      }
    }

    float justifySelfOffset = 0.0f;
    if (justifySelf == YGJustifyEnd) {
      justifySelfOffset = freeSpaceInlineAxisItem;
    } else if (justifySelf == YGJustifyCenter) {
      justifySelfOffset = freeSpaceInlineAxisItem / 2;
    }

    float finalLeft = leadingPaddingAndBorderInline + gridItemInlineStart +
        marginInlineStart + startAutoMarginOffset + justifySelfOffset +
        gridInlineStartOffset;

    if (direction == YGDirectionRTL) {
      finalLeft = getPositionOfOppositeEdge(
          finalLeft, YGFlexDirectionRow, node, item.node);
    }

    // Add relative position offset for relatively positioned items.
    // For RTL, the relative position is in logical coordinates so we subtract
    // it from the physical left.
    float relativePositionInline = item.node->relativePosition(
        YGFlexDirectionRow, direction, containingBlockWidth);
    if (direction == YGDirectionRTL) {
      item.node->setLayoutPosition(
          finalLeft - relativePositionInline, YGEdgeLeft);
    } else {
      item.node->setLayoutPosition(
          finalLeft + relativePositionInline, YGEdgeLeft);
    }

    float actualItemHeight =
        item.node->getLayout().measuredDimensions[YGDimensionHeight];
    auto freeSpaceBlockAxisItem = containingBlockHeight - actualItemHeight -
        marginBlockStart - marginBlockEnd;
    float topAutoMarginOffset = 0.0f;
    if (freeSpaceBlockAxisItem > 0.0f) {
      if (itemStyle.inlineStartMarginIsAuto(YGFlexDirectionColumn, direction) &&
          itemStyle.inlineEndMarginIsAuto(YGFlexDirectionColumn, direction)) {
        topAutoMarginOffset = freeSpaceBlockAxisItem / 2;
        freeSpaceBlockAxisItem = 0.0f;
      } else if (itemStyle.inlineStartMarginIsAuto(
                     YGFlexDirectionColumn, direction)) {
        topAutoMarginOffset = freeSpaceBlockAxisItem;
        freeSpaceBlockAxisItem = 0.0f;
      } else if (itemStyle.inlineEndMarginIsAuto(
                     YGFlexDirectionColumn, direction)) {
        freeSpaceBlockAxisItem = 0.0f;
      }
    }

    float alignSelfOffset = 0.0f;
    if (alignSelf == YGAlignEnd) {
      alignSelfOffset = freeSpaceBlockAxisItem;
    } else if (alignSelf == YGAlignCenter) {
      alignSelfOffset = freeSpaceBlockAxisItem / 2;
    }

    float finalTop = gridItemBlockStart + marginBlockStart +
        topAutoMarginOffset + alignSelfOffset + gridBlockStartOffset +
        leadingPaddingAndBorderBlock;

    // Add relative position offset for relatively positioned items
    float relativePositionBlock = item.node->relativePosition(
        YGFlexDirectionColumn, direction, containingBlockHeight);
    item.node->setLayoutPosition(
        finalTop + relativePositionBlock, YGEdgeTop);
  }

  // Perform layout of absolute children
  // https://www.w3.org/TR/css-grid-1/#abspos
  // TODO: support grid-[row|column]-[start|end] as containing blocks
  if (nodeStyle.positionType() != YGPositionTypeStatic ||
      node->alwaysFormsContainingBlock() || depth == 1) {
    for (auto child : node->getLayoutChildren()) {
      if (child->style().display() == YGDisplayNone) {
        YGZeroOutLayoutRecursively(child, layoutContext);
        child->setHasNewLayout(true);
        child->setDirty(false);
        continue;
      }

      if (child->style().positionType() == YGPositionTypeAbsolute) {
        child->processDimensions();
      }
    }

    layoutAbsoluteDescendants(
        node,
        node,
        widthSizingMode,
        direction,
        config,
        layoutContext,
        layoutMarkerData,
        depth,
        generationCount,
        0.0f,
        0.0f,
        availableInnerWidth,
        availableInnerHeight);
  }
}

GridTracks createGridTracks(
    YGNode* node,
    const ResolvedAutoPlacement& autoPlacement) {
  auto gridExplicitColumns = node->style().gridTemplateColumns();
  auto gridExplicitRows = node->style().gridTemplateRows();

  std::vector<GridTrack> columnTracks;
  std::vector<GridTrack> rowTracks;
  columnTracks.reserve(
      static_cast<size_t>(
          autoPlacement.maxColumnEnd - autoPlacement.minColumnStart));
  rowTracks.reserve(
      static_cast<size_t>(autoPlacement.maxRowEnd - autoPlacement.minRowStart));

  // https://www.w3.org/TR/css-grid-1/#auto-tracks
  auto autoRowTracks = node->style().gridAutoRows().empty()
      ? GridTrackList{GridTrackSize::auto_()}
      : node->style().gridAutoRows();
  auto autoColumnTracks = node->style().gridAutoColumns().empty()
      ? GridTrackList{GridTrackSize::auto_()}
      : node->style().gridAutoColumns();

  // The last implicit grid track before the explicit grid receives the last
  // specified size, and so on backwards. i.e. The pattern repeats backwards
  auto negativeImplicitGridColumnTrackCount = -autoPlacement.minColumnStart;
  auto autoColumnTracksSize = autoColumnTracks.size();
  for (auto i = 0; i < negativeImplicitGridColumnTrackCount; i++) {
    auto currentColumnTrackIndex =
        static_cast<size_t>(negativeImplicitGridColumnTrackCount - i - 1) %
        autoColumnTracksSize;
    auto autoColumnTrack =
        autoColumnTracks[autoColumnTracksSize - currentColumnTrackIndex - 1];
    columnTracks.push_back(GridTrack(autoColumnTrack));
  }

  for (size_t i = 0; i < gridExplicitColumns.size(); i++) {
    columnTracks.push_back(GridTrack(gridExplicitColumns[i]));
  }

  // The first track after the last explicitly-sized track receives the first
  // specified size i.e. the pattern repeats forwards
  for (size_t i = 0; i < static_cast<size_t>(autoPlacement.maxColumnEnd) -
           gridExplicitColumns.size();
       i++) {
    auto autoColumnTrack = autoColumnTracks[i % autoColumnTracksSize];
    columnTracks.push_back(GridTrack(autoColumnTrack));
  }

  auto negativeImplicitGridRowTrackCount = -autoPlacement.minRowStart;
  auto autoRowTracksSize = autoRowTracks.size();
  for (auto i = 0; i < negativeImplicitGridRowTrackCount; i++) {
    auto currentRowTrackIndex =
        static_cast<size_t>(negativeImplicitGridRowTrackCount - i - 1) %
        autoRowTracksSize;
    auto autoRowTrack =
        autoRowTracks[autoRowTracksSize - currentRowTrackIndex - 1];
    rowTracks.push_back(GridTrack(autoRowTrack));
  }
  for (const auto& explicitRow : gridExplicitRows) {
    rowTracks.push_back(GridTrack(explicitRow));
  }
  for (size_t i = 0; i <
       static_cast<size_t>(autoPlacement.maxRowEnd) - gridExplicitRows.size();
       i++) {
    auto autoRowTrack = autoRowTracks[i % autoRowTracksSize];
    rowTracks.push_back(GridTrack(autoRowTrack));
  }

  return {std::move(columnTracks), std::move(rowTracks)};
}

} // namespace facebook::yoga
