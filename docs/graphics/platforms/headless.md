# Headless

`GpuPlatform::Headless` is a GPU device that draws nothing. It exists so that
code which needs a `GpuDevice` or a `GraphicsContext` can run where there is no
GPU and no window - unit tests, CI machines, console tools - without being
rewritten or guarded.

| | |
| --- | --- |
| `GpuPlatform` | `Headless` |
| Runs on | Everywhere |
| Default | Never chosen by a window |
| Shaders | none |
| Compute | No |

`GpuDevice::isPlatformSupported (GpuPlatform::Headless, {})` is always true,
and creating a device always succeeds:

```cpp
auto device = yup::GpuDevice::create (yup::GpuPlatform::Headless, {});
auto context = yup::GraphicsContext::createContext (yup::GpuPlatform::Headless, {});
```

## What it does

Nothing, deliberately. Offscreen targets are not created, offscreen passes do
not render, and pixel readback fails. Every capability query -
`isComputeAvailable()`, `isFragmentStorageBufferAvailable()`,
`isStorageTextureAvailable()`, `isDynamicUniformOffsetAvailable()`,
`isFrameTimingAvailable()` - returns false.

The point is that the calls are *valid*: a component tree can be built, laid
out and asked to paint, a `GpuFrame` can be begun and submitted, and the code
under test exercises its real paths instead of a mock. What it cannot do is
produce pixels, so anything that asserts on rendered output needs a real
backend.

```{note}
`Headless` is not how you render offscreen on a real GPU. For that, create a
real backend's device and use [offscreen targets and
canvases](../rhi/targets.md) - a `GpuDevice` needs no window. The
`GpuDevice::Options::allowHeadlessRendering` flag, which is a different thing
entirely, lets a *real* backend render without a visible window.
```

## Using it in tests

```cpp
TEST (MyComponentTests, LaysOutWithoutAGpu)
{
    auto context = yup::GraphicsContext::createContext (yup::GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);

    MyComponent component;
    component.setBounds ({ 0, 0, 400, 300 });
    component.resized();

    EXPECT_EQ (component.getChildComponent (0)->getWidth(), 400);
}
```

## Related pages

- [Graphics platforms](index.md) - choosing a backend and the capability table.
- [Offscreen targets and canvases](../rhi/targets.md) - rendering without a
  window on a real GPU.
