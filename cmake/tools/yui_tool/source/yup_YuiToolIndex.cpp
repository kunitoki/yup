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


#include "yup_YuiToolIndex.h"

#include <algorithm>

using namespace yup;

namespace
{

constexpr size_t maximumSources = 64;

bool isNameStart (yup_wchar character)
{
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
}

bool isNameCharacter (yup_wchar character)
{
    return isNameStart (character) || (character >= '0' && character <= '9');
}

bool isSpace (yup_wchar character)
{
    return character == ' ' || character == '\t' || character == '\n' || character == '\r';
}

// The value of a hexadecimal digit, or -1
int readHexDigit (yup_wchar character)
{
    if (character >= '0' && character <= '9')
        return static_cast<int> (character - '0');

    if (character >= 'a' && character <= 'f')
        return static_cast<int> (character - 'a') + 10;

    if (character >= 'A' && character <= 'F')
        return static_cast<int> (character - 'A') + 10;

    return -1;
}

String makeString (const std::vector<yup_wchar>& characters, int start, int end)
{
    start = jlimit (0, static_cast<int> (characters.size()), start);
    end = jlimit (start, static_cast<int> (characters.size()), end);

    return start < end ? String (CharPointer_UTF32 (characters.data() + start), static_cast<size_t> (end - start)) : String();
}

// What is written before a brace is a selector or a name, where `#add` is an id rather than a color
void markHeaders (const YuiToolBlock& block, std::vector<char>& isHeader)
{
    for (int position = block.start; position < block.open; ++position)
        isHeader[static_cast<size_t> (position)] = 1;

    for (const auto& held : block.blocks)
        markHeaders (*held, isHeader);
}

} // namespace

//==============================================================================

bool YuiToolBlock::isElement() const
{
    auto characters = header.getCharPointer();

    if (! CharacterFunctions::isUpperCase (*characters))
        return false;

    while (isNameCharacter (*characters))
        ++characters;

    return characters.isEmpty();
}

String YuiToolBlock::getKeyword() const
{
    auto characters = header.getCharPointer();
    int length = 0;

    if (isNameStart (*characters))
    {
        while (isNameCharacter (*characters))
        {
            ++characters;
            ++length;
        }
    }

    return header.substring (0, length);
}

//==============================================================================

YuiToolIndex::YuiToolIndex (const String& textToRead)
{
    for (auto characters = textToRead.getCharPointer(); ! characters.isEmpty();)
        text.push_back (characters.getAndAdvance());

    code = text;
    lineStarts.push_back (0);

    for (int position = 0; position < getLength(); ++position)
        if (text[static_cast<size_t> (position)] == '\n')
            lineStarts.push_back (position + 1);

    blankCommentsAndStrings();
    readBlocks();
    readDeclarations();
}

void YuiToolIndex::blankCommentsAndStrings()
{
    const auto size = getLength();

    const auto at = [this, size] (int position)
    {
        return position < size ? text[static_cast<size_t> (position)] : yup_wchar (0);
    };

    // The lines are kept, so a place is the same in the text and in what is left of it
    const auto blank = [this] (int from, int to)
    {
        for (int position = from; position < to; ++position)
            if (code[static_cast<size_t> (position)] != '\n')
                code[static_cast<size_t> (position)] = ' ';
    };

    for (int position = 0; position < size;)
    {
        const auto character = at (position);
        const auto next = at (position + 1);

        if (character == '/' && next == '/')
        {
            auto stop = position;

            while (stop < size && at (stop) != '\n')
                ++stop;

            blank (position, stop);
            position = stop;
        }
        else if (character == '/' && next == '*')
        {
            auto stop = position + 2;

            while (stop < size && ! (at (stop) == '*' && at (stop + 1) == '/'))
                ++stop;

            stop = jmin (size, stop + 2);
            blank (position, stop);
            position = stop;
        }
        else if (character == '"')
        {
            auto stop = position + 1;

            // A string ends with its line, so one being typed does not take the rest of the text
            while (stop < size && at (stop) != '"' && at (stop) != '\n')
                stop += at (stop) == '\\' ? 2 : 1;

            stop = jmin (size, stop);
            blank (position + 1, stop);
            position = at (stop) == '"' ? stop + 1 : stop;
        }
        else
        {
            ++position;
        }
    }
}

void YuiToolIndex::readBlocks()
{
    const auto size = getLength();
    std::vector<YuiToolBlock*> open { &document };
    int boundary = 0;

    document.end = size;
    document.isClosed = true;

    for (int position = 0; position < size; ++position)
    {
        const auto character = getCodeAt (position);

        if (character != '{' && character != '}' && character != ';')
            continue;

        // What is written since the last brace or semicolon, without the space around it
        auto from = boundary;
        auto to = position;

        while (from < to && isSpace (getCodeAt (from)))
            ++from;

        while (to > from && isSpace (getCodeAt (to - 1)))
            --to;

        if (character == '{')
        {
            auto block = std::make_unique<YuiToolBlock>();
            block->header = getCode (from, to);
            block->start = from;
            block->open = position;
            block->end = size;
            block->parent = open.back();

            auto* opened = block.get();
            open.back()->blocks.push_back (std::move (block));
            open.push_back (opened);
        }
        else if (character == '}')
        {
            // A brace closing nothing is one too many, which the document is not ended by
            if (open.size() > 1)
            {
                open.back()->end = position + 1;
                open.back()->isClosed = true;
                open.pop_back();
            }
        }
        else if (from < to)
        {
            open.back()->statements.push_back ({ from, to, getCode (from, to) });
        }

        boundary = position + 1;
    }
}

void YuiToolIndex::readDeclarations()
{
    for (const auto& top : document.blocks)
    {
        addDeclaration (*top);

        // A style block declares drawings and palettes too
        if (top->header == "style")
        {
            for (const auto& nested : top->blocks)
                if (const auto keyword = nested->getKeyword(); keyword == "drawing" || keyword == "palette")
                    addDeclaration (*nested);
        }

        if (root == nullptr && top->isElement())
            root = top.get();
    }

    for (const auto& statement : document.statements)
    {
        if (! statement.text.startsWith ("import"))
            continue;

        // The path is between the quotes of the statement, which only the text as it is written still has
        auto start = statement.start;

        while (start < statement.end && text[static_cast<size_t> (start)] != '"')
            ++start;

        auto end = start + 1;

        while (end < statement.end && text[static_cast<size_t> (end)] != '"')
            ++end;

        if (end < statement.end)
            imports.push_back ({ getText (start + 1, end), start + 1, end });
    }
}

int YuiToolIndex::readName (int& position, int end, String& name) const
{
    while (position < end && isSpace (getCodeAt (position)))
        ++position;

    const auto start = position;

    if (position >= end || ! isNameStart (getCodeAt (position)))
        return -1;

    while (position < end && isNameCharacter (getCodeAt (position)))
        ++position;

    name = getCode (start, position);
    return start;
}

bool YuiToolIndex::readCharacter (int& position, int end, yup_wchar character) const
{
    while (position < end && isSpace (getCodeAt (position)))
        ++position;

    // With no character to read, tells whether nothing is left
    if (character == 0)
        return position >= end;

    if (position >= end || getCodeAt (position) != character)
        return false;

    ++position;
    return true;
}

void YuiToolIndex::addDeclaration (const YuiToolBlock& block)
{
    YuiToolDeclaration declaration;
    declaration.block = &block;

    if (block.header == "tokens")
    {
        declaration.kind = "tokens";
        declaration.name = "tokens";
        declaration.offset = block.start;
    }
    else
    {
        // `drawing Name`, `palette Name` or `component Name : Base`, and nothing more
        auto position = block.start;

        if (readName (position, block.open, declaration.kind) < 0)
            return;

        if (declaration.kind != "drawing" && declaration.kind != "palette" && declaration.kind != "component")
            return;

        declaration.offset = readName (position, block.open, declaration.name);

        if (declaration.offset < 0)
            return;

        if (readCharacter (position, block.open, ':') && readName (position, block.open, declaration.base) < 0)
            return;

        if (! readCharacter (position, block.open, 0))
            return;
    }

    addMembers (declaration);
    declarations.push_back (std::move (declaration));
}

void YuiToolIndex::addMembers (YuiToolDeclaration& declaration) const
{
    const auto isPalette = declaration.kind == "palette";
    const auto isComponent = declaration.kind == "component";
    const auto declaresProperties = isComponent || declaration.kind == "drawing";

    for (const auto& statement : declaration.block->statements)
    {
        auto position = statement.start;
        YuiToolMember member;
        String first;

        member.offset = readName (position, statement.end, first);

        if (member.offset < 0)
            continue;

        if (isComponent && first == "signal")
        {
            member.offset = readName (position, statement.end, member.name);
            member.detail = "signal";
        }
        else if (declaresProperties)
        {
            // `property name: type = default`, or `property alias name: target`
            if (first != "property")
                continue;

            member.offset = readName (position, statement.end, member.name);

            if (auto after = position; member.name == "alias")
            {
                String aliased;

                if (const auto offset = readName (after, statement.end, aliased); offset >= 0)
                {
                    member.name = aliased;
                    member.offset = offset;
                    position = after;
                }
            }

            if (member.offset < 0 || ! readCharacter (position, statement.end, ':'))
                continue;

            auto end = position;

            while (end < statement.end && getCodeAt (end) != '=')
                ++end;

            member.detail = getText (position, end).trim();
        }
        else
        {
            // The mode and the source colors of a palette are how it is made, not colors to name
            if (! readCharacter (position, statement.end, ':') || (isPalette && (first == "mode" || first == "colors")))
                continue;

            member.name = first;
            member.detail = getText (position, statement.end).trim();
        }

        if (member.offset >= 0)
            declaration.members.push_back (member);
    }
}

const YuiToolDeclaration* YuiToolIndex::findDeclaration (const String& kind, const String& name) const
{
    const auto found = std::find_if (declarations.begin(), declarations.end(), [&] (const YuiToolDeclaration& declaration)
    {
        return declaration.kind == kind && declaration.name == name;
    });

    return found != declarations.end() ? &*found : nullptr;
}

std::vector<YuiToolColor> YuiToolIndex::findColors() const
{
    const auto size = getLength();
    std::vector<char> isHeader (code.size(), 0);
    std::vector<YuiToolColor> colors;

    markHeaders (document, isHeader);

    for (int position = 0; position < size; ++position)
    {
        const auto before = getCodeAt (position - 1);

        if (getCodeAt (position) != '#' || isHeader[static_cast<size_t> (position)] != 0 || isNameCharacter (before) || before == '#')
            continue;

        auto end = position + 1;

        while (readHexDigit (getCodeAt (end)) >= 0)
            ++end;

        const auto numDigits = end - position - 1;

        if ((numDigits != 3 && numDigits != 4 && numDigits != 6 && numDigits != 8) || isNameCharacter (getCodeAt (end)))
            continue;

        // One digit a part, or two: red, green, blue and then alpha when it is written
        const auto digitsPerPart = numDigits <= 4 ? 1 : 2;
        float parts[4] { 0.0f, 0.0f, 0.0f, 1.0f };

        for (int part = 0; part * digitsPerPart < numDigits; ++part)
        {
            const auto high = readHexDigit (getCodeAt (position + 1 + part * digitsPerPart));
            const auto low = digitsPerPart == 2 ? readHexDigit (getCodeAt (position + 2 + part * digitsPerPart)) : high;

            parts[part] = static_cast<float> (high * 16 + low) / 255.0f;
        }

        colors.push_back ({ position, end, parts[0], parts[1], parts[2], parts[3] });
        position = end - 1;
    }

    return colors;
}

//==============================================================================

std::vector<const YuiToolBlock*> YuiToolIndex::findBlocksAt (int offset, bool fromHeader) const
{
    std::vector<const YuiToolBlock*> chain;
    const auto* current = &document;

    for (;;)
    {
        const auto inside = std::find_if (current->blocks.begin(), current->blocks.end(), [offset, fromHeader] (const std::unique_ptr<YuiToolBlock>& block)
        {
            return (fromHeader ? block->start <= offset : block->open < offset)
                && (block->isClosed ? offset < block->end : offset <= block->end);
        });

        if (inside == current->blocks.end())
            return chain;

        chain.push_back (inside->get());
        current = inside->get();
    }
}

const YuiToolBlock* YuiToolIndex::findComponentAt (int offset) const
{
    const auto chain = findBlocksAt (offset, true);

    if (chain.empty() || ! (chain.front()->isElement() || chain.front()->getKeyword() == "component"))
        return nullptr;

    for (auto block = chain.rbegin(); block != chain.rend(); ++block)
        if ((*block)->isElement())
            return *block;

    return nullptr;
}

//==============================================================================

int YuiToolIndex::getOffset (YuiToolPlace place) const noexcept
{
    const auto numLines = static_cast<int> (lineStarts.size());
    const auto line = jlimit (1, numLines, place.line);
    const auto start = lineStarts[static_cast<size_t> (line - 1)];
    const auto end = line < numLines ? lineStarts[static_cast<size_t> (line)] - 1 : getLength();

    return jlimit (start, end, start + place.column - 1);
}

YuiToolPlace YuiToolIndex::getPlace (int offset) const noexcept
{
    offset = jlimit (0, getLength(), offset);

    // The lines starting at the place or before it: the last of them is the one it is in
    const auto line = static_cast<int> (std::upper_bound (lineStarts.begin(), lineStarts.end(), offset) - lineStarts.begin());

    return { line, offset - lineStarts[static_cast<size_t> (line - 1)] + 1 };
}

int YuiToolIndex::getLineStart (int offset) const noexcept
{
    return lineStarts[static_cast<size_t> (getPlace (offset).line - 1)];
}

String YuiToolIndex::getText (int start, int end) const
{
    return makeString (text, start, end);
}

String YuiToolIndex::getCode (int start, int end) const
{
    return makeString (code, start, end);
}

yup_wchar YuiToolIndex::getCodeAt (int offset) const noexcept
{
    return offset >= 0 && offset < getLength() ? code[static_cast<size_t> (offset)] : yup_wchar (0);
}

String YuiToolIndex::findNameAt (int offset, int& start) const
{
    start = jlimit (0, getLength(), offset);
    auto end = start;

    while (isNameCharacter (getCodeAt (start - 1)))
        --start;

    while (isNameCharacter (getCodeAt (end)))
        ++end;

    return start < end && isNameStart (getCodeAt (start)) ? getCode (start, end) : String();
}

bool YuiToolIndex::isPartOfName (yup_wchar character) noexcept
{
    return isNameCharacter (character);
}

//==============================================================================

std::vector<YuiToolSource> readYuiToolSources (const File& file)
{
    std::vector<YuiToolSource> sources;

    sources.push_back ({ file, std::make_unique<YuiToolIndex> (file.loadFileAsString()) });

    // The files found are read in turn for what they import, which is added after them
    for (size_t next = 0; next < sources.size() && sources.size() < maximumSources; ++next)
    {
        const auto folder = sources[next].file.getParentDirectory();
        StringArray paths;

        for (const auto& imported : sources[next].index->getImports())
            paths.add (imported.path);

        for (const auto& path : paths)
        {
            for (const auto& candidate : { folder.getChildFile (path), file.getParentDirectory().getChildFile (path) })
            {
                const auto isKnown = std::any_of (sources.begin(), sources.end(), [&candidate] (const YuiToolSource& source)
                {
                    return source.file == candidate;
                });

                if (! isKnown && ! candidate.existsAsFile())
                    continue;

                if (! isKnown)
                    sources.push_back ({ candidate, std::make_unique<YuiToolIndex> (candidate.loadFileAsString()) });

                break;
            }
        }
    }

    return sources;
}
