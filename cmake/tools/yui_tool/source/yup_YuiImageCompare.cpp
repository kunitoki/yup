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

#include "yup_YuiImageCompare.h"

#include <algorithm>
#include <cmath>

using namespace yup;

namespace
{

constexpr int panelGap = 8;
constexpr uint8 panelBackground = 24;
constexpr int visibleDifference = 24;
constexpr int maximumCells = 8;
constexpr int minimumCellSide = 10;
constexpr double noticeableError = 0.01;
constexpr size_t maximumDifferences = 5;

//==============================================================================
/** An image as three bytes a pixel. */
struct Pixels
{
    Pixels (int widthToUse, int heightToUse)
        : width (widthToUse)
        , height (heightToUse)
        , rgb (static_cast<size_t> (widthToUse) * static_cast<size_t> (heightToUse) * 3)
    {
    }

    uint8* at (int x, int y) noexcept { return rgb.data() + (static_cast<size_t> (y) * static_cast<size_t> (width) + static_cast<size_t> (x)) * 3; }

    const uint8* at (int x, int y) const noexcept { return rgb.data() + (static_cast<size_t> (y) * static_cast<size_t> (width) + static_cast<size_t> (x)) * 3; }

    int width = 0;
    int height = 0;
    std::vector<uint8> rgb;
};

Pixels readPixels (const Image& image, Color background)
{
    Pixels pixels (image.getWidth(), image.getHeight());

    const auto format = image.getPixelFormat();
    const auto stride = image.getPixelStride();
    const uint8 behind[3] { background.getRed(), background.getGreen(), background.getBlue() };

    const auto* source = image.getRawData().data();
    auto* target = pixels.rgb.data();

    for (size_t remaining = pixels.rgb.size() / 3; remaining > 0; --remaining, source += stride, target += 3)
    {
        if (format == PixelFormat::Grayscale)
        {
            target[0] = target[1] = target[2] = source[0];
            continue;
        }

        const int alpha = format == PixelFormat::RGBA ? source[3] : 255;

        for (int channel = 0; channel < 3; ++channel)
            target[channel] = static_cast<uint8> ((source[channel] * alpha + behind[channel] * (255 - alpha) + 127) / 255);
    }

    return pixels;
}

//==============================================================================
/** The pixels of the source a pixel of the result is made of along one axis, with the weight of each. */
struct Taps
{
    int first = 0;
    std::vector<float> weights;
};

std::vector<Taps> findTaps (double start, double length, int count)
{
    std::vector<Taps> taps (static_cast<size_t> (count));
    const auto step = length / count;

    for (int index = 0; index < count; ++index)
    {
        auto& tap = taps[static_cast<size_t> (index)];

        if (step > 1.0)
        {
            // Made smaller, a pixel is the average of what it covers
            const auto from = start + index * step;
            const auto to = from + step;

            tap.first = static_cast<int> (std::floor (from));

            for (auto position = tap.first; position < to; ++position)
                tap.weights.push_back (static_cast<float> ((jmin (to, position + 1.0) - jmax (from, static_cast<double> (position))) / step));
        }
        else
        {
            // Made larger or kept, it blends the two pixels around its middle
            const auto middle = start + (index + 0.5) * step - 0.5;
            const auto fraction = static_cast<float> (middle - std::floor (middle));

            tap.first = static_cast<int> (std::floor (middle));
            tap.weights = { 1.0f - fraction, fraction };
        }
    }

    return taps;
}

/** Brings a part of an image, given in its pixels, to another size. */
Pixels resample (const Pixels& source, const Rectangle<double>& part, int width, int height)
{
    Pixels result (width, height);

    const auto columns = findTaps (part.getX(), part.getWidth(), width);
    const auto rows = findTaps (part.getY(), part.getHeight(), height);

    for (int y = 0; y < height; ++y)
    {
        const auto& row = rows[static_cast<size_t> (y)];

        for (int x = 0; x < width; ++x)
        {
            const auto& column = columns[static_cast<size_t> (x)];
            float sum[3] {};

            for (size_t j = 0; j < row.weights.size(); ++j)
            {
                const auto sourceY = jlimit (0, source.height - 1, row.first + static_cast<int> (j));

                for (size_t i = 0; i < column.weights.size(); ++i)
                {
                    const auto* pixel = source.at (jlimit (0, source.width - 1, column.first + static_cast<int> (i)), sourceY);
                    const auto weight = row.weights[j] * column.weights[i];

                    for (int channel = 0; channel < 3; ++channel)
                        sum[channel] += weight * pixel[channel];
                }
            }

            auto* target = result.at (x, y);

            for (int channel = 0; channel < 3; ++channel)
                target[channel] = static_cast<uint8> (jlimit (0.0f, 255.0f, sum[channel] + 0.5f));
        }
    }

    return result;
}

//==============================================================================
/** The mean difference of each cell of a grid laid over the render. */
struct Cells
{
    Cells (int width, int height)
        : columns (jlimit (1, maximumCells, width / minimumCellSide))
        , rows (jlimit (1, maximumCells, height / minimumCellSide))
        , errors (static_cast<size_t> (columns * rows))
        , counts (static_cast<size_t> (columns * rows))
    {
    }

    size_t indexOf (int column, int row) const noexcept { return static_cast<size_t> (row * columns + column); }

    double meanAt (int column, int row) const noexcept
    {
        const auto index = indexOf (column, row);
        return counts[index] > 0 ? errors[index] / counts[index] : 0.0;
    }

    int columns = 1;
    int rows = 1;
    std::vector<double> errors;
    std::vector<int> counts;
};

/** Joins the cells that differ and touch into the regions holding them, the worst first. */
std::vector<YuiToolComparison::Difference> findDifferences (const Cells& cells, const Rectangle<float>& region)
{
    struct Found
    {
        YuiToolComparison::Difference difference;
        double total = 0.0;
    };

    std::vector<Found> found;
    std::vector<bool> visited (cells.errors.size());

    const auto cellWidth = region.getWidth() / static_cast<float> (cells.columns);
    const auto cellHeight = region.getHeight() / static_cast<float> (cells.rows);

    for (int startRow = 0; startRow < cells.rows; ++startRow)
    {
        for (int startColumn = 0; startColumn < cells.columns; ++startColumn)
        {
            if (visited[cells.indexOf (startColumn, startRow)] || cells.meanAt (startColumn, startRow) < noticeableError)
                continue;

            std::vector<std::pair<int, int>> pending { { startColumn, startRow } };
            visited[cells.indexOf (startColumn, startRow)] = true;

            auto left = startColumn, right = startColumn, top = startRow, bottom = startRow;
            double total = 0.0;
            int count = 0;

            while (! pending.empty())
            {
                const auto [column, row] = pending.back();
                pending.pop_back();

                left = jmin (left, column);
                right = jmax (right, column);
                top = jmin (top, row);
                bottom = jmax (bottom, row);
                total += cells.meanAt (column, row);
                ++count;

                for (const auto& [nextColumn, nextRow] : { std::pair { column - 1, row }, std::pair { column + 1, row }, std::pair { column, row - 1 }, std::pair { column, row + 1 } })
                {
                    if (nextColumn < 0 || nextColumn >= cells.columns || nextRow < 0 || nextRow >= cells.rows)
                        continue;

                    if (visited[cells.indexOf (nextColumn, nextRow)] || cells.meanAt (nextColumn, nextRow) < noticeableError)
                        continue;

                    visited[cells.indexOf (nextColumn, nextRow)] = true;
                    pending.emplace_back (nextColumn, nextRow);
                }
            }

            // Whole pixels, which is what a region to draw is made of
            const auto x = std::floor (region.getX() + static_cast<float> (left) * cellWidth);
            const auto y = std::floor (region.getY() + static_cast<float> (top) * cellHeight);
            const auto farX = std::ceil (region.getX() + static_cast<float> (right + 1) * cellWidth);
            const auto farY = std::ceil (region.getY() + static_cast<float> (bottom + 1) * cellHeight);

            found.push_back ({ { Rectangle<float> (x, y, farX - x, farY - y), total / count }, total });
        }
    }

    std::sort (found.begin(), found.end(), [] (const Found& a, const Found& b)
    {
        return a.total > b.total;
    });

    std::vector<YuiToolComparison::Difference> differences;

    for (size_t index = 0; index < jmin (found.size(), maximumDifferences); ++index)
        differences.push_back (found[index].difference);

    return differences;
}

} // namespace

//==============================================================================

YuiToolComparison compareYuiToolImages (const Image& render,
                                        const Image& shown,
                                        const Image& reference,
                                        const Rectangle<float>& region,
                                        const Size<float>& interfaceSize,
                                        Color background)
{
    YuiToolComparison comparison;
    comparison.referenceSize = { reference.getWidth(), reference.getHeight() };

    const auto width = render.getWidth();
    const auto height = render.getHeight();

    if (width < 1 || height < 1 || ! reference.isValid() || interfaceSize.getWidth() <= 0.0f || interfaceSize.getHeight() <= 0.0f)
        return comparison;

    // The region of the interface, in the pixels of the reference
    const auto scaleX = static_cast<double> (reference.getWidth()) / interfaceSize.getWidth();
    const auto scaleY = static_cast<double> (reference.getHeight()) / interfaceSize.getHeight();
    const Rectangle<double> part (region.getX() * scaleX, region.getY() * scaleY, region.getWidth() * scaleX, region.getHeight() * scaleY);

    comparison.wasResized = std::abs (part.getWidth() - width) > 0.5 || std::abs (part.getHeight() - height) > 0.5;
    comparison.wasStretched = std::abs (scaleX - scaleY) > 0.01 * jmax (scaleX, scaleY);
    comparison.isColumn = width > height;

    const auto drawn = readPixels (render, background);
    const auto meant = resample (readPixels (reference, background), part, width, height);
    const auto hasShown = shown.isValid() && shown.getWidth() == width && shown.getHeight() == height;
    const auto displayed = hasShown ? readPixels (shown, background) : drawn;

    const auto stepX = comparison.isColumn ? 0 : width + panelGap;
    const auto stepY = comparison.isColumn ? height + panelGap : 0;
    const auto totalWidth = width + 2 * stepX;

    comparison.panels = Image (totalWidth, height + 2 * stepY, PixelFormat::RGBA);
    comparison.panels.fillColor (Color (panelBackground, panelBackground, panelBackground));

    auto* panels = comparison.panels.getRawData().data();

    const auto write = [&] (int panel, int x, int y, int red, int green, int blue)
    {
        auto* pixel = panels + (static_cast<size_t> (y + panel * stepY) * static_cast<size_t> (totalWidth) + static_cast<size_t> (x + panel * stepX)) * 4;

        pixel[0] = static_cast<uint8> (red);
        pixel[1] = static_cast<uint8> (green);
        pixel[2] = static_cast<uint8> (blue);
        pixel[3] = 255;
    };

    Cells cells (width, height);
    double totalError = 0.0;
    int64 numDifferent = 0;

    for (int y = 0; y < height; ++y)
    {
        const auto cellRow = y * cells.rows / height;

        for (int x = 0; x < width; ++x)
        {
            const auto* a = meant.at (x, y);
            const auto* b = drawn.at (x, y);
            const auto* c = displayed.at (x, y);

            const int differences[3] { std::abs (a[0] - b[0]), std::abs (a[1] - b[1]), std::abs (a[2] - b[2]) };
            const auto largest = jmax (differences[0], differences[1], differences[2]);
            const auto error = (differences[0] + differences[1] + differences[2]) / 765.0;

            totalError += error;
            numDifferent += largest > visibleDifference ? 1 : 0;

            const auto cell = cells.indexOf (x * cells.columns / width, cellRow);
            cells.errors[cell] += error;
            ++cells.counts[cell];

            write (0, x, y, a[0], a[1], a[2]);
            write (1, x, y, c[0], c[1], c[2]);

            // The render dimmed, with what differs going from yellow to red as it differs more
            const auto dimmed = 16 + (b[0] * 54 + b[1] * 183 + b[2] * 19) / 1024;

            if (largest <= visibleDifference / 3)
            {
                write (2, x, y, dimmed, dimmed, dimmed);
                continue;
            }

            const auto amount = largest / 255.0f;
            const auto strength = jmin (1.0f, 0.4f + amount * 2.0f);
            const auto heat = 230.0f * (1.0f - amount);

            write (2, x, y, static_cast<int> (dimmed + (255.0f - dimmed) * strength), static_cast<int> (dimmed + (heat - dimmed) * strength), static_cast<int> (dimmed * (1.0f - strength)));
        }
    }

    const auto numPixels = static_cast<double> (width) * height;

    comparison.similarity = 1.0 - totalError / numPixels;
    comparison.differentPixels = static_cast<double> (numDifferent) / numPixels;
    comparison.worst = findDifferences (cells, region);

    return comparison;
}
