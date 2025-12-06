# test_94_eslint.md

## Overview

The `test_94_eslint.sh` script validates JavaScript files using ESLint to identify potential bugs, enforce coding standards, and maintain code quality.

## Purpose

This test ensures JavaScript code quality by:

- Analyzing JavaScript files in the project (primarily in examples/ and payloads/)
- Detecting syntax errors and potential runtime issues
- Enforcing consistent coding style and conventions
- Identifying unused variables and unreachable code
- Validating proper use of modern JavaScript features

## Test Categories

### Code Quality Checks

- **Syntax Validation**: Ensures valid JavaScript syntax
- **Error Detection**: Identifies potential runtime errors
- **Style Consistency**: Enforces consistent formatting and naming
- **Best Practices**: Promotes secure and maintainable code patterns
- **ES6+ Features**: Validates proper use of modern JavaScript

## Configuration

The test uses:

- Standard ESLint configuration for JavaScript validation
- Project-specific rules for the Hydrogen codebase context
- Ignores generated or third-party JavaScript files

## Expected Outcomes

- **Pass**: All JavaScript files pass ESLint validation
- **Fail**: Syntax errors or critical issues found
- **Warning**: Style issues that should be addressed

## Integration

This test is part of the static analysis suite (90+ range) and runs automatically with `test_00_all.sh`.