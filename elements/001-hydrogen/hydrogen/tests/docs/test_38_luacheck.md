# Test 38: Lua Code Analysis (luacheck)

## Overview

Test 38 performs static analysis on Lua source files using the `luacheck` tool. This test ensures code quality and consistency in Lua scripts used for database migrations and other purposes in the Hydrogen project.

## Purpose

- **Static Analysis**: Detects potential issues in Lua code before runtime
- **Code Quality**: Enforces consistent coding standards and best practices
- **Error Prevention**: Catches common mistakes like undefined variables, unused code, and syntax issues

## Test Configuration

- **Test Name**: Lua Lint
- **Test Abbreviation**: LUA
- **Test Number**: 38
- **Version**: 1.0.0

## What It Does

1. **File Discovery**: Scans the project for `.lua` files
2. **Exclusion Processing**: Applies `.lintignore` patterns to exclude irrelevant files
3. **Configuration Loading**: Reads `.lintignore-lua` for luacheck-specific settings
4. **Analysis Execution**: Runs luacheck with appropriate parameters
5. **Result Reporting**: Reports any issues found with detailed error messages

## Configuration Files

### .lintignore-lua

Contains luacheck configuration options:

```bash
std=lua51
max-line-length=120
max-cyclomatic-complexity=15
globals=ngx,redis,json
ignore=211,212,213
```

### .lintignore

Standard exclusion patterns for files that shouldn't be analyzed.

## Test Output

The test provides:

- **File Count**: Number of Lua files analyzed
- **Cache Statistics**: Files processed vs. cached results
- **Issue Details**: Specific problems found with file:line:column format
- **Success/Failure**: Clear pass/fail status

## Dependencies

- **luacheck**: Lua static analysis tool
- **Project Structure**: Access to Lua files in the project
- **Configuration Files**: `.lintignore` and `.lintignore-lua`

## Common Issues Detected

- **Undefined Variables**: Variables used without declaration
- **Unused Variables**: Declared but never used variables
- **Global Variables**: Accidental use of global scope
- **Code Complexity**: Functions that are too complex
- **Line Length**: Lines exceeding maximum length limits

## Integration

This test is part of the Hydrogen testing framework and runs automatically with:

```bash
# Run this specific test
./tests/test_38_luacheck.sh

# Run as part of full test suite
./tests/test_00_all.sh
```

## Related Documentation

- [Test Framework Overview](../TESTING.md)
- [Static Analysis Tests](../docs/test_90_markdownlint.md)
- [Shell Script Linting](../docs/test_92_shellcheck.md)