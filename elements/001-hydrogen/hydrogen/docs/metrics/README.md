# Build Metrics Documentation

This directory contains daily metrics files that track the health and progress of the Hydrogen project over time. These metrics are automatically generated after each build and stored in files named `YYYY-MM/YYYY-MM-DD.txt`.

> **Important Note:** These metrics files contain ANSI-formatted text with colors and table formatting designed for terminal display. They are best viewed in the VS Code terminal or other ANSI-compatible terminals. Unfortunately, GitHub is incapable of properly displaying ANSI text in browser windows, so these files will appear as garbled text when viewed on GitHub. Always view them locally in a terminal for the proper formatted experience.

```example
cat docs/metrics/2025-09/2025-09-25.txt
```

## Purpose

The metrics system provides a way to monitor key aspects of the project's development and quality over time. By tracking these metrics daily, we can:

- Monitor code quality and testing reliability
- Track codebase growth and complexity
- Ensure consistent test coverage
- Identify trends in development velocity and quality
- Make data-driven decisions about project health

## The Four Tables

Each daily metrics file contains four tables that provide different perspectives on the project's current state:

### 1. Test Suite Results

This table shows the results of running the complete test suite, which includes over 30 individual tests covering compilation, functionality, security, and code quality.

**Learn more:** See [Test Suite Orchestrator](../../tests/docs/test_00_all.md) for details on how these results are generated and coordinated.

**What it tells us:**

- How many tests passed vs. failed
- Which specific tests are working or need attention
- How long tests take to run (performance indicator)
- Overall reliability of the system

**Key columns:**

- **Test #**: Unique identifier for each test
- **Test Name**: Description of what the test validates
- **Version**: Current version of the test
- **Tests**: Number of sub-tests within each test
- **Pass/Fail**: Success rates
- **Duration**: Time taken to execute

### 2. Code Coverage

This table displays the percentage of code that is exercised by automated tests, broken down by test type.

**What it tells us:**

- How thoroughly the code is tested
- Whether new code additions include adequate test coverage
- Areas that might need additional testing

**Coverage types:**

- **Unity**: Unit tests that test individual functions in isolation
- **Blackbox**: Integration tests that test the system as a whole
- **Combined**: Overall coverage across all test types

There is a great deal of coverage overlap between the two testing strategies, and a great deal of effort has gone into ensuring accurate and precise counts for all project source files.

Unit tests have been created with the Unity Framework. They require about 100x the number of individual tests to get the equivalent coverage of a single blackbox test.The unit tests are grouped into executable files, roughly 10 individual tests per executable file. Unit testing overall is encompassed entirely within Test 10 of the main Test Suite.

Generally speaking, the blackbox tests are focused more on "does it work", sometimes referred to as "happy paths". Unit testing is more concerned with prodding edge cases and stressing various aspects of the implementation, but is limited due to the general low-level
nature of the code in the project, making unit testing more difficult than usual.

As a result, coverage is a bit less than what one might normally expect, with many examples
of having many more individual tests than the number of lines they actually cover, which is
highlighted in this table, just for comedic value mostly.

**Learn more:** See [Coverage Analysis System](../../tests/docs/coverage.md) for technical details on how coverage is calculated and the sophisticated architecture behind these measurements.

### 3. Code Size Analysis (CLOC Main)

This table uses the CLOC (Count Lines Of Code) tool to analyze the codebase and show how many lines of code exist in each programming language.

**What it tells us:**

- The size and composition of the codebase
- Which languages are most heavily used
- Growth trends in different parts of the system
- Complexity indicators (more lines often mean more complexity)

**Typical languages shown:**

- C (main application code)
- Shell scripts (build and test automation)
- Markdown (documentation)
- JSON (configuration files)
- Other supporting languages

### 4. Code Statistics (CLOC Extended)

This table provides additional statistical analysis of the codebase, including averages, totals, and distribution metrics.

**What it tells us:**

- Average file sizes and complexity
- Distribution of code across different file types
- Summary statistics for quick reference
- Historical comparison points

## How to Use These Metrics

### For Project Managers

- Look at the **Test Suite Results** to ensure the system is stable and all tests are passing
- Monitor **Code Coverage** percentages to ensure testing quality remains high
- Track **Code Size** growth to understand development velocity
- Use these metrics in status reports and planning discussions

### For Developers

- Check test results to identify failing tests that need attention
- Review coverage metrics to find areas needing more tests
- Monitor code size changes to understand the impact of new features
- Use historical data to predict timelines and resource needs

### For Quality Assurance

- Verify that coverage percentages meet project standards
- Ensure all critical tests are consistently passing
- Monitor for unexpected changes in code metrics that might indicate issues

This is of course primarily a C project (specifically not C++) so that is where the
bulk of the code should fall. All documentation is in markdown format, so there are
a considerable number of files there. Testing and general management is handled
through extensive Bash scripts, making up the bulk of what is being reported.

A special separation for Core code versus Test code has been made and, unlike cloc's
typical defaults, .c and .h files are combined in these reports for simplicity. Of
particular interest, we tend to ignore the Unity Framework unit tests in our ratios
related to productivity. Substantial effort has been put into the unit tests,
with the Test code exceeding the Core code by a significant margin.

However, virtually all of the Test code has been written with the help of an AI model (primarily Grok). This doesn't take anything away from the test code's validity or value, but might unfairly inflate the human contributions to the project. AI models were used in the core code as well, but to a lesser degree. It kind of balances out overall by separating them out this way and just deriving productivity information from the Core content.

**Learn more:** See [CLOC Analysis Library](../../tests/docs/cloc.md) for technical details on how code size analysis works and the sophisticated exclusion patterns used.

## File Organization

Metrics are stored in monthly directories under `docs/metrics/`:

```directory
docs/metrics/
├── 2025-09/
│   ├── 2025-09-01.txt
│   ├── 2025-09-02.txt
│   └── ...
└── 2025-10/
    └── ...
```

Each file contains the four tables as they existed on that specific date, allowing for historical analysis and trend identification.

## Technical Details

These metrics are generated automatically by Test 00 (the test suite orchestrator) after each complete build. The system ensures that metrics are only created when all tests have been run, providing an accurate snapshot of the project's state at that moment.

**Learn more:** See [Testing Framework](../../docs/testing.md) for the complete testing philosophy and approach that makes this metrics system possible.

## Related Documentation

For deeper understanding of how these metrics are generated and integrated into the build system:

- **[Test Suite Orchestrator](../../tests/docs/test_00_all.md)** - How Test 00 coordinates all testing and generates metrics
- **[CLOC Analysis Library](../../tests/docs/cloc.md)** - Technical details of the code size analysis system
- **[Coverage Analysis System](../../tests/docs/coverage.md)** - Comprehensive coverage calculation and reporting architecture
- **[Testing Framework](../../docs/testing.md)** - Overview of the complete testing approach and philosophy
- **[Test Framework Libraries](../../tests/docs/LIBRARIES.md)** - All modular testing libraries and their functions
- **[Blackbox Testing Guide](../../tests/TESTING.md)** - Primary documentation for blackbox/integration testing framework
- **[Unity Unit Testing](../../tests/TESTING_UNITY.md)** - Primary introduction to unit testing details and framework

These documents provide the technical foundation that makes the metrics system possible and show how deeply integrated metrics generation is with the overall build and testing infrastructure.