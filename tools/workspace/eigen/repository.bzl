# -*- python -*-

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")


def eigen_repository():
    """
    Clone Eigen repository and expose it as a Bazel target.

    Note:
        Previously this rule followed the better practice of downloading source
        code as an archive, decompressing it, and making its targets available
        for binding. Eigen release archives suffer however from checksum issues
        that stem, as of the analysis in April 2025, from the GitLab
        infrastructure rather than from the Eigen project. The investigation is
        as detailed in https://gitlab.com/libeigen/eigen/-/issues/2919 and
        https://gitlab.com/libeigen/eigen/-/issues/2923. This behavior is the
        reason why we rolled back to a git-repository rule.
    """
    git_repository(
        name= "eigen",
        remote = "https://gitlab.com/libeigen/eigen.git",
        commit = "3147391d946bb4b6c68edd901f2add6ac1f31f8c",  # version 3.4.0
        build_file = Label("//tools/workspace/eigen:package.BUILD"),
    )
