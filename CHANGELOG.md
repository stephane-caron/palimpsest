# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added

- examples: Show how to use palimpsest dictionaries like Python ones

## [2.4.0] - 2025-09-03

### Added

- Add `difference` function for dictionary difference
- Add `update` override to update from another dictionary
- docs: Add Developer notes page
- docs: Add Usage page
- examples: Add delta compression example
- examples: Add example for dictionary difference function

### Fixed

- docs: Clean up references to former `extend` function
- docs: Correct internal references

## [2.3.2] - 2025-05-08

### Changed

- Update Eigen workspace rule to a git-repository rather than http-archive one

## [2.3.1] - 2025-05-08

### Changed

- Roll back Eigen dependency to version 3.3.9 and document why

## [2.3.0] - 2025-04-17

### Changed

- CICD: Update checkout action to v4
- Update Eigen dependency to version 3.4.0

## [2.2.1] - 2024-11-18

### Added

- docs: Add a Makefile for local use

### Changed

- CICD: Update documentation workflow

## [2.2.0] - 2024-08-08

### Added

- CICD: Documentation workflow
- Unit test for one-dimensional vector deserialization

### Changed

- Always check types when deserializing
- docs: Don't show include files

## [2.1.0] - 2024-05-24

### Added

- CICD: Add changelog check
- CICD: Unit tests for vectors and vectors-of-vectors serialization
- Serialization of ``std::vector<Eigen::VectorXd>``
- Serialization of ``std::vector<double>`` deserializing to ``Eigen::VectorXd``
- Type check arrays at deserialization based on first element
- WIP: Serialization of ``std::vector``'s of other Eigen types
- Writer for vectors of strings

### Changed

- Raise a TypeError when trying to serialize an unknown type

## [2.0.0] - 2023-10-09

### Breaking changes

- Remove `extend` function
- Remove `insert_initializer` function
- The `update` function now inserts keys that are not already present

### Added

- Bazel: Find clang-format on various operating systems

### Changed

- Add underscore suffix to private functions
- Bazel: Separate coverage, linting and testing jobs
- Bazel: Update Bazelisk script
- Compile in optimized rather than fast-build mode by default
- Remove Makefile from examples directory

## [1.1.0] - 2022-10-04

### Added

- CMake workflow alongside Bazel in CI
- Lint-only test config
- Report unit test coverage
- Unit tests for ``mpack::Writer``
- Unit tests for ``mpack::read`` functions

### Fixed

- Bazel: Label ``build_file`` attributes for downstream dependencies

## [1.0.0] - 2022-04-25

This is the initial release of palimpsest, a small C++ library that provides a ``Dictionary`` type meant for fast value updates and serialization. It is called palimpsest because these dictionaries are designed for frequent rewritings (values change fast) on the same support (keys change slow).

[unreleased]: https://github.com/qpsolvers/qpsolvers/compare/v2.4.0...HEAD
[2.4.0]: https://github.com/qpsolvers/qpsolvers/compare/v2.3.2...v2.4.0
[2.3.2]: https://github.com/qpsolvers/qpsolvers/compare/v2.3.1...v2.3.2
[2.3.1]: https://github.com/qpsolvers/qpsolvers/compare/v2.3.0...v2.3.1
[2.3.0]: https://github.com/qpsolvers/qpsolvers/compare/v2.2.1...v2.3.0
[2.2.1]: https://github.com/qpsolvers/qpsolvers/compare/v2.2.0...v2.2.1
[2.2.0]: https://github.com/qpsolvers/qpsolvers/compare/v2.1.0...v2.2.0
[2.1.0]: https://github.com/qpsolvers/qpsolvers/compare/v2.0.0...v2.1.0
[2.0.0]: https://github.com/qpsolvers/qpsolvers/compare/v1.1.0...v2.0.0
[1.1.0]: https://github.com/qpsolvers/qpsolvers/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/qpsolvers/qpsolvers/releases/tag/v1.0.0
