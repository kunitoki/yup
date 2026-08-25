# Files & Streams

`yup_core` abstracts the filesystem behind `File` and models all I/O as
`InputStream` / `OutputStream`, so the same code reads from files, memory, the
network, or compressed archives.

## File

`File` is an immutable value type representing an absolute path. It does not open
anything by itself - it is a handle you query and act on.

```cpp
File file ("/path/to/data.txt");

bool exists   = file.existsAsFile();
bool isDir    = file.isDirectory();
int64 size    = file.getSize();

String name   = file.getFileName();            // "data.txt"
String stem   = file.getFileNameWithoutExtension();
String ext    = file.getFileExtension();       // ".txt"
File   parent = file.getParentDirectory();
File   child  = parent.getChildFile ("other.txt");
```

### Reading & writing whole files

```cpp
String text = file.loadFileAsString();
file.replaceWithText ("new contents");
```

### Creating & deleting

```cpp
Result r = file.create();                 // create an empty file
file.getParentDirectory().createDirectory();
file.deleteFile();
```

Operations that can fail return a [`Result`](results-and-errors.md).

### Special locations

Resolve standard directories portably rather than hardcoding paths:

```cpp
auto appData = File::getSpecialLocation (File::userApplicationDataDirectory);
auto temp    = File::getSpecialLocation (File::tempDirectory);
auto home    = File::getSpecialLocation (File::userHomeDirectory);
```

### Bundled resources

`File::bundleDirectory` and `File::hostBundleDirectory` resolve to the resource root of a
bundle/asset package rather than a regular filesystem directory - useful for reading files
shipped with `yup_add_bundled_resources()` (see the [CMake API](../build-system/cmake-api.md)):

```cpp
auto asset = File::getSpecialLocation (File::bundleDirectory).getChildFile ("data/config.json");
auto stream = asset.createInputStream();   // works on every platform, incl. Android
```

- `bundleDirectory` is the current YUP binary's own bundle: a plugin's own `.vst3` / `.component`
  / `.clap`, an app's own `.app` bundle, or an Android APK's assets - never the host DAW when
  running as a plugin.
- `hostBundleDirectory` is the *host* application's bundle - the DAW hosting a plugin, or the
  same location as `bundleDirectory` when the current binary is the standalone application.
- On Android, files under `bundleDirectory`/`hostBundleDirectory` are read directly out of the
  APK via `AAssetManager` - there is no first-run copy to disk - and the location is read-only:
  `createOutputStream()`, `deleteFile()` and `createDirectory()` all fail cleanly. A directory
  containing only subdirectories (no files of its own) reports `isDirectory() == false`, and
  `findChildFiles()` / `DirectoryIterator` do not enumerate asset paths.

## Directory iteration

Find children with a filter and wildcard, or iterate lazily with the
range-based iterator.

```cpp
// One-shot search
auto pngs = dir.findChildFiles (File::findFiles, /* recursive */ true, "*.png");

// Lazy, range-based iteration
for (auto entry : RangedDirectoryIterator (dir, true, "*.wav", File::findFiles))
    process (entry.getFile());
```

Related helpers: `DirectoryIterator`, `FileFilter` / `WildcardFileFilter`,
`FileSearchPath` (a list of search directories), and `TemporaryFile` (an
atomically-replaced scratch file).

## Streams

All I/O flows through two abstract bases:

- **`InputStream`** - sequential/random reads with typed helpers.
- **`OutputStream`** - sequential writes with typed helpers and `operator<<`.

```cpp
if (auto in = file.createInputStream())    // std::unique_ptr<FileInputStream>
{
    int64 length = in->getTotalLength();
    int   header = in->readInt();           // little-endian
    String all   = in->readEntireStreamAsString();
}

if (auto out = file.createOutputStream())
{
    out->writeInt (42);
    *out << "line of text" << newLine;
}
```

Typed read/write helpers include `readByte`/`writeByte`, `readInt`/`writeInt`
(and big-endian variants), `readInt64`, `readString`, `readFloat`, plus raw
`read (buffer, maxBytes)`.

### Concrete streams

| Stream | Purpose |
| ------ | ------- |
| `FileInputStream` / `FileOutputStream` | Read/write a file on disk. |
| `MemoryInputStream` / `MemoryOutputStream` | Read/write an in-memory buffer. |
| `BufferedInputStream` | Buffer another stream for many small reads. |
| `SubregionStream` | Expose a slice of another stream. |

```cpp
MemoryOutputStream mos;
mos.writeInt (1);
mos << "text";
MemoryBlock block = mos.getMemoryBlock();

MemoryInputStream mis (block, /* keepCopy */ false);
```

### Input sources

`InputSource` abstracts "something you can open a stream from" - a `File`, a
`URL`, or a platform document - so consumers stay decoupled from the origin
(`FileInputSource`, `URLInputSource`).

## Archives & compression

- **`ZipFile`** - read entries from a ZIP archive and open a stream per entry.
- **`GZIPCompressorOutputStream`** / **`GZIPDecompressorInputStream`** - wrap
  another stream to (de)compress on the fly.

```cpp
ZipFile zip (zipFile);
for (int i = 0; i < zip.getNumEntries(); ++i)
    if (auto entryStream = zip.createStreamForEntry (i))
        read (*entryStream);
```

## See also

- [Memory](memory.md) - `MemoryBlock` backs the memory streams.
- [Networking](networking.md) - `URL` and `WebInputStream` extend streams over
  the network.
- [Data interchange](data-interchange.md) - parse JSON/XML from streams.
- [Results & error handling](results-and-errors.md) - `Result` return values.
