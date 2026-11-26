# Getting Started

New to YUP? Start here.

## Start from the template

The quickest way to start a project is the
[YUP template](https://github.com/kunitoki/yup-template): a working repository
where one source tree builds a standalone application and an audio plugin (CLAP,
VST3, AU, AUv3, LV2, AAX and standalone), with tests, benchmarks, installer
packaging and CI for every platform already set up. Keep both targets, or turn
one off with `-DBUILD_APP=OFF` / `-DBUILD_PLUGIN=OFF`.

1. Click [Use this template](https://github.com/kunitoki/yup-template/generate)
   on GitHub to create your own repository from it, rather than forking it.
2. Clone your repository with its submodules, since YUP comes along as a git
   submodule:

   ```sh
   git clone --recursive https://github.com/<you>/<your-project>.git
   ```

3. Set the project, product and company names at the top of `CMakeLists.txt`.
4. Build it with the [`just`](https://github.com/casey/just) recipes (`just mac`,
   `just win` or `just linux` to generate the project, then `just build`), or
   with CMake directly:

   ```sh
   cmake -G "Ninja Multi-Config" -B build/ninja
   cmake --build build/ninja --config Release --target <PROJECT_NAME>_all
   ```

The template [README](https://github.com/kunitoki/yup-template#readme) covers
testing, packaging and every configuration option.

## Try the examples

The repository ships with working example apps you can build immediately:

- [graphics](https://github.com/kunitoki/yup/tree/main/examples/graphics) - 2D
  vector rendering, SVG, Lottie, DSP visualizers, and the RHI spinning-cube demo.
- [audiograph](https://github.com/kunitoki/yup/tree/main/examples/audiograph) -
  a node-based audio graph editor and plugin host, with a browser of nodes and
  plugins, tabbed subgraph editing, settings, and sandboxed plugin loading.
- [plugin](https://github.com/kunitoki/yup/tree/main/examples/plugin) - a small
  polyphonic instrument plugin (CLAP, VST3, AU, LV2 and standalone) with host
  automation, saved state and a resizable editor with a playable keyboard. For a
  full product see [Retrace](https://github.com/kunitoki/retrace), a spectral FM synthesiser.
- [console](https://github.com/kunitoki/yup/tree/main/examples/console) - a
  headless command-line app.

## Build something

- [Building standalone applications](../build-system/building-standalone.md)
- [Building audio plugins](../build-system/building-plugins.md)
- [CMake API reference](../build-system/cmake-api.md)

## Learn the areas

Once you are building, dive into the area that matches your work:
[Graphics](../graphics/index.md), [UI](../ui/index.md),
[Audio](../audio/index.md), [Data](../data/index.md).

```{note}
A step-by-step "first project" walkthrough will be added here.
```
