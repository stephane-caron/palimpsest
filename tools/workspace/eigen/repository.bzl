# -*- python -*-

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")


def eigen_repository(
        version = "3.3.9",
        sha256 = "690caabab3b813d944a27b074a847cdd9e9a824af0b1bab772c0ab624479b1e2"):
    """
    Download repository from GitLab as a ZIP archive, decompress it, and make
    its targets available for binding.

    Args:
        version: version of the library to get.
        sha256: SHA-256 checksum of the downloaded archive.

    Note:
        Eigen release archives suffer from checksum issues that seem, as of
        April 2025, to be caused by the GitLab infrastructure, as detailed in
        https://gitlab.com/libeigen/eigen/-/issues/2919 and
        https://gitlab.com/libeigen/eigen/-/issues/2923. This behavior is the
        reason why we rolled back from Eigen 3.4.0 to Eigen 3.3.9.
    """
    http_archive(
        name = "eigen",
        urls = [
            "https://gitlab.com/libeigen/eigen/-/archive/{}/eigen-{}.zip".format(version, version),
        ],
        sha256 = sha256,
        strip_prefix = "eigen-{}".format(version),
        build_file = Label("//tools/workspace/eigen:package.BUILD"),
    )
