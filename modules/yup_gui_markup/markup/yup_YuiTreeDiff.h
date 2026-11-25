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
    Patches a `.yui` document tree in place so it becomes equivalent to another one.

    The elements of the target that are still in the source are kept, so anything holding them
    (or keyed by them, like YuiBuilder keys its components) survives the patch. Children are
    matched first by `id` with the same type, then each remaining source child takes the first
    unmatched target child of the same type, in order, so renaming an id keeps the element.
    Unmatched target children are removed, unmatched source children are added as copies, and
    the matched ones are moved into the source order and patched recursively.

    Properties keep the order of the source, since the order is the order YuiBuilder applies
    them in: when the names or their order differ, the properties are replaced, otherwise only
    the changed values are set.

    @code
    const auto result = YuiParser::parse (text);
    if (result.wasOk())
        YuiTreeDiff::apply (liveDocument, result.document, &undoManager);
    @endcode

    @see YuiDocument, YuiParser

    @tags{UI}
*/
class YUP_API YuiTreeDiff
{
public:
    //==============================================================================
    /** Patches a tree so it becomes equivalent to another.

        @param target       The tree to patch. It must have the same type as the source.
        @param source       The tree to copy, left untouched.
        @param undoManager  An optional undo manager recording the changes; they go to its
                            current transaction, start one first to undo them in one step.

        @returns True if anything changed.
    */
    static bool apply (DataTree& target, const DataTree& source, UndoManager* undoManager = nullptr);

    /** Deleted constructor to prevent instantiation of this utility class. */
    YuiTreeDiff() = delete;
};

} // namespace yup
