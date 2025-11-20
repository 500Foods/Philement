# test_98_markdownlint.md

## Overview

The `test_98_markdownlint.sh` script validates Markdown files using markdownlint to ensure consistent documentation formatting, structure, and adherence to best practices.

## Purpose

This test ensures documentation quality by:

- Analyzing all Markdown files in the project documentation
- Enforcing consistent formatting and structure
- Validating proper heading hierarchy and organization
- Ensuring link validity and proper syntax
- Maintaining documentation standards across the project

## Test Categories

### Documentation Validation

- **Syntax Checking**: Ensures valid Markdown syntax
- **Structure Validation**: Verifies proper heading hierarchy (H1 → H2 → H3)
- **Formatting Consistency**: Enforces consistent list, code block, and emphasis formatting
- **Link Validation**: Checks for proper link syntax and structure
- **Best Practices**: Ensures adherence to documentation standards

## Configuration

The test uses:

- `.lintignore-markdown` for project-specific markdown rules
- Standard markdownlint configuration with customizations for technical documentation
- Ignores generated documentation where appropriate

## Files Validated

The test processes Markdown files in:

- Root documentation (README.md, INSTRUCTIONS.md, etc.)
- `docs/` - Comprehensive project documentation
- `tests/docs/` - Test framework documentation
- Example and tutorial markdown files

## Expected Outcomes

- **Pass**: All Markdown files pass formatting and structure validation
- **Fail**: Syntax errors or significant formatting issues found
- **Warning**: Minor style issues that should be addressed

## Integration

This test is part of the static analysis suite (90+ range) and runs automatically with `test_00_all.sh`.