# Offline YDSP bundles

Compile a patch with:

```sh
yup_dsp_compiler patch.ydsp --output patch.ydsb
```

Use `--fast-math` only when changed floating-point evaluation is acceptable.
Failed source compilation does not create an output bundle.

Applications load bundles with `YdspBundle::loadFromFile()`,
`loadFromStream()`, or `loadFromData()`, then call `instantiate()` on the
control thread. The stored source closure supports source fallback when a
matching native artifact is unavailable.

For CMake projects, `yup_add_ydsp_bundle()` runs the host compiler at
configuration time and embeds the result:

```cmake
yup_add_ydsp_bundle (MyPatch
    SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/patch.ydsp
    RESOURCE_NAME MyPatchFile)
```

The source extension remains `.ydsp`; compiled artifacts use `.ydsb`.
