/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <type_traits>

#include <yoga/Yoga.h>

#include "CompactValue.h"
#include "YGFloatOptional.h"
#include "YGGridTrack.h"
#include "Yoga-internal.h"
#include "BitUtils.h"

namespace facebook::yoga {

// rive: grid style is boxed out of YGStyle so non-grid nodes (the common case)
// don't carry ~128B of grid track/line fields. Only allocated when a grid or
// stack property is actually set; a null box means all-default.
struct GridStyle {
  GridTrackList gridTemplateColumns = {};
  GridTrackList gridTemplateRows = {};
  GridTrackList gridAutoColumns = {};
  GridTrackList gridAutoRows = {};
  GridLine gridColumnStart = {};
  GridLine gridColumnEnd = {};
  GridLine gridRowStart = {};
  GridLine gridRowEnd = {};
};

namespace detail {
// A unique_ptr that deep-copies its pointee, so the owning struct stays
// value-copyable (YGStyle is cloned by value) without a hand-written copy ctor.
template <typename T>
struct CopyableBox {
  std::unique_ptr<T> ptr;
  CopyableBox() = default;
  CopyableBox(const CopyableBox& other)
      : ptr(other.ptr ? std::make_unique<T>(*other.ptr) : nullptr) {}
  CopyableBox& operator=(const CopyableBox& other) {
    ptr = other.ptr ? std::make_unique<T>(*other.ptr) : nullptr;
    return *this;
  }
  CopyableBox(CopyableBox&&) = default;
  CopyableBox& operator=(CopyableBox&&) = default;
};
} // namespace detail

} // namespace facebook::yoga

class YOGA_EXPORT YGStyle {
  template <typename Enum>
  using Values =
      facebook::yoga::detail::Values<facebook::yoga::enums::count<Enum>()>;
  using CompactValue = facebook::yoga::detail::CompactValue;

public:
  using Dimensions = Values<YGDimension>;
  using Edges = Values<YGEdge>;
  using Gutters = Values<YGGutter>;

  template <typename T>
  struct BitfieldRef {
    YGStyle& style;
    size_t offset;
    operator T() const {
      return facebook::yoga::detail::getEnumData<T>(style.flags, offset);
    }
    BitfieldRef<T>& operator=(T x) {
      facebook::yoga::detail::setEnumData<T>(style.flags, offset, x);
      return *this;
    }
  };

  template <typename T, T YGStyle::*Prop>
  struct Ref {
    YGStyle& style;
    operator T() const { return style.*Prop; }
    Ref<T, Prop>& operator=(T value) {
      style.*Prop = value;
      return *this;
    }
  };

  template <typename Idx, Values<Idx> YGStyle::*Prop>
  struct IdxRef {
    struct Ref {
      YGStyle& style;
      Idx idx;
      operator CompactValue() const { return (style.*Prop)[idx]; }
      operator YGValue() const { return (style.*Prop)[idx]; }
      Ref& operator=(CompactValue value) {
        (style.*Prop)[idx] = value;
        return *this;
      }
    };

    YGStyle& style;
    IdxRef<Idx, Prop>& operator=(const Values<Idx>& values) {
      style.*Prop = values;
      return *this;
    }
    operator const Values<Idx>&() const { return style.*Prop; }
    Ref operator[](Idx idx) { return {style, idx}; }
    CompactValue operator[](Idx idx) const { return (style.*Prop)[idx]; }
  };

  YGStyle() {
    alignContent() = YGAlignFlexStart;
    alignItems() = YGAlignStretch;
  }
  ~YGStyle() = default;

private:
  static constexpr size_t directionOffset = 0;
  static constexpr size_t flexdirectionOffset =
      directionOffset + facebook::yoga::detail::bitWidthFn<YGDirection>();
  static constexpr size_t justifyContentOffset = flexdirectionOffset +
      facebook::yoga::detail::bitWidthFn<YGFlexDirection>();
  static constexpr size_t alignContentOffset =
      justifyContentOffset + facebook::yoga::detail::bitWidthFn<YGJustify>();
  static constexpr size_t alignItemsOffset =
      alignContentOffset + facebook::yoga::detail::bitWidthFn<YGAlign>();
  static constexpr size_t alignSelfOffset =
      alignItemsOffset + facebook::yoga::detail::bitWidthFn<YGAlign>();
  static constexpr size_t positionTypeOffset =
      alignSelfOffset + facebook::yoga::detail::bitWidthFn<YGAlign>();
  static constexpr size_t flexWrapOffset =
      positionTypeOffset + facebook::yoga::detail::bitWidthFn<YGPositionType>();
  static constexpr size_t overflowOffset =
      flexWrapOffset + facebook::yoga::detail::bitWidthFn<YGWrap>();
  static constexpr size_t displayOffset =
      overflowOffset + facebook::yoga::detail::bitWidthFn<YGOverflow>();

  uint32_t flags = 0;

  YGFloatOptional flex_ = {};
  YGFloatOptional flexGrow_ = {};
  YGFloatOptional flexShrink_ = {};
  CompactValue flexBasis_ = CompactValue::ofAuto();
  Edges margin_ = {};
  Edges position_ = {};
  Edges padding_ = {};
  Edges border_ = {};
  Gutters gap_ = {};
  Dimensions dimensions_{CompactValue::ofAuto()};
  Dimensions minDimensions_ = {};
  Dimensions maxDimensions_ = {};
  // Yoga specific properties, not compatible with flexbox specification
  YGFloatOptional aspectRatio_ = {};

  // rive: grid style backport (facebook/yoga PR #1893). justifyItems/Self are
  // small (used by grid + stack) so stay inline; the heavy track/line fields
  // are boxed (see GridStyle) so non-grid nodes don't pay for them.
  YGJustify justifyItems_ = YGJustifyStretch;
  YGJustify justifySelf_ = YGJustifyAuto;
  facebook::yoga::detail::CopyableBox<facebook::yoga::GridStyle> gridStyle_;

  // Allocate the grid box on first write; non-grid nodes never call this.
  facebook::yoga::GridStyle& ensureGridStyle() {
    if (!gridStyle_.ptr) {
      gridStyle_.ptr = std::make_unique<facebook::yoga::GridStyle>();
    }
    return *gridStyle_.ptr;
  }

public:
  // for library users needing a type
  using ValueRepr = std::remove_reference<decltype(margin_[0])>::type;

  YGDirection direction() const {
    return facebook::yoga::detail::getEnumData<YGDirection>(
        flags, directionOffset);
  }
  BitfieldRef<YGDirection> direction() { return {*this, directionOffset}; }

  YGFlexDirection flexDirection() const {
    return facebook::yoga::detail::getEnumData<YGFlexDirection>(
        flags, flexdirectionOffset);
  }
  BitfieldRef<YGFlexDirection> flexDirection() {
    return {*this, flexdirectionOffset};
  }

  YGJustify justifyContent() const {
    return facebook::yoga::detail::getEnumData<YGJustify>(
        flags, justifyContentOffset);
  }
  BitfieldRef<YGJustify> justifyContent() {
    return {*this, justifyContentOffset};
  }

  YGAlign alignContent() const {
    return facebook::yoga::detail::getEnumData<YGAlign>(
        flags, alignContentOffset);
  }
  BitfieldRef<YGAlign> alignContent() { return {*this, alignContentOffset}; }

  YGAlign alignItems() const {
    return facebook::yoga::detail::getEnumData<YGAlign>(
        flags, alignItemsOffset);
  }
  BitfieldRef<YGAlign> alignItems() { return {*this, alignItemsOffset}; }

  YGAlign alignSelf() const {
    return facebook::yoga::detail::getEnumData<YGAlign>(flags, alignSelfOffset);
  }
  BitfieldRef<YGAlign> alignSelf() { return {*this, alignSelfOffset}; }

  YGPositionType positionType() const {
    return facebook::yoga::detail::getEnumData<YGPositionType>(
        flags, positionTypeOffset);
  }
  BitfieldRef<YGPositionType> positionType() {
    return {*this, positionTypeOffset};
  }

  YGWrap flexWrap() const {
    return facebook::yoga::detail::getEnumData<YGWrap>(flags, flexWrapOffset);
  }
  BitfieldRef<YGWrap> flexWrap() { return {*this, flexWrapOffset}; }

  YGOverflow overflow() const {
    return facebook::yoga::detail::getEnumData<YGOverflow>(
        flags, overflowOffset);
  }
  BitfieldRef<YGOverflow> overflow() { return {*this, overflowOffset}; }

  YGDisplay display() const {
    return facebook::yoga::detail::getEnumData<YGDisplay>(flags, displayOffset);
  }
  BitfieldRef<YGDisplay> display() { return {*this, displayOffset}; }

  YGFloatOptional flex() const { return flex_; }
  Ref<YGFloatOptional, &YGStyle::flex_> flex() { return {*this}; }

  YGFloatOptional flexGrow() const { return flexGrow_; }
  Ref<YGFloatOptional, &YGStyle::flexGrow_> flexGrow() { return {*this}; }

  YGFloatOptional flexShrink() const { return flexShrink_; }
  Ref<YGFloatOptional, &YGStyle::flexShrink_> flexShrink() { return {*this}; }

  CompactValue flexBasis() const { return flexBasis_; }
  Ref<CompactValue, &YGStyle::flexBasis_> flexBasis() { return {*this}; }

  const Edges& margin() const { return margin_; }
  IdxRef<YGEdge, &YGStyle::margin_> margin() { return {*this}; }

  const Edges& position() const { return position_; }
  IdxRef<YGEdge, &YGStyle::position_> position() { return {*this}; }

  const Edges& padding() const { return padding_; }
  IdxRef<YGEdge, &YGStyle::padding_> padding() { return {*this}; }

  const Edges& border() const { return border_; }
  IdxRef<YGEdge, &YGStyle::border_> border() { return {*this}; }

  const Gutters& gap() const { return gap_; }
  IdxRef<YGGutter, &YGStyle::gap_> gap() { return {*this}; }

  const Dimensions& dimensions() const { return dimensions_; }
  IdxRef<YGDimension, &YGStyle::dimensions_> dimensions() { return {*this}; }

  const Dimensions& minDimensions() const { return minDimensions_; }
  IdxRef<YGDimension, &YGStyle::minDimensions_> minDimensions() {
    return {*this};
  }

  const Dimensions& maxDimensions() const { return maxDimensions_; }
  IdxRef<YGDimension, &YGStyle::maxDimensions_> maxDimensions() {
    return {*this};
  }

  // Yoga specific properties, not compatible with flexbox specification
  YGFloatOptional aspectRatio() const { return aspectRatio_; }
  Ref<YGFloatOptional, &YGStyle::aspectRatio_> aspectRatio() { return {*this}; }

  // rive: grid style backport (facebook/yoga PR #1893)
  YGJustify justifyItems() const { return justifyItems_; }
  void setJustifyItems(YGJustify value) { justifyItems_ = value; }

  YGJustify justifySelf() const { return justifySelf_; }
  void setJustifySelf(YGJustify value) { justifySelf_ = value; }

  // Grid Container Properties. Getters return a shared default when the box is
  // unallocated (non-grid node), which reads identically to the old all-default
  // inline fields; setters allocate the box on first write.
  const facebook::yoga::GridTrackList& gridTemplateColumns() const {
    static const facebook::yoga::GridTrackList kEmpty{};
    return gridStyle_.ptr ? gridStyle_.ptr->gridTemplateColumns : kEmpty;
  }
  void setGridTemplateColumns(facebook::yoga::GridTrackList value) {
    ensureGridStyle().gridTemplateColumns = std::move(value);
  }
  void resizeGridTemplateColumns(size_t count) {
    ensureGridStyle().gridTemplateColumns.resize(count);
  }
  void setGridTemplateColumnAt(
      size_t index,
      facebook::yoga::GridTrackSize value) {
    ensureGridStyle().gridTemplateColumns[index] = value;
  }

  const facebook::yoga::GridTrackList& gridTemplateRows() const {
    static const facebook::yoga::GridTrackList kEmpty{};
    return gridStyle_.ptr ? gridStyle_.ptr->gridTemplateRows : kEmpty;
  }
  void setGridTemplateRows(facebook::yoga::GridTrackList value) {
    ensureGridStyle().gridTemplateRows = std::move(value);
  }
  void resizeGridTemplateRows(size_t count) {
    ensureGridStyle().gridTemplateRows.resize(count);
  }
  void setGridTemplateRowAt(size_t index, facebook::yoga::GridTrackSize value) {
    ensureGridStyle().gridTemplateRows[index] = value;
  }

  const facebook::yoga::GridTrackList& gridAutoColumns() const {
    static const facebook::yoga::GridTrackList kEmpty{};
    return gridStyle_.ptr ? gridStyle_.ptr->gridAutoColumns : kEmpty;
  }
  void setGridAutoColumns(facebook::yoga::GridTrackList value) {
    ensureGridStyle().gridAutoColumns = std::move(value);
  }
  void resizeGridAutoColumns(size_t count) {
    ensureGridStyle().gridAutoColumns.resize(count);
  }
  void setGridAutoColumnAt(size_t index, facebook::yoga::GridTrackSize value) {
    ensureGridStyle().gridAutoColumns[index] = value;
  }

  const facebook::yoga::GridTrackList& gridAutoRows() const {
    static const facebook::yoga::GridTrackList kEmpty{};
    return gridStyle_.ptr ? gridStyle_.ptr->gridAutoRows : kEmpty;
  }
  void setGridAutoRows(facebook::yoga::GridTrackList value) {
    ensureGridStyle().gridAutoRows = std::move(value);
  }
  void resizeGridAutoRows(size_t count) {
    ensureGridStyle().gridAutoRows.resize(count);
  }
  void setGridAutoRowAt(size_t index, facebook::yoga::GridTrackSize value) {
    ensureGridStyle().gridAutoRows[index] = value;
  }

  // Grid Item Properties
  const facebook::yoga::GridLine& gridColumnStart() const {
    static const facebook::yoga::GridLine kDefault{};
    return gridStyle_.ptr ? gridStyle_.ptr->gridColumnStart : kDefault;
  }
  void setGridColumnStart(facebook::yoga::GridLine value) {
    ensureGridStyle().gridColumnStart = value;
  }

  const facebook::yoga::GridLine& gridColumnEnd() const {
    static const facebook::yoga::GridLine kDefault{};
    return gridStyle_.ptr ? gridStyle_.ptr->gridColumnEnd : kDefault;
  }
  void setGridColumnEnd(facebook::yoga::GridLine value) {
    ensureGridStyle().gridColumnEnd = value;
  }

  const facebook::yoga::GridLine& gridRowStart() const {
    static const facebook::yoga::GridLine kDefault{};
    return gridStyle_.ptr ? gridStyle_.ptr->gridRowStart : kDefault;
  }
  void setGridRowStart(facebook::yoga::GridLine value) {
    ensureGridStyle().gridRowStart = value;
  }

  const facebook::yoga::GridLine& gridRowEnd() const {
    static const facebook::yoga::GridLine kDefault{};
    return gridStyle_.ptr ? gridStyle_.ptr->gridRowEnd : kDefault;
  }
  void setGridRowEnd(facebook::yoga::GridLine value) {
    ensureGridStyle().gridRowEnd = value;
  }

  // rive: yoga 3.x-style computed accessors used by the grid backport (#1894)
  float computeMarginForAxis(YGFlexDirection axis, float widthSize) const;
  float computeInlineStartMargin(
      YGFlexDirection axis,
      YGDirection direction,
      float widthSize) const;
  float computeInlineEndMargin(
      YGFlexDirection axis,
      YGDirection direction,
      float widthSize) const;
  float computeInlineStartPadding(
      YGFlexDirection axis,
      YGDirection direction,
      float widthSize) const;
  float computeInlineStartBorder(YGFlexDirection axis, YGDirection direction)
      const;
  bool inlineStartMarginIsAuto(YGFlexDirection axis, YGDirection direction)
      const;
  bool inlineEndMarginIsAuto(YGFlexDirection axis, YGDirection direction)
      const;
  float computeGapForDimension(YGDimension dimension, float availableSize)
      const;
  YGFloatOptional resolvedMinDimension(
      YGDirection direction,
      YGDimension dimension,
      float referenceLength,
      float ownerWidth) const;
  YGFloatOptional resolvedMaxDimension(
      YGDirection direction,
      YGDimension dimension,
      float referenceLength,
      float ownerWidth) const;
  facebook::yoga::StyleSizeLength dimension(YGDimension dimension) const;
  facebook::yoga::StyleSizeLength minDimension(YGDimension dimension) const;
  facebook::yoga::StyleSizeLength maxDimension(YGDimension dimension) const;
};

YOGA_EXPORT bool operator==(const YGStyle& lhs, const YGStyle& rhs);
YOGA_EXPORT inline bool operator!=(const YGStyle& lhs, const YGStyle& rhs) {
  return !(lhs == rhs);
}
