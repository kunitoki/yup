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

#include <yup_gui/yup_gui.h>

#include <vector>

//==============================================================================
/** How the render of an interface differs from the image it is meant to look like. */
struct YuiToolComparison
{
    /** A part of the interface that differs, in its logical pixels. */
    struct Difference
    {
        yup::Rectangle<float> region;

        /** The mean difference of its pixels, from 0 to 1. */
        double error = 0.0;
    };

    /** One less the mean difference of the pixels, so 1 for two images that are the same. */
    double similarity = 0.0;

    /** The share of the pixels a person would see differ. */
    double differentPixels = 0.0;

    /** The size of the reference in pixels, as it was read. */
    yup::Size<int> referenceSize;

    /** True when the reference had to be brought to the size of the render. */
    bool wasResized = false;

    /** True when the reference has other proportions than the interface, so it was stretched. */
    bool wasStretched = false;

    /** The parts differing most, the worst first. */
    std::vector<Difference> worst;

    /** The reference, the render and their difference, each of the size of the render. */
    yup::Image panels;

    /** True when the panels are one above the other, false when they are side by side. */
    bool isColumn = false;
};

/** Compares the render of a region of an interface with the same region of a reference.

    The reference is an image of the whole interface at any size. Its part matching the region is
    brought to the size of the render, then the two are compared pixel by pixel.

    @param render        The region as it was drawn, which is what is compared.
    @param shown         The image shown as the render, which may have a grid over it.
    @param reference     The image of the whole interface to compare with.
    @param region        The region drawn, in logical pixels of the interface.
    @param interfaceSize The size of the interface in logical pixels.
    @param background    What shows through the transparent parts of the reference.
*/
YuiToolComparison compareYuiToolImages (const yup::Image& render,
                                        const yup::Image& shown,
                                        const yup::Image& reference,
                                        const yup::Rectangle<float>& region,
                                        const yup::Size<float>& interfaceSize,
                                        yup::Color background);
