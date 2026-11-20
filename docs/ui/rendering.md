# Rendering and performance

What happens between `paint()` and the screen, and how to make it cheaper.
Read [component basics](component-basics.md) first.

- [Component paint profiling](component-profiling.md) - measure and reduce the
  cost of `Component::paint`.
- [Component caching](component-caching.md) - `setCachedToTexture`, caching a
  subtree's paint output in a GPU texture.
- [Component snapshots](component-snapshots.md) - `snapshotToImage` and
  `snapshotToTexture` for pixel capture.
- [Component effects (shaders)](component-effects.md) - GPU shader effects
  applied to a `Component` subtree.
- [Components in 3D](component-3d.md) - live, interactive components on 3D
  geometry with `setManuallyComposited` and `MeshSurfaceMapper`.

```{toctree}
:hidden:
:maxdepth: 1

Paint Profiling <component-profiling>
Caching <component-caching>
Snapshots <component-snapshots>
Effects (Shaders) <component-effects>
Components in 3D <component-3d>
```
