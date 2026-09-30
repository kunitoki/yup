# File chooser

`FileChooser` shows the platform file dialog to open files, open a directory or
pick a file to save. It is asynchronous: the callback runs on the message
thread once the user confirms or cancels.

```cpp
fileChooser = FileChooser::create ("Load an image", File(), "*.png;*.jpg");

fileChooser->browseForFileToOpen ([this] (bool success, const Array<File>& results)
{
    if (success)
        loadImage (results.getFirst());
});
```

Patterns are separated by `;` or `,`; an empty string allows every file.

| Method | Picks |
| ------ | ----- |
| `browseForFileToOpen` | one file |
| `browseForMultipleFilesToOpen` | one or more files |
| `browseForDirectory` | a directory |
| `browseForMultipleFilesOrDirectoriesToOpen` | files or directories (a directory on the web) |
| `browseForFileToSave` | the location of a file to write |

## Web

On the web the browser file picker is used, and the chosen files are copied into
the in-memory filesystem under the temporary directory (a new
`yup_file_chooser/pick` folder for every pick). When the callback runs, every
returned `File` is fully written and can be read with the usual `File` and
stream APIs. A directory pick returns the copied directory, with its whole
tree inside.

Limits:

- The picker must open close to a user gesture (a click or key press). Showing
  a chooser from a mouse or key callback works in Chrome and Firefox; Safari may
  refuse to open it.
- If the browser refuses to open the picker, or doesn't send the `cancel` event
  when it is dismissed (older Safari), the callback is never called.
- The copies live in memory until they are deleted. Delete them once loaded if
  they are large.
- Saving asks for a file name and returns a path in the temporary directory;
  the file is not downloaded.
