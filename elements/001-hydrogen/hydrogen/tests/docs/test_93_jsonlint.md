# test_97_jsonlint.md

## Overview

The `test_97_jsonlint.sh` script validates JSON files to ensure proper syntax, structure, and formatting across all configuration and data files in the project.

## Purpose

This test ensures JSON file integrity by:

- Validating syntax of all JSON files in the project
- Checking configuration files for proper structure
- Ensuring test configuration files are well-formed
- Validating JSON data files used by the application
- Detecting malformed JSON that could cause runtime errors

## Test Categories

### JSON Validation

- **Syntax Checking**: Ensures valid JSON syntax and structure
- **Configuration Validation**: Verifies test and application config files
- **Schema Compliance**: Checks that JSON follows expected structure patterns
- **Encoding Validation**: Ensures proper UTF-8 encoding
- **Format Consistency**: Validates consistent formatting and indentation

## Files Validated

The test checks JSON files in:

- `configs/` - Application configuration files
- `tests/configs/` - Test-specific configuration files  
- `payloads/` - JSON payload definitions
- Any other `.json` files in the project

## Expected Outcomes

- **Pass**: All JSON files are syntactically valid and well-formed
- **Fail**: Syntax errors or malformed JSON detected
- **Info**: Reports on files processed and validation results

## Integration

This test is part of the static analysis suite (90+ range) and runs automatically with `test_00_all.sh`.