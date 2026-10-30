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

constexpr int maxScaleDegree = 100000;

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

    int numerator = 0;
    int denominator = 1; // a token without a slash is a ratio over 1, so "2" means "2/1"

    if (slash < 0)
    {
        if (! parseInteger (token, numerator))
            return false;
    }
    else
    {
        if (slash == 0 || slash == token.length() - 1)
            return false;

        if (! parseInteger (token.substring (0, slash), numerator)
            || ! parseInteger (token.substring (slash + 1), denominator))
            return false;
    }

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

//==============================================================================
// AnaMark tuning files, see https://www.mark-henning.de/files/am/Tuning_File_V2_Doc.pdf

constexpr int numNotes = maxMidiNote + 1;

constexpr double anaMarkBaseFrequency = 8.1757989156437073336;

using NoteArray = std::array<double, numNotes>;

static bool parseSignedInteger (const String& token, int& result) noexcept
{
    const bool negative = token.startsWithChar ('-');
    const auto digits = (negative || token.startsWithChar ('+')) ? token.substring (1) : token;

    if (! parseInteger (digits, result))
        return false;

    result = negative ? -result : result;
    return true;
}

// Reads a number in any of the AnaMark numeric formats, advancing the text past it.
static bool readNumber (String::CharPointerType& text, double& result) noexcept
{
    text = text.findEndOfWhitespace();
    const auto start = text;
    const double value = CharacterFunctions::readDoubleValue (text);

    if (text == start || ! std::isfinite (value))
        return false;

    result = value;
    return true;
}

static bool parseNumber (const String& token, double& result) noexcept
{
    auto text = token.getCharPointer();
    return readNumber (text, result) && text.findEndOfWhitespace().isEmpty();
}

static bool readNoteOffset (String::CharPointerType& text, int& result) noexcept
{
    double value = 0.0;
    if (! readNumber (text, value) || value != std::floor (value))
        return false;

    result = static_cast<int> (jlimit (-1000.0, 1000.0, value));
    return true;
}

// Unquotes a string value, resolving its C style escapes. Unquoted values are taken as they are.
static String parseStringValue (const String& value)
{
    if (! value.startsWithChar ('"'))
        return value;

    String result;

    for (int i = 1; i < value.length() && value[i] != '"'; ++i)
    {
        auto c = value[i];

        if (c == '\\' && i + 1 < value.length())
        {
            c = value[++i];
            c = c == 'n' ? '\n' : c == 't' ? '\t' : c == 'r' ? '\r' : c;
        }

        result += String::charToString (c);
    }

    return result;
}

// Returns true if a MIDIChannels value such as "1-3,5,7" includes the channel; an empty value includes all.
static bool includesChannel (const String& channels, int channel)
{
    if (channels.trim().isEmpty())
        return true;

    for (const auto& item : StringArray::fromTokens (channels, ",", {}))
    {
        const auto part = item.trim();
        const int dash = part.indexOfChar ('-');

        int first = 0;
        int last = 0;
        const bool valid = dash < 0 ? parseInteger (part, first) && parseInteger (part, last)
                                    : parseInteger (part.substring (0, dash).trim(), first) && parseInteger (part.substring (dash + 1).trim(), last);

        if (valid && channel >= first && channel <= last)
            return true;
    }

    return false;
}

static void initEqual (NoteArray& frequencies, int baseNote, double baseFrequency) noexcept
{
    for (int note = 0; note < numNotes; ++note)
        frequencies[static_cast<size_t> (note)] = baseFrequency * std::pow (2.0, (note - baseNote) / 12.0);
}

//==============================================================================
struct FormulaOperand
{
    double value = 0.0;
    int note = 0;
    bool isReference = false;
    bool isRelative = false;
};

/** A [Functional Tuning] note formula: f(x) = range * mul / div * 2^(cents / 1200) + shift. */
struct Formula
{
    std::optional<FormulaOperand> range;
    std::optional<FormulaOperand> shift;
    std::optional<double> fixedFrequency;
    double mul = 1.0;
    double div = 1.0;
    double cents = 0.0;
    int loop = 1;
};

static bool parseFormula (const String& text, Formula& formula) noexcept
{
    for (auto p = text.getCharPointer();;)
    {
        p = p.findEndOfWhitespace();
        if (p.isEmpty())
            return true;

        const auto token = p.getAndAdvance();
        bool valid = false;

        switch (token)
        {
            case '#':
            case '+':
            {
                FormulaOperand operand;
                p = p.findEndOfWhitespace();

                if (*p == '=' || *p == '>')
                {
                    operand.isReference = true;
                    operand.isRelative = p.getAndAdvance() == '>';
                    valid = readNoteOffset (p, operand.note);
                }
                else
                {
                    valid = readNumber (p, operand.value);
                }

                (token == '#' ? formula.range : formula.shift) = operand;
                break;
            }

            case '*': valid = readNumber (p, formula.mul); break;
            case '/': valid = readNumber (p, formula.div) && formula.div != 0.0; break;
            case '%': valid = readNumber (p, formula.cents); break;
            case '~': valid = readNoteOffset (p, formula.loop); break;

            case '!':
            {
                double frequency = 0.0;
                valid = readNumber (p, frequency) && frequency > 0.0;
                formula.fixedFrequency = frequency;
                break;
            }

            default:
                break;
        }

        if (! valid)
            return false;
    }
}

static bool resolveOperand (const FormulaOperand& operand, const NoteArray& frequencies, int note, double& result) noexcept
{
    if (! operand.isReference)
    {
        result = operand.value;
        return true;
    }

    const int target = operand.isRelative ? note + operand.note : operand.note;
    if (! isPositiveAndBelow (target, numNotes))
        return false;

    result = frequencies[static_cast<size_t> (target)];
    return true;
}

static bool applyFormula (const Formula& formula, int note, NoteArray& frequencies) noexcept
{
    if (formula.fixedFrequency.has_value())
    {
        // The "!" token shifts the whole scale so that this note sounds at the given frequency.
        const double current = frequencies[static_cast<size_t> (note)];
        if (current <= 0.0)
            return false;

        for (auto& frequency : frequencies)
            frequency *= *formula.fixedFrequency / current;

        return true;
    }

    const int step = formula.loop < 0 ? -1 : 1;
    const int count = jmax (1, std::abs (formula.loop));

    for (int i = 0, x = note; i < count && isPositiveAndBelow (x, numNotes); ++i, x += step)
    {
        double range = frequencies[static_cast<size_t> (x)];
        double shift = 0.0;

        if ((formula.range.has_value() && ! resolveOperand (*formula.range, frequencies, x, range))
            || (formula.shift.has_value() && ! resolveOperand (*formula.shift, frequencies, x, shift)))
            return false;

        frequencies[static_cast<size_t> (x)] = range * formula.mul / formula.div * std::pow (2.0, formula.cents / 1200.0) + shift;
    }

    return true;
}

//==============================================================================
/** One scale dataset of an AnaMark file, i.e. what lies between [Scale Begin] and [Scale End]. */
struct AnaMarkScale
{
    enum class Section
    {
        other,
        info,
        assignment,
        tuning,
        exactTuning,
        functionalTuning,
        mapping
    };

    AnaMarkScale()
    {
        for (int note = 0; note < numNotes; ++note)
        {
            tuningCents[static_cast<size_t> (note)] = 100.0 * note;
            exactCents[static_cast<size_t> (note)] = 100.0 * note;
            keyboard[static_cast<size_t> (note)] = note;
        }
    }

    void enterSection (const String& sectionName)
    {
        section = sectionName == "info"                ? Section::info
                : sectionName == "assignment"          ? Section::assignment
                : sectionName == "tuning"              ? Section::tuning
                : sectionName == "exact tuning"        ? Section::exactTuning
                : sectionName == "functional tuning"   ? Section::functionalTuning
                : sectionName == "mapping"             ? Section::mapping
                                                       : Section::other;

        hasExactTuning = hasExactTuning || section == Section::exactTuning;

        if (section == Section::functionalTuning)
        {
            hasFunctionalTuning = true;
            initEqual (functional, 69, 440.0);
        }
    }

    // Returns false if the line holds an invalid value for a known key.
    bool readKey (const StringArray& key, const String& value)
    {
        int note = 0;
        const bool isNoteKey = key.size() == 2 && parseInteger (key[1], note) && note < numNotes;

        switch (section)
        {
            case Section::info:
                if (key.size() == 1 && key[0] == "name")
                    name = parseStringValue (value);
                return true;

            case Section::assignment:
                if (key.size() == 1 && key[0] == "midichannels")
                    channels = parseStringValue (value);
                return true;

            case Section::tuning:
                if (key[0] != "note")
                    return true;

                return isNoteKey && parseNumber (value, tuningCents[static_cast<size_t> (note)]);

            case Section::exactTuning:
                if (key.size() == 1 && key[0] == "basefreq")
                    return parseNumber (value, exactBaseFrequency) && exactBaseFrequency > 0.0;

                if (key[0] != "note")
                    return true;

                if (! isNoteKey || ! parseNumber (value, exactCents[static_cast<size_t> (note)]))
                    return false;

                exactSet[static_cast<size_t> (note)] = true;
                return true;

            case Section::functionalTuning:
                if (key.size() == 1 && key[0] == "initequal")
                    return readInitEqual (value);

                if (key[0] != "note")
                    return true;

                {
                    Formula formula;
                    return isNoteKey && parseFormula (parseStringValue (value), formula) && applyFormula (formula, note, functional);
                }

            case Section::mapping:
                if (key.size() == 1 && key[0] == "loopsize")
                    return parseInteger (value, loopSize) && loopSize <= numNotes;

                if (key[0] != "keyboard")
                    return true;

                return isNoteKey && parseSignedInteger (value, keyboard[static_cast<size_t> (note)]);

            case Section::other:
                break;
        }

        return true;
    }

    bool readInitEqual (const String& value)
    {
        if (! value.startsWithChar ('(') || ! value.endsWithChar (')'))
            return false;

        const auto arguments = StringArray::fromTokens (value.substring (1, value.length() - 1), ",", {});

        int baseNote = 0;
        double baseFrequency = 0.0;
        if (arguments.size() != 2 || ! parseSignedInteger (arguments[0].trim(), baseNote) || ! parseNumber (arguments[1], baseFrequency) || baseFrequency <= 0.0)
            return false;

        initEqual (functional, baseNote, baseFrequency);
        return true;
    }

    // Fills the frequency of every MIDI note, -1 for keys mapped outside the scale notes.
    // Returns false if a mapped key gets a frequency that is not positive.
    bool createTable (std::vector<double>& table) const
    {
        table.assign (numNotes, -1.0);

        auto cents = hasExactTuning ? exactCents : tuningCents;
        const double baseFrequency = hasExactTuning ? exactBaseFrequency : anaMarkBaseFrequency;

        if (hasExactTuning)
            completePeriod (cents);

        for (int key = 0; key < numNotes; ++key)
        {
            double frequency = baseFrequency * std::pow (2.0, cents[static_cast<size_t> (key)] / 1200.0);

            if (hasFunctionalTuning)
            {
                const int scaleNote = loopSize > 0 ? keyboard[static_cast<size_t> (key % loopSize)] + (key / loopSize) * loopSize
                                                   : keyboard[static_cast<size_t> (key)];

                if (! isPositiveAndBelow (scaleNote, numNotes))
                    continue;

                frequency = functional[static_cast<size_t> (scaleNote)];
            }

            if (! (frequency > 0.0 && std::isfinite (frequency)))
                return false;

            table[static_cast<size_t> (key)] = frequency;
        }

        return true;
    }

    // The [Exact Tuning] auto completion: the highest note given in the file sets the period repeated above it.
    void completePeriod (NoteArray& cents) const noexcept
    {
        int highest = 0;
        for (int note = 0; note < numNotes; ++note)
            highest = exactSet[static_cast<size_t> (note)] ? note : highest;

        if (highest == 0)
            return;

        const double period = cents[static_cast<size_t> (highest)];
        for (int note = highest + 1; note < numNotes; ++note)
            cents[static_cast<size_t> (note)] = cents[static_cast<size_t> (note - highest)] + period;
    }

    Section section = Section::other;
    String name;
    String channels;

    NoteArray tuningCents {};
    NoteArray exactCents {};
    NoteArray functional {};
    std::array<bool, numNotes> exactSet {};
    std::array<int, numNotes> keyboard {};
    double exactBaseFrequency = anaMarkBaseFrequency;
    int loopSize = 0;

    bool hasExactTuning = false;
    bool hasFunctionalTuning = false;
};

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

    if (! tuningTable.empty())
        return tuningTable[static_cast<size_t> (note)];

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

    if (! tuningTable.empty())
        return tuningTable[static_cast<size_t> (note)] > 0.0;

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

    return ! tuningTable.empty() || activeRange[static_cast<size_t> (note)];
}

//==============================================================================
Result TuningMap::loadScale (const File& file)
{
    auto stream = file.createInputStream();
    if (stream == nullptr)
        return Result::fail ("Cannot open the scale file: " + file.getFullPathName());

    auto result = loadScaleFromString (stream->readEntireStreamAsString());
    if (result.wasOk())
        scaleFile = file;

    return result;
}

Result TuningMap::loadScaleFromString (const String& scaleText)
{
    StringArray lines;
    lines.addLines (scaleText);

    bool descriptionSeen = false;
    String description;
    int noteCount = -1;
    std::vector<double> newScale;

    for (int i = 0; i < lines.size(); ++i)
    {
        const String line = lines[i].trim();
        if (line.startsWithChar ('!'))
            continue;

        if (! descriptionSeen)
        {
            descriptionSeen = true;
            description = line;
            continue;
        }

        if (line.isEmpty())
            continue;

        if (noteCount < 0)
        {
            int count = 0;
            if (! TuningMapHelpers::parseInteger (line, count) || count <= 0)
                return Result::fail ("The scale file contains an invalid note count");

            noteCount = count;
            continue;
        }

        double ratio = 0.0;
        if (! TuningMapHelpers::parseScalaInterval (line.initialSectionNotContaining (" \t"), ratio))
            return Result::fail ("The scale file contains an invalid interval");

        newScale.push_back (ratio);
    }

    if (! descriptionSeen || noteCount < 0 || static_cast<int> (newScale.size()) != noteCount)
        return Result::fail ("The scale file does not describe the declared number of notes");

    scale = std::move (newScale);
    scaleFile = File();
    scaleName = description;
    tuningTable.clear();
    updateBasePitch();

    return Result::ok();
}

Result TuningMap::loadKeyMap (const File& file)
{
    auto stream = file.createInputStream();
    if (stream == nullptr)
        return Result::fail ("Cannot open the key map file: " + file.getFullPathName());

    auto result = loadKeyMapFromString (stream->readEntireStreamAsString());
    if (result.wasOk())
        keyMapFile = file;

    return result;
}

Result TuningMap::loadKeyMapFromString (const String& keyMapText)
{
    StringArray lines;
    lines.addLines (keyMapText);

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
            if (! TuningMapHelpers::parseInteger (line, lastNote) || ! isPositiveAndBelow (lastNote, 128) || lastNote < firstNote)
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
            if (! TuningMapHelpers::parseInteger (line, newRepeatInc) || newRepeatInc > TuningMapHelpers::maxScaleDegree)
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
                if (! TuningMapHelpers::parseInteger (line, scaleDegree) || scaleDegree > TuningMapHelpers::maxScaleDegree)
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

    if (! rangeDeclared)
    {
        for (int i = firstNote; i <= lastNote; ++i)
            newActiveRange[static_cast<size_t> (i)] = true;
    }

    activeRange = newActiveRange;

    keyMapFile = File();
    tuningTable.clear();
    updateBasePitch();

    return Result::ok();
}

Result TuningMap::loadTuning (const File& file, int midiChannel)
{
    auto stream = file.createInputStream();
    if (stream == nullptr)
        return Result::fail ("Cannot open the tuning file: " + file.getFullPathName());

    auto result = loadTuningFromString (stream->readEntireStreamAsString(), midiChannel);
    if (result.wasOk())
    {
        tuningFile = file;

        if (tuningName.isEmpty())
            tuningName = file.getFileNameWithoutExtension();
    }

    return result;
}

Result TuningMap::loadTuningFromString (const String& tuningText, int midiChannel)
{
    using TuningMapHelpers::AnaMarkScale;

    StringArray lines;
    lines.addLines (tuningText);

    // Version 2 files hold their scales between [Scale Begin] and [Scale End], ignoring
    // everything else; older files are a single scale without them.
    const bool hasScaleBlocks = std::any_of (lines.begin(), lines.end(), [] (const String& line)
    {
        return line.trim().equalsIgnoreCase ("[Scale Begin]");
    });

    std::optional<AnaMarkScale> current;
    std::optional<AnaMarkScale> chosen;

    if (! hasScaleBlocks)
        current.emplace();

    for (int i = 0; i < lines.size() && ! chosen.has_value(); ++i)
    {
        const String line = lines[i].trim();
        if (line.isEmpty() || line.startsWithChar (';'))
            continue;

        if (line.startsWithChar ('['))
        {
            const auto sectionName = line.endsWithChar (']') ? line.substring (1, line.length() - 1).trim().toLowerCase() : String();

            if (sectionName == "scale begin")
            {
                current.emplace();
            }
            else if (sectionName == "scale end")
            {
                if (current.has_value() && TuningMapHelpers::includesChannel (current->channels, midiChannel))
                    chosen = std::move (current);

                current.reset();
            }
            else if (current.has_value())
            {
                current->enterSection (sectionName);
            }

            continue;
        }

        const int equals = line.indexOfChar ('=');
        if (! current.has_value() || equals <= 0)
            continue;

        auto key = StringArray::fromTokens (line.substring (0, equals).toLowerCase(), " \t", {});
        key.removeEmptyStrings();

        if (! key.isEmpty() && ! current->readKey (key, line.substring (equals + 1).trim()))
            return Result::fail ("The tuning file contains an invalid value at line " + String (i + 1));
    }

    if (! chosen.has_value() && current.has_value() && TuningMapHelpers::includesChannel (current->channels, midiChannel))
        chosen = std::move (current);

    if (! chosen.has_value())
        return Result::fail ("The tuning file holds no scale for MIDI channel " + String (midiChannel));

    std::vector<double> newTable;
    if (! chosen->createTable (newTable))
        return Result::fail ("The tuning file tunes a note to an invalid frequency");

    tuningTable = std::move (newTable);
    tuningName = chosen->name;
    tuningFile = File();

    return Result::ok();
}

bool TuningMap::isUsingTuningTable() const noexcept
{
    return ! tuningTable.empty();
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

File TuningMap::getScaleFile() const noexcept
{
    return scaleFile;
}

File TuningMap::getKeyMapFile() const noexcept
{
    return keyMapFile;
}

File TuningMap::getTuningFile() const noexcept
{
    return tuningFile;
}

String TuningMap::getName() const noexcept
{
    return tuningTable.empty() ? scaleName : tuningName;
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
