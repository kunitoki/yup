# Animation

`yup_animation` plays and authors [Lottie](https://lottiefiles.com) animations:
vector animations exported from After Effects (or any Lottie tool) as JSON, and
drawn with the same [`Graphics`](../graphics/graphics-class.md) API your
components paint with.

**Modules covered:** `yup_animation`.

Add the module to your target and include it:

```cpp
#include <yup_animation/yup_animation.h>
```

It depends only on `yup_core` and `yup_graphics`, so animations can be rendered
anywhere you have a `Graphics` - a component, an offscreen canvas, or an export
loop.

## The two classes you need

| Class | What it is |
| --- | --- |
| `Animation` | An immutable loaded animation: its size, frame rate, duration, and the ability to render any frame. |
| `AnimationPlayer` | Playback state on top of an `Animation`: play / pause, speed, looping, direction and the current frame. |

Everything else - compositions, layers, groups, properties - is the model the
two are built on, and you only reach for it when you want to
[override values at runtime](#overriding-values-at-runtime).

## Loading an animation

```cpp
auto animation = yup::Animation::loadFromFile (yup::File ("logo.json"));

if (! animation.isValid())
    return; // the file was missing or not valid Lottie
```

`loadFromFile` accepts both `.json` and the zipped `.lottie` format.
`loadFromData` takes the JSON as a `String`, which is what you want for an
animation embedded in your binary resources, and `loadFromStream` reads from
any `InputStream`.

None of them throw or return a `Result` - a failed load gives you an invalid
`Animation`, so always check `isValid()`.

If the animation references external images with relative paths, tell the
loader where they are:

```cpp
yup::Animation::LoadOptions options;
options.resourceDirectory = yup::File ("resources/animations");

auto animation = yup::Animation::loadFromFile (file, options);
```

Once loaded you can ask it about itself:

```cpp
const auto frames   = animation.totalFrames();  // length in frames
const auto fps      = animation.frameRate();    // frames per second
const auto seconds  = animation.duration();     // length in seconds
const auto natural  = animation.size();         // the authored size, in points
```

## Playing it in a component

`AnimationPlayer` holds the playhead. Advance it once per displayed frame from a
`FrameAttachment`, and draw it from `paint`:

```cpp
class AnimatedLogo : public yup::Component
{
public:
    AnimatedLogo()
        : Component ("AnimatedLogo")
        , player (yup::Animation::loadFromFile (yup::File ("logo.json")))
    {
        player.setLooping (true);
        player.play();
    }

    void paint (yup::Graphics& g) override
    {
        player.render (g, getLocalBounds().to<float>());
    }

private:
    void advance (double elapsedSeconds)
    {
        if (player.advanceTime (static_cast<float> (elapsedSeconds)))
            repaint();
    }

    yup::AnimationPlayer player;
    yup::FrameAttachment frameAttachment { *this, yup::bindFront (&AnimatedLogo::advance, this) };
};
```

`advanceTime` takes seconds and returns true only when the displayed frame
actually changed, so repainting on its result keeps an idle or slow animation
from redrawing the window every vsync.

```{note}
The function runs for every frame of the window the component is in, whether or
not anything is repainted. See
[Component basics](../ui/component-basics.md#per-frame-updates).
```

## Controlling playback

```cpp
player.play();                                        // from the current frame
player.pause();                                       // keep the playhead
player.stop();                                        // and rewind it

player.setSpeed (0.5f);                               // half speed
player.setLooping (true);
player.setDirection (yup::AnimationPlayer::Direction::PingPong);
player.setFrameRange (24.0f, 96.0f);                  // 0, 0 means the whole animation

player.seekToFrame (48.0f);
player.seekToProgress (0.5f);                         // 0 to 1 over the frame range
```

`Direction` is `Forward`, `Reverse` or `PingPong`. Three callbacks report what
the playhead did, all called from whichever thread calls `advanceTime`:

```cpp
player.onFrameChanged  = [] (float frameNo) { /* ... */ };
player.onLoopCompleted = [] { /* ... */ };
player.onPlaybackEnded = [] { /* ... */ };  // non-looping playback reached the end
```

Swapping the content keeps the player and its settings:

```cpp
player.setAnimation (yup::Animation::loadFromFile (otherFile));
```

## Fitting the animation to its bounds

Both `Animation` and `AnimationPlayer` render with a
[`Fitting`](../graphics/primitives.md) and a `Justification`, defaulting to
`Fitting::scaleToFit` and `Justification::center`:

```cpp
player.render (g, getLocalBounds().to<float>(),
               yup::Fitting::scaleToFill,
               yup::Justification::topLeft);
```

`scaleToFit` keeps the aspect ratio inside the bounds, `scaleToFill` covers
them and may crop, `fill` stretches, and `none` draws at the authored size.

## Rendering a specific frame

When you drive the time yourself - a timeline, a scrubber, a progress ring, an
offscreen export - skip the player and render straight from the `Animation`:

```cpp
animation.renderFrame      (g, 42.0f, bounds);   // by frame number
animation.renderAtTime     (g, 1.75f, bounds);   // by seconds
animation.renderAtProgress (g, 0.25f, bounds);   // by 0 to 1
```

Animations that use track mattes need offscreen canvases while they draw. Pass
a long-lived `AnimationRenderResources` so those canvases are pooled and reused
between frames instead of being reallocated:

```cpp
class MatteHeavyView : public yup::Component
{
    void paint (yup::Graphics& g) override
    {
        animation.renderAtTime (g, time, getLocalBounds().to<float>(),
                                yup::Fitting::scaleToFit,
                                yup::Justification::center,
                                &renderResources);
    }

    yup::Animation animation;
    yup::AnimationRenderResources renderResources;  // keep it alive across frames
};
```

## Overriding values at runtime

A loaded animation is data, so you can retint or retime parts of it without
re-exporting from the design tool. Overrides are installed on the composition
against a dot-separated *key path* that names the layer, group and shape, and a
property to replace:

```cpp
auto* composition = animation.getComposition();

composition->setPropertyOverride<yup::Color> (
    "Logo.Group 1.Fill 1",
    yup::AnimationPropertyID::FillColor,
    [] (float frameNo) -> std::optional<yup::Color>
    {
        return yup::Color (0xffff8800);
    });
```

The callback is given the frame number and returns the value to use, or
`std::nullopt` to fall back to the authored one - so you can override only part
of the timeline, or animate the override against your own data.

Key paths accept wildcards: `*` matches one name and `**` matches any number,
so `"**.Fill 1"` reaches every fill in the animation.

`AnimationPropertyID` covers the values worth driving from code: fill and stroke
colors, opacity, stroke width, and the transform channels (position, rotation,
scale, anchor and transform opacity).

## Saving and exporting

Animations round-trip back to Lottie JSON, which is how you ship an animation
you built or modified in code:

```cpp
const auto json = animation.toJson();                    // prettyPrint by default

if (auto result = animation.saveToFile (destination); result.failed())
    yup::Logger::writeToLog (result.getErrorMessage());
```

`LottieReader` and `LottieWriter` are the lower-level entry points. Use
`LottieReader` when you want the parse error rather than a validity flag, since
it returns a `ResultValue`:

```cpp
auto parsed = yup::LottieReader::parseFile (file);

if (! parsed.wasOk())
    yup::Logger::writeToLog (parsed.getErrorMessage());
else
    auto animation = yup::Animation::fromComposition (parsed.getValue());
```

It also reads the zipped `.lottie` container, which can hold several animations:

```cpp
for (const auto& id : yup::LottieReader::listAnimationIds (bundle))
{
    auto parsed = yup::LottieReader::parseFromZip (bundle, id);
    // ...
}
```

`LottieWriter::toJson` and `LottieWriter::toFile` write a composition back out
as JSON. They do not produce the zipped `.lottie` container.

## Limitations

- Text layers are parsed but not rendered; they behave as empty layers.
- The writer emits plain JSON only, never a `.lottie` zip.
- Expressions inside the Lottie file are evaluated only where the bundled
  expression support covers them.

## Related areas

- [Graphics](../graphics/index.md) - the `Graphics` API animations render with.
- [UI](../ui/index.md) - components, `FrameAttachment` and continuous rendering.
- [Artboards (Rive)](../ui/artboard.md) - the other animation runtime in YUP,
  for Rive `.riv` files with state machines and data binding.
