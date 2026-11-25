# Hot reloading `.yui` files

`YuiLoader` loads a [`YuiDocument`](documents.md) from files on disk or
from resources embedded in the binary, and reloads it when the files change.
Since every load updates the interface in place (see
[Reloading](documents.md#reloading)), editing a `.yui` file while the
application runs shows the change immediately, keeping the components, their
state and the application's pointers to them. The same goes for the files a
document reads: its shaders, its models and its textures.

## Loading files

```cpp
YuiDocument document;
YuiLoader loader (document);

loader.onLoad = [] (const Result& result)
{
    if (result.failed())
        DBG (result.getErrorMessage());
};

if (loader.loadFile (sourceFolder.getChildFile ("ui.yui")).wasOk())
    addAndMakeVisible (*document.getRoot());

loader.setWatching (true);
```

The paths of `import`s are relative to the folder of the file containing the
`import` statement:

```text
ui/
  ui.yui           import "parts/style.yui";
  parts/
    style.yui      import "palette.yui";
    palette.yui
```

`onLoad` is called after every load with its result, including the reloads
started by watching. A failing load keeps the interface shown, so a typo while
editing is reported and fixed without losing anything. A
[`YuiView`](documents.md#showing-a-document) owns a document and its
loader, and shows the root after every load.

## Watching for changes

`setWatching (true)` watches the folders of the main file and of every file it
imports. When one of them changes on disk and its text differs from the text
loaded, the document is reloaded on the message thread. Changes made close
together, such as saving several files at once, trigger a single reload.

Watching is available on macOS, Windows and Linux, through `Watchdog`. On other
platforms, and for sources that aren't files, it has no effect.

## Shaders, models and textures

A document reads other files than `.yui` texts, by paths relative to the folder
of its main file: the GLSL or the compiled bundle of a
[`shader` block](shaders.md), the glTF model of a `Model3D` and the textures
of a `Material3D` (see [3D scenes](scenes.md#assets-and-hot-reload)). The
loader serves them from where the document comes from, and remembers each one
it was asked for:

```text
ui/
  ui.yui           shader Crt { fragment: "fx/crt.frag"; }      Model3D { source: "models/knob.glb"; }
  fx/
    crt.frag
  models/
    knob.glb
```

- Watching covers them. When one is saved, what read it reads it again, a
  shader is compiled again, a model is shown again, and the document is then
  loaded again: the interface is kept, `onLoad` is called, and the diagnostics
  are those of that load. So a shader saved with an error is listed, and no
  longer is once it is saved fixed.
- `reload()` picks up the files that changed since they were read, also where
  nothing is watched: on mobile, on the web, or with watching off.
- A file that did not exist when it was asked for is remembered too: creating
  it is a change.
- Loading another source, the files of an interface first loaded from embedded
  resources for example, has what is kept of the interface read its files
  again from the new one.
- `Image`, the `image` and `svg` of a drawing and `font ()` read their files by
  themselves: they are neither watched nor served from embedded resources.

From C++, `YuiDocument::readAsset (path)` reads such a file, and
`YuiDocument::Listener::documentAssetChanged` tells that one changed. See
[Reading files through the document](documents.md#reading-files-through-the-document).

## Editing unsaved text

An editor loads its unsaved texts through the loader by overriding the files
they belong to. An override stands for a file, the main one or any import,
until another source is loaded or `clearOverrides()` is called; the other
files are still read from the source:

```cpp
loader.setOverride ("parts/style.yui", styleEditor.getText());
loader.reload();
```

Paths are relative to the folder of the main file, as imports are, so the main
file is its own name. `reloadFromString (text)` overrides the main file and
reloads. Watching ignores the overridden files, so an unsaved text wins over a
save made in another editor, and `getLoadedText (path)` returns the text each
file had in the last load, to show the files again after a reload.

The files a document reads are overridden the same way, when they are text: a
shader, or a `.gltf` model.

```cpp
loader.setOverride ("fx/crt.frag", shaderEditor.getText());
loader.reload();                    // what uses the shader compiles the text
```

## Embedded resources

Shipped applications usually embed their `.yui` files, which also works on
platforms without a filesystem such as WebAssembly and Android. The
`yup_add_yui_resources` CMake function embeds them, with the files they read,
and generates a function finding each one by the path the document writes:

```cmake
yup_add_yui_resources (MyAppUi
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/ui"
    FILES
        ui/ui.yui
        ui/parts/style.yui
        ui/parts/palette.yui
    ASSETS
        ui/models/knob.glb
        ui/textures/wood.png
    SHADERS
        ui/fx/crt.frag
    MATERIAL_SHADERS
        ui/fx/hologram.glsl)

yup_standalone_app (
    # ...
    MODULES
        yup::yup_gui
        MyAppUi)
```

```cpp
#include <MyAppUi.h>

loader.loadResource ("ui.yui", yup::MyAppUi_find);
```

| Argument | Embeds | Found as |
| --- | --- | --- |
| `FILES` | The `.yui` files, the imported ones included. | `ui.yui`, `parts/style.yui` |
| `ASSETS` | Files as they are: models, textures. | `models/knob.glb` |
| `SHADERS` | The fragment of a `shader` block, compiled when the application is built. | `fx/crt.frag.ysl` |
| `MATERIAL_SHADERS` | The material of a `shader` block, compiled for both tiers of the renderer. | `fx/hologram.glsl.raster.ysl`, `fx/hologram.glsl.full.ysl` |

- Nothing is followed: every file a document imports or reads has to be
  listed.
- The GLSL of a shader is not embedded. The document keeps naming the source,
  and finds the compiled bundle first, so the shipped binary compiles nothing
  and needs no shader transpiler.
- A shader that does not compile fails the build.
- Embed a model as a `.glb`: the buffers and the images a `.gltf` keeps in
  other files are read from its folder, which embedded resources do not have.

A common setup reads the files from the source tree while developing, and the
embedded copies in release builds:

```cpp
#if YUP_DEBUG
    loader.loadFile (File (YUI_SOURCE_DIR).getChildFile ("ui.yui"));     // shaders compiled from their GLSL
    loader.setWatching (true);                                           // and again when saved
#else
    loader.loadResource ("ui.yui", yup::MyAppUi_find);                   // the bundles built with the application
#endif
```

where `YUI_SOURCE_DIR` is a compile definition set to the folder of the files.

```{note}
The first branch compiles GLSL when the application runs, so that build needs
the shader transpiler: list the `glslang` and `spirv_cross` modules for it
(`YUP_ENABLE_SHADER_TRANSPILER`). A material compiled from its source also
reads the shaders of `yup_renderer` from the YUP sources the binary was built
from. The second branch needs neither.
```

An override of a shader does nothing on embedded resources, where the compiled
bundle is always found first: live editing is for a document loaded from
files.

Adding `VERIFY` to `yup_add_yui_resources` checks the embedded files while
configuring, loaded as the application will load them: an invalid file, a
shader that does not compile, or a file a document reads that the call does
not list fails the build instead of shipping. See
[Tools for `.yui` files](tooling.md#checking-while-configuring).

The `YuiDrawing` graphics example loads its files this way in a `YuiView`,
watches them, and shows them in editable tabs after every load; a tab edited
overrides its file. Its `YUI 3D` page does the same for scenes, with their
shaders and their models.

## See also

- [Complete UIs in `.yui`](documents.md) - documents and what a reload
  keeps.
- [CMake API](../../build-system/cmake-api.md#yup_add_yui_resources) -
  `yup_add_yui_resources`.
- [Shaders](shaders.md#loading-and-shipping) - how a shader is found, compiled
  and shipped.
- [3D scenes](scenes.md) - the models and the textures a scene reads.
- [Tools for `.yui` files](tooling.md) - checking files before they ship.
