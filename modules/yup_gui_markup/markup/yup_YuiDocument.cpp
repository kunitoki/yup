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
    std::vector<std::pair<int, DataTree>> found;

    for (int i = 0; i < document.getNumChildren(); ++i)
        if (const auto child = document.getChild (i); StyleSheet::isStyleElement (child))
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

var makeSchemaProperty (const String& type, bool required = false, const String& description = {})
{
    DynamicObject::Ptr prop (new DynamicObject());
    prop->setProperty ("type", type);

    if (required)
        prop->setProperty ("required", true);

    if (description.isNotEmpty())
        prop->setProperty ("description", description);

    return var (prop.get());
}

var makeSchemaNode (DynamicObject::Ptr properties, const StringArray& allowedChildren = {}, int maxChildren = -1)
{
    DynamicObject::Ptr node (new DynamicObject());

    if (properties != nullptr)
        node->setProperty ("properties", var (properties.get()));

    if (! allowedChildren.isEmpty() || maxChildren >= 0)
    {
        DynamicObject::Ptr children (new DynamicObject());

        if (! allowedChildren.isEmpty())
        {
            Array<var> allowed;
            allowed.ensureStorageAllocated (allowedChildren.size());

            for (const auto& childType : allowedChildren)
                allowed.add (childType);

            children->setProperty ("allowedTypes", var (allowed));
        }

        if (maxChildren >= 0)
            children->setProperty ("maxCount", maxChildren);

        node->setProperty ("children", var (children.get()));
    }

    return var (node.get());
}

bool isGrammarNodeType (const Identifier& type) noexcept
{
    static const StringArray grammarTypes {
        "yui", "import", "tokens", "palette", "style", "rule", "keyframes", "keyframe",
        "declaration", "property", "alias", "signal", "argument", "component", "drawing", "shader",
        "let", "repeat", "group", "clip", "layer", "mask", "rect", "ellipse", "circle", "line",
        "arc", "path", "polygon", "star", "text", "image", "svg", "bind", "changed",
        "viewport", "use", "pattern", "marker", "clipPath", "filter", "blur", "offset", "flood",
        "colorMatrix", "composite", "blend", "morphology", "lighting", "displacement", "dropShadow"
    };

    return grammarTypes.contains (type.toString());
}

std::optional<DataTreeDiagnostic> validateGrammarNodes (const DataTreeSchema& schema, const DataTree& node, bool isAmongComponents = false)
{
    if (! node.isValid())
        return DataTreeDiagnostic { .message = "Invalid DataTree" };

    const auto nodeType = node.getType();
    static const Identifier repeatType ("repeat");

    // A repeat among components holds components, where the one of a drawing holds what a drawing does
    const auto repeatsComponents = isAmongComponents && nodeType == repeatType;

    if (isGrammarNodeType (nodeType))
    {
        const auto propNames = schema.getPropertyNames (nodeType);

        for (const auto& propNameText : propNames)
        {
            const Identifier propName (propNameText);
            const auto info = schema.getPropertyInfo (nodeType, propName);

            if (info.required && ! node.hasProperty (propName))
                return DataTreeDiagnostic { .message = "Required property '" + propNameText + "' is missing on '" + nodeType.toString() + "'", .element = node };

            if (node.hasProperty (propName))
            {
                const auto check = schema.validatePropertyValue (nodeType, propName, node.getProperty (propName));
                if (check.failed())
                    return DataTreeDiagnostic { .message = check.getErrorMessage(), .element = node, .property = propName };
            }
        }

        if (repeatsComponents)
        {
            for (const auto& child : node)
            {
                if (child.getType() != repeatType && ! detail::isYuiComponentElement (child))
                    return DataTreeDiagnostic { .message = "A repeat of components holds components and other repeats, not '" + child.getType().toString() + "'", .element = child };
            }
        }
        else if (nodeType.toString() != "yui" && nodeType.toString() != "component")
        {
            for (int i = 0; i < node.getNumChildren(); ++i)
            {
                const auto child = node.getChild (i);
                const auto childCheck = schema.validateChildAddition (nodeType, child.getType(), i);
                if (childCheck.failed())
                    return DataTreeDiagnostic { .message = childCheck.getErrorMessage(), .element = child };
            }
        }
    }

    const auto holdsComponents = repeatsComponents || detail::isYuiComponentElement (node);

    for (int i = 0; i < node.getNumChildren(); ++i)
        if (auto problem = validateGrammarNodes (schema, node.getChild (i), holdsComponents))
            return problem;

    return std::nullopt;
}

// A problem is listed once: what it says and where it is written tell it from the others
bool isSameProblem (const DataTreeDiagnostic& a, const DataTreeDiagnostic& b)
{
    return a.line == b.line
        && a.column == b.column
        && a.source == b.source
        && a.message == b.message;
}

} // namespace

//==============================================================================

DataTreeSchema::Ptr YuiDocument::createSchema()
{
    registerYuiComponentTypes();

    const auto registeredTypes = ComponentTypeRegistry::getInstance().getRegisteredTypes();

    StringArray componentTypeNames;
    for (const auto* info : registeredTypes)
        componentTypeNames.add (info->getTypeName().toString());

    StringArray yuiChildren { "import", "tokens", "palette", "style", "drawing", "shader", "component" };
    yuiChildren.addArray (componentTypeNames);

    const StringArray drawingBodyChildren {
        "let", "repeat", "group", "clip", "layer", "viewport", "use", "drawing", "pattern", "marker",
        "rect", "ellipse", "circle", "line", "arc", "path", "polygon", "star", "text", "image", "svg"
    };
    const StringArray clipPathChildren { "let", "repeat", "rect", "ellipse", "circle", "line", "arc", "path", "polygon", "star" };
    StringArray clipChildren = drawingBodyChildren;
    clipChildren.add ("clipPath");
    StringArray layerChildren = clipChildren;
    layerChildren.add ("mask");
    layerChildren.add ("filter");
    const StringArray filterChildren { "blur", "offset", "flood", "colorMatrix", "composite", "blend", "morphology", "lighting", "displacement", "dropShadow" };
    StringArray drawingChildren { "property" };
    drawingChildren.addArray (drawingBodyChildren);

    DynamicObject::Ptr nodeTypes (new DynamicObject());

    nodeTypes->setProperty ("yui", makeSchemaNode (nullptr, yuiChildren));

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("path", makeSchemaProperty ("string", true, "Imported .yui path."));
        nodeTypes->setProperty ("import", makeSchemaNode (props, {}, 0));
    }

    nodeTypes->setProperty ("tokens", makeSchemaNode (nullptr, { "property" }));

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true, "Palette name."));
        props->setProperty ("colors", makeSchemaProperty ("string", false, "Seed color list."));
        props->setProperty ("mode", makeSchemaProperty ("string", false, "Palette mode (dark or light)."));
        nodeTypes->setProperty ("palette", makeSchemaNode (props, {}, 0));
    }

    nodeTypes->setProperty ("style", makeSchemaNode (nullptr, { "rule", "palette", "drawing", "keyframes", "property" }));

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("selector", makeSchemaProperty ("string", true, "CSS-like rule selector."));
        nodeTypes->setProperty ("rule", makeSchemaNode (props, { "declaration", "property" }));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true, "Keyframe sequence name."));
        nodeTypes->setProperty ("keyframes", makeSchemaNode (props, { "keyframe" }));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        DynamicObject::Ptr atProp (new DynamicObject());
        atProp->setProperty ("type", "number");
        atProp->setProperty ("required", true);
        atProp->setProperty ("minimum", 0.0);
        atProp->setProperty ("maximum", 1.0);
        props->setProperty ("at", var (atProp.get()));
        nodeTypes->setProperty ("keyframe", makeSchemaNode (props, { "declaration" }));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true));
        props->setProperty ("value", makeSchemaProperty ("string", true));
        nodeTypes->setProperty ("declaration", makeSchemaNode (props, {}, 0));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true));
        props->setProperty ("type", makeSchemaProperty ("string", true));
        props->setProperty ("value", makeSchemaProperty ("string", false));
        nodeTypes->setProperty ("property", makeSchemaNode (props, {}, 0));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true));
        props->setProperty ("target", makeSchemaProperty ("string", true));
        nodeTypes->setProperty ("alias", makeSchemaNode (props, {}, 0));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true));
        nodeTypes->setProperty ("signal", makeSchemaNode (props, { "argument" }));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true));
        props->setProperty ("type", makeSchemaProperty ("string", true));
        nodeTypes->setProperty ("argument", makeSchemaNode (props, {}, 0));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true));
        props->setProperty ("base", makeSchemaProperty ("string", true));
        nodeTypes->setProperty ("component", makeSchemaNode (props));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", false));
        nodeTypes->setProperty ("drawing", makeSchemaNode (props, drawingChildren));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true, "Shader name."));
        props->setProperty ("fragment", makeSchemaProperty ("string", false, "Path of the fragment shader, GLSL source or a compiled bundle."));
        props->setProperty ("material", makeSchemaProperty ("string", false, "Path of a material function for 3D meshes."));
        nodeTypes->setProperty ("shader", makeSchemaNode (props, { "property" }));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true));
        props->setProperty ("value", makeSchemaProperty ("string", true));
        nodeTypes->setProperty ("let", makeSchemaNode (props, {}, 0));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true));
        props->setProperty ("from", makeSchemaProperty ("string", true));
        props->setProperty ("to", makeSchemaProperty ("string", true));
        nodeTypes->setProperty ("repeat", makeSchemaNode (props, drawingBodyChildren));
    }

    for (const auto* containerType : { "group", "mask", "viewport" })
        nodeTypes->setProperty (containerType, makeSchemaNode (nullptr, drawingBodyChildren));

    nodeTypes->setProperty ("clip", makeSchemaNode (nullptr, clipChildren));
    nodeTypes->setProperty ("layer", makeSchemaNode (nullptr, layerChildren));
    nodeTypes->setProperty ("clipPath", makeSchemaNode (nullptr, clipPathChildren));
    {
        // A filter also runs the shaders a `use` names
        auto filterElements = filterChildren;
        filterElements.add ("use");
        nodeTypes->setProperty ("filter", makeSchemaNode (nullptr, filterElements));
    }

    for (const auto& primitiveType : filterChildren)
        nodeTypes->setProperty (Identifier (primitiveType), makeSchemaNode (nullptr, {}, 0));

    for (const auto* definitionType : { "pattern", "marker" })
    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("name", makeSchemaProperty ("string", true));
        nodeTypes->setProperty (definitionType, makeSchemaNode (props, drawingBodyChildren));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("drawing", makeSchemaProperty ("string", true, "Name of the drawing to paint, or of the shader to paint or to run in a filter."));
        nodeTypes->setProperty ("use", makeSchemaNode (props, {}, 0));
    }

    for (const auto* leafType : { "rect", "ellipse", "circle", "line", "arc", "path", "polygon", "star", "text", "image", "svg" })
        nodeTypes->setProperty (leafType, makeSchemaNode (nullptr, {}, 0));

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("property", makeSchemaProperty ("string", true));
        props->setProperty ("target", makeSchemaProperty ("string", true));
        nodeTypes->setProperty ("bind", makeSchemaNode (props, {}, 0));
    }

    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("property", makeSchemaProperty ("string", true));
        props->setProperty ("handler", makeSchemaProperty ("string", true));
        nodeTypes->setProperty ("changed", makeSchemaNode (props, {}, 0));
    }

    for (const auto* typeInfo : registeredTypes)
    {
        DynamicObject::Ptr props (new DynamicObject());
        props->setProperty ("id", makeSchemaProperty ("string", false, "Component identifier."));
        props->setProperty ("class", makeSchemaProperty ("string", false, "Space-separated style classes."));
        props->setProperty ("palette", makeSchemaProperty ("string", false, "Scoped palette name or 'none'."));
        props->setProperty ("drawing", makeSchemaProperty ("string", false, "Instance drawing name or 'none'."));

        for (const auto* current = typeInfo; current != nullptr; current = current->getBaseTypeInfo())
        {
            for (const auto& prop : current->getOwnProperties())
                if (! props->hasProperty (prop.name))
                    props->setProperty (prop.name, makeSchemaProperty ("string", false, prop.description));

            for (const auto& sig : current->getOwnSignals())
                if (! props->hasProperty (sig.name))
                    props->setProperty (sig.name, makeSchemaProperty ("string", false, sig.description));
        }

        nodeTypes->setProperty (typeInfo->getTypeName(), makeSchemaNode (props));
    }

    DynamicObject::Ptr rootSchema (new DynamicObject());
    rootSchema->setProperty ("nodeTypes", var (nodeTypes.get()));
    return DataTreeSchema::fromJsonSchema (var (rootSchema.get()));
}

//==============================================================================

YuiDocument::YuiDocument()
{
    registerDocument();

    builder.onDiagnostic = [this] (const DataTreeDiagnostic& diagnostic)
    {
        if (! isLoading && lastLoadSucceeded)
            report (sourceMap, diagnostic);
    };

    builder.onTreeMutated = [this]
    {
        if (! isLoading && lastLoadSucceeded && tree.isValid())
            handleLiveTreeMutation();
    };

    builder.onRangeChanged = [this]
    {
        if (! isLoading && tree.isValid())
            handleRangeChange();
    };
}

YuiDocument::~YuiDocument()
{
    // First: the components destroyed below may look for their document
    unregisterDocument();

    resetState();
}

YuiDocument* YuiDocument::findDocumentFor (const Component* component)
{
    return dynamic_cast<YuiDocument*> (findFor (component));
}

std::vector<YuiDocument*> YuiDocument::getAllDocuments()
{
    std::vector<YuiDocument*> documents;

    for (auto* registered : getRegisteredDocuments())
    {
        if (auto* document = dynamic_cast<YuiDocument*> (registered))
            documents.push_back (document);
    }

    return documents;
}

//==============================================================================

std::optional<MemoryBlock> YuiDocument::readAsset (const String& path) const
{
    if (path.isEmpty())
        return std::nullopt;

    if (assetReader != nullptr)
    {
        if (auto bytes = assetReader (path))
            return bytes;
    }

    return readAssetFile (resolveAssetFile (path, getBaseDirectory()));
}

File YuiDocument::getAssetFile (const String& path) const
{
    // What a reader serves with no folder to look into isn't a file
    if (assetReader != nullptr && getBaseDirectory() == File() && ! File::isAbsolutePath (path))
        return {};

    return resolveAssetFile (path, getBaseDirectory());
}

void YuiDocument::notifyAssetChanged (const String& path)
{
    // The stylesheet is told first: it loads its shaders again when it next restyles
    if (auto* root = getRoot())
        if (auto* context = root->getStyleContext())
            context->assetChanged (path);

    // A listener may destroy the document
    const WeakReference<YuiDocument> self (this);

    listeners.call ([&] (Listener& listener)
    {
        listener.documentAssetChanged (*this, path);
    });

    if (self != nullptr)
        notifyDocumentAssetChanged (path);
}

void YuiDocument::reportDiagnostic (DataTreeDiagnostic diagnostic, const Component* subject)
{
    // Where the builder sends the problems its bindings meet, which are dropped while the document loads
    if (builder.onDiagnostic == nullptr)
        return;

    if (subject != nullptr && ! diagnostic.element.isValid())
        diagnostic.element = builder.findElementFor (subject);

    builder.onDiagnostic (diagnostic);
}

//==============================================================================

Result YuiDocument::loadFromString (const String& text, YuiParser::ImportResolver resolveImport)
{
    const auto parsed = YuiParser::parse (text, resolveImport);

    if (! parsed.wasOk())
    {
        diagnostics = parsed.diagnostics;
        return notifyLoaded (fail (diagnostics.front().toString()));
    }

    return load (parsed.document, parsed.sourceMap);
}

//==============================================================================
/** Loads one of two documents, the one before a load when undone and the loaded one when redone. */
class YuiDocument::LoadAction final : public UndoableAction
{
public:
    LoadAction (YuiDocument& documentToLoad,
                DataTree before,
                std::shared_ptr<const DataTreeSourceMap> beforeMap,
                DataTree after,
                std::shared_ptr<const DataTreeSourceMap> afterMap)
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
    std::shared_ptr<const DataTreeSourceMap> previousMap;
    DataTree loadedTree;
    std::shared_ptr<const DataTreeSourceMap> loadedMap;
};

//==============================================================================

Result YuiDocument::load (const DataTree& document, const DataTreeSourceMap& map)
{
    auto sharedMap = std::make_shared<const DataTreeSourceMap> (map);

    // The document that is loaded, the same elements written at the same places, is loaded again for what it reads
    // to be read again: no step to undo
    if (undoManager == nullptr
        || ! undoManager->isEnabled()
        || (tree.isValid() && sourceMap != nullptr && tree.isEquivalentTo (document) && *sourceMap == map))
    {
        return loadTree (document, sharedMap);
    }

    // The undo manager performs the load, and keeps it only when it succeeds
    ReferenceCountedObjectPtr<LoadAction> action (new LoadAction (*this, tree.isValid() ? tree.clone() : DataTree(), sourceMap, document.clone(), std::move (sharedMap)));

    undoManager->beginNewTransaction();
    undoManager->perform (action.get());

    return action->result;
}

Result YuiDocument::loadTree (const DataTree& document, const std::shared_ptr<const DataTreeSourceMap>& map)
{
    return notifyLoaded (buildTree (document, map));
}

Result YuiDocument::fail (const String& message)
{
    lastLoadSucceeded = false;
    return Result::fail (message);
}

Result YuiDocument::notifyLoaded (Result result)
{
    // A listener may destroy the document
    const WeakReference<YuiDocument> self (this);

    listeners.call ([&] (Listener& listener)
    {
        listener.documentLoaded (*this, result);
    });

    if (self != nullptr)
        notifyDocumentLoaded();

    return result;
}

void YuiDocument::report (const std::shared_ptr<const DataTreeSourceMap>& map, const DataTreeDiagnostic& diagnostic)
{
    const auto located = map != nullptr ? map->locate (diagnostic) : diagnostic;

    // Styling reports the same problem again each time a component is restyled
    const auto isKnown = std::any_of (diagnostics.begin(), diagnostics.end(), [&located] (const DataTreeDiagnostic& known)
    {
        return isSameProblem (known, located);
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

Result YuiDocument::buildTree (const DataTree& document, const std::shared_ptr<const DataTreeSourceMap>& map)
{
    const ScopedValueSetter<bool> loading (isLoading, true);
    const auto previousIgnore = builder.isIgnoringTreeChanges();
    builder.setIgnoreTreeChanges (true);
    diagnostics.clear();

    if (const auto schema = createSchema(); schema != nullptr)
    {
        if (! document.isValid() || (document.getType() != Identifier ("yui") && document.getType() != Identifier ("style")))
        {
            report (map, { .message = "Expected a 'yui' document", .element = document });
            builder.setIgnoreTreeChanges (previousIgnore);
            return fail (diagnostics.back().toString());
        }

        if (auto problem = validateGrammarNodes (*schema, document))
        {
            report (map, *problem);
            builder.setIgnoreTreeChanges (previousIgnore);
            return fail (diagnostics.back().toString());
        }
    }

    // An unchanged style keeps the stylesheet, so nothing is restyled from scratch and transitions keep running
    auto sheet = styleSheet;

    if (sheet == nullptr || ! hasSameStyle (tree, document))
    {
        DataTreeDiagnostic failure;
        auto parsed = StyleSheet::fromTree (document, &failure);

        if (parsed.failed())
        {
            report (map, failure);
            builder.setIgnoreTreeChanges (previousIgnore);
            return fail (diagnostics.back().toString());
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
        // Located before the rollback, which detaches the elements the patch added
        for (const auto& diagnostic : builder.getDiagnostics())
            report (map, diagnostic);

        // The error that made the build fail is the last one reported
        const auto failure = (map != nullptr ? map->locate (builder.getDiagnostics().back()) : builder.getDiagnostics().back()).toString();

        // Restoring the same elements lets the next load keep the components still shown
        if (isPatched)
            rollback.undo();
        else
            tree = previousTree;

        builder.setIgnoreTreeChanges (previousIgnore);
        return fail (failure);
    }

    builder.setIgnoreTreeChanges (previousIgnore);

    styleSheet = sheet;
    sourceMap = map;
    lastLoadSucceeded = true;

    auto& root = *builder.getRoot();
    root.setStyleSheet (styleSheet);

    for (const auto& diagnostic : builder.getDiagnostics())
        report (map, diagnostic);

    attachStyleContext (root);
    return Result::ok();
}

void YuiDocument::attachStyleContext (Component& root)
{
    // The style problems met later, when a rule starts matching, are reported too
    auto& context = *root.getStyleContext();
    context.setComponentResolver (builder.getComponentResolver());
    context.onPaletteChanged = [this]
    {
        builder.refreshThemePalettes();
        builder.getBindings().refreshPaletteBindings();
    };
    context.onDiagnostic = [this] (const DataTreeDiagnostic& diagnostic)
    {
        if (lastLoadSucceeded)
            report (sourceMap, diagnostic);
    };

    context.restyleNow();
}

void YuiDocument::handleLiveTreeMutation()
{
    const WeakReference<Component> previousRoot (builder.getRoot());
    const ScopedValueSetter<bool> loading (isLoading, true);
    const auto previousIgnore = builder.isIgnoringTreeChanges();
    builder.setIgnoreTreeChanges (true);
    diagnostics.clear();

    if (const auto schema = createSchema(); schema != nullptr)
    {
        if (auto problem = validateGrammarNodes (*schema, tree))
        {
            report (sourceMap, *problem);
            builder.setIgnoreTreeChanges (previousIgnore);
            return;
        }
    }

    DataTreeDiagnostic failure;
    auto parsedSheet = StyleSheet::fromTree (tree, &failure);
    if (parsedSheet.failed())
    {
        report (sourceMap, failure);
        builder.setIgnoreTreeChanges (previousIgnore);
        return;
    }

    if (builder.build (tree).failed())
    {
        for (const auto& diagnostic : builder.getDiagnostics())
            report (sourceMap, diagnostic);

        builder.setIgnoreTreeChanges (previousIgnore);
        return;
    }

    builder.setIgnoreTreeChanges (previousIgnore);

    styleSheet = parsedSheet.getReference();
    auto& root = *builder.getRoot();
    root.setStyleSheet (styleSheet);

    for (const auto& diagnostic : builder.getDiagnostics())
        report (sourceMap, diagnostic);

    attachStyleContext (root);

    // Whoever shows the root has another one to show
    if (previousRoot.get() != &root)
        notifyLoaded (Result::ok());
}

void YuiDocument::handleRangeChange()
{
    const ScopedValueSetter<bool> loading (isLoading, true);

    // After a load that failed the interface kept still follows its ranges, and the problems shown stay those of the load.
    // Else the builder tells its own again. What the others told since the load still holds, the document being as it
    // was, and nothing has them tell it again: the stylesheet, and the components reporting through the builder
    if (lastLoadSucceeded)
    {
        for (const auto& told : builder.getDiagnostics())
        {
            const auto located = sourceMap != nullptr ? sourceMap->locate (told) : told;

            std::erase_if (diagnostics, [&located] (const DataTreeDiagnostic& known)
            {
                return isSameProblem (known, located);
            });
        }
    }

    // The document is as it was, so its stylesheet is kept: nothing is restyled from scratch and transitions keep running
    const auto built = builder.build (tree);

    if (lastLoadSucceeded)
        for (const auto& diagnostic : builder.getDiagnostics())
            report (sourceMap, diagnostic);

    if (built.wasOk())
        attachStyleContext (*builder.getRoot());
}

void YuiDocument::resetState()
{
    builder.clear();
    tree = {};
    sourceMap.reset();
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

void YuiDocument::setFallbackAction (YuiBuilder::FallbackAction action)
{
    builder.setFallbackAction (std::move (action));
}

void YuiDocument::applyPendingChanges()
{
    builder.applyPendingChanges();
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
