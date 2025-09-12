# Developer notes {#dev-notes}

[TOC]

## Adding custom types

Adding a new custom type boils down to the following steps:

* Add implicit type conversions to `Dictionary.h`
* Add a read function specialization to `mpack/read.h`
* Add a write function specialization to `mpack/Writer.h`
* Add a write function specialization to `mpack/write.h`
* Add a write function specialization to `json/write.h`

Take a look at the existing types in these files and in unit tests for inspiration.

## Insertion order behavior

Palimpsest `Dictionary` objects do not preserve insertion order, unlike Python dictionaries which guarantee insertion order preservation since Python 3.7. This difference stems from our use of `std::unordered_map` as the underlying storage in the `Dictionary` class, which prioritizes lookup performance over order preservation.

While we could consider migrating to an alternative data structure to enforce insertion order conservation (note that `std::map` would not solve this as it maintains sorted order by key, not insertion order), such changes would require careful performance evaluation. In palimpsest, we explicitly prioritize performance over Python compatibility, so future structural changes such as that one should demonstrate that they don't compromise the library's performance on a benchmark of representative use cases.

## Not a header-only library

The main blocker to distributing palimpsest as a header-only library is that we set a custom flush function `mpack_std_vector_writer_flush` to our internal MPack writers. The [MPack Write API](https://ludocode.github.io/mpack/group__writer.html) requires a function pointer for that, and we define that function in the internal `palimpsest::mpack::Writer` class. Open a PR if you have ideas to go around that!
