# Convolution & delay

Two specialized processors for impulse-response convolution and fractional
delay, both realtime-safe once configured.

## PartitionedConvolver

`PartitionedConvolver` is a layered partitioned convolution engine built for
real-time use — ideal for reverb impulse responses and cabinet simulation.

### Layered strategy

A **direct FIR** handles the early IR coefficients (low latency), followed by
one or more **FFT overlap-add layers** with *uniform partitioning per layer*
but *non-uniform hop sizes across layers* (each layer has one fixed hop size
`L`; its FFT size is `2·L`). Geometrically increasing hops — e.g.
`256 → 1024 → 4096` — spread the work of a long IR across the audio thread
evenly.

### Configuration

```cpp
yup::PartitionedConvolver conv;
conv.setTypicalLayout (256, { 256, 1024, 4096 });   // 256 direct taps + 3 FFT layers
conv.prepare (512);                                  // max block size
conv.setImpulseResponse (irData);                    // NOT realtime-safe
```

- `configureLayers (directFIRCoefficients, layers)` — explicit layer specs
  (`LayerSpec { hopSize }`).
- `setTypicalLayout (directCoefficients, hops)` — convenience: hops `< 64`
  are folded into the direct FIR, the rest become FFT layers with the hop
  rounded up to the next power of two.
- `setImpulseResponse (ir, options)` — loads the IR, optionally normalizing
  the peak to 0 dBFS, applying a headroom scale (default `−12 dB`), and
  trimming trailing silence (`IRLoadOptions`). Partitions are distributed
  across the layers in order; unused layers get empty IRs. **Not
  realtime-safe** — call at init or while paused (it swaps under a spin
  lock so `process` can continue running).
- `getImpulseLength()` — IR length in samples after silence trimming.

### Processing

```cpp
// audio thread:
FloatVectorOperations::clear (out, n);
conv.process (in, out, n);   // accumulates into out
```

`process` accepts blocks up to the `maxBlockSize` given to `prepare`, performs
no heap allocation, and is guarded by a spin lock so an IR swap can happen
concurrently. Like `DirectFIR`, it **accumulates** into the output buffer —
clear it first for overwrite behavior. `reset()` clears delay lines and
overlap state but preserves the loaded IR partitions.

## FractionallyAddressedDelay

`FractionallyAddressedDelay<SampleType, CoeffType>` implements Davide
Rocchesso's 1999 *fractionally-addressed delay* (FAD) line — a **single**
fractional read/write pointer instead of the classic two-pointer FIR line.
The benefits:

- lower mean attenuation at high frequencies (≈ 0.5 dB at 2/3 Nyquist vs
  ≈ 1.2 dB for a classic line), and
- a *tension model*: changing the delay length changes the propagation speed,
  so pitch follows `exp(−k)` — the natural waveguide behavior that avoids
  Doppler artifacts. This makes it well suited to chorus, flanger, and
  waveguide pitch-bending.

```cpp
yup::FractionallyAddressedDelay<float> fad;
fad.setMaxDelaySamples (4096);        // allocates; call before processing
fad.setDelaySamples (2048.0f);        // may be fractional

float out = fad.processSample (in);
```

- `setMaxDelaySamples` rounds the buffer up to the next power of two (min 2)
  for cheap masking.
- `setDelaySamples` accepts fractional delays and is clamped to ≥ 1 sample;
  internally the increment is `bufferSize / delaySamples`.
- Reading is linear interpolation between two cells; writing is gap-filling
  interpolation between previous and current input.
- When the delay exceeds the buffer size (`increment < 1`) no write happens —
  the pointer re-reads existing content, giving "delays longer than the
  buffer" for free.
- `prepare` is a no-op (no sample-rate-dependent state); `processBlock`
  requires output to not alias input; `processInPlace` is provided.

## Related

- [Filters](filters.md) - `DirectFIR` covers the short-FIR case and is used
  internally as the first layer of the convolver.
- [Resampling](resampling.md) - the same circular-buffer / sinc machinery the
  FAD and convolver rely on.
