# -*- python -*-
#
# Copyright 2022 Stéphane Caron

load("//tools/lint:lint.bzl", "add_lint_tests")

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "delta_compression",
    srcs = ["delta_compression.cpp"],
    deps = ["@palimpsest"],
)

cc_binary(
    name = "dictionary_difference",
    srcs = ["dictionary_difference.cpp"],
    deps = ["@palimpsest"],
)

py_binary(
    name = "load_dictionary",
    srcs = ["load_dictionary.py"],
)

cc_binary(
    name = "python_like_usage",
    srcs = ["python_like_usage.cpp"],
    deps = ["@palimpsest"],
)

cc_binary(
    name = "save_dictionary",
    srcs = ["save_dictionary.cpp"],
    deps = ["@palimpsest"],
)

cc_binary(
    name = "save_load_dictionary",
    srcs = ["save_load_dictionary.cpp"],
    deps = ["@palimpsest"],
)

cc_binary(
    name = "simple_logger",
    srcs = ["simple_logger.cpp"],
    deps = ["@palimpsest"],
)

cc_binary(
    name = "write_dictionary",
    srcs = ["write_dictionary.cpp"],
    deps = ["@palimpsest"],
)

add_lint_tests()
