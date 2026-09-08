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

#include <cmath>
#include <limits>

namespace yup
{

namespace TuningMapHelpers
{

constexpr int maxMidiNote = 127;

// Returns true and sets the result if the token is a plain non-negative integer.
static bool parseInteger (const String& token, int& result) noexcept
{
    if (token.isEmpty() || ! token.containsOnly ("0123456789"))
        return false;

    const int64 value = token.getLargeIntValue();
    if (value > std::numeric_limits<int>::max())
        return false;

    result = static_cast<int> (value);
    return true;
}

// Returns true and sets the result if the token is a plain decimal number.
static bool parseDecimal (const String& token, double& result) noexcept
{
    int i = 0;
    const int length = token.length();

    if (i < length && (token[i] == '+' || token[i] == '-'))
        ++i;

    bool seenDigit = false;
    bool seenDot = false;
    for (; i < length; ++i)
    {
        const auto c = token[i];
        if (c >= '0' && c <= '9')
        {
            seenDigit = true;
            continue;
        }
        if (c == '.' && ! seenDot)
        {
            seenDot = true;
            continue;
        }
        return false;
    }

    if (! seenDigit)
        return false;

    result = token.getDoubleValue();
    return true;
}

// Parses one Scala interval, either a rational ratio "n/d" or a value in cents.
// Returns false if the token does not describe a usable interval.
static bool parseScalaInterval (const String& token, double& ratioOut) noexcept
{
    if (token.containsChar ('.'))
    {
        double cents = 0.0;
        if (! parseDecimal (token, cents))
            return false;

        ratioOut = std::pow (2.0, cents / 1200.0);
        return true;
    }

    const int slash = token.indexOfChar ('/');
    if (slash <= 0 || slash == token.length() - 1)
        return false;

    int numerator = 0;
    int denominator = 0;
    if (! parseInteger (token.substring (0, slash), numerator))
        return false;
    if (! parseInteger (token.substring (slash + 1), denominator))
        return false;

    if (numerator <= 0 || denominator <= 0)
        return false;

    ratioOut = static_cast<double> (numerator) / static_cast<double> (denominator);
    return true;
}

// Applies an optional "< first last" range line of a .kbm file.
static bool parseActiveRange (const StringArray& tokens, std::array<bool, 128>& range) noexcept
{
    if (tokens.size() < 3 || ! tokens[0].startsWithChar ('<'))
        return false;

    int firstNote = 0;
    int lastNote = 0;
    if (! parseInteger (tokens[1], firstNote) || ! parseInteger (tokens[2], lastNote))
        return false;

    if (! isPositiveAndBelow (firstNote, maxMidiNote + 1) || ! isPositiveAndBelow (lastNote, maxMidiNote + 1))
        return false;

    if (firstNote > lastNote)
        return false;

    for (int i = firstNote; i <= lastNote; ++i)
        range[static_cast<size_t> (i)] = true;

    return true;
}

} // namespace TuningMapHelpers

//==============================================================================
TuningMap::TuningMap()
{
    scale.resize (12);
    for (int i = 1; i <= 12; ++i)
        scale[static_cast<size_t> (i - 1)] = std::pow (2.0, i / 12.0);

    mapping = { 0 };
    activateRange (0, TuningMapHelpers::maxMidiNote);
    updateBasePitch();
}

//==============================================================================
double TuningMap::noteToPitch (int note) const noexcept
{
    jassert (isPositiveAndBelow (note, TuningMapHelpers::maxMidiNote + 1));

    if (! isPositiveAndBelow (note, TuningMapHelpers::maxMidiNote + 1))
        return -1.0;

    if (mapping.empty() || scale.empty())
        return -1.0;

    const int mapSize = static_cast<int> (mapping.size());
    const int scaleSize = static_cast<int> (scale.size());

    int nRepeats = (note - zeroNote) / mapSize;
    int mapIndex = (note - zeroNote) % mapSize;
    if (mapIndex < 0)
    {
        --nRepeats;
        mapIndex += mapSize;
    }

    if (mapping[static_cast<size_t> (mapIndex)] < 0)
        return -1.0;

    const int64 totalDegrees = static_cast<int64> (nRepeats) * mapRepeatInc
                             + mapping[static_cast<size_t> (mapIndex)];

    int64 nOctaves = totalDegrees / scaleSize;
    int64 scaleIndex = totalDegrees % scaleSize;
    if (scaleIndex < 0)
    {
        --nOctaves;
        scaleIndex += scaleSize;
    }

    const double octaveRatio = scale.back();
    const double octaves = std::pow (octaveRatio, static_cast<double> (nOctaves));

    if (scaleIndex == 0)
        return basePitch * octaves;

    return basePitch * octaves * scale[static_cast<size_t> (scaleIndex - 1)];
}

bool TuningMap::isNoteMapped (int note) const noexcept
{
    jassert (isPositiveAndBelow (note, TuningMapHelpers::maxMidiNote + 1));

    if (! isPositiveAndBelow (note, TuningMapHelpers::maxMidiNote + 1))
        return false;

    if (mapping.empty())
        return false;

    const int mapSize = static_cast<int> (mapping.size());

    int mapIndex = (note - zeroNote) % mapSize;
    if (mapIndex < 0)
        mapIndex += mapSize;

    return mapping[static_cast<size_t> (mapIndex)] >= 0;
}

bool TuningMap::isNoteActive (int note) const noexcept
{
    jassert (isPositiveAndBelow (note, TuningMapHelpers::maxMidiNote + 1));

    if (! isPositiveAndBelow (note, TuningMapHelpers::maxMidiNote + 1))
        return false;

    return activeRange[static_cast<size_t> (note)];
}

//==============================================================================
Result TuningMap::loadScale (const File& file)
{
    auto stream = file.createInputStream();
    if (stream == nullptr)
        return Result::fail ("Cannot open the scale file: " + file.getFullPathName());

    StringArray lines;
    lines.addLines (stream->readEntireStreamAsString());

    bool descriptionSeen = false;
    int noteCount = -1;
    std::vector<double> newScale;

    for (int i = 0; i < lines.size(); ++i)
    {
        const String line = lines[i].trim();
        if (line.isEmpty() || line.startsWithChar ('!'))
            continue;

        if (! descriptionSeen)
        {
            descriptionSeen = true;
            continue;
        }

        if (noteCount < 0)
        {
            int count = 0;
            if (! TuningMapHelpers::parseInteger (line, count) || count <= 0)
                return Result::fail ("The scale file contains an invalid note count");

            noteCount = count;
            continue;
        }

        double ratio = 0.0;
        if (! TuningMapHelpers::parseScalaInterval (line, ratio))
            return Result::fail ("The scale file contains an invalid interval");

        newScale.push_back (ratio);
    }

    if (! descriptionSeen || noteCount < 0 || static_cast<int> (newScale.size()) != noteCount)
        return Result::fail ("The scale file does not describe the declared number of notes");

    scale = std::move (newScale);
    scaleFile = file.getFullPathName();
    updateBasePitch();

    return Result::ok();
}

Result TuningMap::loadKeyMap (const File& file)
{
    auto stream = file.createInputStream();
    if (stream == nullptr)
        return Result::fail ("Cannot open the key map file: " + file.getFullPathName());

    StringArray lines;
    lines.addLines (stream->readEntireStreamAsString());

    int mapSize = -1;
    int firstNote = -1;
    int lastNote = -1;
    int newZeroNote = -1;
    int newRefNote = -1;
    int newRepeatInc = -1;
    double newRefPitch = -1.0;

    std::vector<int> newMapping;
    std::array<bool, 128> newActiveRange {};
    bool rangeDeclared = false;

    for (int i = 0; i < lines.size(); ++i)
    {
        const String line = lines[i].trim();
        if (line.isEmpty() || line.startsWithChar ('!'))
            continue;

        if (line.startsWithChar ('<'))
        {
            if (! TuningMapHelpers::parseActiveRange (StringArray::fromTokens (line, " \t", "\""), newActiveRange))
                return Result::fail ("The key map file contains an invalid active range");

            rangeDeclared = true;
            continue;
        }

        if (mapSize < 0)
        {
            if (! TuningMapHelpers::parseInteger (line, mapSize) || mapSize > TuningMapHelpers::maxMidiNote + 1)
                return Result::fail ("The key map file contains an invalid map size");
        }
        else if (firstNote < 0)
        {
            if (! TuningMapHelpers::parseInteger (line, firstNote) || ! isPositiveAndBelow (firstNote, 128))
                return Result::fail ("The key map file contains an invalid first note");
        }
        else if (lastNote < 0)
        {
            if (! TuningMapHelpers::parseInteger (line, lastNote) || ! isPositiveAndBelow (lastNote, 128))
                return Result::fail ("The key map file contains an invalid last note");
        }
        else if (newZeroNote < 0)
        {
            if (! TuningMapHelpers::parseInteger (line, newZeroNote) || ! isPositiveAndBelow (newZeroNote, 128))
                return Result::fail ("The key map file contains an invalid zero note");
        }
        else if (newRefNote < 0)
        {
            if (! TuningMapHelpers::parseInteger (line, newRefNote) || ! isPositiveAndBelow (newRefNote, 128))
                return Result::fail ("The key map file contains an invalid reference note");
        }
        else if (newRefPitch <= 0.0)
        {
            if (! TuningMapHelpers::parseDecimal (line, newRefPitch) || newRefPitch <= 0.0)
                return Result::fail ("The key map file contains an invalid reference pitch");
        }
        else if (newRepeatInc < 0)
        {
            if (! TuningMapHelpers::parseInteger (line, newRepeatInc))
                return Result::fail ("The key map file contains an invalid repeat increment");
        }
        else
        {
            if (line.equalsIgnoreCase ("x"))
            {
                newMapping.push_back (-1);
            }
            else
            {
                int scaleDegree = 0;
                if (! TuningMapHelpers::parseInteger (line, scaleDegree))
                    return Result::fail ("The key map file contains an invalid mapping entry");

                newMapping.push_back (scaleDegree);
            }
        }
    }

    if (newRepeatInc < 0 || newRefPitch <= 0.0)
        return Result::fail ("The key map file ended prematurely");

    if (mapSize == 0)
    {
        if (! newMapping.empty())
            return Result::fail ("The key map file declares an automatic mapping but also contains mapping entries");

        zeroNote = newZeroNote;
        refNote = newRefNote;
        refPitch = newRefPitch;
        mapRepeatInc = 1;

        mapping = { 0 };
    }
    else
    {
        newMapping.resize (static_cast<size_t> (mapSize), -1);

        int refIndex = (newRefNote - newZeroNote) % mapSize;
        if (refIndex < 0)
            refIndex += mapSize;

        if (newMapping[static_cast<size_t> (refIndex)] < 0)
            return Result::fail ("The key map file leaves the reference note unmapped");

        zeroNote = newZeroNote;
        refNote = newRefNote;
        refPitch = newRefPitch;
        mapRepeatInc = newRepeatInc == 0 ? mapSize : newRepeatInc;

        mapping = std::move (newMapping);
    }

    if (rangeDeclared)
        activeRange = newActiveRange;
    else
        activateRange (0, TuningMapHelpers::maxMidiNote);

    keyMapFile = file.getFullPathName();
    updateBasePitch();

    return Result::ok();
}

//==============================================================================
int TuningMap::getZeroNote() const noexcept
{
    return zeroNote;
}

int TuningMap::getReferenceNote() const noexcept
{
    return refNote;
}

double TuningMap::getReferencePitch() const noexcept
{
    return refPitch;
}

int TuningMap::getKeyMapRepeatIncrement() const noexcept
{
    return mapRepeatInc;
}

int TuningMap::getNumberOfScaleDegrees() const noexcept
{
    return static_cast<int> (scale.size());
}

String TuningMap::getScaleFile() const noexcept
{
    return scaleFile;
}

String TuningMap::getKeyMapFile() const noexcept
{
    return keyMapFile;
}

//==============================================================================
void TuningMap::activateRange (int firstNote, int lastNote) noexcept
{
    for (int i = firstNote; i <= lastNote; ++i)
        activeRange[static_cast<size_t> (i)] = true;
}

void TuningMap::updateBasePitch() noexcept
{
    if (mapping.empty() || scale.empty())
        return;

    basePitch = 1.0; // compute the reference ratio relative to 1/1 first

    const double referenceRatio = noteToPitch (refNote);
    if (referenceRatio > 0.0)
        basePitch = refPitch / referenceRatio;
}

} // namespace yup
