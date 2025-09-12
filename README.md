# palimpsest — Fast serializable C++ dictionaries

[![CI](https://img.shields.io/github/actions/workflow/status/stephane-caron/palimpsest/bazel.yml?branch=main)](https://github.com/stephane-caron/palimpsest/actions)
[![Documentation](https://img.shields.io/badge/docs-online-brightgreen?style=flat)](https://stephane-caron.github.io/palimpsest/)
[![Coverage](https://coveralls.io/repos/github/stephane-caron/palimpsest/badge.svg?branch=main)](https://coveralls.io/github/stephane-caron/palimpsest?branch=main)
![C++ version](https://img.shields.io/badge/C++-17/20-blue.svg?style=flat)
[![Release](https://img.shields.io/github/v/release/stephane-caron/palimpsest.svg?sort=semver)](https://github.com/stephane-caron/palimpsest/releases)

The palimpsest library implements a `Dictionary` type for C++ meant for fast value updates and serialization. It is called [palimpsest](https://en.wiktionary.org/wiki/palimpsest#Noun) because dictionaries are designed for frequent rewritings (values change fast) on the same support (keys change slow).

## Example

Let's fill a dictionary and print it to the standard output:

```cpp
#include <iostream>

#include <palimpsest/Dictionary.h>

using palimpsest::Dictionary;

int main() {
  Dictionary world;
  world("name") = "example";
  world("temperature") = 28.0;

  auto& bodies = world("bodies");
  bodies("plane")("orientation") = Eigen::Quaterniond{0.9239, 0.3827, 0., 0.};
  bodies("plane")("position") = Eigen::Vector3d{0.0, 0.0, 100.0};
  bodies("truck")("orientation") = Eigen::Quaterniond::Identity();
  bodies("truck")("position") = Eigen::Vector3d{42.0, 0.0, 0.0};

  std::cout << world << std::endl;
  return EXIT_SUCCESS;
}
```

This code outputs:

```json
{"bodies": {"truck": {"position": [42, 0.5, 0], "orientation": [1, 0, 0, 0]},
"plane": {"position": [0.1, 0, 100], "orientation": [0.9239, 0.3827, 0, 0]}},
"temperature": 28, "name": "example"}
```

We can serialize the dictionary to file:

```cpp
world.write("world.mpack");
```

And deserialize it likewise:

```cpp
Dictionary world_bis;
world_bis.read("world.mpack");
std::cout << world_bis << std::endl;
```

Dictionaries can also be serialized to bytes for transmission over TCP, memory-mapped files, telegraph lines, etc. Check out the [examples](https://github.com/stephane-caron/palimpsest/tree/main/examples) directory for more advanced use cases.

## Comparison to Python dictionaries

Palimpsest will feel familiar if you are used to Python dictionaries, as its API is a subset of Python's `dict`:

| Python `dict` | Palimpsest `Dictionary` |
|---------------|---------------------------|
| [`dict.clear`](https://docs.python.org/3/library/stdtypes.html#dict.clear) | [`Dictionary::clear`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#ae98a88dd6a1c5e5afa84f719189882d9) |
| [`dict.copy`](https://docs.python.org/3/library/stdtypes.html#dict.copy) | Dictionaries are [move-only](https://github.com/stephane-caron/palimpsest?tab=readme-ov-file#non-features) |
| [`copy.deepcopy`](https://docs.python.org/3/library/copy.html#copy.deepcopy) | [`Dictionary::deepcopy`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#a2ddbd4316657e27d11c5a07a476841e6) |
| [`dict.fromkeys`](https://docs.python.org/3/library/stdtypes.html#dict.fromkeys) | [`Dictionary::fromkeys`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#afdd77ff6fefa0d086cd9a32c1f75cb96) |
| [`dict.get`](https://docs.python.org/3/library/stdtypes.html#dict.get) | [`Dictionary::get`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#a74bd56b0ec9e4219f54430bcb6f9a084) |
| [`dict.items`](https://docs.python.org/3/library/stdtypes.html#dict.items) | [`Dictionary::items`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#ac38517fbe0ac91132d98216224577f48) |
| [`dict.keys`](https://docs.python.org/3/library/stdtypes.html#dict.keys) | [`Dictionary::keys`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#abb1589b67dbeadec8774833921644798)  |
| [`dict.pop`](https://docs.python.org/3/library/stdtypes.html#dict.pop) | [`Dictionary::pop`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#a26a5e2623e96221ddae92677f41a69a1) |
| [`dict.popitem`](https://docs.python.org/3/library/stdtypes.html#dict.popitem) | [`Dictionary::popitem`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#a1f131706b9fe3e3ccaaeae405c81f27c) |
| [`dict.setdefault`](https://docs.python.org/3/library/stdtypes.html#dict.setdefault) | [`Dictionary::setdefault`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#aaa4a895aa93a7483ea49d3231f5e0b6b) |
| [`dict.update`](https://docs.python.org/3/library/stdtypes.html#dict.update) | [`Dictionary::update`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#a1b5bb02bcf813b05aef280f47b25ce80) |
| [`dict.values`](https://docs.python.org/3/library/stdtypes.html#dict.values) | [`Dictionary::values`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#af71082c3d7f7c6624fb76fde3eb88ad3) |

There are a few differences:

- Dictionaries are move-only: there is no `copy` function matching Python's `dict.copy`, but `Dictionary::deepcopy` matches Python's `copy.deepcopy`.
- Dictionaries do not conserve insertion order, whereas insertion order conservation is guaranteed in Python since version 3.7 of the language.

## Features and non-features

The two main assumptions in palimpsest dictionaries are that:

* **Keys** are strings.
* **Values** hold either a sub-dictionary or a type that can be unambiguously serialized.

### Features

* Returns references to any stored value or sub-dictionary
* Built-in support for [Eigen](https://eigen.tuxfamily.org/)
* Print dictionaries to standard output as [JSON](https://www.json.org/json-en.html)
* Serialize to and deserialize from [MessagePack](https://msgpack.org/)

### Non-features

* Dictionaries are move-only (deep copies are possible, but now shallow copies)
* Types need to deserialize unambiguously (*e.g.*, positive integers always deserialize to `unsigned`)
* Array values are mostly limited to Eigen tensors (matrix, quaternion, vector)

If any of these design decisions doesn't match what you are looking for, you can also check out a list of alternative libraries below.

## Installation

### Bazel

Add to your `WORKSPACE` file the `http_archive` instruction from the [release page](https://github.com/stephane-caron/palimpsest/releases/tag/v3.0.0).

You can then define C++ targets that depend on ``@palimpsest``:

```python
cc_binary(
    name = "my-target",
    srcs = ["my-target.cpp"],
    deps = ["@palimpsest"],
)
```

### CMake

Make sure Eigen, fmt and spdlog are installed system-wise, for instance on Debian-based distributions:

```console
sudo apt install libeigen3-dev libfmt-dev libspdlog-dev
```

Then follow the standard CMake procedure:

```console
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
make install
```

Note that by default [MPack](https://github.com/ludocode/mpack) will be built and installed from the [``third_party``](https://github.com/stephane-caron/palimpsest/tree/main/third_party) folder. Set `-DBUILD_MPACK=OFF` if you already have MPack 1.1 or later installed on your system.

## See also

The `Dictionary` class in palimpsest derives from two classes of the [mc\_rtc](https://github.com/jrl-umi3218/mc_rtc/) robotics framework: `mc_rtc::Configuration` (similar API, based on RapidJSON) and `mc_rtc::DataStore` (general value types, but not serializable).

If some of the design decisions in palimpsest don't match your requirements, you can also check out the following alternatives:

* [JSON for Modern C++](https://github.com/nlohmann/json): most user-friendly library of this list, serializes to MessagePack and other binary formats, but not designed for speed.
* [mjlib](https://github.com/mjbots/mjlib): includes a `telemetry` library if your use case is more specifically in robotics or embedded systems.
* [Protocol Buffers](https://developers.google.com/protocol-buffers/): good fit if you have a fixed schema (keys don't change at all) that you want to serialize to and from.
* [RapidJSON](https://github.com/Tencent/rapidjson/): low memory footprint, can serialize to MessagePack using other [related projects](https://github.com/Tencent/rapidjson/wiki/Related-Projects), but has linear lookup complexity as it stores dictionaries [as lists of key-value pairs](https://github.com/Tencent/rapidjson/issues/102).
* [simdjson](https://github.com/simdjson/simdjson/): uses SIMD instructions and microparallel algorithms to parse JSON (reportedly 4x faster than RapidJSON and 25x faster than JSON for Modern C++).
