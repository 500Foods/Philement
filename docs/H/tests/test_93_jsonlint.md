# test_93_jsonlint.md

## Overview

The `test_93_jsonlint.sh` script validates JSON files using both basic syntax checking and advanced JSON Schema validation. This test ensures proper syntax, structure, and compliance with Hydrogen configuration schemas.

## Purpose

This test ensures JSON file integrity through two validation layers:

- **Basic JSON Linting**: Validates syntax of all JSON files in the project
- **JSON Schema Validation**: Validates Hydrogen configuration files against formal schema definitions
- **Configuration Validation**: Ensures test and application config files follow defined schemas
- **Error Detection**: Detects malformed JSON and schema violations that could cause runtime errors

## Test Categories

### Basic JSON Validation

- **Syntax Checking**: Ensures valid JSON syntax and structure using jq
- **Configuration Validation**: Verifies test and application config files
- **Encoding Validation**: Ensures proper UTF-8 encoding
- **Format Consistency**: Validates consistent formatting and indentation

### JSON Schema Validation (Added 2026-01-07)

- **Schema Compliance**: Validates Hydrogen config files against [`hydrogen_config_schema.json`](elements/001-hydrogen/hydrogen/tests/artifacts/hydrogen_config_schema.json)
- **Structural Validation**: Ensures required fields, data types, and constraints are met
- **Environment Variable Support**: Validates configuration files with environment variable references
- **Error Reporting**: Provides detailed validation error messages for schema violations

## Files Validated

The test checks JSON files in two phases:

### Phase 1: Basic JSON Linting
- All `.json` files in the project (excluding intentionally invalid test files)
- Configuration files in `configs/` directories
- Test configuration files in `tests/configs/`
- Payload definitions and data files

### Phase 2: JSON Schema Validation
- Hydrogen configuration files in `tests/configs/` directory
- Files matching pattern `hydrogen_test_*.json`
- All files validated against the comprehensive Hydrogen configuration schema

## Requirements

- **jq**: Required for basic JSON syntax validation
- **jsonschema-cli**: Required for JSON Schema validation (optional but recommended)
- **Hydrogen Config Schema**: [`hydrogen_config_schema.json`](elements/001-hydrogen/hydrogen/tests/artifacts/hydrogen_config_schema.json)

## Expected Outcomes

- **Pass**: All JSON files are syntactically valid and schema-compliant
- **Fail**: Syntax errors, malformed JSON, or schema violations detected
- **Skip**: Schema validation skipped if jsonschema-cli not available
- **Info**: Reports on files processed, validation results, and schema compliance

## Integration

This test is part of the static analysis suite (90+ range) and runs automatically with `test_00_all.sh`. The test provides both basic JSON validation and advanced schema validation when jsonschema-cli is available.

## Version History

- **3.2.0** (2026-01-07): Added JSON Schema validation for Hydrogen config files
- **3.1.0** (2025-08-13): Reviewed and optimized test framework integration
- **3.0.0** (2025-07-30): Major overhaul with modular test framework