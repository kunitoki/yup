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

#include <gtest/gtest.h>

namespace yup::test
{

//==============================================================================
/*  Shared plumbing for the browser-parity corpora.

    tests/data/layout/capture.html renders each configuration with real CSS in a
    browser and records every child's getBoundingClientRect(); the JSON it emits
    is replayed here through FlexBox / Grid and compared. The point is that CSS
    conformance is measured against an actual implementation rather than against
    expectations somebody derived from the same reading of the spec that the
    code under test came from.

    Read tests/data/layout/capture.html before adding cases - in particular the
    note explaining why every item is an empty div, which is what makes YUP's
    stubbed intrinsic sizing agree with a browser's.
*/

/** Chrome lays out in 1/64px LayoutUnits, so a value the spec puts at 73.3333
    comes back as 73.3281. Half a pixel absorbs that quantization without being
    loose enough to hide a real algorithmic difference. */
inline constexpr float parityTolerance = 0.5f;

/** Returns the tests/data/layout/ directory holding the golden corpora. */
inline File getLayoutGoldenDirectory()
{
    auto dir = File (__FILE__)
                   .getParentDirectory()
                   .getParentDirectory()
                   .getChildFile ("data")
                   .getChildFile ("layout");

    if (dir.exists())
        return dir;

    dir = File::getCurrentWorkingDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getParentDirectory()
              .getChildFile ("tests")
              .getChildFile ("data")
              .getChildFile ("layout");

    return dir;
}

/** Reads a property that may be absent or JSON null, both of which mean "auto". */
inline bool hasValue (const var& object, const char* name)
{
    const auto value = object.getProperty (Identifier (name), var());
    return ! (value.isVoid() || value.isUndefined());
}

inline float readFloat (const var& object, const char* name, float fallback)
{
    if (! hasValue (object, name))
        return fallback;

    return static_cast<float> (static_cast<double> (object.getProperty (Identifier (name), var())));
}

inline int readInt (const var& object, const char* name, int fallback)
{
    if (! hasValue (object, name))
        return fallback;

    return static_cast<int> (object.getProperty (Identifier (name), var()));
}

inline String readString (const var& object, const char* name, const String& fallback)
{
    if (! hasValue (object, name))
        return fallback;

    return object.getProperty (Identifier (name), var()).toString();
}

/** Loads one of the golden corpora, or returns a void var when it is missing. */
inline var loadGoldenCorpus (const String& filename)
{
    auto file = getLayoutGoldenDirectory().getChildFile (filename);

    if (! file.existsAsFile())
        return {};

    return JSON::parse (file);
}

/** Compares one item's laid-out bounds against the browser's, reporting the
    case name and item index so a failure points at a corpus entry. */
inline void expectParity (const String& caseName,
                          int itemIndex,
                          const Component& component,
                          const var& expected)
{
    const auto bounds = component.getBounds();

    SCOPED_TRACE (("case '" + caseName + "' item " + String (itemIndex)).toStdString());

    EXPECT_NEAR (readFloat (expected, "x", 0.0f), bounds.getX(), parityTolerance);
    EXPECT_NEAR (readFloat (expected, "y", 0.0f), bounds.getY(), parityTolerance);
    EXPECT_NEAR (readFloat (expected, "w", 0.0f), bounds.getWidth(), parityTolerance);
    EXPECT_NEAR (readFloat (expected, "h", 0.0f), bounds.getHeight(), parityTolerance);
}

} // namespace yup::test
