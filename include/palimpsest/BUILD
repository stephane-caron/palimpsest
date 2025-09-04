# -*- python -*-

load("//tools/lint:lint.bzl", "add_lint_tests")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "dictionary",
    hdrs = [
        "Dictionary.h",
        "Value.h",
    ],
    include_prefix = "palimpsest",
    deps = [
        "//include/palimpsest/exceptions",
        "//include/palimpsest/internal",
        "//include/palimpsest/json",
        "//include/palimpsest/mpack",
        "@fmt",
        "@spdlog",
    ],
)

add_lint_tests()
