/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// rive: backport of yoga 3.x grid style types (facebook/yoga PR #1893)
// StyleSizeLength is a C++17 reimplementation of the 3.x type of the same
// name, limited to the units the grid implementation uses.

#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include "yoga/YGEnums.h"
#include "yoga/YGFloatOptional.h"

namespace facebook {
namespace yoga {

class StyleSizeLength {
public:
  enum class Unit : uint8_t {
    Undefined,
    Auto,
    Points,
    Percent,
    Stretch, // fr
  };

  constexpr StyleSizeLength() = default;

  constexpr static StyleSizeLength ofUndefined() {
    return StyleSizeLength{};
  }

  constexpr static StyleSizeLength ofAuto() {
    return StyleSizeLength{Unit::Auto, 0.0f};
  }

  constexpr static StyleSizeLength points(float value) {
    return StyleSizeLength{Unit::Points, value};
  }

  constexpr static StyleSizeLength percent(float value) {
    return StyleSizeLength{Unit::Percent, value};
  }

  constexpr static StyleSizeLength stretch(float value) {
    return StyleSizeLength{Unit::Stretch, value};
  }

  constexpr Unit unit() const { return unit_; }

  constexpr bool isUndefined() const { return unit_ == Unit::Undefined; }
  constexpr bool isDefined() const { return unit_ != Unit::Undefined; }
  constexpr bool isAuto() const { return unit_ == Unit::Auto; }
  constexpr bool isPoints() const { return unit_ == Unit::Points; }
  constexpr bool isPercent() const { return unit_ == Unit::Percent; }
  constexpr bool isStretch() const { return unit_ == Unit::Stretch; }

  YGFloatOptional value() const {
    return (unit_ == Unit::Points || unit_ == Unit::Percent ||
            unit_ == Unit::Stretch)
        ? YGFloatOptional{value_}
        : YGFloatOptional{};
  }

  YGFloatOptional resolve(float referenceLength) const {
    switch (unit_) {
      case Unit::Points:
        return YGFloatOptional{value_};
      case Unit::Percent:
        return std::isnan(referenceLength)
            ? YGFloatOptional{}
            : YGFloatOptional{value_ * referenceLength * 0.01f};
      default:
        return YGFloatOptional{};
    }
  }

  bool operator==(const StyleSizeLength& other) const {
    return unit_ == other.unit_ &&
        (unit_ == Unit::Undefined || unit_ == Unit::Auto ||
         value_ == other.value_);
  }
  bool operator!=(const StyleSizeLength& other) const {
    return !(*this == other);
  }

private:
  constexpr StyleSizeLength(Unit unit, float value)
      : unit_(unit), value_(value) {}

  Unit unit_ = Unit::Undefined;
  float value_ = 0.0f;
};

// https://www.w3.org/TR/css-grid-1/#typedef-track-size
struct GridTrackSize {
  StyleSizeLength minSizingFunction;
  StyleSizeLength maxSizingFunction;

  constexpr GridTrackSize() = default;
  constexpr GridTrackSize(StyleSizeLength min, StyleSizeLength max)
      : minSizingFunction(min), maxSizingFunction(max) {}

  constexpr static GridTrackSize auto_() {
    return GridTrackSize{StyleSizeLength::ofAuto(), StyleSizeLength::ofAuto()};
  }

  constexpr static GridTrackSize length(float points) {
    return GridTrackSize{
        StyleSizeLength::points(points), StyleSizeLength::points(points)};
  }

  constexpr static GridTrackSize fr(float fraction) {
    // Flex sizing function is always a max sizing function
    return GridTrackSize{
        StyleSizeLength::ofAuto(), StyleSizeLength::stretch(fraction)};
  }

  constexpr static GridTrackSize percent(float percentage) {
    return GridTrackSize{
        StyleSizeLength::percent(percentage),
        StyleSizeLength::percent(percentage)};
  }

  constexpr static GridTrackSize minmax(
      StyleSizeLength min,
      StyleSizeLength max) {
    return GridTrackSize{min, max};
  }

  bool operator==(const GridTrackSize& other) const {
    return minSizingFunction == other.minSizingFunction &&
        maxSizingFunction == other.maxSizingFunction;
  }
  bool operator!=(const GridTrackSize& other) const {
    return !(*this == other);
  }
};

// Grid track list for grid-template-rows/columns properties
using GridTrackList = std::vector<GridTrackSize>;

// https://www.w3.org/TR/css-grid-1/#typedef-grid-row-start-grid-line
enum class GridLineType : uint8_t {
  Auto,
  Integer,
  Span,
};

struct GridLine {
  GridLineType type = GridLineType::Auto;
  // Line position (1, 2, -1, -2, etc)
  int32_t integer = 0;

  constexpr GridLine() = default;
  constexpr GridLine(GridLineType t, int32_t i) : type(t), integer(i) {}

  constexpr static GridLine auto_() {
    return GridLine{GridLineType::Auto, 0};
  }

  constexpr static GridLine fromInteger(int32_t value) {
    return GridLine{GridLineType::Integer, value};
  }

  constexpr static GridLine span(int32_t value) {
    return GridLine{GridLineType::Span, value};
  }

  constexpr bool isAuto() const { return type == GridLineType::Auto; }
  constexpr bool isInteger() const { return type == GridLineType::Integer; }
  constexpr bool isSpan() const { return type == GridLineType::Span; }

  bool operator==(const GridLine& other) const {
    return type == other.type && integer == other.integer;
  }
  bool operator!=(const GridLine& other) const { return !(*this == other); }
};

} // namespace yoga
} // namespace facebook

// 2.x convention: public yoga types live in the global namespace (YGStyle,
// YGNode). Aliases so consumers don't need facebook::yoga::.
// (GridTrackList deliberately has no alias; YGGridTrackList is taken by the
// C API's opaque list builder.)
using YGStyleSizeLength = facebook::yoga::StyleSizeLength;
using YGGridTrackSize = facebook::yoga::GridTrackSize;
using YGGridLine = facebook::yoga::GridLine;
using YGGridLineType = facebook::yoga::GridLineType;
