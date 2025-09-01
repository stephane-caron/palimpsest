# Usage {#usage}

[TOC]

## Serialization to bytes

Dictionaries can be serialized (`palimpsest::Dictionary::serialize`) to vectors of bytes:

```cpp
Dictionary world;
std::vector<char> buffer;
size_t size = world.serialize(buffer);
```

The function resizes the buffer automatically if needed, and returns the number of bytes of the serialized message.

## Deserialization from bytes

Dictionaries can be updated (`palimpsest::Dictionary::update`) from byte vectors:

```cpp
Dictionary foo;
foo("bar") = 1;
foo("foo") = 2;

Dictionary bar;
bar("bar") = 3;
std::vector<char> buffer;
size_t size = bar.serialize(buffer);

foo.update(buffer.data(), size);  // OK, now foo("bar") == 3
```

## Adding custom types

Adding a new custom type boils down to the following steps:

* Add implicit type conversions to `Dictionary.h`
* Add a read function specialization to `mpack/read.h`
* Add a write function specialization to `mpack/Writer.h`
* Add a write function specialization to `mpack/write.h`
* Add a write function specialization to `json/write.h`

Take a look at the existing types in these files and in unit tests for inspiration.
