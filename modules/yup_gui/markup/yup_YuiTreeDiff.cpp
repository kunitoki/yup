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

namespace
{

String getTreeDiffId (const DataTree& element)
{
    return element.getProperty ("id").toString().unquoted().trim();
}

bool hasSamePropertyNames (const DataTree& target, const DataTree& source)
{
    if (target.getNumProperties() != source.getNumProperties())
        return false;

    for (int i = 0; i < source.getNumProperties(); ++i)
    {
        if (target.getPropertyName (i) != source.getPropertyName (i))
            return false;
    }

    return true;
}

bool patchProperties (const DataTree& target, const DataTree& source, DataTree::Transaction& transaction)
{
    const auto sameNames = hasSamePropertyNames (target, source);
    auto changed = ! sameNames;

    if (! sameNames)
        transaction.removeAllProperties();

    for (int i = 0; i < source.getNumProperties(); ++i)
    {
        const auto name = source.getPropertyName (i);
        const auto value = source.getProperty (name);

        if (! sameNames || ! target.getProperty (name).equalsWithSameType (value))
        {
            transaction.setProperty (name, value);
            changed = true;
        }
    }

    return changed;
}

std::vector<DataTree> matchChildren (const DataTree& target, const DataTree& source)
{
    std::vector<DataTree> matches (static_cast<size_t> (source.getNumChildren()));
    std::vector<bool> used (static_cast<size_t> (target.getNumChildren()), false);

    const auto match = [&] (int sourceIndex, bool byId)
    {
        const auto sourceChild = source.getChild (sourceIndex);
        const auto id = getTreeDiffId (sourceChild);

        if (byId && id.isEmpty())
            return;

        for (int i = 0; i < target.getNumChildren(); ++i)
        {
            const auto targetChild = target.getChild (i);

            if (used[static_cast<size_t> (i)] || targetChild.getType() != sourceChild.getType())
                continue;

            if (byId && getTreeDiffId (targetChild) != id)
                continue;

            matches[static_cast<size_t> (sourceIndex)] = targetChild;
            used[static_cast<size_t> (i)] = true;
            return;
        }
    };

    for (int j = 0; j < source.getNumChildren(); ++j)
        match (j, true);

    for (int j = 0; j < source.getNumChildren(); ++j)
    {
        if (! matches[static_cast<size_t> (j)].isValid())
            match (j, false);
    }

    return matches;
}

bool patchTree (DataTree& target, const DataTree& source, UndoManager* undoManager)
{
    auto matches = matchChildren (target, source);

    // Queued child changes apply one after the other, so the indices follow a simulated list
    std::vector<DataTree> children;
    auto transaction = target.beginTransaction (undoManager);
    auto changed = patchProperties (target, source, transaction);

    for (const auto& child : target)
    {
        if (std::find (matches.begin(), matches.end(), child) != matches.end())
        {
            children.push_back (child);
            continue;
        }

        transaction.removeChild (child);
        changed = true;
    }

    for (int j = 0; j < source.getNumChildren(); ++j)
    {
        const auto& match = matches[static_cast<size_t> (j)];

        if (! match.isValid())
        {
            const auto copy = source.getChild (j).clone();
            transaction.addChild (copy, j);
            children.insert (children.begin() + j, copy);
            changed = true;
            continue;
        }

        const auto current = static_cast<int> (std::find (children.begin() + j, children.end(), match) - children.begin());
        if (current == j)
            continue;

        transaction.moveChild (current, j);
        std::rotate (children.begin() + j, children.begin() + current, children.begin() + current + 1);
        changed = true;
    }

    transaction.commit();

    for (int j = 0; j < source.getNumChildren(); ++j)
    {
        if (auto& match = matches[static_cast<size_t> (j)]; match.isValid())
            changed = patchTree (match, source.getChild (j), undoManager) || changed;
    }

    return changed;
}

} // namespace

//==============================================================================
bool YuiTreeDiff::apply (DataTree& target, const DataTree& source, UndoManager* undoManager)
{
    jassert (target.getType() == source.getType());
    if (target.getType() != source.getType())
        return false;

    std::optional<UndoManager::ScopedTransaction> scopedTransaction;
    if (undoManager != nullptr && undoManager->isEnabled())
        scopedTransaction.emplace (*undoManager);

    return patchTree (target, source, undoManager);
}

} // namespace yup
