# -*- python -*-

load("//tools/lint:lint.bzl", "add_lint_tests")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "internal",
    hdrs = [
        "Allocator.h",
        "fmt_formatter.h",
        "is_valid_hash.h",
        "type_name.h",
    ],
    include_prefix = "palimpsest/internal",
    deps = [
        "@fmt",
    ],
)

add_lint_tests()
