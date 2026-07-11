# Networking

`yup_core` provides HTTP(S) access, low-level sockets, and address utilities.
Network I/O plugs into the same [stream](files-and-streams.md) abstractions used
for files and memory.

## URL

`URL` models a URL and is the high-level entry point for HTTP(S) requests. Build
requests fluently, then open a stream to read the response.

```cpp
URL url ("https://example.com/api");

// Add query parameters / POST data fluently (each returns a new URL)
URL withParams = url.withParameter ("q", "yup")
                    .withParameter ("limit", "10");

String asText = withParams.toString (/* includeGetParameters */ true);
bool   ok     = url.isWellFormed();

// Open a stream to fetch the resource
if (auto stream = url.createInputStream (URL::InputStreamOptions (URL::ParameterHandling::inAddress)))
{
    String body = stream->readEntireStreamAsString();
}
```

`URL` also constructs from a local `File` (`file:` scheme) and can launch the
default browser.

## WebInputStream

`WebInputStream` is the `InputStream` implementation behind
`URL::createInputStream`. Use it directly when you need fine control over HTTP
headers, method, timeouts, and response status/headers.

```cpp
WebInputStream web (url, /* isPost */ false);
web.withExtraHeaders ("Accept: application/json");

if (web.connect (nullptr))
{
    int status = web.getStatusCode();
    String all = web.readEntireStreamAsString();
}
```

## Sockets

For custom protocols, use the low-level socket types:

- **`StreamingSocket`** — a TCP connection (connect/listen/read/write).
- **`DatagramSocket`** — UDP send/receive, including multicast.

```cpp
StreamingSocket socket;
if (socket.connect ("example.com", 80))
{
    socket.write (request.toRawUTF8(), (int) request.getNumBytesAsUTF8());

    char buffer[1024];
    int  read = socket.read (buffer, sizeof (buffer), /* blockUntilSpecifiedAmountHasArrived */ false);
}
```

## Addresses

- **`IPAddress`** — an IPv4 or IPv6 address with parsing and enumeration.
- **`MACAddress`** — a hardware address, with adapter enumeration.

```cpp
IPAddress local = IPAddress::local();                 // loopback
auto      all   = IPAddress::getAllAddresses();        // all interface addresses
String    text  = local.toString();
```

## NamedPipe

`NamedPipe` provides bidirectional inter-process communication over an OS named
pipe — a lightweight alternative to sockets for local IPC.

```cpp
NamedPipe pipe;
if (pipe.createNewPipe ("my-app-pipe"))
{
    char buffer[256];
    int  read = pipe.read (buffer, sizeof (buffer), /* timeoutMs */ 1000);
}
```

```{seealso}
`InterProcessLock` (in the [Multithreading](../multithreading/index.md) area)
coordinates access across processes; `ChildProcess` launches and communicates
with external programs.
```

## See also

- [Files & streams](files-and-streams.md) — the stream types network I/O uses.
- [Data interchange](data-interchange.md) — parse JSON/XML responses.
