# Hydrogen Project Curiosities and Conventions

In this document we'll cover some project aspects that might be a little different or unusual or just
worth noting to help out those new to the project. This is aimed squarely at developers wanting to
either understand the project for their own deployments, or for those wanting to contribute or make
changes to the code. Nothing remarkably innovative here, just a bit of grounding to help everyone out.

## Structure

The root for the Hydrogen Project is `elements/001-hydrogen/hydrogen`. There are other side-projects
in the parent folder - [deuterium](../deuterium/README.md) and [tritium](elements/001-hydrogen/tritium/README.md), specifically, but they have their own README.md files
you can have a look at - they are not yet directly part of this project.

This folder contains four basic things.

- Top-level documents, like this one.
- Build executables like `hydrogen`, `hydrogen_release`, and `hydrogen_coverage`.
- Project folders
  - `build` contains build artifacts - temporary object files gcov files
  - `cmake` CMakeLists.txt lives here, along with more than a dozen included `.cmake` files
  - `configs` has sample configuration.json files for Hydrogen
  - `docs` is where you'll find most of the documentation, but there is also `tests/docs`
  - `examples` has code for writing clients to talk to Hydrogen
  - `extras` has various little helper tools that have come in handy at some point or other
  - `images` are mostly SVG files generated as part of the build, including database diagrams
  - `installer` is where you'll find the Bash installer scripts and docker images
  - `payloads` contains the code for building a payload - more on that below
  - `src` is the main C source for the project
  - `tests` contain the Bash scripts that implement our blackbox tests
  - `tests\unity` contain the Unity Framework source for all of the unit tests
- Hidden configuration files
  - [.gitignore](.gitignore) - used to determine what doesn't get automatically synced to GitHub
  - [.lintignore](.lintignore) - used by all the lint tests to skip files for various reasons
  - [.lintignore-bash](.lintignore-bash) - options for shellcheck used by [Test 92](tests/docs/test_92_shellcheck.md)

Generally speaking, nothing else should be in this folder. Sometimes during a build or test of some
kind, core dumps or temp files will land here, but those are not included.
