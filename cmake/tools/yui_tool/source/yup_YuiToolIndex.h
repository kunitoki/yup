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

#include <memory>
#include <vector>

//==============================================================================
/** A place of a text, as an editor tells it: a line and a column, both from 1. */
struct YuiToolPlace
{
    int line = 0;
    int column = 0;
};

/** A statement of a block, as `width: 120` is: what ends with a semicolon. */
struct YuiToolStatement
{
    /** Where it starts and ends in the text, without the space around it nor its semicolon. */
    int start = 0;
    int end = 0;

    /** Its text, with its comments and the content of its strings blanked. */
    yup::String text;
};

/** What is written between two braces, with what is written before them. */
struct YuiToolBlock
{
    /** What is written before the brace, such as `drawing Knob` or `Slider`. Empty for the document itself. */
    yup::String header;

    /** Where the header starts, where the brace is, -1 for the document, and where the block ends:
        past its closing brace, or at the end of the text.
    */
    int start = 0;
    int open = -1;
    int end = 0;

    /** False for a block still missing its closing brace. */
    bool isClosed = false;

    const YuiToolBlock* parent = nullptr;
    std::vector<std::unique_ptr<YuiToolBlock>> blocks;
    std::vector<YuiToolStatement> statements;

    /** Returns true for a component of an interface, or a rule of a stylesheet naming a type: `Slider { }`. */
    bool isElement() const;

    /** Returns the first word of the header, such as `drawing` in `drawing Knob`. */
    yup::String getKeyword() const;
};

/** What a declaration holds that its users name: a property of a drawing or of a shader, a color of a palette, a token. */
struct YuiToolMember
{
    yup::String name;
    int offset = 0;

    /** The type of a property, the value of a color or of a token, `signal` for a signal. */
    yup::String detail;
};

/** A drawing, a shader, a palette, a component or the tokens a document declares. */
struct YuiToolDeclaration
{
    /** `drawing`, `shader`, `palette`, `component` or `tokens`. */
    yup::String kind;

    /** The name it is declared with, `tokens` for the tokens. */
    yup::String name;

    /** The type a component is made from, empty for the rest. */
    yup::String base;

    /** Where the name is written. */
    int offset = 0;

    const YuiToolBlock* block = nullptr;
    std::vector<YuiToolMember> members;
};

/** A file a document imports, with where its path is written between its quotes. */
struct YuiToolImport
{
    yup::String path;
    int start = 0;
    int end = 0;
};

/** A color written as `#rgb`, `#rgba`, `#rrggbb` or `#rrggbbaa`, each of its parts from 0 to 1. */
struct YuiToolColor
{
    int start = 0;
    int end = 0;
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    float alpha = 1.0f;
};

//==============================================================================
/** What a `.yui` text is made of, for what an editor asks of it: the blocks it is written in, what
    it declares and imports, and its colors.

    The text is scanned rather than parsed, so one that does not load is read too, as a document
    being typed is most of the time. Places are counted in characters from the start of the text.
*/
class YuiToolIndex
{
public:
    explicit YuiToolIndex (const yup::String& textToRead);

    /** Returns the text as one block holding those written at its top level. */
    const YuiToolBlock& getDocument() const noexcept { return document; }

    /** Returns the drawings, shaders, palettes, components and tokens declared, those of a `style` block included. */
    const std::vector<YuiToolDeclaration>& getDeclarations() const noexcept { return declarations; }

    /** Returns the declaration of a kind and a name, or nullptr. */
    const YuiToolDeclaration* findDeclaration (const yup::String& kind, const yup::String& name) const;

    /** Returns the files imported. */
    const std::vector<YuiToolImport>& getImports() const noexcept { return imports; }

    /** Returns the root component of the interface, or nullptr for a text of declarations alone. */
    const YuiToolBlock* getRoot() const noexcept { return root; }

    /** Returns the colors written, those of the comments, the strings and the ids of selectors left out. */
    std::vector<YuiToolColor> findColors() const;

    //==============================================================================
    /** Returns the blocks holding a place, the innermost last and the document left out.

        @param fromHeader True for a block to hold its header too, as the name of a component is part of it.
    */
    std::vector<const YuiToolBlock*> findBlocksAt (int offset, bool fromHeader = false) const;

    /** Returns the component a place is in: the innermost one of the interface, or of a component
        the text declares. nullptr in a stylesheet, a drawing or a palette, which hold no component.
    */
    const YuiToolBlock* findComponentAt (int offset) const;

    //==============================================================================
    /** Returns the number of characters of the text. */
    int getLength() const noexcept { return static_cast<int> (text.size()); }

    /** Returns where a place of an editor is in the text, brought inside its line. */
    int getOffset (YuiToolPlace place) const noexcept;

    /** Returns the line and the column of a place of the text. */
    YuiToolPlace getPlace (int offset) const noexcept;

    /** Returns where the line of a place starts. */
    int getLineStart (int offset) const noexcept;

    /** Returns a part of the text as it is written. */
    yup::String getText (int start, int end) const;

    /** Returns a part of the text with its comments and the content of its strings blanked. */
    yup::String getCode (int start, int end) const;

    /** Returns the character at a place of the text with its comments and strings blanked, 0 outside of it. */
    yup::yup_wchar getCodeAt (int offset) const noexcept;

    /** Returns the name written around a place, with where it starts, or an empty one. */
    yup::String findNameAt (int offset, int& start) const;

    /** Returns true for a character a name is made of: a letter, a digit or an underscore. */
    static bool isPartOfName (yup::yup_wchar character) noexcept;

private:
    void blankCommentsAndStrings();
    void readBlocks();
    void readDeclarations();
    void addDeclaration (const YuiToolBlock& block);
    void addMembers (YuiToolDeclaration& declaration) const;
    int readName (int& position, int end, yup::String& name) const;
    bool readCharacter (int& position, int end, yup::yup_wchar character) const;

    std::vector<yup::yup_wchar> text;
    std::vector<yup::yup_wchar> code;
    std::vector<int> lineStarts;
    YuiToolBlock document;
    std::vector<YuiToolDeclaration> declarations;
    std::vector<YuiToolImport> imports;
    const YuiToolBlock* root = nullptr;

    YUP_DECLARE_NON_COPYABLE (YuiToolIndex)
};

//==============================================================================
/** A file with what it is made of. */
struct YuiToolSource
{
    yup::File file;
    std::unique_ptr<YuiToolIndex> index;
};

/** Reads a file and those it imports, itself first: each is looked for next to the one naming it,
    then next to the first.
*/
std::vector<YuiToolSource> readYuiToolSources (const yup::File& file);
