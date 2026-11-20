# Build System

YUP is CMake-first. Use it as a standalone repository or bring it into your own
application or plugin project with `FetchContent`. To start a new project with
everything already wired up, create it from the
[YUP template](https://github.com/kunitoki/yup-template) - see
[Getting Started](../getting-started/index.md).

## In this area

- [Building standalone applications](building-standalone.md) - create a native
  or web application target.
- [Building audio plugins](building-plugins.md) - build and package CLAP / VST3
  plugins.
- [CMake API](cmake-api.md) - the functions YUP exposes for declaring apps,
  plugins, and modules.
- [Module format](module-format.md) - the `BEGIN_YUP_MODULE_DECLARATION` block
  and directory conventions every `yup_*` module follows.

```{toctree}
:hidden:
:maxdepth: 1

building-standalone
building-plugins
cmake-api
module-format
```
