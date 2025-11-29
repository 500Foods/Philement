# Hydrogen Project Curiosities and Conventions

In this document we'll cover some project aspects that might be a little different or unusual or just
worth noting to help out those new to the project. This is aimed squarely at developers wanting to
either understand the project for their own deployments, or for those wanting to contribute or make
changes to the code. Nothing remarkably innovative here, just a bit of grounding to help everyone out.

## Structure

The root for the Hydrogen Project is `elements/001-hydrogen/hydrogen`. There are other side-projects
in the hydrogen project folder - [deuterium](https://github.com/500Foods/Philement/blob/main/elements/001-hydrogen/deuterium/README.md) and [tritium](https://github.com/500Foods/Philement/blob/main/elements/001-hydrogen/tritium/README.md), specifically, but they have their own README.md files
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
  - [.gitignore](../.gitignore) - used to determine what doesn't get automatically synced to GitHub
  - [.lintignore](../.lintignore) - used by all the lint tests to skip files for various reasons
  - [.lintignore-bash](../.lintignore-bash) - options for shellcheck used by [Test 92: xhellcheck](../tests/docs/test_92_shellcheck.md)
  - [.lintignore-c](../.lintignore-c) - options for cppcheck used by [Test 91: cppcheck](../tests/docs/test_91_cppcheck.md)
  - [.lintignore-lua](../.lintignore-lua) - options for luacheck used by [Test 38: luacheck](../tests/docs/test_38_luacheck.md)
  - [.lintignore-markdown](../.lintignore-markdown) - options for markdownlint used by [Test 90: markdownlint](../tests/docs/test_90_markdownlint.md)
  - [.sqruff_db2](../.sqruff_db2) - DB2 options for sqruff (SQL lint tool) used by [Test 31: Migrations](../tests/docs/test_31_migrations.md)
  - [.sqruff_mysql](../.sqruff_mysql) - MySQL/MariaDB options for sqruff (SQL lint tool) used by [Test 31: migrations](../tests/docs/test_31_migrations.md)
  - [.sqruff_postgresql](../.sqruff_postgresql) - PostgreSQL options for sqruff (SQL lint tool) used by [Test 31: Migrations](../tests/docs/test_31_migrations.md)
  - [.sqruff_sqlite](../.sqruff_sqlite) - SQLite options for sqruff (SQL lint tool) used by [Test 31: Migrations](../tests/docs/test_31_migrations.md)
  - [.trial-ignore](../.trial-ignore) - used to indicate C files that we don't want lined into the final executables
  
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

The objective for unit tests is to have coverage for every source file except for hydrogen.c - excepted due to how unit tests work. For source
files with less than 100 instrumented lines of code, we expect a minimum of 50% coverage. For files with more than 100 instrumented lines of code
we expect a minimum of 75% coverage. Test 99 also highlights source files with more than 1,000 lines of code (instrumented or not).

Sometimes coverage is hard to obtain due to the nature of system level calls and so on. To get around that, three strategies are used. First,
larger functions are split up using helper functions, where the helper functions are easier to test. Second, in more difficult situations,
larger source files are split up into smaller files, sometimes less than 100 instrumented lines, to get in under the limit. Most of the time
just using helper functions gets the job done. The third strategy is the liberal use of mock libraries to simulate the lower level system
calls (malloc being the biggest culprit) and allow the tests to proceed. This is considerably more complex, but also considerably more
powerful when it comes to implementing useful tests. Details can be found in [Mocks](../tests/unity/mocks/README.md).

## Build System

Our current build environment uses cmake and gcc, typically being developed on Fedora 41 and occasionally Ubuntu 24 and other similar platforms.
Some work was put into a native macOS port until it became clear how much work was going to be involved, versus just running it inside of Docker.
Which conveniently also addresses the Windows platform issue.

The Blackbox scripts are also serving double-duty has the tools to help with managing builds.  Specifically
[Test 01: Compilation](../tests/docs/test_01_compilation.md) runs cmake to build absolutely everyting in one shot, including all the Unity unit test
executables. And [Test 10: Unity](../tests/docs/test_10_unity.md) takes care of running the hundreds of Unity tests and collecting the results.
[Test 99: Coverage](../tests/docs/test_99_coverage.md) tallies up everything and helps give us the final results.

## Linting

This is a big part of the project - using linting tools like cppcheck and shellcheck and many others to ensure that whatever files we create are in
fact valid formats or follow sensible conventions. Sensible is of course impossible to quantify. For cppcheck and shellcheck in particular, we've
created our project from nothing so it was no problem at all to just "go" with the default styles and enforce all those pesky settings. This would be
considerably less fun to impose on an existing codebase, without a doubt. Or if there wasn't a handy AI agent to go and change all the if statements from
using `[ condition ]` to using `[[ condition ]]`. Having said that, there are overrides in some places for some things, like enforcing 80-char
line lengths anywhere is not something that is enforced. Because it is not 1978. Many of these linting tools also support inline exceptions and those
appear with some frequency as well. In the case of shellcheck in particular, [Test 92: xhellcheck](../tests/docs/test_92_shellcheck.md) also goes the
extra mile to check that some effort has been made to include a justification for why an exception has been added, to try and limit the behaviour.

One of the more recent additions in this arena is the use of `sqruff` to essentially serve as a linting tool for SQL. This is a terrible thing to do
most of the time, but we're looking primarily for syntax errors that will stop our database migrations, more so than styling issues. Particularly as
our SQL code is pushed through a Lua-based renderer to get us our multi-engine support. But different engines are infamous for having different style
requirements (keywords needing to be uppercase, for example). So we're onboard with the idea generally but not at all hesitant to add exceptions to
the sqruff configurations to get everyone to play nice together.

## Key Reports

Each time [Test 01: Compilation](../tests/docs/test_01_compilation.md) runs, a suite of "reports" is generated and output. There are four in particular
worth paying attention to.

- [Test Suite Results](../images/COMPLETE.svg) provides the top-level overview, showing all of the Blackbox results in a nicely formatted and organized
table. Ideally we'd have no failing tests here, and the whole test suite would complete in around one minute, or far less if it has been run recently,
as it parallelizes and caches almost everything it possibly can. This also provides key coverage statistics like what our overall coverage is for
the project and the breakdown of Blackbox vs. Unity coverage. This table is available only when running
[Test 01: Compilation](../tests/docs/test_01_compilation.md).
- [Test Suite Coverage](../images/COVERAGE.svg) provides a more detailed analysis file-by-file, showing how many lines of code, Unity tests, Unity
coverage, Blackbox coverage, and so on for each C file in the project that has instrumented code.  Currently a little more than 200 files, with
more than 4,000 individual tests across well over 20,000 lines of instrumented code.  This table can be generated separately by running the
[Coverage Table](../tests/docs/coverage_table.md) script. This uses data from the most recent build.
- [Cloc Code](../images/CLOC_CODE.svg) uses the common `cloc` program to count the number of lines of code for the project, broken down by filetypes
and separating out comments and blank lines and so on. This gives a reasonable overview of the size and scale of the project, and some insight into
its complexity, that makes it a little easier to compare to other projects. You could upload this to an AI model for example and ask how it compares
to something like Klipper or SQLite or another open source project, and it will happily oblige. This table and the following table can be generated
by runining the [Cloc Tables](../tests/docs/cloc_tables.md) command. As with the Test Suite Coverage, this is typically useful only after performing a
full build.
- [Cloc Stats](../images/CLOC_STAT.svg) takes the `cloc` data a step further, combining it with data from our coverage calculations to give an extended
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

## Design Goals

There are some curiosities in the design that are worth mentioning - things that might go against what one might expect.

**SQL.** Naturally, SQL makes up a big part of the project given that the main funcitonality we're building is a REST API server that is a front-end to
a variety of databases. HOWEVER, it is important to note that there is almost zero SQL embedded in the source code anywhere. Maybe a bit in the part
related to database heartbeat checks, but generally our code is completely free of SQL. This is because we want, as much as possible, to have the
database and its contents, queries, and so on be entirely customizable externally, by clients or others, so that our server can be an "atomic" tool.

This means that we can supply a suite of Migrations (Helium project) to the system and it will happily trundle along and apply them regardless of their
contents, database engine, and so on. We do have *some* queries that we rely upon, for login and so forth, which are essentially numbered queries in
a table specified via the bootstrap query in the configuraiton file. These can be altered, by clients or others, to reference different tables or
columns so long as they return what we need to get the job done (which is described in detail in the individual queries). This means that someone
writing an API of some kind could implement their entire system separately and use the same infrastructure, so long as they buy into the same ideas.

## Migrations

As is typical for systems that use databases and queries, a "migration" system is needed to create and manage a database schema and its contents.
These can often be problematic if not maintained in a friendly way (looking at YOU, Canvas LMS by Instructure). We've gone to some effort to make
this a migration system loved by everyone (or equally loathed, either way is fine). The basic idea is that we are using Lua scripts as wrappers
around SQL code. Database-engine-specific code can be used if desired. Or, better, a set of comprehensive macros can be used to write SQL that is
engine-agnostic with just engine-specific code in special cases.

The choice of Lua is kind of fun in particular as it expertly side-steps the issue of quote nesting. It has its own unique mechanism for having
multiline strings wrapped in [[string]], [=[string]=], [==[stirng]==] and so on. This means we can write the SQL or CSS or JSON *exactly* the way
we want to write it, and not have to think much about how it gets from the migration file into the actual database. Internally, we take these
multiline strings and compress them with brotli, encode them with base64, and send them along to the database which deposits them without any
kind of futzing about along the way.

Each migration script is therefore focused largely on a single thing - creating a table, populating a lookup, supplying a CSS theme, and so on.
Each migration script contains both a forward migration (changing the database schema, adding a record, etc.) and a reverse migration where the
change is undone. Hydrogen has a "Test Migration" mode where it first applies all the forward migrations and then applies all the reverse
migrations. Leaving the system with a database that has not been changed from the original state. This helps ensure that our migrations are doing
the job they are supposed to be doing, and across all the supported database engines. Consistently. At scale. Handy.

Each migration script that changes the schema also has a diagram migration. This is basically a bit of JSON that describles a table and its
contents. These aree then collected up across all the migrations to generate a database diagram via Test 39.
