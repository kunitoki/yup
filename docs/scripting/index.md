# Scripting

Python bindings that expose YUP to scripts and tooling.

**Modules covered:** `yup_python`.

```{warning}
**Work in progress.** This area is still being written. Setup instructions and
API-surface documentation for the Python bindings are still to come.
```

## Topics

- **Bindings** - the pybind11-based bridge to YUP core and graphics types.
- **Embedding** - driving YUP from a Python host.
- [**GPU rendering from Python**](python-rhi.md) - what differs between the
  `yup_rhi` C++ API and its Python bindings.

```{toctree}
:hidden:
:maxdepth: 2

python-rhi
```
