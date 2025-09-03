# Examples

## Getting started

Check out how to [use palimpsest dictionaries like Python ones](use_like_python_dict.cpp):

```
./tools/bazelisk run //examples:use_like_python_dict
```

[Save dictionary](save_dictionary.cpp): serialize a C++ dictionary to a MessagePack file.

```
./tools/bazelisk run //examples:save_dictionary
```

[Save/load a dictionary](save_load_dictionary.cpp): save and load a dictionary to/from a MessagePack file.

```
./tools/bazelisk run //examples:save_load_dictionary
```

[Write dictionary](write_dictionary.cpp): write a small dictionary to the standard output.

```
./tools/bazelisk run //examples:write_dictionary
```

## Advanced

[Simple logger](simple_logger.cpp): log dictionaries to a MessagePack file (the output will be located in your Bazel cache, check out ``find ./bazel-out/ -name 'simple_logger.mpack'``):

```
./tools/bazelisk run //examples:simple_logger
```

[Dictionary difference](dictionary_difference.cpp): compute and display differences between dictionaries.

```
./tools/bazelisk run //examples:dictionary_difference
```

[Delta compression](delta_compression.cpp): demonstrate delta compression between dictionary states.

```
./tools/bazelisk run //examples:delta_compression
```
