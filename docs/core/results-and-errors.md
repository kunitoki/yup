# Results & Error Handling

YUP separates *recoverable failures* from *programming errors*:

- **Recoverable failures** (missing file, bad input, network error) return a
  `Result` or `ResultValue<T>` the caller inspects.
- **Programming errors** (broken invariants) use `jassert` in debug builds and
  are handled gracefully (early return) in release.

## Result

`Result` represents success or a failure carrying a human-readable message. It
has no value payload - use it when an operation either works or doesn't.

```cpp
Result doThing()
{
    if (! preconditionMet)
        return Result::fail ("Precondition not met");

    return Result::ok();
}

auto r = doThing();
if (r.wasOk())
    proceed();
else
    DBG (r.getErrorMessage());
```

| Member | Description |
| ------ | ----------- |
| `Result::ok()` | Construct a success. |
| `Result::fail (message)` | Construct a failure with a message. |
| `wasOk()` | True on success. |
| `failed()` | True on failure. |
| `getErrorMessage()` | The failure message (empty on success). |

`Result` is implicitly convertible to `bool` (true = ok), so it reads naturally
in an `if`.

## ResultValue&lt;T&gt;

`ResultValue<T>` is `Result` plus a value: on success it carries a `T`, on
failure it carries an error message. It is the return type for factory functions
and parsers throughout YUP (e.g. `Image::loadFromData`, `GpuPipeline::compile`,
`ShaderBundle::loadFromData`).

```cpp
ResultValue<int> parsePositive (StringRef text)
{
    const int value = String (text).getIntValue();

    if (value <= 0)
        return makeResultValueFail ("Value must be positive");

    return makeResultValueOk (value);        // or simply: return value;
}

auto result = parsePositive ("42");
if (result.wasOk())
    use (result.getValue());
else
    DBG (result.getErrorMessage());
```

| Member | Description |
| ------ | ----------- |
| `makeResultValueOk (value)` | Build a success carrying `value`. |
| `makeResultValueFail (message)` | Build a failure (works for any `ResultValue<T>`). |
| `wasOk()` | True on success. |
| `getValue()` | The value (only valid when `wasOk()`). Has `&`/`&&` overloads. |
| `getErrorMessage()` | The failure message. |

```{tip}
A plain value is implicitly convertible to a successful `ResultValue<T>`, so you
can `return value;` directly on the happy path and reserve `makeResultValueFail`
for errors.
```

## Assertions

Use `jassert` for conditions that must hold if the program is correct. In debug
builds a violation breaks into the debugger; in release it compiles away, so
always pair it with graceful handling:

```cpp
void setGain (float gain)
{
    jassert (gain >= 0.0f);         // catches misuse in debug

    this->gain = jmax (0.0f, gain); // stays safe in release
}
```

`jassertfalse` marks an unreachable/should-not-happen branch.

## See also

- [Files & streams](files-and-streams.md) - `File` operations return `Result`.
- [Imaging: loading](../imaging/loading.md) - `ResultValue<Image>` in practice.
- [RHI: pipelines](../graphics/rhi/pipelines.md) - `ResultValue<GpuPipeline::Ptr>`.
