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

YuiDocument::YuiDocument() = default;

YuiDocument::~YuiDocument()
{
    clear();
}

//==============================================================================

Result YuiDocument::loadFromString (const String& text, YuiParser::ImportResolver resolveImport)
{
    const auto parsed = resolveImport != nullptr ? YuiParser::parse (text, resolveImport) : YuiParser::parse (text);

    if (! parsed.wasOk())
    {
        diagnostics = parsed.diagnostics;
        return notifyLoaded (Result::fail (diagnostics.front().toString()));
    }

    return load (parsed.document, parsed.sourceMap);
}

//==============================================================================
/** Loads one of two documents, the one before a load when undone and the loaded one when redone. */
class YuiDocument::LoadAction final : public UndoableAction
{
public:
    LoadAction (YuiDocument& documentToLoad, DataTree before, YuiSourceMap beforeMap, DataTree after, YuiSourceMap afterMap)
        : document (&documentToLoad)
        , previousTree (std::move (before))
        , previousMap (std::move (beforeMap))
        , loadedTree (std::move (after))
        , loadedMap (std::move (afterMap))
    {
    }

    bool isValid() const override
    {
        return document != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (document == nullptr)
            return false;

        const auto isRedo = state == UndoableActionState::Redo;
        const auto& toLoad = isRedo ? loadedTree : previousTree;

        if (! toLoad.isValid())
        {
            document->clear();
            return true;
        }

        result = document->loadTree (toLoad, isRedo ? loadedMap : previousMap);
        return result.wasOk();
    }

    Result result = Result::ok();

private:
    WeakReference<YuiDocument> document;
    DataTree previousTree;
    YuiSourceMap previousMap;
    DataTree loadedTree;
    YuiSourceMap loadedMap;
};

//==============================================================================

Result YuiDocument::load (const DataTree& document, const YuiSourceMap& map)
{
    if (undoManager == nullptr || ! undoManager->isEnabled())
        return loadTree (document, map);

    // The undo manager performs the load, and keeps it only when it succeeds
    ReferenceCountedObjectPtr<LoadAction> action (new LoadAction (*this, tree.isValid() ? tree.clone() : DataTree(), sourceMap, document.clone(), map));

    undoManager->beginNewTransaction();
    undoManager->perform (action.get());

    return action->result;
}

Result YuiDocument::loadTree (const DataTree& document, const YuiSourceMap& map)
{
    return notifyLoaded (buildTree (document, map));
}

Result YuiDocument::notifyLoaded (Result result)
{
    listeners.call ([&] (Listener& listener)
    {
        listener.documentLoaded (*this, result);
    });

    return result;
}

void YuiDocument::report (const YuiSourceMap& map, const YuiDiagnostic& diagnostic)
{
    const auto located = map.locate (diagnostic);
    const auto text = located.toString();

    // Styling reports the same problem again each time a component is restyled
    const auto isKnown = std::any_of (diagnostics.begin(), diagnostics.end(), [&text] (const YuiDiagnostic& known)
    {
        return known.toString() == text;
    });

    if (isKnown)
        return;

    diagnostics.push_back (located);

    if (! isLoading)
    {
        listeners.call ([&] (Listener& listener)
        {
            listener.documentDiagnosed (*this, located);
        });
    }
}

Result YuiDocument::buildTree (const DataTree& document, const YuiSourceMap& map)
{
    const ScopedValueSetter<bool> loading (isLoading, true);
    diagnostics.clear();

    YuiDiagnostic failure;
    auto sheet = StyleSheet::fromTree (document, &failure);

    if (sheet.failed())
    {
        report (map, failure);
        return Result::fail (diagnostics.back().toString());
    }

    // The builder keeps the components of the elements still in the patched tree
    const auto previousTree = tree;
    const auto isPatched = tree.isValid() && tree.getType() == document.getType();
    UndoManager rollback (RelativeTime {});

    if (isPatched)
        YuiTreeDiff::apply (tree, document, &rollback);
    else
        tree = document.clone();

    if (auto result = builder.build (tree); result.failed())
    {
        // Located before the rollback, which detaches the elements the patch added
        for (const auto& diagnostic : builder.getDiagnostics())
            report (map, diagnostic);

        // The error that made the build fail is the last one reported
        const auto failure = map.locate (builder.getDiagnostics().back()).toString();

        // Restoring the same elements lets the next load keep the components still shown
        if (isPatched)
            rollback.undo();
        else
            tree = previousTree;

        return Result::fail (failure);
    }

    styleSheet = sheet.getReference();
    sourceMap = map;

    auto& root = *builder.getRoot();
    root.setStyleSheet (styleSheet);

    for (const auto& diagnostic : builder.getDiagnostics())
        report (map, diagnostic);

    // The style problems met later, when a rule starts matching, are reported too
    auto& context = *root.getStyleContext();
    context.onDiagnostic = [this] (const YuiDiagnostic& diagnostic)
    {
        report (sourceMap, diagnostic);
    };

    context.restyleNow();
    return Result::ok();
}

void YuiDocument::clear()
{
    builder.clear();
    tree = {};
    sourceMap = {};
    styleSheet = nullptr;
    diagnostics.clear();
}

//==============================================================================

void YuiDocument::setAction (const String& name, YuiBuilder::Action action)
{
    builder.setAction (name, std::move (action));
}

void YuiDocument::setModel (const String& name, BindingProvider& provider)
{
    builder.setModel (name, provider);
}

void YuiDocument::removeModel (const String& name)
{
    builder.removeModel (name);
}

} // namespace yup
