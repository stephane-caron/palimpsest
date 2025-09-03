# Developer notes {#dev-notes}

[TOC]

## Not a header-only library

The main blocker to distributing palimpsest as a header-only library is that we set a custom flush function `mpack_std_vector_writer_flush` to our internal MPack writers. The [MPack Write API](https://ludocode.github.io/mpack/group__writer.html) requires a function pointer for that, and we define that function in [`Writer.cpp`](@ref palimpsest::mpack::Writer). Open a PR if you have ideas to go around that!
