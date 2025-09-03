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

## Not a header-only library

The main blocker to distributing palimpsest as a header-only library is that we set a custom flush function `mpack_std_vector_writer_flush` to our internal MPack writers. The [MPack Write API](https://ludocode.github.io/mpack/group__writer.html) requires a function pointer for that, and we define that function in [`Writer.cpp`](@ref palimpsest::mpack::Writer). Open a PR if you have ideas to go around that!
