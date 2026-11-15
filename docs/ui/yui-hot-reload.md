# Hot reloading `.yui` files

`YuiLoader` loads a [`YuiDocument`](yui-documents.md) from files on disk or
from resources embedded in the binary, and reloads it when the files change.
Since every load updates the interface in place (see
[Reloading](yui-documents.md#reloading)), editing a `.yui` file while the
application runs shows the change immediately, keeping the components, their
state and the application's pointers to them.

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
[`YuiView`](yui-documents.md#showing-a-document) owns a document and its
loader, and shows the root after every load.

## Watching for changes

`setWatching (true)` watches the folders of the main file and of every file it
imports. When one of them changes on disk and its text differs from the text
loaded, the document is reloaded on the message thread. Changes made close
together, such as saving several files at once, trigger a single reload.

Watching is available on macOS, Windows and Linux, through `Watchdog`. On other
platforms, and for sources that aren't files, it has no effect.

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

## Embedded resources

Shipped applications usually embed their `.yui` files, which also works on
platforms without a filesystem such as WebAssembly and Android. The
`yup_add_yui_resources` CMake function embeds them and generates a function
finding each file by the path `import` uses:

```cmake
yup_add_yui_resources (MyAppUi
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/ui"
    FILES
        ui/ui.yui
        ui/parts/style.yui
        ui/parts/palette.yui)

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

A common setup reads the files from the source tree while developing, and the
embedded copies in release builds:

```cpp
#if YUP_DEBUG
    loader.loadFile (File (YUI_SOURCE_DIR).getChildFile ("ui.yui"));
    loader.setWatching (true);
#else
    loader.loadResource ("ui.yui", yup::MyAppUi_find);
#endif
```

where `YUI_SOURCE_DIR` is a compile definition set to the folder of the files.

The `YuiDrawing` graphics example loads its files this way in a `YuiView`,
watches them, and shows them in editable tabs after every load; a tab edited
overrides its file.

## See also

- [Complete UIs in `.yui`](yui-documents.md) - documents and what a reload
  keeps.
- [CMake API](../build-system/cmake-api.md) - `yup_add_yui_resources`.
