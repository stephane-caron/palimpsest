# palimpsest — Fast serializable C++ dictionaries

[![CI](https://img.shields.io/github/actions/workflow/status/stephane-caron/palimpsest/bazel.yml?branch=main)](https://github.com/stephane-caron/palimpsest/actions)
[![Documentation](https://img.shields.io/badge/docs-online-brightgreen?style=flat)](https://stephane-caron.github.io/palimpsest/)
[![Coverage](https://coveralls.io/repos/github/stephane-caron/palimpsest/badge.svg?branch=main)](https://coveralls.io/github/stephane-caron/palimpsest?branch=main)
![C++ version](https://img.shields.io/badge/C++-17/20-blue.svg?style=flat)
[![Release](https://img.shields.io/github/v/release/stephane-caron/palimpsest.svg?sort=semver)](https://github.com/stephane-caron/palimpsest/releases)

_palimpsest_ implements a `Dictionary` type for C++ meant for fast value updates and serialization. It is called [palimpsest](https://en.wiktionary.org/wiki/palimpsest#Noun) because dictionaries are designed for frequent rewritings (values change fast) on the same support (keys change slow).

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

Dictionaries can also be serialized to bytes for transmission over TCP, memory-mapped files, telegraph lines, etc. Code in the [examples](https://github.com/stephane-caron/palimpsest/tree/main/examples) directory shows how to save and load dictionaries to and from C++ and Python.

## Link with Python dictionaries

_palimpsest_ will feel familiar if you are used to Python dictionaries, as its API is a subset of Python's `dict`:

| Python `dict` | _palimpsest_ `Dictionary` |
|---------------|---------------------------|
| `clear`       | [`clear`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#ae98a88dd6a1c5e5afa84f719189882d9) |
| `copy`        | ✖️  |
| `fromkeys`    | ✖️  |
| `get`         | [`get`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#a74bd56b0ec9e4219f54430bcb6f9a084) |
| `items`       | ✖️  |
| `keys`        | [`keys`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#abb1589b67dbeadec8774833921644798)  |
| `pop`         | ✖️  |
| `popitem`     | ✖️  |
| `setdefault`  | ✖️  |
| `update`      | [`update`](https://stephane-caron.github.io/palimpsest/classpalimpsest_1_1Dictionary.html#a1b5bb02bcf813b05aef280f47b25ce80) |
| `values`      | ✖️  |

Implementing one of the missing functions is a great way to [contribute](CONTRIBUTING.md) to this project.

## Features and non-features

All design decisions have their pros and cons. Take a look at the features and non-features below to decide if it is also a fit for _your_ use case.

The two main assumptions in _palimpsest_ dictionaries are that:

* **Keys** are strings.
* **Values** hold either a sub-dictionary or a type that can be unambiguously serialized.

### Features

* Prioritizes speed over user-friendliness
* Returns references to any stored value or sub-dictionaries
* Built for fast inter-process communication with [Python](https://www.python.org/)
* Built-in support for [Eigen](https://eigen.tuxfamily.org/)
* Serialize to and deserialize from [MessagePack](https://msgpack.org/)
* Print dictionaries to standard output as [JSON](https://www.json.org/json-en.html)
* Extensible to custom types, as long as they deserialize unambiguously

### Non-features

* Prioritizes speed over user-friendliness
* Array values are mostly limited to Eigen tensors (matrix, quaternion, vector)
* No copy constructor (but a move constructor)
* Custom types need to deserialize unambiguously
* Shallow and deep copies are not implemented ([PRs welcome](CONTRIBUTING.md))

Check out the existing [alternatives](https://github.com/stephane-caron/palimpsest#alternatives) if any of these choices is a no-go for you.

## Installation

### Bazel

Add to your `WORKSPACE` file the `http_archive` instruction from the [release page](https://github.com/stephane-caron/palimpsest/releases/tag/v2.4.0).

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

## Going further

Further usage examples including serialization, deserialization, and adding custom types, are provided in the [documentation](https://stephane-caron.github.io/palimpsest/).

## See also

* [JSON for Modern C++](https://github.com/nlohmann/json): most user-friendly library of this list, serializes to MessagePack and other binary formats, but not designed for speed.
* [`mc_rtc::Configuration`](https://github.com/jrl-umi3218/mc_rtc/blob/master/include/mc_rtc/Configuration.h) - similar API to palimpsest, based on RapidJSON (see below).
* [`mc_rtc::DataStore`](https://github.com/jrl-umi3218/mc_rtc/blob/master/include/mc_rtc/DataStore.h) - can hold more general value types, like lambda functions, but does not serialize.
* [`mjlib::telemetry`](https://github.com/mjbots/mjlib/tree/master/mjlib/telemetry) - if your use case is more specifically telemetry in robotics or embedded systems.
* [Protocol Buffers](https://developers.google.com/protocol-buffers/): good fit if you have a fixed schema (keys don't change at all) that you want to serialize to and from.
* [RapidJSON](https://github.com/Tencent/rapidjson/): low memory footprint, can serialize to MessagePack using other [related projects](https://github.com/Tencent/rapidjson/wiki/Related-Projects), but has linear lookup complexity as it stores dictionaries [as lists of key-value pairs](https://github.com/Tencent/rapidjson/issues/102).
* [simdjson](https://github.com/simdjson/simdjson/): uses SIMD instructions and microparallel algorithms to parse JSON (reportedly 4x faster than RapidJSON and 25x faster than JSON for Modern C++).
