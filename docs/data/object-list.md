# Object Lists

`DataTreeObjectList<ObjectType>` keeps a list of C++ objects in sync with the
child nodes of a parent [`DataTree`](datatree.md). When a suitable child is
added, it creates the matching object; when a child is removed, it destroys the
object; when children are reordered, it reorders the objects. This is the
bridge between a tree-backed data model and live C++ objects (UI components,
processors, and so on).

## Defining the managed object

The managed type must expose its backing tree via `getDataTree()`. Using
[`CachedValue`](cached-value.md) members keeps the object's state in sync with
the tree automatically:

```cpp
class UIComponent
{
public:
    explicit UIComponent (const DataTree& tree)
        : dataTree (tree)
        , name    (tree, "name", "")
        , visible (tree, "visible", true)
        , x       (tree, "x", 0.0f)
        , y       (tree, "y", 0.0f)
    {
    }

    String getName() const    { return name.get(); }
    bool   isVisible() const  { return visible.get(); }
    float  getX() const       { return x.get(); }
    float  getY() const       { return y.get(); }

    void setName (const String& n)      { name.set (n); }
    void setPosition (float nx, float ny) { x.set (nx); y.set (ny); }

    DataTree getDataTree() const { return dataTree; }   // required

private:
    DataTree dataTree;
    CachedValue<String> name;
    CachedValue<bool>   visible;
    CachedValue<float>  x, y;
};
```

## Implementing the list

Subclass `DataTreeObjectList` and implement the factory hooks. Call
`rebuildObjects()` in the constructor and `freeObjects()` in the destructor
(the base destructor asserts that all objects were freed).

```cpp
class UIComponentList : public DataTreeObjectList<UIComponent>
{
public:
    explicit UIComponentList (const DataTree& parent)
        : DataTreeObjectList<UIComponent> (parent)
    {
        rebuildObjects();   // create objects for existing children
    }

    ~UIComponentList() override
    {
        freeObjects();      // required
    }

protected:
    // Which child nodes get a corresponding object?
    bool isSuitableType (const DataTree& tree) const override
    {
        return tree.getType() == "UIComponent" && tree.hasProperty ("name");
    }

    UIComponent* createNewObject (const DataTree& tree) override
    {
        return new UIComponent (tree);
    }

    void deleteObject (UIComponent* obj) override
    {
        delete obj;
    }

    // Optional notifications:
    void newObjectAdded (UIComponent* o) override   { /* register / update UI */ }
    void objectRemoved  (UIComponent* o) override   { /* clean up */ }
    void objectOrderChanged() override              { /* re-sort rendering */ }
};
```

| Hook | Purpose |
| ---- | ------- |
| `isSuitableType (tree)` | Return true for child nodes that should have an object. |
| `createNewObject (tree)` | Construct the object for a matching child. |
| `deleteObject (obj)` | Destroy an object whose child was removed. |
| `newObjectAdded` / `objectRemoved` / `objectOrderChanged` | Optional lifecycle callbacks. |

## Using the list

The list mirrors the tree - you mutate the **tree**, and the objects follow via
their `objects` member:

```cpp
DataTree uiRoot ("UIRoot");
UIComponentList components (uiRoot);

// Add a child → an object is created automatically.
{
    auto tx = uiRoot.beginTransaction();

    DataTree button ("UIComponent");
    {
        auto b = button.beginTransaction();
        b.setProperty ("name", "SubmitButton");
        b.setProperty ("x", 100.0f);
        b.setProperty ("y", 50.0f);
    }
    tx.addChild (button);
}

jassert (components.objects.size() == 1);
UIComponent* obj = components.objects[0];
jassert (obj->getName() == "SubmitButton");

// Modify via the tree → the object's CachedValue reflects it.
{
    auto tx = uiRoot.getChild (0).beginTransaction();
    tx.setProperty ("x", 200.0f);
}
jassert (obj->getX() == 200.0f);

// Remove the child → the object is destroyed automatically.
{
    auto tx = uiRoot.beginTransaction();
    tx.removeChild (0);
}
jassert (components.objects.size() == 0);
```

```{important}
Always call `rebuildObjects()` once after construction and `freeObjects()` in
your subclass destructor. Objects are owned by the list; do not delete them
directly - remove the corresponding child instead.
```

## See also

- [DataTree](datatree.md) · [Transactions](transactions.md)
- [CachedValue](cached-value.md) - keeps each object's state in sync.
