# test_93_shellcheck.md

## Overview

The `test_93_shellcheck.sh` script validates shell scripts using ShellCheck to identify potential bugs, portability issues, and coding standard violations.

## Purpose

This test ensures shell script quality by:

- Analyzing all shell scripts (.sh files) in the project
- Detecting common shell scripting errors and pitfalls
- Identifying portability issues across different shells
- Validating quoting and variable usage patterns
- Checking for proper error handling

## Test Categories

### Script Analysis

- **Syntax Errors**: Detects malformed shell constructs
- **Logic Issues**: Identifies unreachable code and incorrect conditionals
- **Portability**: Flags bash-specific features when POSIX compliance is expected
- **Security**: Highlights potential command injection vulnerabilities
- **Best Practices**: Ensures proper quoting and variable handling

## Configuration

The test respects:

- ShellCheck directive comments in scripts (e.g., `# shellcheck disable=SC2034`)
- Exception justification checks to ensure disabled warnings are documented
- Project-specific ignore patterns for acceptable deviations

## Expected Outcomes

- **Pass**: All shell scripts pass ShellCheck validation
- **Fail**: Critical errors or unjustified warnings found
- **Info**: Reports on disabled checks and their justifications

## Integration

This test is part of the static analysis suite (90+ range) and runs automatically with `test_00_all.sh`.