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

## Difference between dictionaries

The `difference` function returns a new dictionary containing only the key-value pairs that are in the first dictionary and either not present in the second dictionary, or present but with different values.

```cpp
Dictionary config1, config2;

config1("server")("port") = 8080;
config1("server")("host") = std::string("localhost");
config1("database")("port") = 5432;

config2("server")("port") = 9090;  // different port
config2("server")("host") = std::string("localhost");  // same host
config2("database")("port") = 5432;  // same port

Dictionary diff = config1.difference(config2);

// diff will contain only: {"server": {"port": 8080}}
```

The difference function preserves the nested structure of dictionaries.
