# Filter design

Filter design in `yup_dsp` is separated from filter *processing*: designers
produce coefficient containers, and the [filter classes](filters.md) consume
them. You can design coefficients anywhere (even off the audio thread) and
hand them to a realtime-safe processor.

Two designers are provided:

- **`FilterDesigner<CoeffType>`** - transfer-function designs (first-order,
  RBJ and Zoelzer biquads, Butterworth cascades, Linkwitz-Riley crossovers,
  windowed-sinc FIR).
- **`AnalogFilterDesigner<CoeffType>`** - topology-preserving designs for the
  nonlinear analog-model filters (saturator, two-pole, Korg 35, Moog ladder,
  Roland diode, vowel).

Both are explicitly instantiated for `float` and `double`.

## Filter modes

Designers and filters share a type-safe mode system in `yup_FilterMode.h`:

- `FilterModeFlags` - tag structs: `lowpass`, `highpass`, `bandpassCsg`,
  `bandpassCpg`, `bandstop`, `peak`, `lowshelf`, `highshelf`, `allpass`.
- `FilterModeType` - a `FlagSet` combining those flags; `FilterMode` provides
  the convenience constants, including the composite
  `bandpass = bandpassCsg | bandpassCpg` ("any band-pass variant").
- `resolveFilterMode (requested, supported)` - resolves a composite request to
  the best supported variant, preferring `bandpassCsg` over `bandpassCpg`.

The two band-pass variants differ in their gain convention: **constant skirt
gain (CSG)** peaks at `Q` (0 dB in the passband), while **constant peak gain
(CPG)** peaks at 0 dB.

## FilterDesigner

`FilterDesigner` covers the classic cookbook and procedural designs. All
methods are `noexcept` and take frequencies in Hz.

### First-order sections

`designFirstOrder (mode, frequency, gain, sampleRate)` and the convenience
wrappers `designFirstOrderLowpass`, `designFirstOrderHighpass`,
`designFirstOrderLowShelf`, `designFirstOrderHighShelf`,
`designFirstOrderAllpass` return `FirstOrderCoefficients`.

### RBJ biquads

The RBJ Audio EQ Cookbook designs return `BiquadCoefficients`:

```cpp
auto peak  = FilterDesigner<double>::designRbjPeak (1000.0, 1.4, 6.0, 48000.0);
auto low   = FilterDesigner<double>::designRbjLowpass (1000.0, 0.707, 48000.0);
auto shelf = FilterDesigner<double>::designRbjHighShelf (5000.0, 0.7, -3.0, 48000.0);
```

Available: `designRbjLowpass`, `designRbjHighpass`, `designRbjBandpass`
(constant-skirt-gain form), `designRbjBandstop`, `designRbjAllpass`,
`designRbjPeak`, `designRbjLowShelf`, `designRbjHighShelf`, plus the generic
`designRbj (mode, frequency, q, gain, sampleRate)`.

### Zoelzer biquads

Zoelzer's bilinear designs (`K = tan(ω/2)`) offer both band-pass gain
conventions explicitly:

```cpp
auto bpCsg = FilterDesigner<double>::designZoelzerBandpassCsg (1000.0, 1.0, 48000.0);
auto bpCpg = FilterDesigner<double>::designZoelzerBandpassCpg (1000.0, 1.0, 48000.0);
auto notch = FilterDesigner<double>::designZoelzerNotch (1000.0, 1.0, 48000.0);
auto peaking = FilterDesigner<double>::designZoelzerPeaking (1000.0, 1.0, 6.0, 48000.0);
```

Plus low/highpass, low/high shelf, allpass, and the generic `designZoelzer`.

### Butterworth

`designButterworth` fills a `std::vector<BiquadCoefficients>` with cascaded
sections (the order is snapped to the next even number, clamped to `2..16`):

```cpp
std::vector<BiquadCoefficients<double>> sections;
FilterDesigner<double>::designButterworthLowpass (sections, 4, 1000.0, 48000.0);
// 4th-order lowpass = 2 biquads

std::vector<BiquadCoefficients<double>> bp;
FilterDesigner<double>::designButterworthBandpass (bp, 4, 200.0, 4000.0, 48000.0);
```

Convenience wrappers exist for lowpass, highpass, allpass, bandpass and
bandstop. Frequencies are clamped to `[0.0001 · fs, 0.49 · fs]`; band Q is
derived as `fc / (fh − fl)` and clamped to `[0.08, 20]`.

### Linkwitz-Riley crossovers

`designLinkwitzRiley` produces a pair of coefficient vectors (low and high
outputs) by cascading doubled Butterworth poles:

```cpp
std::vector<BiquadCoefficients<double>> low, high;
FilterDesigner<double>::designLinkwitzRiley4 (low, high, 2000.0, 48000.0);
// 2 biquads per output, -6 dB at the crossover
```

`designLinkwitzRiley2` / `designLinkwitzRiley4` / `designLinkwitzRiley8` cover
the common orders (the generic overload accepts even orders `2..16`).

### Windowed-sinc FIR

`designFIRLowpass`, `designFIRHighpass`, `designFIRBandpass`,
`designFIRBandstop` write taps into a `std::vector<CoeffType>`. Tap count is
snapped to odd, and the window is selectable (default `WindowType::hann`):

```cpp
std::vector<double> taps;
FilterDesigner<double>::designFIRLowpass (taps, 65, 5000.0, 48000.0);
```

Highpass and bandstop are built by spectral inversion of the complementary
design; bandpass is the difference of two sincs.

## AnalogFilterDesigner

`AnalogFilterDesigner` returns topology-describing coefficient structs for the
nonlinear analog-model filters in `AnalogFilters` — it deliberately does *not*
produce transfer-function biquads. All controls are normalized to `0..1`
(except frequency and sample rate); inputs are sanitized (frequencies clamped
to `[20, ...]` derived from the 18000 Hz @ 22050 Hz ratio, sample rates to
`≥ 11025`).

```cpp
AnalogTwoPoleCoefficients<float> svf =
    AnalogFilterDesigner<float>::designTwoPole (FilterMode::lowpass, 1000.f, 48000.f, 0.4f);

AnalogMoogLadderCoefficients<float> moog =
    AnalogFilterDesigner<float>::designMoogLadder (AnalogMoogLadderMode::lowpass24,
                                                   800.f, 48000.f, 0.5f, 0.2f);

AnalogVowelCoefficients<double> vowel =
    AnalogFilterDesigner<double>::designVowel (0.5, 48000.0, 0.3);
```

Designs:

| Method | Produces | Notes |
| --- | --- | --- |
| `designSaturator (drive)` | `AnalogSaturatorCoefficients` | symmetric `atan`; `drive == 0` is transparent |
| `designTwoPole (mode, freq, fs, resonance)` | `AnalogTwoPoleCoefficients` | trapezoidal SVF; lowpass, highpass, bandpass (CSG/CPG), peak, bandstop; resonance clamped `0..1` (max Q 16) |
| `designVowel (vowel, fs, resonance)` | `AnalogVowelCoefficients` | 3 cascaded two-pole peaks; `vowel` interpolates 10 vowel formant tables |
| `designKorg35 (mode, freq, fs, resonance, saturation)` | `AnalogKorg35Coefficients` | lowpass, bandpass (CSG), highpass |
| `designMoogLadder (mode, freq, fs, resonance, saturation)` | `AnalogMoogLadderCoefficients` | 4-pole ladder; 10 output modes (see `AnalogMoogLadderMode`) |
| `designRolandDiode (freq, fs, resonance, saturation)` | `AnalogRolandDiodeCoefficients` | diode-ladder lowpass |

`AnalogMoogLadderMode` enumerators:

`lowpass24`, `highpass24`, `lowpass18`, `highpass18`, `lowpass12`, `highpass12`,
`lowpass6`, `highpass6`, `bandpass12`, `bandpass6`.

## Coefficient containers

| Struct | Members | Used by |
| --- | --- | --- |
| `FirstOrderCoefficients` | `a1`, `b0`, `b1` (a0 = 1) | `FirstOrder` |
| `BiquadCoefficients` | `a0, a1, a2, b0, b1, b2` + `normalize()` | `Biquad`, `BiquadCascade`, all biquad filters |
| `StateVariableCoefficients` | `k`, `g`, `damping` | state-variable filters |
| `AnalogSaturatorCoefficients` | `drive`, `preScale`, `postScale` | `AnalogSaturator` |
| `AnalogTwoPoleCoefficients` | `g`, `h`, `r2`, `gainCorrection`, `lowOut`, `bandOut`, `highOut` | two-pole / vowel filters |
| `AnalogOnePoleCoefficients` | `alpha`, `beta` | ladder poles |
| `AnalogKorg35Coefficients` | `poles[3]`, `alpha0`, `feedback`, `gainCorrection` | Korg 35 |
| `AnalogMoogLadderCoefficients` | `poles[4]`, `outputs[5]`, `alpha0`, `feedback`, `gainCorrection` | Moog ladder |
| `AnalogRolandDiodeCoefficients` | `cutoff`, `feedback`, `gainCorrection`, `a…fg`, `highpassA/B` | Roland diode |
| `AnalogVowelCoefficients` | `formants[3]`, `gainCompensation` | vowel filter |

## Related

- [Filters](filters.md) - how to feed these coefficients into the processing
  classes.
- [Math](math.md) - `bilinearTransform`, `dbToGain`, and Q/bandwidth
  conversions used by the designers.
