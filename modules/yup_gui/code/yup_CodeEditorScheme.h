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
/** A named set of colors used to render a CodeEditor.

    A CodeEditorScheme groups every color the CodeEditor needs to paint itself
    (background, gutter, caret, selection, ...) together with the per-token
    syntax colors, so that switching scheme changes the whole look of the
    editor consistently.

    Colors are stored in a map keyed by Identifier and are accessed through
    setColor / getColor. The editor colors use the string constants in
    ColorId, while token colors use the same names as SyntaxDefinition's JSON
    format (e.g. "keyword", "string", "comment"), see tokenTypeToColorId.

    A default-constructed scheme carries the classic dark editor palette; the
    well-known presets (Monokai, Alabaster, One Dark, Solarized Dark and
    Solarized Light) are available through getBuiltIn.

    Example usage:
    @code
    CodeEditor editor (document);
    editor.setScheme (CodeEditorScheme::getBuiltIn ("monokai"));
    @endcode

    @see CodeEditor, SyntaxDefinition
*/
class YUP_API CodeEditorScheme
{
public:
    //==============================================================================
    /** String identifiers for the editor colors of a scheme. */
    struct ColorId
    {
        static inline const Identifier background { "background" };               /**< The editor background color. */
        static inline const Identifier text { "text" };                           /**< The default text color. */
        static inline const Identifier caret { "caret" };                         /**< The caret color. */
        static inline const Identifier currentLine { "currentLine" };             /**< The current-line highlight color. */
        static inline const Identifier gutterBackground { "gutterBackground" };   /**< The line-number gutter background color. */
        static inline const Identifier gutterText { "gutterText" };               /**< The line-number gutter text color. */
        static inline const Identifier selection { "selection" };                 /**< The text selection color. */
        static inline const Identifier searchHighlight { "searchHighlight" };     /**< The search-match highlight color. */
        static inline const Identifier breakpoint { "breakpoint" };               /**< The breakpoint marker color. */
        static inline const Identifier minimapBackground { "minimapBackground" }; /**< The minimap background color. */
        static inline const Identifier minimapForeground { "minimapForeground" }; /**< The minimap code overview color. */
        static inline const Identifier minimapViewport { "minimapViewport" };     /**< The minimap viewport indicator color. */
    };

    //==============================================================================
    /** Creates a scheme seeded with the default dark editor palette. */
    CodeEditorScheme();

    //==============================================================================
    /** Returns the display name of the scheme (e.g. "One Dark"), or an empty string. */
    const String& getName() const noexcept;

    /** Sets the display name of the scheme. */
    void setName (StringRef newName);

    //==============================================================================
    /** Sets a color for a given color identifier.

        @param colorId The color identifier (see ColorId).
        @param color   The color to use.
    */
    void setColor (const Identifier& colorId, Color color);

    /** Returns the color for a given color identifier, if the scheme provides one.

        @param colorId The color identifier (see ColorId).
        @returns The color, or std::nullopt when not set.
    */
    std::optional<Color> getColor (const Identifier& colorId) const;

    /** Returns true if the scheme provides a color for the given identifier. */
    bool hasColor (const Identifier& colorId) const;

    //==============================================================================
    /** Sets the color used to draw a syntax token type.

        @param tokenType The token type.
        @param color     The color to use.
    */
    void setColor (SyntaxDefinition::TokenType tokenType, Color color);

    /** Returns the color used to draw a syntax token type, if the scheme provides one.

        @param tokenType The token type.
        @returns The color, or std::nullopt when not set.
    */
    std::optional<Color> getColor (SyntaxDefinition::TokenType tokenType) const;

    //==============================================================================
    /** Returns the default scheme (the dark palette of a default-constructed scheme). */
    static const CodeEditorScheme& getDefault();

    /** Returns a built-in scheme by name.

        Matching is case-insensitive and ignores spaces and hyphens, so
        "One Dark", "oneDark" and "one-dark" all resolve to the same scheme.

        @param schemeName The scheme name.
        @returns The built-in scheme, or an empty inert one when unknown.
    */
    static const CodeEditorScheme& getBuiltIn (StringRef schemeName);

    /** Returns a built-in scheme by name, or nullptr when unknown.

        @param schemeName The scheme name.
        @returns The built-in scheme, or nullptr.
    */
    static const CodeEditorScheme* getBuiltInForName (StringRef schemeName);

    /** Returns the display names of all built-in schemes. */
    static std::vector<String> getAvailableSchemeNames();

    /** Returns the color identifier used for a syntax token type.

        The returned identifier matches the color names of the SyntaxDefinition
        JSON format, e.g. "keyword" for TokenType::keyword.
    */
    static Identifier tokenTypeToColorId (SyntaxDefinition::TokenType tokenType);

private:
    String name;
    std::unordered_map<Identifier, Color> colors;
};

} // namespace yup
