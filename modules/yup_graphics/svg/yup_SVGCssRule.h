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

namespace yup
{

/** A single parsed CSS rule from an SVG <style> element. */
struct SVGCssRule
{
    String selector;
    StringArray declarations;
    int specificity = 0;
    int order = 0;
};

//==============================================================================
/** Index for fast CSS rule lookup by tag name, id, and class name.

    Built after all CSS rules are parsed and used by SVGCssParser::applyStylesheetRules
    to avoid O(N×M) brute-force matching of every rule against every element.
*/
struct SVGCssRuleIndex
{
    /** Maps a tag name (e.g. "path", "rect") to the indices of matching rules. */
    HashMap<String, std::vector<int>> byTag;

    /** Maps an id (e.g. "myId") to the indices of matching rules. */
    HashMap<String, std::vector<int>> byId;

    /** Maps a class name (e.g. "myClass") to the indices of matching rules. */
    HashMap<String, std::vector<int>> byClass;

    /** Builds the index from a vector of rules. */
    void buildFrom (const std::vector<SVGCssRule>& rules)
    {
        for (int i = 0; i < static_cast<int> (rules.size()); ++i)
        {
            const auto& rule = rules[i];
            const auto& sel = rule.selector;

            const auto hashIndex = sel.indexOf ("#");
            const auto dotIndex = sel.indexOf (".");
            const auto splitIndex = (hashIndex >= 0 && dotIndex >= 0)
                                      ? jmin (hashIndex, dotIndex)
                                      : jmax (hashIndex, dotIndex);

            if (hashIndex >= 0)
            {
                const auto idEnd = (dotIndex > hashIndex) ? dotIndex : sel.length();
                auto id = sel.substring (hashIndex + 1, idEnd);
                if (id.isNotEmpty())
                    byId.getReference (id).push_back (i);
            }

            if (dotIndex >= 0)
            {
                auto className = sel.substring (dotIndex + 1);
                if (className.isNotEmpty())
                    byClass.getReference (className).push_back (i);
            }

            if (splitIndex != 0 && ! sel.startsWithChar ('#') && ! sel.startsWithChar ('.'))
            {
                auto tagName = (splitIndex > 0) ? sel.substring (0, splitIndex) : sel;
                if (tagName.isNotEmpty())
                    byTag.getReference (tagName).push_back (i);
            }
        }
    }
};

} // namespace yup
