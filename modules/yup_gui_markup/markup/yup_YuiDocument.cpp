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

// A tokens or component block gives a stylesheet the properties it declares, and nothing else of what it holds
bool declaresPropertiesOnly (const DataTree& element)
{
    const auto type = element.getType().toString();
    return type == "tokens" || type == "component";
}

bool isPropertyDeclaration (const DataTree& element)
{
    return element.getType().toString() == "property";
}

/** True when a stylesheet reads the same from two elements: all there is in a style, rule, palette, drawing, shader or
    keyframes block, and the properties a tokens or component block declares, by their name and their type.
*/
bool readsSameStyle (const DataTree& a, const DataTree& b)
{
    if (a.getType() != b.getType() || ! declaresPropertiesOnly (a))
        return a.isEquivalentTo (b);

    const auto findDeclarations = [] (const DataTree& block)
    {
        std::vector<DataTree> declarations;

        for (const auto& child : block)
            if (isPropertyDeclaration (child))
                declarations.push_back (child);

        return declarations;
    };

    return std::ranges::equal (findDeclarations (a), findDeclarations (b), [] (const DataTree& x, const DataTree& y)
    {
        return x.getProperty ("name") == y.getProperty ("name") && x.getProperty ("type") == y.getProperty ("type");
    });
}

/** True when both documents hold, at the same positions, style elements a stylesheet reads the same from, so a
    stylesheet read from one describes the other too, down to where each declaration is written.
*/
bool hasSameStyle (const DataTree& a, const DataTree& b)
{
    return std::ranges::equal (findStyleElements (a), findStyleElements (b), [] (const auto& x, const auto& y)
    {
        return x.first == y.first && readsSameStyle (x.second, y.second);
    });
}

/** Copies what a stylesheet reads of a document, each element where the document has it, so that what the stylesheet
    tells about one is located as in the document: the style elements, the properties the tokens and component blocks
    declare, and an empty element in place of each of the others.
*/
DataTree copyStyle (const DataTree& document)
{
    DataTree copy (document.getType());
    auto transaction = copy.beginTransaction();

    for (const auto& element : document)
    {
        DataTree copied (element.getType());

        if (StyleSheet::isStyleElement (element))
        {
            if (! declaresPropertiesOnly (element))
            {
                copied = element.clone();
            }
            else
            {
                auto declarations = copied.beginTransaction();

                for (const auto& child : element)
                    declarations.addChild (isPropertyDeclaration (child) ? child.clone() : DataTree (child.getType()));
            }
        }

        transaction.addChild (copied);
    }

    transaction.commit();
    return copy;
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

//==============================================================================
// A property of an element of the grammar: a string, or the position of a keyframe
struct GrammarProperty
{
    const char* name = nullptr;
    bool isRequired = false;
    const char* description = "";
    bool isKeyframePosition = false;
};

// An element of the grammar, which is what a document is written with around its components
struct GrammarNode
{
    static constexpr int maxProperties = 3;

    const char* type = nullptr;
    GrammarProperty properties[maxProperties];
    Span<const char* const> children;
    int maxChildren = -1;
};

template <std::size_t NumFirst, std::size_t NumSecond>
constexpr std::array<const char*, NumFirst + NumSecond> joinGrammarNames (const std::array<const char*, NumFirst>& first, const std::array<const char*, NumSecond>& second)
{
    std::array<const char*, NumFirst + NumSecond> names {};

    for (std::size_t i = 0; i < NumFirst; ++i)
        names[i] = first[i];

    for (std::size_t i = 0; i < NumSecond; ++i)
        names[NumFirst + i] = second[i];

    return names;
}

constexpr std::array documentChildren { "import", "tokens", "palette", "style", "drawing", "shader", "component" };
constexpr std::array styleChildren { "rule", "palette", "drawing", "keyframes", "property" };
constexpr std::array ruleChildren { "declaration", "property" };
constexpr std::array propertyChild { "property" };
constexpr std::array keyframeChild { "keyframe" };
constexpr std::array declarationChild { "declaration" };
constexpr std::array argumentChild { "argument" };

constexpr std::array drawingBodyChildren {
    "let", "repeat", "group", "clip", "layer", "viewport", "use", "drawing", "pattern", "marker",
    "rect", "ellipse", "circle", "line", "arc", "path", "polygon", "star", "vertex", "part", "text", "image", "svg"
};

constexpr std::array clipPathChildren { "let", "repeat", "rect", "ellipse", "circle", "line", "arc", "path", "polygon", "star", "vertex" };
constexpr std::array filterPrimitives { "blur", "offset", "flood", "colorMatrix", "composite", "blend", "morphology", "lighting", "displacement", "dropShadow" };

constexpr auto drawingChildren = joinGrammarNames (propertyChild, drawingBodyChildren);
constexpr auto clipChildren = joinGrammarNames (drawingBodyChildren, std::array { "clipPath" });
constexpr auto layerChildren = joinGrammarNames (clipChildren, std::array { "mask", "filter" });

// A filter also runs the shaders a `use` names
constexpr auto filterChildren = joinGrammarNames (filterPrimitives, std::array { "use" });

/* The grammar, in the order the schema lists it: each element with its properties, the elements it holds and how many of
   them at most. An element that tells neither holds any element, and the document holds the components too.
*/
constexpr GrammarNode grammarNodes[] = {
    { "yui", {}, documentChildren },
    { "import", { { "path", true, "Imported .yui path." } }, {}, 0 },
    { "tokens", {}, propertyChild },
    { "palette", { { "name", true, "Palette name." }, { "colors", false, "Seed color list." }, { "mode", false, "Palette mode (dark or light)." } }, {}, 0 },
    { "style", {}, styleChildren },
    { "rule", { { "selector", true, "CSS-like rule selector." } }, ruleChildren },
    { "keyframes", { { "name", true, "Keyframe sequence name." } }, keyframeChild },
    { "keyframe", { { "at", true, "", true } }, declarationChild },
    { "declaration", { { "name", true }, { "value", true } }, {}, 0 },
    { "property", { { "name", true }, { "type", true }, { "value" } }, {}, 0 },
    { "alias", { { "name", true }, { "target", true } }, {}, 0 },
    { "signal", { { "name", true } }, argumentChild },
    { "argument", { { "name", true }, { "type", true } }, {}, 0 },
    { "component", { { "name", true }, { "base", true } } },
    { "drawing", { { "name" } }, drawingChildren },
    { "shader", { { "name", true, "Shader name." }, { "fragment", false, "Path of the fragment shader, GLSL source or a compiled bundle." }, { "material", false, "Path of a material function for 3D meshes." } }, propertyChild },
    { "let", { { "name", true }, { "value", true } }, {}, 0 },
    { "repeat", { { "name", true }, { "from", true }, { "to", true } }, drawingBodyChildren },
    { "group", {}, drawingBodyChildren },
    { "mask", {}, drawingBodyChildren },
    { "viewport", {}, drawingBodyChildren },
    { "clip", {}, clipChildren },
    { "layer", {}, layerChildren },
    { "clipPath", {}, clipPathChildren },
    { "filter", {}, filterChildren },
    { "blur", {}, {}, 0 },
    { "offset", {}, {}, 0 },
    { "flood", {}, {}, 0 },
    { "colorMatrix", {}, {}, 0 },
    { "composite", {}, {}, 0 },
    { "blend", {}, {}, 0 },
    { "morphology", {}, {}, 0 },
    { "lighting", {}, {}, 0 },
    { "displacement", {}, {}, 0 },
    { "dropShadow", {}, {}, 0 },
    { "pattern", { { "name", true } }, drawingBodyChildren },
    { "marker", { { "name", true } }, drawingBodyChildren },
    { "use", { { "drawing", true, "Name of the drawing to paint, or of the shader to paint or to run in a filter." } }, {}, 0 },
    { "part", { { "name", true, "Name of the part of the component to paint." } }, {}, 0 },
    { "rect", {}, {}, 0 },
    { "ellipse", {}, {}, 0 },
    { "circle", {}, {}, 0 },
    { "line", {}, {}, 0 },
    { "arc", {}, {}, 0 },
    { "path", {}, {}, 0 },
    { "polygon", {}, {}, 0 },
    { "star", {}, {}, 0 },
    { "vertex", {}, {}, 0 },
    { "text", {}, {}, 0 },
    { "image", {}, {}, 0 },
    { "svg", {}, {}, 0 },
    { "bind", { { "property", true }, { "target", true } }, {}, 0 },
    { "changed", { { "property", true }, { "handler", true } }, {}, 0 }
};

// Describes the elements of the grammar, the document holding the components of the types named too
DynamicObject::Ptr makeGrammarNodeTypes (const StringArray& componentTypeNames)
{
    DynamicObject::Ptr nodeTypes (new DynamicObject());

    for (const auto& node : grammarNodes)
    {
        DynamicObject::Ptr properties;

        for (const auto& property : node.properties)
        {
            if (property.name == nullptr)
                break;

            if (properties == nullptr)
                properties = new DynamicObject();

            auto described = makeSchemaProperty (property.isKeyframePosition ? "number" : "string", property.isRequired, property.description);

            if (property.isKeyframePosition)
            {
                described.getDynamicObject()->setProperty ("minimum", 0.0);
                described.getDynamicObject()->setProperty ("maximum", 1.0);
            }

            properties->setProperty (property.name, described);
        }

        StringArray children (node.children.data(), static_cast<int> (node.children.size()));

        if (std::string_view (node.type) == "yui")
            children.addArray (componentTypeNames);

        nodeTypes->setProperty (node.type, makeSchemaNode (properties, children, node.maxChildren));
    }

    return nodeTypes;
}

DataTreeSchema::Ptr makeSchema (const DynamicObject::Ptr& nodeTypes)
{
    DynamicObject::Ptr rootSchema (new DynamicObject());
    rootSchema->setProperty ("nodeTypes", var (nodeTypes.get()));
    return DataTreeSchema::fromJsonSchema (var (rootSchema.get()));
}

// The schema is the one of the grammar alone, which holds no component type: a component is checked by what builds it
std::optional<DataTreeDiagnostic> validateGrammarNodes (const DataTreeSchema& schema, const DataTree& node, bool isAmongComponents = false, int depth = 0)
{
    if (! node.isValid())
        return DataTreeDiagnostic { .message = "Invalid DataTree" };

    // A tree built in code is held to the depth of a text, as what reads it goes down one call for each level too. A
    // text nests its elements that deep inside a `component` block too, and they hold their bindings and handlers there
    if (depth > detail::maxYuiNestingDepth + 2)
        return DataTreeDiagnostic { .message = "Elements are nested more than " + String (detail::maxYuiNestingDepth) + " deep", .element = node };

    const auto nodeType = node.getType();
    static const Identifier repeatType ("repeat");

    // A repeat among components holds components, where the one of a drawing holds what a drawing does
    const auto repeatsComponents = isAmongComponents && nodeType == repeatType;

    if (schema.hasNodeType (nodeType))
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
        if (auto problem = validateGrammarNodes (schema, node.getChild (i), holdsComponents, depth + 1))
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

    const auto nodeTypes = makeGrammarNodeTypes (componentTypeNames);

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

    return makeSchema (nodeTypes);
}

//==============================================================================

YuiDocument::YuiDocument()
{
    // The root knows its document as soon as it is made, for the components made under it
    builder.onRootCreated = [this] (Component&)
    {
        attachToRoot();
    };

    builder.onDiagnostic = [this] (const DataTreeDiagnostic& diagnostic)
    {
        if (! isLoading && lastLoadSucceeded)
            report (sourceMap, diagnostic);
    };

    builder.onTreeMutated = [this]
    {
        if (isLoading || ! tree.isValid())
            return;

        // After a load that failed the interface kept does not follow its tree, only the ranges that changed with it
        if (lastLoadSucceeded)
            notifyLoaded (handleLiveTreeMutation());
        else
            builder.applyPendingChanges();
    };

    builder.onRangeChanged = [this]
    {
        if (! isLoading && tree.isValid())
            notifyLoaded (handleRangeChange());
    };
}

YuiDocument::~YuiDocument()
{
    // First: the components destroyed below may look for their document
    detachFromRoot();

    resetState();
}

YuiDocument* YuiDocument::findDocumentFor (const Component* component)
{
    return dynamic_cast<YuiDocument*> (findFor (component));
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
    return load (YuiParser::parse (text, resolveImport));
}

Result YuiDocument::load (const YuiParser::ParseResult& parsed)
{
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
    // Whatever the load left as the root, or none, is the one the document is found from
    attachToRoot();

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

const DataTreeSchema* YuiDocument::getGrammarSchema()
{
    // The grammar is the same whatever the document and the types registered: its schema is made once, for the first
    // tree to check
    if (grammarSchema == nullptr)
        grammarSchema = makeSchema (makeGrammarNodeTypes ({}));

    return grammarSchema.get();
}

std::optional<YuiDocument::Style> YuiDocument::readStyle (const DataTree& document, const std::shared_ptr<const DataTreeSourceMap>& map, bool canKeep)
{
    // An unchanged style keeps the stylesheet, so nothing is restyled from scratch and transitions keep running
    if (canKeep && styleSheet != nullptr && hasSameStyle (styleSource, document))
        return Style { styleSheet, styleSource };

    // The stylesheet is made from a copy that nothing changes: it keeps the elements it reads, and the tree of the
    // document is changed in place. The copy also tells, later, whether a document still reads the same
    Style style { {}, copyStyle (document) };

    DataTreeDiagnostic failure;
    auto parsed = StyleSheet::fromTree (style.source, &failure);

    if (parsed.failed())
    {
        report (map, failure);
        return std::nullopt;
    }

    style.sheet = parsed.getReference();
    return style;
}

Result YuiDocument::buildTree (const DataTree& document, const std::shared_ptr<const DataTreeSourceMap>& map)
{
    const ScopedValueSetter<bool> loading (isLoading, true);
    const auto previousIgnore = builder.isIgnoringTreeChanges();
    builder.setIgnoreTreeChanges (true);
    diagnostics.clear();

    if (const auto* schema = getGrammarSchema(); schema != nullptr)
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

    const auto style = readStyle (document, map, true);

    if (! style.has_value())
    {
        builder.setIgnoreTreeChanges (previousIgnore);
        return fail (diagnostics.back().toString());
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

    styleSheet = style->sheet;
    styleSource = style->source;
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

Result YuiDocument::handleLiveTreeMutation()
{
    const ScopedValueSetter<bool> loading (isLoading, true);
    const auto previousIgnore = builder.isIgnoringTreeChanges();
    builder.setIgnoreTreeChanges (true);
    diagnostics.clear();

    // The map places an element by its index among its siblings: with elements added, removed or moved it would tell the
    // line of a neighbor, so the problems have no position until a text is loaded
    if (builder.hasTreeStructureChanged())
        sourceMap.reset();

    if (const auto* schema = getGrammarSchema(); schema != nullptr)
    {
        if (auto problem = validateGrammarNodes (*schema, tree))
        {
            report (sourceMap, *problem);
            builder.setIgnoreTreeChanges (previousIgnore);
            return Result::fail (diagnostics.back().toString());
        }
    }

    // The problems told so far are forgotten, and a component with some to tell again knows it by the stylesheet or the
    // source map being another one (see getSourceMap). A stylesheet that still reads the same is therefore kept when
    // there is a map to give another one of, and made again when there is none
    const auto style = readStyle (tree, sourceMap, sourceMap != nullptr);

    if (! style.has_value())
    {
        builder.setIgnoreTreeChanges (previousIgnore);
        return Result::fail (diagnostics.back().toString());
    }

    if (builder.build (tree).failed())
    {
        for (const auto& diagnostic : builder.getDiagnostics())
            report (sourceMap, diagnostic);

        builder.setIgnoreTreeChanges (previousIgnore);
        return Result::fail (diagnostics.back().toString());
    }

    builder.setIgnoreTreeChanges (previousIgnore);

    if (style->sheet == styleSheet && sourceMap != nullptr)
        sourceMap = std::make_shared<const DataTreeSourceMap> (*sourceMap);

    styleSheet = style->sheet;
    styleSource = style->source;
    auto& root = *builder.getRoot();
    root.setStyleSheet (styleSheet);

    for (const auto& diagnostic : builder.getDiagnostics())
        report (sourceMap, diagnostic);

    attachStyleContext (root);
    return Result::ok();
}

Result YuiDocument::handleRangeChange()
{
    const ScopedValueSetter<bool> loading (isLoading, true);

    // The changes made to the tree after a load that failed are only built here, see handleLiveTreeMutation
    if (builder.hasTreeStructureChanged())
        sourceMap.reset();

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

    // The problems shown after a load that failed stay those of the load, so its failure is the one told again
    if (! lastLoadSucceeded && ! diagnostics.empty())
        return Result::fail (diagnostics.back().toString());

    return built;
}

void YuiDocument::resetState()
{
    builder.clear();
    tree = {};
    sourceMap.reset();
    styleSheet = nullptr;
    styleSource = {};
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
