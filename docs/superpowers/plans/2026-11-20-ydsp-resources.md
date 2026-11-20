# YDSP resource implementation plan

This is planned functionality, not a description of currently supported APIs.
It amends the resource phase of the YDSP safety, tracing, resources and libraries
plan. The current priority is language features: tracing, match blocks, aggregate
types, improved diagnostics and reusable libraries. Dynamic resources and
multichannel playback remain planned after that language work.

## Performance acceptance for each phase

Safety changes must preserve the speed of proven-safe processing paths. Prefer
compile-time proofs and block-level validation over per-sample guards. Review
generated code and compare the affected benchmarks against the preceding
revision, including echo and reverb when indexing or arithmetic changes.
Correctness tests alone do not establish performance acceptance; investigate
regressions before advancing to the next phase.

## Remaining arithmetic safety work

Width-correct shift and numeric-conversion folding and runtime float-to-integer
saturation/NaN-to-zero handling have passed the reported correctness suite.
ARM64 benchmarks retain the recovered echo and reverb performance; x64 timings
remain unconfirmed. Proven bounded casts keep their direct instruction path;
ARM64 also handles arbitrary inputs in one instruction.

Integer constant folding normalizes operands and results to their storage width;
the reported suite passed. Signed int32 multiplication bounds proofs also passed
(945/945 tests), with the ARM64 echo/reverb listings and performance preserved.

Integer kernel addition, subtraction and multiplication now saturate in native
and WebAssembly code and constant folding. Source arithmetic is distinguished
from compiler-generated indexing. Validated ring arithmetic and nonoverflowing
int32 ranges retain direct instructions. The reported correctness suite passed.
Performance validation still requires comparison of echo, reverb, chorus and
idle-voice processing; unproven arithmetic requires overflow handling.

## Dynamic blob and sample collections

Replace the earlier restriction that each binding holds only one resource and
that waveform banks are deferred. Keep single table/waveform bindings, and add
runtime-sized resource collections so the host can forward a changing number
of samples without recompiling the graph.

- A collection publication supplies its runtime count and immutable descriptors
  for its members. Members may have different shapes. Numeric blobs describe
  element type, dimensions, extents, byte strides and accessible byte extent;
  waveforms additionally describe float32 audio, frame/channel counts and source
  sample rate. Keep supported element types explicit rather than interpreting
  arbitrary bytes as language values.
- Expose read-only collection length and member metadata to YDSP, with checked
  member/channel lookup and indexing. Missing resources and invalid lookups act
  as empty data. Shapes and counts may change between publications. Runtime
  sizes do not authorize unbounded loops.
- Let the host create, register, publish, clear and query collections through
  fallible `Result`/`ResultValue` APIs. Validate every descriptor, including
  dimension/stride arithmetic, alignment, accessible storage and finite positive
  waveform sample rates, before accepting a publication.
- Publish the collection and its descriptors as one immutable generation at the
  start of a valid nonempty outer processing call. Every node, event subdivision
  and oversampling step observes the same generation for that call. Forward
  collections through graphs and subgraphs.
- A voice may capture a retained member handle on note-on. Replacing, shrinking
  or clearing the collection does not invalidate captured samples or release
  tails. Persistent state stores retained handles, never borrowed descriptors,
  slices or raw pointers.
- Configure maximum live generations, members, descriptor dimensions and queued
  publications during preparation. Runtime count and shape vary within these
  capacities. Exceeding capacity fails publication without changing the current
  binding; increasing capacity requires quiescent preparation.
- Preserve the original lifetime contract: one serialized control producer, one
  audio consumer, preallocated generation-tagged records and bounded queues.
  Queued publications, bindings, processing and persistent state retain their
  resources. Audio-thread release only updates bounded bookkeeping; retirement,
  destruction and external callbacks run on the collecting control thread.
  Reuse slots only after retirement acknowledgement. External storage remains
  immutable and accessible until release; WebAssembly zero-copy requires
  accessible linear-memory storage, otherwise copy.

Concrete collection syntax and host descriptor types will be settled during
the resource implementation, alongside uniform metadata and resource IR/ABIs.

## Validation additions

Cover empty collections, growth/shrinkage, heterogeneous member shapes, changing
frame/channel counts, graph forwarding and invalid member/dimension accesses.
Verify atomic visibility across nodes and subdivisions, retained voice tails
after replacement/removal, clearing/reset/stealing, capacity exhaustion without
partial publication, slot reuse, and exactly-once off-thread external release.
Check native and WebAssembly behavior and zero render-thread allocation,
deallocation, logging or external release callbacks, including overflow and
delayed collection.
