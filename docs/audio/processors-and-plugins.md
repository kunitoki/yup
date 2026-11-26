# Processors and plugins

The processor model, and how processors become plugins, host plugins and are
routed in a graph.

- [Audio processors](processors.md) - the processor model, parameters, state
  and editors.
- [Spectral processing](spectral.md) - frequency-domain processors.
- [Writing plugins](plugin-client.md) - one processor as CLAP, VST3, AU, AUv3,
  LV2, AAX and standalone.
- [Plugin hosting](plugin-hosting.md) - scanning, loading and sandboxing
  third-party plugins.
- [Audio graph](audio-graph.md) - realtime routing of processors and plugins.
- [Headless audio](headless.md) - the audio engine in a console application,
  without `yup_gui`.

```{toctree}
:hidden:
:maxdepth: 1

processors
spectral
plugin-client
plugin-hosting
audio-graph
headless
```
