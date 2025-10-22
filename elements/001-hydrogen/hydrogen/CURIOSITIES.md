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
  - [.lintignore-bash](.lintignore-bash) - options for shellcheck used by [Test 92: xhellcheck](tests/docs/test_92_shellcheck.md)
  - [.lintignore-c](.lintignore-c) - options for cppcheck used by [Test 91: cppcheck](tests/docs/test_91_cppcheck.md)
  - [.lintignore-lua](.lintignore-lua) - options for luacheck used by [Test 38: luacheck](tests/docs/test_38_luacheck.md)
  - [.lintignore-markdown](.lintignore-markdown) - options for markdownlint used by [Test 90: markdownlint](tests/docs/test_90_markdownlint.md)
  - [.sqruff_db2](.sqruff-db2) - DB2 options for sqruff (SQL lint tool) used by [Test 31: Migrations](tests/docs/test_31_migrations.md)
  - [.sqruff_mysql](.sqruff-mysql) - MySQL/MariaDB options for sqruff (SQL lint tool) used by [Test 31: migrations](tests/docs/test_31_migrations.md)
  - [.sqruff_postgresql](.sqruff-postgresql) - PostgreSQL options for sqruff (SQL lint tool) used by [Test 31: Migrations](tests/docs/test_31_migrations.md)
  - [.sqruff_sqlite](.sqruff-sqlite) - SQLite options for sqruff (SQL lint tool) used by [Test 31: Migrations](tests/docs/test_31_migrations.md)
  - [.trial-ignore](.trial-ignore) - used to indicate C files that we don't want lined into the final executables
  
Generally speaking, nothing else should be in this folder. Sometimes during a build or test of some
kind, core dumps or temp files will land here, but those are not included.

## Tests

A big part of this project revolves around tests. There are broadly two sets of tests.

- **Blackbox Tests** are a collection of about 40 Bash scripts used to run the Hydrogen server and test its capabilities using things like curl,
or by looking at its output logs for expected behaviours. Sometimes known as integration tests, these largely focus on "happy paths" - ensuring
that stuff we expect to work actually works, but not so much of a focus on edge cases. These all live in the`tests` foldler, with `test_00_all.sh`
being the main orchestrator that runs the other tests.  An enormous amount of effort has gone into ensuring these tests all run extremely quickly.
- **Unit Tests** are a collection of well over 4,000 individual Unity Framework unit tests spread across more than 500 different source files.
Far more testing source code than the project source code that it is testing, actually. These tests, being unit tests, are more concerned with edge
cases and proper inputs and outputs everywhere. They're designed to be very small and quick. And, like the Blackbox Tests, an even greater effort
has gone into ensuring that they all run quickly and efficiently.

In order to determine coverag (lines of code that are exercised by one or both tests), there are different builds of the project. Most notably,
a `hydrogen_coverage` build that has the instrumentation, and a special Unity build that records coverage from the Unity unit tests.  These results
are combined into a comprehensive coverage report for all of the C source files in src/ showing how many lines of coverage from each, and an
overall coverage number for each file. More on that in a bit. The main thing to point out here though is that these two test suites work in tandem
to get us our overall coverage data via very different means.But between them, we can get useful tests and a good deal more covedage than either
approach provides separately.

## Build System

Our current build environment uses cmake and gcc, typically being developed on Fedora 41 and occasionally Ubuntu 24 and other similar platforms.
Some work was put into a native macOS port until it became clear how much work was going to be involved, versus just running it inside of Docker.
Which conveniently also addresses the Windows platform issue.

The Blackbox scripts are also serving double-duty has the tools to help with managing builds.  Specifically
[Test 01: Compilation](tests/docs/test_01_compilation.md) runs cmake to build absolutely everyting in one shot, including all the Unity unit test
executables. And [Test 10: Unity](tests/docs/test_10_unity.md) takes care of running the hundreds of Unity tests and collecting the results.
[Test 99: Coverage](tests/docs/test_99_coverage.md) tallies up everything and helps give us the final results.

## Linting

This is a big part of the project - using linting tools like cppcheck and shellcheck and many others to ensure that whatever files we create are in
fact valid formats or follow sensible conventions. Sensible is of course impossible to quantify. For cppcheck and shellcheck in particular, we've
created our project from nothing so it was no problem at all to just "go" with the default styles and enforce all those pesky settings. This would be
considerably less fun to impose on an existing codebase, without a doubt. Or if there wasn't a handy AI agent to go and change all the if statements from
using `[ condition ]` to using `[[ condition ]]`. Having said that, there are overrides in some places for some things, like enforcing 80-char
line lengths anywhere is not something that is enforced. Because it is not 1978. Many of these linting tools also support inline exceptions and those
appear with some frequency as well. In the case of shellcheck in particular, [Test 92: xhellcheck](tests/docs/test_92_shellcheck.md) also goes the
extra mile to check that some effort has been made to include a justification for why an exception has been added, to try and limit the behaviour.

One of the more recent additions in this arena is the use of `sqruff` to essentially serve as a linting tool for SQL. This is a terrible thing to do
most of the time, but we're looking primarily for syntax errors that will stop our database migrations, more so than styling issues. Particularly as
our SQL code is pushed through a Lua-based renderer to get us our multi-engine support. But different engines are infamous for having different style
requirements (keywords needing to be uppercase, for example). So we're onboard with the idea generally but not at all hesitant to add exceptions to
the sqruff configurations to get everyone to play nice together.

## Key Reports

Each time [Test 01: Compilation](tests/docs/test_01_compilation.md) runs, a suite of "reports" is generated and output. There are four in particular
worth paying attention to.

- [Test Suite Results](images/COMPLETE.svg) provides the top-level overview, showing all of the Blackbox results in a nicely formatted and organized
table. Ideally we'd have no failing tests here, and the whole test suite would complete in around one minute, or far less if it has been run recently,
as it parallelizes and caches almost everything it possibly can. This also provides key coverage statistics like what our overall coverage is for
the project and the breakdown of Blackbox vs. Unity coverage. This table is available only when running
[Test 01: Compilation](tests/docs/test_01_compilation.md).
- [Test Suite Coverage](images/COVERAGE.svg) provides a more detailed analysis file-by-file, showing how many lines of code, Unity tests, Unity
coverage, Blackbox coverage, and so on for each C file in the project that has instrumented code.  Currently a little more than 200 files, with
more than 4,000 individual tests across well over 20,000 lines of instrumented code.  This table can be generated separately by running the
[Coverage Table](tests/docs/coverage_table.md) script. This uses data from the most recent build.
- [Cloc Code](images/CLOC_CODE.svg) uses the common `cloc` program to count the number of lines of code for the project, broken down by filetypes
and separating out comments and blank lines and so on. This gives a reasonable overview of the size and scale of the project, and some insight into
its complexity, that makes it a little easier to compare to other projects. You could upload this to an AI model for example and ask how it compares
to something like Klipper or SQLite or another open source project, and it will happily oblige. This table and the following table can be generated
by runining the [Cloc Tables](tests/docs/cloc_tables.md) command. As with the Test Suite Coverage, this is typically useful only after performing a
full build.
- [Cloc Stats](images/CLOC_STAT.svg) takes the `cloc` data a step further, combining it with data from our coverage calculations to give an extended
view of the project, showing how we're doing in terms of comments versus code, or Unity versus Blackbox coverage in our tests.

All of these are SVG files generated using `tables` for the formatting, and `Oh.sh` to convert ANSI terminal output into SVG. Both of those are
projects created by the same developer that created this Hydrogen project.

## Subsystems

In order to get a project of this scale to be manageable, the individual components inside the Hydrogen server are referred to as subsystems and
are treated as largely independent of one another, often configured to run in their own threads, use their own queues, and so on. In order to
manage them, we've set up a `Launch` and `Landing` mechansim for each. Launch((and Landing) involves following a set subsystem sequence and
passing a series of Go/No-Go checks for each. This allows us the opportunity to check our configuration options and other depenencies while still
continuing forward despite whatever environment we find ourselves in. For example, we want "launch" the Swagger subsystem if we don't have what
we need to "launch" the webserver. That sort of thing. Here are the subsystems currently being managed in our code.
