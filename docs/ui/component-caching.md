# Component Caching

Caching a component's paint output to a GPU texture avoids re-rendering
expensive `paint()` calls every frame. Enable it with `setCachedToTexture(true)`
and manage cache state with `isCachedToTexture()`.

```cpp
#include <yup_gui/yup_gui.h>
```

---

## setCachedToTexture — Cache Own Paint to GPU Texture

When enabled, the component's own `paint()` output is rendered once to an
offscreen `GpuCanvas` and reused across subsequent frames. Children still paint
on top normally, so you can have a heavy cached background with animated
foreground elements without re-rendering the expensive background every frame.

```cpp
// Cache the background so it only repaints when explicitly changed.
background.setCachedToTexture (true);
background.addChildComponent (animatedSpinner); // spinner repaints every frame
```

### Cache lifecycle

The cache is automatically invalidated (and re-rendered on the next frame) when:

| Trigger | Explanation |
|---|---|
| `repaint()` | Any explicit or implicit repaint clears the cache. |
| `setBounds()` | Bounds change → cache dimensions are stale. |
| `setCachedToTexture(true)` | Enabling (or toggling) the flag clears any previous cache. |

```{tip}
If a child calls `repaint()` it does **not** invalidate the parent's cache.
Only the parent's own `repaint()` does. This allows animated children to live
inside a cached background.
```

### Caching in the paint pipeline

When both cache and children are present, the render order is:

1. Draw cached background texture (skips parent `paint()`).
2. Paint children on top (each child runs its normal `paint()`).
3. Call `paintOverChildren()`.

```{note}
Because the cache only captures the component's own `paint()`, setting
`setCachedToTexture(true)` on a component **without** overriding `paint()` is
harmless but has no benefit — the default `Component::paint()` is a no-op.
```

### Querying cache state

Use `isCachedToTexture()` to check whether caching is currently enabled on a
component at runtime:

```cpp
if (expensiveBackground.isCachedToTexture())
    Logger::writeToLog ("Background is currently cached");
```

The getter reflects the value last passed to `setCachedToTexture()`. It returns
`true` even if the cache was invalidated — the next frame will lazily rebuild it.

### Interaction with component effects

When both `setCachedToTexture(true)` and `setComponentEffect(...)` are active on
the same component:

- The **effect takes priority** over the standard cache path.
- The full subtree (self + children + `paintOverChildren`) is rendered offscreen,
  the effect is applied, and the **already-effected result** is stored in the
  cache.
- On subsequent frames the cached texture is drawn directly, skipping both
  `paint()` and `ComponentEffect::apply()`.

If you want a cached background with an effect applied **only to the background**
(not the children), set `setCachedToTexture(true)` on the background component
and place animated children inside it. The children repaint every frame on top
of the cached (un-effected) background.

---

## Common patterns

### Static background with animated overlay

```cpp
class AnimatedPanel : public Component
{
public:
    AnimatedPanel()
    {
        background.setCachedToTexture (true);
        background.setComponentEffect (myRoundedCornersEffect);
        addAndMakeVisible (background);
        addAndMakeVisible (spinner);
    }

    void paint (Graphics& g) override { /* draw checkerboard */ }

    void resized() override
    {
        background.setBounds (getLocalBounds());
        spinner.centerInParent();
    }

private:
    Component background;
    SpinnerComponent spinner;
};
```

### Toggling cache for low-memory mode

Setting `setCachedToTexture(false)` drops the current GPU texture (frees video
memory) and returns the component to normal render-every-frame behaviour. To
temporarily stop caching, call `false`; to re-enable it later, call `true`
again.

```cpp
if (lowMemoryMode)
    complexPanel.setCachedToTexture (false); // drop GPU texture
else
    complexPanel.setCachedToTexture (true);  // re-cache on next frame
```

---

## Related

- [Component snapshots](component-snapshots.md) — `snapshotToImage` and
  `snapshotToTexture`
- [Component effects (shaders)](component-effects.md)
- [Component basics](component-basics.md)
