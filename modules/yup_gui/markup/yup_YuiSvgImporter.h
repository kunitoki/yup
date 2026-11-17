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

namespace yup
{

//==============================================================================
/**
    Converts an SVG into a `.yui` drawing made of native elements.

    A drawing can show an SVG as it is with its `svg` element, which paints it through a
    Drawable. The importer writes the same picture with the elements of the drawing language
    instead, so its geometry, paints and transforms can be bound to the properties of a
    component, animated or edited by hand.

    The drawing paints what a Drawable paints for the same document, fitted in the bounds of
    the component by its `viewBox`:

    - `rect`, `circle`, `ellipse` and `line` stay shapes of their own, everything else is a `path`;
    - groups keep their structure, and set the paint their shapes share;
    - gradients become `let` values, mapped to the `objectBounds` of each shape when they use
      the units of its bounding box;
    - `<symbol>` and the other elements instanced by `<use>` become nested drawings, painted
      with `use`;
    - patterns, markers, clip paths, masks, nested `<svg>` viewports, images and text become
      `pattern`, `marker`, `clipPath`, `viewport`, `image` and anchored `text` elements;
    - blur and blend filters become `feather` and `blend`.

    @code
    const auto imported = YuiSvgImporter::importSvg (File ("logo.svg"));

    if (imported.wasOk())
    {
        file.replaceWithText (imported.toText());                // drawing Imported { ... }
        auto style = DrawingStyle::create (component, imported.drawing);
    }
    @endcode

    @see DrawingStyle, SVGDocument, Drawable, YuiWriter

    @tags{UI}
*/
class YUP_API YuiSvgImporter
{
public:
    //==============================================================================
    /** How an SVG is written as a drawing. */
    struct Options
    {
        /** Creates the default options. */
        Options() noexcept {}

        /** The name of the drawing: letters, digits and `_`, starting with a letter. */
        String name = "Imported";

        /** Returns the expression giving the font of a text, for example `defaultFont.withHeight (12)`.

            Text is written with `defaultFont` at its size when this is not set, or when it
            returns an empty text.

            @param family  The CSS font-family of the text, which may be empty.
            @param size    The font size.
            @param weight  The CSS font-weight (400 is normal, 700 is bold).
            @param italic  True when the font-style is italic or oblique.
        */
        std::function<String (StringRef family, float size, int weight, bool italic)> fontExpression;
    };

    //==============================================================================
    /** The outcome of importing an SVG. */
    struct ImportResult
    {
        /** The `drawing` element, as YuiParser makes it, invalid when the SVG can't be read. */
        DataTree drawing;

        /** What is not painted exactly as a Drawable paints it, empty when everything is. */
        StringArray warnings;

        /** Returns true if the SVG was read. */
        bool wasOk() const noexcept { return drawing.isValid(); }

        /** Returns the drawing as `.yui` text, or an empty text when the SVG wasn't read. */
        String toText() const;
    };

    //==============================================================================
    /** Converts a parsed SVG document.

        @param document  The document to convert.
        @param options   How to write it.

        @returns The drawing, and the warnings about what it paints differently.
    */
    static ImportResult importSvg (const SVGDocument& document, const Options& options = {});

    /** Parses and converts the text of an SVG.

        @param svgText       The text of the SVG.
        @param options       How to write it.
        @param parseOptions  How to parse it, such as where its images are.

        @returns The drawing, invalid when the text is not an SVG.
    */
    static ImportResult importSvg (StringRef svgText, const Options& options = {}, const SVGDocument::ParseOptions& parseOptions = {});

    /** Parses and converts an SVG file.

        @param svgFile       The file to read.
        @param options       How to write it.
        @param parseOptions  How to parse it, its images being looked up next to the file by default.

        @returns The drawing, invalid when the file is not an SVG.
    */
    static ImportResult importSvg (const File& svgFile, const Options& options = {}, const SVGDocument::ParseOptions& parseOptions = {});

    /** Deleted constructor to prevent instantiation of this utility class. */
    YuiSvgImporter() = delete;

private:
    class Converter;
};

} // namespace yup
