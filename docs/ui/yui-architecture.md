# How YUI fits together

YUI builds a user interface from `.yui` text: components and their properties,
stylesheets that dress them, drawings that paint them and bindings that keep
their values up to date. This page maps the classes involved and how data flows
between them, one layer at a time. The other guides explain how to use each
layer.

## The big picture

```mermaid
flowchart LR
    files[".yui text<br/>and imports"]:::ext
    app["Application<br/>actions and models"]:::ext

    subgraph document [YuiDocument]
        parser[YuiParser] -- YuiTreeDiff --> tree[(DataTree)]
        tree --> sheet[StyleSheet]
        tree --> builder[YuiBuilder]
        builder --> bindings[BindingGraph]
    end

    files --> parser
    loader[YuiLoader] -. "reads, watches" .-> files
    app -- "setAction<br/>setModel" --> builder
    builder -- creates --> root["Component tree"]:::self
    view[YuiView] -- "hosts, sizes" --> root
    bindings -- "writes values" --> root
    sheet -- setStyleSheet --> context[StyleContext]
    context -- styles --> root
    root -- paint --> graphics[Graphics]:::ext

    classDef self fill:#005aff,color:#fff,stroke:#003db3;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

- `YuiParser` turns the text, and the files it imports, into one `DataTree`.
  On a reload, `YuiTreeDiff` patches the live tree with it, so the builder keeps
  the components of the elements still there.
- `StyleSheet` collects the `style`, `palette` and `drawing` blocks of that tree.
- `YuiBuilder` creates the components, applies their properties and routes every
  expression through a `BindingGraph`.
- `Component::setStyleSheet` attaches the sheet to the root, which creates the
  `StyleContext` that styles the whole subtree.
- `YuiDocument` runs these steps in order and owns the results.
- `YuiLoader` feeds a document the text of files or embedded resources, with
  in-memory overrides for unsaved edits, and reloads it when the files change.
  `YuiView` owns a document and its loader, and hosts the root as a child
  filling its bounds.

Only the parser, the builder and the document are specific to `.yui`. A
hand-written C++ tree can take a stylesheet, drawings and a `BindingGraph` too.

## Properties, the shared language

Every layer talks to components through named, typed properties. A component's
`ComponentTypeInfo` describes them with `ComponentProperty` descriptors, together
with its `ComponentSignal`s and a factory.

```mermaid
classDiagram
    direction LR
    class Component {
        getTypeInfo()
        setTypeInfo()
        setProperty()
        findProperty()
    }
    class ComponentTypeInfo {
        typeName
        findProperty()
        isSubtypeOf()
        createInstance()
    }
    class ComponentProperty {
        name
        type
        defaultValue
        getter and setter
        isInherited
        affectsParentLayout
    }
    class ComponentSignal {
        name
        connect()
    }
    class ComponentTypeRegistry {
        registerType()
        findType()
        createInstance()
    }
    Component --> ComponentTypeInfo : getTypeInfo
    ComponentTypeInfo --> ComponentTypeInfo : base
    ComponentTypeInfo *-- ComponentProperty
    ComponentTypeInfo *-- ComponentSignal
    ComponentTypeRegistry o-- ComponentTypeInfo : by .yui name
```

- Type infos chain to their base, so a `Row` has every property of `FlexBox`,
  `Layout` and `Component`. Type selectors and the builder walk the same chain.
- `ComponentTypeRegistry` maps the `.yui` type names to type infos and creates
  instances from them.
- A component returns its class type info, unless `setTypeInfo` gave it one of
  its own. Composite components and the document root (for tokens) use this to
  add properties at runtime.
- A descriptor with a getter is *state-backed*: the value lives in the widget
  (a `Slider` value, a `Label` text). Every other property is *store-backed*: the
  value lives in the component's property store.

### Layout containers

Layout containers are components too. Their own properties (`gap`, `padding`,
`direction`, `columns`) and their children's item properties (`flexGrow`,
`margin`, `gridArea`) feed a layout algorithm.

```mermaid
classDiagram
    direction LR
    Component <|-- LayoutComponent
    LayoutComponent <|-- FlexBoxComponent
    FlexBoxComponent <|-- RowComponent
    FlexBoxComponent <|-- ColumnComponent
    LayoutComponent <|-- GridComponent
    LayoutComponent <|-- StackComponent
    FlexBoxComponent ..> FlexBox : runs
    GridComponent ..> Grid : runs
    StackComponent ..> StackLayout : runs
```

Item properties are not inherited and are flagged `affectsParentLayout`. When
one changes on a child, its container gets `childLayoutChanged` and lays out
again.

## Where a property value comes from

`Component::findProperty` is the single read path. Painting, drawings, bindings
and layouts all read through it.

```mermaid
flowchart TD
    ask(["findProperty (name)"]) --> backed{"State-backed?"}
    backed -- yes --> getter(["The widget's getter"])
    backed -- no --> explicit{"Explicit value<br/>on this component?"}
    explicit -- yes --> found([Value])
    explicit -- no --> animated{"Animated value<br/>on this component?"}
    animated -- yes --> found
    animated -- no --> computed{"Stylesheet value<br/>on this component?"}
    computed -- yes --> found
    computed -- no --> inherited{"Inherited property<br/>with a parent?"}
    inherited -- "yes, ask the parent" --> explicit
    inherited -- no --> theme{"Palette color or<br/>theme property?"}
    theme -- yes --> found
    theme -- no --> fallback(["Descriptor default"])
```

Each layer has one writer:

| Layer | Written by |
| --- | --- |
| Explicit | `setProperty` from C++, `.yui` values and bindings |
| Animated | `StyleContext`, from the transitions and animations its `StyleAnimator` runs |
| Stylesheet | `StyleContext`, into the component's `StyleState` |
| Theme | `ApplicationTheme` and the palette in effect for the component |
| State-backed | The widget itself. A stylesheet sets it through the setter, animated values included, and restores the original value when the rule stops matching. |

## Styling

```mermaid
classDiagram
    direction LR
    class StyleSheet {
        fromTree()
        parse()
        getRules()
        findPalette()
        findDrawing()
        findKeyframes()
    }
    class Rule {
        selector
        declarations
    }
    class StyleSelector {
        parse()
        matches()
    }
    class StyleContext {
        refresh()
        propertyChanged()
    }
    class StyleAnimator {
        parseTransitions()
        parseAnimations()
        advance()
    }
    class StyleState {
        computed values
        animated values
        style palette
        shadows
    }
    class ComponentEffectRegistry {
        registerEffect()
        createEffect()
    }
    StyleSheet *-- Rule
    Rule *-- StyleSelector
    StyleSheet *-- ThemePalette : palettes
    Component *-- StyleContext : setStyleSheet
    StyleContext --> StyleSheet
    StyleContext *-- StyleAnimator
    Component *-- StyleState
    StyleContext ..> StyleState : writes
    StyleContext ..> DrawingStyle : installs
    StyleContext ..> ComponentEffectRegistry : effect
    ComponentStyle <|-- DrawingStyle
    Component o-- ComponentStyle : setStyle
```

- A `StyleSheet` is immutable and shared. It holds rules (a `StyleSelector` and
  its declarations), `@keyframes`, palettes and drawings.
- The component the sheet is attached to owns the `StyleContext`. The context
  marks components dirty when their classes, interaction state, children or a
  property a selector watches change.
- Restyling a component writes its `StyleState`: the computed values and the
  palette chosen with `palette:`. A `drawing:` declaration installs a
  `DrawingStyle` with `setStyle`.
- Declaration values that are expressions are compiled with
  `PropertyExpression`, like bindings.
- `transition:` and `animation:` declarations are run by the context's
  `StyleAnimator`. While something runs, every frame writes the values to show
  into the animated layer of the `StyleState`.
- `shadow:` layers are kept in the `StyleState` and painted by the component
  before its content. `effect:` creates a `ComponentEffect` through the
  `ComponentEffectRegistry`, `cursor:` sets the mouse cursor.

### One frame

Styling is batched once per frame, before painting.

```mermaid
sequenceDiagram
    participant Window as Native window
    participant Root as Root component
    participant Context as StyleContext
    participant Knob as Styled component
    participant Drawing as DrawingStyle
    Window->>Root: internalRefreshDisplay
    Root->>Context: refresh
    Note right of Context: Palette switched?<br/>Mark palette-dependent components dirty
    Context->>Knob: restyle the dirty components, top-down
    Note right of Knob: StyleState values, drawing,<br/>transitions to start,<br/>state-backed setters, repaint
    Context->>Knob: advance transitions and animations
    Note right of Knob: Animated values,<br/>repaint while running
    Window->>Knob: paint
    Knob->>Drawing: paint, through findComponentStyle
    Drawing->>Knob: findProperty for each input
    Note left of Drawing: Evaluate the shared program<br/>in this component's slots, render
```

## Drawings

A `DrawingStyle` is a `ComponentStyle` that runs a `DrawingProgram`. Components
painted by the same drawing share one immutable compilation and keep their own
evaluation state, so ten knobs compile the drawing once.

```mermaid
flowchart LR
    source[("drawing knob")]:::ext

    subgraph knobA [Knob A]
        styleA[DrawingStyle] --> programA["DrawingProgram<br/>own slots and properties"]
    end

    subgraph knobB [Knob B]
        styleB[DrawingStyle] --> programB["DrawingProgram<br/>own slots and properties"]
    end

    source --> styleA
    source --> styleB
    programA --> compiled["Compiled<br/>immutable, shared"]:::self
    programB --> compiled
    cache[CompiledCache] -. "weak, keyed by content<br/>and symbol types" .-> compiled

    classDef self fill:#005aff,color:#fff,stroke:#003db3;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

A `DrawingScope` resolves the names an expression reads. Drawings and property
expressions share the same rules:

| Name | Read from |
| --- | --- |
| `palette.primary` | The palette in effect for the component |
| `self.value`, `parent.width` | The component or its parent |
| `gain.value` | The element with id `gain`, found by the builder |
| `value` | The component's own property, else the closest ancestor declaring it as inherited (a token) |
| `bezelSize` | A `property` declared by the drawing, read with `findProperty` at each paint |

## Bindings

```mermaid
classDiagram
    direction LR
    class BindingGraph {
        bind()
        link()
        unbind()
        setComponentResolver()
    }
    class PropertyBinding {
        target
        property
        dependencies
    }
    class PropertyExpression {
        compile()
        evaluate()
        getDependencies()
    }
    BindingGraph *-- PropertyBinding
    BindingGraph ..> Component : listens to
    PropertyBinding *-- PropertyExpression
    PropertyExpression *-- DrawingScope
    PropertyBinding --> Component : writes
```

Each `PropertyBinding` keeps one property up to date from an expression and
remembers the properties it read. The graph listens to every component involved
and re-evaluates only the bindings that read what changed:

```mermaid
sequenceDiagram
    actor User
    participant Gain as gain (Slider)
    participant Graph as BindingGraph
    participant Binding as label.text binding
    participant Label as label (Label)
    User->>Gain: drag
    Gain->>Graph: componentPropertyChanged (value)
    Graph->>Binding: mark dirty and flush
    Binding->>Gain: findProperty (value)
    Binding->>Label: setProperty (text)
    Label->>Graph: componentPropertyChanged (text)
    Note over Graph,Label: Written by its own binding, so it stays.<br/>A write from anywhere else drops the binding.
```

The same change notification reaches the `StyleContext`, which restyles when a
selector watches that property. A two-way link made with `link` copies changes
in both directions and survives writes.

## Documents

The builder turns the declarations of a document into the pieces above.

```mermaid
flowchart LR
    subgraph appSide [Application]
        provider["BindingProvider<br/>DataTreeBindingProvider<br/>AudioParameterBindingProvider"]:::ext
        callback["Action callback"]:::ext
    end

    subgraph builderSide [YuiBuilder]
        tokens["Root type info<br/>tokens as inherited properties"]
        composite["Type info per<br/>component block"]
        model["Hidden model component<br/>state-backed properties"]
        handler["Signal handler<br/>action (...) or root.sig (...)"]
    end

    subgraph treeSide [Component tree]
        root["Document root"]:::self
        instance["Composite instance<br/>for example a ColumnComponent"]:::self
        linked["Linked component"]:::self
    end

    provider -- setModel --> model
    callback -- setAction --> handler
    tokens -- setTypeInfo --> root
    composite -- setTypeInfo --> instance
    model <-->|two-way link| linked
    linked -- "ComponentSignal<br/>onValueChanged" --> handler

    classDef self fill:#005aff,color:#fff,stroke:#003db3;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

- A `component Name : Base` block becomes a type info derived from the base. An
  instance is a plain component of the base class carrying that type info, so
  selectors, drawings and bindings see its extra properties like built-in ones.
- Tokens become inherited properties of the document root, so any expression
  below it can read them by name.
- A model wraps a `BindingProvider` in a hidden component whose state-backed
  properties forward to the provider. `model.cutoff` then reads like any other
  property, and `<=>` reuses the graph's links. Drags on linked components open
  and close the provider's gestures, and any other change is a gesture of its own.
- A property named after a signal connects a handler. When the signal fires, the
  handler runs a document action or emits a signal declared by a composite.

## See also

- [Complete UIs in `.yui`](yui-documents.md) - the document API, tokens,
  composites, handlers and models.
- [Reactive bindings](yui-reactivity.md) - bindings and links in practice.
- [The `.yui` language](yui-language.md) - the syntax of every block.
- [Stylesheets](yui-styles.md) - selectors, the cascade and palettes in practice.
- [Data-driven drawing](yui-drawing.md) - the drawing language and its scopes.
- [Component styling](component-styling.md) - the property model, palettes and
  `ComponentStyle`.
- [Component Layout](component-layout.md) - the layout algorithms behind the
  containers.
