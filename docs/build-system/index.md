# Build System

YUP is CMake-first. Use it as a standalone repository or bring it into your own
application or plugin project with `FetchContent`.

## In this area

- [CMake API](cmake-api.md) - the functions YUP exposes for declaring apps,
  plugins, and modules.
- [Module format](module-format.md) - the `BEGIN_YUP_MODULE_DECLARATION` block
  and directory conventions every `yup_*` module follows.
- [Building standalone applications](building-standalone.md) - create a native
  or web application target.
- [Building audio plugins](building-plugins.md) - build and package CLAP / VST3
  plugins.

```{toctree}
:hidden:
:maxdepth: 1

cmake-api
module-format
building-standalone
building-plugins
```
