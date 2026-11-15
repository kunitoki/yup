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

std::vector<std::pair<int, DataTree>> findStyleElements (const DataTree& document)
{
    static const StringArray types { "style", "rule", "palette", "drawing", "keyframes" };
    std::vector<std::pair<int, DataTree>> found;

    for (int i = 0; i < document.getNumChildren(); ++i)
        if (const auto child = document.getChild (i); types.contains (child.getType().toString()))
            found.emplace_back (i, child);

    return found;
}

/** True when both documents hold equivalent style elements at the same positions, so a stylesheet read from one
    describes the other too, down to where each declaration is written.
*/
bool hasSameStyle (const DataTree& a, const DataTree& b)
{
    return std::ranges::equal (findStyleElements (a), findStyleElements (b), [] (const auto& x, const auto& y)
    {
        return x.first == y.first && x.second.isEquivalentTo (y.second);
    });
}

} // namespace

//==============================================================================

YuiDocument::YuiDocument() = default;

YuiDocument::~YuiDocument()
{
    resetState();
}

//==============================================================================

Result YuiDocument::loadFromString (const String& text, YuiParser::ImportResolver resolveImport)
{
    const auto parsed = resolveImport != nullptr ? YuiParser::parse (text, resolveImport) : YuiParser::parse (text);

    if (! parsed.wasOk())
    {
        sourceMap = {};
        lastLoadSucceeded = false;
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
    ReferenceCountedObjectPtr<LoadAction> action (new LoadAction (*this, tree.isValid() ? tree.clone() : DataTree(), loadedSourceMap, document.clone(), map));

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

    // An unchanged style keeps the stylesheet, so nothing is restyled from scratch and transitions keep running
    auto sheet = styleSheet;

    if (sheet == nullptr || ! hasSameStyle (tree, document))
    {
        YuiDiagnostic failure;
        auto parsed = StyleSheet::fromTree (document, &failure);

        if (parsed.failed())
        {
            sourceMap = {};
            lastLoadSucceeded = false;
            report (map, failure);
            return Result::fail (diagnostics.back().toString());
        }

        sheet = parsed.getReference();
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
        sourceMap = {};
        lastLoadSucceeded = false;

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

    styleSheet = sheet;
    sourceMap = map;
    loadedSourceMap = map;
    lastLoadSucceeded = true;

    auto& root = *builder.getRoot();
    root.setStyleSheet (styleSheet);

    for (const auto& diagnostic : builder.getDiagnostics())
        report (map, diagnostic);

    // The style problems met later, when a rule starts matching, are reported too
    auto& context = *root.getStyleContext();
    context.setComponentResolver (builder.getComponentResolver());
    context.setModelResolver (builder.getModelResolver());
    context.onDiagnostic = [this] (const YuiDiagnostic& diagnostic)
    {
        if (lastLoadSucceeded)
            report (sourceMap, diagnostic);
    };

    context.restyleNow();
    return Result::ok();
}

void YuiDocument::resetState()
{
    builder.clear();
    tree = {};
    sourceMap = {};
    loadedSourceMap = {};
    styleSheet = nullptr;
    diagnostics.clear();
    lastLoadSucceeded = false;
}

void YuiDocument::clear()
{
    resetState();
    notifyLoaded (Result::ok());
}

//==============================================================================

void YuiDocument::setAction (const String& name, YuiBuilder::Action action)
{
    builder.setAction (name, std::move (action));
}

void YuiDocument::setModel (const String& name, BindingProvider& provider)
{
    builder.setModel (name, provider);

    if (auto* root = builder.getRoot())
        if (auto* context = root->getStyleContext())
            context->setModelResolver (builder.getModelResolver());
}

void YuiDocument::removeModel (const String& name)
{
    builder.removeModel (name);

    if (auto* root = builder.getRoot())
        if (auto* context = root->getStyleContext())
            context->setModelResolver (builder.getModelResolver());
}

} // namespace yup
