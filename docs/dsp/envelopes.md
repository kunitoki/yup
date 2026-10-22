# Envelopes

`yup_dsp` describes multistage modulation shapes with `BreakpointCurve` and plays
them per sample with `CurveEnvelope`. The same curve type drives the `custom` shape
of the [LFO](oscillators.md#lfo), so one editor can draw both.

## BreakpointCurve

`BreakpointCurve<FloatType>` holds up to 32 points. Each point has a time, a level
in `[0, 1]` and the curve of the segment leading into it, in `[-1, 1]`. Everything
is stored inline, so copying a curve never allocates and handing one to the audio
thread by value is safe.

Times are in whatever unit the player wants: seconds for `CurveEnvelope`, one
normalized cycle (0 to 1) for an LFO. Points stay sorted: `addPoint()` inserts in
time order, and `setPoint()` clamps the time between the neighbors.

A segment of curve `c` goes from level `a` to level `b` as

```
y (t) = a + (b - a) * (exp (k c t) - 1) / (exp (k c) - 1),    k = 6
```

so `c = 0` is a straight line, positive values start slow and end fast, and
negative values start fast and end slow. The shape is monotone, hits both endpoints
exactly and has a closed form inverse, `BreakpointCurve::inverseShape()`. For an
editor, `curveForMidpoint()` turns a segment midpoint dragged to height `y` into
the curve `c = (2 / k) ln (1 / y - 1)`.

```cpp
yup::BreakpointCurve<float> curve;
curve.addPoint (0.0f, 0.0f);
curve.addPoint (0.01f, 1.0f);         // attack
curve.addPoint (0.3f, 0.6f, -0.5f);   // decay, fast then slow
curve.addPoint (0.8f, 0.0f, -0.5f);   // release
curve.setSustainIndex (2);

float level = curve.evaluate (0.1f);  // read the curve anywhere
```

A curve can carry a sustain point (`setSustainIndex`) and a loop between two
points (`setLoop`). Inserting and removing points keeps both markers on the same
points; removing a marked point clears its marker.

## CurveEnvelope

`CurveEnvelope<FloatType>` copies a curve and plays it with times in seconds:

- `noteOn()` starts from the first point. In `RetriggerMode::legato` an active
  envelope restarts from its current level instead of jumping to the first point.
- While the note is held the envelope stops at the sustain point. With a loop it
  jumps from the loop end back into the segment after the loop start, starting that
  segment from the current level, so looping never clicks.
- `noteOff()` leaves for the segment after the sustain point, or after the loop
  end when there is no sustain point, again from the current level. Releasing in
  the middle of an attack ramps down from where the attack was.
- A curve with neither marker is a one-shot that ignores `noteOff()`.
- After the last point the envelope is no longer active and holds the last level,
  so it also works as a modulation source that ramps and stays. An amplitude
  envelope simply ends at zero.

`setTimeScale()` multiplies every duration and keeps the progress through the
current segment, so it can be automated. Zero-length segments are crossed
instantly, and a loop shorter than a sample holds instead of spinning.

```cpp
yup::CurveEnvelope<float> envelope;
envelope.prepare (sampleRate);
envelope.setCurve (curve);

envelope.noteOn();
envelope.processBlock (gains, numSamples);
envelope.noteOff();

if (! envelope.isActive())
    ; // the voice can be freed
```

## Real-time contract

Nothing allocates. A curved segment costs one multiply per sample (the
exponential is advanced by recurrence), a straight one a multiply-add, and
`processBlock()` writes straight segments as a vectorizable ramp. Starting a
segment costs two `exp` calls.
